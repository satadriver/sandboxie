
#include <windows.h>
#include <Shlobj.h>
#include <stdio.h>
#include <io.h>
#include <UserEnv.h>
#include <tlhelp32.h>
#include <WtsApi32.h>

#include "utils.h"
#include "main.h"
#include "apiFuns.h"

#pragma comment(lib,"wtsapi32.lib")
#pragma comment(lib,"Userenv.lib")

#include "../../common/my_version.h"



int fileReaderW(const WCHAR* filename,WCHAR ** lpbuf,int *lpsize) {
	int result = 0;

	HANDLE hf = CreateFileW(filename, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ,0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hf == INVALID_HANDLE_VALUE)
	{
		result = GetLastError();
		return FALSE;
	}

	DWORD highsize = 0;
	*lpsize = GetFileSize(hf, &highsize);

	if (lpbuf)
	{
		if (*lpbuf == 0)
		{
			*lpbuf = new WCHAR[*lpsize + 1024];
			*(*lpbuf) = 0;
		}
	}
	else {
		CloseHandle(hf);
		return FALSE;
	}

	DWORD readsize = 0;
	result = ReadFile(hf, *lpbuf, *lpsize, &readsize, 0);
	if (result > 0)
	{
		*(WCHAR*)((char*)*lpbuf + readsize) = 0;
	}
	else {
		result = GetLastError();
	}
	CloseHandle(hf);
	return readsize;
}

int fileReaderA(const CHAR* filename, CHAR** lpbuf, int* lpsize) {
	int result = 0;

	HANDLE hf = CreateFileA(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hf == INVALID_HANDLE_VALUE)
	{
		result = GetLastError();
		delete* lpbuf;
		return FALSE;
	}

	DWORD highsize = 0;
	*lpsize = GetFileSize(hf, &highsize);

	if (lpbuf)
	{
		if (*lpbuf == 0)
		{
			*lpbuf = new CHAR[*lpsize + 1024];
			*(*lpbuf) = 0;
		}
	}
	else {
		CloseHandle(hf);
		return FALSE;
	}

	DWORD readsize = 0;
	result = ReadFile(hf, *lpbuf, *lpsize, &readsize, 0);
	if (result > 0)
	{
		*(CHAR*)((char*)*lpbuf + readsize) = 0;
	}
	else {
		result = GetLastError();
	}
	CloseHandle(hf);
	return readsize;
}

