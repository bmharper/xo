#include "pch.h"
#include "MappedPageCache_Windows.h"

enum MappedPageCacheProfiles
{
	PROF_NONE,
	PROF_CREATE_FM,
	PROF_MAPVIEW,
	PROF_UNMAPVIEW,
	PROF_ALI_READ,
	PROF_ALI_WRITE,
	PROF_UNALI_READ,
	PROF_UNALI_WRITE,
};

//#define FIXED_PAGE_SIZE 16

#define PROFILE_SLOT			PROF_NONE
#define PROFILE_MAX_RECORD		64 // make this a power of 2
#define CACHE_LINE_BYTES		64
#define PROFILE_START(slot)		if ( slot == PROFILE_SLOT )		{ Profile += 0; Profile.back() = RDTSC(); }
#define PROFILE_END(slot)		if ( slot == PROFILE_SLOT )		{ Profile.back() = RDTSC() - Profile.back(); if(Profile.size() >= PROFILE_MAX_RECORD) Profile.count = 0; }

void PAPI MPC_ProfileDump( const dvect<int64>& counts )
{
	int n = counts.capacity;
	if ( n == 0 ) return;
	const int64* raw = counts.data;
	dvect<int64> all;
	for ( int i = 0; i < n; i++ ) all += raw[i];
	sort(all);
	n = n * 3 / 4;

	double avg = 0;
	for ( int i = 0; i < n; i++ ) { printf( "Profile: %5d\n", (int) all[i] ); avg += all[i]; }
	printf( "Average: %f\n", avg / n );
}

namespace AbCore
{

MappedPageCache::MappedPageCache()
{
	Verbosity = 0;
	Date = 0;
	MapCount = 0;
	MaxMapped4kPages = (16 * 1024 * 1024) / 4096;
	TrimFreeBlocksx64 = 16;

	SYSTEM_INFO inf;
	GetSystemInfo( &inf );
	AllocationGranularity = inf.dwAllocationGranularity;

	Verify( InitIpc() );
}

MappedPageCache::~MappedPageCache()
{
	for ( int i = 0; i < Files.size(); i++ )
	{
		File* f = Files[i];
		for ( int j = 0; j < f->Blocks.size(); j++ )
		{
			if ( f->Blocks[j].Ptr ) Verify( !!UnmapViewOfFile( f->Blocks[j].Ptr ) );
		}
		CloseHandle( f->FMO );
	}
	delete_all( Files );
	MapCount = 0;
	CloseIpc();
}

void MappedPageCache::Verify( bool res )
{
	AbcVerify(res);
}

uint16 MappedPageCache::AddFile( HANDLE fh, const void* sigBytes, uint32 blockShift )
{
	FileSig sig;
	memcpy( sig.Bytes, sigBytes, sizeof(sig) );
	Verify( (1u << blockShift) >= AllocationGranularity && blockShift <= MaxBlockShift );
#ifdef FIXED_PAGE_SIZE
	Verify( blockShift == FIXED_PAGE_SIZE );
#endif
	File* f = new File();
#ifndef FIXED_PAGE_SIZE
	f->BlockShift = blockShift;
#endif
	f->FH = fh;
	f->FMO = NULL;
	f->IpcSlot = SlotForFile( sig );
	Files += f;
	return Files.size() - 1;
}

void MappedPageCache::RemoveFile( HANDLE fh, const void* sigBytes )
{
	FileSig sig;
	memcpy( sig.Bytes, sigBytes, sizeof(sig) );
	WaitForSingleObject( hGlobalLock, INFINITE );
	for ( int i = 0; i < Files.size(); i++ )
	{
		if ( Files[i]->FH == fh )
		{
			File* f = Files[i];
			IpcIndex[f->IpcSlot].Alive--;
			break;
		}
	}
	ReleaseMutex( hGlobalLock );
}

void MappedPageCache::WriteLockAcquire( uint16 file )
{
	Lock( file, true );
}

void MappedPageCache::WriteLockRelease( uint16 file )
{
	Lock( file, false );
}

void* MappedPageCache::BlockAddr( uint16 file, uint32 block )
{
	File* f = Files[file];
	if ( (uint32) block >= (uint32) f->Blocks.size() || f->Blocks[block].Ptr == NULL )
		Map( file, block );
	Touch( file, block );
	return f->Blocks[block].Ptr;
}

PageHandle MappedPageCache::Acquire( uint16 file, int32 block )
{
	PageHandle h;
	h.File = file;
	h.Block = block;
	h.Ptr = BlockAddr( file, block );
	// find a free slot in the Handles[] array and insert the handle there.
	int hpos = 0;
	for ( ; hpos < Handles.size(); hpos++ )
		if ( Handles[hpos].Internal == -1 )
			break;
	h.Internal = hpos;
	if ( hpos == Handles.size() ) Handles += h;
	else Handles[hpos] = h;
	return h;
}

void MappedPageCache::Acquire( volatile uint32* v )
{
	while ( true )
	{
		if ( InterlockedCompareExchange( (LONG*) v, 1, 0 ) == 0 ) return;
		Sleep(50);
	}
}

void MappedPageCache::Release( PageHandle h )
{
	for ( int i = 0; i < Handles.size(); i++ )
	{
		if ( Handles[i].Internal == h.Internal )
		{
			Handles[i].Internal = -1;
			return;
		}
	}
}

void MappedPageCache::Release( volatile uint32* v )
{
	ASSERT( *v == 1 );
	*v = 0;
}

bool MappedPageCache::InitIpc()
{
	const uint32 headSize = sizeof(SIpcHead) + MaxFiles * (sizeof(*IpcIndex) + sizeof(*IpcFiles));

	/*
	bool first = false;
	hGlobalLock = OpenMutex( SYNCHRONIZE, false, L"AbCore_MPC_GlobalLock" );
	if ( hGlobalLock == NULL )
	{
		hGlobalLock = CreateMutex( NULL, true, L"AbCore_MPC_GlobalLock" );
		if ( hGlobalLock == NULL ) return false;
		first = true;
	}

	if ( first )
		hIpcHead = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, headSize, L"AbCore_MPC_Head" );
	else 
		hIpcHead = OpenFileMapping( FILE_MAP_WRITE, false, L"AbCore_MPC_Head" );
	if ( hIpcHead == NULL ) return false;
	*/
	// Disable the shared system for now. The reason for this is that there needs to be a fallback if
	// the shared stuff doesn't work. The current mechanism will break if you have two users running the app
	// under the same session. This happens in practice with two different services running under session 0,
	// one of them as LocalSystem, and the other running as some other user.
	bool first = true;
	AbcMutexCreate( hGlobalLock, NULL );
	if ( hGlobalLock == NULL )
		return false;
	AbcMutexWait( hGlobalLock, AbcINFINITE );

	hIpcHead = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, headSize, NULL );
	if ( hIpcHead == NULL )
	{
		AbcMutexRelease( hGlobalLock );
		return false;
	}

