#pragma once

// The default styles for tags are defined inside nuDoc::InitializeDefaultTagStyles()

#define NU_TAGS_DEFINE \
XX(NULL, 0) \
XY(Body) \
XY(Div) \
XY(Text) \
XY(Lab) \
XY(END) \

#define XX(a,b) nuTag##a = b,
#define XY(a) nuTag##a,
enum nuTag {
	NU_TAGS_DEFINE
};
#undef XX
#undef XY

extern const char* nuTagNames[nuTagEND + 1];
