#include "pch.h"
#include "xoDoc.h"
#include "xoRenderer.h"
#include "xoRenderGL.h"
#include "xoRenderDomEl.h"
#include "xoTextureAtlas.h"
#include "Text/xoGlyphCache.h"
#include "../Image/xoImage.h"
#include "../Dom/xoDomCanvas.h"

xoRenderResult xoRenderer::Render( const xoDoc* doc, xoImageStore* images, xoStringTable* strings, xoRenderBase* driver, const xoRenderDomNode* root )
{
	Doc = doc;
	Driver = driver;
	Images = images;
	Strings = strings;

	Driver->PreRender();

	// This phase is probably worth parallelizing
	RenderEl( xoPoint(0,0), root );
	// After RenderNode we are serial again.

	Driver->PostRenderCleanup();

	bool needGlyphs = GlyphsNeeded.size() != 0;
	RenderGlyphsNeeded();

	return needGlyphs ? xoRenderResultNeedMore : xoRenderResultIdle;
}

void xoRenderer::RenderEl( xoPoint base, const xoRenderDomEl* el )
{
	if ( el->Tag == xoTagText )
		RenderText( base, static_cast<const xoRenderDomText*>(el) );
	else
	{
		const xoRenderDomNode* node = static_cast<const xoRenderDomNode*>(el);
		RenderNode( base, node );
		xoPoint newBase = base + xoPoint( node->Pos.Left, node->Pos.Top );
		for ( intp i = 0; i < node->Children.size(); i++ )
			RenderEl( newBase, node->Children[i] );
	}
}

void xoRenderer::RenderNode( xoPoint base, const xoRenderDomNode* node )
{
	// always shade rectangles well
	const bool alwaysGoodRects = true;

	const xoStyleRender* style = &node->Style;
	xoBox pos = node->Pos;
	pos.Offset( base );
	xoBoxF border = style->BorderSize.ToRealBox();
	xoBoxF padding = style->Padding.ToRealBox();
	float bottom = xoPosToReal( pos.Bottom ) + border.Bottom + padding.Bottom;
	float top = xoPosToReal( pos.Top ) - border.Top - padding.Top;
	float left = xoPosToReal( pos.Left ) - border.Left - padding.Left;
	float right = xoPosToReal( pos.Right ) + border.Right + padding.Right;

	float radius = style->BorderRadius;
	bool useRectShader = alwaysGoodRects || radius != 0;

	float width = right - left;
	float height = bottom - top;
	float mindim = xoMin(width, height);
	mindim = xoMax(mindim, 0.0f);
	radius = xoMin(radius, mindim / 2);

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

	xoVx_PTC corners[4];
	corners[0].Pos = XOVEC3(left - pad, top - pad, 0);
	corners[1].Pos = XOVEC3(left - pad, bottom + pad, 0);
	corners[2].Pos = XOVEC3(right + pad, bottom + pad, 0);
	corners[3].Pos = XOVEC3(right + pad, top - pad, 0);

	corners[0].UV = XOVEC2(-padU, -padV);
	corners[1].UV = XOVEC2(-padU, 1 + padV);
	corners[2].UV = XOVEC2(1 + padU, 1 + padV);
	corners[3].UV = XOVEC2(1 + padU, -padV);

	//auto bg = style.Get( xoCatBackground );
	//auto bgImage = style.Get( xoCatBackgroundImage );
	xoColor bg = style->BackgroundColor;
	const char* bgImage = Strings->GetStr( style->BackgroundImageID );
	if ( bg.a != 0 )
	{
		for ( int i = 0; i < 4; i++ )
			corners[i].Color = bg.GetRGBA();

		if ( useRectShader )
		{
			Driver->ActivateShader( xoShaderRect );
			Driver->ShaderPerObject.Box = xoVec4f( left, top, right, bottom );
			//Driver->ShaderPerObject.Border = xoVec4f( border.Left, border.Top, border.Right, border.Bottom );
			Driver->ShaderPerObject.Border = xoVec4f( border.Left + 0.5f, border.Top + 0.5f, border.Right + 0.5f, border.Bottom + 0.5f );
			//Driver->ShaderPerObject.Border = xoVec4f( border.Left - 0.5f, border.Top - 0.5f, border.Right - 0.5f, border.Bottom - 0.5f );
			Driver->ShaderPerObject.Radius = radius + 0.5f; // see the shader for an explanation of this 0.5
			Driver->ShaderPerObject.BorderColor = style->BorderColor.GetVec4Linear();
			Driver->DrawQuad( corners );
		}
		else
		{
			if ( bgImage[0] != 0 )
			{
				Driver->ActivateShader( xoShaderFillTex );
				if ( !LoadTexture( Images->GetOrNull( bgImage ), TexUnit0 ) )
					return;
				Driver->DrawQuad( corners );
			}
			else
			{
				Driver->ActivateShader( xoShaderFill );
				Driver->DrawQuad( corners );
			}
		}
	}

	if ( node->IsCanvas() )
	{
		const xoDomCanvas* canvas = static_cast<const xoDomCanvas*>(Doc->GetChildByInternalID( node->InternalID ));
		xoImage* canvasImage = Images->Get( canvas->GetCanvasImageName() );
		if ( canvasImage != nullptr )
		{
			Driver->ActivateShader( xoShaderFillTex );
			if ( LoadTexture( canvasImage, TexUnit0 ) )
				Driver->DrawQuad( corners );
		}
	}
}

