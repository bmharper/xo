#pragma once

// These types must always be resolvable by the linker as simply what you see here. No templates, and no constructors.
// Basically the only important thing here is the memory layout.

struct vec2
{
	double x, y;
};

struct vec2f
{
	float x, y;
};

struct vec3
{
	double x, y, z;
};

struct vec3f
{
	float x, y, z;
};

struct vec4
{
	double x, y, z, w;
};

struct vec4f
{
	float x, y, z, w;
};

template<typename T>	bool vec_IsNaN( T v )			{ return v != v; }
inline					bool vec_IsFinite( float v )	{ return v <= FLT_MAX && v >= -FLT_MAX; }
inline					bool vec_IsFinite( double v )	{ return v <= DBL_MAX && v >= -DBL_MAX; }
