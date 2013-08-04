#pragma once

#include "VirtualFile.h"
#include "../Platform/syncprims.h"
#include "../Other/Log.h"

/* Rolling log file.
We switch between two files. Whenever we make the switch,
we truncate our new destination file. This works from multiple
processes, and multiple threads within those processes. We
simply rely on the OS to handle the concurrency. You might
see the result of race conditions in the output, but it
should still be readable.
*/
class PAPI AbcRollingLog : public AbcILogTarget
{
public:
	bool	AddDate;				// Add the date+time to every message
	bool	AddDateDayOnly;			// If AddDate = true, then only include the day of the month.
	bool	AddProcessID;			// Add the process id to every message
	bool	AddNewLineIfNotExist;	// Adds a \n if the message does not already end with a \n.
	bool	DuplicateToStdOut;		// Send all messages to stdout after sending them to the log file
	bool	DuplicateToKernel;		// Send all messages to OutputDebugString (Windows) before sending them to the log file.

	// Implementation of AbcILogTarget
	virtual void Trace( AbcLogLevel level, const char* msg );

			AbcRollingLog();
			~AbcRollingLog();	// The destructor calls Close()

	// These are not MT safe
	bool	Open( XString basefilename, u64 maxFileSize );
	void	Close();

	// These are MT safe
	void	Writef( const char* fmt, ... );
	void	Write( const char* str, size_t len = -1 );
	void	RollingFiles( XString& xfile, XString& yfile ) const;
	
	// Add a redirect instruction for stdout or stderr. Whenever the log is rolled, 
	// this handle will be reopened to point to the new log. Multiple handles are supported.
	// Regular usage is of stdout and stderr. The file handles here will never be closed.
	void	AddStdRedirect( FILE* stdFile );

protected:
	AbCore::DiskFile		File;
	XString					BaseFilename;
	XString					CurrentFilename;
	u64						MaxFileSize;
	AbcCriticalSection		Lock;
	pvect<FILE*>			Redirects;
	bool					TryOpenOnWrite;		// Flagged when a rollover fails to open the new file. Silently fails to write, and tries to open the file next time.

	bool					IsOpen();
	bool					OpenInternal();
};
