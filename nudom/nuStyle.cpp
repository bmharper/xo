#include "pch.h"
#include "nuDoc.h"
#include "nuStyle.h"
#include "nuCloneHelpers.h"
#include "Text/nuFontStore.h"

#define EQ(a,b) (strcmp(a,b) == 0)

// Styles that are inherited by default
const nuStyleCategories nuInheritedStyleCategories[nuNumInheritedStyleCategories] = {
	nuCatFontFamily,
	nuCatFontSize,
	nuCatColor,
	nuCatText_Align_Vertical,
};

static bool MATCH( const char* s, intp start, intp end, const char* truth )
{
	for ( ; start != end; truth++, start++ )
	{
		if ( s[start] != *truth ) return false;
	}
	return true;
}

static uint8 ParseHexChar( char ch )
{
	if ( ch >= 'A' && ch <= 'F' )	return 10 + ch - 'A';
	if ( ch >= 'a' && ch <= 'f' )	return 10 + ch - 'a';
	else							return ch - '0';
}

static uint8 ParseHexCharPair( const char* ch )
{
	return (ParseHexChar( ch[0] ) << 4) | ParseHexChar( ch[1] );
}

static uint8 ParseHexCharSingle( const char* ch )
{
	uint8 v = ParseHexChar( ch[0] );
	return (v << 4) | v;
}

inline bool IsNumeric( char c )
{
	return (c >= '0' && c <= '9') || (c == '.') || (c == '-');
}

// This is parsing whitespace, not DOM/textual whitespace
// In other words, it is the space between the comma and verdana in "font-family: verdana, arial",
inline bool IsWhitespace( char c )
{
	return c == 32 || c == 9;
}

bool nuSize::Parse( const char* s, intp len, nuSize& v )
{
	// 1.23px
	// 1.23ep
	// 1.23pt
	// 1.23%
	// 0
	char digits[100];
	if ( len > 30 )
	{
		nuParseFail( "Parse failed, size too big (>30 characters)\n" );
		return false;
	}
	nuSize x = nuSize::Pixels(0);
	intp nondig = 0;
	for ( ; nondig < len; nondig++ )
	{
		digits[nondig] = s[nondig];
		if ( !IsNumeric(s[nondig]) )
			break;
	}
	digits[nondig] = 0;
	x.Val = (float) atof( digits );
	if ( nondig == len )
	{
		if ( len == 1 && s[0] == '0' )
		{
			// ok
		}
		else
		{
			nuParseFail( "Parse failed, invalid size: %.*s\n", (int) len, s );
			return false;
		}
	}
	else
	{
		if ( s[nondig] == '%' )
		{
			x.Type = nuSize::PERCENT;
		}
		else if ( s[nondig] == 'p' && len - nondig >= 2 )
		{
			if (		s[nondig + 1] == 'x' ) x.Type = nuSize::PX;
			else if (	s[nondig + 1] == 't' ) x.Type = nuSize::PT;
			else
			{
				nuParseFail( "Parse failed, invalid size: %.*s\n", (int) len, s );
				return false;
			}
		}
		else if ( s[nondig] == 'e' && len - nondig >= 2 && s[nondig + 1] == 'p' )
		{
			x.Type = nuSize::EP;
		}
	}
	v = x;
	return true;
}

bool nuStyleBox::Parse( const char* s, intp len, nuStyleBox& v )
{
	int nspaces = 0;
	int spaces[10];
	for ( intp i = 0; i < len && nspaces < arraysize(spaces) - 1; i++ )
	{
		if ( IsWhitespace(s[i]) )
			spaces[nspaces++] = (int) i;
	}

	// 20px
	// 1px 2px 3px 4px (TODO)
	nuStyleBox b;
	if ( nspaces == 0 )
	{
		nuSize one;
		if ( nuSize::Parse( s, len, one ) )
		{
			b.Left = b.Top = b.Bottom = b.Right = one;
			v = b;
			return true;
		}
	}

	// 1px 2px 3px 4px
	if ( nspaces == 3 )
	{
		bool ok1 = nuSize::Parse( s, spaces[0], b.Left );
		bool ok2 = nuSize::Parse( s + spaces[0] + 1, spaces[1] - spaces[0] - 1, b.Top );
		bool ok3 = nuSize::Parse( s + spaces[1] + 1, spaces[2] - spaces[1] - 1, b.Right );
		bool ok4 = nuSize::Parse( s + spaces[2] + 1, (int) len - spaces[2] - 1, b.Bottom );
		if ( ok1 && ok2 && ok3 && ok4 )
		{
			v = b;
			return true;
		}
	}
	return false;
}

