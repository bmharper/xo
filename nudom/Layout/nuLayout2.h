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
		nuPos	OuterBaseline;
	};

	struct LayoutOutput
	{
		BindingSet	Binds;
		nuPos		NodeWidth;
		nuPos		NodeHeight;
		nuPos		NodeBaseline;
		nuBreakType	Break: 2;		// Keep in mind that you need to mask this off against 3 (ie if (x.Break & 3 == nuBreakAfter)), because of sign extension. ie enums are signed.
	};

	struct Word
	{
		nuPos	Width;
		int32	Start;
		int32	End;
		int32	Length() const { return End - Start; }
	};

	struct TextRunState
	{
		const nuDomText*	Node;
		nuRenderDomText*	RNode;
		podvec<Word>		Words;
		int					GlyphCount;		// Number of non-empty glyphs
		bool				GlyphsNeeded;
		float				FontWidthScale;
	};

	struct FlowState
	{
		nuPos	PosMinor;		// In default flow, this is the horizontal (X) position
		nuPos	PosMajor;		// In default flow, this is the vertical (Y) position
		nuPos	MajorMax;		// In default flow, this is the bottom of the current line
		int		NumLines;
		// Meh -- implement these when the need arises
		// bool	IsVertical;		// default true, normal flow
		// bool	ReverseMajor;	// Major goes from high to low numbers (right to left, or bottom to top)
		// bool	ReverseMinor;	// Minor goes from high to low numbers (right to left, or bottom to top)
	};

	const nuDoc*				Doc;
	u32							DocWidth, DocHeight;
	nuPool*						Pool;
	nuRenderStack				Stack;
	float						PtToPixel;
	float						EpToPixel;
	nuFontTableImmutable		Fonts;
	fhashset<nuGlyphCacheKey>	GlyphsNeeded;
	TextRunState				TempText;
	bool						SnapBoxes;
	bool						SnapSubpixelHorzText;

	void		RenderGlyphsNeeded();
	void		LayoutInternal( nuRenderDomNode& root );
	void		RunNode( const nuDomNode& node, const LayoutInput& in, LayoutOutput& out, nuRenderDomNode* rnode );
	void		RunText( const nuDomText& node, const LayoutInput& in, LayoutOutput& out, nuRenderDomText* rnode );
	void		GenerateTextOutput( const LayoutInput& in, LayoutOutput& out, TextRunState& ts );
	nuPoint		PositionChildFromBindings( const LayoutInput& cin, const LayoutOutput& cout, nuRenderDomEl* rchild );
	void		GenerateTextWords( TextRunState& ts );

	nuPos		ComputeDimension( nuPos container, nuStyleCategories cat );
	nuPos		ComputeDimension( nuPos container, nuSize size );
	nuBox		ComputeBox( nuPos containerWidth, nuPos containerHeight, nuStyleCategories cat );
	nuBox		ComputeBox( nuPos containerWidth, nuPos containerHeight, nuStyleBox box );
	BindingSet	ComputeBinds();

	nuPos		HoriAdvance( const nuGlyph* glyph, const TextRunState& ts );

	static nuPos			HBindOffset( nuHorizontalBindings bind, nuPos width );
	static nuPos			VBindOffset( nuVerticalBindings bind, nuPos baseline, nuPos height );
	static bool				IsSpace( int ch );
	static bool				IsLinebreak( int ch );
	static nuGlyphCacheKey	MakeGlyphCacheKey( nuRenderDomText* rnode );
	static void				FlowNewline( FlowState& flow );
	static nuPoint			FlowRun( const LayoutInput& cin, const LayoutOutput& cout, FlowState& flow, nuRenderDomEl* rendEl );

	static bool				IsDefined( nuPos p )	{ return p != nuPosNULL; }
	static bool				IsNull( nuPos p )		{ return p == nuPosNULL; }

};




