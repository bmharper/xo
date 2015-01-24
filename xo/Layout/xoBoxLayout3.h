#pragma once

#include "../xoDefs.h"
#include "../xoStyle.h"
#include "../Render/xoRenderStack.h"
#include "../Text/xoGlyphCache.h"
#include "../Text/xoFontStore.h"
#include "../xoMem.h"

/* Box placement.
This system receives abstract boxes, and spits out their positions.
We make the conscious decision to let this class deal only with
"boxy" decisions. In other words, this does not walk the DOM tree,
or understand how to resolve styles, etc. This only job of this
class is to execute the layout algorithm.

Inputs are given via BeginNode()/EndNode().
Outputs are sent into xoRenderDomNode objects that enter via BeginNode().

Actually, now I'm thinking that output shouldn't be xoRenderDomNode. After all,
this thing is supposed to be concerned with boxes. It's input is boxes, and
it's output is boxes. Why complicate it?
But then again - if we don't use xoRenderDomNode, then it means ANOTHER stage
where we need to build up a full tree of objects. So I'm going back on that
decision, and rather having this class write its results directly into
xoRenderDomNode objects, but only concern itself with the spatial stuff there.

Note that we have an AddWord() function, but that doesn't mean that words
are a particularly special type of object. Words are flowed like any other
box. The reason they have their own function is for efficiency.

Coordinate Space of parent-flow nodes
It's not obvious what the coordinate system should be of nodes that
do not define their own flow context. The route we take here is to say that
nodes without their own flow context are defined in the coordinate space
of their most recent ancestor with a flow context.
*/
class XOAPI xoBoxLayout3
{
public:
	enum FlowResult
	{
		FlowNormal,
		FlowRestart,
	};
	struct NodeInput
	{
		xoInternalID	InternalID;
		xoTag			Tag;
		xoBox			MarginBorderPadding;	// Sum of Margin, Border, and Padding boxes
		xoPos			ContentWidth;
		xoPos			ContentHeight;
		xoBumpStyle		Bump;
		bool			NewFlowContext;
	};
	struct WordInput
	{
		xoPos			Width;
		xoPos			Height;
	};
	// Every time we start a new line, another one of these is created
	struct LineBox
	{
		xoPos		InnerBaseline;
		int			InnerBaselineDefinedBy;
		int			LastChild;					// This is used to keep track of which line each child is on.
		static LineBox Make(xoPos innerBaseline, int innerBaselineDefinedBy, int lastChild) { return{ innerBaseline, innerBaselineDefinedBy, lastChild }; }
		static LineBox MakeFresh()															{ return{ xoPosNULL, 0, INT32MAX }; }
	};

	xoPool*	Pool = nullptr;

	xoBoxLayout3();
	~xoBoxLayout3();

	void				BeginDocument();
	void				EndDocument();

	void				BeginNode(const NodeInput& in);
	FlowResult			EndNode(xoBox& marginBox);

	FlowResult			AddWord(const WordInput& in, xoBox& marginBox);
	void				AddSpace(xoPos size);
	void				AddLinebreak();
	void				SetBaseline(xoPos baseline, int child);	// Set the baseline of the current line box, but only if it's null
	xoPos				GetBaseline();							// Retrieve the baseline of the current line box
	xoPos				GetFirstBaseline();						// Retrieve the baseline of the first line box. This is the outer baseline of a node.
	
	// Retrieve the linebox 'line_index', from the node that has most recently been finished with EndNode
	// It may seem strange that we can retrieve linebox data from a node that has already ended,
	// because surely we have popped that data structure off it's stack? Yes, we have, but we
	// operate our stack in such a way that we don't wipe the data, we simply decrement a counter.
	// Provided a new node hasn't been started yet, that old data is still there, 100% intact.
	LineBox*			GetLineFromPreviousNode(int line_index);

	void				Restart();								// The layout engine is about to restart layout, after receiving FlowRestart
	bool				WouldFlow(xoPos size);					// Returns true if adding a box of this size would cause us to flow onto a new line

protected:
	struct FlowState
	{
		bool				FlowOnZeroMinor;	// If true, allow flowing to next line even when PosMinor = 0. This exists for injected flow.
		xoPos				PosMinor;			// In default flow, this is the horizontal (X) position
		xoPos				PosMajor;			// In default flow, this is the vertical (Y) position
		xoPos				MaxMinor;			// In default flow, this is the horizontal (X) position at which we wrap around. xoPosNULL means no limit.
		xoPos				MaxMajor;			// In default flow, this is the vertical (Y) position at which we need scroll bars. xoPosNULL means no limit.
		xoPos				HighMinor;			// In default flow, this is the greatest horizontal (X) coordinate seen so far.
		xoPos				HighMajor;			// In default flow, this is the greatest vertical (Y) coordinate seen so far.
		// Meh -- implement these when the need arises
		// bool	IsVertical;		// default true, normal flow
		// bool	ReverseMajor;	// Major goes from high to low numbers (right to left, or bottom to top)
		// bool	ReverseMinor;	// Minor goes from high to low numbers (right to left, or bottom to top)
		podvec<LineBox>		Lines;
		void				Reset();
	};
	struct NodeState
	{
		NodeInput			Input;
		xoBox				MarginBox;
	};

	xoStack<FlowState>		FlowStates;			// We need to be careful to manage the heap-allocated memory inside FlowState.Lines
	xoStack<NodeState>		NodeStates;
	bool					WaitingForRestart;

	FlowResult		EndNodeInternal(xoBox& marginBox, bool insideInjectedFlow);
	bool			MustFlow(const FlowState& flow, xoPos size);
	void			Flow(const NodeState& ns, FlowState& flow, xoBox& marginBox);
	void			NewLine(FlowState& flow);
	intp			MostRecentUniqueFlowAncestor();
};