
#include <windows.h>
#include "../publicFun/apiFuns.h"

#pragma warning(disable:4005)

int __stdcall WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {


	int result = 0;

	int argc = 0;
	WCHAR** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (1)
	//MessageBoxA(0, 0, 0, 0);

	if (0)
	{
		MessageBoxA(0, 0, 0, 0);
	}
	

	
	HANDLE hf = CreateFileA("\\\\.\\PhysicalDrive1\\SafeDesktop.hc", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
	if (hf != INVALID_HANDLE_VALUE)
	{
		MessageBoxA(0, "write data ok", "write data ok", MB_OK);
		DWORD cnt = 0;


		CloseHandle(hf);


	}

	//system("net start veracrypt");

	char szinfo[1024];
	wsprintfA(szinfo, "CreateFileA error code:%x",GetLastError());
	MessageBoxA(0, szinfo, szinfo, MB_OK);
	return result;
}
