#include "pch.h"
#include "Doc.h"
#include "Style.h"
#include "CloneHelpers.h"
#include "Text/FontStore.h"
#include "Render/StyleResolve.h"

namespace xo {

#define EQ(a, b) (strcmp(a, b) == 0)

// Styles that are inherited by default
const StyleCategories InheritedStyleCategories[NumInheritedStyleCategories] = {
    CatFontFamily,
    CatFontSize,
    CatColor,
    CatText_Align_Vertical,
    CatCursor,
};

inline bool IsNumeric(char c) {
	return (c >= '0' && c <= '9') || (c == '.') || (c == '-');
}

static bool IsAlpha(char c) {
	return (c >= 'a' && c <= 'z') ||
	       (c >= 'A' && c <= 'Z');
}

static bool IsIdent(char c) {
	return IsAlpha(c) || c == '_' || c == '-' || c == '.' || (c >= '0' && c <= '9');
}

static bool HasDollar(const char* s, size_t start, size_t end) {
	for (; start != end; start++) {
		if (s[start] == '$')
			return true;
	}
	return false;
}

enum MatchFlags {
	MatchPrefix = 1,
};

static bool MATCH(const char* s, size_t start, size_t end, const char* truth, uint32_t matchFlags = 0) {
	for (; start != end; truth++, start++) {
		if (s[start] != *truth) {
			if (!!(matchFlags & MatchPrefix) && *truth == 0)
				return true;
			else
				return false;
		}
	}
	return *truth == 0;
}

static uint8_t ParseHexChar(char ch) {
	if (ch >= 'A' && ch <= 'F')
		return 10 + ch - 'A';
	if (ch >= 'a' && ch <= 'f')
		return 10 + ch - 'a';
	else
		return ch - '0';
}

static uint8_t ParseHexCharPair(const char* ch) {
	return (ParseHexChar(ch[0]) << 4) | ParseHexChar(ch[1]);
}

static uint8_t ParseHexCharSingle(const char* ch) {
	uint8_t v = ParseHexChar(ch[0]);
	return (v << 4) | v;
}

static int FindSpaces(const char* s, size_t len, int (&spaces)[10], bool& hasDollar) {
	int nspaces = 0;
	hasDollar   = false;
	for (size_t i = 0; i < len && nspaces < arraysize(spaces); i++) {
		if (s[i] == '$')
			hasDollar = true;
		if (IsWhitespace(s[i]))
			spaces[nspaces++] = (int) i;
	}
	// Fill the remaining members of 'spaces' with 'len', so that the parser
	// doesn't need to distinguish between a space and the end of the string
	for (size_t i = nspaces; i < 10; i++)
		spaces[i] = (int) len;
	return nspaces;
}

StyleCategories CatUngenerify(StyleCategories c) {
	switch (c) {
	case CatGenMargin: return CatMargin_Left;
	case CatGenPadding: return CatPadding_Left;
	case CatGenBorder: return CatBorder_Left;
	case CatGenBorderColor: return CatBorderColor_Left;
	case CatGenBorderRadius: return CatBorderRadius_TL;
	default:
		return c;
	}
}

const char* CatNameTable[CatEND] = {
    nullptr,
    "color",
    "Padding_Use_Me_1",
    "background",
    "Padding_Use_Me_2",
    "text_align_vertical",

    "break",
    "canfocus",
    "cursor",

    "overflow-x",
    "overflow-y",
    "padding_use_me_3",

    "margin-left",
    "margin-top",
    "margin-right",
    "margin-bottom",

    "padding-left",
    "padding-top",
    "padding-right",
    "padding-bottom",

    "border-left",
    "border-top",
    "border-right",
    "border-bottom",

    "border-color-left",
    "border-color-top",
    "border-color-right",
    "border-color-bottom",

    "border-radius-top-left",
    "border-radius-top-right",
    "border-radius-bottom-right",
    "border-radius-bottom-left",

    "width",
    "height",

    "top",
    "vcenter",
    "bottom",
    "baseline",

    "left",
    "hcenter",
    "right",

    "font-size",
    "font-family",
    "font-weight",

    "position",
    "flow-context",
    "flow-axis",
    "flow-direction-horizontal",
    "flow-direction-vertical",
    "box-sizing",
    "bump",

    "margin",
    "padding",
    "border",
    "border-color",
    "border-radius",
};

bool Size::Parse(const char* s, size_t len, Size& v) {
	// 1.23em
	// 1.23ep
	// 1.23ex
	// 1.23eh
	// 1.23px
	// 1.23pt
	// 1.23%
	// 1.23%r
	// 0
	char digits[100];
	if (len > 30) {
		ParseFail("Parse failed, size too big (>30 characters)\n");
		return false;
	}
	Size   x      = Size::Pixels(0);
	size_t nondig = 0;
	if (s[0] == '-') {
		digits[0] = '-';
		nondig    = 1;
	}
	for (; nondig < len; nondig++) {
		digits[nondig] = s[nondig];
		if (!IsNumeric(s[nondig]))
			break;
	}
	digits[nondig] = 0;
	x.Val          = (float) atof(digits);
	if (nondig == len) {
		if (len == 1 && s[0] == '0') {
			v = x;
			return true;
		} else {
			ParseFail("Parse failed, invalid size: %.*s\n", (int) len, s);
			return false;
		}
	}

	x.Type = Size::NONE;
	if (s[nondig] == '%') {
		if (len - nondig == 1) {
			x.Type = Size::PERCENT;
		} else if (len - nondig == 2 && s[nondig + 1] == 'r') {
			x.Type = Size::REMAINING;
		}
	} else if (len - nondig == 2) {
		char a = s[nondig];
		char b = s[nondig + 1];
		if (a == 'p' && b == 'x')
			x.Type = Size::PX;
		else if (a == 'p' && b == 't')
			x.Type = Size::PT;
		else if (a == 'e' && b == 'p')
			x.Type = Size::EP;
		else if (a == 'e' && b == 'm')
			x.Type = Size::EM;
		else if (a == 'e' && b == 'x')
			x.Type = Size::EX;
		else if (a == 'e' && b == 'h')
			x.Type = Size::EH;
	}
	if (x.Type == Size::NONE) {
		ParseFail("Parse failed, invalid size: %.*s\n", (int) len, s);
		return false;
	}
	v = x;
	return true;
}

static bool ParseQuadSize(const char* s, size_t len, Size* quad) {
	int  spaces[10];
	bool hasDollar;
	int  nspaces = FindSpaces(s, len, spaces, hasDollar);

	// LRTB
	// 20px
	if (nspaces == 0) {
		Size one;
		if (Size::Parse(s, len, one)) {
			quad[0] = quad[1] = quad[2] = quad[3] = one;
			return true;
		}
	}

	// LR  TB
	// 1px 2px
	if (nspaces == 1) {
		Size tmp[2];
		bool ok1 = Size::Parse(s, spaces[0], tmp[0]);
		bool ok2 = Size::Parse(s + spaces[0] + 1, spaces[1] - spaces[0] - 1, tmp[1]);
		if (ok1 && ok2) {
			quad[0] = tmp[0];
			quad[1] = tmp[1];
			quad[2] = tmp[0];
			quad[3] = tmp[1];
			return true;
		}
	}

	// L   T   R   B
	// 1px 2px 3px 4px
	if (nspaces == 3) {
		Size tmp[4];
		bool ok1 = Size::Parse(s, spaces[0], tmp[0]);
		bool ok2 = Size::Parse(s + spaces[0] + 1, spaces[1] - spaces[0] - 1, tmp[1]);
		bool ok3 = Size::Parse(s + spaces[1] + 1, spaces[2] - spaces[1] - 1, tmp[2]);
		bool ok4 = Size::Parse(s + spaces[2] + 1, spaces[3] - spaces[2] - 1, tmp[3]);
		if (ok1 && ok2 && ok3 && ok4) {
			memcpy(quad, tmp, sizeof(tmp));
			return true;
		}
	}
	return false;
}

bool StyleBox::Parse(const char* s, size_t len, StyleBox& v) {
	return ParseQuadSize(s, len, v.All);
}

bool CornerStyleBox::Parse(const char* s, size_t len, CornerStyleBox& v) {
	return ParseQuadSize(s, len, v.All);
}

bool Color::Parse(const char* s, size_t len, Color& v) {
	Color c = Color::RGBA(0, 0, 0, 0);
	s++;
	// #rgb
	// #rgba
	// #rrggbb
	// #rrggbbaa
	if (len == 4) {
		c.r = ParseHexCharSingle(s + 0);
		c.g = ParseHexCharSingle(s + 1);
		c.b = ParseHexCharSingle(s + 2);
		c.a = 255;
		//Trace( "color %s -> %d\n", s, (int) c.r );
	} else if (len == 5) {
		c.r = ParseHexCharSingle(s + 0);
		c.g = ParseHexCharSingle(s + 1);
		c.b = ParseHexCharSingle(s + 2);
		c.a = ParseHexCharSingle(s + 3);
	} else if (len == 7) {
		c.r = ParseHexCharPair(s + 0);
		c.g = ParseHexCharPair(s + 2);
		c.b = ParseHexCharPair(s + 4);
		c.a = 255;
	} else if (len == 9) {
		c.r = ParseHexCharPair(s + 0);
		c.g = ParseHexCharPair(s + 2);
		c.b = ParseHexCharPair(s + 4);
		c.a = ParseHexCharPair(s + 6);
	} else {
		ParseFail("Parse failed, invalid color %.*s\n", (int) len, s);
		return false;
	}
	v = c;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StyleAttrib::StyleAttrib() {
	memset(this, 0, sizeof(*this));
}

void StyleAttrib::SetU32(StyleCategories cat, uint32_t val) {
	Category = cat;
	ValU32   = val;
}

void StyleAttrib::SetWithSubtypeU32(StyleCategories cat, uint8_t subtype, uint32_t val) {
	Category = cat;
	SubType  = subtype;
	ValU32   = val;
}

void StyleAttrib::SetWithSubtypeF(StyleCategories cat, uint8_t subtype, float val) {
	Category = cat;
	SubType  = subtype;
	ValF     = val;
}

void StyleAttrib::SetString(StyleCategories cat, const char* str, Doc* doc) {
	Category = cat;
	ValU32   = doc->Strings.GetOrCreateID(str);
}

void StyleAttrib::SetInherit(StyleCategories cat) {
	Category = cat;
	Flags    = FlagInherit;
}

void StyleAttrib::SetVerbatim(StyleCategories cat, int verbatimID) {
	Category = cat;
	Flags    = FlagVerbatim;
	ValU32   = verbatimID;
}

const char* StyleAttrib::GetBackgroundImage(StringTable* strings) const {
	return strings->GetStr(ValU32);
}

FontID StyleAttrib::GetFont() const {
	return ValU32;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
static bool ParseSingleAttrib(const char* s, size_t len, bool (*parseFunc)(const char* s, size_t len, T& t), StyleCategories cat, Doc* doc, Style& style) {
	if (HasDollar(s, 0, len)) {
		style.SetVerbatim(cat, doc->GetOrCreateStyleVerbatimID(s, len));
		return true;
	}

	T val;
	if (parseFunc(s, len, val)) {
		StyleAttrib attrib;
		attrib.Set(cat, val);
		style.Set(attrib);
		return true;
	} else {
		ParseFail("Parse failed, unknown value: '%.*s'\n", (int) len, s);
		return false;
	}
}

static bool ParseIntComponent(const char* s, size_t len, int& val) {
	val      = 0;
	int tval = 0;

	size_t i = 0;
	while (i < len && IsWhitespace(s[i]))
		i++;

	for (; i < len; i++) {
		char c = s[i];
		if (c >= '0' && c <= '9') {
			tval = tval * 10 + s[i];
		} else if (IsWhitespace(s[i])) {
			for (i++; i < len; i++) {
				if (!IsWhitespace(s[i]))
					break;
			}
		} else {
			break;
		}
	}

	if (i != len)
		return false;

	val = tval;
	return true;
}

static bool ParseInt(const char* s, size_t len, StyleCategories cat, Doc* doc, Style& style) {
	if (HasDollar(s, 0, len)) {
		style.SetVerbatim(cat, doc->GetOrCreateStyleVerbatimID(s, len));
		return true;
	}

	int val = 0;
	if (!ParseIntComponent(s, len, val)) {
		ParseFail("Parse of %s failed, Not an integer: '%.*s'\n", CatNameTable[cat], (int) len, s);
		return false;
	}

	StyleAttrib attrib;
	attrib.SetInt(cat, val);
	style.Set(attrib);
	return true;
}

// This was added when font-family was stored as a string, but it is now stored as a FontID
static void ParseString(const char* s, size_t len, StyleCategories cat, Doc* doc, Style& style) {
	char        stat[64];
	StyleAttrib attrib;
	if (len < sizeof(stat)) {
		memcpy(stat, s, len);
		stat[len] = 0;
		attrib.Set(cat, stat, doc);
	} else {
		String copy;
		copy.Set(s, len);
		attrib.Set(cat, copy.CStr(), doc);
	}
	style.Set(attrib);
}

static bool ParseBool(const char* s, size_t len, StyleCategories cat, Doc* doc, Style& style) {
	bool val;
	if (len == 4 && s[0] == 't') {
		val = true;
	} else if (len == 5 && s[0] == 'f') {
		val = false;
	} else if (s[0] == '$') {
		style.SetVerbatim(cat, doc->GetOrCreateStyleVerbatimID(s, len));
		return true;
	} else {
		ParseFail("Parse failed, illegal boolean value: '%.*s'. Valid values are 'true' and 'false'.\n", (int) len, s);
		return false;
	}
	StyleAttrib attrib;
	attrib.SetBool(cat, val);
	style.Set(attrib);
	return true;
}

// Returns a modified category, based on 'base', or CatNULL if none match
StyleCategories SubCategoryOffsetLTRB(StyleCategories base, const char* subCategory, size_t subCategoryLen) {
	int offset = -1;
	if (MATCH(subCategory, 0, subCategoryLen, "left"))
		offset = 0;
	else if (MATCH(subCategory, 0, subCategoryLen, "top"))
		offset = 1;
	else if (MATCH(subCategory, 0, subCategoryLen, "right"))
		offset = 2;
	else if (MATCH(subCategory, 0, subCategoryLen, "bottom"))
		offset = 3;
	else {
		ParseFail("Parse failed, unknown sub-type: '%.*s'\n", (int) subCategoryLen, subCategory);
		return CatNULL;
	}
	return (StyleCategories)(CatUngenerify(base) + offset);
}

template <typename T>
static bool ParseBox(const char* s, size_t len, const char* subCategory, size_t subCategoryLen, bool (*parseFunc)(const char* s, size_t len, T& t), StyleCategories cat, Doc* doc, Style& style) {
	if (subCategory) {
		cat = SubCategoryOffsetLTRB(cat, subCategory, subCategoryLen);
		if (cat == CatNULL)
			return false;
	}

	if (HasDollar(s, 0, len)) {
		style.SetVerbatim(cat, doc->GetOrCreateStyleVerbatimID(s, len));
		return true;
	}

	cat = CatUngenerify(cat);

	T val;
	if (parseFunc(s, len, val)) {
		if (subCategory) {
			StyleAttrib attrib;
			attrib.Set(cat, val.Left);
			style.Set(attrib);
			return true;
		}
		style.Set(cat, val);
		return true;
	} else {
		ParseFail("Parse failed, unknown value: '%.*s'\n", (int) len, s);
		return false;
	}
}

template <typename T>
static bool ParseCornerBox(const char* s, size_t len, const char* subCategory, size_t subCategoryLen, bool (*parseFunc)(const char* s, size_t len, T& t), StyleCategories cat, Doc* doc, Style& style) {
	if (subCategory) {
		int offset = -1;
		if (MATCH(subCategory, 0, subCategoryLen, "top-left"))
			offset = 0;
		else if (MATCH(subCategory, 0, subCategoryLen, "top-right"))
			offset = 1;
		else if (MATCH(subCategory, 0, subCategoryLen, "bottom-right"))
			offset = 2;
		else if (MATCH(subCategory, 0, subCategoryLen, "bottom-left"))
			offset = 3;
		if (offset == -1) {
			ParseFail("Parse failed, unknown sub-type: '%.*s'\n", (int) subCategoryLen, subCategory);
			return false;
		}
		cat = (StyleCategories)(cat + offset);
	}

	if (HasDollar(s, 0, len)) {
		style.SetVerbatim(cat, doc->GetOrCreateStyleVerbatimID(s, len));
		return true;
	}

	cat = CatUngenerify(cat);

	T val;
	if (parseFunc(s, len, val)) {
		if (subCategory) {
			StyleAttrib attrib;
			attrib.Set(cat, val.TopLeft);
			style.Set(attrib);
			return true;
		}
		style.Set(cat, val);
		return true;
	} else {
		ParseFail("Parse failed, unknown value: '%.*s'\n", (int) len, s);
		return false;
	}
}

static bool ParseDirect(const char* s, size_t len, const char* subCategory, size_t subCategoryLen,
                        bool (*parseFunc)(const char* s, size_t len, const char* subCategory, size_t subCategoryLen, Doc* doc, Style& style),
                        Doc* doc, Style& style) {
	if (parseFunc(s, len, subCategory, subCategoryLen, doc, style)) {
		return true;
	} else {
		ParseFail("Parse failed on: '%.*s'\n", (int) len, s);
		return false;
	}
}

static bool ParseFontFamily(const char* s, size_t len, FontID& v) {
	bool   onFont = false;
	char   buf[64];
	size_t bufPos = 0;
	for (size_t i = 0; true; i++) {
		if (onFont) {
			if (s[i] == ',' || i == len) {
				buf[bufPos] = 0;
				v           = Global()->FontStore->InsertByFacename(buf);
				if (v != FontIDNull)
					return true;
				onFont = false;
				bufPos = 0;
			} else {
				buf[bufPos++] = s[i];
			}
			if (i == len)
				break;
		} else {
			if (i == len)
				break;
			if (IsWhitespace(s[i]))
				continue;
			onFont        = true;
			buf[bufPos++] = s[i];
		}
		if (bufPos >= arraysize(buf)) {
			ParseFail("Parse failed, font name too long (>63): '%*.s'\n", (int) len, s);
			return false;
		}
	}
	// not sure whether we should do this. One might want no font to be set instead.
	v = Global()->FontStore->GetFallbackFontID();
	return true;
}

template <size_t BufSize>
static void ParseExtractString(const char* t, size_t start, size_t end, char (&nameBuf)[BufSize]) {
	while (start < end && IsWhitespace(t[start]))
		start++;
	while (end > start + 1 && IsWhitespace(t[end - 1]))
		end--;
	if (end - start > BufSize - 1)
		end = start + BufSize - 1;
	auto len = end - start;
	memcpy(nameBuf, t + start, len);
	nameBuf[len] = 0;
}

// Next steps? Use a string table, so that StyleAttrib stores a 32-bit ID for the variable,
// instead of directly storing the string.
bool Style::ParseSheet(const char* t, Doc* doc) {
	enum States {
		NONE,
		STYLE_NAME,
		VAR_NAME,
		STYLE_VAL,
		VAR_VAL
	};

	States S             = NONE;
	size_t nameStartAt   = -1;
	size_t braceOpenedAt = -1;
	size_t valStartAt    = -1;
	char   nameBuf[128];
	char   valBuf[256];
	for (size_t i = 0; true; i++) {
		int c = t[i];
		if (c == 0)
			break;
		switch (S) {
		case NONE:
			if (c == '$') {
				S           = VAR_NAME;
				nameStartAt = i;
			} else if (IsAlpha(c)) {
				S           = STYLE_NAME;
				nameStartAt = i;
			} else if (!IsWhitespace(c)) {
				ParseFail("Unexpected character '%c' at position %d", c, (int) i);
				return false;
			}
			break;
		case STYLE_NAME:
			if (c == '{') {
				ParseExtractString(t, nameStartAt, i, nameBuf);
				S             = STYLE_VAL;
				braceOpenedAt = i + 1;
			} else if (!(IsWhitespace(c) || IsIdent(c))) {
				ParseFail("Unexpected character '%c' at position %d", c, (int) i);
				return false;
			}
			break;
		case VAR_NAME:
			if (c == '=') {
				ParseExtractString(t, nameStartAt, i, nameBuf);
				S          = VAR_VAL;
				valStartAt = i + 1;
			} else if (!(IsWhitespace(c) || IsIdent(c))) {
				ParseFail("Unexpected character '%c' at position %d", c, (int) i);
				return false;
			}
			break;
		case STYLE_VAL:
			if (c == '}') {
				if (!doc->ClassParse(nameBuf, t + braceOpenedAt, i - braceOpenedAt))
					return false;
				S = NONE;
			}
			break;
		case VAR_VAL:
			if (c == ';') {
				ParseExtractString(t, valStartAt, i, valBuf);
				doc->SetStyleVar(nameBuf, valBuf);
				S = NONE;
			}
			break;
		}
	}
	if (S != NONE) {
		ParseFail("State %d not finished", (int) S);
		return false;
	}
	return true;
}

bool Style::Parse(const char* t, Doc* doc) {
	return Parse(t, INT32_MAX, doc);
}

bool Style::Parse(const char* t, size_t maxLen, Doc* doc) {
// "background: #8f8; width: 100%; height: 100%;"
#define TSTART (t + startv)
#define TLEN (i - startv)
#define KSTART (t + startk)
#define KLEN (eq - startk)
	size_t startk = -1;
	size_t eq     = -1;
	size_t dash   = -1;
	size_t startv = -1;
	int    nerror = 0;
	for (size_t i = 0; true; i++) {
		bool eof = i == maxLen || t[i] == 0;
		char c   = eof ? 0 : t[i];
		if (IsWhitespace(c)) {
		} else if (c == ':') {
			eq = i;
		} else if (c == '-' && eq == -1) {
			dash = i;
		} else if (c == ';' || (eof && startv != -1)) {
			// clang-format off
			bool ok = true;
			// Keep these in sync with CatNameTable
			if (MATCH(t, startk, eq, "background"))                       { ok = ParseDirect(TSTART, TLEN, nullptr, 0, &ParseBackground, doc, *this); }
			else if (MATCH(t, startk, eq, "color"))                       { ok = ParseSingleAttrib(TSTART, TLEN, &Color::Parse, CatColor, doc, *this); }
			else if (MATCH(t, startk, eq, "width"))                       { ok = ParseSingleAttrib(TSTART, TLEN, &Size::Parse, CatWidth, doc, *this); }
			else if (MATCH(t, startk, eq, "height"))                      { ok = ParseSingleAttrib(TSTART, TLEN, &Size::Parse, CatHeight, doc, *this); }
			else if (MATCH(t, startk, eq, "padding"))                     { ok = ParseBox(TSTART, TLEN, nullptr, 0, &StyleBox::Parse, CatGenPadding, doc, *this); }
			else if (MATCH(t, startk, eq, "padding-", MatchPrefix))       { ok = ParseBox(TSTART, TLEN, KSTART + 8, KLEN - 8, &StyleBox::Parse, CatPadding_Left, doc, *this); }
			else if (MATCH(t, startk, eq, "margin"))                      { ok = ParseBox(TSTART, TLEN, nullptr, 0, &StyleBox::Parse, CatGenMargin, doc, *this); }
			else if (MATCH(t, startk, eq, "margin-", MatchPrefix))        { ok = ParseBox(TSTART, TLEN, KSTART + 7, KLEN - 7, &StyleBox::Parse, CatMargin_Left, doc, *this); }
			else if (MATCH(t, startk, eq, "position"))                    { ok = ParseSingleAttrib(TSTART, TLEN, &ParsePositionType, CatPosition, doc, *this); }
			else if (MATCH(t, startk, eq, "border-radius"))               { ok = ParseCornerBox(TSTART, TLEN, nullptr, 0, &CornerStyleBox::Parse, CatGenBorderRadius, doc, *this); }
			else if (MATCH(t, startk, eq, "border-radius-", MatchPrefix)) { ok = ParseCornerBox(TSTART, TLEN, KSTART + 14, KLEN - 14, &CornerStyleBox::Parse, CatBorderRadius_TL, doc, *this); }
			else if (MATCH(t, startk, eq, "border"))                      { ok = ParseDirect(TSTART, TLEN, nullptr, 0, &ParseBorder, doc, *this); }
			else if (MATCH(t, startk, eq, "border-", MatchPrefix))        { ok = ParseDirect(TSTART, TLEN, KSTART + 7, KLEN - 7, &ParseBorder, doc, *this); }
			else if (MATCH(t, startk, eq, "break"))                       { ok = ParseSingleAttrib(TSTART, TLEN, &ParseBreakType, CatBreak, doc, *this); }
			else if (MATCH(t, startk, eq, "canfocus"))                    { ok = ParseBool(TSTART, TLEN, CatCanFocus, doc, *this); }
			else if (MATCH(t, startk, eq, "cursor"))                      { ok = ParseSingleAttrib(TSTART, TLEN, &ParseCursor, CatCursor, doc, *this); }
			else if (MATCH(t, startk, eq, "flow-context"))                { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFlowContext, CatFlowContext, doc, *this); }
			else if (MATCH(t, startk, eq, "flow-axis"))                   { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFlowAxis, CatFlowAxis, doc, *this); }
			else if (MATCH(t, startk, eq, "flow-direction-horizontal"))   { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFlowDirection, CatFlowDirection_Horizontal, doc, *this); }
			else if (MATCH(t, startk, eq, "flow-direction-vertical"))     { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFlowDirection, CatFlowDirection_Vertical, doc, *this); }
			else if (MATCH(t, startk, eq, "box-sizing"))                  { ok = ParseSingleAttrib(TSTART, TLEN, &ParseBoxSize, CatBoxSizing, doc, *this); }
			else if (MATCH(t, startk, eq, "font-size"))                   { ok = ParseSingleAttrib(TSTART, TLEN, &Size::Parse, CatFontSize, doc, *this); }
			else if (MATCH(t, startk, eq, "font-family"))                 { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFontFamily, CatFontFamily, doc, *this); }
			else if (MATCH(t, startk, eq, "font-weight"))                 { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFontWeight, CatFontWeight, doc, *this); }
			else if (MATCH(t, startk, eq, "left"))                        { ok = ParseBinding(true, TSTART, TLEN, CatLeft, doc, *this); }
			else if (MATCH(t, startk, eq, "hcenter"))                     { ok = ParseBinding(true, TSTART, TLEN, CatHCenter, doc, *this); }
			else if (MATCH(t, startk, eq, "right"))                       { ok = ParseBinding(true, TSTART, TLEN, CatRight, doc, *this); }
			else if (MATCH(t, startk, eq, "top"))                         { ok = ParseBinding(false, TSTART, TLEN, CatTop, doc, *this); }
			else if (MATCH(t, startk, eq, "vcenter"))                     { ok = ParseBinding(false, TSTART, TLEN, CatVCenter, doc, *this); }
			else if (MATCH(t, startk, eq, "bottom"))                      { ok = ParseBinding(false, TSTART, TLEN, CatBottom, doc, *this); }
			else if (MATCH(t, startk, eq, "baseline"))                    { ok = ParseBinding(false, TSTART, TLEN, CatBaseline, doc, *this); }
			else if (MATCH(t, startk, eq, "bump"))                        { ok = ParseSingleAttrib(TSTART, TLEN, &ParseBump, CatBump, doc, *this); }
			// clang-format on
			else {
				ok = false;
				ParseFail("Parse failed - unknown property: '%.*s'\n", int(eq - startk), t + startk);
			}
			if (!ok)
				nerror++;
			eq     = -1;
			dash   = -1;
			startk = -1;
			startv = -1;
		} else {
			if (startk == -1)
				startk = i;
			else if (startv == -1 && eq != -1)
				startv = i;
		}

		if (eof)
			break;
	}
	return nerror == 0;
