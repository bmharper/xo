#ifndef ABCORE_INCLUDED_IFILE_H
#define ABCORE_INCLUDED_IFILE_H

#include "../Strings/XString.h"

namespace AbCore
{

/** Interface for a file.
This is the interface used by the native file mechanism.
It has quite a few helpers to ease implementation.
**/
class PAPI IFile
{
public:

	/** Share modes that correspond to the definitions in the Win32 CreateFile() definition.
	These are ignored on linux!
	**/
	enum ShareMode
	{
		ShareNone = 0,
		ShareDelete = 1,
		ShareRead = 2,
		ShareWrite = 4
	};

	/** Flush flags.

	It often doesn't make sense to flush to permanent storage without also flushing internal buffers, so implementations are free
	to do a buffer flush implicitly, when asked for a permanent storage flush.

	This interface was added more as a way to exclude permanent storage flush (ie FlushFileBuffers), than as a way to exclude buffer flush.

	**/
	enum FlushFlag
	{
		/// Ensure that any buffers held internally by the IFile implementation have been written to their underlying system.
		FlushFlagBuffers = 1,

		/// Ensure that any content which has been written to the stream, has been written to permanent storage before the function returns.
		FlushFlagPermanentStorage = 2,
	};

	virtual ~IFile() {}
	
	/// Return true if there has been a read or write error
	virtual bool Error() = 0;

	/// Returns the error message, if any.
	virtual XString ErrorMessage() = 0;

	/// Clear the error flag.
	virtual void ClearError() = 0;

	/** Generic flush function.
	Open to reasonable interpretation. An OS-based file should obviously issue an appropriate OS flush call.
	**/
	virtual bool Flush( int flags ) = 0;
	bool FlushBuffers()				{ return Flush(FlushFlagBuffers); }
	bool FlushPermanentStorage()	{ return Flush(FlushFlagBuffers | FlushFlagPermanentStorage); }

	virtual size_t Write( const void* data, size_t bytes ) = 0;
	virtual size_t Read( void* data, size_t bytes ) = 0;
	virtual bool SetLength( UINT64 len ) = 0;
	virtual UINT64 Position() = 0;
	virtual UINT64 Length() = 0;
	virtual bool Seek( UINT64 pos ) = 0;
	virtual bool SeekRel( INT64 pos ) 
	{
		if (pos != 0)
			return Seek( Position() + pos );
		else
			return true;
	}

	bool Eof()
	{
		return Position() == Length();
	}


	/// Seek to pos then write.
	size_t WriteAt( UINT64 pos, const void* data, size_t bytes )
	{
		if ( !Seek(pos) ) return 0;
		return Write( data, bytes );
	}

	/// Seek to pos then read.
	size_t ReadAt( UINT64 pos, void* data, size_t bytes )
	{
		if ( !Seek(pos) ) return 0;
		return Read( data, bytes );
	}

	// Reads intelligently. for forward/back compatibility
	// Won't overwrite memory, but will seek over unknown bytes if neccessary
	void ReadPad( void* data, size_t max_bytes_to_read, size_t bytes_on_disk )
	{
		size_t desire = _min_(max_bytes_to_read, bytes_on_disk);
		INT64 br = Read( data, desire );
		if ( bytes_on_disk > max_bytes_to_read )
			SeekRel( bytes_on_disk - br );
	}

	double ReadDouble( bool peek = false )	{ return ReadPOD<double>(peek); }
	float ReadFloat( bool peek = false )	{ return ReadPOD<float>(peek); }
	INT64 ReadInt64( bool peek = false )	{	return ReadPOD<INT64>(peek); }
	INT32 ReadInt32( bool peek = false )	{	return ReadPOD<INT32>(peek); }
	INT16 ReadInt16( bool peek = false )	{	return ReadPOD<INT16>(peek); }
	INT8  ReadInt8( bool peek = false )		{	return ReadPOD<INT8>(peek); }
	UINT64 ReadUInt64( bool peek = false )	{	return ReadPOD<UINT64>(peek); }
	UINT32 ReadUInt32( bool peek = false )	{	return ReadPOD<UINT32>(peek); }
	UINT16 ReadUInt16( bool peek = false )	{	return ReadPOD<UINT16>(peek); }
	UINT8  ReadUInt8( bool peek = false )	{	return ReadPOD<UINT8>(peek); }

