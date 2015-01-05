/*
This file contains the utility functions that are used by testing code.
*/
#ifdef _WIN32
#include <direct.h>
#include <VersionHelpers.h>
#include <tchar.h>
#pragma warning(push)				// TinyLib-TOP
#pragma warning(disable: 6387)
#pragma warning(disable: 28204)
#include "StackWalker.h"
#else
#include <unistd.h>
#define _T(x) x
#endif

//typedef long long int64;
//typedef unsigned int uint;

#ifdef _WIN32
#define DIRSEP				'\\'
#define EXE_EXTENSION		".exe"
#define DEFAULT_TEMP_DIR	"c:\\temp"
#else
#define DIRSEP				'/'
#define EXE_EXTENSION		""
#define DEFAULT_TEMP_DIR	"/tmp"
#endif

#ifndef MAX_PATH
#define MAX_PATH			255
#endif

static bool			IsExecutingUnderGuidance = false;
static int			MasterPID = 0;
static char*		TestCmdLineArgs[TT_MAX_CMDLINE_ARGS + 1]; // +1 for the null terminator
static char			TestTempDir[TT_TEMP_DIR_SIZE + 1];

// These are defined inside TinyMaster.cpp
int		TTRun_Internal(const TT_TestList& tests, int argc, char** argv);

void TTException::CopyStr(size_t n, char* dst, const char* src)
{
	dst[0] = 0;
	if (src)
	{
		size_t i = 0;
		for (; src[i] && i < n; i++)
			dst[i] = src[i];
		if (i < n) dst[i] = 0;
		dst[n - 1] = 0;
	}
}

void TTException::Set(const char* msg, const char* file, int line)
{
	CopyStr(MsgLen, Msg, msg);
	CopyStr(MsgLen, File, file);
	Line = line;
}

TTException::TTException(const char* msg, const char* file, int line)
{
	TTAssertFailed(msg, file, line, false);
	Set(msg, file, line);
}

TT_TestList::TT_TestList()
{
	// TT_TestList constructor must do nothing, but rely instead on zero-initialization of static data (which is part of the C spec).
	// The reason is because constructor initialization order is undefined, and you will end up in this
	// constructor some time AFTER objects have already started adding themselves to the list.
}

TT_TestList::~TT_TestList()
{
	Clear();
}

void TT_TestList::Add(const TT_Test& t)
{
	if (Count == Capacity)
	{
		int newcap = Capacity * 2;
		if (newcap < 8) newcap = 8;
		TT_Test* newlist = (TT_Test*) malloc(sizeof(List[0]) * newcap);
		if (!newlist)
		{
			printf("TT_TestList out of memory\n");
			exit(1);
		}
		memcpy(newlist, List, sizeof(List[0]) * Count);
		free(List);
		Capacity = newcap;
		List = newlist;
	}
	List[Count++] = t;
}

void TT_TestList::Clear()
{
	free(List);
	List = NULL;
	Count = 0;
	Capacity = 0;
}

void TTSetProcessIdle()
{
#ifdef _WIN32
	bool isVista = false;
#ifdef NTDDI_WINBLUE
	isVista = IsWindowsVistaOrGreater();
#else
	OSVERSIONINFO inf;
	inf.dwOSVersionInfoSize = sizeof(inf);
	GetVersionEx(&inf);
	isVista = inf.dwMajorVersion >= 6;
#endif
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
	if (isVista)
		SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);   // Lowers IO priorities too
#else
	fprintf(stderr, "TTSetProcessIdle not implemented\n");
#endif
}

bool TTFileExists(const char* f)
{
#ifdef _WIN32
	return GetFileAttributesA(f) != INVALID_FILE_ATTRIBUTES;
#else
	return access(f, F_OK) != -1;
#endif
}

void TTWriteWholeFile(const char* filename, const char* str)
{
	FILE* h = fopen(filename, "wb");
	fwrite(str, strlen(str), 1, h);
	fclose(h);
}

void TTLog(const char* msg, ...)
{
	char buff[8192];
	va_list va;
	va_start(va, msg);
	vsprintf(buff, msg, va);
	va_end(va);

	{
		char tz[128];
		time_t t;
		time(&t);
		tm t2;
#ifdef _WIN32
		_localtime64_s(&t2, &t);
#else
		localtime_r(&t, &t2);
#endif
		strftime(tz, sizeof(tz), "%Y-%m-%d %H:%M:%S  ", &t2);
		//fwrite( tz, strlen(tz), 1, f );
		fputs(tz, stdout);

		//fwrite( buff, strlen(buff), 1, f );
		buff[sizeof(buff) - 1] = 0;
		fputs(buff, stdout);
		//fclose(f);
	}
}

