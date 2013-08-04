#pragma once

#include "../Strings/XString.h"
#include "../Containers/pvect.h"

/** A simple logger.
This is a very simple logger. It is basically an array of strings with
corresponding 'message levels'.
It may be extended in future.
**/
class PAPI SimpleLog
{
public:
	SimpleLog(void);
	~SimpleLog(void);

	struct Item
	{
		int Level;
		XString Message;
	};

	void Log( const XString& msg, int level = 1 );
	
	int ItemCount() const;
	const Item& GetItem( int index ) const;

protected:
	pvect< Item* > Messages;
	Item NullItem;
};
