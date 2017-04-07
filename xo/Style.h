#pragma once
#include "Defs.h"
#include "Base/xoString.h"

namespace xo {

// The list of styles that are inherited by child nodes lives in InheritedStyleCategories

// Maximum length of a style variable name
static const size_t MaxStyleVarNameLen = 127;

// Maximum length of an SVG name
static const size_t MaxSvgNameLen = 127;

// This is parsing whitespace, not DOM/textual whitespace
// In other words, it is the space between the comma and verdana in "font-family: verdana, arial",
inline bool IsWhitespace(char c) {
	return c == 9 || c == 32 || c == 10 || c == 13;
}

// Represents a size that is zero, pixels, eye pixels, points, percent, em, ex.
// Zero is represented as 0 pixels
// See "layout" documentation for explanation of units.
struct XO_API Size {
	enum Types { NONE = 0,
		         PX,        // native device pixels
		         PT,        // points (1/72) of an inch
		         EP,        // eye pixels (ie adjusted for resolution of screen)
		         EM,        // colloquially, "height of the 'm' character", but more specifically, the 'ascender' metric of the font face
		         EX,        // height of 'x' character
		         EH,        // Line height. 'eh' looks better as a numeric suffix than 'lh', because the 'l' looks like a 1.
		         PERCENT,   // percentage of parent's content width or height
		         REMAINING, // Fill percentage of remaining space
	};
	// Note that value 255 is used by StyleAttrib::SubType_SizeBinding, so do not
	// use 255 inside this Types enum.

	float Val;
	Types Type;

	static Size Make(Types t, float v) {
		Size s = {v, t};
		return s;
	}
	static Size Em(float v) {
		Size s = {v, EM};
		return s;
	}
	static Size Ex(float v) {
		Size s = {v, EX};
		return s;
	}
	static Size Eh(float v) {
		Size s = {v, EH};
		return s;
	}
	static Size Percent(float v) {
		Size s = {v, PERCENT};
		return s;
	}
	static Size Remaining(float v) {
		Size s = {v, REMAINING};
		return s;
	}
	static Size Points(float v) {
		Size s = {v, PT};
		return s;
	}
	static Size Pixels(float v) {
		Size s = {v, PX};
		return s;
	}
	static Size EyePixels(float v) {
		Size s = {v, EP};
		return s;
	}
	static Size Zero() {
		Size s = {0, PX};
		return s;
	}
	static Size Null() {
		Size s = {0, NONE};
		return s;
	}

	static bool Parse(const char* s, size_t len, Size& v);

	bool operator==(const Size& b) const { return Type == b.Type && Val == b.Val; }
	bool operator!=(const Size& b) const { return Type != b.Type || Val != b.Val; }
};

// Convenience struct used during layout computation
struct XO_API StyleBox {
	// This order (left,top,right,bottom) must be consistent with the order presented inside the StyleCategories enum
	union {
		struct
		{
			Size Left, Top, Right, Bottom;
		};
		Size All[4];
	};

	static bool     Parse(const char* s, size_t len, StyleBox& v);
	static StyleBox Make(Size left, Size top, Size right, Size bottom) {
		StyleBox b;
		b.Left   = left;
		b.Top    = top;
		b.Right  = right;
		b.Bottom = bottom;
		return b;
	}
	static StyleBox MakeUniform(Size all) {
		StyleBox b;
		b.Left   = all;
		b.Top    = all;
		b.Right  = all;
		b.Bottom = all;
		return b;
	}
	static StyleBox MakeZero() {
		StyleBox b;
		b.SetZero();
		return b;
	}
	void SetZero() { Left = Top = Right = Bottom = Size::Pixels(0); }
};

// Convenience struct used during layout computation
struct XO_API CornerStyleBox {
	// This order (topleft, topright, bottomright, bottomleft) must be consistent with the order presented inside the StyleCategories enum
	union {
		struct
		{
			Size TopLeft, TopRight, BottomRight, BottomLeft;
		};
		Size All[4];
	};

