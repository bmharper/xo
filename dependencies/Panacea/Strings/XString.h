#ifndef ABCORE_INCLUDED_XSTRING_H
#define ABCORE_INCLUDED_XSTRING_H

#define XSTRING_DEFINED (1)

#include <string>
#include <stdarg.h>
#include "../warnings.h"
#include "../Other/lmDefs.h"
#include "../HashTab/HashFunctions.h"
#include "../murmur3/MurmurHash3.h"
#include "../fhash/fhashtable.h"

#ifdef _MANAGED
#if _MSC_VER >= 1400
#include <vcclr.h>
#endif
#endif

// nss: natural string sort.
// Without consideration from Michael Herf.
inline int nss_tlower(int b)
{
	if (b >= 'A' && b <= 'Z') return b - 'A' + 'a';
	return b;
}

inline int nss_isnum(int b)
{
	if (b >= '0' && b <= '9') return 1;
	return 0;
}

template< typename T >
inline int nss_parsenum(T *&a)
{
	int result = *a - '0';
	++a;

	while (nss_isnum(*a)) {
		result *= 10;
		result += *a - '0';
		++a;
	}

	--a;
	return result;
}

template< typename T >
inline int nss_TStringCompare(const T *a, const T *b)
{
	if (a == b) return 0;

	if (a == NULL) return -1;
	if (b == NULL) return 1;

	while (*a && *b) {

		int a0, b0;	// will contain either a number or a letter

		if (nss_isnum(*a)) {
			a0 = nss_parsenum(a) + 256;
		} else {
			a0 = nss_tlower(*a);
		}
		if (nss_isnum(*b)) {
			b0 = nss_parsenum(b) + 256;
		} else {
			b0 = nss_tlower(*b);
		}

		if (a0 < b0) return -1;
		if (a0 > b0) return 1;

		++a;
		++b;
	}

	if (*a) return 1;
	if (*b) return -1;

	return 0;
}

inline int nss_StringCompare(const char *a, const char *b)			{ return nss_TStringCompare(a,b); }
inline int nss_StringCompare(const wchar_t *a, const wchar_t *b)	{ return nss_TStringCompare(a,b); }

namespace AbCore
{
	template< typename _tchar >
	size_t TStrLen( const _tchar* str, UINT32 max_chars = -1 )
	{
		if ( str == NULL ) return 0;
		UINT32 i = 0;
		while ( i < max_chars && str[i] != 0 ) i++;
		return i;
	}
}

// These functions are here so that XString's UpCase/LowCase functionality can be duplicated externally.
// Note that doing a table for the most common characters would probably be faster for large strings.
template< typename _tchar >
void XString_MakeUpCase( _tchar* str, int maxlen = INTMAX )
{
	if ( str == NULL ) return;
	for ( int i = 0; str[i] != 0 && i < maxlen; i++ ) 
	{
		if ( str[i] >= 'a' && str[i] <= 'z' )
			str[i] += 'A' - 'a';
	}
}

template< typename _tchar >
void XString_MakeLowCase( _tchar* str, int maxlen = INTMAX )
{
	if ( str == NULL ) return;
	for ( int i = 0; str[i] != 0 && i < maxlen; i++ )
	{
		if ( str[i] >= 'A' && str[i] <= 'Z' )
			str[i] -= 'A' - 'a';
	}
}

template< typename _tchar >
int XString_Compare( _tchar* a, _tchar* b, int maxlen = INTMAX )
{
	if ( a == NULL && b == NULL ) return 0;
	if ( a == NULL ) return -1;
	if ( b == NULL ) return 1;
	for ( int i = 0; i < maxlen; i++ )
	{
		int diff = a[i] - b[i];
		if ( diff != 0 ) return diff;
		// we do not need to check whether b[i] == 0, because the above line asserts that b[i] = 0 ==> a[i] = 0
		if ( a[i] == 0 ) break;
	}
	return 0;
}

template< typename _tchar >
int XString_CompareNoCase( _tchar* a, _tchar* b, int maxlen = INTMAX )
{
	if ( a == NULL && b == NULL ) return 0;
	if ( a == NULL ) return -1;
	if ( b == NULL ) return 1;
	for ( int i = 0; i < maxlen; i++ )
	{
		int cha = a[i];
		int chb = b[i];
		if ( cha >= 'A' && cha <= 'Z' ) cha -= 'A' - 'a';
		if ( chb >= 'A' && chb <= 'Z' ) chb -= 'A' - 'a';
		int diff = cha - chb;
		if ( diff != 0 ) return diff;
		// we do not need to check whether b[i] == 0, because the above line asserts that b[i] = 0 ==> a[i] = 0
		if ( a[i] == 0 ) break;
	}
	return 0;
}

/** String.
Storage is like a normal char*, except that we have an additional 
variable, 'capacity', that stores the number of bytes we've allocated.
We don't generally lower capacity, except in the case of 
GetBuffer(), ReleaseBuffer(), and Shrink().
The storage is classical, with a null terminator always.
The [] operator can be used to manipulate the contents.

Using this as a StringBuilder
-----------------------------

Generally, just use the += operator. This will ensure that the capacity grows exponentially.
You might also want to use Reserve on an empty string and then use += operator if you know the max size up front.

**/
template< typename achar >
class XStringT
{
public:

	// It's never worth it allocating less than 8 bytes on the Windows heap.
	static const int MinAllocChars = 8 / sizeof(achar);

	typedef achar TChar;

	XStringT();
	XStringT(const XStringT &b);
	XStringT(const std::basic_string<achar> &b);			// To convert reliably back to std::string, use the function Std()
	XStringT(const achar* b, int max_chars = -1);
	XStringT(achar b);
	~XStringT();

	static XStringT Exact( const achar* s, int len );		// Use when you have null characters in your string
	void SetExact( const achar* s, int len );				// Use when you have null characters in your string

	/** @name Performance Helpers
	Helpers to avoid unnecessary heap allocs.
	Temporary strings and intended to be READ ONLY!
	**/
	//@{ 
	void MakeTemp( const achar* b, int max_chars = -1 )
	{
		int len = (int) AbCore::TStrLen(b, max_chars);
		ASSERT( b == NULL || b[len] == 0 ); // otherwise we're violating the expected condition that XString is null terminated
		ForceInternals( const_cast<achar*>(b), len+1, len );
	}
	void DestroyTemp()
	{
		ForceInternals( NULL, 0, 0 );
	}
	//@}

	static const int SmallStringThreshold = 32;

//#ifdef _AFX
//	XStringT( const CString &b );
//#endif

	/** Forces the string to be null. 
	This can be used if the string is unioned with other variables.
	Invoke this before the string is going to be destroyed. Try to avoid this of course.
	**/
	void			ForceNull();

	/** Free our internal storage, and set internal pointer to null, but leave capacity and length untouched.
	This is intended to be used in conjunction with ForceInternals().
	This is a helper function for using the string in a custom memory-managed environment.
	**/
	void			DeAlloc();

	/** Forcibly set our 3 internal fields.
	This is intended to be used in conjunction with DeAlloc().
	This is a helper function for using the string in a custom memory-managed environment.
	**/
	void			ForceInternals( achar* str, int capacity, int length );

	/// Morally equivalent to *this = XString(str, length)
	void			Set( const achar* str, int max_chars = -1 ) { Clear(); FromConstStr( str, max_chars ); }

	/** Set the string to the specified value, but do not shrink capacity (grow it if necessary though).
	@param str A string of characters, not necessarily null terminated.
	@param length Length of @a str. This is mandatory.
	**/
	void			SetNoShrink( const achar* str, int length );

	/** Return the size of the string buffer.
	This is not intended for everyday usage. It is for performance critical applications.
	@return The number of characters allocated (including space for the null terminator).
	**/ 
	int				GetCapacity() const { return capacity; }

	/** Returns the raw buffer.
	This is not intended for everyday usage. It is for performance critical applications.
	@return The string buffer.
	**/
	achar*			GetRawBuffer()				{ return str; }
	const achar*	GetRawBufferConst() const { return str; }

	void			Clear() { Resize(0); }

	/** Leave our buffer unchanged, but make our size zero. 
	I created this in order to use the string as a temporary string building buffer.
	**/
	void			ClearNoAlloc()
	{
		length = 0;
		if ( str ) str[0] = 0;
	}

