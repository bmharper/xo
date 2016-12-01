#pragma once

#include "xoRenderGL_Defs.h"
#include "xoRenderBase.h"
#include "../Shaders/Processed_glsl/CurveShader.h"
#include "../Shaders/Processed_glsl/FillShader.h"
#include "../Shaders/Processed_glsl/FillTexShader.h"
#include "../Shaders/Processed_glsl/RectShader.h"
#include "../Shaders/Processed_glsl/Rect2Shader.h"
#include "../Shaders/Processed_glsl/Rect3Shader.h"
#include "../Shaders/Processed_glsl/TextRGBShader.h"
#include "../Shaders/Processed_glsl/TextWholeShader.h"
#include "../Shaders/Processed_glsl/ArcShader.h"
#include "../Shaders/Processed_glsl/UberShader.h"

class XOAPI xoRenderGL : public xoRenderBase
{
public:
#if XO_PLATFORM_WIN_DESKTOP
	HGLRC				GLRC;
	HDC					DC;			// Valid between BeginFrame() and EndFrame()
#endif

	xoGLProg_Rect		PRect;
	xoGLProg_Rect2		PRect2;
	xoGLProg_Rect3		PRect3;
	xoGLProg_Fill		PFill;
	xoGLProg_FillTex	PFillTex;
	xoGLProg_TextRGB	PTextRGB;
	xoGLProg_TextWhole	PTextWhole;
	xoGLProg_Arc		PArc;
	xoGLProg_Curve		PCurve;
	xoGLProg_Uber		PUber;
	static const int	NumProgs = 10;
	xoGLProg*			AllProgs[NumProgs];	// All of the above programs

	xoRenderGL();
	~xoRenderGL();

	bool			CreateShaders();
	void			DeleteShadersAndTextures();
	//void			DrawTriangles( int nvert, const void* v, const uint16* indices );

	// Implementation of xoRenderBase

	virtual const char*	RendererName();

	virtual bool		InitializeDevice(xoSysWnd& wnd);
	virtual void		DestroyDevice(xoSysWnd& wnd);
	virtual void		SurfaceLost();

	virtual bool		BeginRender(xoSysWnd& wnd);							// Gets DC and does a MakeCurrent
	virtual void		EndRender(xoSysWnd& wnd, uint endRenderFlags);		// Releases DC and does a SwapBuffers (unless flags inhibit Swap)

	virtual void		PreRender();
	virtual void		PostRenderCleanup();

	virtual void		Draw(xoGPUPrimitiveTypes type, int nvertex, const void* v);

	virtual xoProgBase* GetShader(xoShaders shader);
	virtual void		ActivateShader(xoShaders shader);

	virtual bool		LoadTexture(xoTexture* tex, int texUnit);
	virtual bool		ReadBackbuffer(xoImage& image);

protected:
	xoShaders		ActiveShader;
	GLuint			BoundTextures[xoMaxTextureUnits];
	std::string		BaseShader;
	bool			Have_Unpack_RowLength;
	bool			Have_sRGB_Framebuffer;
	bool			Have_BlendFuncExtended;

	void			PreparePreprocessor();
	void			DeleteProgram(xoGLProg& prog);
	bool			LoadProgram(xoGLProg& prog);
	bool			LoadProgram(xoGLProg& prog, const char* name, const char* vsrc, const char* fsrc);
	bool			LoadProgram(GLuint& vshade, GLuint& fshade, GLuint& prog, const char* name, const char* vsrc, const char* fsrc);
	bool			LoadShader(GLenum shaderType, GLuint& shader, const char* name, const char* raw_src);
	void			SetShaderFrameUniforms();
	void			SetShaderObjectUniforms();
	void			Check();
	void			Reset();
	void			CheckExtensions();

	template<typename TProg>
	bool			SetMVProj(xoShaders shader, TProg& prog, const Mat4f& mvprojTransposed);

	static GLint	TexFilterToGL(xoTexFilter f);
};

