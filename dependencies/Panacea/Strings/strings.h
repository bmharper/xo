#pragma once

//#include <greta\regexpr2.h>

#include "../Other/lmTypes.h"
#include "../Containers/smallvec.h"
#include "../Containers/podvec.h"

/*
Conventions:

	Use std::string as a generic byte buffer.

*/

namespace AbCore
{
	/*
	class RegExPattern
	{
	public:
		RegExPattern( const XString& str ) 
		{
			pat = regex::rpattern_c( str );
		}
		~RegExPattern() 
		{
		}

	protected:
		regex::rpattern_c pat;
	};
	*/




	template< typename TStr >
	TStr AddCommas( const TStr& s )
	{
		TStr r;
		int len = s.Length();
		bool add = true;
		// find the dot.. start at 2, avoiding a potential minus sign, because we know this
		int i;
		for ( i = 1; i < len; i++ )
		{
			if ( s[i] == '.' ) break;
			if ( s[i] < '0' || s[i] > '9' ) return s;
		}

		// build the result string in reverse
		for ( int j = len - 1; j >= i; j-- )
			r += s[j];

		int c = 0;
		for ( i--; i >= 0; i--, c++ )
		{
			if ( c == 3 && s[i] != '-' )
			{
				c = 0;
				r += ',';
			}
			r += s[i];
		}

		int rlen = r.Length();
		for ( int j = 0; j < rlen/2; j++ )
			AbCore::Swap( r[j], r[rlen - 1 - j] );

		return r;
	}

}

/// Make a value between 0 and 15 into a hex character. Returns character 'x' for input greater than 15.
template< bool UpperCase >
int IntToHexChar( BYTE val )
{
	if ( UpperCase )	return "0123456789ABCDEF"[val & 0xf];
	else				return "0123456789abcdef"[val & 0xf];
}

/// Always writes two characters, and does not write a null terminator. If str is null, the function just returns.
template< typename CH, bool UpperCase >
void ByteToHex( BYTE v, CH* str )
{
	if ( str == NULL ) return;
	str[0] = (CH) IntToHexChar<UpperCase> ( v >> 4 );
	str[1] = (CH) IntToHexChar<UpperCase> ( v );
}

/// Formats Big Endian
template< typename CH, bool UpperCase >
void ShortToHex( UINT16 v, CH* str )
{
	ByteToHex<CH, UpperCase>( v >> 8, str );
	ByteToHex<CH, UpperCase>( v & 0xFF, str + 2 );
}

template< typename CH >
void ByteToHexUC( BYTE v, CH* str ) { return ByteToHex<CH, true>(v, str); }

template< typename CH >
void ByteToHexLC( BYTE v, CH* str ) { return ByteToHex<CH, false>(v, str); }


template< typename CH >
void ShortToHexUC( UINT16 v, CH* str ) { return ShortToHex<CH, true>(v, str); }

template< typename CH >
void ShortToHexLC( UINT16 v, CH* str ) { return ShortToHex<CH, false>(v, str); }


inline bool ParseHexChar( int ch, BYTE& val )
{
	if ( ch >= 'A' && ch <= 'F' ) { val = 10 + ch - 'A'; return true; }
	if ( ch >= 'a' && ch <= 'f' ) { val = 10 + ch - 'a'; return true; }
	if ( ch >= '0' && ch <= '9' ) { val = ch - '0'; return true; }
	return false;
}

inline BYTE ParseHexCharNoCheck( int ch )
{
	if ( ch >= 'A' && ch <= 'F' ) return 10 + ch - 'A';
	if ( ch >= 'a' && ch <= 'f' ) return 10 + ch - 'a';
	else							return ch - '0';
}

template< typename CH >
inline void TBufferToHex( size_t bytes, const void* data, CH* hex, bool upperCase, bool add_terminator = true )
{
	const BYTE* bdata = (const BYTE*) data;
	if ( upperCase )	for ( size_t i = 0; i < bytes; i++ ) ByteToHex<CH,true>( bdata[i], hex + i*2 );
	else				for ( size_t i = 0; i < bytes; i++ ) ByteToHex<CH,false>( bdata[i], hex + i*2 );
	if ( add_terminator ) hex[bytes*2] = 0;
}

