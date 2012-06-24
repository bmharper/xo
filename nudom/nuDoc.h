#pragma once

#include "nuDomEl.h"
#include "nuMem.h"

class NUAPI nuDoc
{
public:
	nuDomEl							Root;							// Root element of the document tree
	nuStyleSet						Styles;							// All styles defined in this document
	uint32							WindowWidth, WindowHeight;		// Device pixels

			nuDoc();
			~nuDoc();
	void	Reset();
	void	InvalidateAppearance();
	void	InvalidateLayout();
	bool	IsAppearanceDirty() const	{ return AppearanceDirty; }
	bool	IsLayoutDirty() const		{ return LayoutDirty; }
	void	AppearanceClean();
	void	LayoutClean();
	void	CloneFastInto( nuDoc& c, uint cloneFlags ) const;		// Used to make a read-only clone for the renderer
	void	ChildAdded( nuDomEl* el );
	void	ChildRemoved( nuDomEl* el );

protected:
	nuPool						Pool;					// Used only when making a clone via CloneFast()
	bool						IsReadOnly;				// Read-only clone used for rendering
	bool						LayoutDirty;
	bool						AppearanceDirty;
	pvect<nuDomEl*>				ChildByInternalID;
	nuInternalID				NextID;
	podvec<nuInternalID>		UsableIDs;				// When we do a re-layout, then FreeIDs are appended to UsableIDs
	podvec<nuInternalID>		FreeIDs;

	void	ResetInternalIDs();
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
	AbcCriticalSection			Lock;

			nuRenderDoc();
			~nuRenderDoc();

	void	Render( nuRenderGL* rgl );
	void	UpdateDoc( const nuDoc& original );

protected:
	int							DocWidth, DocHeight;
	nuDoc						Doc;

	void	ResetRenderData();
};
