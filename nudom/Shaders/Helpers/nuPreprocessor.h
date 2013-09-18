#pragma once

#include "nuDefs.h"

class NUAPI nuPreprocessor
{
public:
	fhashmap<nuString, nuString>	Macros;

	nuString Run( const char* raw );
};