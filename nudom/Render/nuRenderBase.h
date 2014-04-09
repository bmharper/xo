#pragma once

#include "../nuDefs.h"
#include "nuVertexTypes.h"

struct nuShaderPerFrame
{
	nuMat4f		MVProj;
	nuVec2f		VPort_HSize;
	float		Padding[2];
};
static_assert( (sizeof(nuShaderPerFrame) & 15) == 0, "nuShaderPerFrame size must be a multiple of 16 bytes (This is a DirectX constant buffer requirement)" );

struct nuShaderPerObject
{
	nuVec4f		Box;
	float		Radius;
	float		Padding[3];
};
static_assert( (sizeof(nuShaderPerObject) & 15) == 0, "nuShaderPerFrame size must be a multiple of 16 bytes (This is a DirectX constant buffer requirement)" );

/* Base class of device-specific renderer (such as GL or DX).

Texture IDs
-----------
Texture IDs are unsigned 32-bit integers that start at 1 and go up to 2^32-1.
When we "lose" a graphics surface, then we continue to issue texture IDs in the same
sequence that we previously were issuing them. However, during a graphics device
lost event, we record the first ID that is valid. Any IDs lower than that are not
valid. This is not bullet-proof, because you could have some very very old ID
that is not detected as invalid.

For example, say the very first texture you allocate gets assigned ID 1. Then,
a long time goes on, and very many "lost" surface events occur, and eventually
you wrap back around to the start, and you issue texture ID 1 again to a new
caller. The original caller would now believe his old ID 1 is a real texture.
I am hoping that the probability of this occurring is next to zero. Perhaps
a wise thing to do is to force all resource holders to clean up their garbage
at the start of every frame - that would guarantee that such an unlikely
calamity becomes impossible.

*/
class NUAPI nuRenderBase
{
public:
	friend struct nuRenderBase_OnceOff;

	nuShaderPerFrame		ShaderPerFrame;
	nuShaderPerObject		ShaderPerObject;

						nuRenderBase();
	virtual				~nuRenderBase();

	// Setup a matrix equivalent to glOrtho. The matrix 'imat' is multiplied by the ortho matrix.
	void				Ortho( nuMat4f &imat, double left, double right, double bottom, double top, double znear, double zfar );

	void				SurfaceLost_ForgetTextures();
	bool				IsTextureValid( nuTextureID texID ) const;
	nuTextureID			FirstTextureID() const								{ return TexIDOffset + TEX_OFFSET_ONE; }

	// Register a new texture. There is no "unregister".
	nuTextureID			RegisterTexture( void* deviceTexID );
	nuTextureID			RegisterTextureInt( uint deviceTexID )				{ return RegisterTexture( reinterpret_cast<void*>(deviceTexID) );  }
	void*				GetTextureDeviceHandle( nuTextureID texID ) const;
	uint				GetTextureDeviceHandleInt( nuTextureID texID ) const	{ return reinterpret_cast<uint>(GetTextureDeviceHandle( texID )); }

	virtual const char*	RendererName() = 0;

	virtual bool		InitializeDevice( nuSysWnd& wnd ) = 0;	// Initialize this device
	virtual void		DestroyDevice( nuSysWnd& wnd ) = 0;		// Destroy this device and all associated textures, etc
	virtual void		SurfaceLost() = 0;
	
	virtual bool		BeginRender( nuSysWnd& wnd ) = 0;		// Start of a frame
	virtual void		EndRender( nuSysWnd& wnd ) = 0;			// Frame is finished. Present it.

	virtual void		PreRender() = 0;
	virtual void		PostRenderCleanup() = 0;

	virtual nuProgBase* GetShader( nuShaders shader ) = 0;
	virtual void		ActivateShader( nuShaders shader ) = 0;

	virtual void		DrawQuad( const void* v ) = 0;

	virtual bool		LoadTexture( nuTexture* tex, int texUnit ) = 0;
	virtual void		ReadBackbuffer( nuImage& image ) = 0;

protected:
	static const nuTextureID	TEX_OFFSET_ONE = 1;	// This constant causes the nuTextureID that we expose to never be zero.
	nuTextureID					TexIDOffset;
	podvec<void*>				TexIDToNative;		// Maps from nuTextureID to native device texture (eg. GLuint or ID3D11Texture2D*). We're wasting 4 bytes here on OpenGL.

	void				EnsureTextureProperlyDefined( nuTexture* tex, int texUnit );
	std::string			CommonShaderDefines();
};

// This reduces the amount of #ifdef-ing needed, so that on non-Windows platforms
// we can still have a nuRenderDX class defined.
class NUAPI nuRenderDummy
{
public:
	virtual bool		InitializeDevice( nuSysWnd& wnd );
	virtual void		DestroyDevice( nuSysWnd& wnd );
	virtual void		SurfaceLost();
	
	virtual bool		BeginRender( nuSysWnd& wnd );
	virtual void		EndRender( nuSysWnd& wnd );

	virtual void		PreRender();
	virtual void		PostRenderCleanup();

	virtual nuProgBase* GetShader( nuShaders shader );
	virtual void		ActivateShader( nuShaders shader );

	virtual void		DrawQuad( const void* v );

	virtual bool		LoadTexture( nuTexture* tex, int texUnit );
	virtual void		ReadBackbuffer( nuImage& image );
};
