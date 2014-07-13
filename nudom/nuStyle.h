#pragma once

#include "nuDefs.h"
#include "nuString.h"

// The list of styles that are inherited by child nodes lives in nuInheritedStyleCategories

// Represents a size that is zero, pixels, eye pixels, points, percent.
// TODO: em
// Zero is represented as 0 pixels
// See "layout" documentation for explanation of units.
struct NUAPI nuSize
{
	enum	Types { NONE = 0, PX, PT, EP, PERCENT };
	float 	Val;
	Types 	Type;

	static nuSize	Make( Types t, float v )	{ nuSize s = {v, t}; return s; }
	static nuSize	Percent( float v )			{ nuSize s = {v, PERCENT}; return s; }
	static nuSize	Points( float v )			{ nuSize s = {v, PT}; return s; }
	static nuSize	Pixels( float v )			{ nuSize s = {v, PX}; return s; }
	static nuSize	EyePixels( float v )		{ nuSize s = {v, EP}; return s; }
	static nuSize	Zero()						{ nuSize s = {0, PX}; return s; }
	static nuSize	Null()						{ nuSize s = {0, NONE}; return s; }

	static bool		Parse( const char* s, intp len, nuSize& v );
};

// Convenience struct used during layout computation
struct NUAPI nuStyleBox
{
	// This order (left,top,right,bottom) must be consistent with the order presented inside NU_STYLE_DEFINE
	union
	{
		struct 
		{
			nuSize	Left, Top, Right, Bottom;
		};
		nuSize All[4];
	};

	static bool			Parse( const char* s, intp len, nuStyleBox& v );
	static nuStyleBox	Make( nuSize left, nuSize top, nuSize right, nuSize bottom ) { nuStyleBox b; b.Left = left; b.Top = top; b.Right = right; b.Bottom = bottom; return b; }
	static nuStyleBox	MakeZero() { nuStyleBox b; b.SetZero(); return b; }
	void SetZero() { Left = Top = Right = Bottom = nuSize::Pixels(0); }
};

enum nuDisplayType
{
	nuDisplayBlock,
	nuDisplayInline
};

enum nuFlowDirection
{
	nuFlowDirectionNormal,
	nuFlowDirectionReversed
};

enum nuFlowAxis
{
	nuFlowAxisVertical,		// Vertical is the default
	nuFlowAxisHorizontal
};

enum nuTextAlignVertical
{
	nuTextAlignVerticalBaseline,	// Baseline is default
	nuTextAlignVerticalTop			// This is unlikely to be useful, but having it proves that we *could* make other rules if proved useful
};

enum nuVerticalBindings
{
	nuVerticalBindingNULL,
	nuVerticalBindingTop,			// Top of parent's content box
	nuVerticalBindingCenter,		// Center of parent's content box
	nuVerticalBindingBottom,		// Bottom of parent's content box
	nuVerticalBindingBaseline,		// Parent's baseline
};

enum nuHorizontalBindings
{
	nuHorizontalBindingNULL,
	nuHorizontalBindingLeft,		// Left of parent's content box
	nuHorizontalBindingCenter,		// Center of parent's content box
	nuHorizontalBindingRight,		// Right of parent's content box
};

enum nuPositionType
{
	// All of these definitions are the same as HTML's
	nuPositionStatic,		// Default, regular position
	nuPositionAbsolute,		// Absolute, relative to previous explicitly parent that was anything other than "Static". Does not affect flow of subsequent siblings.
	nuPositionRelative,		// Like Absolute, but does affect flow. Flow is the "ghost" of where we would have been, before being moved relatively.
	nuPositionFixed,		// Fixed, according to root device coordinate system. In other words, completely independent of DOM hierarchy.
};

enum nuBoxSizeType
{
	nuBoxSizeContent,
	nuBoxSizeBorder,
	nuBoxSizeMargin			// Created initially for the <body> element
};

