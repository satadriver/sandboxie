#include "kernelstruct.h"
#include "apcinject.h"
#include "memory.h"
#include "path.h"
#include "process.h"
#include "..\..\common\str_util.h"

#include <ntstrsafe.h>
#include <ntifs.h>

#define MIN_USER_PROCESSID                  8
#define NTDLLNAME_SIZE                      32
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC       0x20b
#define SIZE_DOUBLE                         2
#define ADDRESS_INTERVALSIZE                0x20
#define MEMPAGE_ALIGNMENT(size) ((size + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1)));
#define	SYSTEM_PROCESS_INFO_TYPE	5

extern WCHAR* Driver_HomePathDos;
extern WCHAR* Driver_HomePathNt;
static ULONG gs_ApcHookPoolTag = 'sapc';

PWCHAR WhiteApcProcessSystem[] =
{
	L"explorer.exe",
	L"system32",
	L"syswow64",
	L"SystemApps",
	L"WindowsApps",
};

PWCHAR WhiteApcProcessSelf[] =
{
	L"ToDesk.exe",
	L"ToDesk_Service.exe",

	L"SafeDesktopInstall64.exe",
	L"Start.exe",
	L"desktop.exe",
	//L"sfDeskExplorer.exe",
	L"SafeDesktop.exe",
	L"qsdpclient.exe",
	L"sdptunnel.exe",
	L"SdpClientStart.exe",
	L"sfDeskSvc.exe",
	L"KmdUtil.exe",
	L"processInjector.exe",
	L"SafeDesktopBITS.exe",
	L"SafeDesktopCrypto.exe",
	L"SafeDesktopDcomLaunch.exe",
	L"SafeDesktopRpcSs.exe",
	L"SafeDesktopWUAU.exe",
	L"SfDeskCtrl.exe",
	L"SfDeskIni.exe",
	L"VeraCrypt.exe",
	L"VeraCrypt Format.exe",
	L"VeraCryptExpander.exe"
};

//NTSTATUS NTAPI ZwQuerySystemInformation(
//	IN ULONG SystemInformationClass,
//	OUT PVOID SystemInformation,
//	IN ULONG SystemInformationLength,
//	OUT PULONG ReturnLength
//);

HANDLE GetProcessHandleByPid(HANDLE ProcessPid)
{
	NTSTATUS Status;
	HANDLE ProcessHandle;
	OBJECT_ATTRIBUTES objectAttributes;
	CLIENT_ID client;
	client.UniqueProcess = ProcessPid;
	client.UniqueThread = NULL;
	InitializeObjectAttributes(&objectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	Status = ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &objectAttributes, &client);
	if (NT_SUCCESS(Status))
	{
		return ProcessHandle;
	}
	return 0;
}

VOID KernelRoutineCallback(
	PKAPC Apc,
	PKNORMAL_ROUTINE* NormalRoutine,
	PVOID* NormalContext,
	PVOID* SystemArgument1,
	PVOID* SystemArgument2
)
{
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);

	///> Skip execution
	if (NormalRoutine && PsIsThreadTerminating(PsGetCurrentThread()))
	{
		*NormalRoutine = NULL;
	}

	KeTestAlertThread(UserMode);

#ifdef _WIN64
	///> Fix Wow64 APC
	if (PsGetCurrentProcessWow64Process() != NULL)
	{
		PsWrapApcWow64Thread(
			NormalContext,
			(PVOID*)NormalRoutine
		);
	}
#else
	UNREFERENCED_PARAMETER(NormalContext);
#endif

	if (Apc)
	{
		MemFree(Apc);
	}
	return;
}

VOID NTAPI NativeInjectApcDll(
	PVOID NormalContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2
)
{
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);

	PVOID DllBaseAddress = NULL;
	PKINJECT pArgBuff = (PKINJECT)NormalContext;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PStartMonitor pfnStartMonitor = NULL;

	do
	{
		if (!pArgBuff ||
			!pArgBuff->LdrLoadDll ||
			!pArgBuff->LdrGetProcAddress)
		{
			break;
		}

		status = pArgBuff->LdrLoadDll(
			NULL,
			NULL,
			&pArgBuff->DllPath,
			&DllBaseAddress
		);

		if (!NT_SUCCESS(status) || !DllBaseAddress)
		{
			break;
		}

		status = pArgBuff->LdrGetProcAddress(
			DllBaseAddress,
			NULL,
			UEM_HOOK_MODULE_START_MONITOR_FUCN_ID,
			(PVOID*)(&pfnStartMonitor)
		);
		if (!NT_SUCCESS(status) || !pfnStartMonitor)
		{
			break;
		}

		(void)pfnStartMonitor(&pArgBuff->InjectInfo);
	} while (FALSE);

	return;
}

