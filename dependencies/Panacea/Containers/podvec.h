#pragma once

#include "../Platform/stdint.h"
#include "cont_utils.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4345)	// POD type is default-initialized with () syntax
#endif


/*

This is a cut-down vector<>, but built for types that adhere to a specific protocol.

Benefits of using podvec:
* Memory move semantics. If you are storing a vector of XString objects, and the vector needs to grow,
	then we don't need to realloc memory for the content of all those XString objects. Because we simply do a
	memcpy, we avoid that.
	This functionality is what you get from C++11 rvalue references, except we do it here without all
	that pain - and this was written before C++11 was born.

Requirements for types for which podvec_ispod() is false:
1. A zero fill must initialize the object.
2. Nobody (including the class itself) may store internal pointers to the object. We need to be able to move objects around.
3. The destructor must be a no-op when the object has been zero-initialized. Whenever podvec needs to destroy an object,
	it explicitly calls its destructor, and then does a memset(0) on the contents of the object's memory. This sequence
	of events must leave the object in its initialized state (which is the same as constraint #1).
4. No support for polymorphic classes. That is anything with a virtual function (including a virtual destructor).

Additional requirements for types for which podvec_ispod() is true:
5. No constructor necessary.
6. No destructor necessary.
7. The object can be copied with memcpy. ie. The object doesn't "own" any pointers. A string is an example of a type
	that violates this constraint, because if you memcpy'd a string into the vector, then you would end up with two
	strings pointing to the same block of memory, and that memory would eventually be freed twice.

The lack of protected data members (count,capacity,data) makes it a little easier to deal with
when all you want is a managed array.

podvec_ispod() is opt-in, because the requirements are stricter.

Accessors and size() are signed pointer-sized, because historically I use "for (int i = 0; i < v.size(); i++)", and I don't want to break all of that
by littering signed/unsigned comparison warnings -- which is a warning always worth heeding.

In addition, signed comparisons are generally more robust, because you don't wrap around under zero.

Internally we store unsigned pointer-sized integers.
*/

template<typename T> bool podvec_ispod() { return false; }

template<typename T>
struct podvec
{
	uintp	count;
	uintp	capacity;
	T*		data;

	static bool ispod()				{ return podvec_ispod<T>(); }
	static void panic()				{ AbcPanicSilent(); }

	podvec()
	{
		construct();
	}
	podvec( const podvec& b )
	{
		construct();
		*this = b;
	}

	~podvec()
	{
		clear();
	}

	void hack( intp _count, intp _capacity, T* _data )
	{
		count = _count;
		capacity = _capacity;
		data = _data;
	}

	intp size() const { return count; }

	// We are consistent here with the stl's erase, which takes [start,last), where last is one beyond the last element to erase.
	void erase( intp _start, intp _end = -1 )
	{
		uintp start = _start;
		uintp end = _end;
		if ( end == -1 ) end = start + 1;
		if ( start == end ) return;			// on an empty set, you must be able to do erase(0,0), and have it be a nop
		if ( start >= count )	panic();	// erase is rare enough that I feel it's worth it to pay this branch penalty for an early crash.
		if ( end > count )		panic();
		uintp remain = count - end;
		for ( uintp i = start; i < end; i++ ) dtor( data[i] );
		memmove( data + start, data + end, remain * sizeof(T) );
		memset( data + start + remain, 0, (end - start) * sizeof(T) );
		count = start + remain;
	}

	// written originally for adding bytes to a podvec<u8>. Will grow the buffer in powers of 2.
	void addn( const T* arr, intp _n )
	{
		if ( isinternal(arr) ) panic();
		uintp n = _n;
		if ( count + n > capacity ) growfor( count + n );
		if ( ispod() )	{ memcpy( data + count, arr, n * sizeof(T) ); count += n; }
		else			{ for ( uintp i = 0; i < n; i++ ) data[count++] = arr[i]; }
	}

	T& add()
	{
		push( T() );
		return back();
	}

	void resize( intp _inewsize )
	{
		resize_internal( _inewsize, true );
	}

	// This is intended to be the equivalent of a raw malloc(), for the cases where you don't
	// want to pay the cost of zero-initializing the memory that you're about to fill up anyway.
	void resize_uninitialized( intp _inewsize )
	{
		resize_internal( _inewsize, false );
	}

