#ifndef ABCORE_INCLUDED_GUID_H
#define ABCORE_INCLUDED_GUID_H

#include "../Platform/winheaders.h"
#include "../Strings/XString.h"
#include "../HashTab/vHashCommon.h"
#include "../HashTab/ohashmap.h"
#include "../fhash/fhashtable.h"
#include "../Strings/strings.h"
#include "../Strings/fmt.h"

// From Windows guiddef.h
//#ifndef GUID_DEFINED
//typedef struct _GUID {
//	unsigned long  Data1;
//	unsigned short Data2;
//	unsigned short Data3;
//	unsigned char  Data4[ 8 ];
//} GUID;
//#endif

namespace AbCore
{
	class IFile;
}

/** Globally Unique IDentifier.
You should be able to use this instead of a GUID structure defined in the 
Platform SDK.
**/
class PAPI Guid
{
public:
	union
	{
		BYTE Data[16];
#ifdef GUID_DEFINED
		GUID MS;
#endif
		struct
		{
			UINT64 Q1;
			UINT64 Q2;
		};
	};


	Guid()
	{
		Q1 = 0;
		Q2 = 0;
	}

	Guid( const Guid& b )
	{
		*this = b;
	}

	explicit Guid( const void* buf, bool littleEndian )
	{
		ASSERT( littleEndian );
		memcpy( Data, buf, 16 );
	}

#ifdef GUID_DEFINED
	Guid( const GUID& b )
	{
		MS.Data1 = b.Data1;
		MS.Data2 = b.Data2;
		MS.Data3 = b.Data3;
		INT64* p = (INT64*) b.Data4;
		Q2 = *p;
	}

	Guid( DWORD a, WORD b, WORD c, BYTE d, BYTE e, BYTE f, BYTE g, BYTE h, BYTE i, BYTE j, BYTE k )
	{
		MS.Data1 = a;
		MS.Data2 = b;
		MS.Data3 = c;
		MS.Data4[0] = d;
		MS.Data4[1] = e;
		MS.Data4[2] = f;
		MS.Data4[3] = g;
		MS.Data4[4] = h;
		MS.Data4[5] = i;
		MS.Data4[6] = j;
		MS.Data4[7] = k;
	}

	Guid( DWORD a, WORD b, WORD c, BYTE d[8] )
	{
		MS.Data1 = a;
		MS.Data2 = b;
		MS.Data3 = c;
		MS.Data4[0] = d[0];
		MS.Data4[1] = d[1];
		MS.Data4[2] = d[2];
		MS.Data4[3] = d[3];
		MS.Data4[4] = d[4];
		MS.Data4[5] = d[5];
		MS.Data4[6] = d[6];
		MS.Data4[7] = d[7];
	}
#endif

	/// Returns true if all 16 bytes are 0
	bool IsNull() const
	{
		return Q1 == 0 && Q2 == 0;
	}

	/// Sets all bytes to 0
	void SetNull()
	{
		Q1 = 0;
		Q2 = 0;
	}
	
	/// Uses UuidCreateSequential() to create a new GUID.
	static Guid GenerateUnique();

	/// Uses UuidCreate() to create a new GUID.
	static Guid GenerateUniqueSecure();

	/// From registry style GUID strings such as C920469D-CCCE-4969-9A9C-78DAD0C17CCC, or {C920469D-CCCE-4969-9A9C-78DAD0C17CCC},
	/// or a 32 character hex string, such as what we output from ToHexString().
	static Guid FromString( const char* str );
	static Guid FromString( const wchar_t* str );
	static Guid FromStringOrNull( const char* str );
	static Guid FromStringOrNull( const wchar_t* str );

	/// The length of a guid string created using ToString()
	static const int ToStringLength = 8 + 4 + 4 + 4 + 12 + 4;

#pragma warning( push )
#pragma warning( disable: 4996 )
	/// The buffer must be able to hold ToStringLength. Add 1 if you write the null terminator.
	void ToString( char* str, bool nullTerm = true ) const
	{
		// {43E2FE63-7607-4902-8BAB-73AAB0F7ED5E}
		fmt_snprintf( str, ToStringLength, "%08LX-%04hX-%04hX-%02hX%02hX-%02hX%02hX%02hX%02hX%02hX%02hX",
			MS.Data1, MS.Data2, MS.Data3, MS.Data4[0], MS.Data4[1], MS.Data4[2], MS.Data4[3], 
			MS.Data4[4], MS.Data4[5], MS.Data4[6], MS.Data4[7] );
		if ( nullTerm ) str[ToStringLength] = 0;
	}

	/// The buffer must be able to hold ToStringLength. Add 1 if you write the null terminator.
	void ToString( wchar_t* str, bool nullTerm = true ) const
	{
		// {43E2FE63-7607-4902-8BAB-73AAB0F7ED5E}
		// Wide version has different behaviour to narrow, requiring that we set Count+1 instead of Count characters.
		fmt_swprintf( str, ToStringLength + 1, L"%08LX-%04hX-%04hX-%02hX%02hX-%02hX%02hX%02hX%02hX%02hX%02hX",
			MS.Data1, MS.Data2, MS.Data3, MS.Data4[0], MS.Data4[1], MS.Data4[2], MS.Data4[3], 
			MS.Data4[4], MS.Data4[5], MS.Data4[6], MS.Data4[7] );
		if ( nullTerm ) str[ToStringLength] = 0;
	}
#pragma warning( pop )

#ifdef _UNICODE
	/// Creates a registry style GUID string such as C920469D-CCCE-4969-9A9C-78DAD0C17CCC
	XStringW ToString( bool withBraces = false ) const { return ToStringW(withBraces); }
#else
	/// Creates a registry style GUID string such as C920469D-CCCE-4969-9A9C-78DAD0C17CCC
	XStringA ToString( bool withBraces = false ) const { return ToStringA(withBraces); }
#endif

