#include "pch.h"
#include "nuDoc.h"
#include "nuRenderer.h"
#include "nuRenderGL.h"
#include "nuRenderDomEl.h"
#include "nuTextureAtlas.h"
#include "Text/nuGlyphCache.h"
#include "../Image/nuImage.h"

nuRenderResult nuRenderer::Render( nuImageStore* images, nuStringTable* strings, nuRenderBase* driver, nuRenderDomEl* root, int width, int height )
{
	Driver = driver;
	Images = images;
	Strings = strings;

	Driver->PreRender();

	// This phase is probably worth parallelizing
	RenderNodeOuter( root );
	// After RenderNode we are serial again.

	Driver->PostRenderCleanup();

	bool needGlyphs = GlyphsNeeded.size() != 0;
	RenderGlyphsNeeded();

	return needGlyphs ? nuRenderResultNeedMore : nuRenderResultIdle;
}

void nuRenderer::RenderNodeOuter( nuRenderDomEl* node )
{
	if ( node->Text.size() != 0 )
	{
		RenderTextNode( node );
		return;
	}

	RenderNodeInner( node );

	for ( intp i = 0; i < node->Children.size(); i++ )
		RenderNodeOuter( node->Children[i] );
}

void nuRenderer::RenderNodeInner( nuRenderDomEl* node )
{
	// always shade rectangles well
	const bool alwaysGoodRects = false; // TEMP - DX

	nuStyleRender* style = &node->Style;
	float bottom = nuPosToReal( node->Pos.Bottom );
	float top = nuPosToReal( node->Pos.Top );
	float left = nuPosToReal( node->Pos.Left );
	float right = nuPosToReal( node->Pos.Right );

	float radius = style->BorderRadius;
	//radius = 0; // TEMP - try to get DirectX working
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

	//for ( int i = 0; i < 4; i++ )
	//	corners[i].Pos.z = 0.5f;

	Mat4f tx = Driver->ShaderPerFrame.MVProj.Transposed();
	for ( int i = 0; i < 4; i++ )
	{
		//corners[i].Pos = (tx * nuVec4f(corners[i].Pos, 1.0f)).vec3;
		//corners[i].Pos.z = 0.5f;
	}
	//corners[0].Pos = NUVEC3( 0.0f, 0.5f, 0.5f );
	//corners[1].Pos = NUVEC3( 0.5f, -0.5f, 0.5f );
	//corners[2].Pos = NUVEC3( -0.5f, -0.5f, 0.5f );

	//NUTRACE( "node %f\n", left );

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
				Driver->LoadTexture( Images->GetOrNull( bgImage ), 0 );
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

void nuRenderer::RenderTextNode( nuRenderDomEl* node )
{
	bool subPixel = nuGlobal()->EnableSubpixelText;
	for ( intp i = 0; i < node->Text.size(); i++ )
	{
		if ( subPixel )
			RenderTextNodeChar_SubPixel( node, node->Text[i] );
		else
			RenderTextNodeChar_WholePixel( node, node->Text[i] );
	}
}

void nuRenderer::RenderTextNodeChar_SubPixel( nuRenderDomEl* node, const nuRenderTextEl& txtEl )
{
	nuStyleRender* style = &node->Style;

	nuGlyphCacheKey glyphKey( node->FontID, txtEl.Char, style->FontSizePx, nuGlyphFlag_SubPixel_RGB );
	const nuGlyph* glyph = nuGlobal()->GlyphCache->GetGlyph( glyphKey );
	if ( !glyph )
	{
		GlyphsNeeded.insert( glyphKey );
		return;
	}
	//if ( !nuGlobal()->GlyphCache->GetGlyphFromChar( node->FontID, txtEl.Char, style->FontSizePx, nuGlyphFlag_SubPixel_RGB, glyph ) )
	//	return;

	nuTextureAtlas* atlas = nuGlobal()->GlyphCache->GetAtlasMutable( glyph->AtlasID );
	float atlasScaleX = 1.0f / atlas->GetWidth();
	float atlasScaleY = 1.0f / atlas->GetHeight();

	float top = nuPosToReal( nuPosRound(txtEl.Y) );
	float left = nuPosToReal( txtEl.X );

	// Our glyph has a single column on the left and right side, so that our clamped texture
	// reads will pickup a zero when reading off beyond the edge of the glyph
	int horzPad = 1;
	int nonPaddedWidth = glyph->Width - horzPad * 2;

	// Our texture (minus the padding) can be missing 1 or 2 columns on its right side,
	// but our proper width in pixels DOES include those 1 or 2 missing columns (they are simply black).
	// We clamp our texture reads to our freetype-rasterized limits, but the geometry we emit
	// now to the GPU includes those 1 or 2 missing columns.
	int roundedWidth = (nonPaddedWidth + 2) / 3;

	// We don't need to round our horizontal position to any fixed grid - the interpolation
	// is correct regardless, and since we clobber all horizontal hinting, horizontal
	// grid-fitting should have little effect.
	//left = (float) floor(left * 3 + 0.5) / 3.0f;
	//top = (float) floor(top + 0.5); -- vertical rounding is moved to fixed point

	float right = left + roundedWidth;
	float bottom = top + glyph->Height;

	// We have to 'overdraw' on the left and right by 1 pixel, to ensure that we filter over
	// the edges.
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
	for ( int i = 0; i < 4; i++ )
	{
		//corners[i].Color = NURGBA(0,0,0,255);
		//corners[i].Color = NURGBA(255,255,255,255);
		corners[i].Color = NURGBA(0,0,0,255);
		//corners[i].Color = NURGBA(10,10,10,255);
		//corners[i].Color = NURGBA(150,0,0,255);
		corners[i].V4 = clamp;
	}

	Driver->ActivateShader( nuShaderTextRGB );
	Driver->LoadTexture( atlas, 0 );
	Driver->DrawQuad( corners );
}

void nuRenderer::RenderTextNodeChar_WholePixel( nuRenderDomEl* node, const nuRenderTextEl& txtEl )
{
	nuStyleRender* style = &node->Style;

	//nuGlyph glyph;
	//if ( !nuGlobal()->GlyphCache->GetGlyphFromChar( node->FontID, txtEl.Char, style->FontSizePx, 0, glyph ) )
	//	return;
	nuGlyphCacheKey glyphKey( node->FontID, txtEl.Char, style->FontSizePx, 0 );
	const nuGlyph* glyph = nuGlobal()->GlyphCache->GetGlyph( glyphKey );
	if ( !glyph )
	{
		GlyphsNeeded.insert( glyphKey );
		return;
	}

	nuTextureAtlas* atlas = nuGlobal()->GlyphCache->GetAtlasMutable( glyph->AtlasID );
	float atlasScaleX = 1.0f / atlas->GetWidth();
	float atlasScaleY = 1.0f / atlas->GetHeight();

	float top = nuPosToReal( txtEl.Y );
	float left = nuPosToReal( txtEl.X );

	// a single pixel of padding is necessary to ensure that we're not short-sampling
	// the edges of the glyphs
	int pad = 1;

	// We only use whole pixel rendering on high resolution devices, and glyph sizes, as measured
	// in device pixels, are always high enough that we don't even need to round.
	bool round = false;
	if ( round )
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

	for ( int i = 0; i < 4; i++ )
		corners[i].Color = NURGBA(150,0,0,255);

	Driver->ActivateShader( nuShaderTextWhole );
	Driver->LoadTexture( atlas, 0 );
	Driver->DrawQuad( corners );
}

void nuRenderer::RenderGlyphsNeeded()
{
	for ( auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++ )
		nuGlobal()->GlyphCache->RenderGlyph( *it );
	GlyphsNeeded.clear();
}


		/*
		uint16 indices[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
		//uint16 indices[12 * 4];
		nuVx_PTC tri[12];

		Driver->ActivateShader( nuShaderCurve );

		const float A = radius;
		const float B = SQRT_2 * 0.5 * A;
		const float C = A - B;
		const float M1 = 0.5 * (A + C);
		const float M2 = 0.5 * (0 + C);
		const float ME = -0.101 * A;
		for ( int icorner = 0; icorner < 1; icorner++ )
		{
			tri[0].UV = NUVEC2(1,1);
			tri[1].UV = NUVEC2(0.5,0);
			tri[2].UV = NUVEC2(0,0);
			tri[0].Pos = NUVEC3(A,0,0);
			tri[1].Pos = NUVEC3(M1 + ME, M2 + ME,0);
			tri[2].Pos = NUVEC3(C,C,0);

			tri[3].UV = NUVEC2(1,1);
			tri[4].UV = NUVEC2(0.5,0);
			tri[5].UV = NUVEC2(0,0);
			tri[3].Pos = NUVEC3(C,C,0);
			tri[4].Pos = NUVEC3(M2 + ME, M1 + ME,0);
			tri[5].Pos = NUVEC3(0,A,0);

			tri[6].UV = NUVEC2(1,1);
			tri[7].UV = NUVEC2(0.5,0.5);
			tri[8].UV = NUVEC2(0,0);
			tri[6].Pos = NUVEC3(A,0,0);
			tri[7].Pos = NUVEC3(C,C,0);
			tri[8].Pos = NUVEC3(A,A,0);

			tri[9].UV = NUVEC2(1,1);
			tri[10].UV = NUVEC2(0.5,0.5);
			tri[11].UV = NUVEC2(0,0);
			tri[9].Pos = NUVEC3(C,C,0);
			tri[10].Pos = NUVEC3(0,A,0);
			tri[11].Pos = NUVEC3(A,A,0);

			Driver->DrawTriangles( 12, tri, indices );
		}
		*/