bool nuColor::Parse( const char* s, intp len, nuColor& v )
{
	nuColor c = nuColor::RGBA(0,0,0,0);
	s++;
	// #rgb
	// #rgba
	// #rrggbb
	// #rrggbbaa
	if ( len == 4 )
	{
		c.r = ParseHexCharSingle( s + 0 );
		c.g = ParseHexCharSingle( s + 1 );
		c.b = ParseHexCharSingle( s + 2 );
		c.a = 255;
		//NUTRACE( "color %s -> %d\n", s, (int) c.r );
	}
	else if ( len == 5 )
	{
		c.r = ParseHexCharSingle( s + 0 );
		c.g = ParseHexCharSingle( s + 1 );
		c.b = ParseHexCharSingle( s + 2 );
		c.a = ParseHexCharSingle( s + 3 );
	}
	else if ( len == 7 )
	{
		c.r = ParseHexCharPair( s + 0 );
		c.g = ParseHexCharPair( s + 2 );
		c.b = ParseHexCharPair( s + 4 );
		c.a = 255;
	}
	else if ( len == 9 )
	{
		c.r = ParseHexCharPair( s + 0 );
		c.g = ParseHexCharPair( s + 2 );
		c.b = ParseHexCharPair( s + 4 );
		c.a = ParseHexCharPair( s + 6 );
	}
	else
	{
		nuParseFail( "Parse failed, invalid color %.*s\n", (int) len, s );
		return false;
	}
	v = c;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nuStyleAttrib::nuStyleAttrib()
{
	memset( this, 0, sizeof(*this) );
}

void nuStyleAttrib::SetU32( nuStyleCategories cat, uint32 val )
{
	Category = cat;
	ValU32 = val;
}

void nuStyleAttrib::SetWithSubtypeU32( nuStyleCategories cat, uint8 subtype, uint32 val )
{
	Category = cat;
	SubType = subtype;
	ValU32 = val;
}

void nuStyleAttrib::SetWithSubtypeF( nuStyleCategories cat, uint8 subtype, float val )
{
	Category = cat;
	SubType = subtype;
	ValF = val;
}

void nuStyleAttrib::SetString( nuStyleCategories cat, const char* str, nuDoc* doc )
{
	Category = cat;
	ValU32 = doc->Strings.GetId( str );
}

void nuStyleAttrib::SetInherit( nuStyleCategories cat )
{
	Category = cat;
	Flags = FlagInherit;
}

const char* nuStyleAttrib::GetBackgroundImage( nuStringTable* strings ) const
{
	return strings->GetStr( ValU32 );
}

nuFontID nuStyleAttrib::GetFont() const
{
	return ValU32;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
static bool ParseSingleAttrib( const char* s, intp len, bool (*parseFunc)(const char* s, intp len, T& t), nuStyleCategories cat, nuStyle& style )
{
	T val;
	if ( parseFunc( s, len, val ) )
	{
		nuStyleAttrib attrib;
		attrib.Set( cat, val );
		style.Set( attrib );
		return true;
	}
	else
	{
		nuParseFail( "Parse failed, unknown value: '%.*s'\n", (int) len, s );
		return false;
	}
}

// This was added when font-family was stored as a string, but it is now stored as a nuFontID
static void ParseString( const char* s, intp len, nuStyleCategories cat, nuDoc* doc, nuStyle& style )
{
	char stat[64];
	nuStyleAttrib attrib;
	if ( len < sizeof(stat) )
	{
		memcpy( stat, s, len );
		stat[len] = 0;
		attrib.Set( cat, stat, doc );
	}
	else
	{
		nuString copy;
		copy.Set( s, len );
		attrib.Set( cat, copy.Z, doc );
	}
	style.Set( attrib );
}

template<typename T>
static bool ParseCompound( const char* s, intp len, bool (*parseFunc)(const char* s, intp len, T& t), nuStyleCategories cat, nuStyle& style )
{
	T val;
	if ( parseFunc( s, len, val ) )
	{
		style.Set( cat, val );
		return true;
	}
	else
	{
		nuParseFail( "Parse failed, unknown value: '%.*s'\n", (int) len, s );
		return false;
	}
}

static bool ParseFontFamily( const char* s, intp len, nuFontID& v )
{
	bool onFont = false;
	char buf[64];
	intp bufPos = 0;
	for ( intp i = 0; true; i++ )
	{
		if ( onFont )
		{
			if ( s[i] == ',' || i == len )
			{
				buf[bufPos] = 0;
				v = nuGlobal()->FontStore->InsertByFacename( buf );
				if ( v != nuFontIDNull )
					return true;
				onFont = false;
				bufPos = 0;
			}
			else
			{
				buf[bufPos++] = s[i];
			}
			if ( i == len )
				break;
		}
		else
		{
			if ( i == len )
				break;
			if ( IsWhitespace(s[i]) )
				continue;
			onFont = true;
			buf[bufPos++] = s[i];
		}
		if ( bufPos >= arraysize(buf) )
		{
			nuParseFail( "Parse failed, font name too long (>63): '%*.s'\n", (int) len, s );
			return false;
		}
	}
	// not sure whether we should do this. One might want no font to be set instead.
	v = nuGlobal()->FontStore->GetFallbackFontID();
	return true;
}

bool nuStyle::Parse( const char* t, nuDoc* doc )
{
	return Parse( t, INT32MAX, doc );
}

bool nuStyle::Parse( const char* t, intp maxLen, nuDoc* doc )
{
	// "background: #8f8; width: 100%; height: 100%;"
#define TSTART	(t + startv)
#define TLEN	(i - startv)
	intp startk = -1;
	intp eq = -1;
	intp startv = -1;
	int nerror = 0;
	for ( intp i = 0; true; i++ )
	{
		bool eof = t[i] == 0 || i == maxLen;
		if ( t[i] == 32 ) {}
		else if ( t[i] == ':' ) eq = i;
		else if ( t[i] == ';' || (eof && startv != -1) )
		{
			bool ok = true;
			if ( MATCH(t, startk, eq, "background") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuColor::Parse, nuCatBackground, *this ); }
			else if ( MATCH(t, startk, eq, "color") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuColor::Parse, nuCatColor, *this ); }
			else if ( MATCH(t, startk, eq, "width") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuSize::Parse, nuCatWidth, *this ); }
			else if ( MATCH(t, startk, eq, "height") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuSize::Parse, nuCatHeight, *this ); }
			else if ( MATCH(t, startk, eq, "padding") )						{ ok = ParseCompound( TSTART, TLEN, &nuStyleBox::Parse, nuCatPadding_Left, *this ); }
			else if ( MATCH(t, startk, eq, "margin") )						{ ok = ParseCompound( TSTART, TLEN, &nuStyleBox::Parse, nuCatMargin_Left, *this ); }
			else if ( MATCH(t, startk, eq, "display") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuDisplayTypeParse, nuCatDisplay, *this ); }
			else if ( MATCH(t, startk, eq, "position") )					{ ok = ParseSingleAttrib( TSTART, TLEN, &nuPositionTypeParse, nuCatPosition, *this ); }
			else if ( MATCH(t, startk, eq, "border-radius") )				{ ok = ParseSingleAttrib( TSTART, TLEN, &nuSize::Parse, nuCatBorderRadius, *this ); }
			//else if ( MATCH(t, startk, eq, "left") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuSize::Parse, nuCatLeft, *this ); }
			//else if ( MATCH(t, startk, eq, "right") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuSize::Parse, nuCatRight, *this ); }
			//else if ( MATCH(t, startk, eq, "top") )							{ ok = ParseSingleAttrib( TSTART, TLEN, &nuSize::Parse, nuCatTop, *this ); }
			//else if ( MATCH(t, startk, eq, "bottom") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuSize::Parse, nuCatBottom, *this ); }
			else if ( MATCH(t, startk, eq, "break") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuBreakTypeParse, nuCatBreak, *this ); }
			else if ( MATCH(t, startk, eq, "flow-axis") )					{ ok = ParseSingleAttrib( TSTART, TLEN, &nuFlowAxisParse, nuCatFlow_Axis, *this ); }
			else if ( MATCH(t, startk, eq, "flow-direction-horizontal") )	{ ok = ParseSingleAttrib( TSTART, TLEN, &nuFlowDirectionParse, nuCatFlow_Direction_Horizontal, *this ); }
			else if ( MATCH(t, startk, eq, "flow-direction-vertical") )		{ ok = ParseSingleAttrib( TSTART, TLEN, &nuFlowDirectionParse, nuCatFlow_Direction_Vertical, *this ); }
			else if ( MATCH(t, startk, eq, "box-sizing") )					{ ok = ParseSingleAttrib( TSTART, TLEN, &nuBoxSizeParse, nuCatBoxSizing, *this ); }
			else if ( MATCH(t, startk, eq, "font-size") )					{ ok = ParseSingleAttrib( TSTART, TLEN, &nuSize::Parse, nuCatFontSize, *this ); }
			else if ( MATCH(t, startk, eq, "font-family") )					{ ok = ParseSingleAttrib( TSTART, TLEN, &ParseFontFamily, nuCatFontFamily, *this ); }
			else if ( MATCH(t, startk, eq, "text-align-vertical") )			{ ok = ParseSingleAttrib( TSTART, TLEN, &nuTextAlignVerticalParse, nuCatText_Align_Vertical, *this ); }
			else if ( MATCH(t, startk, eq, "left") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuHorizontalBindingParse, nuCatLeft, *this ); }
			else if ( MATCH(t, startk, eq, "hcenter") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuHorizontalBindingParse, nuCatHCenter, *this ); }
			else if ( MATCH(t, startk, eq, "right") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuHorizontalBindingParse, nuCatRight, *this ); }
			else if ( MATCH(t, startk, eq, "top") )							{ ok = ParseSingleAttrib( TSTART, TLEN, &nuVerticalBindingParse, nuCatTop, *this ); }
			else if ( MATCH(t, startk, eq, "vcenter") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuVerticalBindingParse, nuCatVCenter, *this ); }
			else if ( MATCH(t, startk, eq, "bottom") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &nuVerticalBindingParse, nuCatBottom, *this ); }
			else if ( MATCH(t, startk, eq, "baseline") )					{ ok = ParseSingleAttrib( TSTART, TLEN, &nuVerticalBindingParse, nuCatBaseline, *this ); }
			else
			{
				ok = false;
				nuParseFail( "Parse failed - unknown property: '%.*s'\n", int(eq - startk), t + startk );
			}
			if ( !ok )
				nerror++;
			eq = -1;
			startk = -1;
			startv = -1;
		}
		else
		{
			if ( startk == -1 )						startk = i;
			else if ( startv == -1 && eq != -1 )	startv = i;
		}

		if ( eof ) break;
	}
	return nerror == 0;
