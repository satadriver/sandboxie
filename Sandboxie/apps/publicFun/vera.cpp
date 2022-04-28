
#include "vera.h"
#include "Utils.h"
#include <io.h>
#include <windows.h>
#include <process.h>
#include "vera.h"
#include <Dbghelp.h>
#include <Shlobj.h>
#include <string>
#include <io.h>
#include "Utils.h"
#include "box.h"
#include "vera.h"

#include "../../common/my_version.h"

#pragma comment(lib,"Shell32.lib")
#pragma comment(lib,"Dbghelp.lib")


WCHAR g_veraDiskFilePath[MAX_PATH] = { 0 };


WCHAR VERACRYPT_PASSWORD[128];

WCHAR* getVeraDiskFile() {
	if (g_veraDiskFilePath[0] == 0)
	{
		findValueInConfig(L"", VERACRYPT_PATH_KEYNAME, g_veraDiskFilePath);
		if (g_veraDiskFilePath[0] == 0)
		{
			setDefaultVeraDiskFile();
			modifyConfigValue(L"", VERACRYPT_PATH_KEYNAME, g_veraDiskFilePath);
		}
	}

	mylog(L"g_veraDiskFilePath:%ws\r\n", g_veraDiskFilePath);
	
	return g_veraDiskFilePath;
}

WCHAR* setVeraDiskFile(const WCHAR * path) {
	lstrcpyW(g_veraDiskFilePath, path);
	lstrcatW(g_veraDiskFilePath, L"\\" VERACRYPT_FILENAME);

	translateQuota(g_veraDiskFilePath);

	char szpath[MAX_PATH] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, g_veraDiskFilePath, -1, szpath, MAX_PATH, 0, 0);
	char* pos = strrchr(szpath, '\\');
	if (pos )
	{
		*(pos + 1) = 0;
	}
	MakeSureDirectoryPathExists(szpath);

	mylog(L"g_veraDiskFilePath:%ws\r\n", g_veraDiskFilePath);

	modifyConfigValue(L"", VERACRYPT_PATH_KEYNAME, g_veraDiskFilePath);
	return g_veraDiskFilePath;
}




WCHAR * setDefaultVeraDiskFile() {
	if (g_veraDiskFilePath[0] == 0)
	{
		int result = 0;
		wchar_t szdir[MAX_PATH] = { 0 };
		//result = GetWindowsDirectoryW(szdir, MAX_PATH);	//without \\ at end
		result = GetModuleFileNameW(0, szdir, MAX_PATH);
		if (result == 0)
		{
			//mylog(L"createBoxVolume GetWindowsDirectoryW errorcode:%d\r\n", GetLastError());
			return FALSE;
		}
		else {
			WCHAR* pos = wcsrchr(szdir, L'\\');
			if (pos )
			{
				*(pos + 1) = 0;
			}
			else {
				pos = szdir;
			}
			lstrcatW(szdir, VERACRYPT_FILENAME);
			lstrcpyW(g_veraDiskFilePath, szdir);
		}
		//result = wsprintfW(g_veraDiskFilePath, L"%lc:\\%ws", szdir[0], VERACRYPT_FILENAME);	//%c is ok also

	}

	return g_veraDiskFilePath;
}




//"VeraCrypt Format.exe" /create "c:\safedesktop.hc" /password SAFEDESKTOP_PASSWORD /hash sha256 /filesystem NTFS /size 1G /dynamic /force /silent
//"VeraCrypt.exe " /volume "c:\safedesktop.hc" /l x:\ /password SAFEDESKTOP_PASSWORD /force /silent /quit 
//"VeraCrypt.exe" /dismount x:\ /force /silent /quit
DWORD  createBoxVolume(int size) {

	DWORD result = 0;

	WCHAR * diskpath = getVeraDiskFile();

	//MessageBoxW(0, diskpath, diskpath, MB_OK);

	result = _waccess(diskpath, 0);		//can not be 06,why?
	if (result == 0)
	{
		//mylog(L"createBoxVolume disk %ws already exist\r\n", VERACRYPT_FILENAME);
		return TRUE;
	}
	else {
		mylog(L"createBoxVolume disk %ws not exist,to create it\r\n", VERACRYPT_FILENAME);
	}

	const wchar_t* format = L"\"VeraCrypt Format.exe\" /create \"%ws\" /password %ws /hash sha256 /filesystem NTFS /size %uG /dynamic /force /silent";
	// "\"VeraCrypt Format.exe\" /create \"%ws\" /password %ws /hash sha512 /encryption serpent /filesystem NTFS /size %uG /dynamic /force /silent";

	wchar_t szparam[1024];
	result = wsprintfW(szparam, format, diskpath, VERACRYPT_PASSWORD, size);
	STARTUPINFOW si = { 0 };
	si.cb = sizeof(STARTUPINFOW);
	PROCESS_INFORMATION pi = { 0 };
	si.lpDesktop = (WCHAR*)L"WinSta0\\Default";
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;
	result = CreateProcessW(0, szparam, 0, 0, 0, dwCreationFlags, 0, 0, &si, &pi);
	int errorcode = GetLastError();
	DWORD threadcode = 0;
	DWORD processcode = 0;
	if (result) {
		if (pi.hProcess)
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			GetExitCodeProcess(pi.hProcess, &processcode);

		}else if (pi.hThread)
		{
			WaitForSingleObject(pi.hThread, INFINITE);
			GetExitCodeThread(pi.hThread, &threadcode);
		}
		
		if (pi.hProcess)
		{
			CloseHandle(pi.hProcess);
		}

		if (pi.hThread)
		{
			CloseHandle(pi.hThread);
		}	
	}
	mylog(L"command:%ws result:%d thread code:%d process code:%d errorcode:%d\r\n", szparam, result,threadcode,processcode, errorcode);
	return (result);
}