	template< typename POD >
	bool WritePOD( const POD& v ) { return Write( &v, sizeof(v) ) == sizeof(v); }

	bool WriteDouble( double v )			{ return WritePOD(v); }
	bool WriteFloat( float v )				{ return WritePOD(v); }
	bool WriteInt64( INT64 v )				{ return WritePOD(v); }
	bool WriteInt32( INT32 v )				{ return WritePOD(v); }
	bool WriteInt16( INT16 v )				{ return WritePOD(v); }
	bool WriteInt8( INT8 v )				{ return WritePOD(v); }
	bool WriteUInt64( UINT64 v )			{ return WritePOD(v); }
	bool WriteUInt32( UINT32 v )			{ return WritePOD(v); }
	bool WriteUInt16( UINT16 v )			{ return WritePOD(v); }
	bool WriteUInt8( UINT8 v )				{ return WritePOD(v); }


	/** Writes zero in chunks of 8k.
	@return The number of bytes written.
	**/
	UINT64 WriteZero( UINT64 bytes ) { return WriteFill(bytes, 0); }

	/** Analogue to memset.
	@return The number of bytes written.
	**/
	UINT64 WriteFill( UINT64 bytes, BYTE fill )
	{
		BYTE buff[4096];
		size_t bufFillBytes = sizeof(buff);
		if ( bytes < bufFillBytes ) bufFillBytes = bytes;
		memset( buff, fill, bufFillBytes );
		UINT64 sum = 0;
		UINT64 i = 0;
		while ( bytes - i > 0 )
		{
			UINT64 remain = _min_( bytes - i, sizeof(buff) );
			UINT64 written = Write( buff, remain );
			sum += written;
			if ( written != remain ) break;
			i += remain;
		}
		return sum;
	}


	/** Size of ReadLine's static buffer.
	If a line is longer than this number of characters, then we need to allocate on the heap.
	This number is published here primarily for the testing framework.
	We make this number 256 because it seems like a reasonable thumb suck. A 24 inch wide screen
	can comfortably show about 256 characters, and I don't expect human-readable content to
	grow much beyond that number. For machine readable, it's not such an issue, but the versions
	of ReadLine that accept an XString as parameter (rather than returning one) make as few
	heap allocations as possible, and if used properly allow one to read many short or long lines 
	without needing to perform a single heap allocation. This proper usage model is described in 
	ReadLine().
	**/
	static const int ReadLineStaticBufferSize = 256;

	/** ReadLine returns an error if a line grows beyond this number.
	I think it is a reasonable compromise between detecting a bug and allowing outliers.
	**/
	static const int ReadLineMaximumLineLength = 8 * 1024 * 1024;

	/** Reads from the file until a 13 (CR), 10 (LF), or a 0 (null terminator) is reached.
	
	If a 13 is reached, we check whether the immediately following character is a 10. If it
	is a CR+LF pair, then we leave the file's seek position immediately following the LF. 
	If it is not a CR+LF pair, then the seek position is left immediately following the 
	single character (be it a single CR (rare, old Mac OS up to 9) or a single LF.
	In both cases, the newline characters will be left intact at the end of the string.
	If the file has no newline characters at the end, then the last line returned will
	have none of these.
	In case you have not copped it, the idea is to return the data
	verbatim, so that even if one was using ReadLine (and the file did not contain
	any zeroes), you could reconstruct the file perfectly by summing all the returned lines
	together.

	Upon error, the ReadLine functions return false (or set the passed-in 'error' value to true, 
	as the case may be) upon an error. They do not touch the file's internal error flag, unless implicitly
	so, by attempting to read string data. If any error occurs, the file's seek pointer is reset
	to its initial value when the function began.

	**/
	XStringA ReadLineA( bool* error = NULL )
	{
		XStringA str;
		bool ok = TReadLine<char>( this, str );
		if ( error ) *error = ok;
		return str;
	}

	/// Wide-String version of ReadLineA. 
	XStringW ReadLineW( bool* error = NULL )
	{
		XStringW str;
		bool ok = TReadLine<wchar_t>( this, str );
		if ( error ) *error = ok;
		return str;
	}

