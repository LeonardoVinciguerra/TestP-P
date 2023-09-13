//---------------------------------------------------------------------------
//
// Name:        c_combo.cpp
// Author:      Gabriel Ferri
// Created:     25/01/2012
// Description: C_Combo class implementation
//
//---------------------------------------------------------------------------
#include "c_combo.h"

#include "q_grcol.h"
#include "c_combolist.h"

#include "gui_defs.h"
#include "gui_functions.h"

#include <mss.h>


//---------------------------------------------------------------------------
// Costruttore
//
// INPUT:	x,y: posizione nella finestra (text)
//			label: Titolo della combo
//			editLen: Numero massimo di caratteri nella casella di input
//			type: Tipo della combo box
//			style: Stile della combo box
//			dec_digit: Numero di cifre decimali (default = 3)
//---------------------------------------------------------------------------
C_Combo::C_Combo( int x, int y, const char* label, int editLen, int type, char style, int dec_digit )
: GUICombo( label, editLen ), CCell( "", editLen, type, style )
{
	X = x;
	Y = y;

	SetDecimals( dec_digit );

	txtBkgColor = GUI_color(WIN_COL_CLIENTAREA);

	m_clist_parent = NULL;
}

//--------------------------------------------------------------------------
// Distruttore
//--------------------------------------------------------------------------
C_Combo::~C_Combo()
{
}

//---------------------------------------------------------------------------
// Visualizza la combo
//---------------------------------------------------------------------------
void C_Combo::Show( int winX, int winY )
{
	int x = winX + TextToGraphX( X );
	int y = winY + TextToGraphY( Y );

	GUICombo::Show( x, y, txtBkgColor );

	int X1 = x + TextToGraphX( GetLabelSize() + 1 ) - 1;
	int Y1 = y + 1;

	CCell::Show( X1+1, Y1+1 );
}

//--------------------------------------------------------------------------
// Setta il colore per il modo edit on
//--------------------------------------------------------------------------
void C_Combo::SetEditColor( GUI_color fg, GUI_color bg)
{
	fgEditColor = fg;
	bgEditColor = bg;
}

//--------------------------------------------------------------------------
// Setta a default il colore per il modo edit on
//--------------------------------------------------------------------------
void C_Combo::SetEditColor()
{
	bgEditColor = GUI_color( GEN_COL_BG_EDITABLE );
	fgEditColor = GUI_color( GEN_COL_FG_EDITABLE );
}

//--------------------------------------------------------------------------
// Setta il colore per il modo normale (combo non selezionato)
//--------------------------------------------------------------------------
void C_Combo::SetNorColor( GUI_color fg, GUI_color bg)
{
	fgColor = fg;
	bgColor = bg;
}

//--------------------------------------------------------------------------
// Setta a default il colore per il modo normale (combo non selezionato)
//--------------------------------------------------------------------------
void C_Combo::SetNorColor()
{
	bgColor = GUI_color( GR_WHITE );
	fgColor = GUI_color( GR_BLACK );
}

//--------------------------------------------------------------------------
// Setta il colore per il modo selezionato (no edit)
//--------------------------------------------------------------------------
void C_Combo::SetSelectedColor( GUI_color fg, GUI_color bg)
{
	fgSelColor = fg;
	bgSelColor = bg;
}

//--------------------------------------------------------------------------
// Setta a default il colore per il modo selezionato (no edit)
//--------------------------------------------------------------------------
void C_Combo::SetSelectedColor(void)
{
	bgSelColor = GUI_color( GEN_COL_BG_SELECTED );
	fgSelColor = GUI_color( GEN_COL_FG_SELECTED );
}

//--------------------------------------------------------------------------
// Setta il colore del background del testo
//--------------------------------------------------------------------------
void C_Combo::SetTxtBkgColor( GUI_color bg )
{
	txtBkgColor = bg;
}
