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
	uint	TextureID;
	uint16	X;
	uint16	Y;
	uint16	Width;
	uint16	Height;
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

class NUAPI nuGlyphCache
{
public:
	static const int AtlasSize = 256; // 256 x 256 x 8bit = 64k per atlas

			nuGlyphCache();
			~nuGlyphCache();

	void			Clear();
	bool			GetGlyphFromChar( const nuString& facename, int ch, uint8 size, uint8 flags, nuGlyph& glyph );
	bool			GetGlyphFromChar( nuFontID fontID, int ch, uint8 size, uint8 flags, nuGlyph& glyph );
	nuTextureAtlas* HackGetAtlas( int i ) { return Atlas[i]; }

protected:
	pvect<nuTextureAtlas*>				Atlas;
	podvec<nuGlyph>						Glyphs;
	fhashmap<nuGlyphCacheKey, uint>		Table;

	int		RenderGlyph( const nuGlyphCacheKey& key, const nuFont* font );
	void	FilterAndCopyBitmap( const nuFont* font, void* target, int target_stride );
	void	CopyBitmap( const nuFont* font, void* target, int target_stride );
};