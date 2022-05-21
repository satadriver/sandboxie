;----------------------------------------------------------------------------
; Sandboxie Installer
;----------------------------------------------------------------------------

SetCompressor /SOLID /FINAL lzma

!include "MUI.nsh"
!include "IsWow64.nsh"
!include "Registry.nsh"
!include "Cmdline.nsh"
!include "IsAdmin.nsh"
!include "Library.nsh"
!include "WordFunc.nsh"

;----------------------------------------------------------------------------
; Extract Information from my_version.h
;----------------------------------------------------------------------------

!tempfile TMPVERFILE
!system 'ParseVersion.bat "..\common\my_version.h" "${TMPVERFILE}"'
!include "${TMPVERFILE}"
!delfile "${TMPVERFILE}"
!undef TMPVERFILE

!define TEST_CONFIG_FILE	sfdesktop.yml
!define QUICK_LAUNCH		"SafeWorkSpace"
!define PUBLICFUN_DLL		publicFun.dll
!define DESKTOP_APP			desktop.exe
!define UI_MAIN_APP			SafeDesktop.exe
!define SBOXHOSTDLL_DLL 	SboxHostDll.dll
!define KMDUTIL_EXE 		KmdUtil.exe
!define EXPLORERPP_EXE		sfDeskExplorer.exe
!define CYGUARD_DLL			cyguard.dll
!define PROCESSINJECTOR_EXE	processInjector.exe

!define VERACRYPT			"veracrypt"
!define VERACRYPT_FORMAT_EXE "VeraCrypt Format.exe"
!define WDF_INSPECT_SYS		"inspect.sys"
!define WDF_INSPECT			"inspect"

;----------------------------------------------------------------------------
; Global Defines
;----------------------------------------------------------------------------
; these are the build-time config settings.  Need to be cmd line args or something better.
; pick either 32 or 64 bit
;!define _BUILDARCH		Win32
!define _BUILDARCH		x64
;!define _BUILDARCH		"$%SBIE_BUILDARCH%"

!define DISPATCH_APP_MODE SbieRelease
;!define DISPATCH_APP_MODE SbieDebug

; uncomment this line if you want to make the special versions that download VC Redist
;!define INCLUDE_VCREDIST_DNLD

!define BIN_ROOT_BASE	"${SBIE_INSTALLER_PATH}"

;!if "${_BUILDARCH}" == "x64"
;    !define _W7DRV_COMPAT "$%SbieVer%.x64"
;!else
;    !define _W7DRV_COMPAT "$%SbieVer%.x86"
;!endif

;!define SBIEDRV_SYS4    "${SBIEDRV_SYS}.rc4"
;!define SBIEDRV_SYSX    "${SBIEDRV_SYS}.w10"

!define OUTFILE_BOTH    "${PRODUCT_NAME}Install.exe"
!define NAME_Win32      "${PRODUCT_FULL_NAME} ${VERSION} (32-bit)"
!ifdef INCLUDE_VCREDIST_DNLD
!define OUTFILE_Win32   "${PRODUCT_NAME}Install32_VCRedist.exe"
!else
!define OUTFILE_Win32   "${PRODUCT_NAME}Install32.exe"
!endif
!define ARCH_Win32      "Win32"
!define BIN_ROOT_Win32  "${BIN_ROOT_BASE}\Win32\${DISPATCH_APP_MODE}"

!define NAME_x64        "${PRODUCT_FULL_NAME} ${VERSION} (64-bit)"
!ifdef INCLUDE_VCREDIST_DNLD
!define OUTFILE_x64     "${PRODUCT_NAME}Install64_VCRedist.exe"
!else
!define OUTFILE_x64     "${PRODUCT_NAME}Install64.exe"
!endif
!define ARCH_x64        "x64"
!define BIN_ROOT_x64	"${BIN_ROOT_BASE}\x64\${DISPATCH_APP_MODE}"

!define ARCH ${ARCH_${_BUILDARCH}}
!define BIN_ROOT	${BIN_ROOT_${_BUILDARCH}}

!define VCRedist64SrcLocation "..\Build\Support\x64\vcredist_x64.exe"
!define VCRedist32SrcLocation "..\Build\Support\vcredist_x86.exe"

Section binroot
DetailPrint "binary root ${BIN_ROOT}"
SectionEnd

Name "${NAME_${_BUILDARCH}}"

OutFile "${BIN_ROOT}\..\${OUTFILE_${_BUILDARCH}}"

InstallDir "(na)"

SpaceTexts "none"

ShowInstDetails nevershow

RequestExecutionLevel admin

;----------------------------------------------------------------------------
; 
;----------------------------------------------------------------------------

VIProductVersion "${VERSION}.0.0"
VIAddVersionKey "ProductName" "${PRODUCT_FULL_NAME}"
VIAddVersionKey "CompanyName" "${COMPANY_NAME}"
VIAddVersionKey "LegalCopyright" "${COPYRIGHT_STRING}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Installer"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"

;----------------------------------------------------------------------------
; Pages
;----------------------------------------------------------------------------

!define MUI_ICON "..\apps\res\sandbox-full.ico"

!define MUI_PAGE_CUSTOMFUNCTION_PRE "SkipLicensePage"
!insertmacro MUI_PAGE_LICENSE "LICENSE.TXT"

Page custom InstallTypePage
!define MUI_PAGE_CUSTOMFUNCTION_PRE "SkipDirectoryPage"
!insertmacro MUI_PAGE_DIRECTORY

!define MUI_PAGE_CUSTOMFUNCTION_SHOW "SetHeaderForRemove"
;该向导页面显示安装进度
!insertmacro MUI_PAGE_INSTFILES

Page custom DriverPage

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_PAGE_CUSTOMFUNCTION_PRE "DisableBackButton"
!define MUI_FINISHPAGE_TITLE_3LINES
!insertmacro MUI_PAGE_FINISH

;----------------------------------------------------------------------------
; Languages
;----------------------------------------------------------------------------
;!include "LangChinese.nsh"
;!insertmacro MUI_LANGUAGE "English"
;!insertmacro MUI_LANGUAGE "Albanian"
;!insertmacro MUI_LANGUAGE "Arabic"
;!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "SimpChinese"
;!insertmacro MUI_LANGUAGE "TradChinese"
;!insertmacro MUI_LANGUAGE "Croatian"
;!insertmacro MUI_LANGUAGE "Czech"
;!insertmacro MUI_LANGUAGE "Danish"
;!insertmacro MUI_LANGUAGE "Dutch"
;!insertmacro MUI_LANGUAGE "Estonian"
;!insertmacro MUI_LANGUAGE "Farsi"
;!insertmacro MUI_LANGUAGE "Finnish"
;!insertmacro MUI_LANGUAGE "French"
;!insertmacro MUI_LANGUAGE "German"
;!insertmacro MUI_LANGUAGE "Greek"
;!insertmacro MUI_LANGUAGE "Hebrew"
;!insertmacro MUI_LANGUAGE "Hungarian"
;!insertmacro MUI_LANGUAGE "Italian"
;!insertmacro MUI_LANGUAGE "Indonesian"
;!insertmacro MUI_LANGUAGE "Japanese"
;!insertmacro MUI_LANGUAGE "Korean"
;!insertmacro MUI_LANGUAGE "Macedonian"
;!insertmacro MUI_LANGUAGE "Polish"
;!insertmacro MUI_LANGUAGE "PortugueseBr"
;!insertmacro MUI_LANGUAGE "Portuguese"
;!insertmacro MUI_LANGUAGE "Russian"
;!insertmacro MUI_LANGUAGE "Slovak"
;!insertmacro MUI_LANGUAGE "Spanish"
;!insertmacro MUI_LANGUAGE "Swedish"
;!insertmacro MUI_LANGUAGE "Turkish"
;!insertmacro MUI_LANGUAGE "Ukrainian"
;!insertmacro MUI_LANGUAGE "Norwegian"
;!include "MyItalian.nlf"

!macro MyLanguage MYLANG MYCP
;cp936 is also called gbk or ms936, which is often used to encode unified chinese language.
    !system "..\tools\iconv\iconv -f UTF-16 -t ${MYCP} ..\msgs\SbieRelease\NsisText_${MYLANG}.txt > ansi-My${MYLANG}.nlf"
    !include "ansi-My${MYLANG}.nlf"
    ;!delfile "ansi-My${MYLANG}.nlf"
!macroend

;!insertmacro MUI_LANGUAGE "CustomizedChinese"
;!insertmacro MyLanguage "English" LATIN1
;!insertmacro MyLanguage "Albanian" LATIN1
;!insertmacro MyLanguage "Arabic" CP1256
;!insertmacro MyLanguage "Bulgarian" CP1251
!insertmacro MyLanguage "SimpChinese" CP936
;!insertmacro MyLanguage "TradChinese" CP950
;!insertmacro MyLanguage "Croatian" CP1250
;!insertmacro MyLanguage "Czech" CP1250
;!insertmacro MyLanguage "Danish" CP865
;!insertmacro MyLanguage "Dutch" LATIN1
;!insertmacro MyLanguage "Estonian" LATIN1
;!insertmacro MyLanguage "Farsi" CP1256
;!insertmacro MyLanguage "Finnish" LATIN1
;!insertmacro MyLanguage "French" LATIN1
;!insertmacro MyLanguage "German" LATIN1
;!insertmacro MyLanguage "Greek" CP1253
;!insertmacro MyLanguage "Hebrew" CP1255
;!insertmacro MyLanguage "Hungarian" CP1250
;!insertmacro MyLanguage "Italian" LATIN1
;!insertmacro MyLanguage "Indonesian" LATIN1
;!insertmacro MyLanguage "Japanese" CP932
;!insertmacro MyLanguage "Korean" CP949
;!insertmacro MyLanguage "Macedonian" CP1251
;!insertmacro MyLanguage "Polish" CP1250
;!insertmacro MyLanguage "PortugueseBr" LATIN1
;!insertmacro MyLanguage "Portuguese" LATIN1
;!insertmacro MyLanguage "Russian" CP1251
;!insertmacro MyLanguage "Slovak" CP1250
;!insertmacro MyLanguage "Spanish" LATIN1
;!insertmacro MyLanguage "Swedish" LATIN1
;!insertmacro MyLanguage "Turkish" CP1254
;!insertmacro MyLanguage "Ukrainian" CP1251

;!include "SimpChinese.nsh"

;----------------------------------------------------------------------------
; Reserve Files
;----------------------------------------------------------------------------

