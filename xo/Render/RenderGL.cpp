#include "pch.h"
#if XO_BUILD_OPENGL
#include "RenderGL.h"
#include "../Image/Image.h"
#include "TextureAtlas.h"
#include "../Text/GlyphCache.h"
#include "../SysWnd.h"
#include "../SysWnd_windows.h"
#include "../SysWnd_linux.h"

namespace xo {

static bool GLIsBooted = false;

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_RG
#define GL_RG 0x8227
#endif
#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif
#ifndef GL_SRC1_COLOR
#define GL_SRC1_COLOR 0x88F9
#endif
#ifndef GL_ONE_MINUS_SRC1_COLOR
#define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#endif
#ifndef GL_ONE_MINUS_SRC1_ALPHA
#define GL_ONE_MINUS_SRC1_ALPHA 0x88FB
#endif

// GL_XO_RED_OR_LUMINANCE is used to define a single-channel texture
// GL_RED is not defined in ES2
#ifdef GL_RED
#define GL_XO_RED_OR_LUMINANCE GL_RED
#else
#define GL_XO_RED_OR_LUMINANCE GL_LUMINANCE
#endif

static const char* xo_GLSLPrefix = R"(
float fromSRGB_Component(float srgb)
{
	float sRGB_Low	= 0.0031308;
	float sRGB_a	= 0.055;

	if (srgb <= 0.04045)
		return srgb / 12.92;
	else
		return pow((srgb + sRGB_a) / (1.0 + sRGB_a), 2.4);
}

vec4 fromSRGB(vec4 c)
{
	vec4 linear_c;
	linear_c.r = fromSRGB_Component(c.r);
	linear_c.g = fromSRGB_Component(c.g);
	linear_c.b = fromSRGB_Component(c.b);
	linear_c.a = c.a;
	return linear_c;
}

vec4 premultiply(vec4 c)
{
	return vec4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);
}

#define SHADER_TYPE_MASK           15
#define SHADER_FLAG_TEXBG          16
#define SHADER_FLAG_TEXBG_PREMUL   32

#define SHADER_ARC           1
#define SHADER_RECT          2
#define SHADER_TEXT_SIMPLE   3
#define SHADER_TEXT_SUBPIXEL 4

)";

RenderGL::RenderGL() {
#if XO_PLATFORM_WIN_DESKTOP
	GLRC = NULL;
	DC   = NULL;
#endif
	Have_Unpack_RowLength  = false;
	Have_sRGB_Framebuffer  = false;
	Have_BlendFuncExtended = false;
	AllProgs[0]            = &PRect;
	AllProgs[1]            = &PRect2;
	AllProgs[2]            = &PRect3;
	AllProgs[3]            = &PFill;
	AllProgs[4]            = &PFillTex;
	AllProgs[5]            = &PTextRGB;
	AllProgs[6]            = &PTextWhole;
	AllProgs[7]            = &PArc;
	AllProgs[8]            = &PCurve;
	AllProgs[9]            = &PUber;
	static_assert(NumProgs == 10, "Add your new shader here");
	Reset();
}

RenderGL::~RenderGL() {
}

void RenderGL::Reset() {
	for (int i = 0; i < NumProgs; i++)
		AllProgs[i]->Reset();
	memset(BoundTextures, 0, sizeof(BoundTextures));
	ActiveShader = ShaderInvalid;
}

const char* RenderGL::RendererName() {
	return "OpenGL";
}

#if XO_PLATFORM_WIN_DESKTOP

typedef BOOL (*_wglChoosePixelFormatARB)(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);

static void BootGL_FillPFD(PIXELFORMATDESCRIPTOR& pfd) {
	const DWORD flags = 0 | PFD_DRAW_TO_WINDOW // support window
	                    | PFD_SUPPORT_OPENGL   // support OpenGL
	                    | PFD_DOUBLEBUFFER     // double buffer
	                    | 0;

	// Note that this must match the attribs used by wglChoosePixelFormatARB (I find that strange).
	PIXELFORMATDESCRIPTOR base = {
	    sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
	    1,                             // version number
	    flags,
	    PFD_TYPE_RGBA,    // RGBA type
	    24,               // color depth
	    0, 0, 0, 0, 0, 0, // color bits ignored
	    0,                // alpha bits
	    0,                // shift bit ignored
	    0,                // no accumulation buffer
	    0, 0, 0, 0,       // accum bits ignored
	    16,               // z-buffer
	    0,                // no stencil buffer
	    0,                // no auxiliary buffer
	    PFD_MAIN_PLANE,   // main layer
	    0,                // reserved
	    0, 0, 0           // layer masks ignored
	};

	pfd = base;
}

static bool BootGL(HWND wnd) {
	HDC dc = GetDC(wnd);

	// get the best available match of pixel format for the device context
	PIXELFORMATDESCRIPTOR pfd;
	BootGL_FillPFD(pfd);
	int iPixelFormat = ChoosePixelFormat(dc, &pfd);

	if (iPixelFormat != 0) {
		// make that the pixel format of the device context
		BOOL  setOK = SetPixelFormat(dc, iPixelFormat, &pfd);
		HGLRC rc    = wglCreateContext(dc);
		wglMakeCurrent(dc, rc);
		int wglLoad = wgl_LoadFunctions(dc);
		int oglLoad = ogl_LoadFunctions();
		Trace("wgl_Load: %d\n", wglLoad);
		Trace("ogl_Load: %d\n", oglLoad);
		GLIsBooted = true;
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(rc);
	}

	ReleaseDC(wnd, dc);

	return true;
}
#endif

