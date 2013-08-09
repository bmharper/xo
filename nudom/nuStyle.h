#pragma once

#include "nuDefs.h"
#include "nuString.h"

// Represents a size that is zero, pixels, points, percent. TODO: em
// Zero is represented as 0 pixels
struct NUAPI nuSize
{
	enum	Types { NONE = 0, PX, PT, PERCENT };
	float 	Val;
	Types 	Type;

	static nuSize Make( Types t, float v )	{ nuSize s = {v, t}; return s; }
	static nuSize Percent( float v )		{ nuSize s = {v, PERCENT}; return s; }
	static nuSize Points( float v )			{ nuSize s = {v, PT}; return s; }
	static nuSize Pixels( float v )			{ nuSize s = {v, PX}; return s; }
	static nuSize Zero()					{ nuSize s = {0, PX}; return s; }
	static nuSize Parse( const char* s, intp len );
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

	static nuStyleBox Parse( const char* s, intp len );
	static nuStyleBox Make( nuSize left, nuSize top, nuSize right, nuSize bottom ) { nuStyleBox b; b.Left = left; b.Top = top; b.Right = right; b.Bottom = bottom; return b; }
	static nuStyleBox MakeZero() { nuStyleBox b; b.SetZero(); return b; }
	void SetZero() { Left = Top = Right = Bottom = nuSize::Pixels(0); }
};

struct NUAPI nuRGBA
{
	union {
		struct {
#if ENDIANLITTLE
			uint8 a, b, g, r;
#else
			uint8 r: 8;
			uint8 g: 8;
			uint8 b: 8;
			uint8 a: 8;
#endif
		};
		uint32 u;
	};
	static nuRGBA RGBA(uint8 r, uint8 g, uint8 b, uint8 a) { nuRGBA c; c.r = r; c.g = g; c.b = b; c.a = a; return c; }
};

#define NURGBA(r, g, b, a) ((a) << 24 | (b) << 16 | (g) << 8 | (r))

// This is non-premultipled alpha
struct NUAPI nuColor
{
	void	Set( uint8 _r, uint8 _g, uint8 _b, uint8 _a ) { r = _r; g = _g; b = _b; a = _a; }
	uint32	GetRGBA() const { nuRGBA x; x.r = r; x.g = g; x.b = b; x.a = a; return x.u; }

	bool	operator==( const nuColor& x ) const { return u == x.u; }
	bool	operator!=( const nuColor& x ) const { return u != x.u; }

	static nuColor Parse( const char* s, intp len );
	static nuColor RGBA( uint8 _r, uint8 _g, uint8 _b, uint8 _a )		{ nuColor c; c.Set(_r,_g,_b,_a); return c; }
	static nuColor Make( uint32 _u )									{ nuColor c; c.u = _u; return c; }

	union {
		struct {
#if ENDIANLITTLE
			uint8 b, g, r, a;
#else
			uint8 a: 8;
			uint8 r: 8;
			uint8 g: 8;
			uint8 b: 8;
#endif
		};
		uint32 u;
	};
};

enum nuDisplayType
{
	nuDisplayBlock,
	nuDisplayInline
};

enum nuPositionType
{
	// All of these definitions are the same as HTML's
	nuPositionStatic,		// Default, regular position
	nuPositionAbsolute,		// Absolute, relative to previous explicitly parent that was anything other than "Static". Does not affect flow of subsequent siblings.
	nuPositionRelative,		// Like Absolute, but does affect flow. Flow is the "ghost" of where we would have been, before being moved relatively.
	nuPositionFixed,		// Fixed, according to root device coordinate system. In other words, completely independent of DOM hierarchy.
};

