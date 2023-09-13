//---------------------------------------------------------------------------
//
// Name:        gui_combo.cpp
// Author:      Gabriel Ferri
// Created:     25/01/2012
// Description: GUICombo class implementation
//
//---------------------------------------------------------------------------
#include "gui_combo.h"

#include "gui_defs.h"

#include "q_graph.h"

#include <mss.h>


//---------------------------------------------------------------------------
// Costruttore
//---------------------------------------------------------------------------
GUICombo::GUICombo( const char* label, int inputWidth )
{
	if( label )
		m_label = label;
	else
		m_label = "";

	m_inputWidth = inputWidth;
}

//--------------------------------------------------------------------------
// Distruttore
//--------------------------------------------------------------------------
GUICombo::~GUICombo()
{
}

//---------------------------------------------------------------------------
// Visualizza la combo
//---------------------------------------------------------------------------
void GUICombo::Show( int x, int y, GUI_color bg )
{
	GUI_Freeze();

	if( m_label.size() )
	{
		GUI_DrawText( x, y, m_label.c_str(), GUI_DefaultFont, bg, GUI_color( WIN_COL_TXT ) );
	}

	RectI rect_in( x + TextToGraphX( m_label.size() + 1 ) - 1, y + 2, TextToGraphX( m_inputWidth ) + 2, GUI_CharH() - 3 );
	RectI rect_out( rect_in.X-CB_EDIT_BORDER, rect_in.Y-CB_EDIT_BORDER, rect_in.W+CB_EDIT_BORDER+1, rect_in.H+CB_EDIT_BORDER+1 );

	GUI_FillRect( rect_out, GUI_color( CB_COL_EDIT_BORDER ) );
	GUI_FillRect( rect_in, GUI_color( CB_COL_EDIT_BG ) );

	GUI_Thaw();
}

//---------------------------------------------------------------------------
// Ritorna la lunghezza del testo della combo
//---------------------------------------------------------------------------
unsigned int GUICombo::GetLabelSize()
{
	return m_label.size();
}
