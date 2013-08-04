#ifndef PAN_INCLUDED_BUFFERED_FILE_H
#define PAN_INCLUDED_BUFFERED_FILE_H

#include "../Other/ChunkList.h"
#include "../Other/GlobalCache.h"
#include "../HashTab/ohashcommon.h"
#include "VirtualFile.h"

#include "IO.h"

namespace Panacea
{
namespace IO
{

//#define ULTRA_MAP

/** Buffered file that sits on top of an IFile.

[Dec 12 2010] - This this is a piece o' crap. The whole ultra buffers thing is really hard
to understand. One could make it much simpler. Use a power 2 hash table, and just do classic 
linear probing. There shouldn't be an ultra and a regular mode. Just one mode that does everything.
And splitting a read/write over multiple buffers shouldn't make it give up. AND.. fast writes
should obviously always be enabled.. an unnecessary read penalty is ridiculous.

Since most OS-managed files are buffered anyway, there is usually no reason to 
buffer large disk accesses. However, there is still a fixed penalty in calling 
the OS, and this is intended to reduce that number of calls. The intended audience 
is file accesses when the unit size is less than a few hundred bytes.

The BufferedFile works by creating a small set of buffers, typically of size 4096
bytes. If the user attempts to read some data from the file, then the BufferedFile
ensures that there is a buffer covering that region. There are two exceptions to this,
namely when the region to be read spans two buffer locations (for example if the user
were to read bytes 4090 to 5001). The second exception is when the amount of data
requested is larger than the buffer size. In either case, we simply do not buffer.

Whenever a write is made, the BufferedFile behaves similarly to when reading, except
that it will first pull in the requested buffer region before modifying it.

If you create a BufferedFile on top of another, the layered one simply acts in a passthrough
mode, forwarding all requests through to the underlying buffer.


UltraCache thoughts:
Benchmarks looks ok for 32 concurrent streams. Slowdown compared to sequential writes is pretty
small, even when chunk sizes are on the order of about 10 bytes or so. So that means that either
the buffer mechanism in general has bad overheads, or that the hashed buffers are good.
I should actually test against a MemFile, as that is obviously the golden benchmark.

**/
class PAPI BufferedFile : public AbCore::IFile, public AbCore::ICacher
{
protected:
	enum BufferStat
	{
		BuffNone = -1,
		BuffWriteThrough = -2,
		BuffPanic = -3
	};
public:

	static const UINT64 Nowhere = -1;
	static const UINT32 NoHash = -1;

	BufferedFile( IFile* file = NULL, size_t bufferSize = 0, int bufferCount = 0, int ultraSlots = 0 )
	{
		FastWrites = true;
		PassThrough = false;
		BufferDataBlock = NULL;
		Buffers = NULL;
		Base = NULL;
		SwapBuffer = NULL;
		Reset( file, bufferSize, bufferCount, ultraSlots );
	}

	IFile* GetBase() { return Base; }

	void DebugSetFastWrites( bool on ) { FastWrites = on; }

	void Reset( IFile* file = NULL, size_t bufferSize = 0, int bufferCount = 0, int ultraSlots = 0 );

	// Note that Reset() resets the state of the passthrough flag
	void SetPassThrough( bool passThrough )
	{
		ASSERT( !Dirty() );
		PassThrough = passThrough;
	}

	bool IsPassThrough() const { return PassThrough; }

	virtual ~BufferedFile() 
	{
		ASSERT( !Dirty() );
		DestroyBuffers();
	}

	virtual void GlobalCacheTick()
	{
		if ( FlushAllBuffers() )
		{
			DestroyBuffers();
		}
	}

	/** Discards all buffers. Does not flush, does not verify that none are dirty, does not collect $100.
	
	Where might you use this?
	You might use this in a testing environment, because you wanted to avoid a debug assert when the 
	object was closed with dirty buffers outstanding. In this specific case, the BufferedFile sits 
	on top of a faulty file device, and when we detect a fault, we just abort immediately, leaving
	buffers in their dirty state.

	The avdb file also uses this generically when it wants to discard a write operation. It can do so
	because all of its writes are COW. In general though, you would not use this function unless
	an error condition has arisen.

	**/
	void TrashAllBuffers()
	{
		DestroyBuffers();
	}

	bool Dirty()
	{
		if ( Buffers )
		{
			for ( UINT32 i = 0; i < BufferCount; i++ )
				if ( Buffers[i].Dirty ) return true;
		}
		return false;
	}

