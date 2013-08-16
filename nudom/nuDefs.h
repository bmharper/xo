#pragma once

#include "nuPlatform.h"

class nuDomEl;
class nuDoc;
class nuEvent;
class nuImage;
class nuImageStore;
class nuLayout;
class nuPool;
class nuDocGroup;
class nuRenderDoc;
class nuRenderer;
class nuRenderDomEl;
class nuRenderGL;
class nuString;
class nuStringTable;
class nuStyle;
class nuSysWnd;

typedef int32 nuPos;					// fixed-point position
static const u32 nuPosShift = 8;		// 24:8 fixed point coordinates used during layout

// An ID that is internal to nudom - i.e. it is not controllable by external code.
// This ID is an integer that you can use to reference a DOM element. These IDs are recycled.
typedef int32 nuInternalID;		
static const nuInternalID nuInternalIDNull = 0;		// Zero is always an invalid DOM element ID
static const nuInternalID nuInternalIDRoot = 1;		// The root of the DOM tree always has ID = 1


inline int32	nuRealToPos( float real )		{ return int32(real * (1 << nuPosShift)); }
inline int32	nuDoubleToPos( double real )	{ return int32(real * (1 << nuPosShift)); }
inline float	nuPosToReal( int32 pos )		{ return pos * (1.0f / (1 << nuPosShift)); }
inline double	nuPosToDouble( int32 pos )		{ return pos * (1.0 / (1 << nuPosShift)); }

enum nuCloneFlags
{
	nuCloneFlagEvents = 1,		// Include events in clone
};

static const int NU_MAX_TOUCHES = 10;

enum nuMainEvent
{
	nuMainEventInit = 1,
	nuMainEventShutdown,
};

enum nuRenderResult
{
	// SYNC-JAVA
	nuRenderResultNeedMore,
	nuRenderResultIdle
};

#define NU_TAGS_DEFINE \
XX(Body, 1) \
XY(Div) \
XY(END) \

#define XX(a,b) nuTag##a = b,
#define XY(a) nuTag##a,
enum nuTag {
	NU_TAGS_DEFINE
};
#undef XX
#undef XY

struct nuVec2
{
	float x,y;
};
inline nuVec2 NUVEC2(float x, float y) { nuVec2 v = {x,y}; return v; }

struct nuVec3
{
	float x,y,z;
};
inline nuVec3 NUVEC3(float x, float y, float z) { nuVec3 v = {x,y,z}; return v; }

class NUAPI nuPoint
{
public:
	nuPos	X, Y;

	nuPoint() : X(0), Y(0) {}
	nuPoint( nuPos x, nuPos y ) : X(x), Y(y) {}

	void	SetInt( int32 x, int32 y ) { X = nuRealToPos((float) x); Y = nuRealToPos((float) y); }
};

/*
Why does this class have a copy constructor and assignment operator?
Without those, we get data alignment exceptions (signal 7) when running on my Galaxy S3.
I tried explicitly raising the alignment of nuBox to 8 and 16 bytes, but that did not help.
*/
class NUAPI nuBox
{
public:
	nuPos	Left, Right, Top, Bottom;

	nuBox() : Left(0), Right(0), Top(0), Bottom(0) {}
	nuBox( const nuBox& b ) : Left(b.Left), Right(b.Right), Top(b.Top), Bottom(b.Bottom) {}
	nuBox( nuPos left, nuPos top, nuPos right, nuPos bottom ) : Left(left), Right(right), Top(top), Bottom(bottom) {}

	void	SetInt( int32 left, int32 top, int32 right, int32 bottom );
	nuPos	Width() const	{ return Right - Left; }
	nuPos	Height() const	{ return Bottom - Top; }
	void	Offset( int32 x, int32 y ) { Left += x; Right += x; Top += y; Bottom += y; }
	bool	IsInsideMe( const nuPoint& p ) const { return p.X >= Left && p.Y >= Top && p.X < Right && p.Y < Bottom; }

	nuBox&	operator=( const nuBox& b ) { Left = b.Left; Right = b.Right; Top = b.Top; Bottom = b.Bottom; return *this; }
};

class NUAPI nuBoxF
{
public:
	float	Left, Right, Top, Bottom;

	nuBoxF() : Left(0), Right(0), Top(0), Bottom(0) {}
	nuBoxF( float left, float top, float right, float bottom ) : Left(left), Right(right), Top(top), Bottom(bottom) {}
};

struct nuStyleID
{
	uint32		StyleID;

				nuStyleID()				: StyleID(0)	{}
	explicit	nuStyleID( uint32 id )	: StyleID(id)	{}

	operator	uint32 () const { return StyleID; }
};

struct nuJob
{
	void*	JobData;
	void (*JobFunc)( void* jobdata );
};

struct NUAPI nuRenderStats
{
	uint32	Clone_NumEls;		// Number of DOM elements cloned

	void Reset();
};

struct nuGlobalStruct
{
	int							TargetFPS;
	int							NumWorkerThreads;	// Read-only. Set during nuInitialize().

	// Debugging flags. Enabling these should make debugging easier.
	// Some of them may turn out to have a small enough performance hit that you can
	// leave them turned on always.
	// NOPE.. it's just too confusing to have this optional. It's always on.
	//bool						DebugZeroClonedChildList;	// During a document clone, zero out ChildByInternalID before populating. This will ensure that gaps are NULL instead of random memory.

	pvect<nuDocGroup*>			Docs;				// Only Main thread is allowed to touch this.
	TAbcQueue<nuDocGroup*>		DocAddQueue;		// Documents requesting addition
	TAbcQueue<nuDocGroup*>		DocRemoveQueue;		// Documents requesting removal
	TAbcQueue<nuEvent>			EventQueue;			// Global event queue, consumed by the one-and-only UI thread
	TAbcQueue<nuJob>			JobQueue;			// Global job queue, consumed by the worker thread pool
};

NUAPI nuGlobalStruct*	nuGlobal();
NUAPI void				nuInitialize();
NUAPI void				nuShutdown();
NUAPI void				nuProcessDocQueue();
NUAPI void				nuParseFail( const char* msg, ... );
NUAPI void				NUTRACE( const char* msg, ... );
NUAPI void				NUTIME( const char* msg, ... );
#if NU_WIN_DESKTOP
NUAPI void				nuRunWin32MessageLoop();
#endif

// Various tracing options. Uncomment these to enable tracing of that class of events.
//#define NUTRACE_RENDER_ENABLE
//#define NUTRACE_LAYOUT_ENABLE
//#define NUTRACE_EVENTS_ENABLE
//#define NUTRACE_LATENCY_ENABLE

#ifdef NUTRACE_RENDER_ENABLE
	#define NUTRACE_RENDER(msg, ...) NUTIME(msg, ##__VA_ARGS__)
#else
	#define NUTRACE_RENDER(msg, ...) ((void)0)
#endif

#ifdef NUTRACE_LAYOUT_ENABLE
	#define NUTRACE_LAYOUT(msg, ...) NUTIME(msg, ##__VA_ARGS__)
#else
	#define NUTRACE_LAYOUT(msg, ...) ((void)0)
#endif

#ifdef NUTRACE_EVENTS_ENABLE
	#define NUTRACE_EVENTS(msg, ...) NUTIME(msg, ##__VA_ARGS__)
#else
	#define NUTRACE_EVENTS(msg, ...) ((void)0)
#endif

#ifdef NUTRACE_LATENCY_ENABLE
	#define NUTRACE_LATENCY(msg, ...) NUTIME(msg, ##__VA_ARGS__)
#else
	#define NUTRACE_LATENCY(msg, ...) ((void)0)
#endif
