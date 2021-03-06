#include "pch.h"
#include "Doc.h"
#include "DomNode.h"
#include "DomCanvas.h"
#include "../Parse/DocParser.h"

namespace xo {

DomNode::DomNode(xo::Doc* doc, xo::Tag tag, xo::InternalID parentID) : DomEl(doc, tag, parentID) {
}

DomNode::~DomNode() {
	if (!!(AllEventMask & EventDestroy)) {
		Event ev;
		ev.Doc    = Doc;
		ev.Target = this;
		ev.Type   = EventDestroy;
		for (auto& h : Handlers) {
			if (h.Handles(EventDestroy)) {
				ev.Context = h.Context;
				h.Func(ev);
			}
		}
	}
	for (size_t i = 0; i < Children.size(); i++)
		Doc->FreeChild(Children[i]);
	Children.clear();
}

void DomNode::SetText(const char* txt) {
	if (Children.size() == 0 || Children[0]->GetTag() != TagText) {
		AddChild(TagText);
		if (Children.size() != 1) {
			Children.insert(0, nullptr);
			std::swap(Children[0], Children[Children.size() - 1]);
			Children.pop_back();
		}
	}
	Children[0]->SetText(txt);
}

const char* DomNode::GetText() const {
	if (Children.size() != 0 && Children[0]->GetTag() == TagText) {
		return Children[0]->GetText();
	} else {
		return "";
	}
}

void DomNode::CloneSlowInto(DomEl& c, uint32_t cloneFlags) const {
	CloneSlowIntoBase(c, cloneFlags);
	DomNode& cnode = static_cast<DomNode&>(c);
	xo::Doc* cDoc  = c.GetDoc();

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

DomEl* DomNode::AddChild(xo::Tag tag, size_t position) {
	IncVersion();
	DomEl* c = Doc->AllocChild(tag, InternalID);
	if (position >= Children.size())
		Children += c;
	else
		Children.insert(position, c);
	Doc->ChildAdded(c);
	return c;
}

DomNode* DomNode::AddNode(xo::Tag tag, size_t position) {
	// DomText is not a DomNode object, so our return type would be violated here.
	XO_ASSERT(tag != TagText);

	return static_cast<DomNode*>(AddChild(tag, position));
}

DomCanvas* DomNode::AddCanvas(size_t position) {
	return static_cast<DomCanvas*>(AddChild(TagCanvas, position));
}

DomText* DomNode::AddText(const char* txt, size_t position) {
	DomText* el = static_cast<DomText*>(AddChild(TagText, position));
	el->SetText(txt);
	return el;
}

DomText* DomNode::AddText(const std::string& txt, size_t position) {
	return AddText(txt.c_str(), position);
}

void DomNode::Delete() {
	GetParent()->DeleteChild(this);
}

void DomNode::DeleteChild(DomEl* c) {
	if (!c)
		return;
	IncVersion();
	size_t ix = Children.find(c);
	XO_ASSERT(ix != -1);
	Children.erase(ix);
	DeleteChildInternal(c);
}

void DomNode::Clear() {
	IncVersion();
	for (size_t i = 0; i < Children.size(); i++)
		DeleteChildInternal(Children[i]);
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

DomNode* DomNode::NodeByIndex(size_t index) {
	return ChildByIndex(index)->ToNode();
}

const DomNode* DomNode::NodeByIndex(size_t index) const {
	return ChildByIndex(index)->ToNode();
}

void DomNode::Discard() {
	InternalID   = 0;
	AllEventMask = 0;
	Version      = 0;
	Style.Discard();
	Classes.discard();
	Children.discard();
	Handlers.discard();
}

String DomNode::Parse(const char* src) {
	Clear();
	String err;
	ParseAppend(src, &err);
	return err;
}

DomEl* DomNode::ParseAppend(const char* src, String* error) {
	auto      nChild = Children.size();
	DocParser p;
	auto      err = p.Parse(src, this);
	if (!err.IsEmpty()) {
		if (error)
			*error = err;
		return nullptr;
	}
	if (Children.size() > nChild)
		return Children[nChild];
	else
		return nullptr;
}

DomEl* DomNode::ParseAppend(const StringRaw& src, String* error) {
	return ParseAppend(src.CStr(), error);
}

DomEl* DomNode::ParseAppend(const std::string& src, String* error) {
	return ParseAppend(src.c_str(), error);
}

DomNode* DomNode::ParseAppendNode(const char* src, String* error) {
	DomEl* el = ParseAppend(src, error);
	if (!el->ToNode())
		ParseFail("ParseAppendNode: '%20s' is not a node", src);
	return el->ToNode();
}

DomNode* DomNode::ParseAppendNode(const StringRaw& src, String* error) {
	return ParseAppendNode(src.CStr(), error);
}

DomNode* DomNode::ParseAppendNode(const std::string& src, String* error) {
	return ParseAppendNode(src.c_str(), error);
}

bool DomNode::StyleParse(const char* s, size_t maxLen) {
	IncVersion();
	return Style.Parse(s, maxLen, Doc);
}

void DomNode::HackSetStyle(const xo::Style& style) {
	IncVersion();
	Style = style;
}

void DomNode::HackSetStyle(StyleAttrib attrib) {
	IncVersion();
	Style.Set(attrib);
}

void DomNode::AddClass(const char* classes) {
	IncVersion();
	size_t s = 0;
	char   buf[xo::MaxClassNameLen + 1];
	for (size_t i = 0; true; i++) {
		bool space = classes[i] == 32 || classes[i] == 9;
		if (classes[i] == 0 || space) {
			if (i - s == 1 && space) {
				// skip over consecutive whitespace
				s = i + 1;
				continue;
			}

			StyleClassID id;
			if (space) {
				// Use buf
				XO_ASSERT(i - s < sizeof(buf));
				memcpy(buf, classes + s, i - s);
				buf[i - s] = 0;
				id         = Doc->ClassStyles.GetClassID(buf);
			} else {
				// Don't need buf, because we're at the end
				id = Doc->ClassStyles.GetClassID(classes + s);
			}
			if (Classes.find(id) == -1)
				Classes += id;
			if (classes[i] == 0)
				break;
			s = i + 1;
		}
	}
}

void DomNode::RemoveClass(const char* klass) {
	IncVersion();
	StyleClassID id    = Doc->ClassStyles.GetClassID(klass);
	size_t       index = Classes.find(id);
	if (index != -1)
		Classes.erase(index);
}

bool DomNode::HasClass(const char* klass) const {
	return Classes.find(Doc->ClassStyles.GetClassID(klass)) != -1;
}

uint64_t DomNode::AddHandler(Events ev, EventHandlerF func, EventHandlerFlags flags, void* context, uint32_t timerPeriodMS) {
	// Fastest allowable timer is 1ms
	if (ev == EventTimer)
		timerPeriodMS = Max<uint32_t>(timerPeriodMS, 1);

	for (size_t i = 0; i < Handlers.size(); i++) {
		if (Handlers[i].Context == context && Handlers[i].Func == func) {
			XO_ASSERT(flags == Handlers[i].Flags);
			Handlers[i].Mask |= ev;
			Handlers[i].TimerPeriodMS = timerPeriodMS;
			RecalcAllEventMask();
			return Handlers[i].ID;
		}
	}
	auto& h         = Handlers.add();
	h.ID            = NextEventHandlerID++;
	h.Context       = context;
	h.Func          = func;
	h.Mask          = ev;
	h.TimerPeriodMS = timerPeriodMS;
	h.Flags         = flags;
	RecalcAllEventMask();
	return h.ID;
}

uint64_t DomNode::AddHandler(Events ev, EventHandlerLambda0 lambda) {
	EventHandlerLambda0* copy = new EventHandlerLambda0(lambda);
	return AddHandler(ev, EventHandler_LambdaStaticFunc0, EventHandlerFlag_IsLambda0, copy, 0);
}

uint64_t DomNode::AddHandler(Events ev, EventHandlerLambda1 lambda) {
	EventHandlerLambda1* copy = new EventHandlerLambda1(lambda);
	return AddHandler(ev, EventHandler_LambdaStaticFunc1, EventHandlerFlag_IsLambda1, copy, 0);
}

uint64_t DomNode::AddTimerHandler(Events ev, EventHandlerLambda0 lambda, uint32_t periodMS) {
	EventHandlerLambda0* copy = new EventHandlerLambda0(lambda);
	return AddHandler(ev, EventHandler_LambdaStaticFunc0, EventHandlerFlag_IsLambda0, copy, periodMS);
}

uint64_t DomNode::AddTimerHandler(Events ev, EventHandlerLambda1 lambda, uint32_t periodMS) {
	EventHandlerLambda1* copy = new EventHandlerLambda1(lambda);
	return AddHandler(ev, EventHandler_LambdaStaticFunc1, EventHandlerFlag_IsLambda1, copy, periodMS);
}

uint64_t DomNode::AddHandler(Events ev, EventHandlerF func, void* context) {
	return AddHandler(ev, func, EventHandlerFlag_None, context, 0);
}

void DomNode::RemoveHandler(uint64_t id) {
	for (size_t i = 0; i < Handlers.size(); i++) {
		if (Handlers[i].ID == id) {
			Handlers.erase(i);
			return;
		}
	}
}

void DomNode::RemoveAllHandlers() {
	Handlers.clear();
}

EventHandler* DomNode::HandlerByID(uint64_t id) {
	for (size_t i = 0; i < Handlers.size(); i++) {
		if (Handlers[i].ID == id)
			return &Handlers[i];
	}
	return nullptr;
}

uint32_t DomNode::FastestTimerMS() const {
	uint32_t f = UINT32_MAX - 1;
	for (const auto& h : Handlers) {
		if (h.TimerPeriodMS != 0) {
			XO_ASSERT(!!(h.Mask & EventTimer));
			f = Min(f, h.TimerPeriodMS);
		}
	}
	return f != UINT32_MAX - 1 ? f : 0;
}

// This function is not const, because we're going to update EventHandler.TimerLastTickMS after retrieving the list
void DomNode::ReadyTimers(int64_t nowTicksMS, cheapvec<NodeEventIDPair>& handlers) {
	auto ms32 = MilliTicks_32(nowTicksMS);
	for (auto& h : Handlers) {
		if (h.TimerPeriodMS != 0 && ms32 - h.TimerLastTickMS > h.TimerPeriodMS)
			handlers.push({InternalID, h.ID});
	}
}

void DomNode::RenderHandlers(cheapvec<NodeEventIDPair>& handlers) const {
	for (auto& h : Handlers) {
		if (!!(h.Mask & EventRender))
			handlers.push({InternalID, h.ID});
	}
}

void DomNode::DocProcessHandlers(cheapvec<NodeEventIDPair>& handlers) {
	for (auto& h : Handlers) {
		if (!!(h.Mask & EventDocProcess))
			handlers.push({InternalID, h.ID});
	}
}

bool DomNode::HasFocus() const {
	return Doc->UI.IsFocused(InternalID);
}

void DomNode::SetCapture() const {
	Doc->UI.SetCapture(InternalID);
}

void DomNode::ReleaseCapture() const {
	Doc->UI.ReleaseCapture(InternalID);
}

void DomNode::RecalcAllEventMask() {
	bool hadTimer      = !!(AllEventMask & EventTimer);
	bool hadRender     = !!(AllEventMask & EventRender);
	bool hadDocProcess = !!(AllEventMask & EventDocProcess);

	uint32_t m = 0;
	for (size_t i = 0; i < Handlers.size(); i++)
		m |= Handlers[i].Mask;
	AllEventMask = m;

	bool hasTimerNow      = !!(AllEventMask & EventTimer);
	bool hasRenderNow     = !!(AllEventMask & EventRender);
	bool hasDocProcessNow = !!(AllEventMask & EventDocProcess);

	if (!hadTimer && hasTimerNow)
		Doc->NodeGotTimer(InternalID);
	else if (hadTimer && !hasTimerNow)
		Doc->NodeLostTimer(InternalID);

	if (!hadRender && hasRenderNow)
		Doc->NodeGotRender(InternalID);
	else if (hadRender && !hasRenderNow)
		Doc->NodeLostRender(InternalID);

	if (!hadDocProcess && hasDocProcessNow)
		Doc->NodeGotDocProcess(InternalID);
	else if (hadRender && !hasRenderNow)
		Doc->NodeLostDocProcess(InternalID);
}

void DomNode::DeleteChildInternal(DomEl* c) {
	auto node = c->ToNode();
	if (node)
		node->Clear();
	Doc->ChildRemoved(c);
	Doc->FreeChild(c);
}
} // namespace xo
