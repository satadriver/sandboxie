
#include <windows.h>
#include "../publicFun/apiFuns.h"

#pragma warning(disable:4005)

int __stdcall WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

	MessageBoxA(0, 0, 0, 0);
	__debugbreak();

	int result = 0;

	int argc = 0;
	WCHAR** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc < 2)
	{
		//return FALSE;
	}

	if (lstrcmpiW( argv[1],L"get_copypath") == 0 && argc >= 3)
	{

	}


	WCHAR username[64];
	DWORD usernamelen = 64;

	result = GetUserNameW(username,& usernamelen);

	WCHAR filename[MAX_PATH];

	//C:\\Users\\Admin\\Desktop\\

	HANDLE hf = CreateFileA("__debugbreak.txt", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (hf != INVALID_HANDLE_VALUE)
	{
		DWORD cnt = 0;

		const char* data = "hello everyone,good good study!\r\nday day up!\r\n";

		result = WriteFile(hf, data,lstrlenA(data),&cnt,0);

		CloseHandle(hf);

		if (cnt )
		{
			MessageBoxA(0, "write data ok", "write data ok", MB_OK);
			return TRUE;
		}
	}

	char szinfo[1024];
	wsprintfA(szinfo, "write data error code:%x",GetLastError());
	MessageBoxA(0, szinfo, szinfo, MB_OK);
	return result;
}
