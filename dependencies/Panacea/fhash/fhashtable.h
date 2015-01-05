
/* fhash - my final hash table (!).
I wrote this because I was sick of agonizing over the code bloat every time I wanted to add a new hash table.
So this basically uses a table of function pointers.

Examples
========

string -> string
----------------

namespace space { class mystring { uint32 GetHashCode() const {...} }; }

// This must be done in the global namespace
FHASH_SETUP_CLASS_GETHASHCODE( space::mystring, mystring );

fhashmap<mystring,mystring> t;
t.insert( "a", "A" );



ptr* -> int32			Using the address as the hash code
--------------------------------------------------------

struct myobj {}

// As always, this must be written in the global namespace
FHASH_SETUP_POINTER_ADDRESS( myobj );



object -> int32			Using a member function of the form "uint GetHashCode() const"
------------------------------------------------------------------------------------

struct myobj { uint32 GetHashCode() const {...}; }

FHASH_SETUP_CLASS_GETHASHCODE( myobj, myobj );



object -> int32			Using your own hash function
------------------------------------------------------

struct myobj {}


FHASH_SETUP_CLASS_CTOR_DTOR( myobj );	// For classes
FHASH_SETUP_POD_CTOR_DTOR( myobj );		// For PODs that you want default-initialized
FHASH_SETUP_TOR_NOP( myobj );			// For PODs that you want no initialization performed upon

template<> inline uint32 fhash_gethash(const myobj& obj) { return obj.x ^ obj.y; }


BMH
*/

#ifndef FHASHTABLE_H
#define FHASHTABLE_H

#include <malloc.h>
#include <memory.h>

#ifndef FHASH_NORETURN
#	ifdef _MSC_VER
#		define FHASH_NORETURN __declspec(noreturn)
#	else
#		define FHASH_NORETURN __attribute__ ((noreturn))
#	endif
#endif

#ifndef PAPI
#define PAPI
#endif
#ifndef ASSERT
//#define ASSERT(condition) if !(condition) { int* p = 0; *p = 0; }
#define ASSERT(condition) (void)0
#endif
#ifndef NULL
#define NULL    0
#endif

#ifdef _MSC_VER
#ifndef __cplusplus_cli
// 7% speedup over __cdecl - but annoying warnings from CLR compilation
#define FHASH_CALL __fastcall
#else
#define FHASH_CALL
#endif
#else
#define FHASH_CALL
#endif

typedef signed char			int8;
typedef unsigned char		uint8;

typedef short				int16;
typedef unsigned short		uint16;

typedef          int		int32;
typedef unsigned int		uint32;

#ifdef _MSC_VER
typedef          __int64	int64;
typedef unsigned __int64	uint64;
#else
typedef int64_t				int64;
typedef uint64_t			uint64;
#endif

typedef unsigned char		fhashstate_t;
typedef unsigned char		byte;
typedef unsigned int		uint;
typedef uint32				fhashkey_t;

static const size_t fhash_npos = -1;
static const size_t fhash_min_autoshrink_count = 32;

inline size_t fhash_next_power_of_2(size_t v)
{
	size_t s = 1;
	while (s < v) s <<= 1;
	return s;
}

inline FHASH_NORETURN void fhash_die()
{
	*((int*)0) = 0;
	while (1) {} // necessary to satisfy GCC
}

// _tor is ctor or dtor
enum fhash_tor_type
{
	fhash_TOR_FUNC,	///< Function call
	fhash_TOR_ZERO,	///< memset(0) -- illegal for dtor
	fhash_TOR_NOP	///< nop
};

enum fhash_key_states
{
	// can't have more than 4... only 2 bits available
	fhash_Null = 0,
	fhash_Full = 1,
	fhash_Deleted = 2,
	fhash_ERROR = 3
};

inline size_t fhash_state_array_size(size_t asize)
{
	// every item needs 2 bits, so that's 16 items per int32
	return (asize / 4) + 8;
}

// returns the state of a given position in the table
inline fhash_key_states fhash_get_state(fhashstate_t state_array[], size_t pos)
{
	size_t        bytepos = pos >> 2;
	unsigned char bitpos = pos & 3;
	unsigned char masks[4] = { 3, 12, 48, 192 };
	fhash_key_states ks = (fhash_key_states)((state_array[bytepos] & masks[bitpos]) >> (bitpos << 1));
	return ks;
}

inline void fhash_set_state(fhashstate_t state_array[], size_t pos, fhash_key_states newState)
{
	size_t        bytepos = pos >> 2;
	unsigned char bitpos = pos & 3;
	unsigned char masks[4] = { (unsigned char) ~3, (unsigned char) ~12, (unsigned char) ~48, (unsigned char) ~192 };
	unsigned char state = state_array[bytepos] & masks[bitpos];
	state |= newState << (bitpos << 1);
	state_array[bytepos] = state;
}

typedef void	(FHASH_CALL *fhash_func_xfer)(void* dst, const void* src, size_t obj_size);
typedef void	(FHASH_CALL *fhash_func_ctor)(void* obj);
typedef void	(FHASH_CALL *fhash_func_dtor)(void* obj);
typedef int	(FHASH_CALL *fhash_func_keycmp)(const void* a, const void* b);
typedef uint32(FHASH_CALL *fhash_func_gethash)(const void* obj, size_t obj_size);

