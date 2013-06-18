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

	const pvect<nuDomEl*>&			GetChildren() const			{ return Children; }
	const podvec<nuStyleID>&		GetClasses() const			{ return Classes; }
	podvec<nuStyleID>&				GetClassesMutable()			{ IncVersion(); return Classes; }
	const podvec<nuEventHandler>&	GetHandlers() const			{ return Handlers; }
	const nuStyle&					GetStyle() const			{ return Style; }
	nuInternalID					GetInternalID() const		{ return InternalID; }
	nuTag							GetTag() const				{ return Tag; }
	nuDoc*							GetDoc() const				{ return Doc; }
	uint32							GetVersion() const			{ return Version; }

	nuDomEl*		AddChild( nuTag tag );
	void			RemoveChild( nuDomEl* c );
	intp			ChildCount() const { return Children.size(); }
	nuDomEl*		ChildByIndex( intp index );
	void			CloneFastInto( nuDomEl& c, nuPool* pool, uint cloneFlags ) const;
	void			Discard();

	void			SetInternalID( nuInternalID id )			{ InternalID = id; }	// Used by nuDoc at element creation time.
	void			SetDoc( nuDoc* doc )						{ Doc = doc; }			// Used by nuDoc at element creation and destruction time.
	void			SetDocRoot()								{ Tag = nuTagBody; }	// Used by nuDoc at initialization time.

	bool			StyleParse( const char* t )					{ IncVersion(); return Style.Parse( t ); }
	bool			StyleParsef( const char* t, ... );

	// Classes
	void			AddClass( const char* klass );
	void			RemoveClass( const char* klass );

	// Events
	void			AddHandler( nuEvents ev, nuEventHandlerF func, void* context = NULL );
	bool			HandlesEvent( nuEvents ev ) const { return !!(AllEventMask & ev); }

	void			OnTouch( nuEventHandlerF func, void* context = NULL )		{ AddHandler( nuEventTouch, func, context ); }
	void			OnMouseMove( nuEventHandlerF func, void* context = NULL )	{ AddHandler( nuEventMouseMove, func, context ); }

protected:
	nuDoc*					Doc;			// Owning document
	nuInternalID			InternalID;		// Internal 32-bit ID that is used to keep track of an object (memory address is not sufficient)
	nuTag					Tag;			// Tag, such <div>, etc
	nuStyle					Style;			// Styles that override those referenced by the Tag and the Classes.
	uint32					Version;		// Monotonic integer used to detect modified nodes
	podvec<nuStyleID>		Classes;		// Classes of styles

	podvec<nuEventHandler>	Handlers;
	uint32					AllEventMask;
	pvect<nuDomEl*>			Children;

	void			RecalcAllEventMask();
	void			IncVersion()				{ Version++; }

};
