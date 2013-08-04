#include "pch.h"
#include "profile.h"
#include "../Strings/strings.h"
#include "../Platform/timeprims.h"
#include "../Platform/syncprims.h"
#include "../Platform/debuglib.h"
#include "../Platform/stdint.h"

#ifdef __linux
//#include <linux/getcpu.h>
#include <sched.h>
#endif

static u32				MachineMhz = 3 * 1000;		// keep this u32 for atomicity
static volatile u32		HzLock = 0;
static u32				LastProcNum = 0;
static int64			LastRDTSC = 0;
static double			LastRTC = 0;
static int64			Epoch32 = 0;

/*

AHEM.. Use this awesome technique from Charles Bloom:

Profiler_Push(label)  *ptr++ = PUSH(#label); *ptr++ = rdtsc();
Profiler_Pop( label)  *ptr++ = POP( #label); *ptr++ = rdtsc();

#define PUSH(str)  (U64)(str)
#define POP (str) -(S64)(str)

where ptr is some big global array of U64's , and we will later use the stringized label as a unique id to merge traces.
Once your app is done sampling, you have this big array of pushes and pops, you can then parse that to figure out all the hierarichical timing information.
In practice you would want to use this with a scoper class to do the push/pop for you, like :

class rrScopeProfiler { rrScopeProfiler() { Push; } ~rrScopeProfiler() { Pop; } };

#define PROFILE(label)  rrScopeProfiler RR_STRING_JOIN(label,_scoper) ( #label );

*/

#ifdef _WIN32
// secure CRT
#pragma warning( disable: 4996 )
#endif

namespace AbCore
{

#ifdef _WIN32
	// This appears to give identical results to GetCurrentProcessorNumber().
	DWORD GetCurrentProcessorNumberCPUID()
	{
#	ifdef _M_X64
		// XP-64 and up has GetCurrentProcessor, so we should never be called.
		ASSERT(false);
		return 0;
#	else
		__asm
		{
			mov eax, 1
			cpuid
			shr ebx, 24
			mov eax, ebx
		}
#	endif
	}
#endif

#ifdef _WIN32
	typedef DWORD (WINAPI *GetCurrentProcessorNumber_proc)();
	static GetCurrentProcessorNumber_proc KernelGetProcNum = NULL;
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 6309)	// GetProcAddress might fail...
#pragma warning(disable: 6387)	// GetProcAddress might fail...
#endif

	DWORD PAPI GetCurrentProcessorNumber_Gen()
	{
#ifdef _WIN32
		if ( KernelGetProcNum == NULL )
		{
			KernelGetProcNum = (GetCurrentProcessorNumber_proc) GetProcAddress( GetModuleHandle(L"Kernel32.dll"), "GetCurrentProcessorNumber" );
			if ( KernelGetProcNum == NULL ) KernelGetProcNum = (GetCurrentProcessorNumber_proc) INVALID_HANDLE_VALUE;
		}

		if ( KernelGetProcNum == INVALID_HANDLE_VALUE )
			return GetCurrentProcessorNumberCPUID();
		else
		{
			DWORD p1 = KernelGetProcNum();
			/* -- I doubt this behaviour will ever change --
#if defined(_DEBUG) && !defined(_M_X64)
			// ensure that CPUID result is identical to kernel's result. Of course, if our thread is interrupted between these two calls, then this
			// assertion will incorrectly fail.
			DWORD p2 = GetCurrentProcessorNumberCPUID();
			// Indeed -- this does occassionally fail, but that's to be expected.
			//ASSERT( p1 == p2 );
			if ( p1 != p2 ) TRACE( "GetCurrentProcessorNumberCPUID != KernelGetProcNum!! This shouldn't happen very often.\n" );
#endif
			*/
			return p1;
		}
#else
		return (DWORD) sched_getcpu();
#endif
	}


