#pragma once


// The 2003 compiler gives me a bunch of false 'size_t to UINT' warnings.
#pragma warning( push )
#pragma warning( disable: 4267 )

/** A simple space coalescer.

Operations on the space heap are linearly proportional to the number of fragments. The
internal storage is a linear array, not a linked list, so performance is potentially
very bad. We could back this thing's sorted list with a B+Tree if it become necessary.

**/
template< typename TPos = size_t >
class TCoalescer
{
public:
	static const TPos InvalidPos = -1;

	struct Region
	{
		TPos Pos;
		TPos Len;
	};

	TCoalescer()
	{
		Reset(0);
	}

	/** Reset the heap.
	After reset, the entire space is marked as unused.
	**/ 
	void Reset( TPos length )
	{
		Size = length;
		Edge.clear();
		Edge.push_back( 0 );
		Edge.push_back( Size );
		FirstUsed = false;
		TotalUsed = 0;
	}

	/// Retrieve the size of the heap.
	TPos GetSize() const { return Size; }

	/// Retrieve the amount of used space in the heap.
	TPos GetTotalUsed() const { return TotalUsed; }

	/// Returns the number of sections.
	int SectionCount() const { return Edge.size() - 1; }

	/** Grow the heap to the specified size.
	The new space is marked as unused.
	@param size The new size of the heap. This must be greater than or equal to the current heap size.
	**/
	void Grow( TPos size )
	{
		if ( size <= Size ) { ASSERT(false); return; }
		Size = size;

		if ( Used( Edge.size() - 1 ) )
		{
			Edge.push_back( Size );
		}
		else
		{
			Edge[ Edge.size() - 1 ] = Size;
		}
	}

	/** Mark a section as used.
	@param begin The beginning of the section.
	@param length The length of the section.
	**/
	void UseRegion( TPos begin, TPos length )
	{
		ASSERT( length > 0 );
		int reg = FindRegion( begin );
		ASSERT( !Used(reg) );					// do not use an already used region.
		ASSERT( Edge[reg] >= begin + length );	// do not cross boundaries.
		Flip( reg, begin, length );
		TotalUsed += length;
	}

	/** Mark a section as released.
	@param begin The beginning of the section.
	@param length The length of the section.
	**/
	void ReleaseRegion( TPos begin, TPos length )
	{
		ASSERT( length > 0 );
		int reg = FindRegion( begin );
		ASSERT( Used(reg) );					// do not release an unused region.
		ASSERT( Edge[reg] >= begin + length );	// do not cross boundaries.
		Flip( reg, begin, length );
		TotalUsed -= length;
	}

	/** Find an unused section of the specified size.
	@param size The size of the block requested.
	@return The start of the available unused space, or InvalidPos if there is not enough free space.
	**/
	TPos FindUnusedRegion( TPos size ) const
	{
		bool used = FirstUsed;
		for ( int i = 0; i < Edge.size() - 1; i++, used = !used )
		{
			if ( !used )
			{
				if ( Edge[i + 1] - Edge[i] >= size )
				{
					return Edge[i];
				}
			}
		}
		return InvalidPos;
	}

	/// Returns a vector of pairs, containing [Start, Length] of used regions.
	dvect<TPos> GetUsedRegions() const
	{
		return GetRegions( true );
	}

	/// Returns a vector of used Region objects.
	dvect<Region> GetUsedRegionsP() const
	{
		return GetRegionsP( true );
	}

	/// Returns a vector of pairs, containing [Start, Length] of unused regions.
	dvect<TPos> GetUnusedRegions() const
	{
		return GetRegions( false );
	}

	/// Returns a vector of unused Region objects.
	dvect<Region> GetUnusedRegionsP() const
	{
		return GetRegionsP( false );
	}

protected:

	TPos Size;

	TPos TotalUsed;

	/** Edges of regions.
	The value marks the end of the region.
	**/
	dvect< TPos > Edge;

	/// True if region one is used.
	bool FirstUsed;

	/// Returns true if the region is used.
	bool Used( int region ) const
	{
		bool used = (region & 1) != 0;
		return FirstUsed ? used : !used;
	}

	/// Get all used or unused regions
	dvect<TPos> GetRegions( bool used ) const
	{
		dvect<TPos> all;
		bool on = FirstUsed;
		if ( !used ) on = !on;
		for ( int i = 1; i < Edge.size(); i++, on = !on )
		{
			if ( on )
			{
				all += Edge[i - 1];
				all += Edge[i] - Edge[i - 1];
			}
		}
		return all;
	}

