#define _CRT_SECURE_NO_WARNINGS
#include "GpDisplayDriverD3D11.h"
#include "GpDisplayDriverSurfaceD3D11.h"
#include "GpWindows.h"
#include "GpColorCursor_Win32.h"
#include "GpFiber_Win32.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <float.h>
#include <emmintrin.h>

#include <stdio.h>

#pragma comment (lib, "d3d11.lib")

namespace GpBinarizedShaders
{
	extern const unsigned char *g_drawQuadV_D3D11[2];
	extern const unsigned char *g_drawQuadPaletteP_D3D11[2];
	extern const unsigned char *g_drawQuadRGBP_D3D11[2];
	extern const unsigned char *g_drawQuad15BitP_D3D11[2];
}

struct GpShaderCodeBlob
{
	const void *m_data;
	size_t m_size;
};

static GpShaderCodeBlob GetBinarizedShader(const unsigned char **shaderPointers)
{
	GpShaderCodeBlob blob;
	blob.m_data = shaderPointers[0];
	blob.m_size = shaderPointers[1] - shaderPointers[0];

	return blob;
}

void DebugPrintf(const char *fmt, ...)
{
	char buf[256];
	va_list argp;
	va_start(argp, fmt);
	vsnprintf_s(buf, 255, fmt, argp);
	OutputDebugString(buf);
	va_end(argp);
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void StartD3DForWindow(HWND hWnd, GpComPtr<IDXGISwapChain1>& outSwapChain, GpComPtr<ID3D11Device>& outDevice, GpComPtr<ID3D11DeviceContext>& outContext)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;

	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	swapChainDesc.BufferCount = 2;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc;

	ZeroMemory(&swapChainFullscreenDesc, sizeof(swapChainFullscreenDesc));

	swapChainFullscreenDesc.Windowed = TRUE;
	swapChainFullscreenDesc.RefreshRate.Numerator = 60;
	swapChainFullscreenDesc.RefreshRate.Denominator = 1;

	UINT flags = 0;
	const D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_10_0
	};

	flags |= D3D11_CREATE_DEVICE_DEBUG;

	ID3D11Device *device = NULL;
	ID3D11DeviceContext *context = NULL;

	D3D_FEATURE_LEVEL selectedFeatureLevel;

	HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, featureLevels, sizeof(featureLevels) / sizeof(featureLevels[0]),
		D3D11_SDK_VERSION, &device, &selectedFeatureLevel, &context);

	IDXGIDevice2 *dxgiDevice = nullptr;
	result = device->QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void**>(&dxgiDevice));

	IDXGIAdapter *dxgiAdapter = nullptr;
	result = dxgiDevice->GetAdapter(&dxgiAdapter);

	IDXGIFactory2 *dxgiFactory = nullptr;
	result = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory));

	IDXGISwapChain1 *swapChain = nullptr;
	result = dxgiFactory->CreateSwapChainForHwnd(device, hWnd, &swapChainDesc, nullptr, nullptr, &swapChain);

	// GP TODO: Fix the error handling here, it's bad...
	outSwapChain = swapChain;
	outDevice = device;
	outContext = context;
}