#ifdef _MSC_VER
#pragma warning(pop)
#endif

	DWORD PAPI GetNumberOfProcessors()
	{
#ifdef _WIN32
		SYSTEM_INFO inf;
		GetSystemInfo( &inf );
		return inf.dwNumberOfProcessors;
#else
		return sysconf(_SC_NPROCESSORS_ONLN);
#endif
	}

	bool PAPI LockThreadToProcessor()
	{
#ifdef _WIN32
		DWORD_PTR curProc = GetCurrentProcessorNumber_Gen();
		if ( curProc == -1 )
			return false;
		HANDLE proc = GetCurrentProcess();
		DWORD_PTR proAfn, sysAfn;
		BOOL ok = GetProcessAffinityMask( proc, &proAfn, &sysAfn );
		DWORD_PTR curProcBit = (DWORD_PTR) ((u64) 1 << curProc);
		ASSERT( proAfn & curProcBit );
		SetThreadAffinityMask( GetCurrentThread(), curProcBit );
		return true;
#else
		cpu_set_t cpuset;
		CPU_ZERO( &cpuset );
		CPU_SET( GetCurrentProcessorNumber_Gen(), &cpuset );
		pthread_setaffinity_np( AbcThreadCurrent(), sizeof(cpuset), &cpuset );
#endif
	}

	void PAPI ReleaseThreadToAllProcessors()
	{
#ifdef _WIN32
		HANDLE proc = GetCurrentProcess();
		DWORD_PTR proAfn, sysAfn;
		BOOL ok = GetProcessAffinityMask( proc, &proAfn, &sysAfn );
		SetThreadAffinityMask( GetCurrentThread(), proAfn );
#else
		cpu_set_t cpuset;
		CPU_ZERO( &cpuset );
		int nproc = GetNumberOfProcessors();
		for ( int i = 0; i < nproc; i++ )
			CPU_SET( i, &cpuset );
		pthread_setaffinity_np( AbcThreadCurrent(), sizeof(cpuset), &cpuset );
#endif
	}

}

void PAPI AbcTrace( const char* fmt, ... )
{
	const int MAXSIZE = 1024;
	char buff[MAXSIZE+1];

	// We don't use the 'n' functions, because those do not error. It is definitely better to have an error shown than to fail silently.
	va_list va;
	va_start( va, fmt );
	vsnprintf( buff, MAXSIZE, fmt, va );
	va_end( va );
	buff[MAXSIZE] = 0;

	AbcOutputDebugString( buff );
}

// microseconds
int64 PAPI AbcRTC64()
{
#ifdef _WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime( &ft );
	INT64 t = ((INT64) ft.dwHighDateTime << 32) | (INT64) ft.dwLowDateTime;
	return t / 10;
#else
	return (int64) (AbcTimeAccurateRTSeconds() * 1000 * 1000);
#endif
}

// seconds
double PAPI AbcRTC()
{
#ifdef _WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime( &ft );
	INT64 t = ((INT64) ft.dwHighDateTime << 32) | (INT64) ft.dwLowDateTime;
	return t / (10.0 * 1000 * 1000);
#else
	return AbcTimeAccurateRTSeconds();
#endif
}

double PAPI AbcMachineHz()
{
	return MachineMhz * 1000000;
}

void PAPI AbcMachineHz_Ping()
{
	// at time of going to press, 10 gigahertz was in the very far future
	u32 proc = AbCore::GetCurrentProcessorNumber_Gen();
	int64 rdtsc = RDTSC();
	double t = AbcRTC();
	if ( AbcCmpXChg(&HzLock, 1, 0) == 0 )
	{
		int64 delta = t - LastRTC;
		if ( delta != 0 && proc == LastProcNum )
		{
			double mhz = (rdtsc - LastRDTSC) / (delta * 1000000.0);
			if ( mhz > 300 && mhz < 10000 )
			{
				MachineMhz = MachineMhz * 0.99 + mhz * 0.01;
			}
		}
		LastRTC = t;
		LastRDTSC = rdtsc;
		LastProcNum = proc;
		HzLock = 0;
	}
}


int32	PAPI AbcRTC32()
{
	return (AbcRTC64() - Epoch32) / (1000 * 1000);
}

void	PAPI AbcRTC32_Init()
{
	Epoch32 = AbcRTC64();
}


#ifdef _PROFILE

