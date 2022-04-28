#pragma once
#include <Windows.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#define WATERMARK_PNG_NAME L"cy_watermark.png"
#define WATERMARK_WINDOW_TITLE L"cy_watermark_tital"
#define LUCENCY_BORDER_WINDOW_TITLE L"cy_border_tital"

#define LUCENCY_BORDER_COLOR Color(255, 100, 255, 100)
#define LUCENCY_BORDER_WIDTH 5

using namespace Gdiplus;


/* 启动水印
*/
BOOL WaterMarkStart();

/* 停止水印
*/
BOOL WaterMarkStop();

/* 在 Hook ShowWindow 中添加水印
*/
BOOL CreateWaterMarkAndBorderByHook(HWND hwnd);

