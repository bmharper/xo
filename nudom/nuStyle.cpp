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
	Category = nuCatWidth;
}

nuStyleAttrib::nuStyleAttrib( const nuStyleAttrib& b )
{
	Category = nuCatWidth;
	*this = b;
}

nuStyleAttrib::~nuStyleAttrib()
{
	Free();
}

void nuStyleAttrib::Free()
{
	switch ( Category )
	{
	case nuCatFontFamily:
		String.Free();
		break;
	}
}

nuStyleAttrib& nuStyleAttrib::operator=( const nuStyleAttrib& b )
{
	Free();
	Category = b.Category;
	switch ( Category )
	{
	case nuCatFontFamily:
		String.Set( b.String );
		break;
	default:
		memcpy( this, &b, sizeof(b) );
		break;
	}
	return *this;
}

void nuStyleAttrib::Discard()
{
	// We can set ourselves here to anything that has a trivial destructor
	Category = nuCatWidth;
}

void nuStyleAttrib::SetZeroCopy( const nuStyleAttrib& b )
{
	memcpy( this, &b, sizeof(b) );
}

void nuStyleAttrib::CloneFastInto( nuStyleAttrib& c, nuPool* pool ) const
{
	switch ( Category )
	{
	case nuCatFontFamily:
		c.Category = Category;
		String.CloneFastInto( c.String, pool );
		break;
	default:
		memcpy( &c, this, sizeof(c) );
		break;
	}
}

template<typename T>
void nuStyleAttrib::SetA( nuStyleCategories cat, T& prop, const T& value )
{
	Free();
	Category = cat;
	prop = value;
}

void nuStyleAttrib::SetColor( nuStyleCategories cat, nuColor val )
{
	SetA( cat, Color, val );
}

void nuStyleAttrib::SetSize( nuStyleCategories cat, nuSize val )
{
	SetA( cat, Size, val );
}

void nuStyleAttrib::SetBox( nuStyleCategories cat, nuStyleBox val )
{
	SetA( cat, Box, val );
}

void nuStyleAttrib::SetDisplay( nuDisplayType val )
{
	SetA( nuCatDisplay, Display, val );
}

void nuStyleAttrib::SetBorderRadius( nuSize val )
{
	SetA( nuCatBorderRadius, BorderRadius, val );
}

