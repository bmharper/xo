#pragma once

#include "../nuDefs.h"

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
					nuRenderBase();

	void			SurfaceLost_ForgetTextures();
	bool			IsTextureValid( nuTextureID texID ) const;
	nuTextureID		FirstTextureID() const						{ return TexIDOffset + TEX_OFFSET_ONE; }

	// Register a new texture. There no "unregister".
	nuTextureID		RegisterTexture( void* deviceTexID );
	nuTextureID		RegisterTextureInt( uint deviceTexID )				{ return RegisterTexture( reinterpret_cast<void*>(deviceTexID) );  }
	void*			GetTextureDeviceID( nuTextureID texID ) const;
	uint			GetTextureDeviceIDInt( nuTextureID texID ) const	{ return reinterpret_cast<uint>(GetTextureDeviceID( texID )); }

	virtual void	LoadTexture( nuTexture* tex, int texUnit ) = 0;
	virtual void	ReadBackbuffer( nuImage& image ) = 0;

protected:
	static const nuTextureID	TEX_OFFSET_ONE = 1;	// This constant causes the nuTextureID that we expose to never be zero.
	nuTextureID					TexIDOffset;
	podvec<void*>				TexIDToNative;		// Maps from nuTextureID to native device texture (eg. GLuint or ID3D11Texture2D*). We're wasting 4 bytes here on OpenGL.

	
};
