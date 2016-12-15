#pragma once
#include "DomEl.h"

namespace xo {

/* DOM node that is not text
It is vital that this data structure does not grow much bigger than this.
Right now it's 136 bytes on Windows x64.
*/
class XO_API DomNode : public DomEl {
	DISALLOW_COPY_AND_ASSIGN(DomNode);

public:
	DomNode(Doc* doc, Tag tag, InternalID parentID);
	virtual ~DomNode();

	virtual void        SetText(const char* txt) override;
	virtual const char* GetText() const override;
	virtual void        CloneSlowInto(DomEl& c, uint32_t cloneFlags) const override;
	virtual void        ForgetChildren() override;

	const cheapvec<DomEl*>&  GetChildren() const { return Children; }
	cheapvec<StyleClassID>& GetClassesMutable() {
		IncVersion();
		return Classes;
	}
	const cheapvec<StyleClassID>& GetClasses() const { return Classes; }
	const Style&                GetStyle() const { return Style; }
	const cheapvec<EventHandler>& GetHandlers() const { return Handlers; }
	void                        GetHandlers(const EventHandler*& handlers, size_t& count) const {
        handlers = &Handlers[0];
        count    = Handlers.size();
	}

	DomEl*       AddChild(Tag tag);
	DomNode*     AddNode(Tag tag);
	DomCanvas*   AddCanvas();
	DomText*     AddText(const char* txt = nullptr);
	void         RemoveChild(DomEl* c);
	void         RemoveAllChildren();
	size_t         ChildCount() const { return Children.size(); }
	DomEl*       ChildByIndex(size_t index);
	const DomEl* ChildByIndex(size_t index) const;
	void         Discard();

	// Replace all child elements with the given xml-like string. Returns empty string on success, or error message.
	String Parse(const char* src);
	String ParseAppend(const char* src); // Same as Parse, but append to node
	String ParseAppend(const StringRaw& src);

	bool StyleParse(const char* t, size_t maxLen = -1);
	bool StyleParsef(const char* t, ...);
	// TODO: This is here for experiments. Future work needs a better performing method for setting just one attribute of the style.
	void HackSetStyle(const Style& style);
	void HackSetStyle(StyleAttrib attrib); // TODO: This is also "Hack" because it doesn't work for attribute such as background-image

	// Classes
	void AddClass(const char* klass);
	void RemoveClass(const char* klass);

	// Events
	void AddHandler(Events ev, EventHandlerF func, void* context = NULL);
	void AddHandler(Events ev, EventHandlerLambda lambda);
	bool HandlesEvent(Events ev) const { return !!(AllEventMask & ev); }
	uint32_t FastestTimerMS() const;

	// It is tempting to use macros to generate these event handler functions,
	// but the intellisense experience is so much worse that I avoid it.
	// These functions exist purely for discoverability, because one can already achieve
	// the same action by using the generic AddHandler().

	void OnWindowSize(EventHandlerF func, void* context) { AddHandler(EventWindowSize, func, context); }
	void OnTimer(EventHandlerF func, void* context, uint32_t periodMS) { AddHandler(EventTimer, func, false, context, periodMS); }
	void OnGetFocus(EventHandlerF func, void* context) { AddHandler(EventGetFocus, func, context); }
	void OnLoseFocus(EventHandlerF func, void* context) { AddHandler(EventLoseFocus, func, context); }
	void OnTouch(EventHandlerF func, void* context) { AddHandler(EventTouch, func, context); }
	void OnClick(EventHandlerF func, void* context) { AddHandler(EventClick, func, context); }
	void OnDblClick(EventHandlerF func, void* context) { AddHandler(EventDblClick, func, context); }
	void OnMouseMove(EventHandlerF func, void* context) { AddHandler(EventMouseMove, func, context); }
	void OnMouseEnter(EventHandlerF func, void* context) { AddHandler(EventMouseEnter, func, context); }
	void OnMouseLeave(EventHandlerF func, void* context) { AddHandler(EventMouseLeave, func, context); }
	void OnMouseDown(EventHandlerF func, void* context) { AddHandler(EventMouseDown, func, context); }
	void OnMouseUp(EventHandlerF func, void* context) { AddHandler(EventMouseUp, func, context); }

	void OnWindowSize(EventHandlerLambda lambda) { AddHandler(EventWindowSize, lambda); }
	void OnTimer(EventHandlerLambda lambda, uint32_t periodMS) { AddTimerHandler(EventTimer, lambda, periodMS); }
	void OnGetFocus(EventHandlerLambda lambda) { AddHandler(EventGetFocus, lambda); }
	void OnLoseFocus(EventHandlerLambda lambda) { AddHandler(EventLoseFocus, lambda); }
	void OnTouch(EventHandlerLambda lambda) { AddHandler(EventTouch, lambda); }
	void OnClick(EventHandlerLambda lambda) { AddHandler(EventClick, lambda); }
	void OnDblClick(EventHandlerLambda lambda) { AddHandler(EventDblClick, lambda); }
	void OnMouseMove(EventHandlerLambda lambda) { AddHandler(EventMouseMove, lambda); }
	void OnMouseEnter(EventHandlerLambda lambda) { AddHandler(EventMouseEnter, lambda); }
	void OnMouseLeave(EventHandlerLambda lambda) { AddHandler(EventMouseLeave, lambda); }
	void OnMouseDown(EventHandlerLambda lambda) { AddHandler(EventMouseDown, lambda); }
	void OnMouseUp(EventHandlerLambda lambda) { AddHandler(EventMouseUp, lambda); }

protected:
	uint32_t               AllEventMask;
	Style                Style; // Styles that override those referenced by the Tag and the Classes.
	cheapvec<EventHandler> Handlers;
	cheapvec<DomEl*>        Children;
	cheapvec<StyleClassID> Classes; // Classes of styles

	void RecalcAllEventMask();
	void AddHandler(Events ev, EventHandlerF func, bool isLambda, void* context, uint32_t timerPeriodMS);
	void AddTimerHandler(Events ev, EventHandlerLambda lambda, uint32_t periodMS);
};
}
