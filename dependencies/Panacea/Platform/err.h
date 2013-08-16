#pragma once

#include <assert.h>

// The GCC docs show NORETURN after the function name, so I'm not sure that this works
PAPI			int			AbcPanicMsg(LPCSTR file, int line, LPCSTR msg);
PAPI NORETURN	void		AbcDie();

#define AbcAssert(f)			(void) ((f) || (AbcPanicMsg(__FILE__,__LINE__,#f), AbcDie(), 0) )			// Compiled in all builds
#define AbcVerify(f)			AbcAssert(f)																// Compiled in all builds. Different from AbcAssert to express intent, that this is logic altering.
#define AbcPanic(msg)			(void) (AbcPanicMsg(__FILE__,__LINE__,msg), AbcDie())						// Compiled in all builds
#define AbcPanicSilent()		(void) (AbcPanicMsg(__FILE__,__LINE__,""), AbcDie())						// Compiled in all builds
#define AbcTodo()				(void) (AbcPanicMsg(__FILE__,__LINE__,"Not yet implemented"), AbcDie())		// Compiled in all builds

#ifdef _DEBUG
#define ABCASSERT(f) AbcAssert(f)
#else
#define ABCASSERT(f) ((void)0)
#endif

#define AbcCheckNULL( obj )					if ( !(obj) )	{ AbcPanicSilent(); }
#define AbcCheckAlloc( buf )				if ( !(buf) )	{ AbcPanic("Out of memory"); }
#define AbcMemoryExhausted()				AbcPanic("Out of memory")
PAPI void* AbcMallocOrDie( size_t bytes );
PAPI void* AbcReallocOrDie( void* p, size_t bytes );


// If this returns false, then we are a headless process, for example a windows service.
// This means no user32.dll - no messagebox, etc.
PAPI bool AbcAllowGUI();
PAPI void AbcSetAllowGUI( bool allowGUI );
PAPI bool AbcWindowStationVisible();		// Returns true if the Window Station attached to this process is visible. ie.. FALSE when running as a service.
