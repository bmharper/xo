#ifndef VHASHMAP_H
#define VHASHMAP_H

#include "vHashCommon.h"

namespace vHashTables 
{

/// Value Hash Map
/** 
tkey is the key class.
tval is the value class.
keys must support an equality operator (operator==). 
**/

template <class tkey, class tval, class hashFunc = vHashFunction_Cast< tkey > >
class vHashMap
{
public:
	typedef std::pair< tkey, tval > pair_type;


	typedef std::pair< const tkey, tval > iterator_pair_type;
	typedef std::pair< const tkey, const tval > iterator_pair_type_const;

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
		iterator( const vHashMap *p, hashsize_t _pos ) 
		{
			_index = -1;
			pos = _pos == npos ? p->_size : _pos;
			end = _pos >= p->_size || _pos == npos;
			parent = p;
		}
		iterator(const iterator &copy) 
		{
			_index = copy._index;
			pos = copy.pos;
			end = copy.end;
			parent = copy.parent;
		}
		iterator( const vHashMap *p )
		{
			pos = 0;
			_index = -1;
			end = false;
			parent = const_cast<vHashMap*>(p);
			if (parent->_count == 0 || parent->_size == 0) 
			{
				end = true;
			}
			else 
			{
				// make iterator point to first object if it isn't already so
				KeyState state = getState(parent->_state, pos);
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

		tval* valp() const
		{
			return &(parent->_data[pos].second);
		}
		tval& val() const
		{
			return parent->_data[pos].second;
		}
		iterator_pair_type* operator->() const
		{
			return (iterator_pair_type*) &parent->_data[pos];
		}
		iterator_pair_type& operator*() const
		{
			return (iterator_pair_type&) parent->_data[pos];
		}
		tkey& key() const 
		{
			return parent->_data[pos].first;
		}
		tkey* keyp() const 
		{
			return &( parent->_data[pos].first );
		}
		iterator& operator++()
		{
			return (*this)++;
		}
		iterator& operator--()
		{
			return (*this)--;
		}
		// (int) --> postfix
		/// Increment
		iterator& operator++(int) 
		{	 
			if (pos >= parent->_size) return *this;
			pos++;
			while ( pos < parent->_size && getState(parent->_state, pos) != SFull )
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
			if (pos <= 0) return *this;
			pos--;
			while ( pos >= 0 && getState(parent->_state, pos) != SFull )
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
		const vHashMap *parent;
		hashsize_t pos;
		hashsize_t _index;
	};

	iterator begin() const 
	{
		return iterator( this );
	}

	iterator end() const 
	{
		return iterator( this, npos );
	}

	friend class iterator;



	/// Creates the map with a size of 0. ie. Performs no allocations.
	vHashMap(void) 
	{
		_data = 0;
		_state = NULL;
		_size = 0;
		_count = 0;
		age = 0;
		autoShrink = true;
		fillRatio = 2; // keep table 2 times as big as _data
	}
	~vHashMap(void)
	{
		free_arrays();
	}

	vHashMap(const vHashMap &copy)
	{
		_data = 0;
		_state = NULL;
		*this = copy;
	}
	
	vHashMap& operator=(const vHashMap& copy) 
	{
		if ( this == &copy ) return *this;

		free_arrays();

		_size = copy._count > 0 ? copy._size : 0;
		_count = copy._count;
		age = copy._count > 0 ? copy.age : 0;
		fillRatio = copy.fillRatio;

		if ( _count > 0 )
		{
			_state = new hashstate_t[ stateArraySize(_size) ];
			memcpy( _state, copy._state, sizeof(hashstate_t) * stateArraySize(_size) );
			_data = new pair_type[ _size ];

			for (hashsize_t i = 0; i < copy._size; i++)
			{
				if ( getState(_state, i) == SFull )
					_data[i] = copy._data[i];
			}
		}

		return *this;
	}


	/// Clears the map
	void clear()
	{
		if ( _size == 0 && _count == 0 ) return;
		free_arrays();
		_size = 0;
		_count = 0;
		age = 0;
	}

	/// Number of elements in map
	hashsize_t size() const
	{
		return _count;
	}

	/// Retrieve raw size of map
	hashsize_t raw_size() const
	{
		return _size;
	}

	/// Resize the hashtable.
	/**
	In general this is used to prepare the hashtable for a large number of insertions.
	The hashtable is automatically resized during normal use.
	There are some conditions that apply:
	- If newsize == 0, the table is cleared.
	- If 0 < newsize < 17 then newsize = 17.
	- If newsize < _count * fillRatio then we debug assert, increase newsize, and proceed.
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

		pair_type		*odata = _data;	// save old _data
		hashstate_t *ostate = _state;	// save our old state

		_data = new pair_type[newsize];
		_state = new hashstate_t[ stateArraySize( newsize ) ];

		// Make all the keys null
		memset(_state, 0, sizeof( hashstate_t ) * stateArraySize(newsize) );

		int osize = _size;
		_count = 0;
		_size = newsize;
		age = 0;
		for (int i = 0; i < osize; i++) 
		{
			if ( getState(ostate, i) == SFull ) 
				insert( odata[i] );
		}
		if ( odata )	delete []odata;
		if ( ostate ) delete []ostate;
	}

	bool insert( const pair_type& obj ) 
	{
		return insert( obj.first, obj.second );
	}

	/** Insert, always overwriting any existing value. 
	**/
	void set( const tkey& Key, const tval& Val )
	{
		insert( Key, Val, true );
	}


	/** Insert an item into the set.

	@param overwrite If true, then we overwrite any existing value for the key.
	
	@return True if success. False if item already in table (only possible if overwrite is false).

	**/
	bool insert( const tkey& Key, const tval& Val, bool overwrite = false ) 
	{
		if ( (_count + 1) * fillRatio > _size ) resize( (_size + 1) * 2 );
		if (autoShrink && (age > _count)) resize((hashsize_t) ((double) (_count + 1) * (double) fillRatio * 1.5));

		hashkey_t hkey = hashFunc::GetHashCode(Key);

		// We insert at the first deleted slot, or the first null slot, whichever comes first
		// However, we must scan until (1. Find existing) or (2. Scanned entire table)
		hashsize_t pos = npos;
		hashsize_t pos_ins = npos; // remember the first SDeleted position, because that is where we will insert, if we're not already existent
		for ( int i = 0; i != _size; i++ )
		{
			pos = table_pos( hkey, i );
			KeyState state = getState( _state, pos );
			if ( state == SFull )
			{
				if ( _data[pos].first == Key )
				{
					// already exists
					if ( overwrite )	{ _data[pos].second = Val; return true; }
					else				return false; 
				}
			}
			else if ( state == SNull )
			{
				if ( pos_ins == npos ) pos_ins = pos;
				break;
			}
			else /* if ( state == SDeleted ) */
			{
				if ( pos_ins == npos ) pos_ins = pos;
			}
		}

		// insert here
		setState( _state, pos_ins, SFull );
		_data[pos_ins] = pair_type( Key, Val );
		_count++;
		return true;


		/*
		hashkey_t hkey = hashFunc::GetHashCode(Key);
		hashsize_t pos = table_pos( hkey );
		KeyState state = getState( _state, pos );
		if ( state != SFull ) 
		{
			// insert immediately
			setState( _state, pos, SFull );
			_data[pos] = pair_type( Key, Val );
			_count++;
			return true;
		}
		else 
		{
			if ( _data[pos].first == Key )
			{
				// already exists
				if ( overwrite )	{ _data[pos].second = Val; return true; }
				else				return false; 
			}
			int i = 0;
			pos = table_pos( hkey, ++i );
			// Search for an empty slot
			KeyState state = getState( _state, pos );
			while ( state == SFull ) 
			{
				if ( _data[pos].first == Key )
				{
					// already exists
					if ( overwrite )	{ _data[pos].second = Val; return true; }
					else				return false; 
				}
				//search( token, pos );
				pos = table_pos( hkey, ++i );
				state = getState(_state, pos);
			}
			// insert here
			setState( _state, pos, SFull );
			_data[pos] = pair_type( Key, Val );
			_count++;
			return true;
		}
		*/
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
			setState( _state, pos, SDeleted );
			return true;
		}
		else
			return false;
	}

	/// Returns true if the object is contained in the map
	bool contains( const tkey& Key ) const
	{
		return _find(Key) != npos;
	}

	/// Get an item in the set
	/**
	\return The object if found, or tval() if not found.
	**/
	tval get( const tkey& Key ) const 
	{
		hashsize_t pos = _find(Key);
		if (pos != npos) return _data[pos].second;
		else return tval();
	}

	/// Get a pointer to an item in the map
	/**
	\return A pointer to an item in the set, NULL if object is not in set.
	**/
	tval* getp( const tkey& Key ) const 
	{
		hashsize_t pos = _find(Key);
		if (pos != npos) return &(_data[pos].second);
		else return NULL;
	}

	/// Access operator (const)
	/**
	The const version throws vHashNotFoundException if the object does not exist.<br>
	**/
	const tval& operator[] (const tkey& Key) const
	{
		// Make sure the element exists
		tval* ob = getp( Key );
		return *ob; // This will cause an access violation if the value does not exist
		//if (ob == NULL)
		//{
		//	throw vHashNotFoundException();
		//}
		//else 
		//	return *ob;
	}

	/// Access operator
	/**
	The non-const version ensures that the element specified by Key exists.<br>
	It can thus be used to populate the hash table.
	**/
	tval& operator[] (const tkey& Key)
	{
		// Make sure the element exists
		tval* ob = getp( Key );
		if (ob == NULL)
		{
			insert( Key, tval() );
			return *getp( Key );
		}
		else
			return *ob;
	}

	/// Returns the size of the data array + the size of the state array
	size_t mem_usage() const
	{
		return	sizeof(pair_type) * _size +
						sizeof(hashstate_t) * stateArraySize(_size);
	}

	/// Returns a list of all our keys
	dvect<tkey> keys() const
	{
		dvect<tkey> k;
		for ( iterator it = begin(); it != end(); it++ )
			k += it->first;
		return k;
	}

protected:

	void free_arrays()
	{
		if (_data) delete []_data;		_data = 0;
		if (_state) delete []_state;	_state = 0;
	}

	// returns the position of an item if existent
	int _find(const tkey& Key) const 
	{
		if ( _size == 0 ) return npos;
		hashkey_t hkey = hashFunc::GetHashCode(Key);

		// table_pos walks over all positions in exactly _size iterations
		int isize = _size;
		for ( int i = 0; i < isize; i++ )
		{
			hashsize_t pos = table_pos( hkey, i );
			KeyState ks = getState( _state, pos );

			if ( ks == SFull )
			{
				if ( _data[pos].first == Key ) return pos;
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


	pair_type		*_data;
	hashstate_t		*_state;
	int fillRatio;
	hashsize_t age;
	hashsize_t _size; // size of table
	hashsize_t _count;
public:
	bool autoShrink;

};

};


#endif

