
#ifdef _MSC_VER

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_USING_FAILED_CALL_VALUE \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6102) )

	#define DISABLE_CODE_ANALYSIS_WARNINGS_POP \
	__pragma( warning(pop) )

#else
	
	#define __analysis_assume(exp)

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_MIN_MAX_28285
	#define DISABLE_CODE_ANALYSIS_WARNINGS_POP

#endif


