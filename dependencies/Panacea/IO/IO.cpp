#include "pch.h"
#include "IO.h"
#include "BufferedFile.h"
#include "Checksum.h"

namespace Panacea
{
	namespace IO
	{

		PAPI const char* Describe( Error er )
		{
			switch ( er )
			{
			case ErrorOk: return "OK";
			case ErrorReadFile: return "An error occured reading from a file";
			case ErrorWriteFile: return "An error occured writing to a file";
			case ErrorZLib: return "An generic error occured with ZLib ";
			case ErrorMemory: return "Memory could not be allocated";
			case ErrorData: return "The data stream is corrupt (decompression error only)";
			case ErrorFile: return "A generic file error (could be read or write).";
			case ErrorChecksumFail: return "A checksum failed";
			case ErrorNotEnoughData: return "Not enough data was provided.";
			case ErrorTooMuchData: return "Too much data was provided.";
			case ErrorOutputBufferTooSmall: return "Output buffer is too small";

			case ErrorZipArchiveOpen: return "Cannot open the zip archive";
			case ErrorZipFileNotExist: return "A file within the zip archive does not exist";
			case ErrorZipUnknown: return "Unspecified error with the zip archive";

			case ErrorCreateFile: return "Error creating file";
			case ErrorOpenFile: return "Error opening file";
			case ErrorCreateDir: return "Error creating directory";

			default: ASSERT(false); return "unknown error";
			}
		}

		bool PAPI WriteWholeFile( const XStringW& filename, const void* buf, size_t bytes )
		{
			AbCore::DiskFile df;
			if ( !df.Open( filename, L"wb" ) ) return false;
			if ( df.Write( buf, bytes ) != bytes ) return false;
			df.Close();
			return true;
		}

		bool ReadWholeFile( FILE* file, podvec<byte>& bytes )
		{
			byte	buf[1024];
			size_t	nread = 0;

			while ( 0 != (nread = fread( buf, 1, sizeof(buf), file )) )
				bytes.addn( buf, nread );

			return !ferror(file);
		}

		bool ReadWholeFile_Gen( const XStringW& filename, dvect<BYTE>& bytes, int shareMode )
		{
			AbCore::DiskFile df;
			if ( !df.Open( filename, L"rb", shareMode ) ) return false;
			UINT64 len = df.Length();

			// 1 gig
			if ( len + (UINT64) bytes.size() > 0x40000000 )
			{
				AbcTrace( "ReadWholeFile_Gen refuses to read files larger than 1 GB\n" );
				return false;
			}

			size_t orgSize = bytes.size();
			bytes.resize( bytes.size() + len );

			if ( df.Read( &bytes[0] + orgSize, len ) != len ) return false;

			df.Close();

			return true;
		}

		template<typename TVec>
		bool PAPI TReadWholeFileLines( const XStringW& filename, TVec& lines, uint flags )
		{
			AbCore::DiskFile df;
			XStringA trims = "\x09\x20\x0a\x0d";
			if ( df.Open( filename, L"rb", AbCore::IFile::ShareRead ) )
			{
				BufferedFile bf(&df);
				XStringA line;
				while ( bf.ReadLineA( line ) )
				{
					if ( flags & ReadLineFlagTrimWhitespace )
						line.Trim( trims );
					if ( (flags & ReadLineFlagSkipEmptyLines) && line.IsEmpty() )
						continue;
					lines += line;
				}
				return true;
			}
			return false;
		}

		bool PAPI ReadWholeFileLines( const XStringW& filename, dvect<XStringA>& lines, uint flags )
		{
			return TReadWholeFileLines( filename, lines, flags );
		}

		bool PAPI ReadWholeFileLines( const XStringW& filename, podvec<XStringA>& lines, uint flags )
		{
			return TReadWholeFileLines( filename, lines, flags );
		}

		bool PAPI ReadWholeFile_Share( const XStringW& filename, dvect<BYTE>& bytes, int diskFileShareMode )
		{
			return ReadWholeFile_Gen( filename, bytes, diskFileShareMode );
		}

		bool PAPI ReadWholeFile( const XStringW& filename, dvect<BYTE>& bytes )
		{
			return ReadWholeFile_Gen( filename, bytes, AbCore::IFile::ShareRead );
		}

		bool PAPI ReadWholeFile( const XStringW& filename, XStringA& str )
		{
			dvect<BYTE> buf;
			if ( !ReadWholeFile( filename, buf ) ) return false;
			str.SetExact( (const char*) &buf[0], buf.size() );
			return true;
		}

		bool PAPI WriteWholeFile( const XStringW& filename, const XStringA& str )
		{
			return WriteWholeFile( filename, (const char*) str, str.Length() );
		}

