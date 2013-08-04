#include "pch.h"
#include "Compression.h"

// These are ripped straight from BaseStd.h. I can't figure out the proper place to get BaseStd.h included from.
#define MAXSIZE_T   ((SIZE_T)~((SIZE_T)0))
#define MAXSSIZE_T  ((SSIZE_T)(MAXSIZE_T >> 1))
#define MINSSIZE_T  ((SSIZE_T)~MAXSSIZE_T)


// We export fastlz. The exports are explicitly given in panacea.def
#include "fastlz.h"

#include <zlib/contrib/minizip/zip.h>
#include <zlib/contrib/minizip/unzip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace Panacea
{
	namespace IO
	{
		namespace Compression
		{
			const int ZipCaseSense = 0; // Use OS Default

			ZipFile::~ZipFile()
			{
				Close();
			}

			void ZipFile::Close()
			{
				WriteClose();
				if ( HUnz ) unzClose( HUnz );
				HUnz = NULL;
				FLName.clear();
				FLSize.clear();
				FLDate.clear();
			}

			bool ZipFile::ReadOpen( XStringW zipFilePath )
			{
				Close();
				HUnz = unzOpen( zipFilePath.ToUtf8() );
				
				if ( HUnz )
				{
					if ( !EnumContent() ) { Close(); return false; }
					return true;
				}
				return false;
			}

			int ZipFile::ReadCount()
			{
				return FLName.size();
			}

			XStringA ZipFile::ReadName( int index )
			{
				return FLName[index];
			}

			UINT32 ZipFile::ReadSize( int index )
			{
				return (UINT32) FLSize[index];
			}

			Panacea::Sys::Date ZipFile::ReadDate( int index )
			{
				return FLDate[index];
			}

			int ZipFile::ReadIndexOf( const XStringA& fileName )
			{
				return FindIndex( fileName );
			}

			int ZipFile::FindIndex( const XStringA& name )
			{
				return FLName.find( name );
			}

			Error ZipFile::ReadRead( XStringA fileName, AbCore::IFile* outFile, void* cbContext, ZipCallback callback )
			{
				bool ok = false;
				bool crcFail = false;
				if ( unzLocateFile( HUnz, fileName, ZipCaseSense ) == UNZ_OK )
				{
					UINT32 totalBytes = ReadSize( FindIndex(fileName) );
					if ( unzOpenCurrentFile( HUnz ) == UNZ_OK )
					{
						void* buf = NULL;
						ok = true;
						UINT32 bytesRemain = totalBytes;
						while ( bytesRemain > 0 && ok )
						{
							UINT32 doBytes = min(ChunkSize, bytesRemain);
							if ( !buf ) buf = malloc( doBytes );
							int unzRes = unzReadCurrentFile( HUnz, buf, doBytes );
							if ( unzRes == UNZ_CRCERROR )
								crcFail = true;
							ok = unzRes == doBytes;
							if ( ok )
							{
								ok = outFile->Write( buf, doBytes ) == doBytes;
								if ( callback ) callback( cbContext, fileName.ToWide(), totalBytes - bytesRemain, totalBytes );
							}
							bytesRemain -= doBytes;
						}
						free( buf );
						unzCloseCurrentFile( HUnz );
					}
					else return ErrorZipUnknown;
				}
				else return ErrorZipFileNotExist;

				if ( crcFail )
					return ErrorChecksumFail;
				else
					return ok ? ErrorOk : ErrorZipUnknown;
			}

			Error ZipFile::ReadRead( XStringA fileName, void* buf )
			{
				// We assume that you have allocated a buffer large enough
				AbCore::MemFile mf( buf, _UI32_MAX, true );
				return ReadRead( fileName, &mf );
			}

			bool ZipFile::EnumContent()
			{
				if ( unzGoToFirstFile( HUnz ) != UNZ_OK ) return false;
				while ( true )
				{
					unz_file_info inf;
					char nameBuf[256];
					memset( nameBuf, 0, sizeof(nameBuf) );
					if ( unzGetCurrentFileInfo( HUnz, &inf, nameBuf, sizeof(nameBuf), NULL, 0, NULL, 0 ) != UNZ_OK ) return false;
					if ( nameBuf[0] == 0 ) return false;;
					FLName += nameBuf;
					FLSize += inf.uncompressed_size;
					FLDate += Panacea::Sys::Date(	inf.tmu_date.tm_year, inf.tmu_date.tm_mon, inf.tmu_date.tm_mday, 
													inf.tmu_date.tm_hour, inf.tmu_date.tm_min, inf.tmu_date.tm_sec );
					int e = unzGoToNextFile( HUnz );
					if ( e == UNZ_END_OF_LIST_OF_FILE ) break;
					else if ( e != UNZ_OK ) return false;
				}
				return true;
			}


			bool ZipFile::WriteClose()
			{
				int ok = zipClose( HZip, "" );
				HZip = NULL;
				return ok == ZIP_OK;
			}

			bool ZipFile::WriteOpen( XStringW zipFilePath )
			{
				Close();
				HZip = zipOpen( zipFilePath.ToUtf8(), APPEND_STATUS_CREATE );
				return HZip != NULL;
			}

			bool ZipFile::WriteWrite( XStringA fileName, UINT32 bytes, const void* buf, Panacea::Sys::Date* date )
			{
				bool ok = false;
				
				zip_fileinfo inf;
				memset( &inf, 0, sizeof(inf) );
				inf.dosDate = 0;
				inf.internal_fa = 0;
				inf.external_fa = 0;
				
				Panacea::Sys::Date ddate = Panacea::Sys::Date::Now();
				if ( date ) ddate = *date;
				inf.tmz_date.tm_year = ddate.Year( true );
				inf.tmz_date.tm_mon = ddate.Month( true );
				inf.tmz_date.tm_mday = ddate.DayOfMonth( true );
				inf.tmz_date.tm_hour = ddate.HoursAfterMidnight( true );
				inf.tmz_date.tm_min = ddate.MinutesAfterHour( true );
				inf.tmz_date.tm_sec = ddate.SecondsAfterMinute( true );

				if ( zipOpenNewFileInZip( HZip, fileName, &inf, NULL, 0, NULL, 0, NULL, Z_DEFLATED, CompressLevel ) == ZIP_OK )
				{
					ok = zipWriteInFileInZip( HZip, buf, bytes ) == ZIP_OK;
					int ce = zipCloseFileInZip( HZip );
					ok = ok && (ce == ZIP_OK);
				}
				return ok;
			}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


			Error ZLib::Compress( size_t input_size, const void* input, dvect<BYTE>& output, int level_0_to_9 )
			{
				if ( output.size() == 0 ) output.resize( (DVEC_INT) CompressBound(input_size) );

				size_t cbytes = output.capacity;
				Error er = Compress( input_size, input, cbytes, output.data, level_0_to_9 );
				if ( er == ErrorOutputBufferTooSmall )
				{
					output.resize( (DVEC_INT) CompressBound(input_size) );
					cbytes = output.capacity;
					er = Compress( input_size, input, cbytes, output.data, level_0_to_9 );
					ASSERT( er != ErrorOutputBufferTooSmall );
				}
				output.count = (DVEC_INT) cbytes;
				return er;
			}

			Error ZLib::Decompress( size_t input_size, const void* input, dvect<BYTE>& output )
			{
				return Decompress( input_size, input, output.count, output.data );

				// Screw it.. this is too ugly.

				/*
				if ( output.size() == 0 ) output.resize( input_size * 5 );

				Error er = ErrorNone;
				while ( true )
				{
					size_t osize = output.capacity;
					er = Decompress( input_size, input, osize, output.data );
					if ( er != ErrorOutputBufferTooSmall )
					{
						if ( er == ErrorNone )
							output.count = osize;
						break;
					}
					size_t nsize = max( osize * 2, input_size + 2 );
					output.resize( nsize );
				}
				
				if ( er != ErrorNone ) output.count = 0;
				return er;
				*/
			}

			size_t ZLib::CompressBound( size_t inBytes )
			{
				return compressBound( (uLong) inBytes );
			}

			bool ZLib::GetBuffers( size_t desired_bytes, size_t minimum_bytes, size_t& actual_size, void*& buffer1, void*& buffer2 )
			{
				for ( ; desired_bytes >= minimum_bytes; desired_bytes /= 2 )
				{
					buffer1 = malloc( desired_bytes );
					buffer2 = malloc( desired_bytes );
					if ( buffer1 != NULL && buffer2 != NULL ) break;
					if ( buffer1 != NULL ) free( buffer1 );
					if ( buffer2 != NULL ) free( buffer2 );
					buffer1 = buffer2 = NULL;
				}
				if ( buffer1 == NULL || buffer2 == NULL )
				{
					actual_size = 0;
					return false;
				}
				else
				{
					ASSERT( buffer1 != NULL && buffer2 != NULL );
					actual_size = desired_bytes;
					return true;
				}
			}

			void ZLib::FreeBuffers( void* buffer1, void* buffer2 )
			{
				if ( buffer1 != NULL ) free( buffer1 );
				if ( buffer2 != NULL ) free( buffer2 );
			}

			Error ZLib::Compress( size_t inBytes, AbCore::IFile* infile, AbCore::IFile* outfile, int level_1_to_9, size_t maxOutputBytes )
			{
				size_t memoryBlockSize = 256 * 1024;
				size_t minBlockSize = 4096;

				void *tempBuffIn, *tempBuffOut;
				if ( !GetBuffers( memoryBlockSize, minBlockSize, memoryBlockSize, tempBuffIn, tempBuffOut ) )
					return ErrorMemory;

				z_stream_s stream;
				int err = 0;

				stream.zalloc = (alloc_func) NULL;
				stream.zfree = (free_func) NULL;
				stream.opaque = (voidpf) NULL;

				// NOTE: The HTTP server inside panacea assumes a 32k window size, which is what deflateInit gives us.
				// Should you ever change this initialization, then be sure to fix that code too. The prefix bytes will be different
				// if the window size is different.
				err = deflateInit( &stream, level_1_to_9 );

				if ( err != Z_OK ) 
				{
					FreeBuffers( tempBuffIn, tempBuffOut );
					return ErrorZLib;
				}

				bool outOfOutputSpace = false;
				bool readError = false;
				bool writeError = false;
				int defErr = Z_OK;
				size_t bytesRead = 0;
				size_t bytesWritten = 0;

				// compute the amount of bytes we want to read
				size_t maxAvailableBytes = infile->Length() - infile->Position();
				if ( inBytes > maxAvailableBytes )
					ASSERT( false );
				inBytes = min( inBytes, maxAvailableBytes );
				size_t infileLimit = (size_t) (infile->Position() + maxAvailableBytes);
				
				bool finished = false;

				while ( true )
				{
					// compute next input block size
					// (make sure we always have enough output space for any input)
					size_t insize = (size_t) (memoryBlockSize * 0.99) - 100;
					size_t inavailable = (size_t) (infileLimit - infile->Position());
					if ( inavailable < insize )
						insize = inavailable;

					if ( insize == 0 && finished )
						break;

					bool lastpass = false;

					if ( insize > 0 )
					{
						// read and make sure we can read the right amount
						size_t infileRead = infile->Read( tempBuffIn, insize );
						ASSERT( infileRead == insize );
						bytesRead += infileRead;
						if ( infileRead != insize )
						{
							readError = true;
							break;
						}

						stream.avail_in = (uInt) insize;
						stream.next_in = (Bytef*) tempBuffIn;
					}
					else
					{
						lastpass = true;
					}

					while ( stream.avail_in > 0 || (lastpass && !finished) )
					{
						stream.next_out = (Bytef*) tempBuffOut;
						size_t estimatedOut = memoryBlockSize;
						stream.avail_out = (uInt) estimatedOut;

						if ( insize > 0 )
						{
							defErr = deflate( &stream, Z_NO_FLUSH );
						}
						else
						{
							defErr = deflate( &stream, Z_FINISH );
							if ( stream.avail_in == 0 ) 
								finished = true;
						}

						if ( stream.avail_in > 0 )
							int debug_testing = 1;

						if ( defErr != Z_OK && defErr != Z_STREAM_END )
							break;

						size_t bytesToOutput = estimatedOut - stream.avail_out;
						if ( bytesToOutput > maxOutputBytes )
						{
							outOfOutputSpace = true;
							break;
						}
						maxOutputBytes -= bytesToOutput;

						size_t outBytes = outfile->Write( tempBuffOut, bytesToOutput );
						ASSERT( outBytes == estimatedOut - stream.avail_out );
						bytesWritten += outBytes;
						if ( outBytes < estimatedOut - stream.avail_out )
						{
							writeError = true;
							break;
						}
					}
				}

				FreeBuffers( tempBuffIn, tempBuffOut );

				int finishErr = deflateEnd( &stream );
				ASSERT( finishErr == Z_OK );

				if ( outOfOutputSpace )								return ErrorOutputBufferTooSmall;
				if ( finishErr != Z_OK )							return ErrorZLib;
				if ( defErr != Z_OK && defErr != Z_STREAM_END )		return ErrorZLib;
				if ( readError )									return ErrorReadFile;
				if ( writeError )									return ErrorWriteFile;

				return ErrorOk;
			}

			Error ZLib::Compress( size_t input_size, const void* input, size_t& output_size, void* output, int level_0_to_9 )
			{
				if ( output_size > (size_t) MAXSSIZE_T )
				{
					ASSERT( false );
					return ErrorNotEnoughData;
				}

				/* -- This is no longer valid, since Compress() that takes a dvect<BYTE> uses the return flag of ErrorOutputBufferTooSmall to grow the output buffer as needed
				if ( output_size < input_size ) 
				{
					// output_size needs to be set to the size of the output buffer.
					// although the second restriction (os < is) is not always necessarily necessary,
					// I can't think of a reasonable time when it would be worthwhile to assume otherwise.
					ASSERT( false );
					return ErrorNotEnoughData;
				}
				*/

				AbCore::MemFile fileIn( const_cast<void*> (input), input_size, true );
				AbCore::MemFile fileOut( output, output_size, true );
				
				Error st = Compress( input_size, &fileIn, &fileOut, level_0_to_9, output_size );
				output_size = fileOut.Position();
				return st;
			}

			Error ZLib::Decompress( size_t input_size, const void* input, size_t output_size, void* output )
			{
				if ( output_size > (size_t) MAXSSIZE_T || (output_size == 0 && input_size != 0) ) 
				{
					// output_size needs to be set before using this function
					ASSERT( false );
					return ErrorNotEnoughData;
				}

				AbCore::MemFile fileIn( const_cast<void*> (input), input_size, true );
				AbCore::MemFile fileOut( output, output_size, true );
				
				return Decompress( input_size, &fileIn, &fileOut, output_size );
			}

			Error ZLib::Decompress( size_t inBytes, AbCore::IFile* infile, AbCore::IFile* outfile, size_t maxOutputBytes )
			{
				size_t blockSize = 256 * 1024;
				void *tempBuffIn, *tempBuffOut;
				if ( !GetBuffers( blockSize, 4096, blockSize, tempBuffIn, tempBuffOut ) )
					return ErrorMemory;

				z_stream_s stream;
				int err = 0;

				stream.zalloc = (alloc_func) NULL;
				stream.zfree = (free_func) NULL;
				stream.opaque = (voidpf) NULL;

				stream.next_in = NULL;
				stream.avail_in = 0;

				err = inflateInit( &stream );
				if ( err != Z_OK ) return ErrorZLib;
				
				size_t inAvailable = (size_t) (infile->Length() - infile->Position());
				if ( inBytes > inAvailable )
					ASSERT( false );
				size_t inLimit = (size_t) (infile->Position() + min( inBytes, inAvailable ));

				Error myError = ErrorOk;

				while ( true )
				{
					size_t inBytesLeft = (size_t) (inLimit - infile->Position());
					size_t bytesToRead = min( inBytesLeft, blockSize );
					if ( bytesToRead > 0 )
					{
						size_t actualBytesRead = infile->Read( tempBuffIn, bytesToRead );
						if ( actualBytesRead != bytesToRead )
						{
							myError = ErrorReadFile;
							break;
						}

						stream.avail_in = (uInt) actualBytesRead;
						stream.next_in = (Bytef*) tempBuffIn;
				
						while ( stream.avail_in > 0 )
						{
							stream.next_out = (Bytef*) tempBuffOut;
							stream.avail_out = (uInt) blockSize;

							err = inflate( &stream, Z_SYNC_FLUSH );

							if ( err != Z_OK && err != Z_STREAM_END )
							{
								myError = ErrorZLib;
								break;
							}

							size_t bytesToWrite = blockSize - stream.avail_out;
							if ( bytesToWrite > maxOutputBytes )
							{
								myError = ErrorOutputBufferTooSmall;
								break;
							}
							maxOutputBytes -= bytesToWrite;
							size_t writeBytes = outfile->Write( tempBuffOut, bytesToWrite );
							if ( writeBytes != bytesToWrite )
							{
								myError = ErrorWriteFile;
								break;
							}
							if ( stream.avail_in > 0 && err == Z_STREAM_END )
							{
								// too much data.
								myError = ErrorTooMuchData;
								break;
							}
						}
						if (	myError != ErrorOk ||
									err == Z_STREAM_END ) break;
					}
					else
					{
						break;
					}
				}

				if ( err == Z_STREAM_END ) 
					err = Z_OK;

				int endErr = inflateEnd( &stream );
				if ( err == Z_OK && endErr != Z_OK ) 
				{ 
					err = endErr;
					myError = ErrorZLib;
				}

				FreeBuffers( tempBuffIn, tempBuffOut );

				if ( myError == ErrorZLib || err != Z_OK )
					myError = Translate( err );

				return myError;
			}

			Error ZLib::Translate( UINT zlib_error )
			{
				switch ( zlib_error )
				{
				case Z_OK: return ErrorOk;
				case Z_STREAM_ERROR: return ErrorFile;
				case Z_DATA_ERROR: return ErrorData;
				case Z_MEM_ERROR: return ErrorMemory;
				default: return ErrorZLib;
				}
			}

			Error FastLZ::Compress( size_t input_size, const void* input, size_t& output_size, void* output, int level_1_to_2 )
			{
				//if ( input_size < 16 ) return ErrorNotEnoughData;
				output_size = fastlz_compress_level( level_1_to_2, input, (int) input_size, output );
				return ErrorOk;
			}

			Error FastLZ::Decompress( size_t input_size, const void* input, size_t output_size, void* output, size_t* output_bytes )
			{
				int bytes = fastlz_decompress( input, (int) input_size, output, (int) output_size );
				if ( output_bytes ) *output_bytes = bytes;
				return ErrorOk;
			}

			Error Snappy::Compress( size_t input_size, const void* input, size_t& output_size, void* output )
			{
				::snappy::RawCompress( (const char*) input, input_size, (char*) output, &output_size );
				return ErrorOk;
			}

			Error Snappy::Decompress( size_t input_size, const void* input, size_t output_size, void* output, size_t* output_bytes )
			{
				return ::snappy::RawUncompress( (const char*) input, input_size, (char*) output ) ? ErrorOk : ErrorSnappyDecompress;
			}

		}

	}
}
