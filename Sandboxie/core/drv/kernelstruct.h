#ifndef _KERNELSTRUCT_H_
#define _KERNELSTRUCT_H_

#include <ntifs.h>
#include <windef.h>

//Kernel Structures 定义
typedef struct _CURDIR
{
	UNICODE_STRING DosPath;
	HANDLE Handle;
}CURDIR, * PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
	USHORT Flags;
	USHORT Length;
	ULONG TimeStamp;
	STRING DosPath;
}RTL_DRIVE_LETTER_CURDIR, * PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	PVOID StandardInput;
	PVOID StandardOutput;
	PVOID StandardError;
	CURDIR CurrentDirectory;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PVOID Environment;
	ULONG StartingX;
	ULONG StartingY;
	ULONG CountX;
	ULONG CountY;
	ULONG CountCharsX;
	ULONG CountCharsY;
	ULONG FillAttribute;
	ULONG WindowFlags;
	ULONG ShowWindowFlags;
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopInfo;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	RTL_DRIVE_LETTER_CURDIR CurrentDirectores[32];
	ULONG EnvironmentSize;
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB_LDR_DATA
{
	ULONG Length;
	UCHAR Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
}PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _PEB
{
	UCHAR InheritedAddressSpace;
	UCHAR ReadImageFileExecOptions;
	UCHAR BeingDebugged;
	UCHAR BitField;
	PVOID Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;
}PEB, * PPEB;

//pe文件结构定义

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

typedef struct _IMAGE_DOS_HEADER
{						// DOS .EXE header
	WORD e_magic;		// Magic number
	WORD e_cblp;		// Bytes on last page of file
	WORD e_cp;			// Pages in file
	WORD e_crlc;		// Relocations
	WORD e_cparhdr;		// Size of header in paragraphs
	WORD e_minalloc;	// Minimum extra paragraphs needed
	WORD e_maxalloc;	// Maximum extra paragraphs needed
	WORD e_ss;			// Initial (relative) SS value
	WORD e_sp;			// Initial SP value
	WORD e_csum;		// Checksum
	WORD e_ip;			// Initial IP value
	WORD e_cs;			// Initial (relative) CS value
	WORD e_lfarlc;		// File address of relocation table
	WORD e_ovno;		// Overlay number
	WORD e_res[4];		// Reserved words
	WORD e_oemid;		// OEM identifier (for e_oeminfo)
	WORD e_oeminfo;		// OEM information; e_oemid specific
	WORD e_res2[10];	// Reserved words
	LONG e_lfanew;		// File address of new exe header
}IMAGE_DOS_HEADER, * PIMAGE_DOS_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY
{
	DWORD   VirtualAddress;
	DWORD   Size;
}IMAGE_DATA_DIRECTORY, * PIMAGE_DATA_DIRECTORY;

/////////////////////////////////////////////////////////////////////////////

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES  16

/////////////////////////////////////////////////////////////////////////////

typedef struct _IMAGE_OPTIONAL_HEADER
{
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	DWORD BaseOfData;
	DWORD ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Win32VersionValue;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	DWORD SizeOfStackReserve;
	DWORD SizeOfStackCommit;
	DWORD SizeOfHeapReserve;
	DWORD SizeOfHeapCommit;
	DWORD LoaderFlags;
	DWORD NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
}IMAGE_OPTIONAL_HEADER32, * PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_EXPORT_DIRECTORY
{
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	DWORD Name;
	DWORD Base;
	DWORD NumberOfFunctions;
	DWORD NumberOfNames;
	DWORD AddressOfFunctions;     // RVA from base of image
	DWORD AddressOfNames;         // RVA from base of image
	DWORD AddressOfNameOrdinals;  // RVA from base of image
}IMAGE_EXPORT_DIRECTORY, * PIMAGE_EXPORT_DIRECTORY;

#define IMAGE_DIRECTORY_ENTRY_EXPORT  0   // Export Directory

//////////////////////////////////////////////////////////////////////////

