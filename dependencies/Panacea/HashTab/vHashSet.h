#ifndef VHASHSET_H
#define VHASHSET_H

#include "vHashCommon.h"

namespace vHashTables 
{

/// Value Hash Set
/** 
tkey is the key class.
keys must support an equality operator (operator==). 
**/
template <class tkey, class hashFunc = vHashFunction_Cast< tkey > >
class vHashSet
{
public:

	vHashSet()
	{
		aKeys = NULL;
		aState = NULL;
		_size = 0;
		_count = 0;
		age = 0;
		autoShrink = true;
		fillRatio = 2; // keep table 2 times as big as data
	}
	vHashSet(const vHashSet &copy)
	{
		aKeys = NULL;
		aState = NULL;
		*this = copy;
	}
	vHashSet& operator=(const vHashSet &copy) 
	{
		if ( this == &copy ) return *this;

		free_arrays();

		_size = copy._count > 0 ? copy._size : 0;
		_count = copy._count;
		age = copy._count > 0 ? copy.age : 0;
		fillRatio = copy.fillRatio;

		if ( _count > 0 )
		{
			aState = new hashstate_t[ stateArraySize(_size) ];
			memcpy( aState, copy.aState, sizeof(hashstate_t) * stateArraySize(_size) );
			aKeys = new tkey[_size];

			for (hashsize_t i = 0; i < copy._size; i++)
			{
				if ( getState(aState, i) == SFull )
					aKeys[i] = copy.aKeys[i];
			}
		}
		return *this;
	}
	
	~vHashSet() 
	{
		free_arrays();
	}

	/// Clears the set
	void clear()
	{
		free_arrays();
		_size = 0;
		_count = 0;
		age = 0;
	}

	/// Number of elements in set
	hashsize_t size() const
	{
		return _count;
	}

	/// Resize the hashtable.
	/**
	In general this is used to prepare the hashtable for a large number of insertions.
	The hashtable is automatically resized during normal use.
	There are some conditions that apply:
	- If newsize == 0, the table is cleared.
	- If 0 < newsize < 17 then newsize = 17.
	- If newsize < _count * fillRatio then we debug assert, fixup newsize, and continue.
	- If none of the above conditions apply, then newsize = NextPrime( newsize )
	**/
	void resize( hashsize_t newsize )
	{
		if ( newsize == 0 ) { clear(); return; }
		if ( newsize < 17 ) newsize = 17;
		else newsize = NextPrime( newsize );

		if ( newsize < _count * fillRatio )
		{
			ASSERT(false);
			newsize = _count * fillRatio;
		}
		
		tkey		*okeys = aKeys;	// save our old keys
		hashstate_t	*ostate = aState;	// save our old state
		
		// allocate the new keys
		aKeys = new tkey[newsize];
		aState = new hashstate_t[stateArraySize(newsize)];

		// Make all the keys null
		memset(aState, 0, sizeof(hashstate_t) * stateArraySize(newsize));
		
		hashsize_t osize = _size;
		_count = 0;
		_size = newsize;
		age = 0;
		for (hashsize_t i = 0; i < osize; i++)
		{
			KeyState os = getState(ostate, i);
			if ( os == SFull ) insert( okeys[i] );
		}
		if (okeys != NULL) 		delete []okeys;
		if (ostate != NULL) 	delete []ostate;
	}
	
	/// Insert an item into the set
	/**
	\return True if success. False if item already in table.
	**/
	bool insert( const tkey& Key )
	{
		if ( (_count + 1) * fillRatio > _size ) resize( (_size + 1) * 2 );
		if (autoShrink && (age > _count)) resize((hashsize_t) ((double) (_count + 1) * (double) fillRatio * 1.5));

		hashkey_t hkey = hashFunc::GetHashCode(Key);

		// Walk until we find either our key or a completely empty spot. If we encounter any deleted items, but we 
		// do not encounter our key, then we insert at the first deleted item that we encountered.
		hashsize_t pos;
		hashsize_t pos_ins = npos; // remember the first SDeleted position, because that is where we will insert, if we're not already existent
		int i = 0;
		while ( true )
		{
			pos = table_pos( hkey, i++ );
			KeyState state = getState( aState, pos );
			if ( state == SFull )
			{
				if ( aKeys[pos] == Key ) return false; // already in set
			}
			else if ( state == SNull )
			{
				if ( pos_ins == npos ) pos_ins = pos;
				break;
			}
			else if ( state == SDeleted )
			{
				if ( pos_ins == npos ) pos_ins = pos;
			}
		}

		// insert here
		setState( aState, pos_ins, SFull );
		aKeys[pos_ins] = Key;
		_count++;
		return true;
	}

