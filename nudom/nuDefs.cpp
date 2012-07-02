#include "pch.h"
#include "nuDefs.h"
#include "nuProcessor.h"
#include "nuDoc.h"
#include "nuSysWnd.h"
#include "Render/nuRenderGL.h"

static const int					MAX_WORKER_THREADS = 32;
static volatile uint32				ExitSignalled = 0;
static int							InitializeCount = 0;
static nuStyle*						DefaultTagStyles[nuTagEND];
static AbcThreadHandle				WorkerThreads[MAX_WORKER_THREADS];

#if NU_WIN_DESKTOP
static AbcThreadHandle				UIThread = NULL;
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

// add or remove documents that are queued for addition or removal
static void ProcessDocQueue()
{
	nuProcessor* p = NULL;

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

#if NU_WIN_DESKTOP

AbcThreadReturnType AbcKernelCallbackDecl nuUIThread( void* threadContext )
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

#define MSGTRACE NUTRACE
//#define MSGTRACE __noop

void nuRunWin32MessageLoop()
{
	double lastFrameStart = AbcTimeAccurateRTSeconds();

	// Toggled when all renderers report that they have no further work to do (ie no animations playing, now or any time in the future)
	// In that case, the only way we can have something happen is if we have an incoming message.
	// Of course, this is NOT true for IO that is busy occuring on the UI thread.
	// We'll have to devise a way (probably just a process-global custom window message) of causing the main loop to wake up.
	bool renderIdle = false;

	while ( true )
	{
		// When idle, use GetMessage so that the OS can put us into a good sleep
		MSG msg;
		bool haveMsg = true;
		if ( renderIdle )
		{
			MSGTRACE( "Render cold\n" );
			if ( !GetMessage( &msg, NULL, 0, 0 ) )
				break;
			MSGTRACE( "GetMessage returned\n" );
		}
		else
		{
			MSGTRACE( "Render hot\n" );
			haveMsg = !!PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
		}
		
		if ( haveMsg )
		{
			MSGTRACE( "msg start: %x\n", msg.message );
			if ( msg.message == WM_QUIT )
				break;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
			MSGTRACE( "msg end: %x\n", msg.message );
		}

		double now = AbcTimeAccurateRTSeconds();
		double nextFrameStart = lastFrameStart + 1.0 / nuGlobal()->TargetFPS;
		if ( now >= nextFrameStart )
		{
			MSGTRACE( "Render enter\n" );
			renderIdle = true;
			lastFrameStart = now;
			for ( int i = 0; i < nuGlobal()->Docs.size(); i++ )
			{
				nuRenderResult rr = nuGlobal()->Docs[i]->Render();
				if ( rr != nuRenderResultIdle )
					renderIdle = false;
			}
		}
		else
		{
			MSGTRACE( "Render not due for another %.1f ms\n", nextFrameStart - now );
			if ( !renderIdle )
			{
				// It doesn't make sense to sleep for anything other than 0 here, unless our
				// target time-per-frame was significantly longer than a thread time slice.
				AbcSleep( 0 );
			}
		}

		ProcessDocQueue();
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

	AbcMachineInformation minf;
	AbcMachineInformationGet( minf );

	nuGlobals = new nuGlobalStruct();
	nuGlobals->TargetFPS = 60;
	nuGlobals->NumWorkerThreads = min( minf.CPUCount, MAX_WORKER_THREADS );
	nuGlobals->DocAddQueue.Initialize( false );
	nuGlobals->DocRemoveQueue.Initialize( false );
	nuGlobals->EventQueue.Initialize( true );
	nuGlobals->JobQueue.Initialize( true );
	nuInitializeDefaultTagStyles();
	nuSysWnd::PlatformInitialize();
#if NU_WIN_DESKTOP
	nuInitialize_Win32();
#endif
	NUTRACE( "Using %d/%d processors.\n", (int) nuGlobals->NumWorkerThreads, (int) minf.CPUCount );
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

#if NU_WIN_DESKTOP
	nuShutdown_Win32();
#endif

	// signal all threads to exit
	nuJob nullJob = nuJob();
	for ( int i = 0; i < nuGlobal()->NumWorkerThreads; i++ )
		nuGlobal()->JobQueue.Add( nullJob );

	// wait for each thread in turn
	for ( int i = 0; i < nuGlobal()->NumWorkerThreads; i++ )
		NUVERIFY( AbcThreadJoin( WorkerThreads[i] ) );

	delete nuGlobals;
}

NUAPI nuStyle** nuDefaultTagStyles()
{
	return DefaultTagStyles;
}

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
