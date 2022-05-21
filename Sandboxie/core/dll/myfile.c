
#pragma once

#include <wchar.h>
#include "myfile.h"
#include "../common/win32_ntddk.h"
#include "../../common/my_version.h"





const WCHAR* g_FileFilterList[] = { THUMBCACHETODELETE,
THUMBCACHE_FILENAME_PREFIX ,
L"desktop.ini",
L".tmp",
L".ts",
L".TMP",
L"\\INetCache\\",
L"IconCacheToDelete"};

extern ULONGLONG File_CopyLimitKb;
extern ULONG Dll_BoxFilePathLen;
extern const WCHAR* Dll_BoxFilePath;
extern PSECURITY_DESCRIPTOR Secure_NormalSD;

extern P_NtQueryDirectoryFile       __sys_NtQueryDirectoryFile ;
extern P_NtDeleteFile               __sys_NtDeleteFile ;
extern P_NtReadFile                 __sys_NtReadFile ;
extern P_NtWriteFile                __sys_NtWriteFile ;
extern P_NtOpenFile                 __sys_NtOpenFile ;
extern P_NtCreateFile               __sys_NtCreateFile ;
extern P_NtQueryInformationFile     __sys_NtQueryInformationFile ;
extern P_NtClose                    __sys_NtClose ;

int __wcslen(const WCHAR* str) {
	int i = 0;
	while (str[i]) {
		i++;
	}
	return i;
}


WCHAR* __wcsrchr(const WCHAR* str, WCHAR wc) {
	int len = __wcslen(str);

	for (int i = len - 1; i >= 0; i--)
	{
		if (str[i] == wc)
		{
			return (WCHAR*)str + i;
		}
	}
	return 0;
}

WCHAR* __wcschr(const WCHAR* str, const WCHAR wc) {
	int i = 0;
	while (str[i])
	{
		if (str[i] == wc)
		{
			return (WCHAR*)str + i;
		}
		i++;
	}
	return 0;
}



int __wcscpy(WCHAR* str1, const WCHAR* str2) {
	int len2 = __wcslen(str2);

	memcpy(str1, str2, (len2 + 1) * sizeof(WCHAR));
	return len2;
}


int __wcscat(WCHAR* str1, const  WCHAR* str2) {
	int len1 = __wcslen(str1);
	int len2 = __wcslen(str2);
	memcpy(str1 + len1, str2, (len2 + 1) * sizeof(WCHAR));
	return len1 + len2;
}





//删除文件夹
//110080 200021             SYNCHRONIZE | DELETE  			FILE_OPEN_REPARSE_POINT|FILE_SYNCHRONOUS_IO_NONALERT|FILE_DIRECTORY_FILE
//10080 200040              DELETE | FILE_READ_ATTRIBUTES 	FILE_OPEN_REPARSE_POINT | FILE_NON_DIRECTORY_FILE

//重命名文件:
//110080 200020			    SYNCHRONIZE | DELETE  			FILE_OPEN_REPARSE_POINT|FILE_SYNCHRONOUS_IO_NONALERT
//10080  200040		        DELETE | FILE_READ_ATTRIBUTES 	FILE_OPEN_REPARSE_POINT | FILE_NON_DIRECTORY_FILE

int getPrevPath(WCHAR* path, WCHAR* prevpath) {
	prevpath[0] = 0;

	int pathlen = wcslen(path);
	int cnt = 0;
	WCHAR* hdr = 0;
	WCHAR* end = 0;
	for (int i = pathlen - 1; i >= 0; i--)
	{
		if (path[i] == '\\')
		{
			cnt++;
			if (cnt == 1)
			{
				end = path + i;
			}
			else if (cnt == 2)
			{
				hdr = path + i + 1;
				break;
			}
		}
	}
	if (hdr && end)
	{
		int prevlen = (int)(end - hdr);
		wmemcpy(prevpath, hdr, prevlen);
		prevpath[prevlen] = 0;
		return prevlen;
	}
	return 0;
}


int getFileParentPath(WCHAR* path, WCHAR* parent) {
	return getPrevPath(path, parent);
}

