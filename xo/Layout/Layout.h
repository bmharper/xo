#pragma once
#include "../Defs.h"
#include "../Style.h"
#include "../Render/RenderStack.h"
#include "../Text/GlyphCache.h"
#include "../Text/FontStore.h"
#include "../Base/MemPoolsAndContainers.h"
#include "BoxLayout.h"

namespace xo {

/* This performs box layout.

Inside the class we make some separation between text and non-text layout,
because we will probably end up splitting the text stuff out into a separate
class, due to the fact that it gets complex if you're doing it properly
(ie non-latin fonts, bidirectional, asian, etc).

Hidden things that would bite you if you tried to multithread this:
* We get kerning data from Freetype for each glyph pair, and I'm not sure
that is thread safe.

*/
class XO_API Layout {
public:
	void PerformLayout(const Doc& doc, RenderDomNode& root, Pool* pool);

protected:
	// Packed set of bindings between child and parent node
	struct BindingSet {
		HorizontalBindings HChildLeft : 8;
		HorizontalBindings HChildCenter : 8;
		HorizontalBindings HChildRight : 8;

		VerticalBindings VChildTop : 8;
		VerticalBindings VChildCenter : 8;
		VerticalBindings VChildBottom : 8;
		VerticalBindings VChildBaseline : 8;
	};

	struct LayoutInput {
		cheapvec<int32_t>* RestartPoints; // This is IN/OUT
		RenderDomNode*     ParentRNode;
		Pos                ParentWidth;
		Pos                ParentHeight;
	};

	struct LayoutOutput {
		BindingSet   Binds;
		Pos          MarginBoxWidth;
		Pos          MarginBoxHeight;
		Pos          Baseline; // This is given in the coordinate system of the parent
		RenderDomEl* RNode;
		BreakType    Break;
		Pos          BaselinePlusRNodeTop() const;
	};

	enum ChunkType {
		ChunkSpace,
		ChunkLineBreak,
		ChunkWord
	};

	struct Chunk {
		int32_t   Start;
		int32_t   End;
		ChunkType Type;
	};

	struct TextRunState {
		const DomText*        Node;
		RenderDomNode*        RNode;    // Parent of the text nodes
		RenderDomText*        RNodeTxt; // Child of RNode
		RingBuf<RenderCharEl> Chars;
		bool                  GlyphsNeeded;
		bool                  IsSubPixel;
		cheapvec<int32_t>*    RestartPoints;
		float                 FontWidthScale;
		int                   FontSizePx;
		Pos                   FontAscender;
		FontID                FontID;
		Color                 Color;
	};

	struct FlowState {
		Pos PosMinor; // In default flow, this is the horizontal (X) position
		Pos PosMajor; // In default flow, this is the vertical (Y) position
		Pos MajorMax; // In default flow, this is the bottom of the current line
		int NumLines;
		// Meh -- implement these when the need arises
		// bool	IsVertical;		// default true, normal flow
		// bool	ReverseMajor;	// Major goes from high to low numbers (right to left, or bottom to top)
		// bool	ReverseMinor;	// Minor goes from high to low numbers (right to left, or bottom to top)
	};

	const Doc*                Doc;
	BoxLayout                 Boxer;
	Pool*                     Pool;
	RenderStack               Stack;
	FixedSizeHeap             FHeap;
	float                     PtToPixel;
	float                     EpToPixel;
	FontTableImmutable        Fonts;
	ohash::set<GlyphCacheKey> GlyphsNeeded;
	TextRunState              TempText;
	bool                      SnapBoxes;
	bool                      SnapSubpixelHorzText;
	bool                      EnableKerning;

	void  RenderGlyphsNeeded();
	void  LayoutInternal(RenderDomNode& root);
	void  RunNode(const DomNode* node, const LayoutInput& in, LayoutOutput& out);
	void  RunText(const DomText* node, const LayoutInput& in, LayoutOutput& out);
	Point PositionChildFromBindings(const LayoutInput& cin, Pos parentBaseline, LayoutOutput& cout);
	void  GenerateTextWords(TextRunState& ts);
	void  FinishTextRNode(TextRunState& ts, RenderDomText* rnode, size_t numChars);
	void  OffsetTextHorz(TextRunState& ts, Pos offsetHorz, size_t numChars);
	Pos   MeasureWord(const char* txt, const Font* font, Pos fontAscender, Chunk chunk, TextRunState& ts);

	Pos        ComputeDimension(Pos container, StyleCategories cat);
	Pos        ComputeDimension(Pos container, Size size);
	Box        ComputeBox(Pos containerWidth, Pos containerHeight, StyleCategories cat);
	Box        ComputeBox(Pos containerWidth, Pos containerHeight, StyleBox box);
	BindingSet ComputeBinds();

	Pos HoriAdvance(const Glyph* glyph, const TextRunState& ts);

	static Pos           HBindOffset(HorizontalBindings bind, Pos left, Pos width);
	static Pos           VBindOffset(VerticalBindings bind, Pos top, Pos baseline, Pos height);
	static bool          IsSpace(int ch);
	static bool          IsLinebreak(int ch);
	static GlyphCacheKey MakeGlyphCacheKey(RenderDomText* rnode);
	static GlyphCacheKey MakeGlyphCacheKey(const TextRunState& ts);
	static GlyphCacheKey MakeGlyphCacheKey(bool isSubPixel, FontID fontID, int fontSizePx);
	static bool          IsAllZeros(const cheapvec<int32_t>& list);
	static void          MoveLeftTop(RenderDomEl* relem, Point delta);

	static bool IsDefined(Pos p) { return p != PosNULL; }
	static bool IsNull(Pos p) { return p == PosNULL; }

	// Break a string up into chunks, where each chunk is either a word, or
	// a series of one or more identical whitespace characters. A linebreak
	// such as \r\n is emitted as a single chunk.
	// When I get around to it, this will probably do UTF8 interpretation too
	class Chunker {
	public:
		Chunker(const char* txt);

		// Returns false when there are no more chunks
		bool Next(Chunk& c);

	private:
		const char* Txt;
		int32_t     Pos;
	};

	// These helpers make the binding code a lot less repetitive.
	class VBindHelper {
	public:
		Pos ParentHeight;
		Pos ParentBaseline;
		Pos ChildTop;
		Pos ChildHeight;
		Pos ChildBaseline;

		VBindHelper(Pos parentHeight, Pos parentBaseline, Pos childTop, Pos childHeight, Pos childBaseline) : ParentHeight(parentHeight), ParentBaseline(parentBaseline), ChildTop(childTop), ChildHeight(childHeight), ChildBaseline(childBaseline) {}

		Pos Parent(VerticalBindings bind);
		Pos Child(VerticalBindings bind);
		Pos Delta(VerticalBindings parent, VerticalBindings child);
	};

	class HBindHelper {
	public:
		Pos ParentWidth;
		Pos ChildLeft;
		Pos ChildWidth;

		HBindHelper(Pos parentWidth, Pos childLeft, Pos childWidth) : ParentWidth(parentWidth), ChildLeft(childLeft), ChildWidth(childWidth) {}

		Pos Parent(HorizontalBindings bind);
		Pos Child(HorizontalBindings bind);
		Pos Delta(HorizontalBindings parent, HorizontalBindings child);
	};
};
}