void nuStyleAttrib::SetPosition( nuPositionType val )
{
	SetA( nuCatPosition, Position, val );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
static bool Parse1( const char* s, intp len, bool (*parseFunc)(const char* s, intp len, T& t), nuStyleCategories cat, nuStyleAttrib& attrib, T& attribVal )
{
	if ( parseFunc( s, len, attribVal ) )
	{
		attrib.Category = cat;
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
			if ( MATCH(t, startk, eq, "background") )			{ a.SetColor( nuCatBackground, nuColor::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "width") )			{ a.SetSize( nuCatWidth, nuSize::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "height") )			{ a.SetSize( nuCatHeight, nuSize::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "padding") )			{ a.SetBox( nuCatPadding, nuStyleBox::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "margin") )			{ a.SetBox( nuCatMargin, nuStyleBox::Parse(TSTART, TLEN) ); }
			else if ( MATCH(t, startk, eq, "display") )			{ ok = Parse1( TSTART, TLEN, &nuDisplayTypeParse, nuCatDisplay, a, a.Display ); }
			else if ( MATCH(t, startk, eq, "position") )		{ ok = Parse1( TSTART, TLEN, &nuPositionTypeParse, nuCatPosition, a, a.Position ); }
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
			if ( ok )
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

bool nuStyle::Parsef( const char* t, ... )
{
	char buff[8192] = "";
	va_list va;
	va_start( va, t );
	uint r = vsnprintf( buff, arraysize(buff), t, va );
	va_end( va );
	if ( r < arraysize(buff) )
	{
		return Parse( buff );
	}
	else
	{
		NUASSERTDEBUG(false);
		return false;
	}
}

void nuStyle::Compute( const nuDoc& doc, const nuDomEl& node )
{
	NUASSERT( Name.Len == 0 );
	NUASSERT( Attribs.size() == 0 );

	nuStyle** defaults = nuDefaultTagStyles();

	const nuStyle* styles[] = {
		defaults[node.Tag],
		// TODO: Classes
		&node.Style,
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
			Attribs.add().SetZeroCopy( s[i] );
		}
		isrc = 1;
	}

	for ( ; isrc < n; isrc++ )
	{
		auto& s = src[isrc]->Attribs;

		for ( intp i = 0; i < s.size(); i++ )
		{
			u8 existing = lut[s[i].Category];
			if ( existing == EMPTY )
			{
				lut[s[i].Category] = (u8) Attribs.size();
				Attribs.add().SetZeroCopy( s[i] );
			}
			else
			{
				Attribs[existing].SetZeroCopy( s[i] );
			}
		}
	}
}

void nuStyle::Discard()
{
	for ( intp i = 0; i < Attribs.size(); i++ )
		Attribs[i].Discard();
	Attribs.clear_noalloc();
	Name.Discard();
}

void nuStyle::CloneFastInto( nuStyle& c, nuPool* pool ) const
{
	Name.CloneFastInto( c.Name, pool );
	nuClonePodvec( c.Attribs, Attribs, pool );
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
	IsReadOnly = false;
}

nuStyleSet::~nuStyleSet()
{
}

nuStyle* nuStyleSet::GetOrCreate( const char* name )
{
	NUASSERTDEBUG(name && name[0] != 0);
	return GetOrCreate( nuTempString( name ) );
}

nuStyle* nuStyleSet::GetOrCreate( const nuString& name )
{
	NUASSERT(!IsReadOnly);
	NUASSERT(name.Len != 0);
	int index;
	bool exists = NameToIndexMutable.get( name, index );
	if ( !exists )
	{
		index = (int) Styles.size();
		nuStyle& s = Styles.add();
		s.Name = name;
		NameToIndexMutable.insert( name, index );
	}
	return &Styles[index];
}

nuStyle* nuStyleSet::GetReadOnly( const nuString& name )
{
	NUASSERT(IsReadOnly);
	int index;
	bool exists = NameToIndexReadOnly.get( name, index );
	return exists ? &Styles[index] : NULL;
}

void nuStyleSet::CloneFastInto( nuStyleSet& c, nuPool* pool ) const
{
	c.IsReadOnly = true;
	nuClonePodvec( c.Styles, Styles, pool );
	c.BuildIndexReadOnly();
}

void nuStyleSet::Reset()
{
	if ( IsReadOnly )
	{
		Styles.hack( 0, 0, NULL );
	}
	NameToIndexMutable.clear();
	NameToIndexReadOnly.clear();
}

void nuStyleSet::BuildIndexReadOnly()
{
	NUASSERT(NameToIndexReadOnly.size() == 0);
	NameToIndexReadOnly.resize_for( Styles.size() );
	for ( intp i = 0; i < Styles.size(); i++ )
	{
		NUASSERTDEBUG( Styles[i].Name.Len != 0 );
		NUASSERTDEBUG( !NameToIndexReadOnly.contains(Styles[i].Name) );
		NameToIndexReadOnly.insert( Styles[i].Name, (int) i );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nuStyleTable::nuStyleTable()
{
}

nuStyleTable::~nuStyleTable()
{
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
	}
	nuStyle* s = &Styles[index];
	NameToIndex.insert( n, index );
	return s;
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
			NameToIndex.erase( Styles[i].Name );
			Styles[i].Name = "";
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
			Styles.erase(i);
	}

	NameToIndex.clear();
	for ( intp i = 0; i < Styles.size(); i++ )
		NameToIndex.insert( Styles[i].Name, (int) i );

	UnusedSlots.clear();
}

void nuStyleTable::GarbageCollectInternalR( BitMap& used, nuDomEl* node )
{
	for ( intp i = 0; i < node->Classes.size(); i++ )
		used.Set( node->Classes[i], true );
	
	for ( intp i = 0; i < node->ChildCount(); i++ )
		GarbageCollectInternalR( used, node->ChildByIndex(i) );
}

void nuStyleTable::CompactR( const nuStyleID* old2newID, nuDomEl* node )
{
	for ( intp i = 0; i < node->Classes.size(); i++ )
	{
		nuStyleID newval = old2newID[node->Classes[i]];
		node->Classes[i] = newval;
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
