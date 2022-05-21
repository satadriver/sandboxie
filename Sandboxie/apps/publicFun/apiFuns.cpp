

#include "apiFuns.h"

#include "Utils.h"

#include <stdio.h>
#include <wtypes.h>

//#pragma  data_seg(".LJGShared")	//. is part of section name?
//__declspec(dllexport)  BOOLEAN  g_exportFlag = FALSE;
// __declspec(dllexport) BOOLEAN  g_waterMarkControl = FALSE;
// __declspec(dllexport) BOOLEAN  g_screenShotControl = FALSE;
//#pragma  data_seg()
//#pragma  comment(linker, "/Section:.LJGShared,rws") 


HANDLE SbieApi_DeviceHandle = INVALID_HANDLE_VALUE;


void closeDeviceHandle() {
	if (SbieApi_DeviceHandle != INVALID_HANDLE_VALUE) {
		NtClose(SbieApi_DeviceHandle);
	}
	SbieApi_DeviceHandle = INVALID_HANDLE_VALUE;
}




LONG SbieApi_GetHomePath(
	WCHAR* NtPath, ULONG NtPathMaxLen, WCHAR* DosPath, ULONG DosPathMaxLen)
{
	NTSTATUS status;
	__declspec(align(8)) UNICODE_STRING64 nt_path_uni;
	__declspec(align(8)) UNICODE_STRING64 dos_path_uni;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_GET_HOME_PATH_ARGS* args = (API_GET_HOME_PATH_ARGS*)parms;

	nt_path_uni.Length = 0;
	nt_path_uni.MaximumLength = (USHORT)(NtPathMaxLen * sizeof(WCHAR));
	nt_path_uni.Buffer = (ULONG64)(ULONG_PTR)NtPath;

	dos_path_uni.Length = 0;
	dos_path_uni.MaximumLength = (USHORT)(DosPathMaxLen * sizeof(WCHAR));
	dos_path_uni.Buffer = (ULONG64)(ULONG_PTR)DosPath;

	memzero(parms, sizeof(parms));
	args->func_code = API_GET_HOME_PATH;
	if (NtPath)
		args->nt_path.val64 = (ULONG64)(ULONG_PTR)&nt_path_uni;
	if (DosPath)
		args->dos_path.val64 = (ULONG64)(ULONG_PTR)&dos_path_uni;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		if (NtPath)
			NtPath[0] = L'\0';
		if (DosPath)
			DosPath[0] = L'\0';
	}

	return status;
}





NTSTATUS SbieApi_Ioctl(ULONG64* parms)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES objattrs;
	UNICODE_STRING uni;
	IO_STATUS_BLOCK MyIoStatusBlock;

	if (parms == NULL) { // close request as used by kmdutil
		if (SbieApi_DeviceHandle != INVALID_HANDLE_VALUE)
			NtClose(SbieApi_DeviceHandle);
		SbieApi_DeviceHandle = INVALID_HANDLE_VALUE;
	}



	if (SbieApi_DeviceHandle == INVALID_HANDLE_VALUE) {

		RtlInitUnicodeString(&uni, API_DEVICE_NAME);
		InitializeObjectAttributes(
			&objattrs, &uni, OBJ_CASE_INSENSITIVE, NULL, NULL);

		status = NtOpenFile(
			&SbieApi_DeviceHandle, FILE_GENERIC_READ, &objattrs,
			&MyIoStatusBlock,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			0);

		if (status == STATUS_OBJECT_NAME_NOT_FOUND ||
			status == STATUS_NO_SUCH_DEVICE)
			status = STATUS_SERVER_DISABLED;

	}
	else
		status = STATUS_SUCCESS;

	if (status != STATUS_SUCCESS) {

		SbieApi_DeviceHandle = INVALID_HANDLE_VALUE;

	}
	else {

		//
		// note that all requests are synchronous which means
		// NtDeviceIoControlFile will wait until SbieDrv has finished
		// processing a request before sending the next request
		//

		status = NtDeviceIoControlFile(
			SbieApi_DeviceHandle, NULL, NULL, NULL, &MyIoStatusBlock,
			API_SBIEDRV_CTLCODE, parms, sizeof(ULONG64) * 8, NULL, 0);
	}

	return status;
}

