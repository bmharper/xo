#include "pch.h"
#include "nuDoc.h"
#include "nuRenderer.h"
#include "nuRenderGL.h"
#include "nuRenderDomEl.h"
#include "nuTextureAtlas.h"
#include "Text/nuGlyphCache.h"
#include "../Image/nuImage.h"

nuRenderResult nuRenderer::Render( nuImageStore* images, nuStringTable* strings, nuRenderBase* driver, nuRenderDomNode* root, int width, int height )
{
	Driver = driver;
	Images = images;
	Strings = strings;

	Driver->PreRender();

	// This phase is probably worth parallelizing
	RenderEl( nuPoint(0,0), root );
	// After RenderNode we are serial again.

	Driver->PostRenderCleanup();

	bool needGlyphs = GlyphsNeeded.size() != 0;
	RenderGlyphsNeeded();

	return needGlyphs ? nuRenderResultNeedMore : nuRenderResultIdle;
}

void nuRenderer::RenderEl( nuPoint base, nuRenderDomEl* el )
{
	if ( el->Tag == nuTagText )
		RenderText( base, static_cast<nuRenderDomText*>(el) );
	else
	{
		nuRenderDomNode* node = static_cast<nuRenderDomNode*>(el);
		RenderNode( base, node );
		nuPoint newBase = base + nuPoint( node->Pos.Left, node->Pos.Top );
		for ( intp i = 0; i < node->Children.size(); i++ )
			RenderEl( newBase, node->Children[i] );
	}
}

void nuRenderer::RenderNode( nuPoint base, nuRenderDomNode* node )
{
	// always shade rectangles well
	const bool alwaysGoodRects = true;

	nuStyleRender* style = &node->Style;
	nuBox pos = node->Pos;
	pos.Offset( base );
	float bottom = nuPosToReal( pos.Bottom );
	float top = nuPosToReal( pos.Top );
	float left = nuPosToReal( pos.Left );
	float right = nuPosToReal( pos.Right );

	float radius = style->BorderRadius;
	bool useRectShader = alwaysGoodRects || radius != 0;

	float width = right - left;
	float height = bottom - top;
	float mindim = min(width, height);
	mindim = max(mindim, 0.0f);
	radius = min(radius, mindim / 2);

	if ( mindim <= 0 )
		return;

	float padU = 0;
	float padV = 0;
	float pad = useRectShader ? 1.0f : 0.0f;
	if ( pad != 0 )
	{
		padU = pad / width;
		padV = pad / height;
	}

	nuVx_PTC corners[4];
	corners[0].Pos = NUVEC3(left - pad, top - pad, 0);
	corners[1].Pos = NUVEC3(left - pad, bottom + pad, 0);
	corners[2].Pos = NUVEC3(right + pad, bottom + pad, 0);
	corners[3].Pos = NUVEC3(right + pad, top - pad, 0);

	corners[0].UV = NUVEC2(-padU, -padV);
	corners[1].UV = NUVEC2(-padU, 1 + padV);
	corners[2].UV = NUVEC2(1 + padU, 1 + padV);
	corners[3].UV = NUVEC2(1 + padU, -padV);

	//auto bg = style.Get( nuCatBackground );
	//auto bgImage = style.Get( nuCatBackgroundImage );
	nuColor bg = style->BackgroundColor;
	const char* bgImage = Strings->GetStr( style->BackgroundImageID );
	if ( bg.a != 0 )
	{
		for ( int i = 0; i < 4; i++ )
			corners[i].Color = bg.GetRGBA();

		if ( useRectShader )
		{
			Driver->ActivateShader( nuShaderRect );
			Driver->ShaderPerObject.Box = nuVec4f( left, top, right, bottom );
			Driver->ShaderPerObject.Radius = radius + 0.5f; // see the shader for an explanation of this 0.5
			Driver->DrawQuad( corners );
		}
		else
		{
			if ( bgImage[0] != 0 )
			{
				Driver->ActivateShader( nuShaderFillTex );
				if ( !LoadTexture( Images->GetOrNull( bgImage ), 0 ) )
					return;
				Driver->DrawQuad( corners );
			}
			else
			{
				Driver->ActivateShader( nuShaderFill );
				Driver->DrawQuad( corners );
			}
		}
	}
}

