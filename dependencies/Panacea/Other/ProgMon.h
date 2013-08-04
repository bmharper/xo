#pragma once

#include "../Platform/syncprims.h"

namespace AbCore
{
	// Progress monitor
	class PAPI IProgMon
	{
	public:
		enum ImageTypes
		{
			/// Image is a Mip2::Bitmap object.
			ImageMip2
		};

		static const int AutoDelayMS = 100;

		virtual ~IProgMon() {}
		virtual bool ProgGet( XString& topic, double& percent ) { return false; }
		
		void ProgSetVal( int i, int outOf )
		{
			ProgSetVal( i / (double) outOf );
		}

		/** Convenience setter.

		Progress is set at (n / outOf) * (batchUnitOutOf100 / 100) + (batchOffsetOutOf100 / 100).

		The sum of all units is 100

		BatchUnitOutOf100:   How much is the current unit of work, out of 100?
		BatchOffsetOutof100: What is the sum of all units already completed?

		**/
		void ProgSetVal_Chunked( int n, int outOf, int batchOffsetOutOf100, int batchUnitOutOf100 )
		{
			double v = (n / (double) outOf) * (batchUnitOutOf100 / 100.0) + (batchOffsetOutOf100 / 100.0);
			ProgSetVal( (float) v );
		}

		void ProgSetVal( double percent )	{ ProgSet( "", percent ); }
		void ProgSetTxt( XString topic )	{ ProgSet( topic, -1 ); }

		/** Update the progress bar.
		Set percent to -1 if you're not updating it.
		Set topic to empty if you're not updating it.
		**/
		virtual void ProgSet( XString topic, double percent ) = 0;
		virtual bool ProgIsCancelled() = 0;
		virtual bool ProgAllowCancel( bool allow ) = 0;	///< Sets whether cancel is allowed. Returns the previous state.
		virtual void ProgCancel(){}
		virtual void ProgSetModeMarquee( bool menableode ) {}

		// This pair is used by Albion's status bar to create/destroy the progress bar. Generally used to activate/deactivate long-lived progress monitors.
		virtual void ProgBarStart( bool force_start_now = false ) {}
		virtual void ProgBarEnd() {}

	};

	/* Helper class when your progress bar is running on a different (typically UI) thread to the job(s) that are
	setting sending progress update notifications.
	Usage:
		* Attach() your ProgThreadMarshall to the real progress monitor.
		* Pass ProgThreadMarshall as the progress monitor to the threaded jobs.
		* From the UI thread, call Poll() from a timer (reasonable timer resolution is 200ms).
	*/
	class PAPI ProgThreadMarshall : public IProgMon
	{
	public:

		ProgThreadMarshall();
		virtual ~ProgThreadMarshall();

		virtual void ProgSet( XString topic, double percent );
		virtual bool ProgIsCancelled();
		virtual bool ProgAllowCancel( bool allow );

		void Attach( IProgMon* sink, bool allowCancel );
		void Poll();

	protected:
		struct Item
		{
			const wchar_t*	Topic;
			double			Percent;
		};
		struct Chunk
		{
			wchar_t*	Data;
			size_t		Pos;		// Pos in characters
			size_t		Size;		// Size in characters
		};
		IProgMon*			Sink;
		podvec<Item>		Queue;
		podvec<Chunk>		Chunks;
		AbcCriticalSection	Lock;
		uint32				IsCancelled;
		uint32				IsCancelledAllowed;

		void CopyStr( const XString& src, const wchar_t*& s );
	};

	// Helper class to save previous progress bar title
	class PAPI ProgSave
	{
	public:
		IProgMon* Mon;
		XString Topic;
		double Percent;
		bool Good;

		ProgSave( IProgMon* mon )
		{
			Mon = mon;
			Good = false;
			Percent = 0;
			if ( Mon ) Good = Mon->ProgGet( Topic, Percent );
		}
		~ProgSave()
		{
			if ( Mon && Good ) Mon->ProgSet( Topic, Percent );
		}
	};

	/// Null progress monitor
	class PAPI NullProg : public IProgMon
	{
		virtual void ProgSet( XString topic, double percent ) {}
		virtual bool ProgIsCancelled() { return false; }
		virtual bool ProgAllowCancel( bool allow ) { return false; }
	};

	// STDOUT progress bar. 
	class PAPI ProgBarStdOut : public IProgMon
	{
	public:
		double		Last;
		double		Granularity;
		XString		Topic;

		ProgBarStdOut()
		{
			Last = 0;
			Granularity = 0.01;
		}

		virtual void ProgSet( XString topic, double percent )
		{
			bool newTopic = false;
			if ( !topic.IsEmpty() )
			{
				newTopic = true;
				Topic = topic;
			}
			if ( newTopic || fabs(percent - Last) >= Granularity )
			{
				if ( percent != -1 ) Last = percent;
				if ( newTopic ) wprintf( L"\n%s: %%%.1f ", (LPCWSTR) Topic, Last * 100 );
				else			wprintf( L"%%%.1f ", Last * 100 );
			}
		}

		virtual bool ProgGet( XString& topic, double& percent ) { topic = Topic; percent = Last; return true; }
		virtual bool ProgIsCancelled() { return false; }
		virtual bool ProgAllowCancel( bool allow ) { return false; }
	};

}
