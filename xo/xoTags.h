#pragma once

// The default styles for tags are defined inside xoDoc::InitializeDefaultTagStyles()

#define XO_TAGS_DEFINE \
XX(NULL, 0) \
XY(Body) \
XY(Div) \
XY(Text) \
XY(Lab) \
XY(Span) \
XY(Canvas) \
XY(END) \

#define XX(a,b) xoTag##a = b,
#define XY(a) xoTag##a,
enum xoTag {
	XO_TAGS_DEFINE
};
#undef XX
#undef XY

extern const char* xoTagNames[xoTagEND + 1];
