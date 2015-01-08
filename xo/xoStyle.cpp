#include "pch.h"
#include "xoDoc.h"
#include "xoStyle.h"
#include "xoCloneHelpers.h"
#include "Text/xoFontStore.h"

#define EQ(a,b) (strcmp(a,b) == 0)

// Styles that are inherited by default
const xoStyleCategories xoInheritedStyleCategories[xoNumInheritedStyleCategories] = {
	xoCatFontFamily,
	xoCatFontSize,
	xoCatColor,
	xoCatText_Align_Vertical,
	xoCatCursor,
};

inline bool IsNumeric(char c)
{
	return (c >= '0' && c <= '9') || (c == '.') || (c == '-');
}

// This is parsing whitespace, not DOM/textual whitespace
// In other words, it is the space between the comma and verdana in "font-family: verdana, arial",
inline bool IsWhitespace(char c)
{
	return c == 32 || c == 9;
}

static bool MATCH(const char* s, intp start, intp end, const char* truth)
{
	for (; start != end; truth++, start++)
	{
		if (s[start] != *truth) return false;
	}
	return true;
}

static uint8 ParseHexChar(char ch)
{
	if (ch >= 'A' && ch <= 'F')	return 10 + ch - 'A';
	if (ch >= 'a' && ch <= 'f')	return 10 + ch - 'a';
	else							return ch - '0';
}

static uint8 ParseHexCharPair(const char* ch)
{
	return (ParseHexChar(ch[0]) << 4) | ParseHexChar(ch[1]);
}

static uint8 ParseHexCharSingle(const char* ch)
{
	uint8 v = ParseHexChar(ch[0]);
	return (v << 4) | v;
}

static int FindSpaces(const char* s, intp len, int (&spaces)[10])
{
	int nspaces = 0;
	for (intp i = 0; i < len && nspaces < arraysize(spaces); i++)
	{
		if (IsWhitespace(s[i]))
			spaces[nspaces++] = (int) i;
	}
	return nspaces;
}

bool xoSize::Parse(const char* s, intp len, xoSize& v)
{
	// 1.23px
	// 1.23ep
	// 1.23pt
	// 1.23%
	// 0
	char digits[100];
	if (len > 30)
	{
		xoParseFail("Parse failed, size too big (>30 characters)\n");
		return false;
	}
	xoSize x = xoSize::Pixels(0);
	intp nondig = 0;
	for (; nondig < len; nondig++)
	{
		digits[nondig] = s[nondig];
		if (!IsNumeric(s[nondig]))
			break;
	}
	digits[nondig] = 0;
	x.Val = (float) atof(digits);
	if (nondig == len)
	{
		if (len == 1 && s[0] == '0')
		{
			// ok
		}
		else
		{
			xoParseFail("Parse failed, invalid size: %.*s\n", (int) len, s);
			return false;
		}
	}
	else
	{
		if (s[nondig] == '%')
		{
			x.Type = xoSize::PERCENT;
		}
		else if (s[nondig] == 'p' && len - nondig >= 2)
		{
			if (s[nondig + 1] == 'x') x.Type = xoSize::PX;
			else if (s[nondig + 1] == 't') x.Type = xoSize::PT;
			else
			{
				xoParseFail("Parse failed, invalid size: %.*s\n", (int) len, s);
				return false;
			}
		}
		else if (s[nondig] == 'e' && len - nondig >= 2 && s[nondig + 1] == 'p')
		{
			x.Type = xoSize::EP;
		}
	}
	v = x;
	return true;
}

bool xoStyleBox::Parse(const char* s, intp len, xoStyleBox& v)
{
	int spaces[10];
	int nspaces = FindSpaces(s, len, spaces);

	// 20px
	// 1px 2px 3px 4px (TODO)
	xoStyleBox b;
	if (nspaces == 0)
	{
		xoSize one;
		if (xoSize::Parse(s, len, one))
		{
			b.Left = b.Top = b.Bottom = b.Right = one;
			v = b;
			return true;
		}
	}

	// 1px 2px 3px 4px
	if (nspaces == 3)
	{
		bool ok1 = xoSize::Parse(s, spaces[0], b.Left);
		bool ok2 = xoSize::Parse(s + spaces[0] + 1, spaces[1] - spaces[0] - 1, b.Top);
		bool ok3 = xoSize::Parse(s + spaces[1] + 1, spaces[2] - spaces[1] - 1, b.Right);
		bool ok4 = xoSize::Parse(s + spaces[2] + 1, (int) len - spaces[2] - 1, b.Bottom);
		if (ok1 && ok2 && ok3 && ok4)
		{
			v = b;
			return true;
		}
	}
	return false;
}