#if XO_PLATFORM_WIN_DESKTOP
bool RenderGL::InitializeDevice(SysWnd& wnd) {
	if (!GLIsBooted) {
		if (!BootGL(GetHWND(wnd)))
			return false;
	}

	bool  allGood = false;
	HGLRC rc      = NULL;
	HDC   dc      = GetDC(GetHWND(wnd));
	if (!dc)
		return false;

	int attribs[] =
	    {
	        WGL_DRAW_TO_WINDOW_ARB, 1,
	        WGL_SUPPORT_OPENGL_ARB, 1,
	        WGL_DOUBLE_BUFFER_ARB, 1,
	        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
	        WGL_COLOR_BITS_ARB, 24,
	        WGL_ALPHA_BITS_ARB, 0,
	        WGL_DEPTH_BITS_ARB, 16,
	        WGL_STENCIL_BITS_ARB, 0,
	        WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB, // This was an attempt to lower latency on Windows 8.0, but it seems to have no effect
	        0};
	PIXELFORMATDESCRIPTOR pfd;
	BootGL_FillPFD(pfd);
	int      formats[20];
	uint32_t numformats = 0;
	BOOL     chooseOK   = wglChoosePixelFormatARB(dc, attribs, NULL, arraysize(formats), formats, &numformats);
	if (chooseOK && numformats != 0) {
		if (SetPixelFormat(dc, formats[0], &pfd)) {
			rc = wglCreateContext(dc);
			wglMakeCurrent(dc, rc);
			if (wglSwapIntervalEXT)
				wglSwapIntervalEXT(Global()->EnableVSync ? 1 : 0);
			allGood = CreateShaders();
			wglMakeCurrent(NULL, NULL);
		} else {
			Trace("SetPixelFormat failed: %d\n", GetLastError());
		}
	}

	ReleaseDC(GetHWND(wnd), dc);

	if (!allGood) {
		if (rc)
			wglDeleteContext(rc);
		rc = NULL;
	}

	GLRC = rc;
	return GLRC != NULL;
}
#elif XO_PLATFORM_ANDROID
bool RenderGL::InitializeDevice(SysWnd& wnd) {
	if (!CreateShaders())
		return false;
	return true;
}
#elif XO_PLATFORM_LINUX_DESKTOP
bool RenderGL::InitializeDevice(SysWnd& wnd) {
	auto w = (SysWndLinux*) &wnd;
	int oglLoad = ogl_LoadFunctions();
	int glxLoad = glx_LoadFunctions(w->XDisplay, 0);
	Trace("oglload: %d\n", oglLoad);
	Trace("glxload: %d\n", glxLoad);
	if (!CreateShaders())
		return false;
	Trace("Shaders created\n");
	return true;
}
#else
XO_TODO_STATIC
#endif

void RenderGL::CheckExtensions() {
	const char* ver          = (const char*) glGetString(GL_VERSION);
	const char* ext          = (const char*) glGetString(GL_EXTENSIONS);
	auto        hasExtension = [ext](const char* name) -> bool {
        const char* pos = ext;
        while (true) {
            size_t len = strlen(name);
            pos        = strstr(pos, name);
            if (pos != NULL && (pos[len] == ' ' || pos[len] == 0))
                return true;
            else if (pos == NULL)
                return false;
        }
    };

	Trace("Checking OpenGL extensions\n");
	// On my desktop Nvidia I get "4.3.0"

	int dot     = (int) (strstr(ver, ".") - ver);
	int major   = ver[dot - 1] - '0';
	int minor   = ver[dot + 1] - '0';
	int version = major * 10 + minor;

	Trace("OpenGL version: %s\n", ver);
	Trace("OpenGL extensions: %s\n", ext);

	if (strstr(ver, "OpenGL ES")) {
		Trace("OpenGL ES\n");
		Have_Unpack_RowLength = version >= 30 || hasExtension("GL_EXT_unpack_subimage");
		Have_sRGB_Framebuffer = version >= 30 || hasExtension("GL_EXT_sRGB");
	} else {
		Trace("OpenGL Regular (non-ES)\n");
		Have_Unpack_RowLength = true;
		Have_sRGB_Framebuffer = version >= 40 || hasExtension("ARB_framebuffer_sRGB") || hasExtension("GL_EXT_framebuffer_sRGB");
	}

	Have_BlendFuncExtended = hasExtension("GL_ARB_blend_func_extended");
	Trace(
	    "OpenGL Extensions ("
	    "UNPACK_SUBIMAGE=%d, "
	    "sRGB_FrameBuffer=%d, "
	    "blend_func_extended=%d"
	    ")\n",
	    Have_Unpack_RowLength ? 1 : 0,
	    Have_sRGB_Framebuffer ? 1 : 0,
	    Have_BlendFuncExtended ? 1 : 0);
}

