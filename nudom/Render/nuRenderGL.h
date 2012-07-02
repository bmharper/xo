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
	nuVec3	Pos;
	nuVec2	UV;
	uint32	Color;
};

class NUAPI nuRenderGL
{
public:
	nuGLProg	PFill;
	nuGLProg	PRect;
	nuGLProg	PCurve;

	GLint		VarRectBox;
	GLint		VarRectRadius;
	GLint		VarRectVPortHSize;
	GLint		VarRectMVProj;
	GLint		VarRectVColor;
	GLint		VarRectVPos;

	GLint		VarFillMVProj;
	GLint		VarFillVColor;
	GLint		VarFillVPos;

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

protected:
	nuGLProg*	ActiveProgram;
	int			FBWidth, FBHeight;

	void			DeleteProgram( nuGLProg& prog );
	bool			LoadProgram( nuGLProg& prog, const char* vsrc, const char* fsrc );
	bool			LoadProgram( GLuint& vshade, GLuint& fshade, GLuint& prog, const char* vsrc, const char* fsrc );
	bool			LoadShader( GLenum shaderType, GLuint& shader, const char* src );
	void			Check();
	void			Ortho( Mat4f &imat, double left, double right, double bottom, double top, double znear, double zfar );
	void			Reset();

};

