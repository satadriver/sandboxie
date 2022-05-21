#include <windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <tchar.h>
#include "dllmain.h"
#include "hookApi.h"
#include "functions.h"
#include "apiFuns.h"
#include "WaterMark.h"
#include "log.h"
#include "d3dx.h"
#include "../processInjector/main.h"

#pragma warning(disable:4005)

HANDLE g_hMutex = 0;
WCHAR ProcName[MAX_PATH] = { 0 };

BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	
    switch (ul_reason_for_call)
    {
		case DLL_PROCESS_ATTACH:
		{
			//DP0("[LYSM] DLL_PROCESS_ATTACH\r\n");

			// 获取进程名
			if (!GetModuleBaseName(GetCurrentProcess(), NULL, ProcName, MAX_PATH))
			{
				DP1("[LYSM] GetModuleBaseName error:%d \n", GetLastError());
			}

			if (g_tlsIdx == 0)
			{
				g_tlsIdx = TlsAlloc();
			}

			if (g_tlsIdx == TLS_OUT_OF_INDEXES)
			{
				log(L"TLS_OUT_OF_INDEXES");
				break;
			}

			WCHAR procname[MAX_PATH];
			int result = 0;
			result = GetModuleFileNameW(0, procname, MAX_PATH);
			WCHAR* pos = wcsrchr(procname, L'\\');

			if (pos)
			{
				pos++;
				WCHAR mutexname[256];
				wsprintfW(mutexname, L"%ws_%u_mutex", pos,GetCurrentProcessId());


				HANDLE hmutex = CreateMutexW(0, TRUE, mutexname);
				if (hmutex)
				{

					result = GetLastError();
					if (result != ERROR_ALREADY_EXISTS)
					{

						g_hMutex = hmutex;

						//memset(g_trump, 0, sizeof(g_trump));

						hookfunctions();
						log(L"dll init with mutex:%ws ok\r\n", mutexname);
					}
					else {

						ReleaseMutex(hmutex);
						CloseHandle(hmutex);
					}
				}
			}

			break;
		}
		case DLL_THREAD_ATTACH:
		{
			break;
		}
		case DLL_THREAD_DETACH:
		{
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			//DP0("[LYSM] DLL_PROCESS_DETACH \n");

			unhookfunctions();

			closeDeviceHandle();

			break;
		}

    }
    return TRUE;
}

extern "C" __declspec(dllexport) void unhookfunctions() {

	BOOL bInSandBox = FALSE;

	// 判断是否在沙箱内
	HMODULE hSfDeskDll = GetModuleHandle(L"SfDeskDll.dll");
	if (hSfDeskDll)
	{
		bInSandBox = TRUE;
	}

	// 解除水印
	if (hSfDeskDll)
	{
		if (!WaterMarkStop())
		{
			DP0("[LYSM][DllMain] WaterMarkStop failed. \n");
		}
	}
	
	if (g_hMutex)
	{
		ReleaseMutex(g_hMutex);
		CloseHandle(g_hMutex);
		g_hMutex = 0;
	}
	unhookall();
}



DWORD isTargetProcess(const INJECT_PROCESS_INFO injectProcess[], WCHAR* fullpepath, int* seq) {

	WCHAR* filename = wcsrchr(fullpepath, L'\\');
	if (filename)
	{
		filename++;
	}
	else {
		filename = fullpepath;
	}

	int arraylen = sizeof(g_injectProcess) / sizeof(g_injectProcess[0]);
	for (int i = 0; i < arraylen; i++)
	{
		if (lstrcmpiW(filename, injectProcess[i].name) == 0)
		{
			*seq = i;
			return TRUE;
		}
	}
	return FALSE;
}


