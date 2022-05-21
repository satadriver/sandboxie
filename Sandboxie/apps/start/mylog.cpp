#include "mylog.h"

#include <stdio.h>

int __cdecl mylog(const WCHAR* format, ...) {

	WCHAR szbuf[2048];

	va_list   pArgList;

	va_start(pArgList, format);

	int nByteWrite = vswprintf_s(szbuf, format, pArgList);

	va_end(pArgList);

	OutputDebugStringW(szbuf);

	return nByteWrite;
}
