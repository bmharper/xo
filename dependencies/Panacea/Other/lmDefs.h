#ifndef ABCORE_INCLUDED_LMDEFS_H
#define ABCORE_INCLUDED_LMDEFS_H

#include "../coredefs.h"

#ifndef PAPI
#define PAPI
#endif

#ifdef _MSC_VER
#pragma warning(disable:4244)  //conversion from 'double' to 'float', possible loss of data
#endif

#ifndef _MSC_VER
	//#pragma message( "Making a whole lot of assumptions because _MSC_VER is not defined" )
	#ifndef _UNICODE
		#define _UNICODE
	#endif
	typedef wchar_t TCHAR;
	typedef char* LPSTR;
	typedef const char* LPCSTR;
	typedef wchar_t* LPWSTR;
	typedef const wchar_t* LPCWSTR;
	typedef LPWSTR LPTSTR;
	typedef LPCWSTR LPCTSTR;
	#define _stscanf wscanf
#endif

#define SWAP_MEMCPY(T) namespace std { template<> inline void swap(T& a, T& b) { char temp[sizeof(a)]; memcpy(temp, &a, sizeof(a)); memcpy(&a, &b, sizeof(a)); memcpy(&b, temp, sizeof(a)); } }

// define to use secure versions of the CRT functions
#if _MSC_VER >= 1400
#define LM_VS2005_SECURE
#endif

#include "../Platform/stdint.h"
#include "../Platform/mmap.h"
#include "../Platform/err.h"

#include <float.h>
#include <math.h>

#define INLINE inline

#ifndef _max_
#define _max_(a,b) ((a) >= (b) ? (a) : (b))
#endif

#ifndef _min_
#define _min_(a,b) ((a) <= (b) ? (a) : (b))
#endif 

#define ABC_MAX(a,b) ((a) >= (b) ? (a) : (b))
#define ABC_MIN(a,b) ((a) <= (b) ? (a) : (b))

// make sure this is in sync with macro min()
template<typename T>
const T& AbcMin( const T& a, const T& b )
{
	return a <= b ? a : b;
}

// make sure this is in sync with macro min()
template<typename T>
const T& AbcMin( const T& a, const T& b, const T& c )
{
	return AbcMin( a, AbcMin(b, c) );
}

// make sure this is in sync with macro max()
template<typename T>
const T& AbcMax( const T& a, const T& b )
{
	return a >= b ? a : b;
}

// make sure this is in sync with macro max()
template<typename T>
const T& AbcMax( const T& a, const T& b, const T& c )
{
	return AbcMax( a, AbcMax(b, c) );
}

#ifndef BIT
#define BIT(n) (1 << (n))
#endif 

#ifndef _T
#ifdef _UNICODE
#define _T(a) L##a
#else
#define _T(a) a
#endif
#endif

#ifndef ASSERT
#	ifdef _DEBUG
#		define ASSERT(exp) assert(exp)
#		define VERIFY(exp) assert(exp)
#	else
#		define ASSERT(exp) ((void)0)
#		define VERIFY(exp) ((void)(exp))
#	endif
#endif

#define CLAMP( V, VMin, VMax ) ( ((V) < (VMin) ? (VMin) : ((V) > (VMax) ? (VMax) : (V)) ) )
#define ROUNDF( V ) ( floorf( (V) + 0.5f ) )
#define ROUND( V ) ( floor( (V) + 0.5 ) )

#define MM_2_PT (72.0 / (10 * 2.54))
#define PT_2_MM ((2.54 * 10) / 72.0)

#define M_2_PT (MM_2_PT *  1000)
#define PT_2_M (PT_2_MM * 0.001)

#ifndef PI
#define PI 3.1415926535897932384626433832795028841971693993751
#endif

#ifndef PI2
#define PI2 (2*3.1415926535897932384626433832795028841971693993751)
#endif

#ifndef SQRT_2
#define SQRT_2  1.4142135623730950488016887242096980785696718753769
#endif

