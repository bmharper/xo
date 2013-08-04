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

};