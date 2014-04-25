#include "pch.h"
#include "nuDefs.h"
#include "nuDocGroup.h"
#include "nuDoc.h"
#include "nuSysWnd.h"
#include "Render/nuRenderGL.h"
#include "Text/nuFontStore.h"
#include "Text/nuGlyphCache.h"

static const int					MAX_WORKER_THREADS = 32;
static volatile uint32				ExitSignalled = 0;
static int							InitializeCount = 0;
static nuStyle*						DefaultTagStyles[nuTagEND];
static AbcThreadHandle				WorkerThreads[MAX_WORKER_THREADS];

#if NU_PLATFORM_WIN_DESKTOP
static AbcThreadHandle				UIThread = NULL;
#endif

// Single globally accessible data
static nuGlobalStruct*				nuGlobals = NULL;

NUAPI size_t nuTexFormatChannelCount( nuTexFormat f )
{
	switch ( f )
	{
	case nuTexFormatInvalid:	return 0;
	case nuTexFormatRGBA8:		return 4;
	case nuTexFormatGrey8:		return 1;
	default:
		NUTODO;
		return 0;
	}
}

NUAPI size_t nuTexFormatBytesPerChannel( nuTexFormat f )
{
	switch ( f )
	{
	case nuTexFormatInvalid:	return 0;
	case nuTexFormatRGBA8:		return 1;
	case nuTexFormatGrey8:		return 1;
	default:
		NUTODO;
		return 0;
	}
}

NUAPI size_t nuTexFormatBytesPerPixel( nuTexFormat f )
{
	return nuTexFormatBytesPerChannel( f ) * nuTexFormatChannelCount( f );
}

void nuTexture::FlipVertical()
{
	byte sline[4096];
	byte* line = sline;
	size_t astride = std::abs(TexStride);
	if ( astride > sizeof(sline) )
		line = (byte*) AbcMallocOrDie( astride );
	for ( uint32 i = 0; i < TexHeight / 2; i++ )
	{
		memcpy( line, TexDataAtLine(TexHeight - i - 1), astride );
		memcpy( TexDataAtLine(TexHeight - i - 1), TexDataAtLine(i), astride );
		memcpy( TexDataAtLine(i), line, astride );
	}
	if ( line != sline )
		free( line );
}

void nuBox::SetInt( int32 left, int32 top, int32 right, int32 bottom )
{
	Left = nuRealToPos((float) left);
	Top = nuRealToPos((float) top);
	Right = nuRealToPos((float) right);
	Bottom = nuRealToPos((float) bottom);
}

void nuBox::ExpandToFit( const nuBox& expando )
{
	Left = std::min(Left, expando.Left);
	Top = std::min(Top, expando.Top);
	Right = std::max(Right, expando.Right);
	Bottom = std::max(Bottom, expando.Bottom);
}

void nuBox::ClampTo( const nuBox& clamp )
{
	Left = std::max(Left, clamp.Left);
	Top = std::max(Top, clamp.Top);
	Right = std::min(Right, clamp.Right);
	Bottom = std::min(Bottom, clamp.Bottom);
}

nuBox nuBox::ShrunkBy( const nuBox& margins )
{
	nuBox c = *this;
	c.Left += margins.Left;
	c.Right -= margins.Right;
	c.Top += margins.Top;
	c.Bottom -= margins.Bottom;
	return c;
}

static const float sRGB_Low	= 0.0031308f;
static const float sRGB_a	= 0.055f;

NUAPI float	nuSRGB2Linear( uint8 srgb )
{
	float g = srgb * (1.0f / 255.0f);
	if ( g <= 0.04045 )
		return g / 12.92f;
	else
		return pow( (g + sRGB_a) / (1.0f + sRGB_a), 2.4f );
}

NUAPI uint8	nuLinear2SRGB( float linear )
{
	float g;
	if ( linear <= sRGB_Low )
		g = 12.92f * linear;
	else
		g = (1.0f + sRGB_a) * pow(linear, 1.0f / 2.4f) - sRGB_a;
	return (uint8) nuRound( 255.0f * g );
}

