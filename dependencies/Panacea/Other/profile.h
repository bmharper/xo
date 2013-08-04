#pragma once

// uncomment the following line to do profiling.
//#define _PROFILE

#include "../Platform/stdint.h"
#include "../Platform/syncprims.h"
#include "../Platform/timeprims.h"

#ifdef _WIN32
#include <intrin.h>
#endif

namespace AbCore
{

	/** Uses CPUID on XP-32 and below. 
	**/
	DWORD PAPI GetCurrentProcessorNumber_Gen();

	/// Retrieve the number of CPUs in the system.
	DWORD PAPI GetNumberOfProcessors();

	/** Locks the current thread to the processor that it is running on now.
	This allows you to use RDTSC with some relevance. Note that RDTSC's speed will likely shift with processor
	throttling, but at least it's something (something that is monotonically increasing!).
	@return false If GetCurrentProcessorNumber() is not available (XP-32 and below).
	**/
	bool PAPI LockThreadToProcessor();

	/// Undoes the action of LockThreadToProcessor.
	void PAPI ReleaseThreadToAllProcessors();

}

double	PAPI	AbcRTC();									// Uses GetSystemTimeAsFileTime, and returns the number of seconds (accurate to 15 ms, the kernel clock thingy).
void	PAPI	AbcTrace( const char* fmt, ... );			// printf into OutputDebugString
double	PAPI	AbcMachineHz();								// Get the rate measured
void	PAPI	AbcMachineHz_Ping();						// Call this often and the system will eventually have a decent average clock rate

// Goals: trivial atomic time (implies 32-bit timestamp), process can run for 10 years.
// 31,536,000 seconds in a year.
// x10 years = 315,360,000.
// 2^31 / 315,360,000 = 6.8
// So.. let's just say we have an accuracy of 1/4 of a second.
// We use a signed integer instead of unsigned, to allow for clock adjustments (such as from NTP).
// Hmm.. 1/4 seconds is pretty useless - might as well just make it 1 second. If you need higher precision, you won't be using this.
// So at an accuracy of 1 second, we have 68 years running time.

typedef int32 AbcTime32;	// Use this type to signify that your time is from AbcRTC32()

AbcTime32	PAPI AbcRTC32();							// Time in seconds. Uses an epoch set by AbcRTC32_Init().
void		PAPI AbcRTC32_Init();

#define ABC_RTC64_PER_SECOND 1000000
int64	PAPI AbcRTC64();							// Returns the value of GetSystemTimeAsFileTime, as the number of microseconds (1e-6) - use ABC_RTC64_PER_SECOND

enum ProfileSet
{
	PROF_NULL = 0,

	PROF_ADB_COMPARE_FIELD_SET,
	PROF_ADB_MODTABLE_GET,
	PROF_ADB_MODTABLE_GET_RAW,

	PROF_BUF_FILE_READ,
	PROF_BUF_FILE_FIND_BUFFER,
	PROF_BUF_FILE_BRING_IN,
	PROF_BUF_FILE_FLUSH_OLDEST,

