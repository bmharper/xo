#pragma once

#include "xoDefs.h"
#include "xoStyle.h"
#include "Render/xoRenderStack.h"
#include "Text/xoGlyphCache.h"
#include "Text/xoFontStore.h"
#include "../xoMem.h"

/* This performs box layout.

Inside the class we make some separation between text and non-text layout,
because we will probably end up splitting the text stuff out into a separate
class, due to the fact that it gets complex if you're doing it properly
(ie non-latin fonts, bidirectional, asian, etc).

Hidden things that would bite you if you tried to multithread this:
* We get kerning data from Freetype for each glyph pair, and I'm not sure
that is thread safe.

*/
class XOAPI xoLayout2
{
public:
	void Layout( const xoDoc& doc, xoRenderDomNode& root, xoPool* pool );

protected:

	// Packed set of bindings between child and parent node
	struct BindingSet
	{
		xoHorizontalBindings	HChild:		8;
		xoHorizontalBindings	HParent:	8;
		xoVerticalBindings		VChild:		8;
		xoVerticalBindings		VParent:	8;
	};

	struct LayoutInput
	{
		xoPos	ParentWidth;
		xoPos	ParentHeight;
		xoPos	OuterBaseline;
	};

	struct LayoutOutput
	{
		BindingSet		Binds;
		xoPos			NodeWidth;
		xoPos			NodeHeight;
		xoPos			NodeBaseline;
		xoBreakType		Break: 2;		// Keep in mind that you need to mask this off against 3 (ie if (x.Break & 3 == xoBreakAfter)), because of sign extension. ie enums are signed.
		//xoPositionType	Position: 3;
		xoBreakType		GetBreak() const		{ return xoBreakType(Break & 3); }
		//xoPositionType	GetPosition() const		{ return xoPositionType(Position & 7); }
	};

	// Every time we start a new line, another one of these is created
	struct LineBox
	{
		xoPos		InnerBaseline;
		int			InnerBaselineDefinedBy;
		int			LastChild;
		static LineBox Make( xoPos innerBaseline, int innerBaselineDefinedBy, int lastChild ) { return {innerBaseline, innerBaselineDefinedBy, lastChild}; }
	};

	struct Word
	{
		xoPos	Width;
		int32	Start;
		int32	End;
		int32	Length() const { return End - Start; }
	};

	struct TextRunState
	{
		const xoDomText*	Node;
		xoRenderDomText*	RNode;
		podvec<Word>		Words;
		int					GlyphCount;		// Number of non-empty glyphs
		bool				GlyphsNeeded;
		float				FontWidthScale;
	};

	struct FlowState
	{
		xoPos	PosMinor;		// In default flow, this is the horizontal (X) position
		xoPos	PosMajor;		// In default flow, this is the vertical (Y) position
		xoPos	MajorMax;		// In default flow, this is the bottom of the current line
		int		NumLines;
		// Meh -- implement these when the need arises
		// bool	IsVertical;		// default true, normal flow
		// bool	ReverseMajor;	// Major goes from high to low numbers (right to left, or bottom to top)
		// bool	ReverseMinor;	// Minor goes from high to low numbers (right to left, or bottom to top)
	};

	const xoDoc*				Doc;
	xoPool*						Pool;
	xoRenderStack				Stack;
	xoLifoBuf					ChildOutStack;
	xoLifoBuf					LineBoxStack;
	float						PtToPixel;
	float						EpToPixel;
	xoFontTableImmutable		Fonts;
	fhashset<xoGlyphCacheKey>	GlyphsNeeded;
	TextRunState				TempText;
	bool						SnapBoxes;
	bool						SnapSubpixelHorzText;

	void		RenderGlyphsNeeded();
	void		LayoutInternal( xoRenderDomNode& root );
	void		RunNode( const xoDomNode& node, const LayoutInput& in, LayoutOutput& out, xoRenderDomNode* rnode );
	void		RunText( const xoDomText& node, const LayoutInput& in, LayoutOutput& out, xoRenderDomText* rnode );
	void		GenerateTextOutput( const LayoutInput& in, LayoutOutput& out, TextRunState& ts );
	xoPoint		PositionChildFromBindings( const LayoutInput& cin, const LayoutOutput& cout, xoRenderDomEl* rchild );
	void		GenerateTextWords( TextRunState& ts );

	xoPos		ComputeDimension( xoPos container, xoStyleCategories cat );
	xoPos		ComputeDimension( xoPos container, xoSize size );
	xoBox		ComputeBox( xoPos containerWidth, xoPos containerHeight, xoStyleCategories cat );
	xoBox		ComputeBox( xoPos containerWidth, xoPos containerHeight, xoStyleBox box );
	BindingSet	ComputeBinds();

	xoPos		HoriAdvance( const xoGlyph* glyph, const TextRunState& ts );

	static xoPos			HBindOffset( xoHorizontalBindings bind, xoPos width );
	static xoPos			VBindOffset( xoVerticalBindings bind, xoPos baseline, xoPos height );
	static bool				IsSpace( int ch );
	static bool				IsLinebreak( int ch );
	static xoGlyphCacheKey	MakeGlyphCacheKey( xoRenderDomText* rnode );
	static void				FlowNewline( FlowState& flow );
	static bool				FlowBreakBefore( const LayoutOutput& cout, FlowState& flow );
	static xoPoint			FlowRun( const LayoutInput& cin, const LayoutOutput& cout, FlowState& flow, xoRenderDomEl* rendEl );
	static xoPoint			ApplyPosition( const LayoutInput& cin, const LayoutOutput& cout, FlowState& flow, xoRenderDomEl* rendEl );

	static bool				IsDefined( xoPos p )	{ return p != xoPosNULL; }
	static bool				IsNull( xoPos p )		{ return p == xoPosNULL; }

};