// log2 base e
#define LOG2 (0.69314718055994530941723212145818)
#define LOG_2 (0.69314718055994530941723212145818)
// log10 base e
#define LOG_10 (2.3025850929940456840179914546844)

#define DEG2RAD     (PI / 180.0)
#define RAD2DEG     (180.0 / PI)

#define UL_EPS 1e-12
#define UL_EPS2 (UL_EPS * UL_EPS)

// Useful in various scenarios.
enum Trinary
{
	TriFalse	= -1,
	TriNull		= 0,
	TriTrue		= 1,
};

#include "../HashTab/HashFunctions.h"

enum lmIxResult
{
	lmIxNone = 0,
	lmIxPt1 = 2,
	lmIxPt2 = 4,
	lmIxPt3 = 8,
	lmIxTangent = 16
};

namespace AbCore
{

	// This stuff exists for interoperation with Delphi code
#pragma warning( push )
#pragma warning( disable: 4996 ) // not reliable inside managed code
	
#ifdef _WIN32
#ifdef _M_X64
	static const uint ResetFPCW_CRT_Mask = _MCW_DN | _MCW_EM  | _MCW_IC | _MCW_RC;
#else
	static const uint ResetFPCW_CRT_Mask = _MCW_DN | _MCW_EM  | _MCW_IC | _MCW_RC | _MCW_PC;
#endif

	/// Returns the old value
	inline uint ResetFPCW_CRT_Default()
	{
		// You must resample this when the compiler changes. Last verified on 2010.
#if _MSC_VER > 0x1000
#error Check that the CRT control word is still 0x0009001f
#endif
		uint old;
#ifdef _M_X64
		_controlfp_s( &old, 0, 0 ); // Luckily Delphi has no x64 compiler.
#else
		_controlfp_s( &old, 0x0009001f, ResetFPCW_CRT_Mask );
#endif
		return old;
	}

	struct FPCWStateReset
	{
		uint Original;

		FPCWStateReset()	{ Original = AbCore::ResetFPCW_CRT_Default(); }
		~FPCWStateReset()	{ uint a; _controlfp_s( &a, Original, ResetFPCW_CRT_Mask ); }

	};
#pragma warning( pop )
#else
	// not Win32
	inline uint ResetFPCW_CRT_Default() { return 0; }
	class FPCWStateReset
	{
	};
#endif

	enum GeomResult
	{
		GeomEdge = 1,
		GeomInside = 2,
		GeomOutside = 4
	};

	template< typename T >
	T Abs( T a )
	{
		return a >= 0 ? a : -a;
	}

	// returns -1 or +1
	template< typename T >
	int Sign( const T& a )
	{
		return a < 0 ? -1 : 1;
	}

	// returns -1, 0, +1
	template< typename T >
	int SignOrZero( const T& a )
	{
		if ( a == 0 ) return 0;
		return a < 0 ? -1 : 1;
	}

#ifdef _MSC_VER
	inline uint32_t _rotl ( uint32_t x, int8_t r )
	{
		return ::_rotl( x, r );
	}

	inline uint64_t _rotl64 ( uint64_t x, int8_t r )
	{
		return ::_rotl64( x, r );
	}

	inline uint32_t _rotr ( uint32_t x, int8_t r )
	{
		return ::_rotr( x, r );
	}

	inline uint64_t _rotr64 ( uint64_t x, int8_t r )
	{
		return ::_rotr64( x, r );
	}
#else
	inline uint32_t _rotl ( uint32_t x, int8_t r )
	{
		return (x << r) | (x >> (32 - r));
	}

	inline uint64_t _rotl64 ( uint64_t x, int8_t r )
	{
		return (x << r) | (x >> (64 - r));
	}

	inline uint32_t _rotr ( uint32_t x, int8_t r )
	{
		return (x >> r) | (x << (32 - r));
	}

	inline uint64_t _rotr64 ( uint64_t x, int8_t r )
	{
		return (x >> r) | (x << (64 - r));
	}
#endif

