#pragma once

#include "../xoDefs.h"
#include "xoVertexTypes.h"

// This is a mess -- xoShaderPerFrame and xoShaderPerObject are actually DirectX specific.
// We must investigate whether we can make them uniform across GL and DirectX, but that
// would necessitate having the same matrix layout (ie column or row major).
struct xoShaderPerFrame
{
	xoMat4f		MVProj;			// Transposed for DirectX
	xoVec2f		VPort_HSize;	// Half the size of the viewport
	float		Padding[2];
};
static_assert((sizeof(xoShaderPerFrame) & 15) == 0, "xoShaderPerFrame size must be a multiple of 16 bytes (This is a DirectX constant buffer requirement)");

struct xoShaderPerObject
{
	xoVec4f		Box;
	xoVec4f		Border;
	xoVec4f		BorderColor;
	float		Radius;
	xoVec2f		Edges;
	float		Padding[1];
	xoVec4f		ShadowColor;
	xoVec2f		ShadowOffset;
	float		ShadowSizeInv;
	xoVec2f		OutVector;
	float		Padding2[7];
};
static_assert((sizeof(xoShaderPerObject) & 15) == 0, "xoShaderPerFrame size must be a multiple of 16 bytes (This is a DirectX constant buffer requirement)");

enum xoGPUPrimitiveTypes
{
	xoGPUPrimQuads,
	xoGPUPrimTriangles,
};

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
class XOAPI xoRenderBase
{
public:
	friend struct xoRenderBase_OnceOff;

	xoShaderPerFrame	ShaderPerFrame; // This is a mess between DirectX and OpenGL. needs cleanup
	xoShaderPerObject	ShaderPerObject;
	xoMat4f				MVProj;

	xoRenderBase();
	virtual				~xoRenderBase();

	// Setup a matrix equivalent to glOrtho. The matrix 'imat' is multiplied by the ortho matrix.
	static void			Ortho(xoMat4f &imat, double left, double right, double bottom, double top, double znear, double zfar);

	// Project a point 'v' through ShaderPerFrame.MVProj
	void				SetupToScreen(xoMat4f mvproj);
	xoVec2f				ToScreen(xoVec2f v);

	void				SurfaceLost_ForgetTextures();
	bool				IsTextureValid(xoTextureID texID) const;
	xoTextureID			FirstTextureID() const								{ return TexIDOffset + TEX_OFFSET_ONE; }

	// Register a new texture. There is no "unregister".
	xoTextureID			RegisterTexture(uintptr_t deviceTexID);
	xoTextureID			RegisterTextureInt(uint deviceTexID)				{ return RegisterTexture((uintptr_t) deviceTexID);  }
	uintptr_t			GetTextureDeviceHandle(xoTextureID texID) const;
	uint				GetTextureDeviceHandleInt(xoTextureID texID) const	{ return (uint)(uintptr_t)(GetTextureDeviceHandle(texID)); }

	virtual const char*	RendererName() = 0;

	virtual bool		InitializeDevice(xoSysWnd& wnd) = 0;	// Initialize this device
	virtual void		DestroyDevice(xoSysWnd& wnd) = 0;		// Destroy this device and all associated textures, etc
	virtual void		SurfaceLost() = 0;

	virtual bool		BeginRender(xoSysWnd& wnd) = 0;						// Start of a frame
	virtual void		EndRender(xoSysWnd& wnd, uint endRenderFlags) = 0;	// Frame is finished. Present it (or possibly not, depending on flags).

	virtual void		PreRender() = 0;
	virtual void		PostRenderCleanup() = 0;

	virtual xoProgBase* GetShader(xoShaders shader) = 0;
	virtual void		ActivateShader(xoShaders shader) = 0;

	virtual void		Draw(xoGPUPrimitiveTypes type, int nvertex, const void* v) = 0;

	virtual bool		LoadTexture(xoTexture* tex, int texUnit) = 0;
	virtual bool		ReadBackbuffer(xoImage& image) = 0;

protected:
	static const xoTextureID	TEX_OFFSET_ONE = 1;	// This constant causes the xoTextureID that we expose to never be zero.
	xoTextureID					TexIDOffset;
	podvec<uintptr_t>			TexIDToNative;		// Maps from xoTextureID to native device texture (eg. GLuint or ID3D11Texture2D*). We're wasting 4 bytes here on OpenGL.
	int							FBWidth, FBHeight;

	void				EnsureTextureProperlyDefined(xoTexture* tex, int texUnit);
	std::string			CommonShaderDefines();
};

// This reduces the amount of #ifdef-ing needed, so that on non-Windows platforms
// we can still have a xoRenderDX class defined.
class XOAPI xoRenderDummy
{
public:
	virtual bool		InitializeDevice(xoSysWnd& wnd);
	virtual void		DestroyDevice(xoSysWnd& wnd);
	virtual void		SurfaceLost();

	virtual bool		BeginRender(xoSysWnd& wnd);
	virtual void		EndRender(xoSysWnd& wnd, uint endRenderFlags);

	virtual void		PreRender();
	virtual void		PostRenderCleanup();

	virtual xoProgBase* GetShader(xoShaders shader);
	virtual void		ActivateShader(xoShaders shader);

	virtual void		Draw(xoGPUPrimitiveTypes type, int nvertex, const void* v);

	virtual bool		LoadTexture(xoTexture* tex, int texUnit);
	virtual bool		ReadBackbuffer(xoImage& image);
};