DWORD  mountBoxVolume() {

	DWORD result = 0;

	WCHAR* diskpath = getVeraDiskFile();

	result = _waccess(diskpath, 0);	//can not be 06,why?
	if (result != 0)
	{
		mylog(L"mountBoxVolume %ws not found error:%d", diskpath, GetLastError());
		return FALSE;
	}

	result = _waccess(VERACRYPT_DISK_VOLUME, 0);
	if (result == 0)
	{
		//mylog(L"mountBoxVolume disk %ws already exist", VERACRYPT_DISK_VOLUME);
		return TRUE;
	}

	const wchar_t* format = L"\"VeraCrypt.exe\" /volume \"%ws\" /l %ws /password %ws /force /silent /quit";
	wchar_t szparam[1024];
	result = wsprintfW(szparam, format, diskpath, VERACRYPT_DISK_VOLUME, VERACRYPT_PASSWORD);
	STARTUPINFOW si = { 0 };
	si.cb = sizeof(STARTUPINFOW);
	PROCESS_INFORMATION pi = { 0 };
	si.lpDesktop = (WCHAR*)L"WinSta0\\Default";
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;
	result = CreateProcessW(0, szparam, 0, 0, 0, dwCreationFlags, 0, 0, &si, &pi);
	int errorcode = GetLastError();
	DWORD threadcode = 0;
	DWORD processcode = 0;
	if (result) {

		if (pi.hProcess)
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			GetExitCodeProcess(pi.hProcess, &processcode);
		}
		else if (pi.hThread)
		{
			WaitForSingleObject(pi.hThread, INFINITE);
			GetExitCodeThread(pi.hThread, &threadcode);
		}
		
		if (pi.hProcess)
		{
			CloseHandle(pi.hProcess);
		}

		if (pi.hThread)
		{
			CloseHandle(pi.hThread);
		}
	}
	mylog(L"command:%ws result:%d thread code:%d process code:%d errorcode:%d\r\n", szparam, result, threadcode, processcode, errorcode);
	return result;
}




const wchar_t* getVeraBoxPath() {
	int result = 0;
	result = _waccess(VERACRYPT_DISK_VOLUME, 06);	//can not end with \ or /
	if (result == 0)
	{

	}
	else {
		wchar_t szpath[MAX_PATH];
		lstrcpyW(szpath, VERACRYPT_DISK_VOLUME);

		wchar_t szusername[MAX_PATH] = { 0 };
		DWORD len = 0;
		GetUserNameW(szusername, &len);

		lstrcatW(szpath, L"\\Sandbox\\");
		lstrcatW(szpath, szusername);
		lstrcatW(szpath, L"\\" DEFAULT_BOX_NAME L"\\");

		char ascstr[MAX_PATH];
		WideCharToMultiByte(CP_ACP, 0, szpath, -1, ascstr, MAX_PATH, 0, 0);
		result = MakeSureDirectoryPathExists(ascstr);		//MakeSureDirectoryPathExists must end with \ or /
	}

	return VERACRYPT_DISK_VOLUME;
}