LONG LjgApi_ReloadConf(ULONG session_id, ULONG flags)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];

	memset(parms, 0, sizeof(parms));
	parms[0] = API_RELOAD_CONF;
	parms[1] = session_id;
	parms[2] = flags;
	status = SbieApi_Ioctl(parms);

	return status;
}


int LjgApi_SetUsername(const WCHAR * username)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	memset(parms, 0, sizeof(parms));

	API_USERNAME_PASSWORD_ARGS* args = (API_USERNAME_PASSWORD_ARGS*)parms;

	args->type.val = TYPE_USERNAME;
	args->operation.val = OPERATION_WRITE;
	args->func_code = API_USERNAME_PASSWORD;
	args->data.val = (WCHAR*)username;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {

	}

	return status;
}

int LjgApi_getUsername(WCHAR* username)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	memset(parms, 0, sizeof(parms));
	API_USERNAME_PASSWORD_ARGS* args = (API_USERNAME_PASSWORD_ARGS*)parms;

	args->func_code = API_USERNAME_PASSWORD;
	args->type.val = TYPE_USERNAME;
	args->operation.val = OPERATION_READ;
	args->data.val = (WCHAR*)username;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {

	}

	return status;
}

int LjgApi_SetPassword(const WCHAR* password)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	memset(parms, 0, sizeof(parms));
	API_USERNAME_PASSWORD_ARGS* args = (API_USERNAME_PASSWORD_ARGS*)parms;

	args->func_code = API_USERNAME_PASSWORD;
	args->type.val = TYPE_PASSWORD;
	args->operation.val = OPERATION_WRITE;
	args->data.val = (WCHAR*)password;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {

	}

	return status;
}

int LjgApi_getPassword(WCHAR* password)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	memset(parms, 0, sizeof(parms));
	API_USERNAME_PASSWORD_ARGS* args = (API_USERNAME_PASSWORD_ARGS*)parms;

	args->func_code = API_USERNAME_PASSWORD;
	args->type.val = TYPE_PASSWORD;
	args->operation.val = OPERATION_READ;
	args->data.val = (WCHAR*)password;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {

	}

	return status;
}

int LjgApi_setPath(WCHAR* path)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	memset(parms, 0, sizeof(parms));
	API_USERNAME_PASSWORD_ARGS* args = (API_USERNAME_PASSWORD_ARGS*)parms;

	args->func_code = API_USERNAME_PASSWORD;
	args->type.val = TYPE_PATH;
	args->operation.val = OPERATION_WRITE;
	args->data.val = (WCHAR*)path;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {

	}

	return status;
}

int LjgApi_getPath(WCHAR* path)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	memset(parms, 0, sizeof(parms));
	API_USERNAME_PASSWORD_ARGS* args = (API_USERNAME_PASSWORD_ARGS*)parms;

	args->func_code = API_USERNAME_PASSWORD;
	args->type.val = TYPE_PATH;
	args->operation.val = OPERATION_READ;
	args->data.val = (WCHAR*)path;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {

	}

	return status;
}



LONG SbieApi_DuplicateObject(
	HANDLE* TargetHandle,
	HANDLE OtherProcessHandle,
	HANDLE SourceHandle,
	ACCESS_MASK DesiredAccess,
	ULONG Options)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 ResultHandle;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_DUPLICATE_OBJECT_ARGS* args = (API_DUPLICATE_OBJECT_ARGS*)parms;

	memzero(parms, sizeof(parms));
	args->func_code = API_DUPLICATE_OBJECT;

	args->target_handle.val64 = (ULONG64)(ULONG_PTR)&ResultHandle;
	args->process_handle.val64 = (ULONG64)(ULONG_PTR)OtherProcessHandle;
	args->source_handle.val64 = (ULONG64)(ULONG_PTR)SourceHandle;
	args->desired_access.val = DesiredAccess;
	args->options.val = Options;

	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status))
		ResultHandle = 0;
	if (TargetHandle)
		*TargetHandle = (HANDLE*)ResultHandle;

	return status;
}


