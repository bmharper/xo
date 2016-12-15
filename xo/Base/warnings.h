
#ifdef _MSC_VER

#define XO_DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_USING_FAILED_CALL_VALUE \
	__pragma(warning(push))                                         \
	    __pragma(warning(disable : 6102))

#define XO_DISABLE_CODE_ANALYSIS_WARNINGS_POP \
	__pragma(warning(pop))

#else

// #define __analysis_assume(exp)

#define XO_DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_MIN_MAX_28285
#define XO_DISABLE_CODE_ANALYSIS_WARNINGS_POP

#endif
