#pragma once

#ifdef _WIN32
DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_ALL
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <WS2tcpip.h>
DISABLE_CODE_ANALYSIS_WARNINGS_POP
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#ifdef _WIN32
#define ABC_PING_DEFINED 1
#define ABC_GET_MACS_DEFINED 1
#define ABC_MAX_ADAPTER_ADDRESS_LENGTH  MAX_ADAPTER_ADDRESS_LENGTH
#else
// This is just arbitrarily picked to be the same as the Windows value
#define ABC_MAX_ADAPTER_ADDRESS_LENGTH 8
#endif

#include "../Other/lmDefs.h"
#include "../Strings/XString.h"
#include "../murmur3/MurmurHash3.h"
#include "../Other/JobQueue.h"
#include "../Other/profile.h"

#ifdef ABC_PING_DEFINED

struct AbcPingHandle
{
	static const u32 MaxDataSize = 32;
	static const u32 BufBase = 8 + sizeof(ICMP_ECHO_REPLY);
	static const u32 BufSize = BufBase + MaxDataSize;
	HANDLE	HFile;
	HANDLE	EvDone;
	u8		DataSize;
	u8		Expect[MaxDataSize];
	u8		Buf[BufSize];
	const u8*	Data() const	{ return Buf + BufBase; }
	u8*			Data()			{ return Buf + BufBase; }
};

#endif

struct PAPI AbcIpAddress
{
	union
	{
		u32	Ip4_32;
		u8	Ip4[4];
		u8	Ip6[16];
	};
	static const u32 SIZEOF = 16;

	AbcIpAddress();
	void	Set4( const void* ip4 );
	void	Set6( const void* ip6 );
	void	SetNull();
	bool	Is4() const;
	bool	Is6() const;
	bool	IsNull() const;
	u32		GetHashCode() const;
};
FHASH_SETUP_POD_GETHASHCODE(AbcIpAddress);
static_assert(sizeof(AbcIpAddress) == AbcIpAddress::SIZEOF, "AbcIpAddress sizeof");

struct PAPI AbcHostnameCacheItem
{
	AbcHostnameCacheItem();
	AbcTime32		LastUsed;
	AbcIpAddress	Addr4;
	AbcIpAddress	Addr6;
};
FHASH_SETUP_CLASS_CTOR_DTOR(AbcHostnameCacheItem, AbcHostnameCacheItem);

class AbcJobDnsLookup;

// Cache of DNS host names
// Don't use any of the public members -- access everything through the functions.
// All public functions are thread safe.
class PAPI AbcHostnameCache
{
public:
	fhashmap<XStringA, AbcHostnameCacheItem>	Items;
	AbcTime32									LastCleanupTime;		// Time when Cleanup() was last run.
	int32										CleanupIntervalSeconds;	// Automatically run Cleanup() after this many seconds.
	int32										MaxItemAgeSeconds;		// When Cleanup is run, we remove entries older than this.
	AbcCriticalSection							Lock;

	AbcHostnameCache();
	~AbcHostnameCache();
	// Lookup host. If the host is in the cache, it is returned immediately.
	// If the host is not in the cache, and 'job' is not NULL, then we create 'job'. It is your responsibility to make it run on a job queue of your choice,
	// as well as to poll the cache for the address.
	bool Get( const char* host, AbcIpAddress* i4, AbcIpAddress* i6, AbcJobDnsLookup** job );
	bool Get( const char* host, AbcIpAddress* i4, AbcIpAddress* i6, AbcJobQueue* queue );		// Creates the job and puts it in the queue for you
	void Set( const char* host, AbcIpAddress* i4, AbcIpAddress* i6 );
protected:
	void Cleanup();		// Purge items older than MaxAgeSeconds. This is called automatically by Get and Set if the appropriate time has elapsed since the last Cleanup().
};

// Host name lookup via AbcNameToAddr
class PAPI AbcJobDnsLookup : public AbcJob
{
public:
	AbcTime32			TimeStart;
	XStringA			Name;
	AbcIpAddress		Addr4;
	AbcIpAddress		Addr6;
	AbcHostnameCache*	Cache;
	
	AbcJobDnsLookup();
	virtual void Run();
};

// IP4/6 bit masks
//enum AbcIpVFilter
//{
//	AbcIpV_4 = 1,
//	AbcIpV_6 = 2,
//};

#ifdef ABC_PING_DEFINED

PAPI XStringW		AbcIpErr( IP_STATUS code );

PAPI AbcPingHandle*	AbcPingBegin( const char* address, const void* data, u8 datasize, u32 timeoutMilliseconds );

// Send out a ping with the value of RDTSC as the 'data'
PAPI AbcPingHandle*	AbcPingBegin_RDTSC( const char* address, u32 timeoutMilliseconds );

// Finish a ping
PAPI void			AbcPingEnd( AbcPingHandle* handle );

// return true if the asynchronous call has yielded a result
// result is typically IP_SUCCESS, IP_REQ_TIMED_OUT
PAPI bool			AbcPingResult( const AbcPingHandle* handle, IP_STATUS* result, u32* microseconds );

#endif

#ifdef ABC_GET_MACS_DEFINED

// Returns false if the core function fails, or if 'max' is not large enough to hold the required adapters. In such a case, 'count' will hold the required space.
PAPI bool			AbcGetMACs( int& count, int max, u8 MACs[][ABC_MAX_ADAPTER_ADDRESS_LENGTH] );

#endif

// Returns the number of addresses found (0, 1, or 2)
PAPI int			AbcNameToAddr( const char* name, AbcIpAddress* i4, AbcIpAddress* i6 );

PAPI void			AbcAddrToStr( AbcIpAddress& addr, char* name );

PAPI void			AbcAddrToAddr( const AbcIpAddress& in, u16 port, sockaddr_storage* out );
PAPI size_t			AbcAddrSizeOf( sockaddr_storage* addr );

PAPI XStringA		AbcNetHostname();


