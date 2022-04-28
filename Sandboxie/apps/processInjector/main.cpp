
#pragma warning(disable:4005)

#include <windows.h>

#include <stdio.h>

#include <tlhelp32.h>

#include "main.h"

#include "../../common/my_version.h"
#include "../../common/win32_ntddk.h"
#include "../../core/drv/api_defs.h"
#include "../../common/defines.h"

//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows下的AppInit_DLLs项
//只有调用了User32.dll的进程才会发生这种dll注入
//NtCreateThreadEx
//QueueUserAPC


//值不能为 null 一般是工程设置问题，跟编码无关,比如“选定内容没有属性页”

#define CYDUARD_DLL_FILENAME L"cyguard.dll"


#pragma pack(1)

typedef struct
{
	
	PHANDLE handle;
	UNICODE_STRING punistr;
	ULONG flags;
	WCHAR* path2file;
}LDRLOADDLL_PARAMETERS;


#pragma pack()


extern "C" __declspec(dllimport) NTSTATUS __stdcall NtSuspendProcess(HANDLE hProcess);

extern "C" __declspec(dllimport) NTSTATUS __stdcall NtResumeProcess(HANDLE hProcess);

int __cdecl log(const WCHAR* format, ...);







int __cdecl log(const WCHAR* format, ...) {
	WCHAR szbuf[2048];

	va_list   pArgList;

	va_start(pArgList, format);

	int nByteWrite = vswprintf_s(szbuf, format, pArgList);

	va_end(pArgList);

	OutputDebugStringW(szbuf);

	return nByteWrite;
}


BOOL Is64BitOS()
{
	typedef VOID(WINAPI* LPFN_GetNativeSystemInfo)(__out LPSYSTEM_INFO lpSystemInfo);

	HMODULE hkernel32 = GetModuleHandleW(L"kernel32.dll");
	if (hkernel32)
	{
		LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(hkernel32, "GetNativeSystemInfo");
		if (fnGetNativeSystemInfo)
		{
			SYSTEM_INFO stInfo = { 0 };
			fnGetNativeSystemInfo(&stInfo);
			if (stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 || stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}





DWORD getpidsFromNames(const INJECT_PROCESS_INFO  injectProcess[], DWORD pids[], WCHAR* processnames[],int flags[]) {
	int cnt = 0;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		return cnt;
	}

	PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
	if (Process32First(hSnap, &pe) == TRUE)
	{
		int arraylen = sizeof(g_injectProcess) / sizeof(g_injectProcess[0]);
		do
		{
			for (int i = 0; i < arraylen; i++)
			{
				WCHAR* procname =(WCHAR*) injectProcess[i].name;
				if (lstrcmpiW(procname, pe.szExeFile) == 0)
				{
					pids[cnt] = pe.th32ProcessID;
					processnames[cnt] = procname;
					flags[cnt ] = injectProcess[i].flag;
					cnt++;

					log(L"find process:%ws pid:%d", procname, pids[cnt]);

					break;
				}
			}

		} while (Process32Next(hSnap, &pe));
	}

	CloseHandle(hSnap);
	return cnt;
}

#define STATUS_SUCCESS 0

DWORD WINAPI ControlProThreads(DWORD ProcessId,DWORD exceptid, BOOL Suspend)
{
	DWORD LastError = NULL;

	HANDLE DupProcessHandle = NULL;
	DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &DupProcessHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
	DWORD IsThisProcess = GetProcessId(DupProcessHandle);
	if (ProcessId == IsThisProcess) {
		return FALSE;
	}

	HANDLE ProcessHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, ProcessId);
	if (ProcessHandle == INVALID_HANDLE_VALUE)
	{
		LastError = GetLastError();
		return LastError;
	}

	THREADENTRY32 ThreadInfo = { sizeof(ThreadInfo) };
	BOOL fOk = Thread32First(ProcessHandle, &ThreadInfo);

	while (fOk)
	{
		HANDLE ThreadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, ThreadInfo.th32ThreadID);
		if (ThreadHandle && ThreadInfo.th32ThreadID != exceptid)
		{
			if (ThreadInfo.th32OwnerProcessID == ProcessId && Suspend == TRUE)
			{
				LastError = SuspendThread(ThreadHandle);
			}
			else if (ThreadInfo.th32OwnerProcessID == ProcessId && Suspend == FALSE)
			{
				LastError = ResumeThread(ThreadHandle);
			}
		}

		fOk = Thread32Next(ProcessHandle, &ThreadInfo);
	}

	CloseHandle(ProcessHandle);
	return LastError;
}


