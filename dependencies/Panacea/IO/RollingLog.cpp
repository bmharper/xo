#include "pch.h"
#include "RollingLog.h"
#include "Path.h"
#include "../Platform/debuglib.h"
#include "../Platform/process.h"

AbcRollingLog::AbcRollingLog()
{
	AddDate = true;
	AddDateDayOnly = false;
	AddProcessID = true;
	AddNewLineIfNotExist = false;
	DuplicateToStdOut = false;
	DuplicateToKernel = false;
	MaxFileSize = 0;
	AbcCriticalSectionInitialize( Lock );
}

AbcRollingLog::~AbcRollingLog()
{
	Close();
	AbcCriticalSectionDestroy( Lock );
}

void AbcRollingLog::Trace( AbcLogLevel level, const char* msg )
{
	Write( msg );
};

bool AbcRollingLog::Open( XString basefilename, u64 maxFileSize )
{
	Close();
	BaseFilename = basefilename;
	MaxFileSize = maxFileSize;
	return OpenInternal();
}

void AbcRollingLog::Close()
{
	File.Close();
}

bool AbcRollingLog::IsOpen()
{
	return File.IsOpen();
}

bool AbcRollingLog::OpenInternal()
{
	// This does all the work of deciding which file to use, as well as truncating it if necessary
	CurrentFilename = PanPath_RollingPair( BaseFilename, MaxFileSize );
	//File.Open( CurrentFilename, L"ab" );
	//File.Open( CurrentFilename, L"rwb", AbCore::IFile::ShareRead | AbCore::IFile::ShareWrite );
	bool ok = File.Open( CurrentFilename, L"ab", AbCore::IFile::ShareRead | AbCore::IFile::ShareWrite );

	if ( ok )
	{
		for ( int i = 0; i < Redirects.size(); i++ )
		{
			// Just as in AddStdRedirect, we discard the return value, because it's unclear what we should do with it
			FILE* refile = freopen( CurrentFilename.ToUtf8(), "ab", Redirects[i] );
		}
	}

	return ok;
}

void AbcRollingLog::RollingFiles( XString& xfile, XString& yfile ) const
{
	PanPath_RollingPair_Paths( BaseFilename, xfile, yfile );
}

void AbcRollingLog::AddStdRedirect( FILE* stdFile )
{
	TakeCriticalSection lock(Lock);
	
	// This is going to lead to duplicate entries in the log file.
	ASSERT( !(stdFile == stdout && DuplicateToStdOut) );

	if ( Redirects.contains(stdFile) )
		return;
	// Just as in OpenInternal(), we discard the result, because it's unclear what we should do with it.
	FILE* refile = freopen( CurrentFilename.ToUtf8(), "ab", stdFile );
	Redirects += stdFile;
}

void AbcRollingLog::Writef( const char* fmt, ... )
{
	const int MAXSIZE = 2048 - 1;
	char buff[MAXSIZE+1];

	va_list va;
	va_start( va, fmt );
	vsprintf_s( buff, fmt, va );
	va_end( va );
	buff[MAXSIZE] = 0;

	Write( buff );
}

void AbcRollingLog::Write( const char* str, size_t len )
{
	TakeCriticalSection lock(Lock);
	if ( TryOpenOnWrite && !IsOpen() )
	{
		// silently fail and discard this log message
		if ( OpenInternal() )
			TryOpenOnWrite = false;
		else
			return;
	}

	AbcAssert( IsOpen() );

	if ( File.Length() > MaxFileSize )
	{
		File.Close();
		for ( int tryOpen = 0; tryOpen < 10; tryOpen++ )
		{
			if ( OpenInternal() )
				break;
			Sleep(1);
		}
		if ( !IsOpen() )
		{
			// silently fail and discard this log message
			TryOpenOnWrite = true;
			return;
		}
	}

	if ( len == -1 )
		len = strlen(str);

	// overestimate our total bytes
	const int		datelen		= 26 + 1;
	const int		pidlen		= 8 + 1;
	const int		newlinelen	= 1;
	const size_t	total		= datelen + pidlen + len + newlinelen + 1;

	if ( AddDate || AddProcessID || AddNewLineIfNotExist )
	{
		StackBufferT<char, 2048> stackBuf( int(total + 0) );
		char* buf = stackBuf;
		char* bufStart = buf;
		size_t offset = 0;

		if ( AddDate )
		{
			AbcDate d = AbcDate::Now();
			d.Format( buf, AbcDate::Format_YYYY_MM_DD_HH_MM_SS_MS, true );
			offset += 23;
			if ( AddDateDayOnly )
				bufStart += 8;		// chop off 2013-02-
			buf[offset++] = ' ';
		}
		if ( AddProcessID )
		{
			sprintf( buf + offset, "%08x ", AbcProcessGetPID() );
			offset += 9;
		}
		// Add Message
		{
			memcpy( buf + offset, str, len );
			offset += len;
		}
		if ( AddNewLineIfNotExist && offset != 0 && buf[offset - 1] != '\n' )
		{
			buf[offset++] = '\n';
		}

		buf[offset] = 0;
	
		if ( DuplicateToKernel )
			AbcOutputDebugString( bufStart );

		File.Append( bufStart, (buf - bufStart) + offset );

		if ( DuplicateToStdOut )
			fputs( bufStart, stdout );
	}
	else
	{
		if ( DuplicateToKernel )
			AbcOutputDebugString( str );

		File.Append( str, len );
		
		if ( DuplicateToStdOut )
			fputs( str, stdout );
	}
}

