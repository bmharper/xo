#pragma once

#include "nuDefs.h"
#include "../Text/nuGlyphCache.h"

/* An instance of this is created for each render.
Any state that is persisted between renderings is stored in nuRenderGL.
*/
class NUAPI nuRenderer
{
public:
	nuRenderResult	Render( nuImageStore* images, nuStringTable* strings, nuRenderGL* gl, nuRenderDomEl* root, int width, int height );

protected:
	nuRenderGL*					GL;
	nuImageStore*				Images;
	nuStringTable*				Strings;
	fhashset<nuGlyphCacheKey>	GlyphsNeeded;

	void			RenderNodeOuter( nuRenderDomEl* node );
	void			RenderNodeInner( nuRenderDomEl* node );
	void			RenderTextNode( nuRenderDomEl* node );
	void			RenderTextNodeChar_WholePixel( nuRenderDomEl* node, const nuRenderTextEl& txtEl );
	void			RenderTextNodeChar_SubPixel( nuRenderDomEl* node, const nuRenderTextEl& txtEl );
	void			RenderGlyphsNeeded();

};