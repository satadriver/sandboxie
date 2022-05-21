#include "common.h"

#include <stdio.h>

BOOL GetSIDA(CHAR sid[MAX_PATH])
{
	CHAR userName[MAX_PATH] = { 0 };
	DWORD nameSize = sizeof(userName) / sizeof(CHAR);
	if (!GetUserNameA((LPSTR)userName, &nameSize))
	{
		return FALSE;
	}

	CHAR userSID[MAX_PATH] = { 0 };
	CHAR userDomain[MAX_PATH] = { 0 };
	DWORD sidSize = sizeof(userSID);
	DWORD domainSize = sizeof(userDomain) / sizeof(CHAR);
	SID_NAME_USE snu;
	if (!LookupAccountNameA(NULL,
		(LPSTR)userName,
		(PSID)userSID,
		&sidSize,
		(LPSTR)userDomain,
		&domainSize,
		&snu))
	{
		return FALSE;
	}

	PSID_IDENTIFIER_AUTHORITY psia = GetSidIdentifierAuthority(userSID);
	sidSize = sprintf(sid, "S-%lu-", SID_REVISION);
	sidSize += sprintf(sid + strlen(sid), "%-lu", psia->Value[5]);

	int i = 0;
	int subAuthorities = *GetSidSubAuthorityCount(userSID);
	for (i = 0; i < subAuthorities; i++)
	{
		sidSize += sprintf(sid + sidSize, "-%lu", *GetSidSubAuthority(userSID, i));
	}

	return TRUE;
}

BOOL GetSIDW(WCHAR sid[MAX_PATH])
{
	WCHAR userName[MAX_PATH] = { 0 };
	DWORD nameSize = sizeof(userName) / sizeof(WCHAR);
	if (!GetUserNameW((LPWSTR)userName, &nameSize))
	{
		return FALSE;
	}

	WCHAR userSID[MAX_PATH] = { 0 };
	WCHAR userDomain[MAX_PATH] = { 0 };
	DWORD sidSize = sizeof(userSID);
	DWORD domainSize = sizeof(userDomain) / sizeof(WCHAR);
	SID_NAME_USE snu;
	if (!LookupAccountNameW(
		NULL,
		(LPWSTR)userName,
		(PSID)userSID,
		&sidSize,
		(LPWSTR)userDomain,
		&domainSize,
		&snu))
	{
		return FALSE;
	}

	PSID_IDENTIFIER_AUTHORITY psia = GetSidIdentifierAuthority(userSID);
	sidSize = swprintf(sid, L"S-%lu-", SID_REVISION);
	sidSize += swprintf(sid + wcslen(sid), L"%-lu", psia->Value[5]);

	int i = 0;
	int subAuthorities = *GetSidSubAuthorityCount(userSID);
	for (i = 0; i < subAuthorities; i++)
	{
		sidSize += swprintf(sid + sidSize, L"-%lu", *GetSidSubAuthority(userSID, i));
	}

	return TRUE;
}
