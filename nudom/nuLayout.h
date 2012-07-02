#pragma once

#include "nuDefs.h"
#include "nuStyle.h"

/* A box that has been laid out.
*/
class NUAPI nuLayoutEl
{
public:
	nuBox Box;
};

/* This performs box layout.
*/
class NUAPI nuLayout
{
public:

	void Layout( const nuDoc& doc, nuRenderDomEl& root, nuPool* pool );

protected:

	struct NodeState
	{
		nuBox	PositionedAncestor;
		nuBox	ParentContentBox;
		nuBox	LineBox;
		nuPos	PosX, PosY;
	};

	const nuDoc*	Doc;
	nuPool*			Pool;
	float			PtToPixel;
	nuStyleAttrib	DefaultWidth, DefaultHeight, DefaultBorderRadius, DefaultDisplay, DefaultPosition;
	nuStyleBox		DefaultPadding, DefaultMargin;

	nuPos	ComputeDimension( nuPos container, nuSize size );
	nuBox	ComputeBox( nuBox container, nuStyleBox box );
	nuBox	ComputeSpecifiedPosition( const NodeState& s, const nuStyle& style );
	void	ComputeRelativeOffset( const NodeState& s, const nuStyle& style, nuBox& box );

	void	Reset();
	void	Run( NodeState& s, const nuDomEl& node, nuRenderDomEl* rnode );

};
