#pragma once

#include "IO.h"
#include "../HashCrypt/CRC32.h"
#include "../HashCrypt/md5.h"
#include "../HashCrypt/sha1.h"
#include "../HashCrypt/sha2.h"

// Various hash utilities
class Hasher
{
public:
	static const int	MaxDigestBytes = 64;
	IAppendableHash*	Hash;

	Hasher( IAppendableHash* h = NULL )
	{
		Hash = h;
		if ( Hash ) Reset();
	}

	void Reset()
	{
		Hash->Reset(0);
	}

	void Append( const void* data, size_t bytes )
	{
		Hash->Append( data, bytes );
	}

	/** Reads the requested amount of bytes from the file (in chunks) and appends them to the digest.
	The file pointer is incremented by the requested number of bytes.
	**/
	void Append( AbCore::IFile* file, size_t bytes )
	{
		size_t chunk = 256 * 1024;
		chunk = min(chunk, bytes);
		void* buff = malloc( chunk ); 
		while ( bytes > 0 )
		{
			size_t eat = min( bytes, chunk );
			size_t read = file->Read( buff, eat );
			if ( read < eat ) 
			{ 
				// not enough data available in the stream.
				ASSERT(false); 
				if ( read == 0 ) break;
				else eat = read;
			}
			Append( buff, eat );
			bytes -= eat;
		}
		free( buff );
	}

	void Finish( void* digest )
	{
		Hash->Finish( digest );
	}

	/** Compute checksum on entire contents of file.
	@return True if successful, False on memory or file read error.
	**/
	static Panacea::IO::Error ComputeFile( IAppendableHash* hash, void* digest_output, XStringW filename )
	{
		AbCore::DiskFile df;
		if ( !df.Open( filename, L"rb" ) ) return Panacea::IO::ErrorReadFile;
		Panacea::IO::Error er = Compute( hash, digest_output, df.Length(), &df );
		df.Close();
		return er;
	}

	/** Compute checksum of file from current position until pos + bytes.
	The file's pointer is set back to the original when the function leaves.
	\return True if successful, False on memory or file read error.
	**/
	static Panacea::IO::Error Compute( IAppendableHash* hash, void* digest_output, size_t bytes, AbCore::IFile* file )
	{
		size_t avail = file->Length() - file->Position();
		if ( avail < bytes ) 
		{
			return Panacea::IO::ErrorNotEnoughData;
		}

		Panacea::IO::Error err = Panacea::IO::ErrorOk;

		size_t blockSize = 256 * 1024;
		blockSize = min(blockSize, bytes);
		void* tempBuff = malloc( blockSize );
		if ( tempBuff == NULL ) return Panacea::IO::ErrorMemory;

		INT64 originalFilePos = file->Position();

		while ( bytes > 0 )
		{
			size_t block = min( blockSize, bytes );
			size_t bytesRead = file->Read( tempBuff, block );
			if ( bytesRead != block )
			{
				err = Panacea::IO::ErrorReadFile;
				break;
			}
			hash->Append( tempBuff, block );
			bytes -= block;
		}
		hash->Finish( digest_output );
		free( tempBuff );
		file->Seek( originalFilePos );

		return err;
	}


	/// Computes the digest for input_data and returns a digest record
	//static DigestRecord Compute( const void* input_data, size_t bytes )
	//{
	//	DigestRecord rec;
	//	Compute( &rec, input_data, bytes );
	//	return rec;
	//}

	/// Computes the digest for input_data and places it in digest_output
	static void Compute( IAppendableHash* hash, void* digest_output, const void* input_data, size_t bytes )
	{
		hash->Reset(0);
		hash->Append( input_data, bytes );
		hash->Finish( digest_output );
	}

	/// Returns true if message's digest matches digest_check
	static bool Check( IAppendableHash* hash, const void* digest_check, const void* input_data, size_t bytes )
	{
		u8 dig[MaxDigestBytes];
		Compute( hash, dig, input_data, bytes );
		return CryptoMemCmpEq( dig, digest_check, hash->DigestBytes() ) == 0;
	}

	/** Checks the digest from the file's current position until pos + bytes.
	\return ErrorOk if checksums match, ErrorChecksumFail if they don't match, or another error if we fail to compute the checksum for the file.
	**/
	static Panacea::IO::Error Check( IAppendableHash* hash, const void* digest_check, size_t bytes, AbCore::IFile* file )
	{
		u8 dig[MaxDigestBytes];
		Panacea::IO::Error e = Compute( hash, dig, bytes, file );
		if ( e == Panacea::IO::ErrorOk )
			return CryptoMemCmpEq( dig, digest_check, hash->DigestBytes() ) ? Panacea::IO::ErrorOk : Panacea::IO::ErrorChecksumFail;
		else
			return e;
	}

};

// statically typed
template< typename THash >
class THasher
{
public:
	THash	F;
	Hasher	H;

	THasher()
	{
		H = Hasher(&F);
	}

	void Reset()											{ H.Reset(); }
	void Append( const void* data, size_t bytes )			{ H.Append(data, bytes); }
	void Append( AbCore::IFile* file, size_t bytes )		{ H.Append(file, bytes); }
	void Finish( void* digest )								{ H.Finish(digest); }
	
	template<typename T> void TAppend( const T& t )			{ H.Append(&t, sizeof(t)); }

	static Panacea::IO::Error ComputeFile( void* digest_output, XStringW filename )						{ THasher s; return Hasher::ComputeFile( &s.F, digest_output, filename ); }
	static Panacea::IO::Error Compute( void* digest_output, size_t bytes, AbCore::IFile* file )			{ THasher s; return Hasher::Compute( &s.F, digest_output, bytes, file ); }
	static void Compute( void* digest_output, const void* input_data, size_t bytes )					{ THasher s; return Hasher::Compute( &s.F, digest_output, input_data, bytes ); }
	static bool Check( const void* digest_check, const void* input_data, size_t bytes )					{ THasher s; return Hasher::Check( &s.F, digest_check, input_data, bytes ); }
	static Panacea::IO::Error Check( const void* digest_check, size_t bytes, AbCore::IFile* file )		{ THasher s; return Hasher::Check( &s.F, digest_check, bytes, file ); }

};

// appendable
typedef THasher< AbCore::MD5 >				HashMD5;
typedef THasher< AbCore::SHA1 >				HashSHA1;
typedef THasher< AbCore::SHA256 >			HashSHA256;
typedef THasher< AbCore::CRC32 >			HashCRC32;
typedef THasher< AbCore::Adler32 >			HashAdler32;

// non appendable
typedef THasher< NonAppendableHashWrapper<AbCore::Murmur2> >			HashMurmur2;
typedef THasher< NonAppendableHashWrapper<AbCore::Murmur3_x86_32> >		HashMurmur3_x86_32;
typedef THasher< NonAppendableHashWrapper<AbCore::Murmur3_x86_128> >	HashMurmur3_x86_128;

