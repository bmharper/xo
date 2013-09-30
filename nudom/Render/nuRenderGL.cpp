#include "pch.h"
#include "nuRenderGL.h"
#include "../Image/nuImage.h"
#include "nuTextureAtlas.h"
#include "../Text/nuGlyphCache.h"

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
	Have_Unpack_RowLength = false;
	Have_sRGB_Framebuffer = false;
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
}

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
		Have_Unpack_RowLength = version >= 30 || hasExtension( "GL_EXT_unpack_subimage" );
		Have_sRGB_Framebuffer = version >= 30 || hasExtension( "GL_EXT_sRGB" );
		NUTRACE( "OpenGL ES (UNPACK_SUBIMAGE=%d, sRGB_FrameBuffer=%d)\n", Have_Unpack_RowLength ? 1 : 0, Have_sRGB_Framebuffer ? 1 : 0 );
	}
	else
	{
		NUTRACE( "OpenGL Regular (non-ES)\n" );
		Have_Unpack_RowLength = true;
		Have_sRGB_Framebuffer = version >= 40 || hasExtension( "ARB_framebuffer_sRGB" ) || hasExtension( "GL_EXT_framebuffer_sRGB" );
	}
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
			if ( !LoadProgram( *AllProgs[i] ) ) return false;
			if ( !AllProgs[i]->LoadVariablePositions() ) return false;
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

void nuRenderGL::SurfaceLost()
{
	Reset();
	SurfaceLost_ForgetTextures();
	CreateShaders();
}

