#pragma once

#include "nuRenderBase.h"

#if NU_BUILD_DIRECTX

class NUAPI nuRenderDirectX : public nuRenderBase
{
private:
	struct D3DState
	{
		ID3D11Device*			Device;
		ID3D11DeviceContext*	Context;
		IDXGISwapChain*			SwapChain;

		ID3D11RenderTargetView*	RenderTargetView;
		ID3D11VertexShader*     VertexShader;
		ID3D11PixelShader*      PixelShader;
		ID3D11InputLayout*      VertexLayout;
		ID3D11Buffer*           VertexBuffer;

		D3D_FEATURE_LEVEL		FeatureLevel;
	};

public:
	virtual bool	InitializeDevice( nuSysWnd& wnd );
	virtual void	DestroyDevice( nuSysWnd& wnd );
	virtual void	SurfaceLost();

	virtual bool	BeginRender( nuSysWnd& wnd );
	virtual void	EndRender( nuSysWnd& wnd );

	virtual void	LoadTexture( nuTexture* tex, int texUnit );
	virtual void	ReadBackbuffer( nuImage& image );

private:
	D3DState	D3D;

	bool			InitializeDXDevice( nuSysWnd& wnd );
	bool			InitializeDXSurface( nuSysWnd& wnd );

};

#else

class NUAPI nuRenderDirectX : public nuRenderDummy
{
};

#endif