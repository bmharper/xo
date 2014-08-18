#pragma once

#include "xoRenderBase.h"
#include "xoRenderDX_Defs.h"

#if XO_BUILD_DIRECTX

#include "../Shaders/Processed_hlsl/FillShader.h"
#include "../Shaders/Processed_hlsl/RectShader.h"
#include "../Shaders/Processed_hlsl/TextWholeShader.h"
#include "../Shaders/Processed_hlsl/TextRGBShader.h"

class XOAPI xoRenderDX : public xoRenderBase
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
		ID3D11BlendState*		BlendDual;
		ID3D11SamplerState*		SamplerLinear;
		ID3D11SamplerState*		SamplerPoint;
		ID3D11Buffer*           VertBuffer;
		ID3D11Buffer*           QuadIndexBuffer;
		ID3D11Buffer*           ShaderPerFrameConstants;
		ID3D11Buffer*           ShaderPerObjectConstants;

		D3D_FEATURE_LEVEL		FeatureLevel;

		// our own state that
		xoDXProg*				ActiveProgram;
	};

public:
						xoRenderDX();
	virtual				~xoRenderDX();

	virtual const char*	RendererName();

	virtual bool		InitializeDevice( xoSysWnd& wnd );
	virtual void		DestroyDevice( xoSysWnd& wnd );
	virtual void		SurfaceLost();

	virtual bool		BeginRender( xoSysWnd& wnd );
	virtual void		EndRender( xoSysWnd& wnd );

	virtual void		PreRender();
	virtual void		PostRenderCleanup();
	
	virtual xoProgBase* GetShader( xoShaders shader );
	virtual void		ActivateShader( xoShaders shader );

	virtual void		DrawQuad( const void* v );

	virtual bool		LoadTexture( xoTexture* tex, int texUnit );
	virtual bool		ReadBackbuffer( xoImage& image );

private:
	struct Texture2D
	{
		ID3D11Texture2D*			Tex;
		ID3D11ShaderResourceView*	View;
	};

	D3DState				D3D;
	int						FBWidth, FBHeight;

	xoDXProg_Fill			PFill;
	xoDXProg_Rect			PRect;
	xoDXProg_TextRGB		PTextRGB;
	xoDXProg_TextWhole		PTextWhole;
	static const int		NumProgs = 4;
	xoDXProg*				AllProgs[NumProgs];

	bool				InitializeDXDevice( xoSysWnd& wnd );
	bool				InitializeDXSurface( xoSysWnd& wnd );
	bool				WindowResized();
	bool				CreateShaders();
	bool				CreateShader( xoDXProg* prog );
	bool				CreateVertexLayout( xoDXProg* prog, ID3DBlob* vsBlob );
	bool				CompileShader( const char* name, const char* source, const char* shaderTarget, ID3DBlob** blob );
	bool				SetupBuffers();
	bool				SetShaderFrameUniforms();
	bool				SetShaderObjectUniforms();
	ID3D11Buffer*		CreateBuffer( size_t sizeBytes, D3D11_USAGE usage, D3D11_BIND_FLAG bind, uint cpuAccess, const void* initialContent );
	bool				CreateTexture2D( xoTexture* tex );
	void				UpdateTexture2D( ID3D11Texture2D* dxTex, xoTexture* tex );
	
	xoTextureID			RegisterTextureDX( Texture2D* tex )			{ return RegisterTexture(tex); }
	Texture2D*			GetTextureDX( xoTextureID texID ) const		{ return (Texture2D*) GetTextureDeviceHandle(texID); }

};

#else

class XOAPI xoRenderDX : public xoRenderDummy
{
};

#endif