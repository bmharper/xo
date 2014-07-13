#pragma once

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable: 4345 )	// POD constructed with () is default-initialized
#endif

class nuPool;

// This has no constructors or destructors so that we can put it in unions, etc. We know that there is no implicit
// memory management going on here.
class NUAPI nuStringRaw
{
public:
	char*	Z;

	intp	Length() const;
	void	CloneFastInto( nuStringRaw& b, nuPool* pool ) const;
	void	Discard();
	u32		GetHashCode() const;
	intp	Index( const char* find ) const;
	intp	RIndex( const char* find ) const;

	bool	operator==( const char* b ) const;
	bool	operator!=( const char* b ) const			{ return !(*this == b); }
	bool	operator==( const nuStringRaw& b ) const;
	bool	operator!=( const nuStringRaw& b ) const	{ return !(*this == b); }

	bool	operator<( const nuStringRaw& b ) const;

protected:
	static nuStringRaw Temp( char* b );

	void	Alloc( uintp chars );
	void	Free();
};

// This is the classic thing you'd expect from a string. The destructor will free the memory.
class NUAPI nuString : public nuStringRaw
{
public:
				nuString();
				nuString( const nuString& b );
				nuString( const nuStringRaw& b );
				nuString( const char* z, intp maxLength = -1 );	// Calls Set()
				~nuString();

	void				Set( const char* z, intp maxLength = -1 );	// checks maxLength against strlen(z) and clamps automatically
	void				ReplaceAll( const char* find, const char* replace );
	podvec<nuString>	Split( const char* splitter ) const;
	nuString			SubStr( intp start, intp end ) const;	// Returns [start .. end - 1]

	nuString&	operator=( const nuString& b );
	nuString&	operator=( const nuStringRaw& b );
	nuString&	operator=( const char* b );
	nuString&	operator+=( const nuStringRaw& b );
	nuString&	operator+=( const char* b );

	static nuString		Join( const podvec<nuString>& parts, const char* joiner );

};

FHASH_SETUP_CLASS_GETHASHCODE( nuString, nuString );

NUAPI nuString operator+( const nuStringRaw& a, const char* b );
NUAPI nuString operator+( const nuStringRaw& a, const nuStringRaw& b );

// Use this when you need a temporary 'nuString' object, but you don't need any heap allocs or frees
class NUAPI nuTempString : public nuString
{
public:
	nuTempString( const char* z );
	~nuTempString();
};

#ifdef _WIN32
#pragma warning( pop )
#endif
