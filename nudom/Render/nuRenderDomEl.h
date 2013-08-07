#pragma once

#include "nuStyle.h"
#include "nuMem.h"

class nuRenderStack;

// Element that is ready for rendering
class NUAPI nuRenderDomEl
{
public:
				nuRenderDomEl( nuPool* pool = NULL );
				~nuRenderDomEl();

	void		SetPool( nuPool* pool );
	void		Discard();
	void		SetStyle( nuRenderStack& stack );

	nuInternalID				InternalID;			// Reference to our original nuDomEl
	nuBox						Pos;
	nuStyleRender				Style;
	nuPoolArray<nuRenderDomEl*>	Children;
};
