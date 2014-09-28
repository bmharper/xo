#pragma once

#include "xoDefs.h"
#include "../Text/xoGlyphCache.h"

/* An instance of this is created for each render.
Any state that is persisted between renderings is stored in xoRenderGL.
*/
class XOAPI xoRenderer
{
public:
	xoRenderResult	Render( xoImageStore* images, xoStringTable* strings, xoRenderBase* driver, const xoRenderDomNode* root );

protected:
	xoRenderBase*				Driver;
	xoImageStore*				Images;
	xoStringTable*				Strings;
	fhashset<xoGlyphCacheKey>	GlyphsNeeded;

	void			RenderEl( xoPoint base, const xoRenderDomEl* node );
	void			RenderNode( xoPoint base, const xoRenderDomNode* node );
	void			RenderText( xoPoint base, const xoRenderDomText* node );
	void			RenderTextChar_WholePixel( xoPoint base, const xoRenderDomText* node, const xoRenderCharEl& txtEl );
	void			RenderTextChar_SubPixel( xoPoint base, const xoRenderDomText* node, const xoRenderCharEl& txtEl );
	void			RenderGlyphsNeeded();

	bool			LoadTexture( xoTexture* tex, int texUnit );		// Load a texture and reset invalid rectangle

};