void xoRenderer::RenderText( xoPoint base, const xoRenderDomText* node )
{
	bool subPixelGlyphs = node->Flags & xoRenderDomText::FlagSubPixelGlyphs;
	for ( intp i = 0; i < node->Text.size(); i++ )
	{
		if ( subPixelGlyphs )
			RenderTextChar_SubPixel( base, node, node->Text[i] );
		else
			RenderTextChar_WholePixel( base, node, node->Text[i] );
	}
}

void xoRenderer::RenderTextChar_SubPixel( xoPoint base, const xoRenderDomText* node, const xoRenderCharEl& txtEl )
{
	xoGlyphCacheKey glyphKey( node->FontID, txtEl.Char, node->FontSizePx, xoGlyphFlag_SubPixel_RGB );
	const xoGlyph* glyph = xoGlobal()->GlyphCache->GetGlyph( glyphKey );
	if ( !glyph )
	{
		GlyphsNeeded.insert( glyphKey );
		return;
	}

	xoTextureAtlas* atlas = xoGlobal()->GlyphCache->GetAtlasMutable( glyph->AtlasID );
	float atlasScaleX = 1.0f / atlas->GetWidth();
	float atlasScaleY = 1.0f / atlas->GetHeight();

	float top = xoPosToReal( xoPosRound(base.Y + txtEl.Y) );
	float left = xoPosToReal( base.X + txtEl.X );

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

	// You cannot do this here - it needs to happen during layout (which it does indeed)
	//if ( xoGlobal()->SnapSubpixelHorzText )
	//	left = floor(left + 0.5f);

	float right = left + roundedWidth;
	float bottom = top + glyph->Height;

	// We have to 'overdraw' on the left and right by 1 pixel, to ensure that we filter over the edges.
	int overdraw = 1;
	left -= overdraw;
	right += overdraw;

	xoVx_PTCV4 corners[4];
	corners[0].Pos = XOVEC3(left, top, 0);
	corners[1].Pos = XOVEC3(left, bottom, 0);
	corners[2].Pos = XOVEC3(right, bottom, 0);
	corners[3].Pos = XOVEC3(right, top, 0);

	float u0 = (glyph->X + horzPad - overdraw * 3) * atlasScaleX;
	float v0 = glyph->Y * atlasScaleY;
	float u1 = (glyph->X + horzPad + (roundedWidth + overdraw) * 3) * atlasScaleX;
	float v1 = (glyph->Y + glyph->Height) * atlasScaleY;

	corners[0].UV = XOVEC2(u0, v0);
	corners[1].UV = XOVEC2(u0, v1);
	corners[2].UV = XOVEC2(u1, v1);
	corners[3].UV = XOVEC2(u1, v0);

	// Obviously our clamping is not affected by overdraw. It remains our absolute texel limits.
	xoVec4f clamp;
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

	Driver->ActivateShader( xoShaderTextRGB );
	if ( !LoadTexture( atlas, TexUnit0 ) )
		return;
	Driver->DrawQuad( corners );
}

void xoRenderer::RenderTextChar_WholePixel( xoPoint base, const xoRenderDomText* node, const xoRenderCharEl& txtEl )
{
	xoGlyphCacheKey glyphKey( node->FontID, txtEl.Char, node->FontSizePx, 0 );
	const xoGlyph* glyph = xoGlobal()->GlyphCache->GetGlyph( glyphKey );
	if ( !glyph )
	{
		GlyphsNeeded.insert( glyphKey );
		return;
	}

	xoTextureAtlas* atlas = xoGlobal()->GlyphCache->GetAtlasMutable( glyph->AtlasID );
	float atlasScaleX = 1.0f / atlas->GetWidth();
	float atlasScaleY = 1.0f / atlas->GetHeight();

	float top = xoPosToReal( base.Y + txtEl.Y );
	float left = xoPosToReal( base.X + txtEl.X );

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

	xoVx_PTC corners[4];
	corners[0].Pos = XOVEC3(left, top, 0);
	corners[1].Pos = XOVEC3(left, bottom, 0);
	corners[2].Pos = XOVEC3(right, bottom, 0);
	corners[3].Pos = XOVEC3(right, top, 0);

	float u0 = (glyph->X - pad) * atlasScaleX;
	float v0 = (glyph->Y - pad) * atlasScaleY;
	float u1 = (glyph->X + glyph->Width + pad * 2) * atlasScaleX;
	float v1 = (glyph->Y + glyph->Height + pad * 2) * atlasScaleY;

	corners[0].UV = XOVEC2(u0, v0);
	corners[1].UV = XOVEC2(u0, v1);
	corners[2].UV = XOVEC2(u1, v1);
	corners[3].UV = XOVEC2(u1, v0);

	uint32 color = node->Color.GetRGBA();

	for ( int i = 0; i < 4; i++ )
		corners[i].Color = color;

	Driver->ActivateShader( xoShaderTextWhole );
	if ( !LoadTexture( atlas, TexUnit0 ) )
		return;
	Driver->DrawQuad( corners );
}

void xoRenderer::RenderGlyphsNeeded()
{
	for ( auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++ )
		xoGlobal()->GlyphCache->RenderGlyph( *it );
	GlyphsNeeded.clear();
}

bool xoRenderer::LoadTexture( xoTexture* tex, TexUnits texUnit )
{
	if ( !Driver->LoadTexture(tex, texUnit) )
		return false;

	tex->TexClearInvalidRect();
	return true;
}