	/** Performance oriented ReadLine.
	This class of functions, which take an XString as a ref parameter, are geared for performance
	because they allow iteration through many lines, while touching the heap as seldom as possible.
	Unless an error is encountered, the string's internal buffer will never be reduced. In order
	to use them most efficiently, reuse the same XString object for every line.
	@sa ReadLineA
	**/
	template< typename TCH >
	bool ReadLine( XStringT<TCH>& str )
	{
		return TReadLine<TCH>( this, str );
	}

	/** Performance oriented ReadLine.
	@sa ReadLine
	**/
	bool ReadLineA( XStringA& str )
	{
		return TReadLine<char>( this, str );
	}

	/** Performance oriented ReadLine.
	@sa ReadLine
	**/
	bool ReadLineW( XStringW& str )
	{
		return TReadLine<wchar_t>( this, str );
	}

	/** Read in chunks of no more than the size you specify.
	
	I created this when I noticed that WinXP 32 clients could not read a 77 MB chunk in one swoop from
	a new (Nov 2009) Fedora box at GLS.

	@return The total number of bytes read.
	**/
	size_t ReadChunked( void* data, size_t bytes, size_t chunkSize );

	/** Writes \a bytes from the given file at its current position.
	\return Number of bytes written.
	**/
	UINT64 Write( UINT64 bytes, IFile* src );

	/// Wrapper around IFile based Write() that does not get lost by the ridiculous C++ overload derivation rules.
	UINT64 WriteIFile( UINT64 bytes, IFile* src ) { return Write( bytes, src ); }

	/** Read \a bytes to the given file at its current position.
	\return Number of bytes read.
	**/
	UINT64 Read( UINT64 bytes, IFile* dst );

	/// Wrapper around IFile based Read() that does not get lost by the ridiculous C++ overload derivation rules.
	UINT64 ReadIFile( UINT64 bytes, IFile* dst ) { return Read( bytes, dst ); }

	/** Read from src and writes to dst.
	@return Number of bytes written.
	**/
	static UINT64 Copy( UINT64 bytes, IFile* src, IFile* dst );

	/** Read from src and writes to dst.
	@return Number of bytes written.
	**/
	static UINT64 Copy( UINT64 bytes, IFile* src, UINT64 srcPos, IFile* dst, UINT64 dstPos );

	/** Compares two streams.
	@return Zero if identical, and 1 otherwise.
	**/
	static int Compare( UINT64 bytes, IFile* src1, IFile* src2 );

	/// This need not be used.
	XStringW FileName;

protected:

	template< typename TPOD >
	TPOD ReadPOD( bool peek = false ) 
	{
		TPOD tv;
		INT64 br = Read( &tv, sizeof(tv) );
		if (peek) SeekRel( -br );
		return tv;
	}

	/** Find the length of the string, seeking a line terminator.
	Status indicators:
		# -1: No line terminator was found.
		# -2: A null (0) was encountered.
		# -3: Need more data. This is only returned if the buffer ends with a CR (because there could be a sibling LF following it).
	@return The length (up to an including the null terminator), or an error status.
	**/
	template< typename TCH >
	static int TFindLineLength( const TCH* buff, size_t buffSize )
	{
		// remember that our resulting string includes the CR, LF, or CR+LF.
		const int LF = 10;
		const int CR = 13;
		for ( size_t i = 0; i < buffSize; i++ )
		{
			if ( buff[i] == CR )
			{
				// If CR is on last character, then we need more data.
				if ( i == buffSize - 1 ) return -3;
				if ( buff[i+1] == LF )	return int(i + 2);
				else					return int(i + 1);
			}
			else if ( buff[i] == LF )
			{
				return int(i + 1);
			}
			else if ( buff[i] == 0 )
			{
				return -2;
			}
		}
		return -1;
	}

