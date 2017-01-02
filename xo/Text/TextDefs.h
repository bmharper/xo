#pragma once
#include "../Defs.h"

namespace xo {

#ifndef FT_FREETYPE_H
typedef void* FT_Face;
typedef void* FT_Library;
#endif

// Once initialized, all members are immutable
class XO_API Font {
public:
	FontID   ID;
	String   Facename;
	FT_Face  FTFace;
	int32_t  LinearHoriAdvance_Space_x256; // How much does character 32 advance?
	int32_t  LinearXHeight_x256;           // Height of the 'x' character
	int32_t  LineHeight_x256;              // Recommended spacing between consecutive lines
	int32_t  Ascender_x256;                // Ascender
	int32_t  Descender_x256;               // Descender
	uint32_t MaxAutoHinterSize;            // Maximum font size at which we force use of the auto hinter. Heuristic thumb-suck observations. Only applies to sub-pixel rendering.

	Font();
	~Font();
};
}