int fileWriterW(const WCHAR* filename, WCHAR* lpbuf, int lpsize,int append) {
	int result = 0;
	HANDLE h = INVALID_HANDLE_VALUE;
	if (append )
	{
		h = CreateFileW(filename, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (h == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
		DWORD highsize = 0;
		DWORD filesize = GetFileSize(h, &highsize);

		result = SetFilePointer(h, filesize, (long*)&highsize, FILE_BEGIN);
	}
	else {
		h = CreateFileW(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (h == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
	}

	DWORD writesize = 0;
	result = WriteFile(h, lpbuf, lpsize*sizeof(WCHAR), &writesize, 0);
	FlushFileBuffers(h);
	CloseHandle(h);
	return result;
}

int fileWriterA(CHAR* filename, CHAR* lpbuf, int lpsize, int append) {
	int result = 0;
	HANDLE h = INVALID_HANDLE_VALUE;
	if (append)
	{
		h = CreateFileA(filename, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (h == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
		DWORD highsize = 0;
		DWORD filesize = GetFileSize(h, &highsize);

		result = SetFilePointer(h, filesize, (long*)&highsize, FILE_BEGIN);
	}
	else {
		h = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (h == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
	}

	DWORD writesize = 0;
	result = WriteFile(h, lpbuf, lpsize * sizeof(CHAR), &writesize, 0);
	FlushFileBuffers(h);
	CloseHandle(h);
	return result;
}

int getFileType(const WCHAR* filename) {
	int result = 0;
	result = GetFileAttributesW(filename);
	if (result )
	{
		if (result & FILE_ATTRIBUTE_DIRECTORY)
		{
			return FILE_ATTRIBUTE_DIRECTORY;
		}
		else if(result & FILE_ATTRIBUTE_ARCHIVE){
			return FILE_ATTRIBUTE_ARCHIVE;
		}
	}
	
	return FALSE;
}

BOOL Is64BitOS()
{
	typedef VOID(WINAPI* LPFN_GetNativeSystemInfo)(__out LPSYSTEM_INFO lpSystemInfo);
	LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandleW(L"kernel32"), "GetNativeSystemInfo");
	if (fnGetNativeSystemInfo)
	{
		SYSTEM_INFO stInfo = { 0 };
		fnGetNativeSystemInfo(&stInfo);
		if (stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 || stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		{
			return TRUE;
		}
	}
	return FALSE;
}

int IsWow64()
{
	BOOL bIsWow64 = FALSE;
	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function and GetProcAddress to get a pointer to the function if available.
	char szKernel32[] = { 'k','e','r','n','e','l','3','2',0 };
	char szIsWow64Process[] = { 'I','s','W','o','w','6','4','P','r','o','c','e','s','s',0 };
	typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	HMODULE hDllKernel32 = GetModuleHandleA(szKernel32);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(hDllKernel32, szIsWow64Process);
	if (NULL != fnIsWow64Process)
	{
		int iRet = fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
		if (iRet)
		{

		}
	}
	return bIsWow64;
}


void debug_printA(const char* formatstr, ...)
{
	va_list argptr;
	va_start(argptr, formatstr);
	char buf[4096];
	wvsprintfA(buf, formatstr, argptr);
	::OutputDebugStringA(buf);
}

void debug_printW(const WCHAR* formatstr, ...)
{
	va_list argptr;
	va_start(argptr, formatstr);
	WCHAR buf[4096];
	wvsprintfW(buf, formatstr, argptr);
	::OutputDebugStringW(buf);
}






int sblog(const char* categary, const char* module, const char* grad, const char* msg) {

	const char* format = "[%s][%s][%s][%s]\r\n";
	char buf[4096];
	int len = wsprintfA(buf, format, categary, module, grad, msg);

	char szpath[MAX_PATH];
	int result = 0;

	result = GetModuleFileNameA(0, szpath, MAX_PATH);

	char* sep = strrchr(szpath, '\\');
	if (sep <= 0)
	{
		return -1;
	}
	*(sep + 1) = 0;
	lstrcatA(szpath, SANDBOXIE_LOG_FILENAME);

	HANDLE hf = CreateFileA(szpath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (hf == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

	DWORD cnt = 0;
	result = WriteFile(hf, buf, len, &cnt, 0);
	CloseHandle(hf);

	return result;
}



int translateQuota(WCHAR * str) {
	for (int i = 0;i < lstrlenW(str); i ++)
	{
		if (str[i] == L'/')
		{
			str[i] = L'\\';
		}
	}
	return TRUE;
}


BOOL getProcessTokenByPid(HANDLE& hToken, DWORD pid) {
	int bRet = 0;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (hProcess)
	{
		bRet = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken);

		CloseHandle(hProcess);
	}

	return bRet;
}



int getPidFromName(const WCHAR* processname) {
	
	int result = 0;
	int pid = 0;
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	result = Process32First(hProcessSnap, &pe32);
	if (result)
	{
		do
		{
			if (lstrcmpiW(pe32.szExeFile, processname) == 0)
			{
				pid = pe32.th32ProcessID;
				mylog(L"process:%ws pid:%d\r\n", processname, pid);
				break;
			}

		} while (Process32Next(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);

	return pid;
}



BOOL getProcessTokenByName(HANDLE& hToken, const WCHAR* processname)
{
	int result = 0;
	BOOL pid = FALSE;
	DWORD dwCurSessionId = WTSGetActiveConsoleSessionId();
	result = WTSQueryUserToken(dwCurSessionId, &hToken);
	if (result == TRUE)
	{

	}

	PROCESSENTRY32 pe32;

	DWORD dwSessionId = -1;

	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	result = Process32First(hProcessSnap, &pe32);
	if (result)
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
					pid = pe32.th32ProcessID;
					result = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken);
					CloseHandle(hProcess);
					mylog(L"process:%ws pid:%d\r\n", processname, pid);
					break;
				}
			}

		} while (Process32Next(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);

	return pid;
}


BOOL createProcessWithToken(const HANDLE& hSrcToken, DWORD srcpid, WCHAR* lpCmdLine)
{
	int result = 0;

	result = adjustPrivileges();

	HANDLE hTokenDup = NULL;
	result = DuplicateTokenEx(hSrcToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hTokenDup);
	if (result == 0)
	{
		return FALSE;
	}

	DWORD dwSessionId = 0;
	result = ProcessIdToSessionId(srcpid, &dwSessionId);
	if (result == 0)
	{
		result = GetLastError();
		mylog(L"ProcessIdToSessionId errorcode:%d\r\n", result);
		CloseHandle(hTokenDup);
		return FALSE;
	}
	result = SetTokenInformation(hTokenDup, TokenSessionId, &dwSessionId, sizeof(DWORD));
	if (result == 0)
	{
		result = GetLastError();
		mylog(L"SetTokenInformation errorcode:%d\r\n", result);
		//CloseHandle(hTokenDup);
		//return FALSE;
	}

	LPVOID pEnv = NULL;
	result = CreateEnvironmentBlock(&pEnv, hTokenDup, FALSE);
	if (result == 0)
	{
		CloseHandle(hTokenDup);
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
	result = CreateProcessWithTokenW(hTokenDup, 0, 0, lpCmdLine, dwCreationFlags, pEnv, NULL, &si, &pi);
	if (result == 0)
	{
		mylog(L"CreateProcessWithTokenW %ws result:%d errorcode:%d\r\n", lpCmdLine, result, GetLastError());
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hTokenDup);
	if (pEnv != NULL) {
		DestroyEnvironmentBlock(pEnv);
	}
	return TRUE;
}



int createProcessAsExplorer(WCHAR* cmdline) {
	int result = 0;
	HANDLE htoken = 0;
	int pid = getProcessTokenByName(htoken, EXPLORER_PROCESS_NAME);
	if (pid)
	{
		result = createProcessWithToken(htoken, pid, cmdline);
	}

	mylog(L"createProcessAsExplorer:%ws result:%d errorcode:%d\r\n", cmdline, result, GetLastError());
	return result;
}


int EnableDebugPrivilege()
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		return FALSE;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue)) {
		__try {
			if (hToken) {
				CloseHandle(hToken);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {};
		return FALSE;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL)) {
		__try {
			if (hToken) {
				CloseHandle(hToken);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {};
		return FALSE;
	}

	return TRUE;
}


//The process that calls CreateProcessWithTokenW must have the SE_IMPERSONATE_NAME privilege
//CreateProcessAsUser must have the SE_INCREASE_QUOTA_NAME privilege and may require the SE_ASSIGNPRIMARYTOKEN_NAME privilege if the token is not assignable
int adjustPrivileges() {
	HANDLE htoken = 0;
	int result = 0;
	result = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &htoken);
	if (result) {
		DWORD s = sizeof(TOKEN_PRIVILEGES) + 2 * sizeof(LUID_AND_ATTRIBUTES);
		TOKEN_PRIVILEGES* p = (PTOKEN_PRIVILEGES)malloc(s);
		if (LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &(p->Privileges[0].Luid)) == 0
			||
			LookupPrivilegeValueW(NULL, SE_IMPERSONATE_NAME, &(p->Privileges[1].Luid)) == 0
			||
			LookupPrivilegeValueW(NULL, SE_INCREASE_QUOTA_NAME, &(p->Privileges[2].Luid)) == 0)
		{
			free(p);
			return FALSE;
		}
		p->PrivilegeCount = 3;
		for (unsigned long i = 0; i < p->PrivilegeCount; ++i) {
			p->Privileges[i].Luid = p->Privileges[i].Luid;
			p->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
		}
		result = AdjustTokenPrivileges(htoken, FALSE, p, s, NULL, NULL);
		int error = GetLastError();
		if (result == 0 || error != ERROR_SUCCESS) {
			free(p);
			return FALSE;
		}

		mylog(L"AdjustTokenPrivileges success\r\n");
		free(p);
		return TRUE;
	}

	return FALSE;
}



int loadWindowsHook() {
	int result = 0;
	HMODULE hhook = LoadLibraryW(L"publicFun.dll");
	if (hhook)
	{
		typedef int (WINAPI* ptrInstallLaunchEv)();
		ptrInstallLaunchEv InstallLaunchEv = (ptrInstallLaunchEv)GetProcAddress(hhook, "InstallLaunchEv");
		if (InstallLaunchEv)
		{
			result = InstallLaunchEv();
			mylog(L"InstallLaunchEv result:%d\r\n", result);
		}
	}
	return TRUE;
}




int __wcslen(const WCHAR* str) {
	int i = 0;
	while (str[i]) {
		i++;
	}
	return i;
}


WCHAR* __wcsrchr(const WCHAR* str, WCHAR wc) {
	int len = __wcslen(str);

	for (int i = len - 1; i >= 0; i--)
	{
		if (str[i] == wc)
		{
			return (WCHAR*)str + i;
		}
	}
	return 0;
}

WCHAR* __wcschr(const WCHAR* str, const WCHAR wc) {
	int i = 0;
	while (str[i])
	{
		if (str[i] == wc)
		{
			return (WCHAR*)str + i;
		}
		i++;
	}
	return 0;
}



int __wcscpy(WCHAR* str1, const WCHAR* str2) {
	int len2 = __wcslen(str2);

	memcpy(str1, str2, (len2 + 1) * sizeof(WCHAR));
	return len2;
}


int __wcscat(WCHAR* str1, const  WCHAR* str2) {
	int len1 = __wcslen(str1);
	int len2 = __wcslen(str2);
	memcpy(str1 + len1, str2, (len2 + 1) * sizeof(WCHAR));
	return len1 + len2;
}


int GBKToUTF8(const char* gb2312, char** lpdatabuf)
{
	if (lpdatabuf <= 0)
	{
		return FALSE;
	}
	int needunicodelen = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	if (needunicodelen <= 0)
	{
		*lpdatabuf = 0;

		return FALSE;
	}
	needunicodelen += 1024;
	wchar_t* wstr = new wchar_t[needunicodelen];
	memset(wstr, 0, needunicodelen * 2);
	int unicodelen = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, needunicodelen);
	if (unicodelen <= 0)
	{
		*lpdatabuf = 0;
		delete[] wstr;

		return FALSE;
	}
	*(int*)(wstr + unicodelen) = 0;
	int needutf8len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	if (needutf8len <= 0)
	{

		*lpdatabuf = 0;
		delete[] wstr;
		return FALSE;
	}
	needutf8len += 1024;
	*lpdatabuf = new char[needutf8len];
	memset(*lpdatabuf, 0, needutf8len);
	int utf8len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, *lpdatabuf, needutf8len, NULL, NULL);
	delete[] wstr;
	if (utf8len <= 0)
	{
		delete* lpdatabuf;
		*lpdatabuf = 0;

		return FALSE;
	}

	*(*lpdatabuf + utf8len) = 0;
	return utf8len;
}



int UTF8ToGBK(const char* utf8, char** lpdatabuf)
{
	if (lpdatabuf <= 0)
	{
		return FALSE;
	}
	int needunicodelen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (needunicodelen <= 0)
	{

		*lpdatabuf = 0;
		return FALSE;
	}
	needunicodelen += 1024;
	wchar_t* wstr = new wchar_t[needunicodelen];
	memset(wstr, 0, needunicodelen * 2);
	int unicodelen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, needunicodelen);
	if (unicodelen <= 0)
	{
		*lpdatabuf = 0;
		delete[] wstr;

		return FALSE;
	}
	*(int*)(wstr + unicodelen) = 0;
	int needgbklen = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	if (needgbklen <= 0)
	{
		*lpdatabuf = 0;
		delete[] wstr;

		return FALSE;
	}
	needgbklen += 1024;
	*lpdatabuf = new char[needgbklen];
	memset(*lpdatabuf, 0, needgbklen);

	int gbklen = WideCharToMultiByte(CP_ACP, 0, wstr, -1, *lpdatabuf, needgbklen, NULL, NULL);
	delete[] wstr;
	if (gbklen <= 0)
	{
		delete[](*lpdatabuf);
		*lpdatabuf = 0;

		return FALSE;
	}

	*(*lpdatabuf + gbklen) = 0;
	return gbklen;
}


int dospath2LinkPath(const WCHAR* dospath, WCHAR* linkpath) {

	if (dospath[1] != ':' && dospath[2] != '\\')
	{
		return FALSE;
	}

	lstrcpyW(linkpath, L"\\\\.\\");
	lstrcatW(linkpath, dospath);

	return TRUE;
}



int dospath2NTpath(const WCHAR* dospath, WCHAR* ntpath) {

	if (dospath[1] != ':' && dospath[2] != '\\')
	{
		return FALSE;
	}
	WCHAR drive = dospath[0];

	if (drive >= 'A' && drive <= 'Z')
	{
		drive += 0x20;
	}

	int n = drive - 'c' + 1;

	//int len = wsprintfW(ntpath, L"\\Device\\HarddiskVolume%u\\%ws", n, dospath + 3);

	int len = wsprintfW(ntpath, L"\\Device\\PhysicalDrive%d\\%ws", n-1, dospath + 3);
	return len;
}



int ntpath2dospath(const WCHAR* ntpath, WCHAR* dospath) {

	WCHAR drive = 0;
	if (wmemcmp(ntpath,L"\\Device\\HarddiskVolume", lstrlenW(L"\\Device\\HarddiskVolume")) == 0 )
	{
		int len = lstrlenW(L"\\Device\\HarddiskVolume");

		WCHAR drive = ntpath[len];
		drive = drive - 0x30 + 'C';
		dospath[1] = drive;
		dospath[1] = L':';
		dospath[2] = 0;
		lstrcpyW(dospath, ntpath + len + 1);
		return TRUE;
	}else if (wmemcmp(ntpath, VERACRYPT_VOLUME_DEVICE, lstrlenW(VERACRYPT_VOLUME_DEVICE)) == 0)
	{
		int len = lstrlenW(VERACRYPT_VOLUME_DEVICE);
		WCHAR drive = ntpath[len];
		dospath[0] = drive;
		dospath[1] = L':';
		dospath[2] = 0;
		lstrcpyW(dospath, ntpath + len + 1);
		return TRUE;
	}

	return FALSE;
}
