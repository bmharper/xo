#include "pch.h"

#include "TinyTest.h"

#ifdef _MSC_VER
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif

//#define DEBUG_THREADS_CX(fmt, ...)	if (cx->DebugThreads) printf(fmt, __VA_ARGS__)
//#define DEBUG_THREADS(fmt, ...)		if (DebugThreads) printf(fmt, __VA_ARGS__)
#define DEBUG_THREADS_CX(fmt, ...)	(void) 0
#define DEBUG_THREADS(fmt, ...)		(void) 0

#if defined(__X86_64__) || defined(_M_X64)
typedef long long intp;
#else
typedef int intp;
#endif

struct Test;
struct TestContext;
struct OutputDev;
struct Process;

using namespace std;

extern const char* HtmlHeadTxt;
extern const char* HtmlTailTxt;
extern const char* JUnitTailTxt;

string			EscapeHtml( const string& s );
string			EscapeXml( const string& s );
podvec<string>	DiscardEmpties( const podvec<string>& s );
podvec<string>	Split( const string& s, char delim );
string			Trim( const string& s );
string			FilenameOnly( const string& s );
bool			MatchWildcardNocase( const string& s, const string& p );
bool			MatchWildcardNocase( const char* s, const char* p );
bool			WriteWholeFile( const string& filename, const string& s );
string			IntToStr( int64 i );
double			TimeSeconds();
BOOL			CloseHandleAndZero( HANDLE& h );
TTParallel		ParseParallel( const char* s );

struct Test
{
	TestContext*	Context;
	string			Name;
	string			Output;
	string			TrimmedOutput;
	TTSizes			Size;
	TTParallel		Parallel;
	bool			Ignored;
	bool			Pass;
	double			Time;

	explicit Test( TestContext* context, string name, TTSizes size, TTParallel parallel )
	{
		Construct();
		Context = context;
		Name = name;
		Size = size;
		Parallel = parallel;
	}
	Test() { Construct(); }

	void Construct() { Context = NULL; Pass = false; Ignored = false; Time = 0; }
	double TimeoutSeconds() const
	{
		return Size == TTSizeSmall ? 5 * 60 : 15 * 60;
	}
	static int ParallelSort( TTParallel p )
	{
		switch ( p )
		{
		case TTParallelDontCare: return 0;
		case TTParallelWholeCore: return 1;
		case TTParallelSolo: return 2;
		default: return 100;
		}
	}
	static int CompareByParallel( const Test& a, const Test& b )
	{
		int pdiff = ParallelSort(a.Parallel) - ParallelSort(b.Parallel);
		if ( pdiff != 0 )
			return pdiff;
		return strcmp( a.Name.c_str(), b.Name.c_str() );
	}
};

struct OutputDev
{
	virtual bool		ImmediateOut() { return false; }
	virtual string		OutputFilename( string base ) { return ""; }
	virtual string		HeadPre( TestContext& c ) { return ""; }
	virtual string		HeadPost( TestContext& c ) { return ""; }
	virtual string		Tail() = 0;
	virtual string		ItemPre( const Test& t ) { return ""; }
	virtual string		ItemPost( const Test& t ) = 0;
};

// This is our global state. A single instance of these exists for a single run of the application.
struct TestContext
{
	string					ExePath;
	string					Name;
	string					Ident;
	string					Options;
	string					OutDir;
	string					OutText;
	OutputDev*				Out;
	bool					OutputBare;
	bool					RunLargeTests;
	bool					DebugThreads;
	int						NumThreads;
	podvec<Test>			Tests;
	podvec<string>			Include;
	podvec<string>			Exclude;

				TestContext();
				~TestContext();

	int			EnumAndRun( string exepath, int argc, char** argv );

	int			CountPass()	const		{ int c = 0; for ( intp i = 0; i < Tests.size(); i++ ) c += Tests[i].Pass ? 1 : 0; return c; }
	int			CountIgnored() const	{ int c = 0; for ( intp i = 0; i < Tests.size(); i++ ) c += Tests[i].Ignored ? 1 : 0; return c; }
	int			CountFail() const		{ return (int) Tests.size() - CountPass() - CountIgnored(); }
	int			CountRun() const		{ return (int) Tests.size() - CountIgnored(); }
	double		TotalTime() const		{ double t = 0; for ( intp i = 0; i < Tests.size(); i++ ) t += Tests[i].Time; return t; }
	string		FullName() const		{ return Name + "-" + Ident; }

	void		SetOutText();
	void		SetOutJUnit();
	void		SetOutHtml();