	PROF_DRAWING_INSERT,
	PROF_LAYER_INSERT,
	PROF_NAME_ADD,
	PROF_NAME_UPDATE,
	PROF_BNODE_INSERT,
	PROF_BNODE_RESTRUCTURE,
	PROF_BNODE_DRAW,
	PROF_BNODE_DRAW_NC,
	PROF_BNODE_DRAW_CACHED,
	PROF_BNODE_BUFFERGL,
	PROF_BNODE_DRAWGL,
	PROF_BNODE_DRAWGL_INTERNAL,
	PROF_BNODE_DRAWGL_HASCHILD,
	PROF_BNODE_FINDBUFFER,
	PROF_BNODE_TRIMBUFFERS,
	PROF_BNODE_CLEARBUFF_INT,
	PROF_SWAPBUFFERS,
	PROF_PRIMBUFF_VERTEX,
	PROF_PRIMBUFF_VERTEX_ARRAY,
	PROF_PRIMBUFF_BINDTEX,
	PROF_PRIMBUFF_ENSURESPACE,
	PROF_PRIMBUFF_ENDSTORE,
	PROF_PRIMBUFF_RENDACC,
	PROF_PRIMBUFF_CALL_LIST,
	PROF_PRIMBUFF_BEGIN,
	PROF_PRIMBUFF_BEGIN_AUTO,
	PROF_DRAWERGL_DRAW,
	PROF_DRAW_CIRCLE,
	PROF_DRAW_TEXT,
	PROF_DRAW_LINE,
	PROF_DRAW_THIN_BUFFERED,
	PROF_DRAW_THIN_OPT_VEC3,
	PROF_DRAW_THIN_BUF_SETUP,
	PROF_PAN_SWAP_ADD,
	PROF_PAN_SWAP_GET,
	PROF_PAN_SWAP_SWAPIN,
	PROF_PAN_SWAP_SWAPOUT,
	PROF_PAN_SWAP_SWAPOUT_OLDEST,
	PROF_PAN_SWAP_NONSWAPPED,
	PROF_PAN_SWAP_GETWRITEPOS,
	PROF_GIS_SHP_READOBJECT,
	PROF_GIS_GEOM_POINT,
	PROF_GIS_GEOM_ARC,
	PROF_GIS_GEOM_POLYGON,
	PROF_GIS_APPLY_RULES,
	PROF_GIS_RENDER_TO_CAD,
	PROF_GIS_LOAD_DATA,
	PROF_GIS_RENDER_FEATURE,
	PROF_GIS_INSERT_GENERATED,
	PROF_GIS_EVALUATOR_EXECUTE,
	PROF_GIS_TXT_FORMAT,
	PROF_GIS_TXT_RENDER,
	PROF_GIS_RULE_EVALUATE,
	PROF_GIS_APPLY_OUTPUTS,
	PROF_GIS_APPLY_TEXT,
	PROF_GIS_TXT_APPLY_RULES,
	PROF_DB_DBF_ATTRIBUTE,
	PROF_DB_DBF_ATTRIB_EMPTY,
	PROF_TEMP1,
	PROF_TEMP2,
	PROF_TEMP3,
	PROF_TEMP4,
	PROF_TEMP5,
	PROF_SIZE
};

