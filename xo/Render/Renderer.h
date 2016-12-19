#pragma once
#include "../Defs.h"
#include "../Text/GlyphCache.h"

namespace xo {

/* An instance of this is created for each render.
Any state that is persisted between renderings is stored in RenderGL.
*/
class XO_API Renderer {
public:
	// I initially tried to not pass Doc in here, but I eventually needed it to lookup canvas objects
	RenderResult Render(const xo::Doc* doc, ImageStore* images, StringTable* strings, RenderBase* driver, const RenderDomNode* root);

protected:
	enum TexUnits {
		TexUnit0 = 0,
	};
	enum Corners {
		TopLeft,
		BottomLeft,
		BottomRight,
		TopRight,
	};
	const Doc*                Doc;
	RenderBase*               Driver;
	ImageStore*               Images;
	StringTable*              Strings;
	ohash::set<GlyphCacheKey> GlyphsNeeded;

	void RenderEl(Point base, const RenderDomEl* node);
	void RenderNode(Point base, const RenderDomNode* node);
	void RenderCornerArcs(int shaderFlags, Corners corner, Vec2f edge, Vec2f outerRadii, Vec2f borderWidth, Vec2f centerUV, Vec2f uvScale, uint32_t bgRGBA, uint32_t borderRGBA);
	void RenderQuadratic(Point base, const RenderDomNode* node);
	void RenderText(Point base, const RenderDomText* node);
	void RenderTextChar_WholePixel(Point base, const RenderDomText* node, const RenderCharEl& txtEl);
	void RenderTextChar_SubPixel(Point base, const RenderDomText* node, const RenderCharEl& txtEl);
	void RenderGlyphsNeeded();

	bool         LoadTexture(Texture* tex, TexUnits texUnit); // Load a texture and reset invalid rectangle
	static float CircleFrom3Pt(const Vec2f& a, const Vec2f& b, const Vec2f& c, Vec2f& center, float& radius);
	static Vec2f PtOnEllipse(float flipX, float flipY, float a, float b, float theta);
};
}
