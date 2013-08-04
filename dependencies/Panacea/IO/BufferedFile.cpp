#include "pch.h"
#include "BufferedFile.h"
#include "../Other/profile.h"

namespace Panacea
{
namespace IO
{

	void BufferedFile::Reset( IFile* file, size_t bufferSize, int bufferCount, int ultraSlots )
	{
		FlushAndDestroyAllBuffers();
		PassThrough = file != NULL && (typeid(*file) == typeid(BufferedFile) || typeid(*file) == typeid(AbCore::MemFile));
		ResetTime();
		Base = file;
		if ( bufferCount == 0 ) BufferCount = 4;
		else					BufferCount = max( bufferCount, 1 );
		if ( bufferSize == 0 )	BufferSize = 2048;
		else					BufferSize = (int) std::max( bufferSize, (size_t) 32 );

		BufferSize = AbCore::NextPower2(BufferSize);
		
		//UltraSlots = ultraSlots == 0 ? 0 : AbCore::NextPower2(ultraSlots);
		UltraSlots = ultraSlots == 0 ? 0 : ohash::NextPrime( ultraSlots );

		//UltraMask = UltraSlots - 1;
		BufferMask64 = ~((UINT64) BufferSize - 1);
		BufferMask = (UINT32) BufferMask64;

		if ( file != NULL )
		{
			mPosition = file->Position();
			mLength = file->Length();
		}
		else
		{
			mPosition = 0;
			mLength = 0;
		}
	}

	size_t BufferedFile::Write( const void* data, size_t bytes )
	{
		// Sanity check (typically catches underflow)
		ASSERT( bytes < 0x80000000 );

		// This is not just a short-circuit. Our buffer mechanism assumes that length is > 0.
		if ( bytes == 0 ) return 0; 

		//if ( mPosition <= 659459 && 659459 < mPosition + bytes )
		//	int aaaa = 1;
		if ( PassThrough )
		{
			return Base->Write( data, bytes );
		}
		else
		{
			size_t written = 0;
			int bufA, bufB;
			BufferStat bstat = FindBuffer( mPosition, bytes, true, bufA, bufB );
			if ( bstat == BuffPanic ) return 0;
			else if ( bstat == BuffWriteThrough )
			{
				// FindBuffer has flushed this region for us, and told us that the requested size is too large
				// to be worth buffering, so we must write it directly.
				Base->Seek( mPosition );
				written = Base->Write( data, bytes );
				mPosition += written;
			}
			else
			{
				const BYTE* bdat = (const BYTE*) data;
				written = WriteToBuffer( bufA, bdat, bytes );
				mPosition += written;
				bytes -= written;
				bdat += written;
				if ( bufB != BuffNone )
				{
					size_t w2 = WriteToBuffer( bufB, bdat, bytes );
					written += w2;
					mPosition += w2;
				}
			}
			mLength = max( mPosition, mLength );
			return written;
		}
	}

	size_t BufferedFile::WriteToBuffer( int buffer, const BYTE* data, size_t bytes )
	{
		Buffer* b = &Buffers[buffer];
		size_t rel = mPosition - b->Pos;
		bytes = min(bytes, BufferSize - rel);
		memcpy( b->Data + rel, data, bytes );
		b->Dirty = true;
		if ( !b->HasRead )
		{
			b->Chunks.Add( (UINT32) rel, (UINT32) bytes );
		}
		return bytes;
	}

	size_t BufferedFile::Read( void* data, size_t bytes )
	{
		// Sanity check (typically catches underflow)
		ASSERT( bytes < 0x80000000 );
		
		// This is not just a short-circuit. Our buffer mechanism assumes that length is > 0.
		if ( bytes == 0 ) return 0; 

		if ( PassThrough )
		{
			return Base->Read( data, bytes );
		}
		else
		{
			PROFILE_SECTION( PROF_BUF_FILE_READ );
			size_t read = 0;
			int bufA, bufB;
			BufferStat bstat = FindBuffer( mPosition, bytes, false, bufA, bufB );
			if ( bstat == BuffPanic ) return 0;
			else if ( bstat == BuffWriteThrough )
			{
				// FindBuffer will have flushed all relevant buffers.
				Base->Seek( mPosition );
				read = Base->Read( data, bytes );
				mPosition += read;
			}
			else
			{
				BYTE* bdat = (BYTE*) data;
				read = ReadFromBuffer( bufA, bdat, bytes );
				mPosition += read;
				bytes -= read;
				bdat += read;
				if ( bufB != BuffNone )
				{
					size_t r2 = ReadFromBuffer( bufB, bdat, bytes );
					read += r2;
					mPosition += r2;
				}
			}
			return read;
		}
	}

