#include "ZAllocBase.h"
#include <Windows.h>

void** ZAllocBase::AllocRawBlocks(unsigned int uBlockSize, unsigned int uNumberOfBlocks)
{
	/* TODO make this more legible */

	size_t uEnlargedBlockSize = uBlockSize + 4;
	size_t uTotalAllocationSize = (uNumberOfBlocks * uEnlargedBlockSize) + 8;

	HANDLE hHeap = GetProcessHeap();
	void** pAlloc = reinterpret_cast<void**>(HeapAlloc(hHeap, NULL, uTotalAllocationSize));

	/* if we deallocate the entire block collection, this first address is where we start */
	*(pAlloc) = reinterpret_cast<void*>((uNumberOfBlocks * uEnlargedBlockSize) + 4);

	/* block collection header */
	*(pAlloc + 1) = 0;

	/* size of first block */
	*(pAlloc + 2) = reinterpret_cast<void*>(uBlockSize);

	void** pRet = pAlloc + 3;
	DWORD* pdwRet = (DWORD*)(pAlloc + 3);

	for (UINT i = 0; i < uNumberOfBlocks - 1; i++)
	{
		/* initialize each block with a pointer to the next block */
		*pRet = reinterpret_cast<PCHAR>(pRet) + uEnlargedBlockSize;

		/* increase pointer by block size (we divide because the compiler tries to multiply) */
		pRet = reinterpret_cast<void**>(reinterpret_cast<PCHAR>(pRet) + uEnlargedBlockSize);

		/* set the preceding address to equal the size of the block */
		*(pRet - 1) = reinterpret_cast<void*>(uBlockSize);
	}

	/* nullptr indicates last block in the linked list */
	*pRet = nullptr;

	/* return address of the first block in the linked list */
	return pAlloc + 3;
}
