#pragma once

//#include <windows.h>

//end with elements 0

#pragma warning(disable:4005)

#include "../../common/my_version.h"


#define MAX_PID_BUFFER				64

#define SCREENCAPTURE_INJECT_FLAG	1
#define GENERAL_INJECT_FLAG			2

#define INJECTOR_HELPER_PROCESS		L"processInjector.exe"

#pragma pack(1)
typedef struct  
{
	const WCHAR* name;
	int flag;
}INJECT_PROCESS_INFO;
#pragma pack()

INJECT_PROCESS_INFO g_injectProcess[] = {
	{L"DibTest.exe",1},

	{EXPLORER_PROCESS_NAME, 1},

	{L"cmd.exe",1},
	{L"powershell.exe",1},

	//{L"TaskMgr.exe" ,1 },

	{L"SnippingTool.exe",1},
	{L"ScreenSketch.exe", 1},

	{L"WINWORD.EXE", 1},
	{L"POWERPNT.EXE", 1},
	{L"excel.exe", 1},
	{L"MSACCESS.exe",1},
	{L"OUTLOOK.EXE",1},

	{L"wps.exe", 1},

	//{EXPLORER_PROCESS_NAME_PLUSPLUS, 1},

	{L"QQ.exe", 1},
	{L"WeChat.exe" , 1},
	{L"WXWork.exe", 1},
	{L"DingTalk.exe", 1},
	{L"Feishu.exe", 1},
	{L"LxMainNew.exe", 1},

// 	{L"RuntimeBroker.exe",1},
// 	{L"conhost.exe",1},
// 	{L"dllhost.exe",1},
// 	{L"rundll32.exe", 1},
// 	{L"svchost.exe", 1},

	//{L"wordpad.exe", 1},
	//{L"notepad.exe", 1},
	{0,0}
};


INJECT_PROCESS_INFO g_ignoreProcess[] = {
	{L"chrome.exe", 1},
	{L"msedge.exe",1},
	{L"firefox.EXE",1},

	//{L"RuntimeBroker.exe",1},
	//{L"conhost.exe",1},
	//{L"dllhost.exe",1},
	//{L"rundll32.exe", 1},

	{L"WerFault.exe", 1},

	{L"svchost.exe", 1},
	{0}
};


INJECT_PROCESS_INFO g_NoneInjectProcess[] = {
	{START_EXE,1},
	{SBIESVC_EXE, 1},
	{SANDBOXIE_DESKTOPEXE,1},
	{SBIEINI_EXE,1},
	{L"desktop.exe" ,1 },
	{INJECTOR_HELPER_PROCESS, 1},

	{L"KmdUtil.exe" , 1},
	{L"SafeDesktopBITS.exe", 1},
	{L"SafeDesktopCrypto.exe", 1},
	{L"SafeDesktopDcomLaunch.exe", 1},
	{L"SafeDesktopRpcSs.exe", 1},
	{L"SafeDesktopWUAU.EXE", 1},
	{L"qsdpclient.EXE", 1},
	{L"sdptunnel.exe", 1},
	{L"SfDeskCtrl.exe", 1},
	{L"SfDeskIni.exe", 1},
	{L"VeraCrypt.exe", 1},
	{L"VeraCrypt Format.exe", 1},

	//{L"RuntimeBroker.exe",1},
	//{L"conhost.exe",1},
	//{L"dllhost.exe",1},
	//{L"rundll32.exe", 1},
	//{L"svchost.exe", 1},
};


