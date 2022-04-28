#pragma once




typedef struct _LDR_DATA_TABLE_ENTRY64
{
	LIST_ENTRY64    InLoadOrderLinks;
	LIST_ENTRY64    InMemoryOrderLinks;
	LIST_ENTRY64    InInitializationOrderLinks;
	PVOID            DllBase;
	PVOID            EntryPoint;
	ULONG            SizeOfImage;
	UNICODE_STRING    FullDllName;
	UNICODE_STRING     BaseDllName;
	ULONG            Flags;
	USHORT            LoadCount;
	USHORT            TlsIndex;
	PVOID            SectionPointer;
	ULONG            CheckSum;
	PVOID            LoadedImports;
	PVOID            EntryPointActivationContext;
	PVOID            PatchInformation;
	LIST_ENTRY64    ForwarderLinks;
	LIST_ENTRY64    ServiceTagLinks;
	LIST_ENTRY64    StaticLinks;
	PVOID            ContextInformation;
	ULONG64            OriginalBase;
	LARGE_INTEGER    LoadTime;
} LDR_DATA_TABLE_ENTRY64, * PLDR_DATA_TABLE_ENTRY64;

NTSTATUS processProtectLoad(IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryString);

VOID processProtectUnload(IN PDRIVER_OBJECT pDriverObj);

NTSTATUS SetThreadCallback();

char* GetProcessNameByPid(ULONG ulProcessID);

NTSTATUS setProcessCallback();

OB_PREOP_CALLBACK_STATUS ThreadPreCall(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION pObPreOperationInfo);

OB_PREOP_CALLBACK_STATUS processPreCall(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION pOperationInformation);