bool RenderGL::CreateShaders() {
	Check();

	CheckExtensions();

	Check();

	for (int i = 0; i < NumProgs; i++) {
		if (AllProgs[i]->UseOnThisPlatform()) {
			if (!LoadProgram(*AllProgs[i]))
				return false;
			if (!AllProgs[i]->LoadVariablePositions())
				return false;
		}
	}

	Check();

	return true;
}

void RenderGL::DeleteShadersAndTextures() {
	for (int i = MaxTextureUnits - 1; i >= 0; i--) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	cheapvec<GLuint> textures;
	for (size_t i = 0; i < TexIDToNative.size(); i++)
		textures += GetTextureDeviceHandleInt(FirstTextureID() + (TextureID) i);

	glDeleteTextures((GLsizei) textures.size(), &textures[0]);

	glUseProgram(0);

	for (int i = 0; i < NumProgs; i++)
		DeleteProgram(*AllProgs[i]);
}

ProgBase* RenderGL::GetShader(Shaders shader) {
	switch (shader) {
	case ShaderFill: return &PFill;
	case ShaderFillTex: return &PFillTex;
	case ShaderRect: return &PRect;
	case ShaderRect2: return &PRect2;
	case ShaderRect3: return &PRect3;
	case ShaderTextRGB: return &PTextRGB;
	case ShaderTextWhole: return &PTextWhole;
	case ShaderArc: return &PArc;
	case ShaderQuadraticSpline: return &PCurve;
	case ShaderUber: return &PUber;
	default:
		XO_ASSERT(false);
		return NULL;
	}
}

void RenderGL::ActivateShader(Shaders shader) {
	if (ActiveShader == shader)
		return;
	GLProg* p    = (GLProg*) GetShader(shader);
	ActiveShader = shader;
	XO_ASSERT(p->Prog != 0);
	XOTRACE_RENDER("Activate shader %s\n", p->Name());
	glUseProgram(p->Prog);
	if (ActiveShader == ShaderTextRGB || ActiveShader == ShaderUber) {
		if (Have_BlendFuncExtended)
			glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC1_COLOR, GL_ONE, GL_ONE_MINUS_SRC1_ALPHA);
		// else we are screwed!
		// there might be a solution that is better than simply ignoring the problem, but I
		// haven't bothered yet. On my Sandy Bridge (i7-2600K) with 2014-04-15 Intel drivers,
		// the DirectX drivers correctly expose this functionality, but the OpenGL drivers do
		// not support GL_ARB_blend_func_extended, so it's very simple - we just use DirectX.
		// I can't think of a device/OS combination where you'd want OpenGL+Subpixel text,
		// so sticking with DirectX 11 there is hopefully fine. Hmm.. actually desktop linux
		// is indeed such a combination. We'll have to wait and see.
	} else {
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	// this is for non-premultiplied
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // this is premultiplied
	}
	Check();
}

void RenderGL::DestroyDevice(SysWnd& wnd) {
#if XO_PLATFORM_WIN_DESKTOP
	if (GLRC != NULL) {
		DC = GetDC(GetHWND(wnd));
		wglMakeCurrent(DC, GLRC);
		DeleteShadersAndTextures();
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(GLRC);
		ReleaseDC(GetHWND(wnd), DC);
		DC   = NULL;
		GLRC = NULL;
	}
#endif
}

void RenderGL::SurfaceLost() {
	Reset();
	SurfaceLost_ForgetTextures();
	CreateShaders();
}

bool RenderGL::BeginRender(SysWnd& wnd) {
	auto rect = wnd.GetRelativeClientRect();
	FBWidth   = rect.Width();
	FBHeight  = rect.Height();
	XOTRACE_RENDER("BeginRender %d x %d\n", FBWidth, FBHeight);
#if XO_PLATFORM_WIN_DESKTOP
	if (GLRC) {
		DC = GetDC(GetHWND(wnd));
		if (DC) {
			wglMakeCurrent(DC, GLRC);
			return true;
		}
	}
	return false;
#elif XO_PLATFORM_ANDROID
	return true;
#elif XO_PLATFORM_LINUX_DESKTOP
	auto w = (SysWndLinux*) &wnd;
	glXMakeCurrent(w->XDisplay, w->XWindow, w->GLContext);
	return true;
#else
	return true;
#endif
}

void RenderGL::EndRender(SysWnd& wnd, uint32_t endRenderFlags) {
#if XO_PLATFORM_WIN_DESKTOP
	if (!(endRenderFlags & EndRenderNoSwap))
		SwapBuffers(DC);
	wglMakeCurrent(NULL, NULL);
	ReleaseDC(GetHWND(wnd), DC);
	DC = NULL;
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
	auto w = (SysWndLinux*) &wnd;
	if (!(endRenderFlags & EndRenderNoSwap))
		glXSwapBuffers(w->XDisplay, w->XWindow);
	glXMakeCurrent(w->XDisplay, X11Constants::None, NULL);
#endif
}

