#include "TLBSWidget.h"

bool TLBSWidget::isVisible() const
{
	return visible;
}

TLBSWidget* TLBSWidget::getChild(int i)
{
	return children->get(i);
}