VOID NTAPI NativeInjectApcEnd()
{
	///> Mark an end address.
}

NTSTATUS GetNativeInjectCode(
	IN PVOID pLdrLoadDll,
	IN PUNICODE_STRING pusPath,
	IN PVOID pLdrGetProcAddress,
	IN PAppInjectInfo pInjectInfo,
	IN PVOID pExtBuffer,
	IN ULONG uExtBufSize,
	OUT PVOID* ppCodeBuffer,
	OUT PVOID* ppArgBuffer
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	PVOID pCodeBase = NULL;
	PKINJECT pArgBase = NULL;
	ULONG_PTR ulAddMax = 0;
	ULONG_PTR ulAddMin = 0;
	SIZE_T functionSize = 0;
	SIZE_T sizeWrite = 0;
	SIZE_T sizeAlloc = 0;

	do
	{
		if (!pLdrLoadDll ||
			!pusPath ||
			!pusPath->Buffer ||
			!pLdrGetProcAddress ||
			!pInjectInfo ||
			!ppCodeBuffer ||
			!ppArgBuffer)
		{
			break;
		}

		if (pExtBuffer)
		{
			NT_ASSERT(uExtBufSize > 0);
		}

		ulAddMax = max((ULONG_PTR)NativeInjectApcEnd, (ULONG_PTR)NativeInjectApcDll);
		ulAddMin = min((ULONG_PTR)NativeInjectApcEnd, (ULONG_PTR)NativeInjectApcDll);
		functionSize = (SIZE_T)(ulAddMax - ulAddMin);

		///> 加入初始代码部分大小
		sizeAlloc += (functionSize + ADDRESS_INTERVALSIZE);

		///> 加入参数部分大小，包括KINJECT结构体和额外数据长度
		sizeAlloc += (sizeof(KINJECT) + uExtBufSize + ADDRESS_INTERVALSIZE);
		sizeAlloc = MEMPAGE_ALIGNMENT(sizeAlloc);

		status = ZwAllocateVirtualMemory(
			ZwCurrentProcess(),
			(PVOID*)&pCodeBase,
			0,
			&sizeAlloc,
			MEM_COMMIT,
			PAGE_EXECUTE_READWRITE
		);
		if (!NT_SUCCESS(status) || !pCodeBase)
		{
			break;
		}

		RtlCopyMemory(
			pCodeBase,
			(void*)NativeInjectApcDll,
			functionSize
		);

		pArgBase = (PKINJECT)((ULONG_PTR)pCodeBase + (functionSize + ADDRESS_INTERVALSIZE));
		pArgBase->LdrLoadDll = (PLDR_LOAD_DLL)pLdrLoadDll;
		pArgBase->LdrGetProcAddress = (PLdrGetProcAddress)pLdrGetProcAddress;
		RtlCopyMemory(
			&pArgBase->InjectInfo,
			pInjectInfo,
			sizeof(AppInjectInfo)
		);

		if (pExtBuffer && uExtBufSize > 0)
		{
			///> 附加数据写入
			RtlCopyMemory(
				pArgBase->InjectInfo.extData,
				pExtBuffer,
				uExtBufSize
			);
			pArgBase->InjectInfo.extSize = uExtBufSize;
		}

		sizeWrite = pusPath->Length;
		if (sizeWrite >= MAX_PATH * sizeof(WCHAR))
		{
			sizeWrite = (MAX_PATH - 1) * sizeof(WCHAR);
		}
		RtlCopyMemory(
			pArgBase->Buffer,
			pusPath->Buffer,
			sizeWrite
		);

		RtlInitUnicodeString(
			&pArgBase->DllPath,
			pArgBase->Buffer
		);

		*ppCodeBuffer = pCodeBase;
		*ppArgBuffer = pArgBase;
		status = STATUS_SUCCESS;
	} while (FALSE);

	return status;
}