void nuRenderStats::Reset()
{
	memset( this, 0, sizeof(*this) );
}

// add or remove documents that are queued for addition or removal
NUAPI void nuProcessDocQueue()
{
	nuDocGroup* p = NULL;

	while ( p = nuGlobal()->DocRemoveQueue.PopTailR() )
		erase_delete( nuGlobal()->Docs, nuGlobal()->Docs.find(p) );

	while ( p = nuGlobal()->DocAddQueue.PopTailR() )
		nuGlobal()->Docs += p;
}

AbcThreadReturnType AbcKernelCallbackDecl nuWorkerThreadFunc( void* threadContext )
{
	while ( true )
	{
		AbcSemaphoreWait( nuGlobal()->JobQueue.SemaphoreObj(), AbcINFINITE );
		if ( ExitSignalled )
			break;
		nuJob job;
		NUVERIFY( nuGlobal()->JobQueue.PopTail( job ) );
		job.JobFunc( job.JobData );
	}

	return 0;
}

#if NU_PLATFORM_WIN_DESKTOP

AbcThreadReturnType AbcKernelCallbackDecl nuUIThread( void* threadContext )
{
	while ( true )
	{
		AbcSemaphoreWait( nuGlobal()->EventQueue.SemaphoreObj(), INFINITE );
		if ( ExitSignalled )
			break;
		nuEvent ev;
		NUVERIFY( nuGlobal()->EventQueue.PopTail( ev ) );
		ev.DocGroup->ProcessEvent( ev );
	}
	return 0;
}

static void nuInitialize_Win32()
{
	NUVERIFY( AbcThreadCreate( &nuUIThread, NULL, UIThread ) );
}

static void nuShutdown_Win32()
{
	if ( UIThread != NULL )
	{
		nuGlobal()->EventQueue.Add( nuEvent() );
		for ( uint waitNum = 0; true; waitNum++ )
		{
			if ( WaitForSingleObject( UIThread, waitNum ) == WAIT_OBJECT_0 )
				break;
		}
		UIThread = NULL;
	}
}


#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NUAPI nuGlobalStruct* nuGlobal()
{
	return nuGlobals;
}

NUAPI void nuInitialize()
{
	InitializeCount++;
	if ( InitializeCount != 1 )
		return;

	AbcMachineInformation minf;
	AbcMachineInformationGet( minf );

	nuGlobals = new nuGlobalStruct();
	nuGlobals->TargetFPS = 60;
	nuGlobals->NumWorkerThreads = min( minf.LogicalCoreCount, MAX_WORKER_THREADS );
	nuGlobals->MaxSubpixelGlyphSize = 30;
	nuGlobals->PreferOpenGL = false;
	nuGlobals->EnableVSync = false;
	// Freetype's output is linear coverage percentage, so if we treat our freetype texture as GL_LUMINANCE
	// (and not GL_SLUMINANCE), and we use an sRGB framebuffer, then we get perfect results without
	// doing any tweaking to the freetype glyphs.
	// Setting SubPixelTextGamma = 2.2 will get results very close to default cleartype on Windows 8.1
	// As far as I know, leaving the gamma at 1.0 is "true to the font designer", but this does leave the fonts
	// quite a bit heavier than the default on Windows. For this reason, we set the gamma to 1.5. It seems like
	// a reasonable blend between the "correct weight" and "prior art".
	// CORRECTION. A gamma of anything other than 1.0 looks bad at small font sizes (like 12 or 13 pixels)
	// We might want to have a "gamma curve" of pixel size vs gamma.
	nuGlobals->SubPixelTextGamma = 1.0f;
	nuGlobals->WholePixelTextGamma = 1.0f;
#if NU_PLATFORM_WIN_DESKTOP
	nuGlobals->EnableSubpixelText = true;
	nuGlobals->EnableSRGBFramebuffer = true;
	//nuGlobals->EmulateGammaBlending = true;
#else
	nuGlobals->EnableSubpixelText = false;
	nuGlobals->EnableSRGBFramebuffer = false;
	//nuGlobals->EmulateGammaBlending = false;
#endif
	nuGlobals->EnableKerning = true;
	//nuGlobals->DebugZeroClonedChildList = true;
	nuGlobals->MaxTextureID = ~((nuTextureID) 0);
	nuGlobals->ClearColor.Set( 200, 0, 200, 255 );  // Make our clear color a very noticeable purple, so you know when you've screwed up the root node
	nuGlobals->DocAddQueue.Initialize( false );
	nuGlobals->DocRemoveQueue.Initialize( false );
	nuGlobals->EventQueue.Initialize( true );
	nuGlobals->JobQueue.Initialize( true );
	nuGlobals->FontStore = new nuFontStore();
	nuGlobals->FontStore->InitializeFreetype();
	nuGlobals->GlyphCache = new nuGlyphCache();
	nuSysWnd::PlatformInitialize();
#if NU_PLATFORM_WIN_DESKTOP
	nuInitialize_Win32();
#endif
	NUTRACE( "Using %d/%d processors.\n", (int) nuGlobals->NumWorkerThreads, (int) minf.LogicalCoreCount );
	for ( int i = 0; i < nuGlobals->NumWorkerThreads; i++ )
	{
		NUVERIFY( AbcThreadCreate( nuWorkerThreadFunc, NULL, WorkerThreads[i] ) );
	}
}