ReserveFile "InstallType.ini"

!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

ReserveFile "Warning.ini"

ReserveFile "${BIN_ROOT}\${KMDUTIL_EXE}"

;----------------------------------------------------------------------------
; Global Variables
;----------------------------------------------------------------------------

Var InstallType
Var LaunchControl
Var MustReboot
Var BundledInstall
Var DeleteSandboxieIni
;Var Win7Driver

;----------------------------------------------------------------------------
; macro InstallSystemDll
;----------------------------------------------------------------------------

!macro InstallSystemDll DllName Is64Bit

IfFileExists "$SYSDIR\${DllName}" 0 +2
   StrCpy $1 1		; already installed

!if "${_BUILDARCH}" == "x64"
${If} ${Is64Bit} == false
DetailPrint "InstallSystemDll 32 ${DllSrcLocation64}\${DllName} $SYSDIR\${DllName}"
!insertmacro InstallLib DLL $1 REBOOT_NOTPROTECTED ${DllSrcLocation32}\${DllName} "$SYSDIR\${DllName}" $SYSDIR
${Else}
!define LIBRARY_X64
DetailPrint "InstallSystemDll 64 ${DllSrcLocation64}\${DllName} $SYSDIR\${DllName}"
!insertmacro InstallLib DLL $1 REBOOT_NOTPROTECTED ${DllSrcLocation64}\${DllName} "$SYSDIR\${DllName}" $SYSDIR
!insertmacro EnableX64FSRedirection
!undef LIBRARY_X64
${EndIf}
!else
!insertmacro InstallLib DLL $1 REBOOT_NOTPROTECTED ${DllSrcLocation32}\${DllName} "$SYSDIR\${DllName}" $SYSDIR
!endif

!macroend

;----------------------------------------------------------------------------
; macro CompareDllVersions
;----------------------------------------------------------------------------

!macro CompareDllVersions DllName Version

	GetDllVersion "${DllName}" $R0 $R1
	
	IntOp $R2 $R0 / 0x00010000
	IntOp $R3 $R0 & 0x0000FFFF
	IntOp $R0 $R1 >> 16
	IntOp $R4 $R0 & 0x0000FFFF
	IntOp $R5 $R1 & 0x0000FFFF
	StrCpy $0 "$R2.$R3.$R4.$R5"
	${VersionCompare} "$0" "${Version}" $R0
	; result in $R0
	; $R0=0	Versions are equal
    ; $R0=1	DllName version is newer
    ; $R0=2	DllName version is older
!macroend

;----------------------------------------------------------------------------
; macro InstallVCRedist
;----------------------------------------------------------------------------

; eventually, the 64 bit ver will need to include the 32 bit VC Redist.  Not required right now.

!macro InstallVCRedist

!if "${_BUILDARCH}" == "x64"
	File "${VCRedist64SrcLocation}"
!else
	File "${VCRedist32SrcLocation}"
!endif

!if "${_BUILDARCH}" == "x64"
	ExecWait '"${VCRedist64SrcLocation}" /passive' $0
!else
	ExecWait '"${VCRedist32SrcLocation}" /passive' $0
!endif

StrCmp $0 "0" +3
	MessageBox MB_OK|MB_ICONSTOP "Installation of VCRedist failed: $0"
	Quit

!macroend

;----------------------------------------------------------------------------
; macro DownloadInstallVCRedist
;----------------------------------------------------------------------------

!macro DownloadInstallVCRedist

${If} ${RunningX64}
!insertmacro DisableX64FSRedirection
${EndIf}

!insertmacro CompareDllVersions	"$SYSDIR\mfc100u.dll" "10.0.40219.325"
StrCmp $R0 "2" Update_Version
!insertmacro CompareDllVersions	"$SYSDIR\msvcr100.dll" "10.0.40219.325"
StrCmp $R0 "2" Update_Version
Goto	DownloadInstallVCRedist_Exit	; versions are all up to date

Update_Version:

StrCpy $1 "https://www.sandboxie.com/support"
!if "${_BUILDARCH}" == "x64"
	StrCpy $1 "$1/X64"
	StrCpy $2 "vcredist_x64.exe"
!else
	StrCpy $2 "vcredist_x86.exe"
!endif
	
NSISdl::download /TIMEOUT=30000 $1/$2 $TEMP\$2
	Pop $R0 ;Get the return value
	StrCmp $R0 "success" +3
    MessageBox MB_RETRYCANCEL|MB_ICONSTOP "Download failed for: $1/$2.$\nError: $R0.$\nRerun the install when problem corrected." IDRETRY -3
    Quit

ExecWait '"$TEMP\$2" /passive' $0
StrCmp $0 "0" +3
	MessageBox MB_OK|MB_ICONSTOP "Installation of VCRedist failed: $0.$\nRerun the install when problem corrected."
	Quit

DownloadInstallVCRedist_Exit:

${If} ${RunningX64}
!insertmacro EnableX64FSRedirection
${EndIf}
	
!macroend

;----------------------------------------------------------------------------
; Function .onInit
;----------------------------------------------------------------------------

;This callback will be called when the installer is nearly finished initializing. If the '.onInit' function calls Abort, the installer will quit instantly.
Function .onInit
;
; Initialize variables
;

    StrCpy $InstallType "Unknown"
    StrCpy $LaunchControl "N"
    StrCpy $MustReboot "N"
    StrCpy $BundledInstall "N"
    StrCpy $DeleteSandboxieIni "Y"
    StrCpy $Language "0"
	StrCpy $Language "2052"
;    
; Force InstallType if requested on command line
;

    Call GetParameters
    Pop $0
    StrLen $1 $0

Force_Loop:

	;StrCmp val1 val2 jump_if_equal [jump_if_val1_less] [jump_if_val1_more]
    IntCmp $1 6 Force_Done Force_Done
	
    StrCpy $2 $0 $1
    IntOp  $1 $1 - 1

    StrCmp $2 "/lang=" Force_Language
    StrCmp $2 "/install_bundled" Force_Bundled_Install
    StrCmp $2 "/install" Force_Install
    StrCmp $2 "/upgrade" Force_Upgrade
    StrCmp $2 "/remove" Force_Remove
    
    Goto Force_Loop

Force_Language:
	;StrCpy destination str [maxlen] [start_offset]
    StrCpy $Language $0 $1 6
	;StrLen length str
    StrLen $2 $Language
    IntOp  $2 $2 + 7          ; skip "/lang=NNNN "
    StrLen $1 $0
    IntOp  $1 $1 - $2
    StrCpy $0 $0 $1 $2
    StrLen $1 $0
    Goto Force_Loop
    
Force_Bundled_Install:

    StrCpy $BundledInstall "Y"
    StrCpy $LaunchControl "N"
    Goto Force_Done
    
Force_Install:

    StrCpy $InstallType "Install"
    Goto Force_Done

Force_Upgrade:

    StrCpy $InstallType "Upgrade"
    Goto Force_Done

Force_Remove:
    
    StrCpy $InstallType "Remove"
    Goto Force_Done

Force_Done:

;
; Detect existing installation, unless given by /D parameter
;
    
    StrCmp $INSTDIR "(na)" 0 InstDir_Done

    !insertmacro Reg_ReadString "" ${HKEY_LOCAL_MACHINE} "SYSTEM\CurrentControlSet\Services\${SBIEDRV}" "ImagePath"
    Pop $0
    StrCmp $0 "" 0 InstDir_CheckPath
    
    Goto InstDir_ProgramFiles
    
InstDir_CheckPath:

    StrCpy $1 $0 4 0
    StrCmp $1 "\??\" 0 InstDir_Check_Suffix
    StrCpy $0 $0 "" 4
    
InstDir_Check_Suffix:

    Push -12
    Pop  $2
    StrCpy $1 $0 "" $2
    StrCmp $1 "\${SBIEDRV_SYS}" InstDir_Suffix_Good
    
    Goto InstDir_ProgramFiles

InstDir_Suffix_Good:

    StrCpy $0 $0 $2
    Goto InstDir_Found
    
InstDir_ProgramFiles:

    !insertmacro Reg_ReadString "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion" "ProgramFilesDir"
    Pop $0
    StrCmp $0 "" InstDir_Done
    StrCpy $0 "$0\${PRODUCT_FULL_NAME}"

InstDir_Found:

    StrCpy $INSTDIR $0
    
InstDir_Done:

;
; Detect InstallType if not yet known
;

    StrCmp $InstallType "Unknown" 0 InstType_Done

    StrCmp "$EXEDIR"   "$INSTDIR"             InstType_Remove
    StrCmp "$EXEDIR"   "$INSTDIR\"            InstType_Remove
    StrCmp "$EXEDIR\"  "$INSTDIR"             InstType_Remove
    StrCmp "$EXEDIR"   "$WINDIR\Installer"    InstType_Remove
    StrCmp "$EXEDIR"   "$WINDIR\Installer\"   InstType_Remove
    StrCmp "$EXEDIR\"  "$WINDIR\Installer"    InstType_Remove
    
    IfFileExists $INSTDIR\${SBIEDRV_SYS}      InstType_Upgrade
    IfFileExists $INSTDIR\${SBIESVC_EXE}      InstType_Upgrade
    IfFileExists $INSTDIR\${SBIEDLL_DLL}      InstType_Upgrade
    
    ; default InstallType for a new install, unless we detected an installation

    StrCpy $InstallType "Install"
    Goto InstType_Done

InstType_Upgrade:

    StrCpy $InstallType "Upgrade"
    Goto InstType_Done

InstType_Remove:

    StrCpy $InstallType "Remove"

InstType_Done:

