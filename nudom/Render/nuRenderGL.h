#pragma once

#include "nuDefs.h"

struct NUAPI nuGLProg
{
	nuGLProg();

	GLuint Vert;
	GLuint Frag;
	GLuint Prog;
};

// Position, UV, Color
struct NUAPI nuVx_PTC
{
	// Note that nuRenderGL::DrawQuad assumes that we share our base layout with nuVx_PTCV4
	nuVec3	Pos;
	nuVec2	UV;
	uint32	Color;
};

// Position, UV, Color, Vec4
struct NUAPI nuVx_PTCV4
{
	// Note that nuRenderGL::DrawQuad assumes that we share our base layout with nuVx_PTC
	nuVec3	Pos;
	nuVec2	UV;
	uint32	Color;
	nuVec4	V4;
};

class NUAPI nuRenderGL
{
public:
	nuGLProg	PRect;
	nuGLProg	PFill;
	nuGLProg	PFillTex;
	nuGLProg	PTextRGB;
	nuGLProg	PTextWhole;
	nuGLProg	PCurve;

	GLint		VarRectMVProj;
	GLint		VarRectBox;
	GLint		VarRectRadius;
	GLint		VarRectVPortHSize;
	GLint		VarRectVColor;
	GLint		VarRectVPos;

	GLint		VarFillMVProj;
	GLint		VarFillVColor;
	GLint		VarFillVPos;

	GLint		VarFillTexMVProj;
	GLint		VarFillTexVColor;
	GLint		VarFillTexVPos;
	GLint		VarFillTexVUV;
	GLint		VarFillTex0;

	GLint		VarTextRGBMVProj;
	GLint		VarTextRGBVColor;
	GLint		VarTextRGBVPos;
	GLint		VarTextRGBVUV;
	GLint		VarTextRGBVClamp;
	GLint		VarTextRGBTex0;

	GLint		VarTextWholeMVProj;
	GLint		VarTextWholeVColor;
	GLint		VarTextWholeVPos;
	GLint		VarTextWholeVUV;
	GLint		VarTextWholeTex0;

	//GLint		VarRectCornerRadius;
	//GLint		VarCurveTex0;

					nuRenderGL();
					~nuRenderGL();
	bool			CreateShaders();
	void			DeleteShaders();
	void			SurfaceLost();
	void			ActivateProgram( nuGLProg& p );
	void			PreRender( int fbwidth, int fbheight );
	void			PostRenderCleanup();
	void			DrawQuad( const void* v );
	void			DrawTriangles( int nvert, const void* v, const uint16* indices );
	void			LoadTexture( const nuImage* img );
	void			LoadTextureAtlas( const nuTextureAtlas* atlas );

protected:
	nuGLProg*	ActiveProgram;
	int			FBWidth, FBHeight;
	GLuint		SingleTex2D;
	GLuint		SingleTexAtlas2D;

	void			DeleteProgram( nuGLProg& prog );
	bool			LoadProgram( nuGLProg& prog, const char* vsrc, const char* fsrc );
	bool			LoadProgram( GLuint& vshade, GLuint& fshade, GLuint& prog, const char* vsrc, const char* fsrc );
	bool			LoadShader( GLenum shaderType, GLuint& shader, const char* src );
	void			Check();
	void			Ortho( nuMat4f &imat, double left, double right, double bottom, double top, double znear, double zfar );
	void			Reset();

};

