#pragma once

namespace AbCore
{
	namespace Test
	{
#ifdef _WIN32
		PAPI void* SpecialAlloc_GuardAtEnd( size_t bytes );
		PAPI void SpecialFree( void* block );

		PAPI int Exception_AccessViolationCount( int* read_violations = NULL, int* write_violations = NULL );
#endif
	}

}
