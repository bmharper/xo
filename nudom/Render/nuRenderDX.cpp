#include "pch.h"
#include "nuRenderDX.h"
#include "../Image/nuImage.h"
#include "../nuSysWnd.h"

#if NU_BUILD_DIRECTX

#define CHECK_HR(hresult, msg) if (FAILED(hresult)) { NUTRACE("%s failed: 0x%08x\n", msg, hresult); return false; }

nuRenderDX::nuRenderDX()
{
	memset( &D3D, 0, sizeof(D3D) );
	FBWidth = FBHeight = 0;
	AllProgs[0] = &PFill;
	AllProgs[1] = &PRect;
	AllProgs[2] = &PTextRGB;
	AllProgs[3] = &PTextWhole;
	static_assert(NumProgs == 4, "Add new shader here");
}

nuRenderDX::~nuRenderDX()
{
}

const char*	nuRenderDX::RendererName()
{
	return "DirectX 11";
}

bool nuRenderDX::InitializeDevice( nuSysWnd& wnd )
{
	if ( !InitializeDXDevice(wnd) )
		return false;

	if ( !InitializeDXSurface(wnd) )
		return false;

	return true;
}

bool nuRenderDX::InitializeDXDevice( nuSysWnd& wnd )
{
	DXGI_SWAP_CHAIN_DESC swap;
	memset( &swap, 0, sizeof(swap) );
	swap.BufferDesc.Width = 0;
	swap.BufferDesc.Height = 0;
	swap.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	swap.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
	swap.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
	if ( nuGlobal()->EnableVSync )
	{
		// This has no effect on windowed rendering. Also, it is dumb to assume 60 hz.
		swap.BufferDesc.RefreshRate.Numerator = 1;
		swap.BufferDesc.RefreshRate.Denominator = 60;
	}
	swap.SampleDesc.Count = 1;
	swap.SampleDesc.Quality = 0;
	swap.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap.BufferCount = 1;
	swap.OutputWindow = wnd.SysWnd;
	swap.Windowed = true;
	swap.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap.Flags = 0;

	HRESULT eCreate = D3D11CreateDeviceAndSwapChain( 
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_SINGLETHREADED,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&swap,
		&D3D.SwapChain,
		&D3D.Device,
		&D3D.FeatureLevel,
		&D3D.Context
		);

	CHECK_HR(eCreate, "D3D11CreateDeviceAndSwapChain")

	D3D11_RASTERIZER_DESC rast;
	memset( &rast, 0, sizeof(rast) );
	rast.FillMode = D3D11_FILL_SOLID;
	rast.CullMode = D3D11_CULL_NONE;
	rast.FrontCounterClockwise = TRUE; // The default DirectX value is FALSE, but we stick to the default OpenGL value which is CCW = Front
	rast.DepthBias = 0;
	rast.DepthBiasClamp = 0.0f;
	rast.SlopeScaledDepthBias = 0.0f;
	rast.DepthClipEnable = FALSE;
	rast.ScissorEnable = FALSE;
	rast.MultisampleEnable = FALSE;
	rast.AntialiasedLineEnable = FALSE;

	HRESULT eRast = D3D.Device->CreateRasterizerState( &rast, &D3D.Rasterizer );
	CHECK_HR(eRast, "CreateRasterizerState");

	D3D11_BLEND_DESC blend;
	memset( &blend, 0, sizeof(blend) );
	blend.AlphaToCoverageEnable = FALSE;
	blend.IndependentBlendEnable = FALSE;
	blend.RenderTarget[0].BlendEnable    = true;
	blend.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
	blend.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
	blend.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_SRC_ALPHA;
	blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blend.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HRESULT eBlendNormal = D3D.Device->CreateBlendState( &blend, &D3D.BlendNormal );
	CHECK_HR(eBlendNormal, "CreateBlendNormal");

	memset( &blend, 0, sizeof(blend) );
	blend.AlphaToCoverageEnable = FALSE;
	blend.IndependentBlendEnable = FALSE;
	blend.RenderTarget[0].BlendEnable    = true;
	blend.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC1_COLOR;
	blend.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC1_COLOR;
	blend.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
	blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blend.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HRESULT eBlendDual = D3D.Device->CreateBlendState( &blend, &D3D.BlendDual );
	CHECK_HR(eBlendDual, "CreateBlendDual");

	D3D11_SAMPLER_DESC sampler;
	memset( &sampler, 0, sizeof(sampler) );
	sampler.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.MipLODBias = 0.0f;
	sampler.MaxAnisotropy = 1;
	sampler.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler.BorderColor[0] = 0;
	sampler.BorderColor[1] = 0;
	sampler.BorderColor[2] = 0;
	sampler.BorderColor[3] = 0;
	sampler.MinLOD = 0;
	sampler.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT eSampler = D3D.Device->CreateSamplerState( &sampler, &D3D.SamplerLinear );
	CHECK_HR(eSampler, "CreateSamplerLinear");

	return true;
}