	static bool           Parse(const char* s, size_t len, CornerStyleBox& v);
	static CornerStyleBox Make(Size topleft, Size topright, Size bottomright, Size bottomleft) {
		CornerStyleBox b;
		b.TopLeft     = topleft;
		b.TopRight    = topright;
		b.BottomRight = bottomright;
		b.BottomLeft  = bottomleft;
		return b;
	}
	static CornerStyleBox MakeUniform(Size all) {
		CornerStyleBox b;
		b.All[0] = all;
		b.All[1] = all;
		b.All[2] = all;
		b.All[3] = all;
		return b;
	}
	static StyleBox MakeZero() {
		StyleBox b;
		b.SetZero();
		return b;
	}
	void SetZero() { All[0] = All[1] = All[2] = All[3] = Size::Pixels(0); }
};

enum FlowDirection {
	FlowDirectionNormal,
	FlowDirectionReversed // Not implemented
};

enum FlowAxis {
	FlowAxisVertical,  // Vertical is the default
	FlowAxisHorizontal // Not implemented
};

enum VerticalBindings {
	VerticalBindingNULL,
	VerticalBindingTop,      // Top of parent's content box
	VerticalBindingCenter,   // Center of parent's content box
	VerticalBindingBottom,   // Bottom of parent's content box
	VerticalBindingBaseline, // Parent's baseline
};

enum HorizontalBindings {
	HorizontalBindingNULL,
	HorizontalBindingLeft,   // Left of parent's content box
	HorizontalBindingCenter, // Center of parent's content box
	HorizontalBindingRight,  // Right of parent's content box
};

enum PositionType {
	// All of these definitions are the same as HTML's
	PositionStatic,   // Default, regular position
	PositionAbsolute, // Absolute, relative to previous explicitly parent that was anything other than "Static". Does not affect flow of subsequent siblings.
	PositionRelative, // Like Absolute, but does affect flow. Flow is the "ghost" of where we would have been, before being moved relatively.
	PositionFixed,    // Fixed, according to root device coordinate system. In other words, completely independent of DOM hierarchy.
};

enum BoxSizeType {
	BoxSizeContent,
	BoxSizeBorder,
	BoxSizeMargin // Created initially for the <body> element
};

enum BreakType {
	BreakNULL,
	BreakBefore, // Break flow before element
	BreakAfter   // Break flow after element
};

enum FlowContext {
	FlowContextNew,    // This object defines a new flow context, and it's children flow inside that.
	FlowContextInject, // This object does not flow; it's children are promoted into it's parent's flow context.
};

// Bump was a concept created for <span> objects, such as "I told you <span class='highlight'>one hundred times</span> not to do that",
// where the class inside the <span> has non-zero borders, margin, or padding.
enum BumpStyle {
	BumpRegular,  // Normal. Padding + Border + Margin affect surrounding objects
	BumpHorzOnly, // Special for spans, and other things in injected flow. Their padding + border + margin don't affect vertical flow.
	BumpVertOnly, // No intended use - just here for completeness
	BumpNone,     // Neither horizontal nor vertical bumps have effect
};

// The following attributes are "bind sources". You bind a position of your own to a position on your parent node.
// The "bind targets" that you can bind to on your parent node are the same properties that can be used as bind sources.
// So if you bind left:right, then you are binding your left edge to your parent content box's right edge.
// If you bind vcenter:vcenter, then you bind your vertical center to the vertical center of your parent context box.
// * Left
// * HCenter
// * Right
// * Top
// * VCenter
// * Bottom
// * Baseline

// The order of the box components (left,top,right,bottom) must be consistent with the order in StyleBox
// In addition, all 'box' types must fall between Margin_Left and Border_Bottom. This is simply for sanity. It is verified
// inside Style.SetBox()
//
// !!! This must be kept in sync with CatNameTable !!!
enum StyleCategories {
	CatNULL = 0,
	CatColor,
	CatFIRST = CatColor,
	CatPadding_Use_Me_1,
	CatBackground,
	CatPadding_Use_Me_2,
	CatText_Align_Vertical,

	CatBreak,
	CatCanFocus,
	CatCursor,

	CatOverflowX, // TODO
	CatOverflowY, // TODO
	CatPadding_Use_Me_3,

	CatMargin_Left,
	CatMargin_Top,
	CatMargin_Right,
	CatMargin_Bottom,

	CatPadding_Left,
	CatPadding_Top,
	CatPadding_Right,
	CatPadding_Bottom,

	CatBorder_Left,
	CatBorder_Top,
	CatBorder_Right,
	CatBorder_Bottom,

