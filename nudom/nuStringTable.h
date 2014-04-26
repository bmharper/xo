#pragma once

#include "nuString.h"

class NUAPI nuStringTable
{
public:
						nuStringTable();
						~nuStringTable();

	const char*			GetStr( int id ) const;			// Returns an empty string if the id is invalid or zero.
	int					GetId( const char* str );

	// This assumes that nuStringTable is "append-only", which it currently is.
	// Knowing this allows us to make the clone from Document to Render Document trivially fast.
	void				CloneFrom_Incremental( const nuStringTable& src );

protected:
	nuPool						Pool;
	fhashmap<nuString, int>		NameToId;			// This could be improved dramatically, by avoiding the heap alloc for every item
	pvect<const char*>			IdToName;
};