void RenderGL::PreRender() {
	Check();

	XOTRACE_RENDER("PreRender %d %d\n", FBWidth, FBHeight);
	Check();

	glViewport(0, 0, FBWidth, FBHeight);

	auto clear = Global()->ClearColor;

	if (Global()->EnableSRGBFramebuffer && Have_sRGB_Framebuffer) {
		glEnable(GL_FRAMEBUFFER_SRGB);
		glClearColor(SRGB2Linear(clear.r), SRGB2Linear(clear.g), SRGB2Linear(clear.b), clear.a / 255.0f);
	} else {
		glClearColor(clear.r / 255.0f, clear.g / 255.0f, clear.b / 255.0f, clear.a / 255.0f);
	}

	//glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	XOTRACE_RENDER("PreRender 2\n");
	Check();

	// Enable CULL_FACE because it will make sure that we are consistent about vertex orientation
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	XOTRACE_RENDER("PreRender 3\n");
	Check();

	SetShaderFrameUniforms();

	XOTRACE_RENDER("PreRender done\n");
}

void RenderGL::SetShaderFrameUniforms() {
	XOTRACE_RENDER("FB %d x %d\n", FBWidth, FBHeight);
	Mat4f mvproj;
	mvproj.Identity();
	Ortho(mvproj, 0, FBWidth, FBHeight, 0, 1, 0);
	SetupToScreen(mvproj); // this sets up ShaderPerFrame
	Mat4f mvprojT = mvproj.Transposed();

	if (SetMVProj(ShaderRect, PRect, mvprojT))
		glUniform2f(PRect.v_vport_hsize, FBWidth / 2.0f, FBHeight / 2.0f);

	if (SetMVProj(ShaderRect2, PRect2, mvprojT))
		glUniform2f(PRect2.v_vport_hsize, FBWidth / 2.0f, FBHeight / 2.0f);

	if (SetMVProj(ShaderArc, PArc, mvprojT))
		glUniform2f(PArc.v_vport_hsize, FBWidth / 2.0f, FBHeight / 2.0f);

	if (SetMVProj(ShaderUber, PUber, mvprojT))
		glUniform2f(PUber.v_Frame_VPort_HSize, FBWidth / 2.0f, FBHeight / 2.0f);

	SetMVProj(ShaderRect3, PFill, mvprojT);
	SetMVProj(ShaderFill, PFill, mvprojT);
	SetMVProj(ShaderFillTex, PFillTex, mvprojT);
	SetMVProj(ShaderTextRGB, PTextRGB, mvprojT);
	SetMVProj(ShaderTextWhole, PTextWhole, mvprojT);
	SetMVProj(ShaderQuadraticSpline, PCurve, mvprojT);
}

void RenderGL::SetShaderObjectUniforms() {
	if (ActiveShader == ShaderRect) {
		glUniform4fv(PRect.v_box, 1, &ShaderPerObject.Box.x);
		glUniform4fv(PRect.v_border, 1, &ShaderPerObject.Border.x);
		glUniform4fv(PRect.v_border_color, 1, &ShaderPerObject.BorderColor.x);
		glUniform1f(PRect.v_radius, ShaderPerObject.Radius);
	} else if (ActiveShader == ShaderRect2) {
		glUniform2fv(PRect2.v_out_vector, 1, &ShaderPerObject.OutVector.x);
		glUniform2fv(PRect2.v_shadow_offset, 1, &ShaderPerObject.ShadowOffset.x);
		glUniform4fv(PRect2.v_shadow_color, 1, &ShaderPerObject.ShadowColor.x);
		glUniform1f(PRect2.v_shadow_size_inv, ShaderPerObject.ShadowSizeInv);
		glUniform2fv(PRect2.v_edges, 1, &ShaderPerObject.Edges.x);
	}
}

template <typename TProg>
bool RenderGL::SetMVProj(Shaders shader, TProg& prog, const Mat4f& mvprojTransposed) {
	if (prog.Prog == 0) {
		XOTRACE_RENDER("SetMVProj skipping %s, because not compiled\n", (const char*) prog.Name());
		return false;
	} else {
		XOTRACE_RENDER("SetMVProj %s (%d)\n", (const char*) prog.Name(), prog.v_mvproj);
		ActivateShader(shader);
		Check();
		// GLES doesn't support TRANSPOSE = TRUE
		glUniformMatrix4fv(prog.v_mvproj, 1, false, &mvprojTransposed.row[0].x);
		return true;
	}
}

GLint RenderGL::TexFilterToGL(TexFilter f) {
	switch (f) {
	case TexFilterNearest: return GL_NEAREST;
	case TexFilterLinear: return GL_LINEAR;
	default: XO_TODO;
	}
	return GL_NEAREST;
}

void RenderGL::PostRenderCleanup() {
	glUseProgram(0);
	ActiveShader = ShaderInvalid;
}

