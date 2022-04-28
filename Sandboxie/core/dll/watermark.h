#pragma once

#include <windows.h>

int initWatermark(HWND hwnd);

int watermark(HWND hWnd, int left, int top, int update_width, int update_height);

int restoreBackground(HWND hwnd);

int keepBackground(HWND hWnd,int left,int top,int width,int height);

int GetUpdateRectFunction(HWND hwnd, RECT* lprect,BOOL bErase);

int GetWindowLongFunction(HWND hwnd, int style);

int GetClientRectFunction(HWND hwnd, RECT* lprect);

int InvalidRectFunction(HWND hwnd);

int redrawWindowFunction(HWND hwnd);