	static AbcThreadReturnType AbcKernelCallbackDecl ExecuteThread( void* threadContext );

protected:
	podvec<AbcThreadHandle>		Threads;
	volatile unsigned int		ExecTotal;		// Counts total number of executing threads
	volatile unsigned int		ExecWholeCore;	// Counts the number of 'wholecore' jobs executing (or wanting to execute)
	unsigned int				SysNumCores;	// Number of physical (aka whole) cores in this machine
	AbcCriticalSection			QueueLock;		// Guards access to TestPos, as well as OutText
	intp						TestPos;		// Queue position in 'Tests'. Starts at zero and ends at Tests.size(). Guarded by QueueLock.

	bool		CreateThreads();
	int			Run();
	bool		EnumTests();
	bool		TestMatchesFilter( const Test& t ) const;
	void		FlushOutput();
	void		WriteOutputToFile();
};

// This manages a single process that is executing a single test
struct Process
{
	TestContext*		Context;

	Process( TestContext* cx ) : Context(cx) {}

	// Launch a process and wait for it to complete.
	// The return value contains the error if the process failed to launch.
	// If the return value is empty, then the process execution succeeded.
	string			ExecuteAndWait( string exec, double timeoutSeconds, string& childStdOut, int& processExitCode );

private:
	unsigned int		ProcessID;			// Process ID of currently executing process
	podvec<HANDLE>		SubProcesses;		// Sub processes launched by the currently executing test

	void			ReadIPC( uint waitMS );
	void			CloseZombieProcesses();
	string			ExecuteAndWaitOnce( string exec, double timeoutSeconds, string& childStdOut, int& processExitCode );
};

struct OutputDevText : public OutputDev
{
	virtual bool		ImmediateOut()					{ return true; }
	virtual string		HeadPre( TestContext& c )		{ return fmt("%s %s\n", c.Name, c.Ident); }
	virtual string		HeadPost( TestContext& c )		{ return fmt("%d/%d passed\n", c.CountPass(), c.CountRun()); }
	virtual string		Tail()							{ return ""; }
	virtual string		ItemPre( const Test& t )		{ return fmt( "%-35s", t.Name ); }
	virtual string		ItemPost( const Test& t )
	{
		string r = fmt( "%s", t.Pass ? "PASS" : "FAIL" );
		if ( !t.Pass )
			r += "\n" + t.TrimmedOutput;
		r += "\n";
		return r;
	}
};

struct OutputDevHtml : public OutputDev
{
	virtual string		OutputFilename( string base )	{ return base + ".html"; }
	virtual string		HeadPre( TestContext& c )		{ return HtmlHeadTxt; }
	virtual string		Tail()							{ return HtmlTailTxt; }
	virtual string		ItemPost( const Test& t )
	{
		if ( t.Pass )	return fmt( "<div class='test pass'>%s</div>\n", EscapeHtml(t.Name) );
		else			return fmt( "<div class='test fail'>%s<pre class='detail'>%s</pre></div>\n", EscapeHtml(t.Name), EscapeHtml(t.TrimmedOutput) );
	}
};

struct OutputDevJUnit : public OutputDev
{
	virtual string		OutputFilename( string base ) { return base + ".xml"; }
	virtual string		HeadPost( TestContext& c )
	{
		//return fmt( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		//			"<testrun name=\"%v\" project=\"%v\" tests=\"%d\" started=\"%d\" failures=\"%d\" errors=\"%d\" ignored=\"%d\">\n"
		//			"\t<testsuite name=\"%v\" time=\"%f\">\n",
		//			c.Name, c.Ident, c.Tests.size(), c.Tests.size() - c.CountIgnored(), c.CountFail(), 0, c.CountIgnored(), "anon", c.TotalTime() );
		return fmt( "<testsuite name=\"%v\" time=\"%f\">\n",
					c.FullName(), c.TotalTime() );
	}
	virtual string		Tail()		{ return JUnitTailTxt; }
	virtual string		ItemPost( const Test& t )
	{
		if ( t.Pass )	return fmt( "\t\t<testcase name=\"%v\" classname=\"%v\" time=\"%f\"/>\n", EscapeHtml(t.Name), t.Context->FullName(), t.Time );
		else			return fmt( "\t\t<testcase name=\"%v\" classname=\"%v\" time=\"%f\">\n"
									"\t\t\t<failure>%s</failure>\n"
									"\t\t</testcase>\n",
									EscapeHtml(t.Name), t.Context->FullName(), t.Time, EscapeXml(t.TrimmedOutput) );
	}
};