	IpcHead = (SIpcHead*) AbcMapViewOfFile( hIpcHead, FILE_MAP_WRITE, 0, 0, headSize );
	if ( !IpcHead ) { AbcPanic("MappedPageCache.MapViewOfFile failed"); return false; }
	IpcIndex = (SIpcFileTableIndex*) (IpcHead + 1);
	IpcFiles = (SIpcFileTable*) (IpcIndex + MaxFiles);

	if ( first )
	{
		memset( IpcHead, 0, sizeof(*IpcHead) );
		memset( IpcIndex, 0, MaxFiles * sizeof(*IpcIndex) );
		memset( IpcFiles, 0, MaxFiles * sizeof(*IpcFiles) );
		IpcHead->FileTableIndexSize = MaxFiles;
		IpcHead->Version = CurrentVersion;
		AbcMutexRelease( hGlobalLock );
	}

	return true;
}

void MappedPageCache::CloseIpc()
{
	UnmapViewOfFile( IpcHead );
	IpcHead = NULL;
	IpcIndex = NULL;
	IpcFiles = NULL;
	CloseHandle( hIpcHead );
	hIpcHead = NULL;
	CloseHandle( hGlobalLock );
}

uint32 MappedPageCache::SlotForFile( FileSig sig )
{
	WaitForSingleObject( hGlobalLock, INFINITE );
	int pos = -1;
	for ( uint i = 0; i < IpcHead->FileTableIndexSize && pos == -1; i++ )
	{
		// File already has a slot
		if ( memcmp(IpcIndex[i].Sig.Bytes, sig.Bytes, sizeof(sig)) == 0 && IpcIndex[i].Alive != 0 )
			pos = i;
	}

	FileSig nullsig;
	memset( nullsig.Bytes, 0, sizeof(nullsig) );
	for ( uint i = 0; i < IpcHead->FileTableIndexSize && pos == -1; i++ )
	{
		// Find an empty slot
		if ( IpcIndex[i].Alive == 0 )
		{
			memcpy( IpcIndex[i].Sig.Bytes, sig.Bytes, sizeof(sig) );
			pos = i;
		}
	}

	if ( pos != -1 )
		IpcIndex[pos].Alive++;

	ReleaseMutex( hGlobalLock );
	return pos;
}

