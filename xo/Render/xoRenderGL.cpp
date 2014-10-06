#include "pch.h"
#if XO_BUILD_OPENGL
#include "xoRenderGL.h"
#include "../Image/xoImage.h"
#include "xoTextureAtlas.h"
#include "../Text/xoGlyphCache.h"
#include "../xoSysWnd.h"

static bool GLIsBooted = false;

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB                 0x8DB9
#endif
#ifndef GL_RG
#define GL_RG                               0x8227
#endif
#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH                0x0CF2
#endif
#ifndef GL_SRC1_COLOR
#define GL_SRC1_COLOR                       0x88F9
#endif
#ifndef GL_ONE_MINUS_SRC1_COLOR
#define GL_ONE_MINUS_SRC1_COLOR             0x88FA
#endif
#ifndef GL_ONE_MINUS_SRC1_ALPHA
#define GL_ONE_MINUS_SRC1_ALPHA             0x88FB
#endif

static const char* nu_GLSLPrefix = R"(
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
)";

xoRenderGL::xoRenderGL()
{
#if XO_PLATFORM_WIN_DESKTOP
	GLRC = NULL;
	DC = NULL;
#endif
	Have_Unpack_RowLength = false;
	Have_sRGB_Framebuffer = false;
	Have_BlendFuncExtended = false;
	AllProgs[0] = &PRect;
	AllProgs[1] = &PFill;
	AllProgs[2] = &PFillTex;
	AllProgs[3] = &PTextRGB;
	AllProgs[4] = &PTextWhole;
	//AllProgs[5] = &PCurve;
	static_assert(NumProgs == 5, "Add your new shader here");
	Reset();
}

xoRenderGL::~xoRenderGL()
{
}

void xoRenderGL::Reset()
{
	for ( int i = 0; i < NumProgs; i++ )
		AllProgs[i]->Reset();
	memset( BoundTextures, 0, sizeof(BoundTextures) );
	ActiveShader = xoShaderInvalid;
}

const char*	xoRenderGL::RendererName()
{
	return "OpenGL";
}

#if XO_PLATFORM_WIN_DESKTOP

typedef BOOL (*_wglChoosePixelFormatARB) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

static void xoBootGL_FillPFD( PIXELFORMATDESCRIPTOR& pfd )
{
	const DWORD flags = 0
		| PFD_DRAW_TO_WINDOW	// support window
		| PFD_SUPPORT_OPENGL	// support OpenGL 
		| PFD_DOUBLEBUFFER		// double buffer
		| 0;

	// Note that this must match the attribs used by wglChoosePixelFormatARB (I find that strange).
	PIXELFORMATDESCRIPTOR base = { 
		sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
		1,                     // version number 
		flags,
		PFD_TYPE_RGBA,         // RGBA type 
		24,                    // color depth 
		0, 0, 0, 0, 0, 0,      // color bits ignored 
		0,                     // alpha bits
		0,                     // shift bit ignored 
		0,                     // no accumulation buffer 
		0, 0, 0, 0,            // accum bits ignored 
		16,                    // z-buffer 
		0,                     // no stencil buffer 
		0,                     // no auxiliary buffer 
		PFD_MAIN_PLANE,        // main layer 
		0,                     // reserved 
		0, 0, 0                // layer masks ignored 
	};

	pfd = base;
}

static bool xoBootGL( HWND wnd )
{
	HDC dc = GetDC( wnd );

	// get the best available match of pixel format for the device context  
	PIXELFORMATDESCRIPTOR pfd;
	xoBootGL_FillPFD( pfd );
	int iPixelFormat = ChoosePixelFormat( dc, &pfd );

	if ( iPixelFormat != 0 )
	{
		// make that the pixel format of the device context 
		BOOL setOK = SetPixelFormat(dc, iPixelFormat, &pfd); 
		HGLRC rc = wglCreateContext( dc );
		wglMakeCurrent( dc, rc );
		int wglLoad = wgl_LoadFunctions( dc );
		int oglLoad = ogl_LoadFunctions();
		XOTRACE( "wgl_Load: %d\n", wglLoad );
		XOTRACE( "ogl_Load: %d\n", oglLoad );
		GLIsBooted = true;
		wglMakeCurrent( NULL, NULL ); 
		wglDeleteContext( rc );
	}

	ReleaseDC( wnd, dc );

	return true;
}
#endif

