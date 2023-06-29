#include "TEWGraphicButtonWidget.h"

void TEWGraphicButtonWidget::setSelectedIndex(const int32_t i)
{
	selectedIndex = i;
}

void TEWGraphicButtonWidget::click()
{
	uint32_t parametersAddress = parameters;
	uint32_t callAddress = clickFunction;
	TEWGraphicButtonWidget* selfButton = this;

	__asm
	{
		mov eax, parametersAddress
		mov edx, selfButton
		call callAddress
	};
}
