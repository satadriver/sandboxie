#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <Ntstrsafe.h>

//DOS路径转换NT路径
NTSTATUS DosFileNameToNtFileName(IN PUNICODE_STRING ustrDosName, OUT PUNICODE_STRING ustrDeviceName);

//NT路径转换DOS路径
NTSTATUS NtFileNameToDosFileName(IN PUNICODE_STRING ustrDeviceName, OUT PUNICODE_STRING ustrDosName);

//符号链接
NTSTATUS FileMonQuerySymbolicLink(IN PUNICODE_STRING SymbolicLinkName, OUT PUNICODE_STRING LinkTarget);

//文件路径通配
BOOLEAN RtlPatternMatch(WCHAR* pat, WCHAR* str);
