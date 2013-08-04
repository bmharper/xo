#include "pch.h"
#include "nuDoc.h"
#include "nuStyle.h"
#include "nuCloneHelpers.h"

#define EQ(a,b) (strcmp(a,b) == 0)

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

nuSize nuSize::Parse( const char* s, intp len )
{
	// 1.23px
	// 1.23pt
	// 1.23%
	// 0
	char digits[100];
	if ( len > 30 ) { nuParseFail( "Size too big" ); return nuSize::Pixels(0); }
	nuSize x = nuSize::Pixels(0);
	intp nondig = 0;
	for ( ; nondig < len; nondig++ )
	{
		digits[nondig] = s[nondig];
		if ( !IsNumeric(s[nondig]) ) break;
	}
	digits[nondig] = 0;
	x.Val = (float) atof( digits );
	if ( nondig == len )
	{
		if ( len == 1 && s[0] == '0' )
		{
			// ok
		}
		else nuParseFail( "Invalid size: %.*s", (int) len, s );
	}
	else
	{
		if ( s[nondig] == '%' )
			x.Type = nuSize::PERCENT;
		else if ( s[nondig] == 'p' && len - nondig >= 2 )
		{
			if (		s[nondig + 1] == 'x' ) x.Type = nuSize::PX;
			else if (	s[nondig + 1] == 't' ) x.Type = nuSize::PT;
			else nuParseFail( "Invalid size: %.*s", (int) len, s );
		}
	}
	return x;
}

nuStyleBox nuStyleBox::Parse( const char* s, intp len )
{
	// 20px
	// 1px 2px 3px 4px;
	nuStyleBox b;
	nuSize a = nuSize::Parse( s, len );
	b.Left = b.Top = b.Bottom = b.Right = a;
	return b;
}

nuColor nuColor::Parse( const char* s, intp len )
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
		nuParseFail( "Invalid color %.*s", (int) len, s );
	}
	return c;
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

void nuStyleAttrib::SetColor( nuStyleCategories cat, nuColor val )
{
	SetU32( cat, val.u );
}

void nuStyleAttrib::SetSize( nuStyleCategories cat, nuSize val )
{
	SetWithSubtypeF( cat, val.Type, val.Val );
}

void nuStyleAttrib::SetDisplay( nuDisplayType val )
{
	SetU32( nuCatDisplay, val );
}

void nuStyleAttrib::SetBorderRadius( nuSize val )
{
	SetSize( nuCatBorderRadius, val );
}

void nuStyleAttrib::SetPosition( nuPositionType val )
{
	SetU32( nuCatPosition, val );
}

void nuStyleAttrib::SetFont( const char* font, nuDoc* doc )
{
	SetString( nuCatFontFamily, font, doc );
}

void nuStyleAttrib::SetBackgroundImage( const char* image, nuDoc* doc )
{
	SetString( nuCatBackgroundImage, image, doc );
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>	void nuStyleAttrib_SetT( nuStyleAttrib& attrib, const T& v )					{}
template<>				void nuStyleAttrib_SetT( nuStyleAttrib& attrib, const nuDisplayType& v )		{ attrib.SetDisplay(v); }
template<>				void nuStyleAttrib_SetT( nuStyleAttrib& attrib, const nuPositionType& v )		{ attrib.SetPosition(v); }

template<typename T>
//static bool Parse1( const char* s, intp len, bool (*parseFunc)(const char* s, intp len, T& t), nuStyleCategories cat, nuStyleAttrib& attrib, T& attribVal )
static bool Parse1( const char* s, intp len, bool (*parseFunc)(const char* s, intp len, T& t), nuStyleCategories cat, nuStyleAttrib& attrib )
{
	T val;
	if ( parseFunc( s, len, val ) )
	{
		attrib.Category = cat;
		nuStyleAttrib_SetT( attrib, val );
		return true;
	}
	else
	{
		nuParseFail( "Unknown: '%.*s'", (int) len, s );
		return false;
	}
}

bool nuStyle::Parse( const char* t )
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
		if ( t[i] == 32 ) {}
		else if ( t[i] == ':' ) eq = i;
		else if ( t[i] == ';' || (t[i] == 0 && startv != -1) )
		{
			nuStyleAttrib a;
			bool ok = true;
			bool done = false;
			if ( MATCH(t, startk, eq, "background") )			{ a.SetColor( nuCatBackground, nuColor::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "width") )			{ a.SetSize( nuCatWidth, nuSize::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "height") )			{ a.SetSize( nuCatHeight, nuSize::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "padding") )			{ SetBox( nuCatPadding_Left, nuStyleBox::Parse(TSTART, TLEN) ); done = true; }
			else if ( MATCH(t, startk, eq, "margin") )			{ SetBox( nuCatMargin_Left, nuStyleBox::Parse(TSTART, TLEN) ); done = true; }
			else if ( MATCH(t, startk, eq, "display") )			{ ok = Parse1( TSTART, TLEN, &nuDisplayTypeParse, nuCatDisplay, a ); }
			else if ( MATCH(t, startk, eq, "position") )		{ ok = Parse1( TSTART, TLEN, &nuPositionTypeParse, nuCatPosition, a ); }
			else if ( MATCH(t, startk, eq, "border-radius") )	{ a.SetSize( nuCatBorderRadius, nuSize::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "left") )			{ a.SetSize( nuCatLeft, nuSize::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "right") )			{ a.SetSize( nuCatRight, nuSize::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "top") )				{ a.SetSize( nuCatTop, nuSize::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "bottom") )			{ a.SetSize( nuCatBottom, nuSize::Parse(TSTART, TLEN) ); }
			else
			{
				ok = false;
				nuParseFail( "Unknown: '%.*s'", int(eq - startk), t + startk );
			}
			if ( done ) {}
			else if ( ok )
				Set(a);
			else
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

		if ( t[i] == 0 ) break;
	}
	return nerror == 0;
#undef TSTART
#undef TLEN
}