inline XStringW BufferToHexW( size_t bytes, const void* data, bool upperCase = true )
{
	XStringW str;
	str.Resize( (int) bytes * 2 );
	TBufferToHex<wchar_t>( bytes, data, str.GetRawBuffer(), upperCase, true );
	return str;
}

inline XStringA BufferToHexA( size_t bytes, const void* data, bool upperCase = true )
{
	XStringA str;
	str.Resize( (int) bytes * 2 );
	TBufferToHex<char>( bytes, data, str.GetRawBuffer(), upperCase, true );
	return str;
}

/** Parse a hex character pair or a single hex character as a byte value from 0 to 255.
The function will never read more than the first 2 characters of the array, and it will 
stop without error if it receives a null character, or if the string itself is null.
Upon any error, ok will be set to false (if not NULL), and zero returned.
The first character is high and the second character is low.
**/
template< typename CH >
BYTE ParseHexByte( const CH* str, bool* ok = NULL )
{
	if ( str == NULL )										{ if (ok) *ok = false; return 0; }
	if ( str[0] == 0 )										{ if (ok) *ok = false; return 0; }
	BYTE a = 0, b = 0;
	if ( !ParseHexChar( str[0], a ) )						{ if (ok) *ok = false; return 0; }
	if ( str[1] == 0 )										{ if (ok) *ok = true; return a; }
	if ( !ParseHexChar( str[1], b ) )						{ if (ok) *ok = false; return 0; }
	if (ok) *ok = true;
	return (a << 4) | b;
}

template< typename CH >
BYTE ParseHexByteNoCheck( const CH* str )
{
	BYTE a = 0, b = 0;
	ParseHexChar( str[0], a );
	ParseHexChar( str[1], b );
	return (a << 4) | b;
}

template< typename CH, bool upperCase >
void TInt32ToHexString( UINT32 v, CH* buff )
{
	ByteToHex< CH, upperCase >( (v >> 24) & 0xFF, buff + 0 );
	ByteToHex< CH, upperCase >( (v >> 16) & 0xFF, buff + 2 );
	ByteToHex< CH, upperCase >( (v >> 8) & 0xFF, buff + 4 );
	ByteToHex< CH, upperCase >( (v >> 0) & 0xFF, buff + 6 );
}

template< typename CH, bool upperCase >
void TIntToHexString( INT64 v, CH* buff )
{
	ByteToHex< CH, upperCase >( (v >> 56) & 0xFF, buff + 0 );
	ByteToHex< CH, upperCase >( (v >> 48) & 0xFF, buff + 2 );
	ByteToHex< CH, upperCase >( (v >> 40) & 0xFF, buff + 4 );
	ByteToHex< CH, upperCase >( (v >> 32) & 0xFF, buff + 6 );
	ByteToHex< CH, upperCase >( (v >> 24) & 0xFF, buff + 8 );
	ByteToHex< CH, upperCase >( (v >> 16) & 0xFF, buff + 10 );
	ByteToHex< CH, upperCase >( (v >> 8) & 0xFF, buff + 12 );
	ByteToHex< CH, upperCase >( (v >> 0) & 0xFF, buff + 14 );
}

template< typename CH, bool UpperCase >
CH* TIntToHexString_Find( INT64 v, CH* buff )
{
	TIntToHexString< CH, UpperCase >( v, buff );
	buff[16] = 0;
	int off = 0;
	while ( off < 16 && buff[off] == '0' )
		off++;
	if ( off == 16 ) off = 15;
	return buff + off;
}

inline XStringA Int32ToHexStringA( uint32 v, bool upperCase = true )
{
	char buff[9];
	if ( upperCase )	TInt32ToHexString< char, true >( v, buff );
	else				TInt32ToHexString< char, false >( v, buff );
	buff[8] = 0;
	return buff;
}

inline XStringA IntToHexStringA( INT64 v, bool upperCase = true )
{
	char buff[17];
	return upperCase ? 
		TIntToHexString_Find< char, true >( v, buff ) :
		TIntToHexString_Find< char, false >( v, buff );
}