inline void fhash_null_ctor(void* obj) {}
inline void fhash_null_dtor(void* obj) {}
inline void FHASH_CALL fhash_pod_move(void* dst, const void* src, size_t obj_size) { memcpy(dst, src, obj_size); }
inline void FHASH_CALL fhash_pod_copy(void* dst, const void* src, size_t obj_size) { memcpy(dst, src, obj_size); }

// Types of constructors and destructors required by your type
template< typename T > void fhash_tor_types(fhash_tor_type& ctor, fhash_tor_type& dtor) { ctor = fhash_TOR_FUNC; dtor = fhash_TOR_FUNC; }

// Use operator= to copy a value
template< typename T >
void FHASH_CALL fhash_type_copy(void* dst, const void* src, size_t obj_size)
{
	T* tdst = (T*) dst;
	const T* tsrc = (const T*) src;
	*tdst = *tsrc;
}

// Use operator= for pairs of tightly packed Key,Value objects
template< typename K, typename V >
void FHASH_CALL fhash_type_copy_pair(void* dst, const void* src, size_t obj_size)
{
	K* kdst = (K*) dst;
	V* vdst = (V*)(kdst + 1);
	K* ksrc = (K*) src;
	V* vsrc = (V*)(ksrc + 1);
	*kdst = *ksrc;
	*vdst = *vsrc;
}

template<typename TPod>
int FHASH_CALL fhash_pod_keycmp(const void* a, const void* b) { return !(*((const TPod*)a) == *((const TPod*)b)); }

template<typename T>
int FHASH_CALL fhash_keycmp(const void* a, const void* b) { return fhash_pod_keycmp<T>(a,b); }

inline uint32 fhash_gethash_int32(const void* obj) { return *((const uint32*)obj); }
inline uint32 fhash_gethash_int64(const void* obj) { uint64 v = *((const uint64*)obj); return uint32(v ^ (v >> 32)); }

#if ARCH_64
inline uint32 fhash_gethash_ptr(const void* ptr) { uint64 v = reinterpret_cast<uint64>(ptr); return uint32(v) ^ uint32(v >> 32); }
#else
inline uint32 fhash_gethash_ptr(const void* ptr) { return reinterpret_cast<uint32>(ptr); }
#endif

// Provide a specialization of these for your type
template< typename T > uint32 fhash_gethash(const T& obj);

template<> inline uint32 fhash_gethash(const int8& v)		{ return (uint32) v; }
template<> inline uint32 fhash_gethash(const uint8& v)		{ return (uint32) v; }
template<> inline uint32 fhash_gethash(const int16& v)		{ return (uint32) v; }
template<> inline uint32 fhash_gethash(const uint16& v)		{ return (uint32) v; }
template<> inline uint32 fhash_gethash(const int32& v)		{ return fhash_gethash_int32(&v); }
template<> inline uint32 fhash_gethash(const uint32& v)		{ return fhash_gethash_int32(&v); }
template<> inline uint32 fhash_gethash(const int64& v)		{ return fhash_gethash_int64(&v); }
template<> inline uint32 fhash_gethash(const uint64& v)		{ return fhash_gethash_int64(&v); }

template<typename T> uint32 FHASH_CALL fhash_gethash_gen_wrap(const void* v, size_t obj_size)
{
	return fhash_gethash<T>(*((const T*)v));
}

// hash the pointer itself - not what it points to
template<> inline uint32 FHASH_CALL fhash_gethash_gen_wrap<void*>(const void* v, size_t obj_size)
{
	return fhash_gethash_ptr(v);
}

// constructor of your type
template< typename T > void fhash_ctor(T& obj);

// destructor of your type
template< typename T > void fhash_dtor(T& obj);

// Define NULL constructors and destructors for PODs
// Hm. probably better to initialize to zero than to leave uninitialized, because of partial writes.
#define FHASH_SETUP_POD_CTOR_DTOR(T) \
	template<> inline void fhash_ctor(T& obj) { obj = T(); } \
	template<> inline void fhash_dtor(T& obj) {}

#define FHASH_SETUP_TOR_NOP(T) \
	template<> inline void fhash_ctor(T& obj) {} \
	template<> inline void fhash_dtor(T& obj) {}

// POD, ctor=nop dtor=nop
#define FHASH_SETUP_POD(T) \
	FHASH_SETUP_TOR_NOP(T) \
	template<> inline void fhash_tor_types<T>( fhash_tor_type& ctor, fhash_tor_type& dtor ) { ctor = fhash_TOR_NOP; dtor = fhash_TOR_NOP; }

// Create pass-through placement new (and delete, to satisfy compiler) operators for the type
// Note: We memset(0) after destroying. This is because most destructors are not callable more than once, but we need to be.
// This won't work for everything, but at least it covers pointers inside objects.
#define FHASH_SETUP_CLASS_CTOR_DTOR(NST,T) \
	inline void* operator new( size_t bytes, NST* pos ) { return pos; } \
	inline void  operator delete( void* obj, NST* pos ) {} \
	template<> inline void fhash_ctor(NST& obj) { new(&obj) NST(); } \
	template<> inline void fhash_dtor(NST& obj) { obj.~T(); memset(&obj, 0, sizeof(obj)); }

FHASH_SETUP_POD(int8)
FHASH_SETUP_POD(int16)
FHASH_SETUP_POD(int32)
FHASH_SETUP_POD(int64)
FHASH_SETUP_POD(uint8)
FHASH_SETUP_POD(uint16)
FHASH_SETUP_POD(uint32)
FHASH_SETUP_POD(uint64)

