#include "pch.h"
#include "net.h"
#include "../Containers/podvec.h"

#ifdef ABC_PING_DEFINED
#	ifdef _M_X64
		typedef ICMP_ECHO_REPLY32 T_ECHO_REPLY;
#	else
		typedef ICMP_ECHO_REPLY T_ECHO_REPLY;
#	endif
#endif

AbcIpAddress::AbcIpAddress()
{
	SetNull();
}

void AbcIpAddress::Set4( const void* ip4 )
{
	SetNull();
	memcpy( this->Ip4, ip4, 4 );
}

void AbcIpAddress::Set6( const void* ip6 )
{
	memcpy( this->Ip6, ip6, 16 );
}

bool AbcIpAddress::Is4() const
{
	return !Is6();
}

bool AbcIpAddress::Is6() const
{
	return !ismemzero<16 - 4>(this);
}

void AbcIpAddress::SetNull()
{
	memset(this, 0, sizeof(*this));
}

bool AbcIpAddress::IsNull() const
{
	return ismemzero<SIZEOF>(this);
}

u32 AbcIpAddress::GetHashCode() const
{
	u32 out;
	MurmurHash3_x86_32( this, SIZEOF, 0, &out );
	return out;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcJobDnsLookup::AbcJobDnsLookup()
{
	Cache = NULL;
	TimeStart = AbcRTC32();
}

void AbcJobDnsLookup::Run()
{
	//AbcTrace( "DNS lookup %s\n", (LPCSTR) Name );
	int good = AbcNameToAddr( Name, &Addr4, &Addr6 );
	if ( Cache && good != 0 )
	{
		Cache->Set( Name, !Addr4.IsNull() ? &Addr4 : NULL, !Addr6.IsNull() ? &Addr6 : NULL );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcHostnameCacheItem::AbcHostnameCacheItem()
{
	LastUsed = AbcRTC32();
}

AbcHostnameCache::AbcHostnameCache()
{
	CleanupIntervalSeconds = 60;	// cleanup once a minute
	MaxItemAgeSeconds = 30 * 60;	// keep addresses for 30 minutes
	LastCleanupTime = AbcRTC32();
	AbcCriticalSectionInitialize( Lock, 2000 );
}

AbcHostnameCache::~AbcHostnameCache()
{
	AbcCriticalSectionDestroy( Lock );
}

bool AbcHostnameCache::Get( const char* host, AbcIpAddress* i4, AbcIpAddress* i6, AbcJobDnsLookup** job )
{
	TakeCriticalSection cs(Lock);
	Cleanup();
	AbcHostnameCacheItem* item = Items.getp( host );
	if ( item )
	{
		item->LastUsed = AbcRTC32();
		if ( i4 ) *i4 = item->Addr4;
		if ( i6 ) *i6 = item->Addr6;
		return true;
	}
	if ( job )
	{
		*job = new AbcJobDnsLookup();
		(*job)->Cache = this;
		(*job)->Name = host;
	}
	return false;
}

bool AbcHostnameCache::Get( const char* host, AbcIpAddress* i4, AbcIpAddress* i6, AbcJobQueue* queue )
{
	AbcJobDnsLookup* job;
	if ( Get( host, i4, i6, &job ) ) return true;
	queue->AddJob( job );
	return false;
}

void AbcHostnameCache::Set( const char* host, AbcIpAddress* i4, AbcIpAddress* i6 )
{
	TakeCriticalSection cs(Lock);
	Cleanup();
	AbcHostnameCacheItem item;
	if ( i4 ) item.Addr4 = *i4;
	if ( i6 ) item.Addr6 = *i6;
	Items.insert( host, item, true );
}

void AbcHostnameCache::Cleanup()
{
	AbcTime32 now = AbcRTC32();
	if ( now - LastCleanupTime < CleanupIntervalSeconds ) return;
	
	// recreate the hash table from scratch. This is actually necessary for fhashmap, since it accretes "deleted" markers.
	podvec<XStringA>				retain_name;
	podvec<AbcHostnameCacheItem>	retain_item;
	for ( auto it = Items.begin(); it != Items.end(); it++ )
	{
		if ( now - it.val().LastUsed < MaxItemAgeSeconds )
		{
			retain_name += it.key();
			retain_item += it.val();
		}
	}

	Items.clear();
	for ( intp i = 0; i < retain_name.size(); i++ )
		Items.insert( retain_name[i], retain_item[i] );

	LastCleanupTime = AbcRTC32();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef ABC_PING_DEFINED

PAPI XStringW		AbcIpErr( IP_STATUS code )
{
	wchar_t buf[128];
	DWORD bs = arraysize(buf) - 1;
	GetIpErrorString( code, buf, &bs );
	buf[127] = 0;
	return buf;
}

PAPI AbcPingHandle*	AbcPingBegin( const char* address, const void* data, u8 datasize, u32 timeoutMilliseconds )
{
	if ( datasize > AbcPingHandle::MaxDataSize )
	{
		// we have a static sized buffer inside AbcPingHandle. If you need to ping more than 32 bytes.. you're probably doing something wrong?
		ASSERT(false);
		return NULL;
	}
	AbcPingHandle* h = (AbcPingHandle*) malloc( sizeof(AbcPingHandle) );
	if ( !h ) return NULL;
	memset( h, 0, sizeof(AbcPingHandle) );
	memcpy( h->Data(), data, datasize );
	h->DataSize = datasize;
	T_ECHO_REPLY* rep = (T_ECHO_REPLY*) h->Buf;
	h->HFile = IcmpCreateFile();
	h->EvDone = CreateEvent( NULL, true, false, NULL );
	IPAddr addr;
	*((u32*)&addr) = inet_addr( address );
	
	// I don't know whether the async call copies our data out immediately - the docs don't seem to say. I'm taking the cautious route
	// and giving the function our expected reply data as it's input.
	DWORD e = IcmpSendEcho2( h->HFile, h->EvDone, NULL, NULL, addr, h->Data(), h->DataSize, NULL, h->Buf, h->BufSize, timeoutMilliseconds );

	ASSERT( e == ERROR_IO_PENDING || e == ERROR_SUCCESS );
	if ( e == ERROR_SUCCESS )
	{
		// loopback
	}
	return h;
}

PAPI AbcPingHandle*	AbcPingBegin_RDTSC( const char* address, u32 timeoutMilliseconds )
{
	int64 rdtsc = RDTSC();
	return AbcPingBegin( address, &rdtsc, sizeof(rdtsc), timeoutMilliseconds );
}

PAPI void			AbcPingEnd( AbcPingHandle* handle )
{
	IcmpCloseHandle( handle->HFile );
	CloseHandle( handle->EvDone );
	free( handle );
}

PAPI bool			AbcPingResult( const AbcPingHandle* handle, IP_STATUS* result, u32* microseconds )
{
	if ( WaitForSingleObject(handle->EvDone, 0) == WAIT_OBJECT_0 )
	{
		T_ECHO_REPLY* rep = (T_ECHO_REPLY*) handle->Buf;
		if ( rep->Status == IP_SUCCESS )
		{
			// Check the reply data. The packet could come from a previous/future ping.
			
		}
		if ( result ) *result = rep->Status;
		if ( microseconds ) *microseconds = rep->RoundTripTime * 1000;
		return true;
	}
	else return false;
}

#endif

#ifdef ABC_GET_MACS_DEFINED

PAPI bool			AbcGetMACs( int& count, int max, u8 MACs[][ABC_MAX_ADAPTER_ADDRESS_LENGTH] )
{
	count = 0;

	ULONG size = 0;
	IP_ADAPTER_INFO *info_org = NULL;
	GetAdaptersInfo( info_org, &size );
	info_org = (IP_ADAPTER_INFO*) malloc(size);
	if ( !info_org ) return false;
	if ( GetAdaptersInfo(info_org, &size ) != ERROR_SUCCESS ) { free(info_org); return false; }

	for ( IP_ADAPTER_INFO* info = info_org; info != NULL; info = info->Next )
	{
		XStringA adap( info->AdapterName, MAX_ADAPTER_NAME_LENGTH );
		XStringA desc( info->Description, MAX_ADAPTER_DESCRIPTION_LENGTH );
		// I have an info->Type of 71 on Louis Strijdom's laptop. This is undocumented. It seems like when you unplug the ethernet cable, the driver gets unloaded. Windows pops up
		// with a message telling you that you can remove the broadcom ethernet adaptor. But this hack here refers to the Wireless card in the laptop, which returns an undocumented Type = 71.
		bool special_strange_adap = (desc.Contains("Intel") && desc.Contains("Wireless") && info->Type == 71);
		if ( info->AddressLength > 0 && (info->Type == MIB_IF_TYPE_ETHERNET || special_strange_adap) ) 
		{
			if ( count < max )
				memcpy( MACs[count], info->Address, info->AddressLength );
			count++;
		}
	}
		
	free( info_org );

	return count <= max;
}

#endif

PAPI int			AbcNameToAddr( const char* name, AbcIpAddress* i4, AbcIpAddress* i6 )
{
	addrinfo hints;
	addrinfo* res = NULL;
	memset( &hints, 0, sizeof(hints) );
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	//hints.ai_protocol = IPPROTO_TCP;
	//hints.ai_flags = AI_V4MAPPED;
	int nfound = 0;
	int r = getaddrinfo( name, NULL, &hints, &res );
	for ( addrinfo* it = res; it; it = it->ai_next )
	{
		if ( it->ai_family == AF_INET6 && i6 )
		{
			nfound++;
			//LPSOCKADDR sockaddr_ip = (LPSOCKADDR) it->ai_addr;
			//char buf[100];
			//DWORD len = 100;
			//WSAAddressToStringA(sockaddr_ip, (DWORD) it->ai_addrlen, NULL, buf, &len );
			ASSERT( it->ai_addrlen == 28 );	// don't understand this structure
			memcpy( i6->Ip6, it->ai_addr->sa_data + 6, 16 );
		}
		if ( it->ai_family == AF_INET && i4 )
		{
			nfound++;
			ASSERT( it->ai_addrlen == 16 ); // no idea why this is 4
			memcpy( i4->Ip4, it->ai_addr->sa_data + 2, 4 );
		}
	}
	if ( res ) freeaddrinfo( res );
	return nfound;
}

#pragma warning(push)
#pragma warning(disable: 4996) // CRT security
PAPI void			AbcAddrToStr( AbcIpAddress& addr, char* name )
{
	// assume ip4
	sprintf(name, "%u.%u.%u.%u", addr.Ip4[0], addr.Ip4[1], addr.Ip4[2], addr.Ip4[3] );
}
#pragma warning(pop)

PAPI void			AbcAddrToAddr( const AbcIpAddress& in, u16 port, sockaddr_storage* out )
{
	if ( in.Is4() )
	{
		sockaddr_in* out4 = (sockaddr_in*) out;
		out4->sin_family = AF_INET;
		out4->sin_port = htons(port);
		memcpy( &out4->sin_addr, in.Ip4, 4 );
	}
	else
	{
		sockaddr_in6* out6 = (sockaddr_in6*) out;
		out6->sin6_family = AF_INET6;
		out6->sin6_port = htons(port);
		out6->sin6_flowinfo = 0;
		out6->sin6_scope_id = 0;
		memcpy( &out6->sin6_addr, in.Ip6, 16 );
	}
}

PAPI size_t			AbcAddrSizeOf( sockaddr_storage* addr )
{
	switch ( addr->ss_family )
	{
	case AF_INET:	return sizeof(sockaddr_in);
	case AF_INET6:	return sizeof(sockaddr_in6);
	default:		AbcPanic("addr->ss_family is undefined in AbcAddrSizeOf"); return 0;
	}
}

PAPI XStringA		AbcNetHostname()
{
#ifdef _WIN32
	FIXED_INFO *netInfo;
	ULONG infoSize = 0;
	GetNetworkParams( NULL, &infoSize );
	netInfo = (FIXED_INFO*) malloc( infoSize );
	AbcCheckAlloc( netInfo );
	GetNetworkParams( netInfo, &infoSize );
	XStringA host = netInfo->HostName;
	free( netInfo );
	return host;
#else
	const name[200];
	gethostname( name, arraysize(name) );
	name[arraysize(name) - 1] = 0;
	return name;
#endif
}