void ShowHelp()
{
	const char* msg = 
		"tinytest_app run test_program_path " TT_TOKEN_INVOKE " testname1 testname2... [options]\n"
		"  run                 This is the only supported command\n"
		"  test_program_path   Full path to the executable that runs the tests\n"
		"  " TT_TOKEN_INVOKE "                This parameter must be here\n"
		"  testname(s)         Wildcards of the tests that you want to run, or '" TT_TOKEN_ALL_TESTS "' to run all\n"
		"  -xEXCLUDE           Exclude any tests that match the wildcard EXCLUDE. Excludes override includes.\n"
		"  -tt-small           Run only small tests\n"
		"  -tt-html            Output as HTML\n"
		"  -tt-junit           Output as JUnit\n"
		"  -tt-bare            For HTML, exclude header and tail\n"
		"  -tt-head            For HTML, output only header\n"
		"  -tt-tail            For HTML, output only tail\n"
		"  -tt-out DIR         For JUnit, write xml files to this directory\n"
		"  -tt-ident IDENT     Give this test suite a name, such as 'win32-release'. This forms part of the xml file\n"
		"  NOTE                This program is intended to be run indirectly, from a test implementation program\n"
		"\n";
	puts( msg );
}

int main(int argc, char* argv[])
{
	bool showhelp = true;
	//for ( int i = 0; i < argc; i++ ) printf( "arg[%d] = '%s'\n", i, argv[i] );

	TestContext cx;
	cx.SetOutText();
	cx.OutputBare = false;

	if ( strstr(argv[0], "tinytest_app") != NULL )
	{
		// strip our own path
		argc--;
		argv++;
	}

	if ( argc >= 3 && strcmp(argv[0], "run") == 0 )
	{
		string exepath = argv[1];
		std::replace( exepath.begin(), exepath.end(), '/', DIR_SEP );
		std::replace( exepath.begin(), exepath.end(), '\\', DIR_SEP );
		string throwaway = argv[2];		// "test"
		argc -= 3;
		argv += 3;
		return cx.EnumAndRun( exepath, argc, argv );
	}

	if ( showhelp )
	{
		ShowHelp();
		return 1;
	}
	return 0;
}

TestContext::TestContext()
{
	Out = NULL;
	RunLargeTests = true;
	DebugThreads = true;
	NumThreads = 0;
	char buf[256];
	if ( GetEnvironmentVariableA( "TT_NUMTHREADS", buf, arraysize(buf) ) < 10 )
		NumThreads = atoi(buf);
	AbcCriticalSectionInitialize( QueueLock );
}

TestContext::~TestContext()
{
	AbcCriticalSectionDestroy( QueueLock );
	delete Out;
}

bool TestContext::EnumTests()
{
	//printf( "EnumTests\n" );
	FILE* pipe = _popen( (ExePath + " " + TT_TOKEN_INVOKE).c_str(), "rb" );
	if ( !pipe ) { fprintf( stderr, "Failed to popen(%s)\n", ExePath.c_str() ); return false; }

	size_t read = 0;
	char buf[1024];
	podvec<char> all;
	while ( read = fread( buf, 1, sizeof(buf), pipe ) )
		all.addn( buf, read );
	fclose( pipe );
	all += 0;

	podvec<string> lines = Split( string(&all[0]), '\n' );
	bool eat = false;
	for ( intp i = 0; i < lines.size(); i++ )
	{
		string trim = Trim(lines[i]);
		if ( trim == "" ) continue;
		if ( eat )
		{
			podvec<string> parts = DiscardEmpties( Split( trim, ' ' ) );
			if ( parts.size() != 3
				|| (parts[1] != TT_SIZE_SMALL_NAME && parts[1] != TT_SIZE_LARGE_NAME)
				|| (ParseParallel(parts[2].c_str()) == TTParallel_Invalid_) )
			{
				fmtoutf( stderr, "Expected 'testname small|large " TT_PARALLEL_DONTCARE "|" TT_PARALLEL_WHOLECORE "|" TT_PARALLEL_SOLO "' from test enumeration line. Instead '%s'\n", trim.c_str() );
				return false;
			}
			Tests += Test( this, parts[0], parts[1] == TT_SIZE_SMALL_NAME ? TTSizeSmall : TTSizeLarge, ParseParallel(parts[2].c_str()) );
			//printf( "Discovered test '%s'\n", (const char*) tests.back() );
		}
		else if ( trim == Trim(TT_LIST_LINE_END) )
		{
			eat = true;
			continue;
		}
	}

	return true;
}