#undef TSTART
#undef TLEN
}

const nuStyleAttrib* nuStyle::Get( nuStyleCategories cat ) const
{
	for ( intp i = 0; i < Attribs.size(); i++ )
	{
		if ( Attribs[i].Category == cat ) return &Attribs[i];
	}
	return NULL;
}

void nuStyle::GetBox( nuStyleCategories cat, nuStyleBox& box ) const
{
	nuStyleCategories base = nuCatMakeBaseBox(cat);
	for ( intp i = 0; i < Attribs.size(); i++ )
	{
		uint pindex = uint(Attribs[i].Category - base);
		if ( pindex < 4 )
			box.All[pindex] = Attribs[i].GetSize();
	}
}

void nuStyle::SetBox( nuStyleCategories cat, nuStyleBox val )
{
	if ( cat >= nuCatMargin_Left && cat <= nuCatBorder_Bottom )
	{
		SetBoxInternal( nuCatMakeBaseBox(cat), val );
	}
	else NUASSERT(false);
}

void nuStyle::SetBoxInternal( nuStyleCategories catBase, nuStyleBox val )
{
	nuStyleAttrib a;
	a.SetSize( (nuStyleCategories) (catBase + 0), val.Left );	Set( a );
	a.SetSize( (nuStyleCategories) (catBase + 1), val.Top );	Set( a );
	a.SetSize( (nuStyleCategories) (catBase + 2), val.Right );	Set( a );
	a.SetSize( (nuStyleCategories) (catBase + 3), val.Bottom );	Set( a );
}

