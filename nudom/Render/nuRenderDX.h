#pragma once

#include "nuRenderBase.h"
#include "nuRenderDX_Defs.h"

#if NU_BUILD_DIRECTX

#include "../Shaders/Processed_hlsl/FillShader.h"
#include "../Shaders/Processed_hlsl/RectShader.h"
#include "../Shaders/Processed_hlsl/TextWholeShader.h"

class NUAPI nuRenderDX : public nuRenderBase
{
private:
	static const int ConstantSlotPerFrame = 0;
	static const int ConstantSlotPerObject = 1;

	struct D3DState
	{
		ID3D11Device*			Device;
		ID3D11DeviceContext*	Context;
		IDXGISwapChain*			SwapChain;

		ID3D11RasterizerState*	Rasterizer;
		ID3D11RenderTargetView*	RenderTargetView;
		ID3D11BlendState*		BlendNormal;
		ID3D11SamplerState*		SamplerLinear;
		ID3D11Buffer*           VertBuffer;
		ID3D11Buffer*           QuadIndexBuffer;
		ID3D11Buffer*           ShaderPerFrameConstants;
		ID3D11Buffer*           ShaderPerObjectConstants;

		D3D_FEATURE_LEVEL		FeatureLevel;

		nuDXProg*				ActiveProgram;
	};

public:
						nuRenderDX();
	virtual				~nuRenderDX();

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

private:
	struct Texture2D
	{
		ID3D11Texture2D*			Tex;
		ID3D11ShaderResourceView*	View;
	};

	D3DState				D3D;
	int						FBWidth, FBHeight;

	nuDXProg_Fill			PFill;
	nuDXProg_Rect			PRect;
	nuDXProg_TextWhole		PTextWhole;
	static const int		NumProgs = 3;
	nuDXProg*				AllProgs[NumProgs];

	bool				InitializeDXDevice( nuSysWnd& wnd );
	bool				InitializeDXSurface( nuSysWnd& wnd );
	bool				WindowResized();
	bool				CreateShaders();
	bool				CreateShader( nuDXProg* prog );
	bool				CompileShader( const char* name, const char* source, const char* shaderTarget, ID3DBlob** blob );
	bool				SetupBuffers();
	bool				SetShaderFrameUniforms();
	bool				SetShaderObjectUniforms();
	ID3D11Buffer*		CreateBuffer( size_t sizeBytes, D3D11_USAGE usage, D3D11_BIND_FLAG bind, uint cpuAccess, const void* initialContent );
	bool				CreateTexture2D( nuTexture* tex );
	void				UpdateTexture2D( ID3D11Texture2D* dxTex, nuTexture* tex );
	
	nuTextureID			RegisterTextureDX( Texture2D* tex )			{ return RegisterTexture(tex); }
	Texture2D*			GetTextureDX( nuTextureID texID ) const		{ return (Texture2D*) GetTextureDeviceHandle(texID); }

};

#else

class NUAPI nuRenderDX : public nuRenderDummy
{
};

#endif