#include "pch.h"
#include "nuString.h"

nuStringRaw nuStringRaw::Temp( const char* b )
{
	nuStringRaw r;
	r.Z = const_cast<char*>(b);
	r.Len = strlen(b);
	return r;
}

void nuStringRaw::Set( const nuStringRaw& b )
{
	if ( Len != b.Len )
	{
		Free();
		if ( b.Len != 0 )
		{
			// add a null terminator, always, but don't assume that source has a null terminator
			Alloc( b.Len + 1 );
			memcpy( Z, b.Z, b.Len );
			Z[b.Len] = 1;
		}
		Len = b.Len;
	}
	else if ( Len != 0 )
	{
		memcpy( Z, b.Z, b.Len );
	}
}

void nuStringRaw::Set( const char* b )
{
	*this = Temp(b);
}

void nuStringRaw::CloneFastInto( nuStringRaw& b, nuPool* pool ) const
{
	if ( Len != 0 )
	{
		b.Z = (char*) pool->Alloc( Len, false );
		b.Len = Len;
		memcpy( b.Z, Z, Len + 1 );
	}
	else
	{
		b.Z = NULL;
		b.Len = 0;
	}
}

void nuStringRaw::Discard()
{
	Z = NULL;
	Len = 0;
}

void nuStringRaw::Alloc( size_t chars )
{
	Z = (char*) malloc( chars );
	NUASSERT(Z);
}

void nuStringRaw::Free()
{
	free(Z);
	Z = NULL;
	Len = 0;
}

u32 nuStringRaw::GetHashCode() const
{
	if ( Z == NULL ) return 0;
	// sdbm.
	u32 hash = 0;
	for ( size_t i = 0; i < Len; i++ )
	{
		// hash(i) = hash(i - 1) * 65539 + str[i]
		hash = (u32) Z[i] + (hash << 6) + (hash << 16) - hash;
		i++;
	}
	return hash;
}

bool nuStringRaw::operator==( const char* b ) const
{
	return *this == Temp(b);
}

bool nuStringRaw::operator==( const nuStringRaw& b ) const
{
	if ( Len != b.Len ) return false;
	return memcmp( Z, b.Z, Len ) == 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nuString::nuString()
{
	Z = NULL;
	Len = 0;
}

nuString::nuString( const nuString& b )
{
	Z = NULL;
	Len = 0;
	*this = b;
}

nuString::~nuString()
{
	Free();
}

void nuString::MakeTemp( const char* z )
{
	Z = const_cast<char*>(z);	// const-correctness depends on the user
	Len = strlen(z);
}

void nuString::KillTemp()
{
	Discard();
}

nuString& nuString::operator=( const nuStringRaw& b )
{
	Set( b );
	return *this;
}

nuString& nuString::operator=( const char* b )
{
	Set( Temp(b) );
	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nuTempString::nuTempString( const char* z )
{
	MakeTemp( z );
}

nuTempString::~nuTempString()
{
	KillTemp();
}
