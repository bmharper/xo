#pragma once

#include "../Core/DataSig.h"

namespace Panacea
{
	namespace IO
	{
		/** File attributes.
		**/
		enum FileAttribute
		{
			FileAttributeNormal = 0,
			FileAttributeReadOnly = 1,
			FileAttributeSystem = 2,
			FileAttributeHidden = 4,
			FileAttributeArchive = 8,
			FileAttributeTemporary = 16,
			FileAttributeDirectory = 32,
			FileAttributeEncrypted = 64
		};

		/** Reasons why a file open failed. 
		**/
		enum OpenFailReason
		{
			OpenFailPermission = 1,
			OpenFailShare,
			OpenFailReadOnly,
			OpenFailDoesNotExist
		};

		/** Special system folders.
		**/
		enum SpecialFolder
		{
			SpecialFolderSystem = 1,		///< Windows/System32
			SpecialFolderCommonAppData,		///< Documents and Settings/All Users/Application Data
			SpecialFolderPersonal,			///< Documents and Settings/user/My Documents
			SpecialFolderUser,				///< Documents and Settings/user
			SpecialFolderUserAppData,		///< Documents and Settings/user/Application Data
			SpecialFolderLocalUserAppData,	///< Documents and Settings/user/Local Settings/Application Data
			SpecialFolderFonts				///< Windows/Fonts
		};

		static const int MaxPathLength = 4096;

		/** Error messages.
		These messages reflect the nature of the error as best as possible.
		If zlib returns an error that we understand, then we cast it in our
		format that best fits it.
		**/
		enum Error
		{
			ErrorOk = 0,
			ErrorReadFile = 1,
			ErrorWriteFile = 2,
			ErrorZLib = 3,							///< An generic error occured with ZLib 
			ErrorMemory = 4,						///< Memory could not be allocated
			ErrorData = 5,							///< The data stream is corrupt (decompression error only)
			ErrorFile = 6,							///< A generic file error (could be read or write).
			ErrorChecksumFail = 7,					///< A checksum failed
			ErrorNotEnoughData = 8,					///< Not enough data was provided.
			ErrorTooMuchData = 9,					///< Too much data was provided.
			ErrorOutputBufferTooSmall = 10,			///< Output buffer is too small
			ErrorSnappyDecompress = 11,				///< Generic fail from snappy
			
			ErrorZipArchiveOpen = 30,				///< Archive could not be opened
			ErrorZipFileNotExist = 31,				///< A file within the zip file does not exist
			ErrorZipUnknown = 32,					///< Unspecified error with ziplib

			ErrorCreateFile = 50,
			ErrorOpenFile = 51,
			ErrorCreateDir = 52,
		};
		PAPI const char* Describe( Error er );

		// Yes... the inconsistency here between dvect and podvec is horrible...........

		bool PAPI WriteWholeFile( const XStringW& filename, const void* buf, size_t bytes );
		bool PAPI ReadWholeFile( const XStringW& filename, dvect<BYTE>& bytes );
		bool PAPI ReadWholeFile_Share( const XStringW& filename, dvect<BYTE>& bytes, int diskFileShareMode );
		bool PAPI ReadWholeFile( FILE* file, podvec<byte>& bytes );

		/** Reads the entire file as 8-bit characters.
		Obviously, since XString cannot represent null characters, the resulting string will not extend beyond the first 0 byte in the file.
		The entire file will always be read though.
		**/
		bool PAPI ReadWholeFile( const XStringW& filename, XStringA& str );

		enum ReadLinesFlags
		{
			ReadLineFlagSkipEmptyLines = 1,	///< Does not include empty lines (tested for emptiness after trimming).
			ReadLineFlagTrimWhitespace = 2,	///< Trims characters 9, 32, 10, 13 from both ends
		};

		/// Read the entire file, splitting it line by line.
		bool PAPI ReadWholeFileLines( const XStringW& filename, dvect<XStringA>& lines, uint flags = 0 );
		bool PAPI ReadWholeFileLines( const XStringW& filename, podvec<XStringA>& lines, uint flags = 0 );

		/** Writes the string to a file.
		**/
		bool PAPI WriteWholeFile( const XStringW& filename, const XStringA& str );

		/** Looks for the presence of a file with the name "<ExecutingProgram>.Options.<option>".
		We use the path of the currently executing .exe to establish the directory in which to search, as well
		as the ExecutingProgram string. The ExecutingProgram string is the name of the exe file, minus the extension.
		override_app_name Can be used to override the exe's name. For instance, if you send in "PDF" for override_app_name, then
		the function will look for PDF.Options.option
		**/
		bool PAPI GetGlobalExeOption( const char* option, const char* override_app_name = NULL );

		enum CacheCleanupFlags
		{
			CacheCleanupRecursive = 1,
			CacheCleanupRecursiveDeleteDirectories = 2,
			CacheCleanupPurgeAll = 4,
			CacheCleanupForceRunNow = 8
		};

		void PAPI CacheCleanup( XString root, const XString& wildcard, int64 maxCacheBytes, int64 cleanIntervalSeconds, int flags );

		void PAPI DirectoryHash( XString root, DataSig20& hash );

	}
}

// Experiment in naming.
PAPI bool AbcIO_WriteWholeFile( const XString& filename, const void* buf, size_t bytes );
PAPI bool AbcIO_ReadWholeFile( const XString& filename, XStringA& str );
