#pragma once
#include "TLBSWidget.h"
#include "TEWGraphicButtonWidget.h"

class TNTNewServerSelectWidget2 : public TLBSWidget
{
public:
	static TNTNewServerSelectWidget2* getInstance();

	void selectLanguage(int lang);

	void selectServer(int server);

	void selectChannel(int channel);

protected:
	char pad_0024[88]; //0x0024
	TEWGraphicButtonWidget* selectLanguageButton; //0x0074
	char pad_0078[140]; //0x0078
	TEWGraphicButtonWidget* selectServerButton; //0x0104
	TEWGraphicButtonWidget* selectChannelButton; //0x0108
	char pad_010C[344]; //0x010C
}; //Size: 0x026C
static_assert(sizeof(TNTNewServerSelectWidget2) == 0x26C);
