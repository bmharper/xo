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

RenderResult Renderer::Render(const xo::Doc* doc, ImageStore* images, StringTable* strings, RenderBase* driver, const RenderDomNode* root) {
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

	return needGlyphs ? RenderResultNeedMore : RenderResultDone;
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

	enum {
		Left,
		Top,
		Right,
		Bottom,
	};

	const StyleRender* style = &node->Style;
	Box                pos   = node->Pos;
	pos.Offset(base);
	BoxF  border  = style->BorderSize.ToRealBox();
	BoxF  padding = style->Padding.ToRealBox();
	float top     = PosToReal(pos.Top) - border.Top - padding.Top; // why is padding in here?
	float bottom  = PosToReal(pos.Bottom) + border.Bottom + padding.Bottom;
	float left    = PosToReal(pos.Left) - border.Left - padding.Left;
	float right   = PosToReal(pos.Right) + border.Right + padding.Right;

	if (padding.Top < 0)
		int abc = 123;

	float contentWidth  = PosToReal(pos.Right) - PosToReal(pos.Left);
	float contentHeight = PosToReal(pos.Bottom) - PosToReal(pos.Top);

	//float radius = style->BorderRadius;

	float width      = right - left;
	float height     = bottom - top;
	float mindim     = Min(width, height);
	mindim           = Max(mindim, 0.0f);
	float halfMinDim = mindim * 0.5f;
	//radius       = Min(radius, mindim / 2);

	if (mindim <= 0)
		return;

	// Vertex ordering: 0 3
	//                  1 2

	Image* bgImage = nullptr;

	//auto bg = style.Get( CatBackground );
	//auto bgImage = style.Get( CatBackgroundImage );
	Color bg = style->BackgroundColor;
	//const char* bgImageName = Strings->GetStr(style->BackgroundImageID); -- todo
	if (bg == Color(0xdd, 0x55, 0x55, 0xff))
		int abc = 123;

	int shaderFlags = 0;

	if (node->IsCanvas()) {
		const DomCanvas* canvas = static_cast<const DomCanvas*>(Doc->GetChildByInternalID(node->InternalID));
		bgImage                 = Images->Get(canvas->GetCanvasImageName());
	}

	if (bgImage) {
		if (LoadTexture(bgImage, TexUnit0))
			shaderFlags |= SHADER_FLAG_TEXBG;
	}

	BoxF   rr  = style->BorderRadius.ToRealBox2BitPrecision();
	float* rad = (float*) &rr;
	for (int i = 0; i < 4; i++)
		//rad[i] = Min(rad[i], halfMinDim);
		rad[i] = Min(rad[i], mindim); // TODO: if the sum of neighbouring radii exceed mindim, them shrink them proportionally so that they fit
	BoxRadiusSet radii;
	radii.TopLeft     = {rad[0], rad[0]};
	radii.TopRight    = {rad[1], rad[1]};
	radii.BottomRight = {rad[2], rad[2]};
	radii.BottomLeft  = {rad[3], rad[3]};

	bool anyArcs    = radii.TopLeft != VEC2(0, 0) || radii.TopRight != VEC2(0, 0) || radii.BottomLeft != VEC2(0, 0) || radii.BottomRight != VEC2(0, 0);
	bool anyBorders = (border.Left != 0 && style->BorderColor[Left].a != 0) ||
	                  (border.Top != 0 && style->BorderColor[Top].a != 0) ||
	                  (border.Right != 0 && style->BorderColor[Right].a != 0) ||
	                  (border.Bottom != 0 && style->BorderColor[Bottom].a != 0);

	if (bg.a != 0 || bgImage || anyBorders) {
		Vx_Uber vx[48];
		float   vmid      = 0.5f * (top + bottom);
		float   borderPos = border.Top;
		// more padding, for our smooth edges
		float    vpad = 1;
		float    hpad = 1;
		uint32_t borderRGBA[4];
		for (int i = 0; i < 4; i++)
			borderRGBA[i] = style->BorderColor[i].GetRGBA();
		uint32_t bgRGBA = bg.GetRGBA();
		uint32_t red    = xo::RGBA::Make(200, 0, 0, 220).u;
		uint32_t dgreen = xo::RGBA::Make(0, 100, 0, 220).u;
		uint32_t green  = xo::RGBA::Make(0, 200, 0, 220).u;
		uint32_t purple = xo::RGBA::Make(150, 0, 200, 220).u;

		float leftEdgeTopWidth     = Max(radii.TopLeft.x, border.Left);
		float leftEdgeBottomWidth  = Max(radii.BottomLeft.x, border.Left);
		float rightEdgeTopWidth    = Max(radii.TopRight.x, border.Right);
		float rightEdgeBottomWidth = Max(radii.BottomRight.x, border.Right);

		float leftEdgeMax  = Max(leftEdgeTopWidth, leftEdgeBottomWidth);
		float rightEdgeMax = Max(rightEdgeTopWidth, rightEdgeBottomWidth);
		float leftEdgeMin  = Min(leftEdgeTopWidth, leftEdgeBottomWidth);
		float rightEdgeMin = Min(rightEdgeTopWidth, rightEdgeBottomWidth);

		int shader = SHADER_RECT | shaderFlags;

		const float infinitelyThickBorder = 4096; // This constant is just a thumbsuck - unit is in pixels.

		auto uvScale = VEC2(1.0f / contentWidth, 1.0f / contentHeight);

		// Refer to log entry from 2017-01-24 for what A,B,C1,C2,D,E means here, as well as x1..x8 and y1..y4

		float x[8];
		float u[8];
		float y[7];
		float v[7];

		// left (start at the left edge and work our way in)
		x[0] = left - hpad;
		x[1] = left + radii.TopLeft.x;
		x[2] = left + leftEdgeMax;
		x[3] = right - rightEdgeMax;
		x[4] = right - radii.TopRight.x;
		x[5] = right + hpad;
		x[6] = left + radii.BottomLeft.x;
		x[7] = right - radii.BottomRight.x;

		float uZero = left + border.Left; // left edge of content-box
		for (int i = 0; i < arraysize(x); i++)
			u[i] = uvScale.x * (x[i] - uZero);

		for (int i = 1; i <= 5; i++)
			XO_DEBUG_ASSERT(x[i] >= x[i - 1]);
		XO_DEBUG_ASSERT(x[7] >= x[6]);

		// top
		y[0] = top - vpad;
		y[1] = top + Max(radii.TopLeft.y, border.Top);
		y[2] = bottom - Max(radii.BottomLeft.y, border.Bottom);
		y[3] = bottom + vpad;
		y[4] = top + Max(radii.TopRight.y, border.Top);
		y[5] = bottom - Max(radii.BottomRight.y, border.Bottom);
		y[6] = 0.5f * (top + bottom);

		float vZero = top + border.Top; // top edge of content-box
		for (int i = 0; i < arraysize(y); i++)
			v[i] = uvScale.y * (y[i] - vZero);

		for (int i = 1; i <= 3; i++)
			XO_DEBUG_ASSERT(y[i] >= y[i - 1]);
		XO_DEBUG_ASSERT(y[5] >= y[4]);

		int c = 0;
		// C1
		//                                          Border width
		//                                          |           Distance from edge
		//                                          |           |
		vx[c++].Set1(shader, VEC2(x[2], y[0]), VEC4(border.Top, -vpad, u[2], v[0]), bgRGBA, borderRGBA[Top]);
		vx[c++].Set1(shader, VEC2(x[2], y[6]), VEC4(border.Top, y[6] - top, u[2], v[6]), bgRGBA, borderRGBA[Top]);
		vx[c++].Set1(shader, VEC2(x[3], y[6]), VEC4(border.Top, y[6] - top, u[3], v[6]), bgRGBA, borderRGBA[Top]);
		vx[c++].Set1(shader, VEC2(x[3], y[0]), VEC4(border.Top, -vpad, u[3], v[0]), bgRGBA, borderRGBA[Top]);

		// C2
		vx[c++].Set1(shader, VEC2(x[2], y[6]), VEC4(border.Bottom, bottom - y[6], u[2], v[6]), bgRGBA, borderRGBA[Bottom]);
		vx[c++].Set1(shader, VEC2(x[2], y[3]), VEC4(border.Bottom, -vpad, u[2], v[3]), bgRGBA, borderRGBA[Bottom]);
		vx[c++].Set1(shader, VEC2(x[3], y[3]), VEC4(border.Bottom, -vpad, u[3], v[3]), bgRGBA, borderRGBA[Bottom]);
		vx[c++].Set1(shader, VEC2(x[3], y[6]), VEC4(border.Bottom, bottom - y[6], u[3], v[6]), bgRGBA, borderRGBA[Bottom]);

		// A
		if (x[2] - x[0] != 0 && y[2] - y[1] != 0) {
			vx[c++].Set1(shader, VEC2(x[0], y[1]), VEC4(border.Left, -hpad, u[0], v[1]), bgRGBA, borderRGBA[Left]);
			vx[c++].Set1(shader, VEC2(x[0], y[2]), VEC4(border.Left, -hpad, u[0], v[2]), bgRGBA, borderRGBA[Left]);
			vx[c++].Set1(shader, VEC2(x[2], y[2]), VEC4(border.Left, x[2] - left, u[2], v[2]), bgRGBA, borderRGBA[Left]);
			vx[c++].Set1(shader, VEC2(x[2], y[1]), VEC4(border.Left, x[2] - left, u[2], v[1]), bgRGBA, borderRGBA[Left]);
		}

		// B
		if (x[5] - x[3] != 0 && y[5] - y[4] != 0) {
			vx[c++].Set1(shader, VEC2(x[3], y[4]), VEC4(border.Right, right - x[3], u[3], v[4]), bgRGBA, borderRGBA[Right]);
			vx[c++].Set1(shader, VEC2(x[3], y[5]), VEC4(border.Right, right - x[3], u[3], v[5]), bgRGBA, borderRGBA[Right]);
			vx[c++].Set1(shader, VEC2(x[5], y[5]), VEC4(border.Right, -hpad, u[5], v[5]), bgRGBA, borderRGBA[Right]);
			vx[c++].Set1(shader, VEC2(x[5], y[4]), VEC4(border.Right, -hpad, u[5], v[4]), bgRGBA, borderRGBA[Right]);
		}

		// Top D
		if (x[2] - x[1] != 0 && y[1] - y[0] != 0) {
			// If the corner arc is completely contained within the border, then everything that we paint is
			// the border color. We detect that condition here, and alter the 'distance to edge' value so that
			// the entire rectangle is rendered as "border". My first approach here was to alter the background
			// color here, so that it was the same as the border color, but that doesn't work when our background
			// is a texture, because in that case 'bgRGBA' is ignored.
			// If you comment out the special condition here, and always use 'border.Top', then you see this in action with
			// a box of the form "border: 20px 2px 20px 2px; border-radius: 5px". ie thick left/right borders, and thin
			// top/bottom borders.
			float borderWidth = radii.TopLeft.x < border.Left ? infinitelyThickBorder : border.Top;
			vx[c++].Set1(shader, VEC2(x[1], y[0]), VEC4(borderWidth, -vpad, u[6], v[2]), bgRGBA, borderRGBA[Top]);
			vx[c++].Set1(shader, VEC2(x[1], y[1]), VEC4(borderWidth, y[1] - top, u[6], v[3]), bgRGBA, borderRGBA[Top]);
			vx[c++].Set1(shader, VEC2(x[2], y[1]), VEC4(borderWidth, y[1] - top, u[2], v[3]), bgRGBA, borderRGBA[Top]);
			vx[c++].Set1(shader, VEC2(x[2], y[0]), VEC4(borderWidth, -vpad, u[2], v[2]), bgRGBA, borderRGBA[Top]);
		}

		// Bottom D
		if (x[2] - x[6] != 0 && y[3] - y[2] != 0) {
			float borderWidth = radii.BottomLeft.x < border.Left ? infinitelyThickBorder : border.Bottom;
			vx[c++].Set1(shader, VEC2(x[6], y[2]), VEC4(borderWidth, bottom - y[2], u[6], v[2]), bgRGBA, borderRGBA[Bottom]);
			vx[c++].Set1(shader, VEC2(x[6], y[3]), VEC4(borderWidth, -vpad, u[6], v[3]), bgRGBA, borderRGBA[Bottom]);
			vx[c++].Set1(shader, VEC2(x[2], y[3]), VEC4(borderWidth, -vpad, u[2], v[3]), bgRGBA, borderRGBA[Bottom]);
			vx[c++].Set1(shader, VEC2(x[2], y[2]), VEC4(borderWidth, bottom - y[2], u[2], v[2]), bgRGBA, borderRGBA[Bottom]);
		}

		// Top E
		if (x[4] - x[3] != 0 && y[4] - y[0] != 0) {
			float borderWidth = radii.TopRight.x < border.Right ? infinitelyThickBorder : border.Top;
			vx[c++].Set1(shader, VEC2(x[3], y[0]), VEC4(borderWidth, -vpad, u[3], v[0]), bgRGBA, borderRGBA[Top]);
			vx[c++].Set1(shader, VEC2(x[3], y[4]), VEC4(borderWidth, y[4] - top, u[3], v[4]), bgRGBA, borderRGBA[Top]);
			vx[c++].Set1(shader, VEC2(x[4], y[4]), VEC4(borderWidth, y[4] - top, u[4], v[4]), bgRGBA, borderRGBA[Top]);
			vx[c++].Set1(shader, VEC2(x[4], y[0]), VEC4(borderWidth, -vpad, u[4], v[0]), bgRGBA, borderRGBA[Top]);
		}

		// Bottom E
		if (x[7] - x[3] != 0 && y[3] - y[2] != 0) {
			float borderWidth = radii.BottomRight.x < border.Right ? infinitelyThickBorder : border.Bottom;
			vx[c++].Set1(shader, VEC2(x[3], y[5]), VEC4(borderWidth, bottom - y[5], u[3], v[5]), bgRGBA, borderRGBA[Bottom]);
			vx[c++].Set1(shader, VEC2(x[3], y[3]), VEC4(borderWidth, -vpad, u[3], v[3]), bgRGBA, borderRGBA[Bottom]);
			vx[c++].Set1(shader, VEC2(x[7], y[3]), VEC4(borderWidth, -vpad, u[7], v[3]), bgRGBA, borderRGBA[Bottom]);
			vx[c++].Set1(shader, VEC2(x[7], y[5]), VEC4(borderWidth, bottom - y[5], u[7], v[5]), bgRGBA, borderRGBA[Bottom]);
		}

		// Slivers (described at the bottom of log entry).
		// For the slivers, we ignore the UV coords, because the entire sliver is contained inside the border,
		// so the UVs shouldn't matter.

		// top left sliver
		if (border.Top > radii.TopLeft.y) {
			float height = border.Top - radii.TopLeft.y;
			vx[c++].Set1(shader, VEC2(x[0], y[1] - height), VEC4(infinitelyThickBorder, -hpad, u[0], v[0]), bgRGBA, borderRGBA[Left]);
			vx[c++].Set1(shader, VEC2(x[0], y[1]), VEC4(infinitelyThickBorder, -hpad, u[0], v[0]), bgRGBA, borderRGBA[Left]);
			vx[c++].Set1(shader, VEC2(x[1], y[1]), VEC4(infinitelyThickBorder, x[1] - left, u[0], v[0]), bgRGBA, borderRGBA[Left]);
			vx[c++].Set1(shader, VEC2(x[1], y[1] - height), VEC4(infinitelyThickBorder, x[1] - left, u[0], v[0]), bgRGBA, borderRGBA[Left]);
		}

		// bottom left sliver
		if (border.Bottom > radii.BottomLeft.y) {
			float height = border.Bottom - radii.BottomLeft.y;
			vx[c++].Set1(shader, VEC2(x[0], y[2]), VEC4(infinitelyThickBorder, -hpad, u[0], v[0]), bgRGBA, borderRGBA[Left]);
			vx[c++].Set1(shader, VEC2(x[0], y[2] + height), VEC4(infinitelyThickBorder, -hpad, u[0], v[0]), bgRGBA, borderRGBA[Left]);
			vx[c++].Set1(shader, VEC2(x[6], y[2] + height), VEC4(infinitelyThickBorder, x[6] - left, u[0], v[0]), bgRGBA, borderRGBA[Left]);
			vx[c++].Set1(shader, VEC2(x[6], y[2]), VEC4(infinitelyThickBorder, x[6] - left, u[0], v[0]), bgRGBA, borderRGBA[Left]);
		}

		// top right sliver
		if (border.Top > radii.TopRight.y) {
			float height = border.Top - radii.TopRight.y;
			vx[c++].Set1(shader, VEC2(x[4], y[4] - height), VEC4(infinitelyThickBorder, right - x[4], u[0], v[0]), bgRGBA, borderRGBA[Right]);
			vx[c++].Set1(shader, VEC2(x[4], y[4]), VEC4(infinitelyThickBorder, right - x[4], u[0], v[0]), bgRGBA, borderRGBA[Right]);
			vx[c++].Set1(shader, VEC2(x[5], y[4]), VEC4(infinitelyThickBorder, -hpad, u[0], v[0]), bgRGBA, borderRGBA[Right]);
			vx[c++].Set1(shader, VEC2(x[5], y[4] - height), VEC4(infinitelyThickBorder, -hpad, u[0], v[0]), bgRGBA, borderRGBA[Right]);
		}

		// bottom right sliver
		if (border.Bottom > radii.BottomRight.y) {
			float height = border.Bottom - radii.BottomRight.y;
			vx[c++].Set1(shader, VEC2(x[7], y[5]), VEC4(infinitelyThickBorder, right - x[7], u[0], v[0]), bgRGBA, borderRGBA[Right]);
			vx[c++].Set1(shader, VEC2(x[7], y[5] + height), VEC4(infinitelyThickBorder, right - x[7], u[0], v[0]), bgRGBA, borderRGBA[Right]);
			vx[c++].Set1(shader, VEC2(x[5], y[5] + height), VEC4(infinitelyThickBorder, -hpad, u[0], v[0]), bgRGBA, borderRGBA[Right]);
			vx[c++].Set1(shader, VEC2(x[5], y[5]), VEC4(infinitelyThickBorder, -hpad, u[0], v[0]), bgRGBA, borderRGBA[Right]);
		}

		Driver->ActivateShader(ShaderUber);
		Driver->Draw(GPUPrimQuads, c, vx);

		if (anyArcs) {
			// TODO: Fade between adjacent border colors
			RenderCornerArcs(shaderFlags, TopLeft, VEC2(left, top), radii.TopLeft, VEC2(border.Left, border.Top), VEC2(u[1], v[1]), uvScale, bgRGBA, borderRGBA[Left]);
			RenderCornerArcs(shaderFlags, BottomLeft, VEC2(left, bottom), radii.BottomLeft, VEC2(border.Left, border.Bottom), VEC2(u[6], v[2]), uvScale, bgRGBA, borderRGBA[Left]);
			RenderCornerArcs(shaderFlags, BottomRight, VEC2(right, bottom), radii.BottomRight, VEC2(border.Right, border.Bottom), VEC2(u[4], v[5]), uvScale, bgRGBA, borderRGBA[Right]);
			RenderCornerArcs(shaderFlags, TopRight, VEC2(right, top), radii.TopRight, VEC2(border.Right, border.Top), VEC2(u[4], v[4]), uvScale, bgRGBA, borderRGBA[Right]);
		}
	}
}