;
; Language 
;

    StrCmp $Language "0" 0 Lang_Done
    
    ReadRegDWORD $0 HKLM "SYSTEM\CurrentControlSet\Services\${SBIESVC}" "Language"
    IntCmp $0 1033 Lang_AutoSelect  ; English
    IntCmp $0 1052 Lang_AutoSelect  ; Albanian
    IntCmp $0 1025 Lang_AutoSelect  ; Arabic
    IntCmp $0 1026 Lang_AutoSelect  ; Bulgarian
    IntCmp $0 1050 Lang_AutoSelect  ; Croatian
    IntCmp $0 1029 Lang_AutoSelect  ; Czech
    IntCmp $0 1030 Lang_AutoSelect  ; Danish
    IntCmp $0 1043 Lang_AutoSelect  ; Dutch
    IntCmp $0 1061 Lang_AutoSelect  ; Estonian
    IntCmp $0 1065 Lang_AutoSelect  ; Farsi
    IntCmp $0 1035 Lang_AutoSelect  ; Finnish
    IntCmp $0 1036 Lang_AutoSelect  ; French
    IntCmp $0 1031 Lang_AutoSelect  ; German
    IntCmp $0 1032 Lang_AutoSelect  ; Greek
    IntCmp $0 1037 Lang_AutoSelect  ; Hebrew
    IntCmp $0 1038 Lang_AutoSelect  ; Hungarian
    IntCmp $0 1040 Lang_AutoSelect  ; Italian
    IntCmp $0 1057 Lang_AutoSelect  ; Indonesian
    IntCmp $0 1041 Lang_AutoSelect  ; Japanese
    IntCmp $0 1042 Lang_AutoSelect  ; Korean
    IntCmp $0 1071 Lang_AutoSelect  ; Macedonian
    IntCmp $0 1045 Lang_AutoSelect  ; Polish
    IntCmp $0 1046 Lang_AutoSelect  ; Portuguese (Brasil)
    IntCmp $0 2070 Lang_AutoSelect  ; Portuguese (Portugal)
    IntCmp $0 1049 Lang_AutoSelect  ; Russian
    IntCmp $0 1051 Lang_AutoSelect  ; Slovak
    IntCmp $0 1034 Lang_AutoSelect  ; Spanish
    IntCmp $0 1053 Lang_AutoSelect  ; Swedish
    IntCmp $0 1055 Lang_AutoSelect  ; Turkish
    IntCmp $0 1058 Lang_AutoSelect  ; Ukrainian
    IntCmp $0 1028 Lang_AutoSelect  ; Chinese (Traditional)
    IntCmp $0 2052 Lang_AutoSelect  ; Chinese (Simplified)
    StrCpy $0 1033                  ; English - default

Lang_AutoSelect:

    StrCpy $Language $0
    StrCmp $InstallType "Remove" Lang_Done
    
    !define MUI_LANGDLL_WINDOWTITLE "${NAME_${_BUILDARCH}}"
    !insertmacro MUI_LANGDLL_DISPLAY

Lang_Done:

;
; Extract InstallOptions scripts
;
    !insertmacro MUI_INSTALLOPTIONS_EXTRACT "InstallType.ini"
    !insertmacro MUI_INSTALLOPTIONS_EXTRACT "Warning.ini"
    
FunctionEnd

;----------------------------------------------------------------------------
; Function ConfirmRequirements
;----------------------------------------------------------------------------

Function ConfirmRequirements

;
; Confirm we are on Windows XP or later
;

    !insertmacro Reg_ReadString "" ${HKEY_LOCAL_MACHINE} "'Software\Microsoft\Windows NT\CurrentVersion'" "CurrentVersion"
    Pop $0
    StrCmp $0 "" SystemCheck_Fail
    ;StrCmp $0 "5.1" SystemCheck_Done_XP_2003
    ;StrCmp $0 "5.2" SystemCheck_Done_XP_2003
    StrCmp $0 "6.0" SystemCheck_Done
    StrCmp $0 "6.1" SystemCheck_Done
    StrCmp $0 "6.2" SystemCheck_Done
    StrCmp $0 "6.3" SystemCheck_Done
    StrCmp $0 "6.4" SystemCheck_Done
	StrCmp $0 "6.5" SystemCheck_Done
	StrCmp $0 "6.6" SystemCheck_Done
	StrCmp $0 "6.7" SystemCheck_Done
	StrCmp $0 "6.8" SystemCheck_Done
	StrCmp $0 "6.9" SystemCheck_Done
	StrCmp $0 "7.0" SystemCheck_Done
    Goto SystemCheck_Fail

SystemCheck_Fail:

    StrCmp $InstallType "Remove" SystemCheck_Force_Remove

    MessageBox MB_OK|MB_ICONSTOP "$(MSG_8041)$\n\Windows Vista, Windows 7, Windows 8, Windows 10, Windows 11."
    StrCmp $InstallType "Upgrade" SystemCheck_Force_Remove
    Quit

;SystemCheck_Done_XP_2003:
    
!if "${_BUILDARCH}" == "x64"

    MessageBox MB_OK|MB_ICONSTOP "$(MSG_8041)$\n\(64-bit)   Windows Vista (Service Pack 1), Windows 7, Windows 8, Windows 10."
    StrCmp $InstallType "Upgrade" SystemCheck_Force_Remove
    Quit

!endif
    
    Goto SystemCheck_Done

SystemCheck_Force_Remove:
    
    StrCpy $InstallType "ForceRemove"
    Goto SystemCheck_Done

SystemCheck_Done:

    !insertmacro Reg_ReadString "" ${HKEY_LOCAL_MACHINE} "'Software\Microsoft\Windows\CurrentVersion\Uninstall\Sandboxie-Plus_is1'" "UninstallString"
    Pop $0
    StrCmp $0 "" PlusCheck_Done

    MessageBox MB_OK|MB_ICONSTOP "Sandboxie Plus installation detected, you must uninstall it first before you can downgrade to Sandboxie Classic!"
    Quit

PlusCheck_Done:

;
; Confirm we are running as Administrator
;

    Call IsUserAdmin
    Pop $0
    StrCmp $0 1 AdminCheck_Done

    MessageBox MB_OK|MB_ICONSTOP $(MSG_8042)
    Quit
    
AdminCheck_Done:

;
; Confirm that Installer bitness is same as that of the operating system
; Our installer is a 32-bit program, so if WOW-64 mode is enabled in the
; process, it means we are in 64-bit Windows
;

    Call IsWow64
    Pop $0

!if "${_BUILDARCH}" == "x64"
    StrCmp $0 "Y" InitCheck_Done
    MessageBox MB_OK|MB_ICONSTOP $(MSG_8044)
    Quit
!else
    StrCmp $0 "N" InitCheck_Done
    MessageBox MB_OK|MB_ICONSTOP $(MSG_8043)
    Quit
!endif    

InitCheck_Done:

FunctionEnd

;----------------------------------------------------------------------------
; Function SkipLicensePage
;----------------------------------------------------------------------------

Function SkipLicensePage

;
; Confirm minimum Windows version and use of Administrator account
;

    Call ConfirmRequirements

;
; Skip license page if not Install mode
;

    StrCmp $InstallType "Install" SkipLicensePage_Done
    
    Abort
    
SkipLicensePage_Done:

FunctionEnd

;----------------------------------------------------------------------------
; Function InstallTypePage
;----------------------------------------------------------------------------

Function InstallTypePage

;
; Provisional windows 7 support
;
;    !insertmacro Reg_ReadString "" ${HKEY_LOCAL_MACHINE} "'Software\Microsoft\Windows NT\CurrentVersion'" "CurrentVersion"
;    Pop $0
;    StrCmp $0 "6.0" w7_Drv_ask
;    StrCmp $0 "6.1" w7_Drv_ask
;    Goto w7_Skip
;
;w7_Drv_ask:
;
;    MessageBox MB_YESNO|MB_ICONQUESTION "Windows 7 requires a provisional driver package. You will have to download it from the GitHub release page https://github.com/sandboxie-plus/Sandboxie/releases/$\r$\nDo you have it downloaded?" IDYES w7_Drv_ok
;
;    MessageBox MB_YESNO|MB_ICONQUESTION "Do you want to open the download page in your default web browser?" IDNO w7_Drv_cancel
;    ExecShell "open" "https://github.com/sandboxie-plus/Sandboxie/releases/"
;    Goto w7_Drv_ok
;
;w7_Drv_cancel:
;    MessageBox MB_OK|MB_ICONSTOP "On windows 7 the install can not continue without the provisional driver package"
;
;    Quit
;
;w7_Drv_ok:
;
;    nsDialogs::SelectFileDialog open "" "Driver binary (*.${_W7DRV_COMPAT}.rc4)|*.${_W7DRV_COMPAT}.rc4|All Files|*.*"
;    
;    Pop $0
;    StrCmp $0 "" w7_Drv_cancel
;    StrCpy $Win7Driver $0
;
;;    MessageBox MB_OK $Win7Driver
;
;w7_Skip:

    StrCmp $InstallType "Install" InstallType_Done
    StrCmp $InstallType "Upgrade" InstallType_SetFolder
    
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 4" "State" "0"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 5" "State" "1"
    
    StrCmp $InstallType "ForceRemove" 0 InstallType_SetFolder
    StrCpy $InstallType "Remove"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 4" "Flags" "DISABLED"
    
InstallType_SetFolder:
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 2" "State" "$INSTDIR"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Settings" "RTL" "$(^RTL)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Settings" "NextButtonText" "$(MSG_8003)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 1" "Text" "$(MSG_8004)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 3" "Text" "$(MSG_8005)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 4" "Text" "$(MSG_8006)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 5" "Text" "$(MSG_8007)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 6" "Text" "$(MSG_8008)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "InstallType.ini" "Field 7" "Text" "$(MSG_8009)"
    !insertmacro MUI_HEADER_TEXT "$(MSG_8001)" "$(MSG_8002)"
    !insertmacro MUI_INSTALLOPTIONS_DISPLAY "InstallType.ini"

InstallType_Done:
	

FunctionEnd

;----------------------------------------------------------------------------
; Function SkipDirectoryPage
;----------------------------------------------------------------------------

Function SkipDirectoryPage

    StrCmp $InstallType "Install" SkipDirectoryPage_Done
    
;
; Set InstallType according to selected radio button
;

    !insertmacro MUI_INSTALLOPTIONS_READ $4 "InstallType.ini" "Field 4" "State"
    !insertmacro MUI_INSTALLOPTIONS_READ $5 "InstallType.ini" "Field 5" "State"
    
    StrCmp $4 "1" SetType_Upgrade
    StrCmp $5 "1" SetType_Remove
    Goto SetType_Done
    
SetType_Upgrade:

    StrCpy $InstallType "Upgrade"
    Goto SetType_Done

SetType_Remove:

    StrCpy $InstallType "Remove"
    Goto SetType_Done
    
SetType_Done:

    Abort
    
SkipDirectoryPage_Done:

FunctionEnd

;----------------------------------------------------------------------------
; Function SetHeaderForRemove
;----------------------------------------------------------------------------

