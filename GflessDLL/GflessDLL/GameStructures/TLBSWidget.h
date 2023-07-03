#pragma once
#include "TLBSWidgetList.h"
#include <cstdint>

class TLBSWidget
{
public:
	bool isVisible() const;

	TLBSWidget* getChild(int i);

protected:
	char pad_0000[24]; //0x0000
	bool visible; //0x0018
	char pad_0019[7]; //0x0019
	class TLBSWidgetList* children; //0x0020
}; //Size: 0x0024
static_assert(sizeof(TLBSWidget) == 0x24);