// The order of the box components (left,top,right,bottom) must be consistent with the order in nuStyleBox
#define NU_STYLE_DEFINE \
XX(NULL, 0) \
XY(Color) \
XY(Display) \
XY(Background) \
XY(BackgroundImage) \
XY(Dummy1_UseMe) \
XY(Dummy2_UseMe) \
XY(Dummy3_UseMe) \
XY(Margin_Left) \
XY(Margin_Top) \
XY(Margin_Right) \
XY(Margin_Bottom) \
XY(Padding_Left) \
XY(Padding_Top) \
XY(Padding_Right) \
XY(Padding_Bottom) \
XY(Border_Left) \
XY(Border_Top) \
XY(Border_Right) \
XY(Border_Bottom) \
XY(Width) \
XY(Height) \
XY(Top) \
XY(Left) \
XY(Right) \
XY(Bottom) \
XY(FontSize) \
XY(FontFamily) \
XY(BorderRadius) \
XY(Position) \
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
const int						nuNumInheritedStyleCategories = 1;
extern const nuStyleCategories	nuInheritedStyleCategories[nuNumInheritedStyleCategories];

/* Single style attribute (such as Margin-Left, Width, FontSize, etc).
This must be zero-initializable (ie with memset(0)).
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

	void SetColor( nuStyleCategories cat, nuColor val );
	void SetSize( nuStyleCategories cat, nuSize val );
	void SetDisplay( nuDisplayType val );
	void SetBorderRadius( nuSize val );
	void SetPosition( nuPositionType val );
	void SetFont( const char* font, nuDoc* doc );
	void SetBackgroundImage( const char* image, nuDoc* doc );
	void SetInherit( nuStyleCategories cat );

	bool				IsNull() const			{ return Category == nuCatNULL; }
	bool				IsInherit() const		{ return Flags == FlagInherit; }
	nuStyleCategories	GetCategory() const		{ return (nuStyleCategories) Category; }
	nuSize				GetSize() const			{ return nuSize::Make( (nuSize::Types) SubType, ValF ); }
	nuColor				GetColor() const		{ return nuColor::Make( ValU32 ); }
	nuDisplayType		GetDisplayType() const	{ return (nuDisplayType) ValU32; }
	nuPositionType		GetPositionType() const	{ return (nuPositionType) ValU32; }
	int					GetStringID() const		{ return (int) ValU32; }
	const char*			GetBackgroundImage( nuStringTable* strings ) const;

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
	const nuStyleAttrib*	Get( nuStyleCategories cat ) const;
	void					SetBox( nuStyleCategories cat, nuStyleBox val );
	void					GetBox( nuStyleCategories cat, nuStyleBox& box ) const;
	void					Set( const nuStyleAttrib& attrib );
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

					nuStyleSet();
					~nuStyleSet();

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

// The set of style information that is used by the renderer
// This is baked in by the Layout engine.
class NUAPI nuStyleRender
{
public:
	nuColor BackgroundColor;
	int		BackgroundImageID;
	float	BorderRadius;

	nuStyleRender() { memset(this, 0, sizeof(*this)); }
};

/* Store all style classes in one table, that is owned by one document.
This allows us to reference styles by a 32-bit integer ID instead of by name.

NOTE: I wrote GarbageCollect before realizing that this function will probably NEVER be used.
Why would you want to garbage collect your styles? You define them up front, and just because
you're not using them at the moment, doesn't mean you're not going to want to use them in future.
If time passes, and this function is never used, then get rid of the code.
The garbage collection code has never been tested.

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
	void			GarbageCollect( nuDomEl* root );						// Discover unused styles and mark them as unused
	void			CloneSlowInto( nuStyleTable& c ) const;					// Does not clone NameToIndex or UnusedSlots
	void			CloneFastInto( nuStyleTable& c, nuPool* pool ) const;	// Does not clone NameToIndex or UnusedSlots

protected:
	podvec<nuString>			Names;		// Names and Styles are parallels
	podvec<nuStyle>				Styles;
	podvec<int>					UnusedSlots;
	fhashmap<nuString, int>		NameToIndex;

	void			Compact( BitMap& used, nuDomEl* root );

	static void		GarbageCollectInternalR( BitMap& used, nuDomEl* node );
	static void		CompactR( const nuStyleID* old2newID, nuDomEl* node );

};


NUAPI bool nuDisplayTypeParse( const char* s, intp len, nuDisplayType& t );
NUAPI bool nuPositionTypeParse( const char* s, intp len, nuPositionType& t );