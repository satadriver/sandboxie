#pragma once
#include <windows.h>

UINT crc(const WCHAR* data, int size);

void cryptdata(unsigned char* data, int size, unsigned char* key, int keysize);

HANDLE Gui_SetClipboardData(UINT uFormat, HANDLE handle);

int decryptClipboard(unsigned char* data, int size, int format);

BOOL Gui_PrintWindow(
	HWND hwnd,
	HDC  hdcBlt,
	UINT nFlags
);
