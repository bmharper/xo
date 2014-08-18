#include "pch.h"
#include "nuTags.h"

#define XX(a,b) #a,
#define XY(a) #a,

const char* nuTagNames[nuTagEND + 1] =
{
	NU_TAGS_DEFINE
};
