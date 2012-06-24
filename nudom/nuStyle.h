#pragma once

#include "nuDefs.h"
#include "nuString.h"

struct NUAPI nuSize
{
	enum	Types { NONE = 0, PX, PT, PERCENT };
	float 	Val;
	Types 	Type;

	static nuSize Percent( float v ) { nuSize s = {v, PERCENT}; return s; }
	static nuSize Pixels( float v ) { nuSize s = {v, PX}; return s; }
	static nuSize Parse( const char* s, intp len );
};

struct NUAPI nuStyleBox
{
	nuSize	Left, Top, Right, Bottom;

	static nuStyleBox Parse( const char* s, intp len );
	static nuStyleBox Make( nuSize left, nuSize top, nuSize right, nuSize bottom ) { nuStyleBox b; b.Left = left; b.Top = top; b.Right = right; b.Bottom = bottom; return b; }
	static nuStyleBox MakeZero() { nuStyleBox b; b.Left = b.Top = b.Right = b.Bottom = nuSize::Pixels(0); return b; }
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
	static nuColor RGBA( uint8 _r, uint8 _g, uint8 _b, uint8 _a ) { nuColor c; c.Set(_r,_g,_b,_a); return c; }

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

#define NU_STYLE_DEFINE \
XX(NULL, 0) \
XY(Width) \
XY(Height) \
XY(Top) \
XY(Left) \
XY(Right) \
XY(Bottom) \
XY(Margin) \
XY(Padding) \
XY(Border) \
XY(Background) \
XY(Color) \
XY(FontSize) \
XY(FontFamily) \
XY(Display) \
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


// This must be zero-initializable
class NUAPI nuStyleAttrib
{
public:
	nuStyleCategories Category;
	union
	{
		void*			Union;
		nuStyleBox		Box;
		nuColor			Color;
		nuSize			Size;
		nuStringRaw 	String;
		nuDisplayType	Display;
		nuPositionType	Position;
		nuSize			BorderRadius;
	};

	nuStyleAttrib();
	nuStyleAttrib( const nuStyleAttrib& b );
	~nuStyleAttrib();
	nuStyleAttrib& operator=( const nuStyleAttrib& b );

	void SetColor( nuStyleCategories cat, nuColor val );
	void SetSize( nuStyleCategories cat, nuSize val );
	void SetBox( nuStyleCategories cat, nuStyleBox val );
	void SetDisplay( nuDisplayType val );
	void SetBorderRadius( nuSize val );
	void SetPosition( nuPositionType val );

	void Discard();	// Discard heap memory (ie do not free)
	void SetZeroCopy( const nuStyleAttrib& b );
	void CloneFastInto( nuStyleAttrib& c, nuPool* pool ) const;

protected:
	void Free();
	
	template<typename T>
	void SetA( nuStyleCategories cat, T& prop, const T& value );
};

class NUAPI nuStyle
{
public:
	// NOTE: I have the intuition that 'Name' might be unnecessary, because styles are always referenced by named-lookup table. If you can get rid of it, do.
	nuString				Name;			// This should generally be considered immutable, once it's added to a document. This is certainly true for nuStyleSet.
	podvec<nuStyleAttrib>	Attribs;

	bool					Parse( const char* t );
	bool					Parsef( const char* t, ... );
	const nuStyleAttrib*	Get( nuStyleCategories cat ) const;
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
};

FHASH_SETUP_CLASS_CTOR_DTOR(nuStyle, nuStyle);

// Set of styles. This keeps a map from (name -> style), so that lookup by name is O(1).
// NOTE: Now that we have nuStyleTable, we should be able to get rid of this.
class NUAPI nuStyleSet
{
public:
					nuStyleSet();
					~nuStyleSet();
	
	// These are for 'end-user' usage. Not valid when this is a read-only clone used for rendering.
	nuStyle*		GetOrCreate( const nuString& name );
	nuStyle*		GetOrCreate( const char* name );

	// This is for the renderer
	nuStyle*		GetReadOnly( const nuString& name );

	void			CloneFastInto( nuStyleSet& c, nuPool* pool ) const;
	void			Reset();

protected:
	bool						IsReadOnly;
	podvec<nuStyle>				Styles;
	fhashmap<nuStringRaw, int>	NameToIndexReadOnly;	// storage for strings comes from the document's pool
	fhashmap<nuString, int>		NameToIndexMutable;	

	void			BuildIndexReadOnly();
};

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

	nuStyle*		GetOrCreate( const char* name );
	void			GarbageCollect( nuDomEl* root );					// Discover unused styles and mark them as unused

protected:
	podvec<nuStyle>				Styles;
	podvec<int>					UnusedSlots;
	fhashmap<nuString, int>		NameToIndex;

	void			Compact( BitMap& used, nuDomEl* root );

	static void		GarbageCollectInternalR( BitMap& used, nuDomEl* node );
	static void		CompactR( const nuStyleID* old2newID, nuDomEl* node );

};

NUAPI bool nuDisplayTypeParse( const char* s, intp len, nuDisplayType& t );
NUAPI bool nuPositionTypeParse( const char* s, intp len, nuPositionType& t );
