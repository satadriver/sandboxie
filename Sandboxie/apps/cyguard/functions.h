#pragma once
#include <windows.h>
#include <fileapi.h >
#include <unordered_map>

using namespace std;

extern DWORD g_tlsIdx;

typedef BOOL(WINAPI* ShowWindowStub)(
	HWND hWnd,
	int  nCmdShow
	);
extern ShowWindowStub ShowWindowOld;
BOOL WINAPI ShowWindowNew(
	HWND hWnd,
	int  nCmdShow
);

typedef BOOL(__stdcall* SetWindowPosStub)(
	HWND hWnd,
	HWND hWndInsertAfter,
	int  X,
	int  Y,
	int  cx,
	int  cy,
	UINT uFlags
	);
extern SetWindowPosStub SetWindowPosOld;
BOOL __stdcall SetWindowPosNew(
	HWND hWnd,
	HWND hWndInsertAfter,
	int  X,
	int  Y,
	int  cx,
	int  cy,
	UINT uFlags
);

typedef HWND(__stdcall* CreateWindowExWStub)(
	DWORD     dwExStyle,
	LPCWSTR   lpClassName,
	LPCWSTR   lpWindowName,
	DWORD     dwStyle,
	int       X,
	int       Y,
	int       nWidth,
	int       nHeight,
	HWND      hWndParent,
	HMENU     hMenu,
	HINSTANCE hInstance,
	LPVOID    lpParam
	);
extern CreateWindowExWStub CreateWindowExWOld;
HWND __stdcall CreateWindowExWNew(
	DWORD     dwExStyle,
	LPCWSTR   lpClassName,
	LPCWSTR   lpWindowName,
	DWORD     dwStyle,
	int       X,
	int       Y,
	int       nWidth,
	int       nHeight,
	HWND      hWndParent,
	HMENU     hMenu,
	HINSTANCE hInstance,
	LPVOID    lpParam);

typedef BOOL(__stdcall* BitBltStub)(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop);
extern BitBltStub BitBltOld;
BOOL __stdcall BitBltNew(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop);


typedef  int (WINAPI* GetDIBitsStub)(_In_ HDC hdc, _In_ HBITMAP hbm, _In_ UINT start, _In_ UINT cLines,
	_Out_opt_ LPVOID lpvBits, _At_((LPBITMAPINFOHEADER)lpbmi, _Inout_) LPBITMAPINFO lpbmi, _In_ UINT usage);

int WINAPI GetDIBitsNew(_In_ HDC hdc, _In_ HBITMAP hbm, _In_ UINT start, _In_ UINT cLines,
	_Out_opt_ LPVOID lpvBits, _At_((LPBITMAPINFOHEADER)lpbmi, _Inout_) LPBITMAPINFO lpbmi, _In_ UINT usage);

extern GetDIBitsStub GetDIBitsOld;


typedef  BOOL (WINAPI* PrintWindowStub)(_In_ HWND hwnd,_In_ HDC hdcBlt,_In_ UINT nFlags);
BOOL WINAPI PrintWindowNew(_In_ HWND hwnd, _In_ HDC hdcBlt, _In_ UINT nFlags);
extern PrintWindowStub PrintWindowOld;


typedef  HDC (WINAPI* CreateDCWStub)( LPCWSTR pwszDriver,  LPCWSTR pwszDevice,  LPCWSTR pszPort,  CONST DEVMODEW* pdm);
extern CreateDCWStub CreateDCWOld;
HDC WINAPI CreateDCWNew( LPCWSTR pwszDriver,  LPCWSTR pwszDevice,  LPCWSTR pszPort,  CONST DEVMODEW * pdm);



typedef  HDC(WINAPI* CreateDCAStub)( LPCSTR pwszDriver,  LPCSTR pwszDevice,  LPCSTR pszPort,  CONST DEVMODE* pdm);
extern CreateDCAStub CreateDCAOld;
HDC WINAPI CreateDCANew( LPCSTR pszDriver,  LPCSTR pszDevice,  LPCSTR pszPort,  CONST DEVMODE * pd);



typedef HDC (__stdcall* ptrGetWindowDC)(HWND hWnd);
extern ptrGetWindowDC GetWindowDCOld;
HDC __stdcall GetWindowDCNew(HWND hWnd);

typedef HDC (WINAPI *GetDCStub)( HWND hWnd);
HDC   WINAPI GetDCNew(HWND hwnd);
extern GetDCStub GetDCOld;