//---------------------------------------------------------------------------
// SbieApi_OpenProcess
//---------------------------------------------------------------------------


LONG SbieApi_OpenProcess(
	HANDLE* ProcessHandle,
	HANDLE ProcessId)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 ResultHandle;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_OPEN_PROCESS_ARGS* args = (API_OPEN_PROCESS_ARGS*)parms;

	memzero(parms, sizeof(parms));
	args->func_code = API_OPEN_PROCESS;

	args->process_id.val64 = (ULONG64)(ULONG_PTR)ProcessId;
	args->process_handle.val64 = (ULONG64)(ULONG_PTR)&ResultHandle;

	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status))
		ResultHandle = 0;
	if (ProcessHandle)
		*ProcessHandle = (HANDLE*)ResultHandle;

	return status;
}



LONG SbieApi_EnumProcessEx(
	const WCHAR* box_name,          // WCHAR [34]
	BOOLEAN all_sessions,
	ULONG which_session,            // -1 for current session
	ULONG* boxed_pids,              // ULONG [512]
	ULONG* boxed_count)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];

	memset(parms, 0, sizeof(parms));
	parms[0] = API_ENUM_PROCESSES;
	parms[1] = (ULONG64)(ULONG_PTR)boxed_pids;
	parms[2] = (ULONG64)(ULONG_PTR)box_name;
	parms[3] = (ULONG64)(ULONG_PTR)all_sessions;
	parms[4] = (ULONG64)(LONG_PTR)which_session;
	parms[5] = (ULONG64)(LONG_PTR)boxed_count;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status))
		boxed_pids[0] = 0;

	return status;
}


LONG SbieApi_EnumBoxes(
	LONG index, 
	WCHAR* box_name) 
{
	return SbieApi_EnumBoxesEx(index, box_name, FALSE);
}


LONG SbieApi_QueryConf(
	const WCHAR* section_name,
	const WCHAR* setting_name,
	ULONG setting_index,
	WCHAR* out_buffer,
	ULONG buffer_len)
{
	NTSTATUS status;
	__declspec(align(8)) UNICODE_STRING64 Output;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	WCHAR x_section[66];
	WCHAR x_setting[66];

	memzero(x_section, sizeof(x_section));
	memzero(x_setting, sizeof(x_setting));
	if (section_name)
		wcsncpy(x_section, section_name, 64);
	if (setting_name)
		wcsncpy(x_setting, setting_name, 64);

	Output.Length = 0;
	Output.MaximumLength = (USHORT)buffer_len;
	Output.Buffer = (ULONG64)(ULONG_PTR)out_buffer;

	memset(parms, 0, sizeof(parms));
	parms[0] = API_QUERY_CONF;
	parms[1] = (ULONG64)(ULONG_PTR)x_section;
	parms[2] = (ULONG64)(ULONG_PTR)x_setting;
	parms[3] = (ULONG64)(ULONG_PTR)&setting_index;
	parms[4] = (ULONG64)(ULONG_PTR)&Output;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		if (buffer_len > sizeof(WCHAR))
			out_buffer[0] = L'\0';
	}

	return status;
}


LONG SbieApi_EnumBoxesEx(
	LONG index, 
	WCHAR* box_name, 
	BOOLEAN return_all_sections)
{
	LONG rc;
	while (1) {
		++index;
		rc = SbieApi_QueryConf(NULL, NULL, index | CONF_GET_NO_TEMPLS | CONF_GET_NO_EXPAND,box_name, sizeof(WCHAR) * 34);
		if (rc == STATUS_BUFFER_TOO_SMALL)
			continue;
		if (!box_name[0])
			return -1;
		if (return_all_sections ||
			(SbieApi_IsBoxEnabled(box_name) == STATUS_SUCCESS))
			return index;
	}
}


