#include "pch.h"
#include "TextRGBShader.h"

nuDXProg_TextRGB::nuDXProg_TextRGB()
{
	Reset();
}

void nuDXProg_TextRGB::Reset()
{
	ResetBase();

}

const char* nuDXProg_TextRGB::VertSrc()
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
"		float2 texClamp;\n"
"	};\n"
"	\n"
"	VSOutput main(float4 vpos : POSITION, float4 vcol : COLOR, float2 vtexuv0 : TEXCOORD0, float4 vtexClamp)\n"
"	{\n"
"		VSOutput output;\n"
"		output.pos = mul(mvproj, vpos);\n"
"		output.color.rgb = pow(abs(vcol.rgb), float3(2.2, 2.2, 2.2));\n"
"		output.color.a = vcol.a;\n"
"		output.texuv0 = vtexuv0;\n"
"		output.texClamp = vtexClamp;\n"
"		return output;\n"
"	}\n"
;
}

const char* nuDXProg_TextRGB::FragSrc()
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
"		float2 texClamp;\n"
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

const char* nuDXProg_TextRGB::Name()
{
	return "TextRGB";
}


bool nuDXProg_TextRGB::LoadVariablePositions()
{
	int nfail = 0;

	if ( nfail != 0 )
		NUTRACE( "Failed to bind %d variables of shader TextRGB\n", nfail );

	return nfail == 0;
}

uint32 nuDXProg_TextRGB::PlatformMask()
{
	return nuPlatform_All;
}


