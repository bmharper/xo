#pragma once
#include "nuDomEl.h"

/* DOM node that is not text
It is vital that this data structure does not grow much bigger than this.
Right now it's 128 bytes on Windows x64.
*/
class NUAPI nuDomNode : public nuDomEl
{
	DISALLOW_COPY_AND_ASSIGN(nuDomNode);
public:
					nuDomNode( nuDoc* doc, nuTag tag );
					virtual ~nuDomNode();

	virtual void			SetText( const char* txt ) override;
	virtual const char*		GetText() const override;
	virtual void			CloneSlowInto( nuDomEl& c, uint cloneFlags ) const override;
	virtual void			ForgetChildren() override;

	const pvect<nuDomEl*>&			GetChildren() const			{ return Children; }
	podvec<nuStyleID>&				GetClassesMutable()			{ IncVersion(); return Classes; }
	const podvec<nuStyleID>&		GetClasses() const			{ return Classes; }
	const podvec<nuEventHandler>&	GetHandlers() const			{ return Handlers; }
	const nuStyle&					GetStyle() const			{ return Style; }

	nuDomEl*		AddChild( nuTag tag );
	nuDomNode*		AddNode( nuTag tag );
	nuDomText*		AddText( const char* txt = nullptr );
	void			RemoveChild( nuDomEl* c );
	void			RemoveAllChildren();
	intp			ChildCount() const { return Children.size(); }
	nuDomEl*		ChildByIndex( intp index );
	void			Discard();

	bool			StyleParse( const char* t );
	bool			StyleParsef( const char* t, ... );
	// This is here for experiments. Future work needs a better performing method for setting just one attribute of the style.
	void			HackSetStyle( const nuStyle& style );

	// Classes
	void			AddClass( const char* klass );
	void			RemoveClass( const char* klass );

	// Events
	void			AddHandler( nuEvents ev, nuEventHandlerF func, void* context = NULL );
	void			AddHandler( nuEvents ev, nuEventHandlerLambda lambda );
	bool			HandlesEvent( nuEvents ev ) const { return !!(AllEventMask & ev); }

	void			OnTouch( nuEventHandlerF func, void* context )				{ AddHandler( nuEventTouch, func, context ); }
	void			OnMouseMove( nuEventHandlerF func, void* context )			{ AddHandler( nuEventMouseMove, func, context ); }
	void			OnClick( nuEventHandlerF func, void* context )				{ AddHandler( nuEventClick, func, context ); }
	void			OnTimer( nuEventHandlerF func, void* context )				{ AddHandler( nuEventTimer, func, context ); }

	void			OnTouch( nuEventHandlerLambda lambda )						{ AddHandler( nuEventTouch, lambda ); }
	void			OnMouseMove( nuEventHandlerLambda lambda )					{ AddHandler( nuEventMouseMove, lambda ); }
	void			OnClick( nuEventHandlerLambda lambda )						{ AddHandler( nuEventClick, lambda ); }
	void			OnTimer( nuEventHandlerLambda lambda )						{ AddHandler( nuEventTimer, lambda ); }

protected:
	uint32					AllEventMask;
	nuStyle					Style;			// Styles that override those referenced by the Tag and the Classes.
	podvec<nuEventHandler>	Handlers;
	pvect<nuDomEl*>			Children;
	podvec<nuStyleID>		Classes;		// Classes of styles

	void			RecalcAllEventMask();
	void			AddHandler( nuEvents ev, nuEventHandlerF func, bool isLambda, void* context );
};