inline XStringW IntToHexStringW( INT64 v, bool upperCase = true )
{
	wchar_t buff[17];
	return upperCase ? 
		TIntToHexString_Find< wchar_t, true >( v, buff ) :
		TIntToHexString_Find< wchar_t, false >( v, buff );
}


// preferStraight: if the G formatting is equivalent in precision to F formatting,
// then prefer F over G. This only has meaning when the number is less than 0.001.
// So for example if you give 10 digits of precision, and the number is 0.0000023,
// then preferStraight will cause the resulting string to be '0.0000023' instead of '2.3e-6'
XStringA PAPI FloatToXStringA( double v, int digits = 10, bool preferStraight = false, bool integerAsDouble = false );
XStringA PAPI IntToXStringA( INT64 v );
XStringW PAPI FloatToXStringW( double v, int digits = 10, bool preferStraight = false, bool integerAsDouble = false );
XStringW PAPI IntToXStringW( INT64 v );

XStringA PAPI IntToXStringAWithCommas( int64 v );
XStringW PAPI IntToXStringWWithCommas( int64 v );

/// This uses FloatToXString, and then adds the commas afterwards, so digits is not properly respected.
XStringA PAPI FloatToXStringAWithCommas( double v, int digits = 20 );
XStringW PAPI FloatToXStringWWithCommas( double v, int digits = 20 );

bool PAPI FloatToStringDB( char* buf, int digits, double v );
bool PAPI FloatToStringDB( wchar_t* buf, int digits, double v );

#ifdef _UNICODE
inline XString FloatToXString( double v, int digits = 10, bool preferStraight = false, bool forceDecimalPoint = false )
{ 
	return FloatToXStringW( v, digits, preferStraight, forceDecimalPoint ); 
}
inline XString FloatToXStringWithCommas( double v, int digits = 10 ) { return FloatToXStringWWithCommas(v, digits);}
inline XString IntToXString( INT64 v )							{ return IntToXStringW( v ); }
inline XString IntToHexString( INT64 v, bool upperCase = true )	{ return IntToHexStringW( v, upperCase ); }
#else
inline XString FloatToXString( double v, int digits = 10, bool preferStraight = false, bool forceDecimalPoint = false )
{ 
	return FloatToXStringA( v, digits, preferStraight, forceDecimalPoint ); 
}
inline XString FloatToXStringWithCommas( double v, int digits = 10 ) { return FloatToXStringAWithCommas(v, digits);}
inline XString IntToXString( INT64 v )							{ return IntToXStringA( v ); }
inline XString IntToHexString( INT64 v, bool upperCase = true )	{ return IntToHexStringA( v, upperCase ); }
#endif

template< typename TInt, typename CH >
TInt ParseHex( const CH* str, bool* ok = NULL )
{
	if ( ok ) *ok = true;
	TInt val = 0;
	if ( str == NULL ) return val;
	for ( int i = 0; str[i] && i < sizeof(TInt) * 2; i++ )
	{
		BYTE chv;
		bool parse = ParseHexChar( str[i], chv );
		if ( !parse ) { if ( ok ) *ok = false; break; }
		val = (val << 4) | chv;
	}
	return val;
}

inline UINT32 ParseHexA( const char* str, bool* ok = NULL )			{ return ParseHex<UINT32, char>( str, ok ); }
inline UINT32 ParseHexW( const wchar_t* str, bool* ok = NULL )		{ return ParseHex<UINT32, wchar_t>( str, ok ); }
inline UINT64 ParseHex64A( const char* str, bool* ok = NULL )		{ return ParseHex<UINT64, char>( str, ok ); }
inline UINT64 ParseHex64W( const wchar_t* str, bool* ok = NULL )	{ return ParseHex<UINT64, wchar_t>( str, ok ); }

#ifdef _UNICODE
inline UINT32 ParseHex( const wchar_t* str, bool* ok = NULL )		{ return ParseHexW(str, ok); }
inline UINT64 ParseHex64( const wchar_t* str, bool* ok = NULL )		{ return ParseHex64W(str, ok); }
#else
inline UINT32 ParseHex( const char* str, bool* ok = NULL )			{ return ParseHexA(str, ok); }
inline UINT64 ParseHex64( const char* str, bool* ok = NULL )		{ return ParseHex64A(str, ok); }
#endif

