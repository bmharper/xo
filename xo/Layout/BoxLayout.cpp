#include "pch.h"
#include "BoxLayout.h"
#include "Doc.h"
#include "Dom/DomNode.h"
#include "Dom/DomText.h"
#include "Render/RenderDomEl.h"
#include "Render/StyleResolve.h"
#include "Text/FontStore.h"
#include "Text/GlyphCache.h"

namespace xo {

BoxLayout::BoxLayout() {
	// This constant is arbitrarily chosen. The program will panic if the limit is exceeded.
	int maxDomTreeDepth = 100;

	FlowStates.Init(maxDomTreeDepth);
	NodeStates.Init(maxDomTreeDepth);

	WaitingForRestart = false;
}

BoxLayout::~BoxLayout() {
}

void BoxLayout::BeginDocument() {
	// Add a dummy root node. This root node has no limits. This is not the <body> tag,
	// it is the parent of the <body> tag. This is really just a dummy so that <body>
	// can go through the same code paths as all of its children, and so that we never
	// have an empty NodeStates stack.
	NodeState& s           = NodeStates.Add();
	s.Input.NewFlowContext = false;
	s.Input.Tag            = Tag_DummyRoot;
	FlowState& flow        = FlowStates.Add();
	flow.Reset();
	WaitingForRestart = false;
}

void BoxLayout::EndDocument() {
	XO_ASSERT(NodeStates.Count == 1);
	XO_ASSERT(FlowStates.Count == 1);
	NodeStates.Pop();
	FlowStates.Pop();
}

void BoxLayout::BeginNode(const NodeInput& in) {
	NodeState& parentNode = NodeStates.Back();
	NodeState& ns         = NodeStates.Add();
	ns.Input              = in;
	ns.MarginBox          = Box(0, 0, 0, 0);

	FlowState& parentFlow = FlowStates.Back();
	FlowState& flow       = FlowStates.Add();
	flow.Reset();
	flow.MaxMinor = in.ContentWidth;
	flow.MaxMajor = in.ContentHeight;

	// If our node does not define it's own flow context, then we artificially limit
	// the flow limits for it. This makes it easy for child nodes to detect when they
	// should issue a flow restart. I'm not sure what to do if in.ContentWidth != PosNULL.
	// I don't even know how to think about such an object.
	if (!in.NewFlowContext && in.ContentWidth == PosNULL && parentFlow.MaxMinor != PosNULL) {
		// We preload MaxMinor with the remaining space in our parent, minus our own margin and padding.
		flow.MaxMinor = parentFlow.MaxMinor - parentFlow.PosMinor - (ns.Input.MarginBorderPadding.Left + ns.Input.MarginBorderPadding.Right);

		// We also correct the determination of whether we are at the start of a new line
		if (parentFlow.PosMinor != 0 || parentFlow.FlowOnZeroMinor)
			flow.FlowOnZeroMinor = true;
	}
}

BoxLayout::FlowResult BoxLayout::EndNode(Box& marginBox) {
	bool insideInjectedFlow = !NodeStates[NodeStates.Count - 2].Input.NewFlowContext;
	return EndNodeInternal(marginBox, insideInjectedFlow);
}

BoxLayout::FlowResult BoxLayout::AddWord(const WordInput& in, Box& marginBox) {
	NodeInput nin;
	nin.ContentWidth        = in.Width;
	nin.ContentHeight       = in.Height;
	nin.InternalID          = 0;
	nin.MarginBorderPadding = Box(0, 0, 0, 0);
	nin.NewFlowContext      = true;
	nin.Tag                 = Tag_DummyWord;
	nin.Bump                = BumpRegular; // should have no effect, because margins,border,padding are all zero
	nin.Position            = PositionStatic;
	BeginNode(nin);
	// Words are always inside an injected-flow context. We cannot rely on EndNode()'s logic
	// of determining the flow context, because words aren't added inside a special "text"
	// node, they're just added naked inside their container, which could be a <div> or
	// anything else. Conceptually, it might be better to add a proper node for words,
	// which we would call a "line box", but I fear that might just add confusion.
	return EndNodeInternal(marginBox, true);
}

Pos BoxLayout::AddSpace(Pos size) {
	auto oldPos = FlowStates.Back().PosMinor;
	FlowStates.Back().PosMinor += size;
	return oldPos;
}

// This is needed for the case where you have a \n\n sequence in a text object.
// In other words, a blank line. Without this explicit call, HighMajor ends up
// being equal to PosMajor, and so the line box has zero height, and the subsequent
// Linebreak call ends up doing nothing.
void BoxLayout::AddNewLineCharacter(Pos height) {
	auto& flow     = FlowStates.Back();
	flow.HighMajor = Max(flow.HighMajor, flow.PosMajor + height);
}

void BoxLayout::Linebreak() {
	NewLine(FlowStates.Back());
}

void BoxLayout::NotifyNodeEmitted(Pos baseline, int child) {
	auto& line = FlowStates.Back().Lines.back();
	if (line.InnerBaseline == PosNULL) {
		line.InnerBaseline          = baseline;
		line.InnerBaselineDefinedBy = child;
	}
	line.LastChild = child;
}

Pos BoxLayout::RemainingSpaceX() const {
	auto& flow = FlowStates.Back();
	if (flow.MaxMinor != PosNULL)
		return flow.MaxMinor - flow.PosMinor;
	return PosNULL;
}

Pos BoxLayout::RemainingSpaceY() const {
	auto& flow = FlowStates.Back();
	if (flow.MaxMajor != PosNULL)
		return flow.MaxMajor - flow.PosMajor;
	return PosNULL;
}

BoxLayout::LineBox* BoxLayout::GetLineFromPreviousNode(int line_index) {
	// If FlowStates did bounds checking, then the following call would fail that check.
	// We explicitly choose a container for FlowStates that leaves items intact when
	// popping off the end.
	return &FlowStates[FlowStates.Count].Lines[line_index];
}

void BoxLayout::Restart() {
	WaitingForRestart = false;
}

bool BoxLayout::WouldFlow(Pos size) {
	return MustFlow(FlowStates.Back(), size);
}

BoxLayout::FlowResult BoxLayout::EndNodeInternal(Box& marginBox, bool insideInjectedFlow) {
	// If the node being finished did not define its content width or content height,
	// then read them now from its flow state. Once that's done, we can throw away
	// the flow state of the node that's ending.
	NodeState* ns   = &NodeStates.Back();
	FlowState* flow = &FlowStates.Back();

	if (ns->Input.ContentWidth == PosNULL) {
		ns->Input.ContentWidth = Max(flow->HighMinor, flow->PosMinor);
		// If you don't round up here, then subpixel glyph positioning ends up producing boxes
		// that have non-integer widths, which ends up producing smudged borders.
		if (Global()->SnapBoxes)
			ns->Input.ContentWidth = PosRoundUp(ns->Input.ContentWidth);
	}

	if (ns->Input.ContentHeight == PosNULL) {
		ns->Input.ContentHeight = flow->HighMajor;
		// Same thing here as the horizontal case above
		if (Global()->SnapBoxes)
			ns->Input.ContentHeight = PosRoundUp(ns->Input.ContentHeight);
	}

	// We are done with the flow state of the node that's ending (ie the flow of objects inside this node).
	FlowStates.Pop();
	flow = nullptr;

	bool isAbsoluteOrFixed = ns->Input.Position == PositionAbsolute || ns->Input.Position == PositionFixed;

	if (!WaitingForRestart && insideInjectedFlow && !isAbsoluteOrFixed && WouldFlow(ns->Input.ContentWidth)) {
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

bool BoxLayout::MustFlow(const FlowState& flow, Pos size) {
	bool onNewLine = flow.PosMinor == 0;
	bool overflow  = (flow.MaxMinor != PosNULL) && (flow.PosMinor + size > flow.MaxMinor);
	return (!onNewLine || flow.FlowOnZeroMinor) && overflow;
}

void BoxLayout::Flow(const NodeState& ns, FlowState& flow, Box& marginBox) {
	bool doVertBump = ns.Input.Bump == BumpRegular || ns.Input.Bump == BumpVertOnly;
	bool doHorzBump = ns.Input.Bump == BumpRegular || ns.Input.Bump == BumpHorzOnly;

	Pos marginBoxWidth  = ns.Input.MarginBorderPadding.Left + ns.Input.MarginBorderPadding.Right + (ns.Input.ContentWidth != PosNULL ? ns.Input.ContentWidth : 0);
	Pos marginBoxHeight = ns.Input.MarginBorderPadding.Top + ns.Input.MarginBorderPadding.Bottom + (ns.Input.ContentHeight != PosNULL ? ns.Input.ContentHeight : 0);

	Pos widthForFlow = marginBoxWidth;
	if (!doHorzBump)
		widthForFlow = marginBoxWidth - ns.Input.MarginBorderPadding.Left + ns.Input.MarginBorderPadding.Right;

	bool isAbsoluteOrFixed = ns.Input.Position == PositionAbsolute || ns.Input.Position == PositionFixed;

	if (!WaitingForRestart && !isAbsoluteOrFixed && MustFlow(flow, widthForFlow))
		NewLine(flow);

	marginBox.Left   = flow.PosMinor;
	marginBox.Top    = flow.PosMajor;
	marginBox.Right  = flow.PosMinor + marginBoxWidth;
	marginBox.Bottom = flow.PosMajor + marginBoxHeight;

	Pos right = marginBox.Right;
	if (!doHorzBump) {
		// move the box to the left
		marginBox.Left -= ns.Input.MarginBorderPadding.Left;
		marginBox.Right -= ns.Input.MarginBorderPadding.Left;
		// recreate our right-most point, so that it excludes the right 'bump' region
		right = marginBox.Right - ns.Input.MarginBorderPadding.Right;
	}

	Pos bottom = marginBox.Bottom;
	if (!doVertBump) {
		// move the box up
		marginBox.Top -= ns.Input.MarginBorderPadding.Top;
		marginBox.Bottom -= ns.Input.MarginBorderPadding.Top;
		// recreate our lower-most point, so that it excludes the lower 'bump' region
		bottom = marginBox.Bottom - ns.Input.MarginBorderPadding.Bottom;
	}

	if (ns.Input.Position == PositionStatic || ns.Input.Position == PositionRelative) {
		flow.PosMinor  = right;
		flow.HighMajor = Max(flow.HighMajor, bottom);
	}
}

void BoxLayout::NewLine(FlowState& flow) {
	flow.Lines += LineBox::MakeFresh();
	flow.PosMajor  = flow.HighMajor;
	flow.HighMinor = Max(flow.HighMinor, flow.PosMinor);
	flow.PosMinor  = 0;
}

size_t BoxLayout::MostRecentUniqueFlowAncestor() {
	for (size_t i = FlowStates.Count - 1; i != -1; i--) {
		if (NodeStates[i].Input.NewFlowContext)
			return i;
	}
	XO_DIE_MSG("First node (aka dummy node) MUST have its own flow context");
	return 0;
}

void BoxLayout::FlowState::Reset() {
	FlowOnZeroMinor = false;
	PosMinor        = 0;
	PosMajor        = 0;
	HighMinor       = 0;
	HighMajor       = 0;
	MaxMajor        = PosNULL;
	MaxMinor        = PosNULL;
	Lines.clear_noalloc();
	Lines += LineBox::MakeFresh();
}
}
