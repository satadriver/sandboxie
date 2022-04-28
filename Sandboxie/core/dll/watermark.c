
#include "dll.h"
#include <windows.h>


typedef BOOL (WINAPI * ptrScreenToClient)(HWND hWnd,LPPOINT lpPoint);
typedef BOOL(WINAPI* ptrRedrawWindow)(HWND hWnd, const RECT* lprcUpdate, HRGN hrgnUpdate, UINT flags);
typedef BOOL(__stdcall* ptrGetWindowRect)(_In_ HWND hWnd, _Out_ LPRECT lpRect);

typedef BOOL (__stdcall *ptrInvalidateRect)(_In_opt_ HWND hWnd, _In_opt_ CONST RECT* lpRect, _In_ BOOL bErase);

typedef LONG(WINAPI* ptrSetWindowLongW)(_In_ HWND hWnd, _In_ int nIndex, _In_ LONG dwNewLong);

typedef LONG(WINAPI* ptrGetWindowLongW)(_In_ HWND hWnd, _In_ int nIndex);

typedef  BOOL(WINAPI* ptrUpdateLayeredWindow)(
	_In_ HWND hWnd,
	_In_opt_ HDC hdcDst,
	_In_opt_ POINT* pptDst,
	_In_opt_ SIZE* psize,
	_In_opt_ HDC hdcSrc,
	_In_opt_ POINT* pptSrc,
	_In_ COLORREF crKey,
	_In_opt_ BLENDFUNCTION* pblend,
	_In_ DWORD dwFlags);

typedef int (WINAPI* ptrGetSystemMetrics)(_In_ int nIndex);
typedef int (WINAPI* ptrGetDIBits)(_In_ HDC hdc, _In_ HBITMAP hbm, _In_ UINT start, _In_ UINT cLines,
	_Out_opt_ LPVOID lpvBits, LPBITMAPINFO lpbmi, _In_ UINT usage);
typedef int(WINAPI* ptrSetDIBits)(_In_opt_ HDC hdc, _In_ HBITMAP hbm, _In_ UINT start, _In_ UINT cLines, _In_ CONST VOID* lpBits,
	_In_ CONST BITMAPINFO* lpbmi, _In_ UINT ColorUse);

typedef BOOL(WINAPI* ptrClientToScreen)(_In_ HWND hWnd, _Inout_ LPPOINT lpPoint);
typedef HMODULE(WINAPI* ptrLoadLibraryA)(_In_ LPCSTR lpLibFileName);

typedef BOOL(WINAPI* ptrAlphaBlend)(
	_In_ HDC hdcDest,
	_In_ int xoriginDest,
	_In_ int yoriginDest,
	_In_ int wDest,
	_In_ int hDest,
	_In_ HDC hdcSrc,
	_In_ int xoriginSrc,
	_In_ int yoriginSrc,
	_In_ int wSrc,
	_In_ int hSrc,
	_In_ BLENDFUNCTION ftn);

typedef BOOL(WINAPI* ptrTransparentBlt)(
	_In_ HDC hdcDest,
	_In_ int xoriginDest,
	_In_ int yoriginDest,
	_In_ int wDest,
	_In_ int hDest,
	_In_ HDC hdcSrc,
	_In_ int xoriginSrc,
	_In_ int yoriginSrc,
	_In_ int wSrc,
	_In_ int hSrc,
	_In_ UINT crTransparent);

typedef HDC(WINAPI* ptrGetDC)(_In_opt_ HWND hWnd);

typedef BOOL(WINAPI* ptrStretchBlt)(_In_ HDC hdcDest, _In_ int xDest, _In_ int yDest, _In_ int wDest, _In_ int hDest,
	_In_opt_ HDC hdcSrc, _In_ int xSrc, _In_ int ySrc, _In_ int wSrc, _In_ int hSrc, _In_ DWORD rop);
typedef BOOL(WINAPI* ptrBitBlt)(_In_ HDC hdc, _In_ int x, _In_ int y, _In_ int cx, _In_ int cy, _In_opt_ HDC hdcSrc,
	_In_ int x1, _In_ int y1, _In_ DWORD rop);
typedef  BOOL(WINAPI* ptrDeleteObject)(_In_ HGDIOBJ ho);
typedef  int (WINAPI* ptrFillRect)(_In_ HDC hDC, _In_ CONST RECT* lprc, _In_ HBRUSH hbr);
typedef BOOL(WINAPI* ptrDeleteDC)(_In_ HDC hdc);
typedef  HBITMAP(WINAPI* ptrCreateCompatibleBitmap)(_In_ HDC hdc, _In_ int cx, _In_ int cy);

