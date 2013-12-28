float4x4	mvproj;

void main(float4 vpos : POSITION, out float4 outpos : POSITION, float4 vcol : COLOR, out float4 outcol : COLOR)
{
	//gl_Position = mvproj * vpos;
	//color = vcolor;
	outpos = mul(mvproj, vpos);
	outcol = vcol;
}
