#pragma once



class dvec2DBase
{
public:
	int width;
	int height;
	size_t elsize;
	unsigned char* data;

	dvec2DBase()
	{
		data = NULL;
		width = height = 0;
		elsize = 0;
	}

	~dvec2DBase()
	{
		free();
	}

	void free()
	{
		if (data) AbcAlignedFree(data);
		data = NULL;
		width = 0;
		height = 0;
	}

	void fill( int val )
	{
		memset( data, val, elsize * width * height );
	}

	void resize( int w, int h )
	{
		free();
		data = (unsigned char*) AbcAlignedMalloc( w * h * elsize, 8 );
		width = w;
		height = h;
	}

	const void* BPV( int x, int y ) const
	{
		return data + (((y * width) + x) * elsize);
	}

	void* BPV( int x, int y )
	{
		return data + (((y * width) + x) * elsize);
	}

};



template <class T>
class dvec2D : public dvec2DBase
{
public:
	typedef dvec2DBase Base;

	dvec2D(void) : Base()
	{
		elsize = sizeof(T);
	}

	const T& V( int x, int y ) const
	{
		return *( (T*) BPV( x, y ) );
	}

	T& V( int x, int y )
	{
		return *( (T*) BPV( x, y ) );
	}
};