bool xoColor::Parse(const char* s, intp len, xoColor& v)
{
	xoColor c = xoColor::RGBA(0,0,0,0);
	s++;
	// #rgb
	// #rgba
	// #rrggbb
	// #rrggbbaa
	if (len == 4)
	{
		c.r = ParseHexCharSingle(s + 0);
		c.g = ParseHexCharSingle(s + 1);
		c.b = ParseHexCharSingle(s + 2);
		c.a = 255;
		//XOTRACE( "color %s -> %d\n", s, (int) c.r );
	}
	else if (len == 5)
	{
		c.r = ParseHexCharSingle(s + 0);
		c.g = ParseHexCharSingle(s + 1);
		c.b = ParseHexCharSingle(s + 2);
		c.a = ParseHexCharSingle(s + 3);
	}
	else if (len == 7)
	{
		c.r = ParseHexCharPair(s + 0);
		c.g = ParseHexCharPair(s + 2);
		c.b = ParseHexCharPair(s + 4);
		c.a = 255;
	}
	else if (len == 9)
	{
		c.r = ParseHexCharPair(s + 0);
		c.g = ParseHexCharPair(s + 2);
		c.b = ParseHexCharPair(s + 4);
		c.a = ParseHexCharPair(s + 6);
	}
	else
	{
		xoParseFail("Parse failed, invalid color %.*s\n", (int) len, s);
		return false;
	}
	v = c;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoStyleAttrib::xoStyleAttrib()
{
	memset(this, 0, sizeof(*this));
}

void xoStyleAttrib::SetU32(xoStyleCategories cat, uint32 val)
{
	Category = cat;
	ValU32 = val;
}

void xoStyleAttrib::SetWithSubtypeU32(xoStyleCategories cat, uint8 subtype, uint32 val)
{
	Category = cat;
	SubType = subtype;
	ValU32 = val;
}

void xoStyleAttrib::SetWithSubtypeF(xoStyleCategories cat, uint8 subtype, float val)
{
	Category = cat;
	SubType = subtype;
	ValF = val;
}

void xoStyleAttrib::SetString(xoStyleCategories cat, const char* str, xoDoc* doc)
{
	Category = cat;
	ValU32 = doc->Strings.GetId(str);
}

void xoStyleAttrib::SetInherit(xoStyleCategories cat)
{
	Category = cat;
	Flags = FlagInherit;
}

const char* xoStyleAttrib::GetBackgroundImage(xoStringTable* strings) const
{
	return strings->GetStr(ValU32);
}

xoFontID xoStyleAttrib::GetFont() const
{
	return ValU32;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
static bool ParseSingleAttrib(const char* s, intp len, bool (*parseFunc)(const char* s, intp len, T& t), xoStyleCategories cat, xoStyle& style)
{
	T val;
	if (parseFunc(s, len, val))
	{
		xoStyleAttrib attrib;
		attrib.Set(cat, val);
		style.Set(attrib);
		return true;
	}
	else
	{
		xoParseFail("Parse failed, unknown value: '%.*s'\n", (int) len, s);
		return false;
	}
}

// This was added when font-family was stored as a string, but it is now stored as a xoFontID
static void ParseString(const char* s, intp len, xoStyleCategories cat, xoDoc* doc, xoStyle& style)
{
	char stat[64];
	xoStyleAttrib attrib;
	if (len < sizeof(stat))
	{
		memcpy(stat, s, len);
		stat[len] = 0;
		attrib.Set(cat, stat, doc);
	}
	else
	{
		xoString copy;
		copy.Set(s, len);
		attrib.Set(cat, copy.Z, doc);
	}
	style.Set(attrib);
}

static bool ParseBool(const char* s, intp len, xoStyleCategories cat, xoStyle& style)
{
	bool val;
	if (len == 4 && s[0] == 't')
		val = true;
	else if (len == 5 && s[0] == 'f')
		val = false;
	else
	{
		xoParseFail("Parse failed, illegal boolean value: '%.*s'. Valid values are 'true' and 'false'.\n", (int) len, s);
		return false;
	}
	xoStyleAttrib attrib;
	attrib.SetBool(cat, val);
	style.Set(attrib);
	return true;
}

template<typename T>
static bool ParseCompound(const char* s, intp len, bool (*parseFunc)(const char* s, intp len, T& t), xoStyleCategories cat, xoStyle& style)
{
	T val;
	if (parseFunc(s, len, val))
	{
		style.Set(cat, val);
		return true;
	}
	else
	{
		xoParseFail("Parse failed, unknown value: '%.*s'\n", (int) len, s);
		return false;
	}
}

static bool ParseDirect(const char* s, intp len, bool (*parseFunc)(const char* s, intp len, xoStyle& style), xoStyle& style)
{
	if (parseFunc(s, len, style))
	{
		return true;
	}
	else
	{
		xoParseFail("Parse failed on: '%.*s'\n", (int) len, s);
		return false;
	}
}

static bool ParseFontFamily(const char* s, intp len, xoFontID& v)
{
	bool onFont = false;
	char buf[64];
	intp bufPos = 0;
	for (intp i = 0; true; i++)
	{
		if (onFont)
		{
			if (s[i] == ',' || i == len)
			{
				buf[bufPos] = 0;
				v = xoGlobal()->FontStore->InsertByFacename(buf);
				if (v != xoFontIDNull)
					return true;
				onFont = false;
				bufPos = 0;
			}
			else
			{
				buf[bufPos++] = s[i];
			}
			if (i == len)
				break;
		}
		else
		{
			if (i == len)
				break;
			if (IsWhitespace(s[i]))
				continue;
			onFont = true;
			buf[bufPos++] = s[i];
		}
		if (bufPos >= arraysize(buf))
		{
			xoParseFail("Parse failed, font name too long (>63): '%*.s'\n", (int) len, s);
			return false;
		}
	}
	// not sure whether we should do this. One might want no font to be set instead.
	v = xoGlobal()->FontStore->GetFallbackFontID();
	return true;
}

bool xoStyle::Parse(const char* t, xoDoc* doc)
{
	return Parse(t, INT32MAX, doc);
}

bool xoStyle::Parse(const char* t, intp maxLen, xoDoc* doc)
{
	// "background: #8f8; width: 100%; height: 100%;"
#define TSTART	(t + startv)
#define TLEN	(i - startv)
	intp startk = -1;
	intp eq = -1;
	intp startv = -1;
	int nerror = 0;
	for (intp i = 0; true; i++)
	{
		bool eof = t[i] == 0 || i == maxLen;
		if (t[i] == 32) {}
		else if (t[i] == ':') eq = i;
		else if (t[i] == ';' || (eof && startv != -1))
		{
			bool ok = true;
			if (MATCH(t, startk, eq, "background"))							{ ok = ParseSingleAttrib(TSTART, TLEN, &xoColor::Parse, xoCatBackground, *this); }
			else if (MATCH(t, startk, eq, "color"))							{ ok = ParseSingleAttrib(TSTART, TLEN, &xoColor::Parse, xoCatColor, *this); }
			else if (MATCH(t, startk, eq, "width"))							{ ok = ParseSingleAttrib(TSTART, TLEN, &xoSize::Parse, xoCatWidth, *this); }
			else if (MATCH(t, startk, eq, "height"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoSize::Parse, xoCatHeight, *this); }
			else if (MATCH(t, startk, eq, "padding"))						{ ok = ParseCompound(TSTART, TLEN, &xoStyleBox::Parse, xoCatPadding_Left, *this); }
			else if (MATCH(t, startk, eq, "margin"))						{ ok = ParseCompound(TSTART, TLEN, &xoStyleBox::Parse, xoCatMargin_Left, *this); }
			else if (MATCH(t, startk, eq, "display"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseDisplayType, xoCatDisplay, *this); }
			else if (MATCH(t, startk, eq, "position"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParsePositionType, xoCatPosition, *this); }
			else if (MATCH(t, startk, eq, "border"))						{ ok = ParseDirect(TSTART, TLEN, &xoParseBorder, *this); }
			else if (MATCH(t, startk, eq, "border-radius"))					{ ok = ParseSingleAttrib(TSTART, TLEN, &xoSize::Parse, xoCatBorderRadius, *this); }
			//else if ( MATCH(t, startk, eq, "left") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &xoSize::Parse, xoCatLeft, *this ); }
			//else if ( MATCH(t, startk, eq, "right") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &xoSize::Parse, xoCatRight, *this ); }
			//else if ( MATCH(t, startk, eq, "top") )						{ ok = ParseSingleAttrib( TSTART, TLEN, &xoSize::Parse, xoCatTop, *this ); }
			//else if ( MATCH(t, startk, eq, "bottom") )					{ ok = ParseSingleAttrib( TSTART, TLEN, &xoSize::Parse, xoCatBottom, *this ); }
			else if (MATCH(t, startk, eq, "break"))							{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseBreakType, xoCatBreak, *this); }
			else if (MATCH(t, startk, eq, "canfocus"))						{ ok = ParseBool(TSTART, TLEN, xoCatCanFocus, *this); }
			else if (MATCH(t, startk, eq, "cursor"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseCursor, xoCatCursor, *this); }
			else if (MATCH(t, startk, eq, "flow-context"))					{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseFlowContext, xoCatFlowContext, *this); }
			else if (MATCH(t, startk, eq, "flow-axis"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseFlowAxis, xoCatFlowAxis, *this); }
			else if (MATCH(t, startk, eq, "flow-direction-horizontal"))		{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseFlowDirection, xoCatFlowDirection_Horizontal, *this); }
			else if (MATCH(t, startk, eq, "flow-direction-vertical"))		{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseFlowDirection, xoCatFlowDirection_Vertical, *this); }
			else if (MATCH(t, startk, eq, "box-sizing"))					{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseBoxSize, xoCatBoxSizing, *this); }
			else if (MATCH(t, startk, eq, "font-size"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoSize::Parse, xoCatFontSize, *this); }
			else if (MATCH(t, startk, eq, "font-family"))					{ ok = ParseSingleAttrib(TSTART, TLEN, &ParseFontFamily, xoCatFontFamily, *this); }
			else if (MATCH(t, startk, eq, "text-align-vertical"))			{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseTextAlignVertical, xoCatText_Align_Vertical, *this); }
			else if (MATCH(t, startk, eq, "left"))							{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseHorizontalBinding, xoCatLeft, *this); }
			else if (MATCH(t, startk, eq, "hcenter"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseHorizontalBinding, xoCatHCenter, *this); }
			else if (MATCH(t, startk, eq, "right"))							{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseHorizontalBinding, xoCatRight, *this); }
			else if (MATCH(t, startk, eq, "top"))							{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseVerticalBinding, xoCatTop, *this); }
			else if (MATCH(t, startk, eq, "vcenter"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseVerticalBinding, xoCatVCenter, *this); }
			else if (MATCH(t, startk, eq, "bottom"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseVerticalBinding, xoCatBottom, *this); }
			else if (MATCH(t, startk, eq, "baseline"))						{ ok = ParseSingleAttrib(TSTART, TLEN, &xoParseVerticalBinding, xoCatBaseline, *this); }
			else
			{
				ok = false;
				xoParseFail("Parse failed - unknown property: '%.*s'\n", int(eq - startk), t + startk);
			}
			if (!ok)
				nerror++;
			eq = -1;
			startk = -1;
			startv = -1;
		}
		else
		{
			if (startk == -1)						startk = i;
			else if (startv == -1 && eq != -1)	startv = i;
		}

		if (eof) break;
	}
	return nerror == 0;
