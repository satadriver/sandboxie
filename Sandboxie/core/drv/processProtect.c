
#pragma warning (disable:4819)
#ifndef CXX_PROTECTPROCESSX64_H
#define CXX_PROTECTPROCESSX64_H

#include <ntifs.h>

#define PROCESS_TERMINATE         0x0001  
#define PROCESS_VM_OPERATION      0x0008  
#define PROCESS_VM_READ           0x0010  
#define PROCESS_VM_WRITE          0x0020

#include "processProtect.h"
//Inf2Cat.exe /driver:D:\work\sandboxie\processProtect\x64\Release /os:Vista_X64,7_X64,10_X64
//Inf2Cat.exe /driver:D:\work\sandboxie\processProtect\x64\Release /os:2000,XP_X86,XP_X64,Server2003_X86,Server2003_X64,Vista_X86,Vista_X64,7_X86,7_X64,10_X86,10_X64

char* g_protectProcessNames[] = { "DibTest.exe","calc.exe","Calculator.exe","CalculatorApp.exe","sfdesksvc.exe"};

extern UCHAR* PsGetProcessImageFileName(__in PEPROCESS Process);

HANDLE g_obProcessHandle;

HANDLE g_obThreadHandle;

ULONG g_oldFlag = 0;


#define PROCESS_TERMINATE_0       0x1001
//taskkill指令结束代码
#define PROCESS_TERMINATE_1       0x0001 
//taskkill指令加/f参数强杀进程结束码
#define PROCESS_KILL_F			  0x1401


NTSTATUS processProtectLoad(IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryString)
{
	DbgPrint("DriverEntry start,driverobject:%x,path:%wZ\r\n", pDriverObj,pRegistryString);

	NTSTATUS status = STATUS_SUCCESS;

	PLDR_DATA_TABLE_ENTRY64 ldr;

	//pDriverObj->DriverUnload = processProtectUnload;
	// 
	// 绕过MmVerifyCallbackFunction。

	// 驱动程序必须有数字签名才能使用ObRegisterCallbacks函数。国外的黑客对此限制很不爽，他们通过逆向 ObRegisterCallbacks，找到了破解这个限制的方法。
	//经研究，内核通过 MmVerifyCallbackFunction 验证此回调是否合法，
	//但此函数只是简单的验证了一下 DriverObject->DriverSection->Flags 的值是不是为 0x20

	if (g_oldFlag == 0)
	{
		ldr = (PLDR_DATA_TABLE_ENTRY64)pDriverObj->DriverSection;
		g_oldFlag = ldr->Flags;

		ldr->Flags |= 0x20;

		//if PLDR_DATA_TABLE_ENTRY64.flag not set as this,this call will return error
		setProcessCallback();

		SetThreadCallback();
	}

	DbgPrint("DriverEntry complete\r\n");

	return status;
}



NTSTATUS setProcessCallback()
{
	//__debugbreak();

	OB_OPERATION_REGISTRATION opReg;
	memset(&opReg, 0, sizeof(opReg));
	//下面请注意这个结构体的成员字段的设置
	opReg.ObjectType = PsProcessType;
	opReg.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	opReg.PreOperation = (POB_PRE_OPERATION_CALLBACK)&processPreCall;

	OB_CALLBACK_REGISTRATION obReg;
	memset(&obReg, 0, sizeof(obReg));
	obReg.Version = ObGetFilterVersion();
	obReg.OperationRegistrationCount = 1;
	obReg.RegistrationContext = NULL;
	RtlInitUnicodeString(&obReg.Altitude, L"321000");
	obReg.OperationRegistration = &opReg;

	return ObRegisterCallbacks(&obReg, &g_obProcessHandle);
}



