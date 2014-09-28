#pragma once

#include "xoDefs.h"
#include "xoString.h"

// The list of styles that are inherited by child nodes lives in xoInheritedStyleCategories

// Represents a size that is zero, pixels, eye pixels, points, percent.
// TODO: em
// Zero is represented as 0 pixels
// See "layout" documentation for explanation of units.
struct XOAPI xoSize
{
	enum	Types { NONE = 0, PX, PT, EP, PERCENT };
	float 	Val;
	Types 	Type;

	static xoSize	Make( Types t, float v )	{ xoSize s = {v, t}; return s; }
	static xoSize	Percent( float v )			{ xoSize s = {v, PERCENT}; return s; }
	static xoSize	Points( float v )			{ xoSize s = {v, PT}; return s; }
	static xoSize	Pixels( float v )			{ xoSize s = {v, PX}; return s; }
	static xoSize	EyePixels( float v )		{ xoSize s = {v, EP}; return s; }
	static xoSize	Zero()						{ xoSize s = {0, PX}; return s; }
	static xoSize	Null()						{ xoSize s = {0, NONE}; return s; }

	static bool		Parse( const char* s, intp len, xoSize& v );
};

// Convenience struct used during layout computation
struct XOAPI xoStyleBox
{
	// This order (left,top,right,bottom) must be consistent with the order presented inside XO_STYLE_DEFINE
	union
	{
		struct 
		{
			xoSize	Left, Top, Right, Bottom;
		};
		xoSize All[4];
	};

	static bool			Parse( const char* s, intp len, xoStyleBox& v );
	static xoStyleBox	Make( xoSize left, xoSize top, xoSize right, xoSize bottom )	{ xoStyleBox b; b.Left = left; b.Top = top; b.Right = right; b.Bottom = bottom; return b; }
	static xoStyleBox	MakeUniform( xoSize all )										{ xoStyleBox b; b.Left = all; b.Top = all; b.Right = all; b.Bottom = all; return b; }
	static xoStyleBox	MakeZero() { xoStyleBox b; b.SetZero(); return b; }
	void SetZero() { Left = Top = Right = Bottom = xoSize::Pixels(0); }
};

enum xoDisplayType
{
	xoDisplayBlock,
	xoDisplayInline
};

enum xoFlowDirection
{
	xoFlowDirectionNormal,
	xoFlowDirectionReversed
};

enum xoFlowAxis
{
	xoFlowAxisVertical,		// Vertical is the default
	xoFlowAxisHorizontal
};

enum xoTextAlignVertical
{
	xoTextAlignVerticalBaseline,	// Baseline is default
	xoTextAlignVerticalTop			// This is unlikely to be useful, but having it proves that we *could* make other rules if proved useful
};

enum xoVerticalBindings
{
	xoVerticalBindingNULL,
	xoVerticalBindingTop,			// Top of parent's content box
	xoVerticalBindingCenter,		// Center of parent's content box
	xoVerticalBindingBottom,		// Bottom of parent's content box
	xoVerticalBindingBaseline,		// Parent's baseline
};

enum xoHorizontalBindings
{
	xoHorizontalBindingNULL,
	xoHorizontalBindingLeft,		// Left of parent's content box
	xoHorizontalBindingCenter,		// Center of parent's content box
	xoHorizontalBindingRight,		// Right of parent's content box
};

enum xoPositionType
{
	// All of these definitions are the same as HTML's
	xoPositionStatic,		// Default, regular position
	xoPositionAbsolute,		// Absolute, relative to previous explicitly parent that was anything other than "Static". Does not affect flow of subsequent siblings.
	xoPositionRelative,		// Like Absolute, but does affect flow. Flow is the "ghost" of where we would have been, before being moved relatively.
	xoPositionFixed,		// Fixed, according to root device coordinate system. In other words, completely independent of DOM hierarchy.
};

enum xoBoxSizeType
{
	xoBoxSizeContent,
	xoBoxSizeBorder,
	xoBoxSizeMargin			// Created initially for the <body> element
};