Function SetHeaderForRemove

    StrCmp $InstallType "Remove" 0 SetHeaderForRemove_Done

    !insertmacro MUI_HEADER_TEXT "Uninstalling" "$(MSG_8016)"
    ;!insertmacro MUI_HEADER_TEXT "Uninstalling" "are you want to uninstall?"
;
; Ask about removing settings
;
    Goto SetHeaderForRemove_Done
	
    IfSilent SetHeaderForRemove_Done
    
    MessageBox MB_YESNO|MB_ICONQUESTION "$(MSG_8054)" IDNO SetHeaderForRemove_Done
    
    StrCpy $DeleteSandboxieIni "N"
    
SetHeaderForRemove_Done:

FunctionEnd

;----------------------------------------------------------------------------
; Install Section
;----------------------------------------------------------------------------

; We download various files during install just to keep stats on activity

Function DownloadStatPng
;
	Pop $0	; Get the parameter (file name to download)
;	${If} ${RunningX64}
;	SetRegView 64
;	${EndIf}
;	ReadRegStr $1 HKLM "SOFTWARE\Microsoft\Cryptography" "MachineGuid"
;	StrCpy $2 "https://www.sandboxie.com/img/$0?SessionId=$1"
;	
;	;NSISdl::download_quiet /TIMEOUT=3000 $2 $TEMP\$0
;	inetc::get /SILENT /CONNECTTIMEOUT=5000 /RECEIVETIMEOUT=5000  $2 $TEMP\$0 /END
;	Pop $0 ;Get the return value
;	;MessageBox MB_OK|MB_ICONSTOP "DownloadStatPng:  $2$\n$0"
;	${If} ${RunningX64}
;	SetRegView 32
;	${EndIf}
FunctionEnd


Section ""

;LogSet on
;
; Prepare KmdUtil and then use it to warn about running programs
;

    File "/oname=$PLUGINSDIR\${KMDUTIL_EXE}" "${BIN_ROOT}\${KMDUTIL_EXE}"
    File "/oname=$PLUGINSDIR\${SBIEMSG_DLL}" "${BIN_ROOT}\${SBIEMSG_DLL}"
    File "/oname=$PLUGINSDIR\${SBIEDLL_DLL}" "${BIN_ROOT}\${SBIEDLL_DLL}"

    StrCpy $0 ""
    ;IfSilent 0 +2
    StrCpy $0 "_silent"
    StrCpy $0 "scandll$0"
    Push $0
    Call KmdUtilX
    IfErrors FindWindow_Abort
    
;
; Close Sandboxie Control if it is running
;

FindWindow_Check:

    FindWindow $0 "${SANDBOXIE_CONTROL}WndClass" ""
    StrCmp $0 0 FindWindow_Done

    SendMessage $0 ${WM_DESTROY} 0 0 /TIMEOUT=2000
    Sleep 1000
    
    FindWindow $0 "${SANDBOXIE_CONTROL}WndClass" ""
    StrCmp $0 0 FindWindow_Done
    
    MessageBox MB_OKCANCEL|MB_ICONQUESTION "$(MSG_8051)" IDCANCEL FindWindow_Abort

    Goto FindWindow_Check

FindWindow_Abort:
	MessageBox MB_OK "FindWindow_Check error,Kmdutil.exe runas Amin"
    Abort

FindWindow_Done:

	StrCmp $InstallType "Remove" check_thirdparty_driver_end
	
	nsExec::Exec "sc query ${VERACRYPT}"
	Pop $R0
	IntCmp $R0 1060 check_inspect_driver
	;MessageBox MB_OK "Your computer had already installed Veracrypt driver,please uninstall veracrypt first and reinstall sandbox"
	nsExec::Exec "sc stop ${VERACRYPT}"
	nsExec::Exec "sc delete ${VERACRYPT}"
	;StrCpy $InstallType "Upgrade"
	;StrCpy $MustReboot "Y"
	
	check_inspect_driver:
	nsExec::Exec "sc query ${WDF_INSPECT}"
	Pop $R0
	IntCmp $R0 1060 check_sbiesvc_driver
	;MessageBox MB_OK "Your computer had already installed sandbox driver,please uninstall veracrypt first and reinstall sandbox"
	
	nsExec::Exec "sc stop ${WDF_INSPECT}"
	nsExec::Exec "sc delete ${WDF_INSPECT}"
	;StrCpy $InstallType "Upgrade"
	;StrCpy $MustReboot "Y"
	
check_sbiesvc_driver:
	nsExec::Exec "sc query ${SBIESVC}"
	Pop $R0
	IntCmp $R0 1060 check_sbiedrv_driver
	Push 'stop ${SBIESVC}'
    Call KmdUtil
	
	Push 'delete ${SBIESVC}'
    Call KmdUtil
	;StrCpy $InstallType "Upgrade"
	;StrCpy $MustReboot "Y"
	
check_sbiedrv_driver:
	nsExec::Exec "sc query ${SBIEDRV}"
	Pop $R0
	IntCmp $R0 1060 check_thirdparty_driver_end
	Push 'stop ${SBIEDRV}'
    Call KmdUtil
	Push 'delete ${SBIEDRV}'
    Call KmdUtil
	;StrCpy $InstallType "Upgrade"
	;StrCpy $MustReboot "Y"


check_thirdparty_driver_end:

;
; Stop Sandboxie Service
;

	StrCmp $InstallType "Remove" Remove
	
	;MessageBox MB_OK "test section"

	; Windows 10 doesn't allow copying a .sys on top of a running driver.  So we have to stop it here.
    Push "stop ${SBIEDRV}"
    Call KmdUtil	
	
	Push "stop ${SBIESVC}"
    Call KmdUtil

;
; Branch to Install, Upgrade or Remove according to install mode
;

    StrCmp $InstallType "Install" Install

    ReadRegStr $0 HKLM "SOFTWARE\System Safety\System Safety Monitor\2" "Version"
    StrCmp $0 "" NoSystemSafetyMonitor
    StrCpy $MustReboot "Y"
NoSystemSafetyMonitor:

    ReadRegStr $0 HKLM "SOFTWARE\TrendMicro\PC-cillin" "version"
    StrCmp $0 "" NoTrendMicro
    StrCpy $MustReboot "Y"
NoTrendMicro:

    StrCmp $InstallType "Upgrade" Upgrade
    StrCmp $InstallType "Remove" Remove

    Abort

;
; Install and Upgrade
;

Upgrade:
	Goto Section_Done
	
    Call DeleteProductKey
    Call DeleteSystemKeys
    Call DeleteShortCuts
	; we use this file download as a total # of upgrades counter
	Push  "sbietrayfullup.png"
	Call DownloadStatPng
	Goto Install2
	
Install:

	; we use this file download as a total # of installs counter
	Push  "sbietrayfull.png"
	Call DownloadStatPng
	
Install2:	
    Call WriteProgramFiles

	!define DllSrcLocation32 "..\Build\Support"

;	MS libraries are all static with Visual Studio 2015	
;!if "${_BUILDARCH}" == "x64"
;	!define DllSrcLocation64 "..\Build\Support\x64"
;	!insertmacro InstallSystemDll "msvcr100.dll" false
;	!insertmacro InstallSystemDll "mfc100u.dll" true
;	!insertmacro InstallSystemDll "msvcr100.dll" true
;	!insertmacro InstallSystemDll "msvcp100.dll" true
;!endif

;!ifdef INCLUDE_VCREDIST_DNLD
;	!insertmacro DownloadInstallVCRedist
;!else
;	!insertmacro InstallSystemDll "msvcr100.dll" false
;	!insertmacro InstallSystemDll "mfc100u.dll" false
;	!insertmacro InstallSystemDll "msvcp100.dll" false
;!endif

    Call WriteProductKey
    Call WriteShortCuts
    
    Goto Section_Done

;
; Remove
;

Remove:

	; we use this file download as a total # of uninstalls counter
	Push  "sbietrayempty.png"
	Call DownloadStatPng
    Call DeleteProductKey
    Call DeleteProgramFiles
    Call DeleteSystemKeys
    Call DeleteShortCuts

    Goto Section_Done

;
; Done
;

Section_Done:

    IfSilent 0 Section_Done2
    Call DriverPage

Section_Done2:

SectionEnd

;----------------------------------------------------------------------------
; Function WriteProgramFiles
;----------------------------------------------------------------------------

Function WriteProgramFiles

    SetOverwrite try

    SetOutPath "$INSTDIR"

    StrCpy $2 0
	
    