typedef DWORD (__stdcall *ptrGetLogicalDriveStringsW)(DWORD  nBufferLength,LPWSTR lpBuffer);
DWORD __stdcall GetLogicalDriveStringsWNew(DWORD  nBufferLength, LPWSTR lpBuffer);
extern ptrGetLogicalDriveStringsW GetLogicalDriveStringsWOld;


typedef DWORD (__stdcall * ptrGetLogicalDrives)();

DWORD __stdcall GetLogicalDrivesNew();

extern ptrGetLogicalDrives GetLogicalDrivesOld;


typedef HANDLE (__stdcall *ptrFindFirstVolumeW)(LPWSTR lpszVolumeName,DWORD  cchBufferLength);

HANDLE __stdcall FindFirstVolumeWNew(LPWSTR lpszVolumeName, DWORD  cchBufferLength);

extern ptrFindFirstVolumeW FindFirstVolumeWOld;



typedef BOOL (__stdcall *ptrFindNextVolumeW)(HANDLE hFindVolume,LPWSTR lpszVolumeName,DWORD  cchBufferLength);

BOOL __stdcall FindNextVolumeWNew(HANDLE hFindVolume, LPWSTR lpszVolumeName, DWORD  cchBufferLength);

extern ptrFindNextVolumeW FindNextVolumeWOld;

int isVeracryptDisk(WCHAR* diskname);



typedef HRESULT (__stdcall* ptrD3DXSaveSurfaceToFileA)(
	_In_       LPCSTR              pDestFile,
	_In_       LPVOID DestFormat,
	_In_       LPVOID   pSrcSurface,
	_In_ const PALETTEENTRY* pSrcPalette,
	_In_ const RECT* pSrcRect
);

typedef HRESULT(__stdcall* ptrD3DXSaveSurfaceToFileW)(
	_In_       LPCWSTR              pDestFile,
	_In_       LPVOID DestFormat,
	_In_       LPVOID   pSrcSurface,
	_In_ const PALETTEENTRY* pSrcPalette,
	_In_ const RECT* pSrcRect
	);

HRESULT __stdcall D3DXSaveSurfaceToFileANew(
	_In_       LPCSTR              pDestFile,
	_In_       LPVOID DestFormat,
	_In_       LPVOID   pSrcSurface,
	_In_ const PALETTEENTRY* pSrcPalette,
	_In_ const RECT* pSrcRect
);

HRESULT __stdcall D3DXSaveSurfaceToFileWNew(
	_In_       LPCWSTR              pDestFile,
	_In_       LPVOID DestFormat,
	_In_       LPVOID   pSrcSurface,
	_In_ const PALETTEENTRY* pSrcPalette,
	_In_ const RECT* pSrcRect
	);

extern ptrD3DXSaveSurfaceToFileA D3DXSaveSurfaceToFileAOld;

extern ptrD3DXSaveSurfaceToFileW D3DXSaveSurfaceToFileWOld;





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

typedef BOOL(WINAPI* P_OpenPrinterW)(_In_opt_ LPWSTR pPrinterName, _Out_ LPHANDLE phPrinter, _In_opt_ LPPRINTER_DEFAULTSW pDefault);

typedef BOOL(*P_OpenPrinter2W)(void* pPrinterName, HANDLE* phPrinter, void* pDefault, void* pOptions);


 BOOL WINAPI OpenPrinterANew(
	_In_opt_    LPSTR             pPrinterName,
	_Out_       LPHANDLE            phPrinter,
	_In_opt_    LPPRINTER_DEFAULTSA pDefault
	);

 BOOL WINAPI OpenPrinter2ANew(
	_In_opt_      LPCSTR                pPrinterName,
	_Out_         LPHANDLE                phPrinter,
	_In_opt_      PPRINTER_DEFAULTSA      pDefault,
	_In_opt_      PPRINTER_OPTIONSA       pOptions
	);

 BOOL WINAPI OpenPrinterWNew( LPWSTR pPrinterName,  LPHANDLE phPrinter,  LPPRINTER_DEFAULTSW pDefault);

 BOOL OpenPrinter2WNew(void* pPrinterName, HANDLE* phPrinter, void* pDefault, void* pOptions);


 extern P_OpenPrinter2W                 __sys_OpenPrinter2W ;
 extern P_OpenPrinterW                  __sys_OpenPrinterW ;
 extern P_OpenPrinter2A                 __sys_OpenPrinter2A ;
 extern P_OpenPrinterA                  __sys_OpenPrinterA ;




 typedef HMODULE (__stdcall *ptrLoadLibraryW)(LPWSTR lpLibFileName );


 HMODULE __stdcall lpLoadLibraryWNew(LPWSTR lpLibFileName);

 extern ptrLoadLibraryW lpLoadLibraryWOld;
