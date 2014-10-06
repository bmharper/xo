#include "pch.h"
#include "xoGlyphCache.h"
#include "xoFontStore.h"
#include "Render/xoTextureAtlas.h"

// Remember that our horizontal expansion is not only there to clobber the horizontal
// hinting. It also serves to give us more accurate glyph metrics, in particular MetricLeftx256,
// which is also called horiBearingX by Freetype.
// An alternative would be to separately load just the glyph metrics without performing glyph
// rendering. That would give us perfect metrics. This only affects the horizontal spacing of
// glyphs, and I think it's unlikely that one would be able to tell the difference there.
static const uint32 xoSubPixelHintKillShift = 0;
static const uint32 xoSubPixelHintKillMultiplier = (1 << xoSubPixelHintKillShift);

// GCC 4.6 for Android forces us to set the value of this constant in the .cpp file, not in the .h file.
const uint xoGlyphCache::NullGlyphIndex = 0;	// Our first element in 'Glyphs' is always the null glyph

xoGlyphCache::xoGlyphCache()
{
	Initialize();
}

xoGlyphCache::~xoGlyphCache()
{
}

void xoGlyphCache::Clear()
{
	for ( intp i = 0; i < Atlasses.size(); i++ )
		Atlasses[i]->Free();
	delete_all(Atlasses);
	Glyphs.clear();
	Table.clear();
	Initialize();
}

void xoGlyphCache::Initialize()
{
	NullGlyph.SetNull();
	Glyphs += xoGlyph();
	Glyphs.back().SetNull();
}

/*
bool xoGlyphCache::GetGlyphFromChar( const xoString& facename, int ch, uint8 size, uint8 flags, xoGlyph& glyph )
{
	xoFontID fontID = xoFontIDNull;
	const xoFont* font = xoGlobal()->FontStore->GetByFacename( facename );
	if ( font == NULL )
	{
		fontID = xoGlobal()->FontStore->InsertByFacename( facename );
		XOASSERT( fontID != xoFontIDNull );
	}
	else
		fontID = font->ID;

	return GetGlyphFromChar( fontID, ch, size, flags, glyph );
}

bool xoGlyphCache::GetGlyphFromChar( xoFontID fontID, int ch, uint8 size, uint8 flags, xoGlyph& glyph )
{
	// TODO: This needs a better threading model. Perhaps you render with fonts in read-only mode,
	// and then collect all cache misses, then before next render you fill in all cache misses.

	xoGlyphCacheKey key( fontID, ch, size, flags );
	uint pos;
	if ( !Table.get( key, pos ) )
	{
		const xoFont* font = xoGlobal()->FontStore->GetByFontID( fontID );
		pos = RenderGlyph( key );
		if ( pos == -1 )
			return false;
	}
	glyph = Glyphs[pos];

	return true;
}
*/

const xoGlyph* xoGlyphCache::GetGlyph( const xoGlyphCacheKey& key ) const
{
	uint pos;
	if ( Table.get( key, pos ) )
		return &Glyphs[pos];
	else
		return NULL;
}