	size_t BufferedFile::ReadFromBuffer( int buffer, BYTE* data, size_t bytes )
	{
		size_t rel = mPosition - Buffers[buffer].Pos;
		bytes = min(bytes, BufferSize - rel);
		memcpy( data, Buffers[buffer].Data + rel, bytes );
		return bytes;
	}

	bool BufferedFile::FlushBuffer( int index, bool retire )
	{
		if ( Buffers[index].Dirty )
		{
			if ( !Buffers[index].HasRead )
			{
				// We do not know the whole buffer's contents.
				// Should do a heuristic here.. will have to measure.. where we pull in the whole buffer before writing.
				// The heuristic depends on the amount of overhead per call to the kernel.
				Buffer* b = &Buffers[index];
				for ( int i = 0; i < b->Chunks.Count(); i++ )
				{
					UINT32 start, len;
					b->Chunks.Get( i, start, len );
					if ( !Base->Seek( b->Pos + start ) ) return false;
					if ( Base->Write( b->Data + start, len ) != len ) return false;
				}
				Buffers[index].Chunks.Clear();
			}
			else
			{
				// We own the whole buffer's data
				size_t bytes = min( mLength - Buffers[index].Pos, (UINT64) Buffers[index].Size );
				// Should assert here if these operations fail.
				if ( !Base->Seek( Buffers[index].Pos ) ) return false;
				if ( Base->Write( Buffers[index].Data, bytes ) != bytes ) return false;
			}
			Buffers[index].Dirty = false;
		}
		if ( retire ) RetireBuffer( index );
		return true;
	}

	int BufferedFile::FlushOldestBuffer()
	{
		PROFILE_SECTION( PROF_BUF_FILE_FLUSH_OLDEST );

		int oldest = MAXINT;
		int index = 0;
		for ( int i = 0; i < (int) BufferCount; i++ )
		{
			if ( Buffers[i].LastUsed < oldest )
			{
				oldest = Buffers[i].LastUsed;
				index = i;
			}
		}
		if ( !FlushBuffer( index, true ) )
			return BuffPanic;
		else
			return index;
	}

	bool BufferedFile::BringBufferIn( int index )
	{
		PROFILE_SECTION( PROF_BUF_FILE_BRING_IN );

		bool writes = false;
		Buffer* b = &Buffers[index];
		
		if ( b->Pos == 2342912 )
			int aaa = 1;

		if ( b->Dirty )
		{
			// the buffer has only been written to so far.
			ASSERT( !b->HasRead );
			ASSERT( b->Chunks.Count() > 0 );
			writes = true;
			memcpy( SwapBuffer, b->Data, BufferSize );
		}

		if ( b->Pos < Base->Length() )
		{
			size_t rbytes = min( (size_t) (Base->Length() - b->Pos), b->Size );
			if ( !Base->Seek( b->Pos ) ) return false;
			if ( Base->Read( b->Data, rbytes ) != rbytes ) return false;
		}

		if ( writes )
		{
			// bring in dirty writes
			for ( int i = 0; i < b->Chunks.Count(); i++ )
			{
				UINT32 start, len;
				b->Chunks.Get( i, start, len );
				memcpy( b->Data + start, SwapBuffer + start, len );
			}
		}

		b->HasRead = true;
		b->Chunks.Clear();
		
		return true;
	}


	BufferedFile::BufferStat BufferedFile::FindBuffer( UINT64 pos, size_t length, bool write, int& bufA, int& bufB )
	{
		PROFILE_SECTION( PROF_BUF_FILE_FIND_BUFFER );

		// The Root(pos + length - 1) produces a negative value if length is zero, which is why we make this an error condition.
		ASSERT( length > 0 );

		// Flush and return negative if the requested length is more than our buffer size.
		bufA = BuffNone;
		bufB = BuffNone;

		UINT64 rootA = Root(pos);
		UINT64 rootB = Root(pos + length - 1);
		
		if ( length > BufferSize || (rootA != rootB && BufferCount == 1) )
		{
			if ( !FlushAllBuffers( true, pos, length ) )
				return BuffPanic;
			return BuffWriteThrough;
		}

		bufA = FindBufferStep2( rootA, write );
		if ( bufA < 0 )
			return (BufferStat) bufA; // panic

		if ( rootA != rootB )
		{
			bufB = FindBufferStep2( rootB, write );
			ASSERT( bufA != bufB );
		}
		if ( bufB < 0 )
			return (BufferStat) bufB; // panic

		// OK
		return BuffNone;
	}

