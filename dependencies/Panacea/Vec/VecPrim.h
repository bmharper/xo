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