	void reserve( intp _newcap )
	{
		uintp newcap = _newcap;
		if ( newcap <= count ) return;		// resize may not alter count, therefore we do nothing in this case
		resizeto( newcap, true );
	}

	// This is intended to be the equivalent of a raw malloc(), for the cases where you don't
	// want to pay the cost of zero-initializing the memory that you're about to fill up anyway.
	void reserve_uninitialized( intp _newcap )
	{
		uintp newcap = _newcap;
		if ( newcap <= count ) return;		// resize may not alter count, therefore we do nothing in this case
		resizeto( newcap, false );
	}

	void fill( const T& v )
	{
		for ( uintp i = 0; i < count; i++ )
			data[i] = v;
	}

	void insert( intp _pos, const T& v )
	{
		uintp pos = _pos;
		if ( pos > count ) panic();
		if ( isinternal(&v) )
		{
			T copy = v;
			grow();
			if ( count != capacity ) dtorz_block( 1, data + count );
			memmove( data + pos + 1, data + pos, (count - pos) * sizeof(T) );
			//memset( data + pos, 0, sizeof(T) );
			ctor( data[pos] );
			data[pos] = copy;
		}
		else
		{
			grow();
			if ( count != capacity ) dtorz_block( 1, data + count );
			memmove( data + pos + 1, data + pos, (count - pos) * sizeof(T) );
			//memset( data + pos, 0, sizeof(T) );
			ctor( data[pos] );
			data[pos] = v;
		}
		count++;
	}

	void	push_back( const T& v )	{ push(v); }
	void	pop_back()				{ count--; }
	void	pop()					{ pop_back(); }
	T		rpop()					{ T v = back(); pop(); return v; }

	void push( const T& v )
	{
		if ( count == capacity )
		{
			if ( isinternal(&v) )
			{
				// aliased
				T copy = v;
				grow();
				data[count++] = copy;
			}
			else
			{
				grow();
				data[count++] = v;
			}
		}
		else data[count++] = v;
	}

	void clear_noalloc() { count = 0; }
	void clear()
	{
		for ( uintp i = 0; i < capacity; i++ ) dtor(data[i]);
		free(data);
		data = NULL;
		count = capacity = 0;
	}

	intp		find( const T& v ) const		{ for ( uintp i = 0; i < count; i++ ) if (data[i] == v) return i; return -1; }
	bool		contains( const T& v ) const	{ return find(v) != -1; }

	T&			get( intp i )					{ return data[i]; }
	const T&	get( intp i ) const				{ return data[i]; }
	void		set( intp i, const T& v )		{ data[i] = v; }

	T&			operator[]( intp i )			{ return data[i]; }
	const T&	operator[]( intp i ) const		{ return data[i]; }

	T&			front()							{ return data[0]; }
	const T&	front() const					{ return data[0]; }
	T&			back()							{ return data[count-1]; }
	const T&	back() const					{ return data[count-1]; }

	void operator+=( const T& v )			{ push(v); }
	void operator+=( const podvec& b )		{ addn( b.data, b.count ); }

	void operator=( const podvec& b )
	{
		clear();
		resizeto( b.count, !ispod() );
		count = b.count;
		if ( ispod() )	{ memcpy( data, b.data, count * sizeof(T) ); }
		else			{ for ( uintp i = 0; i < b.count; i++ ) data[i] = b.data[i]; }
	}

	void grow() { growfor( count + 1 ); }

protected:
	void construct()
	{
		count = capacity = 0;
		data = NULL;
	}
	void dtor( T& v )
	{
		if ( !ispod() )
			v.T::~T();
	}
	void dtorz_block( uintp n, T* block )
	{
		if ( ispod() ) return;
		for ( uintp i = 0; i < n; i++ )
			block[i].T::~T();
		memset( block, 0, sizeof(T) * n );
	}
	void ctor( T& v )
	{
		//if ( ispod() )
		//	memset( &v, 0, sizeof(T) );
		//else
		//	new (&v) T();
		// On debug builds the memset is awful
		new (&v) T();
	}
	void ctor_block( uintp n, T* block )
	{
		if ( ispod() )
			memset( block, 0, sizeof(T) * n );
		else
		{
			for ( uintp i = 0; i < n; i++ )
				new (&block[i]) T();
		}
	}
	
