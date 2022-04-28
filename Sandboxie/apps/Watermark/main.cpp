
#include "main.h"
#include<iostream>
#include <string>
#include<stdio.h>
#include <math.h>

#pragma comment (lib,"Gdiplus.lib")

#pragma warning(disable:4005)

int m_monitorNum = 0;
bool m_Flag = false;

CMScreenInfoCtrl::CMScreenInfoCtrl(void)
{
	int status = 0;
	GdiplusStartupInput gdiplusStartupInput;

	m_nTop = 0;
	m_nLeft = 0;
	m_nBottom = 0;
	m_nRight = 0;
	m_nWidth = 0;
	m_nHeight = 0;

	m_gdiplusToken = 0;
	m_wsColor = L"green";

	m_wstrText = L"屏幕水印显示";
	m_FontName = L"楷体";
	m_fontsize = 80;

	status = GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
}

CMScreenInfoCtrl::~CMScreenInfoCtrl(void)
{
	GdiplusShutdown(m_gdiplusToken);
}

ATOM CMScreenInfoCtrl::RegisterWindowClass()
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_s_hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = NULL;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = STR_CLASSNAME;
	wcex.hIconSm = NULL;

	return RegisterClassEx(&wcex);
}

WORD CMScreenInfoCtrl::CreateSCWindow()
{
	INT result = 0;
	result = RegisterWindowClass();

	m_Blend.BlendOp = 0;
	m_Blend.BlendFlags = 0;
	m_Blend.AlphaFormat = 1;
	m_Blend.SourceConstantAlpha = 60;

	m_format.SetAlignment(StringAlignmentNear);

	//刷新屏幕位置坐标和尺寸
	RefreshSCWinSize();

	m_hWnd = ::CreateWindowEx((WS_EX_TOOLWINDOW |
		WS_EX_TRANSPARENT | WS_EX_TOPMOST) & ~WS_EX_APPWINDOW,
		STR_CLASSNAME, L"",
		WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP,
		m_nLeft, m_nTop, m_nWidth, m_nHeight, NULL, NULL, m_s_hInstance, NULL);

	result = ::SetProp(m_hWnd, STR_CLASSPOINTER, this);

	result = ::PostMessage(m_hWnd, WM_PAINT, 0, 0);

//     UpdateWindow(m_hWnd);
//     ShowWindow(m_hWnd,SW_SHOW);

	return 0;
}

LRESULT CALLBACK CMScreenInfoCtrl::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HANDLE h = ::GetProp(hWnd, STR_CLASSPOINTER);
	CMScreenInfoCtrl* p = (CMScreenInfoCtrl*)h;
	if (NULL == h)
	{
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}

	return p->OnWndProc(message, wParam, lParam);
}


LRESULT CMScreenInfoCtrl::OnWndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC          hdc;
	PAINTSTRUCT  ps;

	BringWindowToTop(m_hWnd);

	switch (message)
	{
		//屏幕水印 响应屏幕显示变化事件，包括 分辨率变化、增加/减少屏幕
	case WM_DISPLAYCHANGE:

		RefreshSCWinSize(); //刷新水印位置和大小
		//不使用 break，继续执行 Paint 处理

	case WM_PAINT:
		hdc = ::BeginPaint(m_hWnd, &ps);
		OnPaint(hdc);
		EndPaint(m_hWnd, &ps);
		BringWindowToTop(m_hWnd);
		return TRUE;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(m_hWnd, message, wParam, lParam);
	}

	return DefWindowProc(m_hWnd, message, wParam, lParam);
}



void CMScreenInfoCtrl::OnPaintNew(HDC hdc) {

}



void CMScreenInfoCtrl::OnPaint(HDC hdc)
{
	HDC          hdcMemory = CreateCompatibleDC(hdc);
	HBITMAP      hBitMap = CreateCompatibleBitmap(hdc, m_nWidth, m_nHeight);
	GraphicsPath path;

	SelectObject(hdcMemory, hBitMap);
	//
	// Graphics 定义不能放在SelectObject函数之前，否则无法显示水印
	//
	Graphics     graphics(hdcMemory);

	graphics.SetSmoothingMode(SmoothingModeAntiAlias);
	graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

	vector<Point> vecPt;
	GenerateVecPt(vecPt, m_wstrText.c_str());

	FontFamily fontFamily(m_FontName.c_str());

	for (vector<Point>::iterator iterVec = vecPt.begin(); vecPt.end() != iterVec; ++iterVec)
	{
		path.AddString(m_wstrText.c_str(), m_wstrText.length(), &fontFamily,
			FontStyleRegular, m_fontsize, *iterVec, &m_format);
	}

	// 设置透明度
	int iWaterDepth = 100;

	Color screenColor;
	if (0 == _wcsicmp(m_wsColor.c_str(), L"red"))
	{//红色
		screenColor = Color(iWaterDepth, 255, 0, 0);
	}
	else if (0 == _wcsicmp(m_wsColor.c_str(), L"green"))
	{//绿色
		screenColor = Color(iWaterDepth, 0, 255, 0);
	}
	else if (0 == _wcsicmp(m_wsColor.c_str(), L"blue"))
	{//蓝色
		screenColor = Color(iWaterDepth, 0, 0, 255);
	}
	else if (0 == _wcsicmp(m_wsColor.c_str(), L"whiltegray"))
	{//浅灰色
		screenColor = Color(iWaterDepth, 212, 212, 212);
	}
	else if (0 == _wcsicmp(m_wsColor.c_str(), L"black"))
	{//浅灰色
		screenColor = Color(iWaterDepth, 255, 255, 255);
	}
	else
	{//黑色
		screenColor = Color(iWaterDepth, 255, 255, 255);
	}

	Pen pen(screenColor, 3);
	graphics.DrawPath(&pen, &path);

	LinearGradientBrush linGrBrush(
		Point(0, 0), Point(0, 90),
		screenColor,
		screenColor);
	graphics.FillPath(&linGrBrush, &path);

	//窗口扩展属性，设置窗口透明
	LONG lWindowLong = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
	if ((lWindowLong & WS_EX_LAYERED) != WS_EX_LAYERED)
	{
		lWindowLong = lWindowLong | WS_EX_LAYERED;
		::SetWindowLong(m_hWnd, GWL_EXSTYLE, lWindowLong);
	}

	POINT ptSrc = { 0, 0 };
	POINT ptWinPos = { m_nLeft, m_nTop };
	HDC   hdcScreen = ::GetDC(m_hWnd);
	SIZE  sizeWindow = { m_nWidth, m_nHeight };
	BOOL  bRet = UpdateLayeredWindow(m_hWnd, hdcScreen, &ptWinPos,
		&sizeWindow, hdcMemory, &ptSrc,
		0, &m_Blend, ULW_ALPHA);

	::SetWindowPos(m_hWnd, HWND_TOPMOST, m_nLeft, m_nTop, m_nWidth, m_nHeight,
		SWP_SHOWWINDOW);

	::ReleaseDC(m_hWnd, hdcScreen);
	::ReleaseDC(m_hWnd, hdc);

	hdcScreen = NULL;
	hdc = NULL;

	DeleteObject(hBitMap);
	DeleteDC(hdcMemory);
	hdcMemory = NULL;
}