void RenderGL::Draw(GPUPrimitiveTypes type, int nvertex, const void* v) {
	XOTRACE_RENDER("DrawQuad\n");

	SetShaderObjectUniforms();

	int            stride = sizeof(Vx_PTC);
	const uint8_t* vbyte  = (const uint8_t*) v;

	GLint varvpos      = -1;
	GLint varvcol      = -1;
	GLint varvtex0     = -1;
	GLint varvtexClamp = -1;
	GLint vartexUnit0  = -1;
	switch (ActiveShader) {
	case ShaderRect:
		varvpos = PRect.v_vpos;
		varvcol = PRect.v_vcolor;
		break;
	case ShaderRect2:
		stride  = sizeof(Vx_PTCV4);
		varvpos = PRect2.v_vpos;
		varvcol = PRect2.v_vcolor;
		glVertexAttribPointer(PRect2.v_vradius, 1, GL_FLOAT, true, stride, vbyte + offsetof(Vx_PTCV4, UV.x));
		glEnableVertexAttribArray(PRect2.v_vradius);
		glVertexAttribPointer(PRect2.v_vborder_width, 1, GL_FLOAT, true, stride, vbyte + offsetof(Vx_PTCV4, UV.y));
		glEnableVertexAttribArray(PRect2.v_vborder_width);
		glVertexAttribPointer(PRect2.v_vborder_color, 4, GL_UNSIGNED_BYTE, true, stride, vbyte + offsetof(Vx_PTCV4, Color2));
		glEnableVertexAttribArray(PRect2.v_vborder_color);
		Check();
		break;
	case ShaderRect3:
		stride  = sizeof(Vx_PTCV4);
		varvpos = PRect3.v_vpos;
		varvcol = PRect3.v_vcolor;
		glVertexAttribPointer(PRect3.v_vborder_width, 1, GL_FLOAT, true, stride, vbyte + offsetof(Vx_PTCV4, V4.x));
		glEnableVertexAttribArray(PRect3.v_vborder_width);
		glVertexAttribPointer(PRect3.v_vborder_distance, 1, GL_FLOAT, true, stride, vbyte + offsetof(Vx_PTCV4, V4.y));
		glEnableVertexAttribArray(PRect3.v_vborder_distance);
		glVertexAttribPointer(PRect3.v_vborder_color, 4, GL_UNSIGNED_BYTE, true, stride, vbyte + offsetof(Vx_PTCV4, Color2));
		glEnableVertexAttribArray(PRect3.v_vborder_color);
		break;
	case ShaderFill:
		varvpos = PFill.v_vpos;
		varvcol = PFill.v_vcolor;
		XOTRACE_RENDER("vv %d %d\n", varvpos, varvcol);
		break;
	case ShaderFillTex:
		varvpos     = PFillTex.v_vpos;
		varvcol     = PFillTex.v_vcolor;
		varvtex0    = PFillTex.v_vtexuv0;
		vartexUnit0 = PFillTex.v_tex0;
		break;
	case ShaderTextRGB:
		stride       = sizeof(Vx_PTCV4);
		varvpos      = PTextRGB.v_vpos;
		varvcol      = PTextRGB.v_vcolor;
		varvtex0     = PTextRGB.v_vtexuv0;
		varvtexClamp = PTextRGB.v_vtexClamp;
		vartexUnit0  = PTextRGB.v_tex0;
		break;
	case ShaderTextWhole:
		varvpos     = PTextWhole.v_vpos;
		varvcol     = PTextWhole.v_vcolor;
		varvtex0    = PTextWhole.v_vtexuv0;
		vartexUnit0 = PTextWhole.v_tex0;
		break;
	case ShaderArc:
		stride  = sizeof(Vx_PTCV4);
		varvpos = PArc.v_vpos;
		varvcol = PArc.v_vcolor;
		glVertexAttribPointer(PArc.v_vcenter, 4, GL_FLOAT, false, stride, vbyte + offsetof(Vx_PTCV4, V4.x));
		glVertexAttribPointer(PArc.v_vradius1, 1, GL_FLOAT, false, stride, vbyte + offsetof(Vx_PTCV4, UV.x));
		glVertexAttribPointer(PArc.v_vradius2, 1, GL_FLOAT, false, stride, vbyte + offsetof(Vx_PTCV4, UV.y));
		glVertexAttribPointer(PArc.v_vborder_color, 4, GL_UNSIGNED_BYTE, true, stride, vbyte + offsetof(Vx_PTCV4, Color2));
		glEnableVertexAttribArray(PArc.v_vcenter);
		glEnableVertexAttribArray(PArc.v_vradius1);
		glEnableVertexAttribArray(PArc.v_vradius2);
		glEnableVertexAttribArray(PArc.v_vborder_color);
		break;
	case ShaderUber:
		stride = sizeof(Vx_Uber);
		glVertexAttribPointer(PUber.v_v_pos, 2, GL_FLOAT, false, stride, vbyte + offsetof(Vx_Uber, Pos.x));
		glVertexAttribPointer(PUber.v_v_uv1, 4, GL_FLOAT, false, stride, vbyte + offsetof(Vx_Uber, UV1.x));
		glVertexAttribPointer(PUber.v_v_uv2, 4, GL_FLOAT, false, stride, vbyte + offsetof(Vx_Uber, UV2.x));
		glVertexAttribPointer(PUber.v_v_color1, 4, GL_UNSIGNED_BYTE, true, stride, vbyte + offsetof(Vx_Uber, Color1));
		glVertexAttribPointer(PUber.v_v_color2, 4, GL_UNSIGNED_BYTE, true, stride, vbyte + offsetof(Vx_Uber, Color2));
		glVertexAttribPointer(PUber.v_v_shader, 1, GL_UNSIGNED_INT, false, stride, vbyte + offsetof(Vx_Uber, Shader));
		glEnableVertexAttribArray(PUber.v_v_pos);
		glEnableVertexAttribArray(PUber.v_v_uv1);
		glEnableVertexAttribArray(PUber.v_v_uv2);
		glEnableVertexAttribArray(PUber.v_v_color1);
		glEnableVertexAttribArray(PUber.v_v_color2);
		glEnableVertexAttribArray(PUber.v_v_shader);
		break;
	case ShaderQuadraticSpline:
		stride   = sizeof(Vx_PTCV4);
		varvpos  = PCurve.v_vpos;
		varvcol  = PCurve.v_vcolor;
		varvtex0 = PCurve.v_vtexuv0;
		//vartexUnit0 = PCurve.v_tex0;
		glVertexAttribPointer(PCurve.v_vflip, 1, GL_FLOAT, true, stride, vbyte + offsetof(Vx_PTCV4, V4.x));
		glEnableVertexAttribArray(PCurve.v_vflip);
		break;
	case ShaderInvalid:
		XO_DIE();
		break;
	}

	// We assume here that Vx_PTC and Vx_PTCV4 share the same base layout
	if (varvpos != -1) {
		glVertexAttribPointer(varvpos, 3, GL_FLOAT, false, stride, vbyte);
		glEnableVertexAttribArray(varvpos);
	}

	if (varvcol != -1) {
		glVertexAttribPointer(varvcol, 4, GL_UNSIGNED_BYTE, true, stride, vbyte + offsetof(Vx_PTC, Color));
		glEnableVertexAttribArray(varvcol);
	}

	if (vartexUnit0 != -1)
		glUniform1i(vartexUnit0, 0);

	if (varvtex0 != -1) {
		glVertexAttribPointer(varvtex0, 2, GL_FLOAT, true, stride, vbyte + offsetof(Vx_PTC, UV));
		glEnableVertexAttribArray(varvtex0);
	}
	if (varvtexClamp != -1) {
		glVertexAttribPointer(varvtexClamp, 4, GL_FLOAT, true, stride, vbyte + offsetof(Vx_PTCV4, V4));
		glEnableVertexAttribArray(varvtexClamp);
	}

	int nindices = 0;
	switch (type) {
	case GPUPrimQuads: nindices = (nvertex / 2) * 3; break;
	case GPUPrimTriangles: nindices = nvertex; break;
	default: XO_TODO;
	}

	XO_ASSERT(nindices < 65536);
	uint16_t* indices = (uint16_t*) MallocOrDie(sizeof(uint16_t) * nindices);

	switch (type) {
	case GPUPrimQuads:
		for (int i = 0, v = 0; v < nvertex; v += 4) {
			indices[i++] = v;
			indices[i++] = v + 1;
			indices[i++] = v + 3;

			indices[i++] = v + 1;
			indices[i++] = v + 2;
			indices[i++] = v + 3;
		}
		glDrawElements(GL_TRIANGLES, nindices, GL_UNSIGNED_SHORT, indices);
		break;
	case GPUPrimTriangles:
		for (int i = 0; i < nvertex; i++)
			indices[i] = i;
		glDrawElements(GL_TRIANGLES, nvertex, GL_UNSIGNED_SHORT, indices);
		break;
	default:
		XO_TODO;
	}
	free(indices);

	//auto vx = (Vx_PTC*) vbyte;
	//XOTRACE_RENDER( "DrawQuad done (%f,%f) (%f,%f) (%f,%f) (%f,%f)\n", vx[0].Pos.x, vx[0].Pos.y, vx[1].Pos.x, vx[1].Pos.y, vx[2].Pos.x, vx[2].Pos.y, vx[3].Pos.x, vx[3].Pos.y );

	Check();
}

