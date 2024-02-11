#ifndef hl2r_options_h
#define hl2r_options_h
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyPage.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CheckButton* ExplosiveCrabsButton;
CheckButton* FearMovementButton;
CheckButton* ShorterSprintButton;
CheckButton* LessAmmoButton;
CheckButton* EnemyPromotionButton;
CheckButton* RandomWeaponsButton;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
struct CheckButton_t
{

	CheckButton	*Button;
	char		Name[32];
	char		pConVar[128];
};

static CheckButton_t g_NewButton[] =
{
	{ ExplosiveCrabsButton, "ExplosiveCrabsButton", "hl2r_explosive_crabs" },
	{ FearMovementButton, "FearMovementButton", "hl2r_fear_style_movement" },
	{ ShorterSprintButton, "ShorterSprintButton", "hl2r_shorter_sprint" },
	{ LessAmmoButton, "LessAmmoButton", "hl2r_less_ammo" },
	{ EnemyPromotionButton, "EnemyPromotionButton", "hl2r_enemy_promotion" },
	{ RandomWeaponsButton, "RandomWeaponsButton", "hl2r_random_weapons" },
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

ComboBox* LightDetailBox;
ComboBox* QuickinfoBox;
ComboBox* EpisodicLightBox;
ComboBox* HudHintBox;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define MAX_BOX_ELEMENTS 5
struct BoxButton_t
{
	ComboBox	*Box;
	char		Name[32];
	char 		*ElementNames[MAX_BOX_ELEMENTS];
	char		pConVar[128];
	
	// Reverses the index VALUE of elements.
	// EG: [0 1 2] becomes: [2 1 0]. Good for convar control.
	bool		bReversedIndexes;
};

static BoxButton_t g_NewBox[] =
{
	{ LightDetailBox, "LightDetailBox", { "#hl2r_default", "#hl2r_less", "#hl2r_none" }, "hl2r_dynamic_light_level", false },
	{ QuickinfoBox, "QuickinfoBox", { "#hl2r_on", "#hl2r_off" }, "hud_quickinfo", true },
	{ EpisodicLightBox, "EpisodicLightBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_episodic_flashlight", true },
	{ HudHintBox, "HudHintBox", { "#hl2r_on", "#hl2r_off" }, "hl2r_hudhints", true },
};
//------------------------------------------------------------------------------
// Game page
//------------------------------------------------------------------------------
class CSubOptionsGameHL2R : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CSubOptionsGameHL2R, vgui::PropertyPage);

public:
	CSubOptionsGameHL2R(vgui::Panel* parent);
	~CSubOptionsGameHL2R() {}

	virtual void OnApplyChanges();

protected:
	virtual void OnCommand(const char* pcCommand);
private:
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel);
	
	Button* toggleAllButton;
};
//------------------------------------------------------------------------------
// Misc page
//------------------------------------------------------------------------------
class CSubMiscOptionsHL2R : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CSubMiscOptionsHL2R, vgui::PropertyPage);

public:
	CSubMiscOptionsHL2R(vgui::Panel* parent);
	~CSubMiscOptionsHL2R() {}
	
	virtual void OnApplyChanges();

private:
	MESSAGE_FUNC( OnControlModified, "ControlModified" );
	MESSAGE_FUNC( OnTextChanged, "TextChanged" )
	{
		OnControlModified();
	}
	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data )
	{
		OnControlModified();
	}
	
	Slider* viewRollSlider;
	
	int GetElementIndex( BoxButton_t mBox, int iElement );
}; 
//------------------------------------------------------------------------------
// Main panel
//------------------------------------------------------------------------------
class CHL2RMenu : public vgui::PropertyDialog
{
	DECLARE_CLASS_SIMPLE(CHL2RMenu, vgui::PropertyDialog);

public:
	CHL2RMenu(vgui::VPANEL parent);
	~CHL2RMenu() {}
	virtual void Activate();

protected:
	virtual void OnThink();
	virtual void OnClose();

private:
	CSubOptionsGameHL2R* m_pSubOptionsGameHL2R;
	CSubMiscOptionsHL2R* m_pSubMiscOptionsHL2R;
};

#endif