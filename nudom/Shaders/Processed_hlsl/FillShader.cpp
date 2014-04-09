#include "pch.h"
#include "FillShader.h"

nuDXProg_Fill::nuDXProg_Fill()
{
	Reset();
}

void nuDXProg_Fill::Reset()
{
	ResetBase();

}

const char* nuDXProg_Fill::VertSrc()
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
"	struct VertexType_PTC\n"
"	{\n"
"		float4 pos		: POSITION;\n"
"		float2 uv		: TEXCOORD0;\n"
"		float4 color	: COLOR;\n"
"	};\n"
"	\n"
"	struct VertexType_PTCV4\n"
"	{\n"
"		float4 pos		: POSITION;\n"
"		float2 uv		: TEXCOORD1;\n"
"		float4 color	: COLOR;\n"
"		float4 v4		: TEXCOORD2;\n"
"	};\n"
"	\n"
"	float4 vertex_color_in(float4 vertex_color)\n"
"	{\n"
"		float4 color;\n"
"		color.rgb = pow(abs(vertex_color.rgb), float3(2.2, 2.2, 2.2));\n"
"		color.a = vertex_color.a;\n"
"		return color;\n"
"	}\n"
"	\n"
"	// SV_Position is in screen space, but in GLSL it is in normalized device space\n"
"	float2 frag_to_screen(float2 unit_pt)\n"
"	{\n"
"		return unit_pt;\n"
"	}\n"
"	\n"
"	struct VSOutput\n"
"	{\n"
"		float4 pos		: SV_Position;\n"
"		float4 color	: COLOR;\n"
"	};\n"
"	\n"
"	VSOutput main(VertexType_PTC vertex)\n"
"	{\n"
"		VSOutput output;\n"
"		output.pos = mul(mvproj, vertex.pos);\n"
"		output.color = vertex_color_in(vertex.color);\n"
"		return output;\n"
"	}\n"
;
}

const char* nuDXProg_Fill::FragSrc()
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
"	struct VertexType_PTC\n"
"	{\n"
"		float4 pos		: POSITION;\n"
"		float2 uv		: TEXCOORD0;\n"
"		float4 color	: COLOR;\n"
"	};\n"
"	\n"
"	struct VertexType_PTCV4\n"
"	{\n"
"		float4 pos		: POSITION;\n"
"		float2 uv		: TEXCOORD1;\n"
"		float4 color	: COLOR;\n"
"		float4 v4		: TEXCOORD2;\n"
"	};\n"
"	\n"
"	float4 vertex_color_in(float4 vertex_color)\n"
"	{\n"
"		float4 color;\n"
"		color.rgb = pow(abs(vertex_color.rgb), float3(2.2, 2.2, 2.2));\n"
"		color.a = vertex_color.a;\n"
"		return color;\n"
"	}\n"
"	\n"
"	// SV_Position is in screen space, but in GLSL it is in normalized device space\n"
"	float2 frag_to_screen(float2 unit_pt)\n"
"	{\n"
"		return unit_pt;\n"
"	}\n"
"	\n"
"	struct VSOutput\n"
"	{\n"
"		float4 pos		: SV_Position;\n"
"		float4 color	: COLOR;\n"
"	};\n"
"	\n"
"	float4 main(VSOutput input) : SV_Target\n"
"	{\n"
"	    return input.color;\n"
"	}\n"
;
}

const char* nuDXProg_Fill::Name()
{
	return "Fill";
}


bool nuDXProg_Fill::LoadVariablePositions()
{
	int nfail = 0;

	if ( nfail != 0 )
		NUTRACE( "Failed to bind %d variables of shader Fill\n", nfail );

	return nfail == 0;
}

uint32 nuDXProg_Fill::PlatformMask()
{
	return nuPlatform_All;
}

nuVertexType nuDXProg_Fill::VertexType()
{
	return nuVertexType_PTC;
}