#if XO_PLATFORM_WIN_DESKTOP
bool xoRenderGL::InitializeDevice( xoSysWnd& wnd )
{
	if ( !GLIsBooted )
	{
		if ( !xoBootGL( wnd.SysWnd ) )
			return false;
	}

	bool allGood = false;
	HGLRC rc = NULL;
	HDC dc = GetDC( wnd.SysWnd );
	if ( !dc )
		return false;

	int attribs[] =
	{
		WGL_DRAW_TO_WINDOW_ARB,		1,
		WGL_SUPPORT_OPENGL_ARB,		1,
		WGL_DOUBLE_BUFFER_ARB,		1,
		WGL_PIXEL_TYPE_ARB,			WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,			24,
		WGL_ALPHA_BITS_ARB,			0,
		WGL_DEPTH_BITS_ARB,			16,
		WGL_STENCIL_BITS_ARB,		0,
		WGL_SWAP_METHOD_ARB,		WGL_SWAP_EXCHANGE_ARB,	// This was an attempt to lower latency on Windows 8.0, but it seems to have no effect
		0
	};
	PIXELFORMATDESCRIPTOR pfd;
	xoBootGL_FillPFD( pfd );
	int formats[20];
	uint numformats = 0;
	BOOL chooseOK = wglChoosePixelFormatARB( dc, attribs, NULL, arraysize(formats), formats, &numformats );
	if ( chooseOK && numformats != 0 )
	{
		if ( SetPixelFormat( dc, formats[0], &pfd ) )
		{
			rc = wglCreateContext( dc );
			wglMakeCurrent( dc, rc );
			if ( wglSwapIntervalEXT )
				wglSwapIntervalEXT( xoGlobal()->EnableVSync ? 1 : 0 );
			allGood = CreateShaders();
			wglMakeCurrent( NULL, NULL );
		}
		else
		{
			XOTRACE( "SetPixelFormat failed: %d\n", GetLastError() );
		}
	}

	ReleaseDC( wnd.SysWnd, dc );

	if ( !allGood )
	{
		if ( rc )
			wglDeleteContext( rc );
		rc = NULL;
	}

	GLRC = rc;
	return GLRC != NULL;
}
#elif XO_PLATFORM_ANDROID
bool xoRenderGL::InitializeDevice( xoSysWnd& wnd )
{
	if ( !CreateShaders() )
		return false;
	return true;
}
#elif XO_PLATFORM_LINUX_DESKTOP
bool xoRenderGL::InitializeDevice( xoSysWnd& wnd )
{
	int oglLoad = ogl_LoadFunctions();
	int glxLoad = glx_LoadFunctions( wnd.XDisplay, 0 );
	XOTRACE( "oglload: %d\n", oglLoad );
	XOTRACE( "glxload: %d\n", glxLoad );
	if ( !CreateShaders() )
		return false;
	XOTRACE( "Shaders created\n" );
	return true;
}
#else
XOTODO_STATIC
#endif

