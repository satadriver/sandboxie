#pragma once
#include <Windows.h>

/*
* 获取当前用户完整 SID
*/
BOOL GetSIDA(CHAR sid[MAX_PATH]);
BOOL GetSIDW(WCHAR sid[MAX_PATH]);
