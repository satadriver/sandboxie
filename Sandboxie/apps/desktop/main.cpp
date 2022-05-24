
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
		//result = MessageBoxW(0,L"��⵽���������,���\"ȡ��\"�˳�����,���\"ȷ��\"����", L"Detect runing in VM",MB_OKCANCEL);
		result = MessageBoxW(0,L"��⵽���������,���\"ȷ��\"��������,���\"ȡ��\"ֹͣ����",L"VirtualMachie detected",
			//"��⵽���������", 
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

// virtualkd �޷���ȷ������Ҫ����:bcdedit /set dbgtransport kdvm.dll
//vmware �޷���װvmware tool����Ҫ��װ����:2019-09 Security Update for Windows 7 for x64-based Systems (KB4474419)


//Inf2Cat.exe /driver:"D:\work\Sandboxie2\Sandboxie\Bin\x64\sign\forsign" /os:7_X64,10_X64

/*
to setup and start driver without digital signature on 64 bit windows,run below 4 commands in cmd.exe with administrator priority:

bcdedit /set testsigning on
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
����ԱȨ����ִ�У�
sc delete sfdeskdrv
sc delete sfdesksvc
sc delete inspect
sc delete veracrypt
��������ɾ����װĿ¼�����ļ�
*/

//UAC��
//��¼���ǳ�������ԱAdministrator
//��Ȩ��ʱ�򲻻ᵯ��UAC��ʾ���ڡ�

//��¼���ǹ���Ա���ǳ�������ԱAdministrator��
// ������������requireAdministrator���Եĳ���ʱ���ᵯ��UAC��ʾ��

//��¼���Ǳ�׼�û����ǹ���Ա��
//������������requireAdministrator���Եĳ���ʱ���ᵯ���������Ա�����UAC��ʾ��
// �������Ա����󣬲�����������������

//UAC�ر�
//��¼���ǳ�������ԱAdministrator���߹���Ա����ͨ����Ա��
// ������������requireAdministrator���Եĳ��򣬻����뵽����ԱȨ�ޣ��Ҳ��ᵯ��UAC��ʾ��
//����û������requireAdministrator���Եĳ��򣬷���Ҳ���Թ���ԱȨ�����С�

//��¼���Ǳ�׼�û�
//���������벻������ԱȨ�޵ģ������ǿ��������ģ���������ִ����Ҫ����ԱȨ�޵Ĳ������᷵��ʧ�ܡ�

//f the service type is either SERVICE_WIN32_OWN_PROCESS or SERVICE_WIN32_SHARE_PROCESS,
//and the service is running in the context of the LocalSystem account,
//the following type may also be specified:SERVICE_INTERACTIVE_PROCESS