bool ProfileActive = false;
int ProfileTimesSize;
INT64 ProfileTimes[PROF_SIZE];
INT64 ProfileEnter[PROF_SIZE];
INT64 ProfileTimeStart;
INT64 ProfileTimeStart_QPC;
int ProfileStack[8192];
int ProfileStackDepth = 0;

void ProfileStart()
{
	AbCore::LockThreadToProcessor();

	ProfileTimesSize = PROF_SIZE;
	ZeroMemory( ProfileTimes, sizeof(INT64) * ProfileTimesSize );
	ZeroMemory( ProfileEnter, sizeof(INT64) * ProfileTimesSize );
	ZeroMemory( ProfileStack, sizeof(ProfileStack) );
	ProfileTimeStart = RDTSC();
	ProfileActive = true;
	ProfileStackDepth = 0;

	LARGE_INTEGER p1;
	QueryPerformanceCounter( &p1 );
	ProfileTimeStart_QPC = p1.QuadPart;
}

struct CodeTimePair
{
	int		code;
	INT64	etime;
};

int ProfileSortFunc( const void* p1, const void* p2 )
{
	CodeTimePair *pair1 = (CodeTimePair*) p1;
	CodeTimePair *pair2 = (CodeTimePair*) p2;
	if (pair1->etime < pair2->etime)
		return -1;
	else if (pair1->etime == pair2->etime)
		return 0;
	else
		return 1;
}

template < typename TData >
void reverse( TData* base, size_t count )
{
	size_t lim = count / 2;
	for ( size_t i = 0; i < lim; i++ )
	{
		size_t j = count - i - 1;
		TData tt = base[i];
		base[i] = base[j];
		base[j] = tt;
	}
}

void SortTimes( CodeTimePair* pairs )
{
	for (int i = 0; i < PROF_SIZE; i++)
	{
		pairs[i].code = i;
		pairs[i].etime = ProfileTimes[i];
	}

	qsort( pairs, PROF_SIZE, sizeof(CodeTimePair), ProfileSortFunc );

	reverse( pairs, PROF_SIZE );
}

void ProfileEnd( LPCTSTR title )
{
	ASSERT( ProfileStackDepth == 0 );
	ProfileActive = false;
	//double hz = MachineHz();
	INT64 now = RDTSC();

	LARGE_INTEGER qpc, qpf;
	QueryPerformanceCounter( &qpc );
	QueryPerformanceFrequency( &qpf );
	double seconds = (qpc.QuadPart - ProfileTimeStart_QPC) / (double) qpf.QuadPart;
	double hz = (now - ProfileTimeStart) / seconds;
	//double proftime = (now - ProfileTimeStart) / hz;
	double proftime = seconds;

	AbCore::ReleaseThreadToAllProcessors();

	FILE *f = fopen( PANTEST_DIR_A "profile.txt", "at+" );
	if (f)
	{
		//fprintf(f, "--------------------------------------------------------------\n");
		fprintf(f, "Start profile  %S  ---------------------------------------------\n", title );
		fprintf(f, "Runtime: %.2f  Processor HZ: %s --------------------------------------------\n\n", proftime, (LPCSTR) FloatToXStringAWithCommas(floor(hz), 20) );

		CodeTimePair pairs[PROF_SIZE];
		SortTimes( pairs );
		double tottime = 0;
		// count total time.
		for ( int i = 0; i < ProfileTimesSize; i++ )
			tottime += ProfileTimes[i] / hz;

		for ( int i = 0; i < ProfileTimesSize; i++ )
		{
			double ftime = ProfileTimes[pairs[i].code] / hz;
			if ( ftime == 0 ) continue;
			XStringA name = ProfileNames[pairs[i].code];
			name.Trim();
			fprintf( f, "%-32s %5.2f %4.2f%%\n", (LPCSTR) name, ftime, ftime * 100.0 / tottime );
		}

		fprintf(f, "\nEnd profile    -----------------------------------------------\n");
		//fprintf(f, "--------------------------------------------------------------\n\n");

		fclose(f);
	}
	else
		TRACE("profiling file lock error\n");
}
#endif


