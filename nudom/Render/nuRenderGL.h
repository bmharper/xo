#pragma once

#include "nuRenderGL_Defs.h"
#include "nuRenderBase.h"
#include "../Shaders/Helpers/nuPreprocessor.h"
#include "../Shaders/Processed_glsl/CurveShader.h"
#include "../Shaders/Processed_glsl/FillShader.h"
#include "../Shaders/Processed_glsl/FillTexShader.h"
#include "../Shaders/Processed_glsl/RectShader.h"
#include "../Shaders/Processed_glsl/TextRGBShader.h"
#include "../Shaders/Processed_glsl/TextWholeShader.h"

class NUAPI nuRenderGL : public nuRenderBase
{
public:
#if NU_PLATFORM_WIN_DESKTOP
	HGLRC				GLRC;
	HDC					DC;			// Valid between BeginFrame() and EndFrame()
#endif

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
	//void			DrawTriangles( int nvert, const void* v, const uint16* indices );
	
	// Implementation of nuRenderBase
	virtual bool		InitializeDevice( nuSysWnd& wnd );
	virtual void		DestroyDevice( nuSysWnd& wnd );
	virtual void		SurfaceLost();

	virtual bool		BeginRender( nuSysWnd& wnd );		// Gets DC and does a MakeCurrent
	virtual void		EndRender( nuSysWnd& wnd );			// Releases DC and does a SwapBuffers

	virtual void		PreRender();
	virtual void		PostRenderCleanup();
	
	virtual void		DrawQuad( const void* v );

	virtual nuProgBase* GetShader( nuShaders shader );
	virtual void		ActivateShader( nuShaders shader );

	virtual bool		LoadTexture( nuTexture* tex, int texUnit );
	virtual void		ReadBackbuffer( nuImage& image );

protected:
	nuShaders		ActiveShader;
	int				FBWidth, FBHeight;
	GLuint			BoundTextures[nuMaxTextureUnits];
	nuPreprocessor	Preprocessor;
	std::string		BaseShader;
	bool			Have_Unpack_RowLength;
	bool			Have_sRGB_Framebuffer;
	bool			Have_BlendFuncExtended;

	void			PreparePreprocessor();
	void			DeleteProgram( nuGLProg& prog );
	bool			LoadProgram( nuGLProg& prog );
	bool			LoadProgram( nuGLProg& prog, const char* name, const char* vsrc, const char* fsrc );
	bool			LoadProgram( GLuint& vshade, GLuint& fshade, GLuint& prog, const char* name, const char* vsrc, const char* fsrc );
	bool			LoadShader( GLenum shaderType, GLuint& shader, const char* name, const char* raw_src );
	void			SetShaderFrameUniforms();
	void			SetShaderObjectUniforms();
	void			Check();
	void			Reset();
	void			CheckExtensions();

	template<typename TProg>
	bool			SetMVProj( nuShaders shader, TProg& prog, const Mat4f& mvprojTransposed );

};