void xoRenderGL::CheckExtensions()
{
	const char* ver = (const char*) glGetString( GL_VERSION );
	const char* ext = (const char*) glGetString( GL_EXTENSIONS );
	auto hasExtension = [ext]( const char* name ) -> bool
	{
		const char* pos = ext;
		while ( true )
		{
			size_t len = strlen( name );
			pos = strstr( pos, name );
			if ( pos != NULL && (pos[len] == ' ' || pos[len] == 0) )
				return true;
			else if ( pos == NULL )
				return false;
		}
	};

	XOTRACE( "Checking OpenGL extensions\n" );
	// On my desktop Nvidia I get "4.3.0"

	int dot = (int) (strstr(ver, ".") - ver);
	int major = ver[dot - 1] - '0';
	int minor = ver[dot + 1] - '0';
	int version = major * 10 + minor;

	XOTRACE( "OpenGL version: %s\n", ver );
	XOTRACE( "OpenGL extensions: %s\n", ext );

	if ( strstr(ver, "OpenGL ES") )
	{
		XOTRACE( "OpenGL ES\n" );
		Have_Unpack_RowLength = version >= 30 || hasExtension( "GL_EXT_unpack_subimage" );
		Have_sRGB_Framebuffer = version >= 30 || hasExtension( "GL_EXT_sRGB" );
	}
	else
	{
		XOTRACE( "OpenGL Regular (non-ES)\n" );
		Have_Unpack_RowLength = true;
		Have_sRGB_Framebuffer = version >= 40 || hasExtension( "ARB_framebuffer_sRGB" ) || hasExtension( "GL_EXT_framebuffer_sRGB" );
	}

	Have_BlendFuncExtended = hasExtension( "GL_ARB_blend_func_extended" );
	XOTRACE( "OpenGL Extensions ("
		"UNPACK_SUBIMAGE=%d, "
		"sRGB_FrameBuffer=%d, "
		"blend_func_extended=%d"
		")\n",
		Have_Unpack_RowLength ? 1 : 0,
		Have_sRGB_Framebuffer ? 1 : 0,
		Have_BlendFuncExtended ? 1 : 0);
}

bool xoRenderGL::CreateShaders()
{
	Check();

	CheckExtensions();

	Check();

	for ( int i = 0; i < NumProgs; i++ )
	{
		if ( AllProgs[i]->UseOnThisPlatform() )
		{
			if ( !LoadProgram( *AllProgs[i] ) )
				return false;
			if ( !AllProgs[i]->LoadVariablePositions() )
				return false;
		}
	}
	
	Check();

	return true;
}

void xoRenderGL::DeleteShadersAndTextures()
{
	for ( int i = xoMaxTextureUnits - 1; i >= 0; i-- )
	{
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}

	podvec<GLuint> textures;
	for ( intp i = 0; i < TexIDToNative.size(); i++ )
		textures += GetTextureDeviceHandleInt( FirstTextureID() + (xoTextureID) i );

	glDeleteTextures( (GLsizei) textures.size(), &textures[0] );

	glUseProgram( 0 );

	for ( int i = 0; i < NumProgs; i++ )
		DeleteProgram( *AllProgs[i] );
}

xoProgBase* xoRenderGL::GetShader( xoShaders shader )
{
	switch ( shader )
	{
	case xoShaderFill:		return &PFill;
	case xoShaderFillTex:	return &PFillTex;
	case xoShaderRect:		return &PRect;
	case xoShaderTextRGB:	return &PTextRGB;
	case xoShaderTextWhole:	return &PTextWhole;
	default:
		XOASSERT(false);
		return NULL;
	}
}

void xoRenderGL::ActivateShader( xoShaders shader )
{
	if ( ActiveShader == shader )
		return;
	xoGLProg* p = (xoGLProg*) GetShader( shader );
	ActiveShader = shader;
	XOASSERT( p->Prog != 0 );
	XOTRACE_RENDER( "Activate shader %s\n", p->Name() );
	glUseProgram( p->Prog );
	if ( ActiveShader == xoShaderTextRGB )
	{
		// outputColor0 = vec4(color.r, color.g, color.b, avgA);
		// outputColor1 = vec4(aR, aG, aB, avgA);
		if ( Have_BlendFuncExtended )
			glBlendFuncSeparate( GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
		// else we are screwed!
		// there might be a solution that is better than simply ignoring the problem, but I
		// haven't bothered yet. On my Sandy Bridge (i7-2600K) with 2014-04-15 Intel drivers,
		// the DirectX drivers correctly expose this functionality, but the OpenGL drivers do
		// not support GL_ARB_blend_func_extended, so it's very simple - we just use DirectX.
		// I can't think of a device/OS combination where you'd want OpenGL+Subpixel text,
		// so sticking with DirectX 11 there is hopefully fine. Hmm.. actually desktop linux
		// is indeed such a combination. We'll have to wait and see.
	}
	else
	{
		//glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );	// this is for non-premultiplied
		glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );			// this is premultiplied
	}
	Check();
}

