#include "pch.h"
#include "Color.h"
#include "Strings/strings.h"

PAPI bool ParseWebColorBGRA( LPCSTR s, u32& bgra )
{
	// rrggbbaa		8
	// rrggbb		6
	// rgba			4
	// rgb			3
	
	if ( !s ) return false;

	ColorBGRA c;

	if ( s[0] == '#' ) s++;
	auto len = strlen(s);
	if ( len == 8 )
	{
		c.r = ParseHexByteNoCheck( s + 0 );
		c.g = ParseHexByteNoCheck( s + 2 );
		c.b = ParseHexByteNoCheck( s + 4 );
		c.a = ParseHexByteNoCheck( s + 6 );
		bgra = c.u;
		return true;
	}
	else if ( len == 6 )
	{
		c.r = ParseHexByteNoCheck( s + 0 );
		c.g = ParseHexByteNoCheck( s + 2 );
		c.b = ParseHexByteNoCheck( s + 4 );
		c.a = 255;
		bgra = c.u;
		return true;
	}
	else if ( len == 4 )
	{
		c.r = ParseHexCharNoCheck( s[0] );
		c.g = ParseHexCharNoCheck( s[1] );
		c.b = ParseHexCharNoCheck( s[2] );
		c.a = ParseHexCharNoCheck( s[3] );
		c.r = c.r << 4 | c.r;
		c.g = c.g << 4 | c.g;
		c.b = c.b << 4 | c.b;
		c.a = c.a << 4 | c.a;
		bgra = c.u;
		return true;
	}
	else if ( len == 3 )
	{
		c.r = ParseHexCharNoCheck( s[0] );
		c.g = ParseHexCharNoCheck( s[1] );
		c.b = ParseHexCharNoCheck( s[2] );
		c.a = 255;
		c.r = c.r << 4 | c.r;
		c.g = c.g << 4 | c.g;
		c.b = c.b << 4 | c.b;
		bgra = c.u;
		return true;
	}
	return false;
}

// This version comes from http://lol.zoy.org/blog/2013/01/13/fast-rgb-to-hsv
// Although this code is not used inside this project, I just had to include
// the function so that I could be at peace with closing the browser tab.
static void RGB2HSV_Fast_Unclamped(	float r, float g, float b,
									float &h, float &s, float &v)
{
    float K = 0.f;

    if (g < b)
    {
        std::swap(g, b);
        K = -1.f;
    }

    if (r < g)
    {
        std::swap(r, g);
        K = -2.f / 6.f - K;
    }

    float chroma = r - min(g, b);
    h = fabs(K + (g - b) / (6.f * chroma + 1e-20f));
    s = chroma / (r + 1e-20f);
    v = r;
}

void RGBtoHSV( const Vec3f& rgb, Vec3f& hsv )
{
	float vmin, vmax, delta;

	float r = CLAMP( rgb.x, 0, 1 );
	float g = CLAMP( rgb.y, 0, 1 );
	float b = CLAMP( rgb.z, 0, 1 );
	// no more use of 'rgb' since it can be aliased with 'hsv'
	float& h = hsv.x;
	float& s = hsv.y;
	float& v = hsv.z;

	vmin = min(r,g);
	vmin = min(vmin,b);

	vmax = max(r,g);
	vmax = max(vmax,b);

	v = vmax;		// v

	delta = vmax - vmin;

	if( vmax != 0 )
		s = delta / vmax;	// s
	else 
	{
		// r = g = b = 0	// s = 0, v is undefined
		s = 1;
		h = 1;
		v = 0;
		return;
	}

	if( r == vmax )
	{
		if ( delta == 0 )
			h = 0; // r = g = b, h is undefined
		else
			h = ( g - b ) / delta;		// between yellow & magenta
	}
	else if( g == vmax )
		h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		h = 4 + ( r - g ) / delta;	// between magenta & cyan

	h /= 6;
	if ( h < 0 ) h += 1;

	//*h *= 60;			// degrees
	//if( *h < 0 )
	//	*h += 360;
}

void HSVtoRGB( const Vec3f& hsv, Vec3f& rgb )
{
	int i;
	float f, p, q, t;

	float h = 6 * CLAMP( hsv.x, 0, 1 );
	float s = CLAMP( hsv.y, 0, 1 );
	float v = CLAMP( hsv.z, 0, 1 );
	// no more use of 'hsv' since it can be aliased with 'rgb'
	float& r = rgb.x;
	float& g = rgb.y;
	float& b = rgb.z;

	if( s == 0 ) 
	{
		// achromatic (grey)
		r = g = b = v;
		return;
	}

	//h /= 60;		// sector 0 to 5
	i = floor( h );
	f = h - i;		// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) 
	{
		case 0:
		case 6:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		default:	// case 5:
			r = v;
			g = p;
			b = q;
			break;
	}
}

uint32 PAPI BGRAtoHSVA( uint32 bgra )
{
	const float m = float(1.0 / 255.0);
	ColorBGRA p(bgra);
	Vec3f hsv;
	RGBtoHSV( Vec3f(p.r * m, p.g * m, p.b * m), hsv );
	ColorHSVA c( hsv.x * 255, hsv.y * 255, hsv.z * 255, p.a );
	return c.u;
}

uint32 PAPI HSVAtoBGRA( uint32 hsva )
{
	const float m = float(1.0 / 255.0);
	ColorHSVA src( hsva );
	Vec3f rgb;
	HSVtoRGB( Vec3f( src.h * m, src.s * m, src.v * m ), rgb );
	ColorBGRA p( rgb.x * 255, rgb.y * 255, rgb.z * 255, src.a );
	return p.u;
}

void PAPI HSVAtoBGRA( const Vec4f& hsva, uint32& bgra )
{
	Vec3f rgb;
	HSVtoRGB( hsva.vec3, rgb );
	bgra = ColorBGRA( rgb.x * 255, rgb.y * 255, rgb.z * 255, hsva.w * 255 ).u;
}

void PAPI RGBAtoHSVA( const Vec4f& rgba, uint32& hsva )
{
	Vec3f hsv;
	RGBtoHSV( rgba.vec3, hsv );
	hsva = ColorHSVA( hsv.x * 255, hsv.y * 255, hsv.z * 255, rgba.a * 255 ).u;
}
