#include "pch.h"
#include "nuRenderGL.h"
#include "../Image/nuImage.h"
#include "nuTextureAtlas.h"
#include "../Text/nuGlyphCache.h"
#include "../nuSysWnd.h"

void initGLExt();

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

nuRenderGL::nuRenderGL()
{
#if NU_PLATFORM_WIN_DESKTOP
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
	AllProgs[5] = &PCurve;
	static_assert(NumProgs == 6, "Add your new shader here");
	Reset();
}

nuRenderGL::~nuRenderGL()
{
}

void nuRenderGL::Reset()
{
	for ( int i = 0; i < NumProgs; i++ )
		AllProgs[i]->Reset();
	memset( BoundTextures, 0, sizeof(BoundTextures) );
	ActiveShader = nuShaderInvalid;
}

#if NU_PLATFORM_WIN_DESKTOP

typedef BOOL (*_wglChoosePixelFormatARB) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

static void nuBootGL_FillPFD( PIXELFORMATDESCRIPTOR& pfd )
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

static bool nuBootGL( HWND wnd )
{
	HDC dc = GetDC( wnd );

	// get the best available match of pixel format for the device context  
	PIXELFORMATDESCRIPTOR pfd;
	nuBootGL_FillPFD( pfd );
	int iPixelFormat = ChoosePixelFormat( dc, &pfd );

	if ( iPixelFormat != 0 )
	{
		// make that the pixel format of the device context 
		BOOL setOK = SetPixelFormat(dc, iPixelFormat, &pfd); 
		HGLRC rc = wglCreateContext( dc );
		wglMakeCurrent( dc, rc );
		initGLExt();
		GLIsBooted = true;
		//biggleInit();
		wglMakeCurrent( NULL, NULL ); 
		wglDeleteContext( rc );
	}

	ReleaseDC( wnd, dc );

	return true;
}

bool nuRenderGL::InitializeDevice( nuSysWnd& wnd )
{
	if ( !GLIsBooted )
	{
		if ( !nuBootGL( wnd.SysWnd ) )
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
	nuBootGL_FillPFD( pfd );
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
				wglSwapIntervalEXT( nuGlobal()->EnableVSync ? 1 : 0 );
			allGood = CreateShaders();
			wglMakeCurrent( NULL, NULL );
		}
		else
		{
			NUTRACE( "SetPixelFormat failed: %d\n", GetLastError() );
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
#endif

void nuRenderGL::CheckExtensions()
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

	NUTRACE( "Checking OpenGL extensions\n" );
	// On my desktop Nvidia I get "4.3.0"

	int dot = (int) (strstr(ver, ".") - ver);
	int major = ver[dot - 1] - '0';
	int minor = ver[dot + 1] - '0';
	int version = major * 10 + minor;

	NUTRACE( "OpenGL version: %s\n", ver );
	NUTRACE( "OpenGL extensions: %s\n", ext );

	if ( strstr(ver, "OpenGL ES") )
	{
		NUTRACE( "OpenGL ES\n" );
		Have_Unpack_RowLength = version >= 30 || hasExtension( "GL_EXT_unpack_subimage" );
		Have_sRGB_Framebuffer = version >= 30 || hasExtension( "GL_EXT_sRGB" );
	}
	else
	{
		NUTRACE( "OpenGL Regular (non-ES)\n" );
		Have_Unpack_RowLength = true;
		Have_sRGB_Framebuffer = version >= 40 || hasExtension( "ARB_framebuffer_sRGB" ) || hasExtension( "GL_EXT_framebuffer_sRGB" );
	}

	Have_BlendFuncExtended = hasExtension( "GL_ARB_blend_func_extended" );
	NUTRACE( "OpenGL Extensions ("
		"UNPACK_SUBIMAGE=%d, "
		"sRGB_FrameBuffer=%d, "
		"blend_func_extended=%d"
		")\n",
		Have_Unpack_RowLength ? 1 : 0,
		Have_sRGB_Framebuffer ? 1 : 0,
		Have_BlendFuncExtended ? 1 : 0);
}

bool nuRenderGL::CreateShaders()
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

void nuRenderGL::DeleteShadersAndTextures()
{
	for ( int i = nuMaxTextureUnits - 1; i >= 0; i-- )
	{
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}

	podvec<GLuint> textures;
	for ( intp i = 0; i < TexIDToNative.size(); i++ )
		textures += GetTextureDeviceIDInt( FirstTextureID() + (nuTextureID) i );

	glDeleteTextures( (GLsizei) textures.size(), &textures[0] );

	glUseProgram( 0 );

	for ( int i = 0; i < NumProgs; i++ )
		DeleteProgram( *AllProgs[i] );
}

nuProgBase* nuRenderGL::GetShader( nuShaders shader )
{
	switch ( shader )
	{
	case nuShaderFill:		return &PFill;
	case nuShaderFillTex:	return &PFillTex;
	case nuShaderRect:		return &PRect;
	case nuShaderTextRGB:	return &PTextRGB;
	case nuShaderTextWhole:	return &PTextWhole;
	default:
		NUASSERT(false);
		return NULL;
	}
}

void nuRenderGL::ActivateShader( nuShaders shader )
{
	if ( ActiveShader == shader )
		return;
	nuGLProg* p = (nuGLProg*) GetShader( shader );
	ActiveShader = shader;
	NUASSERT( p->Prog != 0 );
	glUseProgram( p->Prog );
	if ( ActiveShader == nuShaderTextRGB )
	{
		// outputColor0 = vec4(color.r, color.g, color.b, avgA);
		// outputColor1 = vec4(aR, aG, aB, avgA);
		glBlendFuncSeparate( GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
	}
	else
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );	// this is for non-premultiplied
		//glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );			// this is premultiplied
	}
	Check();
}