typedef  HDC(WINAPI* ptrCreateCompatibleDC)(_In_opt_ HDC hdc);

typedef void (WINAPI* ptrOutputDebugStringA)(char* msg);

typedef WINGDIAPI int (WINAPI* ptrGetDeviceCaps)(_In_opt_ HDC hdc, _In_ int index);

typedef COLORREF(WINAPI* ptrSetBkColor)(_In_ HDC hdc, _In_ COLORREF color);

typedef int(WINAPI* ptrMulDiv)(_In_ int nNumber, _In_ int nNumerator, _In_ int nDenominator);

typedef BOOL(WINAPI* ptrGetUpdateRect)(_In_ HWND hWnd, _Out_opt_ LPRECT lpRect, _In_ BOOL bErase);

typedef HDC(WINAPI* ptrBeginPaint)(HWND          hWnd, LPPAINTSTRUCT lpPaint);

typedef HDC(WINAPI* ptrEndPaint)(HWND          hWnd, LPPAINTSTRUCT lpPaint);

typedef HDC(WINAPI* ptrGetWindowDC)(HWND          hWnd);

typedef BOOL(WINAPI* ptrReleaseDC)(HWND          hWnd, HDC hdc);

typedef BOOL(WINAPI* ptrGetClientRect)(HWND          hWnd, LPRECT lprect);

typedef BOOL(WINAPI* ptrDrawTextA)(HDC          hWnd, char* lpPaint, int len, LPRECT lprect, int mode);

typedef HFONT(WINAPI* ptrCreateFontA)(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight,
	DWORD fdwltalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision,
	DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCSTR lpszFace);

typedef HGDIOBJ(WINAPI* ptrSelectObject)(HDC     hdc, HGDIOBJ h);
typedef BOOL(WINAPI* ptrTextOutA)(HDC    hdc, int    x, int    y, LPCSTR lpString, int    c);
typedef int (WINAPI* ptrSetBkMode)(HDC, int);
typedef BOOL(WINAPI* ptrGetUserNameA)(LPSTR   lpBuffer, LPDWORD pcbBuffer);
typedef BOOL(WINAPIV* ptrwsprintfA)(LPSTR   lpBuffer, const char* format, ...);
typedef BOOL(WINAPIV* ptrwsprintfW)(LPWSTR   lpBuffer, const wchar_t* format, ...);
typedef BOOL(WINAPI* ptrInvalidateRect)(_In_opt_ HWND hWnd, _In_opt_ CONST RECT* lpRect, _In_ BOOL bErase);
typedef int (WINAPI* ptrGetObjectA)(_In_ HANDLE h, _In_ int c, _Out_writes_bytes_opt_(c) LPVOID pv);
typedef HGDIOBJ(WINAPI* ptrGetStockObject)(_In_ int i);
typedef  HFONT(WINAPI* ptrCreateFontIndirectA)(_In_ CONST LOGFONTA* lplf);
typedef INT(WINAPI* ptrSetTextColor)(HDC, INT);


ptrScreenToClient lpScreenToClient = 0;
ptrClientToScreen lpClientToScreen = 0;
ptrRedrawWindow lpRedrawWindow = 0;
ptrGetWindowRect lpGetWindowRect = 0;
ptrGetDIBits lpGetDIBits = 0;
ptrSetDIBits lpSetDIBits = 0;
ptrGetDC lpGetDC = 0;
ptrStretchBlt lpStretchBlt = 0;
ptrBitBlt lpBitBlt = 0;
ptrFillRect lpFillRect = 0;
ptrDeleteDC lpDeleteDC = 0;
ptrCreateCompatibleBitmap lpCreateCompatibleBitmap = 0;
ptrCreateCompatibleDC lpCreateCompatibleDC = 0;
ptrOutputDebugStringA lpOutputDebugStringA = 0;
ptrSetBkColor lpSetBkColor = 0;
ptrGetDeviceCaps lpGetDeviceCaps = 0;
ptrBeginPaint lpBeginPaint = 0;
ptrMulDiv lpMulDiv = 0;
ptrEndPaint lpEndPaint = 0;
ptrGetWindowDC lpGetWindowDC = 0;
ptrGetClientRect lpGetClientRect = 0;
ptrDrawTextA lpDrawTextA = 0;
ptrCreateFontA lpCreateFontA = 0;
ptrSelectObject lpSelectObject = 0;
ptrDeleteObject lpDeleteObject = 0;
ptrTextOutA lpTextOutA = 0;
ptrGetUserNameA lpGetUserNameA = 0;
ptrReleaseDC lpReleaseDC = 0;
ptrSetBkMode lpSetBkMode = 0;
ptrwsprintfA lpwsprintfA = 0;
ptrwsprintfW lpwsprintfW = 0;
ptrGetUpdateRect lpGetUpdateRect = 0;
ptrGetObjectA lpGetObjectA = 0;
ptrGetStockObject lpGetStockObject = 0;
ptrCreateFontIndirectA lpCreateFontIndirectA = 0;
ptrSetTextColor lpSetTextColor = 0;
ptrInvalidateRect lpInvalidateRect = 0;
ptrTransparentBlt lpTransparentBlt = 0;
ptrLoadLibraryA lpLoadLibraryA = 0;

