#include "pch.h"
#include "TextDefs.h"

namespace xo {

Font::Font() {
	ID                           = FontIDNull;
	FTFace                       = NULL;
	LinearHoriAdvance_Space_x256 = 0;
	LinearXHeight_x256           = 0;
	LineHeight_x256              = 0;
	Ascender_x256                = 0;
	Descender_x256               = 0;
	// See FontStore::LoadFontTweaks for more details
	MaxAutoHinterSize = 0;
}

Font::~Font() {
}
}
