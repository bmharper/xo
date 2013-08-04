#include "pch.h"
#include "MiniDumper_Windows.h"
#include "../IO/VirtualFile.h"
#include "../Platform/err.h"
#include "DbgHelp.h"
#include "profile.h"
#include "../../CrashReport/CrashClient.h"
#include <ShlObj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#undef TRACE
#define TRACE AbcTrace
//#define TRACE(...) ((void)0)

CONTEXT					StaticContext;
EXCEPTION_RECORD		StaticException;
int						StaticDumpBusy = false;
double					StaticTimeLastDump = -1000 * CLOCKS_PER_SEC;
MiniDump_WriteUserData	StaticCustomCallback = NULL;
wchar_t					StaticDumpUrl[512] = L"";
wchar_t					StaticAppName[128] = L"UNKNOWN";
wchar_t					StaticNetworkDir[MAX_PATH] = L"";
wchar_t					StaticLocalDir[MAX_PATH] = L"";

LONG WINAPI MiniExceptionHandler( EXCEPTION_POINTERS* exInfo );

int Filter( unsigned int code, EXCEPTION_POINTERS *ep )
{
	memcpy( &StaticContext, ep->ContextRecord, sizeof(StaticContext) );
	memcpy( &StaticException, ep->ExceptionRecord, sizeof(StaticException) );
	return EXCEPTION_EXECUTE_HANDLER;
}

int FilterCrazy( EXCEPTION_POINTERS *ep )
{
	memcpy( &StaticContext, ep->ContextRecord, sizeof(StaticContext) );
	memcpy( &StaticException, ep->ExceptionRecord, sizeof(StaticException) );
	return EXCEPTION_EXECUTE_HANDLER;
}