WriteLoop:
	
    ;
    ; Write files
    ;
	File /oname=${KMDUTIL_EXE} "${BIN_ROOT}\${KMDUTIL_EXE}"
	
	File /oname=${SBIEINI_EXE} "${BIN_ROOT}\${SBIEINI_EXE}"
	
	File /oname=${TEST_CONFIG_FILE} "${BIN_ROOT}\..\config\${TEST_CONFIG_FILE}"
	
	File /oname=$WINDIR\${SANDBOXIE_INI} "${BIN_ROOT}\..\config\${SANDBOXIE_INI}"
	File /oname=${PUBLICFUN_DLL} "${BIN_ROOT}\${PUBLICFUN_DLL}"
	File /oname=${EXPLORERPP_EXE} "${BIN_ROOT}\..\ui\${EXPLORERPP_EXE}"
	
	File /oname=${UI_MAIN_APP} "${BIN_ROOT}\..\ui\${UI_MAIN_APP}"
	File /oname=Qt5Widgets.dll "${BIN_ROOT}\..\ui\Qt5Widgets.dll"
	File /oname=Qt5Gui.dll "${BIN_ROOT}\..\ui\Qt5Gui.dll"
	File /oname=Qt5Core.dll "${BIN_ROOT}\..\ui\Qt5Core.dll"
	
	File /oname=patch.bat "${BIN_ROOT}\..\ui\patch.bat"
	
	File /oname=Qt5Charts.dll "${BIN_ROOT}\..\ui\Qt5Charts.dll"

	File /oname=Qt5WinExtras.dll "${BIN_ROOT}\..\ui\Qt5WinExtras.dll"

	File /oname=Qt5Network.dll "${BIN_ROOT}\..\ui\Qt5Network.dll"
	
	File /oname=qsdpclient.exe "${BIN_ROOT}\..\ui\qsdpclient.exe"
	File /oname=sdpclient.dll "${BIN_ROOT}\..\ui\sdpclient.dll"
	File /oname=sdptunnel.exe "${BIN_ROOT}\..\ui\sdptunnel.exe"
	File /oname=server_public.key "${BIN_ROOT}\..\ui\server_public.key"
	;File /oname=Watermark.exe "${BIN_ROOT}\..\ui\Watermark.exe"
	
	
	File /oname=SdpClientStart.exe "${BIN_ROOT}\..\ui\SdpClientStart.exe"
	;File /oname=icudt68.dll "${BIN_ROOT}\..\ui\icudt68.dll"
	;File /oname=icuin68.dll "${BIN_ROOT}\..\ui\icuin68.dll"
	;File /oname=icuuc68.dll "${BIN_ROOT}\..\ui\icuuc68.dll"
	
	
	;File /oname=GM3000_Setup.exe "${BIN_ROOT}\..\ui\GM3000_Setup.exe"
	;nsExec::Exec "GM3000_Setup.exe /silent"
	
	;File /oname=${WINDIR}\mtoken_gm3000.dll "${BIN_ROOT}\..\ui\mtoken_gm3000.dll"
	${If} ${RunningX64}
	${DisableX64FSRedirection}
	;File "${WINDIR}\mtoken_gm3000.dll" ; Install 64-bit file on 64-bit Windows
	File /oname=$WINDIR\mtoken_gm3000.dll "${BIN_ROOT}\..\ui\mtoken_gm3000.dll"
	${EnableX64FSRedirection}
	${Else}
	;File "${WINDIR}\mtoken_gm3000.dll" ; Install 32-bit file on 32-bit Windows
	File /oname=$WINDIR\mtoken_gm3000.dll "${BIN_ROOT}\..\ui\mtoken_gm3000.dll"
	${EndIf}
	
	;CreateDirectory $INSTDIR\mingw
	;File /oname=mingw\icudt68.dll "${BIN_ROOT}\..\ui\mingw\icudt68.dll"
	;File /oname=mingw\icuin68.dll "${BIN_ROOT}\..\ui\mingw\icuin68.dll"
	;File /oname=mingw\icuio68.dll "${BIN_ROOT}\..\ui\mingw\icuio68.dll"
	;File /oname=mingw\icutest68.dll "${BIN_ROOT}\..\ui\mingw\icutest68.dll"
	;File /oname=mingw\icutu68.dll "${BIN_ROOT}\..\ui\mingw\icutu68.dll"
	;File /oname=mingw\icuuc68.dll "${BIN_ROOT}\..\ui\mingw\icuuc68.dll"
	;File /oname=mingw\libgcc_s_seh-1.dll "${BIN_ROOT}\..\ui\mingw\libgcc_s_seh-1.dll"
	;File /oname=mingw\libstdc++-6.dll "${BIN_ROOT}\..\ui\mingw\libstdc++-6.dll"
	;File /oname=mingw\libwinpthread-1.dll "${BIN_ROOT}\..\ui\mingw\libwinpthread-1.dll"
	;File /oname=mingw\Qt5Core.dll "${BIN_ROOT}\..\ui\mingw\Qt5Core.dll"
	;File /oname=mingw\Qt5Gui.dll "${BIN_ROOT}\..\ui\mingw\Qt5Gui.dll"
	;File /oname=mingw\Qt5Svg.dll "${BIN_ROOT}\..\ui\mingw\Qt5Svg.dll"
	;File /oname=mingw\Qt5Widgets.dll "${BIN_ROOT}\..\ui\mingw\Qt5Widgets.dll"
	;File /oname=mingw\SafeDesktop.exe "${BIN_ROOT}\..\ui\mingw\SafeDesktop.exe"
	;CreateDirectory $INSTDIR\mingw\platforms
	;File /oname=mingw\platforms\qdirect2d.dll "${BIN_ROOT}\..\ui\mingw\platforms\qdirect2d.dll"
	;File /oname=mingw\platforms\qminimal.dll "${BIN_ROOT}\..\ui\mingw\platforms\qminimal.dll"
	;File /oname=mingw\platforms\qoffscreen.dll "${BIN_ROOT}\..\ui\mingw\platforms\qoffscreen.dll"
	;File /oname=mingw\platforms\qwebgl.dll "${BIN_ROOT}\..\ui\mingw\platforms\qwebgl.dll"
	;File /oname=mingw\platforms\qwindows.dll "${BIN_ROOT}\..\ui\mingw\platforms\qwindows.dll"
	
	;CreateDirectory $INSTDIR\bearer
	;File /oname=bearer\qgenericbearer.dll "${BIN_ROOT}\..\ui\bearer\qgenericbearer.dll"
	
	;CreateDirectory $INSTDIR\iconengines
	;File /oname=iconengines\qsvgicon.dll "${BIN_ROOT}\..\ui\iconengines\qsvgicon.dll"
	
	;CreateDirectory $INSTDIR\imageformats
	;File /oname=imageformats\qgif.dll "${BIN_ROOT}\..\ui\imageformats\qgif.dll"
	;File /oname=imageformats\qicns.dll "${BIN_ROOT}\..\ui\imageformats\qicns.dll"
	;File /oname=imageformats\qico.dll "${BIN_ROOT}\..\ui\imageformats\qico.dll"
	;File /oname=imageformats\qjpeg.dll "${BIN_ROOT}\..\ui\imageformats\qjpeg.dll"
	;File /oname=imageformats\qsvg.dll "${BIN_ROOT}\..\ui\imageformats\qsvg.dll"
	;File /oname=imageformats\qtga.dll "${BIN_ROOT}\..\ui\imageformats\qtga.dll"
	;File /oname=imageformats\qtiff.dll "${BIN_ROOT}\..\ui\imageformats\qtiff.dll"
	;File /oname=imageformats\qwbmp.dll "${BIN_ROOT}\..\ui\imageformats\qwbmp.dll"
	;File /oname=imageformats\qwebp.dll "${BIN_ROOT}\..\ui\imageformats\qwebp.dll"
	
	CreateDirectory $INSTDIR\platforms
	File /oname=platforms\qdirect2d.dll "${BIN_ROOT}\..\ui\platforms\qdirect2d.dll"
	File /oname=platforms\qminimal.dll "${BIN_ROOT}\..\ui\platforms\qminimal.dll"
	File /oname=platforms\qoffscreen.dll "${BIN_ROOT}\..\ui\platforms\qoffscreen.dll"
	File /oname=platforms\qwebgl.dll "${BIN_ROOT}\..\ui\platforms\qwebgl.dll"
	File /oname=platforms\qwindows.dll "${BIN_ROOT}\..\ui\platforms\qwindows.dll"
	
	
	File /oname=clear.exe "${BIN_ROOT}\..\ui\clear.exe"

	
	File /oname=VeraCrypt.exe "${BIN_ROOT}\..\veracrypt\VeraCrypt.exe"
	File /oname=veracrypt.sys "${BIN_ROOT}\..\veracrypt\veracrypt.sys"
	;/oname后面的内容中要是有空格的话就要把包括/oname在内的整条语句用双引号括起来
	File "/oname=${VERACRYPT_FORMAT_EXE}" "${BIN_ROOT}\..\veracrypt\${VERACRYPT_FORMAT_EXE}"
	File /oname=VeraCryptExpander.exe "${BIN_ROOT}\..\veracrypt\VeraCryptExpander.exe"
	
	File /oname=${DESKTOP_APP} "${BIN_ROOT}\${DESKTOP_APP}"
    
	File /oname=${WDF_INSPECT_SYS} "${BIN_ROOT}\..\inspect\${WDF_INSPECT_SYS}"
    
    File /oname=${SBIEDLL_DLL} "${BIN_ROOT}\${SBIEDLL_DLL}"

    File /oname=${SBIESVC_EXE} "${BIN_ROOT}\${SBIESVC_EXE}"
	
	File /oname=${CYGUARD_DLL} "${BIN_ROOT}\${CYGUARD_DLL}"
	
	;File /oname=${PROCESSINJECTOR_EXE} "${BIN_ROOT}\${PROCESSINJECTOR_EXE}"

!if "${_BUILDARCH}" == "x64"
    CreateDirectory $INSTDIR\32

    File /oname=32\${SBIEDLL_DLL} "${BIN_ROOT_Win32}\${SBIEDLL_DLL}"
    
    File /oname=32\${SBIESVC_EXE} "${BIN_ROOT_Win32}\${SBIESVC_EXE}"
	
	File /oname=32\${CYGUARD_DLL} "${BIN_ROOT_Win32}\${CYGUARD_DLL}"
	
	;File /oname=32\${PROCESSINJECTOR_EXE} "${BIN_ROOT_Win32}\${PROCESSINJECTOR_EXE}"
!endif

    File /oname=${SBIEMSG_DLL} "${BIN_ROOT}\${SBIEMSG_DLL}"

    File /oname=${SBIEDRV_SYS} "${BIN_ROOT}\${SBIEDRV_SYS}"
;    File /oname=${SBIEDRV_SYS4} "${BIN_ROOT}\SbieDrv.sys.rc4"
;    File /oname=${SBIEDRV_SYSX} "${BIN_ROOT}\SbieDrv.sys.w10"

    File /oname=${SBOXHOSTDLL_DLL}			   "${BIN_ROOT}\${SBOXHOSTDLL_DLL}"
    
    File /oname=${SANDBOXIE}RpcSs.exe      "${BIN_ROOT}\SandboxieRpcSs.exe"
    File /oname=${SANDBOXIE}DcomLaunch.exe "${BIN_ROOT}\SandboxieDcomLaunch.exe"
    File /oname=${SANDBOXIE}BITS.exe       "${BIN_ROOT}\SandboxieBITS.exe"
    File /oname=${SANDBOXIE}Crypto.exe     "${BIN_ROOT}\SandboxieCrypto.exe"
    File /oname=${SANDBOXIE}WUAU.exe       "${BIN_ROOT}\SandboxieWUAU.exe"
    
    File /oname=${START_EXE} "${BIN_ROOT}\${START_EXE}"
    
    File /oname=${SBIECTRL_EXE} "${BIN_ROOT}\${SBIECTRL_EXE}"
  
    File "..\install\Templates.ini"

    File "Manifest0.txt"
    File "Manifest1.txt"
    File "Manifest2.txt"
    
    ;File "${BIN_ROOT}\License.exe"
    
	;File /oname=Certificate.dat "${BIN_ROOT}\..\sign\public.key"
	
    File "LICENSE.TXT"
	
	
	File /oname=${SBIESVC_EXE}.sig "${BIN_ROOT}\..\sign\${SBIESVC_EXE}.sig"
	
	File /oname=${SBIECTRL_EXE}.sig "${BIN_ROOT}\..\sign\${SBIECTRL_EXE}.sig"
    
    ;
    ; Check for files being held
    ;
    
    IfErrors 0 WriteOk
    
    IntCmp $2 5 WriteError
    IntOp  $2 $2 + 1
    Sleep 1000
    Goto WriteLoop

