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
	uint32		Flags;
	
	nuGlyphCacheKey() : FontID(0), Char(0), Flags(0) {}
	nuGlyphCacheKey( nuFontID fid, uint32 ch, uint32 flags ) : FontID(fid), Char(ch), Flags(flags) {}

	int	GetHashCode() const
	{
		// Assume we'll have less than 4096 fonts, in which case the hash code have great entropy
		return (FontID << 20) ^ (Flags << 18) ^ Char;
	}
	bool operator==( const nuGlyphCacheKey& b ) const { return FontID == b.FontID && Char == b.Char && Flags == b.Flags; }
};
FHASH_SETUP_POD_GETHASHCODE(nuGlyphCacheKey);

class NUAPI nuGlyphCache
{
public:
			nuGlyphCache();
			~nuGlyphCache();

	void			Clear();
	bool			GetGlyphFromChar( const nuString& facename, int ch, uint32 flags, nuGlyph& glyph );
	bool			GetGlyphFromChar( nuFontID fontID, int ch, uint32 flags, nuGlyph& glyph );
	nuTextureAtlas* HackGetAtlas( int i ) { return Atlas[i]; }

protected:
	pvect<nuTextureAtlas*>				Atlas;
	podvec<nuGlyph>						Glyphs;
	fhashmap<nuGlyphCacheKey, uint>		Table;

	int		RenderGlyph( const nuGlyphCacheKey& key, const nuFont* font );
	void	FilterAndCopyBitmap( const nuFont* font, void* target, int target_stride );
};