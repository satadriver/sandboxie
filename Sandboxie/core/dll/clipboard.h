#pragma once
#include <windows.h>

//#define CLIPBOARD_DISABLED_CRYPT

#define CLIPBOARD_FORMAT_KEY		0xfedc0000

//#define CLIPBOARD_FORMAT_KEY		0

#define CLIPBOARD_PREFIX_LENGTH		16


UINT crc(const WCHAR* data, int size);

void cryptdata(unsigned char* data, int size, unsigned char* key, int keysize);

HANDLE Gui_SetClipboardData(UINT uFormat, HANDLE handle);

int decryptClipboard(unsigned char* data, int size, int format);

BOOL Gui_PrintWindow(
	HWND hwnd,
	HDC  hdcBlt,
	UINT nFlags
);
