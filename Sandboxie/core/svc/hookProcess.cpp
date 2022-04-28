#include "stdafx.h"
#include <windows.h>
#include "hookProcess.h"

#include <UserEnv.h>
#include <stdio.h>

#include <tlhelp32.h>
#include "../../common/win32_ntddk.h"

#include "../drv/api_defs.h"
#include <WtsApi32.h>

#include "../../apps/processInjector/main.h"
#include "../../common/my_version.h"








int __cdecl mylog(const WCHAR* format, ...) {
	WCHAR szbuf[0x1000];

	va_list   pArgList;

	va_start(pArgList, format);

	int nByteWrite = vswprintf_s(szbuf, format, pArgList);

	va_end(pArgList);

	OutputDebugStringW(szbuf);

	return nByteWrite;
}


DWORD isTargetProcess(const INJECT_PROCESS_INFO injectProcess[], WCHAR * fullpepath, int * seq) {

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


DWORD isPidWow64(DWORD pid) {
	BOOL wow64 = 0;
	int result = 0;
	HANDLE hprocess = OpenProcess(PROCESS_QUERY_INFORMATION| PROCESS_QUERY_LIMITED_INFORMATION, 0, pid);
	if (hprocess)
	{
		result = IsWow64Process(hprocess, &wow64);
		CloseHandle(hprocess);
	}
	return wow64;
}


DWORD getpidsFromNames(const INJECT_PROCESS_INFO injectProcess[],DWORD pids[],WCHAR * processnames[],int *flags) {
	int cnt = 0;

	//__debugbreak();

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
				const WCHAR* procname = injectProcess[i].name;
				if (lstrcmpiW(procname, pe.szExeFile) == 0)
				{
					pids[cnt] = pe.th32ProcessID;
					processnames[cnt] = (WCHAR*)procname;
					flags[cnt] = injectProcess[i].flag;
					cnt++;

					mylog(L"find process to inject and hook:%ws pid:%d", procname, pids[cnt]);
					break;
				}
			}
		} while (Process32Next(hSnap, &pe));
	}

	CloseHandle(hSnap);
	return cnt;
}

BOOL getProcessTokenByPid(HANDLE& hToken,DWORD pid,DWORD isboxed) {
	int bRet = 0;
	HANDLE hProcess = 0;
	if (isboxed)
	{
		HANDLE hp = ULongToHandle(pid);
		bRet = SbieApi_OpenProcess(&hProcess, hp);
		if (hProcess == 0) {
			mylog(L"SbieApi_OpenProcess %d error %d\r\n", pid, GetLastError());
			return FALSE;
		}
	}
	else {
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME , FALSE, pid);
	}
	
	if (hProcess)
	{
		//bRet = DebugActiveProcess(pid);
		//bRet = ControlProThreads(pid,TRUE);
		//if (bRet == STATUS_SUCCESS)

		if (isboxed)
		{

		}
		else {
// 			bRet = NtSuspendProcess(hProcess);
// 			if (bRet != STATUS_SUCCESS)
// 			{
// 				mylog(L"NtSuspendProcess %d error %d\r\n", pid, GetLastError());
// 			}
		}

		bRet = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken);

		CloseHandle(hProcess);
	}

	return bRet;
}


BOOL getProcessTokenByName(HANDLE& hToken, WCHAR* processname,int injected)
{
	int result = 0;
	DWORD dwCurSessionId = WTSGetActiveConsoleSessionId();
	result = WTSQueryUserToken(dwCurSessionId, &hToken);
	if (result == TRUE)
	{
		return TRUE;
	}

	BOOL bRet = FALSE;
	HANDLE hProcessSnap = NULL;
	PROCESSENTRY32 pe32;
	DWORD dwSessionId = -1;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hProcessSnap, &pe32))
	{
		do
		{
			if (lstrcmpiW(pe32.szExeFile, processname) == 0)
			{
				::ProcessIdToSessionId(pe32.th32ProcessID, &dwSessionId);
				if (dwSessionId != dwCurSessionId)
				{
					continue;
				}

				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
				if (hProcess)
				{
					if (injected)
					{

					}
					else {
// 						bRet = NtSuspendProcess(hProcess);
// 						if (bRet != STATUS_SUCCESS)
// 						{
// 							mylog(L"NtSuspendProcess %d error %d\r\n", pe32.th32ProcessID, GetLastError());
// 						}
					}
					bRet = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken);
					CloseHandle(hProcess);
					break;
				}
			}

		} while (Process32Next(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);

	return bRet;
}


