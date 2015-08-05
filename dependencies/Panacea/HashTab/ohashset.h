#ifndef OHASHSET_H
#define OHASHSET_H

#include <initializer_list>
#include "ohashtable.h"

namespace ohash
{

template <	typename TKey,
			typename THashFunc = func_default< TKey >,
			typename TGetKeyFunc = getkey_self< TKey, TKey >,
			typename TGetValFunc = getval_self< TKey, TKey >
			>
class set : public table< TKey, TKey, THashFunc, TGetKeyFunc, TGetValFunc >
{
public:
	typedef table< TKey, TKey, THashFunc, TGetKeyFunc, TGetValFunc > base;
	// GCC 4 requires this (note the 'typename'). it's something about 'two-phase name lookup'. I don't want to know!
	typedef typename base::iterator iterator;

	set()
	{
	}

	set(std::initializer_list<TKey> list)
	{
		for (size_t i = 0; i < list.size(); i++)
			insert(list.begin()[i]);
	}

	bool insert(const TKey& obj)
	{
		return this->insert_check_exist(obj) != npos;
	}

	/// Wrapper for contains()
	bool operator[](const TKey& obj) const { return contains(obj); }

	/// Calls insert()
	set& operator+=(const TKey& obj)
	{
		insert(obj);
		return *this;
	}

	/// Calls erase()
	set& operator-=(const TKey& obj)
	{
		erase(obj);
		return *this;
	}

	// Stupid C++ overload breaks inheritance rule
	set& operator+=(const set& obj)
	{
		base::operator +=(obj);
		return *this;
	}

	// Stupid C++ overload breaks inheritance rule
	set& operator-=(const set& obj)
	{
		base::operator -=(obj);
		return *this;
	}

#ifdef DVECT_DEFINED
	/// Returns a list of all our keys
	dvect<TKey> keys() const
	{
		dvect<TKey> k;
		for (iterator it = base::begin(); it != base::end(); it++)
			k += *it;
		return k;
	}
#endif

	/** Determine if the set is not equal to set @a b.
	**/
	bool operator!=(const set& b) const
	{
		return !(*this == b);
	}

	/** Determine if the set is equal to set @a b.
	**/
	bool operator==(const set& b) const
	{
		if (base::size() != b.size()) return false;

		for (iterator it = base::begin(); it != base::end(); it++)
			if (!b.contains(*it)) return false;

		return true;
	}

};

}

namespace std
{
template<typename T1, typename T2, typename T3, typename T4> inline void swap(ohash::set<T1,T2,T3,T4>& a, ohash::set<T1,T2,T3,T4>& b)
{
	char tmp[sizeof(ohash::set<T1,T2,T3,T4>)];
	memcpy(tmp, &a, sizeof(a));
	memcpy(&a, &b, sizeof(a));
	memcpy(&b, tmp, sizeof(a));
}
}

#endif
