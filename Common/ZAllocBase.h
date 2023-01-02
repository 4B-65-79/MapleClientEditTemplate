#pragma once

class ZAllocBase
{
public:
	enum BLOCK_SIZE
	{
		BLOCK16 = 0,
		BLOCK32 = 1,
		BLOCK64 = 2,
		BLOCK128 = 3,
	};

	static void** AllocRawBlocks(unsigned int uBlockSize, unsigned int uNumberOfBlocks);
};