enum nuBreakType
{
	nuBreakNULL,
	nuBreakBefore,			// Break flow before element
	nuBreakAfter			// Break flow after element
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

// The order of the box components (left,top,right,bottom) must be consistent with the order in nuStyleBox
// In addition, all 'box' types must fall between Margin_Left and Border_Bottom. This is simply for sanity. It is verified
// inside nuStyle.SetBox()
#define NU_STYLE_DEFINE \
XX(NULL, 0) \
XY(Color) \
XY(Display) \
XY(Background) \
XY(BackgroundImage) \
XY(Text_Align_Vertical) \
\
XY(Break) \
XY(Dummy3_UseMe) \
\
XY(Margin_Left) \
XY(Margin_Top) \
XY(Margin_Right) \
XY(Margin_Bottom) \
\
XY(Padding_Left) \
XY(Padding_Top) \
XY(Padding_Right) \
XY(Padding_Bottom) \
\
XY(Border_Left) \
XY(Border_Top) \
XY(Border_Right) \
XY(Border_Bottom) \
\
XY(Width) \
XY(Height) \
\
XY(Top) \
XY(VCenter) \
XY(Bottom) \
XY(Baseline) \
XY(Left) \
XY(HCenter) \
XY(Right) \
\
XY(FontSize) \
XY(FontFamily) \
\
XY(BorderRadius) \
XY(Position) \
XY(Flow_Axis) \
XY(Flow_Direction_Horizontal) \
XY(Flow_Direction_Vertical) \
XY(BoxSizing) \
XY(END)

#define XX(a,b) nuCat##a = b,
#define XY(a) nuCat##a,
enum nuStyleCategories {
	NU_STYLE_DEFINE
	nuCatFIRST = nuCatColor,
};
#undef XX
#undef XY

static_assert( nuCatMargin_Left % 4 == 0, "Start of boxes must be multiple of 4" );

inline nuStyleCategories nuCatMakeBaseBox( nuStyleCategories c ) { return (nuStyleCategories) (c & ~3); }

// Styles that are inherited by default
// Generally it is text styles that are inherited
// Inheritance means that child nodes inherit the styles of their parents
const int						nuNumInheritedStyleCategories = 4;
extern const nuStyleCategories	nuInheritedStyleCategories[nuNumInheritedStyleCategories];

/* Single style attribute (such as Margin-Left, Width, FontSize, etc).
This must be zero-initializable (i.e. with memset(0)).
It must remain small.
Currently, sizeof(nuStyleAttrib) = 8.
*/
class NUAPI nuStyleAttrib
{
public:
	enum Flag
	{
		// This means that the attribute takes its value from its closest ancestor in the DOM tree.
		// Some styles are inherited by default (the list specified inside nuInheritedStyleCategories).
		FlagInherit = 1		
	};

	uint8				Category;		// type nuStyleCategories
	uint8				SubType;
	uint8				Flags;
	uint8				Unused2;
	union
	{
		uint32			ValU32;
		float			ValF;
	};

	nuStyleAttrib();

	void SetInherit( nuStyleCategories cat );

	void SetColor( nuStyleCategories cat, nuColor val )				{ SetU32( cat, val.u ); }
	void SetSize( nuStyleCategories cat, nuSize val )				{ SetWithSubtypeF( cat, val.Type, val.Val ); }
	void SetDisplay( nuDisplayType val )							{ SetU32( nuCatDisplay, val ); }
	void SetBorderRadius( nuSize val )								{ SetSize( nuCatBorderRadius, val ); }
	void SetPosition( nuPositionType val )							{ SetU32( nuCatPosition, val ); }
	void SetFont( nuFontID val )									{ SetU32( nuCatFontFamily, val ); }
	void SetBackgroundImage( const char* image, nuDoc* doc )		{ SetString( nuCatBackgroundImage, image, doc ); }
	void SetBreak( nuBreakType type )								{ SetU32( nuCatBreak, type); }
	void SetFlowAxis( nuFlowAxis axis )								{ SetU32( nuCatFlow_Axis, axis ); }
	void SetFlowDirectionHorizonal( nuFlowDirection dir )			{ SetU32( nuCatFlow_Direction_Horizontal, dir ); }
	void SetFlowDirectionVertical( nuFlowDirection dir )			{ SetU32( nuCatFlow_Direction_Vertical, dir ); }
	void SetBoxSizing( nuBoxSizeType type )							{ SetU32( nuCatBoxSizing, type ); }
	void SetTextAlignVertical( nuTextAlignVertical align )			{ SetU32( nuCatText_Align_Vertical, align ); }
	void SetLeft( nuHorizontalBindings bind )						{ SetU32( nuCatLeft, bind ); }
	void SetHCenter( nuHorizontalBindings bind )					{ SetU32( nuCatHCenter, bind ); }
	void SetRight( nuHorizontalBindings bind )						{ SetU32( nuCatRight, bind ); }
	void SetTop( nuVerticalBindings bind )							{ SetU32( nuCatTop, bind ); }
	void SetVCenter( nuVerticalBindings bind )						{ SetU32( nuCatVCenter, bind ); }
	void SetBottom( nuVerticalBindings bind )						{ SetU32( nuCatBottom, bind ); }
	void SetBaseline( nuVerticalBindings bind )						{ SetU32( nuCatBaseline, bind ); }