void nuRenderGL::DestroyDevice( nuSysWnd& wnd )
{
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
}

void nuRenderGL::SurfaceLost()
{
	Reset();
	SurfaceLost_ForgetTextures();
	CreateShaders();
}

bool nuRenderGL::BeginRender( nuSysWnd& wnd )
{
	auto rect = wnd.GetRelativeClientRect();
	FBWidth = rect.Width();
	FBHeight = rect.Height();
#if NU_PLATFORM_WIN_DESKTOP
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
#else
	return true;
#endif
}

void nuRenderGL::EndRender( nuSysWnd& wnd )
{
#if NU_PLATFORM_WIN_DESKTOP
	NUTRACE_LATENCY( "SwapBuffers (begin)\n" );
	SwapBuffers( DC );
	wglMakeCurrent( NULL, NULL );
	ReleaseDC( wnd.SysWnd, DC );
	DC = NULL;
	NUTRACE_LATENCY( "SwapBuffers (done)\n" );
#endif
}

void nuRenderGL::PreRender()
{
	Check();

	NUTRACE_RENDER( "PreRender %d %d\n", FBWidth, FBHeight );
	Check();

	glViewport( 0, 0, FBWidth, FBHeight );

	auto clear = nuGlobal()->ClearColor;
	glClearColor( clear.r / 255.0f, clear.g / 255.0f, clear.b / 255.0f, clear.a / 255.0f );

	//glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	if ( nuGlobal()->EnableSRGBFramebuffer && Have_sRGB_Framebuffer )
		glEnable( GL_FRAMEBUFFER_SRGB );

	NUTRACE_RENDER( "PreRender 2\n" );
	Check();

	// Enable CULL_FACE because it will make sure that we are consistent about vertex orientation
	glEnable( GL_CULL_FACE );
	glFrontFace( GL_CCW );
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	NUTRACE_RENDER( "PreRender 3\n" );
	Check();

	SetShaderFrameUniforms();

	NUTRACE_RENDER( "PreRender done\n" );
}

