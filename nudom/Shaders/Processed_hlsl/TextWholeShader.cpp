#include "pch.h"
#include "TextWholeShader.h"

nuDXProg_TextWhole::nuDXProg_TextWhole()
{
	Reset();
}

void nuDXProg_TextWhole::Reset()
{
	ResetBase();

}

const char* nuDXProg_TextWhole::VertSrc()
{
	return
"	\n"
"	cbuffer PerFrame : register(b0)\n"
"	{\n"
"		float4x4	mvproj;\n"
"		float2		vport_hsize;\n"
"	};\n"
"	\n"
"	cbuffer PerObject : register(b1)\n"
"	{\n"
"		float4		box;\n"
"		float		radius;\n"
"	};\n"
"	\n"
"	struct VSOutput\n"
"	{\n"
"		float4 pos		: SV_POSITION;\n"
"		float4 color	: COLOR;\n"
"		float2 texuv0	: TEXCOORD0;\n"
"	};\n"
"	\n"
"	VSOutput main(float4 vpos : POSITION, float4 vcol : COLOR, float2 vtexuv0 : TEXCOORD0)\n"
"	{\n"
"		VSOutput output;\n"
"		output.pos = mul(mvproj, vpos);\n"
"		output.color = vcol;\n"
"		output.texuv0 = vtexuv0;\n"
"		return output;\n"
"	}\n"
;
}

const char* nuDXProg_TextWhole::FragSrc()
{
	return
"	\n"
"	cbuffer PerFrame : register(b0)\n"
"	{\n"
"		float4x4		mvproj;\n"
"		float2			vport_hsize;\n"
"	\n"
"		Texture2D		shader_texture;\n"
"		SamplerState	sample_type;\n"
"	};\n"
"	\n"
"	cbuffer PerObject : register(b1)\n"
"	{\n"
"		float4		box;\n"
"		float		radius;\n"
"	};\n"
"	\n"
"	struct VSOutput\n"
"	{\n"
"		float4 pos		: SV_POSITION;\n"
"		float4 color	: COLOR;\n"
"		float2 texuv0	: TEXCOORD0;\n"
"	};\n"
"	\n"
"	float4 main(VSOutput input) : SV_Target\n"
"	{\n"
"		float4 col;\n"
"		col = input.color * float4(1,1,1, shader_texture.Sample(sample_type, input.texuv0).r);\n"
"	    return col;\n"
"	}\n"
;
}

const char* nuDXProg_TextWhole::Name()
{
	return "TextWhole";
}


bool nuDXProg_TextWhole::LoadVariablePositions()
{
	int nfail = 0;

	if ( nfail != 0 )
		NUTRACE( "Failed to bind %d variables of shader TextWhole\n", nfail );

	return nfail == 0;
}

uint32 nuDXProg_TextWhole::PlatformMask()
{
	return nuPlatform_All;
}