NTSTATUS GetWow64InjectCode(
	IN PVOID pLdrLoadDll,
	IN PUNICODE_STRING pusPath,
	IN PVOID pLdrGetProcAddress,
	IN PAppInjectInfo pInjectInfo,
	IN PVOID pExtBuffer,
	IN ULONG uExtBufSize,
	OUT PVOID* ppCodeBuffer,
	OUT PVOID* ppArgBuffer
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PVOID pCodeBase = NULL;
	PKINJECT32 pArgBase = NULL;
	PUNICODE_STRING32 pusDllPath = NULL;
	USHORT usSizeWrite = 0;
	SIZE_T functionSize = 0;
	SIZE_T sizeAlloc = 0;

	///> x86 ShellCode
	UCHAR ShellCode[] =
	{
		0x55,                   ///> push    ebp
		0x8b, 0xec,             ///> mov     ebp, esp
		0x83, 0xec, 0x10,       ///> sub     esp, 10h
		0xc7, 0x45, 0xf4,       ///> mov     dword ptr[ebp - 0Ch], 0
		0x00, 0x00, 0x00, 0x00, ///> 接上一行
		0x8b, 0x45, 0x08,       ///> mov     eax, dword ptr[ebp + 8]
		0x89, 0x45, 0xfc,       ///> mov     dword ptr[ebp - 4], eax
		0xc7, 0x45, 0xf8,       ///> mov     dword ptr[ebp - 8], 0C0000001h
		0x01, 0x00, 0x00, 0xc0, ///> 接上一行
		0xc7, 0x45, 0xf0,       ///> mov     dword ptr[ebp - 10h], 0
		0x00, 0x00, 0x00, 0x00, ///> 接上一行
		0x83, 0x7d, 0xfc, 0x00, ///> cmp     dword ptr[ebp - 4], 0
		0x74, 0x14,             ///> je      NativeInjectApcDll + 0x3b
		0x8b, 0x4d, 0xfc,       ///> mov     ecx, dword ptr[ebp - 4]
		0x83, 0x39, 0x00,       ///> cmp     dword ptr[ecx], 0
		0x74, 0x0c,             ///> je      NativeInjectApcDll + 0x3b
		0x8b, 0x55, 0xfc,       ///> mov     edx, dword ptr[ebp - 4]
		0x83, 0xba, 0x14, 0x02, 0x00, 0x00, 0x00, ///> cmp dword ptr[edx + 214h], 0
		0x75, 0x02,             ///> jne     NativeInjectApcDll + 0x3d
		0xeb, 0x60,             ///> jmp     NativeInjectApcDll + 0x9d
		0x8d, 0x45, 0xf4,       ///> lea     eax, [ebp - 0Ch]
		0x50,                   ///> push    eax
		0x8b, 0x4d, 0xfc,       ///> mov     ecx, dword ptr[ebp - 4]
		0x83, 0xc1, 0x04,       ///> add     ecx, 4
		0x51,                   ///> push    ecx
		0x6a, 0x00,             ///> push    0
		0x6a, 0x00,             ///> push    0
		0x8b, 0x55, 0xfc,       ///> mov     edx, dword ptr[ebp - 4]
		0x8b, 0x02,             ///> mov     eax, dword ptr[edx]
		0xff, 0xd0,             ///> call    eax
		0x89, 0x45, 0xf8,       ///> mov     dword ptr[ebp - 8], eax
		0x83, 0x7d, 0xf8, 0x00, ///> cmp     dword ptr[ebp - 8], 0
		0x7c, 0x06,             ///> jl      NativeInjectApcDll + 0x62
		0x83, 0x7d, 0xf4, 0x00, ///> cmp     dword ptr[ebp - 0Ch], 0
		0x75, 0x02,             ///> jne     NativeInjectApcDll + 0x64
		0xeb, 0x39,             ///> jmp     NativeInjectApcDll + 0x9d
		0x8d, 0x4d, 0xf0,       ///> lea     ecx, [ebp - 10h]
		0x51,                   ///> push    ecx
		0x6a, 0x01,             ///> push    1
		0x6a, 0x00,             ///> push    0
		0x8b, 0x55, 0xf4,       ///> mov     edx, dword ptr[ebp - 0Ch]
		0x52,                   ///> push    edx
		0x8b, 0x45, 0xfc,       ///> mov     eax, dword ptr[ebp - 4]
		0x8b, 0x88, 0x14, 0x02, 0x00, 0x00, ///> mov ecx, dword ptr[eax + 214h]
		0xff, 0xd1,             ///> call    ecx
		0x89, 0x45, 0xf8,       ///> mov     dword ptr[ebp - 8], eax
		0x83, 0x7d, 0xf8, 0x00, ///> cmp     dword ptr[ebp - 8], 0
		0x7c, 0x06,             ///> jl      NativeInjectApcDll + 0x8a
		0x83, 0x7d, 0xf0, 0x00, ///> cmp     dword ptr[ebp - 10h], 0
		0x75, 0x02,             ///> jne     NativeInjectApcDll + 0x8c
		0xeb, 0x11,             ///> jmp     NativeInjectApcDll + 0x9d
		0x8b, 0x55, 0xfc,       ///> mov     edx, dword ptr[ebp - 4]
		0x81, 0xc2, 0x18, 0x02, 0x00, 0x00, ///> add     edx, 218h
		0x52,                   ///> push    edx
		0xff, 0x55, 0xf0,       ///> call    dword ptr[ebp - 10h]
		0x33, 0xc0,             ///> xor     eax, eax
		0x75, 0x84,             ///> jne     NativeInjectApcDll + 0x21
		0x8b, 0xe5,             ///> mov     esp, ebp
		0x5d,                   ///> pop     ebp
		0xc2, 0x0c, 0x00,       ///> ret     0Ch
	};

	do
	{
		if (!pLdrLoadDll ||
			!pusPath ||
			!pusPath->Buffer ||
			!pLdrGetProcAddress ||
			!pInjectInfo ||
			!ppCodeBuffer ||
			!ppArgBuffer)
		{
			break;
		}

		if (pExtBuffer)
		{
			NT_ASSERT(uExtBufSize > 0);
		}

		functionSize = sizeof(ShellCode);

		///> 加入初始代码部分大小
		sizeAlloc += (functionSize + ADDRESS_INTERVALSIZE);

		///> 加入参数部分大小，包括KINJECT结构体和额外数据长度
		sizeAlloc += (sizeof(KINJECT32) + uExtBufSize + ADDRESS_INTERVALSIZE);
		sizeAlloc = MEMPAGE_ALIGNMENT(sizeAlloc);

		status = ZwAllocateVirtualMemory(
			ZwCurrentProcess(),
			(PVOID*)&pCodeBase,
			0,
			&sizeAlloc,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE
		);
		if (!NT_SUCCESS(status) || !pCodeBase)
		{
			break;
		}

		RtlCopyMemory(pCodeBase, ShellCode, functionSize);

		pArgBase = (PKINJECT32)((ULONG_PTR)pCodeBase + (functionSize + ADDRESS_INTERVALSIZE));
		pArgBase->LdrLoadDll = (ULONG)(ULONG_PTR)pLdrLoadDll;
		pArgBase->LdrGetProcAddress = (ULONG)(ULONG_PTR)pLdrGetProcAddress;
		RtlCopyMemory(
			&pArgBase->InjectInfo,
			pInjectInfo,
			sizeof(AppInjectInfo)
		);

		if (pExtBuffer && uExtBufSize > 0)
		{
			///> 附加数据写入
			RtlCopyMemory(
				pArgBase->InjectInfo.extData,
				pExtBuffer,
				uExtBufSize
			);
			pArgBase->InjectInfo.extSize = uExtBufSize;
		}

		///> Fill DllPath
		usSizeWrite = pusPath->Length;
		if (usSizeWrite >= MAX_PATH * sizeof(WCHAR))
		{
			///> 避免越界
			usSizeWrite = (MAX_PATH - 1) * sizeof(WCHAR);
		}

		pusDllPath = &pArgBase->DllPath;
		pusDllPath->Length = usSizeWrite;
		pusDllPath->MaximumLength = pusPath->MaximumLength;
		pusDllPath->Buffer = (ULONG)(ULONG_PTR)pArgBase->Buffer;
		RtlCopyMemory(
			(PVOID)(ULONG_PTR)pusDllPath->Buffer,
			pusPath->Buffer,
			usSizeWrite
		);

		*ppCodeBuffer = pCodeBase;
		*ppArgBuffer = pArgBase;
		status = STATUS_SUCCESS;
	} while (FALSE);

	return status;
}

