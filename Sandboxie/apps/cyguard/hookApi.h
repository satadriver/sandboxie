#pragma once

#include <windows.h>



#define MAX_TRAMP_SIZE					128
#define TRAMP_BAR_LIMIT					128

#pragma pack(1)

typedef struct  
{
	unsigned char* oldaddr;
	char len;
}REPLACE_CODE;

typedef struct  _HOOK_TRAMPS
{
	_HOOK_TRAMPS * prev;
	_HOOK_TRAMPS * next;
	int valid;
	WCHAR apiName[32];
	//int apiTotal;
	
	BYTE code[MAX_TRAMP_SIZE];

	BYTE oldcode[MAX_TRAMP_SIZE];
	REPLACE_CODE replace;

}HOOK_TRAMPS;

#pragma pack()

//extern HOOK_TRAMPS *g_trump;

//extern HOOK_TRAMPS g_trump[MAX_TRAMP_SIZE];

HOOK_TRAMPS* searchTrump(const WCHAR* funcname);

int deleteTrump(const WCHAR* funcname);

HOOK_TRAMPS* addTrump(const WCHAR* funcname);

extern "C" __declspec(dllexport) int hook(const WCHAR* modulename,const WCHAR* funcname, BYTE * newfuncaddr, PROC* keepaddr);

extern "C" __declspec(dllexport) int inlinehook64(BYTE * newfun, BYTE * oldfun, PROC* keepaddr, const WCHAR * funcname);

extern "C" __declspec(dllexport) int inlinehook32(BYTE * newfun, BYTE * oldfun, PROC* keepaddr, const WCHAR * funcname);

int unhook(CONST WCHAR* modulename, const WCHAR* wstrfuncname);

int  unhookall();
