#pragma once

#include "../xoDoc.h"
#include "xoRenderDomEl.h"

// Output from layout
class XOAPI xoLayoutResult
{
public:
	xoLayoutResult( const xoDoc& doc );
	~xoLayoutResult();

	bool				IsLocked;	// True if we are being used by the UI thread to do things like hit-testing
	xoRenderDomNode		Root;
	xoPool				Pool;
};

/* Document used by renderer.
The 'Doc' member is a complete clone of the original document.
*/
class XOAPI xoRenderDoc
{
public:
	// Defining state
	xoDoc					Doc;

	//xoRenderDomNode				RenderRoot;
	//xoPool						RenderPool;
	//podvec<xoInternalID>		ModifiedNodeIDs;

					xoRenderDoc();
					~xoRenderDoc();

	xoRenderResult	Render( xoRenderBase* driver );
	void			CopyFromCanonical( const xoDoc& canonical, xoRenderStats& stats );
	//xoInternalID	FindElement( xoPoint pos );

	// Acquire the latest layout object. Call ReleaseLayout when you are done using it. Returns nullptr if no layouts exist.
	// Panics if the latest layout has already been acquired.
	xoLayoutResult*	AcquireLatestLayout();						
	void			ReleaseLayout( xoLayoutResult* layout );

protected:
	// Cloned image metadata
	xoImageStore			ClonedImages;

	// Rendered state
	AbcCriticalSection		LayoutLock;				// This guards the pointers LayoutResult and OldLayouts (but not necessarily the content that is pointed to)
	xoLayoutResult*			LatestLayout = nullptr;	// Most recent layout performed
	pvect<xoLayoutResult*>	OldLayouts;				// Layouts there were busy being used by the UI thread while the rendering thread progressed onto doing another layout

	void			PurgeOldLayouts();
	//void			ResetRenderData();
	//xoInternalID	FindElement( const xoRenderDomEl& el, xoPoint pos );
	//static void		FindAlteredNodes( const xoDoc* original, const xoDoc* modified, podvec<xoInternalID>& alteredNodeIDs );

};
