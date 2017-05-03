#pragma once
#include "RenderBase.h"
#include "RenderDX_Defs.h"

#if XO_BUILD_DIRECTX
#include "../Shaders/Processed_hlsl/FillShader.h"
#include "../Shaders/Processed_hlsl/FillTexShader.h"
#include "../Shaders/Processed_hlsl/RectShader.h"
#include "../Shaders/Processed_hlsl/TextWholeShader.h"
#include "../Shaders/Processed_hlsl/TextRGBShader.h"
#include "../Shaders/Processed_hlsl/UberShader.h"
#endif

#if XO_BUILD_DIRECTX
namespace xo {

class XO_API RenderDX : public RenderBase {
private:
	static const int ConstantSlotPerFrame  = 0;
	static const int ConstantSlotPerObject = 1;

	struct D3DState {
		ID3D11Device*        Device;
		ID3D11DeviceContext* Context;
		IDXGISwapChain*      SwapChain;

		ID3D11RasterizerState*  Rasterizer;
		ID3D11RenderTargetView* RenderTargetView;
		ID3D11BlendState*       BlendNormal;
		ID3D11BlendState*       BlendDual;
		ID3D11SamplerState*     SamplerLinear;
		ID3D11SamplerState*     SamplerPoint;
		ID3D11Buffer*           VertBuffer;
		size_t                  VertBufferBytes;
		ID3D11Buffer*           QuadIndexBuffer;
		size_t                  QuadIndexBufferSize;
		ID3D11Buffer*           LinearIndexBuffer;
		size_t                  LinearIndexBufferSize;
		ID3D11Buffer*           ShaderPerFrameConstants;
		ID3D11Buffer*           ShaderPerObjectConstants;

		D3D_FEATURE_LEVEL FeatureLevel;

		// our own state that
		DXProg* ActiveProgram;
	};

public:
	RenderDX();
	virtual ~RenderDX();

	virtual const char* RendererName();

	virtual bool InitializeDevice(SysWnd& wnd);
	virtual void DestroyDevice(SysWnd& wnd);
	virtual void SurfaceLost();

	virtual bool BeginRender(SysWnd& wnd);
	virtual void EndRender(SysWnd& wnd, uint32_t endRenderFlags);

	virtual void PreRender();
	virtual void PostRenderCleanup();

	virtual ProgBase* GetShader(Shaders shader);
	virtual void      ActivateShader(Shaders shader);

	virtual void Draw(GPUPrimitiveTypes type, int nvertex, const void* v);

	virtual bool LoadTexture(Texture* tex, int texUnit);
	virtual bool ReadBackbuffer(Image& image);

private:
	struct Texture2D {
		ID3D11Texture2D*          Tex;
		ID3D11ShaderResourceView* View;
	};

	D3DState D3D;

	DXProg_Fill      PFill;
	DXProg_FillTex   PFillTex;
	DXProg_Rect      PRect;
	DXProg_TextRGB   PTextRGB;
	DXProg_TextWhole PTextWhole;
	DXProg_Uber      PUber;
	static const int NumProgs = 6;
	DXProg*          AllProgs[NumProgs];

	bool          InitializeDXDevice(SysWnd& wnd);
	bool          InitializeDXSurface(SysWnd& wnd);
	bool          WindowResized();
	bool          CreateShaders();
	bool          CreateShader(DXProg* prog);
	bool          CreateVertexLayout(DXProg* prog, ID3DBlob* vsBlob);
	bool          CompileShader(const char* name, const char* source, const char* shaderTarget, ID3DBlob** blob);
	bool          SetupBuffers();
	bool          SetShaderFrameUniforms();
	bool          SetShaderObjectUniforms();
	ID3D11Buffer* CreateBuffer(size_t sizeBytes, D3D11_USAGE usage, D3D11_BIND_FLAG bind, uint32_t cpuAccess, const void* initialContent);
	bool          CreateTexture2D(Texture* tex);
	void          UpdateTexture2D(ID3D11Texture2D* dxTex, Texture* tex);

	TextureID  RegisterTextureDX(Texture2D* tex) { return RegisterTexture((uintptr_t) tex); }
	Texture2D* GetTextureDX(TextureID texID) const { return (Texture2D*) GetTextureDeviceHandle(texID); }

	static int TexFilterToDX(TexFilter f);
};
} // namespace xo
#else

namespace xo {
class XO_API RenderDX : public RenderDummy {
};
} // namespace xo

#endif