void MappedPageCache::Lock( uint16 file, bool acquire )
{
	SIpcFileTable* f = &IpcFiles[ Files[file]->IpcSlot ];
	if ( acquire )
		Acquire( &f->WriteLock );
	else
		Release( &f->WriteLock );
}

void MappedPageCache::Map( uint16 file, int32 block )
{
	if ( MapCount >= MaxMapped4kPages )
		Trim();

	File* f = Files[file];

	uint32 blockShift = f->BlockShift;
	uint32 blockSize = 1 << blockShift;

	if ( f->FMO == NULL || block >= f->Blocks.size() )
	{
		if ( f->FMO )
		{
			// Unmap all existing views
			for ( int i = 0; i < f->Blocks.size(); i++ )
				if ( f->Blocks[i].Ptr )
					Verify( !!UnmapViewOfFile( f->Blocks[i].Ptr ) );
			CloseHandle( f->FMO );
			// lots of dangling pointers now...
		}

		// map the full size of the file, or at least up to this block
		uint64 top = block + 1;
		top = top << blockShift;
		DWORD sizeH;
		DWORD sizeL = GetFileSize( f->FH, &sizeH );
		uint64 existing = (((uint64) sizeH) << 32) | sizeL;
		uint64 existingUp = (existing + blockSize - 1) & ~(blockSize - 1);
		top = max(top, existingUp);
		PROFILE_START(PROF_CREATE_FM);
		f->FMO = CreateFileMapping( f->FH, NULL, PAGE_READWRITE, top >> 32, top, NULL );
		PROFILE_END(PROF_CREATE_FM);
		Verify( f->FMO != NULL );
		//if top > existing then zero data... ??

		int32 topBlock = (top >> blockShift) - 1;
		while ( f->Blocks.size() <= topBlock )
			f->Blocks += BlockP::Make( file );

		// Remap all the views that we trashed above
		for ( int i = 0; i < f->Blocks.size(); i++ )
			if ( f->Blocks[i].Ptr )
				MapBlock( file, i );

		FreshenHandles();
	}

	MapBlock( file, block );
	MapCount += blockSize >> 12;
}

void MappedPageCache::FreshenHandles()
{
	for ( int i = 0; i < Handles.size(); i++ )
	{
		if ( Handles[i].Internal != -1 )
			Handles[i].Ptr = Files[Handles[i].File]->Blocks[Handles[i].Block].Ptr;
	}
}

void MappedPageCache::MapBlock( uint16 file, int32 block )
{
	File* f = Files[file];
	uint32 blockShift = f->BlockShift;
	DWORD mhigh = block >> (32 - blockShift);
	DWORD mlow = block << blockShift;
	PROFILE_START(PROF_MAPVIEW);
	f->Blocks[block].Ptr = MapViewOfFile( Files[file]->FMO, FILE_MAP_WRITE, mhigh, mlow, ((size_t) 1 << blockShift) );
	PROFILE_END(PROF_MAPVIEW);
	Verify( f->Blocks[block].Ptr != NULL );
}

void MappedPageCache::Touch( uint16 file, int32 block )
{
	Files[file]->Blocks[block].Age = Date++;
	if ( Date >= DateWrap )
	{
		// date wraparound
		dvect<BlockP> all;
		CollectAllMapped<true>( all );
		sort( all );
		// We have encoded the block number in the .Ptr field of the block, so we know how to find it after the sort()
		for ( int i = 0; i < all.size(); i++ )
			Files[all[i].File]->Blocks[(int) all[i].Ptr].Age = i;
		// Age is now 0..N-1 where N is number of blocks
		Date = all.size();
	}
}

template< bool make_ptr_block_id >
void MappedPageCache::CollectAllMapped( dvect<BlockP>& all )
{
	for ( int i = 0; i < Files.size(); i++ )
	{
		File* f = Files[i];
		for ( int j = 0; j < f->Blocks.size(); j++ )
			if ( f->Blocks[j].Ptr )
			{
				BlockP tmp = f->Blocks[j];
				tmp.File = i;
				if ( make_ptr_block_id ) tmp.Ptr = (void*) j;
				all += tmp;
			}
	}
}