	/// Creates a registry style GUID string such as C920469D-CCCE-4969-9A9C-78DAD0C17CCC
	XStringA ToStringA( bool withBraces = false ) const;

	/// Creates a registry style GUID string such as C920469D-CCCE-4969-9A9C-78DAD0C17CCC
	XStringW ToStringW( bool withBraces = false ) const;

	/// The length of a guid string created using ToHexString()
	static const int ToHexStringLength = 32;

	template< typename CH >
	void TToHexString( CH* buf, bool nullTerminator ) const
	{
		for ( int i = 0; i < 16; i++, buf += 2 )
			ByteToHex<CH, true>( Data[i], buf );
		if ( nullTerminator )
			*buf = 0;
	}

	/// Creates a flat uppercase hex string, byte by byte (resulting string is always 32 characters long, and is a linear byte dump).
	XStringA ToHexStringA() const
	{
		char buf[33];
		TToHexString( buf, true );
		return buf;
	}

	/// Creates a flat uppercase hex string, byte by byte (resulting string is always 32 characters long, and is a linear byte dump).
	XStringW ToHexStringW() const
	{
		wchar_t buf[33];
		TToHexString( buf, true );
		return buf;
	}

#ifdef _UNICODE
	XStringW ToHexString() const { return ToHexStringW(); }
#else
	XStringA ToHexString() const { return ToHexStringA(); }
#endif

#ifdef GUID_DEFINED
	/// Returns a GUID structure
	operator GUID() const
	{
		return MS;
	}

	/// Returns a reference to a GUID structure
	operator GUID&()
	{
		return MS;
	}
#endif

	int GetHashCode() const
	{
		UINT32* id = (UINT32*) Data;
		return (int) (id[0] ^ id[1] ^ id[2] ^ id[3]);
	}

	bool operator==( const Guid& b ) const
	{
		return CryptoMemCmpEq( this, &b, 16 );
	}

	bool operator!=( const Guid& b ) const
	{
		return !CryptoMemCmpEq( this, &b, 16 );
	}

#ifdef GUID_DEFINED
	bool operator==( const GUID& b ) const
	{
		return CryptoMemCmpEq( this, &b, 16 );
	}

	bool operator!=( const GUID& b ) const
	{
		return !CryptoMemCmpEq( this, &b, 16 );
	}
#endif

	Guid& operator=( const Guid& b ) 
	{
		Q1 = b.Q1;
		Q2 = b.Q2;
		return *this;
	}

	/// Returns 0, -1, or 1.
	int Compare( const Guid& b ) const
	{
		if ( Q1 == b.Q1 && Q2 == b.Q2 ) return 0;
		if ( Q1 < b.Q1 ) return -1;
		else if ( Q1 > b.Q1 ) return 1;
		else
		{
			if ( Q2 < b.Q2 ) return -1;
			else return 1;
		}
	}

	bool operator<( const Guid& b ) const
	{
		return -1 == Compare( b );
	}

	bool operator>( const Guid& b ) const
	{
		return 1 == Compare( b );
	}

	void Serialize( AbCore::IFile* file ) const;
	void Deserialize( AbCore::IFile* file );

	Guid EndianSwapped() const
	{
		Guid g = *this;
		g.EndianSwap();
		return g;
	}

	void EndianSwap()
	{
		BYTE b[8];
		b[0] = Data[3];
		b[1] = Data[2];
		b[2] = Data[1];
		b[3] = Data[0];

		b[4] = Data[5];
		b[5] = Data[4];

		b[6] = Data[7];
		b[7] = Data[6];

		memcpy( Data, b, 8 );
	}

//#ifdef _MANAGED
//
//	System::Guid operator() const
//	{
//		return System::Guid( MS.Data1, MS.Data2, MS.Data3,
//		MS.Data4[0], MS.Data4[1], MS.Data4[2], MS.Data4[3], MS.Data4[4], MS.Data4[5], MS.Data4[6], MS.Data4[7] );
//	}
//
//#endif

protected:
	static Guid InternalGenerateUnique( bool secure );
};

FHASH_SETUP_POD_GETHASHCODE(Guid);

typedef vHashTables::vHashMap< Guid, void*, vHashTables::vHashFunction_GetHashCode< Guid > >	GuidPtrMap;
typedef vHashTables::vHashMap< Guid, INT32, vHashTables::vHashFunction_GetHashCode< Guid > >	GuidInt32Map;
typedef vHashTables::vHashSet< Guid, vHashTables::vHashFunction_GetHashCode< Guid > >			GuidSet;
typedef GuidInt32Map GuidIntMap;

typedef ohash::ohashmap< Guid, void*, ohash::ohashfunc_GetHashCode<Guid> >						OGuidPtrMap;
typedef ohash::ohashmap< Guid, INT32, ohash::ohashfunc_GetHashCode<Guid> >						OGuidInt32Map;
typedef ohash::ohashset< Guid,			ohash::ohashfunc_GetHashCode<Guid> >					OGuidSet;
typedef OGuidInt32Map OGuidIntMap;

#endif
