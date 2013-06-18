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

	nuInternalID				InternalID;			// Reference to our original nuDomEl
	nuBox						Pos;
	float						BorderRadius;
	nuStyle						Style;				// old & dead
	nuStyleSet					ResolvedStyle;		// new & good
	nuPoolArray<nuRenderDomEl*>	Children;
};
