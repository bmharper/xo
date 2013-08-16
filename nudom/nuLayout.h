#pragma once

#include "nuDefs.h"
#include "nuStyle.h"
#include "Render/nuRenderStack.h"

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
	nuRenderStack	Stack;
	float			PtToPixel;

	nuPos	ComputeDimension( nuPos container, nuSize size );
	nuBox	ComputeBox( nuBox container, nuStyleBox box );
	nuBox	ComputeSpecifiedPosition( const NodeState& s );
	void	ComputeRelativeOffset( const NodeState& s, nuBox& box );

	void	Run( NodeState& s, const nuDomEl& node, nuRenderDomEl* rnode );

};