	template<typename T>	T		TRotL(T v, int shift)		{ int* x = 0; *x = 0; }
	template<> inline		u32		TRotL(u32 v, int shift)		{ return _rotl(v, shift); }
	template<> inline		u64		TRotL(u64 v, int shift)		{ return _rotl64(v, shift); }
	template<typename T>	T		TRotR(T v, int shift)		{ int* x = 0; *x = 0; }
	template<> inline		u32		TRotR(u32 v, int shift)		{ return _rotr(v, shift); }
	template<> inline		u64		TRotR(u64 v, int shift)		{ return _rotr64(v, shift); }

	template< typename T >
	void Swap( T& a, T& b )
	{
		std::swap(a, b);
	}

	/// Returns the lesser item, or the first one if they are equal.
	template< typename T >
	T Lesser( T a, T b )
	{
		return b < a ? b : a;
	}

	/// Returns the greater item, or the second one if they are equal.
	template< typename T >
	T Greater( T a, T b )
	{
		return a < b ? a : b;
	}

	template< typename T, typename PT >
	T Lerp( const PT& pos_zero_to_one, const T& a, const T& b )
	{
		return (1 - pos_zero_to_one) * a + pos_zero_to_one * b;
	}

	/// This is easier to read than std::pair<>'s .first and .second
	struct SubString
	{
		int Pos;
		int Length;
	};

	template< typename T >
	unsigned int TPopCount( T a )
	{
		// Do it in groups of 4 bits. Total thumbsuck friday afternoon amusement. No idea what's fastest.
		// Double it up, so we do 2 per loop
		const unsigned int TabBits = 4;
		const T TabMask = (1 << TabBits) - 1;
		unsigned char tab[TabMask+1] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
		unsigned int c1 = 0;
		unsigned int c2 = 0;
		T b = a >> TabBits;
		for ( int i = 0; i < sizeof(T) * 8 / (TabBits * 2); i++ )
		{
			c1 += tab[a & TabMask];
			c2 += tab[b & TabMask];
			a >>= TabBits * 2;
			b >>= TabBits * 2;
		}
		return c1 + c2;
	}

	inline UINT32 PopCount32( UINT32 a ) { return TPopCount(a); }
	inline UINT32 PopCount64( UINT64 a ) { return TPopCount(a); }

	//table
	static const int8 xs_KotayBits[32] =    
	{
		0,  1,  2, 16,  3,  6, 17, 21,
		14,  4,  7,  9, 18, 11, 22, 26,
		31, 15,  5, 20, 13,  8, 10, 25,
		30, 19, 12, 24, 29, 23, 28, 27
	};

	// This is by Sree Kotay. An equivalent solution is described in Bit Twiddling Hacks.
	// only works for powers of 2 inputs
	inline int32 ILogPow2( int32 v )
	{
		//constant is binary 10 01010 11010 00110 01110 11111
		return xs_KotayBits[(uint32(v)*uint32( 0x04ad19df ))>>27];
	}     
	union UIntAndFloat
	{
		float Float;
		UINT32 Integer;
	};

	// integer floor(log2) of a float, utilizing FP exponent. Valid range of x is (x, 2^31)
	inline INT32 IntLog2_Floorf( float x )
	{
		UIntAndFloat v;
		v.Float = x;
		UINT32 exp = (v.Integer >> 23) & 0xFF;
		INT32 log2 = INT32(exp) - 127;
		return log2;
	}

	inline INT32 IntLog2_Floor( int x )
	{
		return IntLog2_Floorf( x );
	}

	// integer ceil(log2) of a float, utilizing FP exponent. Valid range of x is (x, 2^31)
	inline INT32 IntLog2_Ceilf( float x )
	{
		UIntAndFloat v;
		v.Float = x;
		UINT32 exp = (v.Integer >> 23) & 0xFF;
		UINT32 mant = v.Integer & 0x7FFFFF;
		INT32 log2 = INT32(exp) - 127;
		if ( mant != 0 ) log2++;
		return log2;
	}

	inline INT32 IntLog2_Ceil( int x )
	{
		return IntLog2_Ceilf( x );
	}