	achar			Right() const;
	achar			Left() const;
	XStringT		Right( int n ) const;
	XStringT		Left( int n ) const;
	XStringT 		UpCase() const;
	XStringT 		LowCase() const;
	void			MakeUpCase();
	void			MakeLowCase();
	XStringT 		Mid( int start, int count = -1 ) const;
	void 			Erase( int start, int length );
	void 			Insert( int pos, const XStringT& s );
	void 			Insert( int pos, achar ch );
	int 			FindC( const achar* s, int start = 0 ) const;			// This ought to be named Find(), but I added it during crunch time and didn't want any disruptions. BMH
	int 			Find( const XStringT& s, int start = 0 ) const;
	int 			Find( achar ch, int start = 0 ) const;
	int 			FindOneOf( const XStringT& characters, int start = 0 ) const;
	bool			StartsWith( const achar* s ) const;
	bool			StartsWith( const XStringT& s ) const					{ return Find( s ) == 0; }
	bool			StartsWith( achar ch ) const							{ return Length() > 0 && str[0] == ch; }
	bool			EndsWith( const XStringT& s ) const						{ auto rv = ReverseFind( s ); return rv != -1 && rv == Length() - s.Length(); }
	bool			EndsWith( achar ch ) const								{ return Length() > 0 && str[ Length() - 1 ] == ch; }
	bool			Contains( const XStringT& s, int start = 0 ) const		{ return Find( s, start ) >= 0; }
	bool			Contains( achar ch, int start = 0 ) const				{ return Find( ch, start ) >= 0; }
	bool			ContainsC( const achar* s, int start = 0 ) const;
	int 			Count( const achar* str ) const;
	int 			Count( const XStringT& str ) const;
	int 			Count( achar ch ) const;
	XStringT		ConvertAllWhiteSpaceToCh32();
	int 			Replace( achar ch, achar repch, int start = 0 );
	int 			Replace( const XStringT& s, const XStringT& reps, int start = 0 );
	int 			ReverseFind( const XStringT& s, int start = -1 ) const;
	int 			ReverseFind( achar ch, int start = -1 ) const;
	XStringT		Trimmed() const							{ XStringT t = *this; t.Trim(); return t; }
	XStringT		Trimmed( const XStringT& chars ) const	{ XStringT t = *this; t.Trim(chars); return t; }
	void 			Trim();
	void 			Trim( const XStringT& chars );
	void 			TrimLeft();
	void 			TrimLeft( const XStringT& chars );
	void 			TrimRight();
	void 			TrimRight( const XStringT& chars );
	
	/// Returns true if every character is between '0' and '9'
	bool			IsIntegerDigitsOrEmpty() const
	{
		for ( int i = 0; i < length; i++ )
			if( str[i] < '0' || str[i] > '9' )
				return false;
		return true;
	}
	
	/** Chops off the last chopChars character from the string
	Leaves an empty string untouched. Will leave the string empty if you chop >= length of the string.
	**/
	void			Chop( int chopChars = 1 );  

	/// Return a copy of the string, with the last n characters removed.
	XStringT		Chopped( int chopChars = 1 ) const;  

	int				Length() const { return length; }

	/** Allocates the indicated amount of space, disregarding all existing data.
	This is a low level alloc, mostly used internally by the string.
	@param len The length of the buffer, excluding space for the null terminator.
	If the string's capacity is already large enough to hold \a len characters, then no action
	is performed. The string's length member is left untouched.
	**/
	void 			AllocLL( int len );

	/** Resizes the string to length \a len (adding an extra character for the null terminator), and preserving existing data.
	The length member is set to len. Unless \a len is equal to the existing Length(), Resize() will always reallocate the string's storage.
	Calling Resize with \a len zero clears and resets the string.
	**/
	void 			Resize( int len );

	/// Only valid if the string is empty. Sets capacity to len + 1.
	void 			Reserve( int len );

	/** Retrieve a buffer large enough to hold \a len characters (len excluding the null terminator).
	**/
	achar*			GetBuffer( int len );
	void			ReleaseBuffer();
	void			ReleaseBufferSetLen( int len );		// ReleaseBuffer with explicit length. Can be used to store strings with zeros.

	void			AppendExact( const achar* buf, int len );	// Append exactly the indicated number of characters. Null terminator inside input buffer is ignored.

	/** Count the number of matching characters (kicks out as soon as a mismatch is encountered).
	@param startThis Starting character in this string.
	@param startB Starting character in string \a b.
	@return Number of characters matched.
	**/
	int				MatchCount( int startThis, int startB, const XStringT &b ) const;

	/** Count the number of matching characters, starting from the back of each string (kicks out as soon as a mismatch is encountered).
	@param startThis Starting character in this string, zero being the last character.
	@param startB Starting character in string \a b, zero being the last character.
	@return Number of characters matched.
	**/
	int				MatchCountReverse( int startThisRev, int startBRev, const XStringT &b ) const;

	/** Count the number of matching characters, starting from the beginning of each string.
	@return Number of characters matched.
	**/
	int				MatchCount( const XStringT &b ) const { return MatchCount( 0, 0, b ); }

	int				MatchCountNoCase( const achar* z ) const;

	/** Count the number of matching characters, starting from the back of each string.
	@return Number of characters matched.
	**/
	int				MatchCountReverse( const XStringT &b ) const { return MatchCountReverse( 0, 0, b ); }

	/** Get a hash code.
	**/
	int				GetHashCode() const;

	uint32	GetHashCode_Murmur2A( uint32 seed = 0 ) const					{ return MurmurHash2A( str, length * sizeof(achar), seed ); }	// Compute Murmur2A hash
	uint32	GetHashCode_Murmur3_x86_32( uint32 seed = 0 ) const				{ u32 out; MurmurHash3_x86_32( str, length * sizeof(achar), seed, &out ); return out; }
	void	GetHashCode_Murmur3_x86_128( void* out, uint32 seed = 0 ) const	{ MurmurHash3_x86_128( str, length * sizeof(achar), seed, out ); }

	static bool CharEqNoCase( int c1, int c2 );

	bool IsEmpty() const 
	{
		return str == NULL || str[0] == 0;
	}
	bool EqualsNoCase( const XStringT &b ) const;
	bool Equals( const XStringT &b ) const;
	bool operator==( const XStringT &b ) const;
	bool operator!=( const XStringT &b ) const;

	achar Get( int i ) const
	{
		return str[i];
	}

	achar operator[]( int i ) const 
	{
		return str[i];
	}

	achar& operator[]( int i ) 
	{
		return str[i];
	}

	/** Return null-terminated string.
	We never return a null ptr.
	This is a bit of a trick... or a hack if you will.
	Since str == 0 we can return it's address as const char*, thus yielding
	a string whose first character is zero.
	We make the function one line so that it is easier to debug (step over).
	**/
	operator const achar*() const { return str == 0 ? (const achar*) &str : str; }

	// It's a real pity, but unfortunately having this cast operator causes ambiguity in the following assignment
	// XString     x;
	// std::string y;
	// y = x;
	//operator std::basic_string<achar>() const { return str == NULL ? std::basic_string<achar>() : std::basic_string<achar>( str, (size_t) length ); }
	// Instead we have the following function

	// This preserves null characters in the string, which is important if you're using this as a generic data buffer
	std::basic_string<achar> Std() const { return str == NULL ? std::basic_string<achar>() : std::basic_string<achar>( str, (size_t) length ); }

	XStringT& operator=(const XStringT &b);
	XStringT& operator=(const std::basic_string<achar> &b);
	XStringT& operator+=(const XStringT &b);
	XStringT& operator+=(const achar b);

protected:
	void Construct()
	{
		str = NULL;
		length = 0;
		capacity = 0;
	}

	/** Shrink the buffer to the minimum size.
	The length member must be valid before calling this function.
	@param newCap The new capacity of the buffer. If the default value of -1 is provided,
	then the new capacity will be the string's length + 1.
	**/
	void Shrink( int newCap = -1 );

	void GrowBuild( int requiredCapacity );

	void Resize( int len, bool setLengthMinusLen1 );

	int GetHashCode_djb2() const;
	int GetHashCode_sdbm() const;

	void FromConstStr( const achar *b, int max_chars = -1 );
	void UpdateLength();

	achar *str;
	int length;
	int capacity;
};