	// Generic Set() that is used by template code
	void Set( nuStyleCategories cat, nuColor val )					{ SetColor( cat, val ); }
	void Set( nuStyleCategories cat, nuSize val )					{ SetSize( cat, val ); }
	void Set( nuStyleCategories cat, nuDisplayType val )			{ SetDisplay( val ); }
	void Set( nuStyleCategories cat, nuPositionType val )			{ SetPosition( val ); }
	void Set( nuStyleCategories cat, nuBreakType val )				{ SetBreak( val ); }
	void Set( nuStyleCategories cat, nuFlowAxis val )				{ SetFlowAxis( val ); }
	void Set( nuStyleCategories cat, nuFlowDirection val )			{ SetU32( cat, val ); }
	void Set( nuStyleCategories cat, nuBoxSizeType val )			{ SetBoxSizing( val ); }
	void Set( nuStyleCategories cat, nuTextAlignVertical val )		{ SetU32( cat, val ); }
	void Set( nuStyleCategories cat, nuHorizontalBindings val )		{ SetU32( cat, val ); }
	void Set( nuStyleCategories cat, nuVerticalBindings val )		{ SetU32( cat, val ); }
	void Set( nuStyleCategories cat, nuFontID val )					{ SetFont( val ); }
	void Set( nuStyleCategories cat, const char* val, nuDoc* doc )	{ SetString( cat, val, doc ); }

	bool					IsNull() const							{ return Category == nuCatNULL; }
	bool					IsInherit() const						{ return Flags == FlagInherit; }

	nuStyleCategories		GetCategory() const						{ return (nuStyleCategories) Category; }
	nuSize					GetSize() const							{ return nuSize::Make( (nuSize::Types) SubType, ValF ); }
	nuColor					GetColor() const						{ return nuColor::Make( ValU32 ); }
	nuDisplayType			GetDisplayType() const					{ return (nuDisplayType) ValU32; }
	nuPositionType			GetPositionType() const					{ return (nuPositionType) ValU32; }
	nuBreakType				GetBreakType() const					{ return (nuBreakType) ValU32; }
	int						GetStringID() const						{ return (int) ValU32; }
	nuFlowAxis				GetFlowAxis() const						{ return (nuFlowAxis) ValU32; }
	nuFlowDirection			GetFlowDirectionMajor() const			{ return (nuFlowDirection) ValU32; }
	nuFlowDirection			GetFlowDirectionMinor() const			{ return (nuFlowDirection) ValU32; }
	nuBoxSizeType			GetBoxSizing() const					{ return (nuBoxSizeType) ValU32; }
	nuTextAlignVertical		GetTextAlignVertical() const			{ return (nuTextAlignVertical) ValU32; }
	nuHorizontalBindings	GetHorizontalBinding() const			{ return (nuHorizontalBindings) ValU32; }
	nuVerticalBindings		GetVerticalBinding() const				{ return (nuVerticalBindings) ValU32; }

