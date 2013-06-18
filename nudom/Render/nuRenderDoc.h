#pragma once

#include "../nuDoc.h"
#include "nuRenderDomEl.h"


/* Document used by renderer.
This includes a complete clone of the original document.
*/
class NUAPI nuRenderDoc
{
public:
	nuDoc						Doc;

	// Rendered state
	nuRenderDomEl				RenderRoot;
	nuPool						RenderPool;

	podvec<nuInternalID>		ModifiedNodeIDs;

			nuRenderDoc();
			~nuRenderDoc();

	void			Render( nuRenderGL* rgl );
	void			CopyFromCanonical( const nuDoc& original );
	nuInternalID	FindElement( nuPoint pos );

protected:
	void			ResetRenderData();
	nuInternalID	FindElement( const nuRenderDomEl& el, nuPoint pos );
	static void		FindAlteredNodes( const nuDoc* original, const nuDoc* modified, podvec<nuInternalID>& alteredNodeIDs );

};
