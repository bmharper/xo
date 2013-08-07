#include "pch.h"
#include "nuDefs.h"
#include "nuDocGroup.h"
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
	// allow documents scheduled for deletion to be deleted
	nuProcessDocQueue();

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
	if ( InitializeCount != 1 ) return;

	AbcMachineInformation minf;
	AbcMachineInformationGet( minf );

	nuGlobals = new nuGlobalStruct();
	nuGlobals->TargetFPS = 60;
	nuGlobals->NumWorkerThreads = min( minf.CPUCount, MAX_WORKER_THREADS );
	//nuGlobals->DebugZeroClonedChildList = true;
	nuGlobals->DocAddQueue.Initialize( false );
	nuGlobals->DocRemoveQueue.Initialize( false );
	nuGlobals->EventQueue.Initialize( true );
	nuGlobals->JobQueue.Initialize( true );
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

NUAPI void NUTIME( const char* msg, ... )
{
	const int timeChars = 16;
	char buff[1024] = "";
	sprintf( buff, "%-15.3f  ", AbcTimeAccurateRTSeconds() * 1000 );
	va_list va;
	va_start( va, msg );
	uint r = vsnprintf( buff + timeChars, arraysize(buff) - timeChars, msg, va );
	va_end( va );
	if ( r < arraysize(buff) )
		NUTRACE_WRITE(buff);
}
