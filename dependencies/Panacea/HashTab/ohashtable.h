#ifndef OHASHTABLE_H
#define OHASHTABLE_H

#include "ohashcommon.h"

//#include "IFile.h"

//#define OHASH_PRIME_SIZE

namespace ohash
{

template <	typename TKey,
						typename TData,
						typename THashFunc = ohashfunc_cast< TKey >,
						typename TGetKeyFunc = ohashgetkey_self< TKey, TData >,
						typename TGetValFunc = ohashgetval_self< TKey, TData >
					>
class ohashtable
{
public:

	/** Provides a bi-directional iterator through the set.
	A note on iterator consistency:
	If you want to be able to iterate through a table and delete selected items from it,
	then you must first disable the autoshrink mechanism. Failure to do so will result
	in an invalid iterator.
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
		iterator( const ohashtable *p, int itpos )
		{
			_index = -1;
			pos = itpos;
			end = pos == npos;
			parent = p;
		}
		iterator(const iterator &copy) 
		{
			_index = copy._index;
			pos = copy.pos;
			end = copy.end;
			parent = copy.parent;
		}
		iterator(const ohashtable *p) 
		{
			pos = 0;
			_index = 0;
			end = false;
			parent = const_cast<ohashtable*>(p);
			if (parent->mCount == 0 || parent->mSize == 0) 
			{
				end = true;
			}
			else 
			{
				// make iterator point to first object if it isn't already so
				KeyState state = getState(parent->mState, pos);
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

		TData* operator->() const
		{
			return &parent->mData[pos];
		}
		TData& operator*() const
		{
			return parent->mData[pos];
		}


		// (int) --> postfix
		/// Increment
		iterator& operator++(int)
		{	 
			if (pos >= parent->mSize) return *this;
			pos++;
			while ( pos < parent->mSize && getState(parent->mState, pos) != SFull )
			{
				pos++;
			}
			_index++;
			if (pos >= parent->mSize) end = true;
			return *this;
		}
		// (int) --> postfix
		/// Decrement
		iterator& operator--(int)
		{	 
			if (pos < 0) return *this;
			pos--;
			while ( pos >= 0 && getState(parent->mState, pos) != SFull )
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
		const ohashtable *parent;
		hashsize_t pos;
		hashsize_t _index; // valid range is [0, mCount]
	};

	ohashtable()
	{
		mData = NULL;
		mState = NULL;
		mSize = 0;
		mMask = 0;
		mProbeOffset = 0;
		mCount = 0;
		mAge = 0;
		mAutoShrink = true;
		set_fill_ratio( 2 );
	}
	ohashtable(const ohashtable &copy)
	{
		mData = NULL;
		mState = NULL;
		*this = copy;
	}
	ohashtable& operator=(const ohashtable &copy) 
	{
		if ( this == &copy ) return *this;

		free_arrays();

		mSize = copy.mCount > 0 ? copy.mSize : 0;
		mMask = copy.mMask;
		mProbeOffset = copy.mProbeOffset;
		mCount = copy.mCount;
		mAge = copy.mCount > 0 ? copy.mAge : 0;
		mFillRatio = copy.mFillRatio;
		mFillRatioMul4 = copy.mFillRatioMul4;

		if ( mCount > 0 )
		{
			mState = new hashstate_t[ stateArraySize(mSize) ];
			memcpy( mState, copy.mState, sizeof(hashstate_t) * stateArraySize(mSize) );
			mData = new TData[mSize];

			for (hashsize_t i = 0; i < copy.mSize; i++)
			{
				if ( getState(mState, i) == SFull )
					mData[i] = copy.mData[i];
			}
		}
		return *this;
	}
	
	~ohashtable() 
	{
		free_arrays();
	}

	/// Clears the set
	void clear()
	{
		free_arrays();
		mSize = 0;
		mMask = 0;
		mProbeOffset = 0;
		mCount = 0;
		mAge = 0;
	}

	/// Clears the set, but keeps our raw size the same
	void clear_noalloc()
	{
		mCount = 0;
		mAge = 0;

		if ( mSize != 0 )
			memset(mState, 0, sizeof(hashstate_t) * stateArraySize(mSize));

		for ( hashsize_t i = 0; i < mSize; i++ )
			mData[i] = TData();
	}

	/// Number of elements in set
	hashsize_t size() const
	{
		return mCount;
	}

	/// Raw size of table
	hashsize_t raw_size() const
	{
		return mSize;
	}

	/// Resize the hashtable.
	/**
	In general this is used to prepare the hashtable for a large number of insertions.
	The hashtable is automatically resized during normal use.
	There are some conditions that apply:
	- If newsize == 0, the table is cleared.
	- If 0 < newsize < 17 then newsize = 17.
	- If newsize < mCount * mFillRatio then we debug assert, increase newsize, and proceed.
	- If none of the above conditions apply, then newsize = NextPrime( newsize )
	**/
	void resize( hashsize_t newsize )
	{
		if ( newsize == 0 ) { clear(); return; }

		if ( newsize < (hashsize_t) (mCount * mFillRatio) )
		{
			ASSERT(false);
			newsize = (hashsize_t) (mCount * mFillRatio);
		}

#ifdef OHASH_PRIME_SIZE
		if ( newsize < 17 ) newsize = 17;
		else newsize = NextPrime( newsize );
#else
		if ( newsize < 2 ) newsize = 2;
		else newsize = NextPowerOf2( newsize );
#endif

		TData		*odata = mData;		// save our old data
		hashstate_t	*ostate = mState;	// save our old state
		
		// allocate the new keys
		mData = new TData[newsize];
		mState = new hashstate_t[stateArraySize(newsize)];

		// Make all the states null
		memset(mState, 0, sizeof(hashstate_t) * stateArraySize(newsize));
		
		hashsize_t osize = mSize;
		mCount = 0;
		mSize = newsize;
		mMask = mSize - 1;
		mProbeOffset = probeoffset(mSize);
		mAge = 0;
		for (hashsize_t i = 0; i < osize; i++)
		{
			KeyState os = getState(ostate, i);
			if ( os == SFull ) insert_no_check( odata[i] );
		}
		if (odata != NULL) 		delete []odata;
		if (ostate != NULL) 	delete []ostate;
	}
	