template< typename achar, size_t size >
class StaticWrapXString
{
public:
	StaticWrapXString( const achar (&stack_storage)[size] )
	{
		ASSERT( stack_storage != NULL );
		Value.ForceInternals( const_cast<achar*>(stack_storage), size, size - 1 );
	}
	~StaticWrapXString()
	{
		Value.ForceNull();
	}

	XStringT<achar> Value;
};

template< typename achar >
XStringT<achar> XStringT<achar>::Exact( const achar* s, int len )
{
	XStringT self;
	self.SetExact( s, len );
	return self;
}

template< typename achar >
void XStringT<achar>::SetExact( const achar* s, int len )
{
	if ( len == 0 ) { Clear(); return; }

	int targetcap = len + 1;
	targetcap = AbcMax(targetcap, MinAllocChars);

	if ( capacity != targetcap )
	{
		capacity = targetcap;
		delete[] str;
		str = new achar[capacity];
	}
	length = len;
	memcpy(str, s, len * sizeof(achar));
	str[len] = 0;
}

// NOTE: I used to have the operands here be 'char' and not 'achar'. When I did this change, it broke
// any code that would do XStringW() + 'x', since the 'x' would not get implicitly converted into a const wchar_t*!
// How that upconversion makes any sense.. I don't know. Anyway.. I used a regex to scan all places in the code, but it's
// quite likely that I missed some. Perhaps I should try adding explicit 'char' functions for XStringW. Hmm.. with a static_assert inside it.. hmm.
// $XSTRING_OPERATOR+_OVERLOAD
// Ok.. so we make this thing specific to each type...
//template<typename achar>	XStringT<achar> operator+( const XStringT<achar> &a, achar b ) { return a + XStringT<achar>(b); }
//template<typename achar>	XStringT<achar> operator+( achar a, const XStringT<achar> &b ) { return XStringT<achar>(a) + b; }

template< typename achar >
XStringT<achar> operator+( const XStringT< achar > &a, const XStringT< achar > &b)
{
	XStringT<achar> r = a;
	r += b;
	return r;
}

template< typename achar >
XStringT<achar> operator+( const XStringT< achar > &a, const achar* b_str )
{
	XStringT<achar> b = b_str;
	XStringT<achar> r = a;
	r += b;
	return r;
}

template< typename achar >
XStringT<achar> operator+( const achar* a_str, const XStringT< achar > &b )
{
	XStringT<achar> r = a_str;
	r += b;
	return r;
}

template< typename achar >
bool operator==( const achar* a_str, const XStringT< achar > &b )
{
	XStringT<achar> a;
	a.MakeTemp( a_str );
	bool res = a == b;
	a.DestroyTemp();
	return res;
}

////////////// const char* ///////////////

template< typename achar >
bool operator==( const XStringT< achar > &a, const achar* b_str )
{
	XStringT<achar> b;
	b.MakeTemp( b_str );
	bool res = a == b;
	b.DestroyTemp();
	return res;
}

template< typename achar >
bool operator!=( const achar* a_str, const XStringT< achar > &b )
{
	XStringT<achar> a;
	a.MakeTemp( a_str );
	bool res = a != b;
	a.DestroyTemp();
	return res;
}

template< typename achar >
bool operator!=( const XStringT< achar > &a, const achar* b_str )
{
	XStringT<achar> b;
	b.MakeTemp( b_str );
	bool res = a != b;
	b.DestroyTemp();
	return res;
}

////////////// std::string ///////////////

template< typename achar >
bool operator==( const std::basic_string<achar>& a_str, const XStringT< achar > &b )
{
	return a_str.length() == b.length && memcmp( a_str.data(), b.str, b.length * sizeof(achar) ) == 0;
}

template< typename achar >
bool operator==( const XStringT< achar > &a, const std::basic_string<achar>& b_str )
{
	return b_str.length() == a.length && memcmp( b_str.data(), a.str, a.length * sizeof(achar) ) == 0;
}

template< typename achar >
bool operator!=( const std::basic_string<achar>& a_str, const XStringT< achar > &b )
{
	return !(a_str == b);
}

template< typename achar >
bool operator!=( const XStringT< achar > &a, const std::basic_string<achar>& b_str )
{
	return !(a == b_str);
}

template< typename achar >
XStringT<achar>::XStringT()
{
	Construct();
}

template< typename achar >
XStringT<achar>::~XStringT()
{
	delete[] str;
}

template< typename achar >
XStringT<achar>::XStringT(achar b)
{
	str = NULL;
	length = 1;
	capacity = 0;
	AllocLL( 4 );
	str[0] = b;
	str[1] = 0;
}

template< typename achar >
XStringT< achar >::XStringT( const XStringT &b )
{
	Construct();
	*this = b;
}

template< typename achar >
XStringT< achar >::XStringT( const achar* b, int max_chars )
{
	FromConstStr( b, max_chars );
}

template< typename achar >
XStringT< achar >::XStringT( const std::basic_string<achar> &b )
{
	Construct();
	SetExact( b.data(), (int) b.length() );
}

/*
#ifdef _AFX
template< typename achar >
XStringT< achar >::XStringT( const CString &b )
{
	FromConstStr( b );
}
#endif
*/

template< typename achar >
void XStringT< achar >::ForceNull()
{
	str = NULL;
	capacity = 0;
	length = 0;
}

template< typename achar >
void XStringT< achar >::DeAlloc()
{
	delete[] str;
	str = NULL;
}

template< typename achar >
void XStringT< achar >::ForceInternals( achar* nstr, int ncapacity, int nlength )
{
	str = nstr;
	capacity = ncapacity;
	length = nlength;
}

template< typename achar >
void XStringT< achar >::SetNoShrink( const achar* nstr, int nlength )
{
	ASSERT( nlength >= 0 );
	if ( nlength + 1 > capacity )
	{
		Resize( nlength );
	}
	length = nlength;
	memcpy( str, nstr, nlength * sizeof(achar) );
	if ( str )
		str[ length ] = 0;
}


template< typename achar >
void XStringT< achar >::FromConstStr( const achar *b, int max_chars )
{
	str = NULL;
	capacity = 0;
	length = 0;
	if ( b != NULL && max_chars != 0 )
	{
		size_t len = AbCore::TStrLen(b, max_chars);
		if ( len > 0 )
		{
			AllocLL( (int) (len + 1) );
			__analysis_assume( str != NULL );
			memcpy( str, b, len * sizeof(achar) );
			str[len] = 0;
		}
		length = (int) len;
	}
}

template< typename achar >
void XStringT< achar >::Shrink( int newCap )
{
	if ( length == 0 )
	{
		Resize(0);
		return;
	}
	if ( newCap < 0 ) newCap = length + 1;
	// assume that memory does not get allocated in less than 8 byte chunks. This is true for 
	// XP-32's low fragmentation heap.
	int rnd = 8 / sizeof(achar);
	int newCapRounded = ((newCap + rnd-1) / rnd) * rnd;
	int capRounded = ((capacity + rnd-1) / rnd) * rnd;
	ASSERT( newCapRounded <= capRounded );
	if ( newCapRounded != capRounded )
	{
		achar* newStr = new achar[ newCap ];
		capacity = newCap;
		memcpy( newStr, str, (length + 1) * sizeof(achar) );
		delete[] str;
		str = newStr;
	}
}


template< typename achar >
void XStringT< achar >::AllocLL( int ncap )
{
	if ( ncap <= capacity )
		return;
	delete []str;
	str = NULL;
	if ( ncap > 0 )
		str = new achar[ncap];
	capacity = ncap;
}

template< typename achar >
void XStringT< achar >::GrowBuild( int requiredCapacity )
{
	// grow for potentially large string building operations
	int rcap = requiredCapacity;
	if ( capacity < rcap )
	{
		// grow at vector pace (2x) for large string build operations
		if ( capacity > SmallStringThreshold )
		{
			rcap = AbcMax( rcap, capacity * 2 );
		}
		else
		{
			// round up to nearest 8 bytes, because heap allocs don't really grain finer than that
			int ch8 = 8 / sizeof(achar);
			rcap = ch8 * ((rcap + ch8 - 1) / ch8);
		}
		Resize( rcap, false );
	}
}

template< typename achar >
void XStringT< achar >::Reserve( int len )
{
	if ( str ) { ASSERT(false); return; }
	len += 1;
	str = new achar[len];
	capacity = len;
	ASSERT( length == 0 );
}

