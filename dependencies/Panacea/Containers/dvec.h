#ifndef ABCORE_INCLUDED_DVEC_H
#define ABCORE_INCLUDED_DVEC_H

#include <string.h>
#include "cont_utils.h"

#ifdef T
#undef T
#endif

//#define DVECT_RANGE_CHECK

#define DVECT_DEFINED (1)
#define DVECT_PANIC() AbcPanic()

// Use this as a hint that if we move away from dvec, then we should change this to intp or size_t
typedef int DVEC_INT;

template <typename T>
class dvect
{
public:
	typedef T TItem;

	// These data members are made public here so that the vector can be used as straight array.
	T* data;
	int count;
	int capacity;

	static void panic()				{ AbcPanicSilent(); }

	dvect()
	{
		count = 0;
		capacity = 0;
		data = NULL;
	}
	explicit dvect( int _reserve )
	{
		count = 0;
		capacity = 0;
		data = NULL;
		reserve( _reserve );
	}
	
	// copy constructor
	dvect( const dvect& b )
	{
		data = NULL;
		capacity = 0;
		count = 0;
		*this = b;
	}

	~dvect()
	{
		delete[] data;
	}

	void hack( int _count, int _capacity, T* _data )
	{
		count = _count;
		capacity = _capacity;
		data = _data;
	}

	dvect& operator=(const dvect &b) 
	{
		if ( this != &b )
		{
			if ( b.count != capacity ) clear();
			if ( b.count > 0 )
			{
				reserve( b.count );
				for ( int i = 0; i < b.count; i++ )
					data[i] = b.data[i];
			}
			count = b.count;
		}
		return *this;
	}

	bool contains( const T& val ) const { return find(val) != -1; }

	int find( const T& val ) const
	{
		for (int i = 0; i < count; i++)
			if ( data[i] == val ) return i;
		return -1;
	}

	int reverse_find( const T& val ) const
	{
		for ( int i = count - 1; i >= 0; i-- )
			if ( data[i] == val ) return i;
		return -1;
	}

	void fill( const T& val )
	{
		for ( int i = 0; i < count; i++ )
			data[i] = val;
	}

	// safe set. Will resize if neccessary
	void set( int pos, const T& a ) 
	{
		if ( pos >= count )
		{
			// Do we need to resize, or can we just up our count?
			if ( pos >= capacity )
			{
				if ( isinternal(&a) ) 
					return set_aliased( pos, a );
				resize( AbcMax(pos+1, capacity*2) );
			}
			else
			{
				count = pos+1;
			}
		}
		data[pos] = a;
	}

	void insert( int pos, const T& a )
	{
		if ( pos >= capacity || count+1 > capacity )
		{
			if ( isinternal(&a) )
				return insert_aliased( pos, a );
			reserve( AbcMax(capacity*2, pos+4) );
		}
		int nmove = count - pos;
		if ( nmove > 0 )
		{
			for ( int i = nmove + pos; i > pos; i-- )
				data[i] = data[i - 1];
		}
		data[pos] = a;
		count++;
	}

	void erase( intp _start, intp _end = -1 )
	{
		uintp start = _start;
		uintp end = _end;
		if ( end == -1 ) end = start + 1;
		if ( start >= (u32) count ) panic();	// erase is rare enough that I feel it's worth it to pay this branch penalty for an early crash.
		if ( end > (u32) count )	panic();
		uintp delta = end - start;
		uintp remain = count - end;
		for ( uintp i = start; i < start + remain; i++ ) data[i] = data[i + delta];
		count = start + remain;
	}

	/*
	void erase( int pos1, int pos2 = -1 ) -- this version was inclusive of pos2
	{
		if ( pos2 == -1 )			pos2 = pos1;
		else if ( pos2 >= count ) pos2 = count - 1;

		int nmove = (count - pos2) - 1;
		if ( nmove > 0 )
		{
			int delta = 1 + pos2 - pos1;
			for ( int i = pos1; i < pos1 + nmove; i++ )
				data[i] = data[i + delta];
		}
		count -= 1 + (pos2 - pos1);
	}
		*/

	T& pop_back() 
	{
		if (count > 0)
			count--;
		return data[count];
	}

	T& add()
	{
		push_back( T() );
		return back();
	}

	int add_default()
	{
		if ( count >= capacity ) reserve(capacity*2);
		return count++;
	}

	void push_back( const T& a ) 
	{
		if ( count >= capacity )
		{
			if ( isinternal(&a) )
				return push_back_aliased( a );
			reserve( capacity * 2 );
		}
		data[count++] = a;
	}

	dvect& operator+=( const T& a )
	{
		push_back( a );
		return *this;
	}

#ifdef DVECT_RANGE_CHECK
	const T& operator[](intr i) const 
	{
		ASSERT( i >= 0 && i < count );
		return data[i];
	}
#else
	const T& operator[](intr i) const { return data[i]; }
#endif

#ifdef DVECT_RANGE_CHECK
	T& operator[](intr i) 
	{
		ASSERT( i >= 0 && i < count );
		return data[i];
	}
#else
	T& operator[](intr i) { return data[i]; }
#endif