	/// Calls insert()
	vHashSet& operator+=( const tkey& Key )
	{
		insert( Key );
		return *this;
	}

	/// Calls erase()
	vHashSet& operator-=( const tkey& Key )
	{
		erase( Key );
		return *this;
	}

	/// Set merge
	vHashSet& operator+=( const vHashSet& b )
	{
		for ( vHashSet::iterator it = b.begin(); it != b.end(); it++ )
			insert( *it );
		return *this;
	}

	/// Set subtract
	vHashSet& operator-=( const vHashSet& b )
	{
		for ( vHashSet::iterator it = b.begin(); it != b.end(); it++ )
			erase( *it );
		return *this;
	}

	/// Erases an item
	/**
	\return True if the item was found. False if the item was not found.
	**/
	bool erase( const tkey& Key )
	{
		hashsize_t pos = _find(Key);
		if (pos != npos) 
		{
			age++;
			_count--;
			setState(aState, pos, SDeleted);
			return true;
		}
		else
			return false;
	}
		
	/// \deprecated Check if item is in set. VERY non-standard. use contains(), then at least
	/// you can use a derived version of std::hash_set should you wish to.
	/**
	\return True if found. False if not.
	**/
	//bool find( const tkey& Key ) const
	//{
	//	return contains( Key );
	//}

	/// Check if item is in set.
	/**
	\return True if found. False if not.
	**/
	bool contains( const tkey& Key ) const
	{
		hashsize_t pos = _find(Key);
		if (pos != npos) return true;
		else return false;
	}

	/// Wrapper for contains()
	bool operator[] ( const tkey& Key ) const { return contains(Key); }

	/// \internal Searches linearly (for debugging this class)
	bool linearfind( const tkey& Key ) const
	{
		for (hashsize_t i = 0; i < _size; i++)
		{
			if (aKeys[i] == Key) 
			{
				return true;
			}
		}
		return false;
	}

	/// Returns the size of the data array + the size of the state array
	size_t mem_usage() const
	{
		return	sizeof(tkey) * _size +
						sizeof(hashstate_t) * stateArraySize(_size);
	}

protected:
	void free_arrays()
	{
		if (aKeys) delete []aKeys;		aKeys = 0;
		if (aState) delete []aState;	aState = 0;
	}

	/// Returns the position of an item if existent
	int _find( const tkey& Key ) const 
	{
		if ( _size == 0 ) return npos;
		hashkey_t hkey = hashFunc::GetHashCode(Key);

		// table_pos walks over all positions in exactly _size iterations
		int isize = _size;
		for ( int i = 0; i < isize; i++ )
		{
			hashsize_t pos = table_pos( hkey, i );
			KeyState ks = getState( aState, pos );

			if ( ks == SFull )
			{
				if ( aKeys[pos] == Key ) return pos;
			}
			else if ( ks == SNull )
			{
				break;
			}
		}
		return npos;
	}

	hashsize_t table_pos( hashkey_t key, int i ) const
	{
		return (hashsize_t) ( ( (key % _size) + i * (1 + (key % (_size - 1))) ) % _size );
	}

	hashsize_t table_pos( hashkey_t key ) const 
	{
		return (hashsize_t) (key % _size);
	}

	tkey		*aKeys;		///< Key Array
	hashstate_t *aState;	///< State Array (null, occupied, deleted)
	int fillRatio;			///< Minimum ratio of _count : _size
	unsigned int age;		///< Incremented when item is erased
	hashsize_t _size;		///< Size of table
	hashsize_t _count;		///< Number of items in set

public:
	bool autoShrink;///< If true, then the table automatically shrinks itself when it's age reaches a certain limit

