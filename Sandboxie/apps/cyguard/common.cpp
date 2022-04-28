#include "common.h"


PCHAR rand_strA(PCHAR str, CONST INT len)
{
	INT i;
	for (i = 0; i < len; ++i)
		str[i] = 'a' + rand() % 26;
	str[i + 1] = '\0';
	return str;
}

PWCHAR rand_strW(PWCHAR str, CONST INT len)
{
	INT i;
	for (i = 0; i < len; ++i)
		str[i] = L'a' + rand() % 26;
	str[i + 1] = L'\0';
	return str;
}


BOOL Is64BitOS()
{
	typedef VOID(WINAPI* LPFN_GetNativeSystemInfo)(__out LPSYSTEM_INFO lpSystemInfo);

	HMODULE hkernel32 = GetModuleHandleW(L"kernel32.dll");
	if (hkernel32)
	{
		LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(hkernel32, "GetNativeSystemInfo");
		if (fnGetNativeSystemInfo)
		{
			SYSTEM_INFO stInfo = { 0 };
			fnGetNativeSystemInfo(&stInfo);
			if (stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 || stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}
