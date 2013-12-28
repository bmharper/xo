#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include "../include/hlsl2glsl.h"

using namespace std;
typedef unsigned int uint;

static const size_t MaxFileSize = 1024 * 1024;

void showhelp();

int main(int argc, char** argv)
{
	EShLanguage type = EShLangCount;
	ETargetVersion target = ETargetVersionCount;
	for (int i = 1; i < argc; i++)
	{
		if (		strcmp(argv[i], "-v") == 0)		type = EShLangVertex;
		else if (	strcmp(argv[i], "-f") == 0)		type = EShLangFragment;
		else if (	strcmp(argv[i], "-es100") == 0)	target = ETargetGLSL_ES_100;
		else if (	strcmp(argv[i], "-es300") == 0)	target = ETargetGLSL_ES_300;
		else if (	strcmp(argv[i], "-gl100") == 0)	target = ETargetGLSL_110;
		else if (	strcmp(argv[i], "-gl120") == 0)	target = ETargetGLSL_120;
		else if (	strcmp(argv[i], "-gl140") == 0)	target = ETargetGLSL_140;
		else
		{
			showhelp();
			return 1;
		}
	}
	if (	type == EShLangCount ||
			target == ETargetVersionCount )
	{
		showhelp();
		return 1;
	}

	char* input = new char[MaxFileSize];
	size_t input_size = fread(input, 1, MaxFileSize, stdin);
	input[input_size] = 0;

	if (!Hlsl2Glsl_Initialize())
		return 1;

	auto compiler = Hlsl2Glsl_ConstructCompiler(EShLangVertex);

	bool success = false;
	uint options = ETranslateOpAvoidBuiltinAttribNames | ETranslateOpPropogateOriginalAttribNames;
	//uint options = ETranslateOpPropogateOriginalAttribNames;
	if (Hlsl2Glsl_Parse(compiler, input, target, options))
	{
		if (Hlsl2Glsl_Translate(compiler, "main", target, options))
		{
			success = true;
			fputs(Hlsl2Glsl_GetShader(compiler), stdout);
		}
		else
			printf("Translate failed\n");
	}
	else
		printf("Parse failed\n");

	if (!success)
		printf("%s", Hlsl2Glsl_GetInfoLog(compiler));

	Hlsl2Glsl_DestructCompiler(compiler);

	Hlsl2Glsl_Shutdown();
	return 0;
}

void showhelp()
{
	printf("hlslang <-f,-v> <-es100, -es300, -gl100 -gl120 -gl140>\n");
	printf(" -f fragment shader\n");
	printf(" -v fragment shader\n");
	printf(" -es100, ... target version\n");
	printf(" shader is read from stdin\n");
}