void xoRenderGL::DestroyDevice( xoSysWnd& wnd )
{
#if XO_PLATFORM_WIN_DESKTOP
	if ( GLRC != NULL )
	{
		DC = GetDC( wnd.SysWnd );
		wglMakeCurrent( DC, GLRC );
		DeleteShadersAndTextures();
		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( GLRC );
		ReleaseDC( wnd.SysWnd, DC );
		DC = NULL;
		GLRC = NULL;
	}
#endif
}

void xoRenderGL::SurfaceLost()
{
	Reset();
	SurfaceLost_ForgetTextures();
	CreateShaders();
}

bool xoRenderGL::BeginRender( xoSysWnd& wnd )
{
	auto rect = wnd.GetRelativeClientRect();
	FBWidth = rect.Width();
	FBHeight = rect.Height();
	XOTRACE_RENDER( "BeginRender %d x %d\n", FBWidth, FBHeight );
#if XO_PLATFORM_WIN_DESKTOP
	if ( GLRC )
	{
		DC = GetDC( wnd.SysWnd );
		if ( DC )
		{
			wglMakeCurrent( DC, GLRC );
			return true;
		}
	}
	return false;
#elif XO_PLATFORM_ANDROID
	return true
#elif XO_PLATFORM_LINUX_DESKTOP
	glXMakeCurrent( wnd.XDisplay, wnd.XWindow, wnd.GLContext );
#else
	return true;
#endif
}

void xoRenderGL::EndRender( xoSysWnd& wnd )
{
#if XO_PLATFORM_WIN_DESKTOP
	XOTRACE_LATENCY( "SwapBuffers (begin)\n" );
	SwapBuffers( DC );
	wglMakeCurrent( NULL, NULL );
	ReleaseDC( wnd.SysWnd, DC );
	DC = NULL;
	XOTRACE_LATENCY( "SwapBuffers (done)\n" );
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
	XOTRACE_LATENCY( "SwapBuffers (begin)\n" );
	glXSwapBuffers( wnd.XDisplay, wnd.XWindow );
	glXMakeCurrent( wnd.XDisplay, None, NULL );
	XOTRACE_LATENCY( "SwapBuffers (done)\n" );
#endif
}

void xoRenderGL::PreRender()
{
	Check();

	XOTRACE_RENDER( "PreRender %d %d\n", FBWidth, FBHeight );
	Check();

	glViewport( 0, 0, FBWidth, FBHeight );

	auto clear = xoGlobal()->ClearColor;

	if ( xoGlobal()->EnableSRGBFramebuffer && Have_sRGB_Framebuffer )
	{
		glEnable( GL_FRAMEBUFFER_SRGB );
		glClearColor( xoSRGB2Linear(clear.r), xoSRGB2Linear(clear.g), xoSRGB2Linear(clear.b), clear.a / 255.0f );
	}
	else
	{
		glClearColor( clear.r / 255.0f, clear.g / 255.0f, clear.b / 255.0f, clear.a / 255.0f );
	}

	//glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	XOTRACE_RENDER( "PreRender 2\n" );
	Check();

	// Enable CULL_FACE because it will make sure that we are consistent about vertex orientation
	glEnable( GL_CULL_FACE );
	glFrontFace( GL_CCW );
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	XOTRACE_RENDER( "PreRender 3\n" );
	Check();

	SetShaderFrameUniforms();

	XOTRACE_RENDER( "PreRender done\n" );
}