	const char*				GetBackgroundImage( nuStringTable* strings ) const;
	nuFontID				GetFont() const;

protected:
	void SetString( nuStyleCategories cat, const char* str, nuDoc* doc );
	void SetU32( nuStyleCategories cat, uint32 val );
	void SetWithSubtypeU32( nuStyleCategories cat, uint8 subtype, uint32 val );
	void SetWithSubtypeF( nuStyleCategories cat, uint8 subtype, float val );
};

/* Collection of style attributes (border-width-left, color, etc)
This container is simple and list-based.
There is a different container called nuStyleSet that is more performant,
built for use during rendering.
*/
class NUAPI nuStyle
{
public:
	podvec<nuStyleAttrib>	Attribs;

	bool					Parse( const char* t, nuDoc* doc );
	bool					Parse( const char* t, intp maxLen, nuDoc* doc );
	const nuStyleAttrib*	Get( nuStyleCategories cat ) const;
	void					SetBox( nuStyleCategories cat, nuStyleBox val );
	void					GetBox( nuStyleCategories cat, nuStyleBox& box ) const;
	void					Set( const nuStyleAttrib& attrib );
	void					Set( nuStyleCategories cat, nuStyleBox val );
	//void					Compute( const nuDoc& doc, const nuDomEl& node );
	void					Discard();
	void					CloneSlowInto( nuStyle& c ) const;
	void					CloneFastInto( nuStyle& c, nuPool* pool ) const;

// Setter functions with 2 parameters
#define NUSTYLE_SETTERS_2P \
	XX(BackgroundColor,		nuColor,	SetColor,	nuCatBackground) \
	XX(Width,				nuSize,		SetSize,	nuCatWidth) \
	XX(Height,				nuSize,		SetSize,	nuCatHeight) \
	XX(Left,				nuSize,		SetSize,	nuCatLeft) \
	XX(Right,				nuSize,		SetSize,	nuCatRight) \
	XX(Top,					nuSize,		SetSize,	nuCatTop) \
	XX(Bottom,				nuSize,		SetSize,	nuCatBottom) \

// Setter functions with 1 parameter
#define NUSTYLE_SETTERS_1P \
	XX(Position,			nuPositionType,	SetPosition) \

#define XX(name, type, setfunc, cat)	void Set##name( type value );
	NUSTYLE_SETTERS_2P
#undef XX

#define XX(name, type, setfunc)			void Set##name( type value );
	NUSTYLE_SETTERS_1P
#undef XX

protected:
	//void					MergeInZeroCopy( int n, const nuStyle** src );
	void					SetBoxInternal( nuStyleCategories catBase, nuStyleBox val );
};

FHASH_SETUP_CLASS_CTOR_DTOR(nuStyle, nuStyle);

/* A bag of styles in a performant container.

Analysis of storage (assuming nuCatEND = 128)

	Number of attributes	Size of Lookup			Size of Attribs		Total marginal size
	1 << 2 = 4				128 * 2 / 8 = 32		4 * 8   = 32		32 + 32    = 64
	1 << 4 = 16				128 * 4 / 8 = 64		16 * 8  = 128		64 + 128   = 192
	1 << 8 = 256			128 * 8 / 8 = 128		256 * 8 = 2048		128 + 2048 = 2176

One very significant optimization that remains here is to not grow Attrib[] so heavily.
Growing from 16 to 256 is an insane leap.

Idea: There are some attributes that need only a few bits. Instead of packing each of these
into 8 bytes, we can instead store groups of attributes in special 8 byte blocks.

More Idea: I think I might end up writing a very specialized container for this stuff.. where we
separate things out into really tight bags of properties. But we'll wait until it's a bottleneck.

*/
class NUAPI nuStyleSet
{
public:
	static const uint32 InitialBitsPerSlot = 2;		// 1 << 2 = 4, is our lowest non-empty size
	static const uint32 SlotOffset = 1;				// We need to reserve zero for the "empty" state of a slot

					nuStyleSet();		// Simply calls Reset()
					~nuStyleSet();		// Destructor does nothing

	void			Set( int n, const nuStyleAttrib* attribs, nuPool* pool );
	void			Set( const nuStyleAttrib& attrib, nuPool* pool );
	nuStyleAttrib	Get( nuStyleCategories cat ) const;
	bool			Contains( nuStyleCategories cat ) const;
	void			Reset();

protected:
	typedef void  (*SetSlotFunc) ( void* lookup, nuStyleCategories cat, int32 slot );
	typedef int32 (*GetSlotFunc) ( const void* lookup, nuStyleCategories cat );

