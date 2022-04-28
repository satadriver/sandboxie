
#include "hook.h"
#include "apiFuns.h"
#include "Utils.h"
#include <stdio.h>

HHOOK g_hHook = { 0 };

HANDLE g_hookMutex = 0;

HMODULE g_dllModule = 0;


extern "C" __declspec(dllexport) void WINAPI UnInstallLaunchEv()
{
	if (g_hookMutex)
	{
		ReleaseMutex(g_hookMutex);
		CloseHandle(g_hookMutex);
		g_hookMutex = 0;
	}
	UnhookWindowsHookEx(g_hHook);
}

extern "C" __declspec(dllexport) int WINAPI InstallLaunchEv()
{
	int result = SbieApi_QueryScreenshot();
	if (result == BASIC_DISABLE_STATE)
	{
		return FALSE;
	}

	mylog(L"InstallLaunchEv entry");

	const WCHAR* mutexname = L"cyguard_key_mutex";
	g_hookMutex = CreateMutexW(0, TRUE, mutexname);
	if (g_hookMutex)
	{
		result = GetLastError();
		if (result == ERROR_ALREADY_EXISTS)
		{
			ReleaseMutex(g_hookMutex);
			CloseHandle(g_hookMutex);

			mylog(L"InstallLaunchEv mutex:%ws already exist\r\n", mutexname);
			return FALSE;
		}
	}
	else {
		mylog(L"InstallLaunchEv CreateMutexW:%ws error\r\n", mutexname);
		return FALSE;
	}

	//DWORD threadid = GetCurrentThreadId();
	//g_hHook = SetWindowsHookExW(WH_KEYBOARD_LL, (HOOKPROC)LauncherHook, 0, threadid);
	
	g_hHook = SetWindowsHookExW(WH_KEYBOARD_LL, (HOOKPROC)LauncherHook, g_dllModule, 0);
	if (g_hHook == NULL)
	{
		mylog(L"SetWindowsHookExW error code %d", GetLastError());
		//strerror(GetLastError());
	}

	mylog(L"SetWindowsHookExW ok");

	return TRUE;
}


LRESULT CALLBACK JournalRecordProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

//If the hook procedure processed the message, it may return a nonzero value to 
//prevent the system from passing the message to the rest of the hook chain or the target window procedure.
//VK_APPS就是application键，就是右WIN键和右ctrl键中间的那个键。
//没有VK_ALT只有VK_MENU, VK_LMENU, VK_RMENU表示左右2个ALT键
//必须保证回到函数所在的进程依然存活，否则回调不执行
LRESULT CALLBACK LauncherHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	//__debugbreak();

	KBDLLHOOKSTRUCT* Key_Info = (KBDLLHOOKSTRUCT*)lParam;

	//mylog(L"press key :%xh", Key_Info->vkCode);

	if (nCode == HC_ACTION)
	{
		if (WM_KEYDOWN == wParam || WM_SYSKEYDOWN == wParam)
		{
			BOOL b_lctrl = ::GetAsyncKeyState(VK_LCONTROL);
			BOOL b_rctrl = ::GetAsyncKeyState(VK_RCONTROL);
			BOOL b_lAlt = ::GetAsyncKeyState(VK_LMENU);
			BOOL b_rAlt = ::GetAsyncKeyState(VK_RMENU);
			BOOL b_lShift = ::GetAsyncKeyState(VK_LSHIFT);
			BOOL b_rShift = ::GetAsyncKeyState(VK_RSHIFT);
			BOOL b_lWin = ::GetAsyncKeyState(VK_LWIN);
			BOOL b_rWin = ::GetAsyncKeyState(VK_RWIN);

			if (Key_Info->vkCode == VK_SNAPSHOT || Key_Info->vkCode == VK_PRINT)
			{
				mylog(L"press screenshot %x",Key_Info->vkCode);
				return TRUE;
			}
			else if ( (b_lctrl || b_rctrl) && (b_rAlt || b_lAlt) && Key_Info->vkCode == 0x41)
			{
				mylog(L"press ctrl+alt+A");
				return TRUE;
			}
			else if ( (b_rWin || b_lWin) &&(b_lShift || b_rShift ) && Key_Info->vkCode == 'S')
			{
				return TRUE;
			}
// 			else if ( (b_lAlt || b_rAlt) && Key_Info->vkCode==0x41)
// 			{
// 				return TRUE;
// 			}
		}
	}

	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}


void savelog(const char* s)
{
	FILE* p;
	errno_t err = fopen_s(&p, "D:\\my.log", "a+");
	fputs(s, p);
	//fputs是一种函数，具有的功能是向指定的文件写入一个字符串（不自动写入字符串结束标记符‘\0’）。成功写入一个字符串后，文件的位置指针会自动后移，函数返回值为非负整数
	fclose(p);
}

void stringerror(DWORD errno)
{
	void* lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errno,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(char*)&lpMsgBuf,
		0,
		NULL
	);

	savelog((const char*)lpMsgBuf);
	// Free the buffer.
	LocalFree(lpMsgBuf);
}
