#pragma once

#include "../xoDefs.h"
#include "../xoStyle.h"
#include "../Render/xoRenderStack.h"
#include "../Text/xoGlyphCache.h"
#include "../Text/xoFontStore.h"
#include "../xoMem.h"
#include "xoBoxLayout3.h"

/* This performs box layout.

Inside the class we make some separation between text and non-text layout,
because we will probably end up splitting the text stuff out into a separate
class, due to the fact that it gets complex if you're doing it properly
(ie non-latin fonts, bidirectional, asian, etc).

Hidden things that would bite you if you tried to multithread this:
* We get kerning data from Freetype for each glyph pair, and I'm not sure
that is thread safe.

*/
class XOAPI xoLayout3
{
public:
	void Layout(const xoDoc& doc, xoRenderDomNode& root, xoPool* pool);

protected:

	// Packed set of bindings between child and parent node
	struct BindingSet
	{
		xoHorizontalBindings	HChildLeft : 8;
		xoHorizontalBindings	HChildCenter : 8;
		xoHorizontalBindings	HChildRight : 8;

		xoVerticalBindings		VChildTop : 8;
		xoVerticalBindings		VChildCenter : 8;
		xoVerticalBindings		VChildBottom : 8;
		xoVerticalBindings		VChildBaseline : 8;
	};

	struct LayoutInput3
	{
		podvec<int32>*		RestartPoints;	// This is IN/OUT
		xoRenderDomNode*	ParentRNode;
		xoPos				ParentWidth;
		xoPos				ParentHeight;
	};

	struct LayoutOutput3
	{
		BindingSet			Binds;
		xoPos				MarginBoxWidth;
		xoPos				MarginBoxHeight;
		xoPos				Baseline;		// This is given in the coordinate system of the parent
		xoRenderDomEl*		RNode;
		xoBreakType			Break;
		xoPos				BaselineInParent() const;
	};

	enum ChunkType
	{
		ChunkSpace,
		ChunkLineBreak,
		ChunkWord
	};

	struct Chunk
	{
		int32		Start;
		int32		End;
		ChunkType	Type;
	};

	struct TextRunState
	{
		const xoDomText*			Node;
		xoRenderDomNode*			RNode;		// Parent of the text nodes
		xoRenderDomText*			RNodeTxt;	// Child of RNode
		xoRingBuf<xoRenderCharEl>	Chars;
		bool						GlyphsNeeded;
		bool						IsSubPixel;
		podvec<int32>*				RestartPoints;
		float						FontWidthScale;
		int							FontSizePx;
		xoPos						FontAscender;
		xoFontID					FontID;
		xoColor						Color;
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
	xoBoxLayout3				Boxer;
	xoPool*						Pool;
	xoRenderStack				Stack;
	xoFixedSizeHeap				FHeap;
	float						PtToPixel;
	float						EpToPixel;
	xoFontTableImmutable		Fonts;
	ohash::set<xoGlyphCacheKey>	GlyphsNeeded;
	TextRunState				TempText;
	bool						SnapBoxes;
	bool						SnapSubpixelHorzText;
	bool						EnableKerning;

	void		RenderGlyphsNeeded();
	void		LayoutInternal(xoRenderDomNode& root);
	void		RunNode3(const xoDomNode* node, const LayoutInput3& in, LayoutOutput3& out);
	void		RunText3(const xoDomText* node, const LayoutInput3& in, LayoutOutput3& out);
	xoPoint		PositionChildFromBindings(const LayoutInput3& cin, xoPos parentBaseline, LayoutOutput3& cout);
	void		GenerateTextWords(TextRunState& ts);
	void		FinishTextRNode(TextRunState& ts, xoRenderDomText* rnode, intp numChars);
	void		OffsetTextHorz(TextRunState& ts, xoPos offsetHorz, intp numChars);
	xoPos		MeasureWord(const char* txt, const xoFont* font, xoPos fontAscender, Chunk chunk, TextRunState& ts);

	xoPos		ComputeDimension(xoPos container, xoStyleCategories cat);
	xoPos		ComputeDimension(xoPos container, xoSize size);
	xoBox		ComputeBox(xoPos containerWidth, xoPos containerHeight, xoStyleCategories cat);
	xoBox		ComputeBox(xoPos containerWidth, xoPos containerHeight, xoStyleBox box);
	BindingSet	ComputeBinds();

	xoPos		HoriAdvance(const xoGlyph* glyph, const TextRunState& ts);

	static xoPos			HBindOffset(xoHorizontalBindings bind, xoPos left, xoPos width);
	static xoPos			VBindOffset(xoVerticalBindings bind, xoPos top, xoPos baseline, xoPos height);
	static bool				IsSpace(int ch);
	static bool				IsLinebreak(int ch);
	static xoGlyphCacheKey	MakeGlyphCacheKey(xoRenderDomText* rnode);
	static xoGlyphCacheKey	MakeGlyphCacheKey(const TextRunState& ts);
	static xoGlyphCacheKey	MakeGlyphCacheKey(bool isSubPixel, xoFontID fontID, int fontSizePx);
	static bool				IsAllZeros(const podvec<int32>& list);
	static void				MoveLeftTop(xoRenderDomEl* relem, xoPoint delta);

	static bool				IsDefined(xoPos p)	{ return p != xoPosNULL; }
	static bool				IsNull(xoPos p)		{ return p == xoPosNULL; }

	// Break a string up into chunks, where each chunk is either a word, or
	// a series of one or more identical whitespace characters. A linebreak
	// such as \r\n is emitted as a single chunk.
	// When I get around to it, this will probably do UTF8 interpretation too
	class Chunker
	{
	public:
		Chunker(const char* txt);

		// Returns false when there are no more chunks
		bool Next(Chunk& c);

	private:
		const char* Txt;
		int32		Pos;
	};

	// These helpers make the binding code a lot less repetitive.
	class VBindHelper
	{
	public:
		xoPos ParentHeight;
		xoPos ParentBaseline;
		xoPos ChildTop;
		xoPos ChildHeight;
		xoPos ChildBaseline;

 		VBindHelper(xoPos parentHeight, xoPos parentBaseline, xoPos childTop, xoPos childHeight, xoPos childBaseline) :
			ParentHeight(parentHeight), ParentBaseline(parentBaseline), ChildTop(childTop), ChildHeight(childHeight), ChildBaseline(childBaseline) {}

		xoPos Parent(xoVerticalBindings bind);
		xoPos Child(xoVerticalBindings bind);
		xoPos Delta(xoVerticalBindings parent, xoVerticalBindings child);
	};

	class HBindHelper
	{
	public:
		xoPos ParentWidth;
		xoPos ChildLeft;
		xoPos ChildWidth;

		HBindHelper(xoPos parentWidth, xoPos childLeft, xoPos childWidth) :
			ParentWidth(parentWidth), ChildLeft(childLeft), ChildWidth(childWidth) {}

		xoPos Parent(xoHorizontalBindings bind);
		xoPos Child(xoHorizontalBindings bind);
		xoPos Delta(xoHorizontalBindings parent, xoHorizontalBindings child);
	};

};
