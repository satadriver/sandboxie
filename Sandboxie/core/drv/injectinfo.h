#pragma once
#include <minwindef.h>

//安全桌面名信息最大长度
#define SAFE_DESKTOP_DESC_MAX_LEN               (64)

//注入信息结构; 32/64 位公用,注意不要有两个平台长度不一样的数据类型
typedef struct _AppInjectInfo
{
	unsigned char version;                                  //协议版本信息 见 APP_INJECT_INFO_VERSIONS 定义
	unsigned char rev[7];                                   //保留
	UINT procType;                                          //进程类别
	DWORD safeLevel;                                        //安全级别
	DWORD safeAbilityFlags;                                 //预留;开启安全能力列表;描述开启哪些安全能力;有多个安全能力时,可以用与的方式给多个;
	WCHAR safeDesktopDesc[SAFE_DESKTOP_DESC_MAX_LEN + 1];   //描述信息
	DWORD extSize;                                          //附带数据大小
#pragma warning(disable: 4200) 
	char extData[0];                                        //附带数据
}AppInjectInfo, * PAppInjectInfo;

//开始注入工作 StartMonitor 调用号
#define UEM_HOOK_MODULE_START_MONITOR_FUCN_ID                   (1)

//取消注入 StopMonitor 调用号
#define UEM_HOOK_MODULE_STOP_MONITOR_FUCN_ID                    (2)

//构造  HOOK 模块导出接口 UEMHookInterfaces 对象
#define UEM_HOOK_MODULE_CREATE_INTERFACES_INSTANCE_ID           (3)