#undef TSTART
#undef TLEN
#undef KSTART
#undef KLEN
}

const StyleAttrib* Style::Get(StyleCategories cat) const {
	for (size_t i = 0; i < Attribs.size(); i++) {
		if (Attribs[i].Category == cat)
			return &Attribs[i];
	}
	return NULL;
}

void Style::GetBox(StyleCategories cat, StyleBox& val) const {
	GetBoxInternal(cat, val.All);
}

void Style::SetBox(StyleCategories cat, StyleBox val) {
	if (cat >= CatMargin_Left && cat <= CatBorder_Bottom) {
		SetBoxInternal(CatMakeBaseBox(cat), val.All);
	} else {
		XO_ASSERT(false);
	}
}

void Style::GetCornerBox(StyleCategories cat, CornerStyleBox& val) const {
	GetBoxInternal(cat, val.All);
}

void Style::SetCornerBox(StyleCategories cat, CornerStyleBox val) {
	if (cat >= CatBorderRadius_TL && cat <= CatBorderRadius_BL) {
		SetBoxInternal(CatMakeBaseBox(cat), val.All);
	} else {
		XO_ASSERT(false);
	}
}

void Style::SetUniformBox(StyleCategories cat, StyleAttrib val) {
	cat = CatMakeBaseBox(cat);
	for (int i = 0; i < 4; i++) {
		val.Category = (StyleCategories)(cat + i);
		Set(val);
	}
}