ptrAlphaBlend lpAlphaBlend = 0;
ptrGetSystemMetrics lpGetSystemMetrics = 0;
ptrUpdateLayeredWindow lpUpdateLayeredWindow = 0;
ptrSetWindowLongW lpSetWindowLongW = 0;
ptrGetWindowLongW lpGetWindowLongW = 0;


HFONT g_hfont = 0;

//LOGFONTA g_logfont;

DWORD g_watermarkToggle = FALSE;

BOOLEAN g_initFlag = FALSE;

char g_watermark_username[32];



//#define BEGINPAINT
#define SOURCE_HDC_ALPHA			224
#define BITS_PER_PIXEL				32
#define WATERMARK_FONT_COLOR		0X7f7f7f
#define WATERMARK_ANGLE				30
#define WATERMARK_FONT_HEIGHT		40
#define WATERMARK_FONT_WIDTH		WATERMARK_FONT_HEIGHT/2



HFONT init_logfont(LOGFONTA* logfont)
{
	int result = 0;
	HGDIOBJ hgdi = lpGetStockObject(SYSTEM_FONT);
	lpGetObjectA(hgdi, sizeof(LOGFONTA), logfont);
	logfont->lfItalic = TRUE;
	logfont->lfHeight = WATERMARK_FONT_HEIGHT;
	logfont->lfWidth = WATERMARK_FONT_WIDTH;
	logfont->lfWeight = FW_REGULAR;
	logfont->lfCharSet = GB2312_CHARSET;
	logfont->lfEscapement = WATERMARK_ANGLE * 10;
	lpwsprintfA(logfont->lfFaceName, ("%s"), ("Arial"));
	HFONT hfont = lpCreateFontIndirectA(logfont);
	return hfont;
}


