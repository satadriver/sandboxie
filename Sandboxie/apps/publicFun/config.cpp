
#include "config.h"
#include "Utils.h"
#include "../../common/my_version.h"
#include "apiFuns.h"

#pragma comment(lib,"ws2_32.lib")

int parseDomain(const char * dnsname,char * dstname) {
	int len = lstrlenA(dnsname);
	int i = 0;
	int j = 0;
	for (;i < len; )
	{
		if (dnsname[i] == '*' && dnsname[i+1] == '.')
		{
			i+=2;
		}
		else if (dnsname[i] == '.' && dnsname[i + 1] == '*')
		{
			i += 2;
		}
		else {
			dstname[j] = dnsname[i];
			j++;
			i++;
		}
	}
	*(dstname + j) = 0;
	return j;
}

int parseUrl(const char* url, char* dsturl) {
	int urllen = lstrlenA(url);
	lstrcpyA(dsturl, url);
	return urllen;
}

DWORD parseIPv4(const char * strip,DWORD * dstip,DWORD * mask) {

	char stripv4[64];
	lstrcpyA(stripv4, strip);
	char * pos = strchr(stripv4, '/');
	if (pos)
	{
		*pos = 0;
		int masksize = atoi(pos + 1);
		if (masksize == 8)
		{
			*mask = 0xffffff00;
		}else if (masksize == 16)
		{
			*mask = 0xffff0000;
		}else if (masksize == 24)
		{
			*mask = 0xff000000;
		}else if (masksize == 32)
		{
			*mask = 0xffffffff;
		}
		else {
			*mask = 0xffffffff;
		}
	}
	else {
		*mask = 0xffffffff;
	}

	DWORD dwip = inet_addr(stripv4);
	if (dwip && dwip != 0xffffffff )
	{
		*dstip = dwip;
		return TRUE;
	}
	int ipv4len = lstrlenA(stripv4);

	CHAR ipparts[4][4] ;
	const char* src = stripv4;
	int k = 0;
	int m = 0;
	while(src[m])
	{
		if (src[m] == '.')
		{
			src = src + m + 1;
			k++;
			m = 0;
		}
		else {
			ipparts[k][m] = src[m];
			ipparts[k][m+1] = 0;
			m++;
		}
	}

	UCHAR netmask[4] = { 0xff,0xff,0xff,0xff };

	UCHAR ipsplit[4] = { 0 };

	for (int i = 0;i < 4;i ++)
	{
		if (strstr(ipparts[i], "*"))
		{
			ipsplit[i] = 0;
			netmask[i] = 0;
		}
		else {
			ipsplit[i] = (UCHAR)atoi(ipparts[i]);
			if (ipsplit[i] == 0)
			{
				break;
			}
		}
	}

	*mask = *(DWORD*)netmask;
	*dstip = *(DWORD*)ipsplit;
	return TRUE;
}

WORD parsePort(WORD port) {

	return port;
}


MyBoxList * g_boxlist = 0;


MyBoxList *searchboxlist(const WCHAR* boxname) {
	MyBoxList* list = g_boxlist;
	do {

		if (list && list->next)
		{
			if (lstrcmpiW(boxname,list->boxname) == 0)
			{
				return list;
			}
			list = list->next;
		}
		else {
			break;
		}

	} while (list!= g_boxlist);

	return FALSE;
}


int addboxlist(const WCHAR* boxname)
{
	if (g_boxlist == 0)
	{
		g_boxlist = new MyBoxList;
		g_boxlist->prev = g_boxlist;
		g_boxlist->next = g_boxlist;
		
		lstrcpyW(g_boxlist->boxname, boxname);
		return TRUE;
	}
	else {
		MyBoxList* list = searchboxlist(boxname);
		if (list == 0)
		{
			MyBoxList *newlist = new MyBoxList;
			newlist->prev = g_boxlist;
			newlist->next = g_boxlist->next;

			g_boxlist->next->prev = newlist;
			g_boxlist->next = newlist;
			return TRUE;
		}
	}

	return FALSE;
}


int deleteboxlist(const WCHAR* boxname){
	MyBoxList* list = searchboxlist(boxname);
	if (list)
	{
		list->next->prev = list->prev;
		list->prev->next = list->next;

		delete list;
		if (list == g_boxlist)
		{
			g_boxlist = 0;
		}
		return TRUE;
	}

	return FALSE;
}




int startSbieSvc() {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h)
	{
		typedef BOOLEAN(*ptrSbieDll_StartSbieSvc)(BOOLEAN retry);

		ptrSbieDll_StartSbieSvc func = (ptrSbieDll_StartSbieSvc)GetProcAddress(h, "SbieDll_StartSbieSvc");
		if (func)
		{
			result = func(TRUE);
			mylog(L"ptrSbieDll_StartSbieSvc result:%d\r\n", result);
		}
	}
	return TRUE;
}


int setScreenshot(int enable) {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h)
	{
		typedef LONG(*ptrSbieApi_SetScreenshot)(int enable);
		ptrSbieApi_SetScreenshot func = (ptrSbieApi_SetScreenshot)GetProcAddress(h, "SbieApi_SetScreenshot");
		if (func)
		{
			result = func(enable);
			mylog(L"ptrSbieApi_SetScreenshot result:%d\r\n", result);
		}
	}
	return TRUE;
}

