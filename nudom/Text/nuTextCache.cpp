#include "pch.h"
#include "nuTextCache.h"
#include "nuFontStore.h"
#include "Render/nuTextureAtlas.h"

static const uint32 nuSubPixelHintKillShift = 4;
static const uint32 nuSubPixelHintKillMultiplier = (1 << nuSubPixelHintKillShift);

nuGlyphCache::nuGlyphCache()
{
}

nuGlyphCache::~nuGlyphCache()
{
}

void nuGlyphCache::Clear()
{
	for ( intp i = 0; i < Atlas.size(); i++ )
		Atlas[i]->Free();
	delete_all(Atlas);
	Glyphs.clear();
	Table.clear();
}

bool nuGlyphCache::GetGlyphFromChar( const nuString& facename, int ch, uint32 flags, nuGlyph& glyph )
{
	nuFontID fontID = nuFontIDNull;
	const nuFont* font = nuGlobal()->FontStore->GetByFacename( facename );
	if ( font == NULL )
	{
		fontID = nuGlobal()->FontStore->InsertByFacename( facename );
		NUASSERT( fontID != nuFontIDNull );
	}
	else
		fontID = font->ID;

	return GetGlyphFromChar( fontID, ch, flags, glyph );
}

bool nuGlyphCache::GetGlyphFromChar( nuFontID fontID, int ch, uint32 flags, nuGlyph& glyph )
{
	// TODO: This needs a better threading model. Perhaps you render with fonts in read-only mode,
	// and then collect all cache misses, then before next render you fill in all cache misses.

	nuGlyphCacheKey key( fontID, ch, flags );
	uint pos;
	if ( !Table.get( key, pos ) )
	{
		const nuFont* font = nuGlobal()->FontStore->GetByFontID( fontID );
		pos = RenderGlyph( key, font );
		if ( pos == -1 )
			return false;
	}
	glyph = Glyphs[pos];

	return true;
}

int nuGlyphCache::RenderGlyph( const nuGlyphCacheKey& key, const nuFont* font )
{
	FT_UInt iFTGlyph = FT_Get_Char_Index( font->FTFace, key.Char );

	bool isSubPixel = nuGlyphFlag_IsSubPixel(key.Flags);

	uint32 pixSize = 15;

	FT_Error e = FT_Set_Pixel_Sizes( font->FTFace, nuSubPixelHintKillMultiplier * 3 * pixSize, pixSize );
	NUASSERT( e == 0 );

	uint ftflags = FT_LOAD_RENDER;
	if ( isSubPixel )
		ftflags |= FT_LOAD_FORCE_AUTOHINT;

	e = FT_Load_Glyph( font->FTFace, iFTGlyph, ftflags );
	if ( e != 0 )
	{
		NUTRACE( "Failed to load glyph for character %d (%d)\n", key.Char, iFTGlyph );
		return -1;
	}

	int width = font->FTFace->glyph->bitmap.width;
	int height = font->FTFace->glyph->bitmap.rows;
	int naturalWidth = width;
	int horzPad = 0;
	if ( isSubPixel )
	{
		// Note that Freetype's rasterized width is not necessarily divisible by nuSubPixelHintKillMultiplier.
		// We need to round our resulting width up so that is is divisible by nuSubPixelHintKillMultiplier,
		// otherwise we miss the last sub-samples. Note that we don't care if our eventual size is not
		// divisible by 3. Our fragment shader clamps all filter taps, so we would just read zero off the
		// right edge, where our sub-pixel samples are undefined.
		naturalWidth = (width + nuSubPixelHintKillMultiplier - 1) / nuSubPixelHintKillMultiplier;
		// We need to pad our texture on either side with black lines. Our fragment shader will clamp its UV
		// coordinates to the absolute texel bounds of our glyph. The fragment shader will read over the edges
		// of our absolute texel bounds, and when it does so, it must read pure black.
		horzPad = 1;
	}
	uint16 atlasX = 0;
	uint16 atlasY = 0;
	nuTextureAtlas* atlas = NULL;

	for ( int pass = 0; true; pass++ )
	{
		if ( Atlas.size() == 0 || pass != 0 )
		{
			nuTextureAtlas* newAtlas = new nuTextureAtlas();
			newAtlas->Initialize( 64, 64, 1 );
			Atlas += newAtlas;
		}
		atlas = Atlas.back();
		if ( atlas->Alloc( naturalWidth + horzPad * 2, height, atlasX, atlasY ) )
			break;
	}

	// non-sub-pixel would use a simpler CopyBitmap
	NUASSERT( isSubPixel );
	FilterAndCopyBitmap( font, atlas->DataAt(atlasX, atlasY), atlas->GetStride() );

	nuGlyph g;
	g.Width = naturalWidth + horzPad * 2;
	g.Height = height;
	g.X = atlasX;
	g.Y = atlasY;
	g.TextureID = 0; // TODO
	Table.insert( key, (uint) Glyphs.size() );
	Glyphs += g;
	return (int) (Glyphs.size() - 1);
}

void nuGlyphCache::FilterAndCopyBitmap( const nuFont* font, void* target, int target_stride )
{
	uint8* src = font->FTFace->glyph->bitmap.buffer;
	uint8* dst = (uint8*) target;
	uint32 width = font->FTFace->glyph->bitmap.width;
	uint32 height = font->FTFace->glyph->bitmap.rows;

	for ( int py = 0; py < (int) height; py++ )
	{
		uint8* src = (uint8*) font->FTFace->glyph->bitmap.buffer + py * font->FTFace->glyph->bitmap.pitch;
		uint8* dst = (uint8*) target + py * target_stride;
		// single padding sample on the left side
		*dst++ = 0;
		// first N - 1 texels
		uint32 px = 0;
		for ( ; px < width - nuSubPixelHintKillMultiplier; px += nuSubPixelHintKillMultiplier ) 
		{
			uint32 accum = 0;
			for ( int i = 0; i < nuSubPixelHintKillMultiplier; i++, src++ )
				accum += *src;
			accum = accum >> nuSubPixelHintKillShift;
			*dst++ = accum;
		}
		// last texel, which may not have the full 'nuSubPixelHintKillMultiplier' number of samples
		uint32 accum = 0;
		for ( ; px < width; px++, src++ )
			accum += *src;
		accum = accum >> nuSubPixelHintKillShift;
		*dst++ = accum;
		// single padding sample on the right side
		*dst++ = 0;
	}
}
