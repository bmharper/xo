
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;	
};

// NOTE: This is a stupid way of doing rectangles.
// Instead of rendering one big rectangle, we should be rendering it as 4 quadrants.
// This "one big rectangle" approach forces you to evaluate all 4 corners for every pixel.
// NOTE ALSO: This is broken, in the sense that it assumes a uniform border width.
float4 main(VSOutput input) : SV_Target
{
	float2 screenxy = frag_to_screen(input.pos.xy);
	float radius_out = radius;
	float4 mybox = box;
	float4 mycolor = input.color;
	float left_out   = mybox.x + radius_out;
	float right_out  = mybox.z - radius_out;
	float top_out    = mybox.y + radius_out;
	float bottom_out = mybox.w - radius_out;

	float radius_in  = max(border.x, radius); // This is why different border widths screw up.. because we're bound to border.left

	float left_in    = mybox.x + max(border.x, radius);
	float right_in   = mybox.z - max(border.z, radius);
	float top_in     = mybox.y + max(border.y, radius);
	float bottom_in  = mybox.w - max(border.w, radius);

	float2 cent_in = screenxy;
	float2 cent_out = screenxy;
	cent_in.x = clamp(cent_in.x, left_in, right_in);
	cent_in.y = clamp(cent_in.y, top_in, bottom_in);
	cent_out.x = clamp(cent_out.x, left_out, right_out);
	cent_out.y = clamp(cent_out.y, top_out, bottom_out);

	// If you draw the pixels out on paper, and take cognisance of the fact that
	// our samples are at pixel centers, then this -0.5 offset makes perfect sense.
	// This offset is correct regardless of whether you're blending linearly or in gamma space.
	// UPDATE: This is more subtle than it seems. By using a 0.5 offset here, and an additional 0.5 offset
	// that is fed into the shader's "radius" uniform, we effectively get rectangles to be sharp
	// when they are aligned to an integer grid. I haven't thought this through carefully enough,
	// but it does feel right.
	float dist_out = length(screenxy - cent_out) - 0.5f;
	
	float dist_in = length(screenxy - cent_in) - (radius_in - border.x);

	// I don't like this "if (borderWidth > 0.5f)". I feel like I'm doing something
	// wrong here, but I haven't drawn it out carefully enough to know what that something is.
	// When you rewrite this function to render quadrants, then perhaps fix this up too.
	float borderWidthX = max(border.x, border.z);
	float borderWidthY = max(border.y, border.w);
	float borderWidth = max(borderWidthX, borderWidthY);
	float borderMix = clamp(dist_in, 0.0f, 1.0f);
	if (borderWidth > 0.5f)
		mycolor = borderMix * border_color + (1 - borderMix) * mycolor;

	float4 output;
	output.rgb = mycolor.rgb;
	output.a = mycolor.a * clamp(radius_out - dist_out, 0.0f, 1.0f);
	return output;
}
