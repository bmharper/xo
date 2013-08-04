#pragma once

#include "../Containers/pvect.h"
#include "../Other/lmTypes.h"
#include "../Other/profile.h"
#include "../Platform/mmap.h"

void PAPI MPC_ProfileDump( const dvect<int64>& counts );

namespace AbCore
{

class PAPI PageHandle
{
public:
	void*	Ptr;
	uint32	Block;
	uint16	File;
	uint16	Internal;
};

/** Manages the virtual memory - keeping our memory mapped sections within a budget.

The __Aligned functions mean that you are not reading or writing over a block boundary.

Some timings on my Core i7 920. Windows 7 x64. 32-bit binary
------------------------------------------------------------

Operation					Clocks
CreateFileMapping			120,000
MapViewOfFile				4,000
UnmapViewOfFile				3,000

ReadAligned(8 bytes)		75 (range 48-112) (with dynamic page size. By looking at the minimum clocks, I think we can say that we have a 9 clock penalty for dynamic pages.)
ReadAligned(8 bytes)		60 (range 39-115) (with static 16-bit page size)

Concurrency
-----------

Multiple processes can use the cache simultaneously. We have a very restrictive model. Only one process may own the write lock on a file. However,
we do not force reads and writes on a file to be exclusive. I'm assuming for now that I'll be able to build a lock-free data structure on top of this.

Things controlled by the GlobalLock:
* Adding/Removing files from the IpcFileTable


Constraints
-----------

Maximum number of file handles:		65536 (but practically limited to MaxFiles)
Maximum size of mapped file:		BlockSize * 2^32
Range of BlockSize:					[16, 20] (upper range is arbitrary, could probably be 31. lower range is dependent on allocation granularity)
																		Raymond Chen wrote about why the allocation granularity is 64k and not 4k as one might expect.
																		At object creation, we read the system granularity and store it in AllocationGranularity. Your block size
																		cannot be smaller than this.


How does this work?
-------------------

We manage a set of files that you want fast access to. You pass the manager a file HANDLE, it returns
you a 16-bit integer. Thereafter, you use that 16-bit integer id to read and write from the file.

We have a budget of mapped address space, and we make sure not to exceed that budget.

To read from a file, you call Read(), and to write, you call Write(). These functions ensure that the region
you seek is mapped. They also manage the concurrency, so that multiple threads can read and write simultaneously.

Ideas
-----

Updating Date++ is a potential hotspot for multithread read-only access.

Solution 1: Have 16 Date[] slots (on different cache lines), and have every function pass in a thread id. Use the low 4 bits of
	the thread id to find the appropriate Date slot.

Solution 2: Hmm....


**/
class PAPI MappedPageCache
{
public:
	// Make this constant so that we don't have to worry about resizing the IpcFileTableIndex table dynamically. 4096 * 20 = 80kb
	static const uint MaxFiles			= 4096;
	static const uint CurrentVersion	= 1000;
	static const uint MaxBlockShift		= 20;

	struct FileSig
	{
		byte Bytes[16];
	};

	struct SIpcHead
	{
		uint32 Version;
		uint32 FileTableIndexSize;
		//uint32 GlobalLock;
		// Array of IpcFileTableIndex[FileTableIndexSize] follows immediately.
		// Array of SIpcFileTable[FileTableIndexSize] then follows.
	};

	struct SIpcFileTableIndex
	{
					FileSig		Sig;
		volatile	uint32		Alive;	///< Incremented by everybody who uses the file. Decremented when they close the file. Exists so that we can recycle slots.
	};

	// We could put this into SIpcFileTableIndex, but we keep it separate so that they are tighter in the cache.
	struct SIpcFileTable
	{
		volatile uint32 WriteLock;
	};

	//struct PrivateFile
	//{
	//	HANDLE FH;
	//};

	// Block pointer
	struct BlockP
	{
		void*	Ptr;
		uint32	Age;
		int32	File;	// you can make this 16 bit if you need to free up 16 bits (the other half).