void nuRenderer::RenderText( nuPoint base, nuRenderDomText* node )
{
	bool subPixelGlyphs = node->Flags & nuRenderDomText::FlagSubPixelGlyphs;
	for ( intp i = 0; i < node->Text.size(); i++ )
	{
		if ( subPixelGlyphs )
			RenderTextChar_SubPixel( base, node, node->Text[i] );
		else
			RenderTextChar_WholePixel( base, node, node->Text[i] );
	}
}

void nuRenderer::RenderTextChar_SubPixel( nuPoint base, nuRenderDomText* node, const nuRenderCharEl& txtEl )
{
	nuGlyphCacheKey glyphKey( node->FontID, txtEl.Char, node->FontSizePx, nuGlyphFlag_SubPixel_RGB );
	const nuGlyph* glyph = nuGlobal()->GlyphCache->GetGlyph( glyphKey );
	if ( !glyph )
	{
		GlyphsNeeded.insert( glyphKey );
		return;
	}

	nuTextureAtlas* atlas = nuGlobal()->GlyphCache->GetAtlasMutable( glyph->AtlasID );
	float atlasScaleX = 1.0f / atlas->GetWidth();
	float atlasScaleY = 1.0f / atlas->GetHeight();

	float top = nuPosToReal( nuPosRound(base.Y + txtEl.Y) );
	float left = nuPosToReal( base.X + txtEl.X );

	// Our glyph has a single column on the left and right side, so that our clamped texture
	// reads will pickup a zero when reading off beyond the edge of the glyph
	int horzPad = 1;
	int nonPaddedWidth = glyph->Width - horzPad * 2;

	// Our texture (minus the padding) can be missing 1 or 2 columns on its right side,
	// but our proper width in pixels DOES include those 1 or 2 missing columns (they are simply black).
	// We clamp our texture reads to our freetype-rasterized limits, but the vertex coordinates we emit
	// now to the GPU includes those 1 or 2 missing columns.
	// This issue exists because freetype's bitmaps will never include a column of pure black on the left
	// or right edge of the glyph.
	int roundedWidth = (nonPaddedWidth + 2) / 3;

	// We don't need to round our horizontal position to any fixed grid - the interpolation
	// is correct regardless, and since we clobber all horizontal hinting, horizontal
	// grid-fitting should have little effect.
	//left = (float) floor(left * 3 + 0.5) / 3.0f;
	//top = (float) floor(top + 0.5); -- vertical rounding has been moved to fixed point layout
	if ( nuGlobal()->SnapSubpixelHorzText )
		left = floor(left + 0.5f);

	float right = left + roundedWidth;
	float bottom = top + glyph->Height;

	// We have to 'overdraw' on the left and right by 1 pixel, to ensure that we filter over the edges.
	int overdraw = 1;
	left -= overdraw;
	right += overdraw;

	nuVx_PTCV4 corners[4];
	corners[0].Pos = NUVEC3(left, top, 0);
	corners[1].Pos = NUVEC3(left, bottom, 0);
	corners[2].Pos = NUVEC3(right, bottom, 0);
	corners[3].Pos = NUVEC3(right, top, 0);

	float u0 = (glyph->X - overdraw * 3 + horzPad) * atlasScaleX;
	float v0 = glyph->Y * atlasScaleY;
	float u1 = (glyph->X + (roundedWidth + overdraw) * 3 - horzPad) * atlasScaleX;
	float v1 = (glyph->Y + glyph->Height) * atlasScaleY;

	corners[0].UV = NUVEC2(u0, v0);
	corners[1].UV = NUVEC2(u0, v1);
	corners[2].UV = NUVEC2(u1, v1);
	corners[3].UV = NUVEC2(u1, v0);

	// Obviously our clamping is not affected by overdraw. It remains our absolute texel limits.
	nuVec4f clamp;
	clamp.x = (glyph->X + 0.5f) * atlasScaleX;
	clamp.y = (glyph->Y + 0.5f) * atlasScaleY;
	clamp.z = (glyph->X + glyph->Width - 0.5f) * atlasScaleX;
	clamp.w = (glyph->Y + glyph->Height - 0.5f) * atlasScaleY;

	uint32 color = node->Color.GetRGBA();

	for ( int i = 0; i < 4; i++ )
	{
		corners[i].Color = color;
		corners[i].V4 = clamp;
	}

	Driver->ActivateShader( nuShaderTextRGB );
	if ( !LoadTexture( atlas, 0 ) )
		return;
	Driver->DrawQuad( corners );
}

