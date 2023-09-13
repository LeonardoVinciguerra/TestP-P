//---------------------------------------------------------------------------
//
// Name:        gui_mainmenu.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_MainMenu class definition
//              Gestione del menu principale
//
//---------------------------------------------------------------------------

#ifndef __GUI_MAINMENU_H
#define __GUI_MAINMENU_H

#include "gui_menuentry.h"
#include <vector>

using namespace std;

#define MM_MODE_SQUARE		1


class GUI_MainMenu
{
	public:
		GUI_MainMenu();
		~GUI_MainMenu();
		
		void Show();
		void SelPrev();
		void SelNext();
		void Add( GUIMenuEntry* entry );
		void Activate( int pos );
		void GestKey( int c );

	protected:
		int search( int key );
		void drawEntry( int i, bool highlighted );
		bool onSelect();
		
		int curPos;
		vector<GUIMenuEntry*> entries;
};

#endif