	int BufferedFile::FindBufferStep2( UINT64 root, bool write )
	{
		int buffer = -1;

		if ( Buffers )
		{
			bool linear = true;
			if ( UltraSlots )
			{
#ifdef ULTRA_MAP
				linear = false;
				buffer = Ultra.get(root) - 1;
#else
				int ibuf = UltraMain[ UltraHash(root) ];
				if ( ibuf != -1 )
				{
					if ( Buffers[ibuf].Pos == root )
					{
						// bingo
						linear = false;
						buffer = ibuf;
					}
					else
					{
						// collision
					}
				}
				else
				{
					// not existent - no need to search linearly, we know it's not there. This condition is guaranteed because
					// whenever we remove an object from UltraMain, we replace it with any potential collisions. The assumption
					// is that our number of collisions will be low.
					linear = false;
				}
#endif
			}
			if ( linear )
			{
				for ( int i = 0; i < (int) BufferCount; i++ )
				{
					if ( Buffers[i].Pos == root )
					{
						buffer = i;
						break;
					}
				}
			}
		}

		if ( buffer != -1 )
		{
			if ( !Buffers[buffer].HasRead && !write )
			{
				if ( !BringBufferIn( buffer ) )
					return BuffPanic;
			}
			Buffers[buffer].LastUsed = Tick();
			return buffer;
		}

		if ( Dormant() )
		{
			CreateBuffers();
			buffer = 0;
		}
		else
		{
			buffer = FlushOldestBuffer();
			if ( buffer == BuffPanic )
				return BuffPanic;
		}
		Buffer* b = &Buffers[buffer];
		ASSERT( !b->Dirty );
		ASSERT( b->Chunks.Count() == 0 );
		b->Pos = root;
		if ( UltraSlots ) b->Hash = UltraHash(root);
		b->HasRead = false;
		b->LastUsed = Tick();
		RegisterBuffer( buffer );
		if ( !write || !FastWrites )
		{
			if ( !BringBufferIn( buffer ) )
				return BuffPanic;
		}
		else
		{
			// this is a general assumption about files, that they are initialized to zero if you write to a position in the nethers
			memset( b->Data, 0, BufferSize );
		}
		return buffer;
	}


	// logical opposite to RetireBuffer
	void BufferedFile::RegisterBuffer( int index )
	{
		if ( UltraSlots )
		{
#ifdef ULTRA_MAP
			// zero = empty
			Ultra[ Buffers[index].Pos ] = index + 1;
#else
			UINT32 myHash = Buffers[index].Hash;
			if ( UltraMain[myHash] == -1 )
			{
				// no collisions.. good
				UltraMain[myHash] = index;
			}
#endif
		}
	}

	void BufferedFile::RetireBuffer( int index )
	{
		ASSERT( !Buffers[index].Dirty );
		if ( UltraSlots )
		{
#ifdef ULTRA_MAP
			if ( Buffers[index].Pos != Nowhere )
			{
				Ultra.erase( Buffers[index].Pos );
				Buffers[index].Pos = Nowhere;
			}
#else
			// make sure that if any buffer has this one's hash, then it takes it's place
			UINT32 myHash = Buffers[index].Hash;
			Buffers[index].Pos = Nowhere;
			Buffers[index].Hash = NoHash;
			if ( myHash != NoHash && UltraMain[myHash] == index )
			{
				// This buffer occupied the hash slot
				UltraMain[myHash] = -1;
				for ( int i = 0; i < (int) BufferCount; i++ )
				{
					if ( Buffers[i].Hash == myHash )
					{
						// Promote this buffer into the slot
						UltraMain[myHash] = i;
						break;
					}
				}
			}
#endif
		}
		else
		{
			Buffers[index].Pos = Nowhere;
		}
	}

}
}