int TestContext::EnumAndRun( string exepath, int argc, char** argv )
{
	ExePath = exepath;
	Name = FilenameOnly( exepath );
	for ( int i = 0; i < argc; i++ )
	{
		string opt = argv[i];
		if ( opt[0] == '-' )
		{
			if ( opt == "-tt-html" )				{ SetOutHtml(); }
			else if ( opt == "-tt-junit" )			{ SetOutJUnit(); }
			else if ( opt == "-tt-small" )			{ RunLargeTests = false; }
			else if ( opt == "-tt-bare" )			{ OutputBare = true; }
			else if ( opt == "-tt-head" )			{ fputs( HtmlHeadTxt, stdout ); return 0; }
			else if ( opt == "-tt-tail" )			{ fputs( HtmlTailTxt, stdout ); return 0; }
			else if ( opt == "-tt-out" )			{ OutDir = argv[i + 1]; i++; }
			else if ( opt == "-tt-ident" )			{ Ident = argv[i + 1]; i++; }
			else if ( opt.substr(0, 2) == "-x" )	{ Exclude += opt.substr(2); }
			else									{ Options += opt + " "; }
		}
		else
		{
			Include += opt;
		}
	}
	if ( Options.size() != 0 )
		Options.pop_back();

	if ( !EnumTests() )
		return 1;

	if ( Include.size() == 0 && Exclude.size() == 0 )
	{
		for ( int i = 0; i < Tests.size(); i++ )
			printf("  %s\n", Tests[i].Name.c_str() );
		return 0;
	}
	else
	{
		return Run();
	}
}

bool TestContext::TestMatchesFilter( const Test& t ) const
{
	if ( t.Size == TTSizeLarge && !RunLargeTests )
		return false;

	bool pass = false;
	for ( intp i = 0; i < Include.size(); i++ )
	{
		if ( Include[i] == TT_TOKEN_ALL_TESTS )
			pass = true;
		else if ( MatchWildcardNocase( t.Name, Include[i] ) )
			pass = true;
	}
	for ( intp i = 0; i < Exclude.size(); i++ )
	{
		if ( MatchWildcardNocase( t.Name, Exclude[i] ) )
			pass = false;
	}
	return pass;
}

void TestContext::FlushOutput()
{
	if ( Out->ImmediateOut() )
	{
		fputs( OutText.c_str(), stdout );
		OutText = "";
	}
}

void TestContext::WriteOutputToFile()
{
	string outf;
	if ( OutDir != "" )
	{
		outf = OutDir;
		if ( outf.back() != DIR_SEP ) outf += DIR_SEP;
		outf += Name;
		outf += "-" + Ident;
		outf = Out->OutputFilename(outf);
	}
	if ( !Out->ImmediateOut() )
	{
		if ( outf != "" )
			WriteWholeFile( outf, OutText );
		else
			fputs( OutText.c_str(), stdout );
	}
}

void TestContext::SetOutText()	{ delete Out; Out = new OutputDevText; }
void TestContext::SetOutJUnit()	{ delete Out; Out = new OutputDevJUnit; }
void TestContext::SetOutHtml()	{ delete Out; Out = new OutputDevHtml; }

struct ThreadContext
{
	volatile int	Consumed;
	TestContext*	TestCX;
	int				ThreadNumber;
};

bool TestContext::CreateThreads()
{
	AbcMachineInformation inf;
	AbcMachineInformationGet( inf );
	SysNumCores = inf.PhysicalCoreCount;

	int nThreads = inf.LogicalCoreCount;
	if ( NumThreads > 0 )
		nThreads = NumThreads;

	DEBUG_THREADS( "Launching %d threads\n", nThreads );

	for ( int i = 0; i < nThreads; i++ )
	{
		ThreadContext thread;
		thread.Consumed = 0;
		thread.TestCX = this;
		thread.ThreadNumber = i;
		if ( !AbcThreadCreate( ExecuteThread, &thread, Threads.add() ) )
			return false;
		while ( thread.Consumed == 0 )
			AbcSleep(0);
	}

	return true;
}

