#include "pch.h"
#include "xoDoc.h"
#include "xoRenderer.h"
#include "xoRenderGL.h"
#include "xoRenderDomEl.h"
#include "xoTextureAtlas.h"
#include "Text/xoGlyphCache.h"
#include "../Image/xoImage.h"
#include "../Dom/xoDomCanvas.h"

xoRenderResult xoRenderer::Render(const xoDoc* doc, xoImageStore* images, xoStringTable* strings, xoRenderBase* driver, const xoRenderDomNode* root)
{
	Doc = doc;
	Driver = driver;
	Images = images;
	Strings = strings;

	Driver->PreRender();

	// This phase is probably worth parallelizing
	RenderEl(xoPoint(0,0), root);
	// After RenderNode we are serial again.

	Driver->PostRenderCleanup();

	bool needGlyphs = GlyphsNeeded.size() != 0;
	RenderGlyphsNeeded();

	return needGlyphs ? xoRenderResultNeedMore : xoRenderResultIdle;
}

void xoRenderer::RenderEl(xoPoint base, const xoRenderDomEl* el)
{
	if (el->Tag == xoTagText)
	{
		xoPoint newBase = base + xoPoint(el->Pos.Left, el->Pos.Top);
		RenderText(newBase, static_cast<const xoRenderDomText*>(el));
	}
	else
	{
		const xoRenderDomNode* node = static_cast<const xoRenderDomNode*>(el);
		RenderNode(base, node);
		xoPoint newBase = base + xoPoint(node->Pos.Left, node->Pos.Top);
		for (intp i = 0; i < node->Children.size(); i++)
			RenderEl(newBase, node->Children[i]);
	}
}

struct xoBoxRadiusSet
{
	Vec2f TopLeft;
	Vec2f BottomLeft;
	Vec2f BottomRight;
	Vec2f TopRight;
};

