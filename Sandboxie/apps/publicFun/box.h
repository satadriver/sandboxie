#pragma once


#include <windows.h>

#define SECTION_START_FLAG	L"["
#define SECTION_END_FLAG	L"]"



extern "C" __declspec(dllexport) int setUsername(const WCHAR * username);
extern "C" __declspec(dllexport) int getUsername(WCHAR * username);
extern "C" __declspec(dllexport) int setPassword(const WCHAR * pass);
extern "C" __declspec(dllexport) int getPassword(WCHAR * pass);

#define BOX_CONFIG_BUF_SIZE		0X1000

extern "C" __declspec(dllexport) int findValueInConfig(const WCHAR* boxname, const WCHAR* keyname, WCHAR* dst);

extern "C" __declspec(dllexport) int setVeraPath(const WCHAR* path);

extern "C" __declspec(dllexport) int getconfigpath(WCHAR* path);

extern "C" __declspec(dllexport) WCHAR* getBoxSection(const WCHAR* boxname, WCHAR** hdr, WCHAR** body, WCHAR** end);

extern "C" __declspec(dllexport) int deletebox(const WCHAR* boxname);

extern "C" __declspec(dllexport) int createbox(const WCHAR* boxname);

extern "C" __declspec(dllexport) int reloadbox();

extern "C" __declspec(dllexport) int modifyConfigValue(const WCHAR* boxname, const WCHAR* keyname,const WCHAR* value);

extern "C" __declspec(dllexport) int reloadboxInCmd();

extern "C" __declspec(dllexport) int enumSandboxNames(WCHAR* boxnames);


extern "C" __declspec(dllexport) int findBoxNames(WCHAR* buf);

extern "C" __declspec(dllexport) int runDesktopBox(const wchar_t* boxname);

extern "C" __declspec(dllexport) int createRecycleFolderInBox(const WCHAR* boxname);



