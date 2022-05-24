
#include <windows.h>
#include "../publicFun/config.h"
#include "json/json.h"
#include "json/reader.h"
#include "../publicFun/Utils.h"
#include "configStrategy.h"
#include <iostream>
#include "../publicFun/vera.h"
#include "../publicFun/box.h"
#include "../publicFun/main.h"

#include "../publicFun/apiFuns.h"

#include "md5.h"


using namespace std;

#pragma warning(disable: 4996)


int setJsonConfig(const WCHAR* filename, char* utf8data, int utf8size) {
	int result = 0;

	//mylog(L"setConfig start\r\n");

	HANDLE hWfp = CreateFileW(WFP_DEVICE_SYMBOL, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hWfp == 0 || hWfp == INVALID_HANDLE_VALUE)
	{
		mylog(L"CreateFileW %ws error\r\n", WFP_DEVICE_SYMBOL);
#ifndef _DEBUG
		//return FALSE;
#endif
	}
	DWORD dwretcode = 0;
	if (hWfp != INVALID_HANDLE_VALUE)
	{
		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_CLEAR_DNS, 0, 0, NULL, 0, &dwretcode, NULL);
		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_CLEAR_IPPORT, 0, 0, NULL, 0, &dwretcode, NULL);
		result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_CLEAR_PROCESS, 0, 0, NULL, 0, &dwretcode, NULL);
	}

	resetAllBoxList();

	for (int i = 0; i < sizeof(g_processBlackList) / sizeof(WCHAR*); i++)
	{
		if (g_processBlackList[i])
		{
#ifndef _DEBUG
			setProcessMonitor(g_processBlackList[i]);
#endif
		}
	}

	if (utf8data == 0 || utf8size == 0)
	{
		if (filename == 0)
		{
			filename = JSON_CONFIG_FILENAME;
		}
		result = fileReaderW(filename, (WCHAR**)&utf8data, &utf8size);
		if (result <= 0)
		{
			mylog(L"fileReader %ws error %d\r\n", JSON_CONFIG_FILENAME, GetLastError());

			if (hWfp == INVALID_HANDLE_VALUE)
			{
				CloseHandle(hWfp);
			}
			return FALSE;
		}
		else {
			//mylog(L"fileReader %ws size:%d\r\n", JSON_CONFIG_FILENAME, utf8size);
		}
		//DeleteFileW(JSON_CONFIG_FILENAME);
	}

	char* filedata = new char[(ULONGLONG)utf8size + 1024];
	//removeQuota(utf8data, utf8size);
	int filesize = UTF8ToGBK((char*)utf8data, &filedata);
	//delete utf8data;

	Json::Value root;
	Json::Reader reader;
	reader.parse(filedata, root);

	int errorcode = root["err_code"].asInt();
	string retstr = root["ret"].asString();
	if (errorcode != 0 || retstr != "success")
	{
		if (hWfp == INVALID_HANDLE_VALUE)
		{
			CloseHandle(hWfp);
		}
		mylog(L"errro code:%d,ret string:%s\r\n", errorcode, retstr.c_str());
		return FALSE;
	}

	string timestamp = root["timestamp"].asString();
	string cmd = root["cmd"].asString();
	if (cmd != "getSandBoxRule" && cmd != "updateSandBoxRule")
	{
		mylog("cmd:%s\r\n", cmd.c_str());
		//return FALSE;
	}

	Json::Value data = root["data"];
	int datacnt = data.size();
	if (datacnt <= 0)
	{
		mylog(L"data param error\r\n");
	}


	WCHAR wstrusername[MAX_PATH];
	getUsername(wstrusername);

	for (int data_number = 0; data_number < datacnt; data_number++)
	{
		Json::Value spaceinfo = data[data_number]["spaceInfo"];
		Json::Value safepolicy = data[data_number]["safePolicy"];

		string name = spaceinfo["name"].asString();
		int spaceType = spaceinfo["spaceType"].asInt();


		if (name == "")
		{
			continue;
		}

		WCHAR wstrname[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, name.c_str(), -1, wstrname, sizeof(wstrname));

		WCHAR wstrboxname[MAX_PATH];
		wsprintfW(wstrboxname, L"%ws_%ws", wstrusername, wstrname);



		Json::Value fileexportvalue = safepolicy["fileExport"];
		if (fileexportvalue.isNull() == FALSE)
		{
			int fileexport = safepolicy["fileExport"].asInt();
			//public_SetExportFlag(fileexport);
			SbieApi__SetFileExport(wstrboxname,fileexport);
		}

		Json::Value screencapvalue = safepolicy["screenCapture"];
		if (screencapvalue.isNull() == FALSE)
		{
			BOOLEAN screencap = safepolicy["screenCapture"].asInt();
			SbieApi__SetScreenshot(wstrboxname,screencap);
			//SetScreenCaptureControl(screencap);
		}

		Json::Value watermarkvalue = safepolicy["waterMark"];
		if (watermarkvalue.isNull() == FALSE)
		{
			BOOLEAN watermark = safepolicy["waterMark"].asInt();
			//setWatermark(watermark);
			SbieApi__SetWatermark(wstrboxname,watermark);
		}

		Json::Value printervalue = safepolicy["printer"];
		if (printervalue.isNull() == FALSE)
		{
			BOOLEAN printer = safepolicy["printer"].asInt();
			//setPrinterControl(printer);
			SbieApi__SetPrinter(wstrboxname,printer);
		}

		//mylog(L"screen:%d,watermark:%d,printer:%d,fileexport:%d", screencap, watermark, printer, fileexport);

		BOOLEAN programLimit = safepolicy["programLimit"].asBool();
		if (programLimit)
		{
			Json::Value allowPrograms = safepolicy["allowPrograms"];
			int allowprogcnt = allowPrograms.size();
			for (int j = 0; j < allowprogcnt; j++)
			{
				string name = allowPrograms[j]["name"].asString();

				string copyright = allowPrograms[j]["copyrightInfo"].asString();

				string protype = allowPrograms[j]["proType"].asString();

				Json::Value programs = allowPrograms[j]["programs"];
				int programs_cnt = programs.size();
				for (int k = 0; k < programs_cnt; k++)
				{
					string sign = programs[k]["signerInfo"].asString();

					string procname = programs[k]["processName"].asString();
					WCHAR wstrprocname[MAX_PATH];
					MultiByteToWideChar(CP_ACP, 0, procname.c_str(), -1, wstrprocname, MAX_PATH);
					setProcessMonitor(wstrprocname);
				}
			}
		}


		int resProtectState = safepolicy["resProtectState"].asBool();
		if (resProtectState == 1)
		{
			Json::Value resList = safepolicy["resList"];
			int resListcnt = resList.size();
			for (int j = 0; j < resListcnt; j++)
			{
			}
		}

		BOOLEAN outNetworkLimit = safepolicy["outNetworkLimit"].asBool();
		if (outNetworkLimit)
		{
			Json::Value limitNetworks = safepolicy["limitNetworks"];
			int limitNetworkscnt = limitNetworks.size();
			for (int j = 0; j < limitNetworkscnt; j++)
			{
				string strip = limitNetworks[j]["network"].asString();
				IPV4_PARAMS ipparam;
				ipparam.type = IOCTL_WFP_SDWS_ADD_IPV4;
				ipparam.dir = FWP_DIRECTION_OUTBOUND;
				result = parseIPv4(strip.c_str(), &ipparam.ip, &ipparam.mask);
				result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_IPV4, &ipparam, sizeof(IPV4_PARAMS), NULL, 0, &dwretcode, NULL);
				//mylog("DeviceIoControl type:%d ip:%x ipstr:%s\r\n", IOCTL_WFP_SDWS_ADD_IPV4, ipparam.ip, strip.c_str());

				string port = limitNetworks[j]["port"].asString();
				PORT_PARAMS portparam;
				portparam.type = IOCTL_WFP_SDWS_ADD_PORT;
				portparam.dir = FWP_DIRECTION_OUTBOUND;
				portparam.port = parsePort(atoi(port.c_str()));
				result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_PORT, (LPVOID)&portparam, sizeof(PORT_PARAMS), NULL, 0, &dwretcode, NULL);
			}
		}
		
	}

	if (hWfp != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hWfp);
	}

	delete[]filedata;

	//mylog(L"setConfig complete\r\n");
	return 0;
}



