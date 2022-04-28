#pragma once


#ifndef _SHIELDSCREEN_H_
#define _SHIELDSCREEN_H_
#include <windows.h>

extern HMODULE g_dllModule;

#ifdef __cplusplus
extern "C"
{
#endif
	LRESULT CALLBACK LauncherHook(int nCode, WPARAM wParam, LPARAM lParam);
	void stringerror(DWORD errno);
	void savelog(const char* s);

	extern "C" __declspec(dllexport) void WINAPI UnInstallLaunchEv();
	extern "C" __declspec(dllexport) int WINAPI InstallLaunchEv();
#ifdef __cplusplus
};
#endif

#endif