int getCurrentPath(const WCHAR* path, WCHAR* curpath) {
	curpath[0] = 0;

	WCHAR* pos = wcsrchr(path, L'\\');
	if (pos)
	{
		pos++;
		wcscpy(curpath, pos);
		return TRUE;
	}
	else {
		wcscpy(curpath, path);
		return TRUE;
	}
	return 0;
}

int getFilename(WCHAR* path, WCHAR* filename) {
	return getCurrentPath(path, filename);
}

int createNewRecyclePath(const WCHAR* copypath, WCHAR* recyclepath) {
	int result = 0;
	getRecyclePath(recyclepath);

	WCHAR currentpath[MAX_PATH];

	result = getCurrentPath(copypath, currentpath);
	wcscat(recyclepath, currentpath);

	NTSTATUS status;
	HANDLE hfile;
	OBJECT_ATTRIBUTES objattrs;
	UNICODE_STRING objname;
	IO_STATUS_BLOCK IoStatusBlock;

	InitializeObjectAttributes(&objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, Secure_NormalSD);

	status = RtlInitUnicodeString(&objname, recyclepath);
	status = __sys_NtCreateFile(&hfile, FILE_GENERIC_WRITE, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
		FILE_OPEN_IF, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
	{
		return FALSE;
	}
	else {
		__sys_NtClose(hfile);
	}

	return TRUE;
}

int createFullRecyclePath(const WCHAR* copypath) {
	NTSTATUS status;
	HANDLE hfile;
	OBJECT_ATTRIBUTES objattrs;
	UNICODE_STRING objname;
	IO_STATUS_BLOCK IoStatusBlock;

	InitializeObjectAttributes(&objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, Secure_NormalSD);

	WCHAR recyclepath[MAX_PATH];
	getRecyclePath(recyclepath);

	WCHAR leastpath[MAX_PATH];
	__wcscpy(leastpath, copypath + Dll_BoxFilePathLen + 1);

	WCHAR subpath[MAX_PATH];
	int subpathlen = 0;

	WCHAR path[MAX_PATH];
	__wcscpy(path, recyclepath);

	WCHAR* hdr = leastpath;
	WCHAR* end = leastpath;
	int leastpathlen = __wcslen(leastpath);

	while (hdr < leastpath + leastpathlen)
	{
		subpath[0] = 0;

		end = __wcschr(hdr, L'\\');
		if (end)
		{
			subpathlen = (int)(end - hdr);
			wmemcpy(subpath, hdr, subpathlen);
			subpath[subpathlen] = 0;
			end++;
			hdr = end;
		}
		else {
			__wcscpy(subpath, hdr);
			end = leastpath + leastpathlen;
			hdr = end;
		}

		__wcscat(path, subpath);
		__wcscat(path, L"\\");

		status = RtlInitUnicodeString(&objname, path);
		status = __sys_NtCreateFile(&hfile, FILE_GENERIC_WRITE, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
			FILE_OPEN_IF, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (!NT_SUCCESS(status))
		{
			return FALSE;
		}
		else {
			__sys_NtClose(hfile);
		}
	}
	return TRUE;
}

int copypath(const WCHAR* srcpath, const WCHAR* dstpath, BOOLEAN delsrc) {
	NTSTATUS status;

	OBJECT_ATTRIBUTES oa;

	UNICODE_STRING name;

	IO_STATUS_BLOCK iosb;

	HANDLE hfile;

	char temp[1024];

	FILE_BOTH_DIRECTORY_INFORMATION* DirInfo = (FILE_BOTH_DIRECTORY_INFORMATION*)temp;

	int cnt = 0;

	status = createPathRecursive(dstpath);

	//__debugbreak();

	InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, 0);

	RtlInitUnicodeString(&name, srcpath);
	status = __sys_NtOpenFile(&oa.RootDirectory, FILE_GENERIC_READ, &oa, &iosb, FILE_SHARE_VALID_FLAGS, FILE_SYNCHRONOUS_IO_NONALERT);
	if (NT_SUCCESS(status))
	{
		status = __sys_NtQueryDirectoryFile(oa.RootDirectory, NULL, NULL, NULL, &iosb, DirInfo, 1024, FileBothDirectoryInformation, TRUE, NULL, TRUE);

		while (NT_SUCCESS(status))
		{
			DirInfo->FileName[DirInfo->FileNameLength / sizeof(WCHAR)] = 0;

			//OutputDebugStringW(DirInfo->FileName);

			if (DirInfo->FileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			{
				//__debugbreak();

				WCHAR srcfile[MAX_PATH];
				wcscpy(srcfile, srcpath);
				wcscat(srcfile, L"\\");
				wcscat(srcfile, DirInfo->FileName);

				WCHAR dstfile[MAX_PATH];
				wcscpy(dstfile, dstpath);
				wcscat(dstfile, L"\\");
				wcscat(dstfile, DirInfo->FileName);
				status = copyfile(srcfile, dstfile, delsrc);
				cnt++;
			}
			else if (DirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (wcscmp(DirInfo->FileName, L".") == 0 || wcscmp(DirInfo->FileName, L"..") == 0)
				{
					//printf("online\r\n");
				}
				else {
					WCHAR nextsrcpath[MAX_PATH] = { 0 };
					wcscpy(nextsrcpath, srcpath);
					wcscat(nextsrcpath, L"\\");
					wcscat(nextsrcpath, DirInfo->FileName);

					WCHAR nextdstpath[MAX_PATH] = { 0 };
					wcscpy(nextdstpath, dstpath);
					wcscat(nextdstpath, L"\\");
					wcscat(nextdstpath, DirInfo->FileName);

					OBJECT_ATTRIBUTES next_oa;
					UNICODE_STRING next_name;
					InitializeObjectAttributes(&next_oa, &next_name, OBJ_CASE_INSENSITIVE, NULL, 0);

					status = RtlInitUnicodeString(&next_name, nextdstpath);
					status = __sys_NtCreateFile(&hfile, FILE_GENERIC_WRITE, &next_oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
						FILE_OPEN_IF, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
					if (!NT_SUCCESS(status))
					{
						break;
					}
					else {
						__sys_NtClose(hfile);
					}

					cnt += copypath(nextsrcpath, nextdstpath, delsrc);
				}
			}
			else {
				break;
			}

			if (DirInfo->NextEntryOffset)
			{
				DirInfo = (FILE_BOTH_DIRECTORY_INFORMATION*)((unsigned char*)DirInfo + DirInfo->NextEntryOffset);
				status = STATUS_SUCCESS;
			}
			else {
				DirInfo = (FILE_BOTH_DIRECTORY_INFORMATION*)temp;
				status = __sys_NtQueryDirectoryFile(oa.RootDirectory, NULL, NULL, NULL, &iosb, DirInfo, 1024,
					FileBothDirectoryInformation, TRUE, NULL, FALSE);
			}
		}
		__sys_NtClose(oa.RootDirectory);
	}

	return cnt;
}


extern int __CRTDECL Sbie_snwprintf(wchar_t* _Buffer, size_t Count, const wchar_t* const _Format, ...);

extern int __CRTDECL Sbie_snprintf(char* _Buffer, size_t Count, const char* const _Format, ...);


int createPathRecursive(const WCHAR* dstpath) {

	//__debugbreak();

	OBJECT_ATTRIBUTES next_oa;
	UNICODE_STRING next_name;
	InitializeObjectAttributes(&next_oa, &next_name, OBJ_CASE_INSENSITIVE, NULL, 0);

	NTSTATUS status;

	IO_STATUS_BLOCK iosb;

	WCHAR outinfo[1024];

	HANDLE hdir;

	WCHAR path[MAX_PATH];

	WCHAR dstfn[MAX_PATH];

	lstrcpyW(dstfn, dstpath);
	WCHAR* p = wcsrchr(dstfn, L'\\');
	if (p)
	{
		*p = 0;
	}

	WCHAR* hdr = wcsstr(dstfn, HARDDISK_VOLUME_DEVICE);
	if (hdr)
	{
		hdr = hdr + lstrlenW(HARDDISK_VOLUME_DEVICE) + 2;
	}
	else {
		hdr = wcsstr(dstfn, VERACRYPT_VOLUME_DEVICE);
		if (hdr)
		{
			hdr = hdr + lstrlenW(VERACRYPT_VOLUME_DEVICE) + 2;
		}
		else {
			return FALSE;
		}
	}

	int endflag = 0;

	while (1)
	{
		WCHAR *pos = wcschr(hdr, '\\');
		if (pos)
		{
			pos++;

			int len = pos - dstfn;
			wmemcpy(path, dstfn, len);
			path[len] = 0;

			hdr = pos;
		}
		else {
			lstrcpyW(path, dstfn);
			endflag = TRUE;
		}

		status = RtlInitUnicodeString(&next_name, path);
		status = __sys_NtCreateFile(&hdir, FILE_GENERIC_READ, &next_oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_OPEN, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (!NT_SUCCESS(status) )
		{

			if ((status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND))
			{
				// #define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)

				NTSTATUS errorcode = status;

				status = __sys_NtCreateFile(&hdir, FILE_GENERIC_WRITE, &next_oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE,
					FILE_OPEN_IF, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
				if (!NT_SUCCESS(status))
				{
					Sbie_snwprintf(outinfo, sizeof(outinfo) / sizeof(WCHAR), L"createPathRecursive create directory:%ws error code:%x,result:%x",
						path, GetLastError(), status);
					OutputDebugStringW(outinfo);

					return FALSE;
				}
				else {
					Sbie_snwprintf(outinfo, sizeof(outinfo) / sizeof(WCHAR), L"createPathRecursive path:%ws success,error code:%x,result:%x",
						path, errorcode, status);
					OutputDebugStringW(outinfo);
				}
			}
			else if (status == STATUS_ACCESS_DENIED)
			{
				//continue;
			}
			else {
// 				Sbie_snwprintf(outinfo, sizeof(outinfo) / sizeof(WCHAR), L"createPathRecursive path:%ws error,error code:%x,result:%x",
// 					path, GetLastError(), status);
// 				OutputDebugStringW(outinfo);
// 				return FALSE;
			}
		}
		else {
			__sys_NtClose(hdir);
		}

		if (endflag)
		{
			return TRUE;
		}
	}
	
	return FALSE;
}


int copyfile(const WCHAR* srcfn, const WCHAR* dstfn, BOOLEAN delsrc) {
	NTSTATUS status;
	HANDLE hfsrc, hfdst;
	OBJECT_ATTRIBUTES objattrs = { 0 };
	UNICODE_STRING objname = { 0 };
	IO_STATUS_BLOCK IoStatusBlock;

	char buffer[PAGE_SIZE];

	unsigned __int64 filesize;

	FILE_NETWORK_OPEN_INFORMATION fnetwork_openinfo;
	InitializeObjectAttributes(&objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, Secure_NormalSD);

	WCHAR outinfo[1024] = { 0 };

	RtlInitUnicodeString(&objname, srcfn);
	status = __sys_NtCreateFile(&hfsrc, FILE_GENERIC_READ , &objattrs, &IoStatusBlock,NULL, 0,
		FILE_SHARE_VALID_FLAGS,FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
	{
		Sbie_snwprintf(outinfo,sizeof(outinfo)/sizeof(WCHAR), L"copyfile  __sys_NtCreateFile:%ws error code:%x,result:%x",
			srcfn,	GetLastError(), status);
		OutputDebugStringW(outinfo);
		return status;
	}

	status = __sys_NtQueryInformationFile(hfsrc, &IoStatusBlock, &fnetwork_openinfo,sizeof(FILE_NETWORK_OPEN_INFORMATION), FileNetworkOpenInformation);
	filesize = fnetwork_openinfo.EndOfFile.QuadPart;
	if (!NT_SUCCESS(status) || (filesize > File_CopyLimitKb * 1024 ))
	{
		Sbie_snwprintf(outinfo, sizeof(outinfo) / sizeof(WCHAR),
			L"copyfile  __sys_NtQueryInformationFile:%ws error or file size overflow,error code:%x,result:%x,file size:%x",
			srcfn,GetLastError(),status,(DWORD)filesize);
		OutputDebugStringW(outinfo);

		__sys_NtClose(hfsrc);
		return status;
	}


	//STATUS_OBJECT_NAME_NOT_FOUND
// 	char* databuf = VirtualAlloc(0, filesize + 0x1000, MEM_COMMIT, PAGE_READWRITE);
// 	if (databuf <= 0)
// 	{
// 		Sbie_snwprintf(outinfo, sizeof(outinfo) / sizeof(WCHAR),
// 			L"copyfile  VirtualAlloc:%ws error code:%x,result:%x,file size:%x",
// 			srcfn, GetLastError(), status, (DWORD)filesize);
// 		OutputDebugStringW(outinfo);
// 		__sys_NtClose(hfsrc);
// 		return FALSE;
// 	}
// 	status = __sys_NtReadFile(hfsrc, NULL, NULL, NULL, &IoStatusBlock, databuf, filesize, NULL, NULL);
// 	if (!NT_SUCCESS(status)) {
// 		Sbie_snwprintf(outinfo, sizeof(outinfo) / sizeof(WCHAR),
// 			L"copyfile  __sys_NtReadFile:%ws error code:%x,result:%x,file size:%x",
// 			srcfn, GetLastError(), status, (DWORD)filesize);
// 		OutputDebugStringW(outinfo);
// 		__sys_NtClose(hfsrc);
// 		return FALSE;
// 	}
// 	__sys_NtClose(hfsrc);


	RtlInitUnicodeString(&objname, dstfn);
	status = __sys_NtCreateFile(&hfdst, FILE_GENERIC_WRITE|FILE_GENERIC_READ, &objattrs, &IoStatusBlock, 0, FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_VALID_FLAGS,FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
	{
		Sbie_snwprintf(outinfo, sizeof(outinfo) / sizeof(WCHAR),
			L"copyfile  __sys_NtCreateFile:%ws error code:%x,result:%x", dstfn, GetLastError(),status);
		OutputDebugStringW(outinfo);

		status = createPathRecursive(dstfn);

		RtlInitUnicodeString(&objname, dstfn);
		status = __sys_NtCreateFile(&hfdst, FILE_GENERIC_WRITE, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_VALID_FLAGS,FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (!NT_SUCCESS(status))
		{
			Sbie_snwprintf(outinfo, sizeof(outinfo) / sizeof(WCHAR), L"copyfile  __sys_NtCreateFile:%ws second time error code:%x,result:%x",
				dstfn, GetLastError(), status);
			OutputDebugStringW(outinfo);

			//SBIEAPI_EXPORT LONG SbieApi_VERACYPT_CopyFile(const WCHAR* srcpath,const WCHAR * dstpath)
			status = SbieApi_VERACYPT_CopyFile(&srcfn, dstfn);
			//status = SbieApi_VERACYPT_CopyFile(&srcfn, L"\\Device\\HarddiskVolume1\\mytest.txt");
			if (!NT_SUCCESS(status))
			{
				Sbie_snwprintf(outinfo, sizeof(outinfo) / sizeof(WCHAR), L"copyfile  __sys_NtCreateFile:%ws third time error code:%x,result:%x",
					dstfn, GetLastError(), status);
				OutputDebugStringW(outinfo);
				__sys_NtClose(hfsrc);
				return status;
			}
		}
	}

// 	status = __sys_NtWriteFile(hfdst, NULL, NULL, NULL, &IoStatusBlock, databuf, filesize, NULL, NULL);
// 	VirtualFree(databuf, 0, MEM_RELEASE );
	
	while (filesize > 0) {
		ULONG buffer_size = (filesize > PAGE_SIZE) ? PAGE_SIZE : (ULONG)filesize;

		status = __sys_NtReadFile(hfsrc, NULL, NULL, NULL, &IoStatusBlock, buffer, buffer_size, NULL, NULL);
		if (NT_SUCCESS(status)) {
			buffer_size = (ULONG)IoStatusBlock.Information;
			filesize -= (ULONGLONG)buffer_size;

			status = __sys_NtWriteFile(hfdst, NULL, NULL, NULL, &IoStatusBlock, buffer, buffer_size, NULL, NULL);
		}

		if (!NT_SUCCESS(status)) {

			break;
		}
	}

	if (delsrc)
	{
		status = RtlInitUnicodeString(&objname, srcfn);
		status = __sys_NtDeleteFile(&objattrs);
	}
	__sys_NtClose(hfsrc);
	__sys_NtClose(hfdst);

	return status;
}




int addDeleteRecord(const WCHAR* copypath, WCHAR* filename) {

	NTSTATUS status;
	HANDLE hfile;
	OBJECT_ATTRIBUTES objattrs;
	UNICODE_STRING objname;
	IO_STATUS_BLOCK IoStatusBlock;
	//FILE_NETWORK_OPEN_INFORMATION file_network_open_info;

	WCHAR recycleinfo[MAX_PATH];
	getRecycleInfoPath(recycleinfo);

	WCHAR realpath[MAX_PATH];
	WCHAR* veracryptvolume = VERACRYPT_VOLUME_DEVICE;
	int veravlen = wcslen(veracryptvolume);
	if (wmemcmp(copypath, veracryptvolume, veravlen) == 0)
	{
		WCHAR drive = copypath[veravlen];
		realpath[0] = drive;
		realpath[1] = ':';
		realpath[2] = '\\';
		realpath[3] = 0;
		wcscat(realpath, copypath + veravlen + 2);
	}
	else {
		wcscpy(realpath, copypath);
	}

	InitializeObjectAttributes(&objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, Secure_NormalSD);

	__wcscat(recycleinfo, filename);
	status = RtlInitUnicodeString(&objname, recycleinfo);
	status = __sys_NtCreateFile(&hfile, FILE_GENERIC_WRITE, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
		FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (NT_SUCCESS(status))
	{
		int realpathlen = __wcslen(realpath);
		char data[1024];
		GetLocalTime((SYSTEMTIME*)data);
		wmemcpy(data + sizeof(SYSTEMTIME), realpath, realpathlen);

		int writesize = realpathlen * sizeof(WCHAR) + sizeof(SYSTEMTIME);
		status = __sys_NtWriteFile(hfile, NULL, NULL, NULL, &IoStatusBlock, (LPVOID)data, writesize, NULL, NULL);
		__sys_NtClose(hfile);
	}
	else {

	}
	return status;
}

WCHAR boxname[64];

__declspec(dllexport) int keepFilesToRecycle(WCHAR * CopyPath,WCHAR * namebox,WCHAR *username,int FileType) {
	int myresult = 0;
	initOutsideBoxFileApis();

	//__debugbreak();

	if (Dll_BoxFilePathLen || Dll_BoxFilePath == 0)
	{
// 		Sbie_snwprintf(boxname, sizeof(boxname) / sizeof(WCHAR),
// 			L"%ws%lc\\%ws\\%ws", VERACRYPT_VOLUME_DEVICE, VERACRYPT_DISK_VOLUME[0],boxname,username);
// 		Dll_BoxFilePath = boxname;
// 		Dll_BoxFilePathLen = lstrlenW(Dll_BoxFilePath);

		wcscpy(boxname, VERACRYPT_VOLUME_DEVICE);
		//wcscat(boxname, L'X');
		WCHAR pathname[4] = { 0 };
		pathname[0] = VERACRYPT_DISK_VOLUME[0];
		wcscat(boxname, pathname);
		lstrcatW(boxname, L"\\");
		lstrcatW(boxname, namebox);
		lstrcatW(boxname, L"\\");
		lstrcatW(boxname, username);
		Dll_BoxFilePath = boxname;
		Dll_BoxFilePathLen = lstrlenW(Dll_BoxFilePath);
	}

	if (FileType & FILE_NON_DIRECTORY_FILE)
	{
		myresult = recycleFilePath(CopyPath, FALSE);

	}
	else if (FileType & FILE_DIRECTORY_FILE)
	{
		myresult = recycleDirPath(CopyPath);

	}
	return myresult;
}


int recycleFilePath(const WCHAR* copypath, BOOLEAN delsrc) {
	NTSTATUS status;
	HANDLE hfile;
	OBJECT_ATTRIBUTES objattrs;
	UNICODE_STRING objname;
	IO_STATUS_BLOCK IoStatusBlock;

	BOOLEAN firstfind = 0;

	InitializeObjectAttributes(&objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, Secure_NormalSD);

	WCHAR recyclepath[MAX_PATH];
	getRecyclePath(recyclepath);

	WCHAR subpath[MAX_PATH];

	WCHAR path[MAX_PATH];
	__wcscpy(path, recyclepath);

	WCHAR leastpath[MAX_PATH];
	__wcscpy(leastpath, copypath + Dll_BoxFilePathLen + 1);

	WCHAR filename[MAX_PATH];
	WCHAR* pos = wcsrchr(leastpath, L'\\');
	if (pos)
	{
		__wcscpy(filename, pos + 1);
		*pos = 0;
	}
	else {
		__wcscat(path, leastpath);
		status = copyfile(copypath, path, delsrc);

		addDeleteRecord(copypath, leastpath);

		return status;
	}

	int leastpathlen = __wcslen(leastpath);

	WCHAR* hdr = leastpath;
	WCHAR* end = leastpath;

	int subpathlen = 0;
	while (hdr < leastpath + leastpathlen)
	{
		subpath[0] = 0;

		end = __wcschr(hdr, L'\\');
		if (end)
		{
			subpathlen = (int)(end - hdr);
			wmemcpy(subpath, hdr, subpathlen);
			subpath[subpathlen] = 0;
			end++;
			hdr = end;
		}
		else {
			__wcscpy(subpath, hdr);
			end = leastpath + leastpathlen;
			hdr = end;
		}

		__wcscat(path, subpath);
		__wcscat(path, L"\\");

		status = RtlInitUnicodeString(&objname, path);
		status = __sys_NtCreateFile(&hfile, FILE_GENERIC_READ, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
			FILE_OPEN, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (!NT_SUCCESS(status))
		{
			if (firstfind)
			{
				__wcscpy(path, recyclepath);
				__wcscat(path, filename);
				status = copyfile(copypath, path, delsrc);
				addDeleteRecord(copypath, filename);
				return status;
			}
			else {
				__wcscpy(path, recyclepath);
			}
		}
		else {
			if (firstfind == FALSE)
			{
				firstfind = TRUE;
			}
			else {

			}
			__sys_NtClose(hfile);
		}
	}

	__wcscat(path, filename);
	status = copyfile(copypath, path, delsrc);
	if (firstfind == FALSE)
	{
		addDeleteRecord(copypath, filename);
	}
	return status;
}


int recycleDirPath(const WCHAR* copypath) {
	NTSTATUS status;
	HANDLE hfile;
	OBJECT_ATTRIBUTES objattrs;
	UNICODE_STRING objname;
	IO_STATUS_BLOCK IoStatusBlock;

	InitializeObjectAttributes(&objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, Secure_NormalSD);

	WCHAR recyclepath[MAX_PATH];
	getRecyclePath(recyclepath);

	WCHAR leastpath[MAX_PATH];
	__wcscpy(leastpath, copypath + Dll_BoxFilePathLen + 1);

	WCHAR currentpath[MAX_PATH];

	WCHAR prevpath[MAX_PATH];

	WCHAR subpath[MAX_PATH];
	int subpathlen = 0;

	WCHAR path[MAX_PATH];
	__wcscpy(path, recyclepath);

	WCHAR* hdr = leastpath;
	WCHAR* end = leastpath;
	int leastpathlen = __wcslen(leastpath);

	while (hdr < leastpath + leastpathlen)
	{
		subpath[0] = 0;

		end = __wcschr(hdr, L'\\');
		if (end)
		{
			subpathlen = (int)(end - hdr);
			wmemcpy(subpath, hdr, subpathlen);
			subpath[subpathlen] = 0;
			end++;
			hdr = end;
		}
		else {
			__wcscpy(subpath, hdr);
			end = leastpath + leastpathlen;
			hdr = end;
		}

		__wcscat(path, subpath);
		__wcscat(path, L"\\");

		status = RtlInitUnicodeString(&objname, path);
		status = __sys_NtCreateFile(&hfile, FILE_GENERIC_READ, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
			FILE_OPEN, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (!NT_SUCCESS(status))
		{
			getPrevPath(leastpath, prevpath);

			getCurrentPath(leastpath, currentpath);

			WCHAR newpath[MAX_PATH];
			__wcscpy(newpath, recyclepath);
			__wcscat(newpath, prevpath);
			status = RtlInitUnicodeString(&objname, newpath);
			status = __sys_NtCreateFile(&hfile, FILE_GENERIC_READ, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
				FILE_OPEN, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
			if (!NT_SUCCESS(status))
			{
				__wcscpy(newpath, recyclepath);
				__wcscat(newpath, currentpath);
				status = RtlInitUnicodeString(&objname, newpath);
				status = __sys_NtCreateFile(&hfile, FILE_GENERIC_WRITE, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
					FILE_OPEN_IF, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
				if (!NT_SUCCESS(status))
				{
					return FALSE;
				}
				else {
					__sys_NtClose(hfile);

					addDeleteRecord(copypath, currentpath);
					return TRUE;
				}
			}
			else {
				__wcscat(newpath, L"\\");
				__wcscat(newpath, currentpath);
				status = RtlInitUnicodeString(&objname, newpath);
				status = __sys_NtCreateFile(&hfile, FILE_GENERIC_WRITE, &objattrs, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
					FILE_OPEN_IF, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
				if (!NT_SUCCESS(status))
				{
					return FALSE;
				}
				else {
					__sys_NtClose(hfile);
					return TRUE;
				}
			}
		}
		else {
			__sys_NtClose(hfile);
		}
	}
	return TRUE;
}






int getRecycleInfoPath(WCHAR* recycle_info_path) {
	__wcscpy(recycle_info_path, Dll_BoxFilePath);
	__wcscat(recycle_info_path, VERAVOLUME_RECYCLE_INFO_PATH);
	return TRUE;
}


int getRecyclePath(WCHAR* recyclepath) {
	__wcscpy(recyclepath, Dll_BoxFilePath);
	__wcscat(recyclepath, VERAVOLUME_RECYCLE_PATH);
	return TRUE;
}


int isFilterPath(WCHAR* filepath, ULONG filetype) {
	WCHAR path[MAX_PATH];
	int result = 0;

	result = getRecycleInfoPath(path);
	if (wcsstr(filepath, path))
	{
		return TRUE;
	}

	result = getRecyclePath(path);
	if (wcsstr(filepath, path))
	{
		return TRUE;
	}

	//if (filetype & FILE_NON_DIRECTORY_FILE)
	{
		//WCHAR filename[MAX_PATH];
		//result = getFilename(filepath, filename);
		int cnt = sizeof(g_FileFilterList) / sizeof(WCHAR*);
		for (int i = 0; i < cnt; i++)
		{
			if (g_FileFilterList[i] && wcsstr(filepath, g_FileFilterList[i]))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}







int translatePath(const WCHAR* filename, WCHAR* filepath) {
	int veravl = wcslen(VERACRYPT_VOLUME_DEVICE);
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


int isRecyclePath(WCHAR* objectname) {

	int result = 0;

	if (wmemcmp(objectname, L"\\??\\", 4) == 0)
	{
		if (objectname[5] == ':' && objectname[6] == '\\')
		{
			WCHAR drive[2] = { 0 };
			drive[0] = objectname[4];
			if (drive[0] >= 'a' && drive[0] <= 'z')
			{
				drive[0] -= 0x20;
			}
			if (drive[0] != VERACRYPT_DISK_VOLUME[0])
			{
				return FALSE;
			}
		}
		else {
			return FALSE;
		}
	}
	else {
		return FALSE;
	}

	if (wcsstr(objectname, L"\\recycle") || wcsstr(objectname, L"\\recycle_info"))
	{
		return TRUE;
	}
	return FALSE;
}