extern "C" __declspec(dllexport) int __stdcall hookfunctions() {

	int result = 0;
	WCHAR szapp[MAX_PATH];
	result = GetModuleFileNameW(0, szapp, MAX_PATH);
	WCHAR* filename = wcsrchr(szapp, L'\\');
	if (filename)
	{
		filename++;
	}
	else {
		log(L"hook parse filename:%ws error\r\n", filename);
		return FALSE;
	}

	if (lstrcmpiW(filename, EXPLORER_PROCESS_NAME) == 0 || lstrcmpiW(filename, L"cyguard.exe") == 0 ||
		lstrcmpiW(filename, L"cmd.exe") == 0 || lstrcmpiW(filename, L"powershell.exe") == 0
		/*||lstrcmpiW(filename, EXPLORER_PROCESS_NAME_PLUSPLUS) == 0*/)
	{
// 		result = hook(L"kernelbase.dll", L"GetLogicalDrives", (LPBYTE)GetLogicalDrivesNew, (FARPROC*)&GetLogicalDrivesOld);
// 		result = hook(L"kernelbase.dll", L"FindFirstVolumeW", (LPBYTE)FindFirstVolumeWNew, (FARPROC*)&FindFirstVolumeWOld);
// 		result = hook(L"kernelbase.dll", L"FindNextVolumeW", (LPBYTE)FindNextVolumeWNew, (FARPROC*)&FindNextVolumeWOld);
// 		result = hook(L"kernelbase.dll", L"GetLogicalDriveStringsW", (LPBYTE)GetLogicalDriveStringsWNew, (FARPROC*)&GetLogicalDriveStringsWOld);
		return FALSE;
	}

	//__debugbreak();

	BOOL bInSandBox = FALSE;

	// 判断是否在沙箱内	
	HMODULE hSfDeskDll = GetModuleHandle(L"SfDeskDll.dll");
	if (hSfDeskDll)
	{
		bInSandBox = TRUE;
	}

	// 沙箱内
	if (bInSandBox)
	{
		// 绘制水印
		if (WaterMarkStart())
		{
			hook(L"user32.dll", L"ShowWindow", (LPBYTE)ShowWindowNew, (FARPROC*)&ShowWindowOld);	
		}
		else
		{
			DP0("[LYSM][DllMain] WaterMarkStart failed. \n");
		}

		result = hook(L"kernelbase.dll", L"GetLogicalDrives", (LPBYTE)GetLogicalDrivesNew, (FARPROC*)&GetLogicalDrivesOld);
		result = hook(L"kernelbase.dll", L"FindFirstVolumeW", (LPBYTE)FindFirstVolumeWNew, (FARPROC*)&FindFirstVolumeWOld);
		result = hook(L"kernelbase.dll", L"FindNextVolumeW", (LPBYTE)FindNextVolumeWNew, (FARPROC*)&FindNextVolumeWOld);
		result = hook(L"kernelbase.dll", L"GetLogicalDriveStringsW", (LPBYTE)GetLogicalDriveStringsWNew, (FARPROC*)&GetLogicalDriveStringsWOld);
		//result = hook(L"kernelbase.dll", L"LoadLibraryW", (LPBYTE)lpLoadLibraryWNew, (FARPROC*)&lpLoadLibraryWOld);
		// 这个 Hook 会导致 office 打印时进程崩溃

		//result = hook(L"ntdll.dll", L"LdrLoadDll", (LPBYTE)lpLdrLoadDllNew, (FARPROC*)&lpLdrLoadDllOld);

		// 禁止打印
// 		hook(L"winspool.drv", L"OpenPrinterA", (LPBYTE)OpenPrinterANew, (FARPROC*)&__sys_OpenPrinterA);
// 		hook(L"winspool.drv", L"OpenPrinter2A", (LPBYTE)OpenPrinter2ANew, (FARPROC*)&__sys_OpenPrinter2A);
// 		hook(L"winspool.drv", L"OpenPrinterW", (LPBYTE)OpenPrinterWNew, (FARPROC*)&__sys_OpenPrinterW);
// 		hook(L"winspool.drv", L"OpenPrinter2W", (LPBYTE)OpenPrinter2WNew, (FARPROC*)&__sys_OpenPrinter2W);

	}
	// 沙箱外
	else
	{
		// todo
	}

	return 0;
}

int __cdecl log(const WCHAR* format, ...) {
	WCHAR szbuf[2048];

	va_list   pArgList;

	va_start(pArgList, format);

	int nByteWrite = vswprintf_s(szbuf, format, pArgList);

	va_end(pArgList);

	OutputDebugStringW(szbuf);

	return nByteWrite;
}

int __stdcall WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

	int result = 0;

	return result;
}
