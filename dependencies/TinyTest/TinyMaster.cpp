/*
This file has the logic for running tests from the command line.
This launches more instances of itself. Each of those instances runs
a single unit test. We launch as many instances as we can in parallel,
while obeying the flags that the tests specify.
*/

// Above this file is #included TinyLib.cpp, then TinyAssert.cpp.
// You can see that from TinyTestBuild.h

#include <algorithm>
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <sys/locking.h>
#include <sys/stat.h>
#include <fcntl.h>
#define DIR_SEP '\\'
#define DIR_SEP_STR "\\"
#define TT_TEST_DIR "c:\\temp\\tinytest\\"
#define TT_TEST_GLOBAL_LOCK_FILE TT_TEST_DIR "global_machine_lock"
typedef HANDLE TT_PROCESS_HANDLE;
#define popen _popen
#define pclose _pclose
#define open _open
#define close _close
#define lseek _lseek
#else
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#define DIR_SEP '/'
#define DIR_SEP_STR "/"
#define TT_TEST_DIR "/tmp/tinytest/"
#define TT_TEST_GLOBAL_LOCK_FILE TT_TEST_DIR "global_machine_lock"
typedef uint TT_PROCESS_HANDLE;
#define _In_z_
#define _Printf_format_string_
#endif

#include <stdint.h>

static const int TTGlobalLockTimeoutSeconds = 15 * 60;

// Uncomment the following two macros when debugging threading issues
//#define DEBUG_THREADS_CX(fmt, ...)	printf(fmt, ## __VA_ARGS__)
//#define DEBUG_THREADS(fmt, ...)		printf(fmt, ## __VA_ARGS__)
#define DEBUG_THREADS_CX(fmt, ...)	(void) 0
#define DEBUG_THREADS(fmt, ...)		(void) 0

