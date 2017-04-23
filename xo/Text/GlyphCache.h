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
	int16_t  MetricLeft; // intended for use by SnapHorzText
	int16_t  MetricLeftx256;
	int16_t  MetricTop;
	uint16_t MetricWidth;
	int32_t  MetricHoriAdvance; // intended for use by SnapHorzText
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

NOTE: This was initially created for exclusive use by the main xo renderer, which puts text
onto the screen, via the GPU. Subsequently, I needed something to cache glyphs when rendering
text to Canvas, so I decided to reuse this cache for that purpose. The design of the main renderer
is that while rendering, it treats the glyph cache as immutable. This means that if it's
rendering a scene, and a glyph bitmap is not ready, then it simply proceeds, and skips the
rendering of that glyph. It does however record the fact that it needs that glyph. At the
end of the render, it goes back and calls RenderGlyph() for every glyph that it missed.
This design allows the common case (all glyphs are cached) to be fast, because there is
zero locking overhead during rendering.
HOWEVER, the UI runs on a different thread to the renderer, and now that we want to be
able to use this cache for Canvas rendering (which happens on the UI thread), we need
to make the cache safe to use from both the renderer and the UI. As a very simple
first approach, we just add a giant lock over the whole thing. The renderer takes that lock
at a massively coarse scale, and the Canvas renderer takes it at very fine scales (every
time it draws a glyph). Since Canvas is the 2nd class citizen here, we're fine with this.

If a glyph render fails, then the resulting Glyph will have .IsNull() == true.
*/
class XO_API GlyphCache {
public:
	static const uint32_t NullGlyphIndex; // = 0. Our first element in 'Glyphs' is always the null glyph (GCC 4.6 won't allow us to write =0 here)

	// Giant lock on the entire cache. Guards access to all data in here.
	std::mutex Lock;

	GlyphCache();
	~GlyphCache();

	void Clear();

	// Returns NULL if the glyph is not in the cache. Even if the glyph pointer is not NULL, you must still check
	// whether it is the logical "null glyph", which is empty. You can detect that with Glyph.IsNull().
	const Glyph* GetGlyph(const GlyphCacheKey& key) const;

	const Glyph* GetOrRenderGlyph(const GlyphCacheKey& key);

	uint32_t RenderGlyph(const GlyphCacheKey& key);

	const TextureAtlas* GetAtlas(uint32_t i) const { return Atlasses[i]; }
	TextureAtlas*       GetAtlasMutable(uint32_t i) { return Atlasses[i]; }

protected:
	cheapvec<TextureAtlas*>             Atlasses;
	cheapvec<Glyph>                     Glyphs;
	ohash::map<GlyphCacheKey, uint32_t> Table;
	Glyph                               NullGlyph;

	void Initialize();
	void FilterAndCopyBitmap(const Font* font, void* target, int target_stride);
	void CopyBitmap(const Font* font, void* target, int target_stride);
};
} // namespace xo

namespace ohash {
inline ohash::hashkey_t gethashcode(const xo::GlyphCacheKey& k) { return (hashkey_t) k.GetHashCode(); }
} // namespace ohash
