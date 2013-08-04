#include "pch.h"
#include "AutoMemFile.h"
#include "Path.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace Panacea
{
	namespace IO
	{
		AutoMemFile::AutoMemFile( size_t initial )
		{
			SwitchFailed = false;
			DiskMode = false;
			MaximumMemory = 0;
			BaseFile = NULL;
			Reset( initial );
		}

		AutoMemFile::~AutoMemFile()
		{
			delete BaseFile; BaseFile = NULL;
		}

		bool AutoMemFile::Error()
		{
			if ( SwitchFailed ) return true;
			return BaseFile->Error();
		}

		XString AutoMemFile::ErrorMessage()
		{
			if ( SwitchFailed )
			{
				return "AutoMemFile: Failed to switch to disk mode";
			}
			return BaseFile->ErrorMessage();
		}

		void AutoMemFile::ClearError()
		{
			SwitchFailed = false;
			BaseFile->ClearError();
		}

		bool AutoMemFile::Flush( int flags )
		{
			return BaseFile->Flush( flags );
		}

		bool AutoMemFile::UsingMemFile()
		{
			return !DiskMode;
			//return dynamic_cast< AbCore::MemFile* > (BaseFile) != NULL;
		}

		void* AutoMemFile::GetMemFileData()
		{
			AbCore::MemFile* mf = dynamic_cast< AbCore::MemFile* > (BaseFile);
			if ( DiskMode || mf == NULL )
			{
				ASSERT(false);
				return NULL;
			}
			return mf->Data;
		}

		bool AutoMemFile::Reserve( size_t requiredBytes )
		{
			if ( Length() >= requiredBytes ) return true;

			INT64 orgPos = Position();
			int nothing = 0;
			Seek( requiredBytes - sizeof(nothing) );
			size_t written = Write( &nothing, sizeof(nothing) );
			Seek( orgPos );
			return written == sizeof(nothing);
		}

		void AutoMemFile::Reset( size_t desiredMemory )
		{
			delete BaseFile; BaseFile = NULL;
			AbCore::MemFile* memFile = new AbCore::MemFile;
			BaseFile = memFile;
			SwitchFailed = false;
			DiskMode = false;
			if ( desiredMemory > 0 )
			{
				memFile->Reset( desiredMemory );
				if ( memFile->Error() )
				{
					memFile->Reset(0);
					// let the fall-through happen when the user does a Write(). 
					// we don't want to invoke Reserve() here, because it actually changes the length of the file.
				}
			}
		}

		size_t AutoMemFile::Write( const void* data, size_t bytes )
		{
			INT64 orgSize = BaseFile->Length();
			if ( UsingMemFile() && !SwitchFailed && MaximumMemory > 0 && orgSize + bytes > MaximumMemory )
			{
				SwitchToDisk( orgSize );
			}

			size_t ob = BaseFile->Write( data, bytes );
			if ( ob != bytes && !SwitchFailed )
			{
				// We'd like the memory file to either write all or nothing.
				// I do make amends for in case it doesn't do that (viz orgSize, preserveBytes).
				ASSERT( ob == 0 ); 
				ASSERT( BaseFile->Error() );
				if ( !SwitchToDisk( orgSize ) ) 
				{
					return ob;
				}
				return BaseFile->Write( data, bytes );
			}
			else
			{
				return ob;
			}
		}

		bool AutoMemFile::SwitchToDisk( INT64 preserveBytes )
		{
			TRACE( "\nAutoMemFile: Switching to disk file\n" );
			XString tfile = Panacea::IO::Path::GetTempFile( "patm" );
			bool ok = false;
			if ( !tfile.IsEmpty() )
			{
				XString fname = tfile;
				AbCore::DiskFile *df = new AbCore::DiskFile();
				if ( df->Open( fname, _T("w+bDT") ) )
				{
					AbCore::MemFile *memfile = dynamic_cast< AbCore::MemFile* > (BaseFile);
					ASSERT( memfile != NULL );
					if ( memfile != NULL )
					{
						size_t written = df->Write( memfile->Data, preserveBytes );
						if ( written == preserveBytes )
						{
							delete BaseFile;
							BaseFile = df;
							DiskMode = true;
							ok = true;
						}
					}
				}
				if ( !ok ) delete df;
			}
			if ( SwitchFailed )
			{
				ASSERT(false);
				TRACE( "AutoMemFile switch failed\n" );
				//AfxMessageBox( "Switching failed" );
			}
			SwitchFailed = !ok;
			return ok;
		}

	}
}