void Style::SetUniformBox(StyleCategories cat, Color color) {
	StyleAttrib val;
	val.SetColor(cat, color);
	SetUniformBox(cat, val);
}

void Style::SetUniformBox(StyleCategories cat, Size size) {
	StyleAttrib val;
	val.SetSize(cat, size);
	SetUniformBox(cat, val);
}

void Style::GetBoxInternal(StyleCategories cat, Size* quad) const {
	StyleCategories base = CatMakeBaseBox(cat);
	for (size_t i = 0; i < Attribs.size(); i++) {
		uint32_t pindex = uint32_t(Attribs[i].Category - base);
		if (pindex < 4)
			quad[pindex] = Attribs[i].GetSize();
	}
}

void Style::SetVerbatim(StyleCategories cat, int verbatimID) {
	StyleAttrib a;
	a.SetVerbatim(cat, verbatimID);
	Set(a);
}

void Style::SetBoxInternal(StyleCategories catBase, Size* quad) {
	StyleAttrib a;
	for (int i = 0; i < 4; i++) {
		a.SetSize((StyleCategories)(catBase + i), quad[i]);
		Set(a);
	}
}

void Style::Set(StyleAttrib attrib) {
	for (size_t i = 0; i < Attribs.size(); i++) {
		if (Attribs[i].Category == attrib.Category) {
			Attribs[i] = attrib;
			return;
		}
	}
	Attribs += attrib;
}

