#include "pch.h"
#include "xoTextDefs.h"

xoFont::xoFont()
{
	ID = xoFontIDNull;
	FTFace = NULL;
	LinearHoriAdvance_Space_x256 = 0;
	LineHeight_x256 = 0;
	Ascender_x256 = 0;
	Descender_x256 = 0;
	// See xoFontStore::LoadFontTweaks for more details
	MaxAutoHinterSize = 15;
}

xoFont::~xoFont()
{
}
