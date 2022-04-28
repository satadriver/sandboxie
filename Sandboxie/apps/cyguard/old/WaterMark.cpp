#include "WaterMark.h"
#include "log.h"
#include "functions.h"
#include "common.h"
#include "apiFuns.h"

#include <map>
#include <windowsx.h>

extern CreateWindowExWStub CreateWindowExWOld;

GdiplusStartupInput g_gdiSI;
ULONG_PTR g_gdiToken;

std::map<HWND, HWND> g_WaterMarkHwndMap = {};				// <窗口句柄,水印窗口句柄>
std::map<HWND, HWND> g_LucencyBorderHwndMap = {};			// <窗口句柄,水印窗口句柄>
std::map<HWND, WNDPROC> g_PreWndProcMap = {};				// <窗口句柄,替换前的窗口消息回调函数>

struct WaterMarkThreadInfo {
	HWND hwnd;			// 水印窗口宿主窗口句柄
	INT  X;				// 窗口初始 x 坐标
	INT  Y;				// 窗口初始 y 坐标
	INT  nWidth;		// 窗口宽度
	INT  nHeight;		// 窗口高度
};

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;
	UINT  size = 0;

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}

	free(pImageCodecInfo);
	return -1;
}

BOOL CreateWaterMarkPngFile()
{
	// 设置水印内容
	WCHAR UserName[MAX_PATH] = { 0 };
	INT StatusGetUserName = LjgApi_getUsername(UserName);
	if (0 != StatusGetUserName)
	{
		DP1("[LYSM][CreateWaterMarkPngFile] LjgApi_getUsername failed , status = %d \n", StatusGetUserName);
		return FALSE;
	}
	WCHAR WaterMarkStr[MAX_PATH] = { 0 };
	time_t now = time(0);
	tm* ltm = localtime(&now);
	wsprintfW(WaterMarkStr, L"%d/%d/%d %s",
		1900 + ltm->tm_year,
		1 + ltm->tm_mon,
		ltm->tm_mday,
		UserName);
	DP1("[LYSM][CreateWaterMarkPngFile] WaterMarkStr: %s \n", WaterMarkStr);

	// 初始化图形对象
	Bitmap bmp(
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		PixelFormat32bppARGB);
	Graphics* g = Graphics::FromImage(&bmp);

	// 设置水印倾斜
	g->TranslateTransform(
		-(GetSystemMetrics(SM_CXSCREEN) / 2),
		GetSystemMetrics(SM_CYSCREEN) / 2);
	g->RotateTransform(-45);

	// 设置透明背景
	g->Clear(Color::Transparent);

	// 设置水印文字样式
	Gdiplus::FontFamily fontFamily(L"Arial");
	FontStyle fontstyle = FontStyleRegular;
	Gdiplus::SolidBrush solidBrush(Color(20, 0, 0, 0));
	GraphicsPath txtPath(FillModeWinding);
	StringFormat  cStringFormat;

	// 填充图片
	Status status;
	INT IntervalLen = 200;
	INT IntercalHeight = 200;
	INT PieceLen = 200;
	INT PieceHeight = 200;
	INT maxLen = GetSystemMetrics(SM_CXSCREEN) + GetSystemMetrics(SM_CYSCREEN);
	for (INT i = 0; i * IntervalLen < maxLen; ++i)
	{
		for (INT j = 0; j * IntercalHeight < maxLen; ++j)
		{
			status = txtPath.AddString(
				WaterMarkStr,
				-1,
				&fontFamily,
				fontstyle,
				20,
				RectF(i * IntervalLen, j * IntercalHeight, 0, 0),
				&cStringFormat);

		}
	}
	status = g->FillPath(&solidBrush, &txtPath);

	// 保存图片
	CLSID pngClsid;
	WCHAR pngPath[MAX_PATH] = { 0 };
	if (0 == GetTempPath(MAX_PATH - 1, pngPath))
	{
		DP1("[LYSM][CreateWaterMarkPngFile] GetTempPath error : %d \n", GetLastError());
		return FALSE;
	}
	if (0 != wcscat_s(pngPath, WATERMARK_PNG_NAME))
	{
		DP1("[LYSM][CreateWaterMarkPngFile] wcscat_s error : %d \n", GetLastError());
		return FALSE;
	}
	if (-1 == GetEncoderClsid(L"image/png", &pngClsid))
	{
		DP0("[LYSM][CreateWaterMarkPngFile] GetTempPath failed \n");
		return FALSE;
	}
	status = bmp.Save(pngPath, &pngClsid, NULL);

	return TRUE;
}

