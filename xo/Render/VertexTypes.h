#pragma once
namespace xo {

// Position, UV, Color
struct XO_API Vx_PTC {
	// Note that RenderGL::DrawQuad assumes that Vx_PTC and Vx_PTCV4 share their base layout
	VecBase3f Pos;
	VecBase2f UV;
	uint32_t  Color;
};

// Position, UV, Color, Color2, Vec4
struct XO_API Vx_PTCV4 {
	// Note that RenderGL::DrawQuad assumes that Vx_PTC and Vx_PTCV4 share their base layout
	union {
		struct
		{
			VecBase3f Pos;
			VecBase2f UV;
			uint32_t  Color;
		};
		Vx_PTC PTC;
	};
	uint32_t  Color2;
	VecBase4f V4;

	// Set the entire vertex
	void Set(VecBase3f pos, VecBase2f uv, uint32_t color, uint32_t color2, VecBase4f v4) {
		Pos    = pos;
		UV     = uv;
		Color  = color;
		Color2 = color2;
		V4     = v4;
	}
};

struct XO_API Vx_Uber {
	VecBase2f Pos;
	VecBase4f UV1;
	VecBase4f UV2;
	uint32_t  Color1;
	uint32_t  Color2;
	uint32_t  Shader;

	// Set everything except for UV2
	void Set1(uint32_t shader, VecBase2f pos, VecBase4f uv1, uint32_t color1, uint32_t color2) {
		Pos    = pos;
		UV1    = uv1;
		Color1 = color1;
		Color2 = color2;
		Shader = shader;
	}

	// Set everything
	void Set(uint32_t shader, VecBase2f pos, VecBase4f uv1, VecBase4f uv2, uint32_t color1, uint32_t color2) {
		Pos    = pos;
		UV1    = uv1;
		UV2    = uv2;
		Color1 = color1;
		Color2 = color2;
		Shader = shader;
	}
};

enum VertexType {
	VertexType_NULL,
	VertexType_PTC,
	VertexType_PTCV4,
	VertexType_Uber,
	VertexType_END,
};

XO_API size_t VertexSize(VertexType t);
}
