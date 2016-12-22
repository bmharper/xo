#pragma once
#include "Defs.h"
#include "Base/xoString.h"

namespace xo {

// The list of styles that are inherited by child nodes lives in InheritedStyleCategories

// Represents a size that is zero, pixels, eye pixels, points, percent.
// TODO: em
// Zero is represented as 0 pixels
// See "layout" documentation for explanation of units.
struct XO_API Size {
	enum Types { NONE = 0,
		         PX,
		         PT,
		         EP,
		         PERCENT };
	float Val;
	Types Type;

	static Size Make(Types t, float v) {
		Size s = {v, t};
		return s;
	}
	static Size Percent(float v) {
		Size s = {v, PERCENT};
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
};

// Convenience struct used during layout computation
struct XO_API StyleBox {
	// This order (left,top,right,bottom) must be consistent with the order presented inside XO_STYLE_DEFINE
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

enum DisplayType {
	DisplayBlock,
	DisplayInline
};

enum FlowDirection {
	FlowDirectionNormal,
	FlowDirectionReversed
};

enum FlowAxis {
	FlowAxisVertical, // Vertical is the default
	FlowAxisHorizontal
};

enum TextAlignVertical {
	TextAlignVerticalBaseline, // Baseline is default
	TextAlignVerticalTop       // This is unlikely to be useful, but having it proves that we *could* make other rules if proved useful
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
enum StyleCategories {
	CatNULL = 0,
	CatColor,
	CatFIRST = CatColor,
	CatDisplay,
	CatBackground,
	CatBackgroundImage,
	CatText_Align_Vertical,

	CatBreak,
	CatCanFocus,
	CatCursor,

	CatPadding_Use_Me_1,
	CatPadding_Use_Me_2,
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

	CatWidth,
	CatHeight,

	CatTop,
	CatVCenter,
	CatBottom,
	CatBaseline,
	CatLeft,
	CatHCenter,
	CatRight,

	CatFontSize,
	CatFontFamily,

	CatBorderRadius,
	CatPosition,
	CatFlowContext,
	CatFlowAxis,
	CatFlowDirection_Horizontal,
	CatFlowDirection_Vertical,
	CatBoxSizing,
	CatBump,
	CatEND,
};

static_assert(CatMargin_Left % 4 == 0, "Start of boxes must be multiple of 4");

inline StyleCategories CatMakeBaseBox(StyleCategories c) { return (StyleCategories)(c & ~3); }

// Styles that are inherited by default
// Generally it is text styles that are inherited
// Inheritance means that child nodes inherit the styles of their parents
const int                    NumInheritedStyleCategories = 5;
extern const StyleCategories InheritedStyleCategories[NumInheritedStyleCategories];

/* Single style attribute (such as Margin-Left, Width, FontSize, etc).
This must be zero-initializable (i.e. with memset(0)).
It must remain small.
Currently, sizeof(StyleAttrib) = 8. This is verified by a static assertion below.
*/
class XO_API StyleAttrib {
public:
	enum Flag {
		// This means that the attribute takes its value from its closest ancestor in the DOM tree.
		// Some styles are inherited by default (the list specified inside InheritedStyleCategories).
		FlagInherit = 1,
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

	void SetColor(StyleCategories cat, Color val) { SetU32(cat, val.u); }
	void SetSize(StyleCategories cat, Size val) { SetWithSubtypeF(cat, val.Type, val.Val); }
	void SetDisplay(DisplayType val) { SetU32(CatDisplay, val); }
	void SetBorderRadius(Size val) { SetSize(CatBorderRadius, val); }
	void SetPosition(PositionType val) { SetU32(CatPosition, val); }
	void SetFont(FontID val) { SetU32(CatFontFamily, val); }
	void SetBackgroundImage(const char* image, Doc* doc) { SetString(CatBackgroundImage, image, doc); }
	void SetBreak(BreakType type) { SetU32(CatBreak, type); }
	void SetCanFocus(bool canFocus) { SetU32(CatCanFocus, canFocus); }
	void SetCursor(Cursors cursor) { SetU32(CatCursor, cursor); }
	void SetFlowContext(FlowContext flowcx) { SetU32(CatFlowContext, flowcx); }
	void SetFlowAxis(FlowAxis axis) { SetU32(CatFlowAxis, axis); }
	void SetFlowDirectionHorizonal(FlowDirection dir) { SetU32(CatFlowDirection_Horizontal, dir); }
	void SetFlowDirectionVertical(FlowDirection dir) { SetU32(CatFlowDirection_Vertical, dir); }
	void SetBoxSizing(BoxSizeType type) { SetU32(CatBoxSizing, type); }
	void SetTextAlignVertical(TextAlignVertical align) { SetU32(CatText_Align_Vertical, align); }
	void SetLeft(HorizontalBindings bind) { SetU32(CatLeft, bind); }
	void SetHCenter(HorizontalBindings bind) { SetU32(CatHCenter, bind); }
	void SetRight(HorizontalBindings bind) { SetU32(CatRight, bind); }
	void SetTop(VerticalBindings bind) { SetU32(CatTop, bind); }
	void SetVCenter(VerticalBindings bind) { SetU32(CatVCenter, bind); }
	void SetBottom(VerticalBindings bind) { SetU32(CatBottom, bind); }
	void SetBaseline(VerticalBindings bind) { SetU32(CatBaseline, bind); }
	void SetBump(BumpStyle bump) { SetU32(CatBump, bump); }