int apcInject(DWORD pid,WCHAR * path) {
	int result = 0;
	HANDLE ProcessHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
	if (ProcessHandle)
	{
		THREADENTRY32 ThreadInfo = { sizeof(ThreadInfo) };
		BOOL fOk = Thread32First(ProcessHandle, &ThreadInfo);
		if (fOk == FALSE)
		{
			log(L"Thread32First error code %d\r\n", GetLastError());
		}
		else {
			while (fOk) {
				HANDLE ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, ThreadInfo.th32ThreadID);
				if (ThreadHandle)
				{
					result = QueueUserAPC((PAPCFUNC)LoadLibraryW, ThreadHandle, (ULONG_PTR)path);
					if (result)
					{
						log(L"QueueUserAPC %d success\r\n", ThreadInfo.th32ThreadID);
					}
					else {
						log(L"QueueUserAPC %d error code %d\r\n", ThreadInfo.th32ThreadID, GetLastError());
					}
				}
				else {
					log(L"OpenThread %d error code %d\r\n", ThreadInfo.th32ThreadID, GetLastError());
				}

				fOk = Thread32Next(ProcessHandle, &ThreadInfo);

			}
		}
	}
	else {
		log(L"CreateToolhelp32Snapshot error code %d\r\n", GetLastError());
	}
	return result;
}

/*
[16768] create myProcess command:"C:\Program Files\SafeDesktop\processInjector.exe" 3100 ScreenSketch.exe
******************************************************************
* This break indicates this binary is not signed correctly: \Device\HarddiskVolume2\Program Files\SafeDesktop\processInjector.exe
* and does not meet the system policy.
* The binary was attempted to be loaded in the process: \Device\HarddiskVolume2\Program Files\SafeDesktop\sfDeskSvc.exe
* This is not a failure in CI, but a problem with the failing binary.
* Please contact the binary owner for getting the binary correctly signed.
******************************************************************
*/