LONG SbieApi_IsBoxEnabled(const WCHAR* box_name)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_IS_BOX_ENABLED_ARGS* args = (API_IS_BOX_ENABLED_ARGS*)parms;

	memzero(parms, sizeof(parms));
	args->func_code = API_IS_BOX_ENABLED;

	args->box_name.val64 = (ULONG64)(ULONG_PTR)box_name;

	status = SbieApi_Ioctl(parms);

	return status;
}




_FX LONG SbieApi_SetWatermark(BOOLEAN enable)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_SET_WATERMARK_ARGS* args = (API_SET_WATERMARK_ARGS*)parms;

	memset(parms, 0, sizeof(parms));
	args->func_code = API_SET_WATERMARK;
	args->enable.val = (VOID*)enable;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		OutputDebugStringW(L"SbieApi_SetWatermark error");
	}

	return status;
}

_FX DWORD SbieApi_QueryWatermark()
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_SET_WATERMARK_ARGS* args = (API_SET_WATERMARK_ARGS*)parms;

	DWORD enable = 0;
	memset(parms, 0, sizeof(parms));
	args->func_code = API_QUERY_WATERMARK;
	args->enable.val = &enable;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		OutputDebugStringW(L"SbieApi_QueryWatermark error");
	}

	return enable;
}

_FX LONG SbieApi_SetScreenshot(BOOLEAN enable)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_SET_SCREENSHOT_ARGS* args = (API_SET_SCREENSHOT_ARGS*)parms;

	memset(parms, 0, sizeof(parms));
	args->func_code = API_SET_SCREENSHOT;
	args->enable.val = (VOID*)enable;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		OutputDebugStringW(L"SbieApi_SetScreenshot error");
	}

	return status;
}

_FX DWORD SbieApi_QueryScreenshot()
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_SET_SCREENSHOT_ARGS* args = (API_SET_SCREENSHOT_ARGS*)parms;

	DWORD enable = 0;
	memset(parms, 0, sizeof(parms));
	args->func_code = API_QUERY_SCREENSHOT;
	args->enable.val = &enable;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		OutputDebugStringW(L"SbieApi_QueryScreenshot error");
	}

	return enable;
}


_FX LONG SbieApi_ResetProcessMonitor()
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS] = { 0 };
	API_RESET_PROCESS_MONITOR_ARGS* args = (API_RESET_PROCESS_MONITOR_ARGS*)parms;

	memset(parms, 0, sizeof(parms));
	args->func_code = API_RESET_PROCESS_MONITOR;

	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		OutputDebugStringW(L"SbieApi_ResetProcessMonitor error");
	}

	return status;
}

_FX LONG SbieApi_SetProcessMonitor(WCHAR* processname)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS] = { 0 };
	API_SET_PROCESS_MONITOR_ARGS* args = (API_SET_PROCESS_MONITOR_ARGS*)parms;

	memset(parms, 0, sizeof(parms));
	args->func_code = API_SET_PROCESS_MONITOR;
	args->processname.val = processname;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		OutputDebugStringW(L"SbieApi_SetProcessMonitor error");
	}

	return status;
}



