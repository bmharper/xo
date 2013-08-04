
#ifdef _MSC_VER

	#include <CodeAnalysis/Warnings.h>
	//#pragma warning( disable: 6011 )	// NULL pointer access
	#pragma warning( disable: 6308 )	// realloc might return NULL
	#pragma warning( disable: 6031 )	// return value ignored -- I just assume we can create system objects such as critical sections etc.
	#pragma warning( disable: 6211 )	// memory is leaked due to an unhandled c++ exception. If 'new' fails, then we kill the process.
	#pragma warning( disable: 28182 )	// Dereferencing NULL pointer. 'obj' contains the same NULL value as '(foo *)=(obj)' did. This seems to only find false positives on VS 2012.

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_ALL \
	__pragma( warning(push) ) \
	__pragma( warning(disable: ALL_CODE_ANALYSIS_WARNINGS) )

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_LEAK \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6211) )	// memory is leaked due to an unhandled c++ exception

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_UNINITIALIZED_MEM \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6001) )	// using uninitialized memory

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_NULL_DEREF \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6011) )	// dereference null pointer

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_CONSTANT_AND_ZERO \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6237) )	// Expression to the left of && is constant and zero

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_CONSTANT_AND \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6239) )	// Expression to the left of && is constant and non-zero

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_DECOMMIT \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6250) )	// VirtualFree without MEM_RELEASE

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_STACK_SIZE \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6262) )	// function stack size is too big

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_ILL_DEFINED_FOR_LOOP \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6294) )	// Ill-defined for-loop: initial condition does not satisfy test. Loop body not executed

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_32_SHIFT_64_CAST \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6297) )	// 32-bit value is shifted, then cast to 64-bit

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_EXCEPTION_FILTER_1 \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6320) )	// exception filter expression is constant, masking exceptions that you probably don't want to handle

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_REALLOC \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6308) )	// realloc leaks

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_READING_INVALID_BYTES \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6385) )	// Invalid data: accessing xyz, readable size is x bytes, but y bytes might be read

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_WRITING_INVALID_BYTES \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6386) )	// Buffer overrun: accessing.. The writable size is x bytes, but y bytes might be written

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_PARAMETER_COULD_BE_NULL \
	__pragma( warning(push) ) \
	__pragma( warning(disable:6387) )	// '_Param_(n)' could be '0':  this does not adhere to the specification for the function 'foo'.

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_DEREF_NULL_AGAIN \
	__pragma( warning(push) ) \
	__pragma( warning(disable:28182) )	// Dereferencing NULL pointer. 'obj' contains the same NULL value as '(foo *)=(obj)' did

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_MIN_MAX_28285 \
	__pragma( warning(push) ) \
	__pragma( warning(disable:28285) )	// Don't understand this, but looks like internal errors. First saw it when using std::min/max on AbcDate

	#define DISABLE_CODE_ANALYSIS_WARNINGS_POP \
	__pragma( warning(pop) )

#else

	#define __analysis_assume(exp)

	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_ALL
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_LEAK
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_UNINITIALIZED_MEM
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_NULL_DEREF
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_CONSTANT_AND_ZERO
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_CONSTANT_AND
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_DECOMMIT
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_STACK_SIZE
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_ILL_DEFINED_FOR_LOOP
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_32_SHIFT_64_CAST
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_EXCEPTION_FILTER_1
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_REALLOC
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_READING_INVALID_BYTES
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_WRITING_INVALID_BYTES
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_PARAMETER_COULD_BE_NULL
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_DEREF_NULL_AGAIN
	#define DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_MIN_MAX_28285
	#define DISABLE_CODE_ANALYSIS_WARNINGS_POP

#endif


