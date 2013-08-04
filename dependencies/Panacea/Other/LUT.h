#pragma once

// LUT = Look Up Table

template< typename TData, typename TInterp >
class Lut1D
{
public:

	bool OwnBuffer;
	TData* Data;
	uint Size;

	~Lut1D()
	{
		if ( OwnBuffer ) delete[] Data;
	}

	void Init( uint size )
	{
		Init( true, new TData[size], size );
	}

	void Init( bool lutFreesData, TData* data, uint size )
	{
		OwnBuffer = lutFreesData;
		Data = data;
		Size = size;
	}

	TInterp Lerp( TInterp pos )
	{
		pos = std::max(pos, (TInterp) 0);
		pos = std::min(pos, (TInterp) (Size - 1));
		uint posA = (uint) floor(pos);
		uint posB = (uint) ceil(pos);
		TInterp res = pos - posA;
		TInterp sa = (TInterp) Data[posA];
		TInterp sb = (TInterp) Data[posB];
		return (1.0 - res) * sa + res * sb;
	}

};