template< typename achar >
void XStringT< achar >::Resize( int len )
{
	if ( len == 0 )
	{
		delete[] str; str = NULL;
		capacity = 0;
		length = 0;
		return;
	}

	Resize( len + 1, true );
}

// I do not understand the warning here. BMH 2013-03-15. VS 2010.
DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_WRITING_INVALID_BYTES

template< typename achar >
void XStringT< achar >::Resize( int len, bool setLengthMinusLen1 )
{
	if ( len == length + 1 ) return;
	achar* newstr = NULL;
	// allocate a minimum of 8 bytes, because this is the smallest unit of the LFH
	int ch8 = 8 / sizeof(achar);
	int cap = (len == 0) ? 0 : AbcMax( len, ch8 );
	if ( len > 0 )
	{
		newstr = new achar[ cap ];
		newstr[ len - 1 ] = 0;
		if ( capacity > 0 )
			memcpy( newstr, str, AbcMin(len, capacity) * sizeof(achar) );
	}
	capacity = cap;
	delete []str;
	str = newstr;
	if ( setLengthMinusLen1 )
	{
		if ( len > 0 ) length = len - 1;
		else length = 0;
	}
}

DISABLE_CODE_ANALYSIS_WARNINGS_POP

template< typename achar >
achar* XStringT< achar >::GetBuffer( int len )
{
	// for null terminator
	len++; 
	if ( len > capacity ) AllocLL( len );
	// If we are empty, then add a default null terminator, 
	// in case the user of the buffer writes nothing to it.
	// If he writes nothing to it, then we have no null terminator for 
	// UpdateLength() that is called from ReleaseBuffer().
	str[length] = 0;
	// also add a null terminator to the end, so that if the user does modify the first 
	// bit, but does not add his own null terminator, then we at least still have this.
	str[len - 1] = 0;
	return str;
}

template< typename achar >
void XStringT< achar >::UpdateLength()
{
	if ( str == NULL ) length = 0;
	else length = (int) AbCore::TStrLen(str);
}

template< typename achar >
void XStringT< achar >::ReleaseBuffer()
{
	UpdateLength();
	Shrink();
}

template< typename achar >
void XStringT< achar >::ReleaseBufferSetLen( int len )
{
	str[len] = 0;
	length = len;
}

template< typename achar >
void XStringT< achar >::AppendExact( const achar* buf, int len )
{
	for ( int i = 0; i < len; i++ )
		*this += buf[i];
}

template< typename achar >
XStringT< achar >& XStringT< achar >::operator=( const std::basic_string<achar> &b )
{
	auto len = b.length();
	if ( len == length )	memcpy(str, b.data(), len * sizeof(achar));
	else					SetExact( b.data(), (int) b.length() );
	return *this;
}

template< typename achar >
XStringT< achar >& XStringT< achar >::operator=( const XStringT &b )
{
	SetExact( b.str, b.length );
	return *this;
}

template< typename achar >
XStringT< achar >& XStringT< achar >::operator+=( const XStringT &b )
{
	if ( b.length == 0 )
	{
		return *this;
	}
	int blen = b.length;
	int reqCap = length + blen + 1;
	GrowBuild( reqCap );
	memcpy( str + length, b.str, (blen + 1) * sizeof(achar) );
	length += blen;
	return *this;
}

template< typename achar >
XStringT< achar >& XStringT< achar >::operator+=( const achar b )
{
	//ASSERT( b != 0 ); -- This is useful when building up a packed array of null terminated strings
	int len = length;
	GrowBuild( len + 2 );
	str[len] = b;
	str[len+1] = 0;
	length++;
	return *this;
}

template< typename achar >
int XStringT< achar >::GetHashCode() const
{
	return GetHashCode_sdbm();
}

template< typename achar >
int XStringT< achar >::GetHashCode_djb2() const
{
	if (str == NULL) return 0;
	// SYNC-DJB2
	// This was copied from http://www.cs.yorku.ca/~oz/hash.html
	// Two names mentioned on that page are djb2 and sdbm
	// This is djb2.
	unsigned int hash = 5381;
	for ( int i = 0; i < length; i++ )
	{
		hash = ((hash << 5) + hash) + (unsigned int) str[i]; // hash * 33 + c
	}
	return (int) hash;
}

template< typename achar >
int XStringT< achar >::GetHashCode_sdbm() const
{
	if (str == NULL) return 0;
	// SYNC-SDBM
	// This was copied from http://www.cs.yorku.ca/~oz/hash.html
	// Two names mentioned on that page are djb2 and sdbm
	// This is sdbm.
	unsigned int hash = 0;
	for ( int i = 0; i < length; i++ )
	{
		// hash(i) = hash(i - 1) * 65539 + str[i]
		hash = (unsigned int) str[i] + (hash << 6) + (hash << 16) - hash;
		i++;
	}
	return (int) hash;
}

template< typename achar >
bool XStringT< achar >::CharEqNoCase( int c1, int c2 )
{
	c1 = (c1 >= 'A' && c1 <= 'Z') ? c1 + 'a' - 'A' : c1;
	c2 = (c2 >= 'A' && c2 <= 'Z') ? c2 + 'a' - 'A' : c2;
	return c1 == c2;
}

template< typename achar >
bool XStringT< achar >::EqualsNoCase( const XStringT &b ) const
{
	if (length != b.length) return false;
	for ( int i = 0; i < length; i++ )
	{
		if ( !CharEqNoCase( str[i], b.str[i] ) ) return false;
	}
	return true;
}

template< typename achar >
int XStringT< achar >::MatchCountNoCase( const achar* z ) const
{
	int i = 0;
	while ( i < length && z[i] && CharEqNoCase(str[i], z[i]) ) i++;
	return i;
}

template< typename achar >
bool XStringT< achar >::StartsWith( const achar* s ) const
{
	int i = 0;
	for ( ; i < length && s[i]; i++ )
	{
		if ( str[i] != s[i] ) return false;
	}
	if ( s[i] != 0 && i == length ) return false;
	return true;
}

template< typename achar >
bool XStringT< achar >::Equals(const XStringT &b) const
{
	if (length != b.length) return false;
	for ( int i = 0; i < length; i++ )
	{
		if ( str[i] != b.str[i] ) return false;
	}
	return true;
}

template< typename achar >
bool XStringT< achar >::operator==(const XStringT &b) const
{
	return Equals(b);
}

template< typename achar >
bool XStringT< achar >::operator!=(const XStringT &b) const
{
	return !Equals(b);
}

template< typename achar >
void XStringT< achar >::Trim()
{
	TrimLeft();
	TrimRight();
}

template< typename achar >
void XStringT< achar >::Trim( const XStringT& chars )
{
	TrimLeft( chars );
	TrimRight( chars );
}

template< typename achar >
void XStringT< achar >::TrimLeft()
{
	const achar ws[] = {32,9,13,10,0};
	StaticWrapXString<achar,5> chars( ws );
	TrimLeft( chars.Value );
}

template< typename achar >
int XStringT< achar >::Count( const achar* str ) const
{
	XStringT<achar> t;
	t.MakeTemp( str );
	int count = Count(t);
	t.DestroyTemp();
	return count;
}

template< typename achar >
int XStringT< achar >::Count( const XStringT& str ) const
{
	int len = Length();
	int sublen = str.Length();
	int count = 0;
	for ( int i = 0; i < len; i++ )
	{
		int pos = Find( str, i );
		if ( pos != -1 )
		{
			count++;
			i = pos + sublen - 1;
		}
		else
		{
			break;
		}
	}
	return count;
}

template< typename achar >
int XStringT< achar >::Count( achar ch ) const
{
	int len = Length();
	int count = 0;
	for ( int i = 0; i < len; i++ )
	{
		if ( str[i] == ch ) count++;
	}
	return count;
}

template< typename achar >
void XStringT< achar >::TrimLeft( const XStringT& chars )
{
	if ( str == NULL ) return;
	int i;
	for ( i = 0; i < length; i++ ) 
	{
		if ( chars.Find(str[i]) == -1 )
			break;
	}
	if ( i > 0 ) 
	{
		memmove( str, str + i, ((length + 1) - i) * sizeof(achar) );
		length -= i;
	}
}


template< typename achar >
void XStringT< achar >::TrimRight()
{
	achar ws[] = {32,9,13,10,0};
	StaticWrapXString<achar,5> chars( ws );
	TrimRight( chars.Value );
}

