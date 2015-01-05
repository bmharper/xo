#pragma once

// Position, UV, Color
struct XOAPI xoVx_PTC
{
	// Note that xoRenderGL::DrawQuad assumes that we share our base layout with xoVx_PTCV4
	xoVec3f		Pos;
	xoVec2f		UV;
	uint32		Color;
};

// Position, UV, Color, Vec4
struct XOAPI xoVx_PTCV4
{
	// Note that xoRenderGL::DrawQuad assumes that we share our base layout with xoVx_PTC
	xoVec3f		Pos;
	xoVec2f		UV;
	uint32		Color;
	xoVec4f		V4;
};

enum xoVertexType
{
	xoVertexType_NULL,
	xoVertexType_PTC,
	xoVertexType_PTCV4,
	xoVertexType_END,
};

XOAPI size_t xoVertexSize(xoVertexType t);