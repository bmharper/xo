#ifndef TEXPACK_H
#define TEXPACK_H

#include "../Vec/Rect2I.h"

// TexPack - for packing textures.


struct TexPiece 
{
	int id;
	int width, height;
	unsigned char* data;
	// coordinates are bottom-left, bottom-right, top-right, top-left
	Vec2f UV[4];
	// true if the piece has been rotated 90% CW in the texture.
	bool rotated;
};

/** Texture Atlas Packer.
**/
class PAPI TexPack
{
public:
	TexPack(void);
	~TexPack(void);
#if (_MSC_VER > 1200) || !defined(_WIN32)
	static const int MAX_PIECES = 2000;
#else
	#define MAX_PIECES 2000
#endif

	bool Init( int width, int height, int bytes_per_texel ); 
	bool Insert( int id, int swidth, int sheight, void* data );

	/** Attempt to pack the pieces into the given dimensions.
	@param reserveBorders Allows for the specification of borders on the image which will not be touched.
		For example, to leave a border of 4 pixels all-round, specify Rect2I(4,4,4,4).
	**/
	bool Process( Rect2I reserveBorders = Rect2I(0,0,0,0) );

	unsigned char *Bitmap;
	int Width, Height;
	int BytesPerTexel;
	int NPieces;
	int ClearColor;
	TexPiece *Pieces;
	bool bDEBUG;
protected:
	int CurrentU, CurrentV;
	int CurrentRowHeight;
	unsigned char* BitmapPos( int U, int V )
	{
		return Bitmap + ((V * Width + U) * BytesPerTexel);
	}
	unsigned char* TexPos( TexPiece *p, int X, int Y )
	{
		return p->data + ((Y * p->width + X) * BytesPerTexel);
	}
	void NextRow();
	void SortPieces();
	void DeletePieces();
	static int SortFunc( const void* arg1, const void* arg2 );
};

#endif