bool GpDisplayDriverD3D11::InitResources()
{
	// Fetch back buffer
	m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(m_backBufferTexture.GetMutablePtr()));

	{
		D3D11_TEXTURE2D_DESC bbTextureDesc;
		m_backBufferTexture->GetDesc(&bbTextureDesc);

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		if (m_device->CreateRenderTargetView(m_backBufferTexture, &rtvDesc, m_backBufferRTV.GetMutablePtr()) != S_OK)
			return false;
	}

	// Quad vertex constant buffer
	{
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.ByteWidth = sizeof(DrawQuadVertexConstants);
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		if (m_device->CreateBuffer(&bufferDesc, nullptr, m_drawQuadVertexConstantBuffer.GetMutablePtr()) != S_OK)
			return false;
	}

	// Quad index buffer
	{
		const uint16_t indexBufferData[] = { 0, 1, 2, 1, 3, 2 };

		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.ByteWidth = sizeof(indexBufferData);
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA initialData;
		initialData.pSysMem = indexBufferData;
		initialData.SysMemPitch = 0;
		initialData.SysMemSlicePitch = 0;

		if (m_device->CreateBuffer(&bufferDesc, &initialData, m_quadIndexBuffer.GetMutablePtr()) != S_OK)
			return false;
	}

	// Quad vertex buffer
	{
		const float vertexBufferData[] =
		{
			0.f, 0.0f,
			1.f, 0.f,
			0.f, 1.f,
			1.f, 1.f,
		};

		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.ByteWidth = sizeof(vertexBufferData);
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA initialData;
		initialData.pSysMem = vertexBufferData;
		initialData.SysMemPitch = 0;
		initialData.SysMemSlicePitch = 0;

		if (m_device->CreateBuffer(&bufferDesc, &initialData, m_quadVertexBuffer.GetMutablePtr()) != S_OK)
			return false;
	}

	const GpShaderCodeBlob drawQuadVBlob = GetBinarizedShader(GpBinarizedShaders::g_drawQuadV_D3D11);
	const GpShaderCodeBlob drawQuadPalettePBlob = GetBinarizedShader(GpBinarizedShaders::g_drawQuadPaletteP_D3D11);
	const GpShaderCodeBlob drawQuadRGBPBlob = GetBinarizedShader(GpBinarizedShaders::g_drawQuadRGBP_D3D11);
	const GpShaderCodeBlob drawQuad15BitPBlob = GetBinarizedShader(GpBinarizedShaders::g_drawQuad15BitP_D3D11);

	m_device->CreateVertexShader(drawQuadVBlob.m_data, drawQuadVBlob.m_size, nullptr, m_drawQuadVertexShader.GetMutablePtr());
	m_device->CreatePixelShader(drawQuadPalettePBlob.m_data, drawQuadPalettePBlob.m_size, nullptr, m_drawQuadPalettePixelShader.GetMutablePtr());
	m_device->CreatePixelShader(drawQuadRGBPBlob.m_data, drawQuadRGBPBlob.m_size, nullptr, m_drawQuadRGBPixelShader.GetMutablePtr());
	m_device->CreatePixelShader(drawQuad15BitPBlob.m_data, drawQuad15BitPBlob.m_size, nullptr, m_drawQuad15BitPixelShader.GetMutablePtr());

	// Quad input layout
	{
		D3D11_INPUT_ELEMENT_DESC descs[] =
		{
			"POSITION",						// Semantic name
			0,								// Semantic index
			DXGI_FORMAT_R32G32_FLOAT,	// Format
			0,								// Input slot
			0,								// Aligned byte offset
			D3D11_INPUT_PER_VERTEX_DATA,	// Input slot class
			0								// Instance data step rate
		};

		m_device->CreateInputLayout(descs, sizeof(descs) / sizeof(descs[0]), drawQuadVBlob.m_data, drawQuadVBlob.m_size, m_drawQuadInputLayout.GetMutablePtr());
	}

	// Quad depth stencil state
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		desc.DepthEnable = FALSE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		desc.StencilEnable = FALSE;
		desc.StencilReadMask = 0;
		desc.StencilWriteMask = 0;
		desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		if (m_device->CreateDepthStencilState(&desc, m_drawQuadDepthStencilState.GetMutablePtr()) != S_OK)
			return false;
	}

	// Nearest neighbor sampler desc
	{
		D3D11_SAMPLER_DESC samplerDesc;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0.f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = samplerDesc.BorderColor[3] = 1.0f;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = FLT_MAX;

		if (m_device->CreateSamplerState(&samplerDesc, m_nearestNeighborSamplerState.GetMutablePtr()) != S_OK)
			return false;
	}

	DXGI_FORMAT paletteTextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// Palette texture
	{
		D3D11_TEXTURE1D_DESC desc;
		desc.Width = 256;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = paletteTextureFormat;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

		uint8_t initialDataBytes[256][4];
		for (int i = 0; i < 256; i++)
		{
			for (int ch = 0; ch < 4; ch++)
				initialDataBytes[i][ch] = 255;
		}

		D3D11_SUBRESOURCE_DATA initialData;
		initialData.pSysMem = initialDataBytes[0];
		initialData.SysMemPitch = 256 * 4;
		initialData.SysMemSlicePitch = 256 * 4;

		if (m_device->CreateTexture1D(&desc, &initialData, m_paletteTexture.GetMutablePtr()) != S_OK)
			return false;
	}

	// Palette texture SRV
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		desc.Format = paletteTextureFormat;
		desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1D;
		desc.Texture1D.MostDetailedMip = 0;
		desc.Texture1D.MipLevels = 1;

		if (m_device->CreateShaderResourceView(m_paletteTexture, &desc, m_paletteTextureSRV.GetMutablePtr()) != S_OK)
			return false;
	}

	return true;
}


