#pragma once
#include <Windows.h>
#include <iostream>
#include <ctime>

/* 随机字符串
* str：字符串
* len：字符串长度
*/
PCHAR rand_strA(PCHAR str, CONST INT len);
PWCHAR rand_strW(PWCHAR str, CONST INT len);

/* 判断系统位数
*/
BOOL Is64BitOS();

