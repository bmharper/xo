#include "pch.h"

#include "TinyTest.h"

#ifdef _MSC_VER
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif

struct Test;
struct TestContext;
struct OutputDev;

extern const char* HtmlHeadTxt;
extern const char* HtmlTailTxt;
extern const char* JUnitTailTxt;

void		Run( XStringA path, XStringA testcmd, XStringA testname );
XStringA	EscapeHtml( XStringA s );
XStringA	EscapeXml( XStringA s );

struct Test
{
	TestContext*	Context;
	XStringA		Name;
	XStringA		Output;
	XStringA		TrimmedOutput;
	TTSizes			Size;
	bool			Ignored;
	bool			Pass;
	double			Time;

	explicit Test( TestContext* context, XStringA name, TTSizes size ) 
	{
		Construct();
		Context = context;
		Name = name;
		Size = size;
	}
	Test() { Construct(); }

	void Construct() { Context = NULL; Pass = false; Ignored = false; Time = 0; }
	double TimeoutSeconds() const
	{
		return Size == TTSizeSmall ? 120 : 15 * 60;
	}
};

struct OutputDev
{
	virtual bool		ImmediateOut() { return false; }
	virtual XStringA	OutputFilename( XStringA base ) { return ""; }
	virtual XStringA	HeadPre( TestContext& c ) { return ""; }
	virtual XStringA	HeadPost( TestContext& c ) { return ""; }
	virtual XStringA	Tail() = 0;
	virtual XStringA	ItemPre( const Test& t ) { return ""; }
	virtual XStringA	ItemPost( const Test& t ) = 0;
};

struct TestContext
{
	XStringA			ExePath;
	XStringA			Name;
	XStringA			Ident;
	XStringA			Options;
	XStringA			OutDir;
	XStringA			OutText;
	OutputDev*			Out;
	bool				OutputBare;
	bool				RunLargeTests;
	podvec<Test>		Tests;
	podvec<XStringA>	Include;
	podvec<XStringA>	Exclude;
	podvec<HANDLE>		SubProcesses;	// Sub processes launched by the currently executing test
	TT_IPC_Block		IPC;

				TestContext();
				~TestContext();

	int			EnumAndRun( XStringA exepath, int argc, char** argv );

	int			CountPass()	const		{ int c = 0; for ( intp i = 0; i < Tests.size(); i++ ) c += Tests[i].Pass ? 1 : 0; return c; }
	int			CountIgnored() const	{ int c = 0; for ( intp i = 0; i < Tests.size(); i++ ) c += Tests[i].Ignored ? 1 : 0; return c; }
	int			CountFail() const		{ return (int) Tests.size() - CountPass() - CountIgnored(); }
	int			CountRun() const		{ return (int) Tests.size() - CountIgnored(); }
	double		TotalTime() const		{ double t = 0; for ( intp i = 0; i < Tests.size(); i++ ) t += Tests[i].Time; return t; }
	XStringA	FullName() const		{ return Name + "-" + Ident; }

	void		SetOutText();
	void		SetOutJUnit();
	void		SetOutHtml();

protected:
	int			Run();
	XStringA	ExecProcess( XStringA exec, XStringA& output, double timeoutSeconds, int& result );
	bool		EnumTests();
	bool		TestMatchesFilter( const Test& t );
	void		FlushOutput();
	void		WriteOutputToFile();
	void		ReadIPC( uint waitMS );
	void		CloseZombieProcesses();
};

struct OutputDevText : public OutputDev
{
	virtual bool		ImmediateOut()					{ return true; }
	virtual XStringA	HeadPre( TestContext& c )		{ return fmt("%s %s\n", c.Name, c.Ident); }
	virtual XStringA	HeadPost( TestContext& c )		{ return fmt("%d/%d passed\n", c.CountPass(), c.CountRun()); }
	virtual XStringA	Tail()							{ return ""; }
	virtual XStringA	ItemPre( const Test& t )		{ return fmt( "%-35s", t.Name ); }
	virtual XStringA	ItemPost( const Test& t )
	{
		XStringA r = fmt( "%s", t.Pass ? "PASS" : "FAIL" );
		if ( !t.Pass )
			r += "\n" + t.TrimmedOutput;
		r += "\n";
		return r;
	}
};