// Setup for a class that has an "int GetHashCode() const" function,
// but doesn't need to be constructed or destructed. The omission of
// the constructor and destructor calls is a performance optimization.
#define FHASH_SETUP_POD_GETHASHCODE(T) \
	FHASH_SETUP_TOR_NOP(T) \
	template<> inline uint32 fhash_gethash(const T& obj) { return obj.GetHashCode(); }

// Setup for a class that has a regular constructor and destructor
// This is usable then as fhashmap<TheClass,Value>
// See also FHASH_SETUP_POINTER_GETHASHCODE, if you want to use a pointer to the class as the key
#define FHASH_SETUP_CLASS_GETHASHCODE(NST,T) \
	FHASH_SETUP_CLASS_CTOR_DTOR(NST,T) \
	template<> inline uint32 fhash_gethash(const NST& obj) { return obj.GetHashCode(); }

// Pointer to object that has an "int GetHashCode() const" function
// This is usable then as fhashmap<TheClass*,Value>
#define FHASH_SETUP_POINTER_GETHASHCODE(T) \
	template<> inline void fhash_ctor<T*>(T*& obj)		{} \
	template<> inline void fhash_dtor<T*>(T*& obj)		{} \
	template<> inline uint32 fhash_gethash<T*>(T* const &obj)										{ return obj->GetHashCode();} \
	template<> inline void fhash_tor_types<T*>( fhash_tor_type& ctor, fhash_tor_type& dtor )		{ ctor = fhash_TOR_NOP; dtor = fhash_TOR_NOP; } \
	template<> inline int FHASH_CALL fhash_keycmp<T*>(const void* a, const void* b)					{ return **((T**) a) == **((T**) b) ? 0 : 1; }

// Pointer = object identity (and therefore hash value)
#define FHASH_SETUP_POINTER_ADDRESS(T) \
	template<> inline void fhash_ctor<T*>(T*& obj)		{} \
	template<> inline void fhash_dtor<T*>(T*& obj)		{} \
	template<> inline uint32 fhash_gethash<T*>(T* const &obj)										{ return fhash_gethash_ptr(obj);} \
	template<> inline void fhash_tor_types<T*>( fhash_tor_type& ctor, fhash_tor_type& dtor )		{ ctor = fhash_TOR_NOP; dtor = fhash_TOR_NOP; }

template< typename K > void FHASH_CALL fhash_type_ctor(void* obj) { fhash_ctor(*((K*) obj)); }
template< typename K > void FHASH_CALL fhash_type_dtor(void* obj) { fhash_dtor(*((K*) obj)); }

template< typename K, typename V >
void FHASH_CALL fhash_type_ctor_pair(void* obj)
{
	K* kobj = (K*) obj;
	V* vobj = (V*)(kobj + 1);
	fhash_ctor(*kobj);
	fhash_ctor(*vobj);
}

template< typename K, typename V >
void FHASH_CALL fhash_type_dtor_pair(void* obj)
{
	K* kobj = (K*) obj;
	V* vobj = (V*)(kobj + 1);
	fhash_dtor(*kobj);
	fhash_dtor(*vobj);
}

// Convenience method to delete all objects from a map<obj1*, obj2*>
template< typename TMap >
void fhash_delete_all_keys_and_values(TMap& m)
{
	for (typename TMap::iterator it = m.begin(); it != m.end(); it++)
	{
		delete it.key();
		delete it.val();
	}
	m.clear();
}

// Convenience method to delete all keys from a map<key*, ANYTHING>
template< typename TMap >
void fhash_delete_all_keys(TMap& m)
{
	for (typename TMap::iterator it = m.begin(); it != m.end(); it++)
		delete it.key();
	m.clear();
}

// Convenience method to delete all values from a map<ANYTHING, value*>
template< typename TMap >
void fhash_delete_all_values(TMap& m)
{
	for (typename TMap::iterator it = m.begin(); it != m.end(); it++)
		delete it.val();
	m.clear();
}

struct fhash_iface
{
	size_t				Stride;
	fhash_tor_type		NCTor;
	fhash_tor_type		NDTor;
	fhash_func_xfer		Move;
	fhash_func_xfer		Copy;
	fhash_func_ctor		Create;
	fhash_func_dtor		Delete;
	fhash_func_keycmp	KeyCmp;
	fhash_func_gethash	GetHash;
	bool BothNOP() const { return NCTor == fhash_TOR_NOP && NDTor == fhash_TOR_NOP; }
};