void MappedPageCache::Trim()
{
	if ( Verbosity > 0 ) printf( "Trim" );

	// Collect all mapped blocks from all files
	dvect<BlockP> all;
	CollectAllMapped<true>( all );
	sort(all);

	// Make sure we don't trim anything with an active handle
	OInt64Set active;
	for ( int i = 0; i < Handles.size(); i++ )
	{
		if ( Handles[i].Internal != -1 )
			active += MakeFileBlockId( Handles[i].File, Handles[i].Block );
	}

	int chop = (TrimFreeBlocksx64 * all.size()) / 64;
	int chopped = 0;
	for ( int i = 0; i < all.size() && chopped < chop; i++ )
	{
		BlockP& block = Files[all[i].File]->Blocks[(int) all[i].Ptr];
		if ( active.contains( MakeFileBlockId(block.File, (int) block.Ptr) ) )
			continue;

		PROFILE_START(PROF_UNMAPVIEW);
		__analysis_assume( block.Ptr != NULL );
		Verify( !!UnmapViewOfFile( block.Ptr ) );
		PROFILE_END(PROF_UNMAPVIEW);
		block.Ptr = NULL;
		block.Age = 0;
		MapCount -= 1 << (Files[all[i].File]->BlockShift - 12);
		chopped++;
	}

	if ( chopped != chop )
	{
		TRACE( "MappedPageCache.Trim failed to free intended number of pages. Too many handles are outstanding.\n" );
	}
}

void MappedPageCache::ReadAligned(    uint16 file, uint64 start, uint32 bytes, void* dat )			{ PROFILE_START(PROF_ALI_READ);		RWAligned<false>(   file, start, bytes, dat ); PROFILE_END(PROF_ALI_READ); }
void MappedPageCache::ReadUnaligned(  uint16 file, uint64 start, uint32 bytes, void* dat )			{ PROFILE_START(PROF_UNALI_READ);	RWUnaligned<false>( file, start, bytes, dat ); PROFILE_END(PROF_UNALI_READ); }
void MappedPageCache::WriteAligned(   uint16 file, uint64 start, uint32 bytes, const void* dat )	{ PROFILE_START(PROF_ALI_WRITE);	RWAligned<true>(    file, start, bytes, const_cast<void*>(dat) );	PROFILE_END(PROF_ALI_WRITE);  }
void MappedPageCache::WriteUnaligned( uint16 file, uint64 start, uint32 bytes, const void* dat )	{ PROFILE_START(PROF_UNALI_WRITE);	RWUnaligned<true>(  file, start, bytes, const_cast<void*>(dat) ); PROFILE_END(PROF_UNALI_WRITE);  }

template< bool TWrite >
void MappedPageCache::ReadWrite( void* filedata, void* userdata, uint32 bytes )
{
	if ( TWrite )	memcpy( filedata, userdata, bytes );
	else			memcpy( userdata, filedata, bytes );
}

// Aligned data (may not span blocks)
template< bool TWrite >
void MappedPageCache::RWAligned( uint16 file, uint64 start, uint32 bytes, void* dst )
{
	if ( TWrite ) Lock( file, true );
	File* f = Files[file];
	char* ptr = (char*) BlockAddr( file, start >> f->BlockShift );
	uint32 rem = ((uint32) start) & f->BlockMask();
	ASSERT( rem + bytes <= f->BlockSize() );
	ReadWrite<TWrite>( ptr + rem, dst, bytes );
	if ( TWrite ) Lock( file, false );
}

// Unaligned data (can span blocks)
template< bool TWrite >
void MappedPageCache::RWUnaligned( uint16 file, uint64 start, uint32 bytes, void* data )
{
	if ( TWrite ) Lock( file, true );
	File* f = Files[file];
	uint32 blockSize = f->BlockSize();
	uint32 block = start >> f->BlockShift;
	uint32 rem = ((uint32) start) & f->BlockMask();
	// Process unaligned first section
	{
		uint32 chunk = min(bytes, blockSize - rem);
		ReadWrite<TWrite>( (char*) BlockAddr( file, block ) + rem, data, chunk );
		(char*&) data += chunk;
		bytes -= chunk;
		block++;
	}

	// I expect that very often this code path will never enter. One might often say RWUnaligned, just to be safe, but
	// you're only reading a small part and you seldom cross page boundaries. Then it's good to have this loop predicted out.
	// rem is now 0. 
	while ( bytes != 0 )
	{
		uint32 chunk = min(bytes, blockSize);
		ReadWrite<TWrite>( BlockAddr( file, block ), data, chunk );
		(char*&) data += blockSize;
		bytes -= chunk;
		block++;
	}
	if ( TWrite ) Lock( file, false );
}

}

//#define FIXED_PAGE_SIZE 16
#undef PROFILE_SLOT
#undef PROFILE_MAX_RECORD
#undef PROFILE_START
#undef PROFILE_END
#undef CACHE_LINE_BYTES
