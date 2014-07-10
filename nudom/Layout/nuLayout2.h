#pragma once

#include "nuDefs.h"
#include "nuStyle.h"
#include "Render/nuRenderStack.h"
#include "Text/nuGlyphCache.h"
#include "Text/nuFontStore.h"

/* This performs box layout.

Inside the class we make some separation between text and non-text layout,
because we will probably end up splitting the text stuff out into a separate
class, due to the fact that it gets complex if you're doing it properly
(ie non-latin fonts, bidirectional, asian, etc).

Hidden things that would bite you if you tried to multithread this:
* We get kerning data from Freetype for each glyph pair, and I'm not sure
that is thread safe.

*/
class NUAPI nuLayout2
{
public:

	void Layout( const nuDoc& doc, u32 docWidth, u32 docHeight, nuRenderDomNode& root, nuPool* pool );

protected:

	// Packed set of bindings between child and parent node
	struct BindingSet
	{
		nuHorizontalBindings	HChild:		8;
		nuHorizontalBindings	HParent:	8;
		nuVerticalBindings		VChild:		8;
		nuVerticalBindings		VParent:	8;
	};

	struct LayoutInput
	{
		nuPos	ParentWidth;
		nuPos	ParentHeight;
		nuPos	ParentBaseline;
	};

	struct LayoutOutput
	{
		nuPos		NodeWidth;
		nuPos		NodeHeight;
		nuPos		NodeBaseline;
		BindingSet	Binds;
	};

	const nuDoc*				Doc;
	u32							DocWidth, DocHeight;
	nuPool*						Pool;
	nuRenderStack				Stack;
	float						PtToPixel;
	float						EpToPixel;
	nuFontTableImmutable		Fonts;
	fhashset<nuGlyphCacheKey>	GlyphsNeeded;

	void		LayoutInternal( nuRenderDomNode& root );
	void		RunNode( const nuDomNode& node, const LayoutInput& in, LayoutOutput& out, nuRenderDomNode* rnode );
	void		PositionChild( const LayoutInput& cin, const LayoutOutput& cout, nuRenderDomNode* rchild );
	void		OffsetRecursive( nuRenderDomNode* rnode, nuPoint offset );

	nuPos		ComputeDimension( nuPos container, nuStyleCategories cat );
	nuPos		ComputeDimension( nuPos container, nuSize size );
	nuBox		ComputeBox( nuPos containerWidth, nuPos containerHeight, nuStyleCategories cat );
	nuBox		ComputeBox( nuPos containerWidth, nuPos containerHeight, nuStyleBox box );
	BindingSet	ComputeBinds();

	static bool IsDefined( nuPos p )	{ return p != nuPosNULL; }
	static bool IsNull( nuPos p )		{ return p == nuPosNULL; }

};