void Style::Set(StyleCategories cat, StyleBox val) {
	SetBox(cat, val);
}

void Style::Set(StyleCategories cat, CornerStyleBox val) {
	SetCornerBox(cat, val);
}

void Style::Clear() {
	Attribs.clear_noalloc();
}

void Style::CloneSlowInto(Style& c) const {
	c.Attribs = Attribs;
}

void Style::CloneFastInto(Style& c, Pool* pool) const {
	//Name.CloneFastInto( c.Name, pool );
	ClonePodvecWithMemCopy(c.Attribs, Attribs, pool);
}

#define XX(name, type, setfunc, cat) \
	\
void Style::Set##name(type value) \
{ \
		StyleAttrib a;               \
		a.setfunc(cat, value);       \
		Set(a);                      \
	\
}
NUSTYLE_SETTERS_2P
#undef XX

#define XX(name, type, setfunc)      \
	\
void Style::Set##name(type value) \
{ \
		StyleAttrib a;               \
		a.setfunc(value);            \
		Set(a);                      \
	\
}
NUSTYLE_SETTERS_1P
#undef XX

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StyleSet::StyleSet() {
	Reset();
}

StyleSet::~StyleSet() {
}

void StyleSet::Reset() {
	Lookup      = NULL;
	Attribs     = NULL;
	Count       = 0;
	Capacity    = 0;
	BitsPerSlot = 0;
	//SetSlotF = NULL;
	//GetSlotF = NULL;
}

