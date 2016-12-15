#include "pch.h"
#include "Doc.h"
#include "Renderer.h"
#include "RenderGL.h"
#include "RenderDomEl.h"
#include "TextureAtlas.h"
#include "Text/GlyphCache.h"
#include "../Image/Image.h"
#include "../Dom/DomCanvas.h"

namespace xo {

const int SHADER_FLAG_TEXBG = 16;

const int SHADER_ARC           = 1;
const int SHADER_RECT          = 2;
const int SHADER_TEXT_SIMPLE   = 3;
const int SHADER_TEXT_SUBPIXEL = 4;

RenderResult Renderer::Render(const Doc* doc, ImageStore* images, StringTable* strings, RenderBase* driver, const RenderDomNode* root) {
	Doc     = doc;
	Driver  = driver;
	Images  = images;
	Strings = strings;

	Driver->PreRender();

	// This phase is probably worth parallelizing
	RenderEl(Point(0, 0), root);
	// After RenderNode we are serial again.

	Driver->PostRenderCleanup();

	bool needGlyphs = GlyphsNeeded.size() != 0;
	RenderGlyphsNeeded();

	return needGlyphs ? RenderResultNeedMore : RenderResultIdle;
}

void Renderer::RenderEl(Point base, const RenderDomEl* el) {
	if (el->Tag == TagText) {
		Point newBase = base + Point(el->Pos.Left, el->Pos.Top);
		RenderText(newBase, static_cast<const RenderDomText*>(el));
	} else {
		const RenderDomNode* node = static_cast<const RenderDomNode*>(el);
		RenderNode(base, node);
		Point newBase = base + Point(node->Pos.Left, node->Pos.Top);
		for (size_t i = 0; i < node->Children.size(); i++)
			RenderEl(newBase, node->Children[i]);
	}
}

struct BoxRadiusSet {
	Vec2f TopLeft;
	Vec2f BottomLeft;
	Vec2f BottomRight;
	Vec2f TopRight;
};

void Renderer::RenderNode(Point base, const RenderDomNode* node) {
	// Use this to demo the quadratic curve rendering (Blinn/Loop)
	//if (node->Style.BackgroundColor == Color::RGBA(0xff, 0xf0, 0xf0, 0xff)) { RenderQuadratic(base, node); return; }

	const StyleRender* style = &node->Style;
	Box                pos   = node->Pos;
	pos.Offset(base);
	BoxF  border  = style->BorderSize.ToRealBox();
	BoxF  padding = style->Padding.ToRealBox();
	float top     = PosToReal(pos.Top) - border.Top - padding.Top; // why is padding in here?
	float bottom  = PosToReal(pos.Bottom) + border.Bottom + padding.Bottom;
	float left    = PosToReal(pos.Left) - border.Left - padding.Left;
	float right   = PosToReal(pos.Right) + border.Right + padding.Right;

	float contentWidth  = PosToReal(pos.Right) - PosToReal(pos.Left);
	float contentHeight = PosToReal(pos.Bottom) - PosToReal(pos.Top);

	float radius = style->BorderRadius;

	float width  = right - left;
	float height = bottom - top;
	float mindim = Min(width, height);
	mindim       = Max(mindim, 0.0f);
	radius       = Min(radius, mindim / 2);

	if (mindim <= 0)
		return;

	// Vertex ordering: 0 3
	//                  1 2

	Image* bgImage = nullptr;

	//auto bg = style.Get( CatBackground );
	//auto bgImage = style.Get( CatBackgroundImage );
	Color bg = style->BackgroundColor;
	//const char* bgImageName = Strings->GetStr(style->BackgroundImageID); -- todo

	int shaderFlags = 0;

	if (node->IsCanvas()) {
		const DomCanvas* canvas = static_cast<const DomCanvas*>(Doc->GetChildByInternalID(node->InternalID));
		bgImage                 = Images->Get(canvas->GetCanvasImageName());
	}

	if (bgImage) {
		if (LoadTexture(bgImage, TexUnit0))
			shaderFlags |= SHADER_FLAG_TEXBG;
	}

	BoxRadiusSet radii = {
	    {radius, radius}, // left, top
	    {radius, radius}, // left, bottom
	    {radius, radius}, // right, bottom
	    {radius, radius}, // right, top
	};

	bool anyArcs    = radii.TopLeft != VEC2(0, 0) || radii.TopRight != VEC2(0, 0) || radii.BottomLeft != VEC2(0, 0) || radii.BottomRight != VEC2(0, 0);
	bool anyBorders = border != BoxF(0, 0, 0, 0);

	if (bg.a != 0 || bgImage || (style->BorderColor.a != 0 && anyBorders)) {
		Vx_Uber v[16];
		float   vmid      = 0.5f * (top + bottom);
		float   borderPos = border.Top;
		// more padding, for our smooth edges
		float  vpad       = 1;
		float  hpad       = 1;
		uint32_t bgRGBA     = bg.GetRGBA();
		uint32_t borderRGBA = style->BorderColor.GetRGBA();

		// This is a little hack for rounded corners, to prevent bleeding of the border color. Ideally, we should be applying this logic to each corner individually.
		if (!anyBorders)
			borderRGBA = bgRGBA;

		float leftEdge = Max(radii.TopLeft.x, radii.BottomLeft.x);
		leftEdge       = Max(leftEdge, border.Left);

		float rightEdge = Max(radii.TopRight.x, radii.BottomRight.x);
		rightEdge       = Max(rightEdge, border.Right);

		int shader = SHADER_RECT | shaderFlags;

		auto uvScale = VEC2(1.0f / contentWidth, 1.0f / contentHeight);

		// Refer to log entry from 2015-08-19 for what A,B,C,D means here.
		float leftA   = (leftEdge - border.Left) * uvScale.x;
		float rightA  = (contentWidth - (rightEdge - border.Right)) * uvScale.x;
		float topA    = -(border.Top + vpad) * uvScale.y;
		float bottomB = 1.0f + (border.Bottom + vpad) * uvScale.y;
		auto  uv1     = VEC2(leftA, topA);
		auto  uv2     = VEC2(rightA, 0.5f);

		//                                                                                     Border width
		//                                                                                     |           Distance from edge
		//                                                                                     |           |
		// top                                                                                 |           |
		v[0].Set1(shader, VEC2(left + leftEdge, top - vpad), VEC4(border.Top, -vpad, uv1.x, uv1.y), bgRGBA, borderRGBA);
		v[1].Set1(shader, VEC2(left + leftEdge, vmid), VEC4(border.Top, vmid - top, uv1.x, uv2.y), bgRGBA, borderRGBA);
		v[2].Set1(shader, VEC2(right - rightEdge, vmid), VEC4(border.Top, vmid - top, uv2.x, uv2.y), bgRGBA, borderRGBA);
		v[3].Set1(shader, VEC2(right - rightEdge, top - vpad), VEC4(border.Top, -vpad, uv2.x, uv1.y), bgRGBA, borderRGBA);

		uv1.y = 0.5f;
		uv2.y = bottomB;

		// bottom
		v[4].Set1(shader, VEC2(left + leftEdge, vmid), VEC4(border.Bottom, bottom - vmid, uv1.x, uv1.y), bgRGBA, borderRGBA);
		v[5].Set1(shader, VEC2(left + leftEdge, bottom + vpad), VEC4(border.Bottom, -vpad, uv1.x, uv2.y), bgRGBA, borderRGBA);
		v[6].Set1(shader, VEC2(right - rightEdge, bottom + vpad), VEC4(border.Bottom, -vpad, uv2.x, uv2.y), bgRGBA, borderRGBA);
		v[7].Set1(shader, VEC2(right - rightEdge, vmid), VEC4(border.Bottom, bottom - vmid, uv2.x, uv1.y), bgRGBA, borderRGBA);

		float leftC   = (-border.Left - hpad) * uvScale.x;
		float topC    = (top + radii.TopLeft.y - PosToReal(pos.Top)) * uvScale.y;
		float bottomC = (bottom - radii.BottomLeft.y - PosToReal(pos.Top)) * uvScale.y;
		uv1           = VEC2(leftC, topC);
		uv2           = VEC2(leftA, bottomC);

		// left
		v[8].Set1(shader, VEC2(left - hpad, top + radii.TopLeft.y), VEC4(border.Left, -hpad, uv1.x, uv1.y), bgRGBA, borderRGBA);
		v[9].Set1(shader, VEC2(left - hpad, bottom - radii.BottomLeft.y), VEC4(border.Left, -hpad, uv1.x, uv2.y), bgRGBA, borderRGBA);
		v[10].Set1(shader, VEC2(left + leftEdge, bottom - radii.BottomLeft.y), VEC4(border.Left, leftEdge, uv2.x, uv2.y), bgRGBA, borderRGBA);
		v[11].Set1(shader, VEC2(left + leftEdge, top + radii.TopLeft.y), VEC4(border.Left, leftEdge, uv2.x, uv1.y), bgRGBA, borderRGBA);

		float rightD  = 1.0f + (border.Right + hpad) * uvScale.x;
		float topD    = (top + radii.TopRight.y - PosToReal(pos.Top)) * uvScale.y;
		float bottomD = (bottom - radii.BottomRight.y - PosToReal(pos.Top)) * uvScale.y;
		uv1           = VEC2(rightA, topD);
		uv2           = VEC2(rightD, bottomD);

		// right
		v[12].Set1(shader, VEC2(right - rightEdge, top + radii.TopRight.y), VEC4(border.Right, rightEdge, uv1.x, uv1.y), bgRGBA, borderRGBA);
		v[13].Set1(shader, VEC2(right - rightEdge, bottom - radii.BottomRight.y), VEC4(border.Right, rightEdge, uv1.x, uv2.y), bgRGBA, borderRGBA);
		v[14].Set1(shader, VEC2(right + hpad, bottom - radii.BottomRight.y), VEC4(border.Right, -hpad, uv2.x, uv2.y), bgRGBA, borderRGBA);
		v[15].Set1(shader, VEC2(right + hpad, top + radii.TopRight.y), VEC4(border.Right, -hpad, uv2.x, uv1.y), bgRGBA, borderRGBA);

		Driver->ActivateShader(ShaderUber);
		Driver->Draw(GPUPrimQuads, 16, v);

		if (anyArcs) {
			RenderCornerArcs(shaderFlags, TopLeft, VEC2(left, top), radii.TopLeft, VEC2(border.Left, border.Top), VEC2(leftA, topC), uvScale, bgRGBA, borderRGBA);
			RenderCornerArcs(shaderFlags, BottomLeft, VEC2(left, bottom), radii.BottomLeft, VEC2(border.Left, border.Bottom), VEC2(leftA, bottomC), uvScale, bgRGBA, borderRGBA);
			RenderCornerArcs(shaderFlags, BottomRight, VEC2(right, bottom), radii.BottomRight, VEC2(border.Right, border.Bottom), VEC2(rightA, topD), uvScale, bgRGBA, borderRGBA);
			RenderCornerArcs(shaderFlags, TopRight, VEC2(right, top), radii.TopRight, VEC2(border.Right, border.Top), VEC2(rightA, bottomD), uvScale, bgRGBA, borderRGBA);
		}
	}
}

void Renderer::RenderCornerArcs(int shaderFlags, Corners corner, Vec2f edge, Vec2f outerRadii, Vec2f borderWidth, Vec2f centerUV, Vec2f uvScale, uint32_t bgRGBA, uint32_t borderRGBA) {
	if (outerRadii.x == 0 || outerRadii.y == 0)
		return;

	Driver->ActivateShader(ShaderUber);
	float maxOuterRadius = Max(outerRadii.x, outerRadii.y);
	float fanRadius;
	int   divs;
	if (borderWidth.x == borderWidth.y && outerRadii.x == outerRadii.y)
		fanRadius = maxOuterRadius * 1.04f + 2.0f; // I haven't worked out why, but empirically 1.04 seems to be sufficient up to 200px radius.
	else
		fanRadius = maxOuterRadius * 1.1f + 2.0f;

	// It's remarkable what good results we get from approximating ellipses with just two arcs. If you make the
	// eccentricity truly extreme, then this ain't so great, but the thing is, that at those extremities, our
	// arc approximation has numerical problems, so we're screwed anyway. Right now, we just choose not to support
	// those kind of extremes. One big benefit of having a low number of subdivisions is that you avoid the problem
	// where the arc's radius becomes so small that you end up seeing the opposite side of the circle within
	// the corner. Just run the "DoBorder" example in KitchenSink, and crank divs up to 10, to see what I mean.
	// For the majority of cases, 2 arcs actually looks just fine.
	divs = 2;

	Vx_Uber vx[3];

	// we always sweep our arc counter clockwise
	float inc        = (float) (XO_PI / 2) / (float) divs;
	int   slice      = 0;
	float unit_inc   = 1.0f / (float) divs;
	float unit_slice = unit_inc;
	float th;
	bool  swapLimits   = false;
	float ellipseFlipX = 1;
	float ellipseFlipY = 1;
	Vec2f center;
	switch (corner) {
	case TopLeft:
		ellipseFlipX = -1;
		ellipseFlipY = -1;
		th           = (float) (XO_PI * 0.5);
		center       = VEC2(edge.x + outerRadii.x, edge.y + outerRadii.y);
		break;
	case BottomLeft:
		ellipseFlipX = -1;
		th           = (float) (XO_PI * 1.0f);
		swapLimits   = true;
		center       = VEC2(edge.x + outerRadii.x, edge.y - outerRadii.y);
		break;
	case BottomRight:
		th     = (float) (XO_PI * -0.5f);
		center = VEC2(edge.x - outerRadii.x, edge.y - outerRadii.y);
		break;
	case TopRight:
		ellipseFlipY = -1;
		th           = 0;
		swapLimits   = true;
		center       = VEC2(edge.x - outerRadii.x, edge.y + outerRadii.y);
		break;
	}
	auto innerRadii = outerRadii - VEC2(borderWidth.x, borderWidth.y);
	innerRadii.x    = fabs(innerRadii.x);
	innerRadii.y    = fabs(innerRadii.y);
	auto fanPos     = VEC2(center.x + fanRadius * cos(th), center.y - fanRadius * sin(th)); // -y, because our coord system is Y down
	auto fanUV      = centerUV + uvScale * (fanPos - center);

	auto outerPos = VEC2(center.x + outerRadii.x * cos(th), center.y - outerRadii.y * sin(th));
	auto innerPos = center + PtOnEllipse(ellipseFlipX, ellipseFlipY, innerRadii.x, innerRadii.y, th);
	th += inc;
	float hinc = inc * 0.5f;

	// TODO: dynamically pick the cheaper cos/sin for circular arcs, or tan for ellipses
	// ie pick between two functions - PtOnEllipse, and PtOnCircle.

	for (; slice < divs; th += inc, unit_slice += unit_inc, slice++) {
		auto fanPosNext   = VEC2(center.x + fanRadius * cos(th), center.y - fanRadius * sin(th));
		auto outerPosNext = VEC2(center.x + outerRadii.x * cos(th), center.y - outerRadii.y * sin(th));
		auto innerPosNext = center + PtOnEllipse(ellipseFlipX, ellipseFlipY, innerRadii.x, innerRadii.y, th);
		auto outerPosHalf = VEC2(center.x + outerRadii.x * cos(th - hinc), center.y - outerRadii.y * sin(th - hinc));
		auto innerPosHalf = center + PtOnEllipse(ellipseFlipX, ellipseFlipY, innerRadii.x, innerRadii.y, th - hinc);
		auto fanUVNext    = centerUV + uvScale * (fanPosNext - center);

		Vec2f innerCenter, outerCenter;
		float innerRadius, outerRadius;
		CircleFrom3Pt(outerPos, outerPosHalf, outerPosNext, outerCenter, outerRadius);
		CircleFrom3Pt(innerPos, innerPosHalf, innerPosNext, innerCenter, innerRadius);

		auto arcCenters = VEC4(innerCenter.x, innerCenter.y, outerCenter.x, outerCenter.y);
		auto arcRadii   = VEC2(innerRadius, outerRadius);

		vx[0].Set(SHADER_ARC | shaderFlags, center, arcCenters, VEC4(arcRadii.x, arcRadii.y, centerUV.x, centerUV.y), bgRGBA, borderRGBA);
		vx[1].Set(SHADER_ARC | shaderFlags, fanPos, arcCenters, VEC4(arcRadii.x, arcRadii.y, fanUV.x, fanUV.y), bgRGBA, borderRGBA);
		vx[2].Set(SHADER_ARC | shaderFlags, fanPosNext, arcCenters, VEC4(arcRadii.x, arcRadii.y, fanUVNext.x, fanUVNext.y), bgRGBA, borderRGBA);
		Driver->Draw(GPUPrimTriangles, 3, vx);
		fanPos   = fanPosNext;
		outerPos = outerPosNext;
		innerPos = innerPosNext;
		fanUV    = fanUVNext;
	}
}

void Renderer::RenderQuadratic(Point base, const RenderDomNode* node) {
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

	Vx_PTCV4 corners[50];
	Vec2f    uv[3];
	uv[0] = VEC2(0, 0);
	uv[1] = VEC2(0.5, 0);
	uv[2] = VEC2(1, 1);
	Vec3f basef(PosToReal(base.X), PosToReal(base.Y), 0);

	basef = Vec3f(180, 30, 0) + 0.02f * basef;

	for (int i = 0; i < arraysize(vx); i++) {
		Vec3f p(vx[i * 2], 5 - vx[i * 2 + 1], 0);
		p                = 30.0f * p;
		corners[i].Pos   = basef + p;
		corners[i].UV    = uv[i % 3];
		corners[i].Color = i < 3 ? Color::RGBA(250, 50, 50, 255).GetRGBA() : Color::RGBA(50, 50, 250, 255).GetRGBA();
		corners[i].V4.x  = -1;
	}

	Driver->ActivateShader(ShaderQuadraticSpline);
	Driver->Draw(GPUPrimTriangles, 12, corners);
}

void Renderer::RenderText(Point base, const RenderDomText* node) {
	bool subPixelGlyphs = node->Flags & RenderDomText::FlagSubPixelGlyphs;
	for (size_t i = 0; i < node->Text.size(); i++) {
		if (subPixelGlyphs)
			RenderTextChar_SubPixel(base, node, node->Text[i]);
		else
			RenderTextChar_WholePixel(base, node, node->Text[i]);
	}
}

void Renderer::RenderTextChar_SubPixel(Point base, const RenderDomText* node, const RenderCharEl& txtEl) {
	GlyphCacheKey glyphKey(node->FontID, txtEl.Char, node->FontSizePx, GlyphFlag_SubPixel_RGB);
	const Glyph*  glyph = Global()->GlyphCache->GetGlyph(glyphKey);
	if (!glyph) {
		GlyphsNeeded.insert(glyphKey);
		return;
	}

	TextureAtlas* atlas       = Global()->GlyphCache->GetAtlasMutable(glyph->AtlasID);
	float         atlasScaleX = 1.0f / atlas->GetWidth();
	float         atlasScaleY = 1.0f / atlas->GetHeight();

	float top  = PosToReal(PosRound(base.Y + txtEl.Y));
	float left = PosToReal(base.X + txtEl.X);

	// Our glyph has a single column on the left and right side, so that our clamped texture
	// reads will pickup a zero when reading off beyond the edge of the glyph
	int horzPad        = 1;
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
	//if ( Global()->SnapSubpixelHorzText )
	//	left = floor(left + 0.5f);

	float right  = left + roundedWidth;
	float bottom = top + glyph->Height;

	// We have to 'overdraw' on the left and right by 1 pixel, to ensure that we filter over the edges.
	int overdraw = 1;
	left -= overdraw;
	right += overdraw;

	Vx_Uber corners[4];
	corners[0].Pos = VEC2(left, top);
	corners[1].Pos = VEC2(left, bottom);
	corners[2].Pos = VEC2(right, bottom);
	corners[3].Pos = VEC2(right, top);

	float u0 = (glyph->X + horzPad - overdraw * 3) * atlasScaleX;
	float v0 = glyph->Y * atlasScaleY;
	float u1 = (glyph->X + horzPad + (roundedWidth + overdraw) * 3) * atlasScaleX;
	float v1 = (glyph->Y + glyph->Height) * atlasScaleY;

	corners[0].UV1 = VEC4(u0, v0, 0, 0);
	corners[1].UV1 = VEC4(u0, v1, 0, 0);
	corners[2].UV1 = VEC4(u1, v1, 0, 0);
	corners[3].UV1 = VEC4(u1, v0, 0, 0);

	// Obviously our clamping is not affected by overdraw. It remains our absolute texel limits.
	Vec4f clamp;
	clamp.x = (glyph->X + 0.5f) * atlasScaleX;
	clamp.y = (glyph->Y + 0.5f) * atlasScaleY;
	clamp.z = (glyph->X + glyph->Width - 0.5f) * atlasScaleX;
	clamp.w = (glyph->Y + glyph->Height - 0.5f) * atlasScaleY;

	uint32_t color = node->Color.GetRGBA();

	for (int i = 0; i < 4; i++) {
		corners[i].Color1 = color;
		corners[i].Color2 = 0;
		corners[i].UV2    = clamp;
		corners[i].Shader = SHADER_TEXT_SUBPIXEL;
	}

	Driver->ActivateShader(ShaderUber);
	//Driver->ActivateShader(ShaderTextRGB);
	if (!LoadTexture(atlas, TexUnit0))
		return;
	Driver->Draw(GPUPrimQuads, 4, corners);
}

void Renderer::RenderTextChar_WholePixel(Point base, const RenderDomText* node, const RenderCharEl& txtEl) {
	GlyphCacheKey glyphKey(node->FontID, txtEl.Char, node->FontSizePx, 0);
	const Glyph*  glyph = Global()->GlyphCache->GetGlyph(glyphKey);
	if (!glyph) {
		GlyphsNeeded.insert(glyphKey);
		return;
	}

	TextureAtlas* atlas       = Global()->GlyphCache->GetAtlasMutable(glyph->AtlasID);
	float         atlasScaleX = 1.0f / atlas->GetWidth();
	float         atlasScaleY = 1.0f / atlas->GetHeight();

	float top  = PosToReal(base.Y + txtEl.Y);
	float left = PosToReal(base.X + txtEl.X);

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
	if (snapToWholePixels) {
		left = (float) floor(left + 0.5f);
		top  = (float) floor(top + 0.5f);
	}

	left -= pad;
	top -= pad;

	float right  = left + glyph->Width + pad * 2;
	float bottom = top + glyph->Height + pad * 2;

	Vx_Uber corners[4];
	corners[0].Pos = VEC2(left, top);
	corners[1].Pos = VEC2(left, bottom);
	corners[2].Pos = VEC2(right, bottom);
	corners[3].Pos = VEC2(right, top);

	float u0 = (glyph->X - pad) * atlasScaleX;
	float v0 = (glyph->Y - pad) * atlasScaleY;
	float u1 = (glyph->X + glyph->Width + pad) * atlasScaleX;
	float v1 = (glyph->Y + glyph->Height + pad) * atlasScaleY;

	corners[0].UV1 = VEC4(u0, v0, 0, 0);
	corners[1].UV1 = VEC4(u0, v1, 0, 0);
	corners[2].UV1 = VEC4(u1, v1, 0, 0);
	corners[3].UV1 = VEC4(u1, v0, 0, 0);

	uint32_t color = node->Color.GetRGBA();

	for (int i = 0; i < 4; i++) {
		corners[i].UV2    = VEC4(0, 0, 0, 0);
		corners[i].Color1 = color;
		corners[i].Shader = SHADER_TEXT_SIMPLE;
	}

	//Driver->ActivateShader(ShaderTextWhole);
	Driver->ActivateShader(ShaderUber);
	if (!LoadTexture(atlas, TexUnit0))
		return;
	Driver->Draw(GPUPrimQuads, 4, corners);
}

void Renderer::RenderGlyphsNeeded() {
	for (auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++)
		Global()->GlyphCache->RenderGlyph(*it);
	GlyphsNeeded.clear();
}

bool Renderer::LoadTexture(Texture* tex, TexUnits texUnit) {
	if (!Driver->LoadTexture(tex, texUnit))
		return false;

	tex->TexClearInvalidRect();
	return true;
}

float Renderer::CircleFrom3Pt(const Vec2f& a, const Vec2f& b, const Vec2f& c, Vec2f& center, float& radius) {
	float A = b.x - a.x;
	float B = b.y - a.y;
	float C = c.x - a.x;
	float D = c.y - a.y;

	float E = A * (a.x + b.x) + B * (a.y + b.y);
	float F = C * (a.x + c.x) + D * (a.y + c.y);

	float G = 2 * (A * (c.y - b.y) - B * (c.x - b.x));

	if (G == 0) {
		// Pick a point far away. We happen to know what an OK value for "far away" is.
		Vec2f line = (c - a).normalized();
		Vec2f perp(-line.y, line.x);
		radius = 100.0f;
		center = b + radius * perp;
	} else {
		center.x = (D * E - B * F) / G;
		center.y = (A * F - C * E) / G;
		radius   = sqrt((a.x - center.x) * (a.x - center.x) + (a.y - center.y) * (a.y - center.y));
	}
	return G;
}

Vec2f Renderer::PtOnEllipse(float flipX, float flipY, float a, float b, float theta) {
	float x = (a * b) / sqrt(b * b + a * a * powf(tan(theta), 2.0f));
	float y = (a * b) / sqrt(a * a + b * b / powf(tan(theta), 2.0f));
	return Vec2f(flipX * x, flipY * y);
};
}