class PAPI fhashtable_base
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
			pos = fhash_npos;
			end = false;
			parent = 0;
		}
		iterator(const fhashtable_base *p, size_t itpos)
		{
			_index = -1;
			pos = itpos;
			end = pos == fhash_npos;
			parent = p;
		}
		iterator(const iterator &copy)
		{
			_index = copy._index;
			pos = copy.pos;
			end = copy.end;
			parent = copy.parent;
		}
		iterator(const fhashtable_base *p)
		{
			pos = 0;
			_index = 0;
			end = false;
			parent = const_cast<fhashtable_base*>(p);
			if (parent->mCount == 0 || parent->mSize == 0)
			{
				end = true;
			}
			else
			{
				// make iterator point to first object if it isn't already so
				fhash_key_states state = fhash_get_state(parent->mState, pos);
				if (state != fhash_Full)
					(*this)++;
			}
			_index = 0;
		}

		bool operator==(const iterator& b)
		{
			if (end && b.end) return true;
			if (end != b.end) return false;
			return pos == b.pos;
		}

		bool operator!=(const iterator& b)
		{
			return !(*this == b);
		}

		void* operator->() const
		{
			return parent->dpos(pos);
		}
		void* operator*() const
		{
			return parent->dpos(pos);
		}

		// (int) --> postfix
		/// Increment
		iterator& operator++(int)
		{
			if (pos >= parent->mSize) return *this;
			pos++;
			while (pos < parent->mSize && fhash_get_state(parent->mState, pos) != fhash_Full)
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
			if (pos == -1) return *this;
			pos--;
			while (pos != -1 && fhash_get_state(parent->mState, pos) != fhash_Full)
			{
				pos--;
			}
			_index--;
			if (pos == -1) end = true;
			return *this;
		}

		/** Signals that iteration has ended.
		This will be flagged on 3 occasions:
		- The set is empty.
		- Forward iteration with operator++ has caused us to step onto the last entity.
		- Backward iteration with operator-- has caused us to step onto the first entity.

		Note that in neither of two latter cases will the iterator point to an object that is
		not inside the set / table.
		**/
		bool end;

		/** Returns the index of the current object.
		The index is zero for the element referred to after begin(), then incremented
		for every operator++, and decremented for every operator--.

		It does not have widespread use, but can be handy in some cases where the set must be
		referred to as a vector.
		**/
		size_t index() const
		{
			return _index;
		}

	protected:
		const fhashtable_base *parent;
		size_t pos;
		size_t _index; // valid range is [0, mCount]
	};

	fhashtable_base()
	{
		base_init();
	}
	fhashtable_base(const fhashtable_base& copy)
	{
		base_init();
		mConfig = copy.mConfig;
		*this = copy;
	}
	fhashtable_base& operator=(const fhashtable_base& copy)
	{
		if (this == &copy) return *this;

		free_arrays();

		mMask = copy.mMask;
		mProbeOffset = copy.mProbeOffset;
		mAutoShrink = copy.mAutoShrink;
		mAge = copy.mAge;
		mCount = copy.mCount;
		mSize = copy.mSize;
		mMaxCount = copy.mMaxCount;

		if (mCount == 0)
		{
			mAge = 0;
			mSize = 0;
		}
		else
		{
			size_t statesize = sizeof(fhashstate_t) * fhash_state_array_size(mSize);
			mState = (fhashstate_t*) malloc(statesize);
			if (mState == NULL)
				fhash_die();
			memcpy(mState, copy.mState, statesize);
			mData = (byte*) malloc(mConfig.Stride * mSize);
			if (mData == NULL)
				fhash_die();

			for (size_t i = 0; i < mSize; i++)
			{
				create_obj(dpos(i));
				if (fhash_get_state(mState, i) == fhash_Full)
					mConfig.Copy(dpos(i), copy.dpos(i), mConfig.Stride);
			}
		}

		return *this;
	}

	~fhashtable_base()
	{
		free_arrays();
	}

	void init(const fhash_iface& f)
	{
		mConfig = f;
	}

	/// Clears the set
	void clear()
	{
		free_arrays();
		mSize = 0;
		mMaxCount = 0;
		mMask = 0;
		mProbeOffset = 0;
		mCount = 0;
		mAge = 0;
	}

	/// Clears the set, but keeps our raw size the same
	void clear_noalloc()
	{
		if (mSize == 0) return;

		ASSERT(fhash_Null == 0);

		if (mConfig.BothNOP())
		{
			// do nothing (such as for PODs)
		}
		else
		{
			for (size_t i = 0; i < mSize; i++)
			{
				if (fhash_get_state(mState, i) == fhash_Full)
				{
					mConfig.Delete(dpos(i));
					mConfig.Create(dpos(i));
				}
			}
		}
		memset(mState, 0, sizeof(fhashstate_t) * fhash_state_array_size(mSize));

		mCount = 0;
		mAge = 0;
	}

	/// Number of elements in set
	size_t size() const
	{
		return mCount;
	}

	/// Raw size of table
	size_t raw_size() const
	{
		return mSize;
	}

	/** Resize the hashtable.
	In general this is used to prepare the hashtable for a large number of insertions.
	The hashtable is automatically resized during normal use.
	There are some conditions that apply:
	- If newsize == 0, the table is cleared.
	- If 0 < newsize < 2 then newsize = 2.
	- If newsize < mCount * mFillRatio then we debug assert, increase newsize, and proceed.
	- If none of the above conditions apply, then newsize = NextPrime( newsize )
	**/
	void resize(size_t newsize)
	{
		if (newsize < mCount * 2)
			newsize = mCount * 2;

		if (newsize == 0) { clear(); return; }

		if (newsize < 2) newsize = 2;
		else newsize = fhash_next_power_of_2(newsize);

		// save our current sate
		byte			*odata = mData;
		fhashstate_t	*ostate = mState;
		size_t			 osize = mSize;

		// allocate the new keys
		mData = (byte*) malloc(newsize * mConfig.Stride);
		mState = (fhashstate_t*) malloc(fhash_state_array_size(newsize));
		if (mData == NULL || mState == NULL)
			fhash_die();

		// Make all the states null
		memset(mState, 0, fhash_state_array_size(newsize));

		if (mConfig.NCTor == fhash_TOR_NOP)		{}
		else if (mConfig.NCTor == fhash_TOR_ZERO)	memset(mData, 0, newsize * mConfig.Stride);
		else
		{
			// Delay construction until after the move of the existing values. In that case, only slots that are not occupied need to be constructed.
			// The delayed-construction thing could apply to the fhash_TOR_ZERO case too, but my guess is that it's probably better to let an efficient memset()
			// do it's job - esp regarding write combining, etc.
			//for ( size_t i = 0; i < newsize; i++ )
			//	mConfig.Create( dpos(i) );
		}

		mCount = 0;
		mSize = newsize;
		mMaxCount = mSize >> 1;
		mMask = mSize - 1;
		mProbeOffset = mSize >> 1;
		mAge = 0;

		// Copy values
		for (size_t i = 0; i < osize; i++)
			if (fhash_get_state(ostate, i) == fhash_Full)
				insert_no_check(false, odata + i * mConfig.Stride);

		// Run constructors for new objects that were not copied
		if (mConfig.NCTor == fhash_TOR_FUNC)
		{
			for (size_t i = 0; i < newsize; i++)
				if (fhash_get_state(mState, i) != fhash_Full)
					mConfig.Create(dpos(i));
		}

		// Run destructors for old objects that were not copied
		if (mConfig.NDTor == fhash_TOR_FUNC)
		{
			for (size_t i = 0; i < osize; i++)
				if (fhash_get_state(ostate, i) == fhash_Null)
					mConfig.Delete(odata + i * mConfig.Stride);
		}

		if (odata)	free(odata);
		if (ostate) free(ostate);
	}

	void resize_for(size_t count)
	{
		return resize(count * 2);
	}

	bool contains(const void* obj) const
	{
		return _find(obj) != fhash_npos;
	}

	iterator find(const void* obj)
	{
		return iterator(this, _find(obj));
	}

	/// \internal Searches linearly (for debugging this class)
	bool linearfind(const void* obj) const
	{
		for (size_t i = 0; i < mSize; i++)
		{
			if (mConfig.KeyCmp(dpos(i), obj) == 0)
				return true;
		}
		return false;
	}

	/// Returns the size of the data array + the size of the state array
	size_t mem_usage() const
	{
		return mConfig.Stride * mSize + fhash_state_array_size(mSize);
	}

	/// Merge
	fhashtable_base& operator+=(const fhashtable_base& b)
	{
		for (iterator it = b.begin(); it != b.end(); it++)
			insert_check_exist(*it);
		return *this;
	}

	/// Subtract
	fhashtable_base& operator-=(const fhashtable_base& b)
	{
		for (iterator it = b.begin(); it != b.end(); it++)
			_erase(*it);
		return *this;
	}

	// Internal access
	void* dpos(size_t i) const { return mData + mConfig.Stride * i; }

	/** Serializes the table to a file. Since this is a memory dump, it should only be used on tables whos elements contain no pointers.
	After serialization the hash table is useless, and must not be touched again until after calling deserialize_pod.
	@param mem The buffer in which to serialize.
	@param bytes The size of the buffer. If the size is too small, then nothing will be written, and bytes will contain the necessary size.
	**/
	//void serialize_pod( void* mem, size_t& bytes )
	//{
	//	size_t s1 = mSize * sizeof(TData);
	//	size_t s2 = stateArraySize( mSize ) * sizeof(fhashstate_t);
	//	size_t required = s1 + s2;
	//	if ( bytes < required )
	//	{
	//		bytes = required;
	//		return;
	//	}
	//	if ( s1 + s2 == 0 ) return;
	//	if ( mem == NULL ) { ASSERT(false); return; }
	//	BYTE* bmem = (BYTE*) mem;
	//	memcpy( bmem, mData, s1 );
	//	memcpy( bmem + s1, mState, s2 );
	//	free_arrays();
	//}

	/** Deserializes the table from a file.
	No checks are made to ensure that you haven't touched the vector since it was serialized.
	**/
	//void deserialize_pod( const void* mem )
	//{
	//	ASSERT( mSize > 0 );
	//	ASSERT( mData = NULL );

	//	mState = new fhashstate_t[ stateArraySize(mSize) ];
	//	mData = new TData[ mSize ];

	//	size_t s1 = mSize * sizeof(TData);
	//	size_t s2 = stateArraySize( mSize ) * sizeof(fhashstate_t);
	//	if ( s1 + s2 == 0 ) return;
	//	if ( mem == NULL ) { ASSERT(false); return; }
	//	const BYTE* bmem = (const BYTE*) mem;
	//	memcpy( mData, bmem, s1 );
	//	memcpy( mState, bmem + s1, s2 );
	//}

	//size_t debug_table_pos( const TKey& obj, int i ) const
	//{
	//	fhashkey_t hkey = THashFunc::gethashcode( obj );
	//	size_t pos = table_pos( hkey, i );
	//	return pos;
	//}

