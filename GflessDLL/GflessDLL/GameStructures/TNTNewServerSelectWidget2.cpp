#include "TNTNewServerSelectWidget2.h"
#include "../PatternScanner.h"

TNTNewServerSelectWidget2* TNTNewServerSelectWidget2::getInstance()
{
	static DWORD patternAddress = PatternScan(
		"\xa1\x00\x00\x00\x00\x8b\x00\xc6\x40\x00\x00\xa1\x00\x00\x00\x00\xc7\x00",
		"x????xxxx??x????xx",
		1
	);

	DWORD address = patternAddress;
	address = READ_PTR(address, 0);
	address = READ_PTR(address, 0);
	address = READ_PTR(address, 0);
	address = READ_PTR(address, 0x28);

	TNTNewServerSelectWidget2* widget = reinterpret_cast<TNTNewServerSelectWidget2*>(address);

	if (IsBadReadPtr(widget, sizeof(TNTNewServerSelectWidget2)))
		return nullptr;

	return widget;
}

void TNTNewServerSelectWidget2::selectLanguage(int lang)
{
	selectLanguageButton->setSelectedIndex(lang);
	selectLanguageButton->click();
}

void TNTNewServerSelectWidget2::selectServer(int server)
{
	selectServerButton->setSelectedIndex(server);
	selectServerButton->click();
}

void TNTNewServerSelectWidget2::selectChannel(int channel)
{
	selectChannelButton->setSelectedIndex(channel);
	selectChannelButton->click();
}