int initWatermark(HWND hwnd) {

	int result = 0;

	HMODULE huser32 = GetModuleHandleA("user32.dll");
	if (huser32) {
		lpBeginPaint = (ptrBeginPaint)GetProcAddress(huser32, "BeginPaint");
		lpEndPaint = (ptrEndPaint)GetProcAddress(huser32, "EndPaint");
		lpGetWindowDC = (ptrGetWindowDC)GetProcAddress(huser32, "GetWindowDC");
		lpGetClientRect = (ptrGetClientRect)GetProcAddress(huser32, "GetClientRect");
		lpGetWindowRect = (ptrGetWindowRect)GetProcAddress(huser32, "GetWindowRect");
		lpDrawTextA = (ptrDrawTextA)GetProcAddress(huser32, "DrawTextA");
		lpwsprintfA = (ptrwsprintfA)GetProcAddress(huser32, "wsprintfA");
		lpReleaseDC = (ptrReleaseDC)GetProcAddress(huser32, "ReleaseDC");
		lpInvalidateRect = (ptrInvalidateRect)GetProcAddress(huser32, "InvalidateRect");
		lpGetUpdateRect = (ptrGetUpdateRect)GetProcAddress(huser32, "GetUpdateRect");
		lpFillRect = (ptrFillRect)GetProcAddress(huser32, "FillRect");
		lpGetDC = (ptrGetDC)GetProcAddress(huser32, "GetDC");
		lpClientToScreen = (ptrClientToScreen)GetProcAddress(huser32, "ClientToScreen");
		lpGetSystemMetrics = (ptrGetSystemMetrics)GetProcAddress(huser32, "GetSystemMetrics");
		lpUpdateLayeredWindow = (ptrUpdateLayeredWindow)GetProcAddress(huser32, "UpdateLayeredWindow");
		lpSetWindowLongW = (ptrSetWindowLongW)GetProcAddress(huser32, "SetWindowLongW");
		lpGetWindowLongW = (ptrGetWindowLongW)GetProcAddress(huser32, "GetWindowLongW");
		lpRedrawWindow = (ptrRedrawWindow)GetProcAddress(huser32, "RedrawWindow");

		lpScreenToClient = (ptrScreenToClient)GetProcAddress(huser32, "ScreenToClient");
	}
	else {
		OutputDebugStringA("GetModuleHandleA user32 failed\r\n");
	}

	HMODULE hgdi32 = GetModuleHandle(L"Gdi32.dll");
	if (hgdi32)
	{
		lpCreateFontA = (ptrCreateFontA)GetProcAddress(hgdi32, "CreateFontA");
		lpDeleteObject = (ptrDeleteObject)GetProcAddress(hgdi32, "DeleteObject");
		lpSelectObject = (ptrSelectObject)GetProcAddress(hgdi32, "SelectObject");
		lpTextOutA = (ptrTextOutA)GetProcAddress(hgdi32, "TextOutA");
		lpSetBkMode = (ptrSetBkMode)GetProcAddress(hgdi32, "SetBkMode");
		lpSetTextColor = (ptrSetTextColor)GetProcAddress(hgdi32, "SetTextColor");
		lpGetStockObject = (ptrGetStockObject)GetProcAddress(hgdi32, "GetStockObject");
		lpGetObjectA = (ptrGetObjectA)GetProcAddress(hgdi32, "GetObjectA");
		lpCreateFontIndirectA = (ptrCreateFontIndirectA)GetProcAddress(hgdi32, "CreateFontIndirectA");
		lpSetBkColor = (ptrSetBkColor)GetProcAddress(hgdi32, "SetBkColor");
		lpCreateCompatibleBitmap = (ptrCreateCompatibleBitmap)GetProcAddress(hgdi32, "CreateCompatibleBitmap");
		lpCreateCompatibleDC = (ptrCreateCompatibleDC)GetProcAddress(hgdi32, "CreateCompatibleDC");
		lpDeleteDC = (ptrDeleteDC)GetProcAddress(hgdi32, "DeleteDC");
		lpBitBlt = (ptrBitBlt)GetProcAddress(hgdi32, "BitBlt");
		lpStretchBlt = (ptrStretchBlt)GetProcAddress(hgdi32, "StretchBlt");
		lpGetDIBits = (ptrGetDIBits)GetProcAddress(hgdi32, "GetDIBits");
		lpSetDIBits = (ptrSetDIBits)GetProcAddress(hgdi32, "SetDIBits");
	}
	else {
		OutputDebugStringA("GetModuleHandleA Gdi32 failed\r\n");
	}


	HMODULE hadv = LoadLibraryA("Advapi32");
	if (hadv)
	{
		lpGetUserNameA = (ptrGetUserNameA)GetProcAddress(hadv, "GetUserNameA");
	}
	else {
		OutputDebugStringA("GetModuleHandleA Advapi32 failed\r\n");
	}


	HMODULE hkernel = GetModuleHandleA("kernel32");
	if (hkernel)
	{
		lpMulDiv = (ptrMulDiv)GetProcAddress(hkernel, "MulDiv");
		lpOutputDebugStringA = (ptrOutputDebugStringA)GetProcAddress(hkernel, "OutputDebugStringA");
	}
	else {
		OutputDebugStringA("GetModuleHandleA kernel32 failed\r\n");
	}


	HMODULE hmsimg32 = LoadLibraryA("msimg32.dll");
	//HMODULE hmsimg32 = GetModuleHandleA("msimg32.dll");
	if (hmsimg32)
	{
		lpTransparentBlt = (ptrTransparentBlt)GetProcAddress(hmsimg32, "TransparentBlt");
		lpAlphaBlend = (ptrAlphaBlend)GetProcAddress(hmsimg32, "AlphaBlend");
	}
	else {
		OutputDebugStringA("GetModuleHandleA msimg32 failed\r\n");
	}


	g_hfont = lpCreateFontA(WATERMARK_FONT_HEIGHT, WATERMARK_FONT_WIDTH, WATERMARK_ANGLE * 10, WATERMARK_ANGLE * 10, 0, TRUE, 0, 0,
		DEFAULT_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, VARIABLE_PITCH | PROOF_QUALITY, FF_DONTCARE, (char*)"Arial");
	// 	g_hfont = init_logfont(&g_logfont);
	// 	g_logfont.lfWidth = WATERMARK_FONT_WIDTH;
	// 	g_logfont.lfHeight = WATERMARK_FONT_HEIGHT;

	DWORD size = sizeof(g_watermark_username);
	int usernamelen = lpGetUserNameA(g_watermark_username, &size);
	g_watermark_username[size] = 0;

	// 	LONG lWindowLong = lpGetWindowLongW(hWnd, GWL_EXSTYLE);
	// 	if ((lWindowLong & WS_EX_LAYERED) != WS_EX_LAYERED)
	// 	{
	// 		lWindowLong = lWindowLong | WS_EX_LAYERED;
	// 		result = lpSetWindowLongW(hWnd, GWL_EXSTYLE, lWindowLong);
	// 	}

	OutputDebugStringA("initWatermark ok\r\n");

	return TRUE;
}








