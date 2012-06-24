#ifndef ABCORE_INCLUDED_PVECT_H
#define ABCORE_INCLUDED_PVECT_H

// Pointer to <> classes, without necessary compiler support for template specialization
// Note that all functions take "intp" values, but internally we store 32-bit integers.
// Should the day arrive when we need to store more than 4g items, then we'll change.

#include "cont_utils.h"

#ifndef PVECT_DECL_SPEC
#define PVECT_DECL_SPEC
#endif

//#define PVECT_RANGE_CHECK 1

#define PVINT int

class PVECT_DECL_SPEC pvoidvect
{
public:
	// These data members are made public here so that the vector can be used as a plain array.
	void**	data;
	PVINT	count;
	PVINT	capacity;

	pvoidvect()
	{
		count = 0;
		capacity = 0;
		data = NULL;
	}
	explicit pvoidvect(intp _reserve)
	{
		count = 0;
		capacity = 0;
		data = NULL;
		reserve(_reserve);
	}
	
	// copy constructor
	pvoidvect(const pvoidvect& b)
	{
		data = NULL;
		capacity = 0;
		count = 0;
		if (b.count > 0)
		{
			reserve(b.count);
			memcpy(data, b.data, sizeof(void*)*b.count);
		}
		count = b.count;
	}

	~pvoidvect()
	{
		if (data)
			delete []data;
	}

	void hack( intp _count, intp _capacity, void** _data )
	{
		count = PVINT(_count);
		capacity = PVINT(_capacity);
		data = _data;
	}

	pvoidvect& operator=(const pvoidvect &b) 
	{
		if ( this != &b )
		{
			if ( b.count != capacity ) clear();
			if ( b.count > 0 )
			{
				reserve(b.count);
				memcpy(data, b.data, sizeof(void*)*b.count);
			}
			count = b.count;
		}
		return *this;
	}

	bool operator==( const pvoidvect& b ) const
	{
		if ( count != b.count ) return false;
		for ( PVINT i = 0; i < count; i++ )
			if ( data[i] != b.data[i] ) return false;
		return true;
	}
	bool operator!=( const pvoidvect& b ) const { return !(*this == b); }

	// safe set. Will resize if necessary
	void set(intp _pos, const void* a) 
	{
		PVINT pos = PVINT(_pos);
		if (pos >= count) 
		{
			// Do we need to resize, or can we just up our count?
			if (pos >= capacity) 
				resize(pos+1);
			else
				count = pos+1;
		}
		data[pos] = const_cast<void*>(a);
	}

	void insert(intp _pos, const void* a) 
	{
		PVINT pos = PVINT(_pos);
		if (pos >= capacity || count+1 > capacity)
			reserve(_MAX_(capacity*2, pos+4));
		PVINT nmove = count - pos;
		if (nmove > 0)
			memmove(&(data[pos+1]), &(data[pos]), nmove*sizeof(void*));
		data[pos] = const_cast<void*>(a);
		count++;
	}

	void erase(intp _pos1, intp _pos2 = -1)
	{
		PVINT pos1 = PVINT(_pos1);
		PVINT pos2 = PVINT(_pos2);
		if ( pos2 == -1 )			pos2 = pos1;
		else if ( pos2 >= count )	pos2 = count -1;
		ASSERT( pos1 >= 0 && pos1 <= pos2 );

		PVINT nmove = (count - pos2) - 1;
		if (nmove > 0)
			memmove(&(data[pos1]), &(data[pos2+1]), nmove*sizeof(void*));
		count -= 1 + (pos2 - pos1);
	}

	// Grift.... this causes no problems because we know that a void* is only 4 bytes.
	void* pop_back() 
	{
		if (count > 0)
			count--;
		return data[count];
	}

	void push_back(const void* a) 
	{
		if (count >= capacity)
			reserve( _MAX_(capacity*2, 2) );
		data[count++] = const_cast<void*>(a);
	}

#ifdef PVECT_RANGE_CHECK
	const void* operator[](intp _i) const 
	{
		PVINT i = PVINT(_i);
		ASSERT( i >= 0 && i < count );
		return data[_i];
	}
#else
	const void* operator[](intp _i) const { return data[_i]; }
#endif

#ifdef PVECT_RANGE_CHECK
	void*& operator[](intp _i) 
	{
		PVINT i = PVINT(_i);
		ASSERT( i >= 0 && i < count );
		return data[_i];
	}
#else
	void*& operator[](intp _i) { return data[_i]; }
#endif

	void addn( intp n, void** p )
	{
		for ( intp i = 0; i < n; i++ ) push_back(p[i]);
	}

	void* front() const
	{
		return data[0];
	}
	void*& front() 
	{
		return data[0];
	}

	void* back() const 
	{
		return data[count-1];
	}
	void*& back() 
	{
		return data[count-1];
	}

	bool contains( const void* t ) const { return find(t) != -1; }