	template< typename TUInt >
	bool IsPowerOf2( TUInt x )
	{
		return !(x & (x - 1)) && x;
	}

	inline int NextPower2( float v )
	{
		bool neg = v < 0;
		if ( neg ) v = -v;
		if ( v <= 1 ) return neg ? -1 : 1;
		int lv = (int) ceil( logf(v) / LOG2 - FLT_EPSILON );
		int pv = (int) pow( 2.0f, lv );
		return neg ? -pv : pv;
	}

	/// If v is a power of 2, then we return v. Otherwise, we return the next power of two.
	inline INT32 NextPower2Int( INT32 v )
	{
		bool neg = v < 0;
		INT32 av = neg ? -v : v;
		if ( ((av - 1) & av) == 0 ) return v;
		return NextPower2( (float) av );
	}

	template< typename T >
	void Reverse( T* base, size_t count )
	{
		size_t lim = count / 2;
		for ( size_t i = 0; i < lim; i++ )
		{
			T temp = base[i];
			base[i] = base[count - 1 - i];
			base[count - 1 - i] = temp;
		}
	}

#ifdef _WIN32
#define FINITE _finite
#else
#define FINITE isfinite
#endif

	template <typename FT>
	class Traits
	{
	public:
		//static FT Epsilon() { return 0; }
		//static FT Min()		{ return 0; }
		//static FT Max()		{ return 0; }
	};

	template <>
	class Traits<float>
	{
	public:
		static float	Epsilon()			{ return FLT_EPSILON; }
		static float	Min()				{ return FLT_MIN; }
		static float	Max()				{ return FLT_MAX; }
		static bool		IsNaN(float v)		{ return v != v; }
		static bool		Finite(float v)		{ return !!FINITE(v); }
	};

	template <>
	class Traits<double>
	{
	public:
		static double	Epsilon()			{ return DBL_EPSILON; }
		static double	Min()				{ return DBL_MIN; }
		static double	Max()				{ return DBL_MAX; }
		static bool		IsNaN(double v)		{ return v != v; }
		static bool		Finite(double v)	{ return !!FINITE(v); }
	};
	
	inline bool IsNaN( float v )		{ return Traits<float>::IsNaN(v); }
	inline bool IsNaN( double v )		{ return Traits<double>::IsNaN(v); }
	inline bool IsFinite( float v )		{ return !!FINITE(v); }
	inline bool IsFinite( double v )	{ return !!FINITE(v); }

#undef FINITE

	template <>
	class Traits<INT32>
	{
	public:
		typedef INT32 TINT;
		static TINT Min() { return INT32MIN; }
		static TINT Max() { return INT32MAX; }
	};

	template <>
	class Traits<UINT32>
	{
	public:
		typedef UINT32 TINT;
		static TINT Min() { return 0; }
		static TINT Max() { return UINT32MAX; }
	};

	template <>
	class Traits<INT64>
	{
	public:
		typedef INT64 TINT;
		static TINT Min() { return INT64MIN; }
		static TINT Max() { return INT64MAX; }
	};

	template <>
	class Traits<UINT64>
	{
	public:
		typedef UINT64 TINT;
		static TINT Min() { return 0; }
		static TINT Max() { return UINT64MAX; }
	};

	typedef Traits<float>	FloatTraits;
	typedef Traits<double>	DoubleTraits;
	typedef Traits<INT32>	Int32Traits;
	typedef Traits<UINT32>	UInt32Traits;
	typedef Traits<INT64>	Int64Traits;
	typedef Traits<UINT64>	UInt64Traits;

	/** Helper function for serializing enumerations to textual representations (such as xml).
	@return True if we found a match
	**/
	template< typename TEnum, bool CaseSensitive >
	bool ParseEnum( int enumCount, const char** name_table, const char* val, TEnum& enum_value, int start_at = 0 )
	{

#ifdef _MSC_VER
#define CMP_NOCASE _stricmp
#else
#define CMP_NOCASE strcasecmp
#endif

		if ( val == NULL ) return false;
		for ( int i = start_at; i < enumCount; i++ )
		{
			if (  CaseSensitive &&     strcmp(name_table[i], val) == 0 ) { enum_value = (TEnum) i; return true; }
			if ( !CaseSensitive && CMP_NOCASE(name_table[i], val) == 0 ) { enum_value = (TEnum) i; return true; }
		}
		return false;

#undef CMP_NOCASE
	}

