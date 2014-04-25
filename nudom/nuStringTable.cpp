#include "pch.h"
#include "nuStringTable.h"
 
nuStringTable::nuStringTable()
{
	IdToName += "";
	NameToId.insert( IdToName[0], 0 );
}

nuStringTable::~nuStringTable()
{
}

const char* nuStringTable::GetStr( int id ) const
{
	if ( (uint32) id >= (uint32) IdToName.size() )
		return "";
	return IdToName[id];
}

int nuStringTable::GetId( const char* str )
{
	NUASSERT( str != nullptr );
	nuTempString s(str);
	int v = 0;
	if ( NameToId.get( s, v ) )
		return v;
	intp len = s.Length();
	char* copy = (char*) Pool.Alloc( len + 1, false );
	memcpy( copy, str, len );
	copy[len] = 0;
	IdToName += copy;
	NameToId.insert( s, (int) IdToName.size() - 1 );
	return (int) IdToName.size() - 1;
}

void nuStringTable::CloneFrom_Incremental( const nuStringTable& src )
{
	for ( intp i = IdToName.size(); i < src.IdToName.size(); i++ )
	{
		int id = GetId( src.IdToName[i] );
		NUASSERT( id == i );
	}
}