template< typename achar >
void XStringT< achar >::TrimRight( const XStringT& chars )
{
	if ( str == NULL ) return;
	int last_non_white = -1;
	int i;
	for ( i = length - 1; i >= 0; i-- ) 
	{
		if ( chars.Find(str[i]) == -1 )
		{
			last_non_white = i;
			break;
		}
	}
	if ( last_non_white != length - 1 ) 
	{
		length -= (length - 1 - last_non_white);
		str[last_non_white + 1] = 0;
	}
}

template< typename achar >
void XStringT< achar >::Chop( int chopChars )
{
	if ( chopChars >= length )
	{
		Clear();
	}
	else
	{
		while ( chopChars > 0 )
		{
			length--;
			str[length] = 0;
			chopChars--;
		}
	}
}

template< typename achar >
XStringT< achar > XStringT< achar >::Chopped( int chopChars ) const
{
	if ( chopChars >= length )
	{
		return XStringT();
	}
	else
	{
		XStringT c = *this;
		c.Chop( chopChars );
		return c;
	}
}

template< typename achar >
int XStringT< achar >::FindC( const achar* s, int start ) const
{
	XStringT tmp;
	tmp.MakeTemp( s, (int) AbCore::TStrLen(s) );
	int r = Find( tmp, start );
	tmp.DestroyTemp();
	return r;
}

template< typename achar >
bool XStringT< achar >::ContainsC( const achar* s, int start ) const
{
	XStringT tmp;
	tmp.MakeTemp( s, (int) AbCore::TStrLen(s) );
	bool r = Contains( tmp, start );
	tmp.DestroyTemp();
	return r;
}

template< typename achar >
int XStringT< achar >::Find( const XStringT& s, int start ) const
{
	int sl = s.Length();
	if ( sl == 0 || Length() == 0 ) return -1;
	start = AbcMax( start, 0 );

	for ( int i = start; i < 1 + length - sl; i++ ) 
	{
		if ( memcmp(s.str, str + i, sizeof(achar) * sl) == 0 )
			return i;
	}
	return -1;
}

template< typename achar >
int XStringT< achar >::Find( achar ch, int start ) const
{
	if ( str == NULL ) return -1;
	start = AbcMax( start, 0 );
	for ( int i = start; i < length; i++ ) 
	{
		if ( str[i] == ch ) return i;
	}
	return -1;
}

template< typename achar >
int XStringT< achar >::FindOneOf( const XStringT& characters, int start ) const
{
	if ( str == NULL || characters.length < 1 ) return -1;
	start = AbcMax( start, 0 );
	for ( int i = start; i < length; i++ )
	{
		for ( int j = 0; j < characters.length; j++ )
		{
			if ( str[i] == characters[j] ) return i;
		}
	}
	return -1;
}


template< typename achar >
XStringT< achar > XStringT< achar >::ConvertAllWhiteSpaceToCh32()
{
	XStringT ws_str;
	if ( length == 0 ) return ws_str;
	char white_space_chars[4] = { 9, 13, 10 , 0 };
	for ( int i = 0; i < length; i++ ) 
	{
		if ( str[i] < 32 ) 
		{
			char c = 0;
			for ( ; c < 4; c++ )
			{
				if ( str[i] == white_space_chars[c] )
					{
						ws_str += ' ';
						break;
					}
			}
			
			if ( c == 4 )
				ws_str += str[i];		
		}
		else
		{
			ws_str += str[i];
		}
	}
	return ws_str;
}

/**
Replaces instances of ch with repch, starting at start.
Returns the number of items replaced.
**/
template< typename achar >
int XStringT< achar >::Replace( achar ch, achar repch, int start )
{
	if ( length == 0 ) return 0;
	int count = 0;
	start = AbcMax( start, 0 );
	for ( int i = start; i < length; i++ ) 
	{
		if ( str[i] == ch ) 
		{
			str[i] = repch;
			count++;
		}
	}
	return count;
}

/**
Replaces instances of s with reps, starting at start.
Returns the number of items replaced.
**/
template< typename achar >
int XStringT< achar >::Replace( const XStringT& s, const XStringT& reps, int start )
{
	if ( length == 0 ) return 0;
	int alen = s.Length();
	int blen = reps.Length();
	if ( alen == 0 ) return 0;

	start = AbcMax( start, 0 );

	int count = 0;
	for ( int i = start; i < length - alen + 1; i++ ) 
	{
		if ( memcmp(str + i, s.str, alen * sizeof(achar)) == 0 )
		{
			if ( alen == blen )
				memcpy( str + i, reps.str, sizeof(achar) * blen );
			else 
			{
				Erase( i, alen );
				Insert( i, reps );
				i += blen - 1;
			}
			count++;
		}
	}
	return count;
}

/**
Erases from start and with length \a len.
**/
template< typename achar >
void XStringT< achar >::Erase( int start, int len )
{
	// clip
	start = AbcMax( start, 0 );
	if ( start >= capacity ) return;
	if ( start + len >= capacity ) len = capacity - start;

	length -= len;
	memmove( &str[start], &str[start + len], (capacity - len - start) * sizeof(achar) );
}

/**
Inserts BEFORE position pos.
**/
template< typename achar >
void XStringT< achar >::Insert( int pos, const XStringT& s )
{
	int slen = s.Length();
	int len = Length();
	if ( slen == 0 || pos > capacity ) return;
	if ( len + slen >= capacity )
		Resize( capacity + slen + 4, false );
	
	memmove( &str[pos + slen], &str[pos], sizeof(achar) * (capacity - slen - pos - 1) );
	memcpy( &str[pos], s.str, slen * sizeof(achar) );
	length += s.Length();
	str[length] = 0;
}

/**
Inserts BEFORE position pos.
**/
template< typename achar >
void XStringT< achar >::Insert( int pos, achar ch )
{
	int len = Length();
	if (pos > capacity) return;
	if (len + 1 >= capacity)
		Resize( capacity + 4, false );
	
	memmove( &str[pos + 1], &str[pos], sizeof(achar) * (capacity - pos - 1) );
	str[pos] = ch;
	length++;
	str[length] = 0;
}

template< typename achar >
int XStringT< achar >::ReverseFind( achar ch, int start ) const
{
	if ( str == NULL ) return -1;
	if ( start < 0 ) start = length - 1;
	for ( int i = start; i >= 0; i-- ) 
	{
		if ( str[i] == ch ) return i;
	}
	return -1;
}

template< typename achar >
int XStringT< achar >::ReverseFind( const XStringT& s, int start ) const
{
	int sl = s.length;
	if ( sl == 0 || sl > length ) return -1;

	if ( start < 0  )	start = length - sl;
	else				start = AbcMin(start, length - sl);

	for ( int i = start; i >= 0; i-- ) 
	{
		if ( memcmp(str + i, s.str, sizeof(achar) * sl) == 0 )
			return i;
	}
	return -1;
}

template< typename achar >
XStringT< achar >	XStringT< achar >::Right( int n ) const
{
	XStringT r;
	n = AbcMin( n, Length() );
	if ( n == 0 ) return r;
	r.AllocLL( n + 1 );
	int le = Length();
	memcpy( r.str, str + le - n, n * sizeof(achar) );
	r.str[n] = 0;
	r.length = n;
	return r;
}

template< typename achar >
XStringT< achar >	XStringT< achar >::Left( int n ) const
{
	XStringT r;
	n = AbcMin( n, Length() );
	if ( n == 0 ) return r;
	r.AllocLL( n + 1 );
	memcpy( r.str, str, n * sizeof(achar) );
	r.str[n] = 0;
	r.length = n;
	return r;
}


// returns a COPY
template< typename achar >
XStringT< achar > XStringT< achar >::UpCase() const
{
	XStringT r = *this;
	XString_MakeUpCase( r.str, r.length );
	return r;
}

// returns a COPY
template< typename achar >
XStringT< achar > XStringT< achar >::LowCase() const
{
	XStringT r = *this;
	XString_MakeLowCase( r.str, r.length );
	return r;
}

template< typename achar >
void XStringT< achar >::MakeUpCase()
{
	XString_MakeUpCase( str, length );
}

template< typename achar >
void XStringT< achar >::MakeLowCase()
{
	XString_MakeLowCase( str, length );
}