inline int ParseIntA( const XStringA& str )
{
	if ( str.IsEmpty() ) return 0;
	return atoi( str );
}

inline int ParseIntW( const XStringW& str )
{
	if ( str.IsEmpty() ) return 0;
	return _wtoi( str );
}

inline int ParseInt( const XString& str )
{
#ifdef _UNICODE
	return ParseIntW( str );
#else
	return ParseIntA( str );
#endif
}

inline int ParseIntA( const char* str )
{
	if ( str == NULL ) return 0;
	return atoi( str );
}

inline int ParseIntW( const wchar_t* str )
{
	if ( str == NULL ) return 0;
	return _wtoi( str );
}

inline int ParseInt( const TCHAR* str )
{
#ifdef _UNICODE
	return ParseIntW( str );
#else
	return ParseIntA( str );
#endif
}

inline INT64 ParseInt64A( const XStringA& str )
{
	if ( str.IsEmpty() ) return 0;
	return _atoi64( str );
}

inline INT64 ParseInt64W( const XStringW& str )
{
	if ( str.IsEmpty() ) return 0;
	return _wtoi64( str );
}

inline INT64 ParseInt64( const XString& str )
{
#ifdef _UNICODE
	return ParseInt64W( str );
#else
	return ParseInt64A( str );
#endif
}

inline INT64 ParseInt64A( const char* str )
{
	if ( str == NULL ) return 0;
	return _atoi64( str );
}

inline INT64 ParseInt64W( const wchar_t* str )
{
	if ( str == NULL ) return 0;
	return _wtoi64( str );
}

inline INT64 ParseInt64( const TCHAR* str )
{
#ifdef _UNICODE
	return ParseInt64W( str );
#else
	return ParseInt64A( str );
#endif
}

inline double ParseFloatA( const XStringA& str )
{
	if ( str.IsEmpty() ) return 0;
	return atof( str );
}

inline double ParseFloatW( const XStringW& str )
{
	if ( str.IsEmpty() ) return 0;
	return _wtof( str );
}

inline double ParseFloatA( const char* str, int maxLen = -1 )
{
	if ( str == NULL ) return 0;
	if ( maxLen != -1 )
	{
		int top = min(maxLen, 127);
		char stat[128];
		int i = 0;
		for ( ; i < top && str[i] != 0; i++ )
			stat[i] = str[i];
		stat[i] = 0;
		return atof( stat );
	}
	return atof( str );
}

inline double ParseFloatW( const wchar_t* str, int maxLen = -1 )
{
	if ( str == NULL ) return 0;
	if ( maxLen != -1 )
	{
		int top = min(maxLen, 127);
		wchar_t stat[128];
		int i = 0;
		for ( ; i < top && str[i] != 0; i++ )
			stat[i] = str[i];
		stat[i] = 0;
		return _wtof( stat );
	}
	return _wtof( str );
}

inline double ParseFloat( const XString& str )
{
#ifdef _UNICODE
	return ParseFloatW( str );
#else
	return ParseFloatA( str );
#endif
}

inline double ParseFloat( const TCHAR* str )
{
#ifdef _UNICODE
	return ParseFloatW( str );
#else
	return ParseFloatA( str );
#endif
}

// Type must be unsigned or this won't work
template<typename T, typename CH>
void TToBinaryUnsigned( T val, CH* out )
{
	CH* base = out;
	out += sizeof(val) * 8;
	for ( ; out != base; out-- )
	{
		*out = val & 1;
		val = val >> 1;
	}
}

template<typename CH> void ToBinary8(   int8	val, CH* out )	{ TToBinaryUnsigned((uint8) val, out); }
template<typename CH> void ToBinary8(  uint8	val, CH* out )	{ TToBinaryUnsigned((uint8) val, out); }
template<typename CH> void ToBinary16(  int16	val, CH* out )	{ TToBinaryUnsigned((uint16) val, out); }
template<typename CH> void ToBinary16( uint16	val, CH* out )	{ TToBinaryUnsigned((uint16) val, out); }
template<typename CH> void ToBinary32(  int32	val, CH* out )	{ TToBinaryUnsigned((uint32) val, out); }
template<typename CH> void ToBinary32( uint32	val, CH* out )	{ TToBinaryUnsigned((uint32) val, out); }
template<typename CH> void ToBinary64(  int64	val, CH* out )	{ TToBinaryUnsigned((uint64) val, out); }
template<typename CH> void ToBinary64( uint64	val, CH* out )	{ TToBinaryUnsigned((uint64) val, out); }

