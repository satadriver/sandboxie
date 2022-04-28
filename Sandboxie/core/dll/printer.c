
#include <windows.h>

#include "printer.h"


typedef int(__stdcall* ptrMessageBoxA)(HWND    hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT    uType);





typedef struct _PRINTER_OPTIONSA
{
	UINT            cbSize;
	DWORD           dwFlags;
} PRINTER_OPTIONSA, * PPRINTER_OPTIONSA, * LPPRINTER_OPTIONSA;

typedef struct _PRINTER_DEFAULTSA {
	LPSTR         pDatatype;
	LPDEVMODEA pDevMode;
	ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTSA, * PPRINTER_DEFAULTSA, * LPPRINTER_DEFAULTSA;



typedef BOOL(WINAPI* P_OpenPrinterA)(
	_In_opt_    LPSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSA pDefault
	);

typedef BOOL(WINAPI* P_OpenPrinter2A)(
	_In_opt_      LPCSTR                pPrinterName,
	_Out_         LPHANDLE                phPrinter,
	_In_opt_      PPRINTER_DEFAULTSA      pDefault,
	_In_opt_      PPRINTER_OPTIONSA       pOptions
	);

typedef BOOL(WINAPI* P_OpenPrinter2W)(void* pPrinterName, HANDLE* phPrinter, void* pDefault, void* pOptions);

typedef BOOL(WINAPI* P_OpenPrinterW)(_In_opt_ LPWSTR pPrinterName, _Out_ LPHANDLE phPrinter, _In_opt_ LPPRINTER_DEFAULTSW pDefault);

extern P_OpenPrinterA	__sys_OpenPrinterA;

extern P_OpenPrinter2A __sys_OpenPrinter2A;

extern P_OpenPrinter2W __sys_OpenPrinter2W;

extern P_OpenPrinterW __sys_OpenPrinterW;

ptrMessageBoxA lpMessageBoxA = 0;

extern BOOLEAN g_printerControl;


BOOL newMessageBoxA(HWND    hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT    uType) {
	if (lpMessageBoxA == 0)
	{
		HMODULE hdll = LoadLibraryA(L"User32.dll");
		if (hdll)
		{
			lpMessageBoxA = (ptrMessageBoxA)GetProcAddress(hdll, "MessageBoxA");
		}
	}
	int result = 0;
	if (lpMessageBoxA)
	{
		result = lpMessageBoxA(hWnd, lpText, lpCaption, uType);
	}
	
	return result;
}



BOOL WINAPI Gdi_OpenPrinterA(
	_In_opt_    LPSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSA pDefault) {

	OutputDebugStringW(L"OpenPrinterA enter\r\n");

	int result = 0;
	if (g_printerControl)
	{
		if (phPrinter)
		{
			result = IsBadWritePtr((LPVOID)phPrinter, sizeof(HANDLE));
			if (result == 0)
			{
				newMessageBoxA(0, "without permission to print document", "unauthorized printer", MB_OK);
				*phPrinter = 0;
			}
		}
	}
	else {
		return __sys_OpenPrinterA(pPrinterName, phPrinter, pDefault);
	}

	return FALSE;
}

BOOL WINAPI Gdi_OpenPrinter2A(
	_In_opt_    LPCSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSA pDefault, _In_opt_ PPRINTER_OPTIONSA       pOptions) {

	OutputDebugStringW(L"OpenPrinter2A enter\r\n");
	int result = 0;
	if (g_printerControl)
	{
		if ( phPrinter)
		{
			result = IsBadWritePtr((LPVOID)phPrinter, sizeof(HANDLE));
			if (result == 0)
			{
				newMessageBoxA(0, "without permission to print document", "unauthorized printer", MB_OK);
				*phPrinter = 0;
			}
		}
	}
	else {
		return __sys_OpenPrinter2A(pPrinterName, phPrinter, pDefault,pOptions);
	}

	return FALSE;
}

BOOL WINAPI Gdi_OpenPrinterW(
	_In_opt_    LPWSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSW pDefault) {

	OutputDebugStringW(L"OpenPrinterW enter\r\n");
	int result = 0;
	if (g_printerControl)
	{
		if ( phPrinter)
		{
			result = IsBadWritePtr((LPVOID)phPrinter, sizeof(HANDLE));
			if (result == 0)
			{
				newMessageBoxA(0, "without permission to print document", "unauthorized printer", MB_OK);
				*phPrinter = 0;
			}
		}
	}
	else {
		return __sys_OpenPrinterW(pPrinterName, phPrinter, pDefault);
	}

	return FALSE;
}

BOOL Gdi_OpenPrinter2W(void* pPrinterName, HANDLE* phPrinter, void* pDefault, void* pOptions) {
	OutputDebugStringW(L"OpenPrinter2W enter\r\n");
	int result = 0;
	if (g_printerControl)
	{
		if (phPrinter)
		{
			result = IsBadWritePtr((LPVOID)phPrinter, sizeof(HANDLE));
			if (result == 0)
			{
				newMessageBoxA(0, "without permission to print document", "unauthorized printer", MB_OK);
				*phPrinter = 0;
			}
		}
	}
	else {
		return __sys_OpenPrinter2W(pPrinterName, phPrinter, pDefault,pOptions);
	}

	return FALSE;
}