		bool PAPI GetGlobalExeOption( const char* option, const char* override_app_name )
		{
			wchar_t mp[MAX_PATH];
			GetModuleFileName( NULL, mp, MAX_PATH );
			int len = (int) wcslen(mp);
			if ( len < 5 ) return false;
			XString base;
			if ( override_app_name )
			{
				PathRemoveFileSpec( mp );
				base = mp;
				base += L"\\";
				base += XStringW::FromUtf8( override_app_name );
			}
			else
			{
				mp[len - 4] = 0;
				base = mp;
			}
			return Path::FileExists( base + L".Options." + XStringW(option), false );
		}

		// access time is not reliable
		int CompareLastAccessed( const IO::FileFindItem& a, const IO::FileFindItem& b ) { return a.DateAccess.Compare(b.DateAccess); }
		int CompareLastModified( const IO::FileFindItem& a, const IO::FileFindItem& b ) { return a.DateModify.Compare(b.DateModify); }
		int CompareFileNameLengthAsc( const IO::FileFindItem& a, const IO::FileFindItem& b ) { return a.Name.Length() - b.Name.Length(); }

		void PAPI CacheCleanup( XString root, const XString& wildcard, int64 maxCacheBytes, int64 cleanIntervalSeconds, int flags )
		{
			bool purge_all = !!(flags & CacheCleanupPurgeAll);
			bool recursive = !!(flags & CacheCleanupRecursive);
			bool force_run = !!(flags & CacheCleanupForceRunNow);

			Sys::Date now = Sys::Date::Now();

			if ( root.Length() < 2 ) return;

			if ( root.EndsWith('/') ) root.Chop();
			if ( !root.EndsWith('\\') ) root += '\\';
			XString cleanup_marker = root + L"Cache_Last_Cleanup";
			
			bool run = false;
			if (	force_run ||
						purge_all ||
						!Path::FileExists(cleanup_marker) ||
						now.SecondsSince(Path::GetFileWriteTime(cleanup_marker)) > (double) cleanIntervalSeconds )
				run = true;

			if ( !run ) return;

			WriteWholeFile( cleanup_marker, "The last modified date of this file indicates when this cache was last cleaned" );
			Path::TouchFile( cleanup_marker );

			pvect<FileFindItem*> all;
			int ff_flags = FindFullPath;
			if ( recursive ) ff_flags |= FindRecursiveOnAllDirectories;
			Panacea::IO::Path::FindFiles( root + wildcard, all, ff_flags );

			int64 size = 0;
			for ( int i = 0; i < all.size(); i++ )
			{
				if ( !all[i]->Directory )
					size += all[i]->Size;
			}

			if ( size > maxCacheBytes || purge_all )
			{
				// Extremely simple policy: evict least recently used.
				sort( all, &CompareLastModified );

				for ( int i = 0; i < all.size() && (size > maxCacheBytes || purge_all); i++ )
				{
					if ( !all[i]->Directory )
					{
						if ( all[i]->Name == cleanup_marker ) continue;
						DeleteFile( all[i]->Name );
						size -= all[i]->Size;
					}
				}

				// Delete empty directories.
				if ( !!(flags & CacheCleanupRecursiveDeleteDirectories) )
				{
					sort( all, &CompareFileNameLengthAsc );
					reverse( all );

					for ( int i = 0; i < all.size(); i++ )
					{
						if ( all[i]->Directory )
						{
							// rely on the OS to not delete a non-empty directory
							RemoveDirectory( all[i]->Name );
						}
					}
				}
			}
			delete_all( all );
		}

		void PAPI DirectoryHash( XString root, DataSig20& hash )
		{
			HashSHA1 h;

			pvect<FileFindItem*> all;
			int ff_flags = FindRecursiveOnAllDirectories;
			Panacea::IO::Path::FindFiles( root + L"*", all, ff_flags );
			for ( intp i = 0; i < all.size(); i++ )
			{
				//h.Append( all[i]->Name.GetRawBuffer(), all[i]->Name.Length() * sizeof(wchar_t) );
				h.TAppend( all[i]->Size );
				h.TAppend( all[i]->DateModify.mUTC );
			}
			delete_all(all);

			h.Finish( hash.Bytes );
		}

	}
}

PAPI bool AbcIO_WriteWholeFile( const XString& filename, const void* buf, size_t bytes )
{
	return Panacea::IO::WriteWholeFile( filename, buf, bytes );
}

PAPI bool AbcIO_ReadWholeFile( const XString& filename, XStringA& str )
{
	return Panacea::IO::ReadWholeFile( filename, str );
}
