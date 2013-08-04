#include "pch.h"
#include "../Vec/Vec3.h"
#include "../Other/ChunkList.h"
#include "../Bits/BitTree.h"


void junk_instantiate()
{
	{
		Vec3T<double> a, b;
		double dub = 1;
		a = 1.1 * b;
		
		Vec3T<float> fa, fb;
		fa = 1.1f * fb;
	}

	{
		Vec2T<double> a, b;
		a <= b;
	}

	{
		ChunkList32 cl1;
		ChunkList64 cl2;
	}
	{
		AbcBitTree bt;
	}

/*	{
		dvect< Vec2 > vx;
		vx.push_back( Vec2(1,2) );
		vx.push_back( Vec2(2,4) );
		vx.push_back( Vec2(3,8) );
		IsPolygonCCW( 10, &vx.front() );
	}*/
}