	CatBorderColor_Left,
	CatBorderColor_Top,
	CatBorderColor_Right,
	CatBorderColor_Bottom,

	CatBorderRadius_TL,
	CatBorderRadius_TR,
	CatBorderRadius_BR,
	CatBorderRadius_BL,

	CatWidth,
	CatHeight,

	CatTop,
	CatVCenter,
	CatBottom,
	CatBaseline,
	CatVertBinding_FIRST = CatTop,
	CatVertBinding_LAST  = CatBaseline,

	CatLeft,
	CatHCenter,
	CatRight,
	CatHorzBinding_FIRST = CatLeft,
	CatHorzBinding_LAST  = CatRight,

	CatFontSize,
	CatFontFamily,

	CatPosition,
	CatFlowContext,
	CatFlowAxis,
	CatFlowDirection_Horizontal,
	CatFlowDirection_Vertical,
	CatBoxSizing,
	CatBump,

	// Generic categories used for style properties that reference variables. Use CatUngenerify to make a concret _Left category.
	CatGenMargin,
	CatGenPadding,
	CatGenBorder,
	CatGenBorderColor,
	CatGenBorderRadius,

	CatEND,
};

static_assert(CatMargin_Left % 4 == 0, "Start of boxes must be multiple of 4");

extern const char* CatNameTable[CatEND];

inline StyleCategories CatMakeBaseBox(StyleCategories c) { return (StyleCategories)(c & ~3); }

// Translate from CatGenMargin to CatMargin_Left, etc.
StyleCategories CatUngenerify(StyleCategories c);

// Styles that are inherited by default
// Generally it is text styles that are inherited
// Inheritance means that child nodes inherit the styles of their parents
const int                    NumInheritedStyleCategories = 5;
extern const StyleCategories InheritedStyleCategories[NumInheritedStyleCategories];

/* Single style attribute (such as Margin-Left, Width, FontSize, etc).
This must be zero-initializable (i.e. with memset(0)).
It must remain small.
Currently, sizeof(StyleAttrib) = 8. This is verified by a static assertion below.

The binding points left,right,hcenter,top,bottom,vcenter, are quite special, because they
can take on either an alignment point of their parent, such as left:left, or left:hcenter,
OR they can take on an exact value, in which case that value is a number inside their
parent's content box. For example, left:5px. It is highly desirable to maintain this
attribute as a single StyleAttrib, instead of trying to split it out into some other hackish
pair, so we take that pain inside StyleAttrib. We use a special SubType, which is outside
the normal range of the Size subtypes (PX,EM,etc), to represent the fact that the size is
a binding point instead of a size.
*/
class XO_API StyleAttrib {
public:
	enum Flag {
		// This means that the attribute takes its value from its closest ancestor in the DOM tree.
		// Some styles are inherited by default (the list specified inside InheritedStyleCategories).
		FlagInherit = 1,
		// The value inside ValU32 is an integer ID that was assigned by Doc::GetOrCreateStyleVerbatimID().
		// This integer can be used with Doc::GetStyleVerbatim() to get the original string of that style
		// attribute, such as "1px $dark-border".
		FlagVerbatim = 2,
	};

	enum SubTypes {
		// This lives in the same space as Size::Types, so it must not conflict with any of them.
		// This is used to indicate that a horizontal or vertical binding attribute is actually
		// a size value, which means "distance from top or left of parent content-box", depending
		// on whether it's a vertical or horizontal binding.
		// If our category type is any of the Binding values, and the SubType is anything other than
		// SubType_EnumBinding, then the attribute is a Size object.
		SubType_EnumBinding = 255,

		// background is actually a vector icon
		SubType_Background_Vector = 1,
	};

	StyleCategories Category : 8;
	uint32_t        SubType : 8;
	uint32_t        Flags : 8;
	uint32_t        Unused2 : 8;
	union {
		uint32_t ValU32;
		float    ValF;
	};

	StyleAttrib();

	static StyleAttrib MakeWidth(Size val) {
		StyleAttrib a;
		a.SetSize(CatWidth, val);
		return a;
	}
	static StyleAttrib MakeHeight(Size val) {
		StyleAttrib a;
		a.SetSize(CatHeight, val);
		return a;
	}