void StyleSet::Grow(Pool* pool) {
	XO_ASSERT(BitsPerSlot != 8);
	uint32_t     newbits    = BitsPerSlot == 0 ? InitialBitsPerSlot : BitsPerSlot * 2;
	uint32_t     totalBits  = newbits * CatEND;
	void*        newlookup  = pool->Alloc((totalBits + 7) / 8, true);
	StyleAttrib* newattribs = pool->AllocNT<StyleAttrib>(CapacityAt(newbits), false);
	if (BitsPerSlot) {
		if (newbits == 4)
			MigrateLookup(Lookup, newlookup, &GetSlot2, &SetSlot4);
		else if (newbits == 8)
			MigrateLookup(Lookup, newlookup, &GetSlot4, &SetSlot8);

		memcpy(newattribs, Attribs, CapacityAt(BitsPerSlot) * sizeof(StyleAttrib));
	}
	Capacity    = CapacityAt(newbits);
	Lookup      = newlookup;
	Attribs     = newattribs;
	BitsPerSlot = newbits;
	//if (newbits == 2)		{ SetSlotF = &SetSlot2; GetSlotF = &GetSlot2; }
	//else if (newbits == 4)	{ SetSlotF = &SetSlot4; GetSlotF = &GetSlot4; }
	//else if (newbits == 8)	{ SetSlotF = &SetSlot8; GetSlotF = &GetSlot8; }
}

