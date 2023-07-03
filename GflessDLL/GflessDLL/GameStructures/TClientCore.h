#pragma once

#include <cstdint>

class TClientCore
{
public:
	static TClientCore* getInstance();

	uint32_t getSocket() const;

protected:
	char pad_0000[20]; //0x0000
	uint32_t socket; //0x0014
	char pad_0018[17200]; //0x0018
}; //Size: 0x4348
static_assert(sizeof(TClientCore) == 0x4348);