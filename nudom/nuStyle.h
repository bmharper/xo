#pragma once

#include "nuDefs.h"
#include "nuString.h"

// Represents a size that is zero, pixels, points, percent. TODO: em
struct NUAPI nuSize
{
	enum	Types { NONE = 0, PX, PT, PERCENT };
	float 	Val;
	Types 	Type;

	static nuSize Make( Types t, float v )	{ nuSize s = {v, t}; return s; }
	static nuSize Percent( float v )		{ nuSize s = {v, PERCENT}; return s; }
	static nuSize Pixels( float v )			{ nuSize s = {v, PX}; return s; }
	static nuSize Parse( const char* s, intp len );
};

// Convenience struct used during layout computation
struct NUAPI nuStyleBox
{
	// This order must be consistent with the order presented inside NU_STYLE_DEFINE
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
};

struct NUAPI nuColor
{
	void	Set( uint8 _r, uint8 _g, uint8 _b, uint8 _a ) { r = _r; g = _g; b = _b; a = _a; }
	uint32	GetRGBA() const { nuRGBA x; x.r = r; x.g = g; x.b = b; x.a = a; return x.u; }

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
	nuPositionStatic,
	nuPositionAbsolute,
	nuPositionRelative,
	nuPositionFixed,
};

// The order of the boxes must be consistent with the order in nuStyleBox
#define NU_STYLE_DEFINE \
XX(NULL, 0) \
XY(Color) \
XY(Display) \
XY(Background) \
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
};
#undef XX
#undef XY

static_assert( nuCatMargin_Left == 4, "Start of boxes must be multiple of 4" );

inline nuStyleCategories nuCatMakeBaseBox( nuStyleCategories c ) { return (nuStyleCategories) (c & ~3); }

// This must be zero-initializable
class NUAPI nuStyleAttrib
{
public:
	uint8				Category;		// nuStyleCategory
	uint8				SubType;
	uint8				Unused1;
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

	nuStyleCategories	GetCategory() const		{ return (nuStyleCategories) Category; }
	nuSize				GetSize() const			{ return nuSize::Make( (nuSize::Types) SubType, ValF ); }
	nuColor				GetColor() const		{ return nuColor::Make( ValU32 ); }
	nuDisplayType		GetDisplayType() const	{ return (nuDisplayType) ValU32; }
	nuPositionType		GetPositionType() const	{ return (nuPositionType) ValU32; }

protected:
	void SetString( nuStyleCategories cat, const char* str, nuDoc* doc );
	void SetU32( nuStyleCategories cat, uint32 val );
	void SetWithSubtypeU32( nuStyleCategories cat, uint8 subtype, uint32 val );
	void SetWithSubtypeF( nuStyleCategories cat, uint8 subtype, float val );

};

// Collection of styles (border-width-left, color, etc)
class NUAPI nuStyle
{
public:
	podvec<nuStyleAttrib>	Attribs;

	bool					Parse( const char* t );
	const nuStyleAttrib*	Get( nuStyleCategories cat ) const;
	void					SetBox( nuStyleCategories cat, nuStyleBox val );
	void					GetBox( nuStyleCategories cat, nuStyleBox& box ) const;
	void					Set( const nuStyleAttrib& attrib );
	void					Compute( const nuDoc& doc, const nuDomEl& node );
	void					Discard();
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
	void					MergeInZeroCopy( int n, const nuStyle** src );
	void					SetBoxInternal( nuStyleCategories catBase, nuStyleBox val );
};

FHASH_SETUP_CLASS_CTOR_DTOR(nuStyle, nuStyle);

/* Store all styles in one table, that is owned by one document.
This allows us to reference styles by a 32-bit integer ID instead of by name.

NOTE: I wrote GarbageCollect before realizing that this function will probably NEVER be used.
Why would you want to garbage collect your styles? You define them up front, and just because
you're not using them at the moment, doesn't mean you're not going to want to use them in future.
If time passes, and this function is never used, then get rid of the code.

*/
class NUAPI nuStyleTable
{
public:
					nuStyleTable();
					~nuStyleTable();

	void			Discard();
	nuStyle*		GetOrCreate( const char* name );
	nuStyleID		GetStyleID( const char* name );
	void			GarbageCollect( nuDomEl* root );						// Discover unused styles and mark them as unused
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
