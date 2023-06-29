#include "TLBSWidgetList.h"

TLBSWidget* TLBSWidgetList::get(int i) const
{
	if (i < 0 || i >= size)
		return nullptr;

	return elements[i];
}