void xoRenderer::RenderNode(xoPoint base, const xoRenderDomNode* node)
{
	//if (node->Style.BackgroundColor == xoColor::RGBA(0xff, 0xf0, 0xf0, 0xff)) { RenderQuadratic(base, node); return; }

	// always shade rectangles well
	const bool alwaysGoodRects = true;

	const xoStyleRender* style = &node->Style;
	xoBox pos = node->Pos;
	pos.Offset(base);
	xoBoxF border = style->BorderSize.ToRealBox();
	xoBoxF padding = style->Padding.ToRealBox();
	float top = xoPosToReal(pos.Top) - border.Top - padding.Top;
	float bottom = xoPosToReal(pos.Bottom) + border.Bottom + padding.Bottom;
	float left = xoPosToReal(pos.Left) - border.Left - padding.Left;
	float right = xoPosToReal(pos.Right) + border.Right + padding.Right;

	float radius = style->BorderRadius;
	bool useRectShader = alwaysGoodRects || radius != 0;
	// I only tried out rect2 shader on OpenGL, and then went ahead to try Blinn/Loop rendering.
	bool useRect2Shader = false; // strcmp(Driver->RendererName(), "OpenGL") == 0;
	bool useRect3Shader = xoGlobal()->UseRect3;

	float width = right - left;
	float height = bottom - top;
	float mindim = xoMin(width, height);
	mindim = xoMax(mindim, 0.0f);
	radius = xoMin(radius, mindim / 2);

	if (mindim <= 0)
		return;

	float padU = 0;
	float padV = 0;
	float pad = useRectShader ? 1.0f : 0.0f;
	if (pad != 0)
	{
		padU = pad / width;
		padV = pad / height;
	}
	 
	// Vertex ordering: 0 3
	//                  1 2 
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
	const char* bgImage = Strings->GetStr(style->BackgroundImageID);

	if (bg.a != 0 && useRect3Shader)
	{
		xoBoxRadiusSet radii = {
			{ radius, radius },	// left, top
			{ radius, radius }, // left, bottom
			{ radius, radius },	// right, bottom
			{ radius, radius }, // right, top
		};
		Driver->ActivateShader(xoShaderRect3);
		xoVx_PTCV4 vx[16];
		float vmid = 0.5f * (top + bottom);
		float borderPos = border.Top;
		// If we want to work under perspective, then we'll need to make these paddings adjust to
		// the current projection. Failing to do so will result in aliased edges of our boxes.
		// This sounds like a thing you might want to do in a vertex or geometry shader, because you
		// need to project a point in order to see what kind of padding is necessary to produce at
		// least 1 pixel of rasterized border.
		// For now, just use constants.
		float vpad = 1;
		float hpad = 1;
		uint32 bgRGBA = bg.GetRGBA();
		uint32 borderRGBA = style->BorderColor.GetRGBA();

		float leftEdge = xoMax(radii.TopLeft.x, radii.BottomLeft.x);
		leftEdge = xoMax(leftEdge, border.Left);
		
		float rightEdge = xoMax(radii.TopRight.x, radii.BottomRight.x);
		rightEdge = xoMax(rightEdge, border.Right);

		//                                                                                                               Border width
		//                                                                                                               |           Distance from edge
		//                                                                                                               |           |
		// top                                                                                                           |           |
		vx[0].Set(XOVEC3(left + leftEdge, top - vpad, 0),						XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Top, -vpad, 0, 0));
		vx[1].Set(XOVEC3(left + leftEdge, vmid, 0),								XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Top, vmid - top, 0, 0));
		vx[2].Set(XOVEC3(right - rightEdge, vmid, 0),							XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Top, vmid - top, 0, 0));
		vx[3].Set(XOVEC3(right - rightEdge, top - vpad, 0),						XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Top, -vpad, 0, 0));

		// bottom
		vx[4].Set(XOVEC3(left + leftEdge, vmid, 0),								XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Bottom, bottom - vmid, 0, 0));
		vx[5].Set(XOVEC3(left + leftEdge, bottom + vpad, 0),					XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Bottom, -vpad, 0, 0));
		vx[6].Set(XOVEC3(right - rightEdge, bottom + vpad, 0),					XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Bottom, -vpad, 0, 0));
		vx[7].Set(XOVEC3(right - rightEdge, vmid, 0),							XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Bottom, bottom - vmid, 0, 0));

		// left
		vx[8].Set(XOVEC3(left - hpad, top + radii.TopLeft.y, 0),				XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Left, -hpad, 0, 0));
		vx[9].Set(XOVEC3(left - hpad, bottom - radii.BottomLeft.y, 0),			XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Left, -hpad, 0, 0));
		vx[10].Set(XOVEC3(left + leftEdge, bottom - radii.BottomLeft.y, 0),		XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Left, leftEdge, 0, 0));
		vx[11].Set(XOVEC3(left + leftEdge, top + radii.TopLeft.y, 0),			XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Left, leftEdge, 0, 0));

		// right
		vx[12].Set(XOVEC3(right - rightEdge, top + radii.TopRight.y, 0),		XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Right, rightEdge, 0, 0));
		vx[13].Set(XOVEC3(right - rightEdge, bottom - radii.BottomRight.y, 0),	XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Right, rightEdge, 0, 0));
		vx[14].Set(XOVEC3(right + hpad, bottom - radii.BottomRight.y, 0),		XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Right, -hpad, 0, 0));
		vx[15].Set(XOVEC3(right + hpad, top + radii.TopRight.y, 0),				XOVEC2(0, 0), bgRGBA, borderRGBA, XOVEC4(border.Right, -hpad, 0, 0));

		Driver->Draw(xoGPUPrimQuads, 16, vx);

		if (radii.TopLeft.x != 0 && radii.TopLeft.y != 0)
		{
			Driver->ActivateShader(xoShaderArc);
			float maxRadius = xoMax(radii.TopLeft.x, radii.TopLeft.y);
			float fanRadius;
			int divs;
			if (border.Left == border.Top && radii.TopLeft.x == radii.TopLeft.y)
			{
				// I haven't worked out why, but empirically 1.04 seems to be sufficient up to 200px radius.
				fanRadius = maxRadius * 1.04 + 2.0f;
				divs = 3;
			}
			else
			{
				fanRadius = maxRadius * 1.1 + 2.0f;
				divs = (int) xoClamp(maxRadius * 0.4f, 5.0f, 50.0f);
			}

			float inc = (float) (XO_PI / 2) / (float) divs;
			int slice = 0;
			auto center = XOVEC3(left + radii.TopLeft.x, top + radii.TopLeft.y, 0);
			float unit_inc = 1.0f / (float) divs;
			float unit_slice = unit_inc * 0.5;
			float th = XO_PI / 2 + inc;
			xoVec3f pos = XOVEC3(center.x, center.y - fanRadius, 0);
			auto v4 = XOVEC4(center.x, center.y, center.z, 0);
			for (; slice < divs; th += inc, unit_slice += unit_inc, slice++)
			{
				float upos = unit_slice;
				if (slice == 0)
					upos = 0;
				else if (slice == divs - 1)
					upos = 1;
				float borderWidth = xoLerp(upos, border.Top, border.Left);
				float radius = xoLerp(upos, radii.TopLeft.y, radii.TopLeft.x);
				v4.w = radius;
				auto posNext = XOVEC3(center.x + fanRadius * cos(th), center.y - fanRadius * sin(th), center.z);
				auto uv = XOVEC2(borderWidth, 0);
				vx[0].Set(center, uv, bgRGBA, borderRGBA, v4);
				vx[1].Set(pos, uv, bgRGBA, borderRGBA, v4);
				vx[2].Set(posNext, uv, bgRGBA, borderRGBA, v4);
				Driver->Draw(xoGPUPrimTriangles, 3, vx);
				pos = posNext;
			}
		}
	}

	if (bg.a != 0 && !useRect3Shader)
	{
		for (int i = 0; i < 4; i++)
			corners[i].Color = bg.GetRGBA();

		if (useRect2Shader)
		{
			Driver->ActivateShader(xoShaderRect2);
			xoVec2f center = XOVEC2((left + right) / 2.0f,(top + bottom) / 2.0f);
			xoVx_PTCV4 quads[4];
			quads[0].PTC = corners[0];
			quads[1].PTC = corners[1];
			quads[2].PTC = corners[2];
			quads[3].PTC = corners[3];
			for (int i = 0; i < 4; i++)
			{
				quads[i].Color2 = style->BorderColor.GetRGBA();
				quads[i].UV.x = radius;
				quads[i].UV.y = border.Left;
			}

			Driver->ShaderPerObject.ShadowColor = xoVec4f(0, 0, 0, 0);
			Driver->ShaderPerObject.ShadowOffset = xoVec2f(0, 0);
			Driver->ShaderPerObject.ShadowSizeInv = 0;

			// top-left
			Driver->ShaderPerObject.Edges = xoVec2f(left, top);
			Driver->ShaderPerObject.OutVector = xoVec2f(-1, -1);
			quads[0].Pos.vec2 = corners[0].Pos.vec2;
			quads[1].Pos.vec2 = XOVEC2(corners[1].Pos.x, center.y);
			quads[2].Pos.vec2 = XOVEC2(center.x, center.y);
			quads[3].Pos.vec2 = XOVEC2(center.x, corners[3].Pos.y);
			quads[0].UV.y = border.Left;
			quads[1].UV.y = border.Left;
			quads[2].UV.y = border.Left;
			quads[3].UV.y = border.Left;
			Driver->Draw(xoGPUPrimQuads, 4, quads);

			// top-right
			Driver->ShaderPerObject.Edges = xoVec2f(right, top);
			Driver->ShaderPerObject.OutVector = xoVec2f(1, -1);
			quads[0].Pos.vec2 = XOVEC2(center.x, corners[0].Pos.y);
			quads[1].Pos.vec2 = XOVEC2(center.x, center.y);
			quads[2].Pos.vec2 = XOVEC2(corners[2].Pos.x, center.y);
			quads[3].Pos.vec2 = XOVEC2(corners[3].Pos.x, corners[3].Pos.y);
			Driver->Draw(xoGPUPrimQuads, 4, quads);

			// bottom-left
			Driver->ShaderPerObject.Edges = xoVec2f(left, bottom);
			Driver->ShaderPerObject.OutVector = xoVec2f(-1, 1);
			quads[0].Pos.vec2 = XOVEC2(corners[0].Pos.x, center.y);
			quads[1].Pos.vec2 = XOVEC2(corners[1].Pos.x, corners[1].Pos.y);
			quads[2].Pos.vec2 = XOVEC2(center.x, corners[2].Pos.y);
			quads[3].Pos.vec2 = XOVEC2(center.x, center.y);
			Driver->Draw(xoGPUPrimQuads, 4, quads);

			// bottom-right
			Driver->ShaderPerObject.Edges = xoVec2f(right, bottom);
			Driver->ShaderPerObject.OutVector = xoVec2f(1, 1);
			quads[0].Pos.vec2 = XOVEC2(center.x, center.y);
			quads[1].Pos.vec2 = XOVEC2(center.x, corners[1].Pos.y);
			quads[2].Pos.vec2 = XOVEC2(corners[2].Pos.x, corners[2].Pos.y);
			quads[3].Pos.vec2 = XOVEC2(corners[3].Pos.x, center.y);
			Driver->Draw(xoGPUPrimQuads, 4, quads);
		}
		else if (useRectShader)
		{
			Driver->ActivateShader(xoShaderRect);
			Driver->ShaderPerObject.Box = xoVec4f(left, top, right, bottom);
			Driver->ShaderPerObject.Border = xoVec4f(border.Left + 0.5f, border.Top + 0.5f, border.Right + 0.5f, border.Bottom + 0.5f);
			Driver->ShaderPerObject.Radius = radius + 0.5f; // see the shader for an explanation of this 0.5
			Driver->ShaderPerObject.BorderColor = style->BorderColor.GetVec4Linear();
			Driver->Draw(xoGPUPrimQuads, 4, corners);
		}
		else
		{
			if (bgImage[0] != 0)
			{
				Driver->ActivateShader(xoShaderFillTex);
				if (!LoadTexture(Images->GetOrNull(bgImage), TexUnit0))
					return;
				Driver->Draw(xoGPUPrimQuads, 4, corners);
			}
			else
			{
				Driver->ActivateShader(xoShaderFill);
				Driver->Draw(xoGPUPrimQuads, 4, corners);
			}
		}
	}

	if (node->IsCanvas())
	{
		for (int i = 0; i < 4; i++)
			corners[i].Color = 0xffffffff; // could be used to tint the canvas

		const xoDomCanvas* canvas = static_cast<const xoDomCanvas*>(Doc->GetChildByInternalID(node->InternalID));
		xoImage* canvasImage = Images->Get(canvas->GetCanvasImageName());
		if (canvasImage != nullptr)
		{
			Driver->ActivateShader(xoShaderFillTex);
			if (LoadTexture(canvasImage, TexUnit0))
				Driver->Draw(xoGPUPrimQuads, 4, corners);
		}
	}
}