static const char* ProfileNames[]  = 
{
	"PROF_NULL                  ",
	
	"PROF_ADB_COMPARE_FIELD_SET ",
	"PROF_ADB_MODTABLE_GET      ",
	"PROF_ADB_MODTABLE_GET_RAW  ",

	"PROF_BUF_FILE_READ",
	"PROF_BUF_FILE_FIND_BUFFER",
	"PROF_BUF_FILE_BRING_IN",
	"PROF_BUF_FILE_FLUSH_OLDEST",

	"PROF_DRAWING_INSERT        ",
	"PROF_LAYER_INSERT          ",
	"PROF_NAME_ADD              ",
	"PROF_NAME_UPDATE           ",
	"PROF_BNODE_INSERT          ",
	"PROF_BNODE_RESTRUCTURE     ",
	"PROF_BNODE_DRAW            ",
	"PROF_BNODE_DRAW_NC         ",
	"PROF_BNODE_DRAW_CACHED     ",
	"PROF_BNODE_BUFFERGL        ",
	"PROF_BNODE_DRAWGL          ",
	"PROF_BNODE_DRAWGL_INTERNAL ",
	"PROF_BNODE_DRAWGL_HASCHILD ",
	"PROF_BNODE_FINDBUFFER      ",
	"PROF_BNODE_TRIMBUFFERS     ",
	"PROF_BNODE_CLEARBUFF_INT   ",
	"PROF_SWAPBUFFERS           ",
	"PROF_PRIMBUFF_VERTEX       ",
	"PROF_PRIMBUFF_VERTEX_ARRAY ",
	"PROF_PRIMBUFF_BINDTEX      ",
	"PROF_PRIMBUFF_ENSURESPACE  ",
	"PROF_PRIMBUFF_ENDSTORE     ",
	"PROF_PRIMBUFF_RENDACC      ",
	"PROF_PRIMBUFF_CALL_LIST    ",
	"PROF_PRIMBUFF_BEGIN        ",
	"PROF_PRIMBUFF_BEGIN_AUTO   ",
	"PROF_DRAWERGL_DRAW         ",
	"PROF_DRAW_CIRCLE           ",
	"PROF_DRAW_TEXT             ",
	"PROF_DRAW_LINE             ",
	"PROF_DRAW_THIN_BUFFERED    ",
	"PROF_DRAW_THIN_OPT_VEC3    ",
	"PROF_DRAW_THIN_BUF_SETUP   ",
	"PROF_PAN_SWAP_ADD          ",
	"PROF_PAN_SWAP_GET          ",
	"PROF_PAN_SWAP_SWAPIN       ",
	"PROF_PAN_SWAP_SWAPOUT      ",
	"PROF_PAN_SWAP_SWAP_OLDEST  ",
	"PROF_PAN_SWAP_NONSWAPPED   ",
	"PROF_PAN_SWAP_GETWRITEPOS  ",
	"PROF_GIS_SHP_READOBJECT    ",
	"PROF_GIS_GEOM_POINT        ",
	"PROF_GIS_GEOM_ARC          ",
	"PROF_GIS_GEOM_POLYGON      ",
	"PROF_GIS_APPLY_RULES       ",
	"PROF_GIS_RENDER_TO_CAD     ",
	"PROF_GIS_LOAD_DATA         ",
	"PROF_GIS_RENDER_FEATURE    ",
	"PROF_GIS_INSERT_GENERATED  ",
	"PROF_GIS_EVALUATOR_EXECUTE ",
	"PROF_GIS_TXT_FORMAT        ",
	"PROF_GIS_TXT_RENDER        ",
	"PROF_GIS_RULE_EVALUATE     ",
	"PROF_GIS_APPLY_OUTPUTS     ",
	"PROF_GIS_APPLY_TEXT        ",
	"PROF_GIS_TXT_APPLY_RULES   ",
	"PROF_DB_DBF_ATTRIBUTE      ",
	"PROF_DB_DBF_ATTRIB_EMPTY   ",
	"PROF_TEMP1                 ",
	"PROF_TEMP2                 ",
	"PROF_TEMP3                 ",
	"PROF_TEMP4                 ",
	"PROF_TEMP5                 ",
	""
};

#ifdef _PROFILE
extern PAPI bool ProfileActive;
extern PAPI int ProfileTimesSize;
extern PAPI INT64 ProfileTimes[PROF_SIZE];
extern PAPI INT64 ProfileEnter[PROF_SIZE];
extern PAPI int ProfileStack[8192];
extern PAPI int ProfileStackDepth;
#endif

#ifdef _PROFILE
void PAPI ProfileStart();
void PAPI ProfileEnd( LPCTSTR title );
#else
inline void ProfileStart() {}
inline void ProfileEnd( LPCTSTR title ) {}
#endif

#ifdef _PROFILE
#define PROFILE_SECTION(ccc) ProfileSection ProfSECTION( ccc )
#define PROFILE_ENTER(ccc) ProfileEnter[ ccc ] = RDTSC()
#define PROFILE_LEAVE(ccc) ProfileTimes[ ccc ] += RDTSC() - ProfileEnter[ ccc ]
#else
#define PROFILE_SECTION(ccc) ((void)0)
#define PROFILE_ENTER(ccc) ((void)0)
#define PROFILE_LEAVE(ccc) ((void)0)
#endif

#ifdef _WIN32
inline int64 RDTSC()
{
	return __rdtsc();
}
#else
#	if defined(__i386__)

