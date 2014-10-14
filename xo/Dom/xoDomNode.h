#pragma once
#include "xoDomEl.h"

/* DOM node that is not text
It is vital that this data structure does not grow much bigger than this.
Right now it's 136 bytes on Windows x64.
*/
class XOAPI xoDomNode : public xoDomEl
{
	DISALLOW_COPY_AND_ASSIGN(xoDomNode);
public:
							xoDomNode( xoDoc* doc, xoTag tag, xoInternalID parentID );
	virtual					~xoDomNode();

	virtual void			SetText( const char* txt ) override;
	virtual const char*		GetText() const override;
	virtual void			CloneSlowInto( xoDomEl& c, uint cloneFlags ) const override;
	virtual void			ForgetChildren() override;

	const pvect<xoDomEl*>&			GetChildren() const			{ return Children; }
	podvec<xoStyleClassID>&			GetClassesMutable()			{ IncVersion(); return Classes; }
	const podvec<xoStyleClassID>&	GetClasses() const			{ return Classes; }
	const xoStyle&					GetStyle() const			{ return Style; }
	const podvec<xoEventHandler>&	GetHandlers() const			{ return Handlers; }
	void							GetHandlers( const xoEventHandler*& handlers, intp& count ) const { handlers = &Handlers[0]; count = Handlers.size(); }

	xoDomEl*		AddChild( xoTag tag );
	xoDomNode*		AddNode( xoTag tag );
	xoDomCanvas*	AddCanvas();
	xoDomText*		AddText( const char* txt = nullptr );
	void			RemoveChild( xoDomEl* c );
	void			RemoveAllChildren();
	intp			ChildCount() const { return Children.size(); }
	xoDomEl*		ChildByIndex( intp index );
	const xoDomEl*	ChildByIndex( intp index ) const;
	void			Discard();

	// Replace all child elements with the given xml-like string. Returns empty string on success, or error message.
	xoString		Parse( const char* src );
	xoString		ParseAppend( const char* src );	// Same as Parse, but append to node
	xoString		ParseAppend( const xoStringRaw& src );

	bool			StyleParse( const char* t, intp maxLen = INT32MAX );
	bool			StyleParsef( const char* t, ... );
	// TODO: This is here for experiments. Future work needs a better performing method for setting just one attribute of the style.
	void			HackSetStyle( const xoStyle& style );
	void			HackSetStyle( xoStyleAttrib attrib ); // TODO: This is also "Hack" because it doesn't work for attribute such as background-image

	// Classes
	void			AddClass( const char* klass );
	void			RemoveClass( const char* klass );

	// Events
	void			AddHandler( xoEvents ev, xoEventHandlerF func, void* context = NULL );
	void			AddHandler( xoEvents ev, xoEventHandlerLambda lambda );
	bool			HandlesEvent( xoEvents ev ) const { return !!(AllEventMask & ev); }

	// It is tempting to use macros to generate these event handler functions,
	// but the intellisense experience is so much worse that I avoid it.
	// These functions exist purely for discoverability, because one can already achieve
	// the same action by using the generic AddHandler().

	void			OnWindowSize( xoEventHandlerF func, void* context )			{ AddHandler( xoEventWindowSize, func, context ); }
	void			OnTimer( xoEventHandlerF func, void* context )				{ AddHandler( xoEventTimer, func, context ); }
	void			OnGetFocus( xoEventHandlerF func, void* context )			{ AddHandler( xoEventGetFocus, func, context ); }
	void			OnLoseFocus( xoEventHandlerF func, void* context )			{ AddHandler( xoEventLoseFocus, func, context ); }
	void			OnTouch( xoEventHandlerF func, void* context )				{ AddHandler( xoEventTouch, func, context ); }
	void			OnClick( xoEventHandlerF func, void* context )				{ AddHandler( xoEventClick, func, context ); }
	void			OnDblClick( xoEventHandlerF func, void* context )			{ AddHandler( xoEventDblClick, func, context ); }
	void			OnMouseMove( xoEventHandlerF func, void* context )			{ AddHandler( xoEventMouseMove, func, context ); }
	void			OnMouseEnter( xoEventHandlerF func, void* context )			{ AddHandler( xoEventMouseEnter, func, context ); }
	void			OnMouseLeave( xoEventHandlerF func, void* context )			{ AddHandler( xoEventMouseLeave, func, context ); }
	void			OnMouseDown( xoEventHandlerF func, void* context )			{ AddHandler( xoEventMouseDown, func, context ); }
	void			OnMouseUp( xoEventHandlerF func, void* context )			{ AddHandler( xoEventMouseUp, func, context ); }

	void			OnWindowSize( xoEventHandlerLambda lambda )					{ AddHandler( xoEventWindowSize, lambda ); }
	void			OnTimer( xoEventHandlerLambda lambda )						{ AddHandler( xoEventTimer, lambda ); }
	void			OnGetFocus( xoEventHandlerLambda lambda )					{ AddHandler( xoEventGetFocus, lambda ); }
	void			OnLoseFocus( xoEventHandlerLambda lambda )					{ AddHandler( xoEventLoseFocus, lambda ); }
	void			OnTouch( xoEventHandlerLambda lambda )						{ AddHandler( xoEventTouch, lambda ); }
	void			OnClick( xoEventHandlerLambda lambda )						{ AddHandler( xoEventClick, lambda ); }
	void			OnDblClick( xoEventHandlerLambda lambda )					{ AddHandler( xoEventDblClick, lambda ); }
	void			OnMouseMove( xoEventHandlerLambda lambda )					{ AddHandler( xoEventMouseMove, lambda ); }
	void			OnMouseEnter( xoEventHandlerLambda lambda )					{ AddHandler( xoEventMouseEnter, lambda ); }
	void			OnMouseLeave( xoEventHandlerLambda lambda )					{ AddHandler( xoEventMouseLeave, lambda ); }
	void			OnMouseDown( xoEventHandlerLambda lambda )					{ AddHandler( xoEventMouseDown, lambda ); }
	void			OnMouseUp( xoEventHandlerLambda lambda )					{ AddHandler( xoEventMouseUp, lambda ); }

protected:
	uint32					AllEventMask;
	xoStyle					Style;			// Styles that override those referenced by the Tag and the Classes.
	podvec<xoEventHandler>	Handlers;
	pvect<xoDomEl*>			Children;
	podvec<xoStyleClassID>	Classes;		// Classes of styles

	void			RecalcAllEventMask();
	void			AddHandler( xoEvents ev, xoEventHandlerF func, bool isLambda, void* context );
};