	bool isinternal( const T* p ) const
	{
		return (uintp) (p - data) < capacity;
	}

	void growfor( uintp target )
	{
		// Regular growth rate is 2.0, which is what most containers (.NET, STL) use.
		// There is no theoretical optimal. It's simply a trade-off between memcpy time and wasted VM.
		uintp ncap = capacity ? capacity : 1;
		while ( ncap < target )
			ncap = ncap * 2;
		if ( !try_resizeto( ncap, true ) )
		{
			/* Special out of memory behaviour when growing the vector.
			When realloc fails, we try again to grow at a rate of 1.25 instead of our regular exponent of 2.
			The exponent 1.25 is a thumbsuck. There is no theoretically optimal behaviour here.
			This is only intended to save users when arrays are in the range of around 500MB or so, on a 32-bit machine.
			In this type of scenario, moving from 500 to 562 is better than 500 to 1000.
			Trying to recover from out of memory conditions is a dubious game. It's not worth it to try
			too hard, because the user is likely going to run out in the near future in a place where 
			a realloc fail is not recoverable.
			*/
			ncap = capacity;
			if ( ncap < 4 ) ncap = 4;
			while ( ncap < target )
				ncap = ncap + ncap / 4;
			if ( !try_resizeto( ncap, true ) )
				AbcMemoryExhausted();
		}
	}

	void resize_internal( uintp newsize, bool initmem )
	{
		if ( newsize == count ) return;
		if ( newsize == 0 )		{ clear(); return; }
		if ( newsize < capacity ) dtorz_block( capacity - (uintp) newsize, data + newsize );
		resizeto( newsize, initmem );
		count = newsize;
	}

	void resizeto( uintp newcap, bool initmem )
	{
		if ( !try_resizeto( newcap, initmem ) )
			AbcMemoryExhausted();
	}

	bool try_resizeto( uintp newcap, bool initmem )
	{
		if ( newcap == 0 )
			return true;
		T* newdata = (T*) realloc( data, newcap * sizeof(T) );
		if ( newdata == NULL )
			return false;
		data = newdata;
		if ( newcap > capacity && initmem )
		{
			//memset( data + capacity, 0, (newcap - capacity) * sizeof(T) );
			ctor_block( newcap - capacity, data + capacity );
		}
		capacity = newcap;
		return true;
	}

	const T* pbegin()	const { return data; }
	const T* pend()		const { return data + capacity; }
};

template < typename T >
void delete_all( podvec<T>& target )
{
	for ( intp i = 0; i < target.size(); i++ ) delete target[i];
	target.clear();
}

template<typename T>
bool operator==( const podvec<T>& a, const podvec<T>& b )
{
	uintp asize = a.size();
	if ( asize != b.size() ) return false;
	for ( uintp i = 0; i < asize; i++ )
	{
		if ( a.data[i] != b.data[i] ) return false;
	}
	return true;
}
template<typename T>
bool operator!=( const podvec<T>& a, const podvec<T>& b )
{
	return !(a == b);
}

template<> inline bool podvec_ispod<int8>()		{ return true; }
template<> inline bool podvec_ispod<int16>()	{ return true; }
template<> inline bool podvec_ispod<int32>()	{ return true; }
template<> inline bool podvec_ispod<int64>()	{ return true; }
template<> inline bool podvec_ispod<uint8>()	{ return true; }
template<> inline bool podvec_ispod<uint16>()	{ return true; }
template<> inline bool podvec_ispod<uint32>()	{ return true; }
template<> inline bool podvec_ispod<uint64>()	{ return true; }
template<> inline bool podvec_ispod<float>()	{ return true; }
template<> inline bool podvec_ispod<double>()	{ return true; }

template<> inline bool podvec_ispod<bool>()		{ return true; }
template<> inline bool podvec_ispod<void*>()	{ return true; }

namespace std
{
	template<typename T> inline void swap( podvec<T>& a, podvec<T>& b )
	{
		char tmp[sizeof(podvec<T>)];
		memcpy( tmp, &a, sizeof(a) );
		memcpy( &a, &b, sizeof(a) );
		memcpy( &b, tmp, sizeof(a) );
	}
}

INSTANTIATE_VECTOR_FUNCTIONS(podvec)

#ifdef _MSC_VER
#pragma warning(pop)
#endif

