#ifndef ABCORE_INCLUDED_SMALLVEC_H
#define ABCORE_INCLUDED_SMALLVEC_H

#include "cont_utils.h"

#ifndef _WIN32
// for intptr_t
#include <inttypes.h>
#endif

template < typename VT, size_t TStaticSize >
class smallvec
{
private:
	size_t mCount;
	size_t mDynamicCapacity;
	VT mDataStatic[ TStaticSize ];
	VT* mDataDynamic;

	void init()
	{
		mCount = 0;
		mDynamicCapacity = 0;
		mDataDynamic = NULL;
	}

public:
	smallvec()
	{
		init();
	}

	smallvec( const smallvec& b )
	{
		init();
		*this = b;
	}

	~smallvec()
	{
		clear();
	}

	smallvec& operator=( const smallvec& b )
	{
		clear();

		mCount = b.mCount;
		mDynamicCapacity = b.mDynamicCapacity;

		size_t staticUsed = _min_(TStaticSize, mCount);
		for ( size_t i = 0; i < staticUsed; i++ )
			mDataStatic[i] = b.mDataStatic[i];

		if ( mDynamicCapacity > 0 )
		{
			mDataDynamic = new VT[ mDynamicCapacity ];
			size_t dynamicUsed = _max_((ptrdiff_t) mCount - (ptrdiff_t) TStaticSize, 0);
			for ( size_t i = 0; i < dynamicUsed; i++ )
				mDataDynamic[i] = b.mDataDynamic[i];
		}

		return *this;
	}

	void clear()
	{
		mCount = 0;
		delete []mDataDynamic;
		mDataDynamic = NULL;
		mDynamicCapacity = 0;
	}

	void erase( size_t index )
	{
		if ( index >= 0 && index < TStaticSize - 1 )
		{
			for ( size_t i = index; i < TStaticSize - 1; i++ )
				mDataStatic[i] = mDataStatic[ i + 1 ];
		}
		intptr_t dynamicCount = mCount - TStaticSize;
		if ( dynamicCount > 0 )
		{
			if ( index < TStaticSize )
			{
				mDataStatic[TStaticSize - 1] = mDataDynamic[0];
			}
			intptr_t start = index - TStaticSize;
			if ( start < 0 ) start = 0;
			for ( intptr_t i = start; i < dynamicCount - 1; i++ )
				mDataDynamic[i] = mDataDynamic[ i + 1 ];
		}
		mCount--;
	}

	size_t size() const { return mCount; }

	VT& operator[]	( size_t index )
	{
		if ( index >= TStaticSize ) return mDataDynamic[ index - TStaticSize ];
		else return mDataStatic[ index ];
	}

	const VT& operator[]	( size_t index ) const
	{
		if ( index >= TStaticSize ) return mDataDynamic[ index - TStaticSize ];
		else return mDataStatic[ index ];
	}

	size_t occurence( const VT& occurenceOf ) const
	{
		size_t c = 0; 
		for ( size_t i = 0; i < mCount; i++ )
			c += (*this)[i] == occurenceOf;
		return c;
	}

	smallvec& operator+=( const VT& a )
	{
		push_back( a );
		return *this;
	}

	void push_back( const VT& a )
	{
		if ( mCount - TStaticSize == mDynamicCapacity ) AllocDynamic();
		(*this)[ mCount ] = a;
		mCount++;
	}

	void pop_back()
	{
		if ( mCount == 0 ) return;
		mCount--;
	}

protected:

	void AllocDynamic()
	{
		if ( mDataDynamic == NULL ) 
		{
			mDynamicCapacity = _max_(2, TStaticSize);
			mDataDynamic = new VT[ mDynamicCapacity ];
		}
		else 
		{
			size_t newCap = mDynamicCapacity * 2;
			VT* newDat = new VT[ newCap ];
			intptr_t dynamicCount = mCount - TStaticSize;
			for ( intptr_t i = 0; i < dynamicCount; i++ )
				newDat[i] = mDataDynamic[i];
			delete []mDataDynamic;
			mDataDynamic = newDat;
			mDynamicCapacity = newCap;
		}
	}
};

typedef unsigned int uint;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This is designed to be a vector with a pre-allocated buffer on the stack. If that buffer runs out,
// then we switch to a heap allocated buffer. We never go back from the heap to the stack. Lifetime: short.
// Significant difference with smallvec is that our buffer is always contiguous.
template < typename VT, int TStaticSize >
class smallvec_stack
{
private:
	int mCount;
	int mCapacity;
	VT* mData;
	VT	mStatic[TStaticSize];

public:

	smallvec_stack()
	{
		mCount = 0;
		mCapacity = TStaticSize;
		mData = mStatic;
	}

	~smallvec_stack()
	{
		if ( mCapacity != TStaticSize )
			delete[] mData;
	}

	void clear()
	{
		mCount = 0;
	}

	void erase( int index )
	{
		mCount--;
		for ( int i = index; i < mCount; i++ )
			mData[i] = mData[ i + 1 ];
	}

	int size() const { return mCount; }

	VT& operator[] ( int index )
	{
		return mData[ index ];
	}

	smallvec_stack& operator+=( const VT& a )
	{
		push_back( a );
		return *this;
	}

	void push_back( const VT& a )
	{
		ASSERT( &a < mData || &a >= mData + mCapacity ); // no support for inserting an existing member
		if ( mCount == mCapacity ) Alloc();
		mData[mCount++] = a;
	}

	void pop_back()
	{
		if ( mCount != 0 )
			mCount--;
	}

protected:

	void Alloc()
	{
		VT* old = mData;
		bool wasStatic = mCapacity == TStaticSize;
		mCapacity = mCapacity * 2;
		mData = new VT[ mCapacity ];
		for ( int i = 0; i < mCount; i++ )
			mData[i] = old[i];
		if ( !wasStatic ) 
			delete[] old;
	}
};

template < typename TData, int TStaticSize >
void sort( smallvec_stack<TData, TStaticSize>& target )
{
	vect_sort< smallvec_stack<TData, TStaticSize>, TData >( target );
}

#endif