#undef TSTART
#undef TLEN
}

const xoStyleAttrib* xoStyle::Get(xoStyleCategories cat) const
{
	for (intp i = 0; i < Attribs.size(); i++)
	{
		if (Attribs[i].Category == cat) return &Attribs[i];
	}
	return NULL;
}

void xoStyle::GetBox(xoStyleCategories cat, xoStyleBox& box) const
{
	xoStyleCategories base = xoCatMakeBaseBox(cat);
	for (intp i = 0; i < Attribs.size(); i++)
	{
		uint pindex = uint(Attribs[i].Category - base);
		if (pindex < 4)
			box.All[pindex] = Attribs[i].GetSize();
	}
}

void xoStyle::SetBox(xoStyleCategories cat, xoStyleBox val)
{
	if (cat >= xoCatMargin_Left && cat <= xoCatBorder_Bottom)
	{
		SetBoxInternal(xoCatMakeBaseBox(cat), val);
	}
	else XOASSERT(false);
}

void xoStyle::SetUniformBox(xoStyleCategories cat, xoStyleAttrib val)
{
	cat = xoCatMakeBaseBox(cat);
	val.Category = (xoStyleCategories)(cat + 0);	Set(val);
	val.Category = (xoStyleCategories)(cat + 1);	Set(val);
	val.Category = (xoStyleCategories)(cat + 2);	Set(val);
	val.Category = (xoStyleCategories)(cat + 3);	Set(val);
}