void xoRenderer::RenderQuadratic(xoPoint base, const xoRenderDomNode* node)
{
	base.X += node->Pos.Left;
	base.Y += node->Pos.Top;

	// I get these coordinates by drawing circles in Albion. The other circle is centered around the origin.
	float vx[] = {
		0, 5,
		-5, 5,
		-5, 0,

		-5, 0,
		-4, 0,
		-3, 0,

		-3, 0,
		-3, 4,
		0, 4,
	
		0, 4,
		0, 4.5,
		0, 5,
	};

	xoVx_PTCV4 corners[50];
	xoVec2f uv[3];
	uv[0] = XOVEC2(0, 0);
	uv[1] = XOVEC2(0.5, 0);
	uv[2] = XOVEC2(1, 1);
	xoVec3f basef(xoPosToReal(base.X), xoPosToReal(base.Y), 0);

	basef = xoVec3f(180, 30, 0) + 0.02f * basef;

	for (int i = 0; i < arraysize(vx); i++)
	{
		xoVec3f p(vx[i * 2], 5 - vx[i * 2 + 1], 0);
		p = 30.0f * p;
		corners[i].Pos = basef + p;
		corners[i].UV = uv[i % 3];
		corners[i].Color = i < 3 ? xoColor::RGBA(250, 50, 50, 255).GetRGBA() : xoColor::RGBA(50, 50, 250, 255).GetRGBA();
		corners[i].V4.x = -1;
	}

	Driver->ActivateShader(xoShaderQuadraticSpline);
	Driver->Draw(xoGPUPrimTriangles, 12, corners);
}