void nuRenderGL::ActivateProgram( nuGLProg& p )
{
	if ( ActiveProgram == &p )
		return;
	ActiveProgram = &p;
	NUASSERT( p.Prog != 0 );
	glUseProgram( p.Prog );
	if ( ActiveProgram == &PTextRGB )
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

void nuRenderGL::Ortho( nuMat4f &imat, double left, double right, double bottom, double top, double znear, double zfar )
{
	nuMat4f m;
	m.Zero();
	double A = 2 / (right - left);
	double B = 2 / (top - bottom);
	double C = -2 / (zfar - znear);
	double tx = -(right + left) / (right - left);
	double ty = -(top + bottom) / (top - bottom);
	double tz = -(zfar + znear) / (zfar - znear);
	m.m(0,0) = (float) A;
	m.m(1,1) = (float) B;
	m.m(2,2) = (float) C;
	m.m(3,3) = 1;
	m.m(0,3) = (float) tx;
	m.m(1,3) = (float) ty;
	m.m(2,3) = (float) tz;
	imat = imat * m;
}

void nuRenderGL::PreRender( int fbwidth, int fbheight )
{
	Check();

	NUTRACE_RENDER( "PreRender %d %d\n", fbwidth, fbheight );
	Check();

	FBWidth = fbwidth;
	FBHeight = fbheight;
	glViewport( 0, 0, fbwidth, fbheight );

	//glMatrixMode( GL_PROJECTION );
	//glLoadIdentity();
	//glOrtho( 0, fbwidth, fbheight, 0, 0, 1 );

	//glMatrixMode( GL_MODELVIEW );
	//glLoadIdentity();

	// Make our clear color a very noticeable purple, so you know when you've screwed up the root node
	glClearColor( 0.7f, 0.0, 0.7f, 0 );

	//glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	if ( nuGlobal()->EnableSRGBFramebuffer && Have_sRGB_Framebuffer )
		glEnable( GL_FRAMEBUFFER_SRGB );

	NUTRACE_RENDER( "PreRender 2\n" );
	Check();

	// Enable CULL_FACE because it will make sure that we are consistent about vertex orientation
	glEnable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	NUTRACE_RENDER( "PreRender 3\n" );
	Check();

	nuMat4f mvproj;
	mvproj.Identity();
	Ortho( mvproj, 0, fbwidth, fbheight, 0, 0, 1 );
	// GLES doesn't support TRANSPOSE = TRUE
	mvproj = mvproj.Transposed();

	ActivateProgram( PRect );
	glUniform2f( PRect.v_vport_hsize, FBWidth / 2.0f, FBHeight / 2.0f );
	glUniformMatrix4fv( PRect.v_mvproj, 1, false, &mvproj.row[0].x );
	Check();

	ActivateProgram( PFill );

	NUTRACE_RENDER( "PreRender 4 (%d)\n", PFill.v_mvproj );
	Check();

	glUniformMatrix4fv( PFill.v_mvproj, 1, false, &mvproj.row[0].x );

	NUTRACE_RENDER( "PreRender 5 (%d)\n", PFillTex.Prog );
	Check();

	ActivateProgram( PFillTex );

	NUTRACE_RENDER( "PreRender 6 (%d)\n", PFillTex.v_mvproj );
	Check();

	glUniformMatrix4fv( PFillTex.v_mvproj, 1, false, &mvproj.row[0].x );

	if ( PTextRGB.UseOnThisPlatform() )
	{
		NUTRACE_RENDER( "PreRender 7a (%d)\n", PTextRGB.Prog );

		ActivateProgram( PTextRGB );

		NUTRACE_RENDER( "PreRender 7b (%d)\n", PTextRGB.v_mvproj );
		Check();

		glUniformMatrix4fv( PTextRGB.v_mvproj, 1, false, &mvproj.row[0].x );
	}

	ActivateProgram( PTextWhole );

	NUTRACE_RENDER( "PreRender 8 (%d)\n", PTextWhole.v_mvproj );
	Check();

	glUniformMatrix4fv( PTextWhole.v_mvproj, 1, false, &mvproj.row[0].x );

	NUTRACE_RENDER( "PreRender done\n" );
}

void nuRenderGL::PostRenderCleanup()
{
	glUseProgram( 0 );
	ActiveProgram = NULL;
}

void nuRenderGL::DrawQuad( const void* v )
{
	NUTRACE_RENDER( "DrawQuad\n" );

	int stride = sizeof(nuVx_PTC);
	const byte* vbyte = (const byte*) v;

	GLint varvpos = 0;
	GLint varvcol = 0;
	GLint varvtex0 = 0;
	GLint varvtexClamp = 0;
	GLint vartexUnit0 = 0;
	if ( ActiveProgram == &PRect )
	{
		varvpos = PRect.v_vpos;
		varvcol = PRect.v_vcolor;
	}
	else if ( ActiveProgram == &PFill )
	{
		varvpos = PFill.v_vpos;
		varvcol = PFill.v_vcolor;
	}
	else if ( ActiveProgram == &PFillTex )
	{
		varvpos = PFillTex.v_vpos;
		varvcol = PFillTex.v_vcolor;
		varvtex0 = PFillTex.v_vtexuv0;
		vartexUnit0 = PFillTex.v_tex0;
	}
	else if ( ActiveProgram == &PTextRGB )
	{
		stride = sizeof(nuVx_PTCV4);
		varvpos = PTextRGB.v_vpos;
		varvcol = PTextRGB.v_vcolor;
		varvtex0 = PTextRGB.v_vtexuv0;
		varvtexClamp = PTextRGB.v_vtexClamp;
		vartexUnit0 = PTextRGB.v_tex0;
	}
	else if ( ActiveProgram == &PTextWhole )
	{
		varvpos = PTextWhole.v_vpos;
		varvcol = PTextWhole.v_vcolor;
		varvtex0 = PTextWhole.v_vtexuv0;
		vartexUnit0 = PTextWhole.v_tex0;
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

	uint16 indices[6];
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 0;
	indices[4] = 2;
	indices[5] = 3;
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

	NUTRACE_RENDER( "DrawQuad done\n" );

	Check();
}

void nuRenderGL::DrawTriangles( int nvert, const void* v, const uint16* indices )
{
	int stride = sizeof(nuVx_PTC);
	const byte* vbyte = (const byte*) v;
	glDrawElements( GL_TRIANGLES, nvert, GL_UNSIGNED_SHORT, indices );
	Check();
}

void nuRenderGL::LoadTexture( nuTexture* tex, int texUnit )
{
	NUASSERT( tex->TexWidth != 0 && tex->TexHeight != 0 );
	NUASSERT( tex->TexChannelCount >= 1 && tex->TexChannelCount <= 4 );
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
	if ( tex->TexChannelCount == 1 ) format = GL_LUMINANCE;
	else if ( tex->TexChannelCount == 2 ) format = GL_RG;
	else if ( tex->TexChannelCount == 3 ) format = GL_RGB;
	else if ( tex->TexChannelCount == 4 ) format = GL_RGBA;
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

/*
void nuRenderGL::LoadTexture( const nuImage* img )
{
	if ( SingleTex2D == 0 )
		glGenTextures( 1, &SingleTex2D );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, SingleTex2D );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, img->GetWidth(), img->GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img->GetData() );
	glGenerateMipmap( GL_TEXTURE_2D );
}

void nuRenderGL::LoadTextureAtlas( const nuTextureAtlas* atlas )
{
	if ( atlas == BoundToTex0 )
		return;

	if ( SingleTexAtlas2D == 0 )
		glGenTextures( 1, &SingleTexAtlas2D );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, SingleTexAtlas2D );

#if NU_PLATFORM_WIN_DESKTOP
	//int internalFormat = atlas->GetBytesPerTexel() == 1 ? GL_SLUMINANCE8 : GL_RGB;
	int internalFormat = atlas->GetBytesPerTexel() == 1 ? GL_LUMINANCE : GL_RGB;
	int format = atlas->GetBytesPerTexel() == 1 ? GL_LUMINANCE : GL_RGB;
#else
	int internalFormat = atlas->GetBytesPerTexel() == 1 ? GL_LUMINANCE : GL_RGB;
	int format = atlas->GetBytesPerTexel() == 1 ? GL_LUMINANCE : GL_RGB;
#endif
	glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, atlas->GetWidth(), atlas->GetHeight(), 0, format, GL_UNSIGNED_BYTE, atlas->DataAt(0,0) );
	// all assuming this is for a glyph atlas
	//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	// Clamping should have no effect for RGB text, since we clamp inside our fragment shader.
	// Also, when rendering 'whole pixel' glyphs, we shouldn't need clamping either, because
	// our UV coordinates are exact, and we always have a 1:1 texel:pixel ratio.
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	// not necessary for text where we sample NN
	//glGenerateMipmap( GL_TEXTURE_2D );
}
*/

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

	if ( !LoadShader( GL_VERTEX_SHADER, vshade, name, vsrc ) ) return false;
	if ( !LoadShader( GL_FRAGMENT_SHADER, fshade, name, fsrc ) ) return false;

	prog = glCreateProgram();

	glAttachShader( prog, vshade );
	glAttachShader( prog, fshade );
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
	if ( linkStat == 0 ) 
		return false;

	return glGetError() == GL_NO_ERROR;
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