enum xoBreakType
{
	xoBreakNULL,
	xoBreakBefore,			// Break flow before element
	xoBreakAfter			// Break flow after element
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

// The order of the box components (left,top,right,bottom) must be consistent with the order in xoStyleBox
// In addition, all 'box' types must fall between Margin_Left and Border_Bottom. This is simply for sanity. It is verified
// inside xoStyle.SetBox()
#define XO_STYLE_DEFINE \
XX(NULL, 0) \
XY(Color) \
XY(Display) \
XY(Background) \
XY(BackgroundImage) \
XY(Text_Align_Vertical) \
\
XY(Break) \
XY(CanFocus) \
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
XY(BorderColor_Left) \
XY(BorderColor_Top) \
XY(BorderColor_Right) \
XY(BorderColor_Bottom) \
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

#define XX(a,b) xoCat##a = b,
#define XY(a) xoCat##a,
enum xoStyleCategories {
	XO_STYLE_DEFINE
	xoCatFIRST = xoCatColor,
};
#undef XX
#undef XY

static_assert( xoCatMargin_Left % 4 == 0, "Start of boxes must be multiple of 4" );

inline xoStyleCategories xoCatMakeBaseBox( xoStyleCategories c ) { return (xoStyleCategories) (c & ~3); }

// Styles that are inherited by default
// Generally it is text styles that are inherited
// Inheritance means that child nodes inherit the styles of their parents
const int						xoNumInheritedStyleCategories = 4;
extern const xoStyleCategories	xoInheritedStyleCategories[xoNumInheritedStyleCategories];

/* Single style attribute (such as Margin-Left, Width, FontSize, etc).
This must be zero-initializable (i.e. with memset(0)).
It must remain small.
Currently, sizeof(xoStyleAttrib) = 8.
*/
class XOAPI xoStyleAttrib
{
public:
	enum Flag
	{
		// This means that the attribute takes its value from its closest ancestor in the DOM tree.
		// Some styles are inherited by default (the list specified inside xoInheritedStyleCategories).
		FlagInherit = 1		
	};

	uint8				Category;		// type xoStyleCategories
	uint8				SubType;
	uint8				Flags;
	uint8				Unused2;
	union
	{
		uint32			ValU32;
		float			ValF;
	};

	xoStyleAttrib();

	void SetInherit( xoStyleCategories cat );

	void SetColor( xoStyleCategories cat, xoColor val )				{ SetU32( cat, val.u ); }
	void SetSize( xoStyleCategories cat, xoSize val )				{ SetWithSubtypeF( cat, val.Type, val.Val ); }
	void SetDisplay( xoDisplayType val )							{ SetU32( xoCatDisplay, val ); }
	void SetBorderRadius( xoSize val )								{ SetSize( xoCatBorderRadius, val ); }
	void SetPosition( xoPositionType val )							{ SetU32( xoCatPosition, val ); }
	void SetFont( xoFontID val )									{ SetU32( xoCatFontFamily, val ); }
	void SetBackgroundImage( const char* image, xoDoc* doc )		{ SetString( xoCatBackgroundImage, image, doc ); }
	void SetBreak( xoBreakType type )								{ SetU32( xoCatBreak, type); }
	void SetCanFocus( bool canFocus )								{ SetU32( xoCatCanFocus, canFocus ); }
	void SetFlowAxis( xoFlowAxis axis )								{ SetU32( xoCatFlow_Axis, axis ); }
	void SetFlowDirectionHorizonal( xoFlowDirection dir )			{ SetU32( xoCatFlow_Direction_Horizontal, dir ); }
	void SetFlowDirectionVertical( xoFlowDirection dir )			{ SetU32( xoCatFlow_Direction_Vertical, dir ); }
	void SetBoxSizing( xoBoxSizeType type )							{ SetU32( xoCatBoxSizing, type ); }
	void SetTextAlignVertical( xoTextAlignVertical align )			{ SetU32( xoCatText_Align_Vertical, align ); }
	void SetLeft( xoHorizontalBindings bind )						{ SetU32( xoCatLeft, bind ); }
	void SetHCenter( xoHorizontalBindings bind )					{ SetU32( xoCatHCenter, bind ); }
	void SetRight( xoHorizontalBindings bind )						{ SetU32( xoCatRight, bind ); }
	void SetTop( xoVerticalBindings bind )							{ SetU32( xoCatTop, bind ); }
	void SetVCenter( xoVerticalBindings bind )						{ SetU32( xoCatVCenter, bind ); }
	void SetBottom( xoVerticalBindings bind )						{ SetU32( xoCatBottom, bind ); }
	void SetBaseline( xoVerticalBindings bind )						{ SetU32( xoCatBaseline, bind ); }