void xoRenderGL::SetShaderFrameUniforms()
{
	XOTRACE_RENDER( "FB %d x %d\n", FBWidth, FBHeight );
	xoMat4f mvproj;
	mvproj.Identity();
	Ortho( mvproj, 0, FBWidth, FBHeight, 0, 1, 0 );
	xoMat4f mvprojT = mvproj.Transposed();

	if ( SetMVProj( xoShaderRect, PRect, mvprojT ) )
		glUniform2f( PRect.v_vport_hsize, FBWidth / 2.0f, FBHeight / 2.0f );

	SetMVProj( xoShaderFill, PFill, mvprojT );
	SetMVProj( xoShaderFillTex, PFillTex, mvprojT );
	SetMVProj( xoShaderTextRGB, PTextRGB, mvprojT );
	SetMVProj( xoShaderTextWhole, PTextWhole, mvprojT );
}

void xoRenderGL::SetShaderObjectUniforms()
{
	if ( ActiveShader == xoShaderRect )
	{
		glUniform4fv( PRect.v_box, 1, &ShaderPerObject.Box.x );
		glUniform4fv( PRect.v_border, 1, &ShaderPerObject.Border.x );
		glUniform4fv( PRect.v_border_color, 1, &ShaderPerObject.BorderColor.x );
		glUniform1f( PRect.v_radius, ShaderPerObject.Radius );
	}
}

template<typename TProg>
bool xoRenderGL::SetMVProj( xoShaders shader, TProg& prog, const Mat4f& mvprojTransposed )
{
	if ( prog.Prog == 0 )
	{
		XOTRACE_RENDER( "SetMVProj skipping %s, because not compiled\n", (const char*) prog.Name() );
		return false;
	}
	else
	{
		XOTRACE_RENDER( "SetMVProj %s (%d)\n", (const char*) prog.Name(), prog.v_mvproj );
		ActivateShader( shader );
		Check();
		// GLES doesn't support TRANSPOSE = TRUE
		glUniformMatrix4fv( prog.v_mvproj, 1, false, &mvprojTransposed.row[0].x );
		return true;
	}
}

GLint xoRenderGL::TexFilterToGL( xoTexFilter f )
{
	switch (f)
	{
	case xoTexFilterNearest:	return GL_NEAREST;
	case xoTexFilterLinear:		return GL_LINEAR;
	default: XOTODO;
	}
	return GL_NEAREST;
}

void xoRenderGL::PostRenderCleanup()
{
	glUseProgram( 0 );
	ActiveShader = xoShaderInvalid;
}

