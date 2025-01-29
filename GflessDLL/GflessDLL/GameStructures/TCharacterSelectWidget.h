#pragma once
#include "TLBSWidget.h"
#include "TEWButtonWidget.h"

struct CharacterInfo
{
public:
	uint8_t isCreated; //0x0000
	char pad_0001[3]; //0x0001
	char* characterName; //0x0004
	char pad_0008[46]; //0x0008
	uint8_t classId; //0x0036 0 = base, 1 = swordie, 2 = archer, 3 = mage, 4 = fighter
	char pad_0037[117]; //0x0037
}; //Size: 0x00AC
static_assert(sizeof(CharacterInfo) == 0xAC);

class TCharacterSelectWidget : public TLBSWidget
{
public:
	static TCharacterSelectWidget* getInstance();

	void setSelectedIndex(uint8_t i);

	void clickStartButton();

	void clickCharacterButton(int i);

protected:
	static constexpr int MAX_CHARACTERS = 4;

	char pad_0024[76]; //0x0024
	uint8_t selectedIndex; //0x0068
	char pad_0069[3]; //0x0069
	uint32_t numberOfCharacters; //0x006C
	char pad_0070[12]; //0x0070
	CharacterInfo charactersInformation[MAX_CHARACTERS]; //0x007C
	uint32_t N00000245; //0x032C TNTIconViewer / pet
	uint32_t N00000246; //0x0330 TNTIconViewer / pet
	uint32_t N00000247; //0x0334 TNTIconViewer / pet
	uint32_t N00000248; //0x0338 TNTIconViewer / pet
	uint32_t N00000249; //0x033C TNTIconViewer / pet
	uint32_t N0000024A; //0x0340 TNTIconViewer / pet
	uint32_t N0000024B; //0x0344 TNTIconViewer / pet
	uint32_t N0000024C; //0x0348 TNTIconViewer / pet
	uint32_t N0000024D; //0x034C TNTIconViewer / pet
	uint32_t N0000024E; //0x0350 TNTIconViewer / pet
	uint32_t N0000024F; //0x0354 TNTIconViewer / pet
	uint32_t N00000250; //0x0358 TNTIconViewer / pet
	uint32_t N00000251; //0x035C TNTIconViewer / pet
	uint32_t N00000252; //0x0360 TNTIconViewer / selected character icon (middle of screen)
	uint32_t N00000253; //0x0364 TCharacterNewWidget ptr
	uint32_t N00000254; //0x0368 TCharacterDeleteWidget ptr
	uint32_t N00000255; //0x036C TCharacterRenameWidget ptr
	uint32_t N00000256; //0x0370 TEWGraphicButtonWidget ptr / Some nostale events button
	uint32_t N00000257; //0x0374 TNewDialogBoxWidget ptr / Used for events
	uint32_t N00000258; //0x0378 TEWLabels ptr / first character level
	uint32_t N00000259; //0x037C TEWLabels ptr / second character level
	uint32_t N0000025A; //0x0380 TEWLabels ptr / third character level
	uint32_t N0000025B; //0x0384 TEWLabels ptr / fourth character level
	TEWButtonWidget* characterButtons[MAX_CHARACTERS]; //0x0388 TEWButtonWidget / the 4 top character buttons
	uint32_t N0000089C; //0x0398 TNTIconViewer / first character icon
	uint32_t N00000261; //0x039C TNTIconViewer / second character icon
	uint32_t N00000262; //0x03A0 TNTIconViewer / third character icon
	uint32_t N00000263; //0x03A4 TNTIconViewer / fourth character icon
	uint32_t N00000264; //0x03A8 TEWControlWidgetEX / first character (no char widget)
	uint32_t N00000265; //0x03AC TEWControlWidgetEX / second character (no char widget)
	uint32_t N00000266; //0x03B0 TEWControlWidgetEX / third character (no char widget)
	uint32_t N00000267; //0x03B4 TEWControlWidgetEX / fourth character (no char widget)
	uint32_t N00000268; //0x03B8 TLBSWidget / Top main widget
	TLBSWidget* bottomMenu; //0x03BC TLBSWidget / Bottom menu widget (Select server, Start, Delete character) TEWGraphicButtonWidget
}; //Size: 0x03C8
static_assert(sizeof(TCharacterSelectWidget) == 0x3C8);
