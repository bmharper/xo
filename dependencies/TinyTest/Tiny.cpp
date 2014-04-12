#include "pch.h"
#include "TinyTest.h"
#ifdef _WIN32
#include <direct.h>
#include "StackWalker.h"
#else
#include <unistd.h>
#endif

//typedef long long int64;
//typedef unsigned int uint;

#ifdef _WIN32
#define DIRSEP			'\\'
#define EXE_EXTENSION	".exe"
#else
#define DIRSEP			'/'
#define EXE_EXTENSION	""
#define MAX_PATH		255
#endif

const wchar_t* EvMarkerDead = L"TinyTest_Marker_Dead";

static bool			IsExecutingUnderGuidance = false;
static int			ExecutorPID = 0;
static char*		TestCmdLineArgs[TT_MAX_CMDLINE_ARGS + 1]; // +1 for the null terminator

void TTException::CopyStr( size_t n, char* dst, const char* src )
{
	dst[0] = 0;
	if ( src )
	{
		size_t i = 0;
		for ( ; src[i] && i < n; i++ )
			dst[i] = src[i];
		if ( i < n ) dst[i] = 0;
		dst[n - 1] = 0;
	}
}

void TTException::Set( const char* msg, const char* file, int line )
{
	CopyStr( MsgLen, Msg, msg );
	CopyStr( MsgLen, File, file );
	Line = line;
}

