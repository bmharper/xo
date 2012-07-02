#include "pch.h"
#include "nuStringTable.h"
 

nuStringTable::nuStringTable()
{
	IdToName += nuString("");
	NameToId.insert( IdToName[0], 0 );
}
nuStringTable::~nuStringTable()
{
}

const char* nuStringTable::GetStr( int id ) const
{
	if ( (uint32) id >= (uint32) IdToName.size() )
		return "";
	return IdToName[id].Z;
}

int nuStringTable::GetId( const char* str )
{
	nuTempString s(str);
	int v = 0;
	if ( NameToId.get( s, v ) )
		return v;
	IdToName += nuString(str);
	NameToId.insert( nuString(str), (int) IdToName.size() - 1 );
	return (int) IdToName.size() - 1;
}
