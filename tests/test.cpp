#include "pch.h"

TT_TEST_HOME();

int main( int argc, char** argv )
{
	nuInitialize();

	int retval = 0;
	if ( TTRun( argc, argv, &retval ) )
		return retval;

	nuShutdown();

#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}