void nuRenderGL::SetShaderFrameUniforms()
{
	nuMat4f mvproj;
	mvproj.Identity();
	Ortho( mvproj, 0, FBWidth, FBHeight, 0, 1, 0 );
	nuMat4f mvprojT = mvproj.Transposed();

	if ( SetMVProj( nuShaderRect, PRect, mvprojT ) )
		glUniform2f( PRect.v_vport_hsize, FBWidth / 2.0f, FBHeight / 2.0f );

	SetMVProj( nuShaderFill, PFill, mvprojT );
	SetMVProj( nuShaderFillTex, PFillTex, mvprojT );
	SetMVProj( nuShaderTextRGB, PTextRGB, mvprojT );
	SetMVProj( nuShaderTextWhole, PTextWhole, mvprojT );
}

void nuRenderGL::SetShaderObjectUniforms()
{
	if ( ActiveShader == nuShaderRect )
	{
		glUniform4fv( PRect.v_box, 1, &ShaderPerObject.Box.x );
		glUniform1f( PRect.v_radius, ShaderPerObject.Radius );
	}
}

template<typename TProg>
bool nuRenderGL::SetMVProj( nuShaders shader, TProg& prog, const Mat4f& mvprojTransposed )
{
	if ( prog.Prog == 0 )
	{
		NUTRACE_RENDER( "SetMVProj skipping %s, because not compiled\n", (const char*) prog.Name() );
		return false;
	}
	else
	{
		NUTRACE_RENDER( "SetMVProj %s (%d)\n", (const char*) prog.Name(), prog.v_mvproj );
		ActivateShader( shader );
		Check();
		// GLES doesn't support TRANSPOSE = TRUE
		glUniformMatrix4fv( prog.v_mvproj, 1, false, &mvprojTransposed.row[0].x );
		return true;
	}
}

void nuRenderGL::PostRenderCleanup()
{
	glUseProgram( 0 );
	ActiveShader = nuShaderInvalid;
}

void nuRenderGL::DrawQuad( const void* v )
{
	NUTRACE_RENDER( "DrawQuad\n" );

	SetShaderObjectUniforms();

	int stride = sizeof(nuVx_PTC);
	const byte* vbyte = (const byte*) v;

	GLint varvpos = 0;
	GLint varvcol = 0;
	GLint varvtex0 = 0;
	GLint varvtexClamp = 0;
	GLint vartexUnit0 = 0;
	switch ( ActiveShader )
	{
	case nuShaderRect:
		varvpos = PRect.v_vpos;
		varvcol = PRect.v_vcolor;
		break;
	case nuShaderFill:
		varvpos = PFill.v_vpos;
		varvcol = PFill.v_vcolor;
		break;
	case nuShaderFillTex:
		varvpos = PFillTex.v_vpos;
		varvcol = PFillTex.v_vcolor;
		varvtex0 = PFillTex.v_vtexuv0;
		vartexUnit0 = PFillTex.v_tex0;
		break;
	case nuShaderTextRGB:
		stride = sizeof(nuVx_PTCV4);
		varvpos = PTextRGB.v_vpos;
		varvcol = PTextRGB.v_vcolor;
		varvtex0 = PTextRGB.v_vtexuv0;
		varvtexClamp = PTextRGB.v_vtexClamp;
		vartexUnit0 = PTextRGB.v_tex0;
		break;
	case nuShaderTextWhole:
		varvpos = PTextWhole.v_vpos;
		varvcol = PTextWhole.v_vcolor;
		varvtex0 = PTextWhole.v_vtexuv0;
		vartexUnit0 = PTextWhole.v_tex0;
		break;
	}

	// We assume here that nuVx_PTC and nuVx_PTCV4 share the same base layout
	glVertexAttribPointer( varvpos, 3, GL_FLOAT, false, stride, vbyte );
	glEnableVertexAttribArray( varvpos );
	
	glVertexAttribPointer( varvcol, 4, GL_UNSIGNED_BYTE, true, stride, vbyte + offsetof(nuVx_PTC, Color) );
	glEnableVertexAttribArray( varvcol );

	if ( varvtex0 != 0 )
	{
		glUniform1i( vartexUnit0, 0 );
		glVertexAttribPointer( varvtex0, 2, GL_FLOAT, true, stride, vbyte + offsetof(nuVx_PTC, UV) );
		glEnableVertexAttribArray( varvtex0 );
	}
	if ( varvtexClamp != 0 )
	{
		glVertexAttribPointer( varvtexClamp, 4, GL_FLOAT, true, stride, vbyte + offsetof(nuVx_PTCV4, V4) );
		glEnableVertexAttribArray( varvtexClamp );
	}

	uint16 indices[4];
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 3;
	indices[3] = 2;
	glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, indices );

	NUTRACE_RENDER( "DrawQuad done\n" );

	Check();
}