int Filter2( EXCEPTION_POINTERS *ep )
{
	// This doesn't help.
	/*
	__try
	{
		int* mem = 0;
		*mem = 1;
	}
	__except( FilterCrazy(GetExceptionInformation()))
	{
	}
	*/

	int code = ep->ExceptionRecord->ExceptionCode;
	if ( (code == STATUS_SXS_INVALID_DEACTIVATION || code == STATUS_SXS_EARLY_DEACTIVATION) && StaticException.ExceptionCode != 0 )
	{
		// I don't understand how the exception handlers work, but basically you get an access violation here,
		// and then before the handler runs, you get an STATUS_SXS_INVALID_DEACTIVATION. The SXS deactivation is
		// meaningless to us. We want the first exception, which is why we have this block here.
		// I posted a message to an MSDN forum: http://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/3057b665-f4dd-45e8-83c1-5c4196295afb
		// But to date I have not recieved a reply.
	}
	else
	{
		memcpy( &StaticContext, ep->ContextRecord, sizeof(StaticContext) );
		memcpy( &StaticException, ep->ExceptionRecord, sizeof(StaticException) );
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

int FindLast( LPCTSTR name, TCHAR seek )
{
	int last = -1;
	int i = -1;
	while ( name[++i] != 0 )
	{
		if ( name[i] == seek ) last = i;
	}
	return last;
}

bool FileExists( LPCTSTR name )
{
	return GetFileAttributes( name ) != INVALID_FILE_ATTRIBUTES;
}

/// Populates folder with the directory (minus trailing backslash) of the current module
bool ModuleFolder( LPTSTR name )
{
	GetModuleFileName( NULL, name, MAX_PATH );

	int sl = FindLast( name, '\\' );
	if ( sl > 0 )
	{
		name[sl] = 0;
		return true;
	}

	ASSERT(false);
	return false;
}

/// Searches for the existence of a file named 'name', in the folder of the currently executing module (exe).
bool ModuleFileExists( LPCTSTR name )
{
	wchar_t modpath[MAX_PATH * 2];
	if ( !name ) { ASSERT(false); return false; }

	if ( ModuleFolder( modpath ) )
	{
		wcscat( modpath, L"\\" );
		wcscat( modpath, name );
		return FileExists( modpath );
	}

	ASSERT(false);
	return false;
}

void MiniDumpTargetFile( wchar_t* dumpfile, wchar_t* logfile, wchar_t* video )
{
	wchar_t base[MAX_PATH * 2];
	if ( StaticLocalDir[0] != 0 )
	{
		wcscpy_s( base, StaticLocalDir );
	}
	else
	{
		SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, base );
		wcscat( base, L"\\GLS" );			CreateDirectory( base, NULL );
		wcscat( base, L"\\CrashReports" );	CreateDirectory( base, NULL );
	}
	wcscat( base, L"\\crash-report" );

	CrashReport::NextRollingFile( base, 10, dumpfile );

	TRACE( "dumpfile: %S\n", dumpfile );
	
	if ( logfile )
	{
		wcscpy( logfile, dumpfile );
		wcscat( logfile, L"-log" );
		TRACE( "logfile: %S\n", logfile );
	}
	if ( video )
	{
		wcscpy( video, dumpfile );
		wcscat( video, L"-video" );
		TRACE( "video: %S\n", video );
	}
}

bool MiniDumper::DumpOutOfMemory( bool sendToRemote )
{
	wchar_t filename[MAX_PATH];
	MiniDumpTargetFile( filename, NULL, NULL );
	bool ok = DumpInternal( filename );
	if ( ok )
	{
		if ( sendToRemote )
			CrashReport::InvokeCrashHandler( CrashReport::ModeReportNotify, filename, NULL, NULL, StaticNetworkDir, StaticDumpUrl, StaticAppName, CrashReport::XModeOutOfMemory );
	}
	return ok;
}

bool MiniDumper::DumpGenericAndSend()
{
	wchar_t filename[MAX_PATH];
	MiniDumpTargetFile( filename, NULL, NULL );
	bool ok = DumpInternal( filename );
	if ( ok )
		CrashReport::InvokeCrashHandler( CrashReport::ModeReportSilent, filename, NULL, NULL, StaticNetworkDir, StaticDumpUrl, StaticAppName, CrashReport::XModeGeneric );
	return ok;
}

bool MiniDumper::Dump( LPCTSTR filename )
{
	return DumpInternal( filename );
}

#pragma warning(disable: 6011) // NULL pointer deref
bool MiniDumper::DumpInternal( LPCTSTR filename )
{
	bool ok = false;

	// we have to do this __try,__except block in order to generate
	// a proper stack trace for the minidumper.
	__try
	{
		int* mem = 0;
		*mem = 1;
	}
	__except( Filter(GetExceptionCode(), GetExceptionInformation()) ) 
	{
		MINIDUMP_EXCEPTION_INFORMATION exInfo;

		EXCEPTION_POINTERS exp;
		exp.ContextRecord = &StaticContext;
		exp.ExceptionRecord = &StaticException;

		exInfo.ThreadId = GetCurrentThreadId();
		exInfo.ExceptionPointers = &exp;
		exInfo.ClientPointers = NULL;

		HANDLE hFile = CreateFile( filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

		if ( hFile != INVALID_HANDLE_VALUE )
		{
			ok = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &exInfo, NULL, NULL ) != 0;
			if ( ok )
			{
				ok = true;
			}
			CloseHandle( hFile );
		}
	}

	return ok;
}

void MiniDumper::RunProtected( void* context, void (*callback) (void* cx) )
{
	__try
	{
		callback( context );
	}
	__except( Filter2(GetExceptionInformation()) ) 
	{
		EXCEPTION_POINTERS exp;
		exp.ContextRecord = &StaticContext;
		exp.ExceptionRecord = &StaticException;
		MiniExceptionHandler( &exp );
		TerminateProcess( GetCurrentProcess(), 0 );
	}
}

#define MAX_INTERVAL_SECONDS (15)

LONG WINAPI MiniExceptionHandler( EXCEPTION_POINTERS* exInfo )
{
	TRACE( "MiniExceptionHandler enter\n" );
	//__debugbreak();
	if ( StaticDumpBusy ) return EXCEPTION_EXECUTE_HANDLER;
	
	TRACE( "MiniExceptionHandler 1\n" );

	if (	ModuleFileExists( _T(".Albion.PanicOnException") ) ||
				ModuleFileExists( _T("Albion.Options.PanicOnException") ))
	{
		// the process should terminate immediately
		return EXCEPTION_EXECUTE_HANDLER;
	}

	TRACE( "MiniExceptionHandler 2\n" );

	if ( ((clock() - StaticTimeLastDump) / (double) CLOCKS_PER_SEC) < MAX_INTERVAL_SECONDS )
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

	TRACE( "MiniExceptionHandler 3\n" );

	StaticDumpBusy = true;

	StaticTimeLastDump = clock();

	const bool test_mode = false;	// useful to not pollute the web crash db.. DAMMIT I always forget this kind of stuff.... it's been off for days.
	bool dumpOk = false;
	bool abortDump = false;

	wchar_t dumpfile[MAX_PATH * 2];
	wchar_t logfile[MAX_PATH * 2];
	wchar_t videofile[MAX_PATH * 2];
	MiniDumpTargetFile( dumpfile, logfile, videofile );

	TRACE( "Dumping to %S %S %S\n", dumpfile, logfile, videofile );
	printf( "Dumping to %S\n", dumpfile );

	HANDLE hFile = CreateFile( dumpfile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		FILETIME ftime, now;
		GetFileTime( hFile, NULL, NULL, &ftime );
		GetSystemTimeAsFileTime( &now );
		INT64 vfile = ((UINT64) ftime.dwHighDateTime << 32) | (UINT64) ftime.dwLowDateTime;
		INT64 vnow = ((UINT64) now.dwHighDateTime << 32) | (UINT64) now.dwLowDateTime;
		INT64 diff = (vnow - vfile) / (10 * 1000 * 1000);

		if ( diff < MAX_INTERVAL_SECONDS )
		{
			abortDump = true;
		}
		CloseHandle( hFile );
	}

	if ( !abortDump )
	{
		if ( StaticCustomCallback != NULL )
		{
			HANDLE hLogFile = CreateFile( logfile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hLogFile != INVALID_HANDLE_VALUE )
			{
				// allow the client to write auxiliary data, and/or a video file
				wchar_t videosrc[MAX_PATH] = {0};
				AbCore::DiskFile df( hLogFile, false );
				StaticCustomCallback( &df, videosrc );
				CloseHandle( hLogFile );
				if ( videosrc[0] )
				{
					TRACE( "Copying video from %S to %S\n", videosrc, videofile );
					CopyFile( videosrc, videofile, false );	// this is going to pause before showing the crash box. Not cool for the user, as he gets that dreaded feeling.
				}
			}
		}

		if ( test_mode ) wcscpy( dumpfile, L"c:\\temp\\test.mdmp" );
		hFile = CreateFile( dumpfile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

		if ( hFile != INVALID_HANDLE_VALUE )
		{
			MINIDUMP_EXCEPTION_INFORMATION ExInfo;

			ExInfo.ThreadId = GetCurrentThreadId();
			ExInfo.ExceptionPointers = exInfo;
			ExInfo.ClientPointers = NULL;

			TRACE( "Calling MiniDumpWriteDump \n" );

			// write the dump
			BOOL ok = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
			CloseHandle(hFile);
			if ( ok )
			{
				dumpOk = true;
			}

			if ( ok )
			{
				if ( !test_mode )
				{
					TRACE( "Invoking crash handler %S %S %S %S\n", dumpfile, logfile, videofile, StaticNetworkDir );
					bool cok = CrashReport::InvokeCrashHandler( CrashReport::ModeReportNotify, dumpfile, logfile, videofile, StaticNetworkDir, StaticDumpUrl, StaticAppName, CrashReport::XModeCrash );
					TRACE( "Done invoking crash handler (%s)\n", cok ? "OK" : "FAIL" );
				}
			}
		}
		else
		{
			if ( AbcAllowGUI() )
			{
				MessageBox( NULL, L"This application has caused an unrecoverable error.\n"
													L"A diagnostic file COULD NOT BE WRITTEN to C:\\temp\\ErrorReport.mdmp\n"
													L"Please make sure that you have permission to write to C:\\temp\\.\n", L"Error", MB_OK );
			}
		}
	}

	StaticDumpBusy = false;

	if ( dumpOk || abortDump )
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
}



void MiniDumper::Install( MiniDump_WriteUserData custom_callback )
{
	StaticCustomCallback = custom_callback;
	LPTOP_LEVEL_EXCEPTION_FILTER oldFilter = SetUnhandledExceptionFilter( &MiniExceptionHandler );
	if ( oldFilter != NULL )
	{
		//TRACE( "Replacing global unhandled exception handler with MiniDumper\n" );
		//Beep( 160, 50 );
	}
}

void MiniDumper::SetRemoteParameters( XString dumpUrl, XString appName )
{
	ASSERT( dumpUrl.Length() > 0 );
	ASSERT( appName.Length() > 0 );
	wcscpy_s( StaticDumpUrl, dumpUrl );
	wcscpy_s( StaticAppName, appName );
}

void MiniDumper::SetNetworkDirectory( XString network_dir )
{
	wcscpy_s( StaticNetworkDir, network_dir );
}

void MiniDumper::SetLocalDirectory( XString local_dir )
{
	wcscpy_s( StaticLocalDir, local_dir );
}

