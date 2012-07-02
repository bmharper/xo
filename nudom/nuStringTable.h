#pragma once

#include "nuString.h"

class NUAPI nuStringTable
{
public:
					nuStringTable();
					~nuStringTable();

	const char*		GetStr( int id ) const;
	int				GetId( const char* str );

protected:
	fhashmap<nuString, int>		NameToId;
	podvec<nuString>			IdToName;
};