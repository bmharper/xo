#ifndef ABCORE_INCLUDED_CONT_UTILS_H
#define ABCORE_INCLUDED_CONT_UTILS_H

// for std::swap, which is a very sensible function
#include <algorithm>

// ---------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------
// Container Utilities. 
// Specific utilities for my own STL-like containers.
// I don't like the STL much.
// ---------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------

/** Moves the item at index 'top' to position 0 of the list, and bumps everything else down.
This was created for keeping a small list of recently used items. It allows you the convenience of avoiding having
to keep a 'date' field around, and managing it's wrap-around.
**/
template< typename TVal >
void shuffle_to_top( int n, TVal* items, int itop )
{
	if ( n == 0 || itop == 0 ) return;
	TVal old_top = items[itop];

	// bump down everything below itop
	for ( int i = itop; i > 0; i-- ) items[i] = items[i - 1];

	// and insert new top
	items[0] = old_top;
}

template< typename TVect >
void shuffle_to_top_v( TVect& items, int itop )
{
	if ( items.size() > 0 ) shuffle_to_top( items.size(), &items[0], itop );
}

/// Vector equality
template < typename TVect >
bool vect_equals( const TVect& a, const TVect& b )
{
	if ( a.size() != b.size() ) return false;
	for ( size_t i = 0; i < (size_t) a.size(); i++ )
		if ( a[i] != b[i] ) return false;
	return true;
}

// Returns the index of the first item, or -1 if not found
template <typename TVect, typename T>
int vect_indexof( const TVect& list, const T& val )
{
	for ( intp i = 0; i < list.size(); i++ )
		if ( list[i] == val )
			return i;
	return -1;
}

template<typename TVect, typename T>
void vect_insert_if_not_exist( TVect& set, const T& val )
{
	if ( vect_indexof(set, val) == -1 ) set.push_back( val );
}

template<typename TVect, typename T>
void vect_erase_all_of( TVect& set, const T& val )
{
	for ( intp i = set.size() - 1; i >= 0; i-- )
		if ( set[i] == val )
			set.erase(i);
}

template <typename TVect>
TVect vect_sum( const TVect& a, const TVect& b )
{
	TVect res;
	res.reserve( a.size() + b.size() );

	for ( intp i = 0; i < a.size(); i++ )	res.push_back( a[i] );
	for ( intp i = 0; i < b.size(); i++ )	res.push_back( b[i] );

	return res;
}

/** Reverse for vector.
**/
template < typename TVect, typename TData >
void vect_reverse( TVect& target )
{
	int lim = target.size() / 2;
	for ( int i = 0; i < lim; i++ )
	{
		int j = target.size() - i - 1;
		std::swap( target[i], target[j] );
	}
}

// O(n^2)
template<typename TVect>
void vect_remove_duplicates( TVect& target )
{
	for ( intp i = 0; i < target.size(); i++ )
	{
		for ( intp j = i + 1; j < target.size(); j++ )
		{
			if ( target[i] == target[j] )
			{
				target.erase(j);
				j--;
			}
		}
	}
}

/** find smallest item in a vector.
@return -1 If vector is empty, otherwise the index of the minimum value.
	If there are duplicates, it will be the first instance of the minimum.
**/
template< typename TVect, typename TData >
int vect_least( const TVect& items )
{
	if ( items.size() == 0 ) { ASSERT(false); return -1; }
	TData v = items[0];
	int index = 0;
	for ( intp i = 0; i < items.size(); i++ )
	{
		if ( items[i] < v )
		{
			v = items[i];
			index = i;
		}
	}
	return index;
}

/** find greatest item in a vector.
@return -1 If vector is empty, otherwise the index of the maximum value.
	If there are duplicates, it will be the first instance of the maximum.
**/
template< typename TVect, typename TData >
int vect_greatest( const TVect& items )
{
	if ( items.size() == 0 ) { ASSERT(false); return -1; }
	TData v = items[0];
	int index = 0;
	for ( intp i = 0; i < items.size(); i++ )
	{
		if ( v < items[i] )
		{
			v = items[i];
			index = i;
		}
	}
	return index;
}

