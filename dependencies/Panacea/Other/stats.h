#pragma once
#include "../Containers/dvec.h"

#include "../Other/lmDefs.h"
#include "../HashTab/ohashcommon.h"
#include "../Vec/Vec3.h"

template <typename DT, typename hashFunc >
DT FindModeT( const dvect<DT>& data )
{
	typedef ohash::ohashmap< DT, int, hashFunc > valmap;
	valmap count;

	for ( int i = 0; i < data.size(); i++ )
	{
		DT val = (DT) data[i];
		if ( count.contains(val) ) 
		{
			// increment
			count[val]++;
		}
		else 
		{
			count[val] = 1;
		}
	}

	int vmax = -1;
	DT nmax = DT();

	// find max
	for ( typename valmap::iterator it = count.begin(); it != count.end(); it++ ) 
	{
		if ( it->second > vmax ) 
		{
			nmax = it->first;
			vmax = it->second;
		}
	}

	return nmax;
}

//template <typename DT, typename hashFunc >
//DT PAPI FindModeT( const dvect<DT>& data );

// we have to do these wrappers because default template parameters are not allowed
// on templated functions.
inline int FindMode( const dvect<int>& data )
{
	return FindModeT< int, ohash::ohashfunc_cast<int> > ( data );
}

inline UINT FindMode( const dvect<UINT>& data )
{
	return FindModeT< UINT, ohash::ohashfunc_cast<UINT> > ( data );
}

inline INT64 FindMode( const dvect<INT64>& data )
{
	return FindModeT< INT64, ohash::ohashfunc_INT64 > ( data );
}

inline XStringA FindMode( const dvect<XStringA>& data )
{
	return FindModeT< XStringA, ohash::ohashfunc_XString<XStringA> > ( data );
}

inline XStringW FindMode( const dvect<XStringW>& data )
{
	return FindModeT< XStringW, ohash::ohashfunc_XString<XStringW> > ( data );
}

inline double FindMode( const dvect<double>& data )
{
	return FindModeT< double, ohash::ohashfunc_cast<double> > ( data );
}

inline bool FindMode( const dvect<bool>& data )
{
	return FindModeT< bool, ohash::ohashfunc_cast<bool> > ( data );
}

inline Vec3 FindMode( const dvect<Vec3>& data )
{
	return FindModeT< Vec3, ohash::ohashfunc_Vec3 > ( data );
}



template <typename DT>
double FindVariance( const dvect<DT>& data )
{
	return FindVariance( data, FindAverage(data) );
}

template <typename DT>
double FindVariance( const dvect<DT>& data, double average )
{
	double v = 0;
	for (int i = 0; i < data.size(); i++)
	{
		double diff = (data[i] - average);
		v += diff * diff;
	}
	v /= data.size();
	return v;
}

template <typename TVal>
TVal FindMax( size_t n, const TVal* data )
{
	TVal vmax = AbCore::Traits<TVal>::Min();
	for ( size_t i = 0; i < n; i++ )
	{
		if ( data[i] > vmax ) vmax = data[i];
	}
	return vmax;
}

template <typename TVal>
TVal FindMin( size_t n, const TVal* data )
{
	TVal vmin = AbCore::Traits<TVal>::Max();
	for ( size_t i = 0; i < n; i++ )
	{
		if ( data[i] < vmin ) vmin = data[i];
	}
	return vmin;
}

template <typename TVal>
void FindMinMax( size_t n, const TVal* data, TVal& vmin, TVal& vmax )
{
	TVal tmin = AbCore::Traits<TVal>::Max();
	TVal tmax = AbCore::Traits<TVal>::Min();
	for ( size_t i = 0; i < n; i++ )
	{
		if ( data[i] < tmin ) tmin = data[i];
		if ( data[i] > tmax ) tmax = data[i];
	}
	vmin = tmin;
	vmax = tmax;
}

template <typename TVal, typename TAccum>
TAccum FindAverage( size_t n, const TVal* data )
{
	TAccum tot = TAccum();
	for ( size_t i = 0; i < n; i++ )
		tot += data[i];
	return TAccum(tot / n);
}

template< typename TVal, typename TAccum > TAccum FindAverage( const dvect<TVal>& data )	{ return FindAverage<TVal, TAccum>( (size_t) data.size(), &data.front() ); }
template< typename TVal > TVal FindAverage( const dvect<TVal>& data )						{ return FindAverage<TVal, TVal>( (size_t) data.size(), &data.front() ); }

template< typename TVal > TVal FindMin( const dvect<TVal>& data )									{ return FindMin( (size_t) data.size(), &data.front() ); }
template< typename TVal > TVal FindMax( const dvect<TVal>& data )									{ return FindMax( (size_t) data.size(), &data.front() ); }
template< typename TVal > void FindMinMax( const dvect<TVal>& data, TVal& vmin, TVal& vmax )		{ FindMinMax( (size_t) data.size(), &data.front(), vmin, vmax ); }


/*
template <typename DT>
double PAPI FindVariance( const dvect<DT>& data );

template <typename DT>
double PAPI FindVariance( const dvect<DT>& data, double average );

template <typename DT>
double PAPI FindAverage( const dvect<DT>& data );
*/