	/// Erases an item
	/**
	\return True if the item was found. False if the item was not found.
	**/
	bool erase( const TKey& obj )
	{
		hashsize_t pos = _find( obj );
		if ( pos != npos ) 
		{
			mAge++;
			mCount--;
			setState( mState, pos, SDeleted );
			autoshrink();
			return true;
		}
		else
			return false;
	}
		
	bool contains( const TKey& key ) const
	{
		return _find( key ) != npos;
	}

	void set_fill_ratio( float ratio )
	{
		mFillRatio = ratio;
		mFillRatioMul4 = ratio * 4;
	}

	iterator find( const TKey& key ) 
	{
		return iterator( this, _find( key ) );
	}

	/// \internal Searches linearly (for debugging this class)
	bool linearfind( const TKey& key ) const
	{
		for (hashsize_t i = 0; i < mSize; i++)
		{
			if ( TGetKeyFunc::getkey( mData[i] ) == key ) 
			{
				return true;
			}
		}
		return false;
	}

	/// Returns the size of the data array + the size of the state array
	size_t mem_usage() const
	{
		return	sizeof(TData) * mSize +
						sizeof(hashstate_t) * stateArraySize(mSize);
	}

	/// Merge
	ohashtable& operator+=( const ohashtable& b )
	{
		for ( iterator it = b.begin(); it != b.end(); it++ )
			insert_check_exist( *it );
		return *this;
	}

	/// Subtract
	ohashtable& operator-=( const ohashtable& b )
	{
		for ( iterator it = b.begin(); it != b.end(); it++ )
			erase( TGetKeyFunc::getkey( *it ) );
		return *this;
	}


	/** Serializes the table to a file. Since this is a memory dump, it should only be used on tables whos elements contain no pointers.
	After serialization the hash table is useless, and must not be touched again until after calling deserialize_pod.
	@param mem The buffer in which to serialize.
	@param bytes The size of the buffer. If the size is too small, then nothing will be written, and bytes will contain the necessary size.
	**/
	void serialize_pod( void* mem, size_t& bytes )
	{
		size_t s1 = mSize * sizeof(TData);
		size_t s2 = stateArraySize( mSize ) * sizeof(hashstate_t);
		size_t required = s1 + s2;
		if ( bytes < required )
		{
			bytes = required;
			return;
		}
		if ( s1 + s2 == 0 ) return;
		if ( mem == NULL ) { ASSERT(false); return; }
		BYTE* bmem = (BYTE*) mem;
		memcpy( bmem, mData, s1 );
		memcpy( bmem + s1, mState, s2 );
		free_arrays();
	}

