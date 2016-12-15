#include "pch.h"
#include "GlyphCache.h"
#include "FontStore.h"
#include "Render/TextureAtlas.h"

namespace xo {

// Remember that our horizontal expansion is not only there to clobber the horizontal
// hinting. It also serves to give us more accurate glyph metrics, in particular MetricLeftx256,
// which is also called horiBearingX by Freetype.
// An alternative would be to separately load just the glyph metrics without performing glyph
// rendering. That would give us perfect metrics. This only affects the horizontal spacing of
// glyphs, and I think it's unlikely that one would be able to tell the difference there.
static const uint32_t SubPixelHintKillShift      = 0;
static const uint32_t SubPixelHintKillMultiplier = (1 << SubPixelHintKillShift);

// GCC 4.6 for Android forces us to set the value of this constant in the .cpp file, not in the .h file.
const uint32_t GlyphCache::NullGlyphIndex = 0; // Our first element in 'Glyphs' is always the null glyph

GlyphCache::GlyphCache() {
	Initialize();
}

GlyphCache::~GlyphCache() {
}

void GlyphCache::Clear() {
	for (size_t i = 0; i < Atlasses.size(); i++)
		Atlasses[i]->Free();
	delete_all(Atlasses);
	Glyphs.clear();
	Table.clear();
	Initialize();
}

void GlyphCache::Initialize() {
	NullGlyph.SetNull();
	Glyphs += Glyph();
	Glyphs.back().SetNull();
}

/*
bool GlyphCache::GetGlyphFromChar( const String& facename, int ch, uint8_t size, uint8_t flags, Glyph& glyph )
{
	FontID fontID = FontIDNull;
	const Font* font = Global()->FontStore->GetByFacename( facename );
	if ( font == NULL )
	{
		fontID = Global()->FontStore->InsertByFacename( facename );
		XO_ASSERT( fontID != FontIDNull );
	}
	else
		fontID = font->ID;

	return GetGlyphFromChar( fontID, ch, size, flags, glyph );
}

bool GlyphCache::GetGlyphFromChar( FontID fontID, int ch, uint8_t size, uint8_t flags, Glyph& glyph )
{
	// TODO: This needs a better threading model. Perhaps you render with fonts in read-only mode,
	// and then collect all cache misses, then before next render you fill in all cache misses.

	GlyphCacheKey key( fontID, ch, size, flags );
	uint32_t pos;
	if ( !Table.get( key, pos ) )
	{
		const Font* font = Global()->FontStore->GetByFontID( fontID );
		pos = RenderGlyph( key );
		if ( pos == -1 )
			return false;
	}
	glyph = Glyphs[pos];

	return true;
}
*/

const Glyph* GlyphCache::GetGlyph(const GlyphCacheKey& key) const {
	uint32_t* pos = Table.getp(key);
	if (pos != nullptr)
		return &Glyphs[*pos];
	else
		return nullptr;
}

uint32_t GlyphCache::RenderGlyph(const GlyphCacheKey& key) {
	XOTRACE_FONTS("RenderGlyph %d\n", (int) key.Char);

	XO_ASSERT(key.Size != 0);
	const Font* font = Global()->FontStore->GetByFontID(key.FontID);

	FT_UInt iFTGlyph = FT_Get_Char_Index(font->FTFace, key.Char);

	bool isSubPixel    = GlyphFlag_IsSubPixel(key.Flags);
	bool useFTSubpixel = isSubPixel && Global()->SnapSubpixelHorzText;

	uint32_t pixSize                = key.Size;
	int32_t  combinedHorzMultiplier = 1;
	if (isSubPixel)
		combinedHorzMultiplier = SubPixelHintKillMultiplier * (useFTSubpixel ? 1 : 3);

	FT_Error e = FT_Set_Pixel_Sizes(font->FTFace, combinedHorzMultiplier * pixSize, pixSize);
	XO_ASSERT(e == 0);

	uint32_t ftflags = FT_LOAD_RENDER | FT_LOAD_LINEAR_DESIGN;
	// See FontStore::LoadFontTweaks for details of why we have this "MaxAutoHinterSize"
	//if ( isSubPixel && pixSize <= font->MaxAutoHinterSize )
	//	ftflags |= FT_LOAD_FORCE_AUTOHINT;

	if (useFTSubpixel)
		ftflags |= FT_LOAD_TARGET_LCD;

	e = FT_Load_Glyph(font->FTFace, iFTGlyph, ftflags);
	if (e != 0) {
		XOTRACE("Failed to load glyph for character %d (%d)\n", key.Char, iFTGlyph);
		Table.insert(key, NullGlyphIndex);
		return NullGlyphIndex;
	}

	int  width        = font->FTFace->glyph->bitmap.width;
	int  height       = font->FTFace->glyph->bitmap.rows;
	int  naturalWidth = width;
	int  horzPad      = 0;
	bool isEmpty      = (width | height) == 0;
	if (isSubPixel && !isEmpty) {
		// Note that Freetype's rasterized width is not necessarily divisible by SubPixelHintKillMultiplier.
		// We need to round our resulting width up so that is is divisible by SubPixelHintKillMultiplier,
		// otherwise we miss the last sub-samples. Note that we don't care if our eventual size is not
		// divisible by 3. Our fragment shader clamps all filter taps, so we would just read zero off the
		// right edge, where our sub-pixel samples are undefined.
		naturalWidth = (width + SubPixelHintKillMultiplier - 1) / SubPixelHintKillMultiplier;
		// We need to pad our texture on either side with black lines. Our fragment shader will clamp its UV
		// coordinates to the absolute texel bounds of our glyph. The fragment shader will read over the edges
		// of our absolute texel bounds, and when it does so, it must read pure black.
		horzPad = 1;
	}
	uint16_t        atlasX = 0;
	uint16_t        atlasY = 0;
	TextureAtlas* atlas  = NULL;

	// The sub-pixel shader does its own clamping, but the whole-pixel shader is naive, and
	// each glyph needs 3 pixels of padding around it. That could be fixed so that the whole-pixel
	// shader also clamps itself.
	int glyphPadding = isSubPixel ? 0 : 3;

	for (int pass = 0; true; pass++) {
		if (Atlasses.size() == 0 || pass != 0) {
			TextureAtlas* newAtlas = new TextureAtlas();
			newAtlas->Initialize(GlyphAtlasSize, GlyphAtlasSize, TexFormatGrey8, glyphPadding);
			newAtlas->Zero();
			if (isSubPixel) {
				newAtlas->TexFilterMin = TexFilterNearest;
				newAtlas->TexFilterMax = TexFilterNearest;
			} else if (!Global()->SnapSubpixelHorzText || !Global()->RoundLineHeights) {
				newAtlas->TexFilterMin = TexFilterLinear;
				newAtlas->TexFilterMax = TexFilterLinear;
			}
			Atlasses += newAtlas;
		}
		atlas = Atlasses.back();
		XO_ASSERT(naturalWidth + horzPad * 2 <= GlyphAtlasSize);
		if (atlas->Alloc(naturalWidth + horzPad * 2, height, atlasX, atlasY))
			break;
	}

	if (isSubPixel)
		FilterAndCopyBitmap(font, atlas->TexDataAt(atlasX, atlasY), atlas->GetStride());
	else
		CopyBitmap(font, atlas->TexDataAt(atlasX, atlasY), atlas->GetStride());

	if (key.Char == '1')
		int abc = 123;

	Glyph g;
	g.FTGlyphIndex            = iFTGlyph;
	g.Width                   = isEmpty ? 0 : naturalWidth + horzPad * 2;
	g.Height                  = height;
	g.X                       = atlasX;
	g.Y                       = atlasY;
	g.AtlasID                 = (uint32_t) Atlasses.find(atlas);
	g.MetricLeft              = font->FTFace->glyph->bitmap_left / combinedHorzMultiplier;
	g.MetricLeftx256          = font->FTFace->glyph->bitmap_left * 256 / combinedHorzMultiplier;
	g.MetricTop               = font->FTFace->glyph->bitmap_top;
	g.MetricHoriAdvance       = font->FTFace->glyph->advance.x / (64 * combinedHorzMultiplier);
	g.MetricLinearHoriAdvance = (font->FTFace->glyph->linearHoriAdvance * (int32_t) pixSize) / (float) font->FTFace->units_per_EM;
	Table.insert(key, (uint32_t) Glyphs.size());
	Glyphs += g;
	return (uint32_t)(Glyphs.size() - 1);
}

void GlyphCache::FilterAndCopyBitmap(const Font* font, void* target, int target_stride) {
	uint32_t width  = font->FTFace->glyph->bitmap.width;
	uint32_t height = font->FTFace->glyph->bitmap.rows;

	float gamma = Global()->SubPixelTextGamma;

	for (int py = 0; py < (int) height; py++) {
		uint8_t* src = (uint8_t*) font->FTFace->glyph->bitmap.buffer + py * font->FTFace->glyph->bitmap.pitch;
		uint8_t* dst = (uint8_t*) target + py * target_stride;
		// single padding sample on the left side
		*dst++ = 0;
		// first N - 1 texels
		uint32_t px = 0;
		for (; px < width - SubPixelHintKillMultiplier; px += SubPixelHintKillMultiplier) {
			uint32_t accum = 0;
			for (int i = 0; i < SubPixelHintKillMultiplier; i++, src++)
				accum += *src;
			accum = accum >> SubPixelHintKillShift;
			if (gamma != 1)
				accum = (uint32_t)(pow(accum / 255.0f, gamma) * 255.0f);
			*dst++ = accum;
		}
		// last texel, which may not have the full 'SubPixelHintKillMultiplier' number of samples
		uint32_t accum = 0;
		for (; px < width; px++, src++)
			accum += *src;
		accum = accum >> SubPixelHintKillShift;
		if (gamma != 1)
			accum = (uint32_t)(pow(accum / 255.0f, gamma) * 255.0f);
		*dst++ = accum;
		// single padding sample on the right side
		*dst++ = 0;
	}
}

void GlyphCache::CopyBitmap(const Font* font, void* target, int target_stride) {
	uint32_t width  = font->FTFace->glyph->bitmap.width;
	uint32_t height = font->FTFace->glyph->bitmap.rows;

	float gamma = Global()->WholePixelTextGamma;

	for (int py = 0; py < (int) height; py++) {
		uint8_t* src = (uint8_t*) font->FTFace->glyph->bitmap.buffer + py * font->FTFace->glyph->bitmap.pitch;
		uint8_t* dst = (uint8_t*) target + py * target_stride;
		if (gamma == 1)
			memcpy(dst, src, width);
		else {
			for (uint32_t x = 0; x < width; x++) {
				float v = src[x] / 255.0f;
				v       = pow(v, gamma) * 255.0f;
				v       = std::min(v, 255.0f);
				v       = std::max(v, 0.0f);
				dst[x]  = (uint8_t) v;
			}
		}
	}
}
}