template<typename CH>
void ToBinary( intr vsize, const void* val, CH* out )
{
	const u8* val8 = (const u8*) val;
	out += vsize * 8;
	*out-- = 0;
	for ( intr b = 0; b < vsize; b++ )
	{
#ifdef ENDIANLITTLE
		u8 tbyte = val8[b];
#else
		static_assert(false, "untested code");
		u8 tbyte = val8[vsize - 1 - b];
#endif
		for ( intr i = 0; i < 8; i++, out-- )
		{
			*out = '0' + (tbyte & 1);
			tbyte = tbyte >> 1;
		}
	}
}


/* http://code.google.com/p/stringencoders/wiki/PerformanceAscii
Extract:

Here's how it works. Lower case ascii characer are from 0x61 to 0x7A, and uppercase characters are from 0x41 to 0x5A, (a difference of 0x20). For each octet in the uint32_t, we want an intermediate result that is either 0x00 or 0x20 depending. Then we can just subtract. For instance if the input is 0x61614242 ("aaBB") we want 0x20200000, so 0x61614242 - 0x20200000 = 0x41414242 ("AABB").

1. Strip high bits of each octet (e.g. (0x7f7f7f7ful & eax) and then add 0x05 to each octet + 0x05050505ul;. The stripping of high bits is needed when we add 5, overflow doesn't effect the next octet. The addition moves the range of lower case characters to 0x66 to 0x7F.

2. Restrip off the high bits. This means anything that is "bigger" than "z" is removed, and then add 0x1a (26). Now all lower case characters have the hight bit SET. All non-lower case character do NOT. This bit is all we care about now.

3. ebx & ~eax is a bit tricky. All we care about in ~eax is the high bit. We could do ebx & ~(eax & 0x80808080ul) to make the intentions more clear. Regardless, if the original input has the high bit set, then clear our high bit since this is not a lower case character If the original does not have the high bit set, then all is ok.

4. Shift the high bits over by two places, i.e. 0x80 becomes 0x20. Then clear out all the other bits.

5. We now have a integer that has octets of either 0x00 or 0x020. Subtract.

Ta-da.
*/

typedef unsigned int uint32;

inline uint32 ToUpperFastBlock( uint32 eax )
{
	uint32 ebx = (0x7f7f7f7ful & eax) + 0x05050505ul;
	ebx = (0x7f7f7f7ful & ebx) + 0x1a1a1a1aul;
	ebx = ((ebx & ~eax) >> 2 ) & 0x20202020ul;
	return eax - ebx;
}

/** Fast ToUpper, that operates in 4 character chunks.
You must ensure that there is sufficient padding at the end of the string, if the length is not divisible by 4.

NOTE! You should consider using modp_toupper(), since that uses the same technique, but it better respects string lengths.
**/
inline void ToUpperFast( char* ptr, size_t len )
{
	uint32* p = (uint32*) ptr;
	len = (len + 3) >> 2;
	for ( size_t i = 0; i < len; i++ )
		p[i] = ToUpperFastBlock( p[i] );
}

inline bool IsUpperCase( const char* p, int len )
{
	for ( int i = 0; i < len; i++ )
	{
		if ( p[i] >= 'a' && p[i] <= 'z' ) return false;
	}
	return true;
}

template< typename CH >
int StringHash_sdbm( const CH* str )
{
	if ( str == NULL ) return 0;
	// SYNC-SDBM
	// This was copied from http://www.cs.yorku.ca/~oz/hash.html
	// Two names mentioned on that page are djb2 and sdbm
	// This is sdbm.
	unsigned int hash = 0;
	int i = 0;
	while ( str[i] != 0 )
	{
		// hash(i) = hash(i - 1) * 65539 + str[i]
		hash = (unsigned int) str[i] + (hash << 6) + (hash << 16) - hash;
		i++;
	}
	return (int) hash;
}