int blenddata(HDC hdc_src, int client_width, int client_height, HDC hdc_new, HBITMAP hbitmap_new) {

	int result = 0;
	HDC hdc_mem = lpCreateCompatibleDC(hdc_src);
	HBITMAP hbitmap_src = lpCreateCompatibleBitmap(hdc_src, client_width, client_height);
	HGDIOBJ oldobj = lpSelectObject(hdc_mem, hbitmap_src);
	result = lpBitBlt(hdc_mem, 0, 0, client_width, client_height, hdc_src, 0, 0, SRCCOPY);

	BITMAPINFO  bmpinfo = { 0 };
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biWidth = client_width;
	bmpinfo.bmiHeader.biHeight = client_height;
	bmpinfo.bmiHeader.biBitCount = BITS_PER_PIXEL;
	bmpinfo.bmiHeader.biCompression = BI_RGB;
	int scanline = client_width;
	int mod = scanline % 4;
	if (mod)
	{
		scanline = scanline + (4 - mod);
	}

	int imagesize = scanline * client_height * (BITS_PER_PIXEL >> 3);
	bmpinfo.bmiHeader.biSizeImage = imagesize;

	DWORD* g_buf1 = 0;
	DWORD* g_buf2 = 0;
	g_buf1 = (DWORD*)VirtualAlloc(0, imagesize, MEM_COMMIT, PAGE_READWRITE);
	g_buf2 = (DWORD*)VirtualAlloc(0, imagesize, MEM_COMMIT, PAGE_READWRITE);

	//The pages of a process's virtual address space can be in one of the following states.
	//Free
	//Committed
	//Reserved
	//MEM_DECOMMIT:Decommits the specified region of committed pages. After the operation, the pages are in the reserved state.
	//MEM_RELEASE:If you specify this value, dwSize must be 0 (zero),
	//and lpAddress must point to the base address returned by the VirtualAlloc function when the region is reserved. 

	result = lpGetDIBits(hdc_mem, (HBITMAP)hbitmap_src, 0, client_height, g_buf1, &bmpinfo, DIB_RGB_COLORS);
	if (result == 0)
	{
		result = GetLastError();
	}
	result = lpGetDIBits(hdc_new, hbitmap_new, 0, client_height, g_buf2, &bmpinfo, DIB_RGB_COLORS);
	if (result == 0)
	{
		result = GetLastError();
	}
	int cnt = 0;
	int cnt2 = 0;
	for (int i = 0; i < scanline * client_height; i++)
	{
		if (g_buf1[i] != 0)
		{
			cnt++;
			g_buf2[i] = g_buf1[i] * 200 + g_buf2[i] * 55;
		}
		else {
			cnt2++;
			//g_buf2[i] = g_buf1[i];
		}
	}

	result = lpSetDIBits(hdc_new, hbitmap_new, 0, client_height, g_buf2, &bmpinfo, DIB_RGB_COLORS);
	result = lpBitBlt(hdc_src, 0, 0, client_width, client_height, hdc_new, 0, 0, SRCCOPY);

	result = VirtualFree(g_buf1, 0, MEM_RELEASE);
	result = VirtualFree(g_buf2, 0, MEM_RELEASE);

	lpDeleteObject(hbitmap_src);

	lpDeleteDC(hdc_mem);
	return 0;
}