WriteError:
    IfSilent WriteAbort
    StrCpy $2 0
    MessageBox MB_OKCANCEL|MB_ICONSTOP $(MSG_8052) IDOK WriteLoop

WriteAbort:

    Abort
    
WriteOk:
    SetOverwrite on

    ;
    ; Create shortcut 
    ;
    
	CreateShortCut "$DESKTOP\${QUICK_LAUNCH}.lnk" "$INSTDIR\qsdpclient.exe" "" "" "" SW_SHOWNORMAL "" "$(MSG_8026)"
	;CreateShortCut "$DESKTOP\${QUICK_LAUNCH}.lnk" "$INSTDIR\${DESKTOP_APP}" "" "" "" SW_SHOWNORMAL "" "$(MSG_8026)"	
    ;CreateShortCut "$INSTDIR\Quick Launch.lnk" "$INSTDIR\${START_EXE}" default_browser "" "" SW_SHOWNORMAL "" "$(MSG_8026)"

    ;
    ; Copy this Installer program to the Windows\Installer directory
    ;
    
    StrCmp $BundledInstall "Y" SkipCopyInstaller
    
    System::Call 'Kernel32::GetModuleFileNameA(i 0, t .r0, i 1024) i r1'
    ; $0 --> Installer Filename
    
    CopyFiles "$0" "$WINDIR\Installer\${OUTFILE_${_BUILDARCH}}"

SkipCopyInstaller:
    ;
    ; Delete old files
    ;
    
    Delete "$DESKTOP\${PRODUCT_NAME} Quick Launch.lnk"
    Delete "$QUICKLAUNCH\${PRODUCT_NAME} Quick Launch.lnk"

FunctionEnd

;----------------------------------------------------------------------------
; Function DeleteProgramFiles
;----------------------------------------------------------------------------

Function DeleteProgramFiles
;
; Delete program files
;	

!if "${_BUILDARCH}" == "x64"    
    
    Delete "$INSTDIR\32\${SBIEDLL_DLL}"
    
    Delete "$INSTDIR\32\${SBIESVC_EXE}"
	
	Delete "$INSTDIR\32\${CYGUARD_DLL}"
	
	;Delete "$INSTDIR\32\${PROCESSINJECTOR_EXE}"
    
    RMDir "$INSTDIR\32"
!endif   
    Delete "$INSTDIR\${SANDBOXIE}WUAU.exe"
    ;Delete "$INSTDIR\${SANDBOXIE}EventSys.exe"
    Delete "$INSTDIR\${SANDBOXIE}Crypto.exe"
    Delete "$INSTDIR\${SANDBOXIE}BITS.exe"
    Delete "$INSTDIR\${SANDBOXIE}DcomLaunch.exe"
    Delete "$INSTDIR\${SANDBOXIE}RpcSs.exe"

    Delete "$INSTDIR\Templates.ini"
    
    Delete "$INSTDIR\Manifest0.txt"
    Delete "$INSTDIR\Manifest1.txt"
    Delete "$INSTDIR\Manifest2.txt"

	Delete "$INSTDIR\LICENSE.txt"
    ;Delete "$INSTDIR\LICENSE.EXE"
	
	Delete "$INSTDIR\${EXPLORERPP_EXE}"
	
	;kill SafeDesktop.exe
	ExecWait "taskkill /t /f /im ${UI_MAIN_APP}"
	Delete "$INSTDIR\${UI_MAIN_APP}"
	
	Delete "$INSTDIR\Qt5Network.dll"
	Delete "$INSTDIR\Qt5Widgets.dll"
	Delete "$INSTDIR\Qt5Gui.dll"
	Delete "$INSTDIR\Qt5Core.dll"
	Delete "$INSTDIR\Qt5Charts.dll"
	Delete "$INSTDIR\Qt5WinExtras.dll"
	
	;Delete "$INSTDIR\bearer\qgenericbearer.dll"
	;RMDir "$INSTDIR\bearer"
	
	;Delete "$INSTDIR\iconengines\qsvgicon.dll"
	;RMDir "$INSTDIR\iconengines"
	
	;Delete "$INSTDIR\imageformats\qgif.dll"
	;Delete "$INSTDIR\imageformats\qicns.dll"
	;Delete "$INSTDIR\imageformats\qico.dll"
	;Delete "$INSTDIR\imageformats\qjpeg.dll"
	;Delete "$INSTDIR\imageformats\qsvg.dll"
	;Delete "$INSTDIR\imageformats\qtga.dll"
	;Delete "$INSTDIR\imageformats\qtiff.dll"
	;Delete "$INSTDIR\imageformats\qwbmp.dll"
	;Delete "$INSTDIR\imageformats\qwebp.dll"
	;RMDir "$INSTDIR\imageformats"
	
	Delete "$INSTDIR\platforms\qdirect2d.dll"
	Delete "$INSTDIR\platforms\qminimal.dll"
	Delete "$INSTDIR\platforms\qoffscreen.dll"
	Delete "$INSTDIR\platforms\qwebgl.dll"
	Delete "$INSTDIR\platforms\qwindows.dll"
	
	Delete "$INSTDIR\platforms\Qt5Charts.dll"

	RMDir "$INSTDIR\platforms"

	Delete "$INSTDIR\qsdpclient.exe"
	Delete "$INSTDIR\sdpclient.dll"
	Delete "$INSTDIR\sdptunnel.exe"
	Delete "$INSTDIR\server_public.key"
	Delete "$INSTDIR\Watermark.exe"
	
	Delete "$INSTDIR\SdpClientStart.exe"
	;Delete "$INSTDIR\icudt68.dll"
	;Delete "$INSTDIR\icuin68.dll"
	;Delete "$INSTDIR\icuuc68.dll"
	
	
	;Delete "$INSTDIR\GM3000_Setup.exe"
	Delete "$WINDIR\mtoken_gm3000.dll"

	
	Delete "$INSTDIR\Certificate.dat"

	Delete "$INSTDIR\sfdesktop.yml"

	Delete "$INSTDIR\${SBIEINI_EXE}"
    
    Delete "$INSTDIR\${START_EXE}"

    Delete "$INSTDIR\${SBIECTRL_EXE}"
	
	Delete "$INSTDIR\${DESKTOP_APP}"
	
	;C:\Users\你的用户名\AppData\Local
	RMDir /r  "$LOCALAPPDATA\SafeDesktop"
	
	
;
; Delete installer
;
	; use Delete /REBOOTOK (works with wildcards) and use RMDir /REBOOTOK on an empty directory, or a directory that's about to become empty after a reboot.
    Delete /REBOOTOK "$INSTDIR\${OUTFILE_Win32}"
    Delete /REBOOTOK "$INSTDIR\${OUTFILE_x64}"
    Delete /REBOOTOK "$INSTDIR\${OUTFILE_BOTH}"
    Delete /REBOOTOK "$WINDIR\Installer\${OUTFILE_Win32}"
    Delete /REBOOTOK "$WINDIR\Installer\${OUTFILE_x64}"
    Delete /REBOOTOK "$WINDIR\Installer\${OUTFILE_BOTH}"

;
; Delete shortcuts
;
	Delete "$DESKTOP\${QUICK_LAUNCH}.lnk"
    Delete "$INSTDIR\${QUICK_LAUNCH}.lnk"

    Push "$DESKTOP"
    Call DeleteDesktopShortCuts
    
    Push "$QUICKLAUNCH"
    Call DeleteDesktopShortCuts
    
    RMDir /r "$SENDTO\${PRODUCT_NAME}"
    Delete "$SENDTO\${PRODUCT_NAME} - *.lnk"


FunctionEnd

;----------------------------------------------------------------------------
; Function WriteProductKey
;----------------------------------------------------------------------------

Function WriteProductKey
;
; Create key for Add/Remove Programs 
;

    StrCmp $BundledInstall "Y" SkipAddRemovePrograms

    !insertmacro Reg_CreateKey "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Uninstall" "${PRODUCT_NAME}"
    !insertmacro Reg_WriteString "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayName" '"${NAME_${_BUILDARCH}}"'
    !insertmacro Reg_WriteString "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayIcon" '$INSTDIR\${START_EXE}'
    !insertmacro Reg_WriteString "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayVersion" '"${VERSION}"'
    !insertmacro Reg_WriteString "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "Publisher" '"${COMPANY_NAME}"'
    !insertmacro Reg_WriteString "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "UninstallString" `'"$WINDIR\Installer\${OUTFILE_${_BUILDARCH}}" /remove'`

;
; Create GUID value for CancelAutoplay functionality in Sandboxie Control
;

    !insertmacro Reg_TestKey "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\CancelAutoplay\CLSID"
    Pop $0
    StrCmp $0 "0" CancelAutoplay_Done
    !insertmacro Reg_WriteString "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\CancelAutoplay\CLSID" "${MY_AUTOPLAY_CLSID}" '""'
CancelAutoplay_Done:

SkipAddRemovePrograms:

FunctionEnd

;----------------------------------------------------------------------------
; Function DeleteProductKey
;----------------------------------------------------------------------------

Function DeleteProductKey
;
; Delete key for Add/Remove Programs 
; 
    !insertmacro Reg_DeleteKey "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Uninstall" "${PRODUCT_NAME}"

FunctionEnd

;----------------------------------------------------------------------------
; Function WriteShortCuts
;----------------------------------------------------------------------------

Function WriteShortCuts
    StrCmp $BundledInstall "Y" SkipWriteShortCuts

    SetShellVarContext all

    CreateDirectory "$SMPROGRAMS\${PRODUCT_FULL_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_FULL_NAME}\$(MSG_8021)" "$INSTDIR\${START_EXE}" "/box:__ask__ run_dialog" "" "" SW_SHOWNORMAL "" "$(MSG_8022)"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_FULL_NAME}\$(MSG_8023)" "$INSTDIR\${START_EXE}" "/box:__ask__ start_menu" "" "" SW_SHOWNORMAL "" "$(MSG_8024)"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_FULL_NAME}\$(MSG_8025)" "$INSTDIR\${START_EXE}" "default_browser" "" "" SW_SHOWNORMAL "" "$(MSG_8026)"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_FULL_NAME}\$(MSG_8027)" "$INSTDIR\${START_EXE}" "." "" "" SW_SHOWNORMAL "" "$(MSG_8028)"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_FULL_NAME}\$(MSG_8029)" "$INSTDIR\${SBIECTRL_EXE}" "/open" "" "" SW_SHOWNORMAL "" "$(MSG_8030)"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_FULL_NAME}\$(MSG_8031)" "$WINDIR\Installer\${OUTFILE_${_BUILDARCH}}" "/remove" "" "" SW_SHOWNORMAL "" "$(MSG_8032)"