/*
void RenderGL::DrawTriangles( int nvert, const void* v, const uint16_t* indices )
{
int stride = sizeof(Vx_PTC);
const uint8_t* vbyte = (const uint8_t*) v;
glDrawElements( GL_TRIANGLES, nvert, GL_UNSIGNED_SHORT, indices );
Check();
}
*/

uint8_t demultiply(uint8_t a, uint8_t b) {
	if (a * b == 0) {
		return 0;
	} else if (a >= b) {
		return 255;
	} else {
		return uint8_t((a * 255 + (b >> 1)) / b);
	}
}

bool RenderGL::LoadTexture(Texture* tex, int texUnit) {
	EnsureTextureProperlyDefined(tex, texUnit);

	if (!IsTextureValid(tex->TexID)) {
		GLuint t;
		glGenTextures(1, &t);
		tex->TexID = RegisterTextureInt(t);
		tex->InvalidateWholeSurface();
	}

	GLuint glTexID = GetTextureDeviceHandleInt(tex->TexID);
	if (BoundTextures[texUnit] != glTexID) {
		glActiveTexture(GL_TEXTURE0 + texUnit);
		glBindTexture(GL_TEXTURE_2D, glTexID);
		BoundTextures[texUnit] = glTexID;
	}

	// This happens when a texture fails to upload to the GPU during synchronization from UI doc to render doc.
	if (tex->Data == nullptr)
		return true;

	// If the texture has not been updated, then we are done
	Box invRect  = tex->InvalidRect;
	Box fullRect = Box(0, 0, tex->Width, tex->Height);
	invRect.ClampTo(fullRect);
	if (!invRect.IsAreaPositive())
		return true;

	int iformat = 0;
	int format  = 0;
	switch (tex->Format) {
	case TexFormatGrey8:
		iformat = GL_XO_RED_OR_LUMINANCE;
		format  = GL_XO_RED_OR_LUMINANCE;
		break;
	case TexFormatRGBA8:
		iformat = GL_SRGB8_ALPHA8;
		//iformat = GL_RGBA8;
		format = GL_RGBA;
		break;
	default:
		XO_TODO;
	}

	if (Have_Unpack_RowLength)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->Stride / (int) tex->BytesPerPixel());

	if (!Have_Unpack_RowLength || invRect == fullRect) {
		// desperate attempt to understand premultiplied alpha & sRGB
		//xo::RGBA* copy = (xo::RGBA*) malloc(tex->Height * tex->Width * 4);
		//for (int y = 0; y < tex->Height; y++) {
		//	xo::RGBA* s = (xo::RGBA*) tex->DataAtLine(y);
		//	xo::RGBA* p = copy + y * tex->Width;
		//	for (int x = 0; x < tex->Width; x++, p++, s++) {
		//		uint8_t r = demultiply(s->r, s->a);
		//		uint8_t g = demultiply(s->g, s->a);
		//		uint8_t b = demultiply(s->b, s->a);
		//		uint8_t a = s->a;
		//		p->r      = r;
		//		p->g      = g;
		//		p->b      = b;
		//		p->a      = a;
		//	}
		//}
		//
		//glTexImage2D(GL_TEXTURE_2D, 0, iformat, tex->Width, tex->Height, 0, format, GL_UNSIGNED_BYTE, copy);
		//free(copy);

		glTexImage2D(GL_TEXTURE_2D, 0, iformat, tex->Width, tex->Height, 0, format, GL_UNSIGNED_BYTE, tex->Data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TexFilterToGL(tex->FilterMin));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TexFilterToGL(tex->FilterMax));
		// Clamping should have no effect for RGB text, since we clamp inside our fragment shader.
		// Also, when rendering 'whole pixel' glyphs, we shouldn't need clamping either, because
		// our UV coordinates are exact, and we always have a 1:1 texel:pixel ratio.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, invRect.Left, invRect.Top, invRect.Width(), invRect.Height(), format, GL_UNSIGNED_BYTE, tex->DataAt(invRect.Left, invRect.Top));
	}

	if (Have_Unpack_RowLength)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	return true;
}

