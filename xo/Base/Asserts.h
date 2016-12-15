#pragma once

namespace xo {

XO_API void XO_NORETURN Die(const char* file, int line, const char* msg);

#define XO_DIE() Die(__FILE__, __LINE__, "")
#define XO_DIE_MSG(msg) Die(__FILE__, __LINE__, msg)

// NOTE: This is compiled in all builds (Debug, Release)
#define XO_ASSERT(f) (void) ((f) || (Die(__FILE__,__LINE__,#f), 0) )

#ifdef _DEBUG
#define XO_VERIFY(x) XO_ASSERT(x)
#define XO_DEBUG_ASSERT(x) XO_ASSERT(x)
#else
#define XO_VERIFY(x) ((void) (x))
#define XO_DEBUG_ASSERT(x) ((void) 0)
#endif

#define XO_TODO XO_DIE_MSG("not yet implemented")
#define XO_TODO_STATIC static_assert(false, "Implement me");

}