AbcThreadReturnType AbcKernelCallbackDecl TestContext::ExecuteThread( void* context )
{
	TestContext* cx = ((ThreadContext*) context)->TestCX;
	int threadNum = ((ThreadContext*) context)->ThreadNumber;
	((ThreadContext*) context)->Consumed = 1;
	// context is now invalid, since we set Consumed to 1.

	DEBUG_THREADS_CX( "Thread %d start\n", threadNum );

	string outText;

	// If there are 4 physical CPU cores, and 8 threads, then the blessed threads
	// are the first 4. Whenever a 'whole core' job is running, then all non-blessed threads
	// go to sleep.
	bool isBlessedThread = threadNum < (int) cx->SysNumCores;
	
	const int sleepMS = 20;

	auto flushOutput = [cx, &outText]()
	{
		if ( cx->Out->ImmediateOut() )
		{
			fputs( outText.c_str(), stdout );
			outText = "";
		}
	};

	while ( true )
	{
		Test* t = NULL;
		{
			// Find the next test that matches the filter
			TakeCriticalSection lock(cx->QueueLock);
			bool found = false;
			while ( cx->TestPos < cx->Tests.size() )
			{
				t = &cx->Tests[cx->TestPos];
				if ( !cx->TestMatchesFilter(*t) )
				{
					t->Ignored = true;
					cx->TestPos++;
					continue;
				}
				if ( t->Parallel == TTParallelSolo && threadNum != 0 )
				{
					// Tests are sorted so that Solo tests are at the end of the list, and since we
					// are not thread 0, we don't run those. Therefore, we are finished.
					break;
				}
				DEBUG_THREADS_CX( "Thread %d executing test %s (acquire)\n", threadNum, t->Name.c_str() );
				found = true;
				cx->TestPos++;
				break;
			}
			// all tests are finished
			if ( !found )
				break;
		}

		// Hold off if our computational device is being requested
		for ( int iwait = 0; cx->ExecWholeCore != 0 && !isBlessedThread; iwait++ )
		{
			if ( iwait == 0 )
				DEBUG_THREADS_CX( "Thread %d pausing because not blessed and ExecWholeCore = %d\n", threadNum, cx->ExecWholeCore );
			AbcSleep( sleepMS );
		}
		
		for ( int iwait = 0; t->Parallel == TTParallelSolo && cx->ExecTotal != 0; iwait++ )
		{
			if ( iwait == 0 )
				DEBUG_THREADS_CX( "Thread %d pausing, waiting for all other threads to go quiet before executing solo job\n", threadNum );
			AbcSleep( sleepMS );
		}

		if ( t->Parallel == TTParallelWholeCore )
			AbcInterlockedIncrement( &cx->ExecWholeCore );

		AbcInterlockedIncrement( &cx->ExecTotal );

		outText += cx->Out->ItemPre( *t );

		string exec = cx->ExePath + " " + TT_PREFIX_RUNNER_PID + IntToStr(TTGetProcessID()) + " " + TT_TOKEN_INVOKE + " =" + t->Name + " " + cx->Options;

		DEBUG_THREADS_CX( "Thread %d executing test %s (begin)\n", threadNum, t->Name.c_str() );

		string pstdOut;
		int pstatus = 0;
		double tstart = TimeSeconds();
		Process proc( cx );
		string execError = proc.ExecuteAndWait( exec, t->TimeoutSeconds(), pstdOut, pstatus );
		if ( execError != "" )
		{
			pstatus = 1;
			pstdOut = "TinyTest ExecProcess Error: " + execError;
		}

		AbcInterlockedDecrement( &cx->ExecTotal );

		if ( t->Parallel == TTParallelWholeCore )
			AbcInterlockedDecrement( &cx->ExecWholeCore );

		t->Time = TimeSeconds() - tstart;
		t->Output = pstdOut;
		t->TrimmedOutput = pstdOut;
		if ( t->TrimmedOutput.length() > 1000 )
			t->TrimmedOutput = "..." + t->TrimmedOutput.substr( t->TrimmedOutput.length() - 1000 );
		t->Pass = pstatus == 0;

		outText += cx->Out->ItemPost( *t );
		flushOutput();
		DEBUG_THREADS_CX( "Thread %d executing test %s (retire)\n", threadNum, t->Name.c_str() );
	}

	{
		TakeCriticalSection lock(cx->QueueLock);
		cx->OutText += outText;
	}
	
	DEBUG_THREADS_CX( "Thread %d exit\n", threadNum );

	return 0;
}