	// Generic Set() that is used by template code
	void Set( xoStyleCategories cat, xoColor val )					{ SetColor( cat, val ); }
	void Set( xoStyleCategories cat, xoSize val )					{ SetSize( cat, val ); }
	void Set( xoStyleCategories cat, xoDisplayType val )			{ SetDisplay( val ); }
	void Set( xoStyleCategories cat, xoPositionType val )			{ SetPosition( val ); }
	void Set( xoStyleCategories cat, xoBreakType val )				{ SetBreak( val ); }
	void Set( xoStyleCategories cat, xoFlowAxis val )				{ SetFlowAxis( val ); }
	void Set( xoStyleCategories cat, xoFlowDirection val )			{ SetU32( cat, val ); }
	void Set( xoStyleCategories cat, xoBoxSizeType val )			{ SetBoxSizing( val ); }
	void Set( xoStyleCategories cat, xoTextAlignVertical val )		{ SetU32( cat, val ); }
	void Set( xoStyleCategories cat, xoHorizontalBindings val )		{ SetU32( cat, val ); }
	void Set( xoStyleCategories cat, xoVerticalBindings val )		{ SetU32( cat, val ); }
	void Set( xoStyleCategories cat, xoFontID val )					{ SetFont( val ); }
	void Set( xoStyleCategories cat, const char* val, xoDoc* doc )	{ SetString( cat, val, doc ); }

	void SetBool( xoStyleCategories cat, bool val )					{ SetU32( cat, val ); }

	bool					IsNull() const							{ return Category == xoCatNULL; }
	bool					IsInherit() const						{ return Flags == FlagInherit; }

	xoStyleCategories		GetCategory() const						{ return (xoStyleCategories) Category; }
	xoSize					GetSize() const							{ return xoSize::Make( (xoSize::Types) SubType, ValF ); }
	xoColor					GetColor() const						{ return xoColor::Make( ValU32 ); }
	xoDisplayType			GetDisplayType() const					{ return (xoDisplayType) ValU32; }
	xoPositionType			GetPositionType() const					{ return (xoPositionType) ValU32; }
	xoBreakType				GetBreakType() const					{ return (xoBreakType) ValU32; }
	bool					GetCanFocus() const						{ return ValU32 != 0; }
	int						GetStringID() const						{ return (int) ValU32; }
	xoFlowAxis				GetFlowAxis() const						{ return (xoFlowAxis) ValU32; }
	xoFlowDirection			GetFlowDirectionMajor() const			{ return (xoFlowDirection) ValU32; }
	xoFlowDirection			GetFlowDirectionMinor() const			{ return (xoFlowDirection) ValU32; }
	xoBoxSizeType			GetBoxSizing() const					{ return (xoBoxSizeType) ValU32; }
	xoTextAlignVertical		GetTextAlignVertical() const			{ return (xoTextAlignVertical) ValU32; }
	xoHorizontalBindings	GetHorizontalBinding() const			{ return (xoHorizontalBindings) ValU32; }
	xoVerticalBindings		GetVerticalBinding() const				{ return (xoVerticalBindings) ValU32; }

	const char*				GetBackgroundImage( xoStringTable* strings ) const;
	xoFontID				GetFont() const;

protected:
	void SetString( xoStyleCategories cat, const char* str, xoDoc* doc );
	void SetU32( xoStyleCategories cat, uint32 val );
	void SetWithSubtypeU32( xoStyleCategories cat, uint8 subtype, uint32 val );
	void SetWithSubtypeF( xoStyleCategories cat, uint8 subtype, float val );
};

/* Collection of style attributes (border-width-left, color, etc)
This container is simple and list-based.
There is a different container called xoStyleSet that is more performant,
built for use during rendering.
*/
class XOAPI xoStyle
{
public:
	podvec<xoStyleAttrib>	Attribs;

