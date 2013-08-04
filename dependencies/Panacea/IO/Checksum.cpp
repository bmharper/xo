#include "pch.h"
#include "Checksum.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace Panacea
{
	namespace IO
	{
		namespace Checksum
		{
#ifdef NEVER
			Error Adler32::ComputeFile( void* digest_output, XStringW filename )
			{
				AbCore::DiskFile df;
				if ( !df.Open( filename, L"rb" ) ) return ErrorReadFile;
				Error er = Compute( digest_output, df.Length(), &df );
				df.Close();
				return er;
			}

			Error Adler32::Compute( void* digest_output, size_t bytes, AbCore::IFile* file )
			{
				size_t avail = file->Length() - file->Position();
				if ( avail < bytes ) 
				{
					return ErrorNotEnoughData;
				}

				Error err = ErrorNone;

				size_t blockSize = 64 * 1024;
				void* tempBuff = malloc( blockSize );
				if ( tempBuff == NULL ) return ErrorMemory;

				INT64 originalFilePos = file->Position();

				UINT32 q = adler32( 0, NULL, 0 );

				while ( bytes > 0 )
				{
					size_t block = min( blockSize, bytes );
					size_t bytesRead = file->Read( tempBuff, block );
					if ( bytesRead != block )
					{
						err = ErrorReadFile;
						break;
					}
					q = adler32( q, (const Bytef*) tempBuff, (uInt) bytesRead );
					bytes -= block;
				}

				UINT32* digOut = (UINT32*) digest_output;
				*digOut = q;
				free( tempBuff );
				file->Seek( originalFilePos );

				return err;
			}
#endif


		}
	}
}