template <typename T, size_t N>
char (&ArraySizeHelper(T(&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

struct Test;
struct TestContext;
struct ThreadContext;
struct OutputDev;
struct Process;

using namespace std;

extern const char* HtmlHeadTxt;
extern const char* HtmlTailTxt;
extern const char* JUnitTailTxt;

template<typename T> T TTMin(const T& a, const T& b) { return a < b ? a : b; }
template<typename T> T TTMax(const T& a, const T& b) { return a < b ? b : a; }

static void				TTShowHelp();
static int				TTRun_Internal(const TT_TestList& tests, int argc, char** argv);
static void				TTRun_PrepareExecutionEnvironment();

static string			EscapeHtml(const string& s);
static string			EscapeXml(const string& s);
static vector<string>	DiscardEmpties(const vector<string>& s);
static vector<string>	Split(const string& s, char delim);
static string			Trim(const string& s);
static string			Directory(const string& s);
static string			FilenameOnly(const string& s);
static string			SPrintf(_In_z_ _Printf_format_string_ const char* format, ...);
static bool				MatchWildcardNocase(const string& s, const string& p);
static bool				MatchWildcardNocase(const char* s, const char* p);
static bool				WriteWholeFile(const string& filename, const string& s);
static string			IntToStr(int64_t i);
static double			TimeSeconds();
static TTParallel		ParseParallel(const char* s);
static bool				MyCreateDirectory(const string& path);
static bool				DeleteDirectoryRecursive(const string& path);
static bool				DirectoryExists(const string& s);
static int				OpenLockFile(const string& path);		// Return <= 0 on failure
static void				CloseLockFile(int file);
static void				MySleep(unsigned ms);
static void				GetNumCPUCores(int& physicalCores, int& logicalCores);
#ifdef _WIN32
static BOOL				CloseHandleAndZero(HANDLE& h);
#endif

struct Test
{
	TestContext*	Context;
	string			Name;
	string			Output;
	string			TrimmedOutput;
	TTSizes			Size;
	TTParallel		Parallel;
	TTFuncBlank		Init;			// Only used when running in single-process mode
	TTFuncBlank		Teardown;		// Only used when running in single-process mode
	TTFuncBlank		Blank;			// Only used when running in single-process mode
	bool			Ignored;
	bool			Pass;
	double			Time;

	explicit Test(TestContext* context, string name, TTFuncBlank init, TTFuncBlank exec, TTFuncBlank teardown, TTSizes size, TTParallel parallel)
	{
		Construct();
		Context = context;
		Name = name;
		Init = init;
		Blank = exec;
		Teardown = teardown;
		Size = size;
		Parallel = parallel;
	}
	Test() { Construct(); }

	void Construct() { Context = NULL; Pass = false; Ignored = false; Time = 0; }
	double TimeoutSeconds() const
	{
		return Size == TTSizeSmall ? 5 * 60 : 20 * 60;	// Keep this in sync with TinyTest.h (TTSizeSmall, TTSizeLarge)
	}
	static int ParallelSort(TTParallel p)
	{
		switch (p)
		{
		case TTParallelDontCare: return 0;
		case TTParallelWholeCore: return 1;
		case TTParallelSolo: return 2;
		default: return 100;
		}
	}
	static bool CompareByParallel(const Test& a, const Test& b)
	{
		int pdiff = ParallelSort(a.Parallel) - ParallelSort(b.Parallel);
		if (pdiff != 0)
			return pdiff < 0;
		return strcmp(a.Name.c_str(), b.Name.c_str()) < 0;
	}
	static bool CompareByName(const Test& a, const Test& b)
	{
		return strcmp(a.Name.c_str(), b.Name.c_str()) < 0;
	}
};

struct OutputDev
{
	virtual             ~OutputDev() {}
	virtual bool		ImmediateOut() { return false; }
	virtual string		OutputFilename(string base) { return ""; }
	virtual string		HeadPre(TestContext& c) { return ""; }
	virtual string		HeadPost(TestContext& c) { return ""; }
	virtual string		Tail() = 0;
	virtual string		ItemPre(const Test& t) { return ""; }
	virtual string		ItemPost(const Test& t) = 0;
};

// This is our global state. A single instance of these exists for a single run of the application.
struct TestContext
{
	string					Name;
	string					Ident;
	vector<string>			Options;
	string					OutDir;
	string					OutText;
	OutputDev*				Out;
	bool					OutputBare;
	bool					RunLargeTests;
	int						NumThreads;
	vector<Test>			Tests;
	vector<string>			Include;
	vector<string>			Exclude;

	TestContext();
	~TestContext();

	int			ParseAndRun(int argc, char** argv);		// return value is process exit code

	int			CountPass()	const		{ int c = 0; for (size_t i = 0; i < Tests.size(); i++) c += Tests[i].Pass ? 1 : 0; return c; }
	int			CountIgnored() const	{ int c = 0; for (size_t i = 0; i < Tests.size(); i++) c += Tests[i].Ignored ? 1 : 0; return c; }
	int			CountFail() const		{ return (int) Tests.size() - CountPass() - CountIgnored(); }
	int			CountRun() const		{ return (int) Tests.size() - CountIgnored(); }
	double		TotalTime() const		{ double t = 0; for (size_t i = 0; i < Tests.size(); i++) t += Tests[i].Time; return t; }
	string		FullName() const		{ return Name + "-" + Ident; }

	void		SetOutText();
	void		SetOutJUnit();
	void		SetOutHtml();

	static void ExecuteThread(ThreadContext* context);

protected:
	vector<thread>		Threads;
	atomic<unsigned>	ExecTotal;		// Counts total number of executing threads
	atomic<unsigned>	ExecWholeCore;	// Counts the number of 'wholecore' jobs executing (or wanting to execute)
	int					SysNumCores;	// Number of physical (aka whole) cores in this machine
	mutex				QueueLock;		// Guards access to TestPos, as well as OutText
	size_t				TestPos;		// Queue position in 'Tests'. Starts at zero and ends at Tests.size(). Guarded by QueueLock.

	bool		CreateThreads();
	int			RunMaster();
	void		ListTests();
	int			ExecuteTestHere(const char* testname);
	bool		TestMatchesFilter(const Test& t) const;
	Test*		NextFilteredTest(int threadNum);
	void		RunTestOuter(int threadNum, Test* t);
	void		RunTestExec(string tempDir, Test* t);
	void		FlushOutput();
	void		WriteOutputToFile();
};

// This manages a single process that is executing a single test
struct Process
{
	TestContext*		Context;

	Process(TestContext* cx) : Context(cx) {}

	// Launch a process and wait for it to complete.
	// The return value contains the error if the process failed to launch.
	// If the return value is empty, then the process execution succeeded.
	string			ExecuteAndWait(string exec, const vector<string>& args, double timeoutSeconds, string& childStdOut, int& processExitCode);

private:
	unsigned int				ProcessID;			// Process ID of currently executing process
	vector<TT_PROCESS_HANDLE>	SubProcesses;		// Sub processes launched by the currently executing test

	void			ReadIPC(unsigned waitMS);
	void			CloseZombieProcesses();
	string			ExecuteAndWaitOnce(string exec, const vector<string>& args, double timeoutSeconds, string& childStdOut, int& processExitCode);
};

struct OutputDevText : public OutputDev
{
	virtual bool		ImmediateOut()					{ return true; }
	virtual string		HeadPre(TestContext& c)			{ return SPrintf("%s %s\n", c.Name.c_str(), c.Ident.c_str()); }
	virtual string		HeadPost(TestContext& c)		{ return SPrintf("%d/%d passed\n", (int) c.CountPass(), (int) c.CountRun()); }
	virtual string		Tail()							{ return ""; }
	virtual string		ItemPre(const Test& t)			{ return SPrintf("%-35s", t.Name.c_str()); }
	virtual string		ItemPost(const Test& t)
	{
		string r = SPrintf("%s", t.Pass ? "PASS" : "FAIL");
		if (!t.Pass)
			r += "\n" + t.TrimmedOutput;
		r += "\n";
		return r;
	}
};

struct OutputDevHtml : public OutputDev
{
	virtual string		OutputFilename(string base)	{ return base + ".html"; }
	virtual string		HeadPre(TestContext& c)		{ return HtmlHeadTxt; }
	virtual string		Tail()						{ return HtmlTailTxt; }
	virtual string		ItemPost(const Test& t)
	{
		if (t.Pass)	return SPrintf("<div class='test pass'>%s</div>\n", EscapeHtml(t.Name).c_str());
		else			return SPrintf("<div class='test fail'>%s<pre class='detail'>", EscapeHtml(t.Name).c_str()) + EscapeHtml(t.TrimmedOutput) + "</pre></div>\n";
	}
};

struct OutputDevJUnit : public OutputDev
{
	virtual string		OutputFilename(string base) { return base + ".xml"; }
	virtual string		HeadPost(TestContext& c)
	{
		return SPrintf("<testsuite name=\"%s\" time=\"%f\">\n", c.FullName().c_str(), (double) c.TotalTime());
	}
	virtual string		Tail()		{ return JUnitTailTxt; }
	virtual string		ItemPost(const Test& t)
	{
		if (t.Pass)	return SPrintf("\t\t<testcase name=\"%s\" classname=\"%s\" time=\"%f\"/>\n", EscapeHtml(t.Name).c_str(), t.Context->FullName().c_str(), (double) t.Time);
		else		return SPrintf("\t\t<testcase name=\"%s\" classname=\"%s\" time=\"%f\">\n"
								   "\t\t\t<failure>%s</failure>\n"
								   "\t\t</testcase>\n",
									EscapeHtml(t.Name).c_str(), t.Context->FullName().c_str(), (double) t.Time, EscapeXml(t.TrimmedOutput).c_str());
	}
};

void TTShowHelp()
{
	std::string msg =
		" [options] testname1 testname2...\n"
		"  testname(s)         Wildcards of the tests that you want to run, or '" TT_TOKEN_ALL_TESTS "' to run all.\n"
		"                      If you specify no tests, then all tests are listed.\n"
		"                      If you prefix the name of a test with a colon, for example :testname1, then the\n"
		"                      test harness assumes you are debugging that test. In this case, the test app does\n"
		"                      not launch a sub-process for each test, and issues a debug break intrinsic if an\n"
		"                      assertion fails.\n"
		"\n"
		"  -xEXCLUDE           Exclude any tests that match the wildcard EXCLUDE. Excludes override includes.\n"
		"  -tt-small           Run only small tests\n"
		"  -tt-html            Output as HTML\n"
		"  -tt-junit           Output as JUnit\n"
		"  -tt-bare            For HTML, exclude header and tail\n"
		"  -tt-head            For HTML, output only header\n"
		"  -tt-tail            For HTML, output only tail\n"
		"  -tt-out DIR         For JUnit, write xml files to this directory\n"
		"  -tt-ident IDENT     Give this test suite a name, such as 'win32-release'. This forms part of the JUnit xml file\n";
	msg = FilenameOnly(TTGetProcessPath()) + msg;
	fputs(msg.c_str(), stdout);
}

static int TTRun_Internal(const TT_TestList& tests, int argc, char** argv)
{
	TestContext cx;
	cx.SetOutText();
	cx.OutputBare = false;

	cx.Tests.resize(tests.size());
	for (int i = 0; i < tests.size(); i++)
	{
		cx.Tests[i] = Test(&cx, tests[i].Name, tests[i].Init, tests[i].Blank, tests[i].Teardown, tests[i].Size, tests[i].Parallel);

		if (cx.Tests[i].Name == TT_TOKEN_ALL_TESTS)	{ fprintf(stderr, "You may not name a test 'all'. That word is reserved for running all tests.\n"); return 1; }
		if (cx.Tests[i].Name == "help")				{ fprintf(stderr, "You may not name a test 'help'. That word is reserved for show test system documentation.\n"); return 1; }
	}

	return cx.ParseAndRun(argc, argv);
}

static void TTRun_PrepareExecutionEnvironment()
{
#ifdef _WIN32
	SetUnhandledExceptionFilter(TTExceptionHandler);
#endif
	setvbuf(stdout, NULL, _IONBF, 0);									// Disable all buffering on stdout, so that the last words of a dying process are recorded.
	setvbuf(stderr, NULL, _IONBF, 0);									// Same for stderr.
#ifdef _WIN32
	_set_error_mode(_OUT_TO_STDERR);									// prevent CRT dialog box popups. This doesn't work for debug builds.
	_set_purecall_handler(TTPurecallHandler);							// pure virtual function
	_set_invalid_parameter_handler(TTInvalidParameterHandler);		// CRT function with invalid parameters
#endif
	signal(SIGABRT, TTSignalHandler);									// handle calls to abort()
}

TestContext::TestContext()
{
	Out = NULL;
	RunLargeTests = true;
	NumThreads = 0;
	Name = FilenameOnly(TTGetProcessPath());
#ifdef _WIN32
	char buf[256];
	DWORD size = GetEnvironmentVariableA("TT_NUMTHREADS", buf, arraysize(buf));
	if (size > 0 && size < 10)
		NumThreads = atoi(buf);
#endif
}

TestContext::~TestContext()
{
	delete Out;
}

int TestContext::ParseAndRun(int argc, char** argv)
{
	string singleTestName;
	int nTestCmdlineArgs = 0;

	// skip the first command-line argument, which is the program path
	for (int i = 1; i < argc; i++)
	{
		const char* optc = argv[i];
		string opt = argv[i];

		if (opt[0] == '-')
		{
			if (opt == "-tt-html")										{ SetOutHtml(); }
			else if (opt == "-tt-junit")								{ SetOutJUnit(); }
			else if (opt == "-tt-small")								{ RunLargeTests = false; }
			else if (opt == "-tt-bare")									{ OutputBare = true; }
			else if (opt == "-tt-head")									{ fputs(HtmlHeadTxt, stdout); return 0; }
			else if (opt == "-tt-tail")									{ fputs(HtmlTailTxt, stdout); return 0; }
			else if (opt == "-tt-out")									{ OutDir = argv[i + 1]; i++; }
			else if (opt == "-tt-ident")								{ Ident = argv[i + 1]; i++; }
			else if (opt == "-?" || opt == "-help" || opt == "--help")	{ TTShowHelp(); return 1; }
			else if (opt.substr(0, 2) == "-x")							{ Exclude.push_back(opt.substr(2)); }
			else														{ Options.push_back(opt); }
		}
		else if (strstr(optc, TT_PREFIX_RUNNER_PID) == optc)			{ MasterPID = atoi(argv[i] + strlen(TT_PREFIX_RUNNER_PID)); }
		else if (strstr(optc, TT_PREFIX_TESTDIR) == optc)				{ TTSetTempDir(argv[i]); }
		else if (opt == "?" || opt == "help")							{ TTShowHelp(); return 1; }
		else if (opt == "test")
		{
			// Whenever the master invokes a test, it does it like so: "testapp.exe test =testname"
			// The 'test' there is a courtesy for the test application, so that it can also serve other functions
			// via its command line interface. This fact is also used by the function TTIsRunningUnderMaster().
			continue;
		}
		else if (opt[0] == ':' || opt[0] == '=')
		{
			IsExecutingUnderGuidance = opt[0] == '=';
			singleTestName = opt.substr(1);
		}
		else
		{
			// Once a test name has been specified, the remaining arguments are for the test itself
			if (singleTestName != "")
				TestCmdLineArgs[nTestCmdlineArgs++] = argv[i];
			else
				Include.push_back(opt);
		}
	}

	// This is useful when running under the IDE
	if (TTGetTempDir().length() == 0)
		TTSetTempDir(TT_PREFIX_TESTDIR DEFAULT_TEMP_DIR);

	TestCmdLineArgs[nTestCmdlineArgs++] = nullptr;

	if (singleTestName != "")
	{
		return ExecuteTestHere(singleTestName.c_str());
	}
	else if (Include.size() == 0 && Exclude.size() == 0)
	{
		ListTests();
		return 0;
	}
	else
	{
		return RunMaster();
	}
}

void TestContext::ListTests()
{
	sort(Tests.begin(), Tests.end(), Test::CompareByName);
	printf("%s\n", TT_LIST_LINE_1);
	printf("%s\n", TT_LIST_LINE_2);
	for (int i = 0; i < Tests.size(); i++)
		printf("  %-40s %-20s %-20s\n", Tests[i].Name.c_str(), Tests[i].Size == TTSizeSmall ? TT_SIZE_SMALL_NAME : TT_SIZE_LARGE_NAME, ParallelName(Tests[i].Parallel));
}

int TestContext::ExecuteTestHere(const char* testname)
{
	if (IsExecutingUnderGuidance)
		TTRun_PrepareExecutionEnvironment();

	for (int i = 0; i < Tests.size(); i++)
	{
		if (Tests[i].Name == testname)
		{
			if (Tests[i].Init)
				Tests[i].Init();

			Tests[i].Blank();

			if (Tests[i].Teardown)
				Tests[i].Teardown();

			return 0;
		}
	}

	fprintf(stderr, "Test '%s' not found\n", testname);
	return 1;
}

bool TestContext::TestMatchesFilter(const Test& t) const
{
	if (t.Size == TTSizeLarge && !RunLargeTests)
		return false;

	bool pass = false;
	for (size_t i = 0; i < Include.size(); i++)
	{
		if (Include[i] == TT_TOKEN_ALL_TESTS)
			pass = true;
		else if (MatchWildcardNocase(t.Name, Include[i]))
			pass = true;
	}
	for (size_t i = 0; i < Exclude.size(); i++)
	{
		if (MatchWildcardNocase(t.Name, Exclude[i]))
			pass = false;
	}
	return pass;
}

Test* TestContext::NextFilteredTest(int threadNum)
{
	// Find the next test that matches the filter
	lock_guard<mutex> lock(QueueLock);
	while (TestPos < Tests.size())
	{
		Test* t = &Tests[TestPos];
		if (!TestMatchesFilter(*t))
		{
			t->Ignored = true;
			TestPos++;
			continue;
		}
		if (t->Parallel == TTParallelSolo && threadNum != 0)
		{
			// Tests are sorted so that Solo tests are at the end of the list, and since we
			// are not thread 0, we don't run those. Therefore, we are finished.
			return NULL;
		}
		DEBUG_THREADS("Thread %d executing test %s (acquire)\n", threadNum, t->Name.c_str());
		TestPos++;
		return t;
	}
	// all tests are finished
	return NULL;
}

void TestContext::RunTestOuter(int threadNum, Test* t)
{
	// If there are 4 physical CPU cores, and 8 threads, then the blessed threads
	// are the first 4. Whenever a 'whole core' job is running, then all non-blessed threads
	// go to sleep.
	bool isBlessedThread = threadNum < SysNumCores;

	const int sleepMS = 20;

	string outText;
	auto flushOutput = [&]()
	{
		if (Out->ImmediateOut())
		{
			fputs(outText.c_str(), stdout);
			outText = "";
		}
	};

	// Hold off if our computational device is being requested
	for (int iwait = 0; ExecWholeCore.load() != 0 && !isBlessedThread; iwait++)
	{
		if (iwait == 0)
			DEBUG_THREADS("Thread %d pausing because not blessed and ExecWholeCore = %d\n", threadNum, ExecWholeCore.load());
		MySleep(sleepMS);
	}

	for (int iwait = 0; t->Parallel == TTParallelSolo && ExecTotal.load() != 0; iwait++)
	{
		if (iwait == 0)
			DEBUG_THREADS("Thread %d pausing, waiting for all other threads to go quiet before executing solo job\n", threadNum);
		MySleep(sleepMS);
	}

	outText += Out->ItemPre(*t);

	// Form a temporary directory that is unique for this test
	// We use our own process ID combined with the full name of the test
	string tempDir = string(TT_TEST_DIR) + IntToStr(TTGetProcessID()) + "_" + Ident + "_" + Name + "_" + t->Name;

	DeleteDirectoryRecursive(tempDir);

	if (MyCreateDirectory(tempDir))
	{
		int lockfile = -1;
		if (t->Parallel == TTParallelSolo)
		{
			double start = TimeSeconds();
			while (lockfile == -1 && TimeSeconds() - start < TTGlobalLockTimeoutSeconds)
			{
				lockfile = OpenLockFile(TT_TEST_GLOBAL_LOCK_FILE);
				if (lockfile == -1)
					MySleep(1000);
			}
			if (lockfile == -1)
			{
				t->Pass = false;
				t->Output += "Timed out waiting for global lock (test is marked as 'exclusive' across the whole machine)";
			}
		}

		if (t->Parallel != TTParallelSolo || lockfile != -1)
		{
			DEBUG_THREADS("Thread %d executing test %s (begin)\n", threadNum, t->Name.c_str());
			RunTestExec(tempDir, t);
		}

		CloseLockFile(lockfile);
		DeleteDirectoryRecursive(tempDir);
	}
	else
	{
		t->Pass = false;
		t->Output += "Failed to create test directory '" + tempDir + "'";
	}

	t->TrimmedOutput = t->Output;
	if (t->TrimmedOutput.length() > 1000)
		t->TrimmedOutput = "..." + t->TrimmedOutput.substr(t->TrimmedOutput.length() - 1000);

	outText += Out->ItemPost(*t);
	flushOutput();
	DEBUG_THREADS("Thread %d executing test %s (retire)\n", threadNum, t->Name.c_str());

	{
		lock_guard<mutex> lock(QueueLock);
		OutText += outText;
	}
}

void TestContext::RunTestExec(string tempDir, Test* t)
{
	// Warning: Take care when changing this command line. The function TTIsRunningUnderMaster() uses it's layout quite specifically.
	string tempDirSlash = tempDir; // + DIR_SEP_STR;  --- never end a quoted string with a backslash. The backslash before the quote causes the quote to be escaped.
	//string exec = TTGetProcessPath() + " " + TT_TOKEN_INVOKE + " =" + t->Name + " " + TT_PREFIX_RUNNER_PID + IntToStr(TTGetProcessID()) + " \"" + TT_PREFIX_TESTDIR + tempDirSlash + "\"" + " " + Options;
	vector<string> args;
	args.push_back(TT_TOKEN_INVOKE);
	args.push_back("=" + t->Name);
	args.push_back(TT_PREFIX_RUNNER_PID + IntToStr(TTGetProcessID()));
	args.push_back(TT_PREFIX_TESTDIR + tempDirSlash);
	for (const auto& opt : Options)
		args.push_back(opt);

	if (t->Parallel == TTParallelWholeCore)
		ExecWholeCore++;

	ExecTotal++;

	string pstdOut;
	int pstatus = 0;
	double tstart = TimeSeconds();
	Process proc(this);
	string execError = proc.ExecuteAndWait(TTGetProcessPath(), args, t->TimeoutSeconds(), pstdOut, pstatus);
	if (execError != "")
	{
		pstatus = 1;
		pstdOut = "TinyTest ExecProcess Error: " + execError;
	}

	ExecTotal--;

	if (t->Parallel == TTParallelWholeCore)
		ExecWholeCore--;

	t->Time = TimeSeconds() - tstart;
	t->Output = pstdOut;
	t->Pass = pstatus == 0;
}

void TestContext::FlushOutput()
{
	if (Out->ImmediateOut())
	{
		fputs(OutText.c_str(), stdout);
		OutText = "";
	}
}

void TestContext::WriteOutputToFile()
{
	string outf;
	if (OutDir != "")
	{
		outf = OutDir;
		if (outf.back() != DIR_SEP) outf += DIR_SEP;
		outf += Name;
		outf += "-" + Ident;
		outf = Out->OutputFilename(outf);
	}
	if (!Out->ImmediateOut())
	{
		if (outf != "")
			WriteWholeFile(outf, OutText);
		else
			fputs(OutText.c_str(), stdout);
	}
}

void TestContext::SetOutText()	{ delete Out; Out = new OutputDevText; }
void TestContext::SetOutJUnit()	{ delete Out; Out = new OutputDevJUnit; }
void TestContext::SetOutHtml()	{ delete Out; Out = new OutputDevHtml; }

struct ThreadContext
{
	atomic<int>		Consumed;
	TestContext*	TestCX;
	int				ThreadNumber;
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 6001) // Using uninitialized memory 'tx' --- looks like a false positive on VS 2015 Update 3
#endif

bool TestContext::CreateThreads()
{
	int logicalCores = 0;
	GetNumCPUCores(SysNumCores, logicalCores);

	int nThreads = logicalCores;
	if (NumThreads > 0)
		nThreads = NumThreads;

	DEBUG_THREADS("Launching %d threads\n", nThreads);

	for (int i = 0; i < nThreads; i++)
	{
		ThreadContext tx;
		tx.Consumed = 0;
		tx.TestCX = this;
		tx.ThreadNumber = i;
		Threads.push_back(thread(ExecuteThread, &tx));
		// ExecuteThread knows that once it sets Consumed to non-zero, it is not allowed to touch 'tx' again.
		while (tx.Consumed == 0)
			MySleep(0);
	}

	return true;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

void TestContext::ExecuteThread(ThreadContext* context)
{
	TestContext* cx = context->TestCX;
	int threadNum = context->ThreadNumber;
	context->Consumed = 1;
	// context is now invalid, since we set Consumed to 1.

	DEBUG_THREADS_CX("Thread %d start\n", threadNum);

	while (true)
	{
		Test* t = cx->NextFilteredTest(threadNum);
		if (t == nullptr)
			break;
		cx->RunTestOuter(threadNum, t);
	}

	DEBUG_THREADS_CX("Thread %d exit\n", threadNum);
}

int TestContext::RunMaster()
{
	if (!OutputBare) OutText += Out->HeadPre(*this);
	FlushOutput();

	TestPos = 0;
	ExecTotal = 0;
	ExecWholeCore = 0;

	// Get all solo tests to run at the end
	// Only thread 0 runs solo tests.
	sort(Tests.begin(), Tests.end(), Test::CompareByParallel);

	bool ok = CreateThreads();
	if (!ok)
	{
		OutText += "Unable to create threads";
		printf("Unable to create threads\n");
	}

	if (ok)
	{
		DEBUG_THREADS("Waiting for all %d threads to join\n", (int) Threads.size());
		for (size_t i = 0; i < Threads.size(); i++)
			Threads[i].join();
		DEBUG_THREADS("Threads joined\n");
		Threads.clear();
	}

	int nrun = 0;
	int npass = 0;
	for (size_t i = 0; i < Tests.size() && ok; i++)
	{
		nrun += Tests[i].Ignored ? 0 : 1;
		npass += Tests[i].Pass ? 1 : 0;
	}

	if (!OutputBare && ok)
	{
		OutText = Out->HeadPost(*this) + OutText;
		OutText += Out->Tail();
		FlushOutput();
	}

	WriteOutputToFile();

	return (ok && npass == nrun) ? 0 : 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

string Process::ExecuteAndWait(string exec, const vector<string>& args, double timeoutSeconds, string& childStdOut, int& processExitCode)
{
	// This is a very blind guess that attempts to resolve an issue that I have been seeing on our CI machines
	// after enabling parallel tests. The symptom is that the process exits with code 0xC0000142, and produces
	// no output. It might just be related to this: http://support.microsoft.com/kb/960266
	string res;
	for (int attempt = 0; attempt < 5; attempt++)
	{
		res = ExecuteAndWaitOnce(exec, args, timeoutSeconds, childStdOut, processExitCode);
		CloseZombieProcesses();
		if (processExitCode != 0xC0000142 && processExitCode != 0xC0000005)
			break;
	}
	return res;
}

#ifdef _WIN32

static string MakeCommandLine(string exec, const vector<string>& args)
{
	for (const auto& arg : args)
	{
		exec += " ";
		if (arg.find(' ') != string::npos)
		{
			exec += " \"";
			exec += arg;
			exec += " \"";
		}
		else
		{
			exec += arg;
		}
	}
	return exec;
}

string Process::ExecuteAndWaitOnce(string exec, const vector<string>& args, double timeoutSeconds, string& childStdOut, int& processExitCode)
{
	TestContext* cx = Context;
	ProcessID = 0;

	//HANDLE hChildStd_IN_Rd = NULL;
	//HANDLE hChildStd_IN_Wr = NULL;
	HANDLE hChildStd_OUT_Rd = NULL;
	HANDLE hChildStd_OUT_Wr = NULL;

	SECURITY_ATTRIBUTES saAttr;
	memset(&saAttr, 0, sizeof(saAttr));

	//printf("\n->Start of parent execution.\n");

	// Set the bInheritHandle flag so pipe handles are inherited.

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT.

	if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
		return "StdoutRd CreatePipe";

	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
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
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);
	//si.hStdInput = hChildStd_IN_Rd;
	si.hStdError = hChildStd_OUT_Wr;
	si.hStdOutput = hChildStd_OUT_Wr;
	si.dwFlags = STARTF_USESTDHANDLES;

	char execPath[4096];
	string cmdLine = MakeCommandLine(exec, args).c_str();
	if (cmdLine.length() > 4095)
		return SPrintf("Command-line too long (%s)\n", cmdLine.c_str());
	strcpy(execPath, cmdLine.c_str());

	if (!CreateProcessA(NULL, execPath, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		processExitCode = 1;
		return SPrintf("CreateProcess(%s) failed: %d\n", exec.c_str(), (int) GetLastError());
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

	if (!CloseHandleAndZero(hChildStd_OUT_Wr))
		return "StdOutWr CloseHandle";

	// Read from the standard output of the child process.
	char buf[1024];
	DWORD nread = 0;
	int64_t nIdle = 0;
	bool processWasDead = false;
	double start = TimeSeconds();
	for (int64_t tick = 0; true; tick++)
	{
		if (tick % 500 == 0)
			DEBUG_THREADS_CX("Waiting for %s to finish\n", exec.c_str());

		// Reset nIdle when process dies
		bool processDead = WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0;
		if (processDead && !processWasDead)
		{
			nIdle = 0;
			processWasDead = true;
		}

		// Try waiting extra long if we have no output yet from child
		int64_t nIdleThreshold = childStdOut.length() == 0 ? 10 : 5;
		if (nIdle > nIdleThreshold && processDead)
		{
			DEBUG_THREADS_CX("nIdle > nIdleThreshold\n");
			break;
		}

		DWORD avail = 0;
		BOOL peekOK = PeekNamedPipe(hChildStd_OUT_Rd, NULL, 0, NULL, &avail, NULL);
		if (!peekOK)
		{
			DEBUG_THREADS_CX("PeekOK = false, breaking out\n");
			break;
		}
		if (avail == 0)
		{
			nIdle++;
			if (TimeSeconds() - start > timeoutSeconds)
				break;
			ReadIPC(16);
			continue;
		}

		if (!ReadFile(hChildStd_OUT_Rd, buf, sizeof(buf), &nread, NULL))
		{
			DEBUG_THREADS_CX("ReadFile failed, breaking out\n");
			if (GetLastError() == ERROR_BROKEN_PIPE) {}   // normal
			else
				childStdOut += SPrintf("TinyTest: Unexpected ReadFile error on child process pipe: %d \n", (int) GetLastError());
			break;
		}
		childStdOut.append(buf, nread);
	}

	DEBUG_THREADS_CX("%s took %f seconds\n", exec.c_str(), TimeSeconds() - start);

	double timeRemaining = timeoutSeconds - (TimeSeconds() - start);
	timeRemaining = TTMax(timeRemaining, 0.0);
	bool timeout = WaitForSingleObject(pi.hProcess, DWORD(timeRemaining * 1000)) != WAIT_OBJECT_0;

	if (timeout)
		TerminateProcess(pi.hProcess, 1);
	else
		ReadIPC(0);

	DWORD exitcode = 111;
	if (timeout)
		exitcode = 555;
	else
		GetExitCodeProcess(pi.hProcess, &exitcode);
	processExitCode = exitcode;

	if (timeout)
		childStdOut += SPrintf("\nTest timed out (max time %f seconds)", timeoutSeconds);
	else if (exitcode != 0 && Trim(childStdOut).length() == 0)
		childStdOut += SPrintf("\nExit code %08X, but test produced no output", exitcode);

	CloseHandleAndZero(pi.hProcess);
	CloseHandleAndZero(pi.hThread);

	//CloseHandleAndZero( hChildStd_IN_Rd );
	CloseHandleAndZero(hChildStd_OUT_Rd);

	return "";
}

#else

static void SetNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL) | O_NONBLOCK;
	if (-1 == fcntl(fd, F_SETFL, flags))
		fprintf(stderr, "couldn't unblock fd %d\n", fd);
}

static bool ReadFromFile(int fd, vector<char>& out)
{
	while (true)
	{
		char buf[4096];
		auto nbytes = read(fd, buf, sizeof(buf));
		if (nbytes <= 0)
		{
			if (errno == EAGAIN)
				return true;
			else
				return false;
		}
		for (decltype(nbytes) i = 0; i < nbytes; i++)
			out.push_back(buf[i]);
	}
	// practically unreachable
	return false;
}

// NOTE: We discard stderr, although we go through all the motions to save it.
// Should you need it at some point, add it as a parameter to this function
string Process::ExecuteAndWaitOnce(string exec, const vector<string>& args, double timeoutSeconds, string& rStdOut, int& pExit)
{
	ProcessID = 0;
	const int pipe_read = 0;
	const int pipe_write = 1;

	int pipe_stdout[2];
	int pipe_stderr[2];

	if (pipe(pipe_stdout) != 0 || pipe(pipe_stderr) != 0)
		return "Error executing test sub-process (pipe open)";

	vector<char> rStdOutBuf;
	vector<char> rStdErrBuf;

	pid_t pid = fork();
	if (pid == 0)
	{
		// child
		//const char *args[] = { "/bin/sh", "-c", exec.c_str(), nullptr };

		//sigset_t sigs;
		//sigfillset(&sigs);
		//if ( 0 != sigprocmask(SIG_UNBLOCK, &sigs, 0) )
		//	printf( "sigprocmask failed\n" );

		close(pipe_stdout[pipe_read]);
		close(pipe_stderr[pipe_read]);

		if (-1 == dup2(pipe_stdout[pipe_write], STDOUT_FILENO) || -1 == dup2(pipe_stderr[pipe_write], STDERR_FILENO))
		{
			printf("dup2 failed\n");
			exit(1);
		}

		close(pipe_stdout[pipe_write]);
		close(pipe_stderr[pipe_write]);

		vector<const char*> pArgs;
		string allArgs;
		for (const auto& arg : args)
		{
			pArgs.push_back(arg.c_str());
			allArgs += " " + arg;
		}
		pArgs.push_back(nullptr);

		//if (-1 == execv("/bin/sh", (char **) args))
		if (-1 == execv(exec.c_str(), (char**) &pArgs[0]))
		{
			printf("execv failed in forked child (%d %s %s)\n", errno, exec.c_str(), allArgs.c_str());
			exit(1);
		}
		// we never get here
		abort();
	}
	else if (pid > 0)
	{
		// parent, success
		ProcessID = pid;
		int n_fds = 2;
		int fds[2];
		fd_set read_fds;
		fds[0] = pipe_stdout[pipe_read];
		fds[1] = pipe_stderr[pipe_read];

		SetNonBlocking(fds[0]);
		SetNonBlocking(fds[1]);

		close(pipe_stdout[pipe_write]);
		close(pipe_stderr[pipe_write]);

		double tstart = TimeSeconds();
		int exitCount = 0;

		// after the process has exited, try at least once to read stdout.
		// According to the tundra code, this produces a deadlock on OSX. I don't understand how though.
		while (exitCount < 2)
		{
			FD_ZERO(&read_fds);
			int max_fd = 0;
			for (int i = 0; i < n_fds; i++)
			{
				if (fds[i] > max_fd)
					max_fd = fds[i];
				FD_SET(fds[i], &read_fds);
			}

			struct timeval timeout;
			timeout.tv_sec = 5;
			timeout.tv_usec = 500000;

			if (n_fds != 0)
			{
				int count = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
				if (count == -1)    // according to tundra code, this happens in gdb due to syscall interruption
					continue;

				for (int i = 0; i < n_fds; i++)
				{
					vector<char>* buf = fds[i] == pipe_stdout[pipe_read] ? &rStdOutBuf : &rStdErrBuf;
					if (FD_ISSET(fds[i], &read_fds))
					{
						if (!ReadFromFile(fds[i], *buf))
						{
							// pipe is closed
							n_fds--;
							if (i == 0)
								fds[0] = fds[1];
						}
					}
				}
			}

			if (exitCount != 0)
			{
				exitCount++;
				continue;
			}

			pid_t pwait = waitpid(pid, &pExit, WNOHANG);
			if (pwait == 0)
			{
				// child still running
				ReadIPC(16);
				if (timeoutSeconds > 0 && TimeSeconds() - tstart > timeoutSeconds)
				{
					pExit = 1;
					string msg = SPrintf("\nTest timed out (max time %f seconds)\n", timeoutSeconds);
					size_t len = msg.length();
					for (size_t i = 0; i < len; i++)
						rStdOutBuf.push_back(msg[i]);
					break;
				}

				if (n_fds == 0)
					usleep(500000);
			}
			else if (pwait != pid)
			{
				// waitpid failed
				printf("waitpid failed (%d)\n", (int) pwait);
				pExit = 1;
				break;
			}
			else
			{
				// child has exited
				//printf( "forked parent: child has exited (%d). %s\n", pExit, n_fds == 0 ? "and pipes are closed" : "waiting for pipes to close" );
				exitCount = 1;
				if (n_fds == 0)
					break;
			}
		}

		rStdOut.append(&rStdOutBuf[0], rStdOutBuf.size());
		// NOTE: we discard rStdErrBuf

		close(pipe_stdout[pipe_read]);
		close(pipe_stderr[pipe_read]);
		return "";
	}
	else
	{
		// parent, error
		printf("fork failed\n");
		close(pipe_stdout[pipe_read]);
		close(pipe_stdout[pipe_write]);
		close(pipe_stderr[pipe_read]);
		close(pipe_stderr[pipe_write]);
		return "Error executing test sub-process (fork failed)";
	}
}
#endif

void Process::ReadIPC(unsigned waitMS)
{
	char raw[TT_IPC_MEM_SIZE];
	memset(raw, 0, sizeof(raw));

	char filename[256];
	TT_IPC_Filename(true, TTGetProcessID(), ProcessID, filename);
	bool haveData = TT_IPC_Read_Raw(waitMS, filename, raw);

	if (haveData)
	{
		raw[TT_IPC_MEM_SIZE - 1] = 0;
		char cmd[200];
		unsigned u_p0 = 0;
		// Currently the only command is "subprocess-launch", so we don't even bother to check the value of 'cmd'
		if (sscanf(raw, "%200s %u", cmd, &u_p0) == 2)
		{
			// This has potential to fail, if the process has already terminated by the time we get here.
			// I'm assuming that if we can't open the process, then it has already died, and all is well.
#ifdef _WIN32
			HANDLE proc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, false, u_p0);
			if (proc != NULL)
				SubProcesses.push_back(proc);
#else
			SubProcesses.push_back(u_p0);
#endif
		}
		else
		{
			fprintf(stderr, "TinyTest: Unrecognized IPC command\n");
		}
	}
}

void Process::CloseZombieProcesses()
{
#ifdef _WIN32
	for (size_t i = 0; i < SubProcesses.size(); i++)
	{
		if (WaitForSingleObject(SubProcesses[i], 0) != WAIT_OBJECT_0)
			TerminateProcess(SubProcesses[i], 1);
		CloseHandle(SubProcesses[i]);
	}
#else
	for (size_t i = 0; i < SubProcesses.size(); i++)
	{
		int status = 0;
		if (waitpid(SubProcesses[i], &status, WNOHANG) != SubProcesses[i])
			kill(SubProcesses[i], SIGKILL);
	}
#endif
	SubProcesses.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

string Escape(bool xml, const string& s)
{
	string r;
	for (size_t i = 0; i < s.length(); i++)
	{
		if (s[i] == '<') r += "&lt;";
		else if (s[i] == '>') r += "&gt;";
		else if (s[i] == '&') r += "&amp;";
		else if (s[i] == '\r') {}
		else if (s[i] == '\n') r += "&#10;";
		else r += s[i];
	}
	return r;
}

string EscapeHtml(const string& s)
{
	return Escape(false, s);
}

string EscapeXml(const string& s)
{
	return Escape(true, s);
}

vector<string> DiscardEmpties(const vector<string>& s)
{
	vector<string> result;
	for (size_t i = 0; i < s.size(); i++)
	{
		if (s[i] != "")
			result.push_back(s[i]);
	}
	return result;
}

vector<string> Split(const string& s, char delim)
{
	vector<string> res;
	size_t start = 0;
	size_t end = 0;
	for (; end < s.length(); end++)
	{
		if (s[end] == delim)
		{
			res.push_back(s.substr(start, end - start));
			start = end + 1;
		}
	}
	if (end - start > 0 || start != 0)
		res.push_back(s.substr(start, end - start));
	return res;
}

string Trim(const string& s)
{
	size_t i = 0;
	size_t j = s.length() - 1;
	while (i != s.length()	&& (s[i] == 9 || s[i] == 10 || s[i] == 13 || s[i] == 32)) i++;
	while (j != -1			&& (s[j] == 9 || s[j] == 10 || s[j] == 13 || s[j] == 32)) j--;
	ptrdiff_t len = 1 + j - i;
	if (len > 0)
		return s.substr(i, len);
	else
		return "";
}

string Directory(const string& s)
{
	size_t len = s.length();
	size_t lastSlash = s.rfind(DIR_SEP);
	if (lastSlash == len - 1)
		lastSlash = s.rfind('/', len - 2);

	return s.substr(0, lastSlash);
}

string FilenameOnly(const string& s)
{
	size_t dot = s.length();
	size_t i = s.length() - 1;
	for (; i != -1; i--)
	{
		if (s[i] == '.')
			dot = i;
		if (s[i] == '/' || s[i] == '\\')
			break;
	}
	return s.substr(i + 1, dot - i - 1);
}

string SPrintf(_In_z_ _Printf_format_string_ const char* format, ...)
{
	string result;
	va_list va;
	va_start(va, format);
	int size = 0;
#ifdef _MSC_VER
	size = _vscprintf(format, va);
#else
	size = (int) vsnprintf(nullptr, 0, format, va);
#endif
	va_end(va);

	va_start(va, format);
	char* tmp = (char*) malloc(size + 1);
	vsprintf(tmp, format, va);
	va_end(va);

	result = tmp;
	free(tmp);
	return result;
}

bool MatchWildcardNocase(const string& s, const string& p)
{
	return MatchWildcardNocase(s.c_str(), p.c_str());
}

bool MatchWildcardNocase(const char *s, const char *p)
{
	if (*p == '*')
	{
		while (*p == '*')									++p;
		if (*p == '\0')									return true;
		while (*s != '\0' && !MatchWildcardNocase(s, p))	++s;
		return *s != '\0';
	}
	else if (*p == '\0' || *s == '\0')					return tolower(*p) == tolower(*s);
	else if (tolower(*p) == tolower(*s) || *p == '?')		return MatchWildcardNocase(++s, ++p);
	else									return false;
}

bool WriteWholeFile(const string& filename, const string& s)
{
	bool good = false;
	FILE* f = fopen(filename.c_str(), "wb");
	if (f)
	{
		if (s.length() != 0)
			good = fwrite(&s[0], s.length(), 1, f) == 1;
		else
			good = true;
		fclose(f);
	}
	return good;
}

string IntToStr(int64_t i)
{
	char buf[100];
	sprintf(buf, "%lld", (long long) i);
	return buf;
}

#ifdef _WIN32
double TimeSeconds()
{
	LARGE_INTEGER t, f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
	return t.QuadPart / (double) f.QuadPart;
}
#else
double TimeSeconds()
{
	timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec * (1.0 / 1000000000);
}
#endif

#ifdef _WIN32
BOOL CloseHandleAndZero(HANDLE& h)
{
	BOOL c = CloseHandle(h);
	h = NULL;
	return c;
}
#endif

TTParallel ParseParallel(const char* s)
{
	if (0 == strcmp(s, TT_PARALLEL_DONTCARE)) return TTParallelDontCare;
	if (0 == strcmp(s, TT_PARALLEL_WHOLECORE)) return TTParallelWholeCore;
	if (0 == strcmp(s, TT_PARALLEL_SOLO)) return TTParallelSolo;
	return TTParallel_Invalid;
}

bool MyCreateDirectory(const string& path)
{
	// count the nesting level
	int iparent = 0;
	string findRoot = path;
	for (; iparent < 20; iparent++)
	{
		string parent = Directory(findRoot);
		if (parent == findRoot)
			break;
		findRoot = parent;
	}

	// create all parent directories, from the top down
	for (; iparent != 0; iparent--)
	{
		string parent = path;
		for (int i = 1; i < iparent; i++)
			parent = Directory(parent);
#ifdef _WIN32
		int res = _mkdir(parent.c_str());
#else
		int res = mkdir(parent.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
		if (res != 0 && errno != EEXIST)
			return false;
	}
	return true;
}

bool DeleteDirectoryRecursive(const string& path)
{
	// Check first, to avoid shell messages on the console
	if (!DirectoryExists(path))
		return true;
#ifdef _WIN32
	return 0 == system(("rmdir /s /q \"" + path + "\"").c_str());
#else
	return 0 == system(("rm -rf \"" + path + "\"").c_str());
#endif
}

bool DirectoryExists(const string& s)
{
#ifdef _WIN32
	return GetFileAttributesA(s.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat st;
	return stat(s.c_str(), &st) != 0;
#endif
}

// Return <= 0 on failure
int OpenLockFile(const string& path)
{
#ifdef _WIN32
	int mode = _S_IREAD | _S_IWRITE;
#else
	int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif

	int f = open(path.c_str(), O_CREAT | O_WRONLY, mode);
	if (f == -1)
		return -1;

#ifdef _WIN32
	int lock = _locking(f, _LK_NBLCK, 1);
#else
#	ifdef ANDROID
	int lock = flock(f, LOCK_EX);
#	else
	int lock = lockf(f, F_TLOCK, 1);
#	endif
#endif
	if (lock == -1)
	{
		close(f);
		return -1;
	}

	return f;
}

void CloseLockFile(int f)
{
	if (f == -1)
		return;

	lseek(f, 0, SEEK_SET);
#ifdef _WIN32
	_locking(f, _LK_UNLCK, 1);
#else
#	ifdef ANDROID
	flock(f, LOCK_UN);
#	else
	lockf(f, F_ULOCK, 1);
#	endif
#endif
	close(f);
}

void MySleep(unsigned ms)
{
	this_thread::sleep_for(chrono::milliseconds(ms));
}

void GetNumCPUCores(int& physicalCores, int& logicalCores)
{
#ifdef _WIN32
	SYSTEM_INFO inf;
	GetSystemInfo(&inf);
	physicalCores = (int) inf.dwNumberOfProcessors;
	logicalCores = (int) inf.dwNumberOfProcessors;

	SYSTEM_LOGICAL_PROCESSOR_INFORMATION log[256];
	DWORD logSize = sizeof(log);
	if (GetLogicalProcessorInformation(log, &logSize))
	{
		physicalCores = 0;
		for (unsigned i = 0; i < logSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); i++)
		{
			if (log[i].Relationship == RelationProcessorCore)
				physicalCores++;
		}
	}
#else
	logicalCores = sysconf(_SC_NPROCESSORS_ONLN);
	physicalCores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
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


#undef DEBUG_THREADS_CX
#undef DEBUG_THREADS