	void*			Lookup;			// Variable bit-width table that indexes into Attribs. Size is always nuCatEND.
	nuStyleAttrib*	Attribs;		// The Category field in here is wasted.
	int32			Count;			// Size of Attribs
	int32			Capacity;		// Capacity of Attribs
	uint32			BitsPerSlot;	// Number of bits in each slot of Lookup. Our possible sizes are 2,4,8.
	SetSlotFunc		SetSlotF;
	GetSlotFunc		GetSlotF;

	void			Grow( nuPool* pool );
	int32			GetSlot( nuStyleCategories cat ) const;
	void			SetSlot( nuStyleCategories cat, int32 slot );
	void			DebugCheckSanity() const;

	static void		MigrateLookup( const void* lutsrc, void* lutdst, GetSlotFunc getter, SetSlotFunc setter );

	template<uint32 BITS_PER_SLOT>
	static void		TSetSlot( void* lookup, nuStyleCategories cat, int32 slot );

	template<uint32 BITS_PER_SLOT>
	static int32	TGetSlot( const void* lookup, nuStyleCategories cat );

	static int32	GetSlot2( const void* lookup, nuStyleCategories cat );
	static int32	GetSlot4( const void* lookup, nuStyleCategories cat );
	static int32	GetSlot8( const void* lookup, nuStyleCategories cat );
	static void		SetSlot2( void* lookup, nuStyleCategories cat, int32 slot );
	static void		SetSlot4( void* lookup, nuStyleCategories cat, int32 slot );
	static void		SetSlot8( void* lookup, nuStyleCategories cat, int32 slot );

	// The -1 here is for SlotOffset
	static int32	CapacityAt( uint32 bitsPerSlot )	{ return (1 << bitsPerSlot) - 1; }

};

struct nuBox16
{
	uint16 Left;
	uint16 Top;
	uint16 Right;
	uint16 Bottom;
};

// The set of style information that is used by the renderer
// This is baked in by the Layout engine.
// This struct is present in every single nuRenderDomNode, so it pays to keep it tight.
class NUAPI nuStyleRender
{
public:
	nuBox16 BorderSize;
	nuColor BackgroundColor;
	int		BackgroundImageID;
	float	BorderRadius;

	nuStyleRender() { memset(this, 0, sizeof(*this)); }
};

/* Store all style classes in one table, that is owned by one document.
This allows us to reference styles by a 32-bit integer ID instead of by name.
*/
class NUAPI nuStyleTable
{
public:
					nuStyleTable();
					~nuStyleTable();

	void			Discard();
	nuStyle*		GetOrCreate( const char* name );
	const nuStyle*	GetByID( nuStyleID id ) const;
	nuStyleID		GetStyleID( const char* name );
	void			CloneSlowInto( nuStyleTable& c ) const;					// Does not clone NameToIndex
	void			CloneFastInto( nuStyleTable& c, nuPool* pool ) const;	// Does not clone NameToIndex

protected:
	podvec<nuString>			Names;		// Names and Styles are parallels
	podvec<nuStyle>				Styles;
	podvec<int>					UnusedSlots;
	fhashmap<nuString, int>		NameToIndex;
};


NUAPI bool nuDisplayTypeParse( const char* s, intp len, nuDisplayType& t );
NUAPI bool nuPositionTypeParse( const char* s, intp len, nuPositionType& t );
NUAPI bool nuBreakTypeParse( const char* s, intp len, nuBreakType& t );
NUAPI bool nuFlowAxisParse( const char* s, intp len, nuFlowAxis& t );
NUAPI bool nuFlowDirectionParse( const char* s, intp len, nuFlowDirection& t );
NUAPI bool nuBoxSizeParse( const char* s, intp len, nuBoxSizeType& t );
NUAPI bool nuTextAlignVerticalParse( const char* s, intp len, nuTextAlignVertical& t );
NUAPI bool nuHorizontalBindingParse( const char* s, intp len, nuHorizontalBindings& t );
NUAPI bool nuVerticalBindingParse( const char* s, intp len, nuVerticalBindings& t );

