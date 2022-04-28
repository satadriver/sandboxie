#pragma once
#pragma once

#include <windows.h>
#include<vector>

#include <GdiPlus.h>
#include <iostream> 
#include <string>


using namespace  std;
using namespace Gdiplus;

#define STR_CLASSNAME		L"mytestwindow"
#define STR_CLASSPOINTER	L"CLASSPOINTER"

static HINSTANCE m_s_hInstance = 0;

class CMScreenInfoCtrl
{
public:
	CMScreenInfoCtrl(void);
	~CMScreenInfoCtrl(void);

	int				 m_nTop;
	int				 m_nLeft;
	int				 m_nBottom;
	int				 m_nRight;
	int				 m_nWidth;
	int				 m_nHeight;

	HWND			 m_hWnd;
	wstring			 m_wstrText;
	BLENDFUNCTION	 m_Blend;
	wstring			 m_FontName;
	REAL			 m_fontsize;
	StringFormat	 m_format;
	std::wstring     m_wsColor;
	ULONG_PTR		 m_gdiplusToken;

	void RefreshSCWinSize();
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	ATOM RegisterWindowClass();
	WORD CreateSCWindow();
	LRESULT OnWndProc(UINT message, WPARAM wParam, LPARAM lParam);
	void     OnPaint(HDC hdc);
	size_t   GenerateVecPt(vector<Point>& vecPt, const wchar_t* lpwstr);

	void OnPaintNew(HDC hdc);
};
