#pragma once


#include <windows.h>

#define VERACRYPT_VOLUME_DEVICE			L"\\Device\\VeraCryptVolume"

#define FILE_TYPE_TEXT		1
#define FILE_TYPE_PIC		2
#define FILE_TYPE_HTML		4

#define MYCOMPUTER_CLID		L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"
#define MYRECYCLE_CLID		L"::{645FF040-5081-101B-9F08-00AA002F954E}"




#pragma pack(1)

typedef struct
{
	WCHAR* mainpath;
	WCHAR* subpath;
}CREATEPATH_PARAM;

#pragma pack()





extern int g_safedesktopRunning ;

int strreplace(const WCHAR* str, const WCHAR* substr, const WCHAR* replace, WCHAR* dststr);

int translatePath(const WCHAR* filename, WCHAR* filepath);

int translateBoxedpath2Realpath(const WCHAR* filename, const WCHAR* boxname, WCHAR* realpath, int isdevice);

int commandline(WCHAR* szparam, int wait, int show);

int Get_Default_Browser(WCHAR* buffer);


#ifdef __cplusplus
extern "C"  {  
#endif

	__declspec(dllexport) int closeBox(const WCHAR* boxname);

	__declspec(dllexport) DWORD  openMycomputer(const wchar_t* boxname);

	__declspec(dllexport) DWORD  openRecycle(const wchar_t* boxname);

	__declspec(dllexport) DWORD startBoxProcess(const wchar_t* filepath, const wchar_t* boxname,int isadmin);

	__declspec(dllexport) DWORD stopBoxProcess(BOOL all);



	__declspec(dllexport) DWORD setBoxWorkpathInCtrl(const wchar_t* workpath);

	__declspec(dllexport) DWORD createBoxInCtrl(const wchar_t* boxname);

	__declspec(dllexport) DWORD moveFileIntoRecycleInCtrl(const wchar_t* filename);



	__declspec(dllexport) int moveFilesIntoBox(const WCHAR* filename, const WCHAR* boxname,WCHAR * returnpath);

	__declspec(dllexport) int getVeraDiskInfo(ULONGLONG* freesize, ULONGLONG* leastsize, ULONGLONG* totalsize);

	__declspec(dllexport) int initialize();

	__declspec(dllexport) int uninitialize(const WCHAR* boxname);

	__declspec(dllexport) int deleteLocalFiles(const WCHAR* srcpath);

	__declspec(dllexport) int emptyRecycleFiles(const WCHAR* boxname);

	__declspec(dllexport) int restoreRecycleFiles(const WCHAR* srcpath);

	__declspec(dllexport) int getBoxedFile(const WCHAR* filename,const WCHAR * boxname, WCHAR* realpath);

	__declspec(dllexport) int deleteBoxedFiles(const WCHAR* filename, WCHAR* boxname);

	__declspec(dllexport) int renameBoxedFile(WCHAR* srcfilename, WCHAR* boxname, WCHAR* dstfilename);

	__declspec(dllexport) int interchangeFiles(const WCHAR* srcfilename, const WCHAR* srcboxname, const WCHAR* dstboxname,
		WCHAR* dstfilename, WCHAR* outboxfilename);

#ifdef __cplusplus
}
#endif