	/// Provides a bi-directional iterator through the set.
	/**
	Note that there is no end() iterator provided. Our iterator works differently from the 
	std iterators. Instead of end() we have an \a end property of the iterator that will be 
	flagged if the iterator reaches either end or if the set / table is empty.

	The iterator is bidirectional, but only an iterator to the start of the set is provided.
	Since the hash table provides no explicit ordering of elements, it is irrelevant whether we
	start at the front or the back. The ambiguity caused by adding an end() iterator would far
	outweigh the benefit of being able to iterate from the back.
	**/
	class iterator 
	{
	public:
		iterator()
		{
			_index = -1;
			pos = npos;
			end = false;
			parent = 0;
		}
		iterator( const vHashSet *p, int atend ) 
		{
			_index = -1;
			pos = 0;
			end = true;
			parent = p;
		}
		iterator(const iterator &copy) 
		{
			_index = copy._index;
			pos = copy.pos;
			end = copy.end;
			parent = copy.parent;
		}
		iterator(const vHashSet *p) 
		{
			pos = 0;
			_index = 0;
			end = false;
			parent = const_cast<vHashSet*>(p);
			if (parent->_count == 0 || parent->_size == 0) 
			{
				end = true;
			}
			else 
			{
				// make iterator point to first object if it isn't already so
				KeyState state = getState(parent->aState, pos);
				if (state != SFull)
					(*this)++;
			}
			_index = 0;
		}

		bool operator==( const iterator& b )
		{
			if ( end && b.end ) return true;
			if ( end != b.end ) return false;
			return pos == b.pos;
		}

		bool operator!=( const iterator& b )
		{
			return !( *this == b );
		}

		tkey& operator*() const 
		{
			return parent->aKeys[pos];
		}

		tkey* operator->() const 
		{
			return &parent->aKeys[pos];
		}

		// (int) --> postfix
		/// Increment
		iterator& operator++(int)
		{	 
			if (pos >= parent->_size) return *this;
			pos++;
			while ( pos < parent->_size && getState(parent->aState, pos) != SFull )
			{
				pos++;
			}
			_index++;
			if (pos >= parent->_size) end = true;
			return *this;
		}
		// (int) --> postfix
		/// Decrement
		iterator& operator--(int)
		{	 
			if (pos < 0) return *this;
			pos--;
			while ( pos >= 0 && getState(parent->aState, pos) != SFull )
			{
				pos--;
			}
			_index--;
			if (pos < 0) end = true;
			return *this;
		}

		/// Signals that iteration has ended.
		/**
		This will be flagged on 3 occasions:
		- The set is empty.
		- Forward iteration with operator++ has caused us to step onto the last entity.
		- Backward iteration with operator-- has caused us to step onto the first entity.

		Note that in neither of two latter cases will the iterator point to an object that is
		not inside the set / table.
		**/
		bool end;
		
		/// Returns the index of the current object.
		/**
		The index is zero for the element referred to after begin(), then incremented
		for every operator++, and decremented for every operator--.

		It does not have widespread use, but can be handy in some cases where the set must be 
		referred to as a vector.
		**/
		int index() const 
		{
			return _index;
		}

	protected:
		const vHashSet *parent;
		hashsize_t pos;
		hashsize_t _index; // valid range is [0, _count]
	};

	iterator begin() const 
	{
		return iterator (this);
	}

	iterator end() const 
	{
		return iterator (this, true);
	}

	friend class vHashSet::iterator;

	/** Determine if the set is not equal to set @a b.
	**/
	bool operator!=( const vHashSet& b ) const
	{
		return !( *this == b );
	}

	/** Determine if the set is equal to set @a b.
	**/
	bool operator==( const vHashSet& b ) const
	{
		if ( size() != b.size() ) return false;

		for ( iterator it = begin(); it != end(); it++ )
			if ( !b.contains( *it ) ) return false;

		return true;
	}

};

};


#endif
