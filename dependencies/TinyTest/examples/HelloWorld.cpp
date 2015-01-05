#define TT_BUILD
#include "TinyTest.h"

#include "TinyTestBuild.h"

TT_TEST_FUNC(NULL, NULL, TTSizeSmall, Hello, TTParallelDontCare)
{
	printf("Hello\n");
}

TT_TEST_FUNC(NULL, NULL, TTSizeSmall, Fail, TTParallelDontCare)
{
	printf("About to fail\n");
	TTASSERT(1 + 1 == 3);
}

int main(int argc, char** argv)
{
	return TTRun(argc, argv);
}

