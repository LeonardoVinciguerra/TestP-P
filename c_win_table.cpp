//---------------------------------------------------------------------------
// Name:        c_win_table.cpp
// Author:      Gabriel Ferri
// Created:     18/11/2011
// Description: CWindowTable class implementation
//---------------------------------------------------------------------------
#include "c_win_table.h"

#include "msglist.h"
#include "q_oper.h" //SetConfirmRequiredBeforeNextXYMovement
#include "keyutils.h"
#include "gui_desktop.h"

#include <mss.h>

extern GUI_DeskTop* guiDeskTop;


//---------------------------------------------------------------------------
// Costruttore della classe
//---------------------------------------------------------------------------
CWindowTable::CWindowTable( CWindow* parent )
: CWindow( parent )
{
	m_menu = 0;
	m_table = 0;
}

//---------------------------------------------------------------------------
// Distruttore della classe
//---------------------------------------------------------------------------
CWindowTable::~CWindowTable()
{
	if( m_table )
	{
		delete m_table;
	}
}

//---------------------------------------------------------------------------
// Crea e visualizza la finestra
//---------------------------------------------------------------------------
void CWindowTable::Show( bool select, bool focused )
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
	m_table->Show();

	if( (GetStyle() & WIN_STYLE_EDITMODE_ON) || guiDeskTop->GetEditMode() )
	{
		guiDeskTop->SetEditMode( true );
		m_table->SetEdit( true );
	}

	// refresh classi derivate
	onRefresh();

	GUI_Thaw();

	if( !focused )
		return;

	WorkingCycle();
}

//---------------------------------------------------------------------------
// Nasconde la finestra
//---------------------------------------------------------------------------
void CWindowTable::Hide()
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
void CWindowTable::Select()
{
	CWindow::Select();

	// visualizza le barre di stato e "pulisce il desktop"
	guiDeskTop->ShowMenuItem( ( GetStyle() & WIN_STYLE_NO_MENU ) ? false : true );
}

//---------------------------------------------------------------------------
// Setta il fuoco sulla finestra
//---------------------------------------------------------------------------
void CWindowTable::SetFocus()
{
	GUI_Freeze();
	// deselect parent if any
	if( win_parent )
	{
		win_parent->Deselect();
	}
	Select();

	if( !(GetStyle() & WIN_STYLE_NO_MENU) && !(GetStyle() & WIN_STYLE_NO_EDIT) )
	{
		m_table->SetEdit( guiDeskTop->GetEditMode() );
	}
	GUI_Thaw();

	WorkingCycle();

	GUI_Freeze();
	Deselect();
	if( win_parent )
	{
		win_parent->Select();
	}
	GUI_Thaw();
}

//---------------------------------------------------------------------------
// Funzione chiamata al termine dell'edit di una combo
//---------------------------------------------------------------------------
void CWindowTable::ShowMenu()
{
	if( GetStyle() & WIN_STYLE_NO_MENU )
		return;

	m_menu = new CMenuWindowTable( this );
	m_menu->SetPos( 1, 1 );

	if( !(GetStyle() & WIN_STYLE_NO_EDIT) )
	{
		m_menu->Add( MsgGetString(Msg_00247), K_F2 ); // edit
	}

	// show menu classi derivate
	onShowMenu();

	m_menu->Show();

	delete m_menu;
	m_menu = NULL;
}

//---------------------------------------------------------------------------
// Funzione gestione ciclo operativo
//---------------------------------------------------------------------------
void CWindowTable::WorkingCycle()
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
						m_table->SetEdit( true );
						break;

					case K_ESC: // exit
						m_forceExit = true;
						break;

					default:
						if( m_table->ManageKey( c ) )
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

bool CMenuWindowTable::onSelect()
{
	if( entries[curPos]->GetHotKey() == K_F2 && !(m_parent->GetStyle() & WIN_STYLE_NO_EDIT) )
	{
		HideAll();
		guiDeskTop->SetEditMode( true );
		m_parent->m_table->SetEdit( true );
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
