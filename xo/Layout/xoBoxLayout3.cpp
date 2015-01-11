#include "pch.h"
#include "xoBoxLayout3.h"
#include "xoDoc.h"
#include "Dom/xoDomNode.h"
#include "Dom/xoDomText.h"
#include "Render/xoRenderDomEl.h"
#include "Render/xoStyleResolve.h"
#include "Text/xoFontStore.h"
#include "Text/xoGlyphCache.h"

xoBoxLayout3::xoBoxLayout3()
{
	// This constant is arbitrarily chosen. The program will panic if the limit is exceeded.
	int maxDomTreeDepth = 100;

	FlowStates.Init(maxDomTreeDepth);
	NodeStates.Init(maxDomTreeDepth);
}

xoBoxLayout3::~xoBoxLayout3()
{
}

void xoBoxLayout3::BeginDocument()
{
	// Add a dummy root node. This root node has no limits. This does not house the
	// top-level <body> tag; it is the parent of the <body> tag.
	// This is really just a dummy so that <body> can go through the same code paths as all of its children,
	// and so that we never have an empty NodeStates stack.
	NodeState& s = NodeStates.Add();
	s.Input.NewFlowContext = false;
	FlowState& flow = FlowStates.Add();
	flow.Reset();
}

void xoBoxLayout3::EndDocument()
{
	XOASSERT(NodeStates.Count == 1);
	XOASSERT(FlowStates.Count == 1);
	NodeStates.Pop();
	FlowStates.Pop();
}

void xoBoxLayout3::BeginNode(const NodeInput& in)
{
	NodeState& parentNode = NodeStates.Back();
	NodeState& ns = NodeStates.Add();
	ns.Input = in;
	ns.MarginBox = xoBox(0,0,0,0);
	if (in.NewFlowContext)
	{
		FlowState& flow = FlowStates.Add();
		flow.Reset();
		flow.MaxMinor = in.ContentWidth;
		flow.MaxMajor = in.ContentHeight;
	}
}

void xoBoxLayout3::EndNode(xoBox& marginBox)
{
	NodeState* ns = &NodeStates.Back();
	if (ns->Input.NewFlowContext)
	{
		FlowState& flow = FlowStates.Back();
		
		if (ns->Input.ContentWidth == xoPosNULL)
			ns->Input.ContentWidth = flow.PosMinor;
		
		if (ns->Input.ContentHeight == xoPosNULL)
			ns->Input.ContentHeight = flow.HighMajor;

		FlowStates.Pop();
		Flow(*ns, FlowStates.Back(), ns->MarginBox);
	}

	marginBox = ns->MarginBox;

	NodeStates.Pop(); // 'ns' is invalid after Pop()
}

void xoBoxLayout3::AddWord(const WordInput& in, xoBox& marginBox)
{
	NodeState ns;
	ns.Input.ContentWidth = in.Width;
	ns.Input.ContentHeight = in.Height;
	ns.Input.MarginAndPadding = xoBox(0, 0, 0, 0);
	Flow(ns, FlowStates.Back(), marginBox);
}

void xoBoxLayout3::AddSpace(xoPos size)
{
	FlowStates.Back().PosMinor += size;
}

void xoBoxLayout3::AddLinebreak()
{
	NewLine(FlowStates.Back());
}

bool xoBoxLayout3::WouldFlow(xoPos size)
{
	return MustFlow(FlowStates.Back(), size);
}

bool xoBoxLayout3::MustFlow(const FlowState& flow, xoPos size)
{
	bool overflow = flow.PosMinor + size > flow.MaxMinor;
	bool onNewLine = flow.PosMinor == 0;
	return flow.MaxMinor != xoPosNULL && overflow && !onNewLine;
}

void xoBoxLayout3::Flow(const NodeState& ns, FlowState& flow, xoBox& marginBox)
{
	xoPos marginBoxWidth = ns.Input.MarginAndPadding.Left + ns.Input.MarginAndPadding.Right + (ns.Input.ContentWidth != xoPosNULL ? ns.Input.ContentWidth : 0);
	xoPos marginBoxHeight = ns.Input.MarginAndPadding.Top + ns.Input.MarginAndPadding.Bottom + (ns.Input.ContentHeight != xoPosNULL ? ns.Input.ContentHeight : 0);
	if (MustFlow(flow, marginBoxWidth))
		NewLine(flow);

	marginBox.Left = flow.PosMinor;
	marginBox.Top = flow.PosMajor;
	marginBox.Right = flow.PosMinor + marginBoxWidth;
	marginBox.Bottom = flow.PosMajor + marginBoxHeight;

	flow.PosMinor = marginBox.Right;
	flow.HighMajor = xoMax(flow.HighMajor, marginBox.Bottom);
}

void xoBoxLayout3::NewLine(FlowState& flow)
{
	flow.Lines += LineBox::Make(0, 0, 0);
	flow.PosMajor = flow.HighMajor;
	flow.PosMinor = 0;
}

void xoBoxLayout3::FlowState::Reset()
{
	PosMinor = 0;
	PosMajor = 0;
	HighMajor = 0;
	MaxMajor = xoPosNULL;
	MaxMinor = xoPosNULL;
	Lines.clear_noalloc();
}