void xoRenderer::RenderText(xoPoint base, const xoRenderDomText* node)
{
	bool subPixelGlyphs = node->Flags & xoRenderDomText::FlagSubPixelGlyphs;
	for (intp i = 0; i < node->Text.size(); i++)
	{
		if (subPixelGlyphs)
			RenderTextChar_SubPixel(base, node, node->Text[i]);
		else
			RenderTextChar_WholePixel(base, node, node->Text[i]);
	}
}

void xoRenderer::RenderTextChar_SubPixel(xoPoint base, const xoRenderDomText* node, const xoRenderCharEl& txtEl)
{
	xoGlyphCacheKey glyphKey(node->FontID, txtEl.Char, node->FontSizePx, xoGlyphFlag_SubPixel_RGB);
	const xoGlyph* glyph = xoGlobal()->GlyphCache->GetGlyph(glyphKey);
	if (!glyph)
	{
		GlyphsNeeded.insert(glyphKey);
		return;
	}

	xoTextureAtlas* atlas = xoGlobal()->GlyphCache->GetAtlasMutable(glyph->AtlasID);
	float atlasScaleX = 1.0f / atlas->GetWidth();
	float atlasScaleY = 1.0f / atlas->GetHeight();

	float top = xoPosToReal(xoPosRound(base.Y + txtEl.Y));
	float left = xoPosToReal(base.X + txtEl.X);

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

	for (int i = 0; i < 4; i++)
	{
		corners[i].Color = color;
		corners[i].V4 = clamp;
	}

	Driver->ActivateShader(xoShaderTextRGB);
	if (!LoadTexture(atlas, TexUnit0))
		return;
	Driver->Draw(xoGPUPrimQuads, 4, corners);
}

