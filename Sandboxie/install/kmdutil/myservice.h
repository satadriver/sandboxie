#pragma once

#include <windows.h>

BOOL LoadNTDriver(WCHAR* lpszDriverName, WCHAR* lpszDriverPath, int servicetype, int boottype, WCHAR* groupname);
