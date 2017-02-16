#pragma once

/*

Instructions
------------
1. Create an application (not a dynamic library) that will host your tests.
2. #define TT_MODULE_NAME as the name of your application, and include <TinyTest.h> into all files
	that will define tests in them. For example:

	#define TT_MODULE_NAME gizmo
	#include <TinyTest/TinyTest.h>

3. Inside one of your cpp files, #include <TinyTest/TinyTestBuild.h>. This particular .cpp, which is typically
	your "main.cpp" file, is not allowed to define any tests of its own (this is a linker issue that I have not bothered to solve yet).

4. Inside int main(int argc, char** argv), write:

	return TTRun(argc, argv); // This function can only be called once, because it frees the test list before returning

5. In other cpp files, write TT_TEST_FUNC(initfunc, teardown, size, testname, parallel) to define test functions, for example

	void InitSandbox() { // ... }

	TT_TEST_FUNC(&InitSandbox, nullptr, TTSizeSmall, hello, TTParallelDontCare)
	{
		TTASSERT(1 + 1 == 2);
	}

	Typically, you'll define a high level macro that wraps TT_TEST_FUNC, for example:
	#define TESTFUNC(x) TT_TEST_FUNC(MySetup, MyTearDown, TTSizeSmall, x, TTParallelDontCare)

Debugging Tests
---------------
In order to debug a unit test, launch the the test program like so:

	your_test_program.exe :foo

Ordinarily, if you want to run the test named "foo", you don't include the colon before it's name.
The colon is a special instruction to the test system that tells it that you're running this under
a debugger. This causes the test system to run all code within a single process, which makes debugging
simpler.

Including tests in dynamic libraries
------------------------------------
It is possible to include calls to TTASSERT and TTIsRunningUnderMaster() from dynamic libraries
that are linked into your test application. In order to do this, you must define
TT_EXTERNAL_MODULE before including "TinyTest.h", from your dynamic library.
For example:

	#define TT_MODULE_NAME MyDynLibrary
	#define TT_EXTERNAL_MODULE
	#include <TinyTest/TinyTest.h>

	void SomeLibraryFunction()
	{
		TTASSERT(...); // Include testing code inside your application logic.
	}

*/

#include <string>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test size classification.
enum TTSizes
{
	TTSizeSmall,	// Runs for less than 5 minutes
	TTSizeLarge		// Runs for less than 20 minutes
};
#define TT_SIZE_SMALL_NAME		"small"
#define TT_SIZE_LARGE_NAME		"large"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test parallelization
enum TTParallel
{
	// Null value used as error code
	TTParallel_Invalid,

	// This test is independent of any other test, and doesn't need dedicated hardware resources
	TTParallelDontCare,

	// This test is independent of any other test, but do not run it on the same core as another test.
	// Single-threaded performance tests should use this, so that they don't suffer from sharing
	// execution resources with another hyperthread.
	TTParallelWholeCore,

	// This test must be run on its own. It could be a performance test that utilizes all cores,
	// or maybe it uses TCP ports, etc.
	TTParallelSolo,
};
#define TT_PARALLEL_DONTCARE	"any"
#define TT_PARALLEL_WHOLECORE	"core"
#define TT_PARALLEL_SOLO		"solo"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TT_Test;
struct TT_TestList;
struct TTException;

#ifdef _MSC_VER
#define TT_NORETURN __declspec(noreturn)
#else
#define TT_NORETURN __attribute__ ((noreturn))
#endif

// Define these exactly, so that the test host app (tinytest_app) can suck the test names out of the test executor exe
// We define 'all' as a keyword, to avoid shell expansion of wildcards on unix. Not sure whether this is actually necessary.
#define TT_TOKEN_INVOKE			"test"
#define TT_TOKEN_INVOKE_MASTER	"testmaster"
#define TT_TOKEN_ALL_TESTS		"all"
#define TT_PREFIX_RUNNER_PID	"ttpid="
#define TT_IPC_CMD_SUBP_RUN		"subprocess-launch"
#define TT_PREFIX_TESTDIR		"test-dir="
#define TT_LIST_LINE_1			"Tests:"
#define TT_LIST_LINE_2			"  " TT_TOKEN_ALL_TESTS " (run all tests)"
#define TT_LIST_LINE_END		TT_LIST_LINE_2

static const int TT_IPC_MEM_SIZE = 1024;
static const int TT_MAX_CMDLINE_ARGS = 10;	// Maximum number of command-line arguments that TTArgs() will return
static const int TT_TEMP_DIR_SIZE = 2048;	// The maximum number of characters in path name.

TT_NORETURN void TTAssertFailed(const char* expression, const char* filename, int line_number, bool die);

bool			TTIsRunningUnderMaster();				// Return true if this process was launched by a master test process (implies command line of test =TheTestName)
void			TTLog(const char* msg, ...);
void			TTSetProcessIdle();						// Sets the process' priority to IDLE. This is just convenient if you're running tests while working on your dev machine.
void			TTNotifySubProcess(unsigned int pid);	// Notify the test runner that you have launched a sub-process
void			TTLaunchChildProcess(const char* cmd, const char** args);	// Helper function to launch a child process. Calls TTNotifySubProcess for you.
unsigned int	TTGetProcessID();						// Get Process ID of this process
std::string		TTGetProcessPath();
void			TTSleep(unsigned int milliseconds);
char**			TTArgs();								// Retrieve the command-line parameters that were passed in to this test specifically. Terminates with a NULL.
void			TTSetTempDir(const char* tmp);			// Set the global test directory parameter.
std::string		TTGetTempDir();							// Get the global test temporary directory

