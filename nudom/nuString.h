#pragma once

#include "nuDefs.h"

// This has no constructors or destructors so that we can put it in unions, etc. We know that there is no implicit
// memory management going on here.
class NUAPI nuStringRaw
{
public:
	char*	Z;
	size_t	Len;	// Excludes null terminator, which we DO always have.

	void	Set( const nuStringRaw& b );
	void	Set( const char* b );
	void	CloneFastInto( nuStringRaw& b, nuPool* pool ) const;
	void	Free();
	void	Discard();
	u32		GetHashCode() const;

	bool	operator==( const char* b ) const;
	bool	operator==( const nuStringRaw& b ) const;

protected:
	static nuStringRaw Temp( const char* b );

	void	Alloc( size_t chars );
};

FHASH_SETUP_CLASS_GETHASHCODE( nuStringRaw, nuStringRaw );

class NUAPI nuString : public nuStringRaw
{
public:
				nuString();
				nuString( const nuString& b );
	explicit	nuString( const char* z );
				~nuString();

	void		MakeTemp( const char* z );
	void		KillTemp();

	nuString&	operator=( const nuStringRaw& b );
	nuString&	operator=( const char* b );

};

FHASH_SETUP_CLASS_GETHASHCODE( nuString, nuString );

// Use this when you need a temporary 'nuString' object, but you don't need any heap allocs or frees
class NUAPI nuTempString : public nuString
{
public:
	nuTempString( const char* z );
	~nuTempString();
};
