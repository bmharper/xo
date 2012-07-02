#pragma once

#include "nuDefs.h"

class NUAPI nuRenderer
{
public:

	void			Render( nuRenderGL* gl, nuRenderDomEl* root, int width, int height );

protected:
	nuRenderGL*		GL;

	void			RenderNode( nuRenderDomEl* node );

};