#pragma once

// Position, UV, Color
struct XOAPI xoVx_PTC
{
	// Note that xoRenderGL::DrawQuad assumes that we share our base layout with xoVx_PTCV4
	xoVecBase3f		Pos;
	xoVecBase2f		UV;
	uint32			Color;
};

// Position, UV, Color, Color2, Vec4
struct XOAPI xoVx_PTCV4
{
	// Note that xoRenderGL::DrawQuad assumes that we share our base layout with xoVx_PTC
	union
	{
		struct
		{
			xoVecBase3f		Pos;
			xoVecBase2f		UV;
			uint32			Color;
		};
		xoVx_PTC PTC;
	};
	uint32			Color2;
	xoVecBase4f		V4;
};

enum xoVertexType
{
	xoVertexType_NULL,
	xoVertexType_PTC,
	xoVertexType_PTCV4,
	xoVertexType_END,
};

XOAPI size_t xoVertexSize(xoVertexType t);