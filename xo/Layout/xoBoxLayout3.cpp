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
		flow.MaxMinor = parentFlow.MaxMinor - parentFlow.PosMinor - (ns.Input.MarginBorderPadding.Left + ns.Input.MarginBorderPadding.Right);

		// We also correct the determination of whether we are at the start of a new line
		if (parentFlow.PosMinor != 0 || parentFlow.FlowOnZeroMinor)
			flow.FlowOnZeroMinor = true;
	}
}

xoBoxLayout3::FlowResult xoBoxLayout3::EndNode(xoBox& marginBox)
{
	bool insideInjectedFlow = !NodeStates[NodeStates.Count - 2].Input.NewFlowContext;
	return EndNodeInternal(marginBox, insideInjectedFlow);
}

xoBoxLayout3::FlowResult xoBoxLayout3::AddWord(const WordInput& in, xoBox& marginBox)
{
	NodeInput nin;
	nin.ContentWidth = in.Width;
	nin.ContentHeight = in.Height;
	nin.InternalID = 0;
	nin.MarginBorderPadding = xoBox(0, 0, 0, 0);
	nin.NewFlowContext = true;
	nin.Tag = xoTag_DummyWord;
	nin.Bump = xoBumpRegular;	// should have no effect, because margins,border,padding are all zero
	BeginNode(nin);
	// Words are always inside an injected-flow context. We cannot rely on EndNode()'s logic
	// of determining the flow context, because words aren't added inside a special "text"
	// node, they're just added naked inside their container, which could be a <div> or
	// anything else. Conceptually, it might be better to add a proper node for words,
	// which we would call a "line box", but I fear that might just add confusion.
	return EndNodeInternal(marginBox, true);
}

void xoBoxLayout3::AddSpace(xoPos size)
{
	FlowStates.Back().PosMinor += size;
}

void xoBoxLayout3::AddLinebreak()
{
	NewLine(FlowStates.Back());
}

void xoBoxLayout3::SetBaseline(xoPos baseline, int child)
{
	auto& line = FlowStates.Back().Lines.back();
	if (line.InnerBaseline == xoPosNULL)
	{
		line.InnerBaseline = baseline;
		// We probably also need to record which element set the inner baseline, so that if that
		// element is moved by vertical alignment, then we can move the inner baseline too.
	}
	line.LastChild = child;
}

xoPos xoBoxLayout3::GetBaseline()
{
	auto& line = FlowStates.Back().Lines.back();
	return line.InnerBaseline;
}

xoPos xoBoxLayout3::GetFirstBaseline()
{
	if (FlowStates.Back().Lines.size() != 0)
		return FlowStates.Back().Lines[0].InnerBaseline;
	return xoPosNULL;
}

xoBoxLayout3::LineBox xoBoxLayout3::GetLineFromPreviousNode(int line_index)
{
	// If FlowStates did bounds checking, then the following call would fail that check.
	// We explicitly choose a container for FlowStates that leaves items intact when
	// popping off the end.
	return FlowStates[FlowStates.Count].Lines[line_index];
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

xoBoxLayout3::FlowResult xoBoxLayout3::EndNodeInternal(xoBox& marginBox, bool insideInjectedFlow)
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

	// We are done with the flow state of the node that's ending (ie the flow of objects inside this node).
	FlowStates.Pop();
	flow = nullptr;

	if (!WaitingForRestart && insideInjectedFlow && WouldFlow(ns->Input.ContentWidth))
	{
		// Bail out. The caller is going to have to unwind his stack out to the nearest ancestor
		// that defines its own flow. That ancestor is going to have to create another copy of the
		// child that it was busy with, but this new child will end up on a new line.
		// We don't create the new line here. That happens when Restart() is called.
		NodeStates.Pop();
		// Disable any intermediate nodes from emitting new lines. This is necessary when you have
		// for example, <div><span><span>The quick brown fox</span></span></div>. In other words, when
		// you have nested nodes that do not define their own flow contexts. If we didn't set WaitingForRestart
		// to true here, then the outer span object might want to span itself across two lines, which
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

bool xoBoxLayout3::MustFlow(const FlowState& flow, xoPos size)
{
	bool onNewLine = flow.PosMinor == 0;
	bool overflow = (flow.MaxMinor != xoPosNULL) && (flow.PosMinor + size > flow.MaxMinor);
	return (!onNewLine || flow.FlowOnZeroMinor) && overflow;
}

void xoBoxLayout3::Flow(const NodeState& ns, FlowState& flow, xoBox& marginBox)
{
	bool doVertBump = ns.Input.Bump == xoBumpRegular || ns.Input.Bump == xoBumpVertOnly;
	bool doHorzBump = ns.Input.Bump == xoBumpRegular || ns.Input.Bump == xoBumpHorzOnly;

	xoPos marginBoxWidth = ns.Input.MarginBorderPadding.Left + ns.Input.MarginBorderPadding.Right + (ns.Input.ContentWidth != xoPosNULL ? ns.Input.ContentWidth : 0);
	xoPos marginBoxHeight = ns.Input.MarginBorderPadding.Top + ns.Input.MarginBorderPadding.Bottom + (ns.Input.ContentHeight != xoPosNULL ? ns.Input.ContentHeight : 0);
	
	xoPos widthForFlow = marginBoxWidth;
	if (!doHorzBump)
		widthForFlow = marginBoxWidth - ns.Input.MarginBorderPadding.Left + ns.Input.MarginBorderPadding.Right;

	if (!WaitingForRestart && MustFlow(flow, widthForFlow))
		NewLine(flow);

	marginBox.Left = flow.PosMinor;
	marginBox.Top = flow.PosMajor;
	marginBox.Right = flow.PosMinor + marginBoxWidth;
	marginBox.Bottom = flow.PosMajor + marginBoxHeight;
	
	xoPos right = marginBox.Right;
	if (!doHorzBump)
	{
		// move the box to the left
		marginBox.Left -= ns.Input.MarginBorderPadding.Left;
		marginBox.Right -= ns.Input.MarginBorderPadding.Left;
		// recreate our right-most point, so that it excludes the right 'bump' region
		right = marginBox.Right - ns.Input.MarginBorderPadding.Right;
	}

	xoPos bottom = marginBox.Bottom;
	if (!doVertBump)
	{
		// move the box up
		marginBox.Top -= ns.Input.MarginBorderPadding.Top;
		marginBox.Bottom -= ns.Input.MarginBorderPadding.Top;
		// recreate our lower-most point, so that it excludes the lower 'bump' region
		bottom = marginBox.Bottom - ns.Input.MarginBorderPadding.Bottom;
	}

	flow.PosMinor = right;
	flow.HighMajor = xoMax(flow.HighMajor, bottom);
}

void xoBoxLayout3::NewLine(FlowState& flow)
{
	flow.Lines += LineBox::MakeFresh();
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
	FlowOnZeroMinor = false;
	PosMinor = 0;
	PosMajor = 0;
	HighMajor = 0;
	MaxMajor = xoPosNULL;
	MaxMinor = xoPosNULL;
	Lines.clear_noalloc();
	Lines += LineBox::MakeFresh();
}
