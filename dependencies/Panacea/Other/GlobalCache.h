#pragma once

#include "../Containers/pvect.h"

namespace AbCore
{

	class GlobalCacheMonitor;

	/// Plain function cache callback
	typedef void (*CacheCallback) ( void* ud );

	/** Object that maintains a cache of any sort.

	This is basically just a means to a mechanism whereby we can propagate a tick signal
	to all objects that maintain caches. The alternative would be for those objects to start up
	a background thread, but that is obviously very wasteful, and it introduces concurrency complexity.

	**/
	class PAPI ICacher
	{
	public:
		GlobalCacheMonitor* CacheMonitor;

		ICacher()
		{
			CacheMonitor = NULL;
		}
		
		/** Informs our monitor that we are dying. 
		**/
		virtual ~ICacher();

		/** Called by the global cache manager approximately once per second.
		
		This is where you purge your caches if they have not been used for whatever interval you choose.

		Note that it is legal to delete yourself or remove yourself from the cache manager while processing this message.

		**/
		virtual void GlobalCacheTick() = 0;

	};


	/** Global cache monitor.
	
	Our time units are milliseconds.

	**/
	class PAPI GlobalCacheMonitor
	{
	private:
		pvect<ICacher*>			Cachers;
		pvect<CacheCallback>	CachersF;
		pvect<void*>			CachersF_UD;
		UINT64					LastTick;

	public:
		bool					Enable;
		INT32					TickInterval;

		GlobalCacheMonitor();
		~GlobalCacheMonitor();

		void TickIfTime();
		void AddCacher( CacheCallback c, void* ud );		// It is a no-op to add an already existing CacheCallback/UD pair
		void AddCacher( ICacher* c );						// It is a no-op to add an already existing ICacher
		void RemoveCacher( ICacher* c );
		void RemoveCacher( CacheCallback c, void* ud );
		static UINT64 GlobalCacheTimeMS();

	};

}
