#pragma once
#include "../Vec/Vec2.h"
#include "../Vec/Vec3.h"
#include "../Vec/Bounds2.h"
#include "../Vec/Bounds3.h"

template < typename FT >
class TMesh2D
{
public:
	TMesh2D(void)
	{
	}

	TMesh2D( const TMesh2D& copy )
	{
		*this = copy;
	}

	~TMesh2D(void)
	{
	}

	typedef Vec2T<FT> TVec;

	TMesh2D& operator=( const TMesh2D& copy )
	{
		Width = copy.Width;
		Height = copy.Height;
		Vertices = copy.Vertices;
		return *this;
	}

	void Create( int width, int height )
	{
		Width = width;
		Height = height;
		Vertices.resize( Width * Height );
	}

	/** Returns the straight array of vertices.
	The vertices are stored row by row.
	**/
	TVec* GetVertexArray() { return &Vertices[0]; }

	/** Returns the straight array of vertices.
	The vertices are stored row by row.
	**/
	const TVec* GetVertexArray() const { return &Vertices[0]; }

	TVec& Vertex( int x, int y )				{ return Vertices[y * Width + x]; }
	const TVec& Vertex( int x, int y ) const	{ return Vertices[y * Width + x]; }

	/** Returns uncached bounds of mesh.
	**/
	Bounds2 Bounds()
	{
		Bounds2 bb;
		for (int i = 0; i < Width; i++)
		{
			for (int j = 0; j < Height; j++)
			{
				bb.ExpandToFit( Vertex(i,j) );
			}
		}
		return bb;
	}

	TVec UV( int x, int y )
	{
		TVec uv = TVec( x / (FT) (Width - 1), y / (FT) (Height - 1) );
		return uv;
	}

	int GetWidth() const { return Width; }
	int GetHeight() const { return Height; }

protected:
	int Width, Height;
	dvect< TVec > Vertices;
};

typedef TMesh2D<float> MeshFloat2D;
typedef TMesh2D<double> MeshDouble2D;



template < typename FT >
class TMesh3D
{
public:
	TMesh3D(void)
	{
	}

	TMesh3D( const TMesh3D& copy )
	{
		*this = copy;
	}

	~TMesh3D(void)
	{
	}

	typedef Vec2T<FT> TVec2;
	typedef Vec3T<FT> TVec;

	TMesh3D& operator=( const TMesh3D& copy )
	{
		Width = copy.Width;
		Height = copy.Height;
		Vertices = copy.Vertices;
		return *this;
	}

	void Create( int width, int height )
	{
		Width = width;
		Height = height;
		Vertices.resize( Width * Height );
	}

	/** Returns the straight array of vertices.
	The vertices are stored row by row.
	**/
	TVec* GetVertexArray() { return &Vertices[0]; }

	/** Returns the straight array of vertices.
	The vertices are stored row by row.
	**/
	const TVec* GetVertexArray() const { return &Vertices[0]; }

	TVec& Vertex( int x, int y )				{ return Vertices[y * Width + x]; }
	const TVec& Vertex( int x, int y ) const	{ return Vertices[y * Width + x]; }

	/** Returns uncached bounds of mesh.
	**/
	Bounds3 Bounds()
	{
		Bounds3 bb;
		for (int i = 0; i < Width; i++)
		{
			for (int j = 0; j < Height; j++)
			{
				bb.ExpandToFit( Vertex(i,j) );
			}
		}
		return bb;
	}

	TVec2 UV( int x, int y )
	{
		TVec2 uv = TVec2( x / (FT) (Width - 1), y / (FT) (Height - 1) );
		return uv;
	}

	int GetWidth() const { return Width; }
	int GetHeight() const { return Height; }

protected:
	int Width, Height;
	dvect< TVec > Vertices;
};

typedef TMesh3D<float> MeshFloat3D;
typedef TMesh3D<double> MeshDouble3D;
