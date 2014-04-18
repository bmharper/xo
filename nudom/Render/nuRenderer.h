#pragma once

#include "nuDefs.h"
#include "../Text/nuGlyphCache.h"

/* An instance of this is created for each render.
Any state that is persisted between renderings is stored in nuRenderGL.
*/
class NUAPI nuRenderer
{
public:
	nuRenderResult	Render( nuImageStore* images, nuStringTable* strings, nuRenderBase* driver, nuRenderDomNode* root, int width, int height );

protected:
	nuRenderBase*				Driver;
	nuImageStore*				Images;
	nuStringTable*				Strings;
	fhashset<nuGlyphCacheKey>	GlyphsNeeded;

	void			RenderEl( nuRenderDomEl* node );
	void			RenderNode( nuRenderDomNode* node );
	void			RenderText( nuRenderDomText* node );
	void			RenderTextChar_WholePixel( nuRenderDomText* node, const nuRenderCharEl& txtEl );
	void			RenderTextChar_SubPixel( nuRenderDomText* node, const nuRenderCharEl& txtEl );
	void			RenderGlyphsNeeded();

	bool			LoadTexture( nuTexture* tex, int texUnit );		// Load a texture and reset invalid rectangle

};