void Renderer::RenderCornerArcs(int shaderFlags, Corners corner, Vec2f edge, Vec2f outerRadii, Vec2f borderWidth, Vec2f centerUV, Vec2f uvScale, uint32_t bgRGBA, uint32_t borderRGBA) {
	if (outerRadii.x == 0 || outerRadii.y == 0)
		return;

	// I don't know how to get rid of this tweak
	if (borderWidth.x == 0 && borderWidth.y == 0)
		borderRGBA = bgRGBA;

	Driver->ActivateShader(ShaderUber);
	float maxOuterRadius = Max(outerRadii.x, outerRadii.y);
	float fanRadius;
	int   divs;
	if (borderWidth.x == borderWidth.y && outerRadii.x == outerRadii.y)
		fanRadius = maxOuterRadius * 1.07f + 2.0f; // I haven't worked out why, but empirically this value seems to be sufficient up to 200px radius.
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
	//innerRadii.x    = fabs(innerRadii.x);
	//innerRadii.y    = fabs(innerRadii.y);

	// When a border is larger than a radius, then the entire arc is enclosed within
	// that fat border. An inner radius of less than zero indicates this case.
	// Our functions such as PtOnEllipse have special cases in them for dealing with zero radii.
	//if (borderWidth.x >= outerRadii.x || borderWidth.y >= outerRadii.y)
	if (innerRadii.x <= 0 || innerRadii.y <= 0)
		innerRadii = VEC2(0, 0);

	auto fanPos = VEC2(center.x + fanRadius * cos(th), center.y - fanRadius * sin(th)); // -y, because our coord system is Y down
	auto fanUV  = centerUV + uvScale * (fanPos - center);

	auto outerPos = VEC2(center.x + outerRadii.x * cos(th), center.y - outerRadii.y * sin(th));
	auto innerPos = center + PtOnEllipse(ellipseFlipX, ellipseFlipY, innerRadii.x, innerRadii.y, th);
	th += inc;
	float hinc = inc * 0.5f;

	// TODO: dynamically pick the cheaper cos/sin for circular arcs, or tan for ellipses
	// ie pick between two functions - PtOnEllipse, and PtOnCircle.
	bool zeroInnerRadius = innerRadii.x * innerRadii.y == 0;

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
		if (zeroInnerRadius) {
			innerCenter = innerPos;
			innerRadius = 0;
		} else {
			CircleFrom3Pt(innerPos, innerPosHalf, innerPosNext, innerCenter, innerRadius);
		}

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
		if (node->Text[i].Char == 32)
			continue;
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
	//if ( Global()->SnapHorzText )
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
	if (a * b == 0)
		return Vec2f(0, 0);
	float x = (a * b) / sqrt(b * b + a * a * powf(tan(theta), 2.0f));
	float y = (a * b) / sqrt(a * a + b * b / powf(tan(theta), 2.0f));
	return Vec2f(flipX * x, flipY * y);
};
}
