﻿

#include "stdafx.h"

#include <windows.h>
#include <winsvc.h>
#include <conio.h>
#include <stdio.h>


#define DRIVER_NAME L"veracrypt"

#define DRIVER_PATH L"D:\\Program Files\\VeraCrypt\\VeraCrypt\\veracrypt.sys"

//装载NT驱动程序
BOOL LoadNTDriver(WCHAR* lpszDriverName, WCHAR* lpszDriverPath, int servicetype, int boottype,WCHAR * groupname)
{
	WCHAR szshow[256];
	WCHAR szDriverImagePath[256];
	//得到完整的驱动路径
	GetFullPathNameW(lpszDriverPath, 256, szDriverImagePath, NULL);

	BOOL bRet = FALSE;

	SC_HANDLE hServiceMgr = NULL;//SCM管理器的句柄
	SC_HANDLE hServiceDDK = NULL;//NT驱动程序的服务句柄

	//打开服务控制管理器
	hServiceMgr = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hServiceMgr == NULL)
	{
		//OpenSCManager失败
		printf("OpenSCManager() Faild %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		////OpenSCManager成功
		printf("OpenSCManager() ok ! \n");
	}

	//创建驱动所对应的服务
	hServiceDDK = CreateServiceW(hServiceMgr,
		lpszDriverName, //驱动程序的在注册表中的名字
		lpszDriverName, // 注册表驱动程序的 DisplayName 值
		SERVICE_ALL_ACCESS, // 加载驱动程序的访问权限
		servicetype,
		//SERVICE_KERNEL_DRIVER,// 表示加载的服务是驱动程序
		boottype,
		//SERVICE_AUTO_START, // 注册表驱动程序的 Start 值
		SERVICE_ERROR_NORMAL, // 注册表驱动程序的 ErrorControl 值
		szDriverImagePath, // 注册表驱动程序的 ImagePath 值
		groupname,
		NULL,
		NULL,
		NULL,
		NULL);

	DWORD dwRtn;
	//判断服务是否失败
	if (hServiceDDK == NULL)
	{
		dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_EXISTS)
		{
			//由于其他原因创建服务失败
			printf("CreateServiceW() Faild %d ! \n", dwRtn);

			wsprintfW(szshow, L"CreateServiceW() Faild %d!driver name:%ws,path:%ws,service type:%d,boot:%d\n",
				dwRtn,lpszDriverName,szDriverImagePath,servicetype,boottype);
			MessageBoxW(0, szshow, szshow, MB_OK);

			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			//服务创建失败，是由于服务已经创立过
			printf("CreateServiceW() Faild Service is ERROR_IO_PENDING or ERROR_SERVICE_EXISTS! \n");
		}

		// 驱动程序已经加载，只需要打开
		hServiceDDK = OpenServiceW(hServiceMgr, lpszDriverName, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL)
		{
			//如果打开服务也失败，则意味错误
			dwRtn = GetLastError();
			printf("OpenService() Faild %d ! \n", dwRtn);

			wsprintfW(szshow, L"OpenService() Faild %d ! \n", dwRtn);
			MessageBoxW(0, szshow, szshow, MB_OK);

			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			printf("OpenService() ok ! \n");
		}
	}
	else
	{
		printf("CreateServiceW() ok ! \n");
	}

	//开启此项服务
	bRet = StartServiceW(hServiceDDK, 0, 0);
	if (!bRet)
	{
		DWORD dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_ALREADY_RUNNING)
		{
			wsprintfW(szshow,L"StartService() Faild %d ! \n", dwRtn);
			MessageBoxW(0, szshow, szshow, MB_OK);

			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			if (dwRtn == ERROR_IO_PENDING)
			{
				//设备被挂住
				printf("StartService() Faild ERROR_IO_PENDING ! \n");
				bRet = FALSE;
				goto BeforeLeave;
			}
			else
			{
				//服务已经开启
				printf("StartService() Faild ERROR_SERVICE_ALREADY_RUNNING ! \n");
				bRet = TRUE;
				goto BeforeLeave;
			}
		}
	}
	bRet = TRUE;
	//离开前关闭句柄
BeforeLeave:
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}

//卸载驱动程序
BOOL UnloadNTDriver(WCHAR * szSvrName)
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;//SCM管理器的句柄
	SC_HANDLE hServiceDDK = NULL;//NT驱动程序的服务句柄
	SERVICE_STATUS SvrSta;
	//打开SCM管理器
	hServiceMgr = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		//带开SCM管理器失败
		printf("OpenSCManager() Faild %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		//带开SCM管理器失败成功
		printf("OpenSCManager() ok ! \n");
	}
	//打开驱动所对应的服务
	hServiceDDK = OpenServiceW(hServiceMgr, szSvrName, SERVICE_ALL_ACCESS);

	if (hServiceDDK == NULL)
	{
		//打开驱动所对应的服务失败
		printf("OpenService() Faild %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		printf("OpenService() ok ! \n");
	}
	//停止驱动程序，如果停止失败，只有重新启动才能，再动态加载。
	if (!ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta))
	{
		printf("ControlService() Faild %d !\n", GetLastError());
	}
	else
	{
		//打开驱动所对应的失败
		printf("ControlService() ok !\n");
	}
	//动态卸载驱动程序。
	if (!DeleteService(hServiceDDK))
	{
		//卸载失败
		printf("DeleteSrevice() Faild %d !\n", GetLastError());
	}
	else
	{
		//卸载成功
		printf("DelServer:eleteSrevice() ok !\n");
	}
	bRet = TRUE;
BeforeLeave:
	//离开前关闭打开的句柄
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}

void TestDriver()
{
	//测试驱动程序
	HANDLE hDevice = CreateFileA("\\\\.\\HelloDDK",
		GENERIC_WRITE | GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		printf("Create Device ok ! \n");
	}
	else
	{
		printf("Create Device faild %d ! \n", GetLastError());
	}
	CloseHandle(hDevice);
}

int testmain(int argc, char* argv[])
{
	//加载驱动
	BOOL bRet = LoadNTDriver(DRIVER_NAME, DRIVER_PATH, SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START,L"");
	if (!bRet)
	{
		printf("LoadNTDriver error\n");
		return 0;
	}
	//加载成功

	printf("press any to create device!\n");
	_getch();

	TestDriver();

	//这时候你可以通过注册表，或其他查看符号连接的软件验证。
	printf("press any to unload the driver!\n");
	_getch();

	//卸载驱动
	UnloadNTDriver(DRIVER_NAME);
	if (!bRet)
	{
		printf("UnloadNTDriver error\n");
		return 0;
	}

	return 0;
}