void nuStyle::Set( const nuStyleAttrib& attrib )
{
	for ( intp i = 0; i < Attribs.size(); i++ )
	{
		if ( Attribs[i].Category == attrib.Category )
		{
			Attribs[i] = attrib;
			return;
		}
	}
	Attribs += attrib;
}

void nuStyle::Set( nuStyleCategories cat, nuStyleBox val )
{
	SetBox( cat, val );
}

/*
void nuStyle::MergeInZeroCopy( int n, const nuStyle** src )
{
	if ( n == 0 ) return;

	// Use a lookup table to avoid making this merge operation O(n*m).
	// If the list of attributes balloons, then we should probably use something like a two level tree here.
	// This table is:
	// Category > Attrib Index
	u8 lut[nuCatEND];
	static const u8 EMPTY = 255;
	memset( lut, EMPTY, sizeof(lut) );
	static_assert( nuCatEND < EMPTY, "nuCat__ fits in a byte" );

	int isrc = 0;
	if ( Attribs.size() == 0 )
	{
		// optimization for the case where we're initially empty
		auto& s = src[0]->Attribs;
		for ( intp i = 0; i < s.size(); i++ )
		{
			lut[s[i].Category] = (u8) i;
			Attribs += s[i];
		}
		isrc = 1;
	}

	// Apply each nuStyle in turn. Later nuStyles override earlier ones.
	for ( ; isrc < n; isrc++ )
	{
		auto& s = src[isrc]->Attribs;

		for ( intp i = 0; i < s.size(); i++ )
		{
			u8 existing = lut[s[i].Category];
			if ( existing == EMPTY )
			{
				// new category
				lut[s[i].Category] = (u8) Attribs.size();
				Attribs += s[i];
			}
			else
			{
				// overwrite existing category
				Attribs[existing] = s[i];
			}
		}
	}
}
*/