TTException::TTException( const char* msg, const char* file, int line )
{
	TTAssertFailed( msg, file, line, false );
	Set( msg, file, line );
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

void TT_TestList::Add( const TT_Test& t )
{
	if ( Count == Capacity )
	{
		int newcap = Capacity * 2;
		if ( newcap < 8 ) newcap = 8;
		TT_Test* newlist = (TT_Test*) malloc( sizeof(List[0]) * newcap );
		if ( !newlist )
		{
			printf( "TT_TestList out of memory\n" );
			exit(1);
		}
		memcpy( newlist, List, sizeof(List[0]) * Count );
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

void TTSetDead()
{
#ifdef _WIN32
	// We make no attempt to close this event properly, relying instead on the OS
	CreateEvent( NULL, true, true, EvMarkerDead );
#endif
}

bool TTIsDead()
{
#ifdef _WIN32
	HANDLE h = OpenEvent( EVENT_ALL_ACCESS, false, EvMarkerDead );
	if ( h ) CloseHandle(h);
	return h != NULL;
#else
	return false;
#endif
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996) // GetVersionEx is deprecated. Seriously!
#endif

void TTSetProcessIdle()
{
#ifdef _WIN32
	OSVERSIONINFO inf;
	inf.dwOSVersionInfoSize = sizeof(inf);
	GetVersionEx( &inf );
	SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS );
	if ( inf.dwMajorVersion >= 6 )
		SetPriorityClass( GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN ); // Lowers IO priorities too
#else
	fprintf( stderr, "TTSetProcessIdle not implemented\n" );
#endif
}

#ifdef _WIN32
#pragma warning(pop)
#endif

bool TTFileExists( const char* f )
{
#ifdef _WIN32
	return GetFileAttributesA( f ) != INVALID_FILE_ATTRIBUTES;
#else
	return access( f, F_OK ) != -1;
#endif
}

void TTWriteWholeFile( const char* filename, const char* str )
{
	FILE* h = fopen( filename, "w" );
	fwrite( str, strlen(str), 1, h );
	fclose(h);
}

TTModes TTMode()
{
	return IsExecutingUnderGuidance ? TTModeAutomatedTest : TTModeBlank;
}

void TTLog( const char* msg, ... )
{
	char buff[8192];
	va_list va;
	va_start( va, msg );
	vsprintf( buff, msg, va );
	va_end( va ); 

	//FILE* f = fopen( TT_File_Log, "a" );
	//if ( f )
	{
		char tz[128];
		time_t t;
		time( &t );
		tm t2;
#ifdef _WIN32
		_localtime64_s( &t2, &t );
#else
		localtime_r( &t, &t2 );
#endif
		strftime( tz, sizeof(tz), "%Y-%m-%d %H:%M:%S  ", &t2 );
		//fwrite( tz, strlen(tz), 1, f );
		fputs( tz, stdout );

		//fwrite( buff, strlen(buff), 1, f );
		fputs( buff, stdout );
		//fclose(f);
	}
}

static const char* ParallelName( TTParallel p )
{
	switch ( p )
	{
	case TTParallelDontCare: return TT_PARALLEL_DONTCARE;
	case TTParallelWholeCore: return TT_PARALLEL_WHOLECORE;
	case TTParallelSolo: return TT_PARALLEL_SOLO;
	}
	return NULL;
}

void TTListTests( const TT_TestList& tests )
{
	qsort( (void*) &tests[0], tests.size(), sizeof(TT_Test), (int(*)(const void*, const void*)) &TT_Test::CompareName );

	printf( "%s\n", TT_LIST_LINE_1 );
	printf( "%s\n", TT_LIST_LINE_2 );
	for ( int i = 0; i < tests.size(); i++ )
		printf( "  %-40s %-20s %-20s\n", tests[i].Name, tests[i].Size == TTSizeSmall ? TT_SIZE_SMALL_NAME : TT_SIZE_LARGE_NAME, ParallelName(tests[i].Parallel) );
}

unsigned int TTGetProcessID()
{
#ifdef _WIN32
	return (uint) GetCurrentProcessId();
#else
	return (uint) getpid();
#endif
}

void TTSleep( unsigned int milliseconds )
{
#ifdef _WIN32
	Sleep( milliseconds );
#else
	int64 nano = milliseconds * (int64) 1000;
	timespec t;
	t.tv_nsec = nano % 1000000000; 
	t.tv_sec = (nano - t.tv_nsec) / 1000000000;
	nanosleep( &t, NULL );
#endif
}

char** TTArgs()
{
	return TestCmdLineArgs;
}

static void GetProcessPath( char* path )
{
	char buf[MAX_PATH];
#ifdef _WIN32
	GetModuleFileNameA( NULL, buf, MAX_PATH );
#else
    buf[readlink( "/proc/self/exe", buf, MAX_PATH - 1 )] = 0;	// untested
#endif
	strcpy( path, buf );
}

const int MAXARGS = 30;

bool TTRun_WrapperW( TT_TestList& tests, int argc, wchar_t** argv, int* retval )
{
	const int ARGSIZE = 400;
	char* argva[MAXARGS];

	if ( argc >= MAXARGS - 1 ) { printf( "TTRun_InternalW: Too many arguments\n" ); return false; }
	for ( int i = 0; i < argc; i++ )
	{
		argva[i] = (char*) malloc( ARGSIZE );
		wcstombs( argva[i], argv[i], ARGSIZE );
	}
	argva[argc] = NULL;

	bool rv = TTRun_Wrapper( tests, argc, argva, retval );

	for ( int i = 0; i < argc; i++ )
		free(argva[i]);

	return rv;
}

static int	TTRun_Internal_Mode1_Escape( const char* const* options, const char* testname );
static int	TTRun_Internal_Mode2_Execute( const TT_TestList& tests, const char* testname );
static bool	TTRun_Internal( const TT_TestList& tests, int argc, char** argv, int* retval );

bool TTRun_Wrapper( TT_TestList& tests, int argc, char** argv, int* retval )
{
	bool res = TTRun_Internal( tests, argc, argv, retval );
	// ensure that there are no memory leaks by the time we return
	tests.Clear();
	return res;
}

/*
TTRun has three modes. Let's assume that your test program is called "mytest.exe".

	Mode 1	You run "mytest.exe [options] test all" to run all tests. This ends up calling "tinytest_app.exe path\to\mytest.exe test all".
			tinytest_app is going to end up calling mytest.exe again, and this time it will end up in mode 2.

	Mode 2	tinytest_app is calling mytest.exe. In this case, it calls it like so: "mytest.exe [options] test =testname". In this case,
			mytest.exe will actually run the test named "testname". The testname will never be "all". That "all" is enumerated by the
			tinytest_app, and each test is called one by one.
			In this mode, IsExecutingUnderGuidance = true.

	Mode 3	You launch mytest.exe from the debugger. You do it like so: "mytext.exe test :testname". This causes slightly different
			behaviour that makes it easier to debug tests.
			In this mode, IsExecutingUnderGuidance = false.

	NOTE: Options must start with a hyphen
	
*/
bool TTRun_Internal( const TT_TestList& tests, int argc, char** argv, int* retval )
{
	*retval = 1;
	char mypath[300];
	GetProcessPath( mypath );

	int noptions = 0;
	const char* options[MAXARGS];
	
	int nTestArgs = 0;
	bool dotest = false;
	bool ismode2 = false;
	const char* testname = NULL;

	for ( int i = 0; i < argc; i++ )
	{
		if ( strcmp(argv[i], TT_TOKEN_INVOKE) == 0 )					{ dotest = true; }
		else if ( argv[i][0] == '-' )									{ options[noptions++] = argv[i]; }
		else if ( dotest && testname == NULL )							{ testname = argv[i]; }						// only accept test name after 'test' has appeared
		else if ( strstr(argv[i], TT_PREFIX_RUNNER_PID) == argv[i])		{ ExecutorPID = atoi( argv[i] + strlen(TT_PREFIX_RUNNER_PID) ); }
		else if ( testname != NULL )									{ TestCmdLineArgs[nTestArgs++] = argv[i]; }	// only accepted after test name has appeared

		if ( noptions >= MAXARGS - 1 )
		{
			printf( "TTRun: Too many arguments\n" );
			return false;
		}
		if ( nTestArgs >= TT_MAX_CMDLINE_ARGS - 1 )
		{
			printf( "TTRun: Too many test-specific arguments (ie after the test name)\n" );
			return false;
		}
	}

	if ( !dotest )
		return false;

	options[noptions++] = NULL;
	TestCmdLineArgs[nTestArgs++] = NULL;

	if ( testname == NULL || testname[0] == 0 )
	{
		TTListTests( tests );
		*retval = 0;
	}
	else if ( testname && testname[0] == '=' )
	{
		// Mode 2. ie.. we are being called by tinytest_app. We must actually run the test.
		// Since we are executing under the guidance of tinytest_app, don't popup message boxes, etc.
		// Rather send our failure message to stdout if we die, or exit(0) upon success.
		IsExecutingUnderGuidance = true;
		*retval = TTRun_Internal_Mode2_Execute( tests, testname + 1 );
	}
	else if ( testname && testname[0] == ':' )
	{
		// Mode 3. ie.. we are being called invoked from an IDE, likely under a debugger.
		*retval = TTRun_Internal_Mode2_Execute( tests, testname + 1 );
	}
	else
	{
		// Mode 1. Get tinytest_app to run us
		*retval = TTRun_Internal_Mode1_Escape( options, testname );
	}

	return true;
}

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 6054 ) // for 'args' to _execv below
#endif

static int TTRun_Internal_Mode1_Escape( const char* const* options, const char* testname )
{
	// Mode 1. Get tinytest_app to run us. tinytest_app knows when we die, etc.
	//printf( "Escape %s %s\n", testcmdname, testname );
	char mypath[MAX_PATH], host[MAX_PATH];
	GetProcessPath( mypath );
	strcpy( host, mypath );
	int i;
	for ( i = (int) strlen(host); host[i] != DIRSEP; i-- ) {}
	host[i + 1] = 0;
	strcat( host, "tinytest_app" );
	strcat( host, EXE_EXTENSION );

	const char* args[MAXARGS];
	int narg = 0;
	args[narg++] = "run";
	args[narg++] = mypath;
	args[narg++] = TT_TOKEN_INVOKE;
	args[narg++] = testname;
	for ( int j = 0; options[j]; j++ )
		args[narg++] = options[j];
	args[narg++] = NULL;

#ifdef _WIN32
	return _spawnv( _P_WAIT, host, args ) == 0 ? 0 : 1;
#else
	printf( "TODO: Implement spawn on linux\n" );
	//execv( host, (char *const*) args );
#endif

	return 0;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

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
		if ( isCallStackProper )
			printf( "%s", szText );
	}
};