extern "C" __declspec(dllexport) int configBox(const WCHAR * wstrusername, const WCHAR * wstrpassword, WCHAR * verapath, WCHAR * strverasize) {
	int result = 0;
	if (wstrusername == 0 || wstrpassword == 0)
	{
		return FALSE;
	}
	else {
		result = setUsername(wstrusername);
		result = setPassword(wstrpassword);

		unsigned char md5[32] = { 0 };
		unsigned char szpassword[64];
		WideCharToMultiByte(CP_ACP, 0, wstrpassword, -1, (char*)szpassword, sizeof(szpassword), 0, 0);
		getMD5(szpassword, md5);
		MultiByteToWideChar(CP_ACP, 0, (char*)md5, -1, VERACRYPT_PASSWORD, sizeof(VERACRYPT_PASSWORD) / sizeof(WCHAR));
	}

	WCHAR curdir[MAX_PATH];
	result = GetModuleFileNameW(0, curdir, MAX_PATH);
	WCHAR* pos = wcsrchr(curdir, L'\\');
	if (pos)
	{
		//pos++;
		*pos = 0;
	}
	//result = LjgApi_setPath(curdir);

	HANDLE hWfp = CreateFileW(WFP_DEVICE_SYMBOL, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hWfp == INVALID_HANDLE_VALUE)
	{
		mylog(L"CreateFileW %ws error\r\n", WFP_DEVICE_SYMBOL);
#ifndef _DEBUG
		return FALSE;
#endif
	}
	DWORD dwretcode = 0;
	result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_CLEAR_DNS, 0, 0, NULL, 0, &dwretcode, NULL);
	result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_CLEAR_IPPORT, 0, 0, NULL, 0, &dwretcode, NULL);
	result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_CLEAR_PROCESS, 0, 0, NULL, 0, &dwretcode, NULL);

	resetAllBoxList();

	for (int i = 0; i < sizeof(g_processBlackList) / sizeof(WCHAR*); i++)
	{
		if (g_processBlackList[i])
		{
#ifndef _DEBUG
			setProcessMonitor(g_processBlackList[i]);
#endif
		}
	}

	if (verapath)
	{
		int verapathlen = lstrlenW(verapath);
		if (verapath[verapathlen - 1] == '/' || verapath[verapathlen - 1] == '\\')
		{
			verapath[verapathlen - 1] = 0;
		}
		setVeraDiskFile(verapath);
	}
	else {
		verapath = getVeraDiskFile();
	}

	if (strverasize)
	{
		modifyConfigValue(L"", VERACRYPT_SIZE_KEYNAME, strverasize);
	}
	else {
		WCHAR tmpvs[256];
		strverasize = tmpvs;
		findValueInConfig(L"", VERACRYPT_SIZE_KEYNAME, strverasize);
	}
	int verasize = _wtoi(strverasize);
	if (verasize == 0)
	{
		mylog("veracrypt size set to %d\r\n", DEFAULT_VERACRYPT_SIZE);
		verasize = DEFAULT_VERACRYPT_SIZE;
	}
	result = initVeraDisk(verasize);

	CHAR* utf8data = 0;
	int utf8size = 0;
	result = fileReaderW(JSON_CONFIG_FILENAME, (WCHAR**)&utf8data, &utf8size);
	if (result <= 0)
	{
		CloseHandle(hWfp);
		mylog(L"fileReader %ws error\r\n", JSON_CONFIG_FILENAME);
		return FALSE;
	}
	else {
		mylog(L"fileReader %ws size:%d\r\n", JSON_CONFIG_FILENAME, utf8size);
	}
