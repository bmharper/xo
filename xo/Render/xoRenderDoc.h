#pragma once

#include "../xoDoc.h"
#include "xoRenderDomEl.h"


/* Document used by renderer.
The 'Doc' member is a complete clone of the original document.
*/
class XOAPI xoRenderDoc
{
public:
	// Defining state
	xoDoc						Doc;
	uint32						WindowWidth, WindowHeight;		// Device pixels

	// Rendered state
	xoRenderDomNode				RenderRoot;
	xoPool						RenderPool;
	//podvec<xoInternalID>		ModifiedNodeIDs;

			xoRenderDoc();
			~xoRenderDoc();

	xoRenderResult	Render( xoRenderBase* driver );
	void			CopyFromCanonical( const xoDoc& canonical, xoRenderStats& stats );
	//xoInternalID	FindElement( xoPoint pos );

protected:
	// Cloned data. temp hack for webcam demo
	//xoStringTable	ClonedStrings;
	xoImageStore	ClonedImages;

	void			ResetRenderData();
	//xoInternalID	FindElement( const xoRenderDomEl& el, xoPoint pos );
	//static void		FindAlteredNodes( const xoDoc* original, const xoDoc* modified, podvec<xoInternalID>& alteredNodeIDs );

};
