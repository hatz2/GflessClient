#pragma once
#include "TEWGraphicButtonWidget.h"

class TEWButtonWidget : public TEWGraphicButtonWidget
{
protected:
	char pad_00DC[60]; //0x00DC
}; //Size: 0x0120
static_assert(sizeof(TEWButtonWidget) == 0x120);