void xoStyle::SetUniformBox(xoStyleCategories cat, xoColor color)
{
	xoStyleAttrib val;
	val.SetColor(cat, color);
	SetUniformBox(cat, val);
}

void xoStyle::SetUniformBox(xoStyleCategories cat, xoSize size)
{
	xoStyleAttrib val;
	val.SetSize(cat, size);
	SetUniformBox(cat, val);
}

void xoStyle::SetBoxInternal(xoStyleCategories catBase, xoStyleBox val)
{
	xoStyleAttrib a;
	a.SetSize((xoStyleCategories)(catBase + 0), val.Left);	Set(a);
	a.SetSize((xoStyleCategories)(catBase + 1), val.Top);	Set(a);
	a.SetSize((xoStyleCategories)(catBase + 2), val.Right);	Set(a);
	a.SetSize((xoStyleCategories)(catBase + 3), val.Bottom);	Set(a);
}

void xoStyle::Set(xoStyleAttrib attrib)
{
	for (intp i = 0; i < Attribs.size(); i++)
	{
		if (Attribs[i].Category == attrib.Category)
		{
			Attribs[i] = attrib;
			return;
		}
	}
	Attribs += attrib;
}

void xoStyle::Set(xoStyleCategories cat, xoStyleBox val)
{
	SetBox(cat, val);
}