SkipWriteShortCuts:

FunctionEnd

;----------------------------------------------------------------------------
; Function DeleteShortCuts
;----------------------------------------------------------------------------

Function DeleteShortCuts

    SetShellVarContext all
    Call DeleteShortCuts_2
    
    SetShellVarContext current
    Call DeleteShortCuts_2

FunctionEnd

Function DeleteShortCuts_2

    RMDir /r "$SMPROGRAMS\${PRODUCT_FULL_NAME}"

FunctionEnd

;----------------------------------------------------------------------------
; Function DeleteDesktopShortCuts
;----------------------------------------------------------------------------

Function DeleteDesktopShortCuts

    Pop $1
    Delete "$1\${PRODUCT_NAME} ${QUICK_LAUNCH}.lnk"

    ; delete #8025.lnk in selected language and in English
    Delete "$1\$(MSG_8025)"
    Delete "$1\Run Web browser sandboxed.lnk"
    
    ; delete #8033.lnk in selected language and in English
    Delete "$1\$(MSG_8033)"
    Delete "$1\Sandboxed Web Browser.lnk"

FunctionEnd

;----------------------------------------------------------------------------
; Function DeleteSystemKeys
;----------------------------------------------------------------------------

Function DeleteSystemKeys

    StrCmp $InstallType "Remove" 0 ShellKey_Folder_Done
    
;
; Delete SandboxieControl value from Windows\Run key
;

    !insertmacro Reg_ReadString "" ${HKEY_CURRENT_USER} "Software\Microsoft\Windows\CurrentVersion\Run" "${SANDBOXIE_CONTROL}"
    Pop $0
    StrCmp $0 "" RunKey_Done
    
    !insertmacro Reg_DeleteValue "" ${HKEY_CURRENT_USER} "Software\Microsoft\Windows\CurrentVersion\Run" "${SANDBOXIE_CONTROL}"

RunKey_Done:

;
; Delete GUID value for CancelAutoplay functionality in Sandboxie Control
;

    !insertmacro Reg_DeleteValue "" ${HKEY_LOCAL_MACHINE} "Software\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\CancelAutoplay\CLSID" "${MY_AUTOPLAY_CLSID}"

;
; Delete sandbox key from HKCR\*\shell key
;

    !insertmacro Reg_TestKey "" ${HKEY_CLASSES_ROOT} "*\shell\${SANDBOX_VERB}"
    Pop $0
    StrCmp $0 "0" ShellKey_Star_Done

    !insertmacro Reg_DeleteKey "" ${HKEY_CLASSES_ROOT} "*\shell\${SANDBOX_VERB}" "command"
    !insertmacro Reg_DeleteKey "" ${HKEY_CLASSES_ROOT} "*\shell" "${SANDBOX_VERB}"

ShellKey_Star_Done:

;
; Delete sandbox key from HKCR\Folder\shell key
;

    !insertmacro Reg_TestKey "" ${HKEY_CLASSES_ROOT} "Folder\shell\${SANDBOX_VERB}"
    Pop $0
    StrCmp $0 "0" ShellKey_Folder_Done

    !insertmacro Reg_DeleteKey "" ${HKEY_CLASSES_ROOT} "Folder\shell\${SANDBOX_VERB}" "command"
    !insertmacro Reg_DeleteKey "" ${HKEY_CLASSES_ROOT} "Folder\shell" "${SANDBOX_VERB}"

ShellKey_Folder_Done:

;
; Delete keys which may have been left by old versions of Sandboxie
;
; HKLM\Software\Classes\*\shell\sandbox
; HKLM\Software\Classes\Folder\shell\sandbox
; HKCR\exefile\shell
;

;
; Delete sandbox key from HKLM\Software\Classes\*\shell\sandbox
;

    !insertmacro Reg_TestKey "" ${HKEY_LOCAL_MACHINE} "Software\Classes\*\shell\${SANDBOX_VERB}"
    Pop $0
    StrCmp $0 "0" HKLM_ShellKey_Star_Done

    !insertmacro Reg_DeleteKey "" ${HKEY_LOCAL_MACHINE} "Software\Classes\*\shell\${SANDBOX_VERB}" "command"
    !insertmacro Reg_DeleteKey "" ${HKEY_LOCAL_MACHINE} "Software\Classes\*\shell" "${SANDBOX_VERB}"

HKLM_ShellKey_Star_Done:

;
; Delete sandbox key from HKLM\Software\Classes\Folder\shell key
;

    !insertmacro Reg_TestKey "" ${HKEY_LOCAL_MACHINE} "Software\Classes\Folder\shell\${SANDBOX_VERB}"
    Pop $0
    StrCmp $0 "0" HKLM_ShellKey_Folder_Done

    !insertmacro Reg_DeleteKey "" ${HKEY_LOCAL_MACHINE} "Software\Classes\Folder\shell\${SANDBOX_VERB}" "command"
    !insertmacro Reg_DeleteKey "" ${HKEY_LOCAL_MACHINE} "Software\Classes\Folder\shell" "${SANDBOX_VERB}"

HKLM_ShellKey_Folder_Done:

;
; Delete sandbox key from HKCR\exefile\shell key
;

    !insertmacro Reg_TestKey "" ${HKEY_CLASSES_ROOT} "exefile\shell\${SANDBOX_VERB}"
    Pop $0
    StrCmp $0 "0" ShellKey_Exe_Done
    
    !insertmacro Reg_DeleteKey "" ${HKEY_CLASSES_ROOT} "exefile\shell\${SANDBOX_VERB}" "command"
    !insertmacro Reg_DeleteKey "" ${HKEY_CLASSES_ROOT} "exefile\shell" "${SANDBOX_VERB}"

ShellKey_Exe_Done:

FunctionEnd

;----------------------------------------------------------------------------
; Function KmdUtilX
;----------------------------------------------------------------------------

Function KmdUtilX

    Pop $1
    DetailPrint "Running KmdUtil command: $1"
    ;MessageBox MB_OK|MB_ICONEXCLAMATION "Running KmdUtil command: $1"
    ClearErrors
	SetDetailsPrint listonly
    ExecWait '"$PLUGINSDIR\${KMDUTIL_EXE}" /lang=$Language $1'
	SetDetailsPrint both

FunctionEnd

;----------------------------------------------------------------------------
; Function KmdUtil
;----------------------------------------------------------------------------

Function KmdUtil
    Call KmdUtilX
    IfErrors KmdUtil_Errors
    Goto KmdUtil_Done
    
KmdUtil_Errors:
    StrCmp $1 "stop ${SBIEDRV}" KmdUtil_SetReboot
    DetailPrint "Could not process KmdUtil command: $1"
    ;MessageBox MB_OK|MB_ICONEXCLAMATION "$(MSG_8053)$\n$\n$PLUGINSDIR\${KMDUTIL_EXE}  $1"
    StrCmp $1 "stop ${SBIESVC}" KmdUtil_SetReboot
    Goto KmdUtil_Done

KmdUtil_SetReboot:
    SetRebootFlag true
    
KmdUtil_Done:
    ClearErrors

FunctionEnd

;----------------------------------------------------------------------------
; Function DriverPage
;----------------------------------------------------------------------------

Function DriverPage

;
; Show the warning page, unless we're doing a Remove
;

    StrCmp $InstallType "Remove" Driver_Remove

    IfSilent Driver_Silent
    
    !insertmacro MUI_INSTALLOPTIONS_WRITE "Warning.ini" "Settings" "RTL" "$(^RTL)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "Warning.ini" "Field 1" "Text" "$(MSG_8012)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "Warning.ini" "Field 2" "Text" "$(MSG_8013)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "Warning.ini" "Field 4" "Text" "$(MSG_8014)"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "Warning.ini" "Field 5" "Text" "$(MSG_8015)"

    !insertmacro MUI_HEADER_TEXT "$(MSG_8010)" "$(MSG_8011)"
    !insertmacro MUI_INSTALLOPTIONS_DISPLAY_RETURN "Warning.ini"

    Pop $0
    StrCmp $0 "cancel" Driver_Done
	
Driver_Silent:
	StrCmp $InstallType "Upgrade" Driver_Upgrade

;
; For Install and Upgrade, install the driver
;

;
; For Install, we start the driver and finish
;

Driver_Install:

;  StrCmp $Win7Driver "" now_w7_Drv
;
;  MessageBox MB_OK $Win7Driver
;  Delete "$INSTDIR\SbieDrv.sys.w10"
;  Rename "$INSTDIR\SbieDrv.sys" "$INSTDIR\SbieDrv.sys.w10"
;  CopyFiles $Win7Driver "$INSTDIR\SbieDrv.sys.rc4"
;
;now_w7_Drv:

	nsExec::Exec "sc query ${SBIEDRV}"
	Pop $R0
	IntCmp $R0 1060 driver_drv_setup check_driver_svc_setup
driver_drv_setup:
    ;StrCpy $0 'install ${SBIEDRV} "$INSTDIR\${SBIEDRV_SYS}" type=kernel start=auto "msgfile=$INSTDIR\${SBIEMSG_DLL}" altitude=${FILTER_ALTITUDE}'
    ;Push $0
	Push 'install ${SBIEDRV} "$INSTDIR\${SBIEDRV_SYS}" type=kernel start=auto "msgfile=$INSTDIR\${SBIEMSG_DLL}" altitude=${FILTER_ALTITUDE}'
    Call KmdUtil

check_driver_svc_setup:
	nsExec::Exec "sc query ${SBIESVC}"
	Pop $R0
	IntCmp $R0 1060 driver_svc_setup check_driver_veracrypt_setup
