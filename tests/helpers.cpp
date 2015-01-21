#include "pch.h"
#include "helpers.h"

xoString LoadFileAsString(const char* file)
{
	xoString result;
	FILE* f = fopen(file, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		size_t length = ftell(f);
		result.Resize((intp) length);
		fseek(f, 0, SEEK_SET);
		TTASSERT(fread(result.Z, 1, length, f) == length);
		fclose(f);
	}
	return result;
}
