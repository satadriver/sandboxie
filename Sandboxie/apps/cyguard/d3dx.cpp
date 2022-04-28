
#include <windows.h>

#include "d3dx.h"
#include "functions.h"
#include "hookapi.h"

int hookD3dxModule() {
	WCHAR name[MAX_PATH];
	int result = 0;
	int cnt = 0;
	for (int i = 0;i < 100;i ++)
	{
		wsprintfW(name, L"d3dx9_%u.dll", i);
		HMODULE h = GetModuleHandleW(name);
		if (h)
		{
			cnt++;
#ifdef _WIN64
			ptrD3DXSaveSurfaceToFileA lpD3DXSaveSurfaceToFileA = (ptrD3DXSaveSurfaceToFileA)GetProcAddress(h, "D3DXSaveSurfaceToFileA");
			result = inlinehook64((BYTE*)D3DXSaveSurfaceToFileANew, (BYTE*)lpD3DXSaveSurfaceToFileA,
				(PROC*)&D3DXSaveSurfaceToFileAOld, L"D3DXSaveSurfaceToFileA");

			ptrD3DXSaveSurfaceToFileW lpD3DXSaveSurfaceToFileW = (ptrD3DXSaveSurfaceToFileW)GetProcAddress(h, "D3DXSaveSurfaceToFileW");
			result = inlinehook64((BYTE*)D3DXSaveSurfaceToFileWNew, (BYTE*)lpD3DXSaveSurfaceToFileW,
				(PROC*)&D3DXSaveSurfaceToFileWOld, L"D3DXSaveSurfaceToFileW");
#else
			ptrD3DXSaveSurfaceToFileA lpD3DXSaveSurfaceToFileA = (ptrD3DXSaveSurfaceToFileA)GetProcAddress(h, "D3DXSaveSurfaceToFileA");
			result = inlinehook32((BYTE*)D3DXSaveSurfaceToFileANew, (BYTE*)lpD3DXSaveSurfaceToFileA,
				(PROC*)&D3DXSaveSurfaceToFileAOld, L"D3DXSaveSurfaceToFileA");

			ptrD3DXSaveSurfaceToFileW lpD3DXSaveSurfaceToFileW = (ptrD3DXSaveSurfaceToFileW)GetProcAddress(h, "D3DXSaveSurfaceToFileW");
			result = inlinehook32((BYTE*)D3DXSaveSurfaceToFileWNew, (BYTE*)lpD3DXSaveSurfaceToFileW,
				(PROC*)&D3DXSaveSurfaceToFileWOld, L"D3DXSaveSurfaceToFileW");
#endif
		}
	}

	return cnt;
}