	// returns -1 on failure
	intp find( const void* t ) const
	{
		for ( PVINT i = 0; i < count; i++ )
			if ( data[i] == t ) return i;
		return -1;
	}

	void nullfill()
	{
		for ( PVINT i = 0; i < count; i++ )
			data[i] = NULL;
	}

	void clear() 
	{
		delete[] data;
		data = NULL;
		count = 0;
		capacity = 0;
	}

	// frees our heap storage and sets data to null (but does not touch capacity or count)
	void dealloc()
	{
		delete[] data;
		data = NULL;
	}

	// can be used to avoid reallocs
	void clear_noalloc()
	{
		count = 0;
	}

	intp size() const { return count; }

	// preserves existing entities but doesn't set count
	void reserve( intp _newsize )
	{
		PVINT newsize = PVINT(_newsize);
		if ( newsize <= capacity ) return;
		PVINT oc = count;
		resize(_MAX_(newsize, count));
		count = oc;
	}
	// preserves existing entities and sets count to newsize
	// does not realloc unless size is larger than existing capacity
	void resize( intp newsize ) 
	{
		if ( newsize > capacity )
		{
			void** nd = new void*[newsize];
			if (count > 0) 
				memcpy( nd, data, sizeof(void*) * count );
			delete []data;
			data = nd;
			capacity = (PVINT) newsize;
		}
		count = (PVINT) newsize;
	}

protected:
	void add( const pvoidvect& a )
	{
		for ( PVINT i = 0; i < a.count; i++ )
			push_back( a[i] );
	}

	template<typename TYPE> TYPE _MAX_( TYPE a, TYPE b ) { return a < b ? b : a; }
};


template < class T > class PVECT_DECL_SPEC pvect : public pvoidvect
{
public:
	typedef pvoidvect Base;

	pvect() : Base() {}

	explicit pvect(intp reserve) : Base(reserve) {}

	pvect(const pvect& b) : Base(b) {}

	pvect& operator=(const pvect &b) { return (pvect&) Base::operator=(b); }

	bool operator==( const pvoidvect& b ) const { return Base::operator==(b); }
	bool operator!=( const pvoidvect& b ) const { return Base::operator!=(b); }

	void set(intp pos, const T a) { Base::set(pos,a); }

	void insert(intp pos, const T a) { Base::insert(pos, a); }

	void erase(intp pos1, intp pos2 = -1) { Base::erase(pos1, pos2); }

	T pop_back()	{ return static_cast<T>(Base::pop_back()); }
	T pop()			{ return static_cast<T>(Base::pop_back()); }

	void push_back( const T a ) { Base::push_back(a); }

	void addn( intp n, const T* p ) { Base::addn(n, (void**) p); }

	pvect& operator+=( const T a ) { push_back( a ); return *this; }

	pvect& operator+=( const pvect& a ) { add( a ); return *this; }

	//const T operator[](intp i) const { return static_cast<const T>(Base::operator[](i)); }
	const T operator[](intp i) const { return (const T) Base::operator[](i); }

	T& operator[](intp i) { return (T&) Base::operator[](i); }

	// sometimes it's not pretty to force a dereference in order to use operator[]
	//const T get(intp i) const { return static_cast<const T>(Base::operator[](i)); }
	const T get(intp i) const { return (const T) Base::operator[](i); }

	T& get(intp i) { return (T&) Base::operator[](i); }

	const T back() const { return (const T) Base::back(); }
	T& back() { return (T&) Base::back(); }

	const T front() const { return (const T) Base::front(); }
	T& front() { return (T&) Base::front(); }

	intp find( const T a ) const { return Base::find(a); }

	void clear() { Base::clear(); }

	// deletes void* storage, but does not call destructor.
	void dealloc() { Base::dealloc(); }

	void nullfill() { Base::nullfill(); }

	intp size() const { return Base::size(); }

	void reserve(intp newsize) { Base::reserve(newsize); }

	void resize(intp newsize) { Base::resize(newsize); }
};

/** erase and delete for pvect
**/
template < typename TData >
void erase_delete( pvect<TData>& target, intp pos1, intp pos2 = -1 ) 
{ 
	if ( pos2 == -1 )						pos2 = pos1;
	else if ( pos2 >= target.count )		pos2 = target.count - 1;
	ASSERT( pos1 >= 0 && pos1 <= pos2 );
	for ( intp i = pos1; i <= pos2; i++ )
		delete target[i];
	target.erase( pos1, pos2 ); 
}

/** clear and delete all for pvect.
**/
template < typename TData >
void delete_all( pvect<TData>& target )
{
	for ( intp i = 0; i < target.size(); i++ )
		delete target[i];
	target.clear();
}

/** concatenation for pvect.
**/
template < typename TData >
pvect<TData> operator+( const pvect<TData>& a, const pvect<TData>& b )
{
	pvect<TData> c = a;
	c += b;
	return c;
}