int restoreBackground(HWND hwnd) {
	int result = 0;
	if (g_initFlag == FALSE)
	{
		result = initWatermark(hwnd);
	}

	THREAD_DATA* TlsData = Dll_GetTlsData(NULL);
	if (TlsData->gui_hdcBackground && TlsData->gui_hbmpBackground && TlsData->gui_mainWindowWidth && TlsData->gui_mainWindowHeight)
	{
		//get bitmap structure info,the size of first initialized HBITMAP is 1 pixel width，1 pixel height?NO
// 		BITMAP bitmap;
// 		result = lpGetObjectA(TlsData->gui_hbmpBackground, sizeof(BITMAP), (LPSTR)&bitmap);

		HDC hdc = lpGetDC(hwnd);

		POINT p = { TlsData->gui_mainWindowLeft , TlsData->gui_mainWindowTop };
		
		result = lpScreenToClient(hwnd, &p);
		result = lpBitBlt(hdc, p.x, p.y,TlsData->gui_mainWindowWidth, TlsData->gui_mainWindowHeight,
			TlsData->gui_hdcBackground, TlsData->gui_mainWindowLeft, TlsData->gui_mainWindowTop, SRCCOPY);
		result = lpReleaseDC(hwnd, hdc);
		return TRUE;
	}

	return FALSE;
}




int keepBackground(HWND hwnd,int update_left,int update_top,int update_width,int update_height) {
	int result = 0;
	if (g_initFlag == 0)
	{
		result = initWatermark(hwnd);
	}

	HDC hdc = lpGetDC(hwnd);

	THREAD_DATA* TlsData = Dll_GetTlsData(NULL);
	if (TlsData->gui_hdcBackground == 0 || TlsData->gui_hbmpBackground == 0)
	{
		int cx = lpGetSystemMetrics(SM_CXSCREEN);
		int cy = lpGetSystemMetrics(SM_CYSCREEN);
		TlsData->gui_hdcBackground = lpCreateCompatibleDC(hdc);
		TlsData->gui_hbmpBackground = lpCreateCompatibleBitmap(hdc, cx, cy);
		lpSelectObject(TlsData->gui_hdcBackground, TlsData->gui_hbmpBackground);
	}

	POINT p = { update_left,update_top };
	result = lpScreenToClient(hwnd, &p);
	result = lpBitBlt(TlsData->gui_hdcBackground, update_left, update_top, update_width, update_height, hdc, p.x, p.y, SRCCOPY);
	result = lpReleaseDC(hwnd, hdc);

	TlsData->gui_mainWindowLeft = update_left;
	TlsData->gui_mainWindowTop = update_top;
	TlsData->gui_mainWindowWidth = update_width;
	TlsData->gui_mainWindowHeight = update_height;

	return result;
}


int GetUpdateRectFunction(HWND hwnd,RECT* lprect,BOOL bErase) {
	int result = 0;
	if (g_initFlag == 0)
	{
		result = initWatermark(hwnd);
	}

// 	RECT winrect;
// 	result = lpGetWindowRect(hwnd, &winrect);

	result = lpGetUpdateRect(hwnd, lprect, bErase);
	if (result)
	{
		POINT p = { 0,0 };
		result = lpClientToScreen(hwnd, &p);

		lprect->left = lprect->left + p.x;
		lprect->top = lprect->top + p.y;
		lprect->bottom = lprect->bottom + p.y;
		lprect->right = lprect->right + p.x;
	}

	return result;
}


int InvalidRectFunction(HWND hwnd) {
	int result = 0;

	if (g_initFlag == 0)
	{
		result = initWatermark(hwnd);
	}
	
	RECT rect;
	result = lpGetClientRect(hwnd, &rect);

	result = lpInvalidateRect(hwnd, &rect, TRUE);
	return result;
}

//To repaint the desktop, an application uses the RDW_ERASE flag to generate a WM_ERASEBKGND message.
//RDW_ERASE:Causes the window to receive a WM_ERASEBKGND message when the window is repainted.
//The RDW_INVALIDATE flag must also be specified; otherwise, RDW_ERASE has no effect.
int redrawWindowFunction(HWND hwnd) {
	int result = 0;
	if (g_initFlag == 0)
	{
		result = initWatermark(hwnd);
	}

	result = lpRedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	return result;
}


int GetWindowLongFunction(HWND hwnd, int style) {
	return lpGetWindowLongW(hwnd, style);
}

int GetClientRectFunction(HWND hwnd, RECT* lprect) {
	return lpGetClientRect(hwnd, lprect);
}


