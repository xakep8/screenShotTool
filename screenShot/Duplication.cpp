//#include <stdafx.h>
#include "Duplication.h"
#include <windows.h>
#include <gdiplus.h>
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define RESET_OBJECT(obj) { if(obj) obj->Release(); obj=NULL; }

static BOOL g_bAttach = FALSE;

Duplication::Duplication()
{
	m_bInit = FALSE;
	m_hDevice = NULL;
	m_hContext = NULL;
	m_hDeskDupl = NULL;
	ZeroMemory(&m_dxgiOutDesc, sizeof(m_dxgiOutDesc));

}

Duplication::~Duplication()
{
	Deinit();
}

BOOL Duplication::init()
{
	HRESULT hr = S_OK;

	if (m_bInit)
		return FALSE;

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(featureLevels);

	D3D_FEATURE_LEVEL FeatureLevel;

	for (UINT driverTypeIndex = 0; driverTypeIndex < NumDriverTypes; driverTypeIndex++)
	{
		hr = D3D11CreateDevice(
			NULL,
			driverTypes[driverTypeIndex],
			NULL,
			0,
			featureLevels,
			NumFeatureLevels,
			D3D11_SDK_VERSION,
			&m_hDevice,
			&FeatureLevel,
			&m_hContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return FALSE;

	IDXGIDevice* dxgiDevice = NULL;
	hr = m_hDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
	if (FAILED(hr))
		return FALSE;

	IDXGIAdapter* hDxgiAdapter = NULL;
	hr = dxgiDevice->GetAdapter(&hDxgiAdapter);
	RESET_OBJECT(dxgiDevice);
	if (FAILED(hr))
		return FALSE;

	INT nOutput = 0;

	IDXGIOutput* hDxgiOutput = NULL;
	hr = hDxgiAdapter->EnumOutputs(nOutput, &hDxgiOutput);
	RESET_OBJECT(hDxgiAdapter);
	if (FAILED(hr))
		return FALSE;

	hDxgiOutput->GetDesc(&m_dxgiOutDesc);

	IDXGIOutput1* hDxgiOutput1 = NULL;
	hr = hDxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&hDxgiOutput1));
	RESET_OBJECT(hDxgiOutput);
	if (FAILED(hr))
		return FALSE;

	hr = hDxgiOutput1->DuplicateOutput(m_hDevice, &m_hDeskDupl);
	RESET_OBJECT(hDxgiOutput1);
	if (FAILED(hr))
		return FALSE;

	m_bInit = TRUE;
	return TRUE;
}

VOID Duplication::Deinit()
{
	if (!m_bInit)
		return;

	m_bInit = FALSE;

	if (m_hDeskDupl) {
		m_hDeskDupl->Release();
		m_hDeskDupl = NULL;
	}

	if (m_hDevice)
	{
		m_hDevice->Release();
		m_hDevice = NULL;
	}

	if (m_hContext)
	{
		m_hContext->Release();
		m_hContext = NULL;
	}
}

BOOL Duplication::AttachToThread(VOID)
{
	if (g_bAttach)
		return TRUE;

	HDESK hCurrentDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
	if (!hCurrentDesktop)
		return FALSE;

	BOOL bDesktopAttached = SetThreadDesktop(hCurrentDesktop);
	CloseDesktop(hCurrentDesktop);
	hCurrentDesktop = NULL;

	g_bAttach = TRUE;
	return bDesktopAttached;
}

BOOL Duplication::CaptureImage(RECT& rect, void* pData, INT& nLen)
{
	return QueryFrame(pData, nLen);
}

BOOL Duplication::CaptureImage(void* pData, INT& nLen)
{
	return QueryFrame(pData, nLen);
}

BOOL Duplication::ResetDevice()
{
	Deinit();
	return init();
}

BOOL Duplication::QueryFrame(void* pImgData, INT& nImgSize)
{
	if (!m_bInit || !AttachToThread())
		return FALSE;

	nImgSize = 0;
	UINT timeout = 15;
	IDXGIResource* zhDesktopResource = NULL;
	DXGI_OUTDUPL_FRAME_INFO zFrameInfo;
	HRESULT hr = m_hDeskDupl->AcquireNextFrame(500, &zFrameInfo, &zhDesktopResource);
	if (FAILED(hr))
	{
		return FALSE;
	}
	ID3D11Texture2D* zhAcquiredDesktopImage = NULL;
	hr = zhDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&zhAcquiredDesktopImage));
	RESET_OBJECT(zhDesktopResource);
	if (FAILED(hr))
	{
		return FALSE;
	}

	D3D11_TEXTURE2D_DESC desc;
	zhAcquiredDesktopImage->GetDesc(&desc);

	ID3D11Texture2D* hNewDesktopImage = NULL;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	hr = m_hDevice->CreateTexture2D(&desc, NULL, &hNewDesktopImage);
	if (FAILED(hr)) {
		RESET_OBJECT(zhAcquiredDesktopImage);
		m_hDeskDupl->ReleaseFrame();
		return FALSE;
	}
	m_hContext->CopyResource(hNewDesktopImage, zhAcquiredDesktopImage);
	RESET_OBJECT(zhAcquiredDesktopImage);
	m_hDeskDupl->ReleaseFrame();
	IDXGISurface* hStagingSurf = NULL;
	hr = hNewDesktopImage->QueryInterface(__uuidof(IDXGISurface), (void**)(&hStagingSurf));
	RESET_OBJECT(hNewDesktopImage);
	if (FAILED(hr)) {
		return FALSE;
	}
	DXGI_MAPPED_RECT mappedRect;
	hr = hStagingSurf->Map(&mappedRect, DXGI_MAP_READ);
	if (SUCCEEDED(hr)) {
		int imageSize = m_dxgiOutDesc.DesktopCoordinates.right * m_dxgiOutDesc.DesktopCoordinates.bottom * 4;
		memcpy((BYTE*)pImgData, mappedRect.pBits, m_dxgiOutDesc.DesktopCoordinates.right * m_dxgiOutDesc.DesktopCoordinates.bottom * 4);
		nImgSize = imageSize;
		hStagingSurf->Unmap();
	}
	RESET_OBJECT(hStagingSurf);
	return SUCCEEDED(hr);
}

BOOL Duplication::CaptureRegion(INT x, INT y, INT width, INT height, void* pData, INT& nLen)
{
	INT fullScreenSize = GetWidth() * GetHeight() * 4;
	std::unique_ptr<unsigned char[]> fullScreenBuffer(new unsigned char[fullScreenSize]);
	INT capturedSize = fullScreenSize;
	if(!QueryFrame(fullScreenBuffer.get(), capturedSize)) {
		return FALSE;
	}
	INT screenWidth = GetWidth();
	INT screenHeight = GetHeight();
	if (x < 0 || y < 0 || x + width > screenWidth || y + height > screenHeight) {
		if (x < 0) { width += x; x = 0; }
		if (y < 0) { height += y; y = 0; }
		if (x + width > screenWidth) width = screenWidth - x;
		if (y + height > screenHeight) height = screenHeight - y;

		if (width <= 0 || height <= 0) return FALSE; // Invalid region
	}
	nLen = width * height * 4;

	unsigned char* pDst = static_cast<unsigned char*>(pData);
	unsigned char* pSrc = fullScreenBuffer.get();

	for(INT row=0; row < height; row++) {
		int srcPos = ((y + row) * screenWidth + x) * 4;
		int dstPos = row * width * 4;
		memcpy(pDst + dstPos, pSrc + srcPos, width * 4);
	}

	return TRUE;
}