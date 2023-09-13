//---------------------------------------------------------------------------
//
// Name:        c_win_par.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: CWindowParams class implementation
//
//---------------------------------------------------------------------------
#include "c_win_par.h"

#include "msglist.h"
#include "q_oper.h" //SetConfirmRequiredBeforeNextXYMovement
#include "keyutils.h"
#include "gui_desktop.h"
#include "keyutils.h"

#include <mss.h>

extern GUI_DeskTop* guiDeskTop;


//---------------------------------------------------------------------------
// Costruttore della classe
//---------------------------------------------------------------------------
CWindowParams::CWindowParams( CWindow* parent )
: CWindow( parent )
{
	m_menu = 0;
	m_comboList = new CComboList( this );
}

//---------------------------------------------------------------------------
// Distruttore della classe
//---------------------------------------------------------------------------
CWindowParams::~CWindowParams()
{
	for( unsigned int i = 0; i < m_combos.size(); i++ )
	{
		if( m_combos[i] )
		{
			delete m_combos[i];
		}
	}

	if( m_comboList )
	{
		delete m_comboList;
	}
}

//---------------------------------------------------------------------------
// Crea e visualizza la finestra
//---------------------------------------------------------------------------
void CWindowParams::Show( bool select, bool focused )
{
	GUI_Freeze();

	// init classi derivate
	onInit();

	// visualizza dati sulle barre
	ShowCurrentData();

	// visualizza le barre di stato e "pulisce il desktop"
	guiDeskTop->ShowStatusBars( ( GetStyle() & WIN_STYLE_NO_MENU ) ? false : true );

	// visualizza finestra
	CWindow::Show( select );

	// show classi derivate
	onShow();

	// visualizza eventuali controlli
	m_comboList->Show();

	if( (GetStyle() & WIN_STYLE_EDITMODE_ON) || guiDeskTop->GetEditMode() )
	{
		guiDeskTop->SetEditMode( true );
		m_comboList->SetEdit( true );
	}

	// refresh classi derivate
	onRefresh();

	if( !focused )
	{
		m_comboList->CurDeselect();
	}

	GUI_Thaw();

	if( !focused )
	{
		return;
	}

	WorkingCycle();
}

//---------------------------------------------------------------------------
// Nasconde la finestra
//---------------------------------------------------------------------------
void CWindowParams::Hide()
{
	// close classi derivate
	onClose();

	CWindow::Hide();

	// nasconde le barre di stato
	guiDeskTop->HideStatusBars();
}

//---------------------------------------------------------------------------
// Seleziona la finestra
//---------------------------------------------------------------------------
void CWindowParams::Select()
{
	CWindow::Select();

	// visualizza le barre di stato e "pulisce il desktop"
	guiDeskTop->ShowMenuItem( ( GetStyle() & WIN_STYLE_NO_MENU ) ? false : true );
}

//---------------------------------------------------------------------------
// Setta il fuoco sulla finestra
//---------------------------------------------------------------------------
void CWindowParams::SetFocus()
{
	// deselect parent if any
	if( win_parent )
	{
		win_parent->Deselect();
	}
	Select();

	WorkingCycle();

	Deselect();
	if( win_parent )
	{
		win_parent->Select();
	}
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void CWindowParams::SelectFirstCell()
{
	if( m_comboList )
	{
		m_comboList->SelectFirst();
	}
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void CWindowParams::DeselectCells()
{
	if( m_comboList )
	{
		m_comboList->CurDeselect();
	}
}

//---------------------------------------------------------------------------
// Funzione chiamata al termine dell'edit di una combo
//---------------------------------------------------------------------------
void CWindowParams::ShowMenu()
{
	if( GetStyle() & WIN_STYLE_NO_MENU )
		return;

	m_menu = new CMenuWindowParams( this );
	m_menu->SetPos( 1, 1 );

	if( !(GetStyle() & WIN_STYLE_NO_EDIT) )
	{
		m_menu->Add( MsgGetString(Msg_00247), K_F2 ); // edit
	}

	// show menu classi derivate
	onShowMenu();

	//guiDeskTop->MenuItemActivated( true );

	m_menu->Show();

	delete m_menu;
	m_menu = NULL;

	//guiDeskTop->MenuItemActivated( false );
}

//---------------------------------------------------------------------------
// Funzione gestione ciclo operativo
//---------------------------------------------------------------------------
void CWindowParams::WorkingCycle()
{
	m_forceExit = false;

	while( !m_forceExit )
	{
		SetConfirmRequiredBeforeNextXYMovement(true);

		int c = Handle( false );
		if( c != 0 )
		{
			if( onKeyPress( c ) )
			{
				// refresh classi derivate
				GUI_Freeze();
				onRefresh();
				GUI_Thaw();
			}
			else
			{
				switch( c )
				{
					case K_TAB: // open menu
					case K_ALT_M:
						ShowMenu();

						// refresh classi derivate
						GUI_Freeze();
						onRefresh();
						GUI_Thaw();
						break;

					case K_F2: // edit
						if( (GetStyle() & WIN_STYLE_NO_MENU) || (GetStyle() & WIN_STYLE_NO_EDIT) )
							break;

						guiDeskTop->SetEditMode( true );
						m_comboList->SetEdit( true );
						break;

					case K_ESC: // exit
						m_forceExit = true;
						break;

					default:
						if( m_comboList->ManageKey( c ) )
						{
							// edit classi derivate
							onEdit();

							// refresh classi derivate
							GUI_Freeze();
							onRefresh();
							GUI_Thaw();
						}
						break;
				}
			}
		}

		onIdle();
	}
}



	//-------------------//
	//    class CMenu    //
	//-------------------//

bool CMenuWindowParams::onSelect()
{
	if( entries[curPos]->GetHotKey() == K_F2 && !(m_parent->GetStyle() & WIN_STYLE_NO_EDIT) )
	{
		HideAll();
		guiDeskTop->SetEditMode( true );
		m_parent->m_comboList->SetEdit( true );
		return true;
	}

	if( GUI_SubMenu::onSelect() )
	{
		// refresh classi derivate
		GUI_Freeze();
		m_parent->onRefresh();
		GUI_Thaw();
		return true;
	}

	return false;
}