	/// Baked case insensitive version that allows the enum template parameter to be inferred
	template< typename TEnum >
	bool ParseEnumi( int enumCount, const char** name_table, const char* val, TEnum& enum_value, int start_at = 0 )
	{
		return ParseEnum<TEnum, false>( enumCount, name_table, val, enum_value, start_at );
	}

	/** Modulus that is not symmetrical around zero, but has its origin at the lowest possible negative integer value.
	For mod 4, this will create the following pattern: -4:0 -3:1 -2:2 -1:3 0:0 1:1: 2:2: 3:3: 4:0.

	NOTE: If your modulus is a power of 2, then you should simply cast to unsigned and AND with your modulus - 1.
	For example, if you want to wrap on mod 4 - then -1 unsigned equal = 0xffff, and 0xffff & (4-1) = 3, which is what you want.
	This function is useful if you're modding on a non-power-of-2 divisor.

	@param i The numerator. This may be any integer in the range [_I32MIN + m, _I32MAX - m].
	@param m The modulus
	**/
	inline UINT RMod( int i, UINT m )
	{
		UINT org = (2147483647u / m) * m;
		// What we're really doing is (i - -org), but that is obviously just (i + org).
		UINT ai = i + org;
		return ai % m;
	}
}

// Binary Search
// * This will walk to the first in a series of matches
// * There is only one branch in the inner loop
template<typename TData, typename TKey>
intp AbcBinSearch( intp n, const TData* items, const TKey& key, int (*compare)(const TData& item, const TKey& key) )
{
	if ( n == 0 )
		return -1;
	intp imin = 0;
	intp imax = n;
	while ( imax > imin )
	{
		intp imid = (imin + imax) / 2;
		if ( compare(items[imid], key) < 0 )
			imin = imid + 1;
		else
			imax = imid;
	}
	if ( imin == imax && 0 == compare(items[imin], key) )
		return imin;
	else
		return -1;
}

// Binary Search, but always return stopping position, regardless of match
// * This will walk to the first in a series of matches
// * There is only one branch in the inner loop
template<typename TData, typename TKey>
intp AbcBinSearchTry( intp n, const TData* items, const TKey& key, int (*compare)(const TData& item, const TKey& key) )
{
	if ( n == 0 )
		return -1;
	intp imin = 0;
	intp imax = n;
	while ( imax > imin )
	{
		intp imid = (imin + imax) / 2;
		if ( compare(items[imid], key) < 0 )
			imin = imid + 1;
		else
			imax = imid;
	}
	return imin;
}

/// Inserts the contents of the set b into the set a.
template< typename TSet >
void set_add( TSet& a, const TSet& b )
{
	for ( typename TSet::iterator it = b.begin(); it != b.end(); it++ )
		a.insert( *it );
}

/// Returns a new set that is the union of set a and set b.
template< typename TSet >
TSet set_union( const TSet& a, const TSet& b )
{
	TSet c = a;
	for ( typename TSet::iterator it = b.begin(); it != b.end(); it++ )
		c.insert( *it );
	return c;
}

/// Returns the number of objects present in set a and set b
template<typename TSet1, typename TSet2>
size_t set_intersect_count( const TSet1& a, const TSet2& b )
{
	size_t count = 0;
	for ( auto it = a.begin(); it != a.end(); it++ )
	{
		if ( b.contains(*it) )
			count++;
	}
	return count;
}

/// Returns a new set that is a - b
template< typename TSet >
TSet set_difference( const TSet& a, const TSet& b )
{
	TSet c = a;
	for ( typename TSet::iterator it = b.begin(); it != b.end(); it++ )
		c.erase( *it );
	return c;
}

