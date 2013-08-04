#pragma once

#include "../Other/lmDefs.h"
#include "../Vec/Vec3.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) // CRT unsafe
#endif

typedef unsigned char uint8;
typedef unsigned int  uint32;

// The worst round-trip error you can have from RGB -> HSV -> RGB is a delta of 6 (in one channel).
uint32 PAPI BGRAtoHSVA( uint32 bgra );
uint32 PAPI HSVAtoBGRA( uint32 hsva );

void PAPI HSVAtoBGRA( const Vec4f& hsva, uint32& bgra );
void PAPI RGBAtoHSVA( const Vec4f& rgba, uint32& hsva );

void PAPI RGBtoHSV( const Vec3f& rgb, Vec3f& hsv );		// rgb and hsv are allowed to be aliases
void PAPI HSVtoRGB( const Vec3f& hsv, Vec3f& rgb );		// rgb and hsv are allowed to be aliases

PAPI bool ParseWebColorBGRA( LPCSTR s, u32& bgra );

#define WIN32_RGB(r,g,b)          ((UINT32)(((BYTE)(r)|((UINT32)((BYTE)(g))<<8))|(((UINT32)(BYTE)(b))<<16)))

struct ColorHSVA
{

	ColorHSVA() : u()				{}
	ColorHSVA( uint32 i ) : u(i)	{}

	ColorHSVA( uint8 _h, uint8 _s, uint8 _v, uint8 _a ) { h = _h; s = _s; v = _v; a = _a; }
	void Set ( uint8 _h, uint8 _s, uint8 _v, uint8 _a ) { h = _h; s = _s; v = _v; a = _a; }

	ColorHSVA& operator=( uint32 i ) { u = i; return *this; }

	static ColorHSVA White() { return ColorHSVA(255,255,255,255); }
	static ColorHSVA Empty() { return ColorHSVA(0,0,0,0); }
	static ColorHSVA Black() { return ColorHSVA(0,0,0,255); }

	union {
		struct {
			// Apparently Michael Herf has tested this and found it to have optimal implementations on a wide range of compilers and architectures.
			// (This was originally from BGRA pixel format)
#if ENDIANLITTLE
			uint8 h, s, v, a;
#else
			uint8 a: 8;
			uint8 v: 8;
			uint8 s: 8;
			uint8 h: 8;
#endif
		};
		uint32 u;
	};
};

struct ColorBGRA
{

	// Note that all of our methods/constructors/etc take the components in RGBA order. This is an attempt to abstract the color format.
	ColorBGRA() : u()				{}
	ColorBGRA( uint32 i ) : u(i)	{}

	ColorBGRA( uint8 _r, uint8 _g, uint8 _b, uint8 _a ) { r = _r; g = _g; b = _b; a = _a; }
	ColorBGRA( Vec3f rgb )								{ r = rgb.x * 255; g = rgb.y * 255; b = rgb.z * 255; a = 255; }

	void Set ( uint8 _r, uint8 _g, uint8 _b, uint8 _a ) { r = _r; g = _g; b = _b; a = _a; }

	ColorBGRA&	operator=( uint32 c )			{ u = c; return *this; }
	ColorBGRA&	operator=(  Vec3f rgb  )		{ r = rgb.x * 255; g = rgb.y * 255; b = rgb.z * 255; a = 255;  return *this; }
	bool		operator==( ColorBGRA c ) const	{ return u == c.u; }
	bool		operator!=( ColorBGRA c )	const	{ return u != c.u; }

	operator	Vec3f() const	{ return Vec3f( r / 255.0f, g / 255.0f, b / 255.0f );}

	static ColorBGRA White() { return ColorBGRA(255,255,255,255); }
	static ColorBGRA Empty() { return ColorBGRA(0,0,0,0); }
	static ColorBGRA Black() { return ColorBGRA(0,0,0,255); }

#ifdef _GDIPLUS_H
	ColorBGRA( Gdiplus::Color c ) { r = c.GetR(); g = c.GetG(); b = c.GetB(); a = c.GetA(); }
	operator Gdiplus::Color() const { return Gdiplus::Color(a, r, g, b); }
#endif

	// dst is 9 characters including the terminator (ie we don't write a hash at the front). Returns dst back to you.
	char*	ToHexRGBA( char (&dst)[9] )		{ sprintf(dst, "%02X%02X%02X%02X", r,g,b,a); return dst; }
	bool	ParseWebColor( LPCSTR s )		{ return ParseWebColorBGRA( s, u ); }

	union {
		struct {
			// Apparently Michael Herf has tested this and found it to have optimal implementations on a wide range of compilers and architectures.
			// (This was originally from BGRA pixel format)
#if ENDIANLITTLE
			uint8 b, g, r, a;
#else
			uint8 a: 8;
			uint8 r: 8;
			uint8 g: 8;
			uint8 b: 8;
#endif
		};
		uint32 u;
	};
};

inline bool ParseWebColor( LPCSTR s, ColorBGRA& c ) { return c.ParseWebColor(s); }

inline Vec3f RGBtoHSV( const Vec3f& rgb )
{
	Vec3f hsv;
	RGBtoHSV( rgb, hsv );
	return hsv;
}

inline Vec3f HSVtoRGB( const Vec3f& hsv )
{
	Vec3f rgb;
	HSVtoRGB( hsv, rgb );
	return rgb;
}

inline void HSVtoRGB( const Vec3f& hsv, UINT& rgb )
{
	Vec3f q;
	HSVtoRGB( hsv, q );
	rgb = WIN32_RGB( (BYTE) (q.x * 255), (BYTE) (q.y * 255), (BYTE) (q.z * 255) );
}

inline void RGBtoHSV( float r, float g, float b, float& h, float& s, float& v )
{
	Vec3f rgb( r, g, b );
	Vec3f hsv( h, s, v );
	RGBtoHSV( rgb, hsv );
	h = hsv.x;
	s = hsv.y;
	v = hsv.z;
}

inline void HSVtoRGB( float h, float s, float v, float& r, float& g, float& b )
{
	Vec3f rgb( r, g, b );
	Vec3f hsv( h, s, v );
	HSVtoRGB( hsv, rgb );
	r = rgb.x;
	g = rgb.y;
	b = rgb.z;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