	// Generic Set() that is used by template code
	void Set(StyleCategories cat, Color val) { SetColor(cat, val); }
	void Set(StyleCategories cat, Size val) { SetSize(cat, val); }
	void Set(StyleCategories cat, DisplayType val) { SetDisplay(val); }
	void Set(StyleCategories cat, PositionType val) { SetPosition(val); }
	void Set(StyleCategories cat, BreakType val) { SetBreak(val); }
	void Set(StyleCategories cat, Cursors val) { SetCursor(val); }
	void Set(StyleCategories cat, FlowContext val) { SetFlowContext(val); }
	void Set(StyleCategories cat, FlowAxis val) { SetFlowAxis(val); }
	void Set(StyleCategories cat, FlowDirection val) { SetU32(cat, val); }
	void Set(StyleCategories cat, BoxSizeType val) { SetBoxSizing(val); }
	void Set(StyleCategories cat, TextAlignVertical val) { SetU32(cat, val); }
	void Set(StyleCategories cat, HorizontalBindings val) { SetU32(cat, val); }
	void Set(StyleCategories cat, VerticalBindings val) { SetU32(cat, val); }
	void Set(StyleCategories cat, FontID val) { SetFont(val); }
	void Set(StyleCategories cat, BumpStyle val) { SetU32(cat, val); }
	void Set(StyleCategories cat, const char* val, Doc* doc) { SetString(cat, val, doc); }

	void SetBool(StyleCategories cat, bool val) { SetU32(cat, val); }

	bool IsNull() const { return Category == CatNULL; }
	bool IsInherit() const { return Flags == FlagInherit; }

	StyleCategories    GetCategory() const { return (StyleCategories) Category; }
	Size               GetSize() const { return Size::Make((Size::Types) SubType, ValF); }
	Color              GetColor() const { return Color::Make(ValU32); }
	DisplayType        GetDisplayType() const { return (DisplayType) ValU32; }
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
	TextAlignVertical  GetTextAlignVertical() const { return (TextAlignVertical) ValU32; }
	HorizontalBindings GetHorizontalBinding() const { return (HorizontalBindings) ValU32; }
	VerticalBindings   GetVerticalBinding() const { return (VerticalBindings) ValU32; }
	BumpStyle          GetBump() const { return (BumpStyle) ValU32; }

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

	bool               Parse(const char* t, Doc* doc);
	bool               Parse(const char* t, size_t maxLen, Doc* doc);
	const StyleAttrib* Get(StyleCategories cat) const;
	void               SetBox(StyleCategories cat, StyleBox val);
	void               GetBox(StyleCategories cat, StyleBox& box) const;
	void               SetUniformBox(StyleCategories cat, StyleAttrib val);
	void               SetUniformBox(StyleCategories cat, Color color);
	void               SetUniformBox(StyleCategories cat, Size size);
	void               Set(StyleAttrib attrib);
	void               Set(StyleCategories cat, StyleBox val); // This overload is identical to SetBox, but needs to be present for templated parsing functions
	//void					Compute( const Doc& doc, const DomEl& node );
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
	void SetBoxInternal(StyleCategories catBase, StyleBox val);
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

// This is a style class, such as ".button"
class XO_API StyleClass {
public:
	Style Default; // Default styles
	Style Hover;   // Styles present when cursor is over node
	Style Focus;   // Styles present when object has focus
};

// The set of style information that is used by the renderer
// This is baked in by the Layout engine.
// This struct is present in every single RenderDomNode, so it pays to keep it tight.
class XO_API StyleRender {
public:
	Box16 BorderSize;
	Box16 Padding;
	Color BackgroundColor;
	Color BorderColor;
	int   BackgroundImageID;
	float BorderRadius;
	bool  HasHoverStyle : 1; // Element's appearance depends upon whether the cursor is over it.
	bool  HasFocusStyle : 1; // Element's appearance depends upon whether it has the focus

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

protected:
	cheapvec<String>        Names; // Names and Classes are parallel
	cheapvec<StyleClass>    Classes;
	cheapvec<int>           UnusedSlots;
	ohash::map<String, int> NameToIndex;
};

XO_API bool ParseDisplayType(const char* s, size_t len, DisplayType& t);
XO_API bool ParsePositionType(const char* s, size_t len, PositionType& t);
XO_API bool ParseBreakType(const char* s, size_t len, BreakType& t);
XO_API bool ParseCursor(const char* s, size_t len, Cursors& t);
XO_API bool ParseFlowContext(const char* s, size_t len, FlowContext& t);
XO_API bool ParseFlowAxis(const char* s, size_t len, FlowAxis& t);
XO_API bool ParseFlowDirection(const char* s, size_t len, FlowDirection& t);
XO_API bool ParseBoxSize(const char* s, size_t len, BoxSizeType& t);
XO_API bool ParseTextAlignVertical(const char* s, size_t len, TextAlignVertical& t);
XO_API bool ParseHorizontalBinding(const char* s, size_t len, HorizontalBindings& t);
XO_API bool ParseVerticalBinding(const char* s, size_t len, VerticalBindings& t);
XO_API bool ParseBump(const char* s, size_t len, BumpStyle& t);
XO_API bool ParseBorder(const char* s, size_t len, Style& style);
}
