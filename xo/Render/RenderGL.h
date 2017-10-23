#pragma once

#include "RenderGL_Defs.h"
#include "RenderBase.h"
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

namespace xo {

class XO_API RenderGL : public RenderBase {
public:
#if XO_PLATFORM_WIN_DESKTOP
	HGLRC GLRC;
	HDC   DC; // Valid between BeginFrame() and EndFrame()
#endif

	GLProg_Rect      PRect;
	GLProg_Rect2     PRect2;
	GLProg_Rect3     PRect3;
	GLProg_Fill      PFill;
	GLProg_FillTex   PFillTex;
	GLProg_TextRGB   PTextRGB;
	GLProg_TextWhole PTextWhole;
	GLProg_Arc       PArc;
	GLProg_Curve     PCurve;
	GLProg_Uber      PUber;
	static const int NumProgs = 10;
	GLProg*          AllProgs[NumProgs]; // All of the above programs

	RenderGL();
	~RenderGL() override;

	bool CreateShaders();
	void DeleteShadersAndTextures();
	//void			DrawTriangles( int nvert, const void* v, const uint16_t* indices );

	// Implementation of RenderBase

	const char* RendererName() override;

	bool InitializeDevice(SysWnd& wnd) override;
	void DestroyDevice(SysWnd& wnd) override;
	void SurfaceLost() override;

	bool BeginRender(SysWnd& wnd) override;                        // Gets DC and does a MakeCurrent
	void EndRender(SysWnd& wnd, uint32_t endRenderFlags) override; // Releases DC and does a SwapBuffers (unless flags inhibit Swap)

	void PreRender() override;
	void PostRenderCleanup() override;

	void Draw(GPUPrimitiveTypes type, int nvertex, const void* v) override;

	ProgBase* GetShader(Shaders shader) override;
	void      ActivateShader(Shaders shader) override;

	bool LoadTexture(Texture* tex, int texUnit) override;
	bool ReadBackbuffer(Image& image) override;

protected:
	Shaders     ActiveShader;
	GLuint      BoundTextures[MaxTextureUnits];
	std::string BaseShader;
	bool        Have_Unpack_RowLength;
	bool        Have_sRGB_Framebuffer;
	bool        Have_BlendFuncExtended;

	void PreparePreprocessor();
	void DeleteProgram(GLProg& prog);
	bool LoadProgram(GLProg& prog);
	bool LoadProgram(GLProg& prog, const char* name, const char* vsrc, const char* fsrc);
	bool LoadProgram(GLuint& vshade, GLuint& fshade, GLuint& prog, const char* name, const char* vsrc, const char* fsrc);
	bool LoadShader(GLenum shaderType, GLuint& shader, const char* name, const char* raw_src);
	void SetShaderFrameUniforms();
	void SetShaderObjectUniforms();
	void Check();
	void Reset();
	void CheckExtensions();

	template <typename TProg>
	bool SetMVProj(Shaders shader, TProg& prog, const Mat4f& mvprojTransposed);

	static GLint TexFilterToGL(TexFilter f);
};
} // namespace xo
