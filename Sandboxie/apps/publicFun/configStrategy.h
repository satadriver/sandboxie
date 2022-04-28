

#pragma once

#include <windows.h>

#define FWP_DIRECTION_OUTBOUND		0
#define FWP_DIRECTION_INBOUND		1

#define DEFAULT_VERACRYPT_SIZE		1

#define WFP_DEVICE_SYMBOL			L"\\\\.\\inspect"
#define JSON_CONFIG_FILENAME		L"sand_box_rule.txt"

//static const WCHAR* g_processBlackList[] = { L"ScreenSketch.exe",L"SnippingTool.exe" };

static const WCHAR* g_processBlackList[] = { L"" };

extern "C" __declspec(dllexport) int setJsonConfig(const WCHAR* filename,char* filedata, int filesize);

extern "C" __declspec(dllexport) int configBox(const WCHAR* wstrusername, const WCHAR* wstrpassword, WCHAR* verapath, WCHAR* strverasize);