static const char* ParallelName(TTParallel p)
{
	switch (p)
	{
	case TTParallelDontCare: return TT_PARALLEL_DONTCARE;
	case TTParallelWholeCore: return TT_PARALLEL_WHOLECORE;
	case TTParallelSolo: return TT_PARALLEL_SOLO;
	}
	return NULL;
}

unsigned int TTGetProcessID()
{
#ifdef _WIN32
	return (uint) GetCurrentProcessId();
#else
	return (uint) getpid();
#endif
}

std::string TTGetProcessPath()
{
	char buf[1024];
#ifdef _WIN32
	GetModuleFileNameA(NULL, buf, 1024);
#else
	buf[readlink("/proc/self/exe", buf, 1024 - 1)] = 0;	// untested
#endif
	buf[1023] = 0;
	return buf;
}

void TTSleep(unsigned int milliseconds)
{
#ifdef _WIN32
	Sleep(milliseconds);
#else
	int64 nano = milliseconds * (int64) 1000;
	timespec t;
	t.tv_nsec = nano % 1000000000;
	t.tv_sec = (nano - t.tv_nsec) / 1000000000;
	nanosleep(&t, NULL);
#endif
}

char** TTArgs()
{
	return TestCmdLineArgs;
}

void TTSetTempDir(const char* tmp)
{
	strncpy(TestTempDir, tmp + strlen(TT_PREFIX_TESTDIR), TT_TEMP_DIR_SIZE);
	TestTempDir[TT_TEMP_DIR_SIZE] = 0;
	size_t len = strlen(TestTempDir);
	if (len != 0)
	{
		// Fight the shell's interpretation of:
		// test.exe "two=c:\my\dir\"
		// The Windows shell (or perhaps command line argument parser) will treat
		// that last backslash as an escape, and end up including the quote character in the command line parameter.
		// So here we chop off the last character, if it happens to be a quote.
		if (TestTempDir[len - 1] == '"')
			TestTempDir[len - 1] = 0;
	}

	// ensure our temp dir path ends with a dir separator
	len = strlen(TestTempDir);
	if (len != 0 && TestTempDir[len - 1] != DIRSEP)
	{
		TestTempDir[len] = DIRSEP;
		TestTempDir[len + 1] = 0;
	}
}

std::string TTGetTempDir()
{
	return TestTempDir;
}

const int MAXARGS = 30;

int TTRun_WrapperW(TT_TestList& tests, int argc, wchar_t** argv)
{
	const int ARGSIZE = 400;
	char* argva[MAXARGS];

	if (argc >= MAXARGS - 1) { printf("TTRun_InternalW: Too many arguments\n"); return 1; }
	for (int i = 0; i < argc; i++)
	{
		argva[i] = (char*) malloc(ARGSIZE);
		wcstombs(argva[i], argv[i], ARGSIZE);
	}
	argva[argc] = NULL;

	int res = TTRun_Wrapper(tests, argc, argva);

	for (int i = 0; i < argc; i++)
		free(argva[i]);

	return res;
}

int TTRun_Wrapper(TT_TestList& tests, int argc, char** argv)
{
	int res = TTRun_Internal(tests, argc, argv);
	// ensure that there are no memory leaks by the time we return
	tests.Clear();
	return res;
}

#ifndef _WIN32
// Dummy
class StackWalker
{
protected:
	virtual void OnOutput(bool isCallStackProper, const char* szText) {}
};
inline bool IsDebuggerPresent() { return false; }
#endif

class StackWalkerToConsole : public StackWalker
{
protected:
	virtual void OnOutput(bool isCallStackProper, const char* szText)
	{
		if (isCallStackProper)
			printf("%s", szText);
	}
};

static void Die()
{
#ifdef _WIN32
	if (IsDebuggerPresent())
		__debugbreak();
	TerminateProcess(GetCurrentProcess(), 1);
#else
	_exit(1);
#endif
}