/*
void nuRenderGL::DrawTriangles( int nvert, const void* v, const uint16* indices )
{
	int stride = sizeof(nuVx_PTC);
	const byte* vbyte = (const byte*) v;
	glDrawElements( GL_TRIANGLES, nvert, GL_UNSIGNED_SHORT, indices );
	Check();
}
*/

void nuRenderGL::LoadTexture( nuTexture* tex, int texUnit )
{
	NUASSERT( tex->TexWidth != 0 && tex->TexHeight != 0 );
	NUASSERT( tex->TexFormat != nuTexFormatInvalid );
	NUASSERT( texUnit < nuMaxTextureUnits );
	if ( !IsTextureValid( tex->TexID ) )
	{
		GLuint t;
		glGenTextures( 1, &t );
		tex->TexID = RegisterTextureInt( t );
		tex->TexInvalidate();
	}

	GLuint glTexID = GetTextureDeviceIDInt( tex->TexID );
	if ( BoundTextures[texUnit] == glTexID )
		return;

	glActiveTexture( GL_TEXTURE0 + texUnit );
	glBindTexture( GL_TEXTURE_2D, glTexID );
	BoundTextures[texUnit] = glTexID;

	// If the texture has not been updated, then we are done
	nuBox invRect = tex->TexInvalidRect;
	nuBox fullRect = nuBox(0, 0, tex->TexWidth, tex->TexHeight);
	invRect.ClampTo( fullRect );
	if ( invRect.IsAreaZero() )
		return;

	int format = 0;
	switch ( tex->TexFormat )
	{
	case nuTexFormatGrey8: format = GL_LUMINANCE; break;
	//case : format = GL_RG;
	//case : format = GL_RGB;
	case nuTexFormatRGBA8: format = GL_RGBA; break;
	default:
		NUTODO;
	}
	int iformat = format;

	if ( Have_Unpack_RowLength )
		glPixelStorei( GL_UNPACK_ROW_LENGTH, tex->TexStride );

	if ( !Have_Unpack_RowLength || invRect == fullRect )
	{
		glTexImage2D( GL_TEXTURE_2D, 0, iformat, tex->TexWidth, tex->TexHeight, 0, format, GL_UNSIGNED_BYTE, tex->TexData );

		// all assuming this is for a glyph atlas
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
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
}

void nuRenderGL::ReadBackbuffer( nuImage& image )
{
	image.Alloc( nuTexFormatRGBA8, FBWidth, FBHeight );
	if ( Have_Unpack_RowLength )
		glPixelStorei( GL_UNPACK_ROW_LENGTH, image.TexStride / 4 );

	glReadPixels( 0, 0, FBWidth, FBHeight, GL_RGBA, GL_UNSIGNED_BYTE, image.TexDataAtLine(0) );

	// our image is top-down
	// glReadPixels is bottom-up
	image.FlipVertical();

	if ( Have_Unpack_RowLength )
		glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
}

void nuRenderGL::PreparePreprocessor()
{
	if ( BaseShader.size() != 0 )
		return;

#if NU_PLATFORM_WIN_DESKTOP
	BaseShader.append( "#define NU_PLATFORM_WIN_DESKTOP\n" );
#elif NU_PLATFORM_ANDROID
	BaseShader.append( "#define NU_PLATFORM_ANDROID\n" );
#else
	#ifdef _MSC_VER
		#pragma error( "Unknown nuDom platform" )
	#else
		#error Unknown nuDom platform
	#endif
#endif

	BaseShader.append( fmt( "#define NU_GLYPH_ATLAS_SIZE %v\n", nuGlyphAtlasSize ).Z );
	if ( nuGlobal()->EnableSRGBFramebuffer && Have_sRGB_Framebuffer )
		BaseShader.append( "#define NU_SRGB_FRAMEBUFFER\n" );

	/*
	if ( Preprocessor.MacroCount() != 0 )
		return;

	Preprocessor.SetMacro( "NU_GLYPH_ATLAS_SIZE", fmt("%v", nuGlyphAtlasSize).Z );

	if ( nuGlobal()->EnableSRGBFramebuffer )
		Preprocessor.SetMacro( "NU_SRGB_FRAMEBUFFER", "" );
		*/

	//if ( nuGlobal()->EnableSRGBFramebuffer && nuGlobal()->EmulateGammaBlending )
	//	Preprocessor.SetMacro( "NU_EMULATE_GAMMA_BLENDING", "" );
}

void nuRenderGL::DeleteProgram( nuGLProg& prog )
{
	if ( prog.Prog ) glDeleteShader( prog.Prog );
	if ( prog.Vert ) glDeleteShader( prog.Vert );
	if ( prog.Frag ) glDeleteShader( prog.Frag );
	prog = nuGLProg();
}

bool nuRenderGL::LoadProgram( nuGLProg& prog )
{
	return LoadProgram( prog.Vert, prog.Frag, prog.Prog, prog.Name(), prog.VertSrc(), prog.FragSrc() );
}

bool nuRenderGL::LoadProgram( nuGLProg& prog, const char* name, const char* vsrc, const char* fsrc )
{
	return LoadProgram( prog.Vert, prog.Frag, prog.Prog, name, vsrc, fsrc );
}

bool nuRenderGL::LoadProgram( GLuint& vshade, GLuint& fshade, GLuint& prog, const char* name, const char* vsrc, const char* fsrc )
{
	NUASSERT(glGetError() == GL_NO_ERROR);

	bool isTextRGB = strcmp(name, "TextRGB") == 0;

	if ( !LoadShader( GL_VERTEX_SHADER, vshade, name, vsrc ) ) return false;
	if ( !LoadShader( GL_FRAGMENT_SHADER, fshade, name, fsrc ) ) return false;

	prog = glCreateProgram();

	glAttachShader( prog, vshade );
	glAttachShader( prog, fshade );
	if ( isTextRGB )
	{
		// NOTE: The following DOES WORK. It is unnecessary however, on the NVidia hardware that I have tested on.
		//glBindFragDataLocationIndexed( prog, 0, 0, "outputColor0" );
		//glBindFragDataLocationIndexed( prog, 0, 1, "outputColor1" );
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
		NUTRACE( "Shader: %s\n", name );
		NUTRACE( ibuff );
		NUTRACE( "\n" );
	}
	bool ok = linkStat != 0 && glGetError() == GL_NO_ERROR;
	if ( !ok )
		NUTRACE( "Failed to load shader %s: glGetError = %d, linkStat = %d\n", name, glGetError(), linkStat );
	return ok;
}

bool nuRenderGL::LoadShader( GLenum shaderType, GLuint& shader, const char* name, const char* raw_src )
{
	NUASSERT(glGetError() == GL_NO_ERROR);

	shader = glCreateShader( shaderType );

	PreparePreprocessor();
	std::string processed = BaseShader + raw_src;
	//nuString processed = Preprocessor.Run( raw_src );
	//nuString processed(raw_src);
	//processed.ReplaceAll( "NU_GLYPH_ATLAS_SIZE", fmt("%v", nuGlyphAtlasSize).Z );

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
		NUTRACE( "Shader %s (%s)\n", name, shaderType == GL_FRAGMENT_SHADER ? "frag" : "vert" );
		NUTRACE( ibuff );
	}
	if ( compileStat == 0 )
		return false;

	return glGetError() == GL_NO_ERROR;
}

void nuRenderGL::Check()
{
	int e = glGetError();
	if ( e != GL_NO_ERROR )
	{
		NUTRACE( "glError = %d\n", e );
	}
	//NUASSERT( glGetError() == GL_NO_ERROR );
}

