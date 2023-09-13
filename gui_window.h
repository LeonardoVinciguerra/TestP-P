//---------------------------------------------------------------------------
// Name:        gui_window.h
// Author:      Gabriel Ferri
// Created:     15/11/2011
// Description: GUIWindow class definition
//---------------------------------------------------------------------------

#ifndef __GUI_WINDOW_H
#define __GUI_WINDOW_H

#include <string>
#include "gui_object.h"
#include "mathlib.h"


// Window style
#define WIN_STYLE_DEFAULT       0x0000
#define WIN_STYLE_NO_CAPTION    0x0001


class GUIWindow : public GUIObject
{
public:
	GUIWindow();
	GUIWindow( const RectI& clientArea, const std::string& title = "", unsigned int style = WIN_STYLE_DEFAULT );
	~GUIWindow();

	void SetTitle( const std::string& title );

	void SetStyle( unsigned int style ) { win_style = style; }
	unsigned int GetStyle() { return win_style; }

	void Show( bool select = true );
	void Hide( bool saveForeground = false );
	bool IsShown() { return is_shown; }

	virtual void Select();
	void Deselect();
	bool IsSelected() { return is_selected; }

	int GetX() { return client_area.X; }
	int GetY() { return client_area.Y; }
	int GetW() { return client_area.W; }
	int GetH() { return client_area.H; }

private:
	void draw();
	void drawCaption( bool select );

	std::string win_title;
	unsigned int win_style;

	int bg_buf_index;
	int fg_buf_index;
	bool is_shown;
	bool is_selected;
};

#endif
