#pragma once

#include "nuDefs.h"
#include "nuStyle.h"
#include "Render/nuRenderStack.h"
#include "Text/nuGlyphCache.h"

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

	void Layout( const nuDoc& doc, u32 docWidth, u32 docHeight, nuRenderDomEl& root, nuPool* pool );

protected:

	struct NodeState
	{
		nuBox	PositionedAncestor;
		nuBox	ParentContentBox;
		nuBox	LineBox;
		nuPos	PosX, PosY;
	};

	const nuDoc*				Doc;
	u32							DocWidth, DocHeight;
	nuPool*						Pool;
	nuRenderStack				Stack;
	float						PtToPixel;
	fhashset<nuGlyphCacheKey>	GlyphsNeeded;

	nuPos	ComputeDimension( nuPos container, nuSize size );
	nuBox	ComputeBox( nuBox container, nuStyleBox box );
	nuBox	ComputeSpecifiedPosition( const NodeState& s );
	void	ComputeRelativeOffset( const NodeState& s, nuBox& box );

	void	LayoutInternal( nuRenderDomEl& root );
	void	RenderGlyphsNeeded();
	void	Run( NodeState& s, const nuDomEl& node, nuRenderDomEl* rnode );
	void	RunText( NodeState& s, const nuDomEl& node, nuRenderDomEl* rnode );

};