std::string		TT_ToString(int v);
std::string		TT_ToString(double v);
std::string		TT_ToString(std::string v);

// Generate the filename used for IPC between the executor and the tested app
// up: If true, then this is the channel from tested app to test harness app (aka the executor)
//     If false, then this is the channel from the executor to the tested app
void		TT_IPC_Filename(bool up, unsigned int executorPID, unsigned int testedPID, char (&filename)[256]);

// Write an IPC message. Fails by killing the app.
void		TT_IPC_Write_Raw(char (&filename)[256], const char* msg);

// Read an IPC message. Returns true if a full message was found and consumed.
bool		TT_IPC_Read_Raw(unsigned int waitMS, char (&filename)[256], char (&msg)[TT_IPC_MEM_SIZE]);

// Returns a process exit code (0 when all tests pass)
// Before returning, these functions wipe all tests from the 'tests' parameter. This frees memory, causing zero memory leaks.
// This means you can only run these functions once.
// Use the TTRun macro to call one of these functions.
int			TTRun_Wrapper(TT_TestList& tests, int argc, char** argv);
int			TTRun_WrapperW(TT_TestList& tests, int argc, wchar_t** argv);

// You can only run this function once.
#define TTRun( argc, argv ) TTRun_Wrapper( TT_TESTS_ALL, argc, argv )
#define TTRunW( argc, argv ) TTRun_WrapperW( TT_TESTS_ALL, argc, argv )

#ifdef _WIN32
#define TTRunX TTRunW
#else
#define TTRunX TTRun
#endif

struct TTException
{
	static const size_t MsgLen = 256;

	char	Msg[MsgLen];
	char	File[MsgLen];
	int		Line;

	TTException(const char* msg = nullptr, const char* file = nullptr, int line = 0);
	void	CopyStr(size_t n, char* dst, const char* src);
	void	Set(const char* msg = nullptr, const char* file = nullptr, int line = 0);
};

// Test function
typedef void (*TTFuncBlank)();

// A test entry point
struct TT_Test
{
	TTFuncBlank		Init;
	TTFuncBlank		Teardown;
	TTFuncBlank		Blank;
	const char*		Name;
	TTSizes			Size;
	TTParallel		Parallel;

	TT_Test(TTFuncBlank init, TTFuncBlank teardown, TTFuncBlank func, TTSizes size, const char* name, TTParallel parallel) : Init(init), Teardown(teardown),  Blank(func), Size(size), Name(name), Parallel(parallel) {}
	TT_Test(const TT_Test& t) { *this = t; }

	static int CompareName(const TT_Test* a, const TT_Test* b)
	{
		return strcmp(a->Name, b->Name);
	}
};

struct TT_TestList
{
	TT_Test*	List;
	int			Capacity;
	int			Count;

	TT_TestList();
	~TT_TestList();
	void			Add(const TT_Test& t);
	void			Clear();
	int				size() const { return Count; }
	const TT_Test&	operator[](int i) const { return List[i]; }
};

// Define a struct with a constructor that adds this test element to the process-global TT_TESTS_ALL vector. Then, instantiate one of those structs.
// Generally, doing heap allocs before main() has entered is not desirable, but hopefully this won't cause any strange problems.
// Wrap the structure in an anonymous namespace, to avoid any linker symbol pollution.
#define TT_TEST_FUNC(init, teardown, size, func, parallel) \
	void test_##func(); \
	namespace { \
		struct func##_adder { func##_adder() { TT_TESTS_ALL.Add( TT_Test(init, teardown, test_##func, size, #func, parallel) ); }  }; \
		func##_adder func##_do_add; \
	} \
	void test_##func()

// This two step dance is required to get macro expansion to occur before hitting the token pasting operator, which does not do macro expansion.
#define TT_NAME_COMBINER_AB_2(a, b) a ## b
#define TT_NAME_COMBINER_AB_1(a, b) TT_NAME_COMBINER_AB_2(a, b)

#define TT_TESTS_ALL TT_NAME_COMBINER_AB_1(TT_MODULE_NAME, _Tests)

// One of these per link unit (exe/dll)
extern TT_TestList TT_TESTS_ALL;

#define TTLOG(msg)			TTLog(msg)
#define TTASSERT(exp)		(void)( (!!(exp)) || (TTAssertFailed(#exp, __FILE__, __LINE__, true), 0) )
#define TTASSERTEX(exp)		(void)( (!!(exp)) || (throw TTException(#exp, __FILE__, __LINE__), 0) )
#define TTASSEQ(a, b)		{ if (!((a) == (b)))  { TTAssertFailed((std::string("'") + TT_ToString(a) + "' == '" + TT_ToString(b) + "' (" + #a + " == " + #b + ")").c_str(), __FILE__, __LINE__, true ); } }

// "External Module" is typically a DLL that needs to know a little bit about its testing environment,
// or wants to call TTASSERT.
#ifdef TT_EXTERNAL_MODULE
#define TT_UNIVERSAL_FUNC inline
#include "TinyAssert.cpp"
#endif