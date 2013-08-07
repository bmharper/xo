#pragma once

#include "../nuStyle.h"
#include "../nuMem.h"

// A single item on the render stack
class NUAPI nuRenderStackEl
{
public:
	nuStyleSet Styles;
};

/* This is used during layout and rendering.
It is a stack, where the bottom of the stack is the root node, and the
stack follows nodes through the hierarchy up to the current node that
we are processing.
*/
class NUAPI nuRenderStack
{
public:
	const nuDoc*					Doc;
	nuPool*							Pool;
	nuPoolArray<nuRenderStackEl>	Stack;
	//nuStyleAttrib					DefaultWidth, DefaultHeight, DefaultBorderRadius, DefaultDisplay, DefaultPosition;
	//nuStyleBox						DefaultPadding, DefaultMargin;
	nuStyleAttrib					Defaults[nuCatEND];

	void			Initialize( const nuDoc* doc, nuPool* pool );
	nuStyleAttrib	Get( nuStyleCategories cat ) const;
	void			GetBox( nuStyleCategories cat, nuStyleBox& box ) const;
};