/*
void xoStyle::MergeInZeroCopy( int n, const xoStyle** src )
{
	if ( n == 0 ) return;

	// Use a lookup table to avoid making this merge operation O(n*m).
	// If the list of attributes balloons, then we should probably use something like a two level tree here.
	// This table is:
	// Category > Attrib Index
	u8 lut[xoCatEND];
	static const u8 EMPTY = 255;
	memset( lut, EMPTY, sizeof(lut) );
	static_assert( xoCatEND < EMPTY, "xoCat__ fits in a byte" );

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

	// Apply each xoStyle in turn. Later xoStyles override earlier ones.
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

void xoStyle::Discard()
{
	Attribs.clear_noalloc();
	//Name.Discard();
}

void xoStyle::CloneSlowInto(xoStyle& c) const
{
	c.Attribs = Attribs;
}

void xoStyle::CloneFastInto(xoStyle& c, xoPool* pool) const
{
	//Name.CloneFastInto( c.Name, pool );
	xoClonePodvecWithMemCopy(c.Attribs, Attribs, pool);
}

#define XX(name, type, setfunc, cat) \
void xoStyle::Set##name( type value ) \
{ \
	xoStyleAttrib a; \
	a.setfunc( cat, value ); \
	Set( a ); \
}
NUSTYLE_SETTERS_2P
#undef XX

#define XX(name, type, setfunc) \
void xoStyle::Set##name( type value ) \
{ \
	xoStyleAttrib a; \
	a.setfunc( value ); \
	Set( a ); \
}
NUSTYLE_SETTERS_1P
#undef XX

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoStyleSet::xoStyleSet()
{
	Reset();
}

xoStyleSet::~xoStyleSet()
{
}

void xoStyleSet::Reset()
{
	Lookup = NULL;
	Attribs = NULL;
	Count = 0;
	Capacity = 0;
	BitsPerSlot = 0;
	SetSlotF = NULL;
	GetSlotF = NULL;
}

void xoStyleSet::Grow(xoPool* pool)
{
	XOASSERT(BitsPerSlot != 8);
	uint32 newbits = BitsPerSlot == 0 ? InitialBitsPerSlot : BitsPerSlot * 2;
	uint32 totalBits = newbits * xoCatEND;
	void*			newlookup = pool->Alloc((totalBits + 7) / 8, true);
	xoStyleAttrib*	newattribs = pool->AllocNT<xoStyleAttrib>(CapacityAt(newbits), false);
	if (BitsPerSlot)
	{
		if (newbits == 4)			MigrateLookup(Lookup, newlookup, &GetSlot2, &SetSlot4);
		else if (newbits == 8)	MigrateLookup(Lookup, newlookup, &GetSlot4, &SetSlot8);

		memcpy(newattribs, Attribs, CapacityAt(BitsPerSlot) * sizeof(xoStyleAttrib));
	}
	Capacity = CapacityAt(newbits);
	Lookup = newlookup;
	Attribs = newattribs;
	BitsPerSlot = newbits;
	if (newbits == 2)			{ SetSlotF = &SetSlot2; GetSlotF = &GetSlot2; }
	else if (newbits == 4)	{ SetSlotF = &SetSlot4; GetSlotF = &GetSlot4; }
	else if (newbits == 8)	{ SetSlotF = &SetSlot8; GetSlotF = &GetSlot8; }
}

void xoStyleSet::Set(int n, const xoStyleAttrib* attribs, xoPool* pool)
{
	for (int i = 0; i < n; i++)
		Set(attribs[i], pool);
}

void xoStyleSet::Set(const xoStyleAttrib& attrib, xoPool* pool)
{
	if (attrib.Category == xoCatFontSize)
		int abc = 123;
	int32 slot = GetSlot(attrib.GetCategory());
	if (slot != 0)
	{
		Attribs[slot - 1] = attrib;
		return;
	}
	if (Count >= Capacity)
		Grow(pool);
	Attribs[Count] = attrib;
	SetSlot(attrib.GetCategory(), Count + SlotOffset);
	Count++;
	//DebugCheckSanity();
}

xoStyleAttrib xoStyleSet::Get(xoStyleCategories cat) const
{
	int32 slot = GetSlot(cat) - SlotOffset;
	if (slot == -1)
		return xoStyleAttrib();
	return Attribs[slot];
}

void xoStyleSet::DebugCheckSanity() const
{
	for (int i = xoCatFIRST; i < xoCatEND; i++)
	{
		xoStyleCategories cat = (xoStyleCategories) i;
		xoStyleAttrib val = Get(cat);
		XOASSERTDEBUG(val.IsNull() || val.Category == cat);
	}
}

bool xoStyleSet::Contains(xoStyleCategories cat) const
{
	return GetSlot(cat) != 0;
}

void xoStyleSet::MigrateLookup(const void* lutsrc, void* lutdst, GetSlotFunc getter, SetSlotFunc setter)
{
	for (int i = xoCatFIRST; i < xoCatEND; i++)
	{
		int32 slot = getter(lutsrc, (xoStyleCategories) i);
		if (slot != 0)
			setter(lutdst, (xoStyleCategories) i, slot);
	}
}

int32 xoStyleSet::GetSlot(xoStyleCategories cat) const
{
	if (!GetSlotF) return 0;
	return GetSlotF(Lookup, cat);
}

void xoStyleSet::SetSlot(xoStyleCategories cat, int32 slot)
{
	SetSlotF(Lookup, cat, slot);
}

template<uint32 BITS_PER_SLOT>
void xoStyleSet::TSetSlot(void* lookup, xoStyleCategories cat, int32 slot)
{
	const uint32	mask	= (1 << BITS_PER_SLOT) - 1;
	uint8*			lookup8	= (uint8*) lookup;

	if (BITS_PER_SLOT == 8)
	{
		lookup8[cat] = slot;
	}
	else
	{
		uint32	intra_byte_mask;
		uint32	ibyte;
		if (BITS_PER_SLOT == 2)		{ ibyte = ((uint32) cat) >> 2;	intra_byte_mask = 3; }
		else if (BITS_PER_SLOT == 4)	{ ibyte = ((uint32) cat) >> 1;	intra_byte_mask = 1; }

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
int32 xoStyleSet::TGetSlot(const void* lookup, xoStyleCategories cat)
{
	const uint32 mask		= (1 << BITS_PER_SLOT) - 1;
	const uint8* lookup8	= (const uint8*) lookup;

	if (BITS_PER_SLOT == 8)
	{
		return lookup8[cat];
	}
	else
	{
		uint32	intra_byte_mask;
		uint32	ibyte;
		if (BITS_PER_SLOT == 2)		{ ibyte = ((uint32) cat) >> 2;	intra_byte_mask = 3; }
		else if (BITS_PER_SLOT == 4)	{ ibyte = ((uint32) cat) >> 1;	intra_byte_mask = 1; }

		uint8	v = lookup8[ibyte];
		uint8	islotinbyte = ((uint32) cat) & intra_byte_mask;
		uint8	ishift = islotinbyte * BITS_PER_SLOT;
		uint8	shiftedmask = mask << ishift;
		v = (v & shiftedmask) >> ishift;
		return v;
	}
}

int32 xoStyleSet::GetSlot2(const void* lookup, xoStyleCategories cat) { return TGetSlot<2>(lookup, cat); }
int32 xoStyleSet::GetSlot4(const void* lookup, xoStyleCategories cat) { return TGetSlot<4>(lookup, cat); }
int32 xoStyleSet::GetSlot8(const void* lookup, xoStyleCategories cat) { return TGetSlot<8>(lookup, cat); }

void xoStyleSet::SetSlot2(void* lookup, xoStyleCategories cat, int32 slot) { TSetSlot<2>(lookup, cat, slot); }
void xoStyleSet::SetSlot4(void* lookup, xoStyleCategories cat, int32 slot) { TSetSlot<4>(lookup, cat, slot); }
void xoStyleSet::SetSlot8(void* lookup, xoStyleCategories cat, int32 slot) { TSetSlot<8>(lookup, cat, slot); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoStyleTable::xoStyleTable()
{
}

xoStyleTable::~xoStyleTable()
{
}

void xoStyleTable::AddDummyStyleZero()
{
	GetOrCreate("");
}

void xoStyleTable::Discard()
{
	Classes.hack(0, 0, NULL);
	Names.hack(0, 0, NULL);
}

const xoStyleClass* xoStyleTable::GetByID(xoStyleClassID id) const
{
	return &Classes[id];
}

xoStyleClass* xoStyleTable::GetOrCreate(const char* name)
{
	xoTempString n(name);
	// find existing
	int* pindex = NameToIndex.getp(n);
	if (pindex) return &Classes[*pindex];

	// create new
	int index = (int) Classes.size();
	Classes.add();
	Names.add();
	xoStyleClass* s = &Classes[index];
	NameToIndex.insert(n, index);
	Names[index] = xoString(name);
	return s;
}

xoStyleClassID xoStyleTable::GetClassID(const char* name)
{
	xoTempString n(name);
	int* pindex = NameToIndex.getp(n);
	if (pindex)	return xoStyleClassID(*pindex);
	else			return xoStyleClassID(0);
}

void xoStyleTable::CloneSlowInto(xoStyleTable& c) const
{
	// The renderer doesn't need a Name -> ID table. That lookup table is only for end-user convenience.
	c.Classes = Classes;
}

void xoStyleTable::CloneFastInto(xoStyleTable& c, xoPool* pool) const
{
	// The renderer doesn't need a Name -> ID table. That lookup table is only for end-user convenience.
	xoClonePodvecWithMemCopy(c.Classes, Classes, pool);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XOAPI bool xoParseDisplayType(const char* s, intp len, xoDisplayType& t)
{
	if (MATCH(s, 0, len, "block"))		{ t = xoDisplayBlock; return true; }
	if (MATCH(s, 0, len, "inline"))		{ t = xoDisplayInline; return true; }
	return false;
}

XOAPI bool xoParsePositionType(const char* s, intp len, xoPositionType& t)
{
	if (MATCH(s, 0, len, "static"))		{ t = xoPositionStatic; return true; }
	if (MATCH(s, 0, len, "absolute"))	{ t = xoPositionAbsolute; return true; }
	if (MATCH(s, 0, len, "relative"))	{ t = xoPositionRelative; return true; }
	if (MATCH(s, 0, len, "fixed"))		{ t = xoPositionFixed; return true; }
	return false;
}

XOAPI bool xoParseBreakType(const char* s, intp len, xoBreakType& t)
{
	if (MATCH(s, 0, len, "none"))		{ t = xoBreakNULL; return true; }
	if (MATCH(s, 0, len, "before"))		{ t = xoBreakBefore; return true; }
	if (MATCH(s, 0, len, "after"))		{ t = xoBreakAfter; return true; }
	return false;
}

XOAPI bool xoParseCursor(const char* s, intp len, xoCursors& t)
{
	if (MATCH(s, 0, len, "arrow"))		{ t = xoCursorArrow; return true; }
	if (MATCH(s, 0, len, "hand"))		{ t = xoCursorHand; return true; }
	if (MATCH(s, 0, len, "text"))		{ t = xoCursorText; return true; }
	if (MATCH(s, 0, len, "wait"))		{ t = xoCursorWait; return true; }
	return false;
}

XOAPI bool xoParseFlowContext(const char* s, intp len, xoFlowContext& t)
{
	if (MATCH(s, 0, len, "new"))		{ t = xoFlowContextNew; return true; }
	if (MATCH(s, 0, len, "inject"))		{ t = xoFlowContextInject; return true; }
	return false;
}

XOAPI bool xoParseFlowAxis(const char* s, intp len, xoFlowAxis& t)
{
	if (MATCH(s, 0, len, "horizontal"))	{ t = xoFlowAxisHorizontal; return true; }
	if (MATCH(s, 0, len, "vertical"))	{ t = xoFlowAxisVertical; return true; }
	return false;
}

XOAPI bool xoParseFlowDirection(const char* s, intp len, xoFlowDirection& t)
{
	if (MATCH(s, 0, len, "normal"))		{ t = xoFlowDirectionNormal; return true; }
	if (MATCH(s, 0, len, "reverse"))	{ t = xoFlowDirectionReversed; return true; }
	return false;
}

XOAPI bool xoParseBoxSize(const char* s, intp len, xoBoxSizeType& t)
{
	if (MATCH(s, 0, len, "content"))	{ t = xoBoxSizeContent; return true; }
	if (MATCH(s, 0, len, "border"))		{ t = xoBoxSizeBorder; return true; }
	if (MATCH(s, 0, len, "margin"))		{ t = xoBoxSizeMargin; return true; }
	return false;
}

XOAPI bool xoParseTextAlignVertical(const char* s, intp len, xoTextAlignVertical& t)
{
	if (MATCH(s, 0, len, "baseline"))	{ t = xoTextAlignVerticalBaseline; return true; }
	if (MATCH(s, 0, len, "top"))		{ t = xoTextAlignVerticalTop; return true; }
	return false;
}

XOAPI bool xoParseHorizontalBinding(const char* s, intp len, xoHorizontalBindings& t)
{
	if (MATCH(s, 0, len, "left"))		{ t = xoHorizontalBindingLeft; return true; }
	if (MATCH(s, 0, len, "hcenter"))	{ t = xoHorizontalBindingCenter; return true; }
	if (MATCH(s, 0, len, "right"))		{ t = xoHorizontalBindingRight; return true; }
	return false;
}

XOAPI bool xoParseVerticalBinding(const char* s, intp len, xoVerticalBindings& t)
{
	if (MATCH(s, 0, len, "top"))		{ t = xoVerticalBindingTop; return true; }
	if (MATCH(s, 0, len, "vcenter"))	{ t = xoVerticalBindingCenter; return true; }
	if (MATCH(s, 0, len, "bottom"))		{ t = xoVerticalBindingBottom; return true; }
	if (MATCH(s, 0, len, "baseline"))	{ t = xoVerticalBindingBaseline; return true; }
	return false;
}

XOAPI bool xoParseBorder(const char* s, intp len, xoStyle& style)
{
	int spaces[10];
	int nspaces = FindSpaces(s, len, spaces);

	if (nspaces == 0)
	{
		// 1px		OR
		// #000
		xoColor color;
		if (xoColor::Parse(s, len, color))
		{
			style.SetUniformBox(xoCatBorderColor_Left, color);
			return true;
		}
		xoSize size;
		if (xoSize::Parse(s, len, size))
		{
			style.SetBox(xoCatBorder_Left, xoStyleBox::MakeUniform(size));
			return true;
		}
	}
	else if (nspaces == 1)
	{
		// 1px #000
		xoSize size;
		xoColor color;
		if (xoSize::Parse(s, spaces[0], size))
		{
			if (xoColor::Parse(s + spaces[0] + 1, len - spaces[0] - 1, color))
			{
				style.SetBox(xoCatBorder_Left, xoStyleBox::MakeUniform(size));
				style.SetUniformBox(xoCatBorderColor_Left, color);
				return true;
			}
		}
	}
	else if (nspaces == 3)
	{
		// 1px 2px 3px 4px
		xoStyleBox box;
		bool s1 = xoSize::Parse(s, spaces[0], box.Left);
		bool s2 = xoSize::Parse(s + spaces[0] + 1, spaces[1] - spaces[0] - 1, box.Top);
		bool s3 = xoSize::Parse(s + spaces[1] + 1, spaces[2] - spaces[1] - 1, box.Right);
		bool s4 = xoSize::Parse(s + spaces[2] + 1, len - spaces[2] - 1, box.Bottom);
		if (s1 && s2 && s3 && s4)
		{
			style.SetBox(xoCatBorder_Left, box);
			return true;
		}
	}

	return false;
}
