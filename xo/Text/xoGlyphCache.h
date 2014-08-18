#pragma once

#include "xoDefs.h"

class xoTextureAtlas;
class xoFont;

enum xoGlyphFlags
{
	xoGlyphFlag_SubPixel_RGB = 1
};

inline bool xoGlyphFlag_IsSubPixel( uint32 flags ) { return !!(flags & xoGlyphFlag_SubPixel_RGB); }

struct xoGlyph
{
	uint		AtlasID;
	uint		FTGlyphIndex;
	uint16		X;
	uint16		Y;
	uint16		Width;
	uint16		Height;
	int16		MetricLeft;			// intended for use by SnapSubpixelHorzText
	int16		MetricLeftx256;
	int16		MetricTop;
	int32		MetricHoriAdvance;	// intended for use by SnapSubpixelHorzText
	float		MetricLinearHoriAdvance;

	// A Null glyph is one that could not be found in the font
	bool IsNull() const { return Width == 0 && MetricLinearHoriAdvance == 0; }
	void SetNull()		{ memset(this, 0, sizeof(*this)); }
};

struct xoGlyphCacheKey
{
	xoFontID	FontID;
	uint32		Char;
	uint8		Size;
	uint8		Flags;
	
	xoGlyphCacheKey() : FontID(0), Char(0), Size(0), Flags(0) {}
	xoGlyphCacheKey( xoFontID fid, uint32 ch, uint8 size, uint32 flags ) : FontID(fid), Char(ch), Size(size), Flags(flags) {}

	int	GetHashCode() const
	{
		// Assume we'll have less than 1024 fonts
		return (FontID << 22) ^ (Flags << 20) ^ (Size << 10) ^ Char;
	}
	bool operator==( const xoGlyphCacheKey& b ) const { return FontID == b.FontID && Char == b.Char && Size == b.Size && Flags == b.Flags; }
};
FHASH_SETUP_POD_GETHASHCODE(xoGlyphCacheKey);

static const int xoGlyphAtlasSize = 512; // 512 x 512 x 8bit = 256k per atlas

/* Maintains a cache of all information (including textures) that is needed to render text.
During a render, the cache is read-only. After rendering, we go in and resolve all the cache
misses. On the next render, those glyphs will get rendered.

If a glyph render fails, then the resulting xoGlyph will have .IsNull() == true.
*/
class XOAPI xoGlyphCache
{
public:
			xoGlyphCache();
			~xoGlyphCache();

	void			Clear();
	//bool			GetGlyphFromChar( const xoString& facename, int ch, uint8 size, uint8 flags, xoGlyph& glyph );
	//bool			GetGlyphFromChar( xoFontID fontID, int ch, uint8 size, uint8 flags, xoGlyph& glyph );
	
	// Returns NULL if the glyph is not in the cache. Even if the glyph pointer is not NULL, you must still check
	// whether it is the logical "null glyph", which is empty. You can detect that with xoGlyph.IsNull().
	const xoGlyph*		GetGlyph( const xoGlyphCacheKey& key ) const;

	uint				RenderGlyph( const xoGlyphCacheKey& key );

	const xoTextureAtlas*	GetAtlas( uint i ) const		{ return Atlasses[i]; }
	xoTextureAtlas*			GetAtlasMutable( uint i )		{ return Atlasses[i]; }

	static const uint					NullGlyphIndex;	// = 0. Our first element in 'Glyphs' is always the null glyph (GCC 4.6 won't allow us to write =0 here)

protected:
	pvect<xoTextureAtlas*>				Atlasses;
	podvec<xoGlyph>						Glyphs;
	fhashmap<xoGlyphCacheKey, uint>		Table;
	xoGlyph								NullGlyph;

	void	Initialize();
	void	FilterAndCopyBitmap( const xoFont* font, void* target, int target_stride );
	void	CopyBitmap( const xoFont* font, void* target, int target_stride );
};