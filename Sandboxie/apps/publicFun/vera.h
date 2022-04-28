#pragma once

#include <windows.h>

 
#define VERACRYPT_PATH_KEYNAME	L"veracryptPath"
#define VERACRYPT_SIZE_KEYNAME	L"veracryptSize"

extern WCHAR VERACRYPT_PASSWORD[128];

#ifdef __cplusplus
extern "C" {
#endif

	__declspec(dllexport) WCHAR* setDefaultVeraDiskFile();
	__declspec(dllexport) WCHAR * setVeraDiskFile(const WCHAR * path);
	__declspec(dllexport) WCHAR * getVeraDiskFile();

	__declspec(dllexport) int initVeraDisk(int size);

	__declspec(dllexport) DWORD  createBoxVolume(int size);

	__declspec(dllexport) DWORD  mountBoxVolume();

	__declspec(dllexport) int  deleteVeraVolume(const WCHAR * srcfilepath);

	__declspec(dllexport) const wchar_t* getVeraBoxPath();

	__declspec(dllexport) int hidedisk(int toggle);

	__declspec(dllexport) DWORD  DismountBoxVolume();

#ifdef __cplusplus
}
#endif