BOOL createProcessWithToken(const HANDLE& hSrcToken,DWORD srcpid, WCHAR* strPath, WCHAR* lpCmdLine)
{
	int result = 0;

	HANDLE hTokenDup = NULL;
	result = DuplicateTokenEx(hSrcToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hTokenDup);
	if (result == 0)
	{
		mylog(L"DuplicateTokenEx error code %d\r\n",GetLastError());
		return FALSE;
	}

	DWORD dwSessionId = 0;
	result = ProcessIdToSessionId(srcpid, &dwSessionId);
	if (result == 0)
	{
		mylog(L"ProcessIdToSessionId error code %d\r\n", GetLastError());
		CloseHandle(hTokenDup);
		return FALSE;
	}
	result = SetTokenInformation(hTokenDup, TokenSessionId, &dwSessionId, sizeof(DWORD));
	if (result == 0)
	{
		mylog(L"SetTokenInformation error code %d\r\n", GetLastError());
		CloseHandle(hTokenDup);
		return FALSE;
	}

	LPVOID pEnv = NULL;
	result = CreateEnvironmentBlock(&pEnv, hTokenDup, FALSE);
	if (result == 0)
	{
		CloseHandle(hTokenDup);
		mylog(L"CreateEnvironmentBlock error code %d\r\n", GetLastError());
		return FALSE;
	}

	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFOW si = { 0 };
	si.cb = sizeof(STARTUPINFOW);
	si.lpDesktop = (WCHAR*)L"WinSta0\\Default";
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;
	//lpCmdLine can not be const string!else occur exception 0xC0000005!
	result = CreateProcessAsUserW(hTokenDup, strPath, lpCmdLine, NULL, NULL, FALSE, dwCreationFlags, pEnv, NULL, &si, &pi);
	if (result == 0)
	{
		mylog(L"CreateProcessAsUserW error code %d\r\n", GetLastError());
		CloseHandle(hTokenDup);
		if (pEnv != NULL) {
			DestroyEnvironmentBlock(pEnv);
		}

		return FALSE;
	}
	else {
		mylog(L"CreateProcessAsUserW ok\r\n");
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hTokenDup);
	if (pEnv != NULL) {
		DestroyEnvironmentBlock(pEnv);
	}
	return TRUE;
}




