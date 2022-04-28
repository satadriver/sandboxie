
#include <windows.h>
#include <map>
#include "functions.h"
#include "dllmain.h"
#include "apiFuns.h"
#include "log.h"
#include "WaterMark.h"
#include "hookapi.h"

DWORD g_tlsIdx = 0;

extern std::map<HWND, HWND> g_WaterMarkHwndMap;
extern std::map<HWND, WNDPROC> g_PreWndProcMap;

LRESULT CALLBACK NewWndProc(
	HWND    hWnd,
	UINT    Msg,
	WPARAM  wParam,
	LPARAM  lParam
)
{
	WNDPROC lpPrevWndFunc = g_PreWndProcMap.find(hWnd)->second;
	HWND HwndWaterMark = g_WaterMarkHwndMap.find(hWnd)->second;

	//DP2("[LYSM][NewWndProc] lpPrevWndFunc:%p HwndWaterMark:%p \n", lpPrevWndFunc, HwndWaterMark);

	switch (Msg)
	{
	case WM_SHOWWINDOW:
	{
		DP0("[LYSM][NewWndProc] WM_SHOWWINDOW \n");

		// 获取窗口矩形区域
		RECT RectHost;
		if (!GetWindowRect(hWnd, &RectHost))
		{
			DP1("[LYSM][NewWndProc] GetWindowRect error:%d \n", GetLastError());
			break;
		}

		// 获取窗口坐标
		POINT pt = { 0 };
		if (!ClientToScreen(hWnd, &pt))
		{
			DP1("[LYSM][NewWndProc] ClientToScreen error:%d \n", GetLastError());
			break;
		}

		// 移动水印
		if (!MoveWindow(
			HwndWaterMark,
			pt.x + 10,
			pt.y + 10,
			RectHost.right - RectHost.left - 10 * 2,
			RectHost.bottom - RectHost.top - 10 * 2,
			TRUE))
		{
			DP1("[LYSM][NewWndProc] MoveWindow error : %d \n", GetLastError());
			break;
		}

		ShowWindow(HwndWaterMark, SW_SHOW);

		break;
	}

	case WM_WINDOWPOSCHANGED:
	{
		PWINDOWPOS pInfo = (PWINDOWPOS)lParam;
		if (!pInfo)
		{
			break;
		}
		DP4("[LYSM][NewWndProc] WM_WINDOWPOSCHANGED %d %d %d %d \n", pInfo->x, pInfo->y, pInfo->cx, pInfo->cy);

		// 移动水印
		if (!MoveWindow(
			HwndWaterMark,
			pInfo->x + 10,
			pInfo->y + 10,
			pInfo->cx - 10 * 2,
			pInfo->cy - 10 * 2,
			TRUE))
		{
			DP1("[LYSM] MoveWindow error : %d \n", GetLastError());
			break;
		}

		ShowWindow(HwndWaterMark, SW_SHOW);

		break;
	}
	default:
		break;
	}

	return CallWindowProc(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}


SetWindowPosStub SetWindowPosOld = 0;
BOOL __stdcall SetWindowPosNew(
	HWND hWnd,
	HWND hWndInsertAfter,
	int  X,
	int  Y,
	int  cx,
	int  cy,
	UINT uFlags
){
	DP4("[LYSM][SetWindowPosNew] params: %d %d %d %d \n", X, Y, cx, cy);

	BOOL ret = SetWindowPosOld(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

	return ret;
}

CreateWindowExWStub CreateWindowExWOld = 0;
HWND __stdcall CreateWindowExWNew(
	DWORD     dwExStyle,
	LPCWSTR   lpClassName,
	LPCWSTR   lpWindowName,
	DWORD     dwStyle,
	int       X,
	int       Y,
	int       nWidth,
	int       nHeight,
	HWND      hWndParent,
	HMENU     hMenu,
	HINSTANCE hInstance,
	LPVOID    lpParam)
{

	DP4("[LYSM][CreateWindowExWNew] x,y:%d %d | Width,Height:%d %d \n", X, Y, nWidth, nHeight);

	// 过滤不正常窗口
	if (nWidth <= 10 || nHeight <= 10)
	{
		return CreateWindowExWOld(
			dwExStyle,
			lpClassName,
			lpWindowName,
			dwStyle,
			X,
			Y,
			nWidth,
			nHeight,
			hWndParent,
			hMenu,
			hInstance,
			lpParam
		);
	}

	// 保存参数
	INT OldX = X;
	INT OldY = Y;
	INT OldnWidth = nWidth;
	INT OldnHeight = nHeight;

	// 执行原函数
	HWND ret = CreateWindowExWOld(
		dwExStyle,
		lpClassName,
		lpWindowName,
		dwStyle,
		X,
		Y,
		nWidth,
		nHeight,
		hWndParent,
		hMenu,
		hInstance,
		lpParam
	);
	if (!ret)
	{
		return ret;
	}

#if 0
	// 创建水印窗口
	WaterMarkThreadInfo* pInfo = new WaterMarkThreadInfo();
	pInfo->hwnd = ret;
	pInfo->X = OldX;
	pInfo->Y = OldY;
	pInfo->nWidth = OldnWidth;
	pInfo->nHeight = OldnHeight;
	HANDLE hThread = CreateThread(0, 0, CreateWaterMarkWindowByThread, pInfo, 0, 0);
	if (!hThread)
	{
		DP1("[LYSM][CreateWindowExWNew] CreateThread error %d \n", GetLastError());
		return ret;
	}

	// 等待 WaterMarkHwndMap 保存完毕
	BOOL isMapValued = FALSE;
	for (INT i = 0; i < 100; ++i)
	{
		if (g_WaterMarkHwndMap.find(ret) != g_WaterMarkHwndMap.end())
		{
			isMapValued = TRUE;
			break;
		}
		Sleep(10);
	}
	if (!isMapValued)
	{
		DP0("[LYSM][CreateWindowExWNew] WaterMarkHwndMap not legal \n");
		return ret;
	}

	LONG_PTR OldWndProc = GetWindowLongPtr(ret, GWLP_WNDPROC);
	if (!OldWndProc)
	{
		DP1("[LYSM][CreateWindowExWNew] error %d \n", GetLastError());
		return ret;
	}
	g_PreWndProcMap.insert({ ret,(WNDPROC)OldWndProc });
	if (!SetWindowLongPtr(ret, GWLP_WNDPROC, (LONG)NewWndProc))
	{
		DP1("[LYSM][CreateWindowExWNew]  error %d \n", GetLastError());
		return ret;
	}
#endif

	return ret;
}

BitBltStub BitBltOld = 0;

BOOL __stdcall BitBltNew(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop) {

	HDC oldhdc = (HDC)TlsGetValue(g_tlsIdx);
	if (oldhdc == hdcSrc) 
	{
		//HWND hwnd = GetDesktopWindow();
		//SetWindowDisplayAffinity(hwnd, WDA_MONITOR);

		//int width = GetSystemMetrics(SM_CXSCREEN);
		//int height = GetSystemMetrics(SM_CYSCREEN);
		//if (width == cx && height == cy && x == 0 && y == 0 && x1 == 0 && y1 == 0)
		{
			//log(L"BitBlt x:%d y:%d cx:%d cy:%d x1:%d y1:%d\r\n", x, y, cx, cy, x1, y1);
			return TRUE;
		}
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

//QQ.exe
HDC WINAPI GetDCNew(HWND hwnd) {
	//log(L"GetDCNew entry");
	HDC hdc = GetDCOld(hwnd);

	HWND hdesktop = GetDesktopWindow();
	if (hwnd == 0 || hdesktop == hwnd )
	{
		int result = SbieApi_QueryScreenshot();
		if (result == BASIC_DISABLE_STATE)
		{
			//log(L"process %d get desktop window hdc\r\n", GetCurrentProcessId());
			TlsSetValue(g_tlsIdx, hdc);
		}
	}
	//log(L"GetDCNew leave");
	return hdc;
}
/*
监控获取桌面窗口句柄的函数调用来达到监控截屏的目的
类似的获取桌面handle的代码还有：
HWND hProgMan = ::FindWindow(L"ProgMan", NULL);
HWND hWndDesktop;
if(hProgMan)
{
	HWND hShellDefView = ::FindWindowEx(hProgMan, NULL, L"SHELLDLL_DefView", NULL);
	if(hShellDefView)
		hWndDesktop = ::FindWindowEx(hShellDefView, NULL, L"SysListView32", NULL);
}

以及：
HWND hwndWorkerW=NULL;
HWND hShellDefView=NULL;
HWND hwndDesktop=NULL;
while(hwndDesktop==NULL)//必须存在桌面窗口层次
{
	hwndWorkerW=::FindWindowEx(0,hwndWorkerW,L"WorkerW",NULL);//获得WorkerW类的窗口
	if(hwndWorkerW==NULL)
		break;//未知错误
	hShellDefView=::FindWindowEx(hwndWorkerW,NULL,L"SHELLDLL_DefView",NULL);
	if (hShellDefView==NULL)
		continue;
	hwndDesktop=::FindWindowEx(hShellDefView,NULL,L"SysListView32",NULL);
}
*/

CreateDCWStub CreateDCWOld = 0;

HDC   WINAPI CreateDCWNew(_In_opt_ LPCWSTR pwszDriver, _In_opt_ LPCWSTR pwszDevice, _In_opt_ LPCWSTR pszPort, _In_opt_ CONST DEVMODEW* pdm) {
	HDC hdc = CreateDCWOld(pwszDriver, pwszDevice, pszPort, pdm);
	//log(L"CreateDCWNew entry");
	if (hdc)
	{
		if (lstrcmpiW(pwszDriver, L"DISPLAY") == 0)
		{
			int result = SbieApi_QueryScreenshot();
			if (result == BASIC_DISABLE_STATE)
			{
				TlsSetValue(g_tlsIdx, hdc);

				//log(L"process %d call CreateDCW\r\n", GetCurrentProcessId());
			}
		}
	}
	//log(L"CreateDCWNew leave");
	return hdc;
}

CreateDCAStub CreateDCAOld;

HDC WINAPI CreateDCANew( LPCSTR pszDriver,  LPCSTR pszDevice,  LPCSTR pszPort,  CONST DEVMODE * pd) {
	//log(L"CreateDCANew entry");
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
			//log(L"process %d call CreateDCA\r\n", GetCurrentProcessId());
		}
	}
	//log(L"CreateDCANew leave");
	return hdc;
}



ptrGetWindowDC GetWindowDCOld = 0;

HDC __stdcall GetWindowDCNew(HWND hwnd) {
	//log(L"GetWindowDCNew entry");
	HDC hdc = GetWindowDCOld(hwnd);

	HWND hdesktop = GetDesktopWindow();
	if (hdesktop == hwnd)
	{
		int result = SbieApi_QueryScreenshot();
		if (result == BASIC_DISABLE_STATE)
		{
			//log(L"process %d get desktop window hdc\r\n", GetCurrentProcessId());
			TlsSetValue(g_tlsIdx, hdc);
		}
	}
	//log(L"GetWindowDCNew leave");
	return hdc;
}



int isVeracryptDisk(WCHAR* diskname) {
	int result = 0;
	WCHAR devname[MAX_PATH];
	result = QueryDosDeviceW(diskname, devname, MAX_PATH);
	if (result)
	{
		if (wcsstr(devname, L"\\Device\\VeraCryptVolume"))
		{
			//log(L"find veracrypt volume:%ws,pid:%d\r\n", devname,GetCurrentProcessId());
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


ptrGetLogicalDrives GetLogicalDrivesOld = 0;


DWORD __stdcall GetLogicalDrivesNew() {
	int result = 0;
	DWORD drives = GetLogicalDrivesOld();

	HMODULE hSfDeskDll = GetModuleHandle(L"SfDeskDll.dll");

	//log(L"GetLogicalDrives:%u\r\n", drives);

	//__debugbreak();

	DWORD mask = 0xffffffff;

	WCHAR diskname[4];
	diskname[1] = L':';
	diskname[2] = 0;		//without end of slash 

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

	//__debugbreak();
	//log(L" FindFirstVolumeW:%ws\r\n", lpszVolumeName);

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

// \\\\?\\Volume{57e16035-db67-40af-87d8-ddf19d099360}
BOOL __stdcall FindNextVolumeWNew(HANDLE hFindVolume, LPWSTR lpszVolumeName, DWORD  cchBufferLength) {

	int lastresult = 0;
	if (hFindVolume && hFindVolume != INVALID_HANDLE_VALUE)
	{
		lastresult = FindNextVolumeWOld(hFindVolume, lpszVolumeName, cchBufferLength);

		//log(L" FindNextVolumeW:%ws\r\n", lpszVolumeName);

		//__debugbreak();
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

DWORD __stdcall GetLogicalDriveStringsWNew(DWORD  nBufferLength, LPWSTR lpBuffer) {
	DWORD len = GetLogicalDriveStringsWOld(nBufferLength, lpBuffer);

	int itemsize =  4;

	DWORD offset = 0;
	while(offset < len)
	{
		//must not endwith \ 
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

	log(L"D3DXSaveSurfaceToFileANew entry");

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

	log(L"D3DXSaveSurfaceToFileANew entry");

	int result = D3DXSaveSurfaceToFileWOld(pDestFile, DestFormat, pSrcSurface, pSrcPalette, pSrcRect);

	return result;
}


BOOLEAN g_msgboxPrompt = 0;


P_OpenPrinter2W                 __sys_OpenPrinter2W = NULL;

P_OpenPrinterW                  __sys_OpenPrinterW = 0;

P_OpenPrinter2A                 __sys_OpenPrinter2A = NULL;

P_OpenPrinterA                  __sys_OpenPrinterA = 0;








BOOL newMessageBoxA(HWND    hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT    uType) {

	int result = 0;

	if (g_msgboxPrompt == 0)
	{
		g_msgboxPrompt = TRUE;
		result = MessageBoxA(hWnd, lpText, lpCaption, uType);
		//g_msgboxPrompt = FALSE;
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

	//log(L"lpLoadLibraryWNew enter");

	HMODULE hmodule = lpLoadLibraryWOld(lpLibFileName);

	//log(L"lpLoadLibraryWNew doing");

	if (hmodule)
	{
		if (lstrcmpiW(lpLibFileName, L"winspool.drv") == 0)
		{
			result += hook(L"winspool.drv", L"OpenPrinterA", (LPBYTE)OpenPrinterANew, (FARPROC*)&__sys_OpenPrinterA);
			result += hook(L"winspool.drv", L"OpenPrinterW", (LPBYTE)OpenPrinterWNew, (FARPROC*)&__sys_OpenPrinterW);
			//result = hook(L"winspool.drv", L"OpenPrinter2A", (LPBYTE)OpenPrinter2ANew, (FARPROC*)&__sys_OpenPrinter2A);	//SetLastError 0x32
			result += hook(L"winspool.drv", L"OpenPrinter2W", (LPBYTE)OpenPrinter2WNew, (FARPROC*)&__sys_OpenPrinter2W);

			log(L"winspool.drv loading and hook result:%d", result);
		}
	}

	//log(L"lpLoadLibraryWNew leave");
	return hmodule;
}


