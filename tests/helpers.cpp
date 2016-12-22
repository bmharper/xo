#include "pch.h"
#include "helpers.h"

xo::String LoadFileAsString(const char* file)
{
	xo::String result;
	FILE* f = fopen(file, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		size_t length = ftell(f);
		result.Resize(length);
		fseek(f, 0, SEEK_SET);
		TTASSERT(fread(result.Z, 1, length, f) == length);
		fclose(f);
	}
	return result;
}
