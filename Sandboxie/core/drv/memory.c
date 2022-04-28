#include "memory.h"

ULONG g_ulDefaultPoolTag = 'meuS';

PVOID MemAllocateWithTag(
	_In_ int nPoolType,
	_In_ ULONG ulcbSize,
	_In_opt_ ULONG* pulTag
)
/*++

Routine Description:

	This routine is called to allocate memory with different pool types.
	Pool type can be PagedPool and NonPagedPool.

Arguments:

	nPoolType - PagedPool or NonPagedPool.

	pulTag - Allocate memory tag name. If this param is NULL, we will use default
			 tag to allocate memory.

	ulcbSize - Allocate size in bytes.

Return Value:

	 Returns the result of ExAllocatePoolWithTag call.

--*/
{
	PVOID pRetAddr = NULL;

	if (ulcbSize > 0)
	{
		pRetAddr = ExAllocatePoolWithTag(
			nPoolType,
			ulcbSize,
			NULL != pulTag ? *pulTag : g_ulDefaultPoolTag
		);
		if (NULL != pRetAddr)
		{
			RtlZeroMemory(pRetAddr, ulcbSize);
		}
	}

	return pRetAddr;
}

PVOID MemAllocPagePoolWithTag(
	_In_ ULONG ulcbSize,
	_In_opt_ ULONG* pulTag
)
/*++

Routine Description:

	This routine is called to allocate PagedPool memory.

Arguments:

	ulcbSize - Allocate size in bytes.

	pulTag - Allocate memory tag name. If this param is NULL, we will use default
			 tag to allocate memory.

Return Value:

	If memory allocate success will return the memory address, else
	return NULL.

--*/
{
	return MemAllocateWithTag(PagedPool, ulcbSize, pulTag);
}

PVOID MemAllocNonPagePoolWithTag(
	_In_ ULONG ulcbSize,
	_In_opt_ ULONG* pulTag
)
/*++

Routine Description:

	This routine is called to allocate NonPagedPool memory.

Arguments:

	ulcbSize - Allocate size in bytes.

	pulTag - Allocate memory tag name. If this param is NULL, we will use default
			 tag to allocate memory.

Return Value:

	If memory allocate success will return the memory address, else
	return NULL.

--*/
{
	return MemAllocateWithTag(NonPagedPool, ulcbSize, pulTag);
}

void MemFree(
	_In_ PVOID pAddress
)
/*++

Routine Description:

	This routine is called to free memory which is allocate by MemAlloc*.

Arguments:

	pAddress - It's the memroy address which needed to free.

Return Value:

	Returns void.

--*/
{
	if (NULL != pAddress)
	{
		ExFreePool(pAddress);
	}
}
