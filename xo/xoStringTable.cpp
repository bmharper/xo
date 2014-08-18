#include "pch.h"
#include "xoStringTable.h"
 
xoStringTable::xoStringTable()
{
	IdToName += "";
	NameToId.insert( IdToName[0], 0 );
}

xoStringTable::~xoStringTable()
{
}

const char* xoStringTable::GetStr( int id ) const
{
	if ( (uint32) id >= (uint32) IdToName.size() )
		return "";
	return IdToName[id];
}

int xoStringTable::GetId( const char* str )
{
	XOASSERT( str != nullptr );
	xoTempString s(str);
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

void xoStringTable::CloneFrom_Incremental( const xoStringTable& src )
{
	for ( intp i = IdToName.size(); i < src.IdToName.size(); i++ )
	{
		int id = GetId( src.IdToName[i] );
		XOASSERT( id == i );
	}
}