BOOLEAN GetInjectApcThread(
	IN HANDLE ProcessId,
	OUT PETHREAD* ppThread
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PVOID pBuffer = NULL;
	SYSTEM_PROCESS_INFORMATION* pInfo = NULL;
	/*PSYSTEM_THREAD_INFORMATION*/SYSTEM_THREAD_INFORMATION* pThreadsAddr = NULL;
	ULONG ulSize = 0;
	ULONG uIndex = 0;
	BOOLEAN bFind = FALSE;
	BOOLEAN bRet = FALSE;

	do
	{
		if (!ProcessId || !ppThread)
		{
			break;
		}

		ZwQuerySystemInformation(
			SYSTEM_PROCESS_INFO_TYPE,
			NULL,
			0,
			&ulSize
		);
		if (!ulSize)
		{
			break;
		}

		pBuffer = MemAllocNonPagePoolWithTag(
			ulSize,
			&gs_ApcHookPoolTag
		);
		if (!pBuffer)
		{
			break;
		}

		status = ZwQuerySystemInformation(
			SYSTEM_PROCESS_INFO_TYPE,
			pBuffer,
			ulSize,
			&ulSize
		);
		if (STATUS_INFO_LENGTH_MISMATCH == status)
		{
			///> 缓冲区太小，大小翻倍
			ulSize = ulSize * SIZE_DOUBLE;
			MemFree(pBuffer);
			pBuffer = MemAllocNonPagePoolWithTag(
				ulSize,
				&gs_ApcHookPoolTag
			);
			if (!pBuffer)
			{
				break;
			}

			status = ZwQuerySystemInformation(
				SYSTEM_PROCESS_INFO_TYPE,
				pBuffer,
				ulSize,
				&ulSize
			);
		}
		if (!NT_SUCCESS(status))
		{
			break;
		}

		pInfo = (SYSTEM_PROCESS_INFORMATION*)pBuffer;
		while (pInfo)
		{
			if (pInfo->ProcessId == (ULONG_PTR)ProcessId)
			{
				bFind = TRUE;
				break;
			}

			if (!pInfo->NextEntryOffset)
			{
				break;
			}

			pInfo = (SYSTEM_PROCESS_INFORMATION*)(
				(PUCHAR)pInfo + pInfo->NextEntryOffset
				);
		}
		if (!bFind)
		{
			break;
		}
		if (!pInfo)
		{
			break;
		}

		///> 占位符转换真正的数组起始地址
		pThreadsAddr = (/*PSYSTEM_THREAD_INFORMATION*/SYSTEM_THREAD_INFORMATION*)(&pInfo->Threads);
		for (uIndex = 0; uIndex < pInfo->/*NumberOfThreads*/ThreadCount; uIndex++)
		{
			///> Skip current thread
			if (pThreadsAddr[uIndex].ClientId.UniqueThread == PsGetCurrentThread())
			{
				continue;
			}

			status = PsLookupThreadByThreadId(
				pThreadsAddr[uIndex].ClientId.UniqueThread,
				ppThread
			);
			if (NT_SUCCESS(status))
			{
				bRet = TRUE;
				break;
			}
		}
	} while (FALSE);

	if (pBuffer)
	{
		MemFree(pBuffer);
		pBuffer = NULL;
	}

	return bRet;
}

