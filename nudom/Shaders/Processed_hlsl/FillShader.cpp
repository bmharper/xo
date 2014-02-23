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
"		output.color = vcol;\n"
"		return output;\n"
"	}\n"
;
}

const char* nuDXProg_Fill::FragSrc()
{
	return
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