	template< typename TCH >
	static bool TReadLine( IFile* file, XStringT<TCH>& str )
	{
		const int LSIZE = ReadLineStaticBufferSize;
		const int LF = 10;
		const int CR = 13;
		TCH local[ LSIZE + 1 ]; // the extra 1 is for an LF that we stuff in at $001

		INT64 spos = file->Position();
		INT64 flen = file->Length();
		INT32 grab = (INT32) _min_( LSIZE, (flen - spos) / sizeof(TCH) );
		
		// this doesn't make sense. you are not reading on the edge of a symbol.
		if ( spos % sizeof(TCH) != 0 ) { str.Clear(); return false; }

		// at end of file
		if ( grab <= 0 ) { str.Clear(); return false; }

		size_t bread1 = file->Read( local, grab * sizeof(TCH) );

		// add explicit null terminator
		local[grab] = 0;
		
		// this is a file read error
		if ( bread1 != grab * sizeof(TCH) ) { str.Clear(); return false; }

		INT64 pos = spos + grab * sizeof(TCH);

		// first pass: see whether we have a line terminator within the first LSIZE chars.
		// remember that our resulting string includes the CR, LF, or CR+LF.
		int length = TFindLineLength( local, LSIZE );

		if ( length == -3 )
		{
			// $001: ends on CR. peek to see if next is LF. 
			// This is probably premature to optimize the case here where one has EXACTLY a record of LSIZE or LSIZE+1 bytes, 
			// but I couldn't stand not doing it.
			TCH nn = file->ReadPOD<TCH>( true );
			if ( nn == LF )
			{ 
				// local is actually LSIZE + 1 bytes big, especially for this little case.
				file->SeekRel(sizeof(TCH));
				local[LSIZE] = LF;
				length = LSIZE + 1;
			}
			else
			{
				length = LSIZE;
			}
		}

		if ( length == -2 )
		{
			// null terminator.
			if ( pos == flen )
			{
				// end of file
				str.SetNoShrink( local, grab );
				return true;
			}
			else
			{
				// seek back and return false.
				file->Seek( spos );
				str.Clear();
				return false;
			}
		}

		// no terminator found, but we are at the end of the file, so this is by definition the last line
		if ( length == -1 && grab < LSIZE )
		{
			length = grab;
		}

		if ( length > 0 )
		{
			// leave seek position immediately after terminator
			file->Seek( spos + length * sizeof(TCH) );
			str.SetNoShrink( local, length );
			return true;
		}

		// second pass: use memory in str as temporary space. This is a heavy optimization, and is an attempt to reduce
		// the number of heap allocs when reading strings longer than 512 characters.
		
		// alloc for string, and fill up the first LSIZE chars
		if ( str.GetCapacity() < LSIZE + 1 )
			str.AllocLL( LSIZE * 4 + 1 );

		memcpy( str.GetRawBuffer(), local, LSIZE * sizeof(TCH) );
		length = LSIZE;

		INT32 nextchunk = LSIZE * 2;

		bool err = false;

		while ( true )
		{
			INT32 read = nextchunk;
			read = (INT32) _min_( read, (flen - pos) / sizeof(TCH) );
			if ( read == 0 )
			{
				// nothing left. we must retire.
				break;
			}
			else
			{
				if ( length + read + 1 > str.GetCapacity() )
					str.Resize( length + read );
				size_t bread2 = file->Read( str.GetRawBuffer() + length, read * sizeof(TCH) );
				pos += read * sizeof(TCH);
				if ( bread2 != read * sizeof(TCH) )
				{
					// read error
					str.Clear();
					file->Seek( spos );
					return false;
				}

				// start counting 1 character behind the end, to avoid dealing with the complications of the CR+LF boundary problem $001.
				int start = length - 1;
				int tlen = TFindLineLength( str.GetRawBuffer() + start, read + 1 );
				if ( tlen == -2 )
				{
					// null terminator
					str.Clear();
					file->Seek( spos );
					return false;
				}
				else if ( tlen > 0 )
				{
					// bingo
					length = start + tlen;
					break;
				}
				else
				{
					// case -1 and -3 (none found and string ends on CR) can be dealt with in the next pass
					length += read;
				}
			}

			if ( length >= ReadLineMaximumLineLength )
			{
				str.Clear();
				file->Seek( spos );
				return false;
			}

			nextchunk *= 2;
		}

		str.GetRawBuffer()[length] = 0;
		str.ForceInternals( str.GetRawBuffer(), str.GetCapacity(), length );

		file->Seek( spos + length * sizeof(TCH) );

		return true;
	}

};

}

#endif