XString PAPI FixExtension( XString fname, XString ext, bool caseSense = false, bool forceExtension = false );

/// Increment a name such as "File001" to "File002"
XStringA PAPI IncrementName( const XStringA &name );

/// Increment a name such as "File001" to "File002"
XStringW PAPI IncrementName( const XStringW &name );

// Will return the extension plus the dot (.)
XStringA PAPI GetExtension( const XStringA &fname );
XStringW PAPI GetExtension( const XStringW &fname );

// Encodes to base 64. Extra data is padded with the equal sign.
// The character set is "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
// as per RFC1113.
// This is just a wrapper around modp_b64_encode.
XStringA PAPI EncodeBase64( int bytes, const void* src );

// Returns the maximum number of bytes that can result from encoding a raw string of "plaintext_length" characters.
template<typename T> T EncodeBase64_EncodeLen( T plaintext_length ) { return (plaintext_length + 2) / 3 * 4 + 1; }

// Decodes base 64. Returns false if there are any illegal characters in the source.
// Decoding will always stop at the first equals sign. To be safe you will want
// to allocate src.Length() * 3 / 4 + 4 bytes.
// srclen is allowed to be -1
// This version skips whitespace.
bool PAPI DecodeBase64( const char* src, int srclen, int& length, void* dst );
bool PAPI DecodeBase64( const XStringA& src, int& length, void* dst );

// Returns an empty string if an error occurred.
// Uses DecodeBase64 (ie whitespace-tolerant)
podvec<byte> PAPI DecodeBase64( const XStringA& src );

// Returns the maximum number of bytes that can result from decoding a base-64 encode string of "base64_length" characters.
template<typename T> T DecodeBase64_DecodeLen( T base64_length ) { return (base64_length / 4 * 3 + 4); }

// This version does not skip whitespace. Whitespace will cause an error (ie empty string returned)
// Any error will result in an empty string return.
// This is just a wrapper around modp_b64_decode.
std::string	PAPI DecodeBase64_Fast( const XStringA& src );

// Returns the result buffer length if decoding succeeds, or -1 if decoding fails. 
int			PAPI DecodeBase64_Fast( const XStringA& src, void* dst );

typedef bool (*ReplaceText_Callback) ( void* context, const XStringW& key, XStringW& value );

template< typename TMap >
bool ReplaceText_Map( void* context, const XStringW& key, XStringW& value )
{
	TMap* map = (TMap*) context;
	if ( map->contains( key ) )
	{
		value = map->get( key );
		return true;
	}
	return false;
}

/** Replaces magic text, surrounded by equal length open/close tokens.
@param tOpen	Open token. May only be 1 or 2 characters long. Open token must be same length as close token.
@param tClose	Close token. May only be 1 or 2 characters long. Open token must be same length as close token.
**/
PAPI bool ReplaceText_Tokens( const wchar_t* tOpen, const wchar_t* tClose, const XStringW& original, XStringW& final, XStringW& temp, void* context, ReplaceText_Callback callback );

/// Joins a bunch of strings together
template< typename TString >
TString JoinStrings( int n, const TString* strings, const TString& joiner )
{
	TString cat;
	for ( int i = 0; i < n; i++ )
	{
		cat += strings[i];
		if ( i != n - 1 ) cat += joiner;
	}
	return cat;
}

template< typename TString >
TString JoinStringsV( const dvect<TString>& strings, const TString& joiner )
{
	return JoinStrings( strings.size(), &strings[0], joiner );
}







///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// All we do is escape the delimiter and backslashes
PAPI XStringA EncodeStringList( intp n, const XStringA* list, char sep );
PAPI XStringW EncodeStringList( intp n, const XStringW* list, wchar_t sep );

template<typename TVect> XStringA EncodeStringListA( const TVect& v, char sep )			{ return EncodeStringList(v.size(), &v[0], sep); }
template<typename TVect> XStringW EncodeStringListW( const TVect& v, wchar_t sep )		{ return EncodeStringList(v.size(), &v[0], sep); }


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template< typename TBuffer, typename TStr>
void		WrapTempStr( TBuffer& buf, TStr& t )		{ t.MakeTemp(&buf[0], buf.size() - 1); }