template< typename achar >
XStringT< achar > XStringT< achar >::Mid( int start, int count ) const
{
	XStringT r;
	if ( count == 0 ) return r;
	start = AbcMax( start, 0 );

	if ( count == -1 || start + count > length )
		count = length - start;

	if ( count <= 0 ) return r;

	r.AllocLL( count + 1 );
	memcpy( r.str, str + start, sizeof(achar) * count );
	r.str[count] = 0;
	r.length = count;
	return r;
}

template< typename achar >
achar XStringT< achar >::Right() const
{
	if ( length == 0 ) return 0;
	return str[length - 1];
}

template< typename achar >
achar XStringT< achar >::Left() const
{
	if ( length == 0 ) return 0;
	return str[0];
}


/*
XStringT::operator const std::string() const
{
	if ( str == NULL ) return std::string();
	return str;
}*/

#if _MFC_VER >= 0x700
// This one just increases ambiguity
/*XStringT::operator CString() const
{
	return CString( str );
}*/
#endif


template< typename achar >
int XStringT< achar >::MatchCount( int startThis, int startB, const XStringT &b ) const
{
	int i = startThis;
	int j = startB;
	int c = 0;
	while ( i < length && j < b.length )
	{
		if ( str[i] != b.str[j] ) break;
		c++;
		i++;
		j++;
	}
	return c;
}

template< typename achar >
int XStringT< achar >::MatchCountReverse( int startThisRev, int startBRev, const XStringT &b ) const
{
	int i = length - startThisRev - 1;
	int j = b.length - startBRev - 1;
	int c = 0;
	while ( i >= 0 && j >= 0 )
	{
		if ( str[i] != b.str[j] ) break;
		c++;
		i--;
		j--;
	}
	return c;
}


// no ability to collapse splitters here
template< typename TStr >
TStr TokenFromSplit( const TStr& string, int ch, int token )
{
	int len = string.Length();
	int start = 0;
	int tok = 0;
	int i = 0;
	for ( ; i < len; i++ )
	{
		if ( string[i] == ch )
		{
			if ( tok == token ) break;
			tok++;
			start = i + 1;
		}
	}
	if ( tok != token ) return TStr();
	return string.Mid( start, i - start );
}

template< typename achar, typename vectype >
int Split( const XStringT< achar >& string, int ch, vectype& res, bool collapseSplitters = false )
{
	if ( string.Length() == 0 ) return 0;
	int added = 0;
	int len = string.Length();
	XStringT<achar> out;
	for ( int i = 0; i < len; i++ )
	{
		if ( string[i] == ch )
		{
			if ( out.Length() > 0 || !collapseSplitters )
			{
				added++;
				res.push_back( out );
				out.Clear();
			}
		}
		else
		{
			out += string[i];
		}
	}
	added++;
	res.push_back( out );
	return added;
}

template< typename achar, typename vectype >
vectype Split( const XStringT< achar >& string, int ch, bool collapseSplitters = false )
{
	vectype r;
	Split( string, ch, r, collapseSplitters );
	return r;
}

// These have XStringA/XStringW overload further down.
template< typename achar >
XStringT< achar > Join( int n, const XStringT< achar >* strings, const achar* joinStr )
{
	XStringT< achar > res;
	intr jlen = AbCore::TStrLen(joinStr);
	for ( int i = 0; i < n; i++ )
	{
		res += strings[i];
		if ( i < n - 1 )
		{
			for ( intr j = 0; j < jlen; j++ )
				res += joinStr[j];
		}
	}
	return res;
}

template< typename achar, typename vec >
XStringT< achar > Join( const vec& strings, const achar* joinStr )
{
	return ::Join( strings.size(), &strings[0], joinStr );
}

class XStringA;
class XStringW;

/// Ascii specialization of XStringT
/**
We have to define a special class for XStringA so that we can have an implicit conversion from wide to ascii,
as well as some other specializations.
**/
class PAPI XStringA: public XStringT< char >
{
public:
	friend class XStringW;
	typedef XStringT< char > Base;
	XStringA() : Base() {}
	XStringA(const XStringT<char> &b) : Base(b) {}
	XStringA(const std::string &b) : Base(b) {}
	XStringA(const char* b, int max_chars = -1) : Base(b, max_chars) {}
	XStringA(char ch) : Base(ch) {}
	//explicit XStringA(wchar_t ch) : Base((char) ch) {}	// this is not good, but it's better than having a XStringA( L'//' ) be interpreted as XStringA( (const char*) 47 )

	XStringA(const std::wstring &b) : Base()
	{
		FromConstWStr( b.c_str() );
	}

	static XStringA FromWideTruncate( const wchar_t* c, int max_chars = -1 )
	{
		XStringA r;
		r.FromConstWStr( c, max_chars );
		return r;
	}

	/// Provided merely for literal compatibility with XStringW, we don't do any translation here.
	static XStringA FromUtf8( const char* c, int max_chars = -1 )
	{
		return XStringA( c, max_chars );
	}

#ifdef _AFX
	XStringA( const CStringA &b ) : Base()
	{
		*this = (const char *) b;
	}
#endif

#ifdef _MANAGED

	// This template will compile only for
	// class SystemString == System::String

#if _MSC_VER >= 1400
	template< class SystemString >
	XStringA( SystemString^ pString ) : Base()
	{
		//stdcli::language::pin_ptr< const wchar_t > psz = PtrToStringChars( pString );
		pin_ptr< const wchar_t > psz = PtrToStringChars( pString );
		FromConstWStr( psz );
	}
#else
	template<class SystemString>
	XStringA( SystemString * pString ) : Base()
	{
		const wchar_t __pin* psz = PtrToStringChars( pString );
		FromConstWStr( psz );
	}
#endif

#endif

	// need to overload functions that return a string of our own type,
	// because XStringT<char> != XStringA.
	char			Right() const { return Base::Right(); }
	char			Left() const { return Base::Left(); }
	XStringA		Right( int n ) const { return Base::Right(n); }
	XStringA		Left( int n ) const { return Base::Left(n); }
	XStringA 		UpCase() const { return Base::UpCase(); }
	XStringA 		LowCase() const { return Base::LowCase(); }
	XStringA 		Mid( int start, int count = -1 ) const { return Base::Mid( start, count ); }

	int Compare( const XStringA& b ) const
	{
		if ( Length() == 0 && b.Length() != 0 ) return -1;
		if ( Length() != 0 && b.Length() == 0 ) return 1;
		if ( Length() == 0 && b.Length() == 0 ) return 0;
		return strcmp( str, b.str );
	}

	int CompareNoCase( const XStringA& b ) const
	{
		if ( Length() == 0 && b.Length() != 0 ) return -1;
		if ( Length() != 0 && b.Length() == 0 ) return 1;
		if ( Length() == 0 && b.Length() == 0 ) return 0;
#ifdef _WIN32
		return _stricmp( str, b.str );
#else
		return strcasecmp( str, b.str );
#endif
	}

	XStringW ToWide() const;
	XStringW ToWideFromUtf8() const;

	XStringA ToAscii() const
	{
		return *this;
	}

	XStringA ToUtf8() const;	

#ifdef _UNICODE
	XStringW ToNative() const;
#else
	XStringA ToNative() const { return ToAscii(); }
#endif

#pragma warning( push )
#pragma warning( disable: 4793 ) // vararg causes native code generation

	/** printf style formatting.
	**/
	void Format( const char* FormatStr, ... )
	{
		char buff[8192];
		
		va_list va;
		
		va_start( va, FormatStr );
	#if _MSC_VER >= 1400
		vsprintf_s( buff, FormatStr, va );
	#else
		vsprintf( buff, FormatStr, va );
	#endif
		va_end( va ); 

		*this = buff;
	}

	/** printf style formatting.
	**/
	static XStringA FromFormat( const char* FormatStr, ... )
	{
		char buff[8192];
		
		va_list va;
		
		va_start( va, FormatStr );
	#if _MSC_VER >= 1400
		vsprintf_s( buff, FormatStr, va );
	#else
		vsprintf( buff, FormatStr, va );
	#endif
		va_end( va ); 
		XStringA s = buff;
		return s;
	}
#pragma warning( pop )


protected:
	void FromConstWStr( const wchar_t* in, int max_chars = -1 )
	{
		int len = (int) wcslen(in);
		if ( max_chars != -1 ) len = _MIN_(len, max_chars);
		if ( len <= 0 )
		{
			Resize(0);
			return;
		}
		Resize( len + 1, false );
		length = len;
		for ( int i = 0; i < len; i++ )
			str[i] = (char) in[i];
		str[len] = 0;
	}

