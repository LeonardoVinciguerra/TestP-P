//---------------------------------------------------------------------------
//
// Name:        gui_menuentry.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUIMenuEntry class definition
//
//---------------------------------------------------------------------------

#ifndef __GUI_MENUENTRY_H
#define __GUI_MENUENTRY_H

#include <boost/function.hpp>
#include <boost/bind.hpp>


#define MENU_ENTRY_STYLE_DEFAULT    0
#define MENU_ENTRY_STYLE_GRAYED     1


typedef boost::function<int(void)> GUIMenuCallback;


class GUIMenuEntry
{
public:
	GUIMenuEntry( const char* label );
	~GUIMenuEntry();

	const char* GetLabel() { return _label; }

	void SetHotKey( int key ) { _hotKey = key; }
	int GetHotKey() { return _hotKey; }

	void SetSubMenu( void* menu ) { _subMenu = menu; }
	void* GetSubMenu() { return _subMenu; }

	void SetCallback( GUIMenuCallback func ) { _callback = func; }
	GUIMenuCallback GetCallback() { return _callback; }

	void SetExitCallback( GUIMenuCallback func ) { _exitcallback = func; }
	GUIMenuCallback GetExitCallback() { return _exitcallback; }

	void SetPos( int x, int y ) { _x = x; _y = y; }
	int GetPosX() { return _x; }
	int GetPosY() { return _y; }

	void SetMode( int mode ) { _mode = mode; }
	int GetMode() { return _mode; }

private:
	char* _label;
	int _hotKey;
	void* _subMenu;
	GUIMenuCallback _callback;
	GUIMenuCallback _exitcallback;
	int _x, _y;
	int _mode;
};

#endif
