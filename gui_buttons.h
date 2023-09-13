//---------------------------------------------------------------------------
//
// Name:        gui_buttons.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUIButtons class definition
//
//---------------------------------------------------------------------------

#ifndef __GUI_BUTTONS_H
#define __GUI_BUTTONS_H

#include <vector>
#include "gui_window.h"
#include "gui_menuentry.h"

using namespace std;


#define BUTTONCONFIRMED 0x8000

class GUIButtons
{
	public:
		GUIButtons( int y, GUIWindow* _parent );
		~GUIButtons();
		
		void AddButton( const char* label, char focus = 0 );
		bool SetFocus( int index, int focus );
		
		void Activate();
		int GestKey( int c );

	protected:
		void refresh();
		int search( int key );
		
		GUIWindow* parent;
		
		int Y;
		
		int activated;
		int cursel;
		int maxTxtLen;
		
		vector<GUIMenuEntry*> buttons;
};

#endif