void nuRenderer::RenderTextChar_WholePixel( nuPoint base, nuRenderDomText* node, const nuRenderCharEl& txtEl )
{
	nuGlyphCacheKey glyphKey( node->FontID, txtEl.Char, node->FontSizePx, 0 );
	const nuGlyph* glyph = nuGlobal()->GlyphCache->GetGlyph( glyphKey );
	if ( !glyph )
	{
		GlyphsNeeded.insert( glyphKey );
		return;
	}

	nuTextureAtlas* atlas = nuGlobal()->GlyphCache->GetAtlasMutable( glyph->AtlasID );
	float atlasScaleX = 1.0f / atlas->GetWidth();
	float atlasScaleY = 1.0f / atlas->GetHeight();

	float top = nuPosToReal( base.Y + txtEl.Y );
	float left = nuPosToReal( base.X + txtEl.X );

	// a single pixel of padding is necessary to ensure that we're not short-sampling
	// the edges of the glyphs
	int pad = 1;

	// We only use whole pixel rendering on high resolution devices, and glyph sizes, as measured
	// in device pixels, are always high enough that it ends up being more important to have sub-pixel
	// positioning than to render the glyphs without any resampling.
	// 
	//                       Snapping On                                 Snapping Off
	// Rendering             Crisper, because no resampling              Fuzzier, because of resampling
	// Positioning           Text cannot be positioned sub-pixel         Text can be positioned with sub-pixel accuracy in X and Y

	bool snapToWholePixels = false;
	if ( snapToWholePixels )
	{
		left = (float) floor(left + 0.5f);
		top = (float) floor(top + 0.5f);
	}

	left -= pad;
	top -= pad;

	float right = left + glyph->Width + pad * 2;
	float bottom = top + glyph->Height + pad * 2;

	nuVx_PTC corners[4];
	corners[0].Pos = NUVEC3(left, top, 0);
	corners[1].Pos = NUVEC3(left, bottom, 0);
	corners[2].Pos = NUVEC3(right, bottom, 0);
	corners[3].Pos = NUVEC3(right, top, 0);

	float u0 = (glyph->X - pad) * atlasScaleX;
	float v0 = (glyph->Y - pad) * atlasScaleY;
	float u1 = (glyph->X + glyph->Width + pad * 2) * atlasScaleX;
	float v1 = (glyph->Y + glyph->Height + pad * 2) * atlasScaleY;

	corners[0].UV = NUVEC2(u0, v0);
	corners[1].UV = NUVEC2(u0, v1);
	corners[2].UV = NUVEC2(u1, v1);
	corners[3].UV = NUVEC2(u1, v0);

	uint32 color = node->Color.GetRGBA();

	for ( int i = 0; i < 4; i++ )
		corners[i].Color = color;

	Driver->ActivateShader( nuShaderTextWhole );
	if ( !LoadTexture( atlas, 0 ) )
		return;
	Driver->DrawQuad( corners );
}

void nuRenderer::RenderGlyphsNeeded()
{
	for ( auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++ )
		nuGlobal()->GlyphCache->RenderGlyph( *it );
	GlyphsNeeded.clear();
}

bool nuRenderer::LoadTexture( nuTexture* tex, int texUnit )
{
	if ( !Driver->LoadTexture(tex, texUnit) )
		return false;

	tex->TexValidate();
	return true;
}
