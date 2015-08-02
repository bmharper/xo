#pragma once

#include "../xoDefs.h"
#include "../Text/xoGlyphCache.h"

/* An instance of this is created for each render.
Any state that is persisted between renderings is stored in xoRenderGL.
*/
class XOAPI xoRenderer
{
public:
	// I initially tried to not pass xoDoc in here, but I eventually needed it to lookup canvas objects
	xoRenderResult	Render(const xoDoc* doc, xoImageStore* images, xoStringTable* strings, xoRenderBase* driver, const xoRenderDomNode* root);

protected:
	enum TexUnits
	{
		TexUnit0 = 0,
	};
	const xoDoc*				Doc;
	xoRenderBase*				Driver;
	xoImageStore*				Images;
	xoStringTable*				Strings;
	fhashset<xoGlyphCacheKey>	GlyphsNeeded;

	void			RenderEl(xoPoint base, const xoRenderDomEl* node);
	void			RenderNode(xoPoint base, const xoRenderDomNode* node);
	void			RenderRect2(float left, float right, float top, float bottom, xoVx_PTC* corners, const xoStyleRender* style, float radius, xoBoxF& border);
	void			RenderQuadratic(xoPoint base, const xoRenderDomNode* node);
	void			RenderText(xoPoint base, const xoRenderDomText* node);
	void			RenderTextChar_WholePixel(xoPoint base, const xoRenderDomText* node, const xoRenderCharEl& txtEl);
	void			RenderTextChar_SubPixel(xoPoint base, const xoRenderDomText* node, const xoRenderCharEl& txtEl);
	void			RenderGlyphsNeeded();

	bool			LoadTexture(xoTexture* tex, TexUnits texUnit);		// Load a texture and reset invalid rectangle

};