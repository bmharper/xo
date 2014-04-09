
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;	
};

float4 main(VSOutput input) : SV_Target
{
	float2 screenxy = frag_to_screen(input.pos.xy);
	float myradius = radius;
	float4 mybox = box;
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
	output.a = input.color.a * clamp(myradius - dist, 0.0, 1.0);
	return output;
}