bool nuRenderDX::InitializeDXSurface( nuSysWnd& wnd )
{
	HRESULT hr = S_OK;

	auto rect = wnd.GetRelativeClientRect();
	FBWidth = rect.Width();
	FBHeight = rect.Height();
	if ( !WindowResized() )
		return false;

	if ( !CreateShaders() )
		return false;

	if ( !SetupBuffers() )
		return false;

	return true;
}

bool nuRenderDX::WindowResized()
{
	HRESULT hr = S_OK;

	if ( D3D.RenderTargetView != NULL )
	{
		D3D.Context->OMSetRenderTargets( 0, NULL, NULL );
		D3D.RenderTargetView->Release();
		D3D.RenderTargetView = NULL;
		hr = D3D.SwapChain->ResizeBuffers( 0, FBWidth, FBHeight, DXGI_FORMAT_UNKNOWN, 0 );
		CHECK_HR(hr, "SwapChain.ResizeBuffers");
	}

	// Get a buffer and create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = D3D.SwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (void**) &pBackBuffer );
	CHECK_HR(hr, "SwapChain.GetBuffer");

	hr = D3D.Device->CreateRenderTargetView( pBackBuffer, NULL, &D3D.RenderTargetView );
	pBackBuffer->Release();
	CHECK_HR(hr, "Device.CreateRenderTargetView");

	D3D.Context->OMSetRenderTargets( 1, &D3D.RenderTargetView, NULL );

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT) FBWidth;
	vp.Height = (FLOAT) FBHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	D3D.Context->RSSetViewports( 1, &vp );

	return true;
}

bool nuRenderDX::CreateShaders()
{
	for ( int i = 0; i < NumProgs; i++ )
	{
		if ( !CreateShader(AllProgs[i]) )
			return false;
	}
	return true;
}

bool nuRenderDX::CreateShader( nuDXProg* prog )
{
	std::string name = prog->Name();

	std::string vert_src = CommonShaderDefines() + prog->VertSrc();
	std::string frag_src = CommonShaderDefines() + prog->FragSrc();

	// Vertex shader
	ID3DBlob* vsBlob = NULL;
	if ( !CompileShader( (name + "Vertex").c_str(), vert_src.c_str(), "vs_4_0", &vsBlob ) )
		return false;

	HRESULT hr = D3D.Device->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &prog->Vert );
	if ( FAILED(hr) )
	{	
		NUTRACE( "CreateVertexShader failed (0x%08x) for %s", hr, (const char*) prog->Name() );
		vsBlob->Release();
		return false;
	}

	bool layoutOK = CreateVertexLayout( prog, vsBlob );
	vsBlob->Release();
	if ( !layoutOK )
		return false;

	// Pixel shader
	ID3DBlob* psBlob = NULL;
	if ( !CompileShader( (name + "Frag").c_str(), frag_src.c_str(), "ps_4_0", &psBlob ) )
		return false;

	hr = D3D.Device->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &prog->Frag );
	psBlob->Release();
	if( FAILED(hr) )
	{
		NUTRACE( "CreatePixelShader failed (0x%08x) for %s", hr, (const char*) prog->Name() );
		return false;
	}

	return true;
}