struct OutputDevHtml : public OutputDev
{
	virtual XStringA	OutputFilename( XStringA base ) { return base + ".html"; }
	virtual XStringA	HeadPre( TestContext& c )		{ return HtmlHeadTxt; }
	virtual XStringA	Tail()							{ return HtmlTailTxt; }
	virtual XStringA	ItemPost( const Test& t )
	{
		if ( t.Pass )	return fmt( "<div class='test pass'>%s</div>\n", EscapeHtml(t.Name) );
		else			return fmt( "<div class='test fail'>%s<pre class='detail'>%s</pre></div>\n", EscapeHtml(t.Name), EscapeHtml(t.TrimmedOutput) );
	}
};

struct OutputDevJUnit : public OutputDev
{
	virtual XStringA	OutputFilename( XStringA base ) { return base + ".xml"; }
	virtual XStringA	HeadPost( TestContext& c )
	{
		//return fmt( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		//			"<testrun name=\"%v\" project=\"%v\" tests=\"%d\" started=\"%d\" failures=\"%d\" errors=\"%d\" ignored=\"%d\">\n"
		//			"\t<testsuite name=\"%v\" time=\"%f\">\n",
		//			c.Name, c.Ident, c.Tests.size(), c.Tests.size() - c.CountIgnored(), c.CountFail(), 0, c.CountIgnored(), "anon", c.TotalTime() );
		return fmt( "<testsuite name=\"%v\" time=\"%f\">\n",
					c.FullName(), c.TotalTime() );
	}
	virtual XStringA	Tail()		{ return JUnitTailTxt; }
	virtual XStringA	ItemPost( const Test& t )
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
		XStringA exepath = argv[1];
		exepath.Replace( '/', DIR_SEP );
		exepath.Replace( '\\', DIR_SEP );
		XStringA throwaway = argv[2];		// "test"
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
}

TestContext::~TestContext()
{
	delete Out;
}

bool TestContext::EnumTests()
{
	//printf( "EnumTests\n" );
	FILE* pipe = _popen( ExePath + " " + TT_TOKEN_INVOKE, "rb" );
	if ( !pipe ) { fprintf( stderr, "Failed to popen(%s)\n", (const char*) ExePath ); return false; }

	size_t read = 0;
	char buf[1024];
	XStringA all;
	while ( read = fread( buf, 1, sizeof(buf), pipe ) )
		all.AppendExact( buf, (int) read );
	fclose( pipe );

	podvec<XStringA> lines;
	Split( all, '\n', lines );
	bool eat = false;
	for ( intp i = 0; i < lines.size(); i++ )
	{
		XStringA trim = lines[i].Trimmed();
		if ( trim == "" ) continue;
		if ( eat )
		{
			podvec<XStringA> parts;
			Split( trim, ' ', parts, true );
			if ( parts.size() != 2 || (parts[1] != TT_SIZE_SMALL_NAME && parts[1] != TT_SIZE_LARGE_NAME) )
			{
				fmtoutf( stderr, "Expected 'testname small|large' from test enumeration line. Instead '%s'\n", trim );
				return false;
			}
			Tests += Test( this, parts[0], parts[1] == TT_SIZE_SMALL_NAME ? TTSizeSmall : TTSizeLarge );
			//printf( "Discovered test '%s'\n", (const char*) tests.back() );
		}
		else if ( trim == XStringA(TT_LIST_LINE_END).Trimmed() )
		{
			eat = true;
			continue;
		}
	}

	return true;
}