PVOID GetModuleExport(
	IN PVOID pBase,
	IN PCCHAR pFuncName
)
{
	PIMAGE_DOS_HEADER       pDosHdr = (PIMAGE_DOS_HEADER)pBase;
	PIMAGE_NT_HEADERS32     pNtHdr32 = NULL;
	PIMAGE_NT_HEADERS64     pNtHdr64 = NULL;
	PIMAGE_EXPORT_DIRECTORY pExport = NULL;
	ULONG                   expSize = 0;
	PVOID                   pAddress = 0;

	PIMAGE_DATA_DIRECTORY   pImgDirectory = NULL;
	DWORD                   dwVirtulAddress = 0;

	///> Protect from UserMode AV
	__try
	{
		do
		{
			PUSHORT pAddressOfOrds = 0;
			PULONG pAddressOfNames = 0;
			PULONG pAddressOfFuncs = 0;
			ULONG uIndex = 0;

			if (!pBase || !pFuncName)
			{
				break;
			}

			///> Not a PE file
			if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE)
			{
				break;
			}

			pNtHdr32 = (PIMAGE_NT_HEADERS32)((PUCHAR)pBase + pDosHdr->e_lfanew);
			pNtHdr64 = (PIMAGE_NT_HEADERS64)((PUCHAR)pBase + pDosHdr->e_lfanew);

			///> Not a PE file
			if (pNtHdr32->Signature != IMAGE_NT_SIGNATURE)
			{
				break;
			}

			///> 64 bit image
			if (pNtHdr32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			{
				pImgDirectory = pNtHdr64->OptionalHeader.DataDirectory;
				dwVirtulAddress = pImgDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
				pExport = (PIMAGE_EXPORT_DIRECTORY)(dwVirtulAddress + (ULONG_PTR)pBase);
				expSize = pImgDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
			}
			///> 32 bit image
			else
			{
				pImgDirectory = pNtHdr32->OptionalHeader.DataDirectory;
				dwVirtulAddress = pImgDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
				pExport = (PIMAGE_EXPORT_DIRECTORY)(dwVirtulAddress + (ULONG_PTR)pBase);
				expSize = pImgDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
			}

			pAddressOfOrds = (PUSHORT)(pExport->AddressOfNameOrdinals + (ULONG_PTR)pBase);
			pAddressOfNames = (PULONG)(pExport->AddressOfNames + (ULONG_PTR)pBase);
			pAddressOfFuncs = (PULONG)(pExport->AddressOfFunctions + (ULONG_PTR)pBase);

			for (uIndex = 0; uIndex < pExport->NumberOfFunctions; ++uIndex)
			{
				USHORT OrdIndex = 0xFFFF;
				PCHAR  pName = NULL;

				///> Find by index
				if ((ULONG_PTR)pFuncName <= 0xFFFF)
				{
					OrdIndex = (USHORT)uIndex;
				}
				///> Find by name
				else if ((ULONG_PTR)pFuncName > 0xFFFF &&
					uIndex < pExport->NumberOfNames)
				{
					pName = (PCHAR)(pAddressOfNames[uIndex] + (ULONG_PTR)pBase);
					OrdIndex = pAddressOfOrds[uIndex];
				}
				///> Weird params
				else
				{
					break;
				}

				if (((ULONG_PTR)pFuncName <= 0xFFFF &&
					(USHORT)((ULONG_PTR)pFuncName) == OrdIndex + pExport->Base) ||
					((ULONG_PTR)pFuncName > 0xFFFF && !strcmp(pName, pFuncName)))
				{
					pAddress = (PVOID)(pAddressOfFuncs[OrdIndex] + (ULONG_PTR)pBase);
					break;
				}
			}
		} while (FALSE);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

	return pAddress;
}

VOID ApcInject_LoadImageNotify(
	IN PUNICODE_STRING FullImageName,
	IN HANDLE ProcessId,
	IN PIMAGE_INFO ImageInfo
)
{
	if (!FullImageName ||
		FullImageName->Length == 0 ||
		!FullImageName->Buffer)
	{
		return;
	}

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG pid = 0;
	PEPROCESS pEProcess = NULL;
	BOOLEAN bIsWow64 = FALSE;
	WCHAR szDllPath[NTDLLNAME_SIZE] = { 0 };
	UNICODE_STRING usNtDll = { 0 };
	UNICODE_STRING usCurDll = { 0 };
	UNICODE_STRING usDllPath = { 0 };
	UNICODE_STRING NativeDllPath = { 0 };
	LPCWSTR lpNtdllName = L"\\System32\\ntdll.dll";
	LPCWSTR lpNtWow64Name = L"\\SysWOW64\\ntdll.dll";
	ULONG ulNtdllSize = 0;
	KAPC_STATE ApcState = { 0 };
	BOOLEAN bAttach = FALSE;
	PPEB pPeb = NULL;
	WCHAR szImagePath[MAX_PATH + 1] = { 0 };
	WCHAR szFileName[MAX_PATH + 1] = { 0 };
	WCHAR szInjectDllPath[MAX_PATH + 1] = { 0 };
	LPWSTR pLastBackslash = NULL;
	PVOID pLdrLoadDllFunc = NULL;
	PVOID pLdrGetProcAddressFunc = NULL;
	PETHREAD pEThread = NULL;
	PVOID pCodeBuff = NULL;
	PVOID pArgBuff = NULL;
	AppInjectInfo InjectInfo = { 0 };
	PKAPC pApc = NULL;
	BOOLEAN bWhiteProcess = FALSE;
	INT SessionId = 0;
	UNICODE_STRING ProcSidString = { 0 };
	ULONG ProcSessionId = 0;
	PROCESS* proc;

	do
	{
		// 获取进程 pid
		pid = HandleToULong(ProcessId);

		// 不处理系统进程
		if (pid <= MIN_USER_PROCESSID)
		{
			break;
		}

		// 获取进程 EPROCESS 
		status = PsLookupProcessByProcessId(ProcessId, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		// 判读是否为 WOW64 进程，决定选择哪个路径下的 ntdll
#ifdef _WIN64
		if (NULL != PsGetProcessWow64Process(pEProcess))
		{
			bIsWow64 = TRUE;
			RtlInitUnicodeString(&usNtDll, lpNtWow64Name);
		}
		else
		{
			RtlInitUnicodeString(&usNtDll, lpNtdllName);
		}
#else
		{
			RtlInitUnicodeString(&usNtDll, lpNtdllName);
			UNREFERENCED_PARAMETER(lpNtWow64Name);
		}
#endif

		// 判断加载的模块是否为 ntdll
		ulNtdllSize = (ULONG)wcslen(lpNtdllName) * sizeof(WCHAR);
		if (FullImageName->Length < ulNtdllSize)
		{
			break;
		}
		RtlCopyMemory(
			szDllPath,
			(PVOID)((ULONG_PTR)(FullImageName->Buffer) + FullImageName->Length - ulNtdllSize),
			ulNtdllSize
		);
		RtlInitUnicodeString(&usCurDll, szDllPath);
		if (!RtlEqualUnicodeString(&usNtDll, &usCurDll, TRUE))
		{
			break;
		}

		// 附加到进程
		KeStackAttachProcess(pEProcess, &ApcState);
		bAttach = TRUE;

		// 获取进程 peb
		pPeb = (PPEB)PsGetProcessPeb(pEProcess);
		if (!pPeb || !pPeb->ProcessParameters)
		{
			break;
		}
		if (!pPeb->ProcessParameters->ImagePathName.Buffer ||
			pPeb->ProcessParameters->ImagePathName.Length <= sizeof(WCHAR))
		{
			break;
		}

		// 保存当前进程名，确保 \ 不是最后一个字符
		wcsncpy_s(
			szImagePath,
			MAX_PATH,
			pPeb->ProcessParameters->ImagePathName.Buffer,
			pPeb->ProcessParameters->ImagePathName.Length / sizeof(WCHAR)
		);
		pLastBackslash = wcsrchr(szImagePath, L'\\');
		if (!pLastBackslash || wcslen(pLastBackslash) <= 1)
		{
			break;
		}
		wcscpy_s(szFileName, MAX_PATH, pLastBackslash + 1);

		// 判断进程是否属于沙箱
		proc = Process_Find(NULL, NULL);
		if (proc == PROCESS_TERMINATED)
		{
			break;
		}

		// 沙箱内进程统一注入,沙箱外进程按规则注入
		if (proc == NULL)
		{
			// 不注入 SessionId = 0 的进程
			status = Process_GetSidStringAndSessionId(NtCurrentProcess(), NULL, &ProcSidString, &ProcSessionId);
			if (!NT_SUCCESS(status))
			{
				break;
			}
			if (ProcSessionId == 0)
			{
				break;
			}

			// 不注入系统白名单进程
			for (INT i = 0; i < sizeof(WhiteApcProcessSystem) / sizeof(WhiteApcProcessSystem[0]); ++i)
			{
				if (wstristr(szImagePath, WhiteApcProcessSystem[i]) != 0)
				{
					bWhiteProcess = TRUE;
					break;
				}
			}
			if (bWhiteProcess)
			{
				break;
			}

			// 不注入自身白名单进程
			for (INT i = 0; i < sizeof(WhiteApcProcessSelf) / sizeof(WhiteApcProcessSelf[0]); ++i)
			{
				if (wstristr(szImagePath, WhiteApcProcessSelf[i]) != 0)
				{
					bWhiteProcess = TRUE;
					break;
				}
			}
			if (bWhiteProcess)
			{
				break;
			}

			DbgPrintEx(
				DPFLTR_IHVDRIVER_ID,
				DPFLTR_ERROR_LEVEL,
				"[LYSM:k] outside process : %S inject! \n",
				szImagePath
			);
		}
		else
		{
			// 不注入自身白名单进程
			for (INT i = 0; i < sizeof(WhiteApcProcessSelf) / sizeof(WhiteApcProcessSelf[0]); ++i)
			{
				if (wstristr(szImagePath, WhiteApcProcessSelf[i]) != 0)
				{
					bWhiteProcess = TRUE;
					break;
				}
			}
			if (bWhiteProcess)
			{
				break;
			}

			DbgPrintEx(
				DPFLTR_IHVDRIVER_ID,
				DPFLTR_ERROR_LEVEL,
				"[LYSM:k] inside process : %S inject! \n",
				szImagePath
			);
		}

		// 获取 LdrLoadDll、LdrGetProcedureAddress 函数地址
		pLdrLoadDllFunc = GetModuleExport(ImageInfo->ImageBase, "LdrLoadDll");
		pLdrGetProcAddressFunc = GetModuleExport(
			ImageInfo->ImageBase,
			"LdrGetProcedureAddress"
		);
		if (!pLdrLoadDllFunc || !pLdrGetProcAddressFunc)
		{
			break;
		}

		// 获取用于注入 dll 的 apc 线程
		if (!GetInjectApcThread(ProcessId, &pEThread))
		{
			break;
		}

		// 获取注入 dll 的路径
#ifdef _WIN64
		if (bIsWow64)
		{
			wcscpy_s(szInjectDllPath, MAX_PATH, Driver_HomePathNt);
			wcscat_s(szInjectDllPath, MAX_PATH, INJECT_DLL_CYGUARD_WOW64);
		}
		else
		{
			wcscpy_s(szInjectDllPath, MAX_PATH, Driver_HomePathNt);
			wcscat_s(szInjectDllPath, MAX_PATH, INJECT_DLL_CYGUARD);
		}
#else
		wcscpy_s(szInjectDllPath, MAX_PATH, Driver_HomePathNt);
		wcscat_s(szInjectDllPath, MAX_PATH, INJECT_DLL_CYGUARD);
#endif
		RtlInitUnicodeString(&usDllPath, szInjectDllPath);
		status = NtFileNameToDosFileName(&usDllPath, &NativeDllPath);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		// 获取 apc 注入的 shellcoode
		if (bIsWow64)
		{
			status = GetWow64InjectCode(
				pLdrLoadDllFunc,
				&NativeDllPath,
				pLdrGetProcAddressFunc,
				&InjectInfo,
				NULL,
				0,
				&pCodeBuff,
				&pArgBuff
			);
		}
		else
		{
			status = GetNativeInjectCode(
				pLdrLoadDllFunc,
				&NativeDllPath,
				pLdrGetProcAddressFunc,
				&InjectInfo,
				NULL,
				0,
				&pCodeBuff,
				&pArgBuff
			);
		}
		if (!NT_SUCCESS(status) || !pCodeBuff || !pArgBuff)
		{
			break;
		}

		// 添加 apc 并执行
		pApc = (PKAPC)MemAllocNonPagePoolWithTag(
			sizeof(KAPC),
			&gs_ApcHookPoolTag
		);
		if (!pApc)
		{
			break;
		}
		KeInitializeApc(pApc,
			(PRKTHREAD)pEThread,
			OriginalApcEnvironment,
			&KernelRoutineCallback,
			NULL,
			(PKNORMAL_ROUTINE)pCodeBuff,
			UserMode,
			pArgBuff
		);
		if (KeInsertQueueApc(pApc,
			NULL,
			NULL,
			IO_NO_INCREMENT)
			)
		{
			status = STATUS_SUCCESS;
		}
	} while (0);

	if (NULL != pEProcess)
	{
		ObDereferenceObject(pEProcess);
		pEProcess = NULL;
	}
	if (NULL != pEThread)
	{
		ObDereferenceObject(pEThread);
		pEThread = NULL;
	}
	if (bAttach)
	{
		KeUnstackDetachProcess(&ApcState);
	}

	return;
}
