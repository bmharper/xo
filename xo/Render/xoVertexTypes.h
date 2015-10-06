#pragma once

// Position, UV, Color
struct XOAPI xoVx_PTC
{
	// Note that xoRenderGL::DrawQuad assumes that xoVx_PTC and xoVx_PTCV4 share their base layout
	xoVecBase3f		Pos;
	xoVecBase2f		UV;
	uint32			Color;
};

// Position, UV, Color, Color2, Vec4
struct XOAPI xoVx_PTCV4
{
	// Note that xoRenderGL::DrawQuad assumes that xoVx_PTC and xoVx_PTCV4 share their base layout
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

	// Set the entire vertex
	void Set(xoVecBase3f pos, xoVecBase2f uv, uint32 color, uint32 color2, xoVecBase4f v4)
	{
		Pos = pos;
		UV = uv;
		Color = color;
		Color2 = color2;
		V4 = v4;
	}
};

enum xoVertexType
{
	xoVertexType_NULL,
	xoVertexType_PTC,
	xoVertexType_PTCV4,
	xoVertexType_END,
};

XOAPI size_t xoVertexSize(xoVertexType t);