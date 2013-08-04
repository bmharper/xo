#include "pch.h"
#include "JobQueue.h"
#include "profile.h"
#include "../Platform/debuglib.h"
#include <mtlog/mtlog.h>

void die()					{ AbcDebugBreak(); }
void or_die( bool x )		{ if ( !x ) die(); }
#define massert(f) (or_die( !!(f) ))

static u32 TraceJobs = 0;

PAPI void AbcTraceJobs( bool on )
{
	TraceJobs = on;
}

AbcThreadReturnType AbcKernelCallbackDecl AbcJobQueue::WorkerThread( void* threadContext )
{
	AbcJobQueue* q = (AbcJobQueue*) threadContext;
	char desc[AbcJob::DescLen];

	while ( true )
	{
		AbcSemaphoreWait( q->SemQueueCount, AbcINFINITE );
		if ( q->IsExitSignalled() ) break;
	
		AbcJob* job = q->WNextJob();
		if ( !job || job->Priority <= AbcJobPriorityBottom || job->Priority >= AbcJobPriorityDELETED ) { die(); break; } // I had a bug in the queue that I'm trying to track down
		memcpy( desc, job->Desc, sizeof(desc) );

		if ( !job->DelayUntil.IsNull() && job->DelayUntil > AbcDate::Now() )
		{
			if ( TraceJobs ) mtlog( "d:%s (delayed)", desc );
			// Job doesn't want to execute yet. Put it back in the queue.
			// As always, Sleep is bad. My particular use case is for warming up the databases used by IMQS. I want to delay this until a few second after app load.
			// So it isn't so bad there, because you're only running hot at the beginning of the app. I dread the added complexity of waiting on the semaphore as well
			// as a timer.
			if ( q->WIsEmpty() )
			{
				if ( TraceJobs ) mtlog( "sleeping" );
				AbcSleep(200);
			}
			q->WAddDelayedJobBackToQueue( job );
		}
		else
		{
			if ( TraceJobs ) mtlog( "a:%s", desc );
			AbcJobPriority priority = job->Priority;

			// Don't lower our thread priority if exit has been signaled.
			bool doBackground = (priority == AbcJobPriorityBackgroundProcess) && !q->IsExitSignalled();

			if ( doBackground )
				AbcThreadSetPriority( AbcThreadCurrent(), q->OSHasLowPriorityThreads ? AbcThreadPriorityBackgroundBegin : AbcThreadPriorityIdle );

			job->Run();
			q->WJobDone( job );
			job = NULL;

			if ( TraceJobs ) mtlog( "b:%s", desc );

			if ( doBackground )
				AbcThreadSetPriority( AbcThreadCurrent(), q->OSHasLowPriorityThreads ? AbcThreadPriorityBackgroundEnd : AbcThreadPriorityNormal );
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcJob::AbcJob()
{
	Priority = AbcJobPriorityNormal;
	Queue = NULL;
	FenceCount = NULL;
	Desc[0] = 0;
	DelayUntil = AbcDate();
}

AbcJob::~AbcJob()
{
	Priority = AbcJobPriorityDELETED;
}

void AbcJob::SetJobDesc( const char* s )
{
	strncpy( Desc, s, DescLen );
	Desc[DescLen-1] = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcFenceJob::AbcFenceJob( uint32 count, AbcSyncEvent* fence, AbcJobQueue* q )
{
	Count = count;
	Fence = fence;
	Queue = q;
	SetJobDesc("fence");
}

void AbcFenceJob::Run()
{
	while ( Count != 0 )
		AbcSleep(0);
	Queue->WFenceDone( Fence );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcJobQueue::AbcJobQueue()
{
#ifdef _WIN32
	OSVERSIONINFO inf;
	inf.dwOSVersionInfoSize = sizeof(inf);
	GetVersionEx( &inf );
	OSHasLowPriorityThreads = inf.dwMajorVersion >= 6;
#else
	OSHasLowPriorityThreads = false;
#endif
	JobsInFlight = 0;
	ExitSignalled = 0;
	Tail = 0;
	AbcSemaphoreInitialize( SemQueueCount );
	AbcCriticalSectionInitialize( MainCS );
	if ( TraceJobs ) mtlog_open();
}

AbcJobQueue::~AbcJobQueue()
{
	if ( Workers.size() == 0 && !ExitSignalled )
	{
		// For a single-worker, temporary job queue scenario, call Exit() so that Fence handles are cleaned up
		Exit();
	}
	ASSERT( Tail == Jobs.size() );
	ASSERT( Workers.size() == 0 );
	ASSERT( Fences.size() == 0 );
	AbcSemaphoreDestroy( SemQueueCount );
	AbcCriticalSectionDestroy( MainCS );
	if ( TraceJobs ) mtlog_close();
}

void AbcJobQueue::StartWorkers( int n )
{
	for ( int i = 0; i < n; i++ )
	{
		Workers += AbcThreadHandle();
		AbcVerify( AbcThreadCreate( &WorkerThread, this, Workers.back() ) );
	}
}

void AbcJobQueue::WFenceDone( AbcSyncEvent* fence )
{
	fence->Signal();
	
	// Can we immediately reuse the fence HANDLE? I don't know. I suspect we can, because we are using auto reset events.
	// This does imply that SetEvent will not return until the OS has moved the waiting thread to runnable.
	// The obvious and more conservative approach would be to never reuse fence HANDLEs. I don't know the overhead of
	// HANDLE creation and deletion.
	
	AbcCriticalSectionEnter( MainCS );
	for ( int i = 0; i < Fences.size(); i++ )
	{
		if ( fence == Fences[i] )
			FenceFree[i] = true;
	}
	AbcCriticalSectionLeave( MainCS );
}

void AbcJobQueue::AddJobInternal( bool front, AbcJob* job )
{
	if ( ExitSignalled )
	{
		AbcTrace( "Warning! Trying to add a job to the queue when we are exiting. Culling the job without running it.\n" );
		delete job;
		return;
	}
	job->Queue = this;
	AbcCriticalSectionEnter( MainCS );
	Jobs[NewJobSlot()] = job;
	if ( front )
		swap( Jobs[Tail], Jobs.back() );
	AbcCriticalSectionLeave( MainCS );
	AbcSemaphoreRelease( SemQueueCount, 1 );
}

void AbcJobQueue::AddJob( AbcJob* job )
{
	AddJobInternal( false, job );
}

void AbcJobQueue::AddJobToFront( AbcJob* job )
{
	AddJobInternal( true, job );
}

void AbcJobQueue::WaitForAll()
{
	if ( Workers.size() == 0 )
	{
		RunJobsHere();
	}
	else
	{
		while ( true )
		{
			{
				TakeCriticalSection lock(MainCS);
				if ( Jobs.size() - Tail == 0 && JobsInFlight == 0 ) break;
			}
			AbcSleep(0);
		}
	}
}

AbcFenceJob* AbcJobQueue::MakeFenceJob( int count )
{
	int slot = 0;
	for ( ; slot < FenceFree.size(); slot++ )
		if ( FenceFree[slot] )
			break;
	if ( slot == FenceFree.size() )
	{
		AbcSyncEvent* fence = new AbcSyncEvent();
		fence->Initialize( false );
		Fences += fence;
		FenceFree += false;
	}
	FenceFree[slot] = false;
	AbcFenceJob* fj = new AbcFenceJob( count, Fences[slot], this );
	return fj;
}

void AbcJobQueue::AddJobs( int n, AbcJob** jobs, bool wait_for_all )
{
	if ( n == 0 ) return;

	AbcCriticalSectionEnter( MainCS );
	int total_jobs = n;
	AbcFenceJob* fj = NULL;
	AbcSyncEvent* fence = NULL;
	bool doFence = wait_for_all && Workers.size() != 0;
	if ( doFence )
	{
		fj = MakeFenceJob( n );
		fence = fj->Fence;
	}
	for ( int i = 0; i < n; i++ )
	{
		jobs[i]->Queue = this;
		if ( fj )
			jobs[i]->FenceCount = &fj->Count;
		Jobs[NewJobSlot()] = jobs[i];
	}
	if ( doFence )
	{
		total_jobs++;
		Jobs[NewJobSlot()] = fj;
	}
	AbcCriticalSectionLeave( MainCS );
	AbcSemaphoreRelease( SemQueueCount, total_jobs );
	fj = NULL;

	if ( wait_for_all )
	{
		if ( Workers.size() == 0 ) // single CPU scenario
		{
			AbcAssert( !fence );
			RunJobsHere();
		}
		else
		{
			AbcAssert( fence );
			fence->Wait( AbcINFINITE ); // just for consistency - we'll never be waiting on this in practice if Workers.size() == 0
		}
	}
}

void AbcJobQueue::RunJobsHere()
{
	while ( AbcSemaphoreWait(SemQueueCount, 0) )
	{
		AbcJob* j = WNextJob();
		j->Run();
		WJobDone( j );
	}
}

/* Non-obvious issue: We must increment the JobsInFlight before popping the job off the stack. If you don't do this,
then you will get false positives in detecting whether the queue is empty.
*/
AbcJob* AbcJobQueue::WNextJob()
{
	AbcCriticalSectionEnter( MainCS );
	AbcInterlockedIncrement( &JobsInFlight );
	CheckSanity();
	if ( (u32) Tail >= (u32) Jobs.size() )
		AbcDebugBreak();
	AbcJob* j = Jobs[Tail++];
	if ( !j )
		AbcDebugBreak();
	AbcCriticalSectionLeave( MainCS );
	return j;
}

bool AbcJobQueue::WIsEmpty()
{
	AbcCriticalSectionEnter( MainCS );
	bool empty = Tail == Jobs.size();
	AbcCriticalSectionLeave( MainCS );
	return empty;
}

void AbcJobQueue::WJobDone( AbcJob* j )
{
	if ( j->FenceCount )
		AbcInterlockedDecrement( j->FenceCount );
	delete j;
	AbcInterlockedDecrement( &JobsInFlight );
}

void AbcJobQueue::WAddDelayedJobBackToQueue( AbcJob* j )
{
	AbcCriticalSectionEnter( MainCS );
	Jobs[NewJobSlot()] = j;
	AbcInterlockedDecrement( &JobsInFlight );
	AbcCriticalSectionLeave( MainCS );
	AbcSemaphoreRelease( SemQueueCount, 1 );
}

void AbcJobQueue::CheckSanity()
{
	if ( !TraceJobs ) return;
	for ( int i = Tail; i < Jobs.size(); i++ )
	{
		massert( Jobs[i] );
		massert( Jobs[i]->Desc[0] != 0 );
		massert( Jobs[i]->Priority > AbcJobPriorityBottom && Jobs[i]->Priority < AbcJobPriorityDELETED );
		massert( Jobs[i]->Queue == this );
		if ( i > Tail ) massert( Jobs[i] != Jobs[i - 1] );
	}
}

int AbcJobQueue::NewJobSlot()
{
	CheckSanity();
	if ( Tail >= QueueBumpThreshold )
	{
		if ( TraceJobs ) mtlog( "bump %d %d", (int) Jobs.count, (int) Tail );
		// bump down the job list. This is just so much easier to think about than a circular buffer.
		// AHEM: This is a great read -- we should do this instead: http://fgiesen.wordpress.com/2010/12/14/ring-buffers-and-queues/
		int queue_size = Jobs.size() - Tail;
		// All of the debugging code littered throughout here was in order to find the following bug: Jobs[i] = Jobs[i] + Tail;
		// That was a 5 hour bug. It was especially hard to track because initially my limit above was ( Tail >= 50 ), and that condition
		// would never hit on my home i7, with Tail != Jobs.size().. which was a necessary precondition for the bug to surface.
		for ( int i = 0; i < queue_size; i++ ) 
			Jobs[i] = Jobs[i + Tail];
		Jobs.count -= Tail;
		Tail = 0;
		CheckSanity();
	}
	Jobs += NULL;
	return Jobs.size() - 1;
}

void AbcJobQueue::Exit()
{
	ExitSignalled = 1;
	AbcSemaphoreRelease( SemQueueCount, Workers.size() );
	for ( intp i = 0; i < Workers.size(); i++ )
	{
		// This extra release is for paranoia
		AbcSemaphoreRelease( SemQueueCount, 5 );
		AbcThreadJoin( Workers[i] );
		AbcThreadCloseHandle( Workers[i] );
	}
	Workers.clear();
	
	delete_all( Fences );
	FenceFree.clear();
}


