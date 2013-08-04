#pragma once

#include "../Other/ProgMon.h"

namespace Panacea
{
	namespace IO
	{

		/** Tool for synchronizing two paths.
		This ain't rsync!
		**/
		class PAPI PathSync
		{
		public:
			PathSync();
			~PathSync();

			struct SOptions
			{
				bool			Silent;
				bool			Backup;
				bool			BackupOverwrite;
				bool			CaseSensitive;
				bool			Delete;
				bool			RemoveRO;
				bool			OnlyFresh;
				bool			DryRun;
				bool			PauseAfterEveryFile;	// Used for debugging. Sleeps for 500 ms after every file.
				bool			NumFilesToProgPercent;	// Send integer number of files to progress bar percentage output
				bool			AbortOnError;			// Abort on the first error that we encounter
				bool			ProgEachItem;			// Send progress on every item
				HANDLE			Transaction;			// If not NULL, do all operations on TARGET inside this transaction.
				int				Verbose;
				XString			IntermediateRoot;		// For transactions where our source is not transactionable, we need to copy to the local HD first.
				dvect<XStringW> Exclude;
			};

			SOptions Options;

			struct SStatistics
			{
				Sys::Date		StartTime;
				Sys::Date		EndTime;
				int				FilesIdentical;
				int				FilesRORemoved;
				int				FilesOverwritten;
				int				FilesDeleted;
				int				FilesCreated;
				int				FoldersCreated;
				int				FoldersDeleted;
				XString			FirstError;
				dvect<XStringW> FoldersFailedCreate;
				dvect<XStringW> FoldersFailedDelete;
				dvect<XStringW> FilesFailedRORemove;
				dvect<XStringW> FilesFailedTimeSet;
				dvect<XStringW> FilesFailedInspect;
				dvect<XStringW> FilesFailedCopy;
				dvect<XStringW> FilesFailedOverwrite;
				dvect<XStringW> FilesFailedDelete;

				SStatistics();
				~SStatistics();
				int		NumChanges() const;
				int		NumFailures() const;
				void	Reset();
			protected:
				void	ResetPOD();
			};

			SStatistics Stats;

			/** Process.
			To find out whether the process completed without a hitch, check if Stats.NumFailures() == 0.
			@return True if the source directory exists, and the target directory either exists or could be created.
			**/
			bool Process( XStringW srcDir, XStringW dstDir, AbCore::IProgMon* prog = NULL );

			/// Counts total bytes in directory.
			static int64 TotalBytes( XStringW srcDir, AbCore::IProgMon* prog = NULL );

			XStringW PrintStats();

		protected:
			enum Side
			{
				SOURCE,
				DESTINATION
			};

			void Reset();

			bool FindBackupPath( XStringW dstDir );

			void BuildMap( pvect< FileFindItem* >& items, WStrIntMap& map );
			int FindName( const XStringW& name, WStrIntMap& map );
			bool FilesIdentical( const XStringW& name, bool& dstRO );
			bool IsExcluded( const XStringW& name );

			void TakeError( XString msg = "" );
			void TakeErrorAndPrint( XString msg );
			void Print( const XStringW& msg );
			void PrintRaw( const XStringW& msg );
			void SetProg( const XStringW& msg );
			bool IsCancelledOrAborted();

			XStringW			SrcRoot;
			XStringW			DstRoot;
			XStringW			BackupRoot;
			XStringW			ClassCookie;		// Random cookie that we set every time we are constructed
			XStringW			DirSep;
			AbCore::IProgMon*	Prog;

			// working state
			XStringW SrcDir, DstDir;
			pvect< FileFindItem* > SrcItems, DstItems;
			WStrIntMap SrcItemMap, DstItemMap;
			double LastProgTime;

			void ProcessInternal( XStringW src, XStringW dst );

			void	CopyFileTimes( const XStringW& name );
			void	CopyNewFile( const XStringW& name );
			void	CopyExistingFile( const XStringW& name );
			void	DeleteDstFile( const XStringW& name );
			void	DeleteDstFolder( const XStringW& name );
			bool	RemoveRO( const XStringW& name );
			DWORD	GetAttribs( const XStringW& name, Side side );
			bool	SetAttribs( const XStringW& name, DWORD attribs );
			bool	DeleteFileInt( const XStringW& name );
			bool	CopyFileInt( const XStringW& src, const XStringW& dst, bool failIfExist );
			XString	TempFile();

			void PrintList( const dvect<XStringW>& list, XStringW& toStr, LPCWSTR msg );

		};
	}
}
