#include "pch.h"

TT_TEST_HOME();

static int __cdecl hook( int allocType, void *pvData, size_t size, int blockUse, long request, const unsigned char *filename, int fileLine )
{
	return TRUE;
}

int main( int argc, char** argv )
{
	_CrtSetAllocHook( hook );

	xoInitialize();

	// Uncomment this line to run tests on DirectX
	//xoGlobal()->PreferOpenGL = false;

	int retval = 0;
	TTRun( argc, argv, &retval );

	xoShutdown();

#ifdef _WIN32
	TTASSERT( _CrtDumpMemoryLeaks() == FALSE );
#endif
	return retval;
}