uint xoGlyphCache::RenderGlyph( const xoGlyphCacheKey& key )
{
	XOASSERT( key.Size != 0 );
	const xoFont* font = xoGlobal()->FontStore->GetByFontID( key.FontID );

	FT_UInt iFTGlyph = FT_Get_Char_Index( font->FTFace, key.Char );

	bool isSubPixel = xoGlyphFlag_IsSubPixel(key.Flags);
	bool useFTSubpixel = isSubPixel && xoGlobal()->SnapSubpixelHorzText;

	uint32 pixSize = key.Size;
	int32 combinedHorzMultiplier = 1;
	if ( isSubPixel )
		combinedHorzMultiplier = xoSubPixelHintKillMultiplier * (useFTSubpixel ? 1 : 3);

	FT_Error e = FT_Set_Pixel_Sizes( font->FTFace, combinedHorzMultiplier * pixSize, pixSize );
	XOASSERT( e == 0 );

	uint ftflags = FT_LOAD_RENDER | FT_LOAD_LINEAR_DESIGN;
	// See xoFontStore::LoadFontTweaks for details of why we have this "MaxAutoHinterSize"
	//if ( isSubPixel && pixSize <= font->MaxAutoHinterSize )
	//	ftflags |= FT_LOAD_FORCE_AUTOHINT;
	
	if ( useFTSubpixel )
		ftflags |= FT_LOAD_TARGET_LCD;

	e = FT_Load_Glyph( font->FTFace, iFTGlyph, ftflags );
	if ( e != 0 )
	{
		XOTRACE( "Failed to load glyph for character %d (%d)\n", key.Char, iFTGlyph );
		Table.insert( key, NullGlyphIndex );
		return NullGlyphIndex;
	}

	int width = font->FTFace->glyph->bitmap.width;
	int height = font->FTFace->glyph->bitmap.rows;
	int naturalWidth = width;
	int horzPad = 0;
	bool isEmpty = (width | height) == 0;
	if ( isSubPixel && !isEmpty )
	{
		// Note that Freetype's rasterized width is not necessarily divisible by xoSubPixelHintKillMultiplier.
		// We need to round our resulting width up so that is is divisible by xoSubPixelHintKillMultiplier,
		// otherwise we miss the last sub-samples. Note that we don't care if our eventual size is not
		// divisible by 3. Our fragment shader clamps all filter taps, so we would just read zero off the
		// right edge, where our sub-pixel samples are undefined.
		naturalWidth = (width + xoSubPixelHintKillMultiplier - 1) / xoSubPixelHintKillMultiplier;
		// We need to pad our texture on either side with black lines. Our fragment shader will clamp its UV
		// coordinates to the absolute texel bounds of our glyph. The fragment shader will read over the edges
		// of our absolute texel bounds, and when it does so, it must read pure black.
		horzPad = 1;
	}
	uint16 atlasX = 0;
	uint16 atlasY = 0;
	xoTextureAtlas* atlas = NULL;

	// Sub-pixel shader does its own clamping, but the whole pixel shader is naive, and 
	// each glyph needs 3 pixels of padding around it.
	int glyphPadding = isSubPixel ? 0 : 3;

	for ( int pass = 0; true; pass++ )
	{
		if ( Atlasses.size() == 0 || pass != 0 )
		{
			xoTextureAtlas* newAtlas = new xoTextureAtlas();
			newAtlas->Initialize( xoGlyphAtlasSize, xoGlyphAtlasSize, xoTexFormatGrey8, glyphPadding );
			newAtlas->Zero();
			if ( isSubPixel )
			{
				newAtlas->TexFilterMin = xoTexFilterNearest;
				newAtlas->TexFilterMax = xoTexFilterNearest;
			}
			else if ( !xoGlobal()->SnapSubpixelHorzText || !xoGlobal()->RoundLineHeights )
			{
				newAtlas->TexFilterMin = xoTexFilterLinear;
				newAtlas->TexFilterMax = xoTexFilterLinear;
			}
			Atlasses += newAtlas;
		}
		atlas = Atlasses.back();
		XOASSERT( naturalWidth + horzPad * 2 <= xoGlyphAtlasSize );
		if ( atlas->Alloc( naturalWidth + horzPad * 2, height, atlasX, atlasY ) )
			break;
	}

	if ( isSubPixel )
		FilterAndCopyBitmap( font, atlas->TexDataAt(atlasX, atlasY), atlas->GetStride() );
	else
		CopyBitmap( font, atlas->TexDataAt(atlasX, atlasY), atlas->GetStride() );

	if ( key.Char == '1' )
		int abc = 123;

	xoGlyph g;
	g.FTGlyphIndex = iFTGlyph;
	g.Width = isEmpty ? 0 : naturalWidth + horzPad * 2;
	g.Height = height;
	g.X = atlasX;
	g.Y = atlasY;
	g.AtlasID = (uint) Atlasses.find( atlas );
	g.MetricLeft = font->FTFace->glyph->bitmap_left / combinedHorzMultiplier;
	g.MetricLeftx256 = font->FTFace->glyph->bitmap_left * 256 / combinedHorzMultiplier;
	g.MetricTop = font->FTFace->glyph->bitmap_top;
	g.MetricHoriAdvance = font->FTFace->glyph->advance.x / (64 * combinedHorzMultiplier);
	g.MetricLinearHoriAdvance = (font->FTFace->glyph->linearHoriAdvance * (int32) pixSize) / (float) font->FTFace->units_per_EM;
	Table.insert( key, (uint) Glyphs.size() );
	Glyphs += g;
	return (uint) (Glyphs.size() - 1);
}

void xoGlyphCache::FilterAndCopyBitmap( const xoFont* font, void* target, int target_stride )
{
	uint32 width = font->FTFace->glyph->bitmap.width;
	uint32 height = font->FTFace->glyph->bitmap.rows;

	float gamma = xoGlobal()->SubPixelTextGamma;

	for ( int py = 0; py < (int) height; py++ )
	{
		uint8* src = (uint8*) font->FTFace->glyph->bitmap.buffer + py * font->FTFace->glyph->bitmap.pitch;
		uint8* dst = (uint8*) target + py * target_stride;
		// single padding sample on the left side
		*dst++ = 0;
		// first N - 1 texels
		uint32 px = 0;
		for ( ; px < width - xoSubPixelHintKillMultiplier; px += xoSubPixelHintKillMultiplier ) 
		{
			uint32 accum = 0;
			for ( int i = 0; i < xoSubPixelHintKillMultiplier; i++, src++ )
				accum += *src;
			accum = accum >> xoSubPixelHintKillShift;
			if ( gamma != 1 )
				accum = (uint32) (pow(accum / 255.0f, gamma) * 255.0f);
			*dst++ = accum;
		}
		// last texel, which may not have the full 'xoSubPixelHintKillMultiplier' number of samples
		uint32 accum = 0;
		for ( ; px < width; px++, src++ )
			accum += *src;
		accum = accum >> xoSubPixelHintKillShift;
		if ( gamma != 1 )
			accum = (uint32) (pow(accum / 255.0f, gamma) * 255.0f);
		*dst++ = accum;
		// single padding sample on the right side
		*dst++ = 0;
	}
}

void xoGlyphCache::CopyBitmap( const xoFont* font, void* target, int target_stride )
{
	uint32 width = font->FTFace->glyph->bitmap.width;
	uint32 height = font->FTFace->glyph->bitmap.rows;

	float gamma = xoGlobal()->WholePixelTextGamma;

	for ( int py = 0; py < (int) height; py++ )
	{
		uint8* src = (uint8*) font->FTFace->glyph->bitmap.buffer + py * font->FTFace->glyph->bitmap.pitch;
		uint8* dst = (uint8*) target + py * target_stride;
		if ( gamma == 1 )
			memcpy( dst, src, width );
		else
		{
			for ( uint32 x = 0; x < width; x++ )
			{
				float v = src[x] / 255.0f;
				v = pow(v, gamma) * 255.0f;
				v = std::min(v, 255.0f);
				v = std::max(v, 0.0f);
				dst[x] = (uint8) v;
			}
		}
	}
}