#ifndef _DEBUG
	//DeleteFileW(JSON_CONFIG_FILENAME);
#endif


	char* filedata = new char[(ULONGLONG)utf8size + 1024];
	//removeQuota(utf8data, utf8size);
	int filesize = UTF8ToGBK((char*)utf8data, &filedata);
	delete utf8data;

	Json::Value root;
	Json::Reader reader;
	reader.parse(filedata, root);
	int errorcode = root["err_code"].asInt();
	string retstr = root["ret"].asString();
	if (errorcode != 0 || retstr != "success")
	{
		if (hWfp != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hWfp);
		}
		mylog(L"errro code:%d,ret string:%s\r\n", errorcode, retstr.c_str());
		return FALSE;
	}

	string timestamp = root["timestamp"].asString();

	string cmd = root["cmd"].asString();
	if (cmd != "getSandBoxRule" && cmd != "updateSandBoxRule")
	{
		//CloseHandle(hWfp);
		mylog("cmd:%s error\r\n", cmd.c_str());
		//return FALSE;
	}
	else {
		mylog("parse config section:%s", cmd.c_str());
	}

	Json::Value data = root["data"];
	int datacnt = data.size();

	for (int data_number = 0; data_number < datacnt; data_number++)
	{
		Json::Value spaceinfo = data[data_number]["spaceInfo"];
		Json::Value safepolicy = data[data_number]["safePolicy"];

		string name = spaceinfo["name"].asString();
		int spaceType = spaceinfo["spaceType"].asInt();

		if (name == "")
		{
			continue;
		}

		WCHAR wstrboxname[MAX_PATH];

		WCHAR wstrname[MAX_PATH];
		int len = MultiByteToWideChar(CP_ACP, 0, name.c_str(), -1, wstrname, MAX_PATH);
			
		wsprintfW(wstrboxname, L"%ws_%ws", wstrusername, wstrname);
		int cnt = 0;
		do
		{
			result = createbox(wstrboxname);
			if (result)
			{
				mylog(L"createRecycleFolderInBox and runDesktopBox %ws\r\n", wstrboxname);
				result = createRecycleFolderInBox(wstrboxname);

				result = runDesktopBox(wstrboxname);
				if (result == 0)
				{
					LjgApi_ReloadConf(-1, 0);
					cnt++;
				}
				else {
					break;
				}
			}
			else {
				mylog(L"createbox %ws error %d\r\n", wstrboxname, GetLastError());
				LjgApi_ReloadConf(-1, 0);
				cnt++;
			}
			Sleep(500);
		} while (cnt < 3);
		
		Json::Value fileexportvalue = safepolicy["fileExport"];
		if (fileexportvalue.isNull() == FALSE)
		{
			int fileexport = safepolicy["fileExport"].asInt();
			//public_SetExportFlag(fileexport);
			SbieApi__SetFileExport(wstrboxname,fileexport);
		}

		Json::Value screencapvalue = safepolicy["screenCapture"];
		if (screencapvalue.isNull() == FALSE)
		{
			BOOLEAN screencap = safepolicy["screenCapture"].asInt();
			SbieApi__SetScreenshot(wstrboxname,screencap);
			//SetScreenCaptureControl(screencap);
		}

		Json::Value watermarkvalue = safepolicy["waterMark"];
		if (watermarkvalue.isNull() == FALSE)
		{
			BOOLEAN watermark = safepolicy["waterMark"].asInt();
			//setWatermark(watermark);
			SbieApi__SetWatermark(wstrboxname,watermark);
		}

		Json::Value printervalue = safepolicy["printer"];
		if (printervalue.isNull() == FALSE)
		{
			BOOLEAN printer = safepolicy["printer"].asInt();
			//setPrinterControl(printer);
			SbieApi__SetPrinter(wstrboxname,printer);
		}

		//mylog(L"screen:%d,watermark:%d,printer:%d,fileexport:%d", screencap, watermark, printer, fileexport);

		BOOLEAN screencap = SbieApi__QueryScreenshot();
		BOOLEAN watermark = SbieApi__QueryWatermark();
		BOOLEAN printer = SbieApi__QueryPrinter();
		mylog(L"query screen:%d,watermark:%d,printer:%d", screencap, watermark, printer);

		BOOLEAN programLimit = safepolicy["programLimit"].asBool();
		if (programLimit)
		{
			Json::Value allowPrograms = safepolicy["allowPrograms"];
			int allowprogcnt = allowPrograms.size();
			for (int j = 0; j < allowprogcnt; j++)
			{
				string name = allowPrograms[j]["name"].asString();

				string copyright = allowPrograms[j]["copyrightInfo"].asString();

				string protype = allowPrograms[j]["proType"].asString();

				Json::Value programs = allowPrograms[j]["programs"];
				int programs_cnt = programs.size();
				for (int k = 0; k < programs_cnt; k++)
				{
					string sign = programs[k]["signerInfo"].asString();

					string procname = programs[k]["processName"].asString();
					WCHAR wstrprocname[MAX_PATH];
					MultiByteToWideChar(CP_ACP, 0, procname.c_str(), -1, wstrprocname, MAX_PATH);
					setProcessMonitor(wstrprocname);
				}
			}
		}



		int resProtectState = safepolicy["resProtectState"].asBool();
		if (resProtectState == 1)
		{
			Json::Value resList = safepolicy["resList"];
			int resListcnt = resList.size();
			for (int j = 0; j < resListcnt; j++)
			{

			}
		}

		BOOLEAN outNetworkLimit = safepolicy["outNetworkLimit"].asBool();
		if (outNetworkLimit)
		{
			Json::Value limitNetworks = safepolicy["limitNetworks"];
			int limitNetworkscnt = limitNetworks.size();
			for (int j = 0; j < limitNetworkscnt; j++)
			{
				string strip = limitNetworks[j]["network"].asString();
				IPV4_PARAMS ipparam;
				ipparam.type = IOCTL_WFP_SDWS_ADD_IPV4;
				ipparam.dir = FWP_DIRECTION_OUTBOUND;
				result = parseIPv4(strip.c_str(), &ipparam.ip, &ipparam.mask);
				result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_IPV4, &ipparam, sizeof(IPV4_PARAMS), NULL, 0, &dwretcode, NULL);
				//mylog("DeviceIoControl type:%d ip:%x ipstr:%s\r\n", IOCTL_WFP_SDWS_ADD_IPV4, ipparam.ip, strip.c_str());

				string port = limitNetworks[j]["port"].asString();
				PORT_PARAMS portparam;
				portparam.type = IOCTL_WFP_SDWS_ADD_PORT;
				portparam.dir = FWP_DIRECTION_OUTBOUND;
				portparam.port = parsePort(atoi(port.c_str()));
				result = DeviceIoControl(hWfp, IOCTL_WFP_SDWS_ADD_PORT, (LPVOID)&portparam, sizeof(PORT_PARAMS), NULL, 0, &dwretcode, NULL);
				//mylog("DeviceIoControl type:%d port:%d\r\n", IOCTL_WFP_SDWS_ADD_PORT, portparam.port);
			}
		}
	}
	delete[] filedata;

	if (hWfp != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hWfp);
		hWfp = INVALID_HANDLE_VALUE;
	}

	return 0;
}






