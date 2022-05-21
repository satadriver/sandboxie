
#pragma warning(disable:4005)//must be in the first line

#include <windows.h>
#include <string>
#include <io.h>
#include <winsock.h>
#include <stdio.h>
#include <process.h>

#include "../../common/my_version.h"
#include "main.h"
#include "../publicFun/Utils.h"
#include "../publicFun/config.h"
#include "../publicFun/box.h"
#include "../publicFun/vera.h"
#include "../publicFun/configStrategy.h"
#include "apiFuns.h"
#include "configYaml.h"
#include "md5.h"

#pragma comment(lib,"ws2_32.lib")

#pragma comment(lib,"publicfun.lib")



#pragma execution_character_set("utf-8") 

void test() {

#define STRING(x) #x#x
#define CLASS_NAME(myname) myclass##myname

	int test = 123456;

	char* data = (char*)STRING(test);

	char* data1 = (char*)STRING("test");

	char* myclasstest = (char*)"classtest_haha";
	char* data2 = CLASS_NAME(test);
}

#include "../publicFun/apiFuns.h"


#include "main.h"


int __stdcall WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

	int result = 0;
#ifdef _DEBUG
	//setJsonConfig(0, 0, 0);
#endif

	//ULONGLONG l1, l2, l3;
	//getVeraDiskInfo(&l1, &l2, &l3);

	//WCHAR outpath[1024];
	//getboxedFilePath(L"C:\\Users\\Admin\\go\\123.txt", L"Ljg_test", outpath);
	//MessageBoxW(0, outpath, outpath, MB_OK);

	OutputDebugStringW(L"desktop.exe starting!");

	int argc = 0;
	WCHAR ** argv = CommandLineToArgvW(GetCommandLineW(),&argc);
	if (argc < 3)
	{
#ifndef _DEBUG
		return FALSE;
#endif
	}

	result = EnableDebugPrivilege();
	if (result == FALSE)
	{
		mylog(L"get admin privilege error\r\n");
		return FALSE;
	}

#ifndef _DEBUG
	result = startSbieSvc();

	bool isVm = false;
	if (IsVboxVM() == true) {
		isVm = true;
		//printf("Running in vbox!");
	}
	else if (IsVMwareVM() == true) {
		isVm = true;
		//printf("Running in vmware!");
	}
	else {
		//printf("Not running in a VM!");
	}

	if (isVm) {
		//SelfDelete();
		//result = MessageBoxW(0,L"检测到虚拟机环境,点击\"取消\"退出程序,点击\"确定\"继续", L"Detect runing in VM",MB_OKCANCEL);
		result = MessageBoxW(0,L"检测到虚拟机环境,点击\"确定\"继续运行,点击\"取消\"停止运行",L"VirtualMachie detected",
			//"检测到虚拟机环境", 
			MB_OKCANCEL);
		if (result == IDCANCEL)
		{
			exit(-1);
		}
		else if (result == IDOK) {
			
		}
	}
#endif

	if (argc >= 5)
	{
		configBox(argv[1], argv[2], argv[3], argv[4]);
	}
	else {
		configBox(argv[1], argv[2],0,0);
	}
	
	//result = parseConfig(argv[1],argv[2]);

	ExitProcess(0);

	return result;
}

//ctrl + /
//ctrl + u

// virtualkd 无法正确运行需要运行:bcdedit /set dbgtransport kdvm.dll
//vmware 无法安装vmware tool，需要安装更新:2019-09 Security Update for Windows 7 for x64-based Systems (KB4474419)


//Inf2Cat.exe /driver:"D:\work\Sandboxie2\Sandboxie\Bin\x64\sign\forsign" /os:7_X64,10_X64

/*
to setup and start driver without digital signature on 64 bit windows,run below 4 commands in cmd.exe with administrator priority:
bcdedit /set testsigning on

bcdedit /bootdebug ON
bcdedit /debug ON
bcdedit.exe -set loadoptions DDISABLE_INTEGRITY_CHECKS
bcdedit /dbgsettings serial debugport:1 baudrate:115200
bcdedit /set nointegritychecks on


bcdedit -set loadoptions ENABLE_INTEGRITY_CHECKS
bcdedit /set testsigning off
bcdedit /bootdebug off
bcdedit /debug off

bcdedit /set nointegritychecks on

bcdedit /dbgsettings net hostip:192.168.101.115 port:50000 key:1.2.3.4
*/

/*
管理员权限下执行：
sc delete sfdeskdrv
sc delete sfdesksvc
sc delete inspect
sc delete veracrypt
重启，并删除安装目录所有文件
*/

//UAC打开
//登录的是超级管理员Administrator
//提权的时候不会弹出UAC提示窗口。

//登录的是管理员（非超级管理员Administrator）
// 在启动设置了requireAdministrator属性的程序时，会弹出UAC提示框

//登录的是标准用户（非管理员）
//在启动设置了requireAdministrator属性的程序时，会弹出输入管理员密码的UAC提示框
// 输入管理员密码后，才能正常的启动程序。

//UAC关闭
//登录的是超级管理员Administrator或者管理员（普通管理员）
// 在启动设置了requireAdministrator属性的程序，会申请到管理员权限，且不会弹出UAC提示框。
//对于没有设置requireAdministrator属性的程序，发现也会以管理员权限运行。

//登录的是标准用户
//程序是申请不到管理员权限的，程序是可以启动的，但是所有执行需要管理员权限的操作都会返回失败。

//f the service type is either SERVICE_WIN32_OWN_PROCESS or SERVICE_WIN32_SHARE_PROCESS,
//and the service is running in the context of the LocalSystem account,
//the following type may also be specified:SERVICE_INTERACTIVE_PROCESS


