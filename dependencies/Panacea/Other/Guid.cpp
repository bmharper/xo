#include "pch.h"
#include "guid.h"
#include "IO/VirtualFile.h"
#ifdef _WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

Guid Guid::GenerateUnique()
{
	return InternalGenerateUnique( false );
}

Guid Guid::GenerateUniqueSecure()
{
	return InternalGenerateUnique( true );
}

Guid Guid::InternalGenerateUnique( bool secure )
{
	Guid g;
#ifdef _WIN32
	RPC_STATUS st = secure ? UuidCreate( &g.MS ) : UuidCreateSequential( &g.MS );

	// Errors
	if ( st != RPC_S_OK && st != RPC_S_UUID_LOCAL_ONLY )
	{
		ASSERT( false );
		return g;
	}
#else
	if ( secure )
		uuid_generate( (uuit_t) &g );
	else
		uuid_generate_time( (uuit_t) &g );
#endif
	return g;
}


void Guid::Serialize( AbCore::IFile* file ) const
{
	file->Write( &MS, 16 );
}

void Guid::Deserialize( AbCore::IFile* file )
{
	file->Read( &MS, 16 );
}

XStringA Guid::ToStringA( bool withBraces ) const
{
	// {43E2FE63-7607-4902-8BAB-73AAB0F7ED5E}
	//static const GUID <<name>> = 
	//{ 0x43e2fe63, 0x7607, 0x4902, { 0x8b, 0xab, 0x73, 0xaa, 0xb0, 0xf7, 0xed, 0x5e } };
	XStringA str;
	str.Format( withBraces ? "{%08LX-%04hX-%04hX-%02hX%02hX-%02hX%02hX%02hX%02hX%02hX%02hX}" : "%08LX-%04hX-%04hX-%02hX%02hX-%02hX%02hX%02hX%02hX%02hX%02hX", 
		MS.Data1, MS.Data2, MS.Data3, MS.Data4[0], MS.Data4[1], MS.Data4[2], MS.Data4[3], 
		MS.Data4[4], MS.Data4[5], MS.Data4[6], MS.Data4[7] );
	return str;
}

XStringW Guid::ToStringW( bool withBraces ) const
{
	// {43E2FE63-7607-4902-8BAB-73AAB0F7ED5E}
	//static const GUID <<name>> = 
	//{ 0x43e2fe63, 0x7607, 0x4902, { 0x8b, 0xab, 0x73, 0xaa, 0xb0, 0xf7, 0xed, 0x5e } };
	XStringW str;
	str.Format( withBraces ? L"{%08LX-%04hX-%04hX-%02hX%02hX-%02hX%02hX%02hX%02hX%02hX%02hX}" : L"%08LX-%04hX-%04hX-%02hX%02hX-%02hX%02hX%02hX%02hX%02hX%02hX", 
		MS.Data1, MS.Data2, MS.Data3, MS.Data4[0], MS.Data4[1], MS.Data4[2], MS.Data4[3], 
		MS.Data4[4], MS.Data4[5], MS.Data4[6], MS.Data4[7] );
	return str;
}

using namespace AbCore;

#pragma warning( push )
#pragma warning( disable: 4996 )

template< typename TCH >
bool FromHexString32T( const TCH* str, Guid& g )
{
	bool ok = true;
	for ( int i = 0; i < 16 && ok; i++ )
		g.Data[i] = ParseHexByte( str + (i << 1), &ok );
	return ok;
}

template< typename TCH >
Guid FromHexString32T( const TCH* str )
{
	Guid g;
	if ( !FromHexString32T( str, g ) ) g.SetNull();
	return g;
}

template< typename TCH >
Guid FromStringT( const TCH* str, bool tolerateNull )
{
	bool wide = sizeof(TCH) == sizeof(wchar_t);
	Guid g;
	UINT a, b, c, d;
	INT64 e;
	int cv = 0;
	if ( !tolerateNull || str != NULL )
	{
		size_t len;
		if ( wide )
		{
			len = wcslen( (LPCWSTR) str );
			if ( len == 36 )		cv = swscanf( (LPCWSTR) str, L"%X-%X-%X-%X-%" wPRIX64, &a, &b, &c, &d, &e );
			else if ( len == 38 )	cv = swscanf( (LPCWSTR) str, L"{%X-%X-%X-%X-%" wPRIX64 L"}", &a, &b, &c, &d, &e );
		}
		else
		{
			len = strlen( (LPCSTR) str);
			if ( len == 36 )		cv = sscanf( (LPCSTR) str, "%X-%X-%X-%X-%" PRIX64, &a, &b, &c, &d, &e );
			else if ( len == 38 )	cv = sscanf( (LPCSTR) str, "{%X-%X-%X-%X-%" PRIX64 "}", &a, &b, &c, &d, &e );
		}
		if ( len == 32 ) return FromHexString32T( str );
	}
	if ( cv != 5 )
	{
		g.SetNull();
	}
	else
	{
		e |= ((INT64) d) << 48; 
		g.MS.Data1 = a;
		g.MS.Data2 = (unsigned short) b;
		g.MS.Data3 = (unsigned short) c;
		g.Q2 = e;
		Swap( g.MS.Data4[0], g.MS.Data4[7] );
		Swap( g.MS.Data4[1], g.MS.Data4[6] );
		Swap( g.MS.Data4[2], g.MS.Data4[5] );
		Swap( g.MS.Data4[3], g.MS.Data4[4] );
	}
	return g;
}

#pragma warning( pop )

Guid Guid::FromString( const char* str )			{ return FromStringT( str, false ); }
Guid Guid::FromString( const wchar_t* str )			{ return FromStringT( str, false ); }
Guid Guid::FromStringOrNull( const char* str )		{ return FromStringT( str, true ); }
Guid Guid::FromStringOrNull( const wchar_t* str )	{ return FromStringT( str, true ); }