LRESULT CALLBACK WaterMarkCallProc(
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		ExitThread(0);
		break;
	default:
		break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL CreateWaterMarkWindow(
	HWND	  hwndHost,
	int       X,
	int       Y,
	int       nWidth,
	int       nHeight)
{
	BOOL ret = TRUE;
	WNDCLASSEX wincl;
	HWND hwnd = NULL;
	LONG style;
	RECT wndRect;
	SIZE wndSize;
	HDC hdc = NULL;
	HDC memDC = NULL;
	HBITMAP memBitmap = NULL;
	HDC screenDC = NULL;
	POINT ptSrc = { 0,0 };
	BLENDFUNCTION blendFunction;
	MSG msg = { 0 };
	WCHAR pngPath[MAX_PATH] = { 0 };
	WCHAR classNameStub[8] = { 0 };
	WCHAR className[32] = L"cy_watermark_class_";
	HANDLE hPngFile = NULL;
	Gdiplus::Image* pImage = NULL;
	Gdiplus::Graphics* pGraphics = NULL;
	WINDOWINFO wndinfo;
	INT border = 0;

	// 随机窗口类名
	srand(time(NULL));
	rand_strW(classNameStub, 6);
	if (0 != wcscat_s(className, classNameStub))
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] wcscat_s error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}

	// 注册窗口类
	wincl.hInstance = GetModuleHandle(0);
	wincl.lpszClassName = className;
	wincl.lpfnWndProc = WaterMarkCallProc;
	wincl.style = CS_HREDRAW | CS_VREDRAW;
	wincl.cbSize = sizeof(WNDCLASSEX);
	wincl.hIcon = NULL;
	wincl.hIconSm = NULL;
	wincl.hCursor = NULL;
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = CreateSolidBrush(Color::Transparent);
	if (!RegisterClassEx(&wincl))
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] RegisterClassEx error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}

	// 创建窗口
	hwnd = CreateWindowExW/*Old*/(
		WS_EX_LAYERED | WS_EX_TOOLWINDOW,
		className,
		WATERMARK_WINDOW_TITLE,
		WS_OVERLAPPEDWINDOW,
		X,
		Y,
		nWidth,
		nHeight,
		hwndHost,
		0,
		GetModuleHandle(0),
		NULL
	);
	if (!hwnd)
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] CreateWindowExW error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}

	// 记录宿主窗口和水印窗口的对应关系
	g_WaterMarkHwndMap.insert({ hwndHost ,hwnd });

	// 修改水印窗口属性
	style = ::GetWindowLong(hwnd, GWL_STYLE);
	if (style & WS_CAPTION)
		style ^= WS_CAPTION;
	if (style & WS_THICKFRAME)
		style ^= WS_THICKFRAME;
	if (style & WS_SYSMENU)
		style ^= WS_SYSMENU;
	SetWindowLong(hwnd, GWL_STYLE, style);
	style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
	if (style & WS_EX_APPWINDOW)
		style ^= WS_EX_APPWINDOW;
	SetWindowLong(hwnd, GWL_EXSTYLE, style);

	// 加载 png 水印图片
	if (!GetClientRect(hwnd, &wndRect))
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] GetClientRect error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	wndSize = {
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN)
	};
	hdc = GetDC(hwnd);
	if (!hdc)
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] GetDC error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	memDC = CreateCompatibleDC(hdc);
	if (!memDC)
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] CreateCompatibleDC error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	memBitmap = CreateCompatibleBitmap(hdc, wndSize.cx, wndSize.cy);
	if (!memBitmap)
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] CreateCompatibleBitmap error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	if (NULL == SelectObject(memDC, memBitmap))
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] SelectObject error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	if (0 == GetTempPath(MAX_PATH - 1, pngPath))
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] GetTempPath error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	if (0 != wcscat_s(pngPath, WATERMARK_PNG_NAME))
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] wcscat_s error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	pImage = new Gdiplus::Image(pngPath);
	pGraphics = new Gdiplus::Graphics(memDC);
	pGraphics->DrawImage(
		pImage,
		RectF(0, 0, wndSize.cx, wndSize.cy),
		0,
		0,
		wndSize.cx,
		wndSize.cy,
		UnitPixel);
	delete pImage;
	delete pGraphics;

	// 水印透明
	screenDC = GetDC(NULL);
	if (!screenDC)
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] GetDC error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	blendFunction.AlphaFormat = AC_SRC_ALPHA;
	blendFunction.BlendFlags = 0;
	blendFunction.BlendOp = AC_SRC_OVER;
	blendFunction.SourceConstantAlpha = 255;
	if (!UpdateLayeredWindow(
		hwnd,
		screenDC,
		&ptSrc,
		&wndSize,
		memDC,
		&ptSrc,
		0,
		&blendFunction,
		ULW_ALPHA))
	{
		DP1("[LYSM][CyCreateWaterMarkWindow] UpdateLayeredWindow error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}

	// 调整窗口大小后显示窗口
	wndinfo.cbSize = sizeof(WINDOWINFO);
	if (0 == GetWindowInfo(hwndHost, &wndinfo))
	{
		DP1("[LYSM][CreateLucencyBorderWindow] GetWindowInfo error:%d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	border = wndinfo.rcClient.left - wndinfo.rcWindow.left;
	MoveWindow(hwnd, X + border, Y + border, nWidth - border * 2, nHeight - border * 2, TRUE);
	ShowWindow(hwnd, SW_SHOW);

	// 消息循环
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&msg);
		}
	}

end:
	if (hdc)
		ReleaseDC(hwnd, hdc);
	if (memDC)
		DeleteDC(memDC);
	if (memBitmap)
		DeleteObject(DeleteObject);
	if (screenDC)
		ReleaseDC(hwnd, screenDC);

	return ret;
}