void nuStyle::Discard()
{
	Attribs.clear_noalloc();
	//Name.Discard();
}

void nuStyle::CloneSlowInto( nuStyle& c ) const
{
	c.Attribs = Attribs;
}

void nuStyle::CloneFastInto( nuStyle& c, nuPool* pool ) const
{
	//Name.CloneFastInto( c.Name, pool );
	nuClonePodvecWithMemCopy( c.Attribs, Attribs, pool );
}

#define XX(name, type, setfunc, cat) \
void nuStyle::Set##name( type value ) \
{ \
	nuStyleAttrib a; \
	a.setfunc( cat, value ); \
	Set( a ); \
}
NUSTYLE_SETTERS_2P
#undef XX

#define XX(name, type, setfunc) \
void nuStyle::Set##name( type value ) \
{ \
	nuStyleAttrib a; \
	a.setfunc( value ); \
	Set( a ); \
}
NUSTYLE_SETTERS_1P
#undef XX

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nuStyleSet::nuStyleSet()
{
	Reset();
}

nuStyleSet::~nuStyleSet()
{
}

void nuStyleSet::Reset()
{
	Lookup = NULL;
	Attribs = NULL;
	Count = 0;
	Capacity = 0;
	BitsPerSlot = 0;
	SetSlotF = NULL;
	GetSlotF = NULL;
}

