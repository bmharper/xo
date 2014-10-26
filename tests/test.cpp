#include "pch.h"

// In order to debug tests, launch the process as "test.exe test :TheTestName"

#include "../dependencies/TinyTest/TinyTestBuild.h"

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

	// Make clear color a predictable pink, no matter what the default is
	xoGlobal()->ClearColor.Set( 255, 30, 240, 255 );

	int retval = 0;
	TTRun( argc, argv, &retval );

	xoShutdown();

#ifdef _WIN32
	TTASSERT( _CrtDumpMemoryLeaks() == FALSE );
#endif
	return retval;
}