template< typename TVect >
bool vect_is_sorted( const TVect& v )
{
	for ( intp i = 1; i < v.size(); i++ )
	{
		if ( v[i] < v[i - 1] ) return false;
	}
	return true;
}

template< typename TData >
bool array_is_sorted( intp _size, const TData* a )
{
	uintp size = _size;
	for ( uintp i = 1; i < size; i++ )
	{
		if ( a[i] < a[i - 1] ) return false;
	}
	return true;
}

// SYNC-BMH-QSORT
template < typename TData >
void vect_sort_cx( TData* target, intp i, intp j, void* context, int (*compare) (void* context, const TData& a, const TData& b), int stackDepth = 0 )
{
	if ( j <= i ) return;
	if ( j == i + 1 )
	{
		// pair
		if ( compare( context, target[j], target[i] ) < 0 )
			std::swap( target[i], target[j] );
		return;
	}

	// choose alternate pivot if we detect that the list is inversely sorted.
	// not doing so will produce a stack overflow with even a relatively small set.
	//TData pivot = target[(i + j) / 2];
	TData* pivot = target + (i + j) / 2;
	if ( stackDepth > 40 ) 
		//pivot = target[ i + (rand() % (1 + j - i)) ];
		pivot = target + i + (rand() % (1 + j - i));

	intp inI = i;
	intp inJ = j;
	i--;
	j++;
	while ( i < j )
	{
		j--;
		while ( compare(context, *pivot, target[j]) < 0 && i < j ) 
			j--;

		if ( i != j ) 
			i++;
		while ( compare(context, target[i], *pivot) < 0 && i < j )
			i++;

		if ( i < j )
		{
			if ( pivot == target + i )		pivot = target + j;
			else if ( pivot == target + j ) pivot = target + i;
			std::swap( target[i], target[j] );
		}
	}
	if ( inI < i )			vect_sort_cx<TData>( target, inI, i, context, compare, stackDepth + 1 );
	if ( i + 1 < inJ )		vect_sort_cx<TData>( target, i + 1, inJ, context, compare, stackDepth + 1 );
}

// Wrap static TData::less_than function for use as a contexual compare() function
template< typename TData >
int less_than_2_compare( void* context, const TData& a, const TData& b )
{
	return TData::less_than( a, b ) ? -1 : 0;
};

// Wrap a special TData::less_than providing class for use as a contexual compare() function
template< typename TData, typename TCompare >
int less_than_t_2_compare( void* context, const TData& a, const TData& b )
{
	return TCompare::less_than( a, b ) ? -1 : 0;
};

template< typename TData >
int compare_default_contexual( void* context, const TData& a, const TData& b )
{
	return a < b ? -1 : 0;
};

// We use the context to transmit the comparison function. The comparison function that the sorter
// sees is actually this function right here. This extra indirection is the price you pay for
// always supporting a context in the comparator, and also supporting context-less comparison function pointers.
template< typename TData >
int marshal_context_is_compare( void* context, const TData& a, const TData& b )
{
	typedef int (*tcompare)(const TData& a, const TData& b);
	return ((tcompare)context)( a, b );
};

/** Sort for vector.
TCompare is a struct/class that has a static function less_than, which
takes two argument and looks like "bool less_than(const TData& a, const TData& b)".
**/
template < typename TVect, typename TData, typename TCompare >
void vect_sort( TVect& target )
{
	if ( target.size() == 0 ) return;
	vect_sort_cx( &target[0], 0, target.size() - 1, NULL, &less_than_t_2_compare<TData, TCompare> );
}

template < typename TVect, typename TData >
void vect_sort( TVect& target )
{
	if ( target.size() == 0 ) return;
	vect_sort_cx( &target[0], 0, target.size() - 1, NULL, &less_than_2_compare<TData> );
}


