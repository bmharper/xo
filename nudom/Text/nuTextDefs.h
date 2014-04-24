#pragma once

#include "nuDefs.h"

#ifndef FT_FREETYPE_H
typedef void* FT_Face;
typedef void* FT_Library;
#endif

// Once initialized, all members are immutable
class NUAPI nuFont
{
public:
	nuFontID	ID;
	nuString	Facename;
	FT_Face		FTFace;
	int32		LinearHoriAdvance_Space_x256;	// How much does character 32 advance?
	int32		NaturalLineHeight_x256;			// Recommended spacing between consecutive lines

			nuFont();
			~nuFont();
};