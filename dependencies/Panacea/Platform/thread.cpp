#include "pch.h"
#include "thread.h"

#ifdef _WIN32

PAPI bool			AbcThreadCreate( AbcThreadFunc threadfunc, void* context, AbcThreadHandle& handle )
{
	handle = CreateThread( NULL, 0, threadfunc, context, 0, NULL );
	return NULL != handle;
}
PAPI bool			AbcThreadJoin( AbcThreadHandle handle )
{
	return WaitForSingleObject( handle, INFINITE ) == WAIT_OBJECT_0;
}
PAPI void			AbcThreadCloseHandle( AbcThreadHandle handle )
{
	CloseHandle( handle );
}
PAPI AbcThreadHandle AbcThreadCurrent()
{
	return GetCurrentThread();
}
PAPI AbcThreadID	AbcThreadCurrentID()
{
	return GetCurrentThreadId();
}
PAPI bool AbcThreadIDEqual( AbcThreadID a, AbcThreadID b )
{
	return a == b;
}
PAPI void			AbcThreadSetPriority( AbcThreadHandle handle, AbcThreadPriority priority )
{
	DWORD p = THREAD_PRIORITY_NORMAL;
	switch( priority )
	{
	case AbcThreadPriorityIdle:				p = THREAD_PRIORITY_IDLE; break;
	case AbcThreadPriorityNormal:			p = THREAD_PRIORITY_NORMAL; break;
	case AbcThreadPriorityHigh:				p = THREAD_PRIORITY_HIGHEST; break;
	case AbcThreadPriorityBackgroundBegin:	p = THREAD_MODE_BACKGROUND_BEGIN; break;
	case AbcThreadPriorityBackgroundEnd:	p = THREAD_MODE_BACKGROUND_END; break;
	}
	SetThreadPriority( handle, p );
}

#else
// linux

PAPI bool			AbcThreadCreate( AbcThreadFunc threadfunc, void* context, AbcThreadHandle& handle )
{
	pthread_attr_t attr;
	handle = 0;
	AbcVerify( 0 == pthread_attr_init( &attr ) );
	bool ok = pthread_create( &handle, &attr, threadfunc, context );
	AbcVerify( 0 == pthread_attr_destroy( &attr ) );
	return ok;
}
PAPI bool			AbcThreadJoin( AbcThreadHandle handle )
{
	void* rval;
	return 0 == pthread_join( handle, &rval );
}
PAPI void			AbcThreadCloseHandle( AbcThreadHandle handle )
{
	// pthread has no equivalent/requirement of this
}
PAPI AbcThreadHandle AbcThreadCurrent()
{
	return pthread_self();
}
PAPI AbcThreadID	AbcThreadCurrentID()
{
	return pthread_self();
}
PAPI bool			AbcThreadIDEqual( AbcThreadID a, AbcThreadID b )
{
	return !!pthread_equal( a, b );
}
PAPI void			AbcThreadSetPriority( AbcThreadHandle handle, AbcThreadPriority priority )
{
	// On linux, pthread_setschedprio is only applicable to threads on the realtime scheduling policy
}

#endif

PAPI bool			AbcThreadJoinAndCloseHandle( AbcThreadHandle handle )
{
	bool result = AbcThreadJoin( handle );
	AbcThreadCloseHandle( handle );
	return result;
}
