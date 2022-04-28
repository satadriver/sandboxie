
#include <windows.h>
#include <map>
#include "functions.h"
#include "dllmain.h"
#include "apiFuns.h"
#include "log.h"
#include "WaterMark.h"
#include "hookapi.h"

DWORD g_tlsIdx = 0;




ShowWindowStub ShowWindowOld = 0;
BOOL WINAPI ShowWindowNew(
	HWND hWnd,
	int  nCmdShow
)
{
	/*DP1("[LYSM] ShowWindowNew nCmdShow : %d \n", nCmdShow);

	if (nCmdShow != SW_HIDE)
	{
		CreateWaterMarkAndBorderByHook(hWnd);
	}*/

	return ShowWindowOld(hWnd, nCmdShow);
}

BitBltStub BitBltOld = 0;
BOOL __stdcall BitBltNew(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop) {

	HDC oldhdc = (HDC)TlsGetValue(g_tlsIdx);
	if (oldhdc == hdcSrc) 
	{
		return TRUE;
	}

	if (BitBltOld)
	{
		return BitBltOld(hdc, x, y, cx, cy, hdcSrc, x1, y1, rop);
	}

	return FALSE;
}


GetDIBitsStub GetDIBitsOld = 0;
int WINAPI GetDIBitsNew( HDC hdc,  HBITMAP hbm,  UINT start,  UINT cLines, LPVOID lpvBits,LPBITMAPINFO lpbmi,  UINT usage) {

	if (GetDIBitsOld)
	{
		int result = 0;
		result = GetDIBitsOld(hdc, hbm, start, cLines, lpvBits, lpbmi, usage);
		if (lpvBits == 0)
		{
			return result;
		}

		HDC oldhdc=(HDC)TlsGetValue(g_tlsIdx);
		if (oldhdc == hdc )
 		{
			//int height = GetSystemMetrics(SM_CYSCREEN);
			//if (start == 0 && cLines == height)
			{
				//HWND hwnd = GetDesktopWindow();
				//SetWindowDisplayAffinity(hwnd, WDA_MONITOR);

				int cnt = lpbmi->bmiHeader.biSizeImage / sizeof(int);

				int flag = IsBadWritePtr(lpvBits, lpbmi->bmiHeader.biSizeImage);
				if (flag == 0)
				{
					DWORD* color = (DWORD*)lpvBits;
					for (int i = 0; i < cnt; i++)
					{
						color[i] = 0x0000ff00;
					}
				}

				//log(L"GetDIBits start:%d cLines:%d\r\n", start, cLines);
			}
		}

		return result;
	}

	return ERROR_INVALID_PARAMETER;
}


PrintWindowStub PrintWindowOld = 0;
BOOL WINAPI PrintWindowNew(_In_ HWND hwnd, _In_ HDC hdcBlt, _In_ UINT nFlags) {
	//log(L"PrintWindowNew entry");

	if (PrintWindowOld)
	{
		HWND h = GetDesktopWindow();
		if (h == hwnd)
		{
			//HWND hwnd = GetDesktopWindow();
			//SetWindowDisplayAffinity(hwnd, WDA_MONITOR);
			int result = SbieApi_QueryScreenshot();
			if (result == BASIC_DISABLE_STATE)
			{
				log(L"process %d call PrintWindow\r\n", GetCurrentProcessId());
				return TRUE;
			}
		}
		//log(L"PrintWindowNew leave");
		int result = PrintWindowOld(hwnd, hdcBlt, nFlags);

		return result;
	}
	return FALSE;
}


GetDCStub GetDCOld = 0;
HDC WINAPI GetDCNew(HWND hwnd) {

	HDC hdc = GetDCOld(hwnd);

	HWND hdesktop = GetDesktopWindow();
	if (hwnd == 0 || hdesktop == hwnd )
	{
		int result = SbieApi_QueryScreenshot();
		if (result == BASIC_DISABLE_STATE)
		{
			TlsSetValue(g_tlsIdx, hdc);
		}
	}

	return hdc;
}

