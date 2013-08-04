#include "pch.h"
#include "BackgroundFileLoader.h"
#include "Platform/timeprims.h"
#include "Other/profile.h"
#include "../Platform/app.h"

// This is the one and only place in panacea where mtlog gets defined
#define MTLOG_IMPLEMENT 1
#include <mtlog/mtlog.h>

#undef TRACE
//#define TRACE mtlog
#define TRACE AbcTrace

DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_LEAK

AbcBackgroundFileLoader::AbcBackgroundFileLoader()
{
	File = NULL;
	Start = 0;
	Length = 0;
	Pos = 0;
	ChunkSize = 16 * 1024 * 1024;
	ChunkTimeMS = 500;
	Trace = true;
	Priority = AbcJobPriorityBackgroundProcess;
	SetJobDesc("bg file load");
}

void AbcBackgroundFileLoader::WholeFileByHandle( AbCore::IFile* file, AbcJobQueue* queue, double pause_seconds )
{
	auto len = file->Length();
	if ( len == 0 ) { delete file; return; }
	AbcBackgroundFileLoader* self = new AbcBackgroundFileLoader();
	self->Initialize( file, 0, len, pause_seconds );
	queue->AddJob( self );
}

bool AbcBackgroundFileLoader::WholeFileByPathWait( LPCWSTR path )
{
	AbcJobQueue q;
	if ( !WholeFileByPath( path, &q ) ) return false;
	q.WaitForAll();
	return true;
}

bool AbcBackgroundFileLoader::WholeFileByPath( LPCWSTR path, AbcJobQueue* queue, double pause_seconds )
{
	AbCore::DiskFile* df = new AbCore::DiskFile();
	if (	!df->Open( path, L"rb", AbCore::IFile::ShareRead | AbCore::IFile::ShareWrite )
				|| (df->Length() == 0) )
	{
		delete df;
		return false;
	}
	AbcBackgroundFileLoader* self = new AbcBackgroundFileLoader();
	self->Initialize( df, 0, df->Length(), pause_seconds );
	queue->AddJob( self );
	return true;
}

void AbcBackgroundFileLoader::Initialize( AbCore::IFile* file, u64 start, u64 len, double pause_seconds )
{
	AbcAssert( file && len > 0 );
	File = file;
	Start = start;
	Length = len;
	Pos = Start;
	if ( pause_seconds > 0 )
		DelayUntil = AbcDate::Now().PlusSeconds( pause_seconds );
}

/*
Load times:
Loading a hot 300MB dataset, on a hard drive that should otherwise be uncontended, I get a lot of MAX
numbers coming out at 997 microseconds.
There are however spikes as high as 500,000 microseconds. This is obviously not completely uncontended.
There are also numerous little spikes of around 40,000 microseconds.
OK.. when I do a simultaneous dir /s *blah* on the drive, times spike immediately to the half-second (500k) range.
60000 seems to be a reasonable backoff threshold.
*/
void AbcBackgroundFileLoader::Run()
{
	AbcAssert( Pos >= Start );
	AbcAssert( ChunkSize >= SYSTEM_PAGE_SIZE );
	static_assert( ABC_RTC64_PER_SECOND == 1000000, "ABC_RTC64 assumption" );
	if ( Trace ) TRACE( "BGFL: %8d KB (%S)\n", (u32) (Pos / 1024), (LPCWSTR) File->FileName );
	const u64 final = Start + Length - 1;
	bool backOff = false;
	u32 tPageMin = UINT32_MAX;
	u32 tPageMax = 0;
	i64 start = AbcRTC64();
	for ( u32 page = 0; page < ChunkSize / SYSTEM_PAGE_SIZE; page++ )
	{
		u8 junk;
		i64 tPageStart = AbcRTC64();
		
		bool fail = 1 != File->ReadAt( Pos, &junk, 1 );

		u32 tPageMicro = AbcRTC64() - tPageStart;
		tPageMin = min( tPageMin, tPageMicro );
		tPageMax = max( tPageMax, tPageMicro );

		Pos = (Pos & ~(SYSTEM_PAGE_SIZE - 1)) + SYSTEM_PAGE_SIZE;
		if ( fail || Pos >= Start + Length )
		{
			// we are done
			if ( fail && Trace ) TRACE( "BGFL:FAIL (%S)\n", (LPCWSTR) File->FileName );
			delete File;
			return;
		}
		if ( tPageMicro > 60000 )
		{
			TRACE( "BGFL:                                                                   BACKOFF\n" );
			backOff = true;
			break;
		}
		u32 elapsedMS = (AbcRTC64() - start) / 1000;
		if ( elapsedMS > ChunkTimeMS || Queue->IsExitSignalled() || AbcProcessCloseRequested() )
			break;
	}
	//TRACE( "BGFL:                                                                   Min %u  Max %u\n", tPageMin, tPageMax );
	if ( Queue->IsExitSignalled() || AbcProcessCloseRequested() )
	{
		TRACE( "BGFL: Abort\n" );
		delete File;
		return;
	}
	// Spawn another instance of ourselves
	AbcBackgroundFileLoader* next = new AbcBackgroundFileLoader();
	next->Initialize( File, Start, Length, 0 );
	next->Pos = Pos;
	next->ChunkSize = ChunkSize;
	next->ChunkTimeMS = ChunkTimeMS;
	next->Priority = Priority;
	next->Trace = Trace;
	if ( backOff ) next->DelayUntil = AbcDate::Now().PlusSeconds( 5 );
	memcpy(next->Desc, Desc, sizeof(Desc));
	// Add the job to the front of the queue. If there are multiple background file loaders running, this ensures that they execute serially,
	// which is obviously desirable for hard drives.
	Queue->AddJobToFront( next );
}