int __stdcall hookProcess(LPVOID params) {

	return 0;

	SVC_PROCESS_MSG* msg = (SVC_PROCESS_MSG*)params;

	int result = 0;
	
	DWORD pids[MAX_PID_BUFFER] = { 0 };
	int flags[MAX_PID_BUFFER];
	WCHAR* processnames[MAX_PID_BUFFER] = { 0 };
	int cnt = 0;

	if (msg == 0 || msg->process_id == 0)
	{
		cnt = getpidsFromNames(g_injectProcess, pids,processnames,flags);
	}
	else {
		mylog( L"checking process to inject and hook:%ws,pid:%d,iswow:%d,bHostInject:%d,session_id:%d,reason:%d",
			msg->process_name, msg->process_id,msg->is_wow64,msg->bHostInject,msg->session_id,(DWORD)msg->reason);

// 		WCHAR procboxname[64] = { 0 };
// 		WCHAR procsid[MAX_PATH];
// 		WCHAR procimage[MAX_PATH];
// 		ULONG sessionid = 0;
// 		result = SbieApi_QueryProcess((HANDLE)msg->process_id, procboxname,procimage, procsid, &sessionid);
// 		if (procboxname[0])
// 		{
// 			mylog(L"process:%ws,pid:%d already in box:%ws\r\n", procimage, msg->process_id, procboxname);
// 			return FALSE;
// 		}
		

		if (msg->reason) {
			pids[0] = msg->process_id;
			processnames[0] = (WCHAR*)msg->process_name;
			flags[0] = 0;
			cnt = 1;
		}
		else {
			int seq = 0;
			result = isTargetProcess(g_injectProcess, msg->process_name, &seq);
			if (result)
			{
				pids[0] = msg->process_id;
				processnames[0] = (WCHAR*)g_injectProcess[seq].name;
				flags[0] = g_injectProcess[seq].flag;
				cnt = 1;
			}
		}
	}

	if (cnt <= 0)
	{
		return FALSE;
	}

	//__debugbreak();
	for (int i = 0; i < cnt; i++)
	{
		int invalid = FALSE;
		for (int k = 0; k < sizeof(g_NoneInjectProcess) / sizeof(g_NoneInjectProcess[0]); k++)
		{
			if (g_NoneInjectProcess[k].name)
			{
				if (lstrcmpiW(g_NoneInjectProcess[k].name, processnames[i]) == 0)
				{
					invalid = TRUE;
					break;
				}
			}
		}

		if (invalid == FALSE) {
			
			DWORD pid = pids[i];

			WCHAR szpath[MAX_PATH] = { 0 };
			result = GetModuleFileNameW(0, szpath, MAX_PATH);

			WCHAR* pos = wcsrchr(szpath, L'\\');
			if (pos)
			{
				*(pos + 1) = 0;
			}
			else {
				lstrcatW(szpath, L"\\");
			}
			DWORD wow64 = isPidWow64(pid);
			if (wow64)
			{
				lstrcatW(szpath, L"32\\");
			}

			lstrcatW(szpath, INJECTOR_HELPER_PROCESS);
			WCHAR szcmd[1024];
			wsprintfW(szcmd, L"\"%ws\" %d %ws %d", szpath, pid, processnames[i], (DWORD)msg->reason);

			HANDLE token = 0;
			if (msg->reason)
			{
				//ULONGLONG primetoken = SbieApi_QueryProcessInfoEx((HANDLE)pid, 'ptok', 0);
				//token = (HANDLE)primetoken;

				token = (HANDLE)msg->create_time;
				if (token == 0)
				{
					result = getProcessTokenByName(token, EXPLORER_PROCESS_NAME, (DWORD)msg->reason);
					
					if (token == 0)
					{
						result = getProcessTokenByName(token, SBIESVC_EXE, (DWORD)msg->reason);
					}

					mylog(L"pid:%d boxed:%d prime token null\r\n", pid, msg->reason);
					//result = getProcessTokenByPid(token, pid, (DWORD)msg->reason);
				}
				else {
					result = TRUE;
				}
			}
			else {
				result = getProcessTokenByPid(token, pid, (DWORD)msg->reason);
			}

			if (result)
			{
				result = createProcessWithToken(token, pid, 0, szcmd);
				mylog(L"inject process cmd:%ws,token:%u,pid:%u\r\n", szcmd, HandleToULong(token), pid);
			}
			else {
				mylog(L"getProcessTokenByPid pid:%u boxed:%d error\r\n", pid, msg->reason);
			}
		}
		
	}

	return result;
}


LPVOID FileMapping(const WCHAR* name, DWORD size)
{
	LPVOID lpMapAddr = 0;
	HANDLE m_hMapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
	if (m_hMapFile)
	{
		lpMapAddr = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	}
	else
	{
		m_hMapFile = CreateFileMappingW((HANDLE)0xFFFFFFFF, NULL, PAGE_EXECUTE_READWRITE, 0, size, name);
		if (m_hMapFile)
		{
			lpMapAddr = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

			//FlushViewOfFile(lpMapAddr, strTest.length() + 1);
		}
	}

	return lpMapAddr;
}


BOOL IsAdminPrivilege()
{
	BOOL bIsAdmin = FALSE;
	BOOL bRet = FALSE;
	SID_IDENTIFIER_AUTHORITY idetifier = SECURITY_NT_AUTHORITY;
	PSID pAdministratorGroup;
	if (AllocateAndInitializeSid(
		&idetifier,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pAdministratorGroup))
	{
		if (!CheckTokenMembership(NULL, pAdministratorGroup, &bRet))
		{
			bIsAdmin = FALSE;
		}
		if (bRet)
		{
			bIsAdmin = TRUE;
		}
		FreeSid(pAdministratorGroup);
	}

	return bIsAdmin;
}


