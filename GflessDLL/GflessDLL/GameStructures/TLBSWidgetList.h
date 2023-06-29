#pragma once
#include "TLBSWidget.h"
#include <cstdint>

class TLBSWidgetList
{
public:
	class TLBSWidget* get(int i) const;

protected:
	uint32_t vtable; //0x0000
	class TLBSWidget** elements; //0x0004
	uint32_t size; //0x0008
	uint32_t capacity; //0x000C
}; //Size: 0x0010
static_assert(sizeof(TLBSWidgetList) == 0x10);
