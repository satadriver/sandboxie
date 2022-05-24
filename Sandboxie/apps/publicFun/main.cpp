
#pragma warning(disable:4005)
#pragma warning(disable: 4996)
#include <windows.h>
#include <Shlobj.h>
#include <stdio.h>
#include <io.h>
#include <process.h>
#include <Dbghelp.h>

#include "../../common/my_version.h"
#include "main.h"

#include "Utils.h"
#include "vera.h"
#include "hook.h"

#include <wchar.h>
#include <windows.h>
#include <corecrt_wstring.h>
#include <Shlwapi.h>

#include "apiFuns.h"
#include "box.h"

#include "../../core/dll/myfile.h"

#include "configStrategy.h"

#pragma comment(lib,"Shell32.lib")
#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"shlwapi.lib")


#define GET_BOXPATH_SYMBOLINK	0
#define GET_BOXPATH_DEVICE		1
#define CREATE_BOX_SUBPATH		2


const WCHAR* g_filename_sufix[] =
{ L".html",L".htm",L".docx",L".doc",L".xls",L".xlsx",L".ppt",L".pptx",L".rtf",L".txt",L".bmp",L".jpg",L".jpeg",L".png",0 };

int g_safedesktopRunning = 0;



BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call,LPVOID lpReserved )
{
    switch (ul_reason_for_call)
    {
		case DLL_PROCESS_ATTACH: {
			g_dllModule = hModule;

			break;
		}
		case DLL_THREAD_ATTACH: {
			break;
		}
		case DLL_THREAD_DETACH:
		{
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			closeDeviceHandle();
			break;
		} 
    }
    return TRUE;
}




int __stdcall WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	return 0;
}



