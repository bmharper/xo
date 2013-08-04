#include "pch.h"
#include "Log.h"
#include "../Platform/debuglib.h"

AbcILogTarget::~AbcILogTarget()
{
}

AbcLog* AbcILogTarget::ForwardsTo() const
{
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AbcLogTarget_KernelTrace::Trace( AbcLogLevel level, const char* msg )
{
	AbcOutputDebugString( msg );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcLogTarget_File::AbcLogTarget_File( FILE* file ) : File(file) {}
AbcLogTarget_File::~AbcLogTarget_File() {}

void AbcLogTarget_File::Trace( AbcLogLevel level, const char* msg )
{
	if ( !File )
		return;
	fputs( msg, File );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcLogTarget_Forwarder::AbcLogTarget_Forwarder( AbcLog* log ) : Log(log) {}
AbcLogTarget_Forwarder::~AbcLogTarget_Forwarder() {}

void AbcLogTarget_Forwarder::Trace( AbcLogLevel level, const char* msg )
{
	if ( !Log )
		return;
	Log->TraceNoFilter( level, msg );
}

AbcLog* AbcLogTarget_Forwarder::ForwardsTo() const
{
	return Log;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcLog::AbcLog()
{
	AddDate = false;
	AddLevel = false;
	Level = AbcLogLevelDebug;
	Target = NULL;
	OwnTarget = false;
	SetTarget( new AbcLogTarget_KernelTrace, true );
}

AbcLog::~AbcLog()
{
	SetTarget( NULL, true );
}

void AbcLog::Control( AbcLogMsgType msgType, Trinary tri )
{
	if ( tri == TriTrue || tri == TriFalse )
		MsgTypeStatus.insert( msgType, tri );
	else
		MsgTypeStatus.erase( msgType );
}

const char*	AbcLog::MsgLevelName( AbcLogLevel level )
{
	switch ( level )
	{
	case AbcLogLevelBottom:	return "None";
	case AbcLogLevelDebug:	return "Debug";
	case AbcLogLevelWarn:	return "Warn";
	case AbcLogLevelError:	return "Error";
	case AbcLogLevelTop:	return "Top";
	default:
		ASSERT(false);
		return "Unknown";
	}
}

AbcLogLevel	AbcLog::ParseMsgLevel( const char* level )
{
	if ( level )
	{
		switch ( _toupper(level[0]) )
		{
		case 'D': return AbcLogLevelDebug;
		case 'W': return AbcLogLevelWarn;
		case 'E': return AbcLogLevelError;
		}
	}
	return AbcLogLevelBottom;
}

void AbcLog::SetLevel( AbcLogLevel level )
{
	Level = level;
}

int AbcLog::GetEffectiveLevel() const
{
	AbcLog* next = Target ? Target->ForwardsTo() : NULL;
	if ( next )
		return next->GetEffectiveLevel();
	else
		return Level;
}

void AbcLog::ChainBehind( AbcLog* front )
{
	SetTarget( new AbcLogTarget_Forwarder(front), true );
}

void AbcLog::SetTarget( AbcILogTarget* target, bool takeOwnershipOfTarget )
{
	if ( OwnTarget )
		delete Target;
	Target = target;
	OwnTarget = takeOwnershipOfTarget;
}

void AbcLog::TraceNoFilter( AbcLogLevel level, const char* msg )
{
	if ( AddDate || AddLevel )
	{
		const size_t len = strlen(msg);
		StackBufferT<char, 2048> tempBuf( int(100 + len) );
		char* buf = tempBuf;
		size_t offset = 0;
		if ( AddDate )
		{
			AbcDate now = AbcDate::Now();
			now.Format( buf, AbcDate::Format_YYYY_MM_DD_HH_MM_SS_MS, true );
			offset += 23;
			buf[offset++] = ' ';
		}
		if ( AddLevel )
		{
			buf[offset++] = MsgLevelName(level)[0];
			buf[offset++] = ' ';
		}
		if ( len != 0 )
		{
			// If there is no trailing \n, then add one. I realize this is somewhat meddlesome,
			// but I hope it's the right (aka It Just Works) thing to do.
			strcpy( buf + offset, msg );
			offset += len;
			if ( msg[len - 1] != '\n' )
			{
				buf[offset++] = '\n';
				buf[offset++] = 0;
			}
		}
		Target->Trace( level, tempBuf );
	}
	else
	{
		Target->Trace( level, msg );
	}
}

bool AbcLog::Allow( AbcLogMsgType msgType ) const
{
	if ( !Target )
		return false;

	// Important here that TriNull == 0, and that ohash will return zero if it contains nothing.
	Trinary tri = (Trinary) MsgTypeStatus.get( msgType );

	if ( tri == TriTrue )
		return true;
	else if ( tri == TriFalse )
		return false;
	else
		return GetMsgLevel( msgType ) >= GetEffectiveLevel();
}

void AbcLog::Trace( AbcLogMsgType msgType, const char* msg )
{
	if ( !Allow(msgType) ) return;
	TraceNoFilter( GetMsgLevel(msgType), msg );
}

void AbcLog::Tracef( AbcLogMsgType msgType, const char* fmt, ... )
{
	if ( !Allow(msgType) ) return;

	char buff[8192];

	va_list va;

	va_start( va, fmt );
	vsnprintf( buff, arraysize(buff), fmt, va );
	va_end( va ); 
	buff[arraysize(buff) - 1] = 0;

	TraceNoFilter( GetMsgLevel(msgType), buff );
}
