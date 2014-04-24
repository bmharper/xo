#pragma once

#include "../nuStyle.h"
#include "../nuMem.h"

// A single item on the render stack
class NUAPI nuRenderStackEl
{
public:
	nuStyleSet	Styles;
	nuPool*		Pool;		// This *could* be stored only inside nuRenderStack.Stack_Pools, but it is convenient to duplicate it here.

	void Reset();

	// $NU_GCC_ALIGN_BUG.
	// WARNING. This is a straight memcpy
	nuRenderStackEl& operator=( const nuRenderStackEl& b );
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
class NUAPI nuRenderStack
{
public:
	const nuDoc*					Doc;
	nuPool*							Pool;
	nuStyleAttrib					Defaults[nuCatEND];

						nuRenderStack();
						~nuRenderStack();

	void				Initialize( const nuDoc* doc, nuPool* pool );
	void				Reset();
	nuStyleAttrib		Get( nuStyleCategories cat ) const;
	void				GetBox( nuStyleCategories cat, nuStyleBox& box ) const;
	
	nuStyleBox			GetBox( nuStyleCategories cat ) const { nuStyleBox b; GetBox(cat, b); return b; }
	
	void				StackPop();
	nuRenderStackEl&	StackPush();
	nuRenderStackEl&	StackBack()			{ return Stack.back(); }
	nuRenderStackEl&	StackAt( intp pos )	{ return Stack[pos]; }
	intp				StackSize() const	{ return Stack.size(); }

protected:
	nuPoolArray<nuRenderStackEl>	Stack;
	pvect<nuPool*>					Stack_Pools;	// Every position on the stack gets its own pool
};