DWORD WINAPI ControlProThreads(DWORD ProcessId, BOOL Suspend)
{
	// 定义函数错误代码
	DWORD LastError = NULL;

	// 通过 CreateToolhelp32Snapshot 函数获取当前系统所有线程的快照
	HANDLE ProcessHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, ProcessId);
	if (ProcessHandle == INVALID_HANDLE_VALUE)
	{
		// 函数调用失败，返回错误代码，方便打印出错误信息
		LastError = GetLastError();
		return LastError;
	}

	// 判断进程是否为当前进程，SkipMainThread 的作用是是否跳过主线程
	INT SkipMainThread = 0;
	HANDLE DupProcessHandle = NULL;
	DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &DupProcessHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
	DWORD IsThisProcess = GetProcessId(DupProcessHandle);

	// 如果传入的进程 ID 不是当前进程，则将 SkipMainThread 变为 1，表示不跳过主线程
	if (ProcessId != IsThisProcess)
		SkipMainThread = 1;

	THREADENTRY32 ThreadInfo = { sizeof(ThreadInfo) };
	BOOL fOk = Thread32First(ProcessHandle, &ThreadInfo);

	while (fOk)
	{
		// 将线程 ID 转换为线程句柄
		HANDLE ThreadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, ThreadInfo.th32ThreadID);

		// 是否为传入进程的子线程，且判断是挂起操作还是释放操作
		if (ThreadInfo.th32OwnerProcessID == ProcessId && Suspend == TRUE)
		{
			// 如果是当前进程，就跳过主线程，SkipMainThread == 0 表示为主线程
			if (SkipMainThread != 0)
			{
				// 挂起线程
				DWORD count = SuspendThread(ThreadHandle);
				//cout << "[*] 线程号: " << ThreadInfo.th32ThreadID << " 挂起系数 + 1 " << " 上一次挂起系数为: " << count << endl;
			}
			SkipMainThread++;
		}
		else if (ThreadInfo.th32OwnerProcessID == ProcessId && Suspend == FALSE)
		{
			if (SkipMainThread != 0)
			{
				// 释放线程
				DWORD count = ResumeThread(ThreadHandle);
				//cout << "[-] 线程号: " << ThreadInfo.th32ThreadID << " 挂起系数 - 1 " << " 上一次挂起系数为: " << count << endl;
			}
			SkipMainThread++;
		}
		fOk = Thread32Next(ProcessHandle, &ThreadInfo);
	}

	// 关闭句柄
	CloseHandle(ProcessHandle);
	return SkipMainThread;
}


#include "configStrategy.h"


//question:
//1) why does the result of SbieApi_GetHomePath errornious?
//2) why CreateFileW in publicfun.dll failed with error code 2?

void __stdcall configThread(LPVOID params) {

	//mylog(L"configThread start\r\n");

	int result = 0;

	WCHAR curdir[MAX_PATH];
	result = GetModuleFileNameW(0, curdir, MAX_PATH);

	WCHAR* pos = wcsrchr(curdir, L'\\');
	if (pos > 0)
	{
		*(pos + 1) = 0;
	}

	lstrcatW(curdir,JSON_CONFIG_FILENAME);
	//mylog(L"config file:%ws\r\n", curdir);

// 	result = SbieApi_GetHomePath(0, 0, curdir,MAX_PATH);
// 	lstrcatW(curdir, L"\\" JSON_CONFIG_FILENAME);
// 	mylog(L"config file:%ws\r\n", curdir);

	while (1)
	{
// 		WCHAR filepath[MAX_PATH];
// 		LjgApi_getPath(filepath);
// 		lstrcatW(filepath,L"\\" JSON_CONFIG_FILENAME);
// 		mylog(L"config file:%ws\r\n", filepath);

		HANDLE hf = CreateFileW(curdir, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hf && hf != INVALID_HANDLE_VALUE)
		{
			//mylog(L"get new config file\r\n");

			int filesize = GetFileSize(hf, 0);
			char* data = new char[(SIZE_T)filesize + 1024];
			DWORD dwcnt = 0;
			result = ReadFile(hf, data, filesize, &dwcnt, 0);
			CloseHandle(hf);
			if (result)
			{
				*(WORD*)(data + filesize) = 0;
				//mylog(L"svc ReadFile config file %s size %d\r\n",curdir, filesize);
#ifdef _WIN64
				result = setJsonConfig(curdir,data,filesize);
#endif
			}
			else {
				mylog(L"svc ReadFile config file %s error %d\r\n",curdir, GetLastError());
			}
			delete [] data;
			//DeleteFileW(JSON_CONFIG_FILENAME);
		}
		else {
			//mylog(L"svc CreateFileW config file %s error %d\r\n", curdir, GetLastError());
		}

		Sleep(6000);
	}

	return;
}
