#pragma once

#include "nuDefs.h"

class NUAPI nuFont
{
public:
	nuFontID	ID;
	nuString	Facename;
	FT_Face		FTFace;

			nuFont();
			~nuFont();
};