	bool					Parse( const char* t, xoDoc* doc );
	bool					Parse( const char* t, intp maxLen, xoDoc* doc );
	const xoStyleAttrib*	Get( xoStyleCategories cat ) const;
	void					SetBox( xoStyleCategories cat, xoStyleBox val );
	void					GetBox( xoStyleCategories cat, xoStyleBox& box ) const;
	void					SetUniformBox( xoStyleCategories cat, xoStyleAttrib val );
	void					SetUniformBox( xoStyleCategories cat, xoColor color );
	void					SetUniformBox( xoStyleCategories cat, xoSize size );
	void					Set( xoStyleAttrib attrib );
	void					Set( xoStyleCategories cat, xoStyleBox val );	// This overload is identical to SetBox, but needs to be present for templated parsing functions
	//void					Compute( const xoDoc& doc, const xoDomEl& node );
	void					Discard();
	void					CloneSlowInto( xoStyle& c ) const;
	void					CloneFastInto( xoStyle& c, xoPool* pool ) const;

	bool					IsEmpty() const { return Attribs.size() == 0; }

// Setter functions with 2 parameters
#define NUSTYLE_SETTERS_2P \
	XX(BackgroundColor,		xoColor,	SetColor,	xoCatBackground) \
	XX(Width,				xoSize,		SetSize,	xoCatWidth) \
	XX(Height,				xoSize,		SetSize,	xoCatHeight) \
	XX(Left,				xoSize,		SetSize,	xoCatLeft) \
	XX(Right,				xoSize,		SetSize,	xoCatRight) \
	XX(Top,					xoSize,		SetSize,	xoCatTop) \
	XX(Bottom,				xoSize,		SetSize,	xoCatBottom) \

// Setter functions with 1 parameter
#define NUSTYLE_SETTERS_1P \
	XX(Position,			xoPositionType,	SetPosition) \

#define XX(name, type, setfunc, cat)	void Set##name( type value );
	NUSTYLE_SETTERS_2P
#undef XX

#define XX(name, type, setfunc)			void Set##name( type value );
	NUSTYLE_SETTERS_1P
#undef XX

protected:
	//void					MergeInZeroCopy( int n, const xoStyle** src );
	void					SetBoxInternal( xoStyleCategories catBase, xoStyleBox val );
};

FHASH_SETUP_CLASS_CTOR_DTOR(xoStyle, xoStyle);

/* A bag of styles in a performant container.

Analysis of storage (assuming xoCatEND = 128)

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
class XOAPI xoStyleSet
{
public:
	static const uint32 InitialBitsPerSlot = 2;		// 1 << 2 = 4, is our lowest non-empty size
	static const uint32 SlotOffset = 1;				// We need to reserve zero for the "empty" state of a slot

					xoStyleSet();		// Simply calls Reset()
					~xoStyleSet();		// Destructor does nothing

	void			Set( int n, const xoStyleAttrib* attribs, xoPool* pool );
	void			Set( const xoStyleAttrib& attrib, xoPool* pool );
	xoStyleAttrib	Get( xoStyleCategories cat ) const;
	bool			Contains( xoStyleCategories cat ) const;
	void			Reset();

protected:
	typedef void  (*SetSlotFunc) ( void* lookup, xoStyleCategories cat, int32 slot );
	typedef int32 (*GetSlotFunc) ( const void* lookup, xoStyleCategories cat );

	void*			Lookup;			// Variable bit-width table that indexes into Attribs. Size is always xoCatEND.
	xoStyleAttrib*	Attribs;		// The Category field in here is wasted.
	int32			Count;			// Size of Attribs
	int32			Capacity;		// Capacity of Attribs
	uint32			BitsPerSlot;	// Number of bits in each slot of Lookup. Our possible sizes are 2,4,8.
	SetSlotFunc		SetSlotF;
	GetSlotFunc		GetSlotF;

	void			Grow( xoPool* pool );
	int32			GetSlot( xoStyleCategories cat ) const;
	void			SetSlot( xoStyleCategories cat, int32 slot );
	void			DebugCheckSanity() const;

	static void		MigrateLookup( const void* lutsrc, void* lutdst, GetSlotFunc getter, SetSlotFunc setter );

	template<uint32 BITS_PER_SLOT>
	static void		TSetSlot( void* lookup, xoStyleCategories cat, int32 slot );

	template<uint32 BITS_PER_SLOT>
	static int32	TGetSlot( const void* lookup, xoStyleCategories cat );

	static int32	GetSlot2( const void* lookup, xoStyleCategories cat );
	static int32	GetSlot4( const void* lookup, xoStyleCategories cat );
	static int32	GetSlot8( const void* lookup, xoStyleCategories cat );
	static void		SetSlot2( void* lookup, xoStyleCategories cat, int32 slot );
	static void		SetSlot4( void* lookup, xoStyleCategories cat, int32 slot );
	static void		SetSlot8( void* lookup, xoStyleCategories cat, int32 slot );

	// The -1 here is for SlotOffset
	static int32	CapacityAt( uint32 bitsPerSlot )	{ return (1 << bitsPerSlot) - 1; }

};

// This is a style class, such as ".button"
class XOAPI xoStyleClass
{
public:
	xoStyle		Default;	// Default styles
	xoStyle		Hover;		// Styles present when cursor is over node
	xoStyle		Focus;		// Styles present when object has focus
};

// The set of style information that is used by the renderer
// This is baked in by the Layout engine.
// This struct is present in every single xoRenderDomNode, so it pays to keep it tight.
class XOAPI xoStyleRender
{
public:
	xoBox16 BorderSize;
	xoBox16 Padding;			// This is probably not necessary. See log entry from 2014-08-02
	xoColor BackgroundColor;
	xoColor BorderColor;
	int		BackgroundImageID;
	float	BorderRadius;
	bool	HasHoverStyle : 1;	// Element's appearance depends upon whether the cursor is over it it.
	bool	HasFocusStyle : 1;	// Element's appearance depends upon whether it has the focus

	xoStyleRender() { memset(this, 0, sizeof(*this)); }
};

/* Store all style classes in one table, that is owned by one document.
This allows us to reference styles by a 32-bit integer ID instead of by name.
*/
class XOAPI xoStyleTable
{
public:
						xoStyleTable();
						~xoStyleTable();