	template<typename TT> TT _MIN_( TT a, TT b ) { return a < b ? a : b; }
};


// $XSTRING_OPERATOR+_OVERLOAD (see comment higher up in this file)
inline XStringA operator+( const XStringA &a, char b ) { return a + XStringA(b); }
inline XStringA operator+( char a, const XStringA &b ) { return XStringA(a) + b; }

////////////// less than ///////////////

inline bool operator<( const XStringA &a, const XStringA &b )
{
	return a.Compare( b ) < 0;
}

////////////// greater than ///////////////

inline bool operator>( const XStringA &a, const XStringA &b )
{
	return a.Compare( b ) > 0;
}

/** Wide specialization of XStringT.
We have to define a special class for XStringW so that we can have an implicit conversion from ascii to wide,
as well as some other specializations.
**/
class PAPI XStringW: public XStringT< wchar_t >
{
public:
	friend class XStringA;

	typedef XStringT< wchar_t > Base;
	XStringW() : Base() {}
	XStringW(const XStringT<wchar_t> &b) : Base(b) {}
	XStringW(const std::wstring &b) : Base(b) {}
	XStringW(const wchar_t* b, int max_chars = -1) : Base(b, max_chars) {}
	//XStringW(char ch) : Base((wchar_t) ch) {}	// without this explicit overload, you end up implicitly converting "char" into a "const wchar_t*". Idiotic implicit conversion.. that one.
	XStringW(wchar_t ch) : Base(ch) {}

	#ifdef _NATIVE_WCHAR_T_DEFINED
	// This is an assumption that is not true on linux... ie that sizeof(wchar_t) == sizeof(unsigned short),
	// but I don't think that _NATIVE_WCHAR_T_DEFINED will ever be defined on linux... I'm not sure though.
	XStringW(const unsigned short* b, int max_chars = -1) : Base( (const wchar_t*) b, max_chars ) {}
	#endif

	XStringW(const char* b, int max_chars = -1 ) : Base() 
	{
		FromConstAStr( b, max_chars );
	}
	XStringW(const std::string &b) : Base()
	{
		FromConstAStr( b.c_str() );
	}

	void SetFromUtf8( const char* utf8, int max_chars = -1 )
	{
		ConvertFromUtf8( utf8, max_chars );
	}

	void SetFromUtf8( const std::string& b )
	{
		ConvertFromUtf8( b.c_str() );
	}

	void SetFromU16LE( const UINT16* b, int max_chars = -1 )
	{
		int len = 0;
		for ( ; (uint) len < (uint) max_chars && b[len]; len++ )
		{}

		if ( len == 0 )
		{
			Clear();
		}
		else
		{
			if ( len + 1 > capacity )
				Resize( len );
			// We SHOULD do UTF16 to UTF32 conversion here on linux
			for ( int i = 0; i < len; i++ )
				str[i] = b[i];
			length = len;
			str[len] = 0;
		}
	}

	static XStringW FromUtf8( const char* utf8, int max_chars = -1 )
	{
		// optimization for the case where our utf8 string contains only < 127
		XStringW ws;
		if ( utf8 != NULL )
		{
			bool needConversion = false;
			int top = max_chars == -1 ? INTMAX : max_chars;
			for ( int p = 0; p < top && utf8[p] != 0; p++ )
			{
				if ( utf8[p] < 0 )
				{
					needConversion = true;
					break;
				}
			}

			if ( needConversion )	ws.ConvertFromUtf8( utf8, max_chars );
			else					ws.FromConstAStr( utf8, max_chars );
		}

		return ws;
	}

#ifdef _AFX
	XStringW( const CStringA &b ) : Base()
	{
		*this = (const char*) b;
	}
	XStringW( const CStringW &b ) : Base()
	{
		*this = (const wchar_t*) b;
	}
#endif

#ifdef _MANAGED

	// This template will compile only for
	// class SystemString == System::String

#if _MSC_VER >= 1400

	template<class SystemString>
	XStringW( SystemString^ pString ) : Base()
	{
		//stdcli::language::pin_ptr< const wchar_t > psz = PtrToStringChars( pString );
		pin_ptr< const wchar_t > psz = PtrToStringChars( pString );
		*this = psz;
	}

#else

	XStringW( SystemString * pString ) : Base()
	{
		const wchar_t __pin* psz = PtrToStringChars( pString );
		*this = psz;
	}

#endif

#endif

	// need to overload functions that return a string of our own type,
	// because XStringT<wchar_t> != XStringW.
	wchar_t			Right() const { return Base::Right(); }
	wchar_t			Left() const { return Base::Left(); }
	XStringW		Right( int n ) const { return Base::Right(n); }
	XStringW		Left( int n ) const { return Base::Left(n); }
	XStringW 		UpCase() const { return Base::UpCase(); }
	XStringW 		LowCase() const { return Base::LowCase(); }
	XStringW 		Mid( int start, int count = -1 ) const { return Base::Mid( start, count ); }

#pragma warning( push )
#pragma warning( disable: 4793 ) // vararg causes native code generation

	void Format( const wchar_t* FormatStr, ... )
	{
		const int MAXSIZE = 4000;
		wchar_t buff[MAXSIZE+1];
		
		va_list va;
		
		va_start( va, FormatStr );
		#ifdef _WIN32
			vswprintf_s( buff, MAXSIZE, FormatStr, va );
		#else
			wcscpy( buff, L"No vswprintf available on this platform" );
			// fine for linux, not for openbsd (4.0): vswprintf( buff, MAXSIZE, FormatStr, va );
		#endif
		va_end( va ); 

		*this = buff;
	}

	static XStringW FromFormat( const wchar_t* FormatStr, ... )
	{
		const int MAXSIZE = 4000;
		wchar_t buff[MAXSIZE+1];
		
		va_list va;
		
		va_start( va, FormatStr );
		#ifdef _WIN32
			vswprintf_s( buff, MAXSIZE, FormatStr, va );
		#else
			wcscpy( buff, L"No vswprintf available on this platform" );
			// fine for linux, not for openbsd (4.0): vswprintf( buff, MAXSIZE, FormatStr, va );
		#endif
		va_end( va ); 

		return XStringW( buff );
	}

#pragma warning( pop )

	int Compare( const XStringW& b ) const
	{
		if ( Length() == 0 && b.Length() != 0 ) return -1;
		if ( Length() != 0 && b.Length() == 0 ) return 1;
		if ( Length() == 0 && b.Length() == 0 ) return 0;
		return wcscmp( str, b.str );
	}

	int CompareNoCase( const XStringW& b ) const
	{
		if ( Length() == 0 && b.Length() != 0 ) return -1;
		if ( Length() != 0 && b.Length() == 0 ) return 1;
		if ( Length() == 0 && b.Length() == 0 ) return 0;
#ifdef _WIN32
		return _wcsicmp( str, b.str );
#else
		return XString_CompareNoCase( str, b.str );
		// can use this on linux, but not on OpenBSD (4.0): return wcscasecmp( str, b.str );
#endif
	}

	static int CompareNoCase( const XStringW& a, const XStringW& b )
	{
		return a.CompareNoCase( b );
	}

	XStringW ToWide() const
	{
		return *this;
	}

	XStringA ToAscii() const
	{
		if ( length == 0 ) return "";
		XStringA asc;
		asc.Resize( length );
		for ( int i = 0; i < length; i++ )
		{
			if ( str[i] < 256 ) asc[i] = (char) str[i];
			else asc[i] = -1;
		}
		return asc;
	}

	XStringA ToUtf8() const;

#ifdef _UNICODE
	XStringW ToNative() const { return ToWide(); }
#else
	XStringA ToNative() const { return ToAscii(); }
#endif

	bool operator==( const char* b ) const
	{
		return EqualsSpecial( b );
	}

	bool operator==( const XStringW& b ) const
	{
		return Equals( b );
	}

	bool operator==( const wchar_t* b ) const
	{
		return Equals( XStringW(b) );
	}

	bool operator!=( const XStringW& b ) const
	{
		return !Equals( b );
	}

	bool operator!=( const char* b ) const
	{
		return !EqualsSpecial( b );
	}

