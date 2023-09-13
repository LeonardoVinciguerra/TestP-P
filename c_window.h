//---------------------------------------------------------------------------
// Name:        c_window.h
// Author:      Gabriel Ferri
// Created:     15/11/2011
// Description: CWindow class definition
//---------------------------------------------------------------------------

#ifndef __C_WINDOW_H
#define __C_WINDOW_H

#include "gui_window.h"
#include "gui_functions.h"
#include "q_cost.h"

// Window style
#define WIN_STYLE_CENTERED_X    0x0100
#define WIN_STYLE_CENTERED_Y    0x0200
#define WIN_STYLE_CENTERED      WIN_STYLE_CENTERED_X | WIN_STYLE_CENTERED_Y
#define WIN_STYLE_NO_MENU       0x0400
#define WIN_STYLE_NO_EDIT       0x0800
#define WIN_STYLE_EDITMODE_ON   0x1000

// Window exit code
#define WIN_EXITCODE_ESC        0
#define WIN_EXITCODE_ENTER      1


class CWindow : public GUIWindow
{
public:
	CWindow( CWindow* parent );
	CWindow( CWindow* parent, const RectI& clientAreaText, const std::string& title = "", unsigned int style = WIN_STYLE_DEFAULT | WIN_STYLE_CENTERED, int InitAlarm = NO_ALARM );
	CWindow( int x1, int y1, int x2, int y2, const std::string& title = "", int InitAlarm = NO_ALARM );
	~CWindow();

	void SetClientArea( const RectI& clientAreaText ) { client_area_text = clientAreaText; }
	void SetClientAreaPos( int x, int y ) { client_area_text.X = x; client_area_text.Y = y; }
	void SetClientAreaSize( int w, int h ) { client_area_text.W = w; client_area_text.H = h; }

	void Show( bool select = true );
	void Hide();

	void DrawText( int x, int y, const char* buf );
	void DrawText( int x, int y, const char* buf, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor );

	void DrawTextCentered( int y, const char* buf );
	void DrawTextCentered( int y, const char* buf, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor );
	void DrawTextCentered( int x1, int x2, int y, const char* buf );
	void DrawTextCentered( int x1, int x2, int y, const char* buf, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor );

	void DrawTextF( int x, int y, int centered, const char* fmt, ... );
	void DrawTextF( int x, int y, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor, int centered, const char* fmt, ... );

	void DrawSubTitle( int y, const char* text );

	void DrawPanel( const RectI& area );
	void DrawPanel( const RectI& area, const GUI_color& in, const GUI_color& out );

	void DrawGroup( const RectI& area, const char* text );
	void DrawGroup( const RectI& area, const char* text, const GUI_color& bg, const GUI_color& fg );

	void DrawPie( const PointI& center, int radius, int degree );

	void DrawRectangle( const RectI& area, const GUI_color& color );

protected:
	RectI client_area_text;
	CWindow* win_parent;
	int Alarm;        // Livello di allarme della finestra
};

#endif