void xoRenderGL::DrawQuad( const void* v )
{
	XOTRACE_RENDER( "DrawQuad\n" );

	SetShaderObjectUniforms();

	int stride = sizeof(xoVx_PTC);
	const byte* vbyte = (const byte*) v;

	GLint varvpos = 0;
	GLint varvcol = 0;
	GLint varvtex0 = 0;
	GLint varvtexClamp = 0;
	GLint vartexUnit0 = 0;
	switch ( ActiveShader )
	{
	case xoShaderRect:
		varvpos = PRect.v_vpos;
		varvcol = PRect.v_vcolor;
		break;
	case xoShaderFill:
		varvpos = PFill.v_vpos;
		varvcol = PFill.v_vcolor;
		XOTRACE_RENDER( "vv %d %d\n", varvpos, varvcol );
		break;
	case xoShaderFillTex:
		varvpos = PFillTex.v_vpos;
		varvcol = PFillTex.v_vcolor;
		varvtex0 = PFillTex.v_vtexuv0;
		vartexUnit0 = PFillTex.v_tex0;
		break;
	case xoShaderTextRGB:
		stride = sizeof(xoVx_PTCV4);
		varvpos = PTextRGB.v_vpos;
		varvcol = PTextRGB.v_vcolor;
		varvtex0 = PTextRGB.v_vtexuv0;
		varvtexClamp = PTextRGB.v_vtexClamp;
		vartexUnit0 = PTextRGB.v_tex0;
		break;
	case xoShaderTextWhole:
		varvpos = PTextWhole.v_vpos;
		varvcol = PTextWhole.v_vcolor;
		varvtex0 = PTextWhole.v_vtexuv0;
		vartexUnit0 = PTextWhole.v_tex0;
		break;
	}

	// We assume here that xoVx_PTC and xoVx_PTCV4 share the same base layout
	glVertexAttribPointer( varvpos, 3, GL_FLOAT, false, stride, vbyte );
	glEnableVertexAttribArray( varvpos );
	
	glVertexAttribPointer( varvcol, 4, GL_UNSIGNED_BYTE, true, stride, vbyte + offsetof(xoVx_PTC, Color) );
	glEnableVertexAttribArray( varvcol );

	if ( varvtex0 != 0 )
	{
		glUniform1i( vartexUnit0, 0 );
		glVertexAttribPointer( varvtex0, 2, GL_FLOAT, true, stride, vbyte + offsetof(xoVx_PTC, UV) );
		glEnableVertexAttribArray( varvtex0 );
	}
	if ( varvtexClamp != 0 )
	{
		glVertexAttribPointer( varvtexClamp, 4, GL_FLOAT, true, stride, vbyte + offsetof(xoVx_PTCV4, V4) );
		glEnableVertexAttribArray( varvtexClamp );
	}

	uint16 indices[4];
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 3;
	indices[3] = 2;
	glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, indices );

	//auto vx = (xoVx_PTC*) vbyte;
	//XOTRACE_RENDER( "DrawQuad done (%f,%f) (%f,%f) (%f,%f) (%f,%f)\n", vx[0].Pos.x, vx[0].Pos.y, vx[1].Pos.x, vx[1].Pos.y, vx[2].Pos.x, vx[2].Pos.y, vx[3].Pos.x, vx[3].Pos.y );

	Check();
}

/*
void xoRenderGL::DrawTriangles( int nvert, const void* v, const uint16* indices )
{
	int stride = sizeof(xoVx_PTC);
	const byte* vbyte = (const byte*) v;
	glDrawElements( GL_TRIANGLES, nvert, GL_UNSIGNED_SHORT, indices );
	Check();
}
*/

bool xoRenderGL::LoadTexture( xoTexture* tex, int texUnit )
{
	EnsureTextureProperlyDefined( tex, texUnit );

	if ( !IsTextureValid( tex->TexID ) )
	{
		GLuint t;
		glGenTextures( 1, &t );
		tex->TexID = RegisterTextureInt( t );
		tex->TexInvalidateWholeSurface();
	}

	GLuint glTexID = GetTextureDeviceHandleInt( tex->TexID );
	if ( BoundTextures[texUnit] != glTexID )
	{
		glActiveTexture( GL_TEXTURE0 + texUnit );
		glBindTexture( GL_TEXTURE_2D, glTexID );
		BoundTextures[texUnit] = glTexID;
	}

	// This happens when a texture fails to upload to the GPU during synchronization from UI doc to render doc.
	if ( tex->TexData == nullptr )
		return true;

	// If the texture has not been updated, then we are done
	xoBox invRect = tex->TexInvalidRect;
	xoBox fullRect = xoBox(0, 0, tex->TexWidth, tex->TexHeight);
	invRect.ClampTo( fullRect );
	if ( !invRect.IsAreaPositive() )
		return true;

	int format = 0;
	switch ( tex->TexFormat )
	{
	case xoTexFormatGrey8: format = GL_RED; break;	// was luminance
	//case : format = GL_RG;
	//case : format = GL_RGB;
	case xoTexFormatRGBA8: format = GL_RGBA; break;
	default:
		XOTODO;
	}
	int iformat = format;

	if ( Have_Unpack_RowLength )
		glPixelStorei( GL_UNPACK_ROW_LENGTH, tex->TexStride / (int) tex->TexBytesPerPixel() );

	if ( !Have_Unpack_RowLength || invRect == fullRect )
	{
		glTexImage2D( GL_TEXTURE_2D, 0, iformat, tex->TexWidth, tex->TexHeight, 0, format, GL_UNSIGNED_BYTE, tex->TexData );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TexFilterToGL(tex->TexFilterMin) );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TexFilterToGL(tex->TexFilterMax) );
		// Clamping should have no effect for RGB text, since we clamp inside our fragment shader.
		// Also, when rendering 'whole pixel' glyphs, we shouldn't need clamping either, because
		// our UV coordinates are exact, and we always have a 1:1 texel:pixel ratio.
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}
	else
	{
		glTexSubImage2D( GL_TEXTURE_2D, 0, invRect.Left, invRect.Top, invRect.Width(), invRect.Height(), format, GL_UNSIGNED_BYTE, tex->TexDataAt(invRect.Left, invRect.Top) );
	}

	if ( Have_Unpack_RowLength )
		glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

	return true;
}

