#pragma once

#include "../nuDoc.h"
#include "nuRenderDomEl.h"

struct NUAPI nuRenderNodePairs
{
	
};

/* Document used by renderer.
This includes a complete clone of the original document.
*/
class NUAPI nuRenderDoc
{
public:
	// Rendered state
	nuRenderDomEl				RenderRoot;
	nuPool						RenderPool;

	podvec<nuRenderNodePairs>	Nodes;		// Indexed by nuInternalID

	// Input
	nuDoc						Doc;

			nuRenderDoc();
			~nuRenderDoc();

	void	Render( nuRenderGL* rgl );
	void	UpdateDoc( const nuDoc& original );

protected:

	void	ResetRenderData();
};
