#ifndef OHASHMAP_H
#define OHASHMAP_H

#include <initializer_list>
#include "ohashtable.h"

namespace ohash
{

template <	typename TKey,
			typename TVal,
			typename THashFunc = func_default<TKey>,
			typename TGetKeyFunc = getkey_pair<TKey, TVal> ,
			typename TGetValFunc = getval_pair<TKey, TVal>
			>
class map : public table< TKey, std::pair<TKey, TVal>, THashFunc, TGetKeyFunc, TGetValFunc >
{
public:
	typedef std::pair< TKey, TVal > pair_type;
	typedef table< TKey, pair_type, THashFunc, TGetKeyFunc, TGetValFunc > base;
	typedef typename base::iterator iterator;

	map()
	{
	}

	map(std::initializer_list<pair_type> list)
	{
		for (size_t i = 0; i < list.size(); i++)
			insert(list.begin()[i]);
	}

	bool insert(const pair_type& obj)
	{
		return this->insert_check_exist(obj) != npos;
	}

	bool insert(const TKey& key, const TVal& val, bool overwrite = false)
	{
		return this->insert_check_exist(pair_type(key, val), overwrite) != npos;
	}

	// Insert, always overwriting any existing value.
	void set(const TKey& key, const TVal& val)
	{
		insert(key, val, true);
	}

#ifdef DVECT_DEFINED
	/// Returns a list of all our keys
	dvect<TKey> keys() const
	{
		dvect<TKey> k;
		for (iterator it = base::begin(); it != base::end(); it++)
			k += it->first;
		return k;
	}
#endif


	// Get an item in the map.
	// Returns the object if found, or TVal() if not found.
	TVal get(const TKey& key) const
	{
		hashsize_t pos = this->_find(key);
		if (pos != npos) return base::mData[pos].second;
		else return TVal();
	}

	// Get an item in the map.
	// Returns true if the item is in the map, or false if not found.
	bool get(const TKey& key, TVal& val) const
	{
		hashsize_t pos = this->_find(key);
		if (pos != npos)
			val = base::mData[pos].second;
		return pos != npos;
	}

	// Get a pointer to an item in the map
	// Returns a pointer to an item in the set, NULL if object is not in set.
	TVal* getp(const TKey& key) const
	{
		hashsize_t pos = this->_find(key);
		if (pos != npos) return &(base::mData[pos].second);
		else return NULL;
	}

	// const version- does not create object if it does not exist
	const TVal& operator[](const TKey& index) const
	{
		// This was my original implementation, but the fact that npos = -1 means that base::mdata[pos] is usually not
		// an invalid memory location, so we don't trap the error at site.

		// hashsize_t pos = this->_find( index );
		// return base::mData[pos].second;

		// This is better, since it has a much higher likelihood of tripping an access violation
		TVal* p = getp(index);
		return *p;
	}

	TVal& operator[](const TKey& index)
	{
		hashsize_t pos = this->_find(index);
		if (pos == npos)
		{
			// create it.
			pos = this->insert_no_check(pair_type(index, TVal()));
		}
		return base::mData[pos].second;
	}

	bool operator==(const map& other) const
	{
		if (this->size() != other.size())
			return false;

		for (auto& pair : *this)
		{
			auto otherVal = other.getp(pair.first);
			if (otherVal == nullptr)
				return false;
			if (pair.second != *otherVal)
				return false;
		}

		return true;
	}

	bool operator!=(const map& other) const
	{
		return !(*this == other);
	}

};

}

// It is not sufficient to rely on ohash::table's swap. We need to define one for ohash::map, despite the fact that it has the same state
namespace std
{
template<typename T1, typename T2, typename T3, typename T4, typename T5> inline void swap(ohash::map<T1,T2,T3,T4,T5>& a, ohash::map<T1,T2,T3,T4,T5>& b)
{
	char tmp[sizeof(ohash::map<T1,T2,T3,T4,T5>)];
	memcpy(tmp, &a, sizeof(a));
	memcpy(&a, &b, sizeof(a));
	memcpy(&b, tmp, sizeof(a));
}
}

#endif