int TestContext::EnumAndRun( XStringA exepath, int argc, char** argv )
{
	ExePath = exepath;
	Name = FileNameOnly( exepath.ToWideFromUtf8() ).ToUtf8();
	for ( int i = 0; i < argc; i++ )
	{
		XStringA opt = argv[i];
		if ( opt[0] == '-' )
		{
			if ( opt == "-tt-html" )			{ SetOutHtml(); }
			else if ( opt == "-tt-junit" )		{ SetOutJUnit(); }
			else if ( opt == "-tt-small" )		{ RunLargeTests = false; }
			else if ( opt == "-tt-bare" )		{ OutputBare = true; }
			else if ( opt == "-tt-head" )		{ fputs( HtmlHeadTxt, stdout ); return 0; }
			else if ( opt == "-tt-tail" )		{ fputs( HtmlTailTxt, stdout ); return 0; }
			else if ( opt == "-tt-out" )		{ OutDir = argv[i + 1]; i++; }
			else if ( opt == "-tt-ident" )		{ Ident = argv[i + 1]; i++; }
			else if ( opt.Left(2) == "-x" )		{ Exclude += opt.Mid(2); }
			else								{ Options += opt + " "; }
		}
		else
		{
			Include += opt;
		}
	}
	Options.Chop();

	if ( !EnumTests() )
		return 1;

	if ( Include.size() == 0 && Exclude.size() == 0 )
	{
		for ( int i = 0; i < Tests.size(); i++ )
			printf("  %s\n", (const char*) Tests[i].Name );
		return 0;
	}
	else
	{
		return Run();
	}
}

bool TestContext::TestMatchesFilter( const Test& t )
{
	if ( t.Size == TTSizeLarge && !RunLargeTests )
		return false;

	bool pass = false;
	for ( intp i = 0; i < Include.size(); i++ )
	{
		if ( Include[i] == TT_TOKEN_ALL_TESTS )
			pass = true;
		else if ( MatchWildcard( t.Name, Include[i], false ) )
			pass = true;
	}
	for ( intp i = 0; i < Exclude.size(); i++ )
	{
		if ( MatchWildcard( t.Name, Exclude[i], false ) )
			pass = false;
	}
	return pass;
}

void TestContext::FlushOutput()
{
	if ( Out->ImmediateOut() )
	{
		fputs( OutText, stdout );
		OutText.Clear();
	}
}

void TestContext::WriteOutputToFile()
{
	XStringA outf;
	if ( OutDir != "" )
	{
		outf = OutDir;
		if ( !outf.EndsWith(DIR_SEP) ) outf += DIR_SEP;
		outf += Name;
		outf += "-" + Ident;
		outf = Out->OutputFilename(outf);
	}
	if ( !Out->ImmediateOut() )
	{
		if ( outf != "" )
			Panacea::IO::WriteWholeFile( outf.ToWideFromUtf8(), OutText );
		else
			fputs( OutText, stdout );
	}
}

void TestContext::SetOutText()	{ delete Out; Out = new OutputDevText; }
void TestContext::SetOutJUnit()	{ delete Out; Out = new OutputDevJUnit; }
void TestContext::SetOutHtml()	{ delete Out; Out = new OutputDevHtml; }

