//---------------------------------------------------------------------------
//
// Name:        gui_combo.h
// Author:      Gabriel Ferri
// Created:     25/01/2012
// Description: GUICombo class definition
//
//---------------------------------------------------------------------------

#ifndef __GUI_COMBO_H
#define __GUI_COMBO_H

#include <string>
#include "gui_functions.h"

class GUICombo
{
public:
	GUICombo( const char* label, int inputWidth );
	~GUICombo();

	void Show( int x, int y, GUI_color bg );

	unsigned int GetLabelSize();

protected:
	std::string m_label;
	int m_inputWidth;
};

#endif
