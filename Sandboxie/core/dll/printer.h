#pragma once
#include <windows.h>

typedef struct _PRINTER_DEFAULTSW {
	LPWSTR        pDatatype;
	LPDEVMODEW pDevMode;
	ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTSW, * PPRINTER_DEFAULTSW, * LPPRINTER_DEFAULTSW;

BOOL Gdi_OpenPrinter2W(LPWSTR pPrinterName, HANDLE* phPrinter, void* pDefault, void* pOptions);

BOOL WINAPI Gdi_OpenPrinterW(
	_In_opt_    LPWSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSW pDefault);

BOOL WINAPI Gdi_OpenPrinter2A(
	_In_opt_    LPCSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSW pDefault, _In_opt_      void *       pOptions);

BOOL WINAPI Gdi_OpenPrinterA(
	_In_opt_    LPSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSW pDefault);