int TestContext::Run()
{
	if ( !OutputBare ) OutText += Out->HeadPre( *this );
	FlushOutput();

	TestPos = 0;
	ExecTotal = 0;
	ExecWholeCore = 0;

	// Get all solo tests to run at the end
	// Only thread 0 runs solo tests.
	sort( Tests, Test::CompareByParallel );

	bool ok = CreateThreads();
	if ( !ok )
	{
		OutText += "Unable to create threads";
		printf( "Unable to create threads\n" );
	}

	if ( ok )
	{
		DEBUG_THREADS( "Waiting for all %d threads to join\n", (int) Threads.size() );
		for ( intp i = 0; i < Threads.size(); i++ )
			AbcThreadJoinAndCloseHandle( Threads[i] );
		Threads.clear();
	}

	int nrun = 0;
	int npass = 0;
	for ( intp i = 0; i < Tests.size() && ok; i++ )
	{
		nrun += Tests[i].Ignored ? 0 : 1;
		npass += Tests[i].Pass ? 1 : 0;
	}

	if ( !OutputBare && ok )
	{
		OutText = Out->HeadPost( *this ) + OutText;
		OutText += Out->Tail();
		FlushOutput();
	}

	WriteOutputToFile();

	return (ok && npass == nrun) ? 0 : 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

string Process::ExecuteAndWait( string exec, double timeoutSeconds, string& childStdOut, int& processExitCode )
{
	// This is a very blind guess that attempts to resolve an issue that I have been seeing on our CI machines
	// after enabling parallel tests. The symptom is that the process exits with code 0xC0000142, and produces
	// no output. It might just be related to this: http://support.microsoft.com/kb/960266
	string res;
	for ( int attempt = 0; attempt < 5; attempt++ )
	{
		res = ExecuteAndWaitOnce( exec, timeoutSeconds, childStdOut, processExitCode );
		if ( processExitCode != 0xC0000142 )
			break;
	}
	return res;
}

string Process::ExecuteAndWaitOnce( string exec, double timeoutSeconds, string& childStdOut, int& processExitCode )
{
	TestContext* cx = Context;
	ProcessID = 0;

	//HANDLE hChildStd_IN_Rd = NULL;
	//HANDLE hChildStd_IN_Wr = NULL;
	HANDLE hChildStd_OUT_Rd = NULL;
	HANDLE hChildStd_OUT_Wr = NULL;

	SECURITY_ATTRIBUTES saAttr;
	memset( &saAttr, 0, sizeof(saAttr) );
   
	//printf("\n->Start of parent execution.\n");
  
	// Set the bInheritHandle flag so pipe handles are inherited. 
   
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 
  
	// Create a pipe for the child process's STDOUT. 
   
	if ( !CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0) )
		return "StdoutRd CreatePipe"; 
  
	// Ensure the read handle to the pipe for STDOUT is not inherited.
  
	if ( !SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
		return "Stdout SetHandleInformation"; 
  
	/*
	// Create a pipe for the child process's STDIN. 
   
	if ( !CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0)) 
		return "Stdin CreatePipe";
  
	// Ensure the write handle to the pipe for STDIN is not inherited. 
   
	if ( !SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
		return "Stdin SetHandleInformation"; 
	*/
   
	// Create the child process. 

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	memset( &si, 0, sizeof(si) );
	memset( &pi, 0, sizeof(pi) );
	si.cb = sizeof(si);
	//si.hStdInput = hChildStd_IN_Rd;
	si.hStdError = hChildStd_OUT_Wr;
	si.hStdOutput = hChildStd_OUT_Wr;
	si.dwFlags = STARTF_USESTDHANDLES;

	char execPath[2048];
	strcpy( execPath, exec.c_str() );

	if ( !CreateProcessA( NULL, execPath, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi ) )
	{
		//fprintf( stderr, "Failed to popen(%s): %d\n", (const char*) exec, (int) GetLastError() );
		processExitCode = 1;
		return fmt( "Failed to popen(%s): %d\n", exec.c_str(), (int) GetLastError() );
	}

	ProcessID = pi.dwProcessId;

	// Write to the pipe that is the standard input for a child process. 
	// Data is written to the pipe's buffers, so it is not necessary to wait
	// until the child process is running before writing data.

	// WriteFile( hChildStd_IN_Wr, buffer, bytes, &written, NULL);

	// Close the pipe handle so the child process stops reading. 
	//if ( !CloseHandleAndZero(hChildStd_IN_Wr) ) 
	//	return "StdInWr CloseHandle"; 

	// Close the write end of the pipe before reading from the read end of the pipe, to control child process execution.
	// The pipe is assumed to have enough buffer space to hold the data the child process has already written to it.
   
	if ( !CloseHandleAndZero(hChildStd_OUT_Wr) )
		return "StdOutWr CloseHandle"; 

	// Read from the standard output of the child process. 
	char buf[1024];
	DWORD nread = 0;
	int64 nIdle = 0;
	double start = TimeSeconds();
	for ( int64 tick = 0; true; tick++ )
	{
		if ( tick % 500 == 0 )
			DEBUG_THREADS_CX( "Waiting for %s to finish\n", exec.c_str() );

		// Try waiting extra long if we have no output yet from child
		int64 nIdleThreshold = childStdOut.length() == 0 ? 10 : 5;
		if ( nIdle > nIdleThreshold && WaitForSingleObject( pi.hProcess, 0 ) == WAIT_OBJECT_0 )
			break;

		DWORD avail = 0;
		BOOL peekOK = PeekNamedPipe( hChildStd_OUT_Rd, NULL, 0, NULL, &avail, NULL );
		if ( !peekOK )
			break;
		if ( avail == 0 )
		{
			nIdle++;
			if ( TimeSeconds() - start > timeoutSeconds )
				break;
			ReadIPC( 16 );
			continue;
		}

		if ( !ReadFile( hChildStd_OUT_Rd, buf, sizeof(buf), &nread, NULL ) )
		{
			if ( GetLastError() == ERROR_BROKEN_PIPE ) {} // normal
			else
				childStdOut += fmt( "TinyTest: Unexpected ReadFile error on child process pipe: %d \n", (int) GetLastError() );
			break;
		}
		childStdOut.append( buf, nread );
	}

	DEBUG_THREADS_CX( "%s took %f seconds\n", exec.c_str(), TimeSeconds() - start );

	double timeRemaining = timeoutSeconds - (TimeSeconds() - start);
	timeRemaining = std::max(timeRemaining, 0.0);
	bool timeout = WaitForSingleObject( pi.hProcess, DWORD(timeRemaining * 1000) ) != WAIT_OBJECT_0;

	if ( timeout )
		TerminateProcess( pi.hProcess, 1 );
	else
		ReadIPC( 0 );

	DWORD exitcode = 111;
	if ( timeout )
		exitcode = 555;
	else
		GetExitCodeProcess( pi.hProcess, &exitcode );
	processExitCode = exitcode;

	if ( timeout )
		childStdOut += fmt( "\nTest timed out (max time %v seconds)", timeoutSeconds );
	else if ( exitcode != 0 && Trim(childStdOut).length() == 0 )
		childStdOut += fmt( "\nExit code %08X, but test produced no output", exitcode );
  
	CloseHandleAndZero( pi.hProcess );
	CloseHandleAndZero( pi.hThread );
 
	//CloseHandleAndZero( hChildStd_IN_Rd );
	CloseHandleAndZero( hChildStd_OUT_Rd );

	CloseZombieProcesses();

	return "";
}

void Process::ReadIPC( uint waitMS )
{
	char raw[TT_IPC_MEM_SIZE];
	memset( raw, 0, sizeof(raw) );

	char filename[256];
	TT_IPC_Filename( true, TTGetProcessID(), ProcessID, filename );
	bool haveData = TT_IPC_Read_Raw( waitMS, filename, raw );

	if ( haveData )
	{
		raw[TT_IPC_MEM_SIZE - 1] = 0;
		char cmd[200];
		uint u_p0 = 0;
		if ( sscanf( raw, "%200s %u", &cmd, &u_p0 ) == 2 )
		{
			// This has potential to fail, if the process has already terminated by the time we get here.
			// I'm assuming that if we can't open the process, then it has already died, and all is well.
			HANDLE proc = OpenProcess( PROCESS_TERMINATE | SYNCHRONIZE, false, u_p0 );
			if ( proc != NULL )
				SubProcesses += proc;
		}
		else
		{
			fprintf( stderr, "TinyTest: Unrecognized IPC command\n" );
		}
	}
}

void Process::CloseZombieProcesses()
{
#ifdef _WIN32
	for ( intp i = 0; i < SubProcesses.size(); i++ )
	{
		if ( WaitForSingleObject(SubProcesses[i], 0) != WAIT_OBJECT_0 )
			TerminateProcess( SubProcesses[i], 1 );
		CloseHandle( SubProcesses[i] );
	}
#endif
	SubProcesses.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

string Escape( bool xml, const string& s )
{
	string r;
	for ( size_t i = 0; i < s.length(); i++ )
	{
		if ( s[i] == '<' ) r += "&lt;";
		else if ( s[i] == '>' ) r += "&gt;";
		else if ( s[i] == '&' ) r += "&amp;";
		else if ( s[i] == '\r' ) {}
		else if ( s[i] == '\n' ) r += "&#10;";
		else r += s[i];
	}
	return r;
}

string EscapeHtml( const string& s )
{
	return Escape( false, s );
}

string EscapeXml( const string& s )
{
	return Escape( true, s );
}

podvec<string> DiscardEmpties( const podvec<string>& s )
{
	podvec<string> result;
	for ( intp i = 0; i < s.size(); i++ )
	{
		if ( s[i] != "" )
			result += s[i];
	}
	return result;
}

podvec<string> Split( const string& s, char delim )
{
	podvec<string> res;
	size_t start = 0;
	size_t end = 0;
	for ( ; end < s.length(); end++ )
	{
		if ( s[end] == delim )
		{
			res += s.substr( start, end - start );
			start = end + 1;
		}
	}
	if ( end - start > 0 || start != 0 )
		res += s.substr( start, end - start );
	return res;
}

string Trim( const string& s )
{
	size_t i = 0;
	size_t j = s.length() - 1;
	while ( i != s.length()	&& (s[i] == 9 || s[i] == 10 || s[i] == 13 || s[i] == 32) ) i++;
	while ( j != -1			&& (s[j] == 9 || s[j] == 10 || s[j] == 13 || s[j] == 32) ) j--;
	ptrdiff_t len = 1 + j - i;
	if ( len > 0 )
		return s.substr( i, len );
	else
		return "";
}

string FilenameOnly( const string& s )
{
	size_t dot = s.length();
	size_t i = s.length() - 1;
	for ( ; i != -1; i-- )
	{
		if ( s[i] == '.' )
			dot = i;
		if ( s[i] == '/' || s[i] == '\\' )
			break;
	}
	return s.substr( i + 1, dot - i - 1 );
}

bool MatchWildcardNocase( const string& s, const string& p )
{
	return MatchWildcardNocase( s.c_str(), p.c_str() );
}

bool MatchWildcardNocase( const char *s, const char *p )
{
	if ( *p == '*' )
	{
		while( *p == '*' )									++p;
		if ( *p == '\0' )									return true;
		while( *s != '\0' && !MatchWildcardNocase(s, p) )	++s;                
		return *s != '\0';
	}
	else if ( *p == '\0' || *s == '\0' )					return tolower(*p) == tolower(*s);
	else if ( tolower(*p) == tolower(*s) || *p == '?' )		return MatchWildcardNocase( ++s, ++p );
	else									return false;
}

bool WriteWholeFile( const string& filename, const string& s )
{
	bool good = false;
	FILE* f = fopen( filename.c_str(), "wb" );
	if ( f )
	{
		if ( s.length() != 0 )
			good = fwrite( &s[0], s.length(), 1, f ) == 1;
		else
			good = true;
		fclose(f);
	}
	return good;
}

string IntToStr( int64 i )
{
	char buf[100];
	sprintf( buf, "%lld", i );
	return buf;
}

#ifdef _WIN32
double TimeSeconds()
{
	LARGE_INTEGER t, f;
	QueryPerformanceCounter( &t );
	QueryPerformanceFrequency( &f );
	return t.QuadPart / (double) f.QuadPart;
}
#else
double TimeSeconds()
{
	timespec t;
	clock_gettime( CLOCK_MONOTONIC, &t );
	return t.tv_sec + t.tv_nsec * (1.0 / 1000000000);
}
#endif

BOOL CloseHandleAndZero( HANDLE& h )
{
	BOOL c = CloseHandle(h);
	h = NULL;
	return c;
}

TTParallel ParseParallel( const char* s )
{
	if ( 0 == strcmp(s, TT_PARALLEL_DONTCARE) ) return TTParallelDontCare;
	if ( 0 == strcmp(s, TT_PARALLEL_WHOLECORE) ) return TTParallelWholeCore;
	if ( 0 == strcmp(s, TT_PARALLEL_SOLO) ) return TTParallelSolo;
	return TTParallel_Invalid_;
}

#define S(x) #x

const char* HtmlHeadTxt = S((
<!DOCTYPE HTML>
<html>
<head>\n
<style>\n
	body	{ margin: 0; padding: 0; } \n
	h2		{ font-size: 16pt; margin: 5pt 6pt; }  \n
	.tests	{ padding: 5pt; }  \n
	.test	{ font-size 10pt; padding: 2pt 4pt; background: #f8f8f8; border-radius: 5px; border: 1px solid #f0f0f0; margin-top: 1px; }  \n
	.fail	{ color: #822; font-weight: bold; }  \n
	.pass	{ color: #282; display: inline-block; width: 15em; }  \n
	.detail { color: #444; margin-left: 10pt; }  \n
</style>
</head>
<body>
<div class='tests'>
\0)) + 1;

const char* HtmlTailTxt = S((
</div></body></html>
\0)) + 1;

const char* JUnitTailTxt = S((
</testsuite>\n
\0)) + 1;
//</testrun>\n


		/*
		FILE* pipe = _popen( exec, "rb" );
		if ( !pipe ) { fprintf( stderr, "Failed to popen(%s)\n", (const char*) ExePath ); return 1; }

		size_t read = 0;
		char buf[1024];
		string oput;
		while ( !feof(pipe) )
		{
			read = fread( buf, 1, sizeof(buf), pipe );
			oput.AppendExact( buf, read );
		}
		int pstatus = _pclose( pipe );
		*/