bool xoRenderGL::ReadBackbuffer( xoImage& image )
{
	image.Alloc( xoTexFormatRGBA8, FBWidth, FBHeight );
	if ( Have_Unpack_RowLength )
		glPixelStorei( GL_UNPACK_ROW_LENGTH, image.TexStride / 4 );

	glReadPixels( 0, 0, FBWidth, FBHeight, GL_RGBA, GL_UNSIGNED_BYTE, image.TexDataAtLine(0) );

	// our image is top-down
	// glReadPixels is bottom-up
	image.FlipVertical();

	if ( Have_Unpack_RowLength )
		glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

	return true;
}

void xoRenderGL::PreparePreprocessor()
{
	if ( BaseShader.size() != 0 )
		return;

#if XO_PLATFORM_WIN_DESKTOP
	BaseShader.append( "#define XO_PLATFORM_WIN_DESKTOP\n" );
#elif XO_PLATFORM_ANDROID
	BaseShader.append( "#define XO_PLATFORM_ANDROID\n" );
#elif XO_PLATFORM_LINUX_DESKTOP
	BaseShader.append( "#define XO_PLATFORM_LINUX_DESKTOP\n" );
#else
	#ifdef _MSC_VER
		#pragma error( "Unknown xoDom platform" )
	#else
		#error Unknown xoDom platform
	#endif
#endif

	BaseShader.append( CommonShaderDefines() );
	if ( xoGlobal()->EnableSRGBFramebuffer && Have_sRGB_Framebuffer )
		BaseShader.append( "#define XO_SRGB_FRAMEBUFFER\n" );

	BaseShader += nu_GLSLPrefix;

	/*
	if ( Preprocessor.MacroCount() != 0 )
		return;

	Preprocessor.SetMacro( "XO_GLYPH_ATLAS_SIZE", fmt("%v", xoGlyphAtlasSize).Z );

	if ( xoGlobal()->EnableSRGBFramebuffer )
		Preprocessor.SetMacro( "XO_SRGB_FRAMEBUFFER", "" );
		*/

	//if ( xoGlobal()->EnableSRGBFramebuffer && xoGlobal()->EmulateGammaBlending )
	//	Preprocessor.SetMacro( "XO_EMULATE_GAMMA_BLENDING", "" );
}

void xoRenderGL::DeleteProgram( xoGLProg& prog )
{
	if ( prog.Prog ) glDeleteShader( prog.Prog );
	if ( prog.Vert ) glDeleteShader( prog.Vert );
	if ( prog.Frag ) glDeleteShader( prog.Frag );
	prog = xoGLProg();
}

bool xoRenderGL::LoadProgram( xoGLProg& prog )
{
	return LoadProgram( prog.Vert, prog.Frag, prog.Prog, prog.Name(), prog.VertSrc(), prog.FragSrc() );
}

bool xoRenderGL::LoadProgram( xoGLProg& prog, const char* name, const char* vsrc, const char* fsrc )
{
	return LoadProgram( prog.Vert, prog.Frag, prog.Prog, name, vsrc, fsrc );
}