void nuStyle::Compute( const nuDoc& doc, const nuDomEl& node )
{
	NUASSERT( Attribs.size() == 0 );

	nuStyle** defaults = nuDefaultTagStyles();

	const nuStyle* styles[] = {
		defaults[node.GetTag()],
		// TODO: Classes
		&node.GetStyle(),
	};
	MergeInZeroCopy( arraysize(styles), styles );
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
	Lookup = NULL;
	Attribs = NULL;
	Count = 0;
	BitsPerSlot = 0;
	Capacity = 0;
	GetSlotF = NULL;
	SetSlotF = NULL;
}

nuStyleSet::~nuStyleSet()
{
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
	int32 slot = GetSlot( attrib.GetCategory() );
	if ( slot != 0 )
	{
		Attribs[slot] = attrib;
		return;
	}
	if ( Count >= Capacity )
		Grow( pool );
	Attribs[Count] = attrib;
	SetSlot( attrib.GetCategory(), Count + SlotOffset );
	Count++;
}

nuStyleAttrib nuStyleSet::Get( nuStyleCategories cat ) const
{
	int32 slot = GetSlot( cat ) - SlotOffset;
	if ( slot == -1 ) return nuStyleAttrib();
	return Attribs[slot];
}

bool nuStyleSet::Contains( nuStyleCategories cat ) const
{
	return GetSlot( cat ) != 0;
}

void nuStyleSet::Reset()
{
	Lookup = NULL;
	Attribs = NULL;
	Capacity = 0;
	BitsPerSlot = 0;
	Count = 0;
	SetSlotF = NULL;
	GetSlotF = NULL;
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
	NUASSERT( UnusedSlots.size() == 0 && NameToIndex.size() == 0 );
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
	int index;
	if ( UnusedSlots.size() != 0 )
		index = UnusedSlots.rpop();
	else
	{
		index = (int) Styles.size();
		Styles.add();
		Names.add();
	}
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

void nuStyleTable::GarbageCollect( nuDomEl* root )
{
	BitMap used;
	used.Resize( (uint32) Styles.size(), false );
	UnusedSlots.clear();
	GarbageCollectInternalR( used, root );
	for ( int i = 0; i < used.Size(); i++ )
	{
		if ( !used.Get( i ) )
		{
			NameToIndex.erase( Names[i] );
			Names[i] = "";
			Styles[i].Attribs.clear();
			UnusedSlots += i;
		}
	}

	// Compact only if we are wasting more space than we're using
	if ( UnusedSlots.size() > Styles.size() )
	{
		Compact( used, root );
	}
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

void nuStyleTable::Compact( BitMap& used, nuDomEl* root )
{
	podvec<nuStyleID> old2newID;
	old2newID.resize( Styles.size() );
	uint32 newID = 0;
	for ( intp i = 0; i < Styles.size(); i++ )
	{
		if ( used.Get((uint32) i) )
		{
			old2newID[i] = nuStyleID( newID );
			newID++;
		}
		else
			old2newID[i] = nuStyleID( -1 );
	}
	CompactR( &old2newID[0], root );
		
	// We'll have to see how painful these memory bumps are.
	// The 'right' thing to do is to 'move' the objects from the old list into a new list.
	for ( intp i = Styles.size() - 1; i >= 0; i-- )
	{
		if ( !used.Get((uint32) i) )
		{
			Styles.erase(i);
			Names.erase(i);
		}
	}

	NameToIndex.clear();
	for ( intp i = 0; i < Styles.size(); i++ )
		NameToIndex.insert( Names[i], (int) i );

	UnusedSlots.clear();
}

void nuStyleTable::GarbageCollectInternalR( BitMap& used, nuDomEl* node )
{
	for ( intp i = 0; i < node->GetClasses().size(); i++ )
		used.Set( node->GetClasses()[i], true );
	
	for ( intp i = 0; i < node->ChildCount(); i++ )
		GarbageCollectInternalR( used, node->ChildByIndex(i) );
}

void nuStyleTable::CompactR( const nuStyleID* old2newID, nuDomEl* node )
{
	for ( intp i = 0; i < node->GetClasses().size(); i++ )
	{
		nuStyleID newval = old2newID[node->GetClasses()[i]];
		node->GetClassesMutable()[i] = newval;
		NUASSERT( newval != -1 );
	}

	for ( intp i = 0; i < node->ChildCount(); i++ )
		CompactR( old2newID, node->ChildByIndex(i) );
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
