#pragma once

#include <windows.h>




int __cdecl log(const WCHAR * format,...);

extern "C" __declspec(dllexport) int __stdcall hookfunctions();

extern "C" __declspec(dllexport) void unhookfunctions();