		static BlockP Make( uint16 file )			{ BlockP b; b.Ptr = NULL; b.Age = 0; b.File = file; return b; }
		bool operator<( const BlockP& b ) const		{ return Age < b.Age; }
	};

	struct File
	{
		HANDLE			FH;						///< File Handle
		HANDLE			FMO;					///< File Mapping Object
		uint32			IpcSlot;				///< Slot in IpcFileTable[] array
		dvect<BlockP>	Blocks;
#ifdef FIXED_PAGE_SIZE
		static const uint32 BlockShift = FIXED_PAGE_SIZE;
#else
		uint32			BlockShift;
#endif

		uint32 BlockSize() { return  1 << BlockShift; }
		uint32 BlockMask() { return (1 << BlockShift) - 1; }	///< Mask covering lower bits
	};


	uint32				MapCount;				// Number of 4k pages that we have mapped
	uint32				Date;
	uint32				DateWrap;				// Leave this as a non-constant so that it is trivial to test the date wraparound.
	uint32				AllocationGranularity;
	uint32				MaxMapped4kPages;		// Maximum number of 4k pages that we will map.
	int32				TrimFreeBlocksx64;		// Number of blocks that we free when we Trim, expressed as a multiple of 1/64. Setting this to 3 will cause 3/64 of your blocks to be liberated with each Trim().
	int					Verbosity;

	dvect<int64>		Profile;
	pvect<File*>		Files;			// file id = index in this array
	dvect<PageHandle>	Handles;

	void Verify( bool res );

	MappedPageCache();
	~MappedPageCache();

	uint16		AddFile( HANDLE fh, const void* sigBytes, uint32 blockShift );
	void		RemoveFile( HANDLE fh, const void* sigBytes );
	void		WriteLockAcquire( uint16 file );
	void		WriteLockRelease( uint16 file );
	void*		BlockAddr( uint16 file, uint32 block );
	PageHandle	Acquire( uint16 file, int32 block );		// Ensure that this address is mapped, and ensure that it remains mapped until we call Release.
	void		Release( PageHandle h );					// Release a block that was locked by Acquire

	template< bool TWrite >		void ReadWrite( void* filedata, void* userdata, uint32 bytes );
	template< bool TWrite >		void RWAligned( uint16 file, uint64 start, uint32 bytes, void* dst );		// Aligned data (may not span blocks)
	template< bool TWrite >		void RWUnaligned( uint16 file, uint64 start, uint32 bytes, void* data );	// Unaligned data (can span blocks)

	void ReadAligned(    uint16 file, uint64 start, uint32 bytes, void* dat );
	void ReadUnaligned(  uint16 file, uint64 start, uint32 bytes, void* dat );
	void WriteAligned(   uint16 file, uint64 start, uint32 bytes, const void* dat );
	void WriteUnaligned( uint16 file, uint64 start, uint32 bytes, const void* dat );

	uint32 Debug_NumBlocks( uint16 file ) { return Files[file]->Blocks.size(); }

protected:

	HANDLE					hGlobalLock;
	HANDLE					hIpcHead;
		
	SIpcHead*				IpcHead;
	SIpcFileTableIndex*		IpcIndex;
	SIpcFileTable*			IpcFiles;

	// We don't make any attempt to recover from a failure. We don't clean up handles properly etc.
	// The assumption is that if this setup fails, your entire app can't run.
	bool	InitIpc();
	void	CloseIpc();
	uint32	SlotForFile( FileSig sig );
	void	Acquire( volatile uint32* v );
	void	Release( volatile uint32* v );
	void	Lock( uint16 file, bool acquire );
	void	Map( uint16 file, int32 block );		// Memory map this block
	void	FreshenHandles();
	void	MapBlock( uint16 file, int32 block );
	void	Touch( uint16 file, int32 block );
	void	Trim();									// We have exceeded our budget. Trim blocks.

	INT64	MakeFileBlockId( uint16 file, int32 block ) { return ((INT64) file << 32) | block; }

	template< bool make_ptr_block_id >
	void	CollectAllMapped( dvect<BlockP>& all );

};

}
