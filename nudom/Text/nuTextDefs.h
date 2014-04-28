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
	int32		LineHeight_x256;				// Recommended spacing between consecutive lines
	int32		Ascender_x256;					// Ascender
	int32		Descender_x256;					// Descender
	uint32		MaxAutoHinterSize;				// Maximum font size at which we force use of the auto hinter. Heuristic thumb-suck observations. Only applies to sub-pixel rendering.

			nuFont();
			~nuFont();
};