	void SetInherit(StyleCategories cat);
	void SetVerbatim(StyleCategories cat, int verbatimID);

	void SetColor(StyleCategories cat, Color val) { SetU32(cat, val.u); }
	void SetSize(StyleCategories cat, Size val) { SetWithSubtypeF(cat, val.Type, val.Val); }
	//void SetBorderRadius(Size val) { SetSize(CatBorderRadius, val); }
	void SetPosition(PositionType val) { SetU32(CatPosition, val); }
	void SetFont(FontID val) { SetU32(CatFontFamily, val); }
	void SetBackgroundVector(int vectorID) { SetWithSubtypeU32(CatBackground, SubType_Background_Vector, vectorID); }
	void SetBreak(BreakType type) { SetU32(CatBreak, type); }
	void SetCanFocus(bool canFocus) { SetU32(CatCanFocus, canFocus); }
	void SetCursor(Cursors cursor) { SetU32(CatCursor, cursor); }
	void SetFlowContext(FlowContext flowcx) { SetU32(CatFlowContext, flowcx); }
	void SetFlowAxis(FlowAxis axis) { SetU32(CatFlowAxis, axis); }
	void SetFlowDirectionHorizonal(FlowDirection dir) { SetU32(CatFlowDirection_Horizontal, dir); }
	void SetFlowDirectionVertical(FlowDirection dir) { SetU32(CatFlowDirection_Vertical, dir); }
	void SetBoxSizing(BoxSizeType type) { SetU32(CatBoxSizing, type); }

	void SetLeft(HorizontalBindings bind) { SetWithSubtypeU32(CatLeft, SubType_EnumBinding, bind); }
	void SetHCenter(HorizontalBindings bind) { SetWithSubtypeU32(CatHCenter, SubType_EnumBinding, bind); }
	void SetRight(HorizontalBindings bind) { SetWithSubtypeU32(CatRight, SubType_EnumBinding, bind); }
	void SetTop(VerticalBindings bind) { SetWithSubtypeU32(CatTop, SubType_EnumBinding, bind); }
	void SetVCenter(VerticalBindings bind) { SetWithSubtypeU32(CatVCenter, SubType_EnumBinding, bind); }
	void SetBottom(VerticalBindings bind) { SetWithSubtypeU32(CatBottom, SubType_EnumBinding, bind); }
	void SetBaseline(VerticalBindings bind) { SetWithSubtypeU32(CatBaseline, SubType_EnumBinding, bind); }

	void SetLeft(Size val) { SetWithSubtypeF(CatLeft, val.Type, val.Val); }
	void SetHCenter(Size val) { SetWithSubtypeF(CatHCenter, val.Type, val.Val); }
	void SetRight(Size val) { SetWithSubtypeF(CatRight, val.Type, val.Val); }
	void SetTop(Size val) { SetWithSubtypeF(CatTop, val.Type, val.Val); }
	void SetVCenter(Size val) { SetWithSubtypeF(CatVCenter, val.Type, val.Val); }
	void SetBottom(Size val) { SetWithSubtypeF(CatBottom, val.Type, val.Val); }
	void SetBaseline(Size val) { SetWithSubtypeF(CatBaseline, val.Type, val.Val); }

	void SetBump(BumpStyle bump) { SetU32(CatBump, bump); }

	// Generic Set() that is used by template code
	void Set(StyleCategories cat, Color val) { SetColor(cat, val); }
	void Set(StyleCategories cat, Size val) { SetSize(cat, val); }
	void Set(StyleCategories cat, PositionType val) { SetPosition(val); }
	void Set(StyleCategories cat, BreakType val) { SetBreak(val); }
	void Set(StyleCategories cat, Cursors val) { SetCursor(val); }
	void Set(StyleCategories cat, FlowContext val) { SetFlowContext(val); }
	void Set(StyleCategories cat, FlowAxis val) { SetFlowAxis(val); }
	void Set(StyleCategories cat, FlowDirection val) { SetU32(cat, val); }
	void Set(StyleCategories cat, BoxSizeType val) { SetBoxSizing(val); }
	void Set(StyleCategories cat, HorizontalBindings val) { SetWithSubtypeU32(cat, SubType_EnumBinding, val); }
	void Set(StyleCategories cat, VerticalBindings val) { SetWithSubtypeU32(cat, SubType_EnumBinding, val); }
	void Set(StyleCategories cat, FontID val) { SetFont(val); }
	void Set(StyleCategories cat, BumpStyle val) { SetU32(cat, val); }
	void Set(StyleCategories cat, const char* val, Doc* doc) { SetString(cat, val, doc); }

