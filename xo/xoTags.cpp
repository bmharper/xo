#include "pch.h"
#include "xoTags.h"

#define XX(a,b) #a,
#define XY(a) #a,

const char* xoTagNames[xoTagEND + 1] =
{
	XO_TAGS_DEFINE
};

#undef XX
#undef XY