template < typename TData >
void vect_sort( TData* target, int i, int j )
{
	vect_sort_cx( &target[0], 0, j, NULL, &compare_default_contexual<TData> );
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Do not sort original data - sort a table that points into the original data

template<typename TData>
struct indirect_sort_context
{
	const TData*	Data;
	void*			OrgContext;
	int				(*OrgCompare)(void* context, const TData& a, const TData& b);
};

template<typename TData, typename TOrder>
int indirect_compare( void* context, const TOrder& ia, const TOrder& ib )
{
	indirect_sort_context<TData>* cx = (indirect_sort_context<TData>*) context;
	return cx->OrgCompare( cx->OrgContext, cx->Data[ia], cx->Data[ib] );
}

template <typename TData, typename TOrder>
void vect_sort_indirect( const TData* target, TOrder* ordering, intp n, void* context, int (*compare) (void* context, const TData& a, const TData& b) )
{
	indirect_sort_context<TData> wrappedcx;
	wrappedcx.Data = target;
	wrappedcx.OrgContext = context;
	wrappedcx.OrgCompare = compare;

	for ( TOrder k = 0; k < (TOrder) n; k++ ) ordering[k] = k;
	vect_sort_cx( ordering, 0, n - 1, &wrappedcx, indirect_compare<TData,TOrder> );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Explicit half-instantiation of template utils, to reduce template ambiguity.
#define INSTANTIATE_VECTOR_FUNCTIONS(VECT) \
template <class T> VECT<T> operator+( const VECT<T>& a, const VECT<T>& b ) { return vect_sum< VECT<T> >(a, b); } \
\
template<typename TData>	void	reverse( VECT<TData>& target )					{ vect_reverse< VECT<TData>, TData >( target ); } \
template<typename TData>	int		greatest( const VECT<TData>& target )			{ return vect_greatest<VECT<TData>, TData>( target ); } \
template<typename TData>	int		least( const VECT<TData>& target )				{ return vect_least<VECT<TData>, TData>( target ); } \
template<typename TData>	void	remove_duplicates( const VECT<TData>& target )	{ return vect_remove_duplicates< VECT<TData> >( target ); } \
template<typename TData>	bool	is_sorted( const VECT<TData>& target )			{ return vect_is_sorted< VECT<TData> >( target ); } \
\
template<typename TData, typename TCompare>	void sort( VECT<TData>& target )					{ vect_sort_cx( target.data, 0, target.size() - 1, NULL, &less_than_t_2_compare<TData, TCompare> ); }\
template<typename TData>					void sort( VECT<TData>& target )					{ vect_sort_cx( target.data, 0, target.size() - 1, NULL, &compare_default_contexual<TData> ); }\
template<typename TData>					void sort( VECT<TData>& target, intp i, intp j )	{ vect_sort_cx( target.data, i, j, NULL, &compare_default_contexual<TData> ); }\
template<typename TData, typename TCompare>	void sort( VECT<TData>& target, intp i, intp j )	{ vect_sort_cx( target.data, i, j, NULL, &less_than_t_2_compare<TData, TCompare> ); }\
template<typename TData>					void sort( VECT<TData>& target, int (*compare) (const TData& a, const TData& b) )							{ vect_sort_cx( target.data, 0, target.size() - 1, compare, marshal_context_is_compare<TData> ); }\
template<typename TData>					void sort( VECT<TData>& target, void* context, int (*compare) (void* cx, const TData& a, const TData& b) ) { vect_sort_cx( target.data, 0, target.size() - 1, context, compare ); } \
template<typename TData, typename TOrder>	void sort_indirect( VECT<TData>& target, TOrder* ordering, void* context, int (*compare) (void* cx, const TData& a, const TData& b) ) { vect_sort_indirect( target.data, ordering, target.size(), context, compare ); }



#endif
