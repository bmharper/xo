#pragma once

/** A simple chunk list, that coagulates overlapping chunks. 

This is used by the BufferedFile to record dirty write regions.

The number of chunks should be small (less than 20 or so).

**/
template< typename TSize >
class ChunkList
{
public:

	typedef TSize TMySize;
	static const TSize NullSize = -1;

	ChunkList()
	{
		ResetCache();
	}

	void Add( TSize start, TSize length )
	{
		if ( length == 0 ) return;

		ResetCache();

		Chunk c;
		c.Start = start;
		c.Length = length;

		if ( Chunks.size() == 0 )
		{
			Chunks += c;
		}
		else
		{
			// This seems like a reasonable optimization
			if ( start >= Chunks.back().End() )
			{
				AddToEnd( c );
				return;
			}

			// Changing this to a binary search would improve performance.
			int i;
			for ( i = 0; i < Chunks.size(); i++ )
			{
				if ( start <= Chunks[i].Start )
					break;
			}
			if ( i < Chunks.size() )	Chunks.insert( i, c );
			else						Chunks += c;
			if ( i > 0 )				Coalesce( i - 1 );
			if ( i < Chunks.size() )	Coalesce( i );
		}
	}

	int Count() const			{ return Chunks.size(); }

	TSize GetStart( int i ) const	{ return Chunks[i].Start; }
	TSize GetEnd( int i ) const		{ return Chunks[i].Start + Chunks[i].Length; }
	TSize GetLength( int i ) const	{ return Chunks[i].Length; }

	bool Check( int i, TSize start, TSize length ) const
	{
		return Chunks[i].Start == start && Chunks[i].Length == length;
	}

	void Get( int i, TSize& start, TSize& length ) const
	{
		start = Chunks[i].Start;
		length = Chunks[i].Length;
	}

	void EatExplicit( TSize start, TSize length )
	{
		TSize end = start + length;
		for ( int i = 0; i < Chunks.size(); i++ )
		{
			if ( Chunks[i].Start >= end ) break;
			TSize p1 = max( Chunks[i].Start, start );
			TSize p2 = min( Chunks[i].End(), end );
			if ( p1 < p2 )
			{
				if ( start <= Chunks[i].Start )
				{
					// either cut away from the start or the whole block
					if ( end >= Chunks[i].End() )
					{
						// whole block
						Chunks.erase(i);
						i--;
					}
					else
					{
						// eat away from start
						Chunks[i].Length -= end - Chunks[i].Start;
						Chunks[i].Start = end;
					}
				}
				else
				{
					// either cut away from the middle or from the end
					if ( end >= Chunks[i].End() )
					{
						// eat away from the end
						Chunks[i].Length = start - Chunks[i].Start;
					}
					else
					{
						// cut out of the middle
						Chunk c;
						c.Start = end;
						c.Length = Chunks[i].End() - end;
						Chunks[i].Length = start - Chunks[i].Start;
						Chunks.insert( i + 1, c );
					}
				}
			}
		}

	}

	TSize Eat( TSize length )
	{
		if ( SizeUnavailable != NullSize && length >= SizeUnavailable )
			return NullSize;

		// Note that a successful Eat operation cannot decrease SizeUnavailable, so we do not need to touch it.
		for ( int i = 0; i < Chunks.size(); i++ )
		{
			TSize start = Chunks[i].Start;
			if ( Chunks[i].Length == length )
			{
				Chunks.erase(i);
				return start;
			}
			else if ( Chunks[i].Length > length )
			{
				Chunks[i].Start += length;
				Chunks[i].Length -= length;
				return start;
			}
		}
		SizeUnavailable = min( SizeUnavailable, length );
		return NullSize;
	}

	void Clear()
	{
		ResetCache();
		Chunks.clear_noalloc();
	}

protected:
	struct Chunk
	{
		TSize Start, Length;
		TSize End() const { return Start + Length; }
	};

	enum Prox
	{
		PrApart,
		PrTouch,
		PrOverlap
	};

	/// A little cache that remembers failed queries.
	TSize SizeUnavailable;

	dvect<Chunk> Chunks;

	void ResetCache()
	{
		SizeUnavailable = NullSize;
	}

	void AddToEnd( Chunk& c )
	{
		if ( Chunks.back().End() == c.Start )
		{
			Chunks.back().Length += c.Length;
		}
		else
		{
			Chunks += c;
		}
	}

	bool Coalesce( int pos )
	{
		bool any = false;
		while ( true )
		{
			if ( pos == Chunks.size() - 1 ) break;
			Prox pr = Analyze( Chunks[pos], Chunks[pos + 1] );
			if ( pr == PrApart )
			{
				break;
			}
			else
			{
				any = true;
				TSize end = _max_( Chunks[pos].End(), Chunks[pos + 1].End() ); 
				Chunks[pos].Length = end - Chunks[pos].Start;
				Chunks.erase( pos + 1 );
				if ( pr == PrTouch )
					break;
			}
		}
		return any;
	}

	static Prox Analyze( Chunk a, Chunk b )
	{
		if ( b.Start < a.Start ) AbCore::Swap( a, b );
		if ( a.End() == b.Start ) return PrTouch;
		if ( a.End() > b.Start ) return PrOverlap;
		return PrApart;
	}
};

typedef ChunkList<UINT32> ChunkList32;
typedef ChunkList<UINT64> ChunkList64;
