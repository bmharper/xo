#include "pch.h"
#include "GlobalCache.h"
#include "../Platform/timeprims.h"

namespace AbCore
{

ICacher::~ICacher()
{
	if ( CacheMonitor ) CacheMonitor->RemoveCacher( this );
}

GlobalCacheMonitor::GlobalCacheMonitor()
{
	Enable = true;
	LastTick = 0;
	TickInterval = 1000;
}

GlobalCacheMonitor::~GlobalCacheMonitor()
{
	for ( int i = 0; i < Cachers.size(); i++ )
	{
		if ( Cachers[i] ) Cachers[i]->CacheMonitor = NULL;
	}
}

void GlobalCacheMonitor::TickIfTime()
{
	UINT64 now = GlobalCacheTimeMS();
	if ( now - LastTick > TickInterval )
	{
		for ( int i = 0; i < Cachers.size(); i++ )
		{
			// remember that the object may delete itself during a call to GlobalCacheTick.
			if ( Cachers[i] ) Cachers[i]->GlobalCacheTick();
		}
		for ( int i = 0; i < CachersF.size(); i++ )
		{
			if ( CachersF[i] ) CachersF[i]( CachersF_UD[i] );
		}

		LastTick = GlobalCacheTimeMS();
	}
}

void GlobalCacheMonitor::AddCacher( CacheCallback c, void* ud )
{
	for ( int i = 0; i < CachersF.size(); i++ )
	{
		if ( CachersF[i] == c && CachersF_UD[i] == ud ) return;
		if ( CachersF[i] == NULL )
		{
			CachersF[i] = c;
			CachersF_UD[i] = ud;
			return;
		}
	}
	CachersF += c;
	CachersF_UD += ud;
}

void GlobalCacheMonitor::AddCacher( ICacher* c )
{
	c->CacheMonitor = this;
	for ( int i = 0; i < Cachers.size(); i++ )
	{
		if ( Cachers[i] == c ) return;
		if ( Cachers[i] == NULL ) { Cachers[i] = c; return; }
	}
	Cachers += c;
}

void GlobalCacheMonitor::RemoveCacher( ICacher* c )
{
	c->CacheMonitor = NULL;
	for ( int i = 0; i < Cachers.size(); i++ )
	{
		if ( Cachers[i] == c ) { Cachers[i] = NULL; return; }
	}
	ASSERT(false);
}

void GlobalCacheMonitor::RemoveCacher( CacheCallback c, void* ud )
{
	for ( int i = 0; i < CachersF.size(); i++ )
	{
		if ( CachersF[i] == c && CachersF_UD[i] == ud )
		{
			CachersF[i] = NULL;
			CachersF_UD[i] = NULL;
			return;
		}
	}
	ASSERT(false);
}

UINT64 GlobalCacheMonitor::GlobalCacheTimeMS()
{
	return (UINT64) (AbcTimeAccurateRTSeconds() * 1000);
}

}