OB_PREOP_CALLBACK_STATUS processPreCall(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION pOperationInformation)
{
	if (*PsProcessType != pOperationInformation->ObjectType)
	{
		DbgPrint("PsProcessType error\r\n");
		return OB_PREOP_SUCCESS;
	}

	UNREFERENCED_PARAMETER(RegistrationContext);

	PEPROCESS curpe = IoGetCurrentProcess();
	HANDLE curpid = PsGetProcessId(curpe);
	char* curname = GetProcessNameByPid((ULONG)(ULONGLONG)curpid);

	KPROCESSOR_MODE mode = ExGetPreviousMode();
	if (mode == KernelMode) {
		//DbgPrint("kernel mode process:%s\r\n", curname);
		//return OB_PREOP_SUCCESS;
	}


// 	if (_stricmp(curname, "taskmgr.exe") == 0 || _stricmp(curname, "cmd.exe") == 0 ||
// 		_stricmp(curname, "powershell.exe") == 0 || 
// 		_stricmp(curname, "explorer.exe") == 0 || _stricmp(curname, "taskkill.exe") == 0 || 
// 		_stricmp(curname, "system") == 0 || _stricmp(curname, "RuntimeBroker.exe") == 0 ||
// 		/*_stricmp(curname, "svchost.exe") == 0 ||*/ _stricmp(curname, "MSMPENG.EXE") == 0 || _stricmp(curname, "ApplicationFra") == 0 )
	if (_stricmp(curname, "svchost.exe") != 0)
	{
		HANDLE ojbpid = PsGetProcessId((PEPROCESS)pOperationInformation->Object);
		char* objname = GetProcessNameByPid((ULONG)(ULONGLONG)ojbpid);

// 		UNICODE_STRING unistr;
// 		ANSI_STRING str = { 0 };
// 		char strbuf[1024];
// 		str.Buffer = strbuf;
// 		str.Length = sizeof(strbuf);
// 		str.MaximumLength = sizeof(strbuf);
		for (int i = 0; i < sizeof(g_protectProcessNames) / sizeof(WCHAR*); i++)
		{
			if (g_protectProcessNames[i])
			{
// 				RtlInitUnicodeString(&unistr, g_protectProcessNames[i]);		//do not need to free
// 				RtlUnicodeStringToAnsiString(&str, &unistr, TRUE);
// 				str.Buffer[str.Length] = 0;
//				if (_stricmp(objname, str.Buffer) == 0 /*&& _stricmp(curname, "svchost.exe") != 0*/ /*&& _stricmp(curname, str.Buffer) != 0*/)

				if (_stricmp(objname, g_protectProcessNames[i]) == 0)
				{
// 					DbgPrint("current process:%s,object name:%s,OriginalDesiredAccess:%x\r\n", 
// 						curname, objname, pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess);

					if (pOperationInformation->Operation == OB_OPERATION_HANDLE_CREATE)
					{
						//__debugbreak();
						ULONG access = pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess;
						if(access & PROCESS_TERMINATE)
						//if ( access == PROCESS_TERMINATE_0 || access == PROCESS_TERMINATE_1 || access == PROCESS_KILL_F)
						{
							//pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
							pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
						}else if (pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_VM_WRITE)
						{
							//pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
							pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_WRITE;
						}
					}
					else if (pOperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE)
					{
						//__debugbreak();
						if (pOperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess & PROCESS_TERMINATE)
						{
							//pOperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess = 0;
							pOperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
						}
						else if (pOperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess & PROCESS_VM_WRITE)
						{
							//pOperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess = 0;
							pOperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess &= ~PROCESS_VM_WRITE;
						}
					}
					else {
						DbgPrint("unrecognize command:%d\r\n", pOperationInformation->Operation);
					}
				}
				//RtlFreeAnsiString(&str);
			}
		}
	}
	
	return OB_PREOP_SUCCESS;
}



char* GetProcessNameByPid(ULONG ulProcessID)
{
	NTSTATUS  Status;
	PEPROCESS  EProcess = NULL;
	Status = PsLookupProcessByProcessId((HANDLE)ulProcessID, &EProcess);
	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}
	ObDereferenceObject(EProcess);
	return (char*)PsGetProcessImageFileName(EProcess);

}



