#ifndef ABCORE_INCLUDED_RANDOM_H
#define ABCORE_INCLUDED_RANDOM_H

class PAPI Random
{
public:
	Random( int init = 0x0abc0123 );

	double Uniform();
	double Gaussian( double mean, double stddev );
	
	/// Return random integer within a range, [lower_inclusive, upper_inclusive]
	int Int( int lower_inclusive, int upper_inclusive )
	{
		return((int)(Uniform() * (upper_inclusive - lower_inclusive + 1)) + lower_inclusive);
	}

	/// Return random integer within a range, [lower_inclusive, upper_inclusive]
	INT64 Int64( INT64 lower_inclusive, INT64 upper_inclusive )
	{
		return((INT64)(Uniform() * (upper_inclusive - lower_inclusive + 1)) + lower_inclusive);
	}

	/// Return random bool
	bool Bool()
	{
		return Int(0, 1) != 0;
	}

	/// Return random float within a range, lower -> upper
	double Double( double lower, double upper )
	{
		return((upper - lower) * Uniform() + lower);
	}

	void Initialise( int seed );

private:
	double u[97],c,cd,cm;
	int i97,j97;
};

namespace AbCore
{

	template< typename TObj >
	void Shuffle( size_t count, TObj* list, Random* rnd = NULL )
	{
		Random srnd;
		if ( !rnd ) rnd = &srnd;
		for ( size_t i = 0; i < count - 1; i++ )
		{
#ifdef _M_X64
			size_t j = rnd->Int64( i, count - 1 );
#else
			size_t j = rnd->Int( (int) i, (int) (count - 1) );
#endif
			AbCore::Swap( list[i], list[j] );
		}
	}

	template< typename TVect >
	void Shuffle( TVect& list, Random* rnd = NULL )
	{
		Shuffle( list.size(), &list[0], rnd );
	}
}

#endif

