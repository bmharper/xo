#include "pch.h"

TT_TEST_HOME();

static int __cdecl hook( int allocType, void *pvData, size_t size, int blockUse, long request, const unsigned char *filename, int fileLine )
{
	return TRUE;
}

int main( int argc, char** argv )
{
	_CrtSetAllocHook( hook );

	nuInitialize();

	// Uncomment this line to run tests on DirectX
	//nuGlobal()->PreferOpenGL = false;

	int retval = 0;
	TTRun( argc, argv, &retval );

	nuShutdown();

#ifdef _WIN32
	TTASSERT( _CrtDumpMemoryLeaks() == FALSE );
#endif
	return retval;
}
