#ifndef ABCORE_INCLUDED_TYPEINF_H
#define ABCORE_INCLUDED_TYPEINF_H

#include "stdlib.h"

/// Abbreviated type info. READ WARNINGS!
/**
Warning! This only works on classes that are functionally unique. That is, they must
implement at least one virtual method, which causes the class to have a unique vTable.
If two classes have the same vTable then they will be identified as THE SAME.
**/
struct typeinf 
{
	size_t id;
	typeinf() { id = 0; }
	typeinf( const void* object )
	{
		const size_t *ip = (const size_t*) object;
		id = *ip;
	}
	typeinf( const typeinf& copy )
	{
		id = copy.id;
	}

	bool operator==( const typeinf& b ) const
	{
		return id == b.id;
	}
	
	bool operator!=( const typeinf& b ) const
	{
		return id != b.id;
	}

	// for hashing.
	operator size_t() const
	{
		return id;
	}
};

#endif
