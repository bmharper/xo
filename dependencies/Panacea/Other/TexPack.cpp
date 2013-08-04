#include "pch.h"
#include "TexPack.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _MSC_VER
#pragma warning(disable:4244)  //conversion from 'double' to 'float', possible loss of data
#endif

typedef unsigned char BYTE;

TexPack::TexPack(void)
{
	Bitmap = NULL;
	Pieces = new TexPiece[MAX_PIECES];
	Width = Height = 0;
	NPieces = 0;
	BytesPerTexel = 1;
	ClearColor = 0;
	CurrentU = CurrentV = CurrentRowHeight = 0;
	bDEBUG = false;
}

TexPack::~TexPack(void)
{
	DeletePieces();
	delete []Pieces;
	if (Bitmap)
		delete []Bitmap;
}

// assume 2^n width and heights
bool TexPack::Init( int width, int height, int bytes_per_texel )
{
	DeletePieces();
	if (Bitmap)
		delete []Bitmap;
	BytesPerTexel = bytes_per_texel;
	Width = width;
	Height = height;
	NPieces = 0;
	Bitmap = new BYTE[Width * Height * BytesPerTexel];
	memset( Bitmap, ClearColor, Width*Height*BytesPerTexel );
	return true;
}

// Delete the memory storage for the pieces
void TexPack::DeletePieces()
{
	for (int i = 0; i < NPieces; i++) 
		delete []Pieces[i].data;
	NPieces = 0;
}

// Takes no action yet. Copies memory from data.
bool TexPack::Insert( int id, int swidth, int sheight, void* data )
{
	if (NPieces == MAX_PIECES) 
	{
		fprintf( stderr, "too many pieces\n" );
		return false;
	}
	TexPiece *p = &Pieces[NPieces];
	p->id = id;
	p->width = swidth;
	p->height = sheight;
	p->rotated = false;

	int bytes = swidth * sheight * BytesPerTexel;
	p->data = new BYTE[bytes];
	memcpy( Pieces[NPieces].data, data, bytes );

	NPieces++;
	return true;
}

bool TexPack::Process( Rect2I reserveBorders )
{
	SortPieces();

	if ( reserveBorders.x1 < 0 ) { ASSERT(false); reserveBorders.x1 = 0; }
	if ( reserveBorders.y1 < 0 ) { ASSERT(false); reserveBorders.y1 = 0; }
	if ( reserveBorders.x2 < 0 ) { ASSERT(false); reserveBorders.x2 = 0; }
	if ( reserveBorders.y2 < 0 ) { ASSERT(false); reserveBorders.y2 = 0; }

	// Start packing.
	// We pack in pieces from top to bottom, left to right, like the triangles
	// in that one nvidia presentation (for bump mapping i think). 
	CurrentU = reserveBorders.x1;
	CurrentV = reserveBorders.y1;
	for ( int i = 0; i < NPieces; i++ )
	{
		TexPiece *p = &Pieces[i];
		int minDim = min( p->width, p->height );
		int MaxDimension = max( p->width, p->height );
		int rotate = 0;
		if ( MaxDimension == p->width ) rotate = 1;
		if ( minDim + CurrentU > Width - reserveBorders.x2 )
			NextRow(); // Out of width.
		if ( CurrentV + MaxDimension > Height - reserveBorders.y2 ) 
			return false; // ABORT! Out of space.
		if ( rotate )
		{
			// Rotate tex by 90 degrees clockwise.
			for (int y = 0; y < p->height; y++)
			{
				for (int x = 0; x < p->width; x++)
				{
					*BitmapPos(CurrentU + y, CurrentV + p->width - x - 1) = *TexPos(p, x, y);
				}
			}
			p->rotated = true;
			p->UV[0] = Vec2f( CurrentU, CurrentV + p->width );
			p->UV[1] = Vec2f( CurrentU, CurrentV );
			p->UV[2] = Vec2f( CurrentU + p->height, CurrentV );
			p->UV[3] = Vec2f( CurrentU + p->height, CurrentV + p->width );
		}
		else
		{
			// copy in straight
			for (int row = 0; row < p->height; row++)
				memcpy( BitmapPos(CurrentU, CurrentV + row), TexPos(p, 0, row), p->width * BytesPerTexel );
			p->rotated = false;
			p->UV[0] = Vec2f( CurrentU, CurrentV );
			p->UV[1] = Vec2f( CurrentU + p->width, CurrentV );
			p->UV[2] = Vec2f( CurrentU + p->width, CurrentV + p->height );
			p->UV[3] = Vec2f( CurrentU, CurrentV + p->height );
		}
		CurrentRowHeight = max( CurrentRowHeight, MaxDimension );
		CurrentU += minDim;
		if (bDEBUG) CurrentU++;
	}
	//for (int x = 0; x < Width; x++)
	//	for (int y = 0; y < Height; y++) 
	//		*BitmapPos(x,y) = ((x & 1) == 1) ? 255 : 0;

	return true;
}

void TexPack::NextRow()
{
	CurrentV += CurrentRowHeight;
	if (bDEBUG) CurrentV++;
	CurrentU = 0;
	CurrentRowHeight = 0;
}

void TexPack::SortPieces()
{
	qsort( Pieces, NPieces, sizeof(TexPiece), SortFunc );
}

int TexPack::SortFunc( const void* arg1, const void* arg2 )
{
	TexPiece *p1 = (TexPiece*) arg1;
	TexPiece *p2 = (TexPiece*) arg2;
	int minDim1 = min( p1->width, p1->height );
	int minDim2 = min( p2->width, p2->height );
	int maxDim1 = max( p1->width, p1->height );
	int maxDim2 = max( p2->width, p2->height );

	if ( minDim1 < minDim2 )
		return -1;
	else if ( minDim1 > minDim2 )
		return 1;
	else 
		return 0;
}


