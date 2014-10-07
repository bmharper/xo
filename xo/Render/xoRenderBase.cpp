#include "pch.h"
#include "xoRenderBase.h"
#include "../Text/xoGlyphCache.h"

xoRenderBase::xoRenderBase()
{
	TexIDOffset = 0;
}

xoRenderBase::~xoRenderBase()
{
}

void xoRenderBase::Ortho( xoMat4f &imat, double left, double right, double bottom, double top, double znear, double zfar )
{
	xoMat4f m;
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

void xoRenderBase::SurfaceLost_ForgetTextures()
{
	TexIDOffset++;
	if ( TexIDOffset >= xoGlobal()->MaxTextureID )
		TexIDOffset = 0;
	TexIDToNative.clear();
}

bool xoRenderBase::IsTextureValid( xoTextureID texID ) const
{
	xoTextureID relativeID = texID - TEX_OFFSET_ONE - TexIDOffset;
	return relativeID < (xoTextureID) TexIDToNative.size();
}

xoTextureID xoRenderBase::RegisterTexture( void* deviceTexID )
{
	xoTextureID maxTexID = xoGlobal()->MaxTextureID;
	xoTextureID id = TexIDOffset + (xoTextureID) TexIDToNative.size();
	if ( id > maxTexID )
		id -= maxTexID;

	TexIDToNative += deviceTexID;
	return id + TEX_OFFSET_ONE;
}

void* xoRenderBase::GetTextureDeviceHandle( xoTextureID texID ) const
{
	xoTextureID absolute = texID - TEX_OFFSET_ONE - TexIDOffset;
	if ( absolute >= (xoTextureID) TexIDToNative.size() )
	{
		XOPANIC( "xoRenderBase::GetTextureDeviceHandle: Invalid texture ID. Use IsTextureValid() to check if a texture is valid." );
		return NULL;
	}
	return TexIDToNative[absolute];
}

void xoRenderBase::EnsureTextureProperlyDefined( xoTexture* tex, int texUnit )
{
	XOASSERT( tex->TexWidth != 0 && tex->TexHeight != 0 );
	XOASSERT( tex->TexFormat != xoTexFormatInvalid );
	XOASSERT( texUnit < xoMaxTextureUnits );
}

std::string xoRenderBase::CommonShaderDefines()
{
	std::string s;
	s.append( fmt( "#define XO_GLYPH_ATLAS_SIZE %v\n", xoGlyphAtlasSize ).Z );
	return s;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool xoRenderDummy::InitializeDevice( xoSysWnd& wnd )
{
	return false;
}
void xoRenderDummy::DestroyDevice( xoSysWnd& wnd )
{
}
void xoRenderDummy::SurfaceLost()
{
}
	 
bool xoRenderDummy::BeginRender( xoSysWnd& wnd )
{
	return false;
}
void xoRenderDummy::EndRender( xoSysWnd& wnd, uint endRenderFlags )
{
}
	 
void xoRenderDummy::PreRender()
{
}
void xoRenderDummy::PostRenderCleanup()
{
}

xoProgBase* xoRenderDummy::GetShader( xoShaders shader )
{
	return NULL;
}
void xoRenderDummy::ActivateShader( xoShaders shader )
{
}

void xoRenderDummy::DrawQuad( const void* v )
{
}

bool xoRenderDummy::LoadTexture( xoTexture* tex, int texUnit )
{
	return true;
}
bool xoRenderDummy::ReadBackbuffer( xoImage& image )
{
	return false;
}