//veracrypt.exe /dismount x: /force /silent /quit
DWORD  DismountBoxVolume() {

	DWORD result = 0;

	const wchar_t* format = L"\"VeraCrypt.exe\" /dismount %ws /force /silent /quit";
	wchar_t szparam[1024];
	result = wsprintfW(szparam, format, VERACRYPT_DISK_VOLUME);

	STARTUPINFOW si = { 0 };
	si.cb = sizeof(STARTUPINFOW);
	PROCESS_INFORMATION pi = { 0 };
	result = CreateProcessW(0, szparam, 0, 0, 0, 0, 0, 0, &si, &pi);
	int errorcode = GetLastError();
	if (result) {
		if (pi.hProcess)
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			GetExitCodeProcess(pi.hProcess, &result);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	mylog(L"command:%ws result:%d errorcode:%d\r\n", szparam, result, errorcode);
	return result;
}



int hidedisk(int toggle) {

	DWORD diskmask = 1;
	if (toggle == FALSE)
	{
		diskmask = 0;
	}
	else {
		int shiftbit = VERACRYPT_DISK_VOLUME[0] - L'A';
		diskmask = diskmask << shiftbit;
	}

	int result = 0;
	HKEY key = 0;
	DWORD pos = 0;

#define HIDE_DISK_SUBKEY32		L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"
#define HIDE_DISK_SUBKEY		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\"
#define HIDE_DISK_SUBKEY_ITEM	L"NoDrives"

	if (Is64BitOS() == 1 && IsWow64() == 1) {
		result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, HIDE_DISK_SUBKEY,0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, 0, &key, &pos);
	}
	else if (Is64BitOS() == 0 && IsWow64() == 0)
	{
		result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, HIDE_DISK_SUBKEY,0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, &pos);
	}
	else if (Is64BitOS() == 1 && IsWow64() == 0) {
		result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, HIDE_DISK_SUBKEY,0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, 0, &key, &pos);
	}
	else {
		return FALSE;
	}

	if (result || key == 0)
	{
		//sblog("hidedisk", "mydll", "error", "RegCreateKeyExW");
		return FALSE;
	}

	DWORD v[2] = { 0 };
	unsigned char* data = (unsigned char*)v;
	DWORD type = REG_DWORD;
	DWORD size = sizeof(v);
	result = RegQueryValueExW(key, HIDE_DISK_SUBKEY_ITEM, 0, &type, data, &size);
	if (result == 0)
	{
		if (v[0] == diskmask)
		{
			result = SendMessageW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
			RegCloseKey(key);
			return TRUE;
		}
	}

	v[0] = diskmask;
	result = RegSetValueExW(key, HIDE_DISK_SUBKEY_ITEM, 0, REG_DWORD, data, sizeof(DWORD));

	result = RegCloseKey(key);

	result = SendMessageW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);

	//sblog("hidedisk", "mydll", "success", "RegCreateKeyExW");
	return TRUE;
}





int initVeraDisk(int size) {
	int result = createBoxVolume(size);
	if (result == FALSE)
	{
		mylog("createBoxVolume value:%d\r\n", result);
		return FALSE;
	}

	result = mountBoxVolume();
	if (result == FALSE)
	{
		mylog("mountBoxVolume value:%d\r\n",result);
		return FALSE;
	}

	result = hidedisk(TRUE);
	return result;
}



int removeQutoInPath(const WCHAR* src, WCHAR* dst) {
	int len = lstrlenW(src);
	int i = 0;
	int j = 0;
	for (i = 0, j = 0; i < len; i++)
	{
		if (src[i] == '\"' || src[i] == '\'')
		{

		}
		else {
			dst[j] = src[i];
			j++;
		}
	}
	*(dst + j) = 0;
	return j;
}






extern "C" __declspec(dllexport) int  deleteVeraVolume(const WCHAR * srcfilepath) {

	//__debugbreak();

	WCHAR szparam[1024];

	DWORD result = 0;

	result = _waccess(VERACRYPT_DISK_VOLUME, 0);
	if (result == 0)
	{
		WCHAR filepath[MAX_PATH];

		result = removeQutoInPath(srcfilepath, filepath);

		const WCHAR* format = L"\"%ws\\VeraCrypt.exe\" /dismount %ws /force /silent /quit";

		result = wsprintfW(szparam, format, filepath, VERACRYPT_DISK_VOLUME);

		STARTUPINFOW si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		result = CreateProcessW(0, szparam, 0, 0, 0, 0, 0, 0, &si, &pi);
		if (result) {
			WaitForSingleObject(pi.hProcess, INFINITE);
			GetExitCodeProcess(pi.hThread, &result);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else {
			return FALSE;
		}

		WCHAR diskpath[MAX_PATH];
// 		lstrcpyW(diskpath, filepath);
// 		lstrcatW(diskpath, L"\\" VERACRYPT_FILENAME);
		findValueInConfig(L"", VERACRYPT_PATH_KEYNAME, diskpath);
		result = DeleteFileW(diskpath);

		return result;
	}
	else {
		WCHAR szout[1024];
		wsprintfW(szout, L"access veracrypt volume:%ws error", VERACRYPT_DISK_VOLUME);
		//MessageBox(0, szout, szout, MB_OK);
		//if not run for first time,need not to unmount vera volume and delete vera volume file,so we need to return true
		return TRUE;
	}
}