// 线程回调函数
OB_PREOP_CALLBACK_STATUS ThreadPreCall(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION pObPreOperationInfo)
{
	PEPROCESS pEProcess = NULL;

	// 判断对象类型
	if (*PsThreadType != pObPreOperationInfo->ObjectType)
	{
		return OB_PREOP_SUCCESS;
	}
	// 获取线程对应的进程 PEPROCESS
	pEProcess = IoThreadToProcess((PETHREAD)pObPreOperationInfo->Object);

	char* pProcName = (char*)PsGetProcessImageFileName(pEProcess); // 获取要保护的进程名

	for (int i = 0; i < sizeof(g_protectProcessNames) / sizeof(WCHAR*); i++)
	{
		if (g_protectProcessNames[i])
		{
			// 判断是否是要保护的进程, 若是, 则拒绝结束线程
			if (_stricmp(pProcName, g_protectProcessNames[i]) == 0)
			{
				// 判断操作类型是创建句柄还是复制句柄
				if (pObPreOperationInfo->Operation == OB_OPERATION_HANDLE_CREATE)
				{
					//是否具有关闭线程的权限，有的话删掉它
					if (pObPreOperationInfo->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_TERMINATE)
					{
						pObPreOperationInfo->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
					}
				}
				else if (pObPreOperationInfo->Operation == OB_OPERATION_HANDLE_DUPLICATE)
				{
					//是否具有关闭线程的权限，有的话删掉它
					if (pObPreOperationInfo->Parameters->DuplicateHandleInformation.DesiredAccess & PROCESS_TERMINATE)
					{
						pObPreOperationInfo->Parameters->DuplicateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
					}
				}
			}
		}
	}

	return OB_PREOP_SUCCESS;
}



NTSTATUS SetThreadCallback()
{
	NTSTATUS status = STATUS_SUCCESS;
	OB_CALLBACK_REGISTRATION obCallbackReg = { 0 };
	OB_OPERATION_REGISTRATION obOperationReg = { 0 };

	RtlZeroMemory(&obCallbackReg, sizeof(OB_CALLBACK_REGISTRATION));
	RtlZeroMemory(&obOperationReg, sizeof(OB_OPERATION_REGISTRATION));

	// 设置 OB_OPERATION_REGISTRATION
	obOperationReg.ObjectType = PsThreadType;
	obOperationReg.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	obOperationReg.PreOperation = (POB_PRE_OPERATION_CALLBACK)(&ThreadPreCall);

	// 设置 OB_CALLBACK_REGISTRATION
	obCallbackReg.Version = ObGetFilterVersion();
	obCallbackReg.OperationRegistrationCount = 1;
	obCallbackReg.RegistrationContext = NULL;
	RtlInitUnicodeString(&obCallbackReg.Altitude, L"1900");
	obCallbackReg.OperationRegistration = &obOperationReg;

	// 注册回调函数
	status = ObRegisterCallbacks(&obCallbackReg, &g_obThreadHandle);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("ObRegisterCallbacks Error[0x%X]\n", status);
		return status;
	}

	return status;
}



VOID processProtectUnload(IN PDRIVER_OBJECT driverObject)
{
	UNREFERENCED_PARAMETER(driverObject);
	DbgPrint("driver unloading...\n");

	if (g_oldFlag)
	{
		PLDR_DATA_TABLE_ENTRY64 ldr = (PLDR_DATA_TABLE_ENTRY64)driverObject->DriverSection;
		ldr->Flags = g_oldFlag;
		g_oldFlag = 0;
	}

	// 删除进程回调
	if (g_obProcessHandle)
	{
		ObUnRegisterCallbacks(g_obProcessHandle);
		g_obProcessHandle = NULL;
	}

	// 卸载线程回调
	if (NULL != g_obThreadHandle)
	{
		ObUnRegisterCallbacks(g_obThreadHandle);
		g_obThreadHandle = NULL;
	}

	DbgPrint("driver unload complete\r\n");
}






#endif   