static __inline__ int64 RDTSC(void)
{
    int64 x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#	elif defined(__x86_64__)

static __inline__ int64 RDTSC(void)
{
    uint32 hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return int64( (uint64)lo)|( ((uint64)hi)<<32 );
}

#	endif
#endif

// unsigned result
inline uint64 RDTSCu()
{
	return (uint64) RDTSC();
}

inline double RDTSC_Seconds( double clock_speed = 2000000000 )
{
	return RDTSC() / clock_speed;
}

// Timer variable that uses the last CPU number to make sure it doesn't record a bogus RDTSC
// There is obviously a flaw here in that you could switch core between your reading of the current processor number
// and your measurement of RDTSC, but this is better than nothing for doing benchmarks.
// I do make a small attempt to detect this condition, but I haven't tested it to failure.
// Every time sample here takes about 250 clock cycles, because of the CPUID call.
struct AbcTimer
{
	i64		Start;
	u16		ProcessorNum;

	AbcTimer()
	{
		GetTimeAndCore( Start, ProcessorNum );
	}

	i64 End() const
	{
		u16 core;
		i64 time;
		GetTimeAndCore( time, core );
		if ( core == ProcessorNum )
			return time - Start;
		else
			return 0;
	}

	static bool GetTimeAndCore( i64& time, u16& core )
	{
		// I initially thought that 2 attempts should usually suffice, but maybe you end up calling this function just before your time slice expires.
		// However, it seems we need more.
		// On a virtualbox VM, on an i7 2600, I had to raise these numbers.
		// The most important thing here is to detect a context/thread switch.
		// 15 ms * 3ghz = 45000 clocks.
		// So the max delta number below here should definitely be far south of 45000.
		// I can't get it much less than 12k on a VM unfortunately.
		for ( int attempt = 0; attempt < 20; attempt++ )
		{
			i64 t1 = RDTSC();
			core = (u16) AbCore::GetCurrentProcessorNumber_Gen();
			time = RDTSC();
			// Core i7:			250 clocks. 2000 just seems like a reasonable buffer. Different cores normally have WAY different RDTSCs.
			// Core2 quad:		8000 clocks under debugger. 
			// Core i7 2600 VM:	12000 seems to be necessary.
			auto delta = time - t1;
			if ( delta > 10 && delta < 12000 )
				return true;
		}
		return false;
	}

};

// You'll use a bunch of these to accumulate your measurements
struct AbcTimeStat
{
	u32 Time;	// Unit: 256 clock cycles
	u32	Count;	// Number of times we were incremented

	AbcTimeStat() { Time = 0; Count = 0; }
	void	Add( i64 rdtsc_interval )
	{
		AbcInterlockedAdd( &Time, rdtsc_interval >> 8 );
		AbcInterlockedIncrement( &Count );
	}
	double	Clocks() const { return (double) Time * 256.0; }
};

// Updates the timer when it goes out of scope
struct AbcScopedTimer
{
	AbcTimer		T;
	AbcTimeStat*	S;

	AbcScopedTimer( AbcTimeStat& s )	{ S = &s; }
	~AbcScopedTimer()					{ S->Add( T.End() ); }
};

#ifdef _PROFILE
//#pragma optimize( "", off )
class PAPI ProfileSection
{
public:
	int Code;
	ProfileSection( ProfileSet code )
	{
		Code = code;
		PauseCurrent();
		ProfileEnter[code] = RDTSC();
		ProfileStack[ProfileStackDepth++] = code;
	}

	void PauseCurrent()
	{
		if ( ProfileStackDepth > 0 )
		{
			PROFILE_LEAVE( ProfileStack[ProfileStackDepth - 1] );
		}
	}

	void ResumeLast()
	{
		if ( ProfileStackDepth > 0 )
		{
			PROFILE_ENTER( ProfileStack[ProfileStackDepth - 1] );
		}
	}

	~ProfileSection()
	{
		ProfileStackDepth--;
		ResumeLast();
		PROFILE_LEAVE( Code );
	}
};
//#pragma optimize( "", on ) 
#endif


