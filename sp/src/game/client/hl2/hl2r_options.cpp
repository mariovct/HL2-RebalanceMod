#include "cbase.h"
using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Slider.h>
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
		bool bState = true;
		
		for ( int i = 0; i < ARRAYSIZE(g_NewButton); i++ )
		{
			if ( g_NewButton[i].Button->IsSelected() )
				bState = false;
		}
		
		for ( int i = 0; i < ARRAYSIZE(g_NewButton); i++ )
		{
			g_NewButton[i].Button->SetSelected(bState);
		}
	}
}
//------------------------------------------------------------------------------
// Purpose : HL2R's Misc options page
//------------------------------------------------------------------------------
CSubMiscOptionsHL2R::CSubMiscOptionsHL2R(vgui::Panel* parent) : PropertyPage(parent, NULL)
{
	for ( int i = 0; i < ARRAYSIZE(g_NewBox); i++ )
	{
		g_NewBox[i].Box = new ComboBox(this, g_NewBox[i].Name, MAX_BOX_ELEMENTS, false);
		
		for ( int j = 0; g_NewBox[i].ElementNames[j] != NULL; j++ )
		{
			g_NewBox[i].Box->AddItem( g_NewBox[i].ElementNames[j], NULL );
		}
		
		ConVarRef var( g_NewBox[i].pConVar );
		
		int iValue = GetElementIndex( g_NewBox[i], var.GetInt() );
		g_NewBox[i].Box->ActivateItem( iValue );
	}
	
	viewRollSlider = new Slider(this, "viewRollSlider");
	viewRollSlider->SetRange(0, 10); viewRollSlider->SetNumTicks(10);
	
	ConVarRef var( "hl2r_rollangle" );
	viewRollSlider->SetValue(var.GetFloat());
		
	LoadControlSettings("resource/ui/hl2r_optionssubmisc.res");
} 

void CSubMiscOptionsHL2R::OnApplyChanges()
{
	for ( int i = 0; i < ARRAYSIZE(g_NewBox); i++ )
	{
		int iValue = GetElementIndex( g_NewBox[i], g_NewBox[i].Box->GetActiveItem() );
		
		ConVarRef var( g_NewBox[i].pConVar );
		var.SetValue(iValue);
	}
	
	ConVarRef var( "hl2r_rollangle" );
	var.SetValue(viewRollSlider->GetValue());
}

void CSubMiscOptionsHL2R::OnControlModified()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

int CSubMiscOptionsHL2R::GetElementIndex( BoxButton_t mBox, int iElement )
{
	int element = iElement;
		
	// Handle reversed index's.
	if ( mBox.bReversedIndexes )
	{
		int iNumElements = -1;
		for ( int i = 0; mBox.ElementNames[i] != NULL; i++ )
		{
			iNumElements++;
		}
			
		element = iNumElements - element;
	}
	
	return element;
}
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

 	m_pSubMiscOptionsHL2R = new CSubMiscOptionsHL2R(this);
	AddPage(m_pSubMiscOptionsHL2R, "#hl2r_options_misc");
	
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
