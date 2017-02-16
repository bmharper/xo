#include <stdio.h>

TT_UNIVERSAL_FUNC bool TTIsRunningUnderMaster()
{
#ifdef _WIN32
#ifdef _UNICODE
	OutputDebugString(GetCommandLine());
	//printf( "IsRunningUnderMaster? %d\n", wcsstr( GetCommandLine(), L" test =" ) != nullptr );
	return wcsstr(GetCommandLineW(), L" test =") != nullptr;
#else
	return strstr(GetCommandLine(), " test =") != nullptr;
#endif
#else
	bool result = true;
	FILE* f = fopen("/proc/self/cmdline", "r");
	if (f != nullptr)
	{
		char buf[1024];
		int len = fread(buf, 1, 1023, f);
		buf[len] = 0;
		result = strstr(buf, " test =") != nullptr;
		fclose(f);
	}
	return result;
#endif
}

TT_UNIVERSAL_FUNC void TTAssertFailed(const char* exp, const char* filename, int line_number, bool die)
{
	printf("Test Failed:\nExpression: %s\nFile: %s\nLine: %d", exp, filename, line_number);
	fflush(stdout);
	fflush(stderr);

	if (TTIsRunningUnderMaster())
	{
#ifdef _WIN32
		// Use TerminateProcess instead of exit(), because we don't want any C++ cleanup, or CRT cleanup code to run.
		// Such "at-exit" functions are prone to popping up message boxes about resources that haven't been cleaned up, but
		// at this stage, that is merely noise.
		TerminateProcess(GetCurrentProcess(), 1);
#else
		_exit(1);
#endif
	}
	else
	{
#ifdef _WIN32
		__debugbreak();
#else
		__builtin_trap();
#endif
	}
}
