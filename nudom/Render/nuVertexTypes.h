#pragma once

// Position, UV, Color
struct NUAPI nuVx_PTC
{
	// Note that nuRenderGL::DrawQuad assumes that we share our base layout with nuVx_PTCV4
	nuVec3f		Pos;
	nuVec2f		UV;
	uint32		Color;
};

// Position, UV, Color, Vec4
struct NUAPI nuVx_PTCV4
{
	// Note that nuRenderGL::DrawQuad assumes that we share our base layout with nuVx_PTC
	nuVec3f		Pos;
	nuVec2f		UV;
	uint32		Color;
	nuVec4f		V4;
};

enum nuVertexType
{
	nuVertexType_NULL,
	nuVertexType_PTC,
	nuVertexType_PTCV4,
	nuVertexType_END,
};