static void Die()
{
#ifdef _WIN32
	if ( IsDebuggerPresent() )
		__debugbreak();
	TerminateProcess( GetCurrentProcess(), 1 );
#else
	_exit(1);
#endif
}

#ifdef _WIN32
static LONG WINAPI TTExceptionHandler( EXCEPTION_POINTERS* exInfo )
{
	if ( exInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW )
	{
		fputs( "Stack overflow\n", stdout );
	}
	else
	{
		// The words "Stack Trace" must appear at the top, otherwise Jenkins
		// is likely to discard too much of your trace, or your exception information
		StackWalkerToConsole sw;  // output to console
		printf(	"------- Unhandled Exception and Stack Trace ------- \n"
				"   Code:    0x%8.8X\n"
				"   Flags:   %u\n"
				"   Address: 0x%p\n",
			exInfo->ExceptionRecord->ExceptionCode,
			exInfo->ExceptionRecord->ExceptionFlags,
			exInfo->ExceptionRecord->ExceptionAddress);
		fflush( stdout );
		printf(	"-------\n" );
		fflush( stdout );
		sw.ShowCallstack(GetCurrentThread(), exInfo->ContextRecord);
		fflush( stdout );
	}
	fflush( stdout );
	TerminateProcess( GetCurrentProcess(), 33 );
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static void TTPurecallHandler()
{
	printf( "Undefined virtual function called (purecall)\n" );
	fflush( stdout );
	Die();
}

#ifdef _WIN32
static void TTInvalidParameterHandler( const wchar_t * expression, const wchar_t * function, const wchar_t * file, unsigned int line, uintptr_t pReserved )
{
	//if ( expression && function && file )
	//	fputs( "CRT function called with invalid parameters: %S\n%S\n%S:%d\n", expression, function, file, line );
	//else
	fputs( "CRT function called with invalid parameters\n", stdout );
	fflush( stdout );
	Die();
}
#endif

static void TTSignalHandler(int signal)
{
	printf( "Signal %d called", signal );
	fflush( stdout );
	Die();
}

void TT_IPC_Filename( bool up, unsigned int executorPID, unsigned int testedPID, char (&filename)[256] )
{
#ifdef _WIN32
	// We prefer c:\temp over obscure stuff inside the "proper" temp directory, because that tends to fill up with stuff you never see.
	_mkdir( "c:\\temp" );
	sprintf( filename, "c:\\temp\\tiny_test_ipc_%s_%u_%u", up ? "up" : "down", executorPID, testedPID );
#else
	sprintf( filename, "/tmp/tiny_test_ipc_%s_%u_%u", up ? "up" : "down", executorPID, testedPID );
#endif
}

bool TT_IPC_Read_Raw( unsigned int waitMS, char (&filename)[256], char (&msg)[TT_IPC_MEM_SIZE] )
{
	memset( msg, 0, sizeof(msg) );
	FILE* file = fopen( filename, "rb" );
	if ( file == NULL )
	{
		TTSleep( waitMS );
		return false;
	}

	uint length = 0;
	bool good = false;
	if ( fread( &length, sizeof(length), 1, file ) == 1 )
		good = fread( &msg, length, 1, file ) == 1;

	fclose( file );
	// We acknowledge receipt by deleting the file
	if ( good )
		remove( filename );
	return good;
}

void TT_IPC_Write_Raw( char (&filename)[256], const char* msg )
{
	char buf[512];
	unsigned int length = (unsigned int) strlen(msg);
	FILE* file = fopen( filename, "wb" );
	if ( file == NULL )
	{
		sprintf( buf, "TT_IPC failed to open %s", filename );
		TTAssertFailed( buf, "internal", 0, true );
	}
	fwrite( &length, sizeof(length), 1, file );
	fwrite( msg, length, 1, file );
	fclose( file );
	// The other side acknowledges this transmission by deleting the file
	uint sleepMS = 30;
	for ( uint nwait = 0; nwait < 5 * 1000 / sleepMS; nwait++ )
	{
		TTSleep( sleepMS );
		FILE* file = fopen( filename, "rb" );
		if ( file == NULL )
			return;
		fclose(file);
	}
	sprintf( buf, "TT_IPC executor failed to acknowledge read on %s", filename );
	TTAssertFailed( buf, "internal", 0, true );
}

static void TT_IPC( const char* msg, ... )
{
	// This is one-way communication, from the tested app to the test harness app

	// Disable IPC if we're not being run as an automated test.
	// Typical use case is to run a single test from a debugger.
	// Erg... this is a hack, because I haven't yet figured out a clean way to differentiate auto vs manual invoke.
	//if ( IsDebuggerPresent() )
	//	return;
	// Damn stupid that previous statement! This is a trivial way to detect if we're being run under administration:
	if ( ExecutorPID == 0 )
		return;

	char filename[256];
	TT_IPC_Filename( true, ExecutorPID, TTGetProcessID(), filename );

	char buf[8192];
	va_list va;
	va_start( va, msg );
	vsprintf( buf, msg, va );
	va_end( va ); 

	TT_IPC_Write_Raw( filename, buf );
}

void TTNotifySubProcess( unsigned int pid )
{
	TT_IPC( "%s %u", TT_IPC_CMD_SUBP_RUN, pid );
}

static void TTRun_PrepareExecutionEnvironment()
{
#ifdef _WIN32
	SetUnhandledExceptionFilter( TTExceptionHandler );
#endif
	setvbuf( stdout, NULL, _IONBF, 0 );									// Disable all buffering on stdout, so that the last words of a dying process are recorded.
	setvbuf( stderr, NULL, _IONBF, 0 );									// Same for stderr.
#ifdef _WIN32
	_set_error_mode( _OUT_TO_STDERR );									// prevent CRT dialog box popups. This doesn't work for debug builds.
	_set_purecall_handler( TTPurecallHandler );							// pure virtual function
	_set_invalid_parameter_handler( TTInvalidParameterHandler );		// CRT function with invalid parameters
#endif
	signal( SIGABRT, TTSignalHandler );									// handle calls to abort()
}

static int TTRun_Internal_Mode2_Execute( const TT_TestList& tests, const char* testname )
{
	if ( IsExecutingUnderGuidance )
		TTRun_PrepareExecutionEnvironment();

	for ( int i = 0; i < tests.size(); i++ )
	{
		if ( strcmp(tests[i].Name, testname) == 0 )
		{
			if ( tests[i].Init )
				tests[i].Init();
			
			tests[i].Blank();

			if ( tests[i].Teardown )
				tests[i].Teardown();

			return 0;
		}
	}

	fprintf( stderr, "Test '%s' not found\n", testname );
	return 1;
}

void TTAssertFailed( const char* exp, const char* filename, int line_number, bool die )
{
	char txt[4096];
	sprintf( txt, "Test Failed:\nExpression: %s\nFile: %s\nLine: %d", exp, filename, line_number );
	printf( "%s", txt );
	fflush( stdout );
	fflush( stderr );

	if ( IsExecutingUnderGuidance )
	{
#ifdef _WIN32
		// Use TerminateProcess instead of exit(), because we don't want any C++ cleanup, or CRT cleanup code to run.
		// Such "at-exit" functions are prone to popping up message boxes about resources that haven't been cleaned up, but
		// at this stage, that is merely noise.
		TerminateProcess( GetCurrentProcess(), 1 );
#else
		_exit(1);
#endif
	}
	else
	{
#ifdef _WIN32
		__debugbreak();
#else
		_exit(1);
#endif
	}
}
