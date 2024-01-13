#ifndef hl2r_options_h
#define hl2r_options_h
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyPage.h"

struct CheckButton_t
{
	CheckButton	*Button;
	char		Name[32];
	char		pConVar[128];
};

CheckButton* ExplosiveCrabsButton;
CheckButton* FearMovementButton;
CheckButton* ManualPickupButton;
CheckButton* LessAmmoButton;

static CheckButton_t g_NewButton[] =
{
	{ ExplosiveCrabsButton, "ExplosiveCrabsButton", "explosive_crabs" },
	{ FearMovementButton, "FearMovementButton", "fear_style_movement" },
	{ ManualPickupButton, "ManualPickupButton", "manual_pickup" },
	{ LessAmmoButton, "LessAmmoButton", "less_ammo" },
};

// Game page
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
	bool bState;
};

// Effects page
/* class CSubOptionsVisualsHL2R : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CSubOptionsVisualsHL2R, vgui::PropertyPage);

public:
	CSubOptionsVisualsHL2R(vgui::Panel* parent);
	~CSubOptionsVisualsHL2R() {}
}; */

// Main panel
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
	//CSubOptionsVisualsHL2R* m_pSubOptionsEffectsHL2R;
};

#endif