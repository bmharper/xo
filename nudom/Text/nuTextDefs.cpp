#include "pch.h"
#include "nuTextDefs.h"

nuFont::nuFont()
{
	ID = nuFontIDNull;
	FTFace = NULL;
	LinearHoriAdvance_Space_x256 = 0;
	LineHeight_x256 = 0;
	Ascender_x256 = 0;
	Descender_x256 = 0;
	// See nuFontStore::LoadFontTweaks for more details
	MaxAutoHinterSize = 15;
}

nuFont::~nuFont()
{
}
