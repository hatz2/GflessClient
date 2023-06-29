#include "TCharacterSelectWidget.h"
#include "TEWGraphicButtonWidget.h"
#include "../PatternScanner.h"

TCharacterSelectWidget* TCharacterSelectWidget::getInstance()
{
	DWORD address = PatternScan(
		"\xa1\x00\x00\x00\x00\x8b\x00\x33\xd2\xe8\x00\x00\x00\x00\xa1\x00\x00\x00\x00\x8b\x00\x83\x78\x00\x00\x7d\x00\xa1\x00\x00\x00\x00\x8b\x00\x33\xd2\xe8\x00\x00\x00\x00\x8b\xc3\xe8\x00\x00\x00\x00\xa1\x00\x00\x00\x00\x8b\x00\xb2\x00\xe8\x00\x00\x00\x00\xa1",
		"x????xxxxx????x????xxxx??x?x????xxxxx????xxx????x????xxx?x????x",
		1
	);

	address = READ_PTR(address, 0);
	address = READ_PTR(address, 0);
	address = READ_PTR(address, 0);

	TCharacterSelectWidget* widget = reinterpret_cast<TCharacterSelectWidget*>(address);

	if (IsBadReadPtr(widget, sizeof(TCharacterSelectWidget)))
		return nullptr;

	return widget;
}

void TCharacterSelectWidget::setSelectedIndex(uint8_t i)
{
	if (i < 0 || i >= MAX_CHARACTERS)
		return;

	selectedIndex = i;
}

void TCharacterSelectWidget::clickStartButton()
{
	constexpr int startButtonIndex = 2;
	TEWGraphicButtonWidget* startButton = reinterpret_cast<TEWGraphicButtonWidget*>(bottomMenu->getChild(startButtonIndex));

	if (startButton == nullptr) {
		return;
	}

	startButton->click();
}

void TCharacterSelectWidget::clickCharacterButton(int i)
{
	if (i < 0 || i >= MAX_CHARACTERS)
		return;

	TEWButtonWidget* button = characterButtons[i];

	if (button == nullptr) {
		return;
	}

	button->click();
}