	void SetBool(StyleCategories cat, bool val) { SetU32(cat, val); }

	bool IsNull() const { return Category == CatNULL; }
	bool IsInherit() const { return !!(Flags & FlagInherit); }
	bool IsVerbatim() const { return !!(Flags & FlagVerbatim); }
	bool IsHorzBinding() const { return Category >= CatHorzBinding_FIRST && Category <= CatHorzBinding_LAST; }
	bool IsVertBinding() const { return Category >= CatVertBinding_FIRST && Category <= CatVertBinding_LAST; }
	bool IsBindingTypeEnum() const { return SubType == SubType_EnumBinding; }

	StyleCategories    GetCategory() const { return (StyleCategories) Category; }
	Size               GetSize() const { return Size::Make((Size::Types) SubType, ValF); }
	Color              GetColor() const { return Color::Make(ValU32); }
	PositionType       GetPositionType() const { return (PositionType) ValU32; }
	BreakType          GetBreakType() const { return (BreakType) ValU32; }
	bool               GetCanFocus() const { return ValU32 != 0; }
	Cursors            GetCursor() const { return (Cursors) ValU32; }
	int                GetStringID() const { return (int) ValU32; }
	FlowContext        GetFlowContext() const { return (FlowContext) ValU32; }
	FlowAxis           GetFlowAxis() const { return (FlowAxis) ValU32; }
	FlowDirection      GetFlowDirectionMajor() const { return (FlowDirection) ValU32; }
	FlowDirection      GetFlowDirectionMinor() const { return (FlowDirection) ValU32; }
	BoxSizeType        GetBoxSizing() const { return (BoxSizeType) ValU32; }
	HorizontalBindings GetHorizontalBinding() const { return (HorizontalBindings) ValU32; }
	VerticalBindings   GetVerticalBinding() const { return (VerticalBindings) ValU32; }
	BumpStyle          GetBump() const { return (BumpStyle) ValU32; }
	int                GetVerbatimID() const { return ValU32; }

	const char* GetBackgroundImage(StringTable* strings) const;
	FontID      GetFont() const;

protected:
	void SetString(StyleCategories cat, const char* str, Doc* doc);
	void SetU32(StyleCategories cat, uint32_t val);
	void SetWithSubtypeU32(StyleCategories cat, uint8_t subtype, uint32_t val);
	void SetWithSubtypeF(StyleCategories cat, uint8_t subtype, float val);
};

static_assert(sizeof(StyleAttrib) == 8, "Why has StyleAttrib grown?");

/* Collection of style attributes (border-width-left, color, etc)
This container is simple and list-based.
There is a different container called StyleSet that is more performant,
built for use during rendering.
*/
class XO_API Style {
public:
	cheapvec<StyleAttrib> Attribs;

	// Parse a number of styles, enclosed in {} braces
	static bool        ParseSheet(const char* t, Doc* doc);
	bool               Parse(const char* t, Doc* doc);
	bool               Parse(const char* t, size_t maxLen, Doc* doc);
	const StyleAttrib* Get(StyleCategories cat) const;
	void               SetBox(StyleCategories cat, StyleBox val);
	void               GetBox(StyleCategories cat, StyleBox& val) const;
	void               SetCornerBox(StyleCategories cat, CornerStyleBox val);
	void               GetCornerBox(StyleCategories cat, CornerStyleBox& val) const;
	void               SetUniformBox(StyleCategories cat, StyleAttrib val);
	void               SetUniformBox(StyleCategories cat, Color color);
	void               SetUniformBox(StyleCategories cat, Size size);
	void               SetVerbatim(StyleCategories cat, int verbatimID);
	void               Set(StyleAttrib attrib);
	void               Set(StyleCategories cat, StyleBox val);       // This overload is identical to SetBox, but needs to be present for templated parsing functions
	void               Set(StyleCategories cat, CornerStyleBox val); // Same as above

