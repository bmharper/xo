#include "pch.h"
#include "nuTextDefs.h"

nuFont::nuFont()
{
	ID = nuFontIDNull;
	FTFace = NULL;
	LinearHoriAdvance_Space_x256 = 0;
	NaturalLineHeight_x256 = 0;
}

nuFont::~nuFont()
{
}
