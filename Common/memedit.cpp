#include "memedit.h"
#include "logger.h"

#include <Windows.h>
#include <stdlib.h>

bool MemEdit::PatchRetZero(DWORD dwAddress)
{
	BYTE bArr[3];
	bArr[0] = x86XOR;
	bArr[1] = x86EAXEAX;
	bArr[2] = x86RET;

	// https://stackoverflow.com/a/13026295/14784253
	DWORD dwOldValue, dwTemp;

	VirtualProtect((LPVOID)dwAddress, sizeof(bArr), PAGE_EXECUTE_READWRITE, &dwOldValue);
	BOOL bSuccess = WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwAddress, &bArr, sizeof(bArr), NULL);
	VirtualProtect((LPVOID)dwAddress, sizeof(bArr), dwOldValue, &dwTemp);
	return bSuccess;
}

bool MemEdit::PatchJmp(DWORD dwAddress, void* pDestination)
{
	patch_far_jmp pWrite =
	{
		x86JMP,
		(DWORD)pDestination - (dwAddress + sizeof(DWORD) + sizeof(BYTE))
	};

	// https://stackoverflow.com/a/13026295/14784253
	DWORD dwOldValue, dwTemp;

	VirtualProtect((LPVOID)dwAddress, sizeof(pWrite), PAGE_EXECUTE_READWRITE, &dwOldValue);
	BOOL bSuccess = WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwAddress, &pWrite, sizeof(pWrite), NULL);
	VirtualProtect((LPVOID)dwAddress, sizeof(pWrite), dwOldValue, &dwTemp);
	return bSuccess;
}

bool MemEdit::PatchCall(DWORD dwAddress, void* pDestination)
{
	patch_call pWrite =
	{
		x86CALL,
		(DWORD)pDestination - (dwAddress + sizeof(DWORD) + sizeof(BYTE))
	};

	// https://stackoverflow.com/a/13026295/14784253
	DWORD dwOldValue, dwTemp;

	VirtualProtect((LPVOID)dwAddress, sizeof(pWrite), PAGE_EXECUTE_READWRITE, &dwOldValue);
	BOOL bSuccess = WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwAddress, &pWrite, sizeof(pWrite), NULL);
	VirtualProtect((LPVOID)dwAddress, sizeof(pWrite), dwOldValue, &dwTemp);
	return bSuccess;
}

bool MemEdit::PatchNop(DWORD dwAddress, unsigned int nCount)
{
	BYTE* bArr = new BYTE[nCount];

	for (UINT i = 0; i < nCount; i++)
		bArr[i] = x86NOP;

	// https://stackoverflow.com/a/13026295/14784253
	DWORD dwOldValue, dwTemp;

	VirtualProtect((LPVOID)dwAddress, nCount, PAGE_EXECUTE_READWRITE, &dwOldValue);
	BOOL bSuccess = WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwAddress, bArr, nCount, NULL);
	VirtualProtect((LPVOID)dwAddress, nCount, dwOldValue, &dwTemp);
	return bSuccess;
}

bool MemEdit::WriteBytes(DWORD dwAddress, const char* pData, unsigned int nCount)
{
	// https://stackoverflow.com/a/13026295/14784253
	DWORD dwOldValue, dwTemp;

	VirtualProtect((LPVOID)dwAddress, nCount, PAGE_EXECUTE_READWRITE, &dwOldValue);
	BOOL bSuccess = WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwAddress, pData, nCount, NULL);
	VirtualProtect((LPVOID)dwAddress, nCount, dwOldValue, &dwTemp);
	return bSuccess;
}