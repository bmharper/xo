#pragma once

#include "nuDefs.h"

class nuTextureAtlas;
class nuFont;

enum nuGlyphFlags
{
	nuGlyphFlag_SubPixel_RGB = 1
};

inline bool nuGlyphFlag_IsSubPixel( uint32 flags ) { return !!(flags & nuGlyphFlag_SubPixel_RGB); }

struct nuGlyph
{
	uint	AtlasID;
	uint16	X;
	uint16	Y;
	uint16	Width;
	uint16	Height;
	int16	MetricLeftx256;	// low 8 bits are sub-pixel
	int16	MetricTop;
	int32	MetricLinearHoriAdvancex256;

	// A Null glyph is one that could not be found in the font
	bool IsNull() const { return Width == 0 && MetricLinearHoriAdvancex256 == 0; }
	void SetNull()		{ memset(this, 0, sizeof(*this)); }
};

struct nuGlyphCacheKey
{
	nuFontID	FontID;
	uint32		Char;
	uint8		Size;
	uint8		Flags;
	
	nuGlyphCacheKey() : FontID(0), Char(0), Size(0), Flags(0) {}
	nuGlyphCacheKey( nuFontID fid, uint32 ch, uint8 size, uint32 flags ) : FontID(fid), Char(ch), Size(size), Flags(flags) {}

	int	GetHashCode() const
	{
		// Assume we'll have less than 1024 fonts
		return (FontID << 22) ^ (Flags << 20) ^ (Size << 10) ^ Char;
	}
	bool operator==( const nuGlyphCacheKey& b ) const { return FontID == b.FontID && Char == b.Char && Size == b.Size && Flags == b.Flags; }
};
FHASH_SETUP_POD_GETHASHCODE(nuGlyphCacheKey);

static const int nuGlyphAtlasSize = 512; // 512 x 512 x 8bit = 256k per atlas

/* Maintains a cache of all information (including textures) that is needed to render text.
During a render, the cache is read-only. After rendering, we go in and resolve all the cache
misses. On the next render, those glyphs will get rendered.

If a glyph render fails, then the resulting nuGlyph will have .IsNull() == true.
*/
class NUAPI nuGlyphCache
{
public:
			nuGlyphCache();
			~nuGlyphCache();

	void			Clear();
	//bool			GetGlyphFromChar( const nuString& facename, int ch, uint8 size, uint8 flags, nuGlyph& glyph );
	//bool			GetGlyphFromChar( nuFontID fontID, int ch, uint8 size, uint8 flags, nuGlyph& glyph );
	
	// Returns NULL if the glyph is not in the cache. Even if the glyph pointer is not NULL, you must still check
	// whether it is the logical "null glyph", which is empty. You can detect that with nuGlyph.IsNull().
	const nuGlyph*	GetGlyph( const nuGlyphCacheKey& key ) const;

	uint			RenderGlyph( const nuGlyphCacheKey& key );

	const nuTextureAtlas*	GetAtlas( uint i ) const		{ return Atlasses[i]; }
	nuTextureAtlas*			GetAtlasMutable( uint i )		{ return Atlasses[i]; }

	static const uint					NullGlyphIndex;	// = 0. Our first element in 'Glyphs' is always the null glyph (GCC 4.6 won't allow us to write =0 here)

protected:
	pvect<nuTextureAtlas*>				Atlasses;
	podvec<nuGlyph>						Glyphs;
	fhashmap<nuGlyphCacheKey, uint>		Table;
	nuGlyph								NullGlyph;

	void	Initialize();
	void	FilterAndCopyBitmap( const nuFont* font, void* target, int target_stride );
	void	CopyBitmap( const nuFont* font, void* target, int target_stride );
};