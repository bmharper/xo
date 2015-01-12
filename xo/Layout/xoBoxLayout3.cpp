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

	WaitingForRestart = false;
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
	s.Input.Tag = xoTag_DummyRoot;
	FlowState& flow = FlowStates.Add();
	flow.Reset();
	WaitingForRestart = false;
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
	
	FlowState& parentFlow = FlowStates.Back();
	FlowState& flow = FlowStates.Add();
	flow.Reset();
	flow.MaxMinor = in.ContentWidth;
	flow.MaxMajor = in.ContentHeight;

	// If our node does not define it's own flow context, then we artificially limit
	// the flow limits for it. This makes it easy for child nodes to detect when they
	// should issue a flow restart. I'm not sure what to do if in.ContentWidth != xoPosNULL.
	// I don't even know how to think about such an object.
	if (!in.NewFlowContext && in.ContentWidth == xoPosNULL && parentFlow.MaxMinor != xoPosNULL)
	{
		// We preload MaxMinor with the remaining space in our parent, minus our own margin and padding.
		flow.MaxMinor = parentFlow.MaxMinor - parentFlow.PosMinor - (ns.Input.MarginAndPadding.Left + ns.Input.MarginAndPadding.Right);
	}
}

xoBoxLayout3::FlowResult xoBoxLayout3::EndNode(xoBox& marginBox)
{
	// If the node being finished did not define its content width or content height,
	// then read them now from its flow state. Once that's done, we can throw away
	// the flow state of the node that's ending.
	NodeState* ns = &NodeStates.Back();
	FlowState* flow = &FlowStates.Back();
		
	if (ns->Input.ContentWidth == xoPosNULL)
		ns->Input.ContentWidth = flow->PosMinor;
		
	if (ns->Input.ContentHeight == xoPosNULL)
		ns->Input.ContentHeight = flow->HighMajor;

	// We are done with the flow state of the node that's ending.
	FlowStates.Pop();
	flow = nullptr;

	bool insideInjectedFlow = !NodeStates[NodeStates.Count - 2].Input.NewFlowContext;

	if (!WaitingForRestart && insideInjectedFlow && WouldFlow(ns->Input.ContentWidth))
	{
		// Bail out. The caller is going to have to unwind his stack out to the nearest ancestor
		// that defines its own flow. That ancestor is going to have to create another copy of the
		// child that it was busy with, but this new child will end up on a new line. Notice how
		// we are doing NewLine() on our nearest unique-flow ancestor, not our direct parent.
		// This new line is like a placeholder.
		//intp ancestor = MostRecentUniqueFlowAncestor();
		//NewLine(FlowStates[ancestor]);
		NodeStates.Pop();
		// Disable any intermediate nodes from emitting new lines. This is necessary when you have
		// for example, <div><span><span>The quick brown fox</span></span></div>. In other words, when
		// you have nested nodes that do not define their own flow contexts. If we didn't set EnableNewLine
		// to false here, then the outer span object might want to span itself across two lines, which
		// makes no sense. We still need to run the 'Flow' function though, when we end the second span,
		// so that it can calculate its content width.
		WaitingForRestart = true;
		return FlowRestart;
	}

	Flow(*ns, FlowStates.Back(), ns->MarginBox);

	marginBox = ns->MarginBox;

	NodeStates.Pop();
	ns = nullptr;
	return FlowNormal;
}

xoBoxLayout3::FlowResult xoBoxLayout3::AddWord(const WordInput& in, xoBox& marginBox)
{
	//NodeState ns;
	//ns.Input.ContentWidth = in.Width;
	//ns.Input.ContentHeight = in.Height;
	//ns.Input.MarginAndPadding = xoBox(0, 0, 0, 0);
	//Flow(ns, FlowStates.Back(), marginBox);

	NodeInput nin;
	nin.ContentWidth = in.Width;
	nin.ContentHeight = in.Height;
	nin.InternalID = 0;
	nin.MarginAndPadding = xoBox(0,0,0,0);
	nin.NewFlowContext = true;
	nin.Tag = xoTag_DummyWord;
	BeginNode(nin);
	return EndNode(marginBox);
}

void xoBoxLayout3::AddSpace(xoPos size)
{
	FlowStates.Back().PosMinor += size;
}

void xoBoxLayout3::AddLinebreak()
{
	NewLine(FlowStates.Back());
}

void xoBoxLayout3::Restart()
{
	AddLinebreak();
	WaitingForRestart = false;
}

bool xoBoxLayout3::WouldFlow(xoPos size)
{
	return MustFlow(FlowStates.Back(), size);
}

bool xoBoxLayout3::MustFlow(const FlowState& flow, xoPos size)
{
	bool onNewLine = flow.PosMinor == 0;
	bool overflow = (flow.MaxMinor != xoPosNULL) && (flow.PosMinor + size > flow.MaxMinor);
	return !onNewLine && overflow;
}

void xoBoxLayout3::Flow(const NodeState& ns, FlowState& flow, xoBox& marginBox)
{
	xoPos marginBoxWidth = ns.Input.MarginAndPadding.Left + ns.Input.MarginAndPadding.Right + (ns.Input.ContentWidth != xoPosNULL ? ns.Input.ContentWidth : 0);
	xoPos marginBoxHeight = ns.Input.MarginAndPadding.Top + ns.Input.MarginAndPadding.Bottom + (ns.Input.ContentHeight != xoPosNULL ? ns.Input.ContentHeight : 0);
	if (!WaitingForRestart && MustFlow(flow, marginBoxWidth))
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

intp xoBoxLayout3::MostRecentUniqueFlowAncestor()
{
	for (intp i = FlowStates.Count - 1; i >= 0; i--)
	{
		if (NodeStates[i].Input.NewFlowContext)
			return i;
	}
	XOPANIC("First node (aka dummy node) MUST have its own flow context");
	return 0;
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
