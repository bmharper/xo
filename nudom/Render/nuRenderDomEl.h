#pragma once

#include "nuStyle.h"
#include "nuMem.h"

// Element that is ready for rendering
class NUAPI nuRenderDomEl
{
public:
				nuRenderDomEl( nuPool* pool = NULL );
				~nuRenderDomEl();

	void		SetPool( nuPool* pool );
	void		Discard();

	nuInternalID				InternalID;			// A safe way of getting back to our original nuDomEl
	nuBox						Pos;
	float						BorderRadius;
	nuStyle						Style;
	nuPoolArray<nuRenderDomEl*>	Children;
};