bool xoRenderGL::LoadProgram( GLuint& vshade, GLuint& fshade, GLuint& prog, const char* name, const char* vsrc, const char* fsrc )
{
	XOTRACE("Loading shader %s\n", name);
	XOASSERT(glGetError() == GL_NO_ERROR);

	bool isTextRGB = strcmp(name, "TextRGB") == 0;

	if ( !LoadShader( GL_VERTEX_SHADER, vshade, name, vsrc ) ) return false;
	if ( !LoadShader( GL_FRAGMENT_SHADER, fshade, name, fsrc ) ) return false;

	prog = glCreateProgram();

	glAttachShader( prog, vshade );
	glAttachShader( prog, fshade );
	if ( isTextRGB )
	{
		// NOTE: The following DOES WORK. It is unnecessary however, on the NVidia hardware that I have tested on.
		// BUT: It is necessary on Linux, Haswell Intel drivers
		glBindFragDataLocationIndexed( prog, 0, 0, "outputColor0" );
		glBindFragDataLocationIndexed( prog, 0, 1, "outputColor1" );
	}
	glLinkProgram( prog );

	int ilen;
	const int maxBuff = 8000;
	GLchar ibuff[maxBuff];

	GLint linkStat;
	glGetProgramiv( prog, GL_LINK_STATUS, &linkStat );
	glGetProgramInfoLog( prog, maxBuff, &ilen, ibuff );
	if ( ibuff[0] != 0 )
	{
		XOTRACE( "Shader: %s\n", name );
		XOTRACE( ibuff );
		XOTRACE( "\n" );
	}
	bool ok = linkStat != 0 && glGetError() == GL_NO_ERROR;
	if ( !ok )
		XOTRACE( "Failed to load shader %s: glGetError = %d, linkStat = %d\n", name, glGetError(), linkStat );
	return ok;
}

bool xoRenderGL::LoadShader( GLenum shaderType, GLuint& shader, const char* name, const char* raw_src )
{
	XOASSERT(glGetError() == GL_NO_ERROR);

	shader = glCreateShader( shaderType );

	std::string raw_prefix = "";
	std::string raw_other = raw_src;
	// #version must be on the first line
	if ( strstr(raw_src, "#version") == raw_src )
	{
		size_t firstLine = strstr(raw_src, "\n") - raw_src;
		raw_prefix = raw_other.substr( 0, firstLine + 1 );
		raw_other = raw_other.substr( firstLine + 1 );
	}

	PreparePreprocessor();
	std::string processed = raw_prefix + BaseShader + raw_other;
	//xoString processed = Preprocessor.Run( raw_src );
	//xoString processed(raw_src);
	//processed.ReplaceAll( "XO_GLYPH_ATLAS_SIZE", fmt("%v", xoGlyphAtlasSize).Z );
	//fputs( processed.c_str(), stderr );

	GLchar* vstring[1];
	vstring[0] = (GLchar*) processed.c_str();

	glShaderSource( shader, 1, (const GLchar**) vstring, NULL );
	
	int ilen;
	const int maxBuff = 8000;
	GLchar ibuff[maxBuff];

	GLint compileStat;
	glCompileShader( shader );
	glGetShaderiv( shader, GL_COMPILE_STATUS, &compileStat );
	glGetShaderInfoLog( shader, maxBuff, &ilen, ibuff );
	//glGetInfoLogARB( shader, maxBuff, &ilen, ibuff );
	if ( ibuff[0] != 0 )
	{
		XOTRACE( "Shader %s (%s)\n", name, shaderType == GL_FRAGMENT_SHADER ? "frag" : "vert" );
		XOTRACE( ibuff );
	}
	if ( compileStat == 0 )
		return false;

	return glGetError() == GL_NO_ERROR;
}

void xoRenderGL::Check()
{
	int e = glGetError();
	if ( e != GL_NO_ERROR )
	{
		XOTRACE( "glError = %d\n", e );
	}
	//XOASSERT( glGetError() == GL_NO_ERROR );
}

#endif
