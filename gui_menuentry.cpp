//---------------------------------------------------------------------------
//
// Name:        gui_menuentry.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUIMenuEntry class implementation
//
//---------------------------------------------------------------------------
#include "gui_menuentry.h"

#include <mss.h>


//---------------------------------------------------------------------------
// Costruttore della classe
//---------------------------------------------------------------------------
GUIMenuEntry::GUIMenuEntry( const char* label )
{
	_subMenu = 0;
	_callback = 0;
	_exitcallback = 0;
	_mode = MENU_ENTRY_STYLE_DEFAULT;

	int len = strlen(label);
	_label = new char[len+1];
	strcpy( _label, label );

	//get hotkey da label
	_hotKey = 0;
	for( int i = 0; i < len-1; i++ )
	{
		if( _label[i] == '~' )
		{
			_hotKey = _label[i+1];
			if( _hotKey >= 'a' && _hotKey <= 'z' ) // converte ASCII minusc. in maiuscolo
				_hotKey = _hotKey + 'A' - 'a';
			break;
		}
	}
}

//---------------------------------------------------------------------------
// Distruttore della classe
//---------------------------------------------------------------------------
GUIMenuEntry::~GUIMenuEntry()
{
	if( _label )
		delete [] _label;
}