template< typename TStr>
inline void KillTempStr( TStr& t )						{ t.DestroyTemp(); }



// Parse a delimited string list, with support for escaping with a backslash
// This thing takes a void* context so that you can use it with function pointers, to avoid the template bloat from lambdas.
template< typename TStr, typename TCH, typename TOut >
bool BaseParseStringList( const TStr& s, TCH sep, void* context, TOut out )
{
	if ( s.Length() == 0 ) return true;
	smallvec_stack<TCH, 64> tok;
	TStr temp;
	bool go = true;
	for ( int i = 0; i < s.Length(); i++ )
	{
		if ( s[i] == '\\' )
		{
			if ( i < s.Length() - 1 ) tok += s[i+1];
			i++;
		}
		else if ( s[i] == sep )
		{
			tok += 0;
			WrapTempStr( tok, temp );
			go = out( context, temp );
			KillTempStr( temp );
			tok.clear();
			tok[0] = 0;
			if ( !go ) return false;
		}
		else tok += s[i];
	}
	tok += 0;
	WrapTempStr( tok, temp );
	go = out( context, temp );
	KillTempStr( temp );
	if ( !go ) return false;
	return true;
}


typedef bool (*FStringAOut)( void* context, const XStringA& str );
typedef bool (*FStringWOut)( void* context, const XStringW& str );

template< typename TContainer, typename TStr >
inline bool StringOutIntoContainer( void* context, const TStr& s )
{
	TContainer* v = (TContainer*) context;
	*v += s;
	return true;
}

template< typename TContainer >
inline bool TParseStringListContainer( const XStringA& s, char sep, TContainer& container )
{
	return BaseParseStringList( s, sep, NULL, [&container](void* cx, const XStringA& v) -> bool { container += v; return true; } );
}

// Non-bloating.
PAPI bool ParseStringListA( const XStringA& s, char sep,	void* context, FStringAOut target );
PAPI bool ParseStringListW( const XStringW& s, wchar_t sep, void* context, FStringWOut target );

template<typename TContainer> inline bool ParseStringListContainerA( const XStringA& s, char sep,	TContainer& res )	{ return ParseStringListA( s, sep, &res, &StringOutIntoContainer<TContainer,XStringA> ); }
template<typename TContainer> inline bool ParseStringListContainerW( const XStringW& s, wchar_t sep, TContainer& res )	{ return ParseStringListW( s, sep, &res, &StringOutIntoContainer<TContainer,XStringW> ); }

template<typename TContainer> inline bool ParseCommaStringListA( const XStringA& s, TContainer& res )	{ return ParseStringListContainerA(s,  ',', res); }
template<typename TContainer> inline bool ParseCommaStringListW( const XStringW& s, TContainer& res )	{ return ParseStringListContainerW(s, L',', res); }

template<typename TVect> TVect TParseCommaStringListA( const XStringA& s ) { TVect v; ParseCommaStringListA( s, v ); return v; }
template<typename TVect> TVect TParseCommaStringListW( const XStringW& s ) { TVect v; ParseCommaStringListW( s, v ); return v; }


/*
// Lambda accepting version.. FAIL
template< typename TOut >
inline bool TStringIntoLambda( TOut context, const XStringA& val ) { context("Hello"); return true; }// return TOut(val); }

template< typename TOut >
inline void TParseStringList( const XStringA& s, char sep, TOut out )
{
	TStringIntoLambda<TOut>( out, "Hello" );

	ParseStringList( s, sep, out, (FStringOut) &TStringIntoLambda<TOut> );
}
*/











/// XString Key-Value pair
struct XStringKV
{
	XStringKV() {}
	XStringKV( const XString& key, const XString& value ) { Key = key; Value = value; }
	XString Key;
	XString Value;
};