void nuStyleSet::Grow( nuPool* pool )
{
	NUASSERT( BitsPerSlot != 8 );
	uint32 newbits = BitsPerSlot == 0 ? InitialBitsPerSlot : BitsPerSlot * 2;
	uint32 totalBits = newbits * nuCatEND;
	void*			newlookup = pool->Alloc( (totalBits + 7) / 8, true );
	nuStyleAttrib*	newattribs = pool->AllocNT<nuStyleAttrib>( CapacityAt(newbits), false ); 
	if ( BitsPerSlot )
	{
		if ( newbits == 4 )			MigrateLookup( Lookup, newlookup, &GetSlot2, &SetSlot4 );
		else if ( newbits == 8 )	MigrateLookup( Lookup, newlookup, &GetSlot4, &SetSlot8 );

		memcpy( newattribs, Attribs, CapacityAt(BitsPerSlot) * sizeof(nuStyleAttrib) );
	}
	Capacity = CapacityAt(newbits);
	Lookup = newlookup;
	Attribs = newattribs;
	BitsPerSlot = newbits;
	if ( newbits == 2 )			{ SetSlotF = &SetSlot2; GetSlotF = &GetSlot2; }
	else if ( newbits == 4 )	{ SetSlotF = &SetSlot4; GetSlotF = &GetSlot4; }
	else if ( newbits == 8 )	{ SetSlotF = &SetSlot8; GetSlotF = &GetSlot8; }
}

void nuStyleSet::Set( int n, const nuStyleAttrib* attribs, nuPool* pool )
{
	for ( int i = 0; i < n; i++ )
		Set( attribs[i], pool );
}

void nuStyleSet::Set( const nuStyleAttrib& attrib, nuPool* pool )
{
	if ( attrib.Category == nuCatFontSize )
		int abc = 123;
	int32 slot = GetSlot( attrib.GetCategory() );
	if ( slot != 0 )
	{
		Attribs[slot - 1] = attrib;
		return;
	}
	if ( Count >= Capacity )
		Grow( pool );
	Attribs[Count] = attrib;
	SetSlot( attrib.GetCategory(), Count + SlotOffset );
	Count++;
	//DebugCheckSanity();
}

nuStyleAttrib nuStyleSet::Get( nuStyleCategories cat ) const
{
	int32 slot = GetSlot( cat ) - SlotOffset;
	if ( slot == -1 )
		return nuStyleAttrib();
	return Attribs[slot];
}

void nuStyleSet::DebugCheckSanity() const
{
	for ( int i = nuCatFIRST; i < nuCatEND; i++ )
	{
		nuStyleCategories cat = (nuStyleCategories) i;
		nuStyleAttrib val = Get( cat );
		NUASSERTDEBUG( val.IsNull() || val.Category == cat );
	}
}

bool nuStyleSet::Contains( nuStyleCategories cat ) const
{
	return GetSlot( cat ) != 0;
}

void nuStyleSet::MigrateLookup( const void* lutsrc, void* lutdst, GetSlotFunc getter, SetSlotFunc setter )
{
	for ( int i = nuCatFIRST; i < nuCatEND; i++ )
	{
		int32 slot = getter( lutsrc, (nuStyleCategories) i );
		if ( slot != 0 )
			setter( lutdst, (nuStyleCategories) i, slot );
	}
}

int32 nuStyleSet::GetSlot( nuStyleCategories cat ) const
{
	if ( !GetSlotF ) return 0;
	return GetSlotF( Lookup, cat );
}

void nuStyleSet::SetSlot( nuStyleCategories cat, int32 slot )
{
	SetSlotF( Lookup, cat, slot );
}