DWORD startBoxProcess(const wchar_t* runfilename,const wchar_t* boxname,int isadmin) {

	int result = 0;

	wchar_t szparam[1024];

	wchar_t filepath[MAX_PATH];

	int filetype = FALSE;

	int uew = FALSE;

	wchar_t filename[MAX_PATH];

	mylog(L"startBoxProcess:%ws", runfilename);

	int runfilenamelen = lstrlenW(runfilename);

	if (runfilename[0] == '\"' && runfilename[runfilenamelen - 1] == '\"') {
		lstrcpyW(filename, runfilename + 1);
		filename[runfilenamelen - 2] = 0;
	}
	else {
		lstrcpyW(filename, runfilename);
	}
	translateQuota(filename);
	_wcslwr(filename);

	int filepathlen = lstrlenW(filename);

	if (wmemcmp(filename + filepathlen - 4, L".txt", 4) == 0)
	{
		filetype = FILE_TYPE_TEXT;
	}
	else if (wmemcmp(filename + filepathlen - 4, L".bmp", 4) == 0 || wmemcmp(filename + filepathlen - 4, L".jpg", 4) == 0 ||
		wmemcmp(filename + filepathlen - 4, L".png", 4) == 0 || wmemcmp(filename + filepathlen - 5, L".jpeg", 5) == 0)
	{
		//filetype = FILE_TYPE_PIC;     // 图片类型和视频类型已在 ModifyCmdLinePassUwpApp() 中处理
	}
	else if (wmemcmp(filename + filepathlen - 5, L".html", 5) == 0 || wmemcmp(filename + filepathlen - 4, L".htm", 4) == 0 )
	{
		filetype = FILE_TYPE_HTML;
	}
	else {

	}

	int version = 0;
	if (GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrFastFailInLoaderCallout")) {
		version = 10;
	}

	if (lstrcmpiW(filename,MYCOMPUTER_CLID) == 0)
	{
		wsprintfW(filepath, L"\"%ws\" \"%ws\" \"%ws\"", EXPLORER_PROCESS_NAME_PLUSPLUS, L"mycomputer", boxname);
	}
	else if (lstrcmpiW(filename, MYRECYCLE_CLID) == 0)
	{
		wsprintfW(filepath, L"\"%ws\" \"%ws\" \"%ws\"", EXPLORER_PROCESS_NAME_PLUSPLUS, L"recycle", boxname);
	}
	else {
		int attribute = GetFileAttributesW(filename);
		if (attribute & FILE_ATTRIBUTE_DIRECTORY)
		{
			//wsprintfW(tmppath, L"\"%ws\" \"/e,%ws\"", L"explorer.exe", filepath);
			wsprintfW(filepath, L"\"%ws\" \"%ws\" \"%ws\"", EXPLORER_PROCESS_NAME_PLUSPLUS, L"folder", boxname);
		}
		else if ((attribute & FILE_ATTRIBUTE_ARCHIVE) || (attribute & FILE_ATTRIBUTE_NORMAL)) {
			wchar_t* fn = wcsrchr((WCHAR*)filename, L'\\');
			if (fn)
			{
				fn++;
			}
			else {
				fn = (WCHAR*)filename;
			}
			int fnlen = (int)lstrlenW(fn);

			BOOL specialfile = FALSE;
			for (int i = 0; i < sizeof(g_filename_sufix) / sizeof(WCHAR*); i++)
			{
				int sufixlen = lstrlenW(g_filename_sufix[i]);
				if (g_filename_sufix[i] && lstrcmpiW(fn + fnlen - sufixlen, g_filename_sufix[i]) == 0)
				{
					specialfile = TRUE;
					WCHAR defaultopen[MAX_PATH] = { 0 };
					defaultopen[0] = 0;

					DWORD bufsize = sizeof(defaultopen) / sizeof(WCHAR);

					result = AssocQueryStringW(ASSOCF_NOFIXUPS | ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, g_filename_sufix[i], NULL, defaultopen, &bufsize);
					if (defaultopen[0] == 0)
					{
						mylog(L"AssocQueryStringW error");
					}
					else {
						mylog(L"AssocQueryStringW:%ws", defaultopen);

						if (wmemcmp(defaultopen + 1, L":\\Program Files\\WindowsApps", lstrlenW(L":\\Program Files\\WindowsApps")) == 0)
						{
							uew = TRUE;
						}
						else if (wmemcmp(defaultopen + 1, L":\\Program Files (x86)\\WindowsApps\\", lstrlenW(L":\\Program Files (x86)\\WindowsApps\\")) == 0)
						{
							uew = TRUE;
						}
					}

					if (uew)
					{
						if (filetype == FILE_TYPE_TEXT)
						{
							wsprintf(filepath, L"\"%s\" \"%s\"", L"c:\\windows\\system32\\notepad.exe", filename);
						}
						else if (filetype == FILE_TYPE_PIC)
						{
							//"rundll32.exe shimgvw.dll,ImageView_Fullscreen" "C:\Program Files (x86)\GM3000\res\Finger\f1.bmp"
							wsprintf(filepath, L"\"%s\" \"%s\"", L"c:\\windows\\system32\\mspaint.exe", filename);
							//wsprintf(filepath, L"\"rundll32.exe\" \"shimgvw.dll,ImageView_Fullscreen \" \"%ws\"", filename);
						}
						else if (filetype == FILE_TYPE_HTML)
						{
							wsprintf(filepath, L"\"%s\"", filename);
						}
						else {
							wsprintf(filepath, L"\"%s\" \"%s\"", defaultopen, filename);
						}
					}
					else {
						wsprintf(filepath, L"\"%s\"", filename);
					}

					//Start.exe /box:ljg_hello  rundll32.exe shimgvw.dll,ImageView_Fullscreen  C:\ProgramData\Microsoft\User Account Pictures\user.bmp
					if (version >= 10 && filetype == FILE_TYPE_PIC)
					{
						wsprintf(filepath, L"\"%s\" \"%s\"", L"c:\\windows\\system32\\mspaint.exe", filename);
						//wsprintf(filepath, L"\"rundll32.exe shimgvw.dll,ImageView_Fullscreen %ws\"", filename);
						//mylog("[LYSM] filepath[2]:%S ", filepath);
					}
					else if (version >= 10 && filetype == FILE_TYPE_HTML)
					{
						wsprintf(filepath, L"\"%s\" \"%s\"", L"C:\\Program Files\\Internet Explorer\\iexplore.exe", filename);
					}

					break;
				}
			}

			if (specialfile == FALSE)
			{
				wsprintf(filepath, L"\"%s\"", filename);
			}
		}
		else {
			mylog(L"file attribute:%d error\r\n", attribute);
			return FALSE;
		}
	}

	WCHAR flag_admin[16] = { 0 };
	if (isadmin)
	{
		lstrcpyW(flag_admin, L"/elevate");
	}

	wsprintfW(szparam, L"%ws /box:%ws %ws %ws", START_EXE, boxname, flag_admin, filepath);
	
	STARTUPINFOW si = { 0 };
//	si.cb = sizeof(STARTUPINFOW);
// 	si.lpDesktop = (WCHAR*)L"WinSta0\\Default";
// 	si.dwFlags = STARTF_USESHOWWINDOW;
// 	si.wShowWindow = SW_SHOW;
//	DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;

	PROCESS_INFORMATION pi = { 0 };
	DWORD processcode = 0;
	DWORD threadcode = 0;
	//result = CreateProcessW(0, szparam, 0, 0, 0, dwCreationFlags, 0, 0, &si, &pi);
	result = CreateProcessW(0, szparam, 0, 0, 0, 0, 0, 0, &si, &pi);

	int errorcode = GetLastError();
	if (result) {
		//WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeThread(pi.hProcess, &threadcode);
		GetExitCodeProcess(pi.hProcess, &processcode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	mylog(L"command:%ws result:%d process code:%d thread code:%d errorcode:%d\r\n", szparam, result,processcode,threadcode, errorcode);
	return result ;
}



DWORD  openMycomputer(const wchar_t* boxname) {

	//explorer.exe /e,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}
	//DWORD result = startBoxProcess(EXPLORER_PROCESS_NAME_PLUSPLUS,boxname,FALSE);
	DWORD result = startBoxProcess(MYCOMPUTER_CLID, boxname, FALSE);
	return result;
}


//L"\" /env:00000000_SBIE_CURRENT_DIRECTORY=\"D:\\project\\Sandboxie-master-plus\\Sandboxie-master\\Sandboxie\\apps\\start\" /env:=Refresh "
 DWORD  openRecycle(const wchar_t* boxname) {
	DWORD result = startBoxProcess(MYRECYCLE_CLID, boxname, FALSE);

// 	 WCHAR command[MAX_PATH];
// 	 wsprintfW(command, L"\"%ws\" \"%ws\" \"%ws\"", EXPLORER_PROCESS_NAME_PLUSPLUS, L"recycle", boxname);
// 	 DWORD result = startBoxProcess(command, boxname,FALSE);

 	 return result;
}

 DWORD stopBoxProcess(BOOL all) {
	 wchar_t szparam[1024];
	 if (all)
	 {
		 int len = wsprintfW(szparam, L"%ws /terminate_all", START_EXE);
	 }
	 else {
		 int len = wsprintfW(szparam, L"%ws /terminate", START_EXE);
	 }

	 STARTUPINFOW si = { 0 };
	 PROCESS_INFORMATION pi = { 0 };

	 DWORD processcode = 0;
	 DWORD threadcode = 0;
	 DWORD result = CreateProcessW(0, szparam, 0, 0, 0, 0, 0, 0, &si, &pi);
	 int errorcode = GetLastError();
	 if (result) {
		 WaitForSingleObject(pi.hProcess, INFINITE);
		 GetExitCodeThread(pi.hProcess, &threadcode);
		 GetExitCodeProcess(pi.hProcess, &processcode);
		 CloseHandle(pi.hProcess);
		 CloseHandle(pi.hThread);
	 }
	 mylog(L"command:%ws result:%d process code:%d thread code:%d errorcode:%d\r\n", szparam, result, processcode, threadcode, errorcode);
	 return result;
 }



int closeBox(const WCHAR* boxname) {
	 int result = 0;
	 HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	 if (h)
	 {
		 typedef BOOLEAN(*ptrSbieDll_KillAll)(ULONG SessionId, const WCHAR* BoxName);
		 ptrSbieDll_KillAll lpSbieDll_KillAll = (ptrSbieDll_KillAll) GetProcAddress(h, "SbieDll_KillAll");
		 if (lpSbieDll_KillAll)
		 {
			result = lpSbieDll_KillAll(-1, boxname);
			mylog(L"SbieDll_KillAll box:%ws",boxname);
		 }
		 FreeLibrary(h);
	 }

	 WCHAR* format = (WCHAR*)L"taskkill /f /t /im %ws";
	 WCHAR cmd[1024];
	 result = wsprintfW(cmd, format, EXPLORER_PROCESS_NAME_PLUSPLUS);

	 result = commandline(cmd, TRUE, SW_HIDE);

	 //result = DismountBoxVolume();

	return result;
}









DWORD createBoxInCtrl(const wchar_t* boxname) {
	wchar_t szparam[1024];
	int len = wsprintfW(szparam,L"%ws -createBox %ws",SBIECTRL_EXE, boxname);

	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	DWORD processcode = 0;
	DWORD threadcode = 0;
	DWORD result = CreateProcessW(0, szparam, 0, 0, 0, 0, 0, 0, &si, &pi);
	int errorcode = GetLastError();
	if (result) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeThread(pi.hProcess, &threadcode);
		GetExitCodeProcess(pi.hProcess, &processcode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	mylog(L"command:%ws result:%d process code:%d thread code:%d errorcode:%d\r\n", szparam, result, processcode, threadcode, errorcode);
	return result ;
}



DWORD setBoxWorkpathInCtrl(const wchar_t* workpath) {

	DWORD result = 0;

	result = _waccess(workpath, 06);
	if (result != 0)
	{
		return -1;
	}

	wchar_t szparam[1024];

	int len = wsprintfW(szparam,L"%ws -setBoxWorkpath \"%ws\"", SBIECTRL_EXE,workpath);

	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	DWORD processcode = 0;
	DWORD threadcode = 0;
	result = CreateProcessW(0, szparam, 0, 0, 0, 0, 0, 0, &si, &pi);
	int errorcode = GetLastError();
	if (result) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeThread(pi.hProcess, &threadcode);
		GetExitCodeProcess(pi.hProcess, &processcode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	mylog(L"command:%ws result:%d process code:%d thread code:%d errorcode:%d\r\n", szparam, result, processcode, threadcode, errorcode);
	return result ;
}



DWORD moveFileIntoRecycleInCtrl(const wchar_t * filename) {

	wchar_t szparam[1024];

	int len = wsprintfW(szparam, L"%ws %ws -delFileIntoRecycle", START_EXE, SBIECTRL_EXE);

	STARTUPINFOW si = { 0 };

	PROCESS_INFORMATION pi = { 0 };

	DWORD processcode = 0;
	DWORD threadcode = 0;
	DWORD result = CreateProcessW(0, szparam, 0, 0, 0, 0, 0, 0, &si, &pi);
	int errorcode = GetLastError();
	if (result) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeThread(pi.hProcess, &threadcode);
		GetExitCodeProcess(pi.hProcess, &processcode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	mylog(L"command:%ws result:%d process code:%d thread code:%d errorcode:%d\r\n", szparam, result, processcode, threadcode, errorcode);
	return result;
}








int commandline(WCHAR* szparam,int wait,int show) {
	int result = 0;
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	DWORD processcode = 0;
	DWORD threadcode = 0;

	si.cb = sizeof(STARTUPINFOW);
	si.lpDesktop = (WCHAR*)L"WinSta0\\Default";
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = show;
	DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;

	result = CreateProcessW(0, szparam, 0, 0, 0, 0, 0, 0, &si, &pi);
	int errorcode = GetLastError();
	if (result) {
		if (wait)
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			GetExitCodeThread(pi.hProcess, &threadcode);
			GetExitCodeProcess(pi.hProcess, &processcode);
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	mylog(L"command:%ws result:%d process code:%d thread code:%d errorcode:%d\r\n", szparam, result, processcode, threadcode, errorcode);
	return result;
}




int translatePath(const WCHAR* filename,WCHAR * filepath) {
	int veravl = (int)wcslen(VERACRYPT_VOLUME_DEVICE);
	if (wmemcmp((wchar_t*)filename, VERACRYPT_VOLUME_DEVICE, veravl) == 0)
	{
		filepath[0] = filename[veravl];
		filepath[1] = ':';
		filepath[2] = '\\';
		filepath[3] = 0;
		wcscpy(filepath, filename + veravl + 2);
	}
	else {
		wcscpy(filepath, filename);
	}
	return TRUE;
}


#define MY_DELETE_FILE 1

int traversalPath(WCHAR* path,int flag) {

	int result = 0;

	int pathlen = lstrlenW(path);
	if (path[pathlen - 1] == L'\\')
	{

	}
	else {
		lstrcatW(path, L"\\");
	}

	const WCHAR* allfiles = L"*.*";
	WCHAR searchpath[MAX_PATH];
	lstrcpyW(searchpath, path);
	lstrcatW(searchpath, allfiles);
	int cnt = 0;

	WIN32_FIND_DATAW finddata;
	HANDLE hf = FindFirstFileW(searchpath, &finddata);
	if (hf == INVALID_HANDLE_VALUE)
	{
		mylog(L"traversalPath FindFirstFileW:%ws all files result:%d,error code:%x", searchpath, result, GetLastError());
		return FALSE;
	}

	
	do 
	{
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (lstrcmpiW(finddata.cFileName,L".") == 0 || lstrcmpiW(finddata.cFileName, L"..") == 0)
			{
				
			}
			else {
				WCHAR nextpath[MAX_PATH];
				lstrcpyW(nextpath, path);
				lstrcatW(nextpath, finddata.cFileName);
				
				cnt += traversalPath(nextpath,flag);

				mylog(L"traversalPath traversalPath:%ws all files result:%d,error code:%x", nextpath, result, GetLastError());
			}
		}else if (finddata.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
		{
			if (flag == MY_DELETE_FILE)
			{
				WCHAR filename[MAX_PATH];
				lstrcpyW(filename, path);
				lstrcatW(filename, finddata.cFileName);
				result = DeleteFileW(filename);

				mylog(L"traversalPath DeleteFileW:%ws all files result:%d,error code:%x", filename, result, GetLastError());
			}
			cnt++;
		}

		result = FindNextFileW(hf, &finddata);
	} while (result);

	FindClose(hf);
	return cnt;
}


int emptyRecycleFiles(const WCHAR* boxname) {
	int result = 0;
	WCHAR username[MAX_PATH];
	DWORD usernamelen = MAX_PATH;
	result = GetUserNameW(username, &usernamelen);

	WCHAR recyclepath[MAX_PATH];
	lstrcpyW(recyclepath, VERACRYPT_DISK_VOLUME);
	lstrcatW(recyclepath, boxname);
	lstrcatW(recyclepath, L"\\");
	lstrcatW(recyclepath, username);
	lstrcatW(recyclepath, VERAVOLUME_RECYCLE_PATH);
	result = traversalPath(recyclepath, MY_DELETE_FILE);

	//WCHAR cmd[1024];
	//wsprintfW(cmd, L"cmd /c RMDIR /s /q %ws", recyclepath);
	//ShellExecuteW(0, L"open", cmd, 0, 0, SW_HIDE);

	WCHAR cfgpath[MAX_PATH];
	lstrcpyW(cfgpath, VERACRYPT_DISK_VOLUME);
	lstrcatW(cfgpath, boxname);
	lstrcatW(cfgpath, L"\\");
	lstrcatW(cfgpath, username);
	lstrcatW(cfgpath, VERAVOLUME_RECYCLE_INFO_PATH);		//without last slash,this function will create path without last folder 

	//wsprintfW(cmd, L"cmd /c RMDIR /s /q %ws", cfgpath);
	//ShellExecuteW(0, L"open", cmd, 0, 0, SW_HIDE);
	result = traversalPath(cfgpath, MY_DELETE_FILE);

	mylog(L"emptyRecycleFiles:%ws all files result:%d,error code:%x",boxname, result, GetLastError());
	return 0;
}










int restoreRecycleFiles(const WCHAR* filepath) {

	int result = 0;

	//__debugbreak();

	HMODULE hdll = LoadLibraryW(SBIEDLL L".dll");
	if (hdll == 0)
	{
		mylog(L"LoadLibraryW SfDeskDll error\r\n");
		return FALSE;
	}

	typedef int (*ptrdelfile)(const WCHAR* filename);
	ptrdelfile delfile = (ptrdelfile)GetProcAddress(hdll, "SbieApi_DeleteFile");

	typedef int (*ptrcopyfile)(const WCHAR* srcpath,const WCHAR * dstpath);
	ptrcopyfile copyfile = (ptrcopyfile)GetProcAddress(hdll, "SbieApi_CopyFile");

	WCHAR* p = wcsstr((WCHAR*)filepath, VERAVOLUME_RECYCLE_PATH);
	if (p == 0)
	{
		return FALSE;
	}

	WCHAR infofile[MAX_PATH];
	int len = (int)(p - filepath);
	wmemcpy(infofile, filepath, len);
	infofile[len] = 0;
	wcscat(infofile, VERAVOLUME_RECYCLE_INFO_PATH);
	wcscat(infofile, p + wcslen(VERAVOLUME_RECYCLE_PATH));

	WCHAR* data = 0;
	int filesize = 0;
	result = fileReaderW(infofile, &data, &filesize);
	if (result == 0)
	{
		mylog(L"restoreRecycleFiles fileReader %ws errorcode:%d\r\n",infofile, GetLastError());
		return FALSE;
	}

	WCHAR* delfilepath = (WCHAR * )((char*)data + sizeof(SYSTEMTIME));
// 	WCHAR* prevpath = wcsrchr(delfilepath, L'\\');
// 	if (prevpath)
// 	{
// 		*(prevpath + 1) = 0;
// 	}

	copyfile( filepath, delfilepath);

	result = delfile(infofile);
	result = delfile(filepath);

	delete data;

	mylog(L"restoreRecycleFiles from %ws to %ws\r\n",  filepath, delfilepath);

	return result;
}

int deleteLocalFiles(const WCHAR* filename) {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h == 0)
	{
		mylog(L"LoadLibraryW SfDeskDll error\r\n");
		return FALSE;
	}

	typedef int (*ptrfun)(const WCHAR* filename);
	ptrfun deletefunc = (ptrfun)GetProcAddress(h, "SbieApi_DeleteFile");
	if (deletefunc)
	{
		result = deletefunc(filename);
	}

	return result;
}


int deleteBoxedFiles(const WCHAR* filename,WCHAR * boxname) {
	int result = 0;

	WCHAR realpath[MAX_PATH];
	result = translateBoxedpath2Realpath(filename, boxname, realpath, GET_BOXPATH_DEVICE);

	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h == 0)
	{
		mylog(L"LoadLibraryW SfDeskDll error\r\n");
		return FALSE;
	}

	typedef  int (*ptrkeepFilesToRecycle)(WCHAR* CopyPath,WCHAR * boxname,WCHAR * username, int FileType);
	ptrkeepFilesToRecycle keepfiles2recycle = (ptrkeepFilesToRecycle)GetProcAddress(h, "keepFilesToRecycle");
	int filetype = GetFileAttributesW(realpath);

	WCHAR username[MAX_PATH];
	DWORD usernamelen = MAX_PATH;
	result = GetUserNameW(username, &usernamelen);
	username[usernamelen] = 0;

// 	WCHAR dllboxname[MAX_PATH];
// 	wsprintfW(dllboxname, L"%ws%lc\\%ws\\%ws", VERACRYPT_VOLUME_DEVICE, VERACRYPT_DISK_VOLUME[0], boxname, username);
	result = keepfiles2recycle(realpath,boxname, username, filetype);

	typedef int (*ptrSbieApi_DeleteFile)(const WCHAR * srcpath);
	ptrSbieApi_DeleteFile deletefunc = (ptrSbieApi_DeleteFile)GetProcAddress(h, "SbieApi_DeleteFile");
	if (deletefunc)
	{
		result = deletefunc(realpath);
	}

	mylog(L"deleteBoxedFiles:%ws result:%d,error code:%d", realpath, result, GetLastError());
	return result;
}


int  moveFilesIntoBox(const WCHAR* filename, const WCHAR* boxname,WCHAR * returnpath) {
	int result = 0;

	WCHAR realpath[MAX_PATH];
	result = translateBoxedpath2Realpath(filename, boxname, realpath, GET_BOXPATH_DEVICE | CREATE_BOX_SUBPATH);

	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h == 0)
	{
		mylog(L"LoadLibraryW SfDeskDll error\r\n");
		return FALSE;
	}

	typedef int (*ptrcopyfile)(const WCHAR* srcpath, const WCHAR* dstpath);
	ptrcopyfile copyfile = (ptrcopyfile)GetProcAddress(h, "SbieApi_CopyFile");
	if (copyfile)
	{
		result = copyfile(filename,realpath);
	}

	if (wmemcmp(realpath, VERACRYPT_VOLUME_DEVICE,lstrlenW(VERACRYPT_VOLUME_DEVICE)) == 0)
	{
		lstrcpyW (returnpath ,VERACRYPT_DISK_VOLUME);
		lstrcatW(returnpath, realpath + lstrlenW(VERACRYPT_VOLUME_DEVICE) + 1 + 1);
	}

	mylog(L"moveFilesIntoBox from:%ws to:%ws result:%d", filename, returnpath, result);
	return result;
}


int renameBoxedFile(WCHAR *srcfilename,WCHAR *boxname,WCHAR * dstfilename) {
	int result = 0;

	HMODULE hdll = LoadLibraryW(SBIEDLL L".dll");
	if (hdll == 0)
	{
		mylog(L"LoadLibraryW SfDeskDll error\r\n");
		return FALSE;
	}

	WCHAR realpath[MAX_PATH];
	result = translateBoxedpath2Realpath(srcfilename, boxname, realpath, GET_BOXPATH_DEVICE|CREATE_BOX_SUBPATH );
	if (result == FALSE)
	{
		return FALSE;
	}

	typedef int (*ptrcopyfile)(const WCHAR* srcpath, const WCHAR* dstpath);
	ptrcopyfile copyfun = (ptrcopyfile)GetProcAddress(hdll, "SbieApi_CopyFile");

	typedef int (*ptrSbieApi_DeleteFile)(const WCHAR* srcpath);
	ptrSbieApi_DeleteFile delfunc = (ptrSbieApi_DeleteFile)GetProcAddress(hdll, "SbieApi_DeleteFile");

	WCHAR* dstname = wcsrchr(dstfilename, L'\\');
	if (dstname)
	{
		dstname++;
	}
	else {
		dstname = dstfilename;
	}

	WCHAR dstfn[MAX_PATH];
	lstrcpyW(dstfn, realpath);
	WCHAR* fn = wcsrchr(dstfn, L'\\');
	if (fn )
	{
		*fn = 0;
	}
	lstrcatW(dstfn, L"\\");
	lstrcatW(dstfn, dstname);

	result = copyfun(realpath, dstfn);
	result = delfunc(realpath);
	return result;
}







int initialize() {
	int result = 0;

	g_safedesktopRunning = TRUE;

// 	_beginthread_proc_type configThreadMain = (_beginthread_proc_type)configThread;
// 	_beginthread(configThreadMain, 0, NULL);

	mylog(L"initialize entry\r\n");
	result = InstallLaunchEv();

	return result;
}

int uninitialize(const WCHAR* boxname) {
	int result = 0;
	result = closeBox(boxname);

	g_safedesktopRunning = FALSE;
	return result;
}


int getVeraDiskInfo(ULONGLONG *freesize, ULONGLONG* leastsize, ULONGLONG* totalsize) {

	ULARGE_INTEGER lifree;
	ULARGE_INTEGER litotal;
	ULARGE_INTEGER lileast;
	int result = 0;

	result = GetDiskFreeSpaceExW(VERACRYPT_DISK_VOLUME, &lileast, &litotal, &lifree);
	*freesize = lifree.QuadPart;
	*leastsize = litotal.QuadPart - lileast.QuadPart;
	*totalsize = litotal.QuadPart;
	return result;
}





int translateBoxedpath2Realpath(const WCHAR* filepath, const WCHAR* boxname, WCHAR* realpath,int isdevice) {
	int result = 0;

	HMODULE hdll = LoadLibraryW(SBIEDLL L".dll");
	if (hdll == 0)
	{
		mylog(L"LoadLibraryW SfDeskDll error\r\n");
		return FALSE;
	}

	typedef  int (*ptrSbie_createSubPath)(WCHAR* srcfilepath, WCHAR* appendpath);
	ptrSbie_createSubPath createpath = (ptrSbie_createSubPath)GetProcAddress(hdll, "Sbie_createSubPath");

	WCHAR filename[MAX_PATH];
	lstrcpyW(filename, filepath);

	WCHAR username[64];
	DWORD usernamelen = sizeof(username) / 2;
	result = GetUserNameW(username, &usernamelen);
	if (result)
	{
		username[usernamelen] = 0;
	}

	WCHAR sysdir[32];
	result = GetSystemDirectoryW(sysdir, sizeof(sysdir) / sizeof(WCHAR));
	if (sysdir[0]>= 'C' && sysdir[0] <= 'Z')
	{
	}

	WCHAR veracryptpath[64];
	if (isdevice& GET_BOXPATH_DEVICE)
	{
		wsprintfW(veracryptpath, L"%ws%lc\\", VERACRYPT_VOLUME_DEVICE, VERACRYPT_DISK_VOLUME[0]);
	}
	else {
		wsprintfW(veracryptpath, L"%ws",  VERACRYPT_DISK_VOLUME);
	}

	WCHAR currentuserpath[256];
	wsprintfW(currentuserpath, L"Users\\%ws",username);
	int current_userpathlen = lstrlenW(currentuserpath);

	if (filename[1] == ':' && filename[2] == '\\')
	{
		if (filename[0] == sysdir[0] && wmemcmp(filename + 3, currentuserpath, current_userpathlen) == 0) {

			wsprintfW(realpath, L"%ws%ws\\%ws\\", veracryptpath, boxname, username);
			lstrcatW(realpath, L"user\\");
			lstrcatW(realpath, L"current\\");

			WCHAR* pos = wcschr((WCHAR*)filename + 9, '\\');
			if (pos)
			{
				lstrcatW(realpath, pos + 1);
			}
		}
		else if (filename[0] == sysdir[0] && wmemcmp(filename + 3, L"Users\\All\\", lstrlenW(L"Users\\All\\")) == 0)
		{
			wsprintfW(realpath, L"%ws%ws\\%ws\\", veracryptpath, boxname, username);
			lstrcatW(realpath, L"user\\");
			lstrcatW(realpath, L"all\\");

			WCHAR* pos = wcschr((WCHAR*)filename + 9, '\\');
			if (pos)
			{
				lstrcatW(realpath, pos + 1);
			}
		}
		else if (filename[0] == sysdir[0] && wmemcmp(filename + 3, L"Users\\Default\\", lstrlenW(L"Users\\Default\\")) == 0)
		{
			wsprintfW(realpath, L"%ws%ws\\%ws\\", veracryptpath, boxname, username);
			lstrcatW(realpath, L"user\\");
			lstrcatW(realpath, L"default\\");

			WCHAR* pos = wcschr((WCHAR*)filename + 9, '\\');
			if (pos)
			{
				lstrcatW(realpath, pos + 1);
			}
		}
		else if (filename[0] == sysdir[0] && wmemcmp(filename + 3, L"Users\\Public\\", lstrlenW(L"Users\\Public\\")) == 0)
		{
			wsprintfW(realpath, L"%ws%ws\\%ws\\", veracryptpath, boxname, username);
			lstrcatW(realpath, L"user\\");
			lstrcatW(realpath, L"public\\");

			WCHAR* pos = wcschr((WCHAR*)filename + 9, '\\');
			if (pos)
			{
				lstrcatW(realpath, pos + 1);
			}
		}
		else
		{
			wsprintfW(realpath, L"%ws%ws\\%ws\\%ws\\%lc\\", veracryptpath, boxname, username, L"drive", filename[0]);
			lstrcatW(realpath, filename + 3);
		}
	}
	else {
		wsprintfW(realpath,	L"%ws%ws\\%ws\\%ws\\%lc\\windows\\system32\\", veracryptpath, boxname, username, L"drive", sysdir[0]);

		lstrcatW(realpath, filename);
	}

	mylog(L"translateBoxedpath2Realpath:%ws", realpath);

	return TRUE;
}


int getBoxedFile(const WCHAR* filename,const WCHAR * boxname, WCHAR* dospath) {

	mylog(L"getBoxedFile  start\r\n"); 

	DWORD exportFlag = SbieApi__QueryFileExport();
	if (exportFlag != BASIC_ENABLE_STATE)
	{
		mylog(L"exportFlag:%d is not permitted to export file", exportFlag);
		return FALSE;
	}

	FILEEXPORT_PROHIBIT_PARAM param;
	char str_srcfilename[256];
	char str_boxname[256];
	char str_dstfilename[256];
	WideCharToMultiByte(CP_ACP, 0, filename, -1, str_srcfilename, sizeof(str_srcfilename), 0, 0);
	WideCharToMultiByte(CP_ACP, 0, boxname, -1, str_boxname, sizeof(str_boxname), 0, 0);
	WideCharToMultiByte(CP_ACP, 0, dospath, -1, str_dstfilename, sizeof(str_dstfilename), 0, 0);
	param.boxname = str_boxname;
	param.srcfn = str_srcfilename;
	param.dstfn = str_dstfilename;
	alarmWarning(FILEEXPORT_WARNING,&param);

	if ( (filename[0] <= 'Z' && filename[0] >= 'C') || (filename[0] <= 'z' && filename[0] >= 'c') || filename[1] != ':')
	{

	}
	else {
		return FALSE;
	}

	if (filename[2] != '\\' && filename[2] != '//')
	{
		return FALSE;
	}

	int result = 0;

	WCHAR realpath[MAX_PATH];

	result = translateBoxedpath2Realpath(filename, boxname, realpath, GET_BOXPATH_DEVICE | CREATE_BOX_SUBPATH);

	HMODULE hdll = LoadLibraryW(SBIEDLL L".dll");
	if (hdll == 0)
	{
		mylog(L"LoadLibraryW SfDeskDll error\r\n");
		return FALSE;
	}

	typedef int (*ptrSbieApi_CopyFile)(const WCHAR* srcpath, const WCHAR* dstpath);
	ptrSbieApi_CopyFile copyfile = (ptrSbieApi_CopyFile)GetProcAddress(hdll, "SbieApi_CopyFile");
	if (copyfile )
	{
		WCHAR ntpath[MAX_PATH];
		//应用曾不能打开形如:/Device/HarddiskVolume1/xxx之类的设备
		dospath2NTpath(dospath, ntpath);
		//dospath2LinkPath(dospath, ntpath);
		//lstrcpyW(ntpath, dospath);
		
		result = copyfile(realpath, ntpath);
		mylog(L"ptrSbieApi_CopyFile from:%ws to:%ws result:%d", realpath, ntpath, result);		
		return result == 0;

		/*
		WCHAR username[MAX_PATH];
		DWORD usernamelen = MAX_PATH;
		result = GetUserNameW(username, &usernamelen);

		WCHAR copypath[MAX_PATH];
		lstrcpyW(copypath, VERACRYPT_DISK_VOLUME);
		lstrcatW(copypath, boxname);
		lstrcatW(copypath, L"\\");
		lstrcatW(copypath, username);
		lstrcatW(copypath, VERAVOLUME_RECYCLE_COPY_PATH);

		WCHAR* fname = wcsrchr((WCHAR*)filename, L'\\');
		if (fname)
		{
			lstrcatW(copypath, fname + 1);
		}
		else {
			lstrcatW(copypath, filename);
		}
		
		result = copyfile(realpath, copypath);

		mylog(L"ptrSbieApi_CopyFile from:%ws to:%ws result:%d", realpath, copypath,result);
		return result == 0;
		*/
	}

	return FALSE;
}



int Get_Default_Browser(WCHAR* pgmname)
{
	const ULONG _cmdline = 10240;
	HRESULT rc;
	DWORD cmdlen;

	BOOL  iexplore = FALSE;

	WCHAR* assoc_name = (WCHAR*)L".html";
	OSVERSIONINFO osvi;
	memset(&osvi, 0, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((OSVERSIONINFO*)&osvi);
	if ((osvi.dwMajorVersion == 10) || (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 2)) // Win 10 or Win 8
	{
		assoc_name = (WCHAR*)L"http";
	}

	cmdlen = _cmdline;

	cmdlen = _cmdline;
	rc = AssocQueryString(0, ASSOCSTR_EXECUTABLE,
		assoc_name, NULL, pgmname, &cmdlen);

	if (rc != S_OK)
		iexplore = TRUE;
	else {
		WCHAR* backslash = wcsrchr(pgmname, L'\\');
		if (!backslash)
			iexplore = TRUE;
		else if (_wcsicmp(backslash + 1, L"notepad.exe") == 0)
			iexplore = TRUE;
		else if (_wcsicmp(backslash + 1, L"openwith.exe") == 0)
			iexplore = TRUE;
		else if (_wcsicmp(backslash + 1, L"LaunchWinApp.exe") == 0)     // MS Edge (Metro)
			iexplore = TRUE;
	}

	if (iexplore) {

		cmdlen = _cmdline;
		rc = AssocQueryString(ASSOCF_INIT_BYEXENAME, ASSOCSTR_EXECUTABLE,
			L"iexplore", NULL, pgmname, &cmdlen);

		if (rc != S_OK) {

			rc = SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL,
				SHGFP_TYPE_CURRENT, pgmname);
			if (rc != S_OK) {

				memset(pgmname, 0, 16);
				GetSystemWindowsDirectory(pgmname, MAX_PATH);
				if (!pgmname[0])
					wcscpy(pgmname, L"C:\\");
				wcscpy(pgmname + 3, L"Program Files");
			}

			wcscat(pgmname, L"\\Internet Explorer\\iexplore.exe");
		}
	}

	return TRUE;
}






int strreplace(const WCHAR* str,const WCHAR* substr,const WCHAR * replace, WCHAR * dststr) {

	//__debugbreak();

	mylog(L"str:%ws substr:%ws replace:%ws dststr:%ws",str,substr,replace,dststr);

	int replace_len = lstrlenW(replace);
	int substr_len = lstrlenW(substr);
	int len = 0;
	WCHAR* hdr = wcsstr((WCHAR*)str, substr);
	if (hdr)
	{
		len += hdr - str;
		wmemcpy(dststr, str, len);
		dststr[len] = 0;

		hdr += substr_len;
	}
	else {
		return FALSE;
	}

	lstrcatW(dststr, replace);

	lstrcatW(dststr, hdr);
	return TRUE;
}



int interchangeFiles(const WCHAR * srcfilename,const WCHAR *srcboxname,const WCHAR * dstboxname,WCHAR * filename,WCHAR * outbox_dst_fn) {
	int result = 0;

	//__debugbreak();

	WCHAR* subpath = wcschr((WCHAR*)srcfilename, L'\\');
	if (subpath)
	{
		subpath++;
		subpath = wcschr(subpath, L'\\');
		if (subpath)
		{
			subpath++;
			subpath = wcschr(subpath, L'\\');
			if (subpath)
			{
				subpath++;
			}
		}
	}

	WCHAR username[64];
	DWORD usernamelen = sizeof(username) / 2;
	result = GetUserNameW(username, &usernamelen);
	if (result)
	{
		username[usernamelen] = 0;
	}

	//WCHAR filename[MAX_PATH];
	wsprintfW(filename, L"%ws%lc\\%ws\\%ws\\%ws", VERACRYPT_VOLUME_DEVICE, VERACRYPT_DISK_VOLUME[0], dstboxname, username,subpath);

	HMODULE hdll = LoadLibraryW(SBIEDLL L".dll");
	if (hdll == 0)
	{
		mylog(L"LoadLibraryW SfDeskDll error\r\n");
		return FALSE;
	}

	WCHAR realpath[MAX_PATH];
	result = translateBoxedpath2Realpath(srcfilename, srcboxname, realpath, GET_BOXPATH_DEVICE | CREATE_BOX_SUBPATH);
	if (result == FALSE)
	{
		return FALSE;
	}

	typedef int (*ptrcopyfile)(const WCHAR* srcpath, const WCHAR* dstpath);
	ptrcopyfile copyfun = (ptrcopyfile)GetProcAddress(hdll, "SbieApi_CopyFile");
	
	result = copyfun(realpath, filename);

	result = strreplace(realpath, srcboxname, dstboxname, outbox_dst_fn);

	mylog(L"source file:%ws realpath:%ws dstfilename:%ws outboxfilename:%ws result:%d", srcfilename, realpath,filename, outbox_dst_fn, result);

	ntpath2dospath(filename, outbox_dst_fn);

	return result;
}



