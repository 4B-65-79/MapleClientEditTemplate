#pragma once
#include "asserts.h"

// thanks raj for the foundation of this module

#define x86CMPEAX 0x3D
#define x86XOR 0x33
#define x86EAXEAX 0xC0
#define x86RET 0xC3
#define x86JMP 0xE9
#define x86CALL 0xE8
#define x86NOP 0x90

// forward declarations
typedef unsigned char BYTE;
typedef unsigned long DWORD;

class MemEdit {
private:

	// need to pack this so it doesnt auto-align to 8 bytes
#pragma pack(1)
	typedef struct patch_call
	{
		BYTE nPatchType;
		DWORD dwAddress;
	} patch_far_jmp;
#pragma pack()
	assert_size(sizeof(patch_call), 0x5);

public:
	static bool PatchRetZero(DWORD dwAddress);
	static bool PatchJmp(DWORD dwAddress, void* pDestination);
	static bool PatchCall(DWORD dwAddress, void* pDestination);
	static bool PatchNop(DWORD dwAddress, unsigned int nCount);
	static bool WriteBytes(DWORD dwAddress, const char* pData, unsigned int nCount);

	/// <summary>
	/// Attempts to (over)write a value to the specified location in memory.
	/// </summary>
	/// <typeparam name="TType">Type that will be written</typeparam>
	/// <param name="dwAddress">Address to write to</param>
	/// <param name="pValue">Pointer to the value to be written</param>
	/// <returns>True if write operation was successful, otherwise false.</returns>
	template <typename TType>
	static bool WriteValue(DWORD dwAddress, TType* pValue)
	{
		return WriteBytes(dwAddress, reinterpret_cast<char*>(pValue), sizeof(TType));
	}

	/// <summary>
	/// Reads the value at the given memory location and returns a pointer to it.
	/// </summary>
	/// <typeparam name="TType">Type that will be read</typeparam>
	/// <param name="dwAddr"></param>
	/// <returns>Pointer to the value at the given location</returns>
	template <typename TType>
	static TType* ReadValue(DWORD dwAddr)
	{
		return reinterpret_cast<TType*>(dwAddr);
	}
};