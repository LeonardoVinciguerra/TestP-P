//---------------------------------------------------------------------------
//
// Name:        gui_submenu.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_SubMenu class definition
//
//---------------------------------------------------------------------------

#ifndef __GUI_SUBMENU_H
#define __GUI_SUBMENU_H

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <vector>
#include "gui_window.h"
#include "gui_menuentry.h"
#include "mathlib.h"

#define SM_GRAYED    1


class GUI_SubMenu : public GUIWindow
{
public:
	GUI_SubMenu( int x = 0, int y = 0, GUI_SubMenu* parent = 0 );
	~GUI_SubMenu();

	void SetPos( int x, int y ) { client_area_text.X  = x; client_area_text.Y  = y; }
	void SetParent( GUI_SubMenu* parent ) { menu_parent = parent; }

	void Add( const char* label, int hotkey = 0, int mode = 0, GUI_SubMenu* subMenu = 0, GUIMenuCallback callback = 0, GUIMenuCallback exitcallback = 0 );

	int GetWidth();
	int GetHeight();

	int Show();
	void HideAll();

	bool ManageKey( int key );

	int GetSelectionIndex(void) { return curPos; }

protected:
	virtual bool onSelect();

	int curPos;  // posizione corrente del menu
	std::vector<GUIMenuEntry*> entries;

private:
	int search( int key );
	void drawEntry( int i, bool highlight );
	void calGUI_SubMenuPosition( GUI_SubMenu* sm, int& x, int& y );
	int checkHotkey( int hotkey );

	RectI client_area_text;
	GUI_SubMenu* menu_parent;

	int maxTxtLen;
};

#endif
