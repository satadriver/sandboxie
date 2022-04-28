#pragma once

#include <windows.h>

#define CONFIG_FILENAME "sfdesktop.yml"


#define IOCTL_WFP_SDWS_ADD_SERVER	CTL_CODE(FILE_DEVICE_UNKNOWN,0x801,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_WFP_SDWS_ADD_DNS		CTL_CODE(FILE_DEVICE_UNKNOWN,0x802,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_WFP_SDWS_ADD_PORT		CTL_CODE(FILE_DEVICE_UNKNOWN,0x803,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_WFP_SDWS_ADD_IPV4		CTL_CODE(FILE_DEVICE_UNKNOWN,0x804,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_WFP_SDWS_ADD_IPV6		CTL_CODE(FILE_DEVICE_UNKNOWN,0x805,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_WFP_SDWS_ADD_URL		CTL_CODE(FILE_DEVICE_UNKNOWN,0x806,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS|FILE_ANY_ACCESS)
#define IOCTL_WFP_SDWS_ADD_PROCESS	CTL_CODE(FILE_DEVICE_UNKNOWN,0x807,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS|FILE_ANY_ACCESS)
#define IOCTL_WFP_SDWS_ADD_INTRANET	CTL_CODE(FILE_DEVICE_UNKNOWN,0x808,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS|FILE_ANY_ACCESS)

#define IOCTL_WFP_SDWS_CLEAR_DNS		CTL_CODE(FILE_DEVICE_UNKNOWN,0x809,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS|FILE_ANY_ACCESS)
#define IOCTL_WFP_SDWS_CLEAR_IPPORT		CTL_CODE(FILE_DEVICE_UNKNOWN,0x80A,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS|FILE_ANY_ACCESS)
#define IOCTL_WFP_SDWS_CLEAR_PROCESS	CTL_CODE(FILE_DEVICE_UNKNOWN,0x80B,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS|FILE_ANY_ACCESS)



// #define IPV4_TYPE	0X12345678
// #define PORT_TYPE	0X01234567
// #define DNS_TYPE	0X23456789
// #define URL_TYPE	0X3456789A


#pragma pack(1)

typedef struct  
{
	DWORD type;
	WORD port;
	UCHAR dir;
}PORT_PARAMS;

typedef struct
{
	DWORD type;
	DWORD ip;
	DWORD mask;
	UCHAR dir;
}IPV4_PARAMS;

typedef struct
{
	DWORD type;
	UCHAR ipv6[16];
	UCHAR dir;
}IPV6_PARAMS;

typedef struct
{
	WCHAR boxname[34];
	WCHAR filepath[256];
	WCHAR verafilename[MAX_PATH];
}RUNNING_PARAMETERS;


typedef struct _MyBoxList
{
	_MyBoxList* next;
	_MyBoxList* prev;
	WCHAR boxname[34];
}MyBoxList;

#pragma pack()


MyBoxList * searchboxlist(const WCHAR* boxname);

int addboxlist(const WCHAR *boxname);

int deleteboxlist(const WCHAR *boxname);

extern "C" __declspec(dllexport) int parseUrl(const char* url, char* dsturl);

extern "C" __declspec(dllexport) int parseDomain(const char* dnsname, char* dstname);

extern "C" __declspec(dllexport) DWORD parseIPv4(const char* stripv4, DWORD* dstip, DWORD* mask);

extern "C" __declspec(dllexport) WORD parsePort(WORD port);

extern "C" __declspec(dllexport) int resetProcessMonitor();

extern "C" __declspec(dllexport) int setProcessMonitor(const WCHAR* processname);

extern "C" __declspec(dllexport) int setPrinterControl(BOOLEAN enable);

extern "C" __declspec(dllexport) BOOLEAN SetWatermarkControl(BOOLEAN enable);

extern "C" __declspec(dllexport) BOOLEAN SetScreenCaptureControl(BOOLEAN enable);

extern "C" __declspec(dllexport) int setScreenshot(int enable);

extern "C" __declspec(dllexport) int setWatermark(int enable);

extern "C" __declspec(dllexport) int getProcessInBox(const WCHAR* boxname,DWORD * pids);

extern "C" __declspec(dllexport) int startSbieSvc();

extern "C" __declspec(dllexport) bool IsVboxVM();

extern "C" __declspec(dllexport) bool IsVMwareVM();

BOOL SelfDelete();