	template <typename T>
	void Set(StyleCategories cat, const T& v) {
		StyleAttrib a;
		a.Set(cat, v);
		Set(a);
	}

	void Discard();
	void CloneSlowInto(Style& c) const;
	void CloneFastInto(Style& c, Pool* pool) const;

	bool IsEmpty() const { return Attribs.size() == 0; }

// Setter functions with 2 parameters
#define NUSTYLE_SETTERS_2P                              \
	XX(BackgroundColor, Color, SetColor, CatBackground) \
	XX(Width, Size, SetSize, CatWidth)                  \
	XX(Height, Size, SetSize, CatHeight)                \
	XX(Left, Size, SetSize, CatLeft)                    \
	XX(Right, Size, SetSize, CatRight)                  \
	XX(Top, Size, SetSize, CatTop)                      \
	XX(Bottom, Size, SetSize, CatBottom)

// Setter functions with 1 parameter
#define NUSTYLE_SETTERS_1P \
	XX(Position, PositionType, SetPosition)

#define XX(name, type, setfunc, cat) void Set##name(type value);
	NUSTYLE_SETTERS_2P
#undef XX

#define XX(name, type, setfunc) void Set##name(type value);
	NUSTYLE_SETTERS_1P
#undef XX

protected:
	//void					MergeInZeroCopy( int n, const Style** src );
	void GetBoxInternal(StyleCategories catBase, Size* quad) const;
	void SetBoxInternal(StyleCategories catBase, Size* quad);
};

/* A bag of styles in a performant container.

Analysis of storage (assuming CatEND = 128)

	Number of attributes	Size of Lookup			Size of Attribs		Total marginal size
	1 << 2 = 4				128 * 2 / 8 = 32		4 * 8   = 32		32 + 32    = 64
	1 << 4 = 16				128 * 4 / 8 = 64		16 * 8  = 128		64 + 128   = 192
	1 << 8 = 256			128 * 8 / 8 = 128		256 * 8 = 2048		128 + 2048 = 2176

One very significant optimization that remains here is to not grow Attrib[] so heavily.
Growing from 16 to 256 is an insane leap.

Idea: There are some attributes that need only a few bits. Instead of packing each of these
into 8 bytes, we can instead store groups of attributes in special 8 uint8_t blocks.

More Idea: I think I might end up writing a very specialized container for this stuff.. where we
separate things out into really tight bags of properties. But we'll wait until it's a bottleneck.

*/
class XO_API StyleSet {
public:
	static const uint32_t InitialBitsPerSlot = 2; // 1 << 2 = 4, is our lowest non-empty size
	static const uint32_t SlotOffset         = 1; // We need to reserve zero for the "empty" state of a slot

	StyleSet();  // Simply calls Reset()
	~StyleSet(); // Destructor does nothing

	void        Set(int n, const StyleAttrib* attribs, Pool* pool);
	void        Set(const StyleAttrib& attrib, Pool* pool);
	StyleAttrib Get(StyleCategories cat) const;
	void        EraseOrSetNull(StyleCategories cat) const; // If an item was already set, then Contains() will return true, but Get() will return a null StyleAttrib
	bool        Contains(StyleCategories cat) const;
	void        Reset();

protected:
	typedef void (*SetSlotFunc)(void* lookup, StyleCategories cat, int32_t slot);
	typedef int32_t (*GetSlotFunc)(const void* lookup, StyleCategories cat);

	void*        Lookup;      // Variable bit-width table that indexes into Attribs. Size is always CatEND.
	StyleAttrib* Attribs;     // The Category field in here is wasted.
	int32_t      Count;       // Size of Attribs
	int32_t      Capacity;    // Capacity of Attribs
	uint32_t     BitsPerSlot; // Number of bits in each slot of Lookup. Our possible sizes are 2,4,8.
	//SetSlotFunc		SetSlotF;
	//GetSlotFunc		GetSlotF;

	void    Grow(Pool* pool);
	int32_t GetSlot(StyleCategories cat) const;
	void    SetSlot(StyleCategories cat, int32_t slot);
	void    DebugCheckSanity() const;

	static void MigrateLookup(const void* lutsrc, void* lutdst, GetSlotFunc getter, SetSlotFunc setter);

	template <uint32_t BITS_PER_SLOT>
	static void TSetSlot(void* lookup, StyleCategories cat, int32_t slot);