	/** Deserializes the table from a file.
	No checks are made to ensure that you haven't touched the vector since it was serialized.
	**/
	void deserialize_pod( const void* mem )
	{
		ASSERT( mSize > 0 );
		ASSERT( mData = NULL );

		mState = new hashstate_t[ stateArraySize(mSize) ];
		mData = new TData[ mSize ];

		size_t s1 = mSize * sizeof(TData);
		size_t s2 = stateArraySize( mSize ) * sizeof(hashstate_t);
		if ( s1 + s2 == 0 ) return;
		if ( mem == NULL ) { ASSERT(false); return; }
		const BYTE* bmem = (const BYTE*) mem;
		memcpy( mData, bmem, s1 );
		memcpy( mState, bmem + s1, s2 );
	}

	hashsize_t debug_table_pos( const TKey& obj, int i ) const
	{
		hashkey_t hkey = THashFunc::gethashcode( obj );
		hashsize_t pos = table_pos( hkey, i );
		return pos;
	}

protected:

	hashkey_t get_hash_code( const TData& obj )
	{
		return THashFunc::gethashcode( TGetKeyFunc::getkey( obj ) );
	}

	void insert_check_resize()
	{
		// the +1 is the the one we're about to insert. The +3 is because of our 4x multiple.
		hashsize_t newSize = ((mCount + 1 + 3) / 4) * mFillRatioMul4;
		if ( newSize > mSize ) resize( (mSize + 1) * 2 );
	}

	/// Insert an item into the set without checking if it exists. Returns position of insertion.
	hashsize_t insert_no_check( const TData& obj )
	{
		insert_check_resize();
		autoshrink();
		hashkey_t hkey = get_hash_code( obj );
		hashsize_t pos = table_pos( hkey );
		KeyState state = getState( mState, pos );
		if ( state != SFull ) 
		{
			// insert immediately
			setState( mState, pos, SFull );
			mData[pos] = obj;
			mCount++;
		}
		else 
		{
			// Search for an empty slot
			int i = 0;
			while ( state == SFull ) 
			{
				pos = table_pos( hkey, ++i );
				state = getState( mState, pos );
				if ( i >= (int) mSize ) ASSERT( false );
			}
			// insert here
			setState(mState, pos, SFull);
			mData[pos] = obj;
			mCount++;
		}
		return pos; 
	}