NUAPI void nuSurfaceLost()
{

}

NUAPI void nuShutdown()
{
	NUASSERT(InitializeCount > 0);
	InitializeCount--;
	if ( InitializeCount != 0 ) return;

	AbcInterlockedSet( &ExitSignalled, 1 );

	for ( int i = 0; i < nuTagEND; i++ )
		delete DefaultTagStyles[i];

	// allow documents scheduled for deletion to be deleted
	nuProcessDocQueue();

#if NU_PLATFORM_WIN_DESKTOP
	nuShutdown_Win32();
#endif

	// signal all threads to exit
	nuJob nullJob = nuJob();
	for ( int i = 0; i < nuGlobal()->NumWorkerThreads; i++ )
		nuGlobal()->JobQueue.Add( nullJob );

	// wait for each thread in turn
	for ( int i = 0; i < nuGlobal()->NumWorkerThreads; i++ )
		NUVERIFY( AbcThreadJoin( WorkerThreads[i] ) );

	nuGlobals->GlyphCache->Clear();
	delete nuGlobals->GlyphCache;
	nuGlobals->GlyphCache = NULL;

	nuGlobals->FontStore->Clear();
	nuGlobals->FontStore->ShutdownFreetype();
	delete nuGlobals->FontStore;
	nuGlobals->FontStore = NULL;

	delete nuGlobals;
}

NUAPI nuStyle** nuDefaultTagStyles()
{
	return DefaultTagStyles;
}

NUAPI void nuParseFail( const char* msg, ... )
{
	char buff[4096] = "";
	va_list va;
	va_start( va, msg );
	uint r = vsnprintf( buff, arraysize(buff), msg, va );
	va_end( va );
	if ( r < arraysize(buff) )
		NUTRACE_WRITE(buff);
}

NUAPI void NUTRACE( const char* msg, ... )
{
	char buff[4096] = "";
	va_list va;
	va_start( va, msg );
	uint r = vsnprintf( buff, arraysize(buff), msg, va );
	va_end( va );
	if ( r < arraysize(buff) )
		NUTRACE_WRITE(buff);
}

NUAPI void NUTIME( const char* msg, ... )
{
	const int timeChars = 16;
	char buff[4096] = "";
	sprintf( buff, "%-15.3f  ", AbcTimeAccurateRTSeconds() * 1000 );
	va_list va;
	va_start( va, msg );
	uint r = vsnprintf( buff + timeChars, arraysize(buff) - timeChars, msg, va );
	va_end( va );
	if ( r < arraysize(buff) )
		NUTRACE_WRITE(buff);
}
