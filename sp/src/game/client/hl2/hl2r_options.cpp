#include "cbase.h"
using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Button.h>
#include "ienginevgui.h"
#include "hl2r_options.h"
//------------------------------------------------------------------------------
// Purpose : HL2R's Game options page
//------------------------------------------------------------------------------
CSubOptionsGameHL2R::CSubOptionsGameHL2R(vgui::Panel* parent) : PropertyPage(parent, NULL)
{
 	for ( int i = 0; i < ARRAYSIZE(g_NewButton); i++ )
	{
		g_NewButton[i].Button = new CheckButton(this, g_NewButton[i].Name, "");
		
		ConVarRef var( g_NewButton[i].pConVar );
		g_NewButton[i].Button->SetSelected(var.GetBool());
	}
	
	toggleAllButton = new Button(this, "toggleAllButton", "");
	toggleAllButton->SetCommand("toggleall");

	LoadControlSettings("resource/ui/hl2r_optionssubgame.res");
}

void CSubOptionsGameHL2R::OnApplyChanges()
{
	for ( int i = 0; i < ARRAYSIZE(g_NewButton); i++ )
	{
		ConVarRef var( g_NewButton[i].pConVar );
		var.SetValue( g_NewButton[i].Button->IsSelected());
	}
}

void CSubOptionsGameHL2R::OnCheckButtonChecked(Panel* panel)
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

void CSubOptionsGameHL2R::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);
	if (!Q_stricmp(pcCommand, "toggleall"))
	{
		bState = true;
		
		for ( int i = 0; i < ARRAYSIZE(g_NewButton); i++ )
		{
			if ( g_NewButton[i].Button->IsSelected() )
				bState = false;
		}
		
		for ( int i = 0; i < ARRAYSIZE(g_NewButton); i++ )
		{
			g_NewButton[i].Button->SetSelected(bState);
			
			ConVarRef var( g_NewButton[i].pConVar );
			var.SetValue( bState );
		}
	}
}
//------------------------------------------------------------------------------
// Purpose : HL2R's Visual options page
//------------------------------------------------------------------------------
/* CSubOptionsVisualsHL2R::CSubOptionsVisualsHL2R(vgui::Panel* parent) : PropertyPage(parent, NULL)
{
	LoadControlSettings("resource/ui/hl2r_optionssubvisuals.res");
} */
//------------------------------------------------------------------------------
// Purpose : HL2R's Options panel
//------------------------------------------------------------------------------
CHL2RMenu::CHL2RMenu(vgui::VPANEL parent) : BaseClass(NULL, "HL2RMenu")
{
	SetDeleteSelfOnClose(true);
	SetBounds(0, 0, 406, 768);
	SetSizeable(false);
	MoveToCenterOfScreen();

	SetTitle("#hl2r_options_title", true);

	m_pSubOptionsGameHL2R = new CSubOptionsGameHL2R(this);
	AddPage(m_pSubOptionsGameHL2R, "#hl2r_options_game");

/* 	m_pSubOptionsEffectsHL2R = new CSubOptionsVisualsHL2R(this);
	AddPage(m_pSubOptionsEffectsHL2R, "#hl2r_options_visuals"); */
	
	SetApplyButtonVisible(true);
	GetPropertySheet()->SetTabWidth(168);
}

void CHL2RMenu::Activate()
{
	BaseClass::Activate();
	EnableApplyButton(false);
}

void CHL2RMenu::OnThink()
{
	BaseClass::OnThink();
	if (engine->IsPaused() || engine->IsLevelMainMenuBackground() || !engine->IsInGame())
		SetVisible(true);
	else
		SetVisible(false);
}

bool isHL2RActive = false;

void CHL2RMenu::OnClose()
{
	BaseClass::OnClose();
	isHL2RActive = false;
}

CHL2RMenu* HL2RMenu = NULL;

CON_COMMAND(EnableHL2RPanel, "Turns on the HL2R options Panel")
{
	VPANEL gameToolParent = enginevgui->GetPanel(PANEL_CLIENTDLL_TOOLS);
	if (!isHL2RActive)
	{
		HL2RMenu = new CHL2RMenu(gameToolParent);
		isHL2RActive = true;
	}

	HL2RMenu->Activate();
}
