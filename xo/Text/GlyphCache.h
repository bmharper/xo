#pragma once
#include "../Defs.h"

namespace xo {

class TextureAtlas;
class Font;

enum GlyphFlags {
	GlyphFlag_SubPixel_RGB = 1
};

inline bool GlyphFlag_IsSubPixel(uint32_t flags) { return !!(flags & GlyphFlag_SubPixel_RGB); }

struct Glyph {
	uint32_t AtlasID;
	uint32_t FTGlyphIndex;
	uint16_t X;
	uint16_t Y;
	uint16_t Width;
	uint16_t Height;
	int16_t  MetricLeft; // intended for use by SnapSubpixelHorzText
	int16_t  MetricLeftx256;
	int16_t  MetricTop;
	uint16_t MetricWidth;
	int32_t  MetricHoriAdvance; // intended for use by SnapSubpixelHorzText
	float    MetricLinearHoriAdvance;

	// A Null glyph is one that could not be found in the font
	bool IsNull() const { return Width == 0 && MetricLinearHoriAdvance == 0; }
	void SetNull() { memset(this, 0, sizeof(*this)); }
};

struct GlyphCacheKey {
	FontID   FontID;
	uint32_t Char;
	uint8_t  Size;
	uint8_t  Flags;

	GlyphCacheKey() : FontID(0), Char(0), Size(0), Flags(0) {}
	GlyphCacheKey(xo::FontID fid, uint32_t ch, uint8_t size, uint32_t flags) : FontID(fid), Char(ch), Size(size), Flags(flags) {}

	int GetHashCode() const {
		// Assume we'll have less than 1024 fonts
		return (FontID << 22) ^ (Flags << 20) ^ (Size << 10) ^ Char;
	}
	bool operator==(const GlyphCacheKey& b) const { return FontID == b.FontID && Char == b.Char && Size == b.Size && Flags == b.Flags; }
};

static const int GlyphAtlasSize = 512; // 512 x 512 x 8bit = 256k per atlas

/* Maintains a cache of all information (including textures) that is needed to render text.
During a render, the cache is read-only. After rendering, we go in and resolve all the cache
misses. On the next render, those glyphs will get rendered.

If a glyph render fails, then the resulting Glyph will have .IsNull() == true.
*/
class XO_API GlyphCache {
public:
	GlyphCache();
	~GlyphCache();

	void Clear();
	//bool			GetGlyphFromChar( const String& facename, int ch, uint8_t size, uint8_t flags, Glyph& glyph );
	//bool			GetGlyphFromChar( FontID fontID, int ch, uint8_t size, uint8_t flags, Glyph& glyph );

	// Returns NULL if the glyph is not in the cache. Even if the glyph pointer is not NULL, you must still check
	// whether it is the logical "null glyph", which is empty. You can detect that with Glyph.IsNull().
	const Glyph* GetGlyph(const GlyphCacheKey& key) const;

	uint32_t RenderGlyph(const GlyphCacheKey& key);

	const TextureAtlas* GetAtlas(uint32_t i) const { return Atlasses[i]; }
	TextureAtlas*       GetAtlasMutable(uint32_t i) { return Atlasses[i]; }

	static const uint32_t NullGlyphIndex; // = 0. Our first element in 'Glyphs' is always the null glyph (GCC 4.6 won't allow us to write =0 here)

protected:
	cheapvec<TextureAtlas*>             Atlasses;
	cheapvec<Glyph>                     Glyphs;
	ohash::map<GlyphCacheKey, uint32_t> Table;
	Glyph                               NullGlyph;

	void Initialize();
	void FilterAndCopyBitmap(const Font* font, void* target, int target_stride);
	void CopyBitmap(const Font* font, void* target, int target_stride);
};
}

namespace ohash {
inline ohash::hashkey_t gethashcode(const xo::GlyphCacheKey& k) { return (hashkey_t) k.GetHashCode(); }
}
