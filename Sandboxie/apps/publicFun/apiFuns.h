#pragma once

#include <windows.h>

#include "../../common/my_version.h"
#include "../../common/win32_ntddk.h"
#include "../../core/drv/api_defs.h"
#include "../../common/defines.h"


#define OPERATION_READ		1
#define OPERATION_WRITE		2
#define TYPE_USERNAME		1
#define TYPE_PASSWORD		2
#define TYPE_PATH			3


#define BASIC_ENABLE_STATE	1
#define BASIC_DISABLE_STATE	2
#define BASIC_DEFAULT_STATE	0



#define WATERMARK_DEFAULT			0
#define WATERMARK_FULLSCREEN_WATER	2
#define WATERMARK_DARK_WATER		1

#ifdef __cplusplus
extern "C" {
#endif

	void closeDeviceHandle();
	__declspec(dllexport) LONG SbieApi_OpenProcess(HANDLE* ProcessHandle,HANDLE ProcessId);

	__declspec(dllexport) LONG SbieApi_DuplicateObject(HANDLE* TargetHandle,HANDLE OtherProcessHandle,
		HANDLE SourceHandle,ACCESS_MASK DesiredAccess,ULONG Options);

	__declspec(dllexport) LONG SbieApi_EnumProcessEx(const WCHAR* box_name,BOOLEAN all_sessions,
		ULONG which_session,ULONG* boxed_pids,ULONG* boxed_count);

	__declspec(dllexport) int LjgApi_setPath(WCHAR* path);

	__declspec(dllexport) int LjgApi_getPath(WCHAR* path);

	__declspec(dllexport) NTSTATUS SbieApi_Ioctl(ULONG64* parms);

	__declspec(dllexport) int LjgApi_SetUsername(const WCHAR* username);

	__declspec(dllexport) LONG LjgApi_ReloadConf(ULONG session_id, ULONG flags);

	__declspec(dllexport) int LjgApi_getUsername(WCHAR* username);

	__declspec(dllexport) int LjgApi_SetPassword(const WCHAR* password);

	__declspec(dllexport) int LjgApi_getPassword(WCHAR* password);

	__declspec(dllexport) LONG SbieApi_GetHomePath(WCHAR* NtPath, ULONG NtPathMaxLen, WCHAR* DosPath, ULONG DosPathMaxLen);

	__declspec(dllexport) LONG SbieApi_QueryConf(const WCHAR* section_name,const WCHAR* setting_name,ULONG setting_index,WCHAR* out_buffer,ULONG buffer_len);

	__declspec(dllexport) LONG SbieApi_EnumBoxes(LONG index, WCHAR* box_name);

	__declspec(dllexport) LONG SbieApi_IsBoxEnabled(const WCHAR* box_name);

	__declspec(dllexport) LONG SbieApi_EnumBoxesEx(LONG index, WCHAR* box_name, BOOLEAN return_all_sections);

	__declspec(dllexport)   LONG SbieApi_ResetProcessMonitor();

	__declspec(dllexport)  LONG SbieApi_SetProcessMonitor(WCHAR* processname);

	__declspec(dllexport)  LONG SbieApi_SetWatermark(BOOLEAN enable);

	__declspec(dllexport)  DWORD SbieApi_QueryWatermark();

	__declspec(dllexport)  DWORD SbieApi_QueryScreenshot();

	__declspec(dllexport)  LONG SbieApi_SetScreenshot(BOOLEAN enable);

	__declspec(dllexport)  DWORD SbieApi_SetPrinter(BOOLEAN enable);

	__declspec(dllexport)  DWORD SbieApi_QueryPrinter();

	__declspec(dllexport)  DWORD public_SetExportFlag(BOOLEAN enable);

	__declspec(dllexport)  DWORD public_GetExportFlag();

	__declspec(dllexport)  DWORD SbieApi_SetFileExport(BOOLEAN enable);
	__declspec(dllexport)  DWORD SbieApi_QueryFileExport();

#ifdef __cplusplus
}
#endif


