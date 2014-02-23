
cbuffer PerFrame : register(b0)
{
	float4x4	mvproj;
	float2		vport_hsize;
};

cbuffer PerObject : register(b1)
{
	float4		box;
	float		radius;
};

struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;	
};

float2 to_screen(float2 unit_pt)
{
	//return (float2(unit_pt.x, -unit_pt.y) + float2(1,1)) * float2(700, 300);
	//return (float2(unit_pt.x, -unit_pt.y) + float2(1,1)) * vport_hsize;
	return unit_pt;
}

float4 main(VSOutput input) : SV_Target
{
	float2 screenxy = to_screen(input.pos.xy);
	float myradius = radius;
	//myradius = 10;
	float4 mybox = box;
	//mybox.x = 0;
	//mybox.z = 100;
	//mybox.y = 0;
	//mybox.w = 100;
	float left = mybox.x + myradius;
	float right = mybox.z - myradius;
	float top = mybox.y + myradius;
	float bottom = mybox.w - myradius;
	float2 cent = screenxy;

	cent.x = clamp(cent.x, left, right);
	cent.y = clamp(cent.y, top, bottom);

	// If you draw the pixels out on paper, and take cognisance of the fact that
	// our samples are at pixel centers, then this -0.5 offset makes perfect sense.
	// This offset is correct regardless of whether you're blending linearly or in gamma space.
	// UPDATE: This is more subtle than it seems. By using a 0.5 offset here, and an additional 0.5 offset
	// that is fed into the shader's "radius" uniform, we effectively get rectangles to be sharp
	// when they are aligned to an integer grid. I haven't thought this through carefully enough,
	// but it does feel right.
	float dist = length(screenxy - cent) - 0.5f;

	float4 output;
	output.rgb = input.color.rgb;
	//output.r = dist * (1.0f / 100.0f);
	//output.r = myradius / 3.0f;
	//output.r = 0.99 * (vport_hsize.y / 712.0f);
	//output.r = clamp(input.pos.x / 100.0f, 0, 1);
	//output.g = 0;
	//output.b = 0;
	output.a = input.color.a * clamp(myradius - dist, 0.0, 1.0);
	//output.a = 1.0f;
	return output;
}