	const T& get(int pos) const
	{
#ifdef DVECT_RANGE_CHECK
		ASSERT( pos >= 0 && pos < count );
#endif
		return data[pos];
	}

	T& get(int pos)
	{
#ifdef DVECT_RANGE_CHECK
		ASSERT( pos >= 0 && pos < count );
#endif
		return data[pos];
	}

	const T& back() const { return data[count-1]; }

	T& back() { return data[count-1]; }

	const T& front() const { return data[0]; }

	T& front() { return data[0]; }

	dvect& operator+=( const dvect& b )
	{
		// Once again- this premature optimization can cause bad behaviour. Stupid monkey!
		//reserve( size() + b.size() );

		for (int i = 0; i < b.size(); i++)
			push_back( b[i] );
		return *this;
	}

	bool operator==( const dvect& b ) const { return vect_equals(*this, b); }
	bool operator!=( const dvect& b ) const { return !(*this == b); }

	void clear() 
	{
		delete[] data;
		data = NULL;
		count = 0;
		capacity = 0;
	}
	
	// can be used to avoid reallocs
	void clear_noalloc()
	{
		count = 0;
	}

	// like clear_noalloc, but takes a hint as to the new size. If the old capacity = new size, then we do not resize.
	void clear_noalloc( int hint )
	{
		if ( hint == capacity ) clear_noalloc();
		else					clear();
	}

	// frees our heap storage and sets data to null (but does not touch capacity or count)
	void dealloc()
	{
		delete[] data;
		data = NULL;
	}

	int size() const { return count; }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 6385 6386)	// I believe there is a false positive on the static analysis inside this function
#endif

	// preserves existing entities but doesn't set count
	void reserve( int newsize )
	{
		int oc = count;
		resize(AbcMax(newsize, count));
		count = oc;
	}

	// preserves existing entities and sets count to newsize
	void resize( int newsize, bool force = false ) 
	{
		int ocount = count;
		count = newsize;
		// don't realloc if not more than 20% waste.
		if ( newsize < capacity && !force )
		{
#if ARCH_64
			bool use64 = true;
#else
			bool use64 = capacity > INT32MAX / 16;
#endif
			if ( use64 )
			{
				if ( (INT64) newsize * 10 > (INT64) capacity * 8 ) 
					return;
			}
			else
			{
				if ( newsize * 10 > capacity * 8 ) 
					return;
			}
		}

		if ( newsize == 0 ) newsize = 2;
		T* nd = new T[newsize];
		int tt = std::min(ocount, newsize);
		for (int i = 0; i < tt; i++)
			nd[i] = data[i];
		delete []data;
		data = nd;
		capacity = newsize;
	}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	class iterator
	{
	public:
		iterator( )
		{
			pvec = NULL;
			pos = 0;
		}
		iterator( const dvect* p )
		{
			pvec = p;
			pos = 0;
		}
		T operator*() const 
		{
			if (pos >= pvec->size()) return T();
			return pvec->data[pos];
		}
		iterator& operator++(int)
		{
			if (pos < pvec->size())
				pos++;
			return *this;
		}
		bool operator==( const iterator& b )
		{
			return pvec == b.pvec && pos == b.pos;
		}
		bool operator!=( const iterator& b )
		{
			return !(*this == b);
		}
		int pos;
		const dvect *pvec;
	};

	iterator begin() const
	{
		iterator it(this);
		return it;
	}
	iterator end() const
	{
		iterator it(this);
		it.pos = size();
		return it;
	}

protected:

	bool isinternal( const T* p ) const
	{
		return (uint) (p - data) < (uint) capacity;
	}

	// identical to set, except that we make a copy of 'a'
	void set_aliased( int pos, T a )
	{
		if ( pos >= count )
		{
			// Do we need to resize, or can we just up our count?
			if ( pos >= capacity )
				resize( AbcMax(pos+1, capacity*2) );
			else
				count = pos+1;
		}
		data[pos] = a;
	}

	// identical to push_back, except that we make a copy of 'a'
	void push_back_aliased( T a ) 
	{
		if ( count >= capacity )
			reserve( capacity * 2 );
		data[count++] = a;
	}

	// identical to insert, except that we make a copy of 'a'
	void insert_aliased( int pos, T a )
	{
		if ( pos >= capacity || count+1 > capacity )
			reserve( AbcMax(capacity*2, pos+4) );
		int nmove = count - pos;
		if ( nmove > 0 )
		{
			for ( int i = nmove + pos; i > pos; i-- )
				data[i] = data[i - 1];
		}
		data[pos] = a;
		count++;
	}

};

INSTANTIATE_VECTOR_FUNCTIONS(dvect)

#endif