void StyleSet::Set(int n, const StyleAttrib* attribs, Pool* pool) {
	for (int i = 0; i < n; i++)
		Set(attribs[i], pool);
}

void StyleSet::Set(const StyleAttrib& attrib, Pool* pool) {
	int32_t slot = GetSlot(attrib.GetCategory());
	if (slot != 0) {
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

StyleAttrib StyleSet::Get(StyleCategories cat) const {
	int32_t slot = GetSlot(cat) - SlotOffset;
	if (slot == -1)
		return StyleAttrib();
	return Attribs[slot];
}

void StyleSet::EraseOrSetNull(StyleCategories cat) const {
	int32_t slot = GetSlot(cat) - SlotOffset;
	if (slot != -1)
		Attribs[slot] = StyleAttrib();
}

bool StyleSet::Contains(StyleCategories cat) const {
	return GetSlot(cat) != 0;
}

void StyleSet::MigrateLookup(const void* lutsrc, void* lutdst, GetSlotFunc getter, SetSlotFunc setter) {
	for (int i = CatFIRST; i < CatEND; i++) {
		int32_t slot = getter(lutsrc, (StyleCategories) i);
		if (slot != 0)
			setter(lutdst, (StyleCategories) i, slot);
	}
}

int32_t StyleSet::GetSlot(StyleCategories cat) const {
	if (Lookup == nullptr)
		return 0;
	GetSlotFunc f = (BitsPerSlot == 2) ? &GetSlot2 : (BitsPerSlot == 4 ? &GetSlot4 : &GetSlot8);
	return f(Lookup, cat);
}

void StyleSet::SetSlot(StyleCategories cat, int32_t slot) {
	SetSlotFunc f = (BitsPerSlot == 2) ? &SetSlot2 : (BitsPerSlot == 4 ? &SetSlot4 : &SetSlot8);
	f(Lookup, cat, slot);
}

void StyleSet::DebugCheckSanity() const {
	for (int i = CatFIRST; i < CatEND; i++) {
		StyleCategories cat = (StyleCategories) i;
		StyleAttrib     val = Get(cat);
		XO_DEBUG_ASSERT(val.IsNull() || val.Category == cat);
	}
}

template <uint32_t BITS_PER_SLOT>
void StyleSet::TSetSlot(void* lookup, StyleCategories cat, int32_t slot) {
	const uint32_t mask    = (1 << BITS_PER_SLOT) - 1;
	uint8_t*       lookup8 = (uint8_t*) lookup;

	if (BITS_PER_SLOT == 8) {
		lookup8[cat] = slot;
	} else {
		uint32_t intra_byte_mask;
		uint32_t ibyte;
		if (BITS_PER_SLOT == 2) {
			ibyte           = ((uint32_t) cat) >> 2;
			intra_byte_mask = 3;
		} else if (BITS_PER_SLOT == 4) {
			ibyte           = ((uint32_t) cat) >> 1;
			intra_byte_mask = 1;
		}

		uint8_t v           = lookup8[ibyte];
		uint8_t islotinbyte = ((uint32_t) cat) & intra_byte_mask;
		uint8_t ishift      = islotinbyte * BITS_PER_SLOT;
		uint8_t shiftedmask = mask << ishift;
		v                   = v & ~shiftedmask;
		v                   = v | (((uint32_t) slot) << ishift);
		lookup8[ibyte]      = v;
	}
}

template <uint32_t BITS_PER_SLOT>
int32_t StyleSet::TGetSlot(const void* lookup, StyleCategories cat) {
	const uint32_t mask    = (1 << BITS_PER_SLOT) - 1;
	const uint8_t* lookup8 = (const uint8_t*) lookup;

	if (BITS_PER_SLOT == 8) {
		return lookup8[cat];
	} else {
		uint32_t intra_byte_mask;
		uint32_t ibyte;
		if (BITS_PER_SLOT == 2) {
			ibyte           = ((uint32_t) cat) >> 2;
			intra_byte_mask = 3;
		} else if (BITS_PER_SLOT == 4) {
			ibyte           = ((uint32_t) cat) >> 1;
			intra_byte_mask = 1;
		}

		uint8_t v           = lookup8[ibyte];
		uint8_t islotinbyte = ((uint32_t) cat) & intra_byte_mask;
		uint8_t ishift      = islotinbyte * BITS_PER_SLOT;
		uint8_t shiftedmask = mask << ishift;
		v                   = (v & shiftedmask) >> ishift;
		return v;
	}
}

int32_t StyleSet::GetSlot2(const void* lookup, StyleCategories cat) { return TGetSlot<2>(lookup, cat); }
int32_t StyleSet::GetSlot4(const void* lookup, StyleCategories cat) { return TGetSlot<4>(lookup, cat); }
int32_t StyleSet::GetSlot8(const void* lookup, StyleCategories cat) { return TGetSlot<8>(lookup, cat); }

void StyleSet::SetSlot2(void* lookup, StyleCategories cat, int32_t slot) { TSetSlot<2>(lookup, cat, slot); }
void StyleSet::SetSlot4(void* lookup, StyleCategories cat, int32_t slot) { TSetSlot<4>(lookup, cat, slot); }
void StyleSet::SetSlot8(void* lookup, StyleCategories cat, int32_t slot) { TSetSlot<8>(lookup, cat, slot); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StyleTable::StyleTable() {
}

StyleTable::~StyleTable() {
}

void StyleTable::AddDummyStyleZero() {
	GetOrCreate("");
}

void StyleTable::Discard() {
	Classes.discard();
	Names.discard();
}

const StyleClass* StyleTable::GetByID(StyleClassID id) const {
	return &Classes[id];
}

StyleClass* StyleTable::GetOrCreate(const char* name) {
	TempString n(name);
	// find existing
	int* pindex = NameToIndex.getp(n);
	if (pindex)
		return &Classes[*pindex];

	// create new
	int index = (int) Classes.size();
	Classes.add();
	Names.add();
	StyleClass* s = &Classes[index];
	NameToIndex.insert(n, index);
	Names[index] = String(name);
	return s;
}

StyleClassID StyleTable::GetClassID(const char* name) const {
	TempString n(name);
	int*       pindex = NameToIndex.getp(n);
	if (pindex)
		return StyleClassID(*pindex);
	else
		return StyleClassID(0);
}

void StyleTable::CloneSlowInto(StyleTable& c) const {
	c.Classes = Classes;
// The renderer doesn't need a Name -> ID table. That lookup table is only for end-user convenience.
// HOWEVER, it can be useful when debugging
#ifdef _DEBUG
	c.NameToIndex = NameToIndex;
#endif
}

void StyleTable::CloneFastInto(StyleTable& c, Pool* pool) const {
	// The renderer doesn't need a Name -> ID table. That lookup table is only for end-user convenience.
	ClonePodvecWithMemCopy(c.Classes, Classes, pool);
}

void StyleTable::ExpandVerbatimVariables(Doc* doc) {
	cheapvec<char> buf;

	for (auto& c : Classes) {
		Style* pseudoGroup = c.All4PseudoTypes();
		for (size_t i = 0; i < 4; i++) {
			Style& pseudo = pseudoGroup[i];

			// Instead of modifying the list of attributes in-place, which can potentially
			// involve a lot of memmoves, we rather build up a new style, the moment we
			// detect any verbatim replacements
			Style newStyle;
			bool  usingNewStyle = false;

			for (size_t j = 0; j < pseudo.Attribs.size(); j++) {
				const StyleAttrib& attrib = pseudo.Attribs[j];
				if (attrib.IsVerbatim()) {
					if (!usingNewStyle) {
						usingNewStyle = true;
						for (size_t k = 0; k < j; k++)
							newStyle.Attribs.push(pseudo.Attribs[k]);
					}
					// ignore parsing errors inside ExplodeVerbatimAttrib - there is nothing we can do about it
					StyleResolver::ExplodeVerbatimAttrib(doc, attrib, buf, newStyle);
				} else {
					// not verbatim
					if (usingNewStyle)
						newStyle.Attribs.push(attrib);
				}
			}

			if (usingNewStyle)
				std::swap(newStyle, pseudo);
		}
	}
}

void StyleTable::DebugDump() const {
	Trace("%d classes:\n", (int) NameToIndex.size());
	for (auto it : NameToIndex)
		Trace("%s: %d\n", it.first.CStr(), (int) it.second);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XO_API bool ParsePositionType(const char* s, size_t len, PositionType& t) {
	if (MATCH(s, 0, len, "static")) {
		t = PositionStatic;
		return true;
	}
	if (MATCH(s, 0, len, "absolute")) {
		t = PositionAbsolute;
		return true;
	}
	if (MATCH(s, 0, len, "relative")) {
		t = PositionRelative;
		return true;
	}
	if (MATCH(s, 0, len, "fixed")) {
		t = PositionFixed;
		return true;
	}
	return false;
}

XO_API bool ParseBreakType(const char* s, size_t len, BreakType& t) {
	if (MATCH(s, 0, len, "none")) {
		t = BreakNULL;
		return true;
	}
	if (MATCH(s, 0, len, "before")) {
		t = BreakBefore;
		return true;
	}
	if (MATCH(s, 0, len, "after")) {
		t = BreakAfter;
		return true;
	}
	return false;
}

XO_API bool ParseCursor(const char* s, size_t len, Cursors& t) {
	if (MATCH(s, 0, len, "arrow")) {
		t = CursorArrow;
		return true;
	}
	if (MATCH(s, 0, len, "hand")) {
		t = CursorHand;
		return true;
	}
	if (MATCH(s, 0, len, "text")) {
		t = CursorText;
		return true;
	}
	if (MATCH(s, 0, len, "wait")) {
		t = CursorWait;
		return true;
	}
	return false;
}

XO_API bool ParseFlowContext(const char* s, size_t len, FlowContext& t) {
	if (MATCH(s, 0, len, "new")) {
		t = FlowContextNew;
		return true;
	}
	if (MATCH(s, 0, len, "inject")) {
		t = FlowContextInject;
		return true;
	}
	return false;
}

XO_API bool ParseFlowAxis(const char* s, size_t len, FlowAxis& t) {
	if (MATCH(s, 0, len, "horizontal")) {
		t = FlowAxisHorizontal;
		return true;
	}
	if (MATCH(s, 0, len, "vertical")) {
		t = FlowAxisVertical;
		return true;
	}
	return false;
}

XO_API bool ParseFlowDirection(const char* s, size_t len, FlowDirection& t) {
	if (MATCH(s, 0, len, "normal")) {
		t = FlowDirectionNormal;
		return true;
	}
	if (MATCH(s, 0, len, "reverse")) {
		t = FlowDirectionReversed;
		return true;
	}
	return false;
}

XO_API bool ParseBoxSize(const char* s, size_t len, BoxSizeType& t) {
	if (MATCH(s, 0, len, "content")) {
		t = BoxSizeContent;
		return true;
	}
	if (MATCH(s, 0, len, "border")) {
		t = BoxSizeBorder;
		return true;
	}
	if (MATCH(s, 0, len, "margin")) {
		t = BoxSizeMargin;
		return true;
	}
	return false;
}

XO_API bool ParseHorizontalBinding(const char* s, size_t len, HorizontalBindings& t) {
	if (MATCH(s, 0, len, "none")) {
		t = HorizontalBindingNULL;
		return true;
	}
	if (MATCH(s, 0, len, "left")) {
		t = HorizontalBindingLeft;
		return true;
	}
	if (MATCH(s, 0, len, "hcenter")) {
		t = HorizontalBindingCenter;
		return true;
	}
	if (MATCH(s, 0, len, "right")) {
		t = HorizontalBindingRight;
		return true;
	}
	return false;
}

XO_API bool ParseVerticalBinding(const char* s, size_t len, VerticalBindings& t) {
	if (MATCH(s, 0, len, "none")) {
		t = VerticalBindingNULL;
		return true;
	}
	if (MATCH(s, 0, len, "top")) {
		t = VerticalBindingTop;
		return true;
	}
	if (MATCH(s, 0, len, "vcenter")) {
		t = VerticalBindingCenter;
		return true;
	}
	if (MATCH(s, 0, len, "bottom")) {
		t = VerticalBindingBottom;
		return true;
	}
	if (MATCH(s, 0, len, "baseline")) {
		t = VerticalBindingBaseline;
		return true;
	}
	return false;
}

XO_API bool ParseBinding(bool isHorz, const char* s, size_t len, StyleCategories cat, Doc* doc, Style& style) {
	if (HasDollar(s, 0, len)) {
		style.SetVerbatim(cat, doc->GetOrCreateStyleVerbatimID(s, len));
		return true;
	}

	StyleAttrib attrib;
	Size        size;
	bool        ok = false;
	if (isHorz) {
		HorizontalBindings hb;
		if (ParseHorizontalBinding(s, len, hb)) {
			ok = true;
			attrib.Set(cat, hb);
		}
	} else {
		VerticalBindings vb;
		if (ParseVerticalBinding(s, len, vb)) {
			ok = true;
			attrib.Set(cat, vb);
		}
	}

	if (!ok) {
		ok = size.Parse(s, len, size);
		if (ok)
			attrib.Set(cat, size);
	}

	if (ok) {
		style.Set(attrib);
		return true;
	} else {
		ParseFail("Parse failed, unknown value: '%.*s'\n", (int) len, s);
		return false;
	}
}

XO_API bool ParseBump(const char* s, size_t len, BumpStyle& t) {
	if (MATCH(s, 0, len, "regular")) {
		t = BumpRegular;
		return true;
	}
	if (MATCH(s, 0, len, "horizontal")) {
		t = BumpHorzOnly;
		return true;
	}
	if (MATCH(s, 0, len, "vertical")) {
		t = BumpVertOnly;
		return true;
	}
	if (MATCH(s, 0, len, "none")) {
		t = BumpNone;
		return true;
	}
	return false;
}

XO_API bool ParseBorder(const char* s, size_t len, const char* subCategory, size_t subCategoryLen, Doc* doc, Style& style) {
	StyleCategories cat = CatGenBorder;
	if (subCategory) {
		cat = SubCategoryOffsetLTRB(cat, subCategory, subCategoryLen);
		if (cat == CatNULL)
			return false;
	}

	int  spaces[10];
	bool hasDollar;
	int  nspaces = FindSpaces(s, len, spaces, hasDollar);
	if (hasDollar) {
		style.SetVerbatim(cat, doc->GetOrCreateStyleVerbatimID(s, len));
		return true;
	}

	StyleCategories catColor = (StyleCategories)(cat + (CatBorderColor_Left - CatBorder_Left));

	if (nspaces == 0) {
		// 1px		OR
		// #000
		Color color;
		if (Color::Parse(s, len, color)) {
			if (subCategory)
				style.Set(catColor, color);
			else
				style.SetUniformBox(CatBorderColor_Left, color);
			return true;
		}
		Size size;
		if (Size::Parse(s, len, size)) {
			if (subCategory)
				style.Set(cat, size);
			else
				style.SetBox(CatBorder_Left, StyleBox::MakeUniform(size));
			return true;
		}
	} else if (nspaces == 1) {
		// 1px #000
		Size  size;
		Color color;
		if (Size::Parse(s, spaces[0], size)) {
			if (Color::Parse(s + spaces[0] + 1, len - spaces[0] - 1, color)) {
				if (subCategory) {
					style.Set(cat, size);
					style.Set(catColor, color);
				} else {
					style.SetBox(CatBorder_Left, StyleBox::MakeUniform(size));
					style.SetUniformBox(CatBorderColor_Left, color);
				}
				return true;
			}
		}
	} else if ((nspaces == 3 || nspaces == 4) && !subCategory) {
		// 1px 2px 3px 4px #000
		StyleBox box;
		bool     s1 = Size::Parse(s, spaces[0], box.Left);
		bool     s2 = Size::Parse(s + spaces[0] + 1, spaces[1] - spaces[0] - 1, box.Top);
		bool     s3 = Size::Parse(s + spaces[1] + 1, spaces[2] - spaces[1] - 1, box.Right);
		bool     s4 = Size::Parse(s + spaces[2] + 1, spaces[3] - spaces[2] - 1, box.Bottom);
		if (!(s1 && s2 && s3 && s4))
			return false;
		style.SetBox(CatBorder_Left, box);
		if (nspaces == 4) {
			Color color;
			if (!Color::Parse(s + spaces[3] + 1, spaces[4] - spaces[3] - 1, color))
				return false;
			style.SetUniformBox(CatBorderColor_Left, color);
		}
		return true;
	}

	return false;
}

XO_API bool ParseBackground(const char* s, size_t len, const char* subCategory, size_t subCategoryLen, Doc* doc, Style& style) {
	// #rgb
	// #rgba
	// #rrggbb
	// #rrggbbaa
	// svg(image name)

	char buf[MaxSvgNameLen + 1];

	if (s[0] == '#') {
		Color c;
		if (Color::Parse(s, len, c)) {
			style.Set(xo::StyleCategories::CatBackground, c);
			return true;
		}
	} else if (s[0] == 's' && s[1] == 'v' && s[2] == 'g' && s[3] == '(') {
		size_t i = 4;
		for (; i < len; i++) {
			if (s[i] == ')') {
				size_t nameLen = i - 4;
				if (nameLen > MaxSvgNameLen)
					return false;
				memcpy(buf, s + 4, nameLen);
				buf[nameLen] = 0;
				int id       = doc->GetSvgID(buf);
				if (id == 0)
					return false;
				StyleAttrib a;
				a.SetBackgroundVector(id);
				style.Set(a);
				return true;
			}
		}
	}

	return false;
}

XO_API bool ParseFontWeight(const char* s, size_t len, FontWeight& val) {
	// This table should match what we have inside FontStore.cpp
	if (MATCH(s, 0, len, "thin")) {
		val = FontWeightThin;
		return true;
	}
	if (MATCH(s, 0, len, "light")) {
		val = FontWeightLight;
		return true;
	}
	if (MATCH(s, 0, len, "semilight")) {
		val = FontWeightSemiLight;
		return true;
	}
	if (MATCH(s, 0, len, "normal")) {
		val = FontWeightNormal;
		return true;
	}
	if (MATCH(s, 0, len, "medium")) {
		val = FontWeightMedium;
		return true;
	}
	if (MATCH(s, 0, len, "bold")) {
		val = FontWeightBold;
		return true;
	}
	if (MATCH(s, 0, len, "black")) {
		val = FontWeightBlack;
		return true;
	}
	int ival = 0;
	if (ParseIntComponent(s, len, ival)) {
		ival = Clamp(ival, 100, 900); // at render time we divide by 100 and store as byte, so only expect 1..9
		val  = (FontWeight) ival;
		return true;
	}
	return false;
}

} // namespace xo