CreateDCWStub CreateDCWOld = 0;
HDC WINAPI CreateDCWNew(
	_In_opt_ LPCWSTR pwszDriver,
	_In_opt_ LPCWSTR pwszDevice,
	_In_opt_ LPCWSTR pszPort,
	_In_opt_ CONST DEVMODEW* pdm
) {

	HDC hdc = CreateDCWOld(pwszDriver, pwszDevice, pszPort, pdm);

	if (hdc)
	{
		if (lstrcmpiW(pwszDriver, L"DISPLAY") == 0)
		{
			int result = SbieApi_QueryScreenshot();
			if (result == BASIC_DISABLE_STATE)
			{
				TlsSetValue(g_tlsIdx, hdc);
			}
		}
	}

	return hdc;
}

CreateDCAStub CreateDCAOld;
HDC WINAPI CreateDCANew(
	LPCSTR pszDriver,
	LPCSTR pszDevice,
	LPCSTR pszPort,
	CONST DEVMODE * pd
) {

	HDC hdc = CreateDCAOld(pszDriver, pszDevice, pszPort, pd);
	if ( hdc)
	{
		if (lstrcmpiA(pszDriver, "DISPLAY") == 0)
		{
			int result = SbieApi_QueryScreenshot();
			if (result == BASIC_DISABLE_STATE)
			{
				TlsSetValue(g_tlsIdx, hdc);
			}
		}
	}

	return hdc;
}

ptrGetWindowDC GetWindowDCOld = 0;
HDC __stdcall GetWindowDCNew(HWND hwnd)
{

	HDC hdc = GetWindowDCOld(hwnd);
	HWND hdesktop = GetDesktopWindow();

	if (hdesktop == hwnd)
	{
		int result = SbieApi_QueryScreenshot();
		if (result == BASIC_DISABLE_STATE)
		{
			TlsSetValue(g_tlsIdx, hdc);
		}
	}

	return hdc;
}





ptrGetLogicalDrives GetLogicalDrivesOld = 0;
int isVeracryptDisk(WCHAR* diskname) {

	int result = 0;
	WCHAR devname[MAX_PATH];

	result = QueryDosDeviceW(diskname, devname, MAX_PATH);
	if (result)
	{
		if (wcsstr(devname, L"\\Device\\VeraCryptVolume"))
		{
			return TRUE;
		}
	}

	return FALSE;
}
int isUsbStorage(WCHAR* diskname) {
	int result = 0;
	result = GetDriveTypeW(diskname);
	if (result == DRIVE_REMOVABLE)
	{
		return TRUE;
	}
	return FALSE;
}
DWORD __stdcall GetLogicalDrivesNew() {

	int result = 0;
	DWORD drives = GetLogicalDrivesOld();
	HMODULE hSfDeskDll = GetModuleHandle(L"SfDeskDll.dll");
	DWORD mask = 0xffffffff;
	WCHAR diskname[4];
	diskname[1] = L':';
	diskname[2] = 0;

	for (int i = 0;i < 26 ;i ++)
	{
		DWORD diskno = (drives >> i) & 1;
		if (diskno)
		{
			diskname[0] = 0x41 + i;
			int isvera = isVeracryptDisk(diskname);
			if (isvera)
			{
				DWORD submask = ~(1 << i);
				mask &= submask;
			}

			
			if (hSfDeskDll) {
				int isusb = isUsbStorage(diskname);
				if (isusb)
				{
					DWORD submask = ~(1 << i);
					mask &= submask;
				}
			}
		}
	}
	
	return drives & mask;
}

ptrFindFirstVolumeW FindFirstVolumeWOld = 0;
HANDLE __stdcall FindFirstVolumeWNew(LPWSTR lpszVolumeName, DWORD  cchBufferLength) {

	HANDLE handle = 0;
	handle = FindFirstVolumeWOld(lpszVolumeName, cchBufferLength);

	if (handle && *lpszVolumeName)
	{
		WCHAR tmpbuf[256];
		lstrcpyW(tmpbuf, &lpszVolumeName[4]);
		int tmpbuflen = lstrlenW(tmpbuf);
		tmpbuf[tmpbuflen - 1] = 0;

		int result = isVeracryptDisk(tmpbuf);
		if (result)
		{
			lpszVolumeName[0] = 0;
		}
	}

	return handle;
}

