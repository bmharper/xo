#pragma once

#include "../Defs.h"
#include "../Style.h"
#include "../Render/RenderStack.h"
#include "../Text/GlyphCache.h"
#include "../Text/FontStore.h"
#include "../Base/MemPoolsAndContainers.h"

namespace xo {

/* Box placement.
This system receives abstract boxes, and spits out their positions.
We make the conscious decision to let this class deal only with
"boxy" decisions. In other words, this does not walk the DOM tree,
or understand how to resolve styles, etc. The only job of this
class is to execute the layout algorithm.

Inputs are given via BeginNode()/EndNode().
Outputs are sent into RenderDomNode objects that enter via BeginNode().

I initially thought that output of the boxer should be some abstract data
structure, and not RenderDomNode. After all, this thing is supposed to be
concerned only with boxes, not the final render representation.
But if we don't use RenderDomNode, then it means ANOTHER stage
where we need to build up a full tree of objects. So that's why we write
our results directly into RenderDomNode objects. Of course, we don't populate
fields such as colors - we only populate the positional fields.

Note that we have an AddWord() function, but that doesn't mean that words
are a particularly special type of box. Words are flowed like any other
box. The reason they have their own function is for efficiency.

Coordinate Space Of Injected-Flow Nodes

It's not obvious what the coordinate system should be of nodes that
do not define their own flow context. The route we take here is to say that
nodes without their own flow context are defined in the coordinate space
of their most recent ancestor with a flow context.
*/
class XO_API BoxLayout {
public:
	enum FlowResult {
		FlowNormal,
		FlowRestart,
	};
	struct NodeInput {
		xo::InternalID InternalID;
		xo::Tag        Tag;
		Box            MarginBorderPadding; // Sum of Margin, Border, and Padding boxes
		Pos            ContentWidth;
		Pos            ContentHeight;
		BumpStyle      Bump;
		bool           NewFlowContext;
		PositionType   Position;
	};
	struct WordInput {
		Pos Width;
		Pos Height;
	};
	// Every time we start a new line, another one of these is created
	struct LineBox {
		Pos            InnerBaseline;
		int            InnerBaselineDefinedBy;
		int            LastChild; // This is used to keep track of which line each child is on.
		static LineBox Make(Pos innerBaseline, int innerBaselineDefinedBy, int lastChild) { return {innerBaseline, innerBaselineDefinedBy, lastChild}; }
		static LineBox MakeFresh() { return Make(PosNULL, 0, INT32_MAX); }
	};

	xo::Pool* Pool = nullptr;

	BoxLayout();
	~BoxLayout();

	void BeginDocument();
	void EndDocument();

	void       BeginNode(const NodeInput& in);
	FlowResult EndNode(Box& marginBox);

	FlowResult AddWord(const WordInput& in, Box& marginBox);
	Pos        AddSpace(Pos size);
	void       AddNewLineCharacter(Pos height); // Doesn't call Linebreak. Just makes sure that PosMajor is bumped down.

	void Linebreak();
	void NotifyNodeEmitted(Pos baseline, int child); // Notify the boxer of a new node. Set the baseline of the current line box, but only if it's null
	Pos  RemainingSpaceX() const;                    // Return amount of horizontal space remaining
	Pos  RemainingSpaceY() const;                    // Return amount of vertical space remaining

	// Retrieve the linebox at line_index, from the node that has most recently been finished with EndNode.
	// It may seem strange that we can retrieve linebox data from a node that has already ended,
	// because surely we have popped that data structure off it's stack? Yes, we have, but we
	// operate our stack in such a way that we don't wipe the data, we simply decrement a counter.
	// Provided a new node hasn't been started yet, that old data is still there, 100% intact.
	LineBox* GetLineFromPreviousNode(int line_index);

	void Restart();           // The layout engine is about to restart layout, after receiving FlowRestart
	bool WouldFlow(Pos size); // Returns true if adding a box of this size would cause us to flow onto a new line

protected:
	struct FlowState {
		bool FlowOnZeroMinor; // If true, allow flowing to next line even when PosMinor = 0. This exists for injected flow.
		Pos  PosMinor;        // In default flow, this is the horizontal (X) position
		Pos  PosMajor;        // In default flow, this is the vertical (Y) position
		Pos  MaxMinor;        // In default flow, this is the horizontal (X) position at which we wrap around. PosNULL means no limit.
		Pos  MaxMajor;        // In default flow, this is the vertical (Y) position at which we need scroll bars. PosNULL means no limit.
		Pos  HighMinor;       // In default flow, this is the greatest horizontal (X) coordinate seen so far.
		Pos  HighMajor;       // In default flow, this is the greatest vertical (Y) coordinate seen so far.
		// Meh -- implement these when the need arises
		// bool	IsVertical;		// default true, normal flow
		// bool	ReverseMajor;	// Major goes from high to low numbers (right to left, or bottom to top)
		// bool	ReverseMinor;	// Minor goes from high to low numbers (right to left, or bottom to top)
		cheapvec<LineBox> Lines;
		void              Reset();
	};
	struct NodeState {
		NodeInput Input;
		Box       MarginBox;
	};

	Stack<FlowState> FlowStates; // We need to be careful to manage the heap-allocated memory inside FlowState.Lines
	Stack<NodeState> NodeStates;
	bool             WaitingForRestart;

	FlowResult EndNodeInternal(Box& marginBox, bool insideInjectedFlow);
	bool       MustFlow(const FlowState& flow, Pos size);
	void       Flow(const NodeState& ns, FlowState& flow, Box& marginBox);
	void       NewLine(FlowState& flow);
	size_t     MostRecentUniqueFlowAncestor();
};
} // namespace xo
