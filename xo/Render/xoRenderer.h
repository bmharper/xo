#pragma once

#include "xoDefs.h"
#include "../Text/xoGlyphCache.h"

/* An instance of this is created for each render.
Any state that is persisted between renderings is stored in xoRenderGL.
*/
class XOAPI xoRenderer
{
public:
	xoRenderResult	Render( xoImageStore* images, xoStringTable* strings, xoRenderBase* driver, xoRenderDomNode* root, int width, int height );

protected:
	xoRenderBase*				Driver;
	xoImageStore*				Images;
	xoStringTable*				Strings;
	fhashset<xoGlyphCacheKey>	GlyphsNeeded;

	void			RenderEl( xoPoint base, xoRenderDomEl* node );
	void			RenderNode( xoPoint base, xoRenderDomNode* node );
	void			RenderText( xoPoint base, xoRenderDomText* node );
	void			RenderTextChar_WholePixel( xoPoint base, xoRenderDomText* node, const xoRenderCharEl& txtEl );
	void			RenderTextChar_SubPixel( xoPoint base, xoRenderDomText* node, const xoRenderCharEl& txtEl );
	void			RenderGlyphsNeeded();

	bool			LoadTexture( xoTexture* tex, int texUnit );		// Load a texture and reset invalid rectangle

};