bool GpDisplayDriverD3D11::PresentFrameAndSync()
{
	SynchronizeCursors();

	float clearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

	m_deviceContext->ClearRenderTargetView(m_backBufferRTV, clearColor);

	ID3D11RenderTargetView *const rtv = m_backBufferRTV;
	m_deviceContext->OMSetRenderTargets(1, &rtv, nullptr);

	{
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = static_cast<FLOAT>(m_windowWidth);
		viewport.Height = static_cast<FLOAT>(m_windowHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		m_deviceContext->RSSetViewports(1, &viewport);
	}

	m_properties.m_renderFunc(m_properties.m_renderFuncContext);

	DXGI_PRESENT_PARAMETERS presentParams;

	ZeroMemory(&presentParams, sizeof(presentParams));

	UINT lastPresentCount = 0;

	if (FAILED(m_swapChain->GetLastPresentCount(&lastPresentCount)))
		return false;

	if (FAILED(m_swapChain->Present1(1, 0, &presentParams)))
		return false;

	//DebugPrintf("r: %i\n", static_cast<int>(r));

	DXGI_FRAME_STATISTICS stats;
	if (FAILED(m_swapChain->GetFrameStatistics(&stats)))
		return false;

	if (stats.SyncQPCTime.QuadPart != 0)
	{
		if (m_syncTimeBase.QuadPart == 0)
			m_syncTimeBase = stats.SyncQPCTime;

		LARGE_INTEGER timestamp;
		timestamp.QuadPart = stats.SyncQPCTime.QuadPart - m_syncTimeBase.QuadPart;

		bool compacted = false;
		if (m_presentHistory.Size() > 0)
		{
			CompactedPresentHistoryItem &lastItem = m_presentHistory[m_presentHistory.Size() - 1];
			LONGLONG timeDelta = timestamp.QuadPart - lastItem.m_timestamp.QuadPart;

			if (timeDelta < 0)
				timeDelta = 0;	// This should never happen

			if (timeDelta * static_cast<LONGLONG>(m_properties.m_frameTimeLockDenominator) < m_QPFrequency.QuadPart * static_cast<LONGLONG>(m_properties.m_frameTimeLockNumerator))
			{
				lastItem.m_numFrames++;
				compacted = true;
			}
		}

		if (!compacted)
		{
			if (m_presentHistory.Size() == m_presentHistory.CAPACITY)
				m_presentHistory.RemoveFromStart();

			CompactedPresentHistoryItem *newItem = m_presentHistory.Append();
			newItem->m_timestamp = timestamp;
			newItem->m_numFrames = 1;
		}
	}

	if (m_presentHistory.Size() >= 2)
	{
		const size_t presentHistorySizeMinusOne = m_presentHistory.Size() - 1;
		unsigned int numFrames = 0;
		for (size_t i = 0; i < presentHistorySizeMinusOne; i++)
			numFrames += m_presentHistory[i].m_numFrames;

		LONGLONG timeFrame = m_presentHistory[presentHistorySizeMinusOne].m_timestamp.QuadPart - m_presentHistory[0].m_timestamp.QuadPart;

		unsigned int cancelledFrames = 0;
		LONGLONG cancelledTime = 0;

		const int overshootTolerance = 2;

		for (size_t i = 0; i < presentHistorySizeMinusOne; i++)
		{
			LONGLONG blockTimeframe = m_presentHistory[i + 1].m_timestamp.QuadPart - m_presentHistory[i].m_timestamp.QuadPart;
			unsigned int blockNumFrames = m_presentHistory[i].m_numFrames;

			if (blockTimeframe * static_cast<LONGLONG>(numFrames) >= timeFrame * static_cast<LONGLONG>(blockNumFrames) * overshootTolerance)
			{
				cancelledTime += blockTimeframe;
				cancelledFrames += blockNumFrames;
			}
		}

		numFrames -= cancelledFrames;
		timeFrame -= cancelledTime;

		// timeFrame / numFrames = Frame timestep
		// Unless Frame timestep is within the frame lock range, a.k.a.
		// timeFrame / numFrames / qpFreq >= minFrameTimeNum / minFrameTimeDenom

		bool isInFrameTimeLock = false;
		if (timeFrame * static_cast<LONGLONG>(m_properties.m_frameTimeLockMinDenominator) >= static_cast<LONGLONG>(numFrames) * static_cast<LONGLONG>(m_properties.m_frameTimeLockMinNumerator) * m_QPFrequency.QuadPart
			&& timeFrame * static_cast<LONGLONG>(m_properties.m_frameTimeLockMaxDenominator) <= static_cast<LONGLONG>(numFrames) * static_cast<LONGLONG>(m_properties.m_frameTimeLockMaxNumerator) * m_QPFrequency.QuadPart)
		{
			isInFrameTimeLock = true;
		}

		LONGLONG frameTimeStep = m_frameTimeSliceSize;
		if (!isInFrameTimeLock)
		{
			const int MAX_FRAMES_PER_STEP = 4;

			frameTimeStep = timeFrame / numFrames;
			if (frameTimeStep > m_frameTimeSliceSize * MAX_FRAMES_PER_STEP)
				frameTimeStep = m_frameTimeSliceSize * MAX_FRAMES_PER_STEP;
		}

		m_frameTimeAccumulated += frameTimeStep;
		while (m_frameTimeAccumulated >= m_frameTimeSliceSize)
		{
			m_properties.m_tickFunc(m_properties.m_tickFuncContext, m_vosFiber);
			m_frameTimeAccumulated -= m_frameTimeSliceSize;
		}
	}

	return true;
}

void GpDisplayDriverD3D11::SynchronizeCursors()
{
	HCURSOR replacementCursor = nullptr;

	if (m_activeCursor)
	{
		if (m_pendingCursor != m_activeCursor)
		{
			if (m_pendingCursor == nullptr)
			{
				m_currentStandardCursor = m_pendingStandardCursor;
				ChangeToStandardCursor(m_currentStandardCursor);

				m_activeCursor->DecRef();
				m_activeCursor = nullptr;
			}
			else
			{
				ChangeToCursor(m_pendingCursor->GetHCursor());

				m_pendingCursor->IncRef();
				m_activeCursor->DecRef();
				m_activeCursor = m_pendingCursor;
			}
		}
	}
	else
	{
		if (m_pendingCursor)
		{
			m_pendingCursor->IncRef();
			m_activeCursor = m_pendingCursor;

			ChangeToCursor(m_activeCursor->GetHCursor());
		}
		else
		{
			if (m_pendingStandardCursor != m_currentStandardCursor)
			{
				ChangeToStandardCursor(m_pendingStandardCursor);
				m_currentStandardCursor = m_pendingStandardCursor;
			}
		}
	}
}


void GpDisplayDriverD3D11::ChangeToCursor(HCURSOR cursor)
{
	if (m_mouseIsInClientArea)
		SetCursor(cursor);

	SetClassLongPtrW(m_hwnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cursor));
}

