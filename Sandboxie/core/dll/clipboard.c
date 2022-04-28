#pragma warning(disable:4005)

#include <windows.h>
#include "dll.h"
#include "gui_p.h"



typedef int (__stdcall *ptrWideCharToMultiByte)(
	UINT                               CodePage,
	DWORD                              dwFlags,
	_In_NLS_string_(cchWideChar)LPCWCH lpWideCharStr,
	int                                cchWideChar,
	LPSTR                              lpMultiByteStr,
	int                                cbMultiByte,
	LPCCH                              lpDefaultChar,
	LPBOOL                             lpUsedDefaultChar
);

//注册自定义的格式 RegisterClipboardFormat(szFormatName);
//iFormat要介于0xC000～0xFFFF之间。这种格式也可以被EnumClipboardFormats枚举出来。
//要获取该名称，可以调用：GetClipboardFormatName(iFormat,psBuffer,iMaxCount)取得
unsigned int g_clipboardMask = 0;

extern P_PrintWindow                __sys_PrintWindow;




UINT crc(const WCHAR* wstr, int wlen) {
	UINT crc = 0;

	char data[MAX_PATH] = { 0 };
	int size = WideCharToMultiByte(CP_ACP, 0, wstr, wlen, data, MAX_PATH, 0, 0);
	if (size > 0)
	{
		data[size] = 0;
	}
	else {
		char* str = "forgive yourselves";
		size = lstrlenA(str);
		lstrcpyA(data, str);
	}
	

	const DWORD* lpdw = (const DWORD*)data;
	int dwsize = size * sizeof(CHAR) / sizeof(DWORD);
	for (int i = 0; i < dwsize; i++)
	{
		crc += lpdw[i];
	}

	int dwmod = size % sizeof(DWORD);
	if (dwmod)
	{
		DWORD last = 0;
		memcpy(&last, (unsigned char*)&lpdw[dwsize], dwmod);
		crc += last;
	}

	// 	typedef UINT (__stdcall *ptrRegisterClipboardFormatW)(LPCWSTR lpszFormat);
	//     ptrRegisterClipboardFormatW lpRegisterClipboardFormatW = (ptrRegisterClipboardFormatW)
	//         GetProcAddress(LoadLibraryW(L"user32.dll"), "RegisterClipboardFormatW");
	//     crc = lpRegisterClipboardFormatW(Dll_BoxName);

	return crc;
}


void cryptdata(unsigned char* data, int size, unsigned char* key, int keysize) {
	for (int i = 0, j = 0; i < size; i++)
	{
		data[i++] ^= key[j++];
		j = j % keysize;
	}
}



HANDLE Gui_SetClipboardData(UINT uFormat, HANDLE handle) {

	//return __sys_SetClipboardData(uFormat + 0xabcd0000, handle);

	//__debugbreak();

	if (g_clipboardMask == 0)
	{
		g_clipboardMask = crc(Dll_BoxName, lstrlenW(Dll_BoxName));
	}

	HANDLE result = 0;

	if (handle)
	{
		unsigned char* data = (char*)GlobalLock(handle);
		if (data)
		{
			int size = (int)GlobalSize(handle);
			cryptdata(data, size, (unsigned char*)&g_clipboardMask, sizeof(g_clipboardMask));
			GlobalUnlock(handle);
		}
	}

	result = __sys_SetClipboardData(uFormat, (HANDLE)handle);
	if (result == 0)
	{
		//__debugbreak();
	}
	return  result;
}



int decryptClipboard(unsigned char* data,int size, int format) {

	//__debugbreak();
	cryptdata(data, size, (unsigned char*)&g_clipboardMask, sizeof(g_clipboardMask));
	if (format == CF_UNICODETEXT)
	{
		*(short*)(data + size - sizeof(WCHAR)) = 0;
	}
	else if (format == CF_TEXT)
	{
		data[size - sizeof(char)] = 0;
	}

	return TRUE;
}




BOOL Gui_PrintWindow(
	HWND hwnd,
	HDC  hdcBlt,
	UINT nFlags
) {
	int result = 0;

	return result;

	result = __sys_PrintWindow(hwnd, hdcBlt, nFlags);

	return result;
}