	/** Insert an item into the set.
	
	@param overwrite If true, then we overwrite any existing value for the specified key. This is a specialization
		that is only applicable to hash maps (not hash sets).

	@return npos if item already in table (only possible if overwrite is false).

	**/
	hashsize_t insert_check_exist( const TData& obj, bool overwrite = false )
	{
		insert_check_resize();
		autoshrink();

		hashkey_t hkey = get_hash_code( obj );

		// We insert at the first deleted slot, or the first null slot, whichever comes first
		// However, we must scan until (1. Find existing) or (2. Scanned entire table)
		hashsize_t pos = npos;
		hashsize_t pos_ins = npos;	// remember the first SDeleted position, because that is where we will insert, if we're not already existent
		for ( uint i = 0; i != mSize; i++ )
		{
			pos = table_pos( hkey, i );
			KeyState ks = getState( mState, pos );
			if ( ks == SFull )
			{
				if ( TGetKeyFunc::getkey(mData[pos]) == TGetKeyFunc::getkey(obj) )
				{
					// key already present
					if ( overwrite )	{ TGetValFunc::getval_noconst(mData[pos]) = TGetValFunc::getval(obj); return pos; }
					else				return npos; 
				}
			}
			else if ( ks == SNull )
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
		setState( mState, pos_ins, SFull );
		mData[pos_ins] = obj;
		mCount++;
		return pos_ins;
	}

	void free_arrays()
	{
		delete []mData;		mData = NULL;
		delete []mState;	mState = NULL;
	}

	/// Returns the position of an item if existent
	int _find( const TKey& key ) const 
	{
		// Since this condition will be very rare, it's OK that we check the first item twice (because it avoids one branch, and is easier to read).
		//if ( pos == first ) break;

		if ( mSize == 0 ) return npos;
		hashkey_t hkey = THashFunc::gethashcode( key );
		hashsize_t pos = table_pos( hkey );
		hashsize_t first = pos;
		// quick positive/empty check
		KeyState ks = getState(mState, pos);
		if ( ks == SFull && TGetKeyFunc::getkey( mData[pos] ) == key ) return pos;
		else if ( ks == SNull ) return npos;
		else 
		{ 
			// exhaustive
			int i = 0;
			pos = table_pos( hkey, ++i );
			while ( getState(mState, pos) != SNull ) 
			{
				if ( getState(mState, pos) == SFull && TGetKeyFunc::getkey( mData[pos] ) == key ) return pos;
				pos = table_pos( hkey, ++i );
				if ( pos == first ) break;
			}
			return npos;
		}
	}

	/// Erases all instances of key. Returns number of items erased.
	int _erase_all( const TKey& key )
	{
		if ( mSize == 0 ) return 0;
		hashkey_t hkey = THashFunc::gethashcode( key );
		hashsize_t pos = table_pos( hkey );
		hashsize_t first = pos;
		int del = 0;
		int i = 0;
		while ( true )
		{
			KeyState state = getState(mState, pos);
			if ( state == SFull && TGetKeyFunc::getkey( mData[pos] ) == key ) 
			{
				del++;
				setState( mState, pos, SDeleted );
			}
			else if ( state == SNull ) break;
			pos = table_pos( hkey, ++i );
			if ( pos == first ) break;
		}
		return del;
	}

#ifdef OHASH_PRIME_SIZE
	hashsize_t table_pos( hashkey_t key ) const
	{
		return ohash::table_pos( mSize, key );
	}

	hashsize_t table_pos( hashkey_t key, uint i ) const
	{
		return ohash::table_pos( mSize, key, i );
	}
#else
	hashsize_t table_pos( hashkey_t key ) const
	{
		return ohash::table_pos( mMask, key );
	}

	hashsize_t table_pos( hashkey_t key, uint i ) const
	{
		return ohash::table_pos( mMask, mProbeOffset, key, i );
	}
#endif

	void autoshrink()
	{
		if ( mAutoShrink && mAge * 2 > mCount ) resize( (hashsize_t) ((double) (mCount + 1) * mFillRatio * 1.5) );
	}

	TData			*mData;			///< Key Array
	hashstate_t		*mState;		///< State Array (null, occupied, deleted)
	float			mFillRatio;		///< Minimum ratio of mSize / mCount 
	hashsize_t		mFillRatioMul4;	///< Fill ratio * 4
	unsigned int	mAge;			///< Incremented when item is erased
	hashsize_t		mSize;			///< Size of table
	hashsize_t		mProbeOffset;	///< Initial offset of probe	(only used when not OHASH_PRIME_SIZE)
	hashsize_t		mMask;			///< Size of table - 1			(only used when not OHASH_PRIME_SIZE)
	hashsize_t		mCount;			///< Number of items in set
	bool			mAutoShrink;	///< If true, then the table automatically shrinks itself when it's mAge reaches a certain limit
public:

	/** Enables or disables auto-shrinking. 
	Auto-shrinking needs to be disabled if you wish to iterate through the set and erase items as you are going.
	**/
	void auto_shrink( bool on )
	{ 
		if ( on == mAutoShrink ) return;
		mAutoShrink = on;
		if ( on ) autoshrink();
	}

	iterator begin() const 
	{
		return iterator (this);
	}

	iterator end() const 
	{
		return iterator (this, npos);
	}

	friend class ohashtable::iterator;
};


}
#endif

/*
Timings:

Before adding fold() functions:
L1cache    ohash<int,int> insertion                    40.88
L1cache    ohash<int64,int64> insertion                48.18
L1cache    ohash<int,int> lookup                       15.55
L1cache    ohash<int64,int64> lookup                   18.99

After adding fold() functions:
L1cache    ohash<int,int> insertion                    41.73
L1cache    ohash<int64,int64> insertion                49.27
L1cache    ohash<int,int> lookup                       19.40
L1cache    ohash<int64,int64> lookup                   21.07

Alternate implementation of fold, which does the fold decision on every lookup,
instead of using a function pointer:
L1cache    ohash<int,int> insertion                    43.94
L1cache    ohash<int64,int64> insertion                49.06
L1cache    ohash<int,int> lookup                       19.43
L1cache    ohash<int64,int64> lookup                   20.74

With fixed, non-configurable mixing
L1cache    ohash<int,int> insertion                    45.41
L1cache    ohash<int64,int64> insertion                52.32
L1cache    ohash<int,int> lookup                       19.90
L1cache    ohash<int64,int64> lookup                   22.03

*/