protected:

	void base_init()
	{
		memset(&mConfig, 0, sizeof(mConfig));
		mData = NULL;
		mState = NULL;
		mMaxCount = 0;
		mSize = 0;
		mMask = 0;
		mProbeOffset = 0;
		mCount = 0;
		mAge = 0;
		mAutoShrink = true;
	}

	fhashkey_t get_hash_code(const void* obj) const
	{
		return mConfig.GetHash(obj, mConfig.Stride);
	}

	/** Erases an item.
	\return True if the item was found. False if the item was not found.
	**/
	bool _erase(const void* obj)
	{
		size_t pos = _find(obj);
		if (pos != fhash_npos)
		{
			mAge++;
			mCount--;
			fhash_set_state(mState, pos, fhash_Deleted);
			delete_obj(dpos(pos));
			autoshrink();
			return true;
		}
		else
			return false;
	}

	void insert_check_resize()
	{
		if (mCount >= mMaxCount)
			resize((mCount + 1) << 1);
	}

	void copy_or_move(bool copy, void* dst, const void* obj)
	{
		if (copy) mConfig.Copy(dst, obj, mConfig.Stride);
		else
		{
			// The ONLY path that hits this is when resizing the array, and copying over existing values. In that case, we have a target array of freshly constructed objects.
			// We cannot simply memcpy over those fresh objects. We need to call their destructors first.
			//if ( mConfig.NDTor == fhash_TOR_FUNC ) mConfig.Delete( dst );
			// ALTERATION: We can simply memcpy over. The reason is because I've changed the initialization to delay running the constructors until after we've copied
			// the existing values in. This is better because we avoid an unnecessary construction/destruction cycle.
			mConfig.Move(dst, obj, mConfig.Stride);
		}
	}

	/// Insert an item into the set without checking if it exists. Returns position of insertion.
	size_t insert_no_check(bool copy, const void* obj)
	{
		insert_check_resize();
		fhashkey_t hkey = get_hash_code(obj);
		size_t pos = table_pos(hkey);
		fhash_key_states state = fhash_get_state(mState, pos);
		if (state == fhash_Full)
		{
			// Search for an empty slot
			uint i = 0;
			while (state == fhash_Full)
			{
				pos = table_pos(hkey, ++i);
				state = fhash_get_state(mState, pos);
				if (i >= mSize) ASSERT(false);
			}
		}
		fhash_set_state(mState, pos, fhash_Full);
		copy_or_move(copy, dpos(pos), obj);
		mCount++;
		return pos;
	}

	/** Insert an item into the set.

	@param overwrite If true, then we overwrite any existing value for the specified key. This is a specialization
		that is only applicable to hash maps (not hash sets).

	@return fhash_npos if item already in table (only possible if overwrite is false).

	**/
	size_t insert_check_exist(const void* obj, bool overwrite = false)
	{
		insert_check_resize();

		fhashkey_t hkey = get_hash_code(obj);

		// We insert at the first deleted slot, or the first null slot, whichever comes first
		// However, we must scan until (1. Find existing) or (2. Scanned entire table)
		size_t pos = fhash_npos;
		size_t pos_ins = fhash_npos; // remember the first fhash_Deleted position, because that is where we will insert, if we're not already existent
		for (uint i = 0; i != mSize; i++)
		{
			pos = table_pos(hkey, i);
			fhash_key_states ks = fhash_get_state(mState, pos);
			if (ks == fhash_Full)
			{
				if (mConfig.KeyCmp(dpos(pos), obj) == 0)
				{
					// key already present
					if (overwrite)
					{
						copy_or_move(true, dpos(pos), obj);
						return pos;
					}
					else
						return fhash_npos;
				}
			}
			else if (ks == fhash_Null)
			{
				if (pos_ins == fhash_npos) pos_ins = pos;
				break;
			}
			else /* if ( state == fhash_Deleted ) */
			{
				if (pos_ins == fhash_npos) pos_ins = pos;
			}
		}

		// insert here
		fhash_set_state(mState, pos_ins, fhash_Full);
		copy_or_move(true, dpos(pos_ins), obj);
		mCount++;
		return pos_ins;
	}

	void free_arrays()
	{
		if (mConfig.NDTor == fhash_TOR_NOP) {}
		else
		{
			for (size_t i = 0; i < mSize; i++)
				mConfig.Delete(dpos(i));
		}

		free(mData); mData = NULL;
		free(mState); mState = NULL;
	}

	void delete_obj(void* obj)
	{
		if (mConfig.NDTor == fhash_TOR_NOP) return;
		mConfig.Delete(obj);
	}

	void create_obj(void* obj)
	{
		if (mConfig.NCTor == fhash_TOR_NOP) return;
		mConfig.Create(obj);
	}

	/// Returns the position of an item if existent
	size_t _find(const void* obj) const
	{
		if (mSize == 0) return fhash_npos;
		fhashkey_t hkey = get_hash_code(obj);
		size_t pos = table_pos(hkey);
		size_t first = pos;
		// quick positive/empty check
		fhash_key_states ks = fhash_get_state(mState, pos);
		if (ks == fhash_Full && mConfig.KeyCmp(dpos(pos), obj) == 0) return pos;
		else if (ks == fhash_Null) return fhash_npos;
		else
		{
			// exhaustive
			uint i = 0;
			pos = table_pos(hkey, ++i);
			while (fhash_get_state(mState, pos) != fhash_Null)
			{
				if (fhash_get_state(mState, pos) == fhash_Full && mConfig.KeyCmp(dpos(pos), obj) == 0) return pos;
				pos = table_pos(hkey, ++i);
				if (pos == first) break;
			}
			return fhash_npos;
		}
	}

	/// Erases all instances of key. Returns number of items erased.
	size_t _erase_all(const void* obj)
	{
		if (mSize == 0) return 0;
		fhashkey_t hkey = get_hash_code(obj);
		size_t pos = table_pos(hkey);
		size_t first = pos;
		size_t del = 0;
		uint i = 0;
		while (true)
		{
			fhash_key_states state = fhash_get_state(mState, pos);
			if (state == fhash_Full && mConfig.KeyCmp(dpos(pos), obj) == 0)
			{
				del++;
				delete_obj(dpos(pos));
				fhash_set_state(mState, pos, fhash_Deleted);
			}
			else if (state == fhash_Null) break;
			pos = table_pos(hkey, ++i);
			if (pos == first) break;
		}
		return del;
	}

	/// The hash function (optimization of generic table_pos with i = 0)
	size_t table_pos(fhashkey_t key) const
	{
		return (size_t)(fold(key) & mMask);
	}

	/// probe (when i = 0, this function must be identical to table_pos(key))
	size_t table_pos(fhashkey_t key, uint i) const
	{
		key = fold(key);
		uint mul = key >> 1;
		mul |= 1; // ensure multiplier is odd
		return (size_t)((key + i * mul) & mMask);
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// NOTE: This code used to be inside table_pos, but it is invalid. It violates our "visit every slot exactly once" rule.
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// This offset particularly helps speed up failed lookups when our table is a dense array of integers (consecutive hashes).
	//// The layout in that case is that half of the table is completely occupied, and the other half (remember our fill factor -- there is always an 'other half')
	//// is completely empty. This probe offset immediately sends us into the territory of the 'other half', thereby reducing the time that we spend walking through
	//// the populated half.

	//// The difference here seems to be negligible, but the simpler version should be better for future compilers.
	//uint offset = i == 0 ? 0 : (uint) mProbeOffset;
	////uint offset = ~(int(i - 1) >> 31) & mProbeOffset; // branch-less version
	//return (size_t) ((key + offset + i * mul) & mMask);
	///////////////////////////////////////////////////////////////////////////////


	// Very simple mix function that at least gives us better behaviour when the only entropy is higher than our mask.
	// This simplistic function probably causes evil behaviour in certain pathological cases, but it's better than not having it at all.
	// This mixing solves cases such as values of the form 0x03000000, 0x04000000, 0x05000000. Without this folding
	// function, those keys would all end up with the same table position, unless the table was larger than 0x0fffffff.
	static fhashkey_t fold(fhashkey_t k)
	{
		uint32 u = k;
		u = u ^ (u >> 16);
		u = u ^ (u >> 8);
		return u;
	}

	void autoshrink()
	{
		if ((mAge & (32-1)) != 0 || !mAutoShrink) return;

		// Only shrink when we're more than 2x the size we need to be. The reason this is not
		// simply 1x is to avoid possible ping-ponging during a repeated insert/erase pattern.
		size_t necessary = fhash_next_power_of_2(mCount * 2);

		// I don't like having this heuristic here of fhash_min_autoshrink_count, but in all practicality, I have never made a hash table with such a huge key size
		// that it would matter that you're storing 64 and not 32 or 16.
		if (mSize > necessary * 2 && mSize > fhash_min_autoshrink_count)
			resize(0);
	}

	void move(void* dst, const void* src)
	{
		mConfig.Move(dst, src, mConfig.Stride);
	}
	void copy(void* dst, const void* src)
	{
		mConfig.Copy(dst, src, mConfig.Stride);
	}

	byte			*mData;			///< Key Array
	fhashstate_t	*mState;		///< State Array (null, occupied, deleted)
	unsigned int	mAge;			///< Incremented when item is erased
	size_t			mMaxCount;		///< Maximum count before we must increase our raw size
	size_t			mSize;			///< Size of table
	size_t			mProbeOffset;	///< Initial offset of probe	(only used when not OHASH_PRIME_SIZE)
	size_t			mMask;			///< Size of table - 1			(only used when not OHASH_PRIME_SIZE)
	size_t			mCount;			///< Number of items in set
	bool			mAutoShrink;	///< If true, then the table automatically shrinks itself when it's mAge reaches a certain limit

	fhash_iface		mConfig;

public:

	/** Enables or disables auto-shrinking.
	Auto-shrinking needs to be disabled if you wish to iterate through the set and erase items as you are going.
	**/
	void auto_shrink(bool on)
	{
		if (on == mAutoShrink) return;
		mAutoShrink = on;
		if (on) autoshrink();
	}

	iterator begin() const	{ return iterator(this); }
	iterator end() const	{ return iterator(this, fhash_npos); }

	friend class fhashtable_base::iterator;
};

