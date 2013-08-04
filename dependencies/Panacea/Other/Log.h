#pragma once

#include "../HashTab/ohashmap.h"

/* Log system
Log messages are identified by a 16-bit code.
The high 3 bits of the code are the log level. Currently 3 levels are used (of a maximum of 6). We reserve level 0 as not a valid level.
The low 13 bits of the code are application-defined.

The idea with the log codes is not for a human to interpret these integers when reading the logs
(indeed, they never appear in the output). Instead, the the log codes are simply a mechanism to
turn particular classes of messages on or off with fine granularity.
The log level (Debug,Warn,Error,etc) DO usually make it into the actual log (when AddDateAndLevel is true).

The filtering of which messages go through depends on two things: The log level, and and the individual overrides.
The level of the message must be greater than or equal to the current log level in order for it to be sent to
the log target.
However, before checking the log level, the system first checks whether that particular log type has been
overridden. An override can be either negative or positive. In other words, you can force a particular
message to always go through, or to always be ignored.

The log messages are routed to an AbcILogTarget. This will typically be a file, or a kernel debug
printer.

Thinking behind the integer message types, Chaining, etc
--------------------------------------------------------

This system was built around the assumption that you want a single linear log file.
There are numerous agents that feed into this single log file. Each agent has their
own AbcLog object, and their own set of integer message types. The agents perform
log filtering themselves, and if the filter passes, then they forward the message
on to the next agent in the chain, until it eventually reaches the canonical log.

Basically, the method that you use to hook up various different pieces of code
to output into the same log, is to use the ChainBehind() function on each of them,
except for the final one. Usually only the final AbcLog wil have AddDateAndLevel turned on.

If you're looking for a rolling file target, see AbcRollingLog
*/

class AbcLog;

enum AbcLogLevel
{
	AbcLogLevelBottom	= 0,	// This is used as a null/invalid log level, such as for functions that need to return a log level
	AbcLogLevelDebug	= 1,
	AbcLogLevelWarn		= 2,
	AbcLogLevelError	= 3,
	AbcLogLevelTop		= 4,	// you have only 3 bits here, so max of 7
};

// Use ABCLOG_MSGTYPE to form a usable AbcLogMsgType
// We merely use an enum to enforce typing
enum AbcLogMsgType
{
};

#define ABCLOG_MSGTYPE_SHIFT 13

// Level is AbcLogLevelDebug, AbcLogLevelWarn, AbcLogLevelError
// Code is from 0 to 8191, inclusive
#define ABCLOG_MSGTYPE( level, code ) ((AbcLogMsgType) ((level) << ABCLOG_MSGTYPE_SHIFT | (code)))

class PAPI AbcILogTarget
{
public:
	virtual ~AbcILogTarget();
	virtual void	Trace( AbcLogLevel level, const char* msg ) = 0;
	virtual AbcLog* ForwardsTo() const;	// Special hack-ish thing to detect chains of loggers
};

// Sends to OutputDebugString
class PAPI AbcLogTarget_KernelTrace : public AbcILogTarget
{
public:
	virtual void Trace( AbcLogLevel level, const char* msg );
};

// Sends to FILE*
class PAPI AbcLogTarget_File : public AbcILogTarget
{
public:
	FILE*	File;
					AbcLogTarget_File( FILE* file = NULL );
	virtual			~AbcLogTarget_File();
	virtual void	Trace( AbcLogLevel level, const char* msg );
};

// Sends to another AbcLog
class PAPI AbcLogTarget_Forwarder : public AbcILogTarget
{
public:
	AbcLog*	Log;
					AbcLogTarget_Forwarder( AbcLog* log = NULL );
	virtual			~AbcLogTarget_Forwarder();
	virtual void	Trace( AbcLogLevel level, const char* msg );
	virtual AbcLog* ForwardsTo() const;
};

/* Log system.

There are two ways to control whether a log message is sent to the target:
1. Change the log level
2. Toggle the message as always enabled or always disabled

*/
class PAPI AbcLog
{
public:
	// Add date/time to all log messages.
	// You'll typically enable these only on the last AbcLog in the chain.
	bool AddDate;
	// Add message level to all log messages.
	bool AddLevel;

	AbcLog();
	~AbcLog();

	static AbcLogLevel	GetMsgLevel( AbcLogMsgType msgType )	{ return (AbcLogLevel) ((u32) msgType >> ABCLOG_MSGTYPE_SHIFT); }
	static uint16		GetMsgCode( AbcLogMsgType msgType )		{ return msgType & ((1 << ABCLOG_MSGTYPE_SHIFT) - 1); }
	static const char*	MsgLevelName( AbcLogLevel level );		// this must obviously correspond with ParseMsgLevel
	static AbcLogLevel	ParseMsgLevel( const char* level );		// expects "debug", "warn", "error". Returns AbcLogLevelBottom on error.

	// Control whether a message is enabled, disabled, or default (ie use level)
	void Control( AbcLogMsgType msgType, Trinary tri );
	
	// Only messages with a level greater than or equal to this level will be dispatched
	void SetLevel( AbcLogLevel level );

	// Retrieve the effective log level. If we are chained behind another AbcLog, then we use that AbcLog's level.
	// This process is naturally recursive.
	int GetEffectiveLevel() const;

	// Chain this logger so that it forwards all its logs to "front".
	// This does two things:
	// 1. SetTarget( new AbcLogTarget_Forwarder(front) )
	// 2. Changes the log level behaviour, so that all upstream loggers ask their downstream loggers for the log level.
	//    This means you don't have to set the log level on all members of the chain, you simply set it on the last level.
	void ChainBehind( AbcLog* front );

	// Set the target.
	// takeOwnershipOfTarget	If true, then we will delete this target when we are destroyed, or when a new target is set.
	void SetTarget( AbcILogTarget* target, bool takeOwnershipOfTarget );

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// All Trace functions are thread-safe
	void Tracef( AbcLogMsgType msgType, const char* fmt, ... );
	void Trace( AbcLogMsgType msgType, const char* msg );
	void TraceNoFilter( AbcLogLevel level, const char* msg );		// This is the low-level function that ends up being called by the others, if the filter passes

protected:
	ohash::ohashmap<uint16, int16>	MsgTypeStatus;
	AbcLogLevel						Level;
	AbcILogTarget*					Target;
	bool							OwnTarget;

	bool Allow( AbcLogMsgType msgType ) const;

};