/** Reverse for pvect.
**/
template < typename TData >
void reverse( pvect<TData>& target )
{
	intp lim = target.size() / 2;
	for ( intp i = 0; i < lim; i++ )
	{
		intp j = target.size() - i - 1;
		TData tt = target[i];
		target[i] = target[j];
		target[j] = tt;
	}
}

// Sort for pvect.
// These have to be custom written so that they can dereference the objects inside the pvect.

template <typename T> int pvect_compare_operator_lt( void* context, const T& a, const T& b )
{
	return *a < *b ? -1 : 0;
}

// See marshal_context_is_compare in cont_utils.h for an explanation.
// We do the extra work of dereferencing
template <typename T>
int pvect_marshal_context_is_compare( void* context, const T& a, const T& b )
{
	// I have to use this decltype() hack here because I can't figure out how to pass pvect_marshal_context_is_compare<T> instead of pvect_marshal_context_is_compare<T*> to vect_sort_cx.
	// GCC doesn't like this.
	typedef decltype(*a) TType;
	typedef int (*tcompare)(const TType& a, const TType& b);
	return ((tcompare) context)( *a, *b );
}

struct pvect_double_context
{
	void* Context;
	void* Compare;
};

// We need two levels of indirection because we need to perform the dereference first
template <typename T>
int pvect_double_marshal_context_is_compare( void* context, const T& a, const T& b )
{
	// save decltype() hack as above
	typedef decltype(*a) TType;
	typedef int (*tcompare)(void* context, const TType& a, const TType& b);

	pvect_double_context* cxA = (pvect_double_context*) context;

	return ((tcompare) cxA->Compare)( cxA->Context, *a, *b );
}

template <typename T> void sort( pvect<T*>& target )
{
	vect_sort_cx<T*>( &target[0], 0, target.size() - 1, NULL, pvect_compare_operator_lt<T*> );
}

template <typename T> void sort( pvect<T*>& target, int (*compare) (const T& a, const T& b) )
{
	vect_sort_cx<T*>( &target[0], 0, target.size() - 1, compare, pvect_marshal_context_is_compare<T*> );
}

template <typename T> void sort( pvect<T*>& target, void* context, int (*compare) (void* context, const T& a, const T& b) )
{
	// double-trouble!
	pvect_double_context cxA;
	cxA.Compare = compare;
	cxA.Context = context;
	vect_sort_cx<T*>( &target[0], 0, target.size() - 1, &cxA, pvect_double_marshal_context_is_compare<T*> );
}

// Returns a copy of the pvect, sorted based on your comparison function
template< typename T >
pvect<T*> sorted_by( const pvect<T*>& unsorted, int (*compare) (const T& a, const T& b) )
{
	pvect<T*> all = unsorted;
	sort( all, compare );
	return all;
}


template < typename TData >
void sort_insertion( pvect<TData>& target )
{
	sort_insertion( target, 0, target.size() - 1 );
}

/** Very simple insertion sort for pvect.
The idea behind this is to avoid generating all the code needed for qsort,
when only a very simple sort for a tiny set is required.
**/
template < typename TData >
void sort_insertion( pvect<TData>& target, intp i, intp j )
{
	pvect<TData> rep;
	for ( intp k = i; k <= j; k++ )
	{
		TData a = target[k];
		intp q;
		for ( q = 0; q < rep.size(); q++ )
		{
			if ( *a < *rep[q] )
			{
				rep.insert( q, a );
				break;
			}
		}
		if ( q == rep.size() )
			rep.push_back( a );
	}
	for ( intp k = i, s = 0; k <= j; k++, s++ )
		target[k] = rep[s];
}

/** Generic sort item.
Most custom sorts can be made using the following structure that has
for it's template argument either a string, float, or integer.
The idea is that you use these to reduce bloat.
**/
template< typename ST >
struct sort_item
{
	ST Key;
	void* Object;
	bool operator< ( const sort_item& b ) const
	{
		return Key < b.Key;
	}
};


template< typename TData >
intp least( const pvect<TData>& items )
{
	if ( items.size() == 0 ) { ASSERT(false); return -1; }
	TData v = items[0];
	intp index = 0;
	for ( intp i = 0; i < items.size(); i++ )
	{
		if ( *items[i] < *v )
		{
			v = items[i];
			index = i;
		}
	}
	return index;
}

template< typename TData >
intp greatest( const pvect<TData>& items )
{
	if ( items.size() == 0 ) { ASSERT(false); return -1; }
	TData v = items[0];
	intp index = 0;
	for ( intp i = 0; i < items.size(); i++ )
	{
		if ( *v < *items[i] )
		{
			v = items[i];
			index = i;
		}
	}
	return index;
}

template<typename TData> void remove_duplicates( pvect<TData>& target )	{ return vect_remove_duplicates( target ); }

//#include "pvect_sort.h"

#undef PVINT
#endif