int watermark(HWND hwnd, int update_left, int update_top, int update_width, int update_height) {
	int result = 0;

	if (g_initFlag == 0)
	{
		result = initWatermark(hwnd);
	}

	SYSTEMTIME t;
	GetLocalTime(&t);
	char info[256];
	int msglen = lpwsprintfA(info, "%04d/%02d/%02d %s", t.wYear, t.wMonth, t.wDay, g_watermark_username);
	
	RECT client_rect;
	result = lpGetClientRect(hwnd, &client_rect);
	int client_width = client_rect.right - client_rect.left;
	int client_height = client_rect.bottom - client_rect.top;

	POINT client_pos = {0,0};
	result = lpClientToScreen(hwnd, &client_pos);


	HDC hdc_src = lpGetDC(hwnd);
	HDC hdc_new = lpCreateCompatibleDC(hdc_src);

	int screen_width = lpGetSystemMetrics(SM_CXSCREEN);
	int screen_height = lpGetSystemMetrics(SM_CYSCREEN);

	HBITMAP hbitmap_new = lpCreateCompatibleBitmap(hdc_src, screen_width, screen_height);
	HBITMAP hbmp_new_old = (HBITMAP)lpSelectObject(hdc_new, hbitmap_new);

	result = lpBitBlt(hdc_new, client_pos.x, client_pos.y, client_width, client_height, hdc_src, 0, 0, SRCCOPY);

	//what is the function SetBkColor purpose?
	COLORREF color = 0;
	lpSetBkColor(hdc_new, color);

	lpSelectObject(hdc_new, g_hfont);
	lpSetBkMode(hdc_new, TRANSPARENT);
	//the COLORREF value has the following hexadecimal form:0x00bbggrr
	lpSetTextColor(hdc_new, WATERMARK_FONT_COLOR);

	// 	HBRUSH brush = (HBRUSH)lpGetStockObject(COLOR_WINDOW + 1); //ok
	// 	//brush = CreateSolidBrush(color);	//error
	// 	result = lpFillRect(hdc_new, &rect, brush);

	for (int y = WATERMARK_FONT_HEIGHT * 4; y < screen_height; y += (WATERMARK_FONT_HEIGHT * 4))
	{
		for (int x = WATERMARK_FONT_WIDTH; x < screen_width; x += (WATERMARK_FONT_WIDTH * msglen + WATERMARK_FONT_WIDTH * 8))
		{
			lpTextOutA(hdc_new, x, y, info, msglen);
		}
	}

	//result = blenddata(hdc_src, client_width, client_height, hdc_new, hbitmap_new);

	//result = lpTransparentBlt(hdc_new, 0, 0, client_width, client_height, hdc_mem, 0, 0, client_width, client_height, WATERMARK_FONT_COLOR);

	BLENDFUNCTION bf = { 0 };
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = 0;
	bf.SourceConstantAlpha = SOURCE_HDC_ALPHA;
	THREAD_DATA* TlsData = Dll_GetTlsData(NULL);
	if (TlsData->gui_hdcBackground && TlsData->gui_hbmpBackground)
	{
		result = lpAlphaBlend(hdc_new, update_left, update_top, update_width, update_height, TlsData->gui_hdcBackground, 
			update_left, update_top, update_width, update_height, bf);

		POINT screen_pos = { update_left,update_top };
		result = lpScreenToClient(hwnd, &screen_pos);
		result = lpBitBlt(hdc_src, screen_pos.x, screen_pos.y, update_width, update_height, hdc_new, update_left, update_top, SRCCOPY);

		//result = lpUpdateLayeredWindow(hWnd, hdc_src, &p, &s, hdc_new, &p, 0, &bf, ULW_ALPHA);
	}
	else {
		OutputDebugStringA("watermark TlsData gui_hdcBackground null\r\n");
	}

	lpReleaseDC(hwnd, hdc_src);
	lpDeleteObject(hbitmap_new);
	lpDeleteDC(hdc_new);

	//返回值TRUE，这就相当于告诉DefWindowProc，这个消息已经得到了处理，所以不需要系统再进行默认的消息处理了。
	return TRUE;
}