// This is basically just here for packing two things together in memory before calling into the base hash table
template< typename TKey, typename TVal >
struct fhash_pair
{
	fhash_pair(const TKey& k, const TVal& v)
	{
		Key = k;
		Val = v;
	}
	TKey Key;
	TVal Val;
};

template< typename TKey >
void fhash_setup_set(fhash_iface& f)
{
	f.Stride = sizeof(TKey);
	fhash_tor_types<TKey>(f.NCTor, f.NDTor);
	ASSERT(f.NDTor != fhash_TOR_ZERO);   // not allowed. use either FUNC or NOP for dtor
	f.Create = &fhash_type_ctor<TKey>;
	f.Delete = &fhash_type_dtor<TKey>;
	f.Copy = &fhash_type_copy<TKey>;
	f.Move = &fhash_pod_move;
	f.KeyCmp = &fhash_keycmp<TKey>;
	f.GetHash = &fhash_gethash_gen_wrap<TKey>;
}

inline fhash_tor_type fhash_reduce(fhash_tor_type a, fhash_tor_type b)
{
	if (a == fhash_TOR_NOP && b == fhash_TOR_NOP) return fhash_TOR_NOP;
	if (a == fhash_TOR_FUNC || b == fhash_TOR_FUNC) return fhash_TOR_FUNC;
	return fhash_TOR_ZERO;
}

