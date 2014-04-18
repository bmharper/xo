#pragma once

#include "nuString.h"

class NUAPI nuStringTable
{
public:
					nuStringTable();
					~nuStringTable();

	const nuString*	GetStr( int id ) const;			// Returns an empty string if the id is invalid or zero.
	int				GetId( const char* str );

	// temp hack. should probably not be here.
	void			CloneFrom( const nuStringTable& src ); 

protected:
	fhashmap<nuString, int>		NameToId;
	podvec<nuString>			IdToName;
	nuString					Empty;
};