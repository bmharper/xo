#include "pch.h"
#include "xoDefs.h"
#include "xoString.h"

intp xoStringRaw::Length() const
{
	return Z == nullptr ? 0 : strlen(Z);
}

void xoStringRaw::CloneFastInto( xoStringRaw& b, xoPool* pool ) const
{
	auto len = Length();
	if ( len != 0 )
	{
		b.Z = (char*) pool->Alloc( len, false );
		memcpy( b.Z, Z, len + 1 );
	}
	else
	{
		b.Z = nullptr;
	}
}

void xoStringRaw::Discard()
{
	Z = nullptr;
}

void xoStringRaw::Alloc( uintp chars )
{
	Z = (char*) malloc( chars );
	XOASSERT(Z);
}

void xoStringRaw::Free()
{
	free(Z);
	Z = nullptr;
}

u32 xoStringRaw::GetHashCode() const
{
	if ( Z == nullptr )
		return 0;
	// sdbm.
	u32 hash = 0;
	for ( uintp i = 0; Z[i] != 0; i++ )
	{
		// hash(i) = hash(i - 1) * 65539 + str[i]
		hash = (u32) Z[i] + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

intp xoStringRaw::Index( const char* find ) const
{
	const char* p = strstr( Z, find );
	if ( p == nullptr )
		return -1;
	return p - Z;
}

intp xoStringRaw::RIndex( const char* find ) const
{
	intp pos = 0;
	intp last = -1;
	while ( true )
	{
		const char* p = strstr( Z + pos, find );
		if ( p == nullptr )
			return last;
		last = p - Z;
		pos = last + 1;
	}
	XOPANIC("supposed to be unreachable");
	return last;
}

bool xoStringRaw::operator==( const char* b ) const
{
	return *this == Temp(const_cast<char*>(b));
}

bool xoStringRaw::operator==( const xoStringRaw& b ) const
{
	if ( Z == nullptr )
	{
		return b.Z == nullptr || b.Z[0] == 0;
	}
	if ( b.Z == nullptr )
	{
		// First case here is already handled
		return /* Z == nullptr || */ Z[0] == 0;
	}
	return strcmp( Z, b.Z ) == 0;
}

bool xoStringRaw::operator<( const xoStringRaw& b ) const
{
	if ( Z == nullptr )
	{
		return b.Z != nullptr && b.Z[0] != 0;
	}
	if ( b.Z == nullptr )
	{
		return false;
	}
	return strcmp( Z, b.Z ) < 0;
}

xoStringRaw xoStringRaw::Temp( char* b )
{
	xoStringRaw r;
	r.Z = b;
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoString::xoString()
{
	Z = nullptr;
}

xoString::xoString( const xoString& b )
{
	Z = nullptr;
	*this = b;
}

xoString::xoString( const xoStringRaw& b )
{
	Z = nullptr;
	*this = b;
}

xoString::xoString( const char* z, intp maxLength )
{
	Z = nullptr;
	Set( z, maxLength );
}

xoString::~xoString()
{
	Free();
}

void xoString::Set( const char* z, intp maxLength )
{
	intp newLength = 0;
	if ( z != nullptr )
		newLength = strlen(z);
	
	if ( maxLength >= 0 )
		newLength = xoMin( newLength, maxLength );

	if ( Length() == newLength )
	{
		memcpy( Z, z, newLength );
	}
	else
	{
		Free();
		if ( newLength != 0 )
		{
			// add a null terminator, always, but don't assume that source has a null terminator
			Alloc( newLength + 1 );
			memcpy( Z, z, newLength );
			Z[newLength] = 0;
		}
	}
}

void xoString::ReplaceAll( const char* find, const char* replace )
{
	size_t findLen = strlen(find);
	size_t replaceLen = strlen(replace);
	std::string self = Z;
	size_t pos = 0;
	while ( (pos = self.find( find, pos )) != std::string::npos )
	{
		self.replace( pos, findLen, replace );
		pos += replaceLen;
	}
	*this = self.c_str();
}

podvec<xoString> xoString::Split( const char* splitter ) const
{
	podvec<xoString> v;
	intp splitLen = strlen(splitter);
	intp pos = 0;
	intp len = Length();
	while ( true )
	{
		const char* next = strstr( Z + pos, splitter );
		if ( next == nullptr )
		{
			v += SubStr( pos, len );
			break;
		}
		else
		{
			v += SubStr( pos, next - Z );
			pos = next - Z + splitLen;
		}
	}
	return v;
}

xoString xoString::SubStr( intp start, intp end ) const
{
	auto len = Length();
	start = xoClamp<intp>( start, 0, len );
	end = xoClamp<intp>( end, 0, len );
	xoString s;
	s.Set( Z + start, end - start );
	return s;
}

xoString& xoString::operator=( const xoString& b )
{
	Set( b.Z );
	return *this;
}

xoString& xoString::operator=( const xoStringRaw& b )
{
	Set( b.Z );
	return *this;
}

xoString& xoString::operator=( const char* b )
{
	Set( b );
	return *this;
}

xoString& xoString::operator+=( const xoStringRaw& b )
{
	intp len1 = Length();
	intp len2 = b.Length();
	char* newZ = (char*) malloc( len1 + len2 + 1 );
	XOASSERT(newZ);
	memcpy( newZ, Z, len1 );
	memcpy( newZ + len1, b.Z, len2 );
	newZ[len1 + len2] = 0;
	free( Z );
	Z = newZ;
	return *this;
}

xoString& xoString::operator+=( const char* b )
{
	*this += xoTempString(b);
	return *this;
}

xoString xoString::Join( const podvec<xoString>& parts, const char* joiner )
{
	xoString r;
	if ( parts.size() != 0 )
	{
		intp jlen = joiner == nullptr ? 0 : (intp) strlen(joiner);
		intp total = 0;
		for ( intp i = 0; i < parts.size(); i++ )
			total += parts[i].Length() + jlen;
		r.Alloc( total + 1 );
	
		intp rpos = 0;
		for ( intp i = 0; i < parts.size(); i++ )
		{
			const char* part = parts[i].Z;
			if ( part != nullptr )
			{
				for ( intp j = 0; part[j]; j++, rpos++ )
					r.Z[rpos] = part[j];
			}
			if ( i != parts.size() - 1 )
			{
				for ( intp j = 0; j < jlen; j++, rpos++ )
					r.Z[rpos] = joiner[j];
			}
		}
		r.Z[rpos] = 0;
	}
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XOAPI xoString operator+( const char* a, const xoStringRaw& b )
{
	xoString r = a;
	r += b;
	return r;
}

XOAPI xoString operator+( const xoStringRaw& a, const char* b )
{
	xoString r = a;
	r += b;
	return r;
}

XOAPI xoString operator+( const xoStringRaw& a, const xoStringRaw& b )
{
	xoString r = a;
	r += b.Z;
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoTempString::xoTempString( const char* z )
{
	Z = const_cast<char*>(z);
}

xoTempString::~xoTempString()
{
	Z = nullptr;
}
