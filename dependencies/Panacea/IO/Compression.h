#pragma once

#include "IO.h"
#include "../System/Date.h"
#include "../snappy/snappy.h"

namespace Panacea
{
	namespace IO
	{
		namespace Compression
		{

			typedef void (__cdecl *ZipCallback) ( void* context, LPCWSTR filename, uint32 bytesDone, uint32 bytesTotal );

			/** Zip File interface.
			
			This is not intended for speed, nor for large archives.

			Read Method:
			-# Call ReadOpen()
			-# Call ReadCount, ReadName, and ReadSize to enumerate the contents.
			-# Call ReadRead to read each file. You must read entire files atomically.
			-# Call ReadClose()

			Write Method:
			-# Call WriteOpen()
			-# Call WriteWrite for each file. You must write entire files atomically.
			-# Call WriteClose()

			**/
			class PAPI ZipFile
			{
			public:
				ZipFile()
				{
					HUnz = NULL;
					HZip = NULL;
					CompressLevel = Z_DEFAULT_COMPRESSION;
				}
				~ZipFile();

				/// Maximum size of chunks used when we read data. We always write whole files.
				static const UINT32 ChunkSize = 1024 * 1024;

				bool				ReadOpen( XStringW zipFilePath );
				int					ReadCount();
				int					ReadIndexOf( const XStringA& fileName );
				XStringA			ReadName( int index );
				UINT32				ReadSize( int index );
				Panacea::Sys::Date	ReadDate( int index );
				Error				ReadRead( XStringA fileName, void* buf );
				Error				ReadRead( XStringA fileName, AbCore::IFile* outFile, void* cbContext = NULL, ZipCallback callback = NULL );
				void				ReadClose() { Close(); }

				bool				WriteOpen( XStringW zipFilePath );
				bool				WriteClose();
				bool				WriteWrite( XStringA fileName, UINT32 bytes, const void* buf, Panacea::Sys::Date* date = NULL );

				/// Set Compression factor (0 = store, 9 = maximum)
				void				WriteSetCompression( int zero_to_nine )
				{
					if ( zero_to_nine == Z_DEFAULT_COMPRESSION )	CompressLevel = Z_DEFAULT_COMPRESSION;
					else											CompressLevel = CLAMP( zero_to_nine, 0, 9 );
				}

			protected:
				typedef void* unzFile;
				typedef void* zipFile;
				unzFile HUnz;
				zipFile HZip;
				int CompressLevel;

				dvect<XStringA> FLName;
				dvect<UINT32> FLSize;	///< Uncompressed size
				dvect<Panacea::Sys::Date> FLDate;
				int FindIndex( const XStringA& name );

				void Close();
				bool EnumContent();

			};

			class PAPI ZLib
			{
			public:

				/// Returns the maximum size of the compressed buffer, for a given uncompressed buffer
				static size_t CompressBound( size_t inBytes );

				/** Compresses \a infile to \a outfile.
				@param inBytes The number of bytes to read from the input file (from it's current position).
				@param infile The input file. The file is read from it's current position until 
				it's end or until \a inBytes bytes have been compressed.
				@param outfile The output file. The output file is written to starting at it's current position.
				@param level_1_to_9 Compression level. 0 is no compression. 9 is maximim.
				@param maxOutputBytes We will not write more than this. If we need to, we return ErrorOutputBufferTooSmall.
				@return ErrorNone if successful.
				**/
				static Error Compress( size_t inBytes, AbCore::IFile* infile, AbCore::IFile* outfile, int level_0_to_9, size_t maxOutputBytes = -1 );

				/** Decompresses bytes from the given file to the output file.
				@param inBytes The number of input bytes in the file to decompress (from the current position).
				@param infile The input file. Decompression starts at the current position and continues until inBytes have been
				decompressed or the end of the file has been reached.
				@param outfile The output file. The file is written to starting at the current position.
				@param maxOutputBytes We will not write more than this. If we need to, we return ErrorOutputBufferTooSmall.
				**/
				static Error Decompress( size_t inBytes, AbCore::IFile* infile, AbCore::IFile* outfile, size_t maxOutputBytes = -1 );