size_t CMScreenInfoCtrl::GenerateVecPt(vector<Point>& vecPt, const wchar_t* lpwstr)
{
	Point        pt;
	RectF        boundRect;
	GraphicsPath pathTemp;
	FontFamily   fontFamily(m_FontName.c_str());
	pathTemp.AddString(lpwstr, -1, &fontFamily,
		FontStyleRegular, m_fontsize, pt, &m_format);
	pathTemp.GetBounds(&boundRect);
	const INT    offset = 20;

	int num = 0;        //屏幕索引，主屏幕为0
	Point rpt(0, 0);     //当前屏幕左上角与水印屏幕左上角的相对位置
	BOOL flag = TRUE;   //显示器有效标志
	BOOL rflag = TRUE;  //真显示器有效标志
	DISPLAY_DEVICE dd;  //显示器属性
	DEVMODE devMode;    //显示器配置
	//初始化变量
	ZeroMemory(&devMode, sizeof(devMode));
	devMode.dmSize = sizeof(devMode);
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);

	//循环遍历多个屏幕，计算水印窗口所需大小------------------------------------------
	do
	{
		flag = EnumDisplayDevices(NULL, num, &dd, 0);
		rflag = flag && EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &devMode);

		if (rflag)
		{
			//屏幕中间
			rpt.X = devMode.dmPosition.x - m_nLeft;
			rpt.Y = devMode.dmPosition.y - m_nTop;

			pt.X = rpt.X + INT(devMode.dmPelsWidth - boundRect.Width - offset * 2) / 2;
			pt.Y = rpt.Y + (devMode.dmPelsHeight - boundRect.Height - offset * 2) / 2;
			vecPt.push_back(pt);

		}

		num++; //增加屏幕索引

	} while (flag);
	//----------------------------------------------------------------------------------------

	return vecPt.size();
}

void CMScreenInfoCtrl::RefreshSCWinSize()
{
	int num = 0;
	int iTop = 0;
	int iBottom = 0;
	int iLeft = 0;
	int iRight = 0;
	BOOL flag = TRUE;
	BOOL rflag = TRUE;
	DISPLAY_DEVICE dd;
	DEVMODE devMode;

	//初始化变量
	ZeroMemory(&devMode, sizeof(devMode));
	devMode.dmSize = sizeof(devMode);
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);

	//循环遍历多个屏幕，计算水印窗口所需大小
	do
	{
		flag = EnumDisplayDevicesW(NULL, num, &dd, 0);
		rflag = flag && EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &devMode);

		if (rflag)
		{
			if (iTop > devMode.dmPosition.y)
			{
				iTop = devMode.dmPosition.y;
			}
			if (iLeft > devMode.dmPosition.x)
			{
				iLeft = devMode.dmPosition.x;
			}
			if (iBottom < devMode.dmPosition.y + devMode.dmPelsHeight)
			{
				iBottom = devMode.dmPosition.y + devMode.dmPelsHeight;
			}
			if (iRight < devMode.dmPosition.x + devMode.dmPelsWidth)
			{
				iRight = devMode.dmPosition.x + devMode.dmPelsWidth;
			}
		}

		num++;

	} while (flag);

	//设置成员变量
	m_nLeft = iLeft;
	m_nTop = iTop;
	m_nWidth = iRight - iLeft;
	m_nHeight = iBottom - iTop;

	return;
}



void __stdcall windowThead() {
	Sleep(1000);
	CMScreenInfoCtrl* ctrl = new CMScreenInfoCtrl();
	//::SetThreadDesktop(NULL);
	ctrl->CreateSCWindow();

	//The message loop    
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


int __stdcall WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	//m_s_hInstance = hPrevInstance;

	HANDLE h = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)windowThead, 0, 0, 0);
	Sleep(-1);
	return 0;
}
