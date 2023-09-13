//---------------------------------------------------------------------------
//
// Name:        c_combo.h
// Author:      Gabriel Ferri
// Created:     25/01/2012
// Description: C_Combo class definition
//
//---------------------------------------------------------------------------

#ifndef __C_COMBO_H
#define __C_COMBO_H

#include "gui_combo.h"
#include "c_cell.h"


class CComboList;


class C_Combo : public GUICombo, public CCell
{
public:
	C_Combo( int x, int y, const char* label, int editLen, int type, char style = CELL_STYLE_DEFAULT, int dec_digit = 3 );
	~C_Combo();

	void SetParent( CComboList* parent ) { m_clist_parent = parent; }

	void SetEditColor( GUI_color fg, GUI_color bg );
	void SetNorColor( GUI_color fg, GUI_color bg );
	void SetSelectedColor( GUI_color fg, GUI_color bg );
	void SetEditColor();
	void SetNorColor();
	void SetSelectedColor();
	void SetTxtBkgColor( GUI_color bg );

	void Show( int winX, int winY );

protected:
	int X, Y;     // posizione della combo (text)
	GUI_color txtBkgColor;

	CComboList* m_clist_parent;
};

#endif