/*
系统内部有一个z-order序列，记录着当前窗口从屏幕底部（从屏幕到眼睛的方向），到屏幕最高层的一个窗口句柄的排序
当任意一个窗口接收到WM_PAINT消息产生重绘，更新区域绘制完成以后，就搜索它的前面的一个窗口，如果此窗口的范围和更新区域有交集，
就向这个窗口发送WM_PAINT消息，周而复始，直到执行到顶层窗口。才算完成。
对于一个对话框（主窗口）来说，理论上其所有子窗口都在他的前面——也就是更靠近眼睛的位置），
当主窗口接收WM_PAINT绘制完成后，会引起更新区域上所有子窗口的重绘（所有子窗口也是自底向上排序的）。
子窗口是具有WS_CHILD或者WS_CHILDWINDOW样式的窗口。

The WM_PAINT message is generated by the system and should not be sent by an application.
The system sends this message when there are no other messages in the application's message queue.

An application should return nonzero inresponse to WM_ERASEBKGND if it processes the message and erases the background;
this indicates that no further erasing is required.
If the application returns zero, the window will remain marked for erasing.
(Typically, this indicates that the fErase member of the PAINTSTRUCT structure will be TRUE.)

系统为什么不在调用Invalidate时发送WM_PAINT消息呢？又为什么非要等应用消息队列为空时才发送WM_PAINT消息呢？
两个WM_PAINT消息之间通过 InvalidateRect和InvaliateRgn使之失效的区域就会被累加起来，然后在一个WM_PAINT消息中一次得到更新，
不仅能避免多次重复地更新同一区域，也优化了应用的更新操作。
像这种通过InvalidateRect和InvalidateRgn来使窗口区域无效，依赖于系统在合适的时机发送WM_PAINT消息的机制实际上是一种异步工作方式，
也就是说，在无效化窗口区域和发送WM_PAINT消息之间是有延迟的；有时候这种延迟并不是我们希望的，
这时我们当然可以在无效化窗口区域后利用SendMessage发送一条WM_PAINT消息来强制立即重画，
但不如使用Windows GDI为我们提供的更方便和强大的函数：UpdateWindow和RedrawWindow。
UpdateWindow会检查窗口的Update Region，当其不为空时才发送WM_PAINT消息；
RedrawWindow则给我们更多的控制：是否重画非客户区和背景，是否总是发送 WM_PAINT消息而不管Update Region是否为空等。

BeginPaint sets the update region of a window to NULL. This clears the region, preventing it from generating subsequent WM_PAINT messages.
If an application processes a WM_PAINT message but does not call BeginPaint or otherwise clear the update region,
the application continues to receive WM_PAINT messages as long as the region is not empty.
In all cases,an application must clear the update region before returning from the WM_PAINT message.

当窗口的Update Region被标志为需要擦除背景时，BeginPaint会发送WM_ERASEBKGND消息来重画背景，同时在其返回信息里有一个标志表明窗口背景是否被重画过。
当我们用InvalidateRect和InvalidateRgn来把指定区域加到Update Region中时，可以设置该区域是否需要被擦除背景，
这样下一个BeginPaint就知道是否需要发送WM_ERASEBKGND消息了。

DefWindowProc会清除背景，可以不执行DefWindowProc在WM_ERASEBKGND消息中重画背景
WM_ERASEBKGND
调用InvalidateRect的时候参数为TRUE，那么会使用窗口的缺省背景画刷刷新背景(一般情况下是白刷)，而造成屏幕闪动，如果参数是FALSE，则不会重刷背景。
随后产生WM_ERASEBKGND消息


The system sends a WM_PAINT message to a window whenever its update region is not empty and there are no other messages in the application queue
for that window.

//WNDCLASSW中hbrBackground为0使得InvalidateRect等函数不会刷新背景

//The bErase parameter specifies whether GetUpdateRect should erase the background of the update region.
//If bErase is TRUE and the update region is not empty, the background is erased.
//To erase the background, GetUpdateRect sends the WM_ERASEBKGND message.

//The update rectangle retrieved by the BeginPaint member function is identical to that retrieved by the GetUpdateRect member function.
//The BeginPaint member function automatically validates the update region,
//so any call to GetUpdateRect made immediately after a call to BeginPaint retrieves an empty update region.
//after BeginPaint GetUpdateRect will get a empty RECT

	//hbrBackground
	//When this member is NULL, an application must paint its own background whenever it is requested to paint in its client area.
	//To determine whether the background must be painted, an application can either process the WM_ERASEBKGND message or
	//test the fErase member of the PAINTSTRUCT structure filled by the BeginPaint function.

	//GetWindowDC用来取得整个窗口的HDC，包括标题栏，菜单栏，状态栏，滚动条和边框(frame)。
	//GetDC返回的HDC可以在整个显示区域进行绘图，但是不会将无效区域变为有效区域。GetDC(0) is equal to CreateDC("DISPLAY")
	//SelectObject:
	//Bitmaps can only be selected into memory DC's. A single bitmap cannot be selected into more than one DC at the same time.
	//This function returns the previously selected object of the specified type.
	// An application should always replace a new object with the original, default object after it has finished drawing with the new object.
*/