void xoRenderer::RenderTextChar_WholePixel(xoPoint base, const xoRenderDomText* node, const xoRenderCharEl& txtEl)
{
	xoGlyphCacheKey glyphKey(node->FontID, txtEl.Char, node->FontSizePx, 0);
	const xoGlyph* glyph = xoGlobal()->GlyphCache->GetGlyph(glyphKey);
	if (!glyph)
	{
		GlyphsNeeded.insert(glyphKey);
		return;
	}

	xoTextureAtlas* atlas = xoGlobal()->GlyphCache->GetAtlasMutable(glyph->AtlasID);
	float atlasScaleX = 1.0f / atlas->GetWidth();
	float atlasScaleY = 1.0f / atlas->GetHeight();

	float top = xoPosToReal(base.Y + txtEl.Y);
	float left = xoPosToReal(base.X + txtEl.X);

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
	if (snapToWholePixels)
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
	float u1 = (glyph->X + glyph->Width + pad) * atlasScaleX;
	float v1 = (glyph->Y + glyph->Height + pad) * atlasScaleY;

	corners[0].UV = XOVEC2(u0, v0);
	corners[1].UV = XOVEC2(u0, v1);
	corners[2].UV = XOVEC2(u1, v1);
	corners[3].UV = XOVEC2(u1, v0);

	uint32 color = node->Color.GetRGBA();

	for (int i = 0; i < 4; i++)
		corners[i].Color = color;

	Driver->ActivateShader(xoShaderTextWhole);
	if (!LoadTexture(atlas, TexUnit0))
		return;
	Driver->Draw(xoGPUPrimQuads, 4, corners);
}

void xoRenderer::RenderGlyphsNeeded()
{
	for (auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++)
		xoGlobal()->GlyphCache->RenderGlyph(*it);
	GlyphsNeeded.clear();
}

bool xoRenderer::LoadTexture(xoTexture* tex, TexUnits texUnit)
{
	if (!Driver->LoadTexture(tex, texUnit))
		return false;

	tex->TexClearInvalidRect();
	return true;
}
