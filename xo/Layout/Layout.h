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
	// This is unfortunately a large data structure. I haven't found a simple way of making it smaller.
	// The key thing driving the size up here, is that each binding point can be either a Size type,
	// or a binding point. We could represent this in a more compact manner by have 7x 32-bit fields for
	// the enum or size value, and 7x 4-bit fields, for the nature of the size or binding type, but
	// that only brings us down to 32 bytes, which I'm not convinced is worth the extra code.
	struct BindingSet {
		StyleAttrib HChildLeft;
		StyleAttrib HChildCenter;
		StyleAttrib HChildRight;

		StyleAttrib VChildTop;
		StyleAttrib VChildCenter;
		StyleAttrib VChildBottom;
		StyleAttrib VChildBaseline;
	};

	struct LayoutInput {
		cheapvec<int32_t>* RestartPoints; // This is IN/OUT
		RenderDomNode*     ParentRNode;
		Pos                ParentWidth;
		Pos                ParentHeight;
	};

	struct LayoutOutput {
		BindingSet   Binds;
		Box          MarginBox;
		Pos          Baseline; // This is in the coordinate system of the child. In other words, this is a distance from RNodeTop.
		Pos          RNodeTop; // Usually equal to RNode->Pos.Top, but empty text objects need this too, and they have RNode = null. Rather store it here than an extra RenderDomEl.
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
		cheapvec<int32_t>*    RestartPoints;
		float                 FontWidthScale;
		int                   FontSizePx;
		bool                  GlyphsNeeded;
		bool                  IsSubPixel;
		Pos                   FontAscender;
		xo::FontID            FontID;
		xo::Color             Color;
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

	const xo::Doc*               Doc;
	BoxLayout                    Boxer;
	xo::Pool*                    Pool;
	RenderStack                  Stack;
	FixedSizeHeap                FHeap;
	float                        PtToPixel;
	float                        EpToPixel;
	FontTableImmutable           Fonts;
	ohash::set<GlyphCacheKey>    GlyphsNeeded;
	ohash::set<FontIDWeightPair> FontsNeeded; // Will only be here because of a weight that is not 400 (regular)
	TextRunState                 TempText;
	bool                         SnapBoxes;
	bool                         SnapHorzText;
	bool                         EnableKerning;

	void  RenderFontsNeeded();
	void  RenderGlyphsNeeded();
	void  LayoutInternal(RenderDomNode& root);
	void  RunNode(const DomNode* node, const LayoutInput& in, LayoutOutput& out);
	void  RunText(const DomText* node, const LayoutInput& in, LayoutOutput& out);
	Point PositionChildFromBindings(const LayoutInput& cin, Pos parentBaseline, LayoutOutput& cout);
	void  GenerateTextWords(TextRunState& ts);
	void  FinishTextRNode(TextRunState& ts, RenderDomText* rnode, size_t numChars);
	void  OffsetTextHorz(TextRunState& ts, Pos offsetHorz, size_t numChars);
	Pos   MeasureWord(const char* txt, const Font* font, Pos fontAscender, Chunk chunk, TextRunState& ts);

	Pos  ComputeWidthOrHeightDimension(Pos containerSize, Pos containerRemaining, StyleCategories cat);
	Pos  ComputeWidthOrHeightDimension(Pos containerSize, Pos containerRemaining, Size size);
	Pos  ComputeDimension(Pos container, StyleCategories cat);
	Pos  ComputeDimension(Pos container, Size size);
	Box  ComputeBox(Pos containerWidth, Pos containerHeight, StyleCategories cat);
	Box  ComputeBox(Pos containerWidth, Pos containerHeight, StyleBox box);
	void PopulateBindings(BindingSet& bindings);

	Pos HoriAdvance(const Glyph* glyph, const TextRunState& ts);

	static Pos           HBindOffset(HorizontalBindings bind, Pos left, Pos width);
	static Pos           VBindOffset(VerticalBindings bind, Pos top, Pos baseline, Pos height);
	static bool          IsSpace(int ch);
	static bool          IsLinebreak(int ch);
	static GlyphCacheKey MakeGlyphCacheKey(RenderDomText* rnode);
	static GlyphCacheKey MakeGlyphCacheKey(const TextRunState& ts);
	static GlyphCacheKey MakeGlyphCacheKey(bool isSubPixel, FontID fontID, int fontSizePx);
	static bool          IsAllZeros(const cheapvec<int32_t>& list);
	static void          MoveChildren(RenderDomEl* relem, Point delta);

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
	// Why can child can never be a Size?
	// That would be like writing "123:top", which is equivalent to "top:-123", so we
	// only allow the top:-123 formulation, in part because the 123:top is hard to read, but it also opens
	// a can of worms for our style parser.
	class VBindHelper {
	public:
		xo::Layout* Layout;
		Pos         ParentHeight;
		Pos         ParentBaseline;
		Pos         ChildTop;
		Pos         ChildHeight;
		Pos         ChildBaseline;

		VBindHelper(xo::Layout* layout, Pos parentHeight, Pos parentBaseline, Pos childTop, Pos childHeight, Pos childBaseline) : Layout(layout), ParentHeight(parentHeight), ParentBaseline(parentBaseline), ChildTop(childTop), ChildHeight(childHeight), ChildBaseline(childBaseline) {}

		Pos Parent(StyleAttrib bind);
		Pos Child(VerticalBindings bind);
		Pos Delta(StyleAttrib parent, VerticalBindings child);
	};

	class HBindHelper {
	public:
		xo::Layout* Layout;
		Pos         ParentWidth;
		Pos         ChildLeft;
		Pos         ChildWidth;

		HBindHelper(xo::Layout* layout, Pos parentWidth, Pos childLeft, Pos childWidth) : Layout(layout), ParentWidth(parentWidth), ChildLeft(childLeft), ChildWidth(childWidth) {}

		Pos Parent(StyleAttrib bind);
		Pos Child(HorizontalBindings bind);
		Pos Delta(StyleAttrib parent, HorizontalBindings child);
	};
};
} // namespace xo
