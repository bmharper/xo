#include "pch.h"
#include "xoDoc.h"
#include "xoDomNode.h"
#include "xoDomCanvas.h"
#include "../Parse/xoDocParser.h"

xoDomNode::xoDomNode(xoDoc* doc, xoTag tag, xoInternalID parentID) : xoDomEl(doc, tag, parentID)
{
	AllEventMask = 0;
}

xoDomNode::~xoDomNode()
{
	for (intp i = 0; i < Children.size(); i++)
		Doc->FreeChild(Children[i]);
	Children.clear();
}

void xoDomNode::SetText(const char* txt)
{
	if (Children.size() != 1 || Children[0]->GetTag() != xoTagText)
	{
		RemoveAllChildren();
		AddChild(xoTagText);
	}
	Children[0]->SetText(txt);
}

const char* xoDomNode::GetText() const
{
	if (Children.size() == 1 && Children[0]->GetTag() == xoTagText)
	{
		return Children[0]->GetText();
	}
	else
	{
		return "";
	}
}

void xoDomNode::CloneSlowInto(xoDomEl& c, uint cloneFlags) const
{
	CloneSlowIntoBase(c, cloneFlags);
	xoDomNode& cnode = static_cast<xoDomNode&>(c);
	xoDoc* cDoc = c.GetDoc();

	Style.CloneSlowInto(cnode.Style);
	cnode.Classes = Classes;

	// By the time we get here, all relevant DOM elements inside the destination document
	// have already been created. That is why we are not recursive here.
	cnode.Children.clear_noalloc();
	for (intp i = 0; i < Children.size(); i++)
		cnode.Children += cDoc->GetChildByInternalIDMutable(Children[i]->GetInternalID());

	if (!!(cloneFlags & xoCloneFlagEvents))
		XOPANIC("clone events is TODO");
}

void xoDomNode::ForgetChildren()
{
	Children.clear_noalloc();
}

xoDomEl* xoDomNode::AddChild(xoTag tag)
{
	IncVersion();
	xoDomEl* c = Doc->AllocChild(tag, InternalID);
	Children += c;
	Doc->ChildAdded(c);
	return c;
}

xoDomNode* xoDomNode::AddNode(xoTag tag)
{
	AbcAssert(tag != xoTagText);
	return static_cast<xoDomNode*>(AddChild(tag));
}

xoDomCanvas* xoDomNode::AddCanvas()
{
	return static_cast<xoDomCanvas*>(AddChild(xoTagCanvas));
}

xoDomText* xoDomNode::AddText(const char* txt)
{
	xoDomText* el = static_cast<xoDomText*>(AddChild(xoTagText));
	el->SetText(txt);
	return el;
}

void xoDomNode::RemoveChild(xoDomEl* c)
{
	if (!c) return;
	IncVersion();
	intp ix = Children.find(c);
	XOASSERT(ix != -1);
	Children.erase(ix);
	Doc->ChildRemoved(c);
	Doc->FreeChild(c);
}

void xoDomNode::RemoveAllChildren()
{
	IncVersion();
	for (intp i = 0; i < Children.size(); i++)
	{
		Doc->ChildRemoved(Children[i]);
		Doc->FreeChild(Children[i]);
	}
	Children.clear();
}

xoDomEl* xoDomNode::ChildByIndex(intp index)
{
	XOASSERT((uintp) index < (uintp) Children.size());
	return Children[index];
}

const xoDomEl* xoDomNode::ChildByIndex(intp index) const
{
	XOASSERT((uintp) index < (uintp) Children.size());
	return Children[index];
}

void xoDomNode::Discard()
{
	InternalID = 0;
	AllEventMask = 0;
	Version = 0;
	Style.Discard();
	Classes.hack(0, 0, NULL);
	Children.hack(0, 0, NULL);
	Handlers.hack(0, 0, NULL);
}

xoString xoDomNode::Parse(const char* src)
{
	RemoveAllChildren();
	return ParseAppend(src);
}

xoString xoDomNode::ParseAppend(const char* src)
{
	xoDocParser p;
	return p.Parse(src, this);
}

xoString xoDomNode::ParseAppend(const xoStringRaw& src)
{
	return ParseAppend(src.Z);
}

bool xoDomNode::StyleParse(const char* t, intp maxLen)
{
	IncVersion();
	return Style.Parse(t, maxLen, Doc);
}

bool xoDomNode::StyleParsef(const char* t, ...)
{
	char buff[8192];
	va_list va;
	va_start(va, t);
	uint r = vsnprintf(buff, arraysize(buff), t, va);
	va_end(va);
	buff[arraysize(buff) - 1] = 0;
	if (r < arraysize(buff))
	{
		return StyleParse(buff);
	}
	else
	{
		xoString str = xoString(t);
		str.Z[50] = 0;
		xoParseFail("Parse string is too long for StyleParsef: %s...", str.Z);
		XOASSERTDEBUG(false);
		return false;
	}
}

void xoDomNode::HackSetStyle(const xoStyle& style)
{
	IncVersion();
	Style = style;
}

void xoDomNode::HackSetStyle(xoStyleAttrib attrib)
{
	IncVersion();
	Style.Set(attrib);
}

void xoDomNode::AddClass(const char* klass)
{
	IncVersion();
	xoStyleClassID id = Doc->ClassStyles.GetClassID(klass);
	if (Classes.find(id) == -1)
		Classes += id;
}

void xoDomNode::RemoveClass(const char* klass)
{
	IncVersion();
	xoStyleClassID id = Doc->ClassStyles.GetClassID(klass);
	intp index = Classes.find(id);
	if (index != -1)
		Classes.erase(index);
}

void xoDomNode::AddHandler(xoEvents ev, xoEventHandlerF func, bool isLambda, void* context)
{
	for (intp i = 0; i < Handlers.size(); i++)
	{
		if (Handlers[i].Context == context && Handlers[i].Func == func)
		{
			XOASSERT(isLambda == Handlers[i].IsLambda());
			Handlers[i].Mask |= ev;
			RecalcAllEventMask();
			return;
		}
	}
	auto& h = Handlers.add();
	h.Context = context;
	h.Func = func;
	h.Mask = ev;
	if (isLambda)
		h.SetLambda();
	RecalcAllEventMask();
}

void xoDomNode::AddHandler(xoEvents ev, xoEventHandlerLambda lambda)
{
	xoEventHandlerLambda* copy = new xoEventHandlerLambda(lambda);
	AddHandler(ev, xoEventHandler_LambdaStaticFunc, true, copy);
}

void xoDomNode::AddHandler(xoEvents ev, xoEventHandlerF func, void* context)
{
	AddHandler(ev, func, false, context);
}

void xoDomNode::RecalcAllEventMask()
{
	uint32 m = 0;
	for (intp i = 0; i < Handlers.size(); i++)
		m |= Handlers[i].Mask;
	AllEventMask = m;
}