BOOL CreateLucencyBorderWindow(
	HWND	  hwndHost,
	int       X,
	int       Y,
	int       nWidth,
	int       nHeight)
{
	BOOL ret = TRUE;
	WNDCLASSEX wc;
	HWND hWnd;
	MSG msg = { 0 };
	WCHAR classNameStub[8] = { 0 };
	WCHAR className[32] = L"cy_border_class_";
	Pen BorderPen(LUCENCY_BORDER_COLOR, LUCENCY_BORDER_WIDTH);
	INT border = 0;
	WINDOWINFO wndinfo;
	Gdiplus::Graphics* pGraphics = NULL;

	// 随机窗口类名
	srand(time(NULL));
	rand_strW(classNameStub, 6);
	if (0 != wcscat_s(className, classNameStub))
	{
		DP1("[LYSM][CreateLucencyBorderWindow] wcscat_s error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}

	// 注册窗口类
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(0);
	wc.hIcon = NULL;
	wc.hIconSm = NULL;
	wc.hbrBackground = CreateSolidBrush(Color::Black);
	wc.hCursor = NULL;
	wc.lpfnWndProc = WaterMarkCallProc;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = className;
	if (!RegisterClassEx(&wc))
	{
		DP1("[LYSM][CreateLucencyBorderWindow] RegisterClassEx error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}

	// 创建窗口
	hWnd = CreateWindowEx(
		WS_EX_LAYERED,
		className,
		LUCENCY_BORDER_WINDOW_TITLE,
		WS_POPUP,
		X,
		Y,
		nWidth,
		nHeight,
		hwndHost,
		0,
		GetModuleHandle(0),
		0
	);
	if (hWnd == 0)
	{
		DP1("[LYSM][CreateLucencyBorderWindow] CreateWindowEx error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}

	// 记录宿主窗口和描边窗口的对应关系
	g_LucencyBorderHwndMap.insert({ hwndHost ,hWnd });

	// 指定颜色透明
	if (!SetLayeredWindowAttributes(hWnd, Color::Black, 0, LWA_COLORKEY))
	{
		DP1("[LYSM][CreateLucencyBorderWindow] SetLayeredWindowAttributes error : %d \n", GetLastError());
		ret = FALSE;
		goto end;
	}

	// 显示窗口
	ShowWindow(hWnd, SW_SHOW);

	// 先绘制一次，防止窗口没有触发 WM_PAINT 事件
	wndinfo.cbSize = sizeof(WINDOWINFO);
	if (0 == GetWindowInfo(hwndHost, &wndinfo))
	{
		DP1("[LYSM][CreateLucencyBorderWindow] GetWindowInfo error:%d \n", GetLastError());
		ret = FALSE;
		goto end;
	}
	pGraphics = new Gdiplus::Graphics(GetDC(hWnd));
	border = wndinfo.rcClient.left - wndinfo.rcWindow.left;
	pGraphics->DrawRectangle(
		&BorderPen,
		Rect(
			border,
			0,
			wndinfo.rcWindow.right - wndinfo.rcWindow.left - border * 2,
			wndinfo.rcWindow.bottom - wndinfo.rcWindow.top - border
		)
	);
	delete pGraphics;

	// 消息循环
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&msg);
		}
	}

end:

	return ret;
}

DWORD WINAPI CreateWaterMarkWindowByThread(PVOID pParam)
{
	WaterMarkThreadInfo* pInfo = (WaterMarkThreadInfo*)pParam;

	DP4("[LYSM][CreateWaterMarkWindowThread] params:%d %d %d %d \n",
		pInfo->X,
		pInfo->Y,
		pInfo->nWidth,
		pInfo->nHeight);

	(VOID)CreateWaterMarkWindow(pInfo->hwnd, pInfo->X, pInfo->Y, pInfo->nWidth, pInfo->nHeight);

	return 0;
}

DWORD WINAPI CreateLucencyBorderWindowByThread(PVOID pParam)
{
	WaterMarkThreadInfo* pInfo = (WaterMarkThreadInfo*)pParam;

	DP4("[LYSM][CreateLucencyBorderWindowByThread] params:%d %d %d %d \n",
		pInfo->X,
		pInfo->Y,
		pInfo->nWidth,
		pInfo->nHeight);

	(VOID)CreateLucencyBorderWindow(pInfo->hwnd, pInfo->X, pInfo->Y, pInfo->nWidth, pInfo->nHeight);

	return 0;
}

LRESULT CALLBACK NewWndProc2(
	HWND    hWnd,
	UINT    Msg,
	WPARAM  wParam,
	LPARAM  lParam
)
{
	WNDPROC lpPrevWndFunc = g_PreWndProcMap.find(hWnd)->second;
	HWND HwndWaterMark = g_WaterMarkHwndMap.find(hWnd)->second;
	HWND HwndLcencyBorder = g_LucencyBorderHwndMap.find(hWnd)->second;

	//DP3("[LYSM][NewWndProc2] lpPrevWndFunc:%p HwndWaterMark:%p HwndLcencyBorder:%p \n", lpPrevWndFunc, HwndWaterMark, HwndLcencyBorder);

	switch (Msg)
	{
	case WM_PAINT:
	{
		//DP0("[LYSM] WM_PAINT \n");

		// 绘制窗口描边
		if (!HwndLcencyBorder)
		{
			DP0("[LYSM][NewWndProc2] HwndLcencyBorder is null , break! \n");
			break;
		}
		WINDOWINFO wndInfo;
		wndInfo.cbSize = sizeof(WINDOWINFO);
		if (0 == GetWindowInfo(hWnd, &wndInfo))
		{
			DP1("[LYSM][NewWndProc2] GetWindowInfo error:%d \n", GetLastError());
			break;
		}
		Pen BorderPen(LUCENCY_BORDER_COLOR, LUCENCY_BORDER_WIDTH);
		Gdiplus::Graphics graphics(GetDC(HwndLcencyBorder));
		INT border = wndInfo.rcClient.left - wndInfo.rcWindow.left;

		graphics.DrawRectangle(
			&BorderPen,
			Rect(
				border,
				0,
				wndInfo.rcWindow.right - wndInfo.rcWindow.left - border * 2,
				wndInfo.rcWindow.bottom - wndInfo.rcWindow.top - border
			));

		break;
	}
	case WM_SHOWWINDOW:
	{
		DP1("[LYSM][NewWndProc2] WM_SHOWWINDOW:%d \n", (INT)wParam);

		// 同步水印的隐藏和显示
		if (wParam == 0)
		{
			ShowWindow(HwndWaterMark, SW_HIDE);
			ShowWindow(HwndLcencyBorder, SW_HIDE);
		}
		else if (wParam == 1)
		{
			ShowWindow(HwndWaterMark, SW_SHOW);
			ShowWindow(HwndLcencyBorder, SW_SHOW);
		}
		else
		{
			break;
		}

		break;
	}
	case WM_WINDOWPOSCHANGED:
	{
		PWINDOWPOS pInfo = (PWINDOWPOS)lParam;
		if (!pInfo)
		{
			break;
		}

		DP4("[LYSM][NewWndProc2] WM_WINDOWPOSCHANGED xy: %d %d %d %d \n", pInfo->x, pInfo->y, pInfo->cx, pInfo->cy);

		// 处理特殊的窗口隐藏方式
		if (pInfo->flags & SWP_HIDEWINDOW)
		{
			ShowWindow(HwndWaterMark, SW_HIDE);
			ShowWindow(HwndLcencyBorder, SW_HIDE);
		}

		// 获取窗口信息
		WINDOWINFO wndInfo;
		wndInfo.cbSize = sizeof(WINDOWINFO);
		if (0 == GetWindowInfo(hWnd, &wndInfo))
		{
			DP1("[LYSM][NewWndProc2] GetWindowInfo error:%d \n", GetLastError());
			break;
		}
		INT border = wndInfo.rcClient.left - wndInfo.rcWindow.left;

		// 移动水印
		if (!MoveWindow(
			HwndWaterMark,
			pInfo->x + border,
			pInfo->y + border,
			pInfo->cx - border * 2,
			pInfo->cy - border * 2,
			TRUE))
		{
			DP1("[LYSM][NewWndProc2] MoveWindow error : %d \n", GetLastError());
			break;
		}

		// 移动描边
		if (!MoveWindow(
			HwndLcencyBorder,
			pInfo->x,
			pInfo->y,
			pInfo->cx,
			pInfo->cy,
			TRUE))
		{
			DP1("[LYSM][NewWndProc2] MoveWindow error : %d \n", GetLastError());
			break;
		}

		break;
	}
	case WM_DESTROY:
	{
		DP1("[LYSM][NewWndProc2] WM_DESTROY:%p \n", hWnd);

		// 销毁水印窗口
		PostMessage(HwndWaterMark, WM_DESTROY, NULL, NULL);
		PostMessage(HwndLcencyBorder, WM_DESTROY, NULL, NULL);

		// 从 map 中删除数据
		map<HWND, HWND>::iterator iterWaterMark = g_WaterMarkHwndMap.begin();
		for (; iterWaterMark != g_WaterMarkHwndMap.end();)
		{
			if (iterWaterMark->second == hWnd)
			{
				g_WaterMarkHwndMap.erase(iterWaterMark++);
			}
			else
			{
				iterWaterMark++;
			}
		}
		map<HWND, HWND>::iterator iterLucencyBorder = g_WaterMarkHwndMap.begin();
		for (; iterLucencyBorder != g_WaterMarkHwndMap.end();)
		{
			if (iterLucencyBorder->second == hWnd)
			{
				g_WaterMarkHwndMap.erase(iterLucencyBorder++);
			}
			else
			{
				iterLucencyBorder++;
			}
		}

		break;
	}
	default:
		//DP1("[LYSM] msg:%x \n", Msg);
		break;
	}

	return CallWindowProc(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

typedef struct EnumHWndsArg
{
	DWORD dwProcessId;
}EnumHWndsArg, * LPEnumHWndsArg;
BOOL CALLBACK lpEnumFunc(HWND hwnd, LPARAM lParam)
{
	EnumHWndsArg* pArg = (LPEnumHWndsArg)lParam;
	DWORD  ProcessId;

	DWORD ThreadId = GetWindowThreadProcessId(hwnd, &ProcessId);

	if (ProcessId == pArg->dwProcessId)
	{
		// 判断是否存在对应的水印窗口
		if (g_WaterMarkHwndMap.find(hwnd) == g_WaterMarkHwndMap.end())
		{
			do
			{
				// 白名单放通
				WCHAR WndClass[MAX_PATH] = { 0 };
				WCHAR WndTitle[MAX_PATH] = { 0 };
				LONG WndStyle = GetWindowLong(hwnd, GWL_STYLE);
				LONG WndExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
				GetClassName(hwnd, WndClass, MAX_PATH - 1);
				GetWindowText(hwnd, WndTitle, MAX_PATH - 1);
				if (IsWindowVisible(hwnd) &&
					wcsstr(WndTitle, L"QQ") != 0 &&
					wcsstr(WndClass, L"TXGuiFoundation") != 0 &&
					WndStyle & WS_THICKFRAME)
				{
					// QQ 主窗口（不包括登录窗口）
					break;
				}

				// 非主窗口不处理
				if (GetWindow(hwnd, GW_OWNER) != 0 ||
					!IsWindowVisible(hwnd))
				{
					return TRUE;
				}

				// 工具栏窗口不处理
				if (WndExStyle & WS_EX_TOOLWINDOW)
				{
					return TRUE;
				}

			} while (0);


			DP3("[LYSM][lpEnumFunc] (%d-%d)add watermark : %p \n", ProcessId, ThreadId, hwnd);

			// 创建水印窗口、描边窗口
			RECT rect;
			GetWindowRect(hwnd, &rect);
			WaterMarkThreadInfo* pInfo = new WaterMarkThreadInfo();
			pInfo->hwnd = hwnd;
			pInfo->X = rect.left;
			pInfo->Y = rect.top;
			pInfo->nWidth = rect.right - rect.left;
			pInfo->nHeight = rect.bottom - rect.top;

			HANDLE hThreadWaterMark = CreateThread(0, 0, CreateWaterMarkWindowByThread, pInfo, 0, 0);
			if (hThreadWaterMark)
			{
				CloseHandle(hThreadWaterMark);
			}
			else
			{
				DP1("[LYSM][lpEnumFunc] WaterMark CreateThread error:%d \n", GetLastError());
				return TRUE;
			}
			HANDLE hThreadLucencyBorder = CreateThread(0, 0, CreateLucencyBorderWindowByThread, pInfo, 0, 0);
			if (hThreadLucencyBorder)
			{
				CloseHandle(hThreadLucencyBorder);
			}
			else
			{
				DP1("[LYSM][lpEnumFunc] LucencyBorder CreateThread error:%d \n", GetLastError());
				return TRUE;
			}

			// 等待 WaterMarkHwndMap、LucencyBorderHwndMap 保存完毕
			BOOL isMapValued = FALSE;
			for (INT i = 0; i < 100; ++i)
			{
				if (g_WaterMarkHwndMap.find(hwnd) != g_WaterMarkHwndMap.end() &&
					g_LucencyBorderHwndMap.find(hwnd) != g_LucencyBorderHwndMap.end()
					)
				{
					isMapValued = TRUE;
					break;
				}
				Sleep(10);
			}
			if (!isMapValued)
			{
				DP0("[LYSM][CreateWindowExWNew] WaterMarkHwndMap or LucencyBorderHwndMap not legal \n");
				return TRUE;
			}

			// 替换窗口消息处理函数
			LONG_PTR OldWndProc = GetWindowLongPtr(hwnd, GWLP_WNDPROC);
			if (!OldWndProc)
			{
				DP1("[LYSM][CreateWindowExWNew] error %d \n", GetLastError());
				return TRUE;
			}
			g_PreWndProcMap.insert({ hwnd,(WNDPROC)OldWndProc });
			if (!SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)NewWndProc2))
			{
				DP1("[LYSM][CreateWindowExWNew] SetWindowLongPtr error %d \n", GetLastError());
				return TRUE;
			}
		}
	}

	return TRUE;
}
DWORD WINAPI CreateProcWndMonitor(PVOID pParam)
{
	EnumHWndsArg wi;
	wi.dwProcessId = GetCurrentProcessId();

	while (TRUE)
	{
		EnumWindows(lpEnumFunc, (LPARAM)&wi);

		Sleep(1000);
	}

	return 0;
}


/* ------------------------------------------------------------------------- */

BOOL WaterMarkStart()
{
	// 判断策略是否开启
	INT enable = SbieApi_QueryWatermark();
	if (enable == FALSE)
	{
		DP1("[LYSM][WaterMarkStartUp] SbieApi_QueryWatermark failed:%d\r\n", enable);
		return FALSE;
	}

	// 初始化 gdi+
	Gdiplus::Status status = Gdiplus::Status::Ok;
	status = GdiplusStartup(&g_gdiToken, &g_gdiSI, NULL);
	if (status != Gdiplus::Status::Ok)
	{
		DP1("[LYSM][WaterMarkStartUp] GdiplusStartup failed status:%d \n", status);
		return FALSE;
	}

	// 生成水印文件
	if (!CreateWaterMarkPngFile())
	{
		DP0("[LYSM][WaterMarkStartUp] CreateWaterMarkPngFile failed \n", status);
		return FALSE;
	}

	// 创建窗口监控线程
	HANDLE hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CreateProcWndMonitor, 0, 0, 0);
	if (hThread)
	{
		CloseHandle(hThread);
	}
	else
	{
		DP0("[LYSM][WaterMarkStartUp] CreateThread failed \n", status);
		return FALSE;
	}

	return TRUE;
}

BOOL WaterMarkStop()
{
	// 判断策略是否开启
	INT enable = SbieApi_QueryWatermark();
	if (enable == FALSE)
	{
		DP1("[LYSM][WaterMarkStop] SbieApi_QueryWatermark failed:%d\r\n", enable);
		return FALSE;
	}

	// 反初始化 gdi+
	GdiplusShutdown(g_gdiToken);

	return TRUE;
}
