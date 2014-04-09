#include "pch.h"
#include "RectShader.h"

nuDXProg_Rect::nuDXProg_Rect()
{
	Reset();
}

void nuDXProg_Rect::Reset()
{
	ResetBase();

}

const char* nuDXProg_Rect::VertSrc()
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

const char* nuDXProg_Rect::FragSrc()
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
"		float2 screenxy = frag_to_screen(input.pos.xy);\n"
"		float myradius = radius;\n"
"		float4 mybox = box;\n"
"		float left = mybox.x + myradius;\n"
"		float right = mybox.z - myradius;\n"
"		float top = mybox.y + myradius;\n"
"		float bottom = mybox.w - myradius;\n"
"		float2 cent = screenxy;\n"
"	\n"
"		cent.x = clamp(cent.x, left, right);\n"
"		cent.y = clamp(cent.y, top, bottom);\n"
"	\n"
"		// If you draw the pixels out on paper, and take cognisance of the fact that\n"
"		// our samples are at pixel centers, then this -0.5 offset makes perfect sense.\n"
"		// This offset is correct regardless of whether you're blending linearly or in gamma space.\n"
"		// UPDATE: This is more subtle than it seems. By using a 0.5 offset here, and an additional 0.5 offset\n"
"		// that is fed into the shader's \"radius\" uniform, we effectively get rectangles to be sharp\n"
"		// when they are aligned to an integer grid. I haven't thought this through carefully enough,\n"
"		// but it does feel right.\n"
"		float dist = length(screenxy - cent) - 0.5f;\n"
"	\n"
"		float4 output;\n"
"		output.rgb = input.color.rgb;\n"
"		output.a = input.color.a * clamp(myradius - dist, 0.0, 1.0);\n"
"		return output;\n"
"	}\n"
;
}

const char* nuDXProg_Rect::Name()
{
	return "Rect";
}


bool nuDXProg_Rect::LoadVariablePositions()
{
	int nfail = 0;

	if ( nfail != 0 )
		NUTRACE( "Failed to bind %d variables of shader Rect\n", nfail );

	return nfail == 0;
}

uint32 nuDXProg_Rect::PlatformMask()
{
	return nuPlatform_All;
}

nuVertexType nuDXProg_Rect::VertexType()
{
	return nuVertexType_PTC;
}