void GpDisplayDriverD3D11::ChangeToStandardCursor(EGpStandardCursor_t cursor)
{
	switch (cursor)
	{
	case EGpStandardCursors::kArrow:
	default:
		ChangeToCursor(m_arrowCursor);
		break;
	}
}

void GpDisplayDriverD3D11::Run()
{
	WNDCLASSEX wc;

	LPVOID fiber = ConvertThreadToFiberEx(this, 0);
	if (!fiber)
		return;	// ???

	m_vosFiber = new GpFiber_Win32(fiber);

	ZeroMemory(&wc, sizeof(wc));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WinProc;
	wc.hInstance = g_gpWindowsGlobals.m_hInstance;
	wc.hCursor = m_arrowCursor;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "GPD3D11WindowClass";

	RegisterClassEx(&wc);

	LONG windowStyle = WS_OVERLAPPEDWINDOW;
	HMENU menus = NULL;

	// TODO: Fix the resolution here
	RECT wr = { 0, 0, m_windowWidth, m_windowHeight };
	AdjustWindowRect(&wr, windowStyle, menus != NULL);

	m_hwnd = CreateWindowExW(NULL, L"GPD3D11WindowClass", L"GlidePort (Direct3D 11)", WS_OVERLAPPEDWINDOW, 300, 300, wr.right - wr.left, wr.bottom - wr.top, NULL, menus, g_gpWindowsGlobals.m_hInstance, NULL);

	ShowWindow(m_hwnd, g_gpWindowsGlobals.m_nCmdShow);

	StartD3DForWindow(m_hwnd, m_swapChain, m_device, m_deviceContext);

	InitResources();

	LARGE_INTEGER lastTimestamp;
	memset(&lastTimestamp, 0, sizeof(lastTimestamp));

	MSG msg;
	for (;;)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);

			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;
			else if (msg.message == WM_MOUSEMOVE)
				m_mouseIsInClientArea = true;
			else if (msg.message == WM_MOUSELEAVE)
				m_mouseIsInClientArea = false;
		}
		else
		{
			PresentFrameAndSync();
		}
	}

	// Exit
	ConvertFiberToThread();
}

