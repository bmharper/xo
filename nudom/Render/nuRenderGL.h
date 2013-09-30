#pragma once

#include "nuRenderGL_Defs.h"
#include "nuRenderBase.h"
#include "../Shaders/Helpers/nuPreprocessor.h"
#include "../Shaders/Processed/CurveShader.h"
#include "../Shaders/Processed/FillShader.h"
#include "../Shaders/Processed/FillTexShader.h"
#include "../Shaders/Processed/RectShader.h"
#include "../Shaders/Processed/TextRGBShader.h"
#include "../Shaders/Processed/TextWholeShader.h"

class NUAPI nuRenderGL : public nuRenderBase
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

					nuRenderGL();
					~nuRenderGL();
	bool			CreateShaders();
	void			DeleteShadersAndTextures();
	void			SurfaceLost();
	void			ActivateProgram( nuGLProg& p );
	void			PreRender( int fbwidth, int fbheight );
	void			PostRenderCleanup();
	void			DrawQuad( const void* v );
	void			DrawTriangles( int nvert, const void* v, const uint16* indices );
	//void			LoadTexture( const nuImage* img );
	//void			LoadTextureAtlas( const nuTextureAtlas* atlas );
	
	// Implementation of nuRenderBase
	virtual void	LoadTexture( nuTexture* tex, int texUnit );

protected:
	nuGLProg*		ActiveProgram;
	int				FBWidth, FBHeight;
	GLuint			BoundTextures[nuMaxTextureUnits];
	nuPreprocessor	Preprocessor;
	std::string		BaseShader;
	bool			Have_Unpack_RowLength;
	bool			Have_sRGB_Framebuffer;

	void			PreparePreprocessor();
	void			DeleteProgram( nuGLProg& prog );
	bool			LoadProgram( nuGLProg& prog );
	bool			LoadProgram( nuGLProg& prog, const char* name, const char* vsrc, const char* fsrc );
	bool			LoadProgram( GLuint& vshade, GLuint& fshade, GLuint& prog, const char* name, const char* vsrc, const char* fsrc );
	bool			LoadShader( GLenum shaderType, GLuint& shader, const char* name, const char* raw_src );
	void			Check();
	void			Ortho( nuMat4f &imat, double left, double right, double bottom, double top, double znear, double zfar );
	void			Reset();
	void			CheckExtensions();

};

