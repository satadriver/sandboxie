#include "path.h"

const ULONG PathTag = 'koiw';

//符号链接
NTSTATUS FileMonQuerySymbolicLink(IN PUNICODE_STRING SymbolicLinkName, OUT PUNICODE_STRING LinkTarget)
{
	//定义变量
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE LinkHandle = NULL;

	//参数效验
	if (MmIsAddressValid(SymbolicLinkName) == FALSE || MmIsAddressValid(LinkTarget) == FALSE)
	{
		ASSERT(FALSE);
		return STATUS_INVALID_PARAMETER;
	}

	do
	{
		InitializeObjectAttributes(&ObjectAttributes, SymbolicLinkName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, 0, 0);
		status = ZwOpenSymbolicLinkObject(&LinkHandle, GENERIC_READ, &ObjectAttributes);
		if (!NT_SUCCESS(status))
		{
			ASSERT(FALSE);
			break;
		}

		LinkTarget->Length = LinkTarget->MaximumLength = PAGE_SIZE;
		LinkTarget->Buffer = ExAllocatePoolWithTag(NonPagedPool, LinkTarget->MaximumLength, PathTag);
		if (LinkTarget->Buffer == NULL)
		{
			ASSERT(FALSE);
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		RtlZeroMemory(LinkTarget->Buffer, LinkTarget->MaximumLength);
		status = ZwQuerySymbolicLinkObject(LinkHandle, LinkTarget, NULL);
		if (!NT_SUCCESS(status))
		{
			ASSERT(FALSE);
			ExFreePool(LinkTarget->Buffer);
			LinkTarget->Length = LinkTarget->MaximumLength = 0;
		}

	} while (FALSE);


	if (LinkHandle)
	{
		ZwClose(LinkHandle);
	}
	return status;
}

//DOS路径转换NT路径
NTSTATUS DosFileNameToNtFileName(IN PUNICODE_STRING ustrDosName, OUT PUNICODE_STRING ustrDeviceName)
{
	//定义变量
	NTSTATUS ntretstatus = STATUS_UNSUCCESSFUL;
	NTSTATUS status;
	HANDLE hFile;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	PFILE_OBJECT FileObject;
	UNICODE_STRING volumeDosName;
	UNICODE_STRING pLinkTarget;
	static wchar_t szText[PAGE_SIZE] = { 0 };

	//参数效验
	if (MmIsAddressValid(ustrDosName) == FALSE || MmIsAddressValid(ustrDeviceName) == FALSE)
	{
		ASSERT(FALSE);
		return STATUS_INVALID_PARAMETER;
	}

	do
	{
		InitializeObjectAttributes(&ObjectAttributes, ustrDosName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenFile(&hFile, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
		if (!NT_SUCCESS(status))
		{
			ASSERT(FALSE);
			break;
		}

		status = ObReferenceObjectByHandle(hFile, FILE_READ_ATTRIBUTES, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
		if (!NT_SUCCESS(status))
		{
			ASSERT(FALSE);
			ZwClose(hFile);
			break;
		}
		ZwClose(hFile);

		//获取盘符
		volumeDosName.Length = volumeDosName.MaximumLength = PAGE_SIZE;
		volumeDosName.Buffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, PathTag);
		if (volumeDosName.Buffer == NULL)
		{
			ASSERT(FALSE);
			ntretstatus = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		RtlZeroMemory(volumeDosName.Buffer, PAGE_SIZE);
		status = IoVolumeDeviceToDosName(FileObject->DeviceObject, &volumeDosName);
		if (!NT_SUCCESS(status))
		{
			ASSERT(FALSE);
			ExFreePool(volumeDosName.Buffer);
			break;
		}

		RtlZeroMemory(szText, sizeof(szText));
		RtlStringCbPrintfExW(szText, sizeof(szText), NULL, NULL, STRSAFE_FILL_BEHIND_NULL, L"\\??\\%wZ", &volumeDosName);
		ExFreePool(volumeDosName.Buffer);
		RtlInitUnicodeString(&volumeDosName, szText);
		status = FileMonQuerySymbolicLink(&volumeDosName, &pLinkTarget);
		if (!NT_SUCCESS(status))
		{
			ASSERT(FALSE);
			break;
		}

		//设置变量
		ustrDeviceName->Length = ustrDeviceName->Length = pLinkTarget.Length + FileObject->FileName.Length;
		ustrDeviceName->MaximumLength = ustrDeviceName->Length + sizeof(wchar_t);
		ustrDeviceName->Buffer = ExAllocatePoolWithTag(NonPagedPool, ustrDeviceName->MaximumLength, PathTag);
		if (ustrDeviceName->Buffer == NULL)
		{
			ASSERT(FALSE);
			ExFreePool(pLinkTarget.Buffer);
			ntretstatus = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		else
		{
			RtlZeroMemory(szText, sizeof(szText));
			RtlZeroMemory(ustrDeviceName->Buffer, ustrDeviceName->MaximumLength);
			RtlStringCbPrintfExW(szText, sizeof(szText), NULL, NULL, STRSAFE_FILL_BEHIND_NULL, L"%wZ%wZ", &pLinkTarget, &FileObject->FileName);
			RtlInitUnicodeString(&volumeDosName, szText);
			RtlCopyMemory(ustrDeviceName->Buffer, volumeDosName.Buffer, volumeDosName.Length);
			ExFreePool(pLinkTarget.Buffer);
			ntretstatus = STATUS_SUCCESS;
		}


	} while (FALSE);

	return ntretstatus;
}


//NT路径转换DOS路径
NTSTATUS NtFileNameToDosFileName(IN PUNICODE_STRING ustrDeviceName, OUT PUNICODE_STRING ustrDosName)
{
	NTSTATUS status;
	HANDLE hFile = NULL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	PFILE_OBJECT FileObject = NULL;
	POBJECT_NAME_INFORMATION lpName = NULL;

	//参数效验
	if (MmIsAddressValid(ustrDosName) == FALSE || MmIsAddressValid(ustrDeviceName) == FALSE)
	{
		ASSERT(FALSE);
		return STATUS_INVALID_PARAMETER;
	}

	do
	{
		InitializeObjectAttributes(&ObjectAttributes, ustrDeviceName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenFile(&hFile, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
		if (!NT_SUCCESS(status))
		{
			ASSERT(FALSE);
			break;
		}

		status = ObReferenceObjectByHandle(hFile, FILE_READ_ATTRIBUTES, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
		if (!NT_SUCCESS(status))
		{
			ASSERT(FALSE);
			break;
		}

		status = IoQueryFileDosDeviceName(FileObject, &lpName);
		if (!NT_SUCCESS(status))
		{
			ASSERT(FALSE);
			break;
		}

		ustrDosName->Length = lpName->Name.Length;
		ustrDosName->MaximumLength = ustrDosName->Length + sizeof(wchar_t);
		ustrDosName->Buffer = ExAllocatePoolWithTag(NonPagedPool, ustrDosName->MaximumLength, PathTag);
		if (ustrDosName->Buffer == NULL)
		{
			ASSERT(FALSE);
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		RtlZeroMemory(ustrDosName->Buffer, ustrDosName->MaximumLength);
		RtlCopyMemory(ustrDosName->Buffer, lpName->Name.Buffer, lpName->Name.Length);
	} while (FALSE);


	if (lpName)
	{
		ExFreePool(lpName);
	}

	if (FileObject)
	{
		ObDereferenceObject(FileObject);
	}

	if (hFile)
	{
		ZwClose(hFile);
	}
	return status;
}

static int __inline Lower(int c)
{
	if ((c >= L'A') && (c <= L'Z'))
	{
		return(c + (L'a' - L'A'));
	}
	else
	{
		return(c);
	}
}

BOOLEAN RtlPatternMatch(WCHAR* pat, WCHAR* str)
{
	register WCHAR* s;
	register WCHAR* p;
	BOOLEAN star = FALSE;

loopStart:
	for (s = str, p = pat; *s; ++s, ++p) {
		switch (*p) {
		case L'?':
			if (*s == L'.') goto starCheck;
			break;
		case L'*':
			star = TRUE;
			str = s, pat = p;
			if (!*++pat) return TRUE;
			goto loopStart;
		default:
			if (Lower(*s) != Lower(*p))
				goto starCheck;
			break;
		}
	}
	if (*p == L'*') ++p;
	return (!*p);

starCheck:
	if (!star) return FALSE;
	str++;
	goto loopStart;
}
