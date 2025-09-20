#pragma once

#include <sal.h>
#include <d3d11.h>
#include <dxgi1_2.h>

class Duplication {
public:
	Duplication();
	~Duplication();

public:
	BOOL init();
	VOID Deinit();

public:
	virtual BOOL CaptureImage(RECT& rect, void* pData, INT& nLen);
	virtual BOOL CaptureImage(void* pData, INT& nLen);
	virtual BOOL ResetDevice();

private:
	BOOL AttachToThread(VOID);
	BOOL QueryFrame(void* pImgData, INT& nImgSize);
	BOOL QueryFrame(void* pImagData, INT& nImgSize, int z);

private:
	IDXGIResource* zhDesktopResource;
	DXGI_OUTDUPL_FRAME_INFO zFrameInfo;
	ID3D11Texture2D* zhAcquiredDesktopImage;
	IDXGISurface* zhStagingStuff;

private:
	BOOL m_bInit;
	int m_iWidth, m_iHeight;
	ID3D11Device* m_hDevice;
	ID3D11DeviceContext* m_hContext;
	IDXGIOutputDuplication* m_hDeskDupl;
	DXGI_OUTPUT_DESC m_dxgiOutDesc;

public:
	INT GetWidth() const { 
		return m_dxgiOutDesc.DesktopCoordinates.right - m_dxgiOutDesc.DesktopCoordinates.left; 
	}

	INT GetHeight() const { 
		return m_dxgiOutDesc.DesktopCoordinates.bottom - m_dxgiOutDesc.DesktopCoordinates.top; 
	}

	BOOL CaptureRegion(INT x, INT y, INT width, INT height, void* pData, INT& nLen);
};