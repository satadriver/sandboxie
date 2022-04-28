#pragma once


#include <windows.h>

#define SANDBOXIE_LOG_FILENAME		"safeDesktop.log"



int dospath2NTpath(const WCHAR* dospath, WCHAR* ntpath);

extern "C" __declspec(dllexport) int getFileType(const WCHAR* filename);


#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) int fileReaderW(const WCHAR* filename, WCHAR** lpbuf, int* lpsize);
__declspec(dllexport) int fileReaderA(const CHAR* filename, CHAR** lpbuf, int* lpsize);
__declspec(dllexport) int fileWriterA(const CHAR* filename, CHAR* lpbuf, int lpsize, int append);
__declspec(dllexport) int fileWriterW(const WCHAR* filename, WCHAR* lpbuf, int lpsize, int append);

#ifdef __cplusplus
}
#endif



int translateQuota(WCHAR* str);

extern "C" __declspec(dllexport) int IsWow64();

extern "C" __declspec(dllexport) BOOL Is64BitOS();

extern "C" __declspec(dllexport) void debug_printA(const char* formatstr, ...);

extern "C" __declspec(dllexport) void debug_printW(const WCHAR* formatstr, ...);

int getPidFromName(const WCHAR* processname);

extern "C" __declspec(dllexport) int sblog(const char* categary, const char* module, const char* grad, const char* msg);

extern "C" __declspec(dllexport) int createProcessAsExplorer(WCHAR* cmdline);



extern "C" __declspec(dllexport) int EnableDebugPrivilege();

extern "C" __declspec(dllexport) int adjustPrivileges();


extern __declspec(dllexport) int __cdecl mylog(const WCHAR* format, ...);

extern __declspec(dllexport) int __cdecl mylog(const CHAR* format, ...);

extern "C" __declspec(dllexport) int __wcslen(const WCHAR* str);

extern "C" __declspec(dllexport) WCHAR* __wcsrchr(const WCHAR* str, WCHAR wc);

extern "C" __declspec(dllexport) WCHAR* __wcschr(const WCHAR* str, const WCHAR wc);

extern "C" __declspec(dllexport) int __wcscpy(WCHAR* str1, const WCHAR* str2);

extern "C" __declspec(dllexport) int __wcscat(WCHAR* str1, const  WCHAR* str2);

extern "C" __declspec(dllexport) int GBKToUTF8(const char* gb2312, char** lpdatabuf);

extern "C" __declspec(dllexport) int UTF8ToGBK(const char* utf8, char** lpdatabuf);
