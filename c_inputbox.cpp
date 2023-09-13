//---------------------------------------------------------------------------
//
// Name:        c_inputbox.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: CInputBox class implementation
//
//---------------------------------------------------------------------------
#include "c_inputbox.h"

#include "msglist.h"
#include "keyutils.h"
#include "gui_defs.h"

#include <mss.h>


//---------------------------------------------------------------------------
// Costruttore/Distruttore
//---------------------------------------------------------------------------
CInputBox::CInputBox( CWindow* parent, int y, const char* title, const char* label, int editLen, int type, int dec_digit )
: CWindowParams( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU | WIN_STYLE_EDITMODE_ON );
	SetClientAreaPos( 0, y );

	int w1 = strlen( label ) + editLen + 8;
	int w2 = strlen( title ) + 8;
	SetClientAreaSize( MAX( w1, w2 ), 9 );

	SetTitle( title );

	m_combos[0] = new C_Combo( 3, 2, label, editLen, type, CELL_STYLE_DEFAULT, dec_digit );

	m_exitCode = WIN_EXITCODE_ESC;
}

//---------------------------------------------------------------------------
// Visualizza inputbox
//---------------------------------------------------------------------------
void CInputBox::Show( bool select, bool focused )
{
	CWindowParams::Show( select, focused );
	CWindowParams::Hide();
}

//---------------------------------------------------------------------------
// Inizializzazione
//---------------------------------------------------------------------------
void CInputBox::onInit()
{
	// add to combo list
	m_comboList->Add( m_combos[0], 0, 0 );
}

//---------------------------------------------------------------------------
// Visualizzazione
//---------------------------------------------------------------------------
void CInputBox::onShow()
{
	DrawPanel( RectI( 2, 4, GetW()/GUI_CharW() - 4, 4 ) );
	DrawTextCentered( 5, MsgGetString(Msg_00296), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	DrawTextCentered( 6, MsgGetString(Msg_00297), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
}

//---------------------------------------------------------------------------
// Gestione tasti
//---------------------------------------------------------------------------
bool CInputBox::onKeyPress( int key )
{
	switch( key )
	{
		case K_ESC:
			m_exitCode = WIN_EXITCODE_ESC;
			break;

		case K_ENTER:
			forceExit();
			m_exitCode = WIN_EXITCODE_ENTER;
			return true;

		default:
			break;
	}

	return false;
}

//---------------------------------------------------------------------------
// Chiusura
//---------------------------------------------------------------------------
void CInputBox::onClose()
{
}