void GpDisplayDriverD3D11::Shutdown()
{
	delete this;
}

void GpDisplayDriverD3D11::GetDisplayResolution(unsigned int *width, unsigned int *height, PortabilityLayer::PixelFormat *pixelFormat)
{
	if (width)
		*width = m_windowWidth;
	if (height)
		*height = m_windowHeight;
	if (pixelFormat)
		*pixelFormat = PortabilityLayer::PixelFormat_8BitStandard;
}

IGpDisplayDriverSurface *GpDisplayDriverD3D11::CreateSurface(size_t width, size_t height, PortabilityLayer::PixelFormat pixelFormat)
{
	return GpDisplayDriverSurfaceD3D11::Create(m_device, m_deviceContext, width, height, pixelFormat);
}

void GpDisplayDriverD3D11::DrawSurface(IGpDisplayDriverSurface *surface, size_t x, size_t y, size_t width, size_t height)
{
	ID3D11Buffer *vbPtr = m_quadVertexBuffer;
	UINT vbStride = sizeof(float) * 2;
	UINT zero = 0;

	GpDisplayDriverSurfaceD3D11 *d3d11Surface = static_cast<GpDisplayDriverSurfaceD3D11*>(surface);

	//m_deviceContext->OMSetDepthStencilState(m_drawQuadDepthStencilState, 0);

	{
		const float twoDivWidth = 2.0f / static_cast<float>(m_windowWidth);
		const float negativeTwoDivHeight = -2.0f / static_cast<float>(m_windowHeight);

		DrawQuadVertexConstants constantsData;
		constantsData.m_ndcOriginX = static_cast<float>(x) * twoDivWidth - 1.0f;
		constantsData.m_ndcOriginY = static_cast<float>(y) * negativeTwoDivHeight + 1.0f;
		constantsData.m_ndcWidth = static_cast<float>(width) * twoDivWidth;
		constantsData.m_ndcHeight = static_cast<float>(height) * negativeTwoDivHeight;

		constantsData.m_surfaceDimensionX = static_cast<float>(d3d11Surface->GetWidth());
		constantsData.m_surfaceDimensionY = static_cast<float>(d3d11Surface->GetHeight());

		D3D11_MAPPED_SUBRESOURCE mappedConstants;
		if (m_deviceContext->Map(m_drawQuadVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedConstants) == S_OK)
		{
			memcpy(mappedConstants.pData, &constantsData, sizeof(constantsData));
			m_deviceContext->Unmap(m_drawQuadVertexConstantBuffer, 0);
		}
	}

	m_deviceContext->IASetVertexBuffers(0, 1, &vbPtr, &vbStride, &zero);
	m_deviceContext->IASetIndexBuffer(m_quadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->IASetInputLayout(m_drawQuadInputLayout);

	ID3D11Buffer *vsConstants = m_drawQuadVertexConstantBuffer;
	m_deviceContext->VSSetShader(m_drawQuadVertexShader, nullptr, 0);
	m_deviceContext->VSSetConstantBuffers(0, 1, &vsConstants);

	ID3D11SamplerState *samplerStates[] =
	{
		m_nearestNeighborSamplerState,
	};
	m_deviceContext->PSSetSamplers(0, sizeof(samplerStates) / sizeof(samplerStates[0]), samplerStates);

	PortabilityLayer::PixelFormat pixelFormat = d3d11Surface->GetPixelFormat();
	if (pixelFormat == PortabilityLayer::PixelFormat_8BitStandard || pixelFormat == PortabilityLayer::PixelFormat_8BitCustom)
	{
		ID3D11ShaderResourceView *psResourceViews[] =
		{
			d3d11Surface->GetSRV(),
			m_paletteTextureSRV
		};

		m_deviceContext->PSSetShader(m_drawQuadPalettePixelShader, nullptr, 0);
		m_deviceContext->PSSetShaderResources(0, sizeof(psResourceViews) / sizeof(psResourceViews[0]), psResourceViews);
	}
	else if (pixelFormat == PortabilityLayer::PixelFormat_RGB555)
	{
		ID3D11ShaderResourceView *psResourceViews[] =
		{
			d3d11Surface->GetSRV(),
		};

		m_deviceContext->PSSetShader(m_drawQuad15BitPixelShader, nullptr, 0);
		m_deviceContext->PSSetShaderResources(0, sizeof(psResourceViews) / sizeof(psResourceViews[0]), psResourceViews);
	}
	else if (pixelFormat == PortabilityLayer::PixelFormat_RGB32)
	{
		ID3D11ShaderResourceView *psResourceViews[] =
		{
			d3d11Surface->GetSRV(),
		};

		m_deviceContext->PSSetShader(m_drawQuadRGBPixelShader, nullptr, 0);
		m_deviceContext->PSSetShaderResources(0, sizeof(psResourceViews) / sizeof(psResourceViews[0]), psResourceViews);
	}
	else
	{
		return;
	}

	m_deviceContext->DrawIndexed(6, 0, 0);
}

IGpColorCursor *GpDisplayDriverD3D11::LoadColorCursor(int cursorID)
{
	const size_t bufSize = MAX_PATH;
	wchar_t path[bufSize];

	int sz = _snwprintf(path, bufSize, L"%sPackaged\\WinCursors\\%i.cur", m_osGlobals->m_baseDir, cursorID);
	if (sz < 0 || static_cast<size_t>(sz) >= bufSize)
		return nullptr;

	return GpColorCursor_Win32::Load(path);
}

// We can't just set the cursor because we want to post WM_SETCURSOR to keep it limited
// to the game window area, but depending on the fiber implementation, this may not be
// the window thread.
void GpDisplayDriverD3D11::SetColorCursor(IGpColorCursor *colorCursor)
{
	GpColorCursor_Win32 *winCursor = static_cast<GpColorCursor_Win32*>(colorCursor);

	winCursor->IncRef();

	if (m_pendingCursor)
		m_pendingCursor->DecRef();

	m_pendingCursor = winCursor;
}

void GpDisplayDriverD3D11::SetStandardCursor(EGpStandardCursor_t standardCursor)
{
	if (m_pendingCursor)
	{
		m_pendingCursor->DecRef();
		m_pendingCursor = nullptr;
	}

	m_pendingStandardCursor = standardCursor;
}

void GpDisplayDriverD3D11::UpdatePalette(const void *paletteData)
{
	const size_t dataSize = 256 * 4;
	const uint8_t *paletteDataBytes = static_cast<const uint8_t *>(paletteData);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (m_deviceContext->Map(m_paletteTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) == S_OK)
	{
		uint8_t *textureDataBytes = static_cast<uint8_t *>(mappedResource.pData);

		for (size_t chunkOffset = 0; chunkOffset < dataSize; chunkOffset += sizeof(__m128i))
		{
			__m128i chunkData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(paletteDataBytes + chunkOffset));
			_mm_stream_si128(reinterpret_cast<__m128i*>(textureDataBytes + chunkOffset), chunkData);
		}

		m_deviceContext->Unmap(m_paletteTexture, 0);
	}
}

