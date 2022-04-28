#ifndef _INJECTAPCDLL_H
#define _INJECTAPCDLL_H
#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include "injectinfo.h"

#define INJECT_DLL_CYGUARD_WOW64	L"\\32\\cyguard.dll"
#define INJECT_DLL_CYGUARD			L"\\cyguard.dll"

typedef enum _KAPC_ENVIRONMENT
{
	OriginalApcEnvironment,
	AttachedApcEnvironment,
	CurrentApcEnvironment,
	InsertApcEnvironment
}KAPC_ENVIRONMENT, * PKAPC_ENVIRONMENT;

typedef VOID(NTAPI* PKNORMAL_ROUTINE)(
	PVOID NormalContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2
	);

typedef VOID KKERNEL_ROUTINE(
	PRKAPC Apc,
	PKNORMAL_ROUTINE* NormalRoutine,
	PVOID* NormalContext,
	PVOID* SystemArgument1,
	PVOID* SystemArgument2
);

typedef KKERNEL_ROUTINE(NTAPI* PKKERNEL_ROUTINE);

typedef VOID(NTAPI* PKRUNDOWN_ROUTINE)(
	PRKAPC Apc
	);

VOID KeInitializeApc(
	PRKAPC Apc,
	PRKTHREAD Thread,
	KAPC_ENVIRONMENT Environment,
	PKKERNEL_ROUTINE KernelRoutine,
	PKRUNDOWN_ROUTINE RundownRoutine,
	PKNORMAL_ROUTINE NormalRoutine,
	KPROCESSOR_MODE ProcessorMode,
	PVOID NormalContext
);

BOOLEAN KeInsertQueueApc(
	PRKAPC Apc,
	PVOID SystemArgument1,
	PVOID SystemArgument2,
	KPRIORITY Increment
);

BOOLEAN NTAPI KeTestAlertThread(
	IN KPROCESSOR_MODE AlertMode
);

#ifdef _WIN64
PVOID NTAPI PsGetCurrentProcessWow64Process();
PVOID PsGetProcessWow64Process(PEPROCESS Process);
#endif

//NTSTATUS NTAPI ZwQuerySystemInformation(
//	IN ULONG SystemInformationClass,
//	OUT PVOID SystemInformation,
//	IN ULONG SystemInformationLength,
//	OUT PULONG ReturnLength
//);

typedef NTSTATUS(NTAPI* PLDR_LOAD_DLL)(PWSTR, PULONG, PUNICODE_STRING, PVOID*);

///> LdrGetProcedureAddress 函数指针类型
typedef NTSTATUS(NTAPI* PLdrGetProcAddress)(
	PVOID ModuleImageBase,
	PANSI_STRING ProcedureName,
	ULONG ProcedureOrdinal,
	PVOID* ProcedureAddress
	);

///> Hook Dll 导出函数指针类型
typedef BOOLEAN(NTAPI* PStartMonitor)(const PAppInjectInfo pInjectInfo);

typedef struct _KINJECT
{
	PLDR_LOAD_DLL LdrLoadDll;
	UNICODE_STRING DllPath;
	WCHAR Buffer[MAX_PATH];

	///> 拓展部分
	PLdrGetProcAddress LdrGetProcAddress;
	AppInjectInfo InjectInfo; ///> StartMonitor 接口参数结构
}KINJECT, * PKINJECT;

typedef struct _KINJECT32
{
	ULONG LdrLoadDll;
	UNICODE_STRING32 DllPath;
	WCHAR Buffer[MAX_PATH];

	ULONG LdrGetProcAddress;
	AppInjectInfo InjectInfo;
}KINJECT32, * PKINJECT32;

VOID ApcInject_LoadImageNotify(
	IN PUNICODE_STRING FullImageName,
	IN HANDLE ProcessId,
	IN PIMAGE_INFO ImageInfo
);

#endif