int __stdcall injectIntoProcessSession(DWORD pid,const WCHAR * procname, int injected) {

	int result = -1;
	//__debugbreak();

	log(L"prepare to inject pid %u,process:%ws, inbox:%d\r\n",pid,procname, injected);

	for (int i = 0; i < sizeof(g_ignoreProcess) / sizeof(g_ignoreProcess[0]); i++)
	{
		if (g_ignoreProcess[i].name)
		{
			if (lstrcmpiW(g_ignoreProcess[i].name, procname) == 0)
			{
				return FALSE;
			}
		}
	}

// 	for (int i = 0; i < sizeof(g_NoneInjectProcess) / sizeof(g_NoneInjectProcess[0]); i++)
// 	{
// 		if (g_NoneInjectProcess[i].name)
// 		{
// 			if (lstrcmpiW(g_NoneInjectProcess[i].name, procname) == 0)
// 			{
// 				return FALSE;
// 			}
// 		}
// 	}

// 	int valid = FALSE;
// 	int arraylen = sizeof(g_injectProcess) / sizeof(g_injectProcess[0]);
// 	for (int i = 0; i < arraylen; i++)
// 	{
// 		if (g_injectProcess[i].name)
// 		{
// 			if (lstrcmpiW(procname, g_injectProcess[i].name) == 0)
// 			{
// 				valid = TRUE;
// 				break;
// 			}
// 		}
// 	}
// 	if (valid == FALSE)
// 	{
// 		return FALSE;
// 	}


	HANDLE hprocess = OpenProcess( PROCESS_SUSPEND_RESUME | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_VM_READ |
		PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD , 0, pid);
	if (hprocess)
	{
		SIZE_T addrsize = 1024;
		void*  params = VirtualAllocEx(hprocess, 0, addrsize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		//result = NtAllocateVirtualMemory(hprocess, &vaddr, 0, &addrsize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (params)
		{

			WCHAR szpath[MAX_PATH + 4] = { 0 };
			result = GetModuleFileNameW(0, szpath, MAX_PATH);
			WCHAR* p = wcsrchr(szpath, L'\\');
			if (p)
			{
				*(p + 1) = 0;
			}
			lstrcatW((WCHAR*)szpath, CYDUARD_DLL_FILENAME);

			SIZE_T writesize = 0;
			result = WriteProcessMemory(hprocess, params, szpath,(lstrlenW(szpath) + 1) * sizeof(WCHAR), &writesize);
			if (result)
			{
// 				result = apcInject(pid, (WCHAR*)params);
// 				result = NtResumeProcess(hprocess);
//				HMODULE hntdll = GetModuleHandleW(L"ntdll.dll");

				HMODULE hkernel32 = GetModuleHandleW(L"kernel32.dll");
				if (hkernel32)
				{
// 					void* function = LdrLoadDll;
// 					LDRLOADDLL_PARAMETERS* ldrparams = (LDRLOADDLL_PARAMETERS*)((char*)params + 0x100);
// 					RtlInitUnicodeString(&ldrparams->punistr, (WCHAR*)params);
// 					ldrparams->flags = 0;
// 					ldrparams->handle = 0;
// 					ldrparams->path2file = 0;

					LPVOID pLoadLib = (LPVOID)GetProcAddress(hkernel32, "LoadLibraryW");
					if (pLoadLib)
					{

						if (injected)
						{
							//Sleep(3000);
						}

						//result = NtSuspendProcess(hprocess);

						

						DWORD threadid = 0;
						HANDLE hremote = CreateRemoteThread(hprocess, 0, 0, (LPTHREAD_START_ROUTINE)pLoadLib, params, 0, &threadid);

						//result = ControlProThreads(pid, threadid, TRUE);

						//HANDLE hremote = CreateRemoteThread(hprocess, 0, 0, (LPTHREAD_START_ROUTINE)function, (LPVOID)ldrparams, 0, 0);
						if (hremote)
						{
							result = WaitForSingleObject(hremote, 6000);

							//result = ControlProThreads(pid, -1, FALSE);

							if (result == WAIT_TIMEOUT)
							{
								log(L"WaitForSingleObject error code:%d\r\n", GetLastError());
								CloseHandle(hremote);
								CloseHandle(hprocess);
								return FALSE;
							}
							else if (result == WAIT_OBJECT_0)
							{

							}
							else {

							}
							DWORD threadretcode = -1;
							result = GetExitCodeThread(hremote, &threadretcode);
							CloseHandle(hremote);

							

// 							result = NtResumeProcess(hprocess);
// 							if (result != STATUS_SUCCESS)
// 							{
// 								log(L"NtResumeProcess pid:%d process:%ws error code %d return value %d\r\n", pid, procname, GetLastError(), result);
// 							}

							CloseHandle(hprocess);
							log(L"CreateRemoteThread pid:%d process:%ws remote handle:%X ok with exit code:%X\r\n",pid,procname, hremote, threadretcode);
							return threadretcode;
						}
						else {
							result = GetLastError();
							log(L"CreateRemoteThread pid:%d process:%ws error code %d\r\n",pid,procname, result);
						}

						//result = ControlProThreads(pid, -1, FALSE);
					}
					else {
						result = GetLastError();
					}
				}
				else {
					result = GetLastError();
				}
			}
			else {
				result = GetLastError();
				log(L"WriteProcessMemory pid:%d process:%ws error code %d\r\n",pid, procname, result);
			}
			VirtualFreeEx(hprocess, params, 0, MEM_RELEASE);
		}
		else {
			result = GetLastError();
			log(L"VirtualAllocEx pid:%d process:%ws error code %d\r\n",pid,procname, result);
		}

		//result = NtResumeProcess(hprocess);
		//log(L"NtResumeProcess %d return code %d errorcode %d\r\n",pid, result,GetLastError());

		CloseHandle(hprocess);
	}
	else {
		result = GetLastError();
		log(L"OpenProcess pid:%d process:%ws error code %d\r\n",pid,procname, result);
	}
		
	return result;
}






int __stdcall injectIntoProcess(DWORD pid,const WCHAR* procname,BOOLEAN injected) {

	int result = 0;
	WCHAR mutexname[256];
	wsprintfW(mutexname, L"%ws_%u_mutex", procname,pid);

	HANDLE h = CreateMutexW(0, FALSE, mutexname);
	if (h)
	{
		CloseHandle(h);
		result = GetLastError();
		if (result == ERROR_ALREADY_EXISTS)
		{
			log(L"process:%ws pid:%d had already been injected\r\n", procname, pid);
			return 0;
		}
		else {
			log(L"prepare to be injected and to hook process:%ws and pid:%d\r\n", procname, pid);

			return injectIntoProcessSession(pid,procname, injected);
		}
	}
	else {
		log(L"process:%ws pid:%d CreateMutexW error\r\n", procname, pid);
		return -1;
	}
}






int __stdcall WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

	//__debugbreak();

	log(L"process injector start\r\n");

	int result = -1;

	WCHAR * cmd = GetCommandLineW();

	int argc = 0;
	LPWSTR  * params = CommandLineToArgvW(cmd, &argc);
	if (argc >= 2)
	{
		int pid = _wtoi(params[1]);
		if (pid)
		{
			if (argc == 2)
			{
				result = injectIntoProcess(pid, L"",FALSE);
			}else if (argc == 3)
			{
				result = injectIntoProcess(pid, params[2], FALSE);
			}else if (argc >= 4)
			{
				int injected = _wtoi(params[3]);
				result = injectIntoProcess(pid, params[2], injected);
			}
		}
		else {
			log(L"inject params:%ws,params cnt:%d error\r\n", cmd,argc);
		}
	}else if (argc == 1)
	{
		DWORD pids[MAX_PID_BUFFER] = { 0 };
		WCHAR *procnames[MAX_PID_BUFFER] = { 0 };
		int flags[MAX_PID_BUFFER];
		int cnt = getpidsFromNames(g_injectProcess, pids, procnames,flags);
		for (int i = 0; i < cnt; i++)
		{
			//int flag = SbieApi_QueryScreenshot();
			//if ( (flags[i] & SCREENCAPTURE_INJECT_FLAG) || (flags[i] & (~SCREENCAPTURE_INJECT_FLAG)) )
			{
				result = injectIntoProcess(pids[i], procnames[i], TRUE);
			}
		}
	}
	else {

	}
	return result;
}
