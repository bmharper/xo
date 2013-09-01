#pragma once

#include "nuDefs.h"

class NUAPI nuRenderer
{
public:
	void			Render( nuImageStore* images, nuStringTable* strings, nuRenderGL* gl, nuRenderDomEl* root, int width, int height );

protected:
	nuRenderGL*		GL;
	nuImageStore*	Images;
	nuStringTable*	Strings;

	void			RenderNode( nuRenderDomEl* node );
	void			RenderTextNode( nuRenderDomEl* node );
	void			RenderTextNodeChar_WholePixel( nuRenderDomEl* node, const nuRenderTextEl& txtEl );
	void			RenderTextNodeChar_SubPixel( nuRenderDomEl* node, const nuRenderTextEl& txtEl );

};