	void				AddDummyStyleZero();
	void				Discard();
	xoStyleClass*		GetOrCreate( const char* name );
	const xoStyleClass*	GetByID( xoStyleClassID id ) const;
	xoStyleClassID		GetClassID( const char* name );
	void				CloneSlowInto( xoStyleTable& c ) const;					// Does not clone NameToIndex
	void				CloneFastInto( xoStyleTable& c, xoPool* pool ) const;	// Does not clone NameToIndex

protected:
	podvec<xoString>			Names;		// Names and Classes are parallel
	podvec<xoStyleClass>		Classes;
	podvec<int>					UnusedSlots;
	fhashmap<xoString, int>		NameToIndex;
};


XOAPI bool xoParseDisplayType( const char* s, intp len, xoDisplayType& t );
XOAPI bool xoParsePositionType( const char* s, intp len, xoPositionType& t );
XOAPI bool xoParseBreakType( const char* s, intp len, xoBreakType& t );
XOAPI bool xoParseFlowAxis( const char* s, intp len, xoFlowAxis& t );
XOAPI bool xoParseFlowDirection( const char* s, intp len, xoFlowDirection& t );
XOAPI bool xoParseBoxSize( const char* s, intp len, xoBoxSizeType& t );
XOAPI bool xoParseTextAlignVertical( const char* s, intp len, xoTextAlignVertical& t );
XOAPI bool xoParseHorizontalBinding( const char* s, intp len, xoHorizontalBindings& t );
XOAPI bool xoParseVerticalBinding( const char* s, intp len, xoVerticalBindings& t );
XOAPI bool xoParseBorder( const char* s, intp len, xoStyle& style );

