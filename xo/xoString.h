#pragma once

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable: 4345 )	// POD constructed with () is default-initialized
#endif

class xoPool;

// This has no constructors or destructors so that we can put it in unions, etc. We know that there is no implicit
// memory management going on here.
class XOAPI xoStringRaw
{
public:
	char*	Z;

	intp	Length() const;
	void	CloneFastInto( xoStringRaw& b, xoPool* pool ) const;
	void	Discard();
	u32		GetHashCode() const;
	intp	Index( const char* find ) const;
	intp	RIndex( const char* find ) const;

	bool	operator==( const char* b ) const;
	bool	operator!=( const char* b ) const			{ return !(*this == b); }
	bool	operator==( const xoStringRaw& b ) const;
	bool	operator!=( const xoStringRaw& b ) const	{ return !(*this == b); }

	bool	operator<( const xoStringRaw& b ) const;

protected:
	static xoStringRaw Temp( char* b );

	void	Alloc( uintp chars );
	void	Free();
};

// This is the classic thing you'd expect from a string. The destructor will free the memory.
class XOAPI xoString : public xoStringRaw
{
public:
				xoString();
				xoString( const xoString& b );
				xoString( const xoStringRaw& b );
				xoString( const char* z, intp maxLength = -1 );	// Calls Set()
				~xoString();

	void				Set( const char* z, intp maxLength = -1 );	// checks maxLength against strlen(z) and clamps automatically
	void				ReplaceAll( const char* find, const char* replace );
	podvec<xoString>	Split( const char* splitter ) const;
	xoString			SubStr( intp start, intp end ) const;	// Returns [start .. end - 1]

	xoString&	operator=( const xoString& b );
	xoString&	operator=( const xoStringRaw& b );
	xoString&	operator=( const char* b );
	xoString&	operator+=( const xoStringRaw& b );
	xoString&	operator+=( const char* b );

	static xoString		Join( const podvec<xoString>& parts, const char* joiner );

};

FHASH_SETUP_CLASS_GETHASHCODE( xoString, xoString );

XOAPI xoString operator+( const char* a, const xoStringRaw& b );
XOAPI xoString operator+( const xoStringRaw& a, const char* b );
XOAPI xoString operator+( const xoStringRaw& a, const xoStringRaw& b );

// Use this when you need a temporary 'xoString' object, but you don't need any heap allocs or frees
class XOAPI xoTempString : public xoString
{
public:
	xoTempString( const char* z );
	~xoTempString();
};

void XOAPI xoItoa( int64 value, char* buf, int base );

#ifdef _WIN32
#pragma warning( pop )
#endif