template< typename TSet, typename TVector >
void vector_to_set( const TVector& vect, TSet& set )
{
	for ( intp i = 0; i < vect.size(); i++ )
		set.insert( vect[i] );
}

template< typename TSet, typename TVector >
void set_to_vector( const TSet& set, TVector& vect )
{
	for ( typename TSet::iterator it = set.begin(); it != set.end(); it++ )
		vect.push_back( *it );
}

// Useful for copying, for instance, from 
template< typename TV1, typename TV2 >
void vector_add_all_from( TV1& dst, const TV2& src )
{
	intp size = src.size();
	for ( intp i = 0; i < size; i++ )
		dst.push_back( src[i] );
}

// src => dst  (ie postcondition: src == dst)
template< typename TV1, typename TV2 >
void vector_copy( const TV1& src, TV2& dst )
{
	intp size = src.size();
	dst.resize( size );
	for ( intp i = 0; i < size; i++ )
		dst[i] = src[i];
}

// This should be easier to use with C++11 left-side inference
template< typename TV1, typename TV2 >
TV1 vector_copy( const TV2& src )
{
	TV1 dst;
	vector_copy( src, dst );
	return dst;
}

template< typename TVect, typename THash >
void GetAllKeys( const THash& map, TVect& keys )
{
	for ( typename THash::iterator it = map.begin(); it != map.end(); it++ )
		keys += it->first;
}

template< typename TVect, typename THash >
void GetAllValues( const THash& map, TVect& values )
{
	for ( typename THash::iterator it = map.begin(); it != map.end(); it++ )
		values += it->second;
}

template<size_t bytes>
inline bool CryptoMemCmpEq( const void* a, const void* b )
{
	// essential that this is constant runtime, otherwise you have a trivial timing attack
	static_assert((bytes & 3) == 0, "size of CryptoMemCmp must be multiple of 4");
	u32 diff = 0;
	for ( size_t i = 0; i < bytes / 4; i++ ) diff |= ((u32*)a)[i] - ((u32*)b)[i];
	return diff == 0;
}

inline bool CryptoMemCmpEq( const void* a, const void* b, size_t bytes )
{
	assert((bytes & 3) == 0);
	u32 diff = 0;
	for ( size_t i = 0; i < bytes / 4; i++ ) diff |= ((u32*)a)[i] - ((u32*)b)[i];
	return diff == 0;
}

inline void memxorcpy( void* dst, const void* src, size_t bytes )
{
	u8* bdst = (u8*) dst;
	const u8* bsrc = (const u8*) src;
	for ( size_t i = 0; i < bytes; i++ )
		bdst[i] = bdst[i] ^ bsrc[i];
}

// This gets about 8GB/s on a Core2 2.4ghz, 32-bit
template<typename TPrim>
bool ismemzeroT( const void* b, size_t bytes )
{
	const TPrim* mem1 = (const TPrim*) b;
	const u32 inc = 8;
	const u32 div = inc * sizeof(TPrim);
	size_t chunks = (bytes / div) * inc;

	TPrim z = 0;
	for ( size_t i = 0; i < chunks; i += inc )
	{
		z |= mem1[i];
		z |= mem1[i+1];
		z |= mem1[i+2];
		z |= mem1[i+3];
		z |= mem1[i+4];
		z |= mem1[i+5];
		z |= mem1[i+6];
		z |= mem1[i+7];
		if ( z != 0 ) return false;
	}

	// tail of non-aligned bytes
	const u8* mem2 = (const u8*) b;
	for ( size_t i = (bytes / div) * div; i < bytes; i++ )
		if ( mem2[i] != 0 ) return false;

	return true;
}

PAPI bool ismemzero( const void* b, size_t bytes );

template<size_t bytes>
inline bool ismemzero( const void* b )
{
	if ( bytes > 64 )
		return ismemzero( b, bytes );

	static_assert((bytes & 3) == 0, "size of ismemzeroT must be multiple of 4");
	const u32* mem = (const u32*) b;
	for ( size_t i = 0; i < bytes / 4; i++ )
	{
		if ( mem[i] != 0 ) return false;
	}
	return true;
}


