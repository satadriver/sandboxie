/*
 * Copyright 2004-2020 Sandboxie Holdings, LLC 
 * Copyright 2020-2021 David Xanatos, xanasoft.com
 *
 * This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Version Information

#ifndef _MY_VERSION_H
#define _MY_VERSION_H

//#include "../apps/publicFun/mydll.h"

#ifdef _WIN64
#define _BUILDARCH 				x64
#elif defined _WIN32
#define _BUILDARCH 				Win32
#else
#define _BUILDARCH 				x64
#endif

#define MY_VERSION_BINARY       1,0,0
#define MY_VERSION_STRING       "1.0.0"
#define MY_VERSION_COMPAT		"1.0.0" // this refers to the driver ABI compatibility

// These #defines are used by either Resource Compiler, or by NSIC installer
#define SBIE_INSTALLER_PATH		"..\\Bin\\"
#define SBIE_INSTALLER_PATH_32  "..\\Bin\\Win32\\SafeDesktopInstall32.exe"
#define SBIE_INSTALLER_PATH_64  "..\\Bin\\x64\\SafeDesktopInstall64.exe"

#define MY_PRODUCT_NAME_STRING  "SafeDesktop"
#define MY_COMPANY_NAME_STRING  "sowings.com"
#define MY_COPYRIGHT_STRING     "Copyright (C) 2020-2022 Sowing LLC. All Rights Reserved."
#define MY_COPYRIGHT_STRING_OLD "Copyright 2004-2020 by Sandboxie Holdings, LLC"

#define SANDBOXIE               L"SafeDesktop"
#define SBIE                    L"sfDesk"

#define SANDBOXIE_USER			L"SafeDesktop"
#define SANDBOXIE_DESKTOPEXE	L"SafeDesktop.exe"
#define DEFAULT_BOX_NAME		L"DefaultBox"

#define SBIE_BOXED_             SBIE L"_BOXED_"
#define SBIE_BOXED_LEN          (6 + 7)

#define SANDBOXIE_INI           L"SafeDesktop.ini"

#define SBIEDRV                 L"sfDeskDrv"
#define SBIEDRV_SYS             L"sfDeskDrv.sys"

#define SBIESVC                 L"sfDeskSvc"
#define SBIESVC_EXE             L"sfDeskSvc.exe"

#define SANDBOXIE_CONTROL       L"SafeDesktopControl"
#define SBIECTRL_EXE            L"SfDeskCtrl.exe"
#define SBIECTRL_               L"SfDeskCtrl_"

#define START_EXE               L"Start.exe"

// see also environment variable in session.bat
#define SBIEDLL                 L"SfDeskDll"


#define CYGUARDDLL                 L"cyguard"

#define SBIEMSG_DLL             L"SfDeskMsg.dll"
#define SBIE_IN_MSGS            L"SfDesk"

#define SBIEINI                 L"SfDeskIni"
#define SBIEINI_EXE             L"SfDeskIni.exe"

#define SANDBOX_VERB            L"SafeDesktop"

//#define EXPLORER_PROCESS_NAME_PLUSPLUS		L"explorer++.exe"

#define EXPLORER_PROCESS_NAME		L"explorer.exe"

#define EXPLORER_PROCESS_NAME_PLUSPLUS			L"sfDeskExplorer.exe"

#define MY_AUTOPLAY_CLSID_STR   "7E950284-E123-49F4-B32B-A806C090D747"
#define MY_AUTOPLAY_CLSID       0x7E950284, 0xE123, 0x49F4, \
                                { 0xB3, 0x2B, 0xA8,0x06, 0xC0, 0x90, 0xD7, 0x47 }

#define SBIECTRL_LOGO_IMAGE     "../res/MastheadLogo.jpg"

#define TITLE_SUFFIX_W          L"    "
#define TITLE_SUFFIX_A           "    "
// #define TITLE_SUFFIX_W          L" [SDWS]"
// #define TITLE_SUFFIX_A           " [SDWS]"

// extern wchar_t* TITLE_SUFFIX_W[32];
// extern wchar_t* TITLE_SUFFIX_A[16];

#define FILTER_ALTITUDE         L"86900"

#define OPTIONAL_VALUE(x,y)     VALUE x, y

#define VERACRYPT_DISK_VOLUME	L"X:\\"

#define VERACRYPT_FILENAME		L"SafeDesktop.hc"

//#define VERACRYPT_PASSWORD		L"SAFEDESKTOP_PASSWORD"



#endif	// _MY_VERSION_H