	bool operator!=( const wchar_t* b ) const
	{
		return !Equals( XStringW(b) );
	}

	void ConvertFromUtf8( const char* utf8, int max_chars = -1 );

protected:

	bool EqualsSpecial( const char* b ) const
	{
		XStringA str;
		str.MakeTemp( b );
		bool res = EqualsSpecial( str );
		str.DestroyTemp();
		return res;
	}

	bool EqualsSpecial( const XStringA &b ) const
	{
		if (length != b.length) return false;
		for ( int i = 0; i < length; i++ )
		{
			if ( str[i] != b.str[i] ) return false;
		}
		return true;
	}

	void FromConstAStr( const char* ins, int max_chars = -1 )
	{
		str = NULL;
		capacity = 0;
		length = 0;
		if ( ins != NULL && max_chars != 0 )
		{
			int len = (int) AbCore::TStrLen( ins, max_chars );
			if ( max_chars != -1 ) len = AbcMin( len, max_chars );
			if ( len == 0 )
			{
				return;
			}
			Resize( len + 1, false );
			length = len;
			for ( int i = 0; i < len; i++ )
				str[i] = (unsigned char) ins[i];
			str[len] = 0;
		}
	}

};

// $XSTRING_OPERATOR+_OVERLOAD (see comment higher up in this file)
inline XStringW operator+( const XStringW &a, wchar_t b ) { return a + XStringW(b); }
inline XStringW operator+( wchar_t a, const XStringW &b ) { return XStringW(a) + b; }

////////////// less than ///////////////

inline bool operator<( const XStringW &a, const XStringW &b )
{
	return a.Compare( b ) < 0;
}

////////////// greater than ///////////////

inline bool operator>( const XStringW &a, const XStringW &b )
{
	return a.Compare( b ) > 0;
}

struct XStringACompareNoCase
{
	inline static bool less_than( const XStringA& a, const XStringA& b )
	{
		return a.CompareNoCase( b ) < 0;
	}
};

struct XStringWCompareNoCase
{
	inline static bool less_than( const XStringW& a, const XStringW& b )
	{
		return a.CompareNoCase( b ) < 0;
	}
};

inline int XStringACompare_Natural( const XStringA& a, const XStringA& b ) { return nss_StringCompare( a, b ); };
inline int XStringWCompare_Natural( const XStringW& a, const XStringW& b ) { return nss_StringCompare( a, b ); };

inline int XStringACompare_Binary( const XStringA& a, const XStringA& b ) { return a.Compare( b ); }
inline int XStringWCompare_Binary( const XStringW& a, const XStringW& b ) { return a.Compare( b ); }

inline int XStringACompareCx( void* cx, const XStringA& a, const XStringA& b ) { return a.Compare(b); }
inline int XStringWCompareCx( void* cx, const XStringW& a, const XStringW& b ) { return a.Compare(b); }

#pragma warning( push )
#pragma warning( disable: 4793 ) // vararg causes native code generation

static const int XStringPF_StaticBufferSize = 4000;

inline XStringA XStringPFA( const char* formatString, ... )
{
	const int MAXSIZE = XStringPF_StaticBufferSize;
	char buff[MAXSIZE+1];

	// We don't use the 'n' functions, because those do not error. It is definitely better to have an error shown than to fail silently.
	va_list va;
	va_start( va, formatString );
#if _MSC_VER >= 1400
	vsprintf_s( buff, MAXSIZE, formatString, va );
#else
	vsprintf( buff, formatString, va );
#endif
	va_end( va );
	buff[MAXSIZE] = 0;

	return XStringA( buff );
}

inline XStringW XStringPFW( const wchar_t* formatString, ... )
{
	const int MAXSIZE = XStringPF_StaticBufferSize;
	wchar_t buff[MAXSIZE+1];

	// We don't use the 'n' functions, because those do not error. It is definitely better to have an error shown than to fail silently.
	va_list va;
	va_start( va, formatString );
#ifdef _WIN32
#if _MSC_VER >= 1400
	// int len = _vsnwprintf_s( buff, MAXSIZE, formatString, va );
	vswprintf_s( buff, MAXSIZE, formatString, va );
#else
	// int len = _vsnwprintf( buff, MAXSIZE, formatString, va );
	vswprintf( buff, formatString, va );
#endif
#else
	wcscpy( buff, L"No vswprintf available on this platform" );
	// fine for linux, not for openbsd (4.0): vswprintf( buff, MAXSIZE, FormatStr, va );
#endif
	va_end( va );
	buff[MAXSIZE] = 0;

	return XStringW( buff );
}

#pragma warning( pop )


#ifdef _UNICODE
inline XStringW XStringA::ToNative() const { return ToWide(); }
#endif

inline XStringW XStringA::ToWide() const
{
	if ( length == 0 ) return L"";
	XStringW wide;
	wide.Resize( length );
	for ( int i = 0; i < length; i++ )
	{
		wide[i] = (unsigned char) str[i];
	}
	return wide;
}

inline XStringW XStringA::ToWideFromUtf8() const
{
	return XStringW::FromUtf8( str, length );
}

#ifdef _UNICODE
typedef XStringW XString;
typedef XStringWCompareNoCase XStringCompareNoCase;
#define XStringPF XStringPFW
#else
typedef XStringA XString;
typedef XStringACompareNoCase XStringCompareNoCase;
#define XStringPF XStringPFA
#endif

SWAP_MEMCPY(XStringA)
SWAP_MEMCPY(XStringW)


template<> inline uint32 fhash_gethash(const XStringW& v)	{ return v.GetHashCode_Murmur2A(); }
template<> inline uint32 fhash_gethash(const XStringA& v)	{ return v.GetHashCode_Murmur2A(); }
template<> inline void fhash_tor_types<XStringA>( fhash_tor_type& ctor, fhash_tor_type& dtor ) { ctor = fhash_TOR_ZERO; dtor = fhash_TOR_FUNC; }
template<> inline void fhash_tor_types<XStringW>( fhash_tor_type& ctor, fhash_tor_type& dtor ) { ctor = fhash_TOR_ZERO; dtor = fhash_TOR_FUNC; }
//template<> inline bool fhash_ctor_with_zero<XStringW>()	{ return true; }
//template<> inline bool fhash_ctor_with_zero<XStringA>()	{ return true; }
template<> inline void fhash_ctor(XStringW& v)				{ v.ForceNull(); }
template<> inline void fhash_ctor(XStringA& v)				{ v.ForceNull(); }
template<> inline void fhash_dtor(XStringW& v)				{ v.DeAlloc(); v.ForceNull(); }
template<> inline void fhash_dtor(XStringA& v)				{ v.DeAlloc(); v.ForceNull(); }


/** @internal Unified multiline iterator.
I initially had multiple copies of the logic that does \r\n interpretation
and all that jazz, and I slipped up, causing various bugs that were very 
difficult to trace. This solves that mess.
**/
class MultilineIterator
{
public:
	MultilineIterator( const XString& str )
	{
		Construct();
		Str = str;
		Len = str.Length();
	}
	MultilineIterator( LPCTSTR str, int len )
	{
		Construct();
		Str = str;
		Len = len;
	}

	bool Done() { return mPos > Len; }
	bool LineEnd() { return mPos == Len || Str[mPos] == 10 || Str[mPos] == 13; }
	bool LineEndNoTerm() { return Str[mPos] == 10 || Str[mPos] == 13; }
	bool AtEnd() { return mPos >= Len; }
	MultilineIterator& operator++( int ) { Next(); return *this; }
	LPCTSTR StartPtr() { return Str + mStart; }
	int Start() { return mStart; }
	int Pos() { return mPos; }
	int LineLen() { return mPos - mStart; }
	int LineNumber() { return mLine; }
	TCHAR Ch() { return Str[mPos]; }

protected:
	void Construct()
	{
		mStart = 0;
		mLine = 0;
		mPos = 0;
	}
	void Next()
	{
		if ( Str[mPos] == 13 || Str[mPos] == 10 )
		{
			mLine++;
			if ( mPos < Len - 1 && Str[mPos] == 13 && Str[mPos + 1] == 10 ) 
				mPos += 2;
			else
				mPos += 1;
			mStart = mPos;
		}
		else mPos++;
	}
	int mStart;
	int mPos;
	int mLine;
	LPCTSTR Str;
	int Len;
};

#endif