int TestContext::Run()
{
	if ( !OutputBare ) OutText += Out->HeadPre( *this );
	IPC.Initialize( AbcProcessGetPID() );
	FlushOutput();

	//if ( HtmlMode )	fputs( "<div class='tests'>\n", stdout );

	int nrun = 0;
	for ( int i = 0; i < Tests.size(); i++ )
	{
		Test& t = Tests[i];
		if ( !TestMatchesFilter(t) )
		{
			t.Ignored = true;
			continue;
		}
		nrun++;

		//if ( !HtmlMode ) printf( "%-35s", (const char*) t.Name );
		OutText += Out->ItemPre( t );
		FlushOutput();

		XStringA exec = ExePath + " " + TT_PREFIX_RUNNER_PID + IntToXStringA(AbcProcessGetPID()) + " " + TT_TOKEN_INVOKE + " :" + t.Name + " " + Options;

		XStringA oput;
		int pstatus = 0;
		double tstart = AbcTimeAccurateRTSeconds();
		XStringA execError = ExecProcess( exec, oput, t.TimeoutSeconds(), pstatus );
		if ( execError != "" )
		{
			pstatus = 1;
			oput = "TinyTest ExecProcess Error: " + execError;
		}

		t.Time = AbcTimeAccurateRTSeconds() - tstart;
		t.Output = oput;
		t.TrimmedOutput = oput;
		if ( t.TrimmedOutput.Length() > 1000 )
			t.TrimmedOutput = "..." + t.TrimmedOutput.Right(1000);
		t.Pass = pstatus == 0;

		OutText += Out->ItemPost( t );
		FlushOutput();
	}

	//if ( HtmlMode )	fputs( "</div>\n", stdout );

	int npass = 0;
	for ( intp i = 0; i < Tests.size(); i++ )
		npass += Tests[i].Pass ? 1 : 0;

	//if ( HtmlMode && !HtmlBare )	fputs( HtmlTailTxt, stdout );
	//else if ( !HtmlMode )			printf( "%d/%d tests passed\n", npass, nrun );
	if ( !OutputBare )
	{
		OutText = Out->HeadPost( *this ) + OutText;
		OutText += Out->Tail();
		FlushOutput();
	}

	IPC.Close();
	WriteOutputToFile();

	return npass == nrun ? 0 : 1;
}

void TestContext::CloseZombieProcesses()
{
	for ( intp i = 0; i < SubProcesses.size(); i++ )
	{
		if ( WaitForSingleObject(SubProcesses[i], 0) != WAIT_OBJECT_0 )
			TerminateProcess( SubProcesses[i], 1 );
		CloseHandle( SubProcesses[i] );
	}
	SubProcesses.clear();
}

void DisplayError( XStringA msg )
{
}

void ErrorExit( XString msg )
{
}

BOOL CloseHandleAndZero( HANDLE& h )
{
	BOOL c = CloseHandle(h);
	h = NULL;
	return c;
}

void TestContext::ReadIPC( uint waitMS )
{
	if ( WaitForSingleObject( IPC.DataReady, waitMS ) == WAIT_OBJECT_0 )
	{
		char cmd[200];
		uint u_p0 = 0;
		if ( sscanf( IPC.FileMapPtr, "%200s %u", &cmd, &u_p0 ) == 2 )
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
		SetEvent( IPC.DataFetched );
	}
}

//#define TEXT(a) a

// TODO: Isolate this beast

