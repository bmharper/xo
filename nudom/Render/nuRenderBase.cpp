#include "pch.h"
#include "nuRenderBase.h"
#include "../Text/nuGlyphCache.h"

nuRenderBase::nuRenderBase()
{
	TexIDOffset = 0;
}

nuRenderBase::~nuRenderBase()
{
}

void nuRenderBase::Ortho( nuMat4f &imat, double left, double right, double bottom, double top, double znear, double zfar )
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

void nuRenderBase::SurfaceLost_ForgetTextures()
{
	TexIDOffset++;
	if ( TexIDOffset >= nuGlobal()->MaxTextureID )
		TexIDOffset = 0;
	TexIDToNative.clear();
}

bool nuRenderBase::IsTextureValid( nuTextureID texID ) const
{
	nuTextureID relativeID = texID - TEX_OFFSET_ONE - TexIDOffset;
	return relativeID < (nuTextureID) TexIDToNative.size();
}

nuTextureID nuRenderBase::RegisterTexture( void* deviceTexID )
{
	nuTextureID maxTexID = nuGlobal()->MaxTextureID;
	nuTextureID id = TexIDOffset + (nuTextureID) TexIDToNative.size();
	if ( id > maxTexID )
		id -= maxTexID;

	TexIDToNative += deviceTexID;
	return id + TEX_OFFSET_ONE;
}

void* nuRenderBase::GetTextureDeviceHandle( nuTextureID texID ) const
{
	nuTextureID absolute = texID - TEX_OFFSET_ONE - TexIDOffset;
	if ( absolute >= (nuTextureID) TexIDToNative.size() )
	{
		NUPANIC( "nuRenderBase::GetTextureDeviceHandle: Invalid texture ID. Use IsTextureValid() to check if a texture is valid." );
		return NULL;
	}
	return TexIDToNative[absolute];
}

void nuRenderBase::EnsureTextureProperlyDefined( nuTexture* tex, int texUnit )
{
	NUASSERT( tex->TexWidth != 0 && tex->TexHeight != 0 );
	NUASSERT( tex->TexFormat != nuTexFormatInvalid );
	NUASSERT( texUnit < nuMaxTextureUnits );
}

std::string nuRenderBase::CommonShaderDefines()
{
	std::string s;
	s.append( fmt( "#define NU_GLYPH_ATLAS_SIZE %v\n", nuGlyphAtlasSize ).Z );
	return s;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool nuRenderDummy::InitializeDevice( nuSysWnd& wnd )
{
	return false;
}
void nuRenderDummy::DestroyDevice( nuSysWnd& wnd )
{
}
void nuRenderDummy::SurfaceLost()
{
}
	 
bool nuRenderDummy::BeginRender( nuSysWnd& wnd )
{
	return false;
}
void nuRenderDummy::EndRender( nuSysWnd& wnd )
{
}
	 
void nuRenderDummy::PreRender()
{
}
void nuRenderDummy::PostRenderCleanup()
{
}

nuProgBase* nuRenderDummy::GetShader( nuShaders shader )
{
	return NULL;
}
void nuRenderDummy::ActivateShader( nuShaders shader )
{
}

void nuRenderDummy::DrawQuad( const void* v )
{
}

bool nuRenderDummy::LoadTexture( nuTexture* tex, int texUnit )
{
	return true;
}
void nuRenderDummy::ReadBackbuffer( nuImage& image )
{
}