ptrFindNextVolumeW FindNextVolumeWOld = 0;
BOOL __stdcall FindNextVolumeWNew(
	HANDLE hFindVolume,
	LPWSTR lpszVolumeName,
	DWORD  cchBufferLength
) {

	int lastresult = 0;
	if (hFindVolume && hFindVolume != INVALID_HANDLE_VALUE)
	{
		lastresult = FindNextVolumeWOld(hFindVolume, lpszVolumeName, cchBufferLength);
		if (lastresult)
		{
			WCHAR tmpbuf[256];
			lstrcpyW(tmpbuf, &lpszVolumeName[4]);
			int tmpbuflen = lstrlenW(tmpbuf);
			tmpbuf[tmpbuflen - 1] = 0;

			int bveracrypt = isVeracryptDisk(tmpbuf);
			if (bveracrypt)
			{
				lpszVolumeName[0] = 0;
			}
		}
	}

	return lastresult;
}


ptrGetLogicalDriveStringsW GetLogicalDriveStringsWOld = 0;
DWORD __stdcall GetLogicalDriveStringsWNew(
	DWORD  nBufferLength,
	LPWSTR lpBuffer
) {
	DWORD len = GetLogicalDriveStringsWOld(nBufferLength, lpBuffer);
	int itemsize =  4;
	DWORD offset = 0;

	while(offset < len)
	{
		WCHAR path[16];
		lstrcpyW(path, lpBuffer + offset);
		path[2] = 0;
		int isvera = isVeracryptDisk(path);
		if (isvera)
		{
			wmemcpy(lpBuffer + offset, lpBuffer + offset + itemsize,len - offset - itemsize);
			len -= itemsize;
			continue;
		}

		int isusb = isUsbStorage(path);
		if (isusb)
		{
			wmemcpy(lpBuffer + offset, lpBuffer + offset + itemsize,len - offset - itemsize);
			len -= itemsize;
			continue;
		}

		offset += itemsize;
	}

	lpBuffer[len] = 0;
	return len;
}

ptrD3DXSaveSurfaceToFileW D3DXSaveSurfaceToFileWOld;
ptrD3DXSaveSurfaceToFileA D3DXSaveSurfaceToFileAOld = 0;
HRESULT __stdcall D3DXSaveSurfaceToFileANew(
	_In_       LPCSTR              pDestFile,
	_In_       LPVOID DestFormat,
	_In_       LPVOID   pSrcSurface,
	_In_ const PALETTEENTRY* pSrcPalette,
	_In_ const RECT* pSrcRect) {

	int result = D3DXSaveSurfaceToFileAOld(pDestFile, DestFormat, pSrcSurface, pSrcPalette, pSrcRect);

	return result;
}
HRESULT __stdcall D3DXSaveSurfaceToFileWNew(
	_In_       LPCWSTR              pDestFile,
	_In_       LPVOID DestFormat,
	_In_       LPVOID   pSrcSurface,
	_In_ const PALETTEENTRY* pSrcPalette,
	_In_ const RECT* pSrcRect
){
	int result = D3DXSaveSurfaceToFileWOld(pDestFile, DestFormat, pSrcSurface, pSrcPalette, pSrcRect);

	return result;
}