bool RenderGL::ReadBackbuffer(Image& image) {
	image.Alloc(TexFormatRGBA8, FBWidth, FBHeight);
	if (Have_Unpack_RowLength)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, image.Stride / 4);

	glReadPixels(0, 0, FBWidth, FBHeight, GL_RGBA, GL_UNSIGNED_BYTE, image.DataAtLine(0));

	// our image is top-down
	// glReadPixels is bottom-up
	image.FlipVertical();

	if (Have_Unpack_RowLength)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	return true;
}

void RenderGL::PreparePreprocessor() {
	if (BaseShader.size() != 0)
		return;

#if XO_PLATFORM_WIN_DESKTOP
	BaseShader.append("#define XO_PLATFORM_WIN_DESKTOP\n");
#elif XO_PLATFORM_ANDROID
	BaseShader.append("#define XO_PLATFORM_ANDROID\n");
#elif XO_PLATFORM_LINUX_DESKTOP
	BaseShader.append("#define XO_PLATFORM_LINUX_DESKTOP\n");
#else
#ifdef _MSC_VER
#pragma error("Unknown Dom platform")
#else
#error Unknown Dom platform
#endif
#endif

	BaseShader.append(CommonShaderDefines());
	if (Global()->EnableSRGBFramebuffer && Have_sRGB_Framebuffer)
		BaseShader.append("#define XO_SRGB_FRAMEBUFFER\n");

	BaseShader += xo_GLSLPrefix;

	/*
	if ( Preprocessor.MacroCount() != 0 )
	return;

	Preprocessor.SetMacro( "XO_GLYPH_ATLAS_SIZE", fmt("%v", GlyphAtlasSize).CStr() );

	if ( Global()->EnableSRGBFramebuffer )
	Preprocessor.SetMacro( "XO_SRGB_FRAMEBUFFER", "" );
	*/

	//if ( Global()->EnableSRGBFramebuffer && Global()->EmulateGammaBlending )
	//	Preprocessor.SetMacro( "XO_EMULATE_GAMMA_BLENDING", "" );
}

