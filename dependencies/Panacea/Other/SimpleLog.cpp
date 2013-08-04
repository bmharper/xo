#include "pch.h"
#include ".\simplelog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

SimpleLog::SimpleLog(void)
{
	NullItem.Level = 0;
	NullItem.Message = "";
}

SimpleLog::~SimpleLog(void)
{
	delete_all( Messages );
}

void SimpleLog::Log( const XString& msg, int level )
{
	Item *it = new Item();
	it->Message = msg;
	it->Level = level;
	Messages.push_back( it );
}

int SimpleLog::ItemCount() const
{
	return Messages.size();
}

const SimpleLog::Item& SimpleLog::GetItem( int index ) const
{
	if ( index < 0 || index >= Messages.size() ) { ASSERT(false); return NullItem; }
	return *(Messages[index]);
}