BOOLEAN g_msgboxPrompt = 0;
P_OpenPrinter2W  __sys_OpenPrinter2W = NULL;
P_OpenPrinterW __sys_OpenPrinterW = 0;
P_OpenPrinter2A __sys_OpenPrinter2A = NULL;
P_OpenPrinterA __sys_OpenPrinterA = 0;
BOOL newMessageBoxA(HWND    hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT   uType) {

	int result = 0;

	if (g_msgboxPrompt == 0)
	{
		g_msgboxPrompt = TRUE;
		result = MessageBoxA(hWnd, lpText, lpCaption, uType);
	}
	
	return result;
}
BOOL WINAPI OpenPrinterANew(
	_In_opt_    LPSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSA pDefault) {

	OutputDebugStringW(L"OpenPrinterA enter\r\n");

	int result = SbieApi_QueryPrinter();
	if (result == BASIC_DISABLE_STATE)
	{
		if (phPrinter)
		{
			result = IsBadWritePtr((LPVOID)phPrinter, sizeof(HANDLE));
			if (result == 0)
			{
				newMessageBoxA(0, "without permission to print document", "unauthorized printer", MB_OK);
				*phPrinter = 0;
				return FALSE;
			}
		}
	}

	return __sys_OpenPrinterA(pPrinterName, phPrinter, pDefault);
}
BOOL WINAPI OpenPrinter2ANew(
	_In_opt_    LPCSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSA pDefault, _In_opt_ PPRINTER_OPTIONSA       pOptions) {

	OutputDebugStringW(L"OpenPrinter2A enter\r\n");
	int result = SbieApi_QueryPrinter();
	if (result == BASIC_DISABLE_STATE)
	{
		if (phPrinter)
		{
			result = IsBadWritePtr((LPVOID)phPrinter, sizeof(HANDLE));
			if (result == 0)
			{
				newMessageBoxA(0, "without permission to print document", "unauthorized printer", MB_OK);
				*phPrinter = 0;
				return FALSE;
			}
		}
	}

	return __sys_OpenPrinter2A(pPrinterName, phPrinter, pDefault, pOptions);
}
BOOL WINAPI OpenPrinterWNew(
	_In_opt_    LPWSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSW pDefault) {

	OutputDebugStringW(L"OpenPrinterW enter\r\n");
	int result = SbieApi_QueryPrinter();
	if (result == BASIC_DISABLE_STATE)
	{
		if (phPrinter)
		{
			result = IsBadWritePtr((LPVOID)phPrinter, sizeof(HANDLE));
			if (result == 0)
			{
				newMessageBoxA(0, "without permission to print document", "unauthorized printer", MB_OK);
				*phPrinter = 0;
				return FALSE;
			}
		}
	}

	return __sys_OpenPrinterW(pPrinterName, phPrinter, pDefault);
}
BOOL OpenPrinter2WNew(void* pPrinterName, HANDLE* phPrinter, void* pDefault, void* pOptions) {
	OutputDebugStringW(L"OpenPrinter2W enter\r\n");
	int result = SbieApi_QueryPrinter();
	if (result == BASIC_DISABLE_STATE)
	{
		if (phPrinter)
		{
			result = IsBadWritePtr((LPVOID)phPrinter, sizeof(HANDLE));
			if (result == 0)
			{
				newMessageBoxA(0, "without permission to print document", "unauthorized printer", MB_OK);
				*phPrinter = 0;
				return FALSE;
			}
		}
	}

	return __sys_OpenPrinter2W(pPrinterName, phPrinter, pDefault, pOptions);
}


ptrLoadLibraryW lpLoadLibraryWOld;
HMODULE __stdcall lpLoadLibraryWNew(LPWSTR lpLibFileName) {

	int result = 0;
	HMODULE hmodule = lpLoadLibraryWOld(lpLibFileName);

	if (hmodule)
	{
		if (lstrcmpiW(lpLibFileName, L"winspool.drv") == 0)
		{
			result += hook(L"winspool.drv", L"OpenPrinterA", (LPBYTE)OpenPrinterANew, (FARPROC*)&__sys_OpenPrinterA);
			result += hook(L"winspool.drv", L"OpenPrinterW", (LPBYTE)OpenPrinterWNew, (FARPROC*)&__sys_OpenPrinterW);
			result += hook(L"winspool.drv", L"OpenPrinter2W", (LPBYTE)OpenPrinter2WNew, (FARPROC*)&__sys_OpenPrinter2W);

			log(L"winspool.drv loading and hook result:%d", result);
		}
	}

	return hmodule;
}