driver_svc_setup:
	;Push 'my_install_start_service ${SBIESVC} "$INSTDIR\${SBIESVC_EXE}" '
    Push 'install ${SBIESVC} "\"$INSTDIR\${SBIESVC_EXE}"\" type=own start=auto "display=${PRODUCT_FULL_NAME} Service" group=UIGroup "msgfile=$INSTDIR\${SBIEMSG_DLL}"'
    Call KmdUtil

    WriteRegDWORD HKLM "SYSTEM\CurrentControlSet\Services\${SBIESVC}" "Language" $Language
    
    WriteRegDWORD HKLM "SYSTEM\CurrentControlSet\Services\${SBIESVC}" "PreferExternalManifest" 1
    
	;why not start sbiedrv in original code?
	;Push "start ${SBIEDRV}"
    ;Call KmdUtil
	
	Push "start ${SBIESVC}"
    Call KmdUtil
	
check_driver_veracrypt_setup:
	nsExec::Exec "sc query veracrypt"
	Pop $R0
	IntCmp $R0 1060 driver_veracrypt_setup check_driver_inspect_setup
driver_veracrypt_setup:
	Push 'my_install_start_driver veracrypt "$INSTDIR\veracrypt.sys" type=kernel start=auto'
    Call KmdUtil
	
check_driver_inspect_setup:
	nsExec::Exec "sc query inspect"
	Pop $R0
	IntCmp $R0 1060 driver_inspect_setup check_driver_setup_end
driver_inspect_setup:
	Push 'my_install_start_driver inspect "$INSTDIR\inspect.sys" type=kernel start=auto'
    Call KmdUtil
	
check_driver_setup_end:

	;MessageBox MB_YESNO|MB_ICONQUESTION "Your computer will restart now" IDNO Driver_Done
	;Reboot
    Goto Driver_Done

;
; For Upgrade, we make sure the driver is set to start manually,
; then stop driver (service was already stopped), and fallthrough
; to install processing to re-start the service (and driver).
;
; Note that KmdUtil "stop SbieDrv" will set the reboot flag, 
; in case the driver cannot be unloaded.
;

Driver_Upgrade:
	nsExec::Exec "sc query inspect"
	Pop $R0
	IntCmp $R0 1060 driver_inspect_upgrade check_driver_veracrypt_upgrade
	Push 'stop inspect'
    Call KmdUtil
    Push 'my_delete_only_driver inspect'
    Call KmdUtil
driver_inspect_upgrade:	
	Push 'my_install_start_driver inspect "$INSTDIR\inspect.sys" type=kernel start=auto'
    Call KmdUtil
	
check_driver_veracrypt_upgrade:
	nsExec::Exec "sc query veracrypt"
	Pop $R0
	IntCmp $R0 1060 driver_veracrypt_upgrade check_driver_drv_upgrade
	Push 'my_unmount_delete_vera_volume veracrypt "$INSTDIR"'
    Call KmdUtil
	Push 'stop veracrypt'
    Call KmdUtil
	Push 'my_delete_only_driver veracrypt'
    Call KmdUtil
driver_veracrypt_upgrade:
	Push 'my_install_start_driver veracrypt "$INSTDIR\veracrypt.sys" type=kernel start=auto'
    Call KmdUtil

check_driver_drv_upgrade:
	nsExec::Exec "sc query ${SBIEDRV}"
	Pop $R0
	IntCmp $R0 1060 driver_drv_upgrade check_driver_svc_upgrade

	WriteRegDWORD HKLM "SYSTEM\CurrentControlSet\Services\${SBIEDRV}" "Start" 3
	Push 'stop ${SBIESVC}'
    Call KmdUtil
	;if a driver need to be uninstalled,the driver must be firstly stopped ,then secondly be deleted
	Push 'stop ${SBIEDRV}'
    Call KmdUtil
	
	Push 'delete ${SBIESVC}'
    Call KmdUtil
	Push 'delete ${SBIEDRV}'
    Call KmdUtil
	
driver_drv_upgrade:
check_driver_svc_upgrade:
	nsExec::Exec "sc query ${SBIESVC}"
	Pop $R0
	IntCmp $R0 1060 driver_svc_upgrade driver_upgrade_end
driver_svc_upgrade:

	StrCpy $0 'install ${SBIEDRV} "$INSTDIR\${SBIEDRV_SYS}" type=kernel start=auto "msgfile=$INSTDIR\${SBIEMSG_DLL}" altitude=${FILTER_ALTITUDE}'
    Push $0
    Call KmdUtil
	
	Push 'install ${SBIESVC} "\"$INSTDIR\${SBIESVC_EXE}"\" type=own start=auto "display=${PRODUCT_FULL_NAME} Service" group=UIGroup "msgfile=$INSTDIR\${SBIEMSG_DLL}"'
	Call KmdUtil
	
	Push "start ${SBIEDRV}"
    Call KmdUtil
	Push 'start ${SBIESVC}'
    Call KmdUtil

driver_upgrade_end:	
    SetRebootFlag true
    Goto Driver_Done
    
;
; For Remove, delete the driver and set the reboot flag
;

Driver_Remove:
	nsExec::Exec "sc query inspect"
	Pop $R0
	IntCmp $R0 1060 check_driver_veracrypt_remove
	
	Push 'stop inspect'
    Call KmdUtil
    Push 'my_delete_only_driver inspect'
    Call KmdUtil
	Delete /REBOOTOK '$INSTDIR\${WDF_INSPECT_SYS}'
	
check_driver_veracrypt_remove:
	nsExec::Exec "sc query ${VERACRYPT}"
	Pop $R0
	IntCmp $R0 1060 check_driver_svc_remove
	
	;whith quota
	Push 'my_unmount_delete_vera_volume veracrypt "$INSTDIR"'
    Call KmdUtil
	Push 'stop veracrypt'
    Call KmdUtil
	Push 'my_delete_only_driver veracrypt'
    Call KmdUtil
	Delete "$INSTDIR\VeraCryptExpander.exe"
	Delete "$INSTDIR\${VERACRYPT_FORMAT_EXE}"
	Delete "$INSTDIR\VeraCrypt.exe"
	Delete /REBOOTOK "$INSTDIR\veracrypt.sys"

;    Delete "$INSTDIR\${SBIEDRV_SYS4}"
;    Delete "$INSTDIR\${SBIEDRV_SYSX}"


	
check_driver_svc_remove:
	nsExec::Exec "sc query ${SBIESVC}"
	Pop $R0
	IntCmp $R0 1060 check_driver_drv_remove
	Push 'stop ${SBIESVC}'
    Call KmdUtil
	

	
check_driver_drv_remove:
	nsExec::Exec "sc query ${SBIEDRV}"
	Pop $R0
	IntCmp $R0 1060 check_driver_remove_end
	Push 'stop ${SBIEDRV}'
    Call KmdUtil
	

	
	Push 'delete ${SBIESVC}'
    Call KmdUtil
	Delete /REBOOTOK "$INSTDIR\${SBIESVC_EXE}"
	
	
	Push 'delete ${SBIEDRV}'
    Call KmdUtil
	Delete /REBOOTOK "$INSTDIR\${SBIEDRV_SYS}"
	
	
	
check_driver_remove_end:
	;Delete "$INSTDIR\${PROCESSINJECTOR_EXE}"
    
	Delete "$INSTDIR\${SBIEMSG_DLL}"
	
	Delete "$INSTDIR\${SBOXHOSTDLL_DLL}"
	
	Delete "$INSTDIR\${PUBLICFUN_DLL}"
	
	Delete /REBOOTOK "$INSTDIR\${SBIEDLL_DLL}"
	
	Delete /REBOOTOK "$INSTDIR\${CYGUARD_DLL}"
	
	Delete "$INSTDIR\${KMDUTIL_EXE}"
	
	
	;
	; Delete configuration files
	;

    StrCmp $DeleteSandboxieIni "N" SkipDeleteSandboxieIni
    Delete "$INSTDIR\${SANDBOXIE_INI}"
    Delete "$WINDIR\${SANDBOXIE_INI}"
	
	Delete "$INSTDIR\${SBIESVC_EXE}.sig"
	Delete "$INSTDIR\${SBIECTRL_EXE}.sig"
	
	nsExec::Exec "$INSTDIR\clear.exe"
	Delete "$INSTDIR\clear.exe"
	
	
SkipDeleteSandboxieIni:

;
; Delete installation folder
;  
	
	;nsExec::Exec "taskkill /f /im explorer.exe"
	;nsExec::Exec "explorer.exe"
	
	RMDir /r /REBOOTOK $INSTDIR
	
	;nsExec::Exec "rd /s/q $INSTDIR"
	
	;RMDir /REBOOTOK "$INSTDIR"
	
	MessageBox MB_YESNO|MB_ICONQUESTION "Your computer will restart now" IDNO Driver_Done
	Reboot
	
    SetRebootFlag true
	
Driver_Done:

FunctionEnd

;----------------------------------------------------------------------------
; Function DisableBackButton
;----------------------------------------------------------------------------

Function DisableBackButton

    !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Settings" "BackEnabled" "0"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Settings" "CancelEnabled" "0"
    
    StrCmp $InstallType "Remove" 0 DisableBackButton_Done

;
; Replace "MUI_TEXT_FINISH_INFO_TEXT" with "MUI_UNTEXT_FINISH_INFO_TEXT".
;
; Note, for this to work, "Modern UI\System.nsh" must contain the following macro:
;
;     !macro MUI_LANGUAGEFILE_UNLANGSTRING_PAGE_1 PAGE NAME
;         LangString "${NAME}" 0 "${${NAME}}"
;         !undef "${NAME}"
;     !macroend
;
; and a line in System.nsh, in the macro "MUI_LANGUAGEFILE_END", must be changed from:
;
;     !insertmacro MUI_LANGUAGEFILE_UNLANGSTRING_PAGE FINISH "MUI_UNTEXT_FINISH_INFO_TEXT"
;
; so that it invokes the special new _1 macro:
;
;     !insertmacro MUI_LANGUAGEFILE_UNLANGSTRING_PAGE_1 FINISH "MUI_UNTEXT_FINISH_INFO_TEXT"
;

    !insertmacro MUI_INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 3" "Text" "$(MUI_UNTEXT_FINISH_INFO_TEXT)"

DisableBackButton_Done:

FunctionEnd

;----------------------------------------------------------------------------
; Function .onGUIEnd
;----------------------------------------------------------------------------

Function .onGUIEnd

    StrCmp $LaunchControl "Y" 0 Done

    ExecWait '"$INSTDIR\${START_EXE}" run_sbie_ctrl' $0

Done:

FunctionEnd
