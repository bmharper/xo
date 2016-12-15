#include "pch.h"
#include "Doc.h"
#include "DomNode.h"
#include "DomCanvas.h"
#include "../Parse/DocParser.h"

namespace xo {

DomNode::DomNode(Doc* doc, Tag tag, InternalID parentID) : DomEl(doc, tag, parentID) {
	AllEventMask = 0;
}

DomNode::~DomNode() {
	for (size_t i = 0; i < Children.size(); i++)
		Doc->FreeChild(Children[i]);
	Children.clear();
}

void DomNode::SetText(const char* txt) {
	if (Children.size() != 1 || Children[0]->GetTag() != TagText) {
		RemoveAllChildren();
		AddChild(TagText);
	}
	Children[0]->SetText(txt);
}

const char* DomNode::GetText() const {
	if (Children.size() == 1 && Children[0]->GetTag() == TagText) {
		return Children[0]->GetText();
	} else {
		return "";
	}
}

void DomNode::CloneSlowInto(DomEl& c, uint32_t cloneFlags) const {
	CloneSlowIntoBase(c, cloneFlags);
	DomNode& cnode = static_cast<DomNode&>(c);
	Doc*     cDoc  = c.GetDoc();

	Style.CloneSlowInto(cnode.Style);
	cnode.Classes = Classes;

	// By the time we get here, all relevant DOM elements inside the destination document
	// have already been created. That is why we are not recursive here.
	cnode.Children.clear_noalloc();
	for (size_t i = 0; i < Children.size(); i++)
		cnode.Children += cDoc->GetChildByInternalIDMutable(Children[i]->GetInternalID());

	if (!!(cloneFlags & CloneFlagEvents))
		XO_DIE_MSG("clone events is TODO");
}

void DomNode::ForgetChildren() {
	Children.clear_noalloc();
}

DomEl* DomNode::AddChild(Tag tag) {
	IncVersion();
	DomEl* c = Doc->AllocChild(tag, InternalID);
	Children += c;
	Doc->ChildAdded(c);
	return c;
}

DomNode* DomNode::AddNode(Tag tag) {
	AbcAssert(tag != TagText);
	return static_cast<DomNode*>(AddChild(tag));
}

DomCanvas* DomNode::AddCanvas() {
	return static_cast<DomCanvas*>(AddChild(TagCanvas));
}

DomText* DomNode::AddText(const char* txt) {
	DomText* el = static_cast<DomText*>(AddChild(TagText));
	el->SetText(txt);
	return el;
}

void DomNode::RemoveChild(DomEl* c) {
	if (!c)
		return;
	IncVersion();
	size_t ix = Children.find(c);
	XO_ASSERT(ix != -1);
	Children.erase(ix);
	Doc->ChildRemoved(c);
	Doc->FreeChild(c);
}

void DomNode::RemoveAllChildren() {
	IncVersion();
	for (size_t i = 0; i < Children.size(); i++) {
		Doc->ChildRemoved(Children[i]);
		Doc->FreeChild(Children[i]);
	}
	Children.clear();
}

DomEl* DomNode::ChildByIndex(size_t index) {
	XO_ASSERT(index < Children.size());
	return Children[index];
}

const DomEl* DomNode::ChildByIndex(size_t index) const {
	XO_ASSERT(index < Children.size());
	return Children[index];
}

void DomNode::Discard() {
	InternalID   = 0;
	AllEventMask = 0;
	Version      = 0;
	Style.Discard();
	Classes.hack(0, 0, NULL);
	Children.hack(0, 0, NULL);
	Handlers.hack(0, 0, NULL);
}

String DomNode::Parse(const char* src) {
	RemoveAllChildren();
	return ParseAppend(src);
}

String DomNode::ParseAppend(const char* src) {
	DocParser p;
	return p.Parse(src, this);
}

String DomNode::ParseAppend(const StringRaw& src) {
	return ParseAppend(src.Z);
}

bool DomNode::StyleParse(const char* t, size_t maxLen) {
	IncVersion();
	return Style.Parse(t, maxLen, Doc);
}

bool DomNode::StyleParsef(const char* t, ...) {
	char    buff[8192];
	va_list va;
	va_start(va, t);
	uint32_t r = vsnprintf(buff, arraysize(buff), t, va);
	va_end(va);
	buff[arraysize(buff) - 1] = 0;
	if (r < arraysize(buff)) {
		return StyleParse(buff);
	} else {
		String str = String(t);
		str.Z[50]  = 0;
		ParseFail("Parse string is too long for StyleParsef: %s...", str.Z);
		XO_DEBUG_ASSERT(false);
		return false;
	}
}

void DomNode::HackSetStyle(const Style& style) {
	IncVersion();
	Style = style;
}

void DomNode::HackSetStyle(StyleAttrib attrib) {
	IncVersion();
	Style.Set(attrib);
}

void DomNode::AddClass(const char* klass) {
	IncVersion();
	StyleClassID id = Doc->ClassStyles.GetClassID(klass);
	if (Classes.find(id) == -1)
		Classes += id;
}

void DomNode::RemoveClass(const char* klass) {
	IncVersion();
	StyleClassID id    = Doc->ClassStyles.GetClassID(klass);
	size_t       index = Classes.find(id);
	if (index != -1)
		Classes.erase(index);
}

void DomNode::AddHandler(Events ev, EventHandlerF func, bool isLambda, void* context, uint32_t timerPeriodMS) {
	for (size_t i = 0; i < Handlers.size(); i++) {
		if (Handlers[i].Context == context && Handlers[i].Func == func) {
			XO_ASSERT(isLambda == Handlers[i].IsLambda());
			Handlers[i].Mask |= ev;
			Handlers[i].TimerPeriodMS = timerPeriodMS;
			RecalcAllEventMask();
			return;
		}
	}
	auto& h         = Handlers.add();
	h.Context       = context;
	h.Func          = func;
	h.Mask          = ev;
	h.TimerPeriodMS = timerPeriodMS;
	if (isLambda)
		h.SetLambda();
	RecalcAllEventMask();
}

void DomNode::AddHandler(Events ev, EventHandlerLambda lambda) {
	EventHandlerLambda* copy = new EventHandlerLambda(lambda);
	AddHandler(ev, EventHandler_LambdaStaticFunc, true, copy, 0);
}

void DomNode::AddTimerHandler(Events ev, EventHandlerLambda lambda, uint32_t periodMS) {
	EventHandlerLambda* copy = new EventHandlerLambda(lambda);
	AddHandler(ev, EventHandler_LambdaStaticFunc, true, copy, periodMS);
}

void DomNode::AddHandler(Events ev, EventHandlerF func, void* context) {
	AddHandler(ev, func, false, context, 0);
}

// Returns our fastest ticking timer event handler (or zero if none)
uint32_t DomNode::FastestTimerMS() const {
	uint32_t f = UINT32MAX - 1;
	for (const auto& h : Handlers) {
		if (h.TimerPeriodMS != 0) {
			XO_ASSERT(!!(h.Mask & EventTimer));
			f = Min(f, h.TimerPeriodMS);
		}
	}
	return f != UINT32MAX - 1 ? f : 0;
}

void DomNode::RecalcAllEventMask() {
	bool hadTimer = !!(AllEventMask & EventTimer);

	uint32_t m = 0;
	for (size_t i = 0; i < Handlers.size(); i++)
		m |= Handlers[i].Mask;
	AllEventMask = m;

	bool hasTimerNow = !!(AllEventMask & EventTimer);

	if (!hadTimer && hasTimerNow)
		Doc->NodeGotTimer(InternalID);
	else if (hadTimer && !hasTimerNow)
		Doc->NodeLostTimer(InternalID);
}
}
