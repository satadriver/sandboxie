#pragma once

#include <windows.h>

#pragma pack(1)

#pragma pack()

void __stdcall configThread(LPVOID params);

int __stdcall hookProcess(LPVOID params);

int __cdecl mylog(WCHAR* format, ...);

DWORD WINAPI ControlProThreads(DWORD ProcessId, BOOL Suspend);

BOOL createProcessWithToken(const HANDLE& hSrcToken, DWORD srcpid, WCHAR* strPath, WCHAR* lpCmdLine);

int killProcessCord(LPVOID params);
