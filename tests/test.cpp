#include "pch.h"

TT_TEST_HOME();

int main( int argc, char** argv )
{
	nuInitialize();

	int retval = 0;
	TTRun( argc, argv, &retval );

	nuShutdown();

#ifdef _WIN32
	TTASSERT( _CrtDumpMemoryLeaks() == FALSE );
#endif
	return retval;
}
