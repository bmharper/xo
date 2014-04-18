#include "pch.h"
#include "nuStringTable.h"
 
nuStringTable::nuStringTable()
{
	// We must do this manually, otherwise nuString will "optimize away" the single character of the string
	Empty.Z = "";

	IdToName += nuTempString(""); // this will end up as a blank string with no heap alloc
	NameToId.insert( IdToName[0], 0 );
}
nuStringTable::~nuStringTable()
{
	// reverse the hack that we establish in the constructor
	Empty.Z = nullptr;
}

const nuString* nuStringTable::GetStr( int id ) const
{
	if ( (uint32) id >= (uint32) IdToName.size() )
		return &Empty;
	return IdToName[id].Z != NULL ? &IdToName[id] : &Empty;
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
