#pragma once

#include "nuStyle.h"
#include "nuMem.h"
#include "nuEvent.h"

/* DOM node in the tree.
*/
class NUAPI nuDomEl
{
public:
				nuDomEl();
				~nuDomEl();

	nuDoc*					Doc;			// Owning document
	nuInternalID			InternalID;		// Internal 32-bit ID that is used to keep track of an object (memory address is not sufficient)
	nuTag					Tag;			// Tag, such <div>, etc
	nuStyle					Style;			// Styles that override those referenced by the Tag and the Classes.
	podvec<nuStyleID>		Classes;		// Classes of styles

	const pvect<nuDomEl*>&			GetChildren() const { return Children; }
	const podvec<nuEventHandler>&	GetHandlers() const { return Handlers; }

	void		AddChild( nuDomEl* c );
	void		RemoveChild( nuDomEl* c );
	intp		ChildCount() const { return Children.size(); }
	nuDomEl*	ChildByIndex( intp index );
	void		CloneFastInto( nuDomEl& c, nuPool* pool, uint cloneFlags ) const;
	void		Discard();

	// Classes
	void		AddClass( const char* klass );
	void		RemoveClass( const char* klass );

	// Events
	void		AddHandler( nuEvents ev, nuEventHandlerF func, void* context = NULL );
	bool		HandlesEvent( nuEvents ev ) const { return !!(AllEventMask & ev); }

	void		OnTouch( nuEventHandlerF func, void* context = NULL )		{ AddHandler( nuEventTouch, func, context ); }
	void		OnMouseMove( nuEventHandlerF func, void* context = NULL )	{ AddHandler( nuEventMouseMove, func, context ); }

protected:
	podvec<nuEventHandler>	Handlers;
	uint32					AllEventMask;
	pvect<nuDomEl*>			Children;

	void		RecalcAllEventMask();

};

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
