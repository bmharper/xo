#include "pch.h"
#include "BackgroundFileScanner.h"

namespace Panacea
{
namespace IO
{

DWORD WINAPI BackgroundFileScanner::BgThread( LPVOID lp )
{
	BackgroundFileScanner* bs = (BackgroundFileScanner*) lp;

	int emptyLoops = 0;
	int batchSize = 4;

	while ( true )
	{
		EnterCriticalSection( &bs->BigLock );

		if ( bs->Queue.size() == 0 )
		{
			LeaveCriticalSection( &bs->BigLock );
			emptyLoops++;
		}
		else
		{
			emptyLoops = 0;
			dvect<QueryItem> myqueue;
			dvect<Result> myresults;
			
			for ( int i = 0; i < batchSize && i < bs->Queue.size(); i++ )
				myqueue += bs->Queue[i];
			LeaveCriticalSection( &bs->BigLock );

			ASSERT( myqueue.size() > 0 ); // If this fails, then somebody is messing with the Queue without taking the critical section

			for ( int i = 0; i < myqueue.size(); i++ )
			{
				Result r;
				r.Query = myqueue[i];
				Sys::Date tstart = Sys::Date::Now();
				switch ( myqueue[i].Type )
				{
				case QFileExists:			r.ResultBool = Panacea::IO::Path::FileExists( myqueue[i].Filename, false );	break;
				case QFileOrFolderExists:	r.ResultBool = Panacea::IO::Path::FileExists( myqueue[i].Filename, false ) || Panacea::IO::Path::FolderExists( myqueue[i].Filename, false );	break;
				case QMetaDelay30MS:
					Sleep(30);
					r.ResultBool = true;
					break;
				default: ASSERT(false);
				}
				Sys::Date tend = Sys::Date::Now();
				r.MomentMeasured = tend;
				r.MeasureDurationSeconds = tend.SecondsSince( tstart );
				myresults += r;
				if ( bs->IsShutdown ) break;
			}
			if ( bs->IsShutdown ) break; // This doesn't violate crit sec enter/exit counts.

			EnterCriticalSection( &bs->BigLock );
			
			// erase from query table and then from query list
			for ( int i = 0; i < myqueue.size(); i++ )
				bs->QMap.erase( myqueue[i] );
			bs->Queue.erase( 0, myqueue.size() );
			
			// insert into result list and then into result table
			for ( int i = 0; i < myresults.size(); i++ )
			{
				ASSERT( !bs->RMap.contains(myresults[i].Query) );
				bs->ResultList += myresults[i];
				bs->RMap.insert( myresults[i].Query, bs->ResultList.size() ); // Value = pos + 1 = [size() after insertion]
			}

			ASSERT( bs->QMap.size() == bs->Queue.size() );
			ASSERT( bs->RMap.size() == bs->ResultList.size() );

			LeaveCriticalSection( &bs->BigLock );
		}
		int idleExitTimeMS = 300;
		int sleepTime = bs->DebugFastBGTick ? 1 : 50;
		if ( emptyLoops * sleepTime >= idleExitTimeMS ) break;
		if ( emptyLoops > 0 )
		{
			Sleep( sleepTime );
		}
	}

	if ( bs->DebugOutput ) printf( "<BackgroundFileScanner thread exiting>" );

	return 0;
}


BackgroundFileScanner::BackgroundFileScanner()
{
	IsShutdown = false;
	MaxResultListSize = 500;
	ResultLifetimeSeconds = 10;
	ResultLifetimeBoost = 10;
	HThread = NULL;
	DebugOutput = false;
	DebugFastBGTick = false;
	InitializeCriticalSection( &BigLock );
}

BackgroundFileScanner::~BackgroundFileScanner()
{
	ASSERT( Queue.size() == 0 );
	Spark();

	DeleteCriticalSection( &BigLock );
}

void BackgroundFileScanner::Shutdown()
{
	IsShutdown = true;
	{
		TakeCriticalSection cs( BigLock );
		Queue.clear();
	}
	if ( HThread )
	{
		WaitForSingleObject( HThread, INFINITE );
		CloseHandle( HThread );
		HThread = NULL;
	}
}

int BackgroundFileScanner::GetMaxResultListSize()
{
	return MaxResultListSize;
}

void BackgroundFileScanner::SetMaxResultListSize( int maxSize )
{
	MaxResultListSize = maxSize;
}

int BackgroundFileScanner::QueueSize()
{
	return Queue.size();
}

BackgroundFileScanner::Status BackgroundFileScanner::QueryGen( const QueryItem& qi, bool* resultBool )
{
	if ( IsShutdown ) { ASSERT(false); return StatusWait; }
	TakeCriticalSection cs1( BigLock );

	// Search for an existing result
	Clean();
	int p = RMap.get( qi );
	if ( p != 0 )
	{
		*resultBool = ResultList[p-1].ResultBool;
		return StatusReady;
	}

	// Check if the query is already in the queue. If not, put it there.
	if ( QMap.contains(qi) )
		return StatusWait;

	// Back off if the queue is too large
	if ( QMap.size() > (size_t) MaxResultListSize )
		return StatusWait;

	Queue += qi;
	QMap.insert( qi, 1 );
	Spark();

	return StatusWait;
}

void BackgroundFileScanner::QueryMetaDelay30MS()
{
	QueryItem qi;
	qi.Type = QMetaDelay30MS;
	bool result;
	QueryGen( qi, &result );
}

BackgroundFileScanner::Status BackgroundFileScanner::QueryFileExists( const wchar_t* filename, bool& result )
{
	return QueryGen( QueryItem(QFileExists, filename), &result );
}
BackgroundFileScanner::Status BackgroundFileScanner::QueryFileOrFolderExists( const wchar_t* filename, bool& result )
{
	return QueryGen( QueryItem(QFileOrFolderExists, filename), &result );
}

void BackgroundFileScanner::Clean()
{
	Sys::Date now = Sys::Date::Now();

	// wait until we're 1.5x oversubscribed
	if (	ResultList.size() > MaxResultListSize + MaxResultListSize / 2 ||
				now.SecondsSince( LastCleanTime ) > ResultLifetimeSeconds ) 
	{
		if ( DebugOutput ) printf( "<Cleaning background file scanner>" );
		dvect<Result> survive;
		for ( int i = 0; i < ResultList.size(); i++ )
		{
			if ( now.SecondsSince( ResultList[i].MomentMeasured ) < ActualResultLifetime(ResultList[i].MeasureDurationSeconds) )
				survive += ResultList[i];
		}

		// if we're still oversubscribed, then purge half
		if ( survive.size() > MaxResultListSize )
		{
			// sort by MomentMeasured, so that we discard the oldest results
			sort( survive );
			survive.erase( 0, survive.size() / 2 );
		}

		ResultList = survive;
		RebuildRMap();
		LastCleanTime = Sys::Date::Now();
	}
}

void BackgroundFileScanner::RebuildRMap()
{
	RMap.clear();
	for ( int i = 0; i < ResultList.size(); i++ )
		RMap.insert( ResultList[i].Query, i + 1 );
}

void BackgroundFileScanner::Spark()
{
	if ( HThread != NULL )
	{
		if ( WaitForSingleObject(HThread, 0) == WAIT_OBJECT_0 )
		{
			// The thread has terminated.
			CloseHandle(HThread);
			HThread = NULL;
		}
	}

	if ( HThread == NULL && Queue.size() > 0 )
	{
		HThread = CreateThread( NULL, 0, &BgThread, this, 0, NULL );
	}
}

bool BackgroundFileScanner::DebugIsThreadAlive()
{
	if ( HThread == NULL ) return NULL;
	return WaitForSingleObject( HThread, 0 ) != WAIT_OBJECT_0;
}


}
}