	/** Flushes all dirty buffers to the underlying IFile object, but does not call Flush() on the underlying IFile.
	For a full flush, use the virtual function Flush().
	
	@param invalidate If true, then we invalidate all buffers after the flush. You would use this if you were going to
		modify the file directly yourself, immediately following the call to FlushAllBuffers().

	**/
	bool FlushAllBuffers( bool invalidate = false, UINT64 start = 0, UINT64 length = -1 )
	{
		if ( Buffers )
		{
			bool all = length == -1;
			for ( UINT32 i = 0; i < BufferCount; i++ )
			{
				bool doit;
				if ( all )
					doit = true;
				else
					doit = Buffers[i].PositiveUnion( start, length );

				if ( doit )
				{
					if ( !FlushBuffer( i, false ) )
						return false;
					if ( invalidate )
						Buffers[i].HasRead = false;
				}
			}
		}
		return true;
	}

	void FlushAndDestroyAllBuffers()
	{
		// We have an error hole here.
		FlushAllBuffers( false );
		DestroyBuffers();
	}

	/// Calls FlushAllBuffers(), sleeps the buffers, and then calls Flush() on the underlying IFile object.
	virtual bool Flush( int flags )
	{
		if ( PassThrough )
		{
			return Base->Flush( flags );
		}
		else
		{
			bool res = FlushAllBuffers();
			if ( !res ) return false;
			if ( Base != NULL ) res = Base->Flush( flags );
			if ( !res ) return false;
			DestroyBuffers();
			return true;
		}
	}

	int GetBufferSize() { return BufferSize; }
	int GetBufferCount() { return BufferCount; }

	virtual bool Error() { return Base ? Base->Error() : ErrorOk; }
	virtual XString ErrorMessage() { return Base ? Base->ErrorMessage() : ""; }
	virtual void ClearError() { if ( Base ) Base->ClearError(); }

	virtual size_t Write( const void* data, size_t bytes );
	virtual size_t Read( void* data, size_t bytes );

	// Have to do these overloads for stupid C++ inheritance rule.
	// It's as stupid as the template specialization rule of having to specify the entire specialization explicitly.
	size_t Write( size_t bytes, IFile* src )
	{
		return IFile::Write( bytes, src );
	}

	size_t Read( size_t bytes, IFile* dst )
	{
		return IFile::Read( bytes, dst );
	}

	virtual UINT64 Position() { return PassThrough ? Base->Position() : mPosition; }

	virtual UINT64 Length() { return PassThrough ? Base->Length() : mLength; }

	virtual bool SetLength( UINT64 len )
	{
		if ( len < mLength )
		{
			if ( !FlushAllBuffers() ) return false;
		}
		if ( Base->SetLength( len ) )
		{
			mLength = len;
			return true;
		}
		return false;
	}

	virtual bool Seek( UINT64 pos )
	{
		if ( PassThrough )
		{
			return Base->Seek(pos);
		}
		else
		{
			mPosition = pos;
			return true;
		}
	}

	virtual bool SeekRel( INT64 pos )
	{
		if ( PassThrough )
		{
			return Base->SeekRel( pos );
		}
		else
		{
			if ( pos != 0 )
				return Seek( mPosition + pos );
			else
				return true;
		}
	}

protected:
	IFile* Base;

	struct Buffer
	{
		//~Buffer() { free( Data ); }

		/// Returns true if the buffer fully covers the region
		bool Occupies( UINT64 pos, size_t length ) { return Pos >= 0 && Pos <= pos && Pos + Size >= pos + length; }
		
		/// Returns true if the buffer covers any byte of the region
		bool PositiveUnion( UINT64 pos, size_t length )
		{
			if ( Pos < pos )
				return Pos + (UINT64) Size > pos;
			else
				return pos + (UINT64) length > Pos;
		}

		// Guaranteed to be aligned to BufferSize
		UINT64 Pos;
		UINT32 Hash;
		size_t Size;
		bool Dirty;
		bool HasRead;
		int LastUsed;
		BYTE* Data;
		ChunkList32 Chunks;
	};

	UINT64 mPosition;
	UINT64 mLength;
	
	/** Allow writes that do not read the buffer in before writing.
	This should always be turned on. I only had this flag initially when testing it.
	**/
	bool FastWrites;

