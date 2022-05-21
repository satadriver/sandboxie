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

#define SCREENSHOT_WARNING	1
#define PRINTER_WARNING		2
#define WATERMARK_WARNING	3
#define FILEEXPORT_WARNING	4
#define PROCESS_PROHIBIT	5


typedef struct  
{
	char* filename;
	char* printername;
}PRINTER_PROHIBIT_PARAM;


typedef struct  
{
	char* srcfn;
	char* dstfn;
	char* boxname;
}FILEEXPORT_PROHIBIT_PARAM;

#define SBIEAPI_EXPORT  __declspec(dllexport)


__declspec(dllexport) int __cdecl mylog(const WCHAR* format, ...);

__declspec(dllexport) int __cdecl mylog(const CHAR* format, ...);

#ifdef __cplusplus
extern "C" {
#endif


	void closeDeviceHandle();

	__declspec(dllexport) LONG SbieApi_OpenProcess(HANDLE* ProcessHandle,HANDLE ProcessId);

	//__declspec(dllexport) LONG SbieApi_DuplicateObject(HANDLE* TargetHandle,HANDLE OtherProcessHandle,HANDLE SourceHandle,ACCESS_MASK DesiredAccess,ULONG Options);

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

	__declspec(dllexport) int alarmWarning(int type,LPVOID params);

	__declspec(dllexport)  DWORD SbieApi_SetFileExport(BOOLEAN enable);
	__declspec(dllexport)  DWORD SbieApi_QueryFileExport();

	SBIEAPI_EXPORT
		LONG SbieApi_QueryProcess(
			HANDLE ProcessId,
			WCHAR* out_box_name_wchar34,        // WCHAR [34]
			WCHAR* out_image_name_wchar96,      // WCHAR [96]
			WCHAR* out_sid_wchar96,             // WCHAR [96]
			ULONG* out_session_id);             // ULONG

	SBIEAPI_EXPORT
		LONG SbieApi_QueryProcessEx(
			HANDLE ProcessId,
			ULONG image_name_len_in_wchars,
			WCHAR* out_box_name_wchar34,        // WCHAR [34]
			WCHAR* out_image_name_wcharXXX,     // WCHAR [?]
			WCHAR* out_sid_wchar96,             // WCHAR [96]
			ULONG* out_session_id);             // ULONG

	SBIEAPI_EXPORT
		LONG SbieApi_QueryProcessEx2(
			HANDLE ProcessId,
			ULONG image_name_len_in_wchars,
			WCHAR* out_box_name_wchar34,        // WCHAR [34]
			WCHAR* out_image_name_wcharXXX,     // WCHAR [?]
			WCHAR* out_sid_wchar96,             // WCHAR [96]
			ULONG* out_session_id,              // ULONG
			ULONG64* out_create_time);          // ULONG64

#ifdef __cplusplus
}
#endif