bool nuRenderDX::CreateVertexLayout( nuDXProg* prog, ID3DBlob* vsBlob )
{
	nuVertexType vtype = prog->VertexType();

	static_assert(nuVertexType_END == 3, "Create new vertex layout here");

	D3D11_INPUT_ELEMENT_DESC layouts[nuVertexType_END - 1][4] =
	{
		// nuVertexType_PTC
		{
			{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, offsetof(nuVx_PTC,Pos),		D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,		0, offsetof(nuVx_PTC,UV),		D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",		0, DXGI_FORMAT_R8G8B8A8_UNORM,		0, offsetof(nuVx_PTC,Color),	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		},
		// nuVertexType_PTCV4
		{
			{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, offsetof(nuVx_PTCV4,Pos),	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	1, DXGI_FORMAT_R32G32_FLOAT,		0, offsetof(nuVx_PTCV4,UV),		D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",		0, DXGI_FORMAT_R8G8B8A8_UNORM,		0, offsetof(nuVx_PTCV4,Color),	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	2, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, offsetof(nuVx_PTCV4,V4),		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		},
	};
	int layoutSizes[nuVertexType_END - 1] =
	{
		3,	// PTC
		4,	// PTCV4
	};

	static_assert(nuVertexType_NULL == 0, "nuVertexType_NULL = 0");
	int index = vtype - 1;

	HRESULT hr = D3D.Device->CreateInputLayout( layouts[index], layoutSizes[index], vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &prog->VertLayout );
	if ( FAILED(hr) )
	{
		NUTRACE( "Vertex layout for %s invalid (0x%08x)", (const char*) prog->Name(), hr );
		return false;
	}

	return true;
}

bool nuRenderDX::CompileShader( const char* name, const char* source, const char* shaderTarget, ID3DBlob** blob )
{
	//D3D_SHADER_MACRO macros[1] = {
	//	{NULL,NULL}
	//};
	uint32 flags1 = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
	flags1 |= D3DCOMPILE_DEBUG;

	ID3DBlob* err = NULL;
	HRESULT hr = D3DCompile( source, strlen(source), name, NULL, NULL, "main", shaderTarget, flags1, 0, blob, &err );
	if ( FAILED(hr) )
	{
		if ( err != NULL )
		{
			NUTRACE( "Shader %s compilation failed (0x%08x): %s\n", name, hr, (const char*) err->GetBufferPointer() );
			err->Release();
		}
		else
			NUTRACE( "Shader %s compilation failed (0x%08x)\n", name, hr );
		return false;
	}

	return true;
}

bool nuRenderDX::SetupBuffers()
{
	if ( NULL == (D3D.VertBuffer = CreateBuffer( sizeof(nuVx_PTCV4) * 4, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, NULL )) )
		return false;

	uint16 quadIndices[4] = {0, 1, 3, 2};
	if ( NULL == (D3D.QuadIndexBuffer = CreateBuffer( sizeof(quadIndices), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0, quadIndices )) )
		return false;

	if ( NULL == (D3D.ShaderPerFrameConstants = CreateBuffer( sizeof(nuShaderPerFrame), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, NULL )) )
		return false;

	if ( NULL == (D3D.ShaderPerObjectConstants = CreateBuffer( sizeof(nuShaderPerObject), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, NULL )) )
		return false;

	return true;
}

void nuRenderDX::DestroyDevice( nuSysWnd& wnd )
{
	// free up all buffers etc
}

void nuRenderDX::SurfaceLost()
{
}

bool nuRenderDX::BeginRender( nuSysWnd& wnd )
{
	nuBox rect = wnd.GetRelativeClientRect();
	if ( rect.Width() != FBWidth || rect.Height() != FBHeight )
	{
		FBWidth = rect.Width();
		FBHeight = rect.Height();
		if ( !WindowResized() )
			return false;
	}

	return true;
}

void nuRenderDX::EndRender( nuSysWnd& wnd )
{
	HRESULT hr = D3D.SwapChain->Present( 0, 0 );
	if (!SUCCEEDED(hr))
		NUTRACE( "DirectX Present failed: 0x%08x", hr );

	D3D.ActiveProgram = nullptr;

	// TODO: Handle
	//	DXGI_ERROR_DEVICE_REMOVED
	//	DXGI_ERROR_DRIVER_INTERNAL_ERROR
	// as described here: http://msdn.microsoft.com/en-us/library/windows/apps/dn458383.aspx "Handling device removed scenarios in Direct3D 11"
}

void nuRenderDX::PreRender()
{
	D3D.Context->RSSetState( D3D.Rasterizer );

	// Clear the back buffer 
	auto clear = nuGlobal()->ClearColor;
	float clearColor[4] = {clear.r / 255.0f, clear.g / 255.0f, clear.b / 255.0f, clear.a / 255.0f};
	clearColor[0] = nuSRGB2Linear( clear.r );
	clearColor[1] = nuSRGB2Linear( clear.g );
	clearColor[2] = nuSRGB2Linear( clear.b );
	D3D.Context->ClearRenderTargetView( D3D.RenderTargetView, clearColor );

	SetShaderFrameUniforms();
}

bool nuRenderDX::SetShaderFrameUniforms()
{
	nuMat4f mvproj;
	mvproj.Identity();
	Ortho( mvproj, 0, FBWidth, FBHeight, 0, 0, 1 );
	ShaderPerFrame.MVProj = mvproj.Transposed();
	ShaderPerFrame.VPort_HSize = Vec2f( FBWidth / 2.0f, FBHeight / 2.0f );

	D3D11_MAPPED_SUBRESOURCE sub;
	if ( FAILED(D3D.Context->Map( D3D.ShaderPerFrameConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub )) )
		return false;
	memcpy( sub.pData, &ShaderPerFrame, sizeof(ShaderPerFrame) );
	D3D.Context->Unmap( D3D.ShaderPerFrameConstants, 0 );

	return true;
}

bool nuRenderDX::SetShaderObjectUniforms()
{
	D3D11_MAPPED_SUBRESOURCE sub;
	if ( FAILED(D3D.Context->Map( D3D.ShaderPerObjectConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub )) )
		return false;
	memcpy( sub.pData, &ShaderPerObject, sizeof(ShaderPerObject) );
	D3D.Context->Unmap( D3D.ShaderPerObjectConstants, 0 );
	D3D.Context->VSSetConstantBuffers( ConstantSlotPerObject, 1, &D3D.ShaderPerObjectConstants );
	D3D.Context->PSSetConstantBuffers( ConstantSlotPerObject, 1, &D3D.ShaderPerObjectConstants );
	return true;
}

ID3D11Buffer* nuRenderDX::CreateBuffer( size_t sizeBytes, D3D11_USAGE usage, D3D11_BIND_FLAG bind, uint cpuAccess, const void* initialContent )
{
	ID3D11Buffer* buffer = NULL;
	D3D11_BUFFER_DESC bd;
	memset( &bd, 0, sizeof(bd) );
	bd.ByteWidth = (UINT) sizeBytes;
	bd.Usage = usage;
	bd.BindFlags = bind;
	bd.CPUAccessFlags = cpuAccess;
	D3D11_SUBRESOURCE_DATA sub;
	sub.pSysMem = initialContent;
	sub.SysMemPitch = 0;
	sub.SysMemSlicePitch = 0;
	HRESULT hr = D3D.Device->CreateBuffer( &bd, initialContent != NULL ? &sub : NULL, &buffer );
	if ( !SUCCEEDED(hr) )
		NUTRACE( "CreateBuffer failed: %08x", hr );
	return buffer;
}

bool nuRenderDX::CreateTexture2D( nuTexture* tex )
{
	D3D11_TEXTURE2D_DESC desc;
	memset( &desc, 0, sizeof(desc) );
	desc.Width = tex->TexWidth;
	desc.Height = tex->TexHeight;
	desc.MipLevels = 1;					// 0 = generate all levels. 1 = just one level
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	switch ( tex->TexFormat )
	{
	case nuTexFormatRGBA8:	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
	case nuTexFormatGrey8:	desc.Format = DXGI_FORMAT_R8_UNORM; break;
	default:				NUPANIC("Unrecognized texture format");
	}
	ID3D11Texture2D* dxTex = NULL;
	HRESULT hr = D3D.Device->CreateTexture2D( &desc, NULL, &dxTex );
	if ( !SUCCEEDED(hr) )
	{
		NUTRACE( "CreateTexture2D failed: %08x", hr );
		return false;
	}
	Texture2D* t = new Texture2D();
	t->Tex = dxTex;
	hr = D3D.Device->CreateShaderResourceView( dxTex, NULL, &t->View );
	if ( !SUCCEEDED(hr) )
	{
		NUTRACE( "CreateTexture2D.CreateShaderResourceView failed: %08x", hr );
		delete t;
		dxTex->Release();
		return false;
	}
	tex->TexID = RegisterTextureDX( t );
	tex->TexInvalidate();
	return true;
}

void nuRenderDX::UpdateTexture2D( ID3D11Texture2D* dxTex, nuTexture* tex )
{
	nuBox invRect = tex->TexInvalidRect;
	nuBox fullRect = nuBox(0, 0, tex->TexWidth, tex->TexHeight);
	invRect.ClampTo( fullRect );

	if ( invRect.IsAreaZero() )
		return;
	D3D11_BOX box;
	box.left = invRect.Left;
	box.right = invRect.Right;
	box.top = invRect.Top;
	box.bottom = invRect.Bottom;
	box.front = 0;
	box.back = 1;
	D3D.Context->UpdateSubresource( dxTex, 0, &box, tex->TexDataAt(invRect.Left, invRect.Top), tex->TexStride, 0 );
}

void nuRenderDX::PostRenderCleanup()
{
}

nuProgBase* nuRenderDX::GetShader( nuShaders shader )
{
	switch ( shader )
	{
	case nuShaderFill:		return &PFill;
	//case nuShaderFillTex:	return &PFillTex;
	case nuShaderRect:		return &PRect;
	case nuShaderTextRGB:	return &PTextRGB;
	case nuShaderTextWhole:	return &PTextWhole;
	default:
		NUTODO;
		return NULL;
	}
}

void nuRenderDX::ActivateShader( nuShaders shader )
{
	nuDXProg* p = (nuDXProg*) GetShader( shader );
	if ( p == D3D.ActiveProgram )
		return;

	D3D.Context->VSSetShader( p->Vert, NULL, 0 );
	D3D.Context->PSSetShader( p->Frag, NULL, 0 );
	D3D.Context->IASetInputLayout( p->VertLayout );
	D3D.Context->VSSetConstantBuffers( ConstantSlotPerFrame, 1, &D3D.ShaderPerFrameConstants );
	D3D.Context->PSSetConstantBuffers( ConstantSlotPerFrame, 1, &D3D.ShaderPerFrameConstants );
	D3D.ActiveProgram = p;

	float blendFactors[4] = {0,0,0,0};
	uint sampleMask = 0xffffffff;

	if ( shader == nuShaderTextRGB )
		D3D.Context->OMSetBlendState( D3D.BlendDual, blendFactors, sampleMask );
	else
		D3D.Context->OMSetBlendState( D3D.BlendNormal, blendFactors, sampleMask );

	D3D.Context->PSSetSamplers( 0, 1, &D3D.SamplerLinear );
}

void nuRenderDX::DrawQuad( const void* v )
{
	SetShaderObjectUniforms();

	const int nvert = 4;
	const byte* vbyte = (const byte*) v;

	// map vertex buffer with DISCARD
	D3D11_MAPPED_SUBRESOURCE map;
	memset( &map, 0, sizeof(map) );
	HRESULT hr = D3D.Context->Map( D3D.VertBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map );
	if ( FAILED(hr) )
	{
		NUTRACE_RENDER( "Vertex buffer map failed: %d", hr);
		return;
	}

	//int vertexSize = D3D.ActiveProgramInfo->VertexSize;
	int vertexSize = (int) nuVertexSize( D3D.ActiveProgram->VertexType() );

	memcpy( map.pData, v, nvert * vertexSize );
	D3D.Context->Unmap( D3D.VertBuffer, 0 );

	UINT stride = vertexSize;
	UINT offset = 0;
	D3D.Context->IASetVertexBuffers( 0, 1, &D3D.VertBuffer, &stride, &offset );
	D3D.Context->IASetIndexBuffer( D3D.QuadIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
	//D3D.Context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	D3D.Context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
	D3D.Context->DrawIndexed( 4, 0, 0 );

	//D3D.Context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	//D3D.Context->Draw( 3, 0 );
}

bool nuRenderDX::LoadTexture( nuTexture* tex, int texUnit )
{
	EnsureTextureProperlyDefined( tex, texUnit );

	if ( !IsTextureValid( tex->TexID ) )
	{
		if ( !CreateTexture2D( tex ) )
			return false;
	}
	Texture2D* mytex = GetTextureDX( tex->TexID );

	UpdateTexture2D( mytex->Tex, tex );

	D3D.Context->PSSetShaderResources( 0, 1, &mytex->View );

	return true;
}

bool nuRenderDX::ReadBackbuffer( nuImage& image )
{
	D3D11_TEXTURE2D_DESC desc;
	memset( &desc, 0, sizeof(desc) );
	desc.Width = FBWidth;
	desc.Height = FBHeight;
	desc.MipLevels = 1;					// 0 = generate all levels. 1 = just one level
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ID3D11Texture2D* tempTex = NULL;
	HRESULT hr = D3D.Device->CreateTexture2D( &desc, NULL, &tempTex );
	if ( !SUCCEEDED(hr) )
	{
		NUTRACE( "CreateTexture2D for ReadBackBuffer failed: %08x", hr );
		return false;
	}

	ID3D11Texture2D* backBuffer = NULL;
	hr = D3D.SwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (void**) &backBuffer );
	bool ok = false;
	if ( SUCCEEDED(hr) )
	{
		D3D11_BOX srcBox = { 0, 0, 0, FBWidth, FBHeight, 1 };
		D3D.Context->CopySubresourceRegion( tempTex, 0, 0, 0, 0, backBuffer, 0, &srcBox );
		backBuffer->Release();

		D3D11_MAPPED_SUBRESOURCE map;
		if ( SUCCEEDED(D3D.Context->Map( tempTex, 0, D3D11_MAP_READ, 0, &map )) )
		{
			image.Alloc( nuTexFormatRGBA8, FBWidth, FBHeight );
			for ( int i = 0; i < FBHeight; i++ )
				memcpy( image.TexDataAtLine(i), (char*) map.pData + map.RowPitch * (uint) i, image.TexStride );
			D3D.Context->Unmap( tempTex, 0 );
			ok = true;
		}
	}

	tempTex->Release();

	return ok;
}


#endif
