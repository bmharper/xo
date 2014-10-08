
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;	
};

#define LEFT x
#define TOP y
#define RIGHT z
#define BOTTOM w

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
	float radius_in = max(border.x, radius); // This is why different border widths screw up.. because we're bound to border.left
	float4 out_box = mybox + float4(radius_out, radius_out, -radius_out, -radius_out);
	float4 in_box = mybox + float4(max(border.LEFT, radius), max(border.TOP, radius), -max(border.RIGHT, radius), -max(border.BOTTOM, radius));

	float2 cent_in = screenxy;
	float2 cent_out = screenxy;
	cent_in.x = clamp(cent_in.x, in_box.LEFT, in_box.RIGHT);
	cent_in.y = clamp(cent_in.y, in_box.TOP, in_box.BOTTOM);
	cent_out.x = clamp(cent_out.x, out_box.LEFT, out_box.RIGHT);
	cent_out.y = clamp(cent_out.y, out_box.TOP, out_box.BOTTOM);

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
		mycolor = lerp(mycolor, border_color, borderMix);

	float4 output;
	output.rgb = mycolor.rgb;
	output.a = mycolor.a * clamp(radius_out - dist_out, 0.0f, 1.0f);
	return premultiply(output);
}