GpDisplayDriverD3D11 *GpDisplayDriverD3D11::Create(const GpDisplayDriverProperties &properties)
{
	return new GpDisplayDriverD3D11(properties);
}

GpDisplayDriverD3D11::GpDisplayDriverD3D11(const GpDisplayDriverProperties &properties)
	: m_properties(properties)
	, m_frameTimeAccumulated(0)
	, m_windowWidth(640)
	, m_windowHeight(480)
	, m_vosFiber(nullptr)
	, m_osGlobals(static_cast<GpWindowsGlobals*>(properties.m_osGlobals))
	, m_pendingCursor(nullptr)
	, m_activeCursor(nullptr)
	, m_currentStandardCursor(EGpStandardCursors::kArrow)
	, m_pendingStandardCursor(EGpStandardCursors::kArrow)
	, m_mouseIsInClientArea(false)
{
	memset(&m_syncTimeBase, 0, sizeof(m_syncTimeBase));

	QueryPerformanceFrequency(&m_QPFrequency);

	m_frameTimeSliceSize = m_QPFrequency.QuadPart * static_cast<LONGLONG>(properties.m_frameTimeLockNumerator) / static_cast<LONGLONG>(properties.m_frameTimeLockDenominator);

	m_arrowCursor = reinterpret_cast<HCURSOR>(LoadImageW(nullptr, MAKEINTRESOURCEW(OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_SHARED));
}

GpDisplayDriverD3D11::~GpDisplayDriverD3D11()
{
	// GP TODO: Sloppy cleanup... Close the window!!
}