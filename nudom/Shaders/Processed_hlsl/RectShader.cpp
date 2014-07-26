#include "pch.h"
#if NU_BUILD_DIRECTX
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
"		float4		border;\n"
"		float4		border_color;\n"
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
"		float4		border;\n"
"		float4		border_color;\n"
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
"	// NOTE: This is a stupid way of doing rectangles.\n"
"	// Instead of rendering one big rectangle, we should be rendering it as 4 quadrants.\n"
"	// This \"one big rectangle\" approach forces you to evaluate all 4 corners for every pixel.\n"
"	// NOTE ALSO: This is broken, in the sense that it assumes a uniform border width.\n"
"	float4 main(VSOutput input) : SV_Target\n"
"	{\n"
"		float2 screenxy = frag_to_screen(input.pos.xy);\n"
"		float radius_out = radius;\n"
"		float4 mybox = box;\n"
"		float4 mycolor = input.color;\n"
"		float left_out   = mybox.x + radius_out;\n"
"		float right_out  = mybox.z - radius_out;\n"
"		float top_out    = mybox.y + radius_out;\n"
"		float bottom_out = mybox.w - radius_out;\n"
"	\n"
"		float radius_in  = max(border.x, radius); // This is why different border widths screw up.. because we're bound to border.left\n"
"	\n"
"		float left_in    = mybox.x + max(border.x, radius);\n"
"		float right_in   = mybox.z - max(border.z, radius);\n"
"		float top_in     = mybox.y + max(border.y, radius);\n"
"		float bottom_in  = mybox.w - max(border.w, radius);\n"
"	\n"
"		float2 cent_in = screenxy;\n"
"		float2 cent_out = screenxy;\n"
"		cent_in.x = clamp(cent_in.x, left_in, right_in);\n"
"		cent_in.y = clamp(cent_in.y, top_in, bottom_in);\n"
"		cent_out.x = clamp(cent_out.x, left_out, right_out);\n"
"		cent_out.y = clamp(cent_out.y, top_out, bottom_out);\n"
"	\n"
"		// If you draw the pixels out on paper, and take cognisance of the fact that\n"
"		// our samples are at pixel centers, then this -0.5 offset makes perfect sense.\n"
"		// This offset is correct regardless of whether you're blending linearly or in gamma space.\n"
"		// UPDATE: This is more subtle than it seems. By using a 0.5 offset here, and an additional 0.5 offset\n"
"		// that is fed into the shader's \"radius\" uniform, we effectively get rectangles to be sharp\n"
"		// when they are aligned to an integer grid. I haven't thought this through carefully enough,\n"
"		// but it does feel right.\n"
"		float dist_out = length(screenxy - cent_out) - 0.5f;\n"
"	\n"
"		float dist_in = length(screenxy - cent_in) - (radius_in - border.x);\n"
"	\n"
"		// I don't like this \"if (borderWidth > 0.5f)\". I feel like I'm doing something\n"
"		// wrong here, but I haven't drawn it out carefully enough to know what that something is.\n"
"		// When you rewrite this function to render quadrants, then perhaps fix this up too.\n"
"		float borderWidthX = max(border.x, border.z);\n"
"		float borderWidthY = max(border.y, border.w);\n"
"		float borderWidth = max(borderWidthX, borderWidthY);\n"
"		float borderMix = clamp(dist_in, 0.0f, 1.0f);\n"
"		if (borderWidth > 0.5f)\n"
"			mycolor = borderMix * border_color + (1 - borderMix) * mycolor;\n"
"	\n"
"		float4 output;\n"
"		output.rgb = mycolor.rgb;\n"
"		output.a = mycolor.a * clamp(radius_out - dist_out, 0.0f, 1.0f);\n"
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

#endif // NU_BUILD_DIRECTX

