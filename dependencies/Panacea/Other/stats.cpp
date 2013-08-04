#include "pch.h"
#include "libmath.h"
#include "stats.h"





void force_linkage()
{
	dvect<double> a;
	dvect<float> b;
	dvect<int> c;
	dvect<INT64> d;
	dvect<Vec3> e;
	dvect<bool> f;
	dvect<XStringA> g;
	dvect<XStringW> h;
	dvect<UINT> i;
	FindMode( c );
	FindMode( d );
	FindMode( e );
	FindMode( f );
	FindMode( g );
	FindMode( h );
	FindMode( i );
	FindAverage( a );
	FindAverage( b );
	FindAverage( c );
	FindAverage( d );
	FindVariance( a );
	FindVariance( a, 0 );
	FindVariance( b );
	FindVariance( b, 0 );
	FindVariance( c );
	FindVariance( c, 0 );
	FindVariance( d );
	FindVariance( d, 0 );
}