	/// Get all used or unused regions
	dvect<Region> GetRegionsP( bool used ) const
	{
		dvect<Region> all;
		bool on = FirstUsed;
		if ( !used ) on = !on;
		for ( int i = 1; i < Edge.size(); i++, on = !on )
		{
			if ( on )
			{
				Region r;
				r.Pos = Edge[i - 1];
				r.Len = Edge[i] - Edge[i - 1];
				all += r;
			}
		}
		return all;
	}

	void FlipWhole( int region )
	{
		if ( region == 1 && Edge.size() == 2 )
		{
			// there is only one region, and that region must be flipped.
			FirstUsed = !FirstUsed;
		}
		else if ( region == 1 )
		{
			// flip the first region
			Edge.erase( 1 );
			FirstUsed = !FirstUsed;
		}
		else if ( region == Edge.size() - 1 )
		{
			// flip the last region (which is to extend the 2nd last region)
			Edge.erase( region - 1 );
		}
		else
		{
			// flip any other region
			Edge.erase( region );
			Edge.erase( region - 1 );
		}
		ASSERT( Edge.size() >= 2 );
	}

	void Flip( int region, TPos begin, TPos length )
	{
		if ( begin == Edge[ region - 1 ] && begin + length == Edge[ region ] )
		{
			// flip an entire region
			FlipWhole( region );
			return;
		}

		if ( begin == Edge[ region - 1 ] )
		{
			// eat off beginning of region
			// Since a split always changes the state of the region it is splitting,
			// we can usually just increase the edge behind us. Unless that edge happens
			// to be the zero edge.
			if ( region == 1 )
			{
				ASSERT( begin == 0 );
				FirstUsed = !FirstUsed;
				Edge.insert( 1, length );
			}
			else
			{
				// shift the prior region up.
				Edge[ region - 1 ] = begin + length;
			}
		}
		else if ( begin + length == Edge[ region ] )
		{
			// eat off end of region
			// Bring the region down, unless we are the last region.
			if ( region == Edge.size() - 1 )
			{
				Edge.insert( region, begin );
			}
			else
			{
				// bring the region down
				Edge[ region ] = begin;
			}
		}
		else
		{
			// eat middle of region
			Edge.insert( region, begin );
			Edge.insert( region + 1, begin + length );
		}
		ASSERT( Edge.size() >= 2 );
	}

	/// Find the region owning the position.
	int FindRegion( TPos pos ) const
	{
		if ( pos < 0 || pos > Size ) { ASSERT(false); return -1; }

		for ( int i = 1; i < Edge.size(); i++ )
		{
			if ( Edge[i] > pos ) return i; 
		}

		// this should not be reachable.
		ASSERT(false);
		return 0;
	}

};

typedef TCoalescer<size_t> Coalescer;
typedef TCoalescer<UINT32> Coalescer32;
typedef TCoalescer<UINT64> Coalescer64;

template< typename TPos >
class TMiniHeap
{
public:

	typedef TCoalescer<TPos> TSpace;
	static const TPos InvalidPos = TSpace::InvalidPos;

	void Reset( TPos size )
	{
		Space.Reset( size );
		Regions.clear();
	}

	void Grow( TPos size )
	{
		Space.Grow( size );
	}

	/// Returns InvalidPos if there is no more free space left.
	TPos Alloc( TPos len )
	{
		return Space.FindUnusedRegion( len );
	}

	/// Returns the size of an allocated block
	TPos SizeOf( TPos pos )
	{
		int reg = FindRegion(pos);
		if ( reg < 0 ) { ASSERT(false); return InvalidPos; }
		return Regions[pos].Len;
	}

	void Free( TPos pos )
	{
		int reg = FindRegion(pos);
		if ( reg < 0 ) { ASSERT(false); return; }
		Space.ReleaseRegion( Regions[reg].Pos, Regions[reg].Len );
		Regions.erase( reg );
	}

protected:
	typedef typename TSpace::Region Region;

	TSpace Space;
	dvect< Region > Regions;

	int FindRegion( TPos pos )
	{
		for ( int i = 0; i < Regions.size(); i++ )
		{
			if ( Regions[i].Pos == pos ) return i;
		}
		return -1;
	}
};

#pragma warning( pop )