// BOOLEAN LjgApi_SetPrinterControl(BOOLEAN enable) {
// 	g_printerControl = enable;
// 	return g_printerControl;
// }



 DWORD SbieApi_SetPrinter(BOOLEAN enable)
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_SET_PRINTER_ARGS* args = (API_SET_PRINTER_ARGS*)parms;

	memset(parms, 0, sizeof(parms));
	args->func_code = API_SET_PRINTER;
	args->enable.val = (VOID*)enable;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		OutputDebugStringW(L"SbieApi_SetPrinter error");
	}

	return status;
}

 DWORD SbieApi_QueryPrinter()
{
	NTSTATUS status;
	__declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	API_SET_PRINTER_ARGS* args = (API_SET_PRINTER_ARGS*)parms;

	DWORD enable = 0;
	memset(parms, 0, sizeof(parms));
	args->func_code = API_QUERY_PRINTER;
	args->enable.val = &enable;
	status = SbieApi_Ioctl(parms);

	if (!NT_SUCCESS(status)) {
		OutputDebugStringW(L"SbieApi_QueryPrinter error");
	}

	return enable;
}







 DWORD SbieApi_SetFileExport(BOOLEAN enable)
 {
	 NTSTATUS status;
	 __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	 API_SET_FILEEXPORT_ARGS* args = (API_SET_FILEEXPORT_ARGS*)parms;

	 memset(parms, 0, sizeof(parms));
	 args->func_code = API_SET_FILEEXPORT;
	 args->enable.val = (VOID*)enable;
	 status = SbieApi_Ioctl(parms);

	 if (!NT_SUCCESS(status)) {
		 OutputDebugStringW(L"SbieApi_SetFileExport error");
	 }

	 return status;
 }

 DWORD SbieApi_QueryFileExport()
 {
	 NTSTATUS status;
	 __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	 API_SET_FILEEXPORT_ARGS* args = (API_SET_FILEEXPORT_ARGS*)parms;

	 DWORD enable = 0;
	 memset(parms, 0, sizeof(parms));
	 args->func_code = API_QUERY_FILEEXPORT;
	 args->enable.val = &enable;
	 status = SbieApi_Ioctl(parms);

	 if (!NT_SUCCESS(status)) {
		 OutputDebugStringW(L"SbieApi_QueryFileExport error");
	 }

	 return enable;
 }



 /*
 message DenyPrintLog{
  string             spaceName         = 1;  //拦截空间名称
  string             programName       = 2;  //打印源程序名称
  string             programPath       = 3;  //打印源程序路径,可选？
  string             fileName          = 4;  //打印文件名称
  string             printerName       = 5;  //目标打印机名称
  string             remark            = 6;  //备注
  int64              time              = 7;  //发生时间戳
}
 */

 int alarmWarning(int type,LPVOID params) {

	 int result = 0;

	 char strboxname[64];
	 WCHAR procboxname[64] = { 0 };

	mylog(L"alarmWarning enter:%d", type);

	//__debugbreak();

	if (type == FILEEXPORT_WARNING)
	{
		//return FALSE;
	}
	else {
		
		WCHAR procsid[MAX_PATH];
		WCHAR procimage[MAX_PATH];
		ULONG sessionid = 0;
		HANDLE procid = (HANDLE)GetCurrentProcessId();
		result = SbieApi_QueryProcess((HANDLE)procid, procboxname, procimage, procsid, &sessionid);
		if (procboxname[0] == 0)
		{
			//mylog(L"process:%ws,pid:%d already in box:%ws\r\n", procimage, msg->process_id, procboxname);
			return FALSE;
		}
		
		WideCharToMultiByte(CP_ACP, 0, procboxname, -1, strboxname, sizeof(strboxname), 0, 0);
	}

	 HMODULE hdll = LoadLibraryW(SBIEDLL L".dll");
	 if (hdll == 0)
	 {
		 //mylog(L"LoadLibraryW SfDeskDll error\r\n");
		 return FALSE;
	 }

	 typedef int (*ptrSbie_writeAlert)(const WCHAR* dstfn, char* data, int datasize);
	 ptrSbie_writeAlert func = (ptrSbie_writeAlert)GetProcAddress(hdll, "Sbie_writeAlert");

	 char sztime[1024];

	 char szinfo[1024];
	 int infosize = 0;

	 SYSTEMTIME syst;
	 GetLocalTime(&syst);
	 const CHAR* format = "%04d/%02d/%02d %02d:%02d:%02d";

	 infosize = wsprintfA(sztime, format, syst.wYear, syst.wMonth, syst.wDay, syst.wHour, syst.wMinute, syst.wSecond);

	 WCHAR* shortname = 0;
	 if (type == SCREENSHOT_WARNING)
	 {
		 shortname = (WCHAR*)L"screenshot.wrn";
	 }
	 else if (type == FILEEXPORT_WARNING)
	 {
		 shortname = (WCHAR*)L"fileexport.wrn";

		 FILEEXPORT_PROHIBIT_PARAM* fexport = (FILEEXPORT_PROHIBIT_PARAM*)params;

		 infosize = wsprintfA(szinfo, "file export source file name:%s,dest file name:%s,box:%s,time:%s",
			 fexport->srcfn, fexport->dstfn, fexport->boxname, sztime);
		 lstrcpyA(strboxname, fexport->boxname);
		 MultiByteToWideChar(CP_ACP, 0, strboxname, -1, procboxname, sizeof(procboxname)/sizeof(WCHAR));
	 }
	 else if (type == WATERMARK_WARNING)
	 {
		 shortname = (WCHAR*)L"watermark.wrn";
	 }
	 else if (type == PRINTER_WARNING)
	 {
		 shortname = (WCHAR*)L"printer.wrn";

		 PRINTER_PROHIBIT_PARAM * printerparam = (PRINTER_PROHIBIT_PARAM*)params;

		 infosize = wsprintfA(szinfo, "printer:%s,file name:%s,box:%s,time:%s", printerparam->printername, printerparam->filename,strboxname, sztime);
	 }
	 else if (type == PROCESS_PROHIBIT)
	 {
		 shortname = (WCHAR*)L"process.wrn";

		 SVC_PROCESS_MSG msg = *(SVC_PROCESS_MSG*)params;

		 char processname[256];

		 WideCharToMultiByte(CP_ACP, 0, msg.process_name, -1, processname, sizeof(processname), 0, 0);

		 infosize = wsprintfA(szinfo, "processname:%s,pid:%u,box:%s,time:%s",processname, msg.process_id,strboxname, sztime);	 
	 }
	 else {
		 return FALSE;
	 }

	 WCHAR filename[MAX_PATH];

	 WCHAR username[64];
	 DWORD usernamelen = sizeof(username) / 2;
	 result = GetUserNameW(username, &usernamelen);
	 if (result)
	 {
		 username[usernamelen] = 0;
	 }

	 WCHAR veracryptpath[64];
	 wsprintfW(veracryptpath, L"%ws%lc", VERACRYPT_VOLUME_DEVICE, VERACRYPT_DISK_VOLUME[0]);

	 wsprintfW(filename, L"%ws\\%ws\\%ws\\warning\\%ws", veracryptpath, procboxname, username, shortname);

	 result = func(filename, szinfo, infosize);

	 mylog(L"alarmWarning filename:%ws result:%d",filename, result);
	 return result;
 }



 //---------------------------------------------------------------------------
