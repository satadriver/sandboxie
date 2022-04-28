

#include "screenshot.h"
#include "sbieapi.h"


typedef  BOOL(WINAPI* ptrDeleteObject)(_In_ HGDIOBJ ho);
typedef BOOL(WINAPI* ptrDeleteDC)(_In_ HDC hdc);
typedef  HBITMAP(WINAPI* ptrCreateCompatibleBitmap)(_In_ HDC hdc, _In_ int cx, _In_ int cy);
typedef  HDC(WINAPI* ptrCreateCompatibleDC)(_In_opt_ HDC hdc);
typedef HGDIOBJ(WINAPI* ptrSelectObject)(HDC hdc, HGDIOBJ h);
typedef int (WINAPI* ptrGetSystemMetrics)(_In_ int nIndex);

extern ptrDeleteDC lpDeleteDC;
extern ptrDeleteObject lpDeleteObject;
extern ptrSelectObject lpSelectObject;
extern ptrCreateCompatibleBitmap lpCreateCompatibleBitmap;
extern ptrCreateCompatibleDC lpCreateCompatibleDC;
extern ptrGetSystemMetrics lpGetSystemMetrics;

extern P_BitBlt                        __sys_BitBlt;
extern P_GetDIBits                     __sys_GetDIBits;

int WINAPI Gdi_GetDIBits(_In_ HDC hdc, _In_ HBITMAP hbm, _In_ UINT start, _In_ UINT cLines, LPVOID lpvBits,
	_At_((LPBITMAPINFOHEADER)lpbmi, _Inout_) LPBITMAPINFO lpbmi, _In_ UINT usage) {

	int result = 0;
	if (lpvBits == 0)
	{
		result = __sys_GetDIBits(hdc, hbm, start, cLines, lpvBits, lpbmi, usage);
		return result;
	}

	int height = 0;
	int width = 0;
	if (lpGetSystemMetrics == 0)
	{
		HMODULE huser32 = GetModuleHandleA("user32.dll");
		if (huser32) {
			lpGetSystemMetrics = (ptrGetSystemMetrics)GetProcAddress(huser32, "GetSystemMetrics");
			if (lpGetSystemMetrics)
			{
				height = lpGetSystemMetrics(SM_CYSCREEN);
				width = lpGetSystemMetrics(SM_CXSCREEN);
			}
			else {
				result = __sys_GetDIBits(hdc, hbm, start, cLines, lpvBits, lpbmi, usage);
				return result;
			}
		}
		else {
			result = __sys_GetDIBits(hdc, hbm, start, cLines, lpvBits, lpbmi, usage);
			return result;
		}
	}
	else {
		height = lpGetSystemMetrics(SM_CYSCREEN);
		width = lpGetSystemMetrics(SM_CXSCREEN);
	}

	int enable = SbieApi_QueryScreenshot();
	if (enable)
	{
		if (start == 0 && cLines == height && lpbmi->bmiHeader.biHeight == height && lpbmi->bmiHeader.biWidth == width)
		{
			int cnt = lpbmi->bmiHeader.biSizeImage / sizeof(int);
			DWORD* color = (DWORD*)lpvBits;
			for (int i = 0; i < cnt; i++)
			{
				color[i] = 0x00000000;
			}
			return cLines;
		}		
	}

	result = __sys_GetDIBits(hdc, hbm, start, cLines, lpvBits, lpbmi, usage);
	return result;
}




BOOL Gdi_BitBlt(HDC   hdc, int   x, int   y, int   cx, int   cy, HDC   hdcSrc, int   x1, int   y1, DWORD rop) {

	int result = 0;

	int height = 0;
	int width = 0;

	if (lpGetSystemMetrics == 0)
	{
		HMODULE huser32 = GetModuleHandleA("user32.dll");
		if (huser32) {
			lpGetSystemMetrics = (ptrGetSystemMetrics)GetProcAddress(huser32, "GetSystemMetrics");
			if (lpGetSystemMetrics)
			{
				height = lpGetSystemMetrics(SM_CYSCREEN);
				width = lpGetSystemMetrics(SM_CXSCREEN);
			}
			else {
				return __sys_BitBlt(hdc, x, y, cx, cy, hdcSrc, x1, y1, rop);
			}
		}
		else {
			return __sys_BitBlt(hdc, x, y, cx, cy, hdcSrc, x1, y1, rop);
		}
	}
	else {
		height = lpGetSystemMetrics(SM_CYSCREEN);
		width = lpGetSystemMetrics(SM_CXSCREEN);
	}

	if (x == 0 && y == 0 && cx == width && cy == height && x1 == 0 && y1 == 0 && rop == SRCCOPY)
	{
		int enable = SbieApi_QueryScreenshot();
		if (enable)
		{
			if (lpCreateCompatibleDC == 0 || lpCreateCompatibleBitmap == 0 || lpSelectObject == 0 || lpDeleteObject == 0 || lpDeleteDC == 0)
			{
				HMODULE hgdi32 = GetModuleHandle(L"Gdi32.dll");
				if (hgdi32)
				{
					lpDeleteObject = (ptrDeleteObject)GetProcAddress(hgdi32, "DeleteObject");
					lpSelectObject = (ptrSelectObject)GetProcAddress(hgdi32, "SelectObject");
					lpCreateCompatibleBitmap = (ptrCreateCompatibleBitmap)GetProcAddress(hgdi32, "CreateCompatibleBitmap");
					lpCreateCompatibleDC = (ptrCreateCompatibleDC)GetProcAddress(hgdi32, "CreateCompatibleDC");
					lpDeleteDC = (ptrDeleteDC)GetProcAddress(hgdi32, "DeleteDC");
				}
			}
			HDC testdc = lpCreateCompatibleDC(hdc);
			HBITMAP testbmp = lpCreateCompatibleBitmap(testdc, width, height);
			lpSelectObject(testdc, testbmp);
			result = __sys_BitBlt(hdc, 0, 0, width, height, testdc, 0, 0, SRCCOPY);
			lpDeleteObject(testbmp);
			lpDeleteDC(testdc);
			return TRUE;
		}
	}

	return __sys_BitBlt(hdc, x, y, cx, cy, hdcSrc, x1, y1, rop);

}