template<uint32 BITS_PER_SLOT>
void nuStyleSet::TSetSlot( void* lookup, nuStyleCategories cat, int32 slot )
{
	const uint32	mask	= (1 << BITS_PER_SLOT) - 1;
	uint8*			lookup8	= (uint8*) lookup;

	if ( BITS_PER_SLOT == 8 )
	{
		lookup8[cat] = slot;
	}
	else
	{
		uint32	intra_byte_mask;
		uint32	ibyte;
		if ( BITS_PER_SLOT == 2 )		{ ibyte = ((uint32) cat) >> 2;	intra_byte_mask = 3; }
		else if ( BITS_PER_SLOT == 4 )	{ ibyte = ((uint32) cat) >> 1;	intra_byte_mask = 1; }

		uint8	v = lookup8[ibyte];
		uint8	islotinbyte = ((uint32) cat) & intra_byte_mask;
		uint8	ishift = islotinbyte * BITS_PER_SLOT;
		uint8	shiftedmask = mask << ishift;
		v = v & ~shiftedmask;
		v = v | (((uint32) slot) << ishift);
		lookup8[ibyte] = v;
	}
}

template<uint32 BITS_PER_SLOT>
int32 nuStyleSet::TGetSlot( const void* lookup, nuStyleCategories cat )
{
	const uint32 mask		= (1 << BITS_PER_SLOT) - 1;
	const uint8* lookup8	= (const uint8*) lookup;

	if ( BITS_PER_SLOT == 8 )
	{
		return lookup8[cat];
	}
	else
	{
		uint32	intra_byte_mask;
		uint32	ibyte;
		if ( BITS_PER_SLOT == 2 )		{ ibyte = ((uint32) cat) >> 2;	intra_byte_mask = 3; }
		else if ( BITS_PER_SLOT == 4 )	{ ibyte = ((uint32) cat) >> 1;	intra_byte_mask = 1; }

		uint8	v = lookup8[ibyte];
		uint8	islotinbyte = ((uint32) cat) & intra_byte_mask;
		uint8	ishift = islotinbyte * BITS_PER_SLOT;
		uint8	shiftedmask = mask << ishift;
		v = (v & shiftedmask) >> ishift;
		return v;
	}
}

int32 nuStyleSet::GetSlot2( const void* lookup, nuStyleCategories cat ) { return TGetSlot<2>( lookup, cat ); }
int32 nuStyleSet::GetSlot4( const void* lookup, nuStyleCategories cat ) { return TGetSlot<4>( lookup, cat ); }
int32 nuStyleSet::GetSlot8( const void* lookup, nuStyleCategories cat ) { return TGetSlot<8>( lookup, cat ); }

