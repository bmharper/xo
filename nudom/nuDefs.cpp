#include "pch.h"
#include "nuDefs.h"
#include "nuProcessor.h"
#include "nuRenderGL.h"
#include "nuDoc.h"
#include "nuSysWnd.h"

static nuStyle*						DefaultTagStyles[nuTagEND];
static volatile uint32				ExitSignalled = 0;
static int							InitializeCount = 0;
//static TAbcQueue<nuProcessor*>*		RenderQueue = NULL;

#if NU_WIN_DESKTOP
//static HANDLE						RenderThread = NULL;
static HANDLE						UIThread = NULL;
#endif

// Single globally accessible data
static nuGlobalStruct*				nuGlobals = NULL;

void nuBox::SetInt( int32 left, int32 top, int32 right, int32 bottom )
{
	Left = nuRealToPos((float) left);
	Top = nuRealToPos((float) top);
	Right = nuRealToPos((float) right);
	Bottom = nuRealToPos((float) bottom);
}

#if NU_WIN_DESKTOP

//DWORD WINAPI nuRenderThread( LPVOID tp )
//{
//	while ( true )
//	{
//		AbcSemaphoreWait( RenderQueue->SemaphoreObj(), INFINITE );
//		if ( ExitSignalled )
//			break;
//		nuProcessor* proc = NULL;
//		NUVERIFY( RenderQueue->PopTail( proc ) );
//		proc->Render();
//	}
//	return 0;
//}

DWORD WINAPI nuUIThread( LPVOID tp )
{
	while ( true )
	{
		AbcSemaphoreWait( nuGlobal()->EventQueue.SemaphoreObj(), INFINITE );
		if ( ExitSignalled )
			break;
		nuEvent ev;
		NUVERIFY( nuGlobal()->EventQueue.PopTail( ev ) );
		ev.Processor->ProcessEvent( ev );
	}
	return 0;
}

static void nuInitialize_Win32()
{
	//RenderQueue = new TAbcQueue<nuProcessor*>();
	//RenderQueue->Initialize( true );
	//RenderThread = CreateThread( NULL, 0, &nuRenderThread, NULL, 0, NULL );
	//UIThread = CreateThread( NULL, 0, &nuUIThread, NULL, 0, NULL );
	AbcVerify( AbcThreadCreate( &nuUIThread, NULL, UIThread ) );
}

static void nuShutdown_Win32()
{
	InterlockedExchange( &ExitSignalled,  1 );
	/*
	if ( RenderThread != NULL )
	{
		for ( uint waitNum = 0; true; waitNum++ )
		{
			RenderQueue->Add( NULL );
			if ( WaitForSingleObject( RenderThread, waitNum ) == WAIT_OBJECT_0 )
				break;
		}
		CloseHandle( RenderThread );
		RenderThread = NULL;
	}

	delete RenderQueue;
	RenderQueue = NULL;
	*/
	if ( UIThread != NULL )
	{
		nuGlobal()->EventQueue.Add( nuEvent() );
		for ( uint waitNum = 0; true; waitNum++ )
		{
			if ( WaitForSingleObject( UIThread, waitNum ) == WAIT_OBJECT_0 )
				break;
		}
		AbcThreadCreate( &nuUIThread, NULL, UIThread );
		UIThread = NULL;
	}
}

#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void nuInitializeDefaultTagStyles()
{
	// These definitions must follow the ordering of NU_TAGS_DEFINE

	auto make = [](const char* z) -> nuStyle* {
		nuStyle* s = new nuStyle();
		NUVERIFY(s->Parse(z));
		return s;
	};

	// NULL
	DefaultTagStyles[0] = new nuStyle();
	
	// body
	DefaultTagStyles[1] = make( "background: #fff; width: 100%; height: 100%;" );

	// div
	DefaultTagStyles[2] = make( "display: block;" );

	const nuStyleAttrib* bodybg = DefaultTagStyles[1]->Get( nuCatBackground );
	//NUTRACE( "body background: %x\n", bodybg ? bodybg->Color.u : 0xdeadbeef );

	static_assert(2 == nuTagEND - 1, "add default style for new tag");
}


NUAPI nuGlobalStruct* nuGlobal()
{
	return nuGlobals;
}

NUAPI void nuInitialize()
{
	InitializeCount++;
	if ( InitializeCount != 1 ) return;
	nuGlobals = new nuGlobalStruct();
	nuGlobals->TargetFPS = 60;
	nuGlobals->DocAddQueue.Initialize( false );
	nuGlobals->DocRemoveQueue.Initialize( false );
	nuGlobals->EventQueue.Initialize( true );
	nuInitializeDefaultTagStyles();
	nuSysWnd::PlatformInitialize();
#if NU_WIN_DESKTOP
	nuInitialize_Win32();
#endif
}

NUAPI void nuShutdown()
{
	NUASSERT(InitializeCount > 0);
	InitializeCount--;
	if ( InitializeCount != 0 ) return;

	for ( int i = 0; i < nuTagEND; i++ )
		delete DefaultTagStyles[i];

#if NU_WIN_DESKTOP
	nuShutdown_Win32();
#endif

	delete nuGlobals;
}

NUAPI nuStyle** nuDefaultTagStyles()
{
	return DefaultTagStyles;
}

//NUAPI void nuQueueRender( nuProcessor* proc )
//{
//	RenderQueue->Add( proc );
//}

NUAPI void nuParseFail( const char* msg, ... )
{
	char buff[1024] = "";
	va_list va;
	va_start( va, msg );
	uint r = vsnprintf( buff, arraysize(buff), msg, va );
	va_end( va );
	if ( r < arraysize(buff) )
		NUTRACE_WRITE(buff);
}

NUAPI void NUTRACE( const char* msg, ... )
{
	char buff[1024] = "";
	va_list va;
	va_start( va, msg );
	uint r = vsnprintf( buff, arraysize(buff), msg, va );
	va_end( va );
	if ( r < arraysize(buff) )
		NUTRACE_WRITE(buff);
}