// SbieApi_QueryProcess
//---------------------------------------------------------------------------


 _FX LONG SbieApi_QueryProcess(
	 HANDLE ProcessId,
	 WCHAR* out_box_name_wchar34,
	 WCHAR* out_image_name_wchar96,
	 WCHAR* out_sid_wchar96,
	 ULONG* out_session_id)
 {
	 return SbieApi_QueryProcessEx2(
		 ProcessId, 96, out_box_name_wchar34, out_image_name_wchar96,
		 out_sid_wchar96, out_session_id, NULL);
 }


 //---------------------------------------------------------------------------
 // SbieApi_QueryProcessEx
 //---------------------------------------------------------------------------


 _FX LONG SbieApi_QueryProcessEx(
	 HANDLE ProcessId,
	 ULONG image_name_len_in_wchars,
	 WCHAR* out_box_name_wchar34,
	 WCHAR* out_image_name_wcharXXX,
	 WCHAR* out_sid_wchar96,
	 ULONG* out_session_id)
 {
	 return SbieApi_QueryProcessEx2(
		 ProcessId, image_name_len_in_wchars, out_box_name_wchar34,
		 out_image_name_wcharXXX, out_sid_wchar96, out_session_id, NULL);
 }


 //---------------------------------------------------------------------------
 // SbieApi_QueryProcessEx2
 //---------------------------------------------------------------------------


 _FX LONG SbieApi_QueryProcessEx2(
	 HANDLE ProcessId,
	 ULONG image_name_len_in_wchars,
	 WCHAR* out_box_name_wchar34,
	 WCHAR* out_image_name_wcharXXX,
	 WCHAR* out_sid_wchar96,
	 ULONG* out_session_id,
	 ULONG64* out_create_time)
 {
	 NTSTATUS status;
	 __declspec(align(8)) UNICODE_STRING64 BoxName;
	 __declspec(align(8)) UNICODE_STRING64 ImageName;
	 __declspec(align(8)) UNICODE_STRING64 SidString;
	 __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
	 API_QUERY_PROCESS_ARGS* args = (API_QUERY_PROCESS_ARGS*)parms;

	 memzero(parms, sizeof(parms));
	 args->func_code = API_QUERY_PROCESS;

	 args->process_id.val64 = (ULONG64)(ULONG_PTR)ProcessId;

	 if (out_box_name_wchar34) {
		 BoxName.Length = 0;
		 BoxName.MaximumLength = (USHORT)(sizeof(WCHAR) * 34);
		 BoxName.Buffer = (ULONG64)(ULONG_PTR)out_box_name_wchar34;
		 args->box_name.val64 = (ULONG64)(ULONG_PTR)&BoxName;
	 }

	 if (out_image_name_wcharXXX) {
		 ImageName.Length = 0;
		 ImageName.MaximumLength =
			 (USHORT)(sizeof(WCHAR) * image_name_len_in_wchars);
		 ImageName.Buffer = (ULONG64)(ULONG_PTR)out_image_name_wcharXXX;
		 args->image_name.val64 = (ULONG64)(ULONG_PTR)&ImageName;
	 }

	 if (out_sid_wchar96) {
		 SidString.Length = 0;
		 SidString.MaximumLength = (USHORT)(sizeof(WCHAR) * 96);
		 SidString.Buffer = (ULONG64)(ULONG_PTR)out_sid_wchar96;
		 args->sid_string.val64 = (ULONG64)(ULONG_PTR)&SidString;
	 }

	 if (out_session_id)
		 args->session_id.val64 = (ULONG64)(ULONG_PTR)out_session_id;

	 if (out_create_time)
		 args->create_time.val64 = (ULONG64)(ULONG_PTR)out_create_time;

	 status = SbieApi_Ioctl(parms);

	 if (!NT_SUCCESS(status)) {

		 ULONG_PTR x = (ULONG_PTR)out_session_id;
		 if (x == 0 || x > 4) {

			 //
			 // reset parameters on error except when out_session_id
			 // is a special internal flag in the range 1 to 4
			 //

			 if (out_box_name_wchar34)
				 *out_box_name_wchar34 = L'\0';
			 if (out_image_name_wcharXXX)
				 *out_image_name_wcharXXX = L'\0';
			 if (out_sid_wchar96)
				 *out_sid_wchar96 = L'\0';
			 if (out_session_id)
				 *out_session_id = 0;
		 }
	 }

	 return status;
 }



 int __cdecl mylog(const WCHAR* format, ...) {
	 WCHAR szbuf[2048];

	 va_list   pArgList;

	 va_start(pArgList, format);

	 int nByteWrite = vswprintf_s(szbuf, format, pArgList);

	 va_end(pArgList);

	 OutputDebugStringW(szbuf);

	 return nByteWrite;
 }

 int __cdecl mylog(const CHAR* format, ...) {
	 CHAR szbuf[2048];

	 va_list   pArgList;

	 va_start(pArgList, format);

	 int nByteWrite = vsprintf_s(szbuf, format, pArgList);

	 va_end(pArgList);

	 OutputDebugStringA(szbuf);

	 return nByteWrite;
 }