XStringA TestContext::ExecProcess( XStringA exec, XStringA& output, double timeoutSeconds, int& result )
{
	HANDLE g_hChildStd_IN_Rd = NULL;
	HANDLE g_hChildStd_IN_Wr = NULL;
	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;

	SECURITY_ATTRIBUTES saAttr; 
   
	//printf("\n->Start of parent execution.\n");
  
	// Set the bInheritHandle flag so pipe handles are inherited. 
   
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 
  
	// Create a pipe for the child process's STDOUT. 
   
	if ( !CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) )
		return "StdoutRd CreatePipe"; 
  
	// Ensure the read handle to the pipe for STDOUT is not inherited.
  
	if ( !SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
		return "Stdout SetHandleInformation"; 
  
	// Create a pipe for the child process's STDIN. 
   
	if ( !CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) 
		return "Stdin CreatePipe";
  
	// Ensure the write handle to the pipe for STDIN is not inherited. 
   
	if ( !SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
		return "Stdin SetHandleInformation"; 
   
	// Create the child process. 
     
	//CreateChildProcess();

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	memset( &si, 0, sizeof(si) );
	memset( &pi, 0, sizeof(pi) );
	si.cb = sizeof(si);
	si.hStdInput = g_hChildStd_IN_Rd;
	si.hStdError = g_hChildStd_OUT_Wr;
	si.hStdOutput = g_hChildStd_OUT_Wr;
	si.dwFlags = STARTF_USESTDHANDLES;

	if ( !CreateProcessA( NULL, exec.GetRawBuffer(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi ) )
	{
		//fprintf( stderr, "Failed to popen(%s): %d\n", (const char*) exec, (int) GetLastError() );
		result = 1;
		return fmt( "Failed to popen(%s): %d\n", (const char*) exec, (int) GetLastError() );
	}

	// Write to the pipe that is the standard input for a child process. 
	// Data is written to the pipe's buffers, so it is not necessary to wait
	// until the child process is running before writing data.

	// WriteFile( g_hChildStd_IN_Wr, buffer, bytes, &written, NULL);

	// Close the pipe handle so the child process stops reading. 
	if ( !CloseHandleAndZero(g_hChildStd_IN_Wr) ) 
		return "StdInWr CloseHandle"; 

	// Close the write end of the pipe before reading from the 
	// read end of the pipe, to control child process execution.
	// The pipe is assumed to have enough buffer space to hold the
	// data the child process has already written to it.
   
	if ( !CloseHandleAndZero(g_hChildStd_OUT_Wr) )
		return "StdOutWr CloseHandle"; 

	//timeoutSeconds = 1;

	// Read from the standard output of the child process. 
	char buf[1024];
	DWORD nread = 0;
	double start = AbcRTC();
	while ( true )
	{
		DWORD avail = 0;
		BOOL peekOK = PeekNamedPipe( g_hChildStd_OUT_Rd, NULL, 0, NULL, &avail, NULL );
		if ( !peekOK )
			break;
		if ( avail == 0 )
		{
			if ( AbcRTC() - start > timeoutSeconds )
				break;
			ReadIPC( 16 );
			//Sleep( 16 );
			continue;
		}

		if ( !ReadFile( g_hChildStd_OUT_Rd, buf, sizeof(buf), &nread, NULL ) )
		{
			if ( GetLastError() == ERROR_BROKEN_PIPE ) {} // normal
			else
				output += fmt( "TinyTest: Unexpected ReadFile error on child process pipe: %d \n", (int) GetLastError() );
			break;
		}
		output.AppendExact( buf, nread );
	}

	double timeRemaining = timeoutSeconds - (AbcRTC() - start);
	timeRemaining = std::max(timeRemaining, 0.0);
	bool timeout = WaitForSingleObject( pi.hProcess, DWORD(timeRemaining * 1000) ) != WAIT_OBJECT_0;

	if ( timeout )
		TerminateProcess( pi.hProcess, 1 );
	else
		ReadIPC( 0 );

	DWORD exitcode = 1;
	GetExitCodeProcess( pi.hProcess, &exitcode );
	result = exitcode;

	if ( timeout )
		output += fmt( "\nTest timed out (max time %v seconds)", timeoutSeconds );
	else if ( exitcode != 0 && output.Trimmed().Length() == 0 )
		output += fmt( "\nExit code %d, but test produced no output", exitcode );
  
	CloseHandleAndZero( pi.hProcess );
	CloseHandleAndZero( pi.hThread );
 
	CloseHandleAndZero( g_hChildStd_IN_Rd );
	CloseHandleAndZero( g_hChildStd_OUT_Rd );

	CloseZombieProcesses();

	// The remaining open handles are cleaned up when this process terminates. 
	// To avoid resource leaks in a larger application, close handles explicitly. 
  
	return "";
}

XStringA Escape( bool xml, XStringA s )
{
	XStringA r;
	for ( int i = 0; i < s.Length(); i++ )
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

XStringA EscapeHtml( XStringA s )
{
	return Escape( false, s );
}

XStringA EscapeXml( XStringA s )
{
	return Escape( true, s );
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
		XStringA oput;
		while ( !feof(pipe) )
		{
			read = fread( buf, 1, sizeof(buf), pipe );
			oput.AppendExact( buf, read );
		}
		int pstatus = _pclose( pipe );
		*/