				/** Compresses the memory block.
				@param input_size The size (in bytes) of the input data.
				@param input The input data.
				@param output_size On input, must contain the size of the output buffer. On return, will contain the number of bytes places in the output buffer.
				@param output The output buffer. This must be allocated before calling Compress(). This must be at least (1.003 * input_size + 11) bytes.
				**/
				static Error Compress( size_t input_size, const void* input, size_t& output_size, void* output, int level_0_to_9 );

				/** Decompresses the memory block.
				@param input_size The size of the input buffer.
				@param input The input data.
				@param output_size The size of the output buffer.
				@param output The output data.
				The output buffer must be large enough to hold the decompressed data.
				No checks are made on this.
				**/
				static Error Decompress( size_t input_size, const void* input, size_t output_size, void* output );

				/// The output buffer does not need to be initialized.
				static Error Compress( size_t input_size, const void* input, dvect<BYTE>& output, int level_0_to_9 );
				
				/// The output buffer needs to be initialized to the exact target size. It's size will never be altered.
				static Error Decompress( size_t input_size, const void* input, dvect<BYTE>& output );

			protected:

				/** Acquires buffers.
				We attempt to get two buffers of the requested size. If this is not possible,
				then we halve the amount until it is possible, or upon dropping below minimum_size
				we return false.
				\param desired_bytes The initial desired number of bytes to allocate to each buffer.
				\param minimum_bytes The minimum number of bytes that we will allocate to each buffer.
				\param actual_size Upon return, contains the number of bytes we allocated. This will
					be zero on failure.
				\param buffer1 The first buffer. This will be null on failure.
				\param buffer2 The second buffer. This will be null on failure.
				\return True if successful, False otherwise.
				**/
				static bool GetBuffers( size_t desired_bytes, size_t minimum_bytes, size_t& actual_size, void*& buffer1, void*& buffer2 );

				/// Free buffers allocated with GetBuffers()
				static void FreeBuffers( void* buffer1, void* buffer2 );

				/// Translate zlib error to one of our own.
				static Error Translate( UINT zlib_error );
			};


			class PAPI FastLZ
			{
			public:

				/// Returns the maximum size of the compressed buffer, for a given uncompressed buffer
				static size_t CompressBound( size_t inBytes )
				{
					// We want to divide by 20, to get 0.05.
					// Actual: 66 + inBytes + (inBytes * ceil(64 / 20)) >> 6;
					return 66 + inBytes + ((inBytes * (80 / 20)) >> 6);
				}

				/** Compresses the memory block.
				@param input_size The size (in bytes) of the input data.
				@param input The input data.
				@param output_size On input, must contain the size of the output buffer. On return, will contain the number of bytes places in the output buffer.
				@param output The output buffer. This must be allocated before calling Compress(). This must be at least (1.05 * input_size + 66) bytes.
				**/
				static Error Compress( size_t input_size, const void* input, size_t& output_size, void* output, int level_1_to_2 );

				/** Decompresses the memory block.
				@param input_size The size of the input buffer.
				@param input The input data.
				@param output_size The size of the output buffer.
				@param output The output data.
				The output buffer must be large enough to hold the decompressed data.
				No checks are made on this.
				**/
				static Error Decompress( size_t input_size, const void* input, size_t output_size, void* output, size_t* output_bytes );

			};

			class PAPI Snappy
			{
			public:

				static size_t CompressBound( size_t inBytes )
				{
					return ::snappy::MaxCompressedLength( inBytes );
				}
				static Error Compress( size_t input_size, const void* input, size_t& output_size, void* output );
				static Error Decompress( size_t input_size, const void* input, size_t output_size, void* output, size_t* output_bytes );

			};

		}
	}
}
