#pragma once

#include "../xoDoc.h"
#include "xoRenderDomEl.h"

// Output from layout
class XOAPI xoLayoutResult
{
public:
	xoLayoutResult( const xoDoc& doc );
	~xoLayoutResult();

	bool				IsLocked;	// True if we are being used by the UI thread
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
	// Cloned data. temp hack for webcam demo
	//xoStringTable	ClonedStrings;
	xoImageStore			ClonedImages;

	// Rendered state
	AbcCriticalSection		LayoutLock;			// This guards LayoutResult and OldLayouts
	xoLayoutResult*			LatestLayout = nullptr;
	pvect<xoLayoutResult*>	OldLayouts;

	void			PurgeOldLayouts();
	//void			ResetRenderData();
	//xoInternalID	FindElement( const xoRenderDomEl& el, xoPoint pos );
	//static void		FindAlteredNodes( const xoDoc* original, const xoDoc* modified, podvec<xoInternalID>& alteredNodeIDs );

};
