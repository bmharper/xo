#pragma once
#include "../Style.h"
#include "../Base/MemPoolsAndContainers.h"

namespace xo {

// A single item on the render stack
class XO_API RenderStackEl {
public:
	Pool*    Pool; // This *could* be stored only inside RenderStack.Stack_Pools, but it is convenient to duplicate it here.
	StyleSet Styles;
	bool     HasHoverStyle : 1;
	bool     HasFocusStyle : 1;

	void Reset();

	// $XO_GCC_ALIGN_BUG.
	// WARNING. This is a straight memcpy
	RenderStackEl& operator=(const RenderStackEl& b);
};

/* This is used during layout and rendering.
It is a stack, where the bottom of the stack is the root node, and the
stack follows nodes through the hierarchy up to the current node that
we are processing.

RenderStack has its own pool, which is used for various minor allocations. The bulk of
allocations happen inside the 'Stack_Pools'. Stack_Pools is a list of pools that
is parallel to the stack of RenderStackEl objects. Every object on the stack gets
its own pool, and when that object is popped off the stack, then its pool is
rewound to zero (but its memory is left intact).

The pools inside Stack_Pools have a chunk size of 8 KB. I don't expect resolved
styles to blow that limit often. If they do, then it's simply a performance hit.
*/
class XO_API RenderStack {
public:
	const Doc*  Doc;
	Pool*       Pool;
	StyleAttrib Defaults[CatEND];

	RenderStack();
	~RenderStack();

	void        Initialize(const xo::Doc* doc, xo::Pool* pool);
	void        Reset();
	StyleAttrib Get(StyleCategories cat) const;
	void        GetBox(StyleCategories cat, StyleBox& box) const;

	StyleBox GetBox(StyleCategories cat) const {
		StyleBox b;
		GetBox(cat, b);
		return b;
	}

	bool HasHoverStyle() const;
	bool HasFocusStyle() const;

	void           StackPop();
	RenderStackEl& StackPush();
	RenderStackEl& StackBack() { return Stack.back(); }
	RenderStackEl& StackAt(size_t pos) { return Stack[pos]; }
	size_t         StackSize() const { return Stack.size(); }

protected:
	PoolArray<RenderStackEl> Stack;
	cheapvec<xo::Pool*>      Stack_Pools; // Every position on the stack gets its own pool
};
}
