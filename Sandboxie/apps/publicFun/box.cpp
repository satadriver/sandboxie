#include "main.h"
#include "box.h"
#include "../../common/my_version.h"
#include "Utils.h"
#include "config.h"
#include "vera.h"
#include <Shlobj.h>
#include <DbgHelp.h>
#include <wchar.h>
#include "apiFuns.h"
#include "../../core/dll/myfile.h"

#pragma comment(lib,"Shell32.lib")

#pragma comment(lib,"DbgHelp.lib")


int getconfigpath(WCHAR* path) {

	int result = 0;

	result = GetWindowsDirectoryW(path, MAX_PATH);

	lstrcatW(path, L"\\" SANDBOXIE_INI);

	return TRUE;
}


int findValueInConfig(const WCHAR* boxname, const WCHAR* keyname, WCHAR * dst) {
	int result = 0;

	WCHAR filepath[MAX_PATH];

	result = getconfigpath(filepath);
	HANDLE hf = CreateFileW(filepath, GENERIC_READ , FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hf == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	LONG high = 0;
	int size = GetFileSize(hf, (DWORD*)&high);
	WCHAR* data = (WCHAR*)new char[(ULONGLONG)size + 0x1000];
	if (data <= 0)
	{
		CloseHandle(hf);
		return FALSE;
	}
	DWORD cnt = 0;
	result = ReadFile(hf, data, size, &cnt, 0);
	CloseHandle(hf);
	if (result == 0)
	{
		delete []data;
		return FALSE;
	}

	WCHAR* boxdatapos = 0;
	if (boxname[0])
	{
		boxdatapos = wcsstr(data, boxname);
	}
	else {
		boxdatapos = wcsstr(data, L"[GlobalSettings]");
	}

	if (boxdatapos <= 0)
	{
		delete []data;
		return FALSE;
	}
	else {
		boxdatapos = wcsstr(boxdatapos, L"\r\n");
		boxdatapos += 2;
	}

	int ptr = 0;
	WCHAR* pos = wcsstr(boxdatapos, keyname);
	if (pos > 0)
	{
		WCHAR * hdr = wcschr(pos, L'=');
		if (hdr)
		{
			hdr++;
			WCHAR* end = wcsstr(hdr, L"\r\n");
			if (end )
			{
				int len = (int)(end - hdr);
				wmemcpy(dst, hdr, len);
				dst[len] = 0;

				ptr =(int)(hdr - data);
			}
		}
	}

	delete []data;
	return ptr;
}



int findBoxNames(WCHAR* buf) {
	int result = 0;

	WCHAR filepath[MAX_PATH];

	result = getconfigpath(filepath);
	HANDLE hf = CreateFileW(filepath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hf == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	LONG high = 0;
	int size = GetFileSize(hf, (DWORD*)&high);
	WCHAR* data = (WCHAR*)new char[(ULONGLONG)size + 0x1000];
	if (data <= 0)
	{
		CloseHandle(hf);
		return FALSE;
	}
	DWORD cnt = 0;
	result = ReadFile(hf, data, size, &cnt, 0);
	CloseHandle(hf);
	if (result == 0)
	{
		delete[]data;
		return FALSE;
	}
	data[size / sizeof(WCHAR)] = 0;

	const WCHAR* globalsettings = L"[GlobalSettings]";

	const WCHAR* usersettings = L"[UserSettings_";

	WCHAR* dst = buf;
	int total = 0;

	WCHAR* hdr = data;
	while ((ULONGLONG)hdr < (ULONGLONG)data + size)
	{
		hdr = wcschr(hdr, L'[');
		if (hdr)
		{
			if (wmemcmp(hdr, globalsettings, wcslen(globalsettings)) == 0)
			{
				hdr += wcslen(globalsettings);
				mylog(L"get globalsettings\r\n");
			}
			else if (wmemcmp(hdr, usersettings, wcslen(usersettings)) == 0)
			{
				mylog(L"get usersettings\r\n");

				hdr += wcslen(usersettings);
				hdr = wcschr(hdr, L']');
				if (hdr == 0)
				{
					break;
				}
				else {
					hdr++;
				}
			}
			else {
				hdr++;
				WCHAR* end = wcschr(hdr, L']');
				if (end)
				{
					int len = (int)(end - hdr);
					wmemcpy(dst, hdr, len);
					dst[len] = 0;
					mylog(L"find box name:%ws\r\n", dst);
					dst += 34;
					total++;
					hdr = end + 1;
				}
				else {
					break;
				}
			}
		}
		else {
			break;
		}
	}

	delete[]data;
	return total;
}




int modifyConfigValue(const WCHAR* boxname, const WCHAR* keyname, const WCHAR* value) {

	int result = 0;

	int valuesize = lstrlenW(value);

	WCHAR filepath[MAX_PATH];
	result = getconfigpath(filepath);
	HANDLE hf = CreateFileW(filepath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hf == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	LONG high = 0;
	int size = GetFileSize(hf, (DWORD*)&high);
	WCHAR* data = (WCHAR*)new char[(ULONGLONG)size + 0x1000];
	if (data <= 0)
	{
		CloseHandle(hf);
		return FALSE;
	}
	DWORD cnt = 0;
	result = ReadFile(hf, data, size, &cnt, 0);
	if (result == 0)
	{
		delete []data;
		CloseHandle(hf);
		return FALSE;
	}

	WCHAR* boxdatapos = 0;
	if (boxname[0])
	{
		boxdatapos = wcsstr(data, boxname);
	}
	else {
		boxdatapos = wcsstr(data, L"[GlobalSettings]");
	}

	if (boxdatapos <= 0)
	{
		delete []data;
		CloseHandle(hf);
		return FALSE;
	}
	else {
		boxdatapos = wcsstr(boxdatapos, L"\r\n");
		boxdatapos += 2;
	}

	WCHAR* pos = wcsstr(boxdatapos, keyname);
	if (pos > 0)
	{
		WCHAR* hdr = wcschr(pos, L'=');
		if (hdr)
		{
			hdr++;

			high = 0;
			result = SetFilePointer(hf, (LONG)(hdr - data) * sizeof(WCHAR), &high, FILE_BEGIN);
			result = WriteFile(hf, value, valuesize * sizeof(WCHAR), &cnt, 0);

			WCHAR* end = wcsstr(hdr, L"\r\n");
			if (end)
			{
				int leastlen = (int)(size - (end - data) * sizeof(WCHAR)) ;
				result = WriteFile(hf, end, leastlen , &cnt, 0);
			}
		}
	}
	else {
		WCHAR kv[1024];
		int kvlen = wsprintfW(kv, L"%ws=%ws\r\n", keyname, value);
		high = 0;
		result = SetFilePointer(hf, (LONG)(boxdatapos - data)*sizeof(WCHAR), &high, FILE_BEGIN);
		result = WriteFile(hf, kv, kvlen * sizeof(WCHAR), &cnt, 0);

		int leastlen = (int)(size - (boxdatapos - data) * sizeof(WCHAR));
		result = WriteFile(hf, boxdatapos, leastlen, &cnt, 0);
	}
	SetEndOfFile(hf);
	FlushFileBuffers(hf);
	CloseHandle(hf);
	delete[]data;

	result = LjgApi_ReloadConf(-1,0);
	//result = reloadbox();
	return TRUE;
}

WCHAR * getBoxSection(const WCHAR* boxname,WCHAR ** hdr,WCHAR ** body, WCHAR ** end) {

	WCHAR* data = 0;
	int size = 0;

	int result = 0;

	WCHAR filepath[MAX_PATH];

	result = getconfigpath(filepath);

	result = fileReaderW(filepath, &data, &size);
	if (result == 0)
	{
		return FALSE;
	}

	WCHAR flag[MAX_PATH];

	int len = wsprintfW(flag, L"[%ws]\r\n", boxname);

	WCHAR* pos = wcsstr(data, flag);
	if (pos > 0)
	{
		*hdr = pos;

		pos += len;
		*body = pos;

		pos = wcsstr(pos, SECTION_START_FLAG);
		if (pos > 0)
		{
			*end = pos;
			return data;
		}
		else {
			*end = data + size;
			return data;
		}
	}

	delete data;
	return FALSE;
}

int deletebox(const WCHAR* boxname) {
	int result = 0;

	WCHAR* hdr = 0;
	WCHAR* end = 0;
	WCHAR* body = 0;
	WCHAR * data = getBoxSection(boxname,&hdr,&body,&end);
	if (data)
	{
		int besidesize = (int)lstrlenW(end) + (int)(hdr - data);
		lstrcpyW(hdr, end);
		WCHAR filename[MAX_PATH];
		result = getconfigpath(filename);
		result = fileWriterW(filename, data, besidesize,FALSE);
		delete data;

		result = LjgApi_ReloadConf(-1, 0);
		//result = reloadbox();
		return result;
	}

	return FALSE;
}


int createRecycleFolderInBox(const WCHAR* boxname) {
	int result = 0;

	WCHAR username[MAX_PATH];
	DWORD usernamelen = MAX_PATH;
	result = GetUserNameW(username, &usernamelen);

	WCHAR cfgpath[MAX_PATH];
	lstrcpyW(cfgpath, VERACRYPT_DISK_VOLUME);
	lstrcatW(cfgpath, boxname);
	lstrcatW(cfgpath, L"\\");
	lstrcatW(cfgpath, username);
	lstrcatW(cfgpath, VERAVOLUME_RECYCLE_INFO_PATH);		//without last slash,this function will create path without last folder 

	char szbuf[MAX_PATH];
	int len = WideCharToMultiByte(CP_ACP, 0, cfgpath, -1, szbuf, MAX_PATH, 0, 0);
	if (len > 0)
	{
		szbuf[len] = 0;
		result = MakeSureDirectoryPathExists(szbuf);
	}

	WCHAR recyclepath[MAX_PATH];
	lstrcpyW(recyclepath, VERACRYPT_DISK_VOLUME);
	lstrcatW(recyclepath, boxname);
	lstrcatW(recyclepath, L"\\");
	lstrcatW(recyclepath, username);
	lstrcatW(recyclepath, VERAVOLUME_RECYCLE_PATH);
	len = WideCharToMultiByte(CP_ACP, 0, recyclepath, -1, szbuf, MAX_PATH, 0, 0);
	if (len > 0) {
		szbuf[len] = 0;
		result = MakeSureDirectoryPathExists(szbuf);
	}


	WCHAR copypath[MAX_PATH];
	lstrcpyW(copypath, VERACRYPT_DISK_VOLUME);
	lstrcatW(copypath, boxname);
	lstrcatW(copypath, L"\\");
	lstrcatW(copypath, username);
	lstrcatW(copypath, VERAVOLUME_RECYCLE_COPY_PATH);
	len = WideCharToMultiByte(CP_ACP, 0, copypath, -1, szbuf, MAX_PATH, 0, 0);
	if (len > 0) {
		szbuf[len] = 0;
		result = MakeSureDirectoryPathExists(szbuf);
	}
	
	return result;
}


int reloadbox() {
	int result = 0;
	//why call "start.exe /reload" have no effection?
	typedef LONG(*ptrfun)(ULONG session_id, ULONG flags);

	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	ptrfun fun = (ptrfun)GetProcAddress(h, "SbieApi_ReloadConf");
	if (fun)
	{
		result = fun(-1, 0);
		if (result == 0)
		{
			result = TRUE;
		}
		mylog(L"SbieApi_ReloadConf result %d", result);
	}
	else {
		mylog(L"get SbieApi_ReloadConf in dll error");
	}

	return result;
}

int createbox(const WCHAR* boxname) {
	int result = 0;

	WCHAR* hdr = 0;
	WCHAR* end = 0;
	WCHAR* body = 0;
	WCHAR* data = getBoxSection(boxname, &hdr, &body, &end);
	if (data)
	{
		mylog(L"box:%ws already exist\r\n",boxname);
		delete data;
		return TRUE;
	}

	data = getBoxSection(DEFAULT_BOX_NAME, &hdr,&body, &end);
	if (data)
	{
		WCHAR buf[BOX_CONFIG_BUF_SIZE];
		int offset = wsprintfW(buf, L"[%ws]\r\n", boxname);

		int size = (int)(end - body);

		RtlCopyMemory(buf + offset, body, size * sizeof(WCHAR));
		buf[size+offset] = 0;

		WCHAR filename[MAX_PATH];
		result = getconfigpath(filename);
		result = fileWriterW(filename, buf, size + offset, TRUE);
		delete data;

		result = LjgApi_ReloadConf(-1, 0);
		//result = reloadbox();
	
		return result;
	}

	return FALSE;
}


int setVeraPath(const WCHAR* path) {
	int result = 0;

	WCHAR* hdr = 0;
	WCHAR* end = 0;
	WCHAR* body = 0;
	WCHAR * data = getBoxSection(L"[GlobalSettings]", &hdr, &body, &end);
	if (data)
	{
		WCHAR* pos = wcsstr(body, L"FileRootPath=");
		if (pos == 0)
		{
			WCHAR buf[BOX_CONFIG_BUF_SIZE];
			int len = wsprintfW(buf, L"FileRootPath=%ws\r\n", path);

			WCHAR filename[MAX_PATH];
			result = getconfigpath(filename);

			result = fileWriterW(filename, data, (int)(end - data), 0);

			result = fileWriterW(filename, buf, len, TRUE);

			int endlen = lstrlenW(end);
			result = fileWriterW(filename, end, endlen, TRUE);

			result = LjgApi_ReloadConf(-1, 0);
			//result = reloadbox();
		}

		delete data;
		
		return result;
	}

	return FALSE;
}


int enumSandboxNames(WCHAR * boxnames) {
	WCHAR* name = boxnames;
	int index = -1;
	int cnt = 0;
	while (1) {
		index = SbieApi_EnumBoxesEx(index, name, TRUE);
		if (index == -1) {
			break;
		}
		LONG rc = SbieApi_IsBoxEnabled(name);
		if (rc == STATUS_ACCOUNT_RESTRICTION || rc == TRUE) {
			cnt++;
			
			mylog(L"get boxname:%ws\r\n", name);
			name += 34;
		}
	}

	mylog(L"get boxname %d\r\n", cnt);
	return cnt;
}




extern "C" __declspec(dllexport) int reloadboxInCmd() {
	wchar_t szparam[1024];

	int len = wsprintfW(szparam, L"%ws /reload", START_EXE);

	STARTUPINFOW si = { 0 };
	si.cb = sizeof(STARTUPINFOW);
// 	si.lpDesktop = (WCHAR*)L"WinSta0\\Default";
// 	si.dwFlags = STARTF_USESHOWWINDOW;
// 	si.wShowWindow = SW_HIDE;
// 	DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;

	PROCESS_INFORMATION pi = { 0 };
	//DWORD result = CreateProcessW(0, szparam, 0, 0, 0, dwCreationFlags, 0, 0, &si, &pi);
	DWORD result = CreateProcessW(0, szparam, 0, 0, 0, 0, 0, 0, &si, &pi);
	int errorcode = GetLastError();
	if (result) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &result);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	mylog(L"command:%ws result:%d errorcode:%d\r\n", szparam, result, errorcode);
	return result ;
}



int runDesktopBox(const wchar_t* boxname) {
	int result = 0;
	wchar_t szparam[1024];

// 	WCHAR curpath[MAX_PATH];
// 	GetModuleFileNameW(0, curpath, MAX_PATH);
// 	WCHAR* pos = wcsrchr(curpath, L'\\');
// 	if (pos)
// 	{
// 		*(pos + 1) = 0;
// 	}
// 	lstrcatW(curpath, L"mingw" SANDBOXIE_DESKTOPEXE);
//	int len = wsprintfW(szparam, L"%ws %ws", curpath, boxname);
	int len = wsprintfW(szparam, L"%ws %ws", SANDBOXIE_DESKTOPEXE, boxname);
	result = IsUserAnAdmin();
	if (result)
	{
		result = createProcessAsExplorer(szparam);
	}
	else {
		result = commandline(szparam, FALSE,SW_SHOW);
	}

	return result;
}




extern "C" __declspec(dllexport) int setUsername(const WCHAR * username) {
	
	return LjgApi_SetUsername(username);
}
extern "C" __declspec(dllexport) int getUsername( WCHAR * username) {
	return LjgApi_getUsername(username);
}


extern "C" __declspec(dllexport) int setPassword(const WCHAR * pass) {
	return LjgApi_SetPassword(pass);
}
extern "C" __declspec(dllexport) int getPassword( WCHAR * pass) {
	return LjgApi_getPassword(pass);

}
