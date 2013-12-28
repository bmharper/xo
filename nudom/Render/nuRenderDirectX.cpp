#include "pch.h"
#include "nuRenderDirectX.h"
#include "../nuSysWnd.h"

#if NU_BUILD_DIRECTX

#define CHECK_HR(hresult, msg) if (FAILED(hresult)) { NUTRACE("%s failed: %d\n", msg, hresult); return false; }

bool nuRenderDirectX::InitializeDevice( nuSysWnd& wnd )
{
	if ( !InitializeDXDevice(wnd) )
		return false;

	if ( !InitializeDXSurface(wnd) )
		return false;

	return true;
}

bool nuRenderDirectX::InitializeDXDevice( nuSysWnd& wnd )
{
	DXGI_SWAP_CHAIN_DESC swap;
	memset( &swap, 0, sizeof(swap) );
	swap.BufferDesc.Width = 0;
	swap.BufferDesc.Height = 0;
	swap.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	swap.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
	swap.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
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

	CHECK_HR(eCreate, "D3D11CreateDeviceAndSwapChain");

	return true;
}

bool nuRenderDirectX::InitializeDXSurface( nuSysWnd& wnd )
{
	HRESULT hr = S_OK;

	nuBox crect = wnd.GetRelativeClientRect();
	uint width = crect.Width();
	uint height = crect.Height();

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = D3D.SwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (void**) &pBackBuffer );
	CHECK_HR(hr, "SwapChain.GetBuffer");

	hr = D3D.Device->CreateRenderTargetView( pBackBuffer, NULL, &D3D.RenderTargetView );
	pBackBuffer->Release();
	CHECK_HR(hr, "Device.CreateRenderTargetView");

	D3D.Context->OMSetRenderTargets( 1, &D3D.RenderTargetView, NULL );

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT) width;
	vp.Height = (FLOAT) height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	D3D.Context->RSSetViewports( 1, &vp );

	/*
	// Compile the vertex shader
	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile( L"Tutorial02.fx", "VS", "vs_4_0", &pVSBlob );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
		return hr;
	}

	// Create the vertex shader
	hr = D3D.Device->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &D3D.VertexShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE( layout );

	// Create the input layout
	hr = D3D.Device->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &D3D.VertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
		return hr;

	// Set the input layout
	D3D.Context->IASetInputLayout( D3D.VertexLayout );

	// Compile the pixel shader
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile( L"Tutorial02.fx", "PS", "ps_4_0", &pPSBlob );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
		return hr;
	}

	// Create the pixel shader
	hr = D3D.Device->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &D3D.PixelShader );
	pPSBlob->Release();
	if( FAILED( hr ) )
		return hr;

	// Create vertex buffer
	SimpleVertex vertices[] =
	{
		XMFLOAT3( 0.0f, 0.5f, 0.5f ),
		XMFLOAT3( 0.5f, -0.5f, 0.5f ),
		XMFLOAT3( -0.5f, -0.5f, 0.5f ),
	};
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( SimpleVertex ) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = vertices;
	hr = D3D.Device->CreateBuffer( &bd, &InitData, &D3D.VertexBuffer );
	if( FAILED( hr ) )
		return hr;

	// Set vertex buffer
	UINT stride = sizeof( SimpleVertex );
	UINT offset = 0;
	D3D.Context->IASetVertexBuffers( 0, 1, &D3D.VertexBuffer, &stride, &offset );

	// Set primitive topology
	D3D.Context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	*/
	return true;
}

void nuRenderDirectX::DestroyDevice( nuSysWnd& wnd )
{
}

void nuRenderDirectX::SurfaceLost()
{
}

bool nuRenderDirectX::BeginRender( nuSysWnd& wnd )
{
	return false;
}

void nuRenderDirectX::EndRender( nuSysWnd& wnd )
{
	// TODO: Handle
	//	DXGI_ERROR_DEVICE_REMOVED
	//	DXGI_ERROR_DRIVER_INTERNAL_ERROR
	// as described here: http://msdn.microsoft.com/en-us/library/windows/apps/dn458383.aspx "Handling device removed scenarios in Direct3D 11"
}

void nuRenderDirectX::LoadTexture( nuTexture* tex, int texUnit )
{
}

void nuRenderDirectX::ReadBackbuffer( nuImage& image )
{
}


#endif
