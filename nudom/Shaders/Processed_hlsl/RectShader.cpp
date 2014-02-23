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
"		float4 pos		: SV_Position;\n"
"		float4 color	: COLOR;\n"
"	};\n"
"	\n"
"	VSOutput main(float4 vpos : POSITION, float4 vcol : COLOR)\n"
"	{\n"
"		VSOutput output;\n"
"		output.pos = mul(mvproj, vpos);\n"
"		output.color.rgb = pow(abs(vcol.rgb), float3(2.2, 2.2, 2.2));\n"
"		output.color.a = vcol.a;\n"
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
"		float4 pos		: SV_Position;\n"
"		float4 color	: COLOR;\n"
"	};\n"
"	\n"
"	float2 to_screen(float2 unit_pt)\n"
"	{\n"
"		//return (float2(unit_pt.x, -unit_pt.y) + float2(1,1)) * float2(700, 300);\n"
"		//return (float2(unit_pt.x, -unit_pt.y) + float2(1,1)) * vport_hsize;\n"
"		return unit_pt;\n"
"	}\n"
"	\n"
"	float4 main(VSOutput input) : SV_Target\n"
"	{\n"
"		float2 screenxy = to_screen(input.pos.xy);\n"
"		float myradius = radius;\n"
"		//myradius = 10;\n"
"		float4 mybox = box;\n"
"		//mybox.x = 0;\n"
"		//mybox.z = 100;\n"
"		//mybox.y = 0;\n"
"		//mybox.w = 100;\n"
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
"		//output.r = dist * (1.0f / 100.0f);\n"
"		//output.r = myradius / 3.0f;\n"
"		//output.r = 0.99 * (vport_hsize.y / 712.0f);\n"
"		//output.r = clamp(input.pos.x / 100.0f, 0, 1);\n"
"		//output.g = 0;\n"
"		//output.b = 0;\n"
"		output.a = input.color.a * clamp(myradius - dist, 0.0, 1.0);\n"
"		//output.a = 1.0f;\n"
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