/** Parse command line options and parameters into a Key-Value vector, and a string vector, respectively.

Anything that begins with a minus or double minus is an 'option', and these go into the 'options' array.
We use a vector for options and not a map because we want to be able to handle more than 1 instance of a particular key.
It is up to the user to decide how to merge them (or raise an error).
I initially thought a string->string map was better, but I have subsequently realized that one always wants to scan
over all of your options anyway, to verify their correctness. So you end up looping over them all anyway, and a
map really gives you nothing in that regard. It only removes the ability to have more than 1 value for any key.

Anything that doesn't start with a minus is a 'parameter', and these go into the 'params' array.

Blank strings are ignored.

-yes=true   ==> options["yes"] = "true"
--yes=true  ==> options["yes"] = "true"
--force     ==> options["force"] = ""

Either 'params' or 'options' maybe NULL, in which case they will not be populated.

**/
PAPI void ParseCmdLineOptions( int n, const XString* opt, dvect<XString>* params, dvect<XStringKV>* options );
PAPI void ParseCmdLineOptionsARGV( int argc, wchar_t** argv, dvect<XString>* params, dvect<XStringKV>* options );	// Don't make argv const - The C spec doesn't have const in there
PAPI void ParseCmdLineOptionsARGV( int argc, char** argv, dvect<XString>* params, dvect<XStringKV>* options );		// Don't make argv const - The C spec doesn't have const in there

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Things that I use to make my C++ test suites more concise
template< typename TVect >
void ParseInts( const char* s, TVect& v )
{
	dvect<XStringA> t;
	Split( XStringA(s), ' ', t, true );
	for ( int i = 0; i < t.size(); i++ )
		v += _atoi64( t[i] );
}

// Example:
// IntArrayEQ( Query( "rowid < 3" ), "0 1 2" );
template< typename TVect >
bool IntArrayEQ( const TVect& v, const char* s )
{
	TVect verify;
	ParseInts( s, verify );
	if ( v.size() != verify.size() ) return false;
	for ( int i = 0; i < verify.size(); i++ )
		if ( verify[i] != v[i] ) return false;
	return true;
}


// This is stupid and brute force. If you need something decent, write it.
template< typename T >
void LongestCommonSubstring( const T* a, const T* b, intp& start_a, intp& start_b, intp& length )
{
	intp best = -1;
	for ( intp i = 0; a[i]; i++ )
	{
		for ( intp j = 0; b[j]; j++ )
		{
			intp k = 0;
			for ( ; b[j+k] && b[j+k] == a[i+k]; k++ ) {}
			if ( k > best )
			{
				start_a = i;
				start_b = j;
				length = k;
				best = k;
			}
		}
	}
}

// Splits lines, allowing  \r\n  \n  \r
// The lines sent to consume include the newline characters. This means that you can
//   reconstruct the file perfectly by concatenating all of the characters sent to consume.
// consume is only called if length is positive (non-zero)
// Return false from 'consume' to stop iteration
// The return value of SplitLines is true if consume always returned true (or the string is empty)
PAPI bool SplitLines( const char* src, uintp maxlen, bool (*consume)( void* cx, const char* start, intp len ), void* cx );
PAPI bool SplitLines( const wchar_t* src, uintp maxlen, bool (*consume)( void* cx, const wchar_t* start, intp len ), void* cx );

PAPI void SplitLines( const char* src, uintp maxlen, podvec<XStringA>& lines );
PAPI void SplitLines( const wchar_t* src, uintp maxlen, podvec<XStringW>& lines );
PAPI void SplitLines( const XStringA& src, podvec<XStringA>& lines );
PAPI void SplitLines( const XStringW& src, podvec<XStringW>& lines );

// Compares strings after trimming whitespace
PAPI int wcscmp_trimmed(  const wchar_t* a, const wchar_t* b );									
PAPI int wcsicmp_trimmed( const wchar_t* a, const wchar_t* b );									
PAPI int wcscmp_trimmed(  const wchar_t* a, size_t a_size, const wchar_t* b, size_t b_size );	
PAPI int wcsicmp_trimmed( const wchar_t* a, size_t a_size, const wchar_t* b, size_t b_size );	

PAPI int strcmp_trimmed(  const char* a, const char* b );									
PAPI int strcmpi_trimmed( const char* a, const char* b );									
PAPI int strcmp_trimmed(  const char* a, size_t a_size, const char* b, size_t b_size );	
PAPI int strcmpi_trimmed( const char* a, size_t a_size, const char* b, size_t b_size );	
