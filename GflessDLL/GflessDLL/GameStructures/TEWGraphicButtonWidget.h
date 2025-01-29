#pragma once
#include "TLBSWidget.h"
#include <Windows.h>

class TEWGraphicButtonWidget : public TLBSWidget
{
public:
	void setSelectedIndex(const int32_t i);

	void click();

protected:
	char pad_0024[60]; //0x0024
	int32_t selectedIndex; //0x0060
	char pad_0064[84]; //0x0064
	uint32_t clickFunction; //0x00B0
	uint32_t parameters; //0x00B4
	char pad_00B8[36]; //0x00B8
}; //Size: 0x00E4
static_assert(sizeof(TEWGraphicButtonWidget) == 0xE4);