	template <uint32_t BITS_PER_SLOT>
	static int32_t TGetSlot(const void* lookup, StyleCategories cat);

	static int32_t GetSlot2(const void* lookup, StyleCategories cat);
	static int32_t GetSlot4(const void* lookup, StyleCategories cat);
	static int32_t GetSlot8(const void* lookup, StyleCategories cat);
	static void    SetSlot2(void* lookup, StyleCategories cat, int32_t slot);
	static void    SetSlot4(void* lookup, StyleCategories cat, int32_t slot);
	static void    SetSlot8(void* lookup, StyleCategories cat, int32_t slot);

	// The -1 here is for SlotOffset
	static int32_t CapacityAt(uint32_t bitsPerSlot) { return (1 << bitsPerSlot) - 1; }
};

// This is a style class, such as "xo.controls.button"
class XO_API StyleClass {
public:
	Style        Default; // Default styles
	Style        Hover;   // Styles present when cursor is over node
	Style        Focus;   // Styles present when object has focus
	Style        Capture; // Styles present when object has input capture
	Style*       All4PseudoTypes() { return &Default; }
	const Style* All4PseudoTypes() const { return &Default; }
};

// The set of style information that is used by the renderer
// This is baked in by the Layout engine.
// This struct is present in every single RenderDomNode, so it pays to keep it tight.
class XO_API StyleRender {
public:
	Box16    BorderSize;
	Box16    Padding;
	Box16    BorderRadius; // 2 bits of sub-pixel precision, giving maximum radius of 16384
	Color    BackgroundColor;
	Color    BorderColor[4];
	uint16_t BackgroundImageID;   // I hope 64k is enough for this
	bool     HasHoverStyle : 1;   // Element's appearance depends upon whether the cursor is over it
	bool     HasFocusStyle : 1;   // Element's appearance depends upon whether it has the focus
	bool     HasCaptureStyle : 1; // Element's appearance depends upon whether it has the input captured

	StyleRender() { memset(this, 0, sizeof(*this)); }
};

/* Store all style classes in one table, that is owned by one document.
This allows us to reference styles by a 32-bit integer ID instead of by name.
*/
class XO_API StyleTable {
public:
	StyleTable();
	~StyleTable();

	void              AddDummyStyleZero();
	void              Discard();
	StyleClass*       GetOrCreate(const char* name);
	const StyleClass* GetByID(StyleClassID id) const;
	StyleClassID      GetClassID(const char* name);
	void              CloneSlowInto(StyleTable& c) const;             // Does not clone NameToIndex
	void              CloneFastInto(StyleTable& c, Pool* pool) const; // Does not clone NameToIndex
	void              ExpandVerbatimVariables(Doc* doc);              // Expand style $variables
	void              DebugDump() const;

protected:
	cheapvec<String>        Names; // Names and Classes are parallel
	cheapvec<StyleClass>    Classes;
	ohash::map<String, int> NameToIndex;
};

XO_API bool ParsePositionType(const char* s, size_t len, PositionType& t);
XO_API bool ParseBreakType(const char* s, size_t len, BreakType& t);
XO_API bool ParseCursor(const char* s, size_t len, Cursors& t);
XO_API bool ParseFlowContext(const char* s, size_t len, FlowContext& t);
XO_API bool ParseFlowAxis(const char* s, size_t len, FlowAxis& t);
XO_API bool ParseFlowDirection(const char* s, size_t len, FlowDirection& t);
XO_API bool ParseBoxSize(const char* s, size_t len, BoxSizeType& t);
XO_API bool ParseHorizontalBinding(const char* s, size_t len, HorizontalBindings& t);
XO_API bool ParseVerticalBinding(const char* s, size_t len, VerticalBindings& t);
XO_API bool ParseBinding(bool isHorz, const char* s, size_t len, StyleCategories cat, Doc* doc, Style& style);
XO_API bool ParseBump(const char* s, size_t len, BumpStyle& t);
XO_API bool ParseBorder(const char* s, size_t len, const char* subCategory, size_t subCategoryLen, Doc* doc, Style& style);
XO_API bool ParseBackground(const char* s, size_t len, const char* subCategory, size_t subCategoryLen, Doc* doc, Style& style);
}
