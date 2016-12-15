#include "pch.h"
#include "../Defs.h"
#include "../Doc.h"
#include "DocParser.h"

namespace xo {

struct StringBuf {
	size_t Capacity;
	size_t Len;
	char*  Buf;
	char   StaticBuf[256];

	StringBuf() {
		Capacity = arraysize(StaticBuf);
		Len      = 0;
		Buf      = StaticBuf;
		Buf[0]   = 0;
	}
	~StringBuf() {
		if (Buf != StaticBuf)
			free(Buf);
	}
	void Add(char c) {
		if (Len == Capacity - 1) {
			if (Capacity == arraysize(StaticBuf)) {
				Capacity *= 2;
				Buf = (char*) MallocOrDie(Capacity);
				memcpy(Buf, StaticBuf, Len);
			} else {
				Capacity *= 2;
				Buf = (char*) ReallocOrDie(Buf, Capacity);
			}
		}
		Buf[Len++] = c;
	}
	void Terminate() {
		Buf[Len] = 0;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String DocParser::Parse(const char* src, DomNode* target) {
	enum States {
		SText,
		STagOpen,
		STagClose,
		SCompactClose,
		SAttribs,
		SAttribName,
		SAttribBodyStart,
		SAttribBodySingleQuote,
		SAttribBodyDoubleQuote,
	};

	States             s        = SText;
	ssize_t            pos      = 0;
	ssize_t            xStart   = 0;
	ssize_t            xEnd     = 0;
	ssize_t            txtStart = 0;
	cheapvec<DomNode*> stack;
	stack += target;

	auto err = [&](const char* msg) -> String {
		ssize_t start = Max<ssize_t>(pos - 1, 0);
		String  sample;
		sample.Set(src + start, 10);
		return tsf::fmt("Parse error at position %v (%v): %v", pos, sample.Z, msg).c_str();
	};

	auto newNode = [&]() -> String {
		if (xEnd - xStart <= 0)
			return "Tag is empty";
		ssize_t i = TagNULL + 1;
		for (; i < TagEND; i++) {
			if (EqNoCase(TagNames[i], src + xStart, xEnd - xStart))
				break;
		}
		if (i == TagEND)
			return String("Unrecognized tag ") + String(src + xStart, xEnd - xStart);
		stack += stack.back()->AddNode((Tag) i);
		return "";
	};

	auto newText = [&]() -> String {
		bool      white  = true; // True if the entire string is whitespace or empty
		ssize_t   escape = -1;
		StringBuf str;
		for (ssize_t i = txtStart; i < pos; i++) {
			int c = src[i];
			if (escape != -1) {
				if (c == ';') {
					ssize_t len = i - escape;
					if (len == 2 && src[escape] == 'l' && src[escape + 1] == 't')
						str.Add('<');
					else if (len == 2 && src[escape] == 'g' && src[escape + 1] == 't')
						str.Add('>');
					else if (len == 2 && src[escape] == 's' && src[escape + 1] == 'p')
						str.Add(' ');
					else if (len == 3 && src[escape] == 'a' && src[escape + 1] == 'm' && src[escape + 2] == 'p')
						str.Add('&');
					else
						return tsf::fmt("Invalid escape sequence (%v)", String(src + escape, len).Z).c_str();
					escape = -1;
				}
			} else {
				bool w = IsWhite(c);
				if (c == '&')
					escape = i + 1;
				else if (c == '\r') {
				} // ignore '\r'
				else
					str.Add(c);
				white = white && w;
			}
		}
		if (escape != -1)
			return "Unfinished escape sequence";
		// Pure whitespace is discarded. This is solely so that one can indent DOM elements.
		if (white)
			return "";
		if (str.Len != 0) {
			str.Terminate();
			stack.back()->AddText(str.Buf);
		}
		return "";
	};

	auto closeNodeCompact = [&]() -> String {
		if (stack.size() == 1)
			return "Too many closing tags"; // not sure if this is reachable; suspect not.
		stack.pop();
		return "";
	};

	auto closeNode = [&]() -> String {
		if (stack.size() == 1)
			return "Too many closing tags";
		DomNode* top = stack.back();
		if (!EqNoCase(TagNames[top->GetTag()], src + xStart, xEnd - xStart))
			return tsf::fmt("Cannot close %v here. Expected %v close.", String(src + xStart, xEnd - xStart).Z, TagNames[top->GetTag()]).c_str();
		stack.pop();
		return "";
	};

	auto setAttrib = [&]() -> String {
		if (xEnd - xStart <= 0)
			return "Attribute name is empty"; // should be impossible to reach this, due to possible state transitions

		DomNode* node = stack.back();
		char     buf[64];

		ssize_t bodyStart = xEnd + 2;
		if (EqNoCase("style", src + xStart, xEnd - xStart)) {
			if (node->StyleParse(src + bodyStart, pos - bodyStart))
				return "";
			return String("Invalid style: ") + String(src + bodyStart, pos - bodyStart);
		} else if (EqNoCase("class", src + xStart, xEnd - xStart)) {
			ssize_t cstart = bodyStart;
			for (ssize_t i = bodyStart; i <= pos; i++) {
				if (i == pos || IsWhite(src[i])) {
					ssize_t len = i - cstart;
					if (len > 0) {
						if (len < arraysize(buf) - 1) {
							memcpy(buf, src + cstart, len);
							buf[len] = 0;
							node->AddClass(buf);
						} else {
							node->AddClass(String(src + cstart, i - cstart).Z);
						}
					}
					cstart = i + 1;
				}
			}
			return "";
		}

		return String("Unrecognized attribute ") + String(src + xStart, xEnd - xStart);
	};

	for (; src[pos]; pos++) {
		int    c = src[pos];
		String e;
		switch (s) {
		case SText:
			if (c == '<') {
				s      = STagOpen;
				xStart = pos + 1;
				e      = newText();
				break;
			} else {
				break;
			}
		case STagOpen:
			if (IsWhite(c)) {
				s    = SAttribs;
				xEnd = pos;
				e    = newNode();
				break;
			} else if (c == '/') {
				s      = STagClose;
				xStart = pos + 1;
				break;
			} else if (c == '>') {
				s        = SText;
				txtStart = pos + 1;
				xEnd     = pos;
				e        = newNode();
				break;
			} else if (IsAlpha(c)) {
				break;
			} else {
				return err("Expected a tag name");
			}
		case STagClose:
			if (c == '>') {
				s        = SText;
				txtStart = pos + 1;
				xEnd     = pos;
				e        = closeNode();
				break;
			} else if (IsAlpha(c)) {
				break;
			} else {
				return err("Expected >");
			}
		case SCompactClose:
			if (c == '>') {
				s        = SText;
				txtStart = pos + 1;
				e        = closeNodeCompact();
				break;
			} else {
				return err("Expected >");
			}
		case SAttribs:
			if (IsWhite(c)) {
				break;
			} else if (IsAlpha(c)) {
				s      = SAttribName;
				xStart = pos;
				break;
			} else if (c == '/') {
				s = SCompactClose;
				break;
			} else if (c == '>') {
				s        = SText;
				txtStart = pos + 1;
				break;
			} else {
				return err("Expected attributes or >");
			}
		case SAttribName:
			if (IsAlpha(c)) {
				break;
			} else if (c == '=') {
				xEnd = pos;
				s    = SAttribBodyStart;
				break;
			} else {
				return err("Expected attribute name or =");
			}
		case SAttribBodyStart:
			if (c == '\'') {
				s = SAttribBodySingleQuote;
				break;
			}
			if (c == '"') {
				s = SAttribBodyDoubleQuote;
				break;
			} else {
				return err("Expected \"");
			}
		case SAttribBodySingleQuote:
			if (c == '\'') {
				s = SAttribs;
				e = setAttrib();
				break;
			}
		case SAttribBodyDoubleQuote:
			if (c == '"') {
				s = SAttribs;
				e = setAttrib();
				break;
			}
		}
		if (e != "")
			return err(e.Z);
	}

	if (s != SText)
		return err("Unfinished");

	if (stack.size() != 1)
		return err("Unclosed tags");

	return newText();
}

bool DocParser::IsWhite(int c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool DocParser::IsAlpha(int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool DocParser::EqNoCase(const char* a, const char* b, size_t bLen) {
	for (size_t i = 0;; i++) {
		if (a[i] == 0 && i == bLen)
			return true;
		if (a[i] == 0 && i < bLen)
			return false;
		if (i == bLen)
			return false;
		int aa = a[i] >= 'a' && a[i] <= 'z' ? a[i] + 'A' - 'a' : a[i];
		int bb = b[i] >= 'a' && b[i] <= 'z' ? b[i] + 'A' - 'a' : b[i];
		if (aa != bb)
			return false;
	}
	// should be unreachable
	XO_DEBUG_ASSERT(false);
	return false;
}
}