#define SYSTEM_PAGE_BITS 12
#define SYSTEM_PAGE_SIZE (1 << SYSTEM_PAGE_BITS)

// Enn = Endian neutral
// SHOUT OUT!!!! This code is crap -- way too many overloads, they're confusing, and the names are too long.
// If you read this on a friday afternoon... fix this crap up.

/// Change this if you ever compile to a different architecture.
#define ENDIANLITTLE 1
#if ENDIANLITTLE
static const bool EnnSystemIsLittle = true;
#else
static const bool EnnSystemIsLittle = false;
#endif

static const bool EnnSystemIsBig = !EnnSystemIsLittle;

// src and dst may be aliases
inline void EnnSwapBytes( uintp n, const void* src, void* dst )
{
	const char* bs = (const char*) src;
				char* bd =       (char*) dst;
	// signed comparison is necessary for case where n <= 1
	intp j = n - 1;
	for ( intp i = 0; i <= j; i++, j-- )
	{
		char t = bs[i];
		bd[i] = bs[j];
		bd[j] = t;
	}
}

inline void EnnBigToNative( uintp n, const void* src, void* dst )
{
	if ( EnnSystemIsLittle ) EnnSwapBytes( n, src, dst );
}

inline void EnnNativeToBig( uintp n, const void* src, void* dst )
{
	if ( EnnSystemIsLittle ) EnnSwapBytes( n, src, dst );
}

template< typename T > void EnnBigToNative( const T& src, T& dst ) { EnnBigToNative( sizeof(src), &src, &dst ); }
template< typename T > void EnnNativeToBig( const T& src, T& dst ) { EnnNativeToBig( sizeof(src), &src, &dst ); }

template< typename T > void EnnBigToNative( T& dst ) { EnnBigToNative( sizeof(dst), &dst, &dst ); }
template< typename T > void EnnNativeToBig( T& dst ) { EnnNativeToBig( sizeof(dst), &dst, &dst ); }

template< typename T > T EnnNativeToBigCopy( const T& src ) { T copy; EnnNativeToBig(src, copy); return copy; }

template< typename T > T EnnSwap( const T& src )
{
	T r;
	EnnSwapBytes( sizeof(r), &src, &r );
	return r;
}

template< typename T > T EnnToNative( bool src_is_little_endian, const T& src )
{
	if ( src_is_little_endian == EnnSystemIsLittle ) return src;
	return EnnSwap( src );
}

template< typename T > T EnnToNativeP( bool src_is_little_endian, const void* src )
{
	if ( src_is_little_endian == EnnSystemIsLittle ) return *((const T*) src);
	T r = *((T*) src);
	return EnnSwap( r );
}
template< typename T > T EnnBigToNativeP( const void* src ) { return EnnToNativeP<T>(false, src); }

template< typename T > void EnnWriteBig( const T& src, void* dst )
{
	T* tdst = (T*) dst;
	*tdst = src;
	if ( EnnSystemIsLittle ) EnnSwapBytes( sizeof(src), tdst, tdst );
}

template< typename T > void EnnWriteLittle( const T& src, void* dst )
{
	T* tdst = (T*) dst;
	*tdst = src;
	if ( EnnSystemIsBig ) EnnSwapBytes( sizeof(src), tdst, tdst );
}

struct CPUINFO
{
	bool MMX;
	bool SSE1;
	bool SSE2;
	bool SSE3;
	bool SSSE3;
	bool SSE4_1;
	bool SSE4_2;
	bool AMD_3DNOW; // Still useful for Athlon processors, because of slow unaligned SSE2 memory accesses on those processors

	//bool Intel;
	//bool P3;
	//bool P4;

	//bool AMD;
	//bool K7;	// athlon
	//bool K8;	// hammer
};

struct LMPLATFORM
{
	CPUINFO CPU;
	bool Linux;
	bool Windows;
	bool CRLF;
};

#endif
