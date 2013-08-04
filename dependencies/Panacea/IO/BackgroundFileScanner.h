#pragma once

#include "../System/Date.h"

namespace Panacea
{
	namespace IO
	{

		/** Scans for file information in a background thread.
		
		This was originally created for adb's grid control, so that it could detect if arbitrary strings were OS files,
		and then present them as hyperlinks.

		My original design had two locks - one for the queue and one for the result list, but that ends up creating subtle race conditions
		and potential for deadlocks (specifically one may not insert into the queue if that result already exists)
		so I've gone for a simpler design where there is only one lock, which controls both the queue and the result list.

		**/
		class PAPI BackgroundFileScanner
		{
		public:
			BackgroundFileScanner();
			~BackgroundFileScanner();

			/// Informs the scanner that the process is shutting down. Do not alloc from the heap, just leave in silence. There is no un-shutdown.
			void Shutdown();

			enum Queries
			{
				QFileExists = 1,
				QFileOrFolderExists = 2,
				QMetaDelay30MS			///< Special query type that takes 30 milliseconds to complete (made for testing shutdown mechanism)
			};

			enum Status
			{
				StatusWait,
				StatusReady
			};

			struct QueryItem
			{
				bool operator==( const QueryItem& b ) const { return Type == b.Type && Filename == b.Filename; }
				bool operator!=( const QueryItem& b ) const { return !(*this == b); }
				int GetHashCode() const
				{
					int b[2];
					b[0] = Filename.GetHashCode();
					b[1] = Type;
					return GetHashCode_sdbm( b, sizeof(b) );
				}
				QueryItem() {}
				QueryItem( Queries t, LPCWSTR file ) { Type = t; Filename = file; }
				Queries Type;
				XString Filename;
			};

			struct Result
			{
				bool operator< ( const Result& b ) const
				{
					return MomentMeasured < b.MomentMeasured;
				}
				QueryItem Query;
				Panacea::Sys::Date MomentMeasured;
				double MeasureDurationSeconds;
				bool ResultBool;
			};

			/// Returns the number of items in the queue
			int QueueSize();

			void	SetMaxResultListSize( int maxSize );
			int		GetMaxResultListSize();

			/** Query whether a file exists.
			@param filename The file
			@param result Result of the query
			@return Status of the query
			**/
			Status QueryFileExists( const wchar_t* filename, bool& result );
			Status QueryFileOrFolderExists( const wchar_t* filename, bool& result );

			Status QueryGen( const QueryItem& qi, bool* resultBool );
			void QueryMetaDelay30MS();

			bool DebugIsThreadAlive();
			bool DebugOutput;
			bool DebugFastBGTick;

			/** Baseline discard timeout.
			The exact timeout value is ResultLifetimeSeconds + ResultLifetimeBoost * [Time to acquire result]
			**/
			double ResultLifetimeSeconds;
			double ResultLifetimeBoost;		// Booster factor multiplied by result acquisition time.

			double ActualResultLifetime( double acquisitionTime ) { return ResultLifetimeSeconds + ResultLifetimeBoost * acquisitionTime; }

		protected:
			
			typedef ohash::ohashmap< QueryItem, int, ohash::ohashfunc_GetHashCode<QueryItem> > TQMap;

			dvect<QueryItem>	Queue;
			dvect<Result>		ResultList;
			TQMap				QMap;					// Values are always 1. Presence is only thing that is measured.
			TQMap				RMap;					// Values are [Index in ResultList] + 1.
			CRITICAL_SECTION	BigLock;
			volatile bool		IsShutdown;
			HANDLE				HThread;
			int					MaxResultListSize;		// Maximum number of objects that we will store in our result list
			Panacea::Sys::Date	LastCleanTime;

			static DWORD WINAPI BgThread( LPVOID lp );

			void Clean();
			void Spark();
			void RebuildRMap();
		};
	}
}
