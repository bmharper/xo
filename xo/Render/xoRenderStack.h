#pragma once

#include "../xoStyle.h"
#include "../xoMem.h"

// A single item on the render stack
class XOAPI xoRenderStackEl
{
public:
	xoPool*		Pool;		// This *could* be stored only inside xoRenderStack.Stack_Pools, but it is convenient to duplicate it here.
	xoStyleSet	Styles;
	bool		HasHoverStyle : 1;
	bool		HasFocusStyle : 1;

	void Reset();

	// $XO_GCC_ALIGN_BUG.
	// WARNING. This is a straight memcpy
	xoRenderStackEl& operator=(const xoRenderStackEl& b);
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
class XOAPI xoRenderStack
{
public:
	const xoDoc*		Doc;
	xoPool*				Pool;
	xoStyleAttrib		Defaults[xoCatEND];

	xoRenderStack();
	~xoRenderStack();

	void				Initialize(const xoDoc* doc, xoPool* pool);
	void				Reset();
	xoStyleAttrib		Get(xoStyleCategories cat) const;
	void				GetSizeQuad(xoStyleCategories cat, xoSizeQuad& quad) const;
	void				GetColorQuad(xoStyleCategories cat, xoColorQuad& quad) const;

	xoSizeQuad			GetSizeQuad(xoStyleCategories cat) const { xoSizeQuad q; GetSizeQuad(cat, q); return q; }
	xoColorQuad			GetColorQuad(xoStyleCategories cat) const { xoColorQuad q; GetColorQuad(cat, q); return q; }

	bool				HasHoverStyle() const;
	bool				HasFocusStyle() const;

	void				StackPop();
	xoRenderStackEl&	StackPush();
	xoRenderStackEl&	StackBack()			{ return Stack.back(); }
	xoRenderStackEl&	StackAt(intp pos)	{ return Stack[pos]; }
	intp				StackSize() const	{ return Stack.size(); }

protected:
	xoPoolArray<xoRenderStackEl>	Stack;
	pvect<xoPool*>					Stack_Pools;	// Every position on the stack gets its own pool
};