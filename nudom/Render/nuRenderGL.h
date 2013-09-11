#pragma once

#include "nuRenderGL_Defs.h"
#include "../Shaders/Processed/CurveShader.h"
#include "../Shaders/Processed/FillShader.h"
#include "../Shaders/Processed/FillTexShader.h"
#include "../Shaders/Processed/RectShader.h"
#include "../Shaders/Processed/TextRGBShader.h"
#include "../Shaders/Processed/TextWholeShader.h"

class NUAPI nuRenderGL
{
public:
	nuGLProg_Rect		PRect;
	nuGLProg_Fill		PFill;
	nuGLProg_FillTex	PFillTex;
	nuGLProg_TextRGB	PTextRGB;
	nuGLProg_TextWhole	PTextWhole;
	nuGLProg_Curve		PCurve;
	static const int	NumProgs = 6;
	nuGLProg*			AllProgs[NumProgs];	// All of the above programs

	/*
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
	*/

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
	nuGLProg*		ActiveProgram;
	int				FBWidth, FBHeight;
	GLuint			SingleTex2D;
	GLuint			SingleTexAtlas2D;

	void			DeleteProgram( nuGLProg& prog );
	bool			LoadProgram( nuGLProg& prog );
	bool			LoadProgram( nuGLProg& prog, const char* vsrc, const char* fsrc );
	bool			LoadProgram( GLuint& vshade, GLuint& fshade, GLuint& prog, const char* vsrc, const char* fsrc );
	bool			LoadShader( GLenum shaderType, GLuint& shader, const char* raw_src );
	void			Check();
	void			Ortho( nuMat4f &imat, double left, double right, double bottom, double top, double znear, double zfar );
	void			Reset();

};

