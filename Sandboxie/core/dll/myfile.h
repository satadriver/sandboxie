#pragma once

#include <windows.h>

#define VERAVOLUME_RECYCLE_COPY_PATH	L"\\copy_files\\"

#define VERAVOLUME_RECYCLE_INFO_PATH	L"\\recycle_info\\"

#define VERAVOLUME_RECYCLE_PATH			L"\\recycle\\"

#define VERACRYPT_VOLUME_DEVICE			L"\\Device\\VeraCryptVolume"

#define HARDDISK_VOLUME_DEVICE			L"\\Device\\HarddiskVolume"

#define THUMBCACHE_FILENAME_PREFIX		L"\\thumbcache_"

#define THUMBCACHETODELETE				L"\\ThumbCacheToDelete"


int createPathRecursive(const WCHAR* dstpath);

__declspec(dllexport) int keepFilesToRecycle(WCHAR* CopyPath, WCHAR* namebox, WCHAR* username, int FileType);

int recycleFilePath(const WCHAR* copypath, BOOLEAN delsrc);

int recycleDirPath(const WCHAR* copypath);

int isFilterPath(WCHAR* filepath, ULONG filetype);

int getRecycleInfoPath(WCHAR* recyclepath);

int getRecyclePath(WCHAR* recycle_info_path);



int copyfile(const WCHAR* srcfn, const WCHAR* dstfn,BOOLEAN delsrc);

int copypath(const WCHAR* srcpath, const WCHAR* dstpath, BOOLEAN delsrc);

int createFullRecyclePath(const WCHAR* copypath);

int createNewRecyclePath(const WCHAR* copypath, WCHAR* recyclepath);

int getFileParentPath(WCHAR* path, WCHAR* parent);

int getCurrentPath(const WCHAR* path, WCHAR* curpath);

int getFilename(WCHAR* path, WCHAR* filename);

int getPrevPath(WCHAR* path, WCHAR* prevpath);

int __wcslen(const WCHAR* str);

WCHAR* __wcsrchr(const WCHAR* str, WCHAR wc);

WCHAR* __wcschr(const WCHAR* str, const WCHAR wc);

int __wcscpy(WCHAR* str1, const WCHAR* str2);

int __wcscat(WCHAR* str1, const  WCHAR* str2);

int isRecyclePath(WCHAR* TruePath);

int addDeleteRecord(const WCHAR* copypath, WCHAR* filename);



/*
DesiredAccess:

#define DELETE                           (0x00010000L)
#define READ_CONTROL                     (0x00020000L)
#define WRITE_DAC                        (0x00040000L)
#define WRITE_OWNER                      (0x00080000L)
#define SYNCHRONIZE                      (0x00100000L)
#define FILE_READ_DATA            ( 0x0001 )    // file & pipe
#define FILE_LIST_DIRECTORY       ( 0x0001 )    // directory
#define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
#define FILE_ADD_FILE             ( 0x0002 )    // directory
#define FILE_APPEND_DATA          ( 0x0004 )    // file
#define FILE_ADD_SUBDIRECTORY     ( 0x0004 )    // directory
#define FILE_CREATE_PIPE_INSTANCE ( 0x0004 )    // named pipe
#define FILE_READ_EA              ( 0x0008 )    // file & directory
#define FILE_WRITE_EA             ( 0x0010 )    // file & directory
#define FILE_EXECUTE              ( 0x0020 )    // file
#define FILE_TRAVERSE             ( 0x0020 )    // directory
#define FILE_DELETE_CHILD         ( 0x0040 )    // directory
#define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all
#define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all


CreateDisposition:

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005



CreateOptions:
#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080
#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_REMOTE_INSTANCE               0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800
#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000
#if (NTDDI_VERSION >= NTDDI_WIN7)
#define FILE_OPEN_REQUIRING_OPLOCK              0x00010000
#define FILE_DISALLOW_EXCLUSIVE                 0x00020000
#endif
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define FILE_SESSION_AWARE                      0x00040000
#endif 

//
//  CreateOptions flag to pass in call to CreateFile to allow the write through xro.sys
//

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000
*/