void nuStyleSet::SetSlot2( void* lookup, nuStyleCategories cat, int32 slot ) { TSetSlot<2>( lookup, cat, slot ); }
void nuStyleSet::SetSlot4( void* lookup, nuStyleCategories cat, int32 slot ) { TSetSlot<4>( lookup, cat, slot ); }
void nuStyleSet::SetSlot8( void* lookup, nuStyleCategories cat, int32 slot ) { TSetSlot<8>( lookup, cat, slot ); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nuStyleTable::nuStyleTable()
{
}

nuStyleTable::~nuStyleTable()
{
}

void nuStyleTable::Discard()
{
	Styles.hack( 0, 0, NULL );
	Names.hack( 0, 0, NULL );
}

const nuStyle* nuStyleTable::GetByID( nuStyleID id ) const
{
	return &Styles[id];
}

nuStyle* nuStyleTable::GetOrCreate( const char* name )
{
	nuTempString n(name);
	// find existing
	int* pindex = NameToIndex.getp( n );
	if ( pindex ) return &Styles[*pindex];
	
	// create new
	int index = (int) Styles.size();
	Styles.add();
	Names.add();
	nuStyle* s = &Styles[index];
	NameToIndex.insert( n, index );
	Names[index] = nuString( name );
	return s;
}

nuStyleID nuStyleTable::GetStyleID( const char* name )
{
	nuTempString n(name);
	int* pindex = NameToIndex.getp( n );
	if ( pindex )	return nuStyleID( *pindex );
	else			return nuStyleID(0);
}

void nuStyleTable::CloneSlowInto( nuStyleTable& c ) const
{
	// The renderer doesn't need a Name -> ID table. That lookup table is only for end-user convenience.
	c.Styles = Styles;
}

void nuStyleTable::CloneFastInto( nuStyleTable& c, nuPool* pool ) const
{
	// The renderer doesn't need a Name -> ID table. That lookup table is only for end-user convenience.
	nuClonePodvecWithMemCopy( c.Styles, Styles, pool );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NUAPI bool nuDisplayTypeParse( const char* s, intp len, nuDisplayType& t )
{
	if ( MATCH(s, 0, len, "block") ) { t = nuDisplayBlock; return true; }
	if ( MATCH(s, 0, len, "inline") ) { t = nuDisplayInline; return true; }
	return false;
}

NUAPI bool nuPositionTypeParse( const char* s, intp len, nuPositionType& t )
{
	if ( MATCH(s, 0, len, "static") )	{ t = nuPositionStatic; return true; }
	if ( MATCH(s, 0, len, "absolute") ) { t = nuPositionAbsolute; return true; }
	if ( MATCH(s, 0, len, "relative") ) { t = nuPositionRelative; return true; }
	if ( MATCH(s, 0, len, "fixed") )	{ t = nuPositionFixed; return true; }
	return false;
}

NUAPI bool nuBreakTypeParse( const char* s, intp len, nuBreakType& t )
{
	if ( MATCH(s, 0, len, "none") )		{ t = nuBreakNULL; return true; }
	if ( MATCH(s, 0, len, "before") )	{ t = nuBreakBefore; return true; }
	if ( MATCH(s, 0, len, "after") )	{ t = nuBreakAfter; return true; }
	return false;
}

NUAPI bool nuFlowAxisParse( const char* s, intp len, nuFlowAxis& t )
{
	if ( MATCH(s, 0, len, "horizontal") )	{ t = nuFlowAxisHorizontal; return true; }
	if ( MATCH(s, 0, len, "vertical") )		{ t = nuFlowAxisVertical; return true; }
	return false;
}

NUAPI bool nuFlowDirectionParse( const char* s, intp len, nuFlowDirection& t )
{
	if ( MATCH(s, 0, len, "normal") )	{ t = nuFlowDirectionNormal; return true; }
	if ( MATCH(s, 0, len, "reverse") )	{ t = nuFlowDirectionReversed; return true; }
	return false;
}

NUAPI bool nuBoxSizeParse( const char* s, intp len, nuBoxSizeType& t )
{
	if ( MATCH(s, 0, len, "content") )	{ t = nuBoxSizeContent; return true; }
	if ( MATCH(s, 0, len, "border") )	{ t = nuBoxSizeBorder; return true; }
	if ( MATCH(s, 0, len, "margin") )	{ t = nuBoxSizeMargin; return true; }
	return false;
}

NUAPI bool nuTextAlignVerticalParse( const char* s, intp len, nuTextAlignVertical& t )
{
	if ( MATCH(s, 0, len, "baseline") )	{ t = nuTextAlignVerticalBaseline; return true; }
	if ( MATCH(s, 0, len, "top") )		{ t = nuTextAlignVerticalTop; return true; }
	return false;
}

NUAPI bool nuHorizontalBindingParse( const char* s, intp len, nuHorizontalBindings& t )
{
	if ( MATCH(s, 0, len, "left") )		{ t = nuHorizontalBindingLeft; return true; }
	if ( MATCH(s, 0, len, "hcenter") )	{ t = nuHorizontalBindingCenter; return true; }
	if ( MATCH(s, 0, len, "right") )	{ t = nuHorizontalBindingRight; return true; }
	return false;
}

NUAPI bool nuVerticalBindingParse( const char* s, intp len, nuVerticalBindings& t )
{
	if ( MATCH(s, 0, len, "top") )		{ t = nuVerticalBindingTop; return true; }
	if ( MATCH(s, 0, len, "vcenter") )	{ t = nuVerticalBindingCenter; return true; }
	if ( MATCH(s, 0, len, "bottom") )	{ t = nuVerticalBindingBottom; return true; }
	if ( MATCH(s, 0, len, "baseline") )	{ t = nuVerticalBindingBaseline; return true; }
	return false;
}