template< typename TKey, typename TVal >
void fhash_setup_map(fhash_iface& f, size_t msize)
{
	f.Stride = msize;
	fhash_tor_type ckey, dkey;
	fhash_tor_type cval, dval;
	fhash_tor_types<TKey>(ckey, dkey);
	fhash_tor_types<TVal>(cval, dval);
	ASSERT(dkey != fhash_TOR_ZERO && dval != fhash_TOR_ZERO);   // not allowed. use either FUNC or NOP for dtor
	f.NCTor = fhash_reduce(ckey, cval);
	f.NDTor = fhash_reduce(dkey, dval);
	f.Create = &fhash_type_ctor_pair<TKey, TVal>;
	f.Delete = &fhash_type_dtor_pair<TKey, TVal>;
	f.Copy = &fhash_type_copy_pair<TKey, TVal>;
	f.Move = &fhash_pod_move;
	f.KeyCmp = &fhash_keycmp<TKey>;
	f.GetHash = &fhash_gethash_gen_wrap<TKey>;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hash Map
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename TKey, typename TVal >
class fhashmap : public fhashtable_base
{
public:
	typedef fhash_pair<TKey, TVal> TPair;
	static const size_t TSize = sizeof(TKey) + sizeof(TVal);

	fhashmap()
	{
		fhash_setup_map<TKey, TVal>(mConfig, sizeof(TKey) + sizeof(TVal));
	}

	void insert(const TKey& key, const TVal& val, bool overwrite = true)
	{
		char t[TSize];
		memcpy(t, &key, sizeof(TKey));
		memcpy(t + sizeof(TKey), &val, sizeof(TVal));
		insert_check_exist(t, overwrite);
	}

	bool erase(const TKey& key)
	{
		return _erase(&key);
	}

	bool contains(const TKey& key) const { return _find(&key) != fhash_npos; }

	TVal* getp(const TKey& key) const
	{
		size_t pos = _find(&key);
		if (pos == fhash_npos) return NULL;
		return offset_val(dpos(pos));
	}

	TVal get(const TKey& key) const
	{
		TVal* p = getp(key);
		return p ? *p : TVal();
	}

	bool get(const TKey& key, TVal& val) const
	{
		TVal* p = getp(key);
		if (p) val = *p;
		return p != NULL;
	}

	template<typename TContainer>
	void keys(TContainer& keys) const
	{
		for (auto it = begin(); it != end(); it++)
			keys += it.key();
	}

	template<typename TContainer>
	void values(TContainer& vals) const
	{
		for (auto it = begin(); it != end(); it++)
			vals += it.val();
	}

	// We do not allow assignment via operator[], because it is ambiguous. You don't know whether you're assigning
	// an empty string or fetching a value without knowing whether your instance is const or now, and I've burned myself like that.
	TVal operator[](const TKey& key) const
	{
		return get(key);
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Iterator
	/////////////////////////////////////////////////////////////////////////////////////

	class iterator : public fhashtable_base::iterator
	{
	public:
		typedef fhashtable_base::iterator base;
		iterator() : base() {}
		iterator(const fhashtable_base* p, size_t itpos) : base(p,itpos) {}
		iterator(const fhashtable_base* p) : base(p) {}

		const TKey& key() const	{ return *((const TKey*) parent->dpos(pos)); }
		const TVal& val() const { return *((const TVal*) offset_val(parent->dpos(pos))); }
	};

	friend class iterator;

	iterator begin() const	{ return iterator(this); }
	iterator end() const	{ return iterator(this, fhash_npos); }

protected:
	static TVal* offset_val(void* p) { return (TVal*)((char*) p + sizeof(TKey)); }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hash Set
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename TKey >
class fhashset : public fhashtable_base
{
public:
	static const size_t TSize = sizeof(TKey);

	fhashset()
	{
		fhash_setup_set<TKey>(mConfig);
	}

	void insert(const TKey& key)				{ insert_check_exist(&key, false); }
	bool erase(const TKey& key)				{ return _erase(&key); }
	bool contains(const TKey& key) const		{ return _find(&key) != fhash_npos; }

	fhashset& operator+=(const TKey& key)		{ insert(key); return *this; }
	fhashset& operator-=(const TKey& key)		{ erase(key); return *this; }

	/////////////////////////////////////////////////////////////////////////////////////
	// Iterator
	/////////////////////////////////////////////////////////////////////////////////////

	class iterator : public fhashtable_base::iterator
	{
	public:
		typedef fhashtable_base::iterator base;
		iterator() : base() {}
		iterator(const fhashtable_base* p, size_t itpos) : base(p,itpos) {}
		iterator(const fhashtable_base* p) : base(p) {}

		const TKey& operator*() const	{ return *((const TKey*) parent->dpos(pos)); }
	};

	friend class iterator;

	iterator begin() const	{ return iterator(this); }
	iterator end() const	{ return iterator(this, fhash_npos); }
};


#endif