	/** UltraCache slots. Stupid name for huge cache.
	This mode was written for avdb, which stores each database field in a separate file. Thus, it's usage pattern
	is typically very wide, in the sense that it is concurrently accessing many streams at once. This would
	render our simple cache useless. UltraCache mode essentially allows for a much larger number of buffers,
	and it employs a hash-like mechanism to manage those buffers.
	**/
	UINT32 UltraSlots;
	

#ifdef ULTRA_MAP
	ohashmap<UINT64, UINT32> Ultra;
#else
	/// UltraCache buffers.
	dvect<int> UltraMain;
#endif

	/** Lower half of a mask for removing the insignificant bits of a full address.
	If you discard the higher 32 bits, then AND the lower 32 with BufferMask you basically remove the offset portion of the address.
	**/
	UINT32 BufferMask;

	/// Full BufferMask
	UINT64 BufferMask64;

	/// Final mask that yields us a slot.
	//UINT32 UltraMask;

	bool PassThrough;			///< What it says
	int TickStamp;
	UINT32 BufferCount;
	UINT32 BufferSize;
	Buffer* Buffers;
	BYTE* SwapBuffer;			///< Last buffer in BufferDataBlock
	BYTE* BufferDataBlock;		///< Block from which the individual buffer data memories are sliced, as well as the SwapBuffer

	bool Dormant() { return Buffers == NULL; }

	void DestroyBuffers()
	{
		//free( SwapBuffer ); SwapBuffer = NULL;
		//Buffers.clear();
		free(BufferDataBlock); BufferDataBlock = NULL;
		delete[] Buffers; Buffers = NULL;
#ifdef ULTRA_MAP
		Ultra.clear();
#else
		UltraMain.clear();
#endif
		SwapBuffer = NULL;
	}

	void CreateBuffers()
	{
		DestroyBuffers();

		if ( UltraSlots )
		{
#ifdef ULTRA_MAP
			Ultra.clear();
#else
			UltraMain.resize( UltraSlots );
			UltraMain.fill( -1 );
#endif
		}

		// Allocate enough space for all our buffers, as well as the SwapBuffer (therefore the +1)
		BufferDataBlock = (BYTE*) malloc((BufferCount + 1) * BufferSize);

		SwapBuffer = BufferDataBlock + BufferCount * BufferSize;
		
		Buffers = new Buffer[BufferCount];

		for ( UINT32 i = 0; i < BufferCount; i++ )
		{
			Buffer* b = Buffers + i;
			b->Pos = Nowhere;
			b->Hash = NoHash;
			b->Size = BufferSize;
			b->Dirty = false;
			b->HasRead = false;
			b->LastUsed = Tick();
			b->Data = BufferDataBlock + i * BufferSize;
		}
	}

	/** Yields a slot number, given an absolute address.
	It is imperative that -1 be an invalid slot address. This should not be a problem.
	**/
	UINT32 UltraHash( UINT64 bpos )
	{
		UINT32 a = bpos >> 32;
		UINT32 b = ((UINT32) bpos) & BufferMask;
		UINT32 o1 = a ^ b;
		UINT32 c = o1 >> 16;
		UINT32 d = o1 & 0xFFFF;
		UINT32 o2 = c + 65539 * d;
		//return o2 & UltraMask;
		return o2 % UltraSlots;
	}

	int Tick()
	{
		if ( TickStamp > 100000 )
			ResetTime();
		TickStamp++;
		return TickStamp;
	}

	void ResetTime()
	{
		if ( Buffers )
		{
			// discard all time
			for ( UINT32 i = 0; i < BufferCount; i++ )
				Buffers[i].LastUsed = 1;
		}
		TickStamp = 1;
	}

	bool FlushBuffer( int index, bool retire );
	void RetireBuffer( int index );
	void RegisterBuffer( int index );
	int FlushOldestBuffer();

	UINT64 Root( UINT64 pos )
	{
		return pos & BufferMask64;
	}

	bool BringBufferIn( int index );

	BufferStat FindBuffer( UINT64 pos, size_t length, bool write, int& bufA, int& bufB );
	int FindBufferStep2( UINT64 root, bool write );

	size_t WriteToBuffer( int buffer, const BYTE* data, size_t bytes );
	size_t ReadFromBuffer( int buffer, BYTE* data, size_t bytes );

};

}
}

#endif
