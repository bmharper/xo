#ifndef SUPP_STD_H
#define SUPP_STD_H

#include <functional>
#include <string>

namespace std
{

class string_compare
{
	less<string> comp;
public:
	enum
	{
		bucket_size = 4,
		min_buckets = 8 
	};

	string_compare( ) : comp() {}
	string_compare( less<string> pred ) : comp( pred )	{	}
	size_t operator( )( const string& key ) const
	{
		unsigned long h = 0; 
		for (size_t i = 0; i < key.length(); i++)
		{
			h = 5 * h + key[i];
		}
		return size_t( h );
	}
	bool operator( )( const string& key1, const string& key2 ) const
	{
		return comp( key1, key2 );
	}
};

}

#endif