int setWatermark(int enable) {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h)
	{
		typedef LONG(*ptrSbieApi_SetWatermark)(int enable);
		ptrSbieApi_SetWatermark func = (ptrSbieApi_SetWatermark)GetProcAddress(h, "SbieApi_SetWatermark");
		if (func)
		{
			result = func(enable);
			mylog(L"ptrSbieApi_SetWatermark result:%d\r\n", result);
		}
	}
	return TRUE;
}


int resetAllBoxList() {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h)
	{
		typedef LONG(*ptrSbieApi_ResetProcessMonitor)();
		ptrSbieApi_ResetProcessMonitor func = (ptrSbieApi_ResetProcessMonitor)GetProcAddress(h, "SbieApi_ResetAllBoxList");
		if (func)
		{
			result = func();
			//mylog(L"ptrSbieApi_ResetProcessMonitor result:%d\r\n", result);
		}
	}
	return TRUE;
}


int setProcessMonitor(const WCHAR * processname) {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h)
	{
		typedef LONG(*ptrSbieApi_SetProcessMonitor)(const WCHAR* processname);
		ptrSbieApi_SetProcessMonitor func = (ptrSbieApi_SetProcessMonitor)GetProcAddress(h, "SbieApi_SetProcessMonitor");
		if (func)
		{
			result = func(processname);
			//mylog(L"SbieApi_SetProcessMonitor result:%d\r\n", result);
		}
	}
	return TRUE;
}


/*
int setPrinterControl(BOOLEAN enable) {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h)
	{
		typedef LONG(*ptrSbieDll_SetPrinterControl)(BOOLEAN enable);
		ptrSbieDll_SetPrinterControl func = (ptrSbieDll_SetPrinterControl)GetProcAddress(h, "SbieDll_SetPrinterControl");
		if (func)
		{
			result = func(enable);
			mylog(L"SbieDll_SetPrinterControl result:%d\r\n", result);
		}
	}
	return TRUE;
}
BOOLEAN SetWatermarkControl(BOOLEAN enable) {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h)
	{
		typedef LONG(*ptrSbieDll_SetWatermarkControl)(BOOLEAN enable);
		ptrSbieDll_SetWatermarkControl func = (ptrSbieDll_SetWatermarkControl)GetProcAddress(h, "SbieDll_SetWatermarkControl");
		if (func)
		{
			result = func(enable);
			mylog(L"ptrSbieDll_SetWatermarkControl result:%d\r\n", result);
		}
	}
	return result;
}
BOOLEAN SetScreenCaptureControl(BOOLEAN enable) {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h)
	{
		typedef LONG(*ptrSbieDll_SetScreenCaptureControl)(BOOLEAN enable);
		ptrSbieDll_SetScreenCaptureControl func = (ptrSbieDll_SetScreenCaptureControl)GetProcAddress(h, "SbieDll_SetScreenCaptureControl");
		if (func)
		{
			result = func(enable);
			mylog(L"ptrSbieDll_SetScreenCaptureControl result:%d\r\n", result);
		}
	}
	return result;
}
*/

int getProcessInBox(const WCHAR* boxname, DWORD* pids) {
	int result = 0;
	HMODULE h = LoadLibraryW(SBIEDLL L".dll");
	if (h)
	{
		typedef LONG(*ptrSbieApi_EnumProcessEx)(const WCHAR* box_name,
			BOOLEAN all_sessions,
			ULONG which_session,            
			ULONG* boxed_pids,             
			ULONG* boxed_count);
		ptrSbieApi_EnumProcessEx func = (ptrSbieApi_EnumProcessEx)GetProcAddress(h, "SbieApi_EnumProcessEx");
		if (func)
		{
			result = func(boxname, FALSE, -1, pids, NULL);
			mylog(L"SbieApi_SetProcessMonitor result:%d\r\n", result);
		}
	}
	return TRUE;
}





#pragma once

#include <windows.h>
#include <tchar.h>
#include <stdbool.h>

//namespace DetectVM {
	bool IsVboxVM() {
		HANDLE handle = CreateFile(_T("\\\\.\\VBoxMiniRdrDN"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			CloseHandle(handle);
			return true;
		}
		return false;
	}

	bool IsVMwareVM() {
		HKEY hKey = 0;
		DWORD dwType = REG_SZ;
		char buf[255] = { 0 };
		DWORD dwBufSize = sizeof(buf);
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\VMware, Inc.\\VMware Tools"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
			return true;
		}
		return false;
	}

	BOOL SelfDelete() {
		TCHAR szFile[MAX_PATH], szCmd[MAX_PATH];
		if ((GetModuleFileName(0, szFile, MAX_PATH) != 0) && (GetShortPathName(szFile, szFile, MAX_PATH) != 0)) {
			lstrcpy(szCmd, _T("/c del "));
			lstrcat(szCmd, szFile);
			lstrcat(szCmd, _T(" >> NUL"));
			if ((GetEnvironmentVariable(_T("ComSpec"), szFile, MAX_PATH) != 0) && ((INT)ShellExecute(0, 0, szFile, szCmd, 0, SW_HIDE) > 32))
				return TRUE;
		}
		return FALSE;
	}
//}
