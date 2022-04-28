#pragma once

#include <wdm.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus



	//////////////////////////////////////////////////////////////////////////

	PVOID MemAllocateWithTag(
		_In_ int nPoolType,
		_In_ ULONG ulcbSize,
		_In_opt_ ULONG* pulTag
	);

	PVOID MemAllocPagePoolWithTag(
		_In_ ULONG ulcbSize,
		_In_opt_ ULONG* pulTag
	);

	PVOID MemAllocNonPagePoolWithTag(
		_In_ ULONG ulcbSize,
		_In_opt_ ULONG* pulTag
	);

	void MemFree(
		_In_ PVOID pAddress
	);
	//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif // __cplusplus
