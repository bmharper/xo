#pragma once

#include "Vec5.h"
#include "Vec4.h"
#include "Mat4.h"

#ifndef NO_XSTRING
#include "../Strings/XString.h"
#endif

#ifndef DEFINED_Mat5
#define DEFINED_Mat5
#include "VecDef.h"

template <class FT>
class Mat5T
{
public:
	Vec5T<FT> row[5];

	Mat5T(void)
	{
		Identity();
	}

	// Don't initialize to identity
	Mat5T( int uninit )
	{
	}

	FT&	m(int i, int j) { return row[i][j]; }
	FT	m(int i, int j) const { return row[i][j]; }

	Vec5T<FT>&	operator[](int i)		{ return row[i]; }
	Vec5T<FT>	operator[](int i) const { return row[i]; }

	/// Returns the diagonal
	Vec5T<FT> Diagonal() const { return Vec5T<FT>( row[0].a, row[1].b, row[2].c, row[3].d, row[4].e ); }

	Mat5T Transposed() const 
	{
		Mat5T t;
		for ( int i = 0; i < 5; i++ )
		{
			for ( int j = 0; j < 5; j++ )
			{
				t.row[i].n[j] = row[j].n[i];
			}
		}
		return t;
	} 

	bool IsIdentity() const
	{
		Mat5T ident;
		ident.Identity();
		return *this == ident;
	}

	void Identity()
	{
		for ( int i = 0; i < 5; i++ )
			for ( int j = 0; j < 5; j++ )
				row[i][j] = 0;

		for ( int i = 0; i < 5; i++ )
			row[i][i] = 1;
	}

	void SetColumn( int j, const Vec5T<FT>& v )
	{
		for ( int i = 0; i < 5; i++ )
			row[i][j] = v[i];
	}

	Vec5T<FT> Column( int j ) const
	{
		Vec5T<FT> v;
		for ( int i = 0; i < 5; i++ )
			v[i] = row[i][j];
		return v;
	}

	Mat4T<FT> UpperLeft4x4() const
	{
		Mat4T<FT> m;
		for ( int i = 0; i < 4; i++ )
			for ( int j = 0; j < 4; j++ )
				m.row[i][j] = row[i][j];
		return m;
	}

	void SetUpperLeft4x4( const Mat4T<FT>& m )
	{
		for ( int i = 0; i < 4; i++ )
			for ( int j = 0; j < 4; j++ )
				row[i][j] = m.row[i][j];
	}

	bool operator==( const Mat5T& b ) const
	{
		return memcmp(this, &b, sizeof(b)) == 0;
		//const FT* pa = &row[0][0];
		//const FT* pb = &b.row[0][0];
		//for ( int i = 0; i < 5*5; i++ )
		//{
		//	if ( pa[i] != pb[i] ) return false;
		//}
		//return true;
	}

	bool operator!=( const Mat5T& b ) const
	{
		return !(*this == b);
		//const FT* pa = &row[0][0];
		//const FT* pb = &b.row[0][0];
		//for ( int i = 0; i < 5*5; i++ )
		//{
		//	if ( pa[i] != pb[i] ) return true;
		//}
		//return false;
	}

};


typedef Mat5T<double> Mat5d;
typedef Mat5T<double> Mat5;
typedef Mat5T<float> Mat5f;

#include "VecUndef.h"
#endif // DEFINED_Mat5

