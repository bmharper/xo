#pragma once

#include "nuStyle.h"
#include "nuMem.h"
#include "nuEvent.h"

/* DOM node in the tree.
It is vital that this data structure does not grow much bigger than this.
Right now it's 120 bytes on Windows x64.
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
	void			CloneSlowInto( nuDomEl& c, uint cloneFlags ) const;
	void			CloneFastInto( nuDomEl& c, nuPool* pool, uint cloneFlags ) const;
	void			Discard();
	void			ForgetChildren();

	void			SetInternalID( nuInternalID id )			{ InternalID = id; }	// Used by nuDoc at element creation time.
	void			SetDoc( nuDoc* doc )						{ Doc = doc; }			// Used by nuDoc at element creation and destruction time.
	void			SetDocRoot()								{ Tag = nuTagBody; }	// Used by nuDoc at initialization time.

	bool			StyleParse( const char* t )					{ IncVersion(); return Style.Parse( t ); }
	bool			StyleParsef( const char* t, ... );
	// This is here for experiments. Future work needs a better performing method for setting just one attribute of the style.
	void			HackSetStyle( const nuStyle& style );

	// Classes
	void			AddClass( const char* klass );
	void			RemoveClass( const char* klass );

	// Events
	void			AddHandler( nuEvents ev, nuEventHandlerF func, void* context = NULL );
#if NU_LAMBDA
	void			AddHandler( nuEvents ev, nuEventHandlerLambda lambda );
#endif
	bool			HandlesEvent( nuEvents ev ) const { return !!(AllEventMask & ev); }

	void			OnTouch( nuEventHandlerF func, void* context = NULL )		{ AddHandler( nuEventTouch, func, context ); }
	void			OnMouseMove( nuEventHandlerF func, void* context = NULL )	{ AddHandler( nuEventMouseMove, func, context ); }
	void			OnTimer( nuEventHandlerF func, void* context = NULL )		{ AddHandler( nuEventTimer, func, context ); }

#if NU_LAMBDA
	void			OnTouch( nuEventHandlerLambda lambda )						{ AddHandler( nuEventTouch, lambda ); }
	void			OnMouseMove( nuEventHandlerLambda lambda )					{ AddHandler( nuEventMouseMove, lambda ); }
#endif

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
	void			AddHandler( nuEvents ev, nuEventHandlerF func, bool isLambda, void* context );

	void			IncVersion();

};
