#include "pch.h"
#include "nuStringTable.h"
 

nuStringTable::nuStringTable()
{
	IdToName += nuTempString(""); // this will end up as a blank string with no heap alloc
	NameToId.insert( IdToName[0], 0 );
}
nuStringTable::~nuStringTable()
{
}

const char* nuStringTable::GetStr( int id ) const
{
	if ( (uint32) id >= (uint32) IdToName.size() )
		return "";
	return IdToName[id].Z != NULL ? IdToName[id].Z : "";
}

int nuStringTable::GetId( const char* str )
{
	nuTempString s(str);
	int v = 0;
	if ( NameToId.get( s, v ) )
		return v;
	IdToName += s;
	NameToId.insert( s, (int) IdToName.size() - 1 );
	return (int) IdToName.size() - 1;
}

void nuStringTable::CloneFrom( const nuStringTable& src )
{
	IdToName = src.IdToName;
	NameToId = src.NameToId;
}
