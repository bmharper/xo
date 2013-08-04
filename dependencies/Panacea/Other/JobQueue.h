#pragma once

#include "../Containers/pvect.h"
#include "../System/Date.h"
#include "../Platform/syncprims.h"
#include "../Platform/thread.h"

PAPI void AbcTraceJobs( bool on );		// Toggle tracing job info via mtlog

class AbcJobQueue;

enum AbcJobPriority
{
	// Leave zero invalid to aid in catching this memory corruption bug
	AbcJobPriorityBottom = 0,
	AbcJobPriorityBackgroundProcess,
	AbcJobPriorityNormal,
	AbcJobPriorityDELETED,						// For debugging. Set by object destructor.
};

#pragma warning( push )
#pragma warning( disable: 4996 )
/// Interface of a job for the JobQueue
class PAPI AbcJob
{
public:
	static const int	DescLen = 20;
	AbcJobPriority		Priority;
	AbcJobQueue*		Queue;
	volatile uint32*	FenceCount;			// points to FenceJob::Count
	char				Desc[DescLen];		// For debugging
	AbcDate				DelayUntil;			// Do not start until this time

	AbcJob();
	virtual ~AbcJob();
	virtual void Run() = 0;

	void SetJobDesc( const char* s );
};
#pragma warning( pop )

class PAPI AbcFenceJob : public AbcJob
{
public:
	AbcSyncEvent*	Fence;
	volatile uint32	Count;	// when this reaches zero, all jobs have finished
	AbcFenceJob( uint32 count, AbcSyncEvent* fence, AbcJobQueue* q );
	virtual void Run();
};

/** Job Queue.

The idea is that you create one of these per process. Then you create a bunch of threads, and those threads wait upon the queue
until it is hot. Then they all wake up and process jobs until the queue is empty.

The queue is FIFO.

Usage with >= 1 worker thread
------------------------------

* StartWorkers(0..n)
* AddJob or AddJobs. If you want to wait for your job(s) to finish, you must use AddJobs. This will create
	a fence object that is associated with that job set. The fence does not wait for older jobs that may still be executing.
* Exit()

Usage with 0 worker threads
---------------------------

* AddJob or AddJobs.
* WaitForAll()
	
Visualization of buffer
-----------------------

+ Completed job
* Waiting in queue
@ Busy being operated on

Jobs.size()   Slot where new jobs go
P2            Oldest job still executing (of no concern to the queue)
Tail          Next job to be fetched from queue

									P2           Tail        Jobs.size()    
++++++++++++++++++^@+@+@@@+@@@+^***********^
									^            ^           ^

We don't care about jobs currently executing. Once a job has been fetched from the queue, we never touch it again.
So our policy is simply to bump down the entire list, between Tail and Jobs.size(), so that Tail = 0. Then we start
adding again.

**/
class PAPI AbcJobQueue
{
public:
	friend class AbcFenceJob;

	// We bump down the queue when the tail hits this number.
	static const int QueueBumpThreshold = 100;

	volatile uint32 JobsInFlight;	// Managed by JobRun

	AbcJobQueue();
	~AbcJobQueue();

	// Start n worker threads associated with this queue. Do this once at process startup.
	void StartWorkers( int n );
	void Exit();
	bool IsExitSignalled() { return !!ExitSignalled; }

	int NumberOfWorkerThreads() { return Workers.size(); }

	void AddJobs( int n, AbcJob** jobs, bool wait_for_all );
	void AddJob( AbcJob* job );
	void AddJobToFront( AbcJob* job );		// Add a job to the front of the queue. Used by jobs that are optimally serial. Specifically, background file loader from a hard disk.
	void WaitForAll();	// This is unsophisticated. It does not use events to signal completion - it simply does Sleep(0) until the queue is empty and JobsInFlight == 0

private:

	AbcCriticalSection		MainCS;
	AbcSemaphore			SemQueueCount;
	bool					OSHasLowPriorityThreads;
	int						Tail;
	pvect<AbcJob*>			Jobs;
		
	podvec<u32>				FenceFree;		// parallel
	pvect<AbcSyncEvent*>	Fences;			// parallel

	podvec<AbcThreadHandle>	Workers;

	volatile int32			ExitSignalled;

	static AbcThreadReturnType AbcKernelCallbackDecl	WorkerThread( void* threadContext );
	static void											JobRun( AbcJob* job );

	int				NewJobSlot();
	AbcFenceJob*	MakeFenceJob( int count );
	void			RunJobsHere();
	void			CheckSanity();
	void			AddJobInternal( bool front, AbcJob* job );

	// W prefix means the function is called by worker threads
	void		WFenceDone( AbcSyncEvent* f );
	AbcJob*		WNextJob();
	void		WJobDone( AbcJob* j );
	void		WAddDelayedJobBackToQueue( AbcJob* j );
	bool		WIsEmpty();

};

