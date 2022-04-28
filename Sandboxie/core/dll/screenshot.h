#pragma once

#include <windows.h>


typedef int (WINAPI* P_GetDIBits)(_In_ HDC hdc, _In_ HBITMAP hbm, _In_ UINT start, _In_ UINT cLines,
	_Out_opt_ LPVOID lpvBits, _At_((LPBITMAPINFOHEADER)lpbmi, _Inout_) LPBITMAPINFO lpbmi, _In_ UINT usage);

typedef BOOL(WINAPI* P_BitBlt)(HDC   hdc, int   x, int   y, int   cx, int   cy, HDC   hdcSrc, int   x1, int   y1, DWORD rop);

int WINAPI Gdi_GetDIBits(_In_ HDC hdc, _In_ HBITMAP hbm, _In_ UINT start, _In_ UINT cLines, LPVOID lpvBits,
	_At_((LPBITMAPINFOHEADER)lpbmi, _Inout_) LPBITMAPINFO lpbmi, _In_ UINT usage);

BOOL Gdi_BitBlt(HDC   hdc, int   x, int   y, int   cx, int   cy, HDC   hdcSrc, int   x1, int   y1, DWORD rop);
