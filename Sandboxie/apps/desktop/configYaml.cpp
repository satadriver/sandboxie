
#include <windows.h>
#include "Yaml.hpp"
#include <string>
#include <io.h>
#include <winsock.h>
#include <stdio.h>
#include <process.h>
#include "configYaml.h"

#include "../../common/my_version.h"
#include "main.h"

#include "../publicFun/Utils.h"
#include "../publicFun/config.h"
#include "../publicFun/box.h"
#include "../publicFun/vera.h"

#include "../publicFun/configStrategy.h"

using namespace std;
using namespace Yaml;

int parseConfig(const WCHAR* wstrusername, const WCHAR* wstrpassword) {
	int result = 0;
	CHAR* data = 0;
	int filesize = 0;
	result = fileReaderA(CONFIG_FILENAME, &data, &filesize);
	if (result <= 0)
	{
		mylog("fileReader %s error\r\n", CONFIG_FILENAME);
		return FALSE;
	}
	else {
		mylog("fileReader %s size:%d\r\n", CONFIG_FILENAME, filesize);
	}

	Node root;
	try
	{
		Parse(root, CONFIG_FILENAME);
	}
	catch (const Exception e)
	{
		mylog("Parse yaml file type:%d,error:%s", e.Type(), e.what());
		return 0;
	}
	//#ifndef _DEBUG
	Node vera = root["Veracrypt"];
	Node strveraSize = vera["Size"];
	int verasize = strveraSize.As<int>();
	result = initVeraDisk(verasize);

	//not run this in a process that now work always
	//result = loadWindowsHook();

	string verapath = vera["Path"].As<std::string>();
	//#endif

	Node boxes = root["Box"];
#if 0
	for (auto itS = boxes.Begin(); itS != boxes.End(); itS++)
	{
		Node box = (*itS).second;
#else
	for (int i = 0; i < boxes.Size(); i++)
	{
		Node box = boxes[i];
#endif
		//yaml is case sensitive
		string boxname = box["Name"].As<std::string>();

		string path = box["Path"].As<std::string>();
		boolean launch = box["Launch"].As<bool>();
		mylog("get box Name:%s Path:%s Launch:%d\r\n", boxname.c_str(), path.c_str(), launch);
		if (launch)
		{
#ifndef _DEBUG
			char username[MAX_PATH];
			WideCharToMultiByte(CP_ACP, 0, wstrusername, -1, username, MAX_PATH, 0, 0);
			char newboxname[MAX_PATH];
			wsprintfA(newboxname, "%s_%s", username, boxname.c_str());
			WCHAR wstrboxname[MAX_PATH];
			int len = MultiByteToWideChar(CP_ACP, 0, newboxname, -1, wstrboxname, MAX_PATH);
			if (len) {
				wstrboxname[len] = 0;
				result = createbox(wstrboxname);
				if (result)
				{
					result = createRecycleFolderInBox(wstrboxname);
					result = runDesktopBox(wstrboxname);
				}
				else {
					mylog(L"createbox %ws error\r\n", wstrboxname);
				}
			}
#endif
		}
	}

	HANDLE hWfp = CreateFileW(WFP_DEVICE_SYMBOL, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hWfp == INVALID_HANDLE_VALUE)
	{
		mylog(L"CreateFileW %ws error:%d\r\n", WFP_DEVICE_SYMBOL, GetLastError());
#ifndef _DEBUG
		return FALSE;
#endif
	}
	else {
		mylog(L"CreateFileW %ws handle:%d\r\n", WFP_DEVICE_SYMBOL, hWfp);
	}

	Node screenshot = root["Screenshot"];
	bool scrnenable = screenshot.As<bool>(false);
#ifndef _DEBUG
	setScreenshot(scrnenable);
#endif

	Node windowsHook = root["WindowsHook"];
	bool winhookEnable = windowsHook.As<bool>(false);

	Node watermark = root["Watermark"];
	bool watermarkenable = watermark.As<bool>(false);
#ifndef _DEBUG
	setWatermark(watermarkenable);
#endif

	Node process = root["Process"];
	for (int i = 0; i < process.Size(); i++) {
		string processname = process[i].As<std::string>();

		WCHAR procname[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, processname.c_str(), -1, procname, MAX_PATH);
#ifndef _DEBUG
		setProcessMonitor(procname);
#endif
	}

	for (int i = 0; i < sizeof(g_processBlackList) / sizeof(WCHAR*); i++)
	{
		if (g_processBlackList[i])
		{
#ifndef _DEBUG
			setProcessMonitor(g_processBlackList[i]);
#endif
		}
	}


	Node network = root["Network"];

	Node allip = network["IPV4"];
	Node ips = allip["Src"];
	Node ipd = allip["Dst"];

	Node allport = network["Port"];
	Node ports = allport["Src"];
	Node portd = allport["Dst"];

	Node dnses = network["Dns"];

	Node urls = network["Url"];

	Node procnames = network["Process"];

	Node intranet = network["Intranet"];

	DWORD dwsize = 0;

	BOOLEAN bintranet = intranet.As<bool>(false);
	result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_INTRANET, &bintranet, sizeof(BOOLEAN), NULL, 0, &dwsize, NULL);

	// 	char* databuf = new char[(int)filesize + 0x1000];
	// 	char* dataptr = databuf;


	for (int i = 0; i < ips.Size(); i++) {
		string strip = ips[i].As<std::string>();

		IPV4_PARAMS param;
		param.type = IOCTL_WFP_SDWS_ADD_IPV4;
		param.dir = FWP_DIRECTION_OUTBOUND;
		result = parseIPv4(strip.c_str(), &param.ip, &param.mask);

		// 		memcpy(dataptr, &param, sizeof(IPV4_PARAMS));
		// 		dataptr += sizeof(IPV4_PARAMS);

		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_IPV4, &param, sizeof(IPV4_PARAMS), NULL, 0, &dwsize, NULL);
		mylog("DeviceIoControl type:%d ip:%x ipstr:%s\r\n", IOCTL_WFP_SDWS_ADD_IPV4, param.ip, strip.c_str());
	}


	for (int i = 0; i < ipd.Size(); i++)
	{
		string strip = ipd[i].As<std::string>();

		IPV4_PARAMS param;
		param.type = IOCTL_WFP_SDWS_ADD_IPV4;
		param.dir = FWP_DIRECTION_INBOUND;
		result = parseIPv4(strip.c_str(), &param.ip, &param.mask);

		// 		memcpy(dataptr, &param, sizeof(IPV4_PARAMS));
		// 		dataptr += sizeof(IPV4_PARAMS);

		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_IPV4, &param, sizeof(IPV4_PARAMS), NULL, 0, &dwsize, NULL);
		mylog("DeviceIoControl type:%d ip:%x ipstr:%s\r\n", IOCTL_WFP_SDWS_ADD_IPV4, param.ip, strip.c_str());
	}


	for (int i = 0; i < ports.Size(); i++) {
		int port = ports[i].As<int>();

		PORT_PARAMS param;
		param.type = IOCTL_WFP_SDWS_ADD_PORT;
		param.dir = FWP_DIRECTION_OUTBOUND;
		param.port = parsePort(port);

		// 		memcpy(dataptr, &param, sizeof(PORT_PARAMS));
		// 		dataptr += sizeof(PORT_PARAMS);

		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_PORT, (LPVOID)&param, sizeof(PORT_PARAMS), NULL, 0, &dwsize, NULL);
		mylog("DeviceIoControl type:%d port:%d\r\n", IOCTL_WFP_SDWS_ADD_PORT, param.port);
	}

	for (int i = 0; i < portd.Size(); i++) {
		int port = portd[i].As<int>();

		PORT_PARAMS param;
		param.type = IOCTL_WFP_SDWS_ADD_PORT;
		param.dir = FWP_DIRECTION_INBOUND;
		param.port = parsePort(port);

		// 		memcpy(dataptr, &param, sizeof(PORT_PARAMS));
		// 		dataptr += sizeof(PORT_PARAMS);

		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_PORT, (LPVOID)&param, sizeof(PORT_PARAMS), NULL, 0, &dwsize, NULL);
		mylog("DeviceIoControl type:%d port:%d\r\n", IOCTL_WFP_SDWS_ADD_PORT, param.port);
	}

	for (int i = 0; i < dnses.Size(); i++) {
		string dns = dnses[i].As<std::string>();

		char dnsdst[256 + 4];

		*(DWORD*)dnsdst = IOCTL_WFP_SDWS_ADD_DNS;

		int dnslen = parseDomain(dns.c_str(), dnsdst + sizeof(DWORD));

		// 		memcpy(dataptr, dnsdst, dnslen + 1 + sizeof(DWORD));
		// 		dataptr += dnslen + 1 + sizeof(DWORD);

		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_DNS, (LPVOID)dnsdst, dnslen + 1 + sizeof(DWORD), NULL, 0, &dwsize, NULL);
		mylog("DeviceIoControl type:%d dns:%s\r\n", IOCTL_WFP_SDWS_ADD_DNS, dnsdst + sizeof(DWORD));
	}

	for (int i = 0; i < urls.Size(); i++) {
		string url = urls[i].As<std::string>();

		char dsturl[256 + 4];

		*(DWORD*)dsturl = IOCTL_WFP_SDWS_ADD_URL;

		int urllen = parseUrl(url.c_str(), dsturl + sizeof(DWORD));

		// 		memcpy(dataptr, dsturl, urllen + 1 + sizeof(DWORD));
		// 		dataptr += urllen + 1 + sizeof(DWORD);

		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_URL, (LPVOID)dsturl, urllen + 1 + sizeof(DWORD), NULL, 0, &dwsize, NULL);
		mylog("DeviceIoControl type:%d dns:%s\r\n", IOCTL_WFP_SDWS_ADD_URL, dsturl + sizeof(DWORD));
	}


	for (int i = 0; i < procnames.Size(); i++) {
		string procname = procnames[i].As<std::string>();

		char dstname[256 + 4];

		*(DWORD*)dstname = IOCTL_WFP_SDWS_ADD_PROCESS;

		lstrcpyA(dstname + sizeof(DWORD), procname.c_str());

		int namelen = (int)procname.length();

		// 		memcpy(dataptr, dstname, namelen + 1 + sizeof(DWORD));
		// 		dataptr += namelen + 1 + sizeof(DWORD);

		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_PROCESS, (LPVOID)dstname, namelen + 1 + sizeof(DWORD), NULL, 0, &dwsize, NULL);
		mylog("DeviceIoControl type:%d dns:%s\r\n", IOCTL_WFP_SDWS_ADD_PROCESS, dstname + sizeof(DWORD));
	}

	CloseHandle(hWfp);

	//result = fileWriter((char*)"c:\\windows\\config.txt", (char*)databuf, (int)(dataptr - databuf),(int)FALSE);
	//delete []databuf;

	delete data;
	return TRUE;
}




