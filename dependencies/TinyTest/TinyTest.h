#pragma once

/*

Usage
-----

1. #define TT_MODULE_NAME as the name of your application, for example "#define TT_MODULE_NAME imqstool".
2. #include <TinyTest/TinyTest.h> in your project.
3. Inside one of your cpp files, write TT_TEST_HOME();
4. Inside int main( int argc, char** argv ), write:

	int testretval = 0;
	if ( TTRun( argc, argv, &testretval ) )
		return testretval;

5. In other cpp files, write TT_TEST_FUNC( initfunc, teardown, size, testname, parallel ) to define test functions, for example
	
	void InitSandbox() {...}
	TT_TEST_FUNC( &InitSandbox, NULL, TTSizeSmall, hello, TT_PARALLEL_DONTCARE )
	{
		TTASSERT( 1 + 1 == 2 );
	}

	Typically, you'll define a high level macro that wraps TT_TEST_FUNC, for example:
	#define TESTFUNC(x) TT_TEST_FUNC(MySetup, MyTearDown, TTSizeSmall, x, TT_PARRALLEL_DONTCARE)
	
*/

enum TTModes
{
	TTModeBlank,
	TTModeAutomatedTest
};

// Test size classification.
// The idea is that you run 'small' tests on your continuous integration, and 'large' tests nightly.
enum TTSizes
{
	TTSizeSmall,	// Runs for less than 1 minute
	TTSizeLarge		// Runs for more than 1 minute
};
#define TT_SIZE_SMALL_NAME		"small"
#define TT_SIZE_LARGE_NAME		"large"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test parallelization

// This test is independent of any other test, and doesn't need dedicated hardware resources
#define TT_PARALLEL_DONTCARE	""

// This test is independent of any other test, but do not run it on the same core as another test
// Single-threaded performance tests should use this, so that they don't suffer from sharing
// execution resources with another hyperthread.
#define TT_PARALLEL_WHOLECORE	"!core"

// This test must be run on its own. It could be a performance test that utilizes all cores,
// or maybe it uses TCP ports, etc.
#define TT_PARALLEL_SOLO		"!solo"

/*
Aside from these predefined values, you can specify any string as a group.
No two tests from the same group will ever be run.
A test can belong to multiple groups - separate them with whitespace

For example, a test might be
  
  "http ctempdb"

This hypothetical test is one that serves content on port 80 (hence the http), and it uses
an sqlite DB on C:\temp\db.sqlite.

If you had another port labeled as

  "http"

then these two tests would never be run simultaneously.

Do not use an exclamation mark (!) in your group names - because such names are reserved
for use by tinytest as special test groups.

*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
#define TT_TOKEN_ALL_TESTS		"all"
#define TT_PREFIX_RUNNER_PID	"ttpid="
#define TT_IPC_CMD_SUBP_RUN		"subprocess-launch"
#define TT_LIST_LINE_1			"Tests:"
#define TT_LIST_LINE_2			"  " TT_TOKEN_ALL_TESTS " (run all tests)"
#define TT_LIST_LINE_END		TT_LIST_LINE_2

static const int TT_IPC_MEM_SIZE = 1024;
static const int TT_MAX_CMDLINE_ARGS = 10;	// Maximum number of command-line arguments that TTArgs() will return

TT_NORETURN void TTAssertFailed( const char* expression, const char* filename, int line_number, bool die );

void			TTLog( const char* msg, ... );
bool			TTIsDead();								// Returns true if a TT assertion has failed, and exit(1) has been called
TTModes			TTMode();								// Returns TTModeAutomatedTest if this is an automated test run by a build bot or something like that
void			TTSetProcessIdle();						// Sets the process' priority to IDLE. This is just convenient if you're running tests while working on your dev machine.
void			TTNotifySubProcess( unsigned int pid );	// Notify the test runner that you have launched a sub-process
void			TTListTests( const TT_TestList& tests );
unsigned int	TTGetProcessID();						// Get Process ID of this process
void			TTSleep( unsigned int milliseconds );
char**			TTArgs();								// Retrieve the command-line parameters that were passed in to this test specifically. Terminates with a NULL.

// Generate the filename used for IPC between the executor and the tested app
// up: If true, then this is the channel from tested app to test harness app (aka the executor)
//     If false, then this is the channel from the executor to the tested app
void		TT_IPC_Filename( bool up, unsigned int executorPID, unsigned int testedPID, char (&filename)[256] );

// Write an IPC message. Fails by killing the app.
void		TT_IPC_Write_Raw( char (&filename)[256], const char* msg );

// Read an IPC message. Returns true if a full message was found and consumed.
bool		TT_IPC_Read_Raw( unsigned int waitMS, char (&filename)[256], char (&msg)[TT_IPC_MEM_SIZE] );

// Returns true if TT handled this call. In this case, call exit( *retval ). If false returned, continue as usual.
// Use the TTRun macro to call this
bool		TTRun_Internal( const TT_TestList& tests, int argc, char** argv, int* retval );
bool		TTRun_InternalW( const TT_TestList& tests, int argc, wchar_t** argv, int* retval );

#define TTRun( argc, argv, retval ) TTRun_Internal( TT_TESTS_ALL, argc, argv, retval )
#define TTRunW( argc, argv, retval ) TTRun_InternalW( TT_TESTS_ALL, argc, argv, retval )

// Generally I use this so that I don't have to uncomment the full suite of tests when I'm fooling around with one small component, and want to run only its tests from the IDE.
inline bool TTIsAutomatedTest() { return TTMode() == TTModeAutomatedTest; }

struct TTException
{
	static const size_t MsgLen = 256;

	char	Msg[MsgLen];
	char	File[MsgLen];
	int		Line;

			TTException( const char* msg = NULL, const char* file = NULL, int line = 0 );
	void	CopyStr( size_t n, char* dst, const char* src );
	void	Set( const char* msg = NULL, const char* file = NULL, int line = 0 );
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
	const char*		Parallel;
	TTSizes			Size;

	TT_Test( TTFuncBlank init, TTFuncBlank teardown, TTFuncBlank func, TTSizes size, const char* name, const char* parallel ) : Init(init), Teardown(teardown),  Blank(func), Size(size), Name(name), Parallel(parallel) {}
	TT_Test( const TT_Test& t ) { memcpy(this, &t, sizeof(t)); }

	static int CompareName( const TT_Test* a, const TT_Test* b )
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
	void			Add( const TT_Test& t );
	int				size() const { return Count; }
	const TT_Test&	operator[]( int i ) const { return List[i]; }
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

// You define this is ONE of your cpp files.
// The file that has this defined, may not define any tests of its own. This is a linker issue that I haven't solved (nor put any effort into solving).
#define TT_TEST_HOME() TT_TestList TT_TESTS_ALL

#define TT_LIST_TESTS() TTListTests( TT_TESTS_ALL );

#define TTLOG(msg)			TTLog(msg)
#define TTASSERT(exp)		(void)( (!!(exp)) || (TTAssertFailed(#exp, __FILE__, __LINE__, true), 0) )
#define TTASSERTEX(exp)		(void)( (!!(exp)) || (throw TTException(#exp, __FILE__, __LINE__), 0) )
#define TTASSEQ(a, b)		{ if (!((a) == (b)))  { TTAssertFailed( fmt("'%v' == '%v' (%v == %v)", (a), (b), #a, #b), __FILE__, __LINE__, true ); } }