//typedef struct _SYSTEM_THREAD_INFORMATION
//{
//	LARGE_INTEGER KernelTime;
//	LARGE_INTEGER UserTime;
//	LARGE_INTEGER CreateTime;
//	PVOID WaitTime;
//	PVOID StartAddress;
//	CLIENT_ID ClientId;
//	KPRIORITY Priority;
//	LONG BasePriority;
//	ULONG ContextSwitches;
//	ULONG ThreadState;
//	KWAIT_REASON WaitReason;
//}SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;
//////////////////////////////////////////////////////////////////////////

#define	SYSTEM_PROCESS_INFO_TYPE	5

//typedef struct _SYSTEM_PROCESS_INFORMATION {
//	ULONG NextEntryOffset;
//	ULONG NumberOfThreads;
//	BYTE Reserved1[48];
//	PVOID Reserved2[3];
//	HANDLE ProcessId;
//	HANDLE InheritedFromProcessId;
//	ULONG HandleCount;
//	BYTE Reserved4[4];
//	PVOID Reserved5[11];
//	SIZE_T PeakPagefileUsage;
//	SIZE_T PrivatePageCount;
//	LARGE_INTEGER Reserved6[6];
//	PSYSTEM_THREAD_INFORMATION Threads; ///> 由于[0]数组报错，设置占位符
//}SYSTEM_PROCESS_INFORMATION;

/////////////////////////////////////////////////////////////////////////////

typedef struct _IMAGE_FILE_HEADER
{
	WORD Machine;
	WORD NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD SizeOfOptionalHeader;
	WORD Characteristics;
}IMAGE_FILE_HEADER, * PIMAGE_FILE_HEADER;

/////////////////////////////////////////////////////////////////////////////

typedef struct
{
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	DWORD Type;
	DWORD SizeOfData;
	DWORD AddressOfRawData;
	DWORD PointerToRawData;
}IMAGE_DEBUG_DIRECTORY, * PIMAGE_DEBUG_DIRECTORY;

/////////////////////////////////////////////////////////////////////////////

typedef struct _IMAGE_DEBUG_MISC
{
	DWORD DataType;		// type of misc data, see defines
	DWORD Length;		// total length of record, rounded to four
	// byte multiple.
	BOOLEAN Unicode;	// TRUE if data is unicode string
	BYTE Reserved[3];
	BYTE Data[1];		// Actual data
}IMAGE_DEBUG_MISC, * PIMAGE_DEBUG_MISC;

typedef struct _IMAGE_NT_HEADERS {
	DWORD Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, * PIMAGE_NT_HEADERS32;

/////////////////////////////////////////////////////////////////////////////

typedef struct _IMAGE_OPTIONAL_HEADER64 {
	WORD        Magic;
	BYTE        MajorLinkerVersion;
	BYTE        MinorLinkerVersion;
	DWORD       SizeOfCode;
	DWORD       SizeOfInitializedData;
	DWORD       SizeOfUninitializedData;
	DWORD       AddressOfEntryPoint;
	DWORD       BaseOfCode;
	ULONGLONG   ImageBase;
	DWORD       SectionAlignment;
	DWORD       FileAlignment;
	WORD        MajorOperatingSystemVersion;
	WORD        MinorOperatingSystemVersion;
	WORD        MajorImageVersion;
	WORD        MinorImageVersion;
	WORD        MajorSubsystemVersion;
	WORD        MinorSubsystemVersion;
	DWORD       Win32VersionValue;
	DWORD       SizeOfImage;
	DWORD       SizeOfHeaders;
	DWORD       CheckSum;
	WORD        Subsystem;
	WORD        DllCharacteristics;
	ULONGLONG   SizeOfStackReserve;
	ULONGLONG   SizeOfStackCommit;
	ULONGLONG   SizeOfHeapReserve;
	ULONGLONG   SizeOfHeapCommit;
	DWORD       LoaderFlags;
	DWORD       NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, * PIMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_NT_HEADERS64 {
	DWORD Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, * PIMAGE_NT_HEADERS64;

//PPEB NTAPI PsGetProcessPeb(PEPROCESS);

#endif //_KERNELSTRUCT_H_
