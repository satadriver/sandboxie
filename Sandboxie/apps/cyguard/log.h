#pragma once
#include <Windows.h>    
#include <tchar.h>    

#define DP0(fmt) {TCHAR sOut[256];_stprintf_s(sOut,_T(fmt));OutputDebugString(sOut);}    
#define DP1(fmt,var) {TCHAR sOut[256];_stprintf_s(sOut,_T(fmt),var);OutputDebugString(sOut);}    
#define DP2(fmt,var1,var2) {TCHAR sOut[256];_stprintf_s(sOut,_T(fmt),var1,var2);OutputDebugString(sOut);}    
#define DP3(fmt,var1,var2,var3) {TCHAR sOut[256];_stprintf_s(sOut,_T(fmt),var1,var2,var3);OutputDebugString(sOut);}
#define DP4(fmt,var1,var2,var3,var4) {TCHAR sOut[256];_stprintf_s(sOut,_T(fmt),var1,var2,var3,var4);OutputDebugString(sOut);}   