#ifdef _WIN32
static LONG WINAPI TTExceptionHandler(EXCEPTION_POINTERS* exInfo)
{
	if (exInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
	{
		fputs("Stack overflow\n", stdout);
	}
	else
	{
		// The words "Stack Trace" must appear at the top, otherwise Jenkins
		// is likely to discard too much of your trace, or your exception information
		StackWalkerToConsole sw;  // output to console
		printf("------- Unhandled Exception and Stack Trace ------- \n"
			   "   Code:    0x%8.8X\n"
			   "   Flags:   %u\n"
			   "   Address: 0x%p\n",
			   exInfo->ExceptionRecord->ExceptionCode,
			   exInfo->ExceptionRecord->ExceptionFlags,
			   exInfo->ExceptionRecord->ExceptionAddress);
		fflush(stdout);
		printf("-------\n");
		fflush(stdout);
		sw.ShowCallstack(GetCurrentThread(), exInfo->ContextRecord);
		fflush(stdout);
	}
	fflush(stdout);
	TerminateProcess(GetCurrentProcess(), 33);
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static void TTPurecallHandler()
{
	printf("Undefined virtual function called (purecall)\n");
	fflush(stdout);
	Die();
}

#ifdef _WIN32
static void TTInvalidParameterHandler(const wchar_t * expression, const wchar_t * function, const wchar_t * file, unsigned int line, uintptr_t pReserved)
{
	//if ( expression && function && file )
	//	fputs( "CRT function called with invalid parameters: %S\n%S\n%S:%d\n", expression, function, file, line );
	//else
	fputs("CRT function called with invalid parameters\n", stdout);
	fflush(stdout);
	Die();
}
#endif

static void TTSignalHandler(int signal)
{
	printf("Signal %d called", signal);
	fflush(stdout);
	Die();
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 6031) // return value ignored for mkdir
#endif

void TT_IPC_Filename(bool up, unsigned int masterPID, unsigned int testedPID, char (&filename)[256])
{
#ifdef _WIN32
	// We prefer c:\temp over obscure stuff inside the "proper" temp directory, because that tends to fill up with stuff you never see.
	_mkdir("c:\\temp");
	sprintf(filename, "c:\\temp\\tiny_test_ipc_%s_%u_%u", up ? "up" : "down", masterPID, testedPID);
#else
	sprintf(filename, "/tmp/tiny_test_ipc_%s_%u_%u", up ? "up" : "down", masterPID, testedPID);
#endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

bool TT_IPC_Read_Raw(unsigned int waitMS, char (&filename)[256], char (&msg)[TT_IPC_MEM_SIZE])
{
	memset(msg, 0, sizeof(msg));
	FILE* file = fopen(filename, "rb");
	if (file == NULL)
	{
		TTSleep(waitMS);
		return false;
	}

	uint length = 0;
	bool good = false;
	if (fread(&length, sizeof(length), 1, file) == 1)
	{
		if (length > TT_IPC_MEM_SIZE)
			printf("Error: TT_IPC_Read_Raw encountered a block larger than TT_IPC_MEM_SIZE\n");
		else
			good = fread(&msg, length, 1, file) == 1;
	}

	fclose(file);
	// We acknowledge receipt by deleting the file
	if (good)
		remove(filename);
	return good;
}

void TT_IPC_Write_Raw(char (&filename)[256], const char* msg)
{
	char buf[512];
	unsigned int length = (unsigned int) strlen(msg);
	FILE* file = fopen(filename, "wb");
	if (file == NULL)
	{
		sprintf(buf, "TT_IPC failed to open %s", filename);
		TTAssertFailed(buf, "internal", 0, true);
	}
	fwrite(&length, sizeof(length), 1, file);
	fwrite(msg, length, 1, file);
	fclose(file);
	file = NULL;
	// The other side acknowledges this transmission by deleting the file
	uint sleepMS = 30;
	for (uint nwait = 0; nwait < 5 * 1000 / sleepMS; nwait++)
	{
		TTSleep(sleepMS);
		file = fopen(filename, "rb");
		if (file == NULL)
			return;
		fclose(file);
	}
	sprintf(buf, "TT_IPC executor failed to acknowledge read on %s", filename);
	TTAssertFailed(buf, "internal", 0, true);
}

static void TT_IPC(const char* msg, ...)
{
	// This is one-way communication, from the tested process to the master process

	// Disable IPC if we're not being run as an automated test.
	if (MasterPID == 0)
		return;

	char filename[256];
	TT_IPC_Filename(true, MasterPID, TTGetProcessID(), filename);

	char buf[8192];
	va_list va;
	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);

	TT_IPC_Write_Raw(filename, buf);
}

void TTNotifySubProcess(unsigned int pid)
{
	TT_IPC("%s %u", TT_IPC_CMD_SUBP_RUN, pid);
}


#ifdef _MSC_VER
#pragma warning( pop )	// TinyLib-TOP
#endif
