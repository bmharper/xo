#pragma once

#include "nuStyle.h"
#include "nuMem.h"

class nuRenderStack;

struct NUAPI nuRenderTextEl
{
	int		Char;
	nuPos	X;		// Left point of baseline
	nuPos	Y;		// Left point of baseline
};

// Element that is ready for rendering
class NUAPI nuRenderDomEl
{
public:
				nuRenderDomEl( nuPool* pool = NULL );
				~nuRenderDomEl();

	void		SetPool( nuPool* pool );
	void		Discard();
	void		SetStyle( nuRenderStack& stack );

	nuInternalID					InternalID;			// Reference to our original nuDomEl
	nuBox							Pos;
	nuStyleRender					Style;
	
	// Following are relevant for text only ... this must be split out into a separate nuRenderDomElText or something
	nuFontID						FontID;
	int								Char;
	nuPoolArray<nuRenderTextEl>		Text; // let's try this

	nuPoolArray<nuRenderDomEl*>		Children;
};
