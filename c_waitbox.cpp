//---------------------------------------------------------------------------
//
// Name:        c_waitbox.cpp
// Author:      Gabriel Ferri
// Created:     06/06/2012
// Description: CWaitBox class implementation
//
//---------------------------------------------------------------------------
#include "c_waitbox.h"

#include <mss.h>

//---------------------------------------------------------------------------
// Costruttore/Distruttore
//---------------------------------------------------------------------------
CWaitBox::CWaitBox( CWindow* parent, int y, const char* label, int maxValue )
: CWindow( parent )
{
	m_label = label;

	if( y == -1 )
	{
		SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
	}
	else
	{
		SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU );
		SetClientAreaPos( 0, y );
	}

	int len = strlen( label );
	len = MAX( 40, len + 8 );
	SetClientAreaSize( len, 5 );

	m_progBar = new GUI_ProgressBar( this, RectI( 2*GUI_CharW(), 3*GUI_CharH(), (len-4)*GUI_CharW(), GUI_CharH() ) );
	m_progBar->SetVMinMax( 0, maxValue );
}

//---------------------------------------------------------------------------
// Distruttore della classe
//---------------------------------------------------------------------------
CWaitBox::~CWaitBox()
{
	delete m_progBar;
}

//---------------------------------------------------------------------------
// Crea e visualizza la finestra
//---------------------------------------------------------------------------
void CWaitBox::Show()
{
	GUI_Freeze_Locker lock;

	// visualizza finestra
	CWindow::Show( false );

	DrawTextCentered( 1, m_label.c_str() );
	m_progBar->Show();
}