void RenderGL::DeleteProgram(GLProg& prog) {
	if (prog.Prog)
		glDeleteShader(prog.Prog);
	if (prog.Vert)
		glDeleteShader(prog.Vert);
	if (prog.Frag)
		glDeleteShader(prog.Frag);
	prog = GLProg();
}

bool RenderGL::LoadProgram(GLProg& prog) {
	return LoadProgram(prog.Vert, prog.Frag, prog.Prog, prog.Name(), prog.VertSrc(), prog.FragSrc());
}

bool RenderGL::LoadProgram(GLProg& prog, const char* name, const char* vsrc, const char* fsrc) {
	return LoadProgram(prog.Vert, prog.Frag, prog.Prog, name, vsrc, fsrc);
}

bool RenderGL::LoadProgram(GLuint& vshade, GLuint& fshade, GLuint& prog, const char* name, const char* vsrc, const char* fsrc) {
	Trace("Loading shader %s\n", name);
	XO_ASSERT(glGetError() == GL_NO_ERROR);

	bool isTextRGB = strcmp(name, "TextRGB") == 0;
	bool isUber    = strcmp(name, "Uber") == 0;
	if (isUber)
		int abc = 123;

	if (!LoadShader(GL_VERTEX_SHADER, vshade, name, vsrc))
		return false;
	if (!LoadShader(GL_FRAGMENT_SHADER, fshade, name, fsrc))
		return false;

	prog = glCreateProgram();

	glAttachShader(prog, vshade);
	glAttachShader(prog, fshade);
	if (isTextRGB || isUber) {
// NOTE: The following DOES WORK. It is unnecessary however, on the NVidia hardware that I have tested on.
// BUT: It is necessary on Linux, Haswell Intel drivers
#ifdef glBindFragDataLocationIndexed
		glBindFragDataLocationIndexed(prog, 0, 0, "out_color0");
		glBindFragDataLocationIndexed(prog, 0, 1, "out_color1");
		Check();
#else
		XO_DIE_MSG("glBindFragDataLocationIndexed not defined. Necessary for subpixel RGB text");
#endif
	}
	glLinkProgram(prog);

	int       ilen;
	const int maxBuff = 8000;
	GLchar    ibuff[maxBuff];

	GLint linkStat;
	glGetProgramiv(prog, GL_LINK_STATUS, &linkStat);
	glGetProgramInfoLog(prog, maxBuff, &ilen, ibuff);
	if (ibuff[0] != 0) {
		Trace("Shader: %s\n", name);
		Trace(ibuff);
		Trace("\n");
	}
	bool ok = linkStat != 0 && glGetError() == GL_NO_ERROR;
	if (!ok)
		Trace("Failed to load shader %s: glGetError = %d, linkStat = %d\n", name, glGetError(), linkStat);
	return ok;
}

bool RenderGL::LoadShader(GLenum shaderType, GLuint& shader, const char* name, const char* raw_src) {
	XO_ASSERT(glGetError() == GL_NO_ERROR);

	shader = glCreateShader(shaderType);

	std::string raw_prefix = "";
	std::string raw_other  = raw_src;
	// #version must be on the first line
	if (strstr(raw_src, "#version") == raw_src) {
		size_t firstLine = strstr(raw_src, "\n") - raw_src;
		raw_prefix       = raw_other.substr(0, firstLine + 1);
		raw_other        = raw_other.substr(firstLine + 1);
	}

#if XO_PLATFORM_ANDROID
	if (shaderType == GL_FRAGMENT_SHADER)
		raw_prefix += "precision mediump float;\n";
#elif XO_PLATFORM_WIN_DESKTOP || XO_PLATFORM_LINUX_DESKTOP
	raw_prefix += "#version 130\n"; // necessary for dual color blending
	raw_prefix += "#extension GL_EXT_gpu_shader4 : enable\n"; // for "bool"
#endif

	PreparePreprocessor();
	std::string processed = raw_prefix + BaseShader + raw_other;
	//String processed = Preprocessor.Run( raw_src );
	//String processed(raw_src);
	//processed.ReplaceAll( "XO_GLYPH_ATLAS_SIZE", fmt("%v", GlyphAtlasSize).CStr() );
	//fputs( processed.c_str(), stderr );

	GLchar* vstring[1];
	vstring[0] = (GLchar*) processed.c_str();

	glShaderSource(shader, 1, (const GLchar**) vstring, NULL);

	int       ilen;
	const int maxBuff = 8000;
	GLchar    ibuff[maxBuff];

	GLint compileStat;
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStat);
	glGetShaderInfoLog(shader, maxBuff, &ilen, ibuff);
	//glGetInfoLogARB( shader, maxBuff, &ilen, ibuff );
	if (ibuff[0] != 0) {
		Trace("Shader %s (%s)\n", name, shaderType == GL_FRAGMENT_SHADER ? "frag" : "vert");
		Trace(ibuff);
	}
	if (compileStat == 0)
		return false;

	return glGetError() == GL_NO_ERROR;
}

void RenderGL::Check() {
	int e = glGetError();
	if (e != GL_NO_ERROR) {
		Trace("glError = %d\n", e);
	}
	//XO_ASSERT( glGetError() == GL_NO_ERROR );
}
} // namespace xo
#endif
