#pragma once

// This is brain dead naive. We obviously need a better box packer. The one from freetype-gl looks decent.
class NUAPI nuTextureAtlas
{
public:
				nuTextureAtlas();
				~nuTextureAtlas();
	void		Initialize( uint width, uint height, uint bytes_per_texel );
	void		Free();
	bool		Alloc( uint16 width, uint16 height, uint16& x, uint16& y );
	
	void*		DataAt( int x, int y )				{ return Data + BytesPerTexel * (y * Stride + x); }
	const void*	DataAt( int x, int y ) const		{ return Data + BytesPerTexel * (y * Stride + x); }
	int			GetBytesPerTexel() const			{ return BytesPerTexel; }
	int			GetStride() const					{ return Stride; }
	uint		GetWidth() const					{ return Width; }
	uint		GetHeight() const					{ return Height; }

protected:
	byte*	Data;
	uint	Width;
	uint	Height;
	uint	BytesPerTexel;
	int		Stride;

	uint	PosTop;
	uint	PosBottom;
	uint	PosRight;
};