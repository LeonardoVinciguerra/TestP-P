//---------------------------------------------------------------------------
// Name:        c_window.cpp
// Author:      Gabriel Ferri
// Created:     15/11/2011
// Description: CWindow class implementation
//---------------------------------------------------------------------------
#include "c_window.h"

#include <stdarg.h>
#include <stdio.h>
#include "gui_functions.h"
#include "gui_defs.h"
#include "q_grcol.h"
#include "q_oper.h"

#include <mss.h>


CWindow::CWindow( CWindow* parent )
: GUIWindow()
{
	win_parent = parent;

	Alarm = NO_ALARM;
}

CWindow::CWindow( CWindow* parent, const RectI& clientAreaText, const std::string& title, unsigned int style, int InitAlarm )
: GUIWindow()
{
	win_parent = parent;
	client_area_text = clientAreaText;

	Alarm = InitAlarm;

	SetTitle( title );
	SetStyle( style );
}

//TODO: togliere quando tutte le finestre sono sistemate
CWindow::CWindow( int x1, int y1, int x2, int y2, const std::string& title, int InitAlarm )
: GUIWindow()
{
	win_parent = 0;

	client_area_text.X = x1;
	client_area_text.Y = y1;
	client_area_text.W = x2-x1+1;
	client_area_text.H = y2-y1+1;

	Alarm = InitAlarm;

	SetTitle( title );
	SetStyle( WIN_STYLE_DEFAULT | WIN_STYLE_CENTERED_X );
}

CWindow::~CWindow()
{
	StopAlarm( Alarm );

	Hide();
}

void CWindow::Show( bool select )
{
	if( !IsShown() )
	{
		StartAlarm( Alarm );

		GUI_Freeze_Locker lock;

		RectI client_area_pix;
		GUI_TextToGraph( client_area_text, client_area_pix );

		if( GetStyle() & WIN_STYLE_CENTERED_X )
		{
			client_area_pix.X = (GUI_ScreenW() - client_area_pix.W) / 2;
		}
		if( GetStyle() & WIN_STYLE_CENTERED_Y )
		{
			client_area_pix.Y = (GUI_ScreenH() - client_area_pix.H) / 2;
		}

		GUIWindow::SetClientArea( client_area_pix );

		// deselect parent if any
		if( win_parent && select )
		{
			win_parent->Deselect();
		}

		GUIWindow::Show( select );
	}
}

void CWindow::Hide()
{
	if( IsShown() )
	{
		GUI_Freeze_Locker lock;

		GUIWindow::Hide();

		// select parent if any
		if( win_parent )
		{
			win_parent->Select();
		}
	}
}



//---------------------------------------------------------------------------
// Visualizza una stringa nella finestra ad una determinata posizione
// INPUT:   x,y     : coordinate relative alla finestra (text)
//          buf     : stringa da stampare
//---------------------------------------------------------------------------
void CWindow::DrawText( int x, int y, const char* buf )
{
	DrawText( x, y, buf, GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_TXT ) );
}

void CWindow::DrawText( int x, int y, const char* buf, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor )
{
	GUI_Freeze_Locker lock;

	std::string tmp = buf;
	bool end = false;

	int count = 0;
	int last_spc_pos = -1;

	for( unsigned int i = 0; i < tmp.size(); i++ )
	{
		if(tmp[i] != '\n')
		{
			count++;
		}

		if(tmp[i] == ' ')
		{
			last_spc_pos = i;
		}

		if( count >= client_area_text.W )
		{
			if(last_spc_pos != -1)
			{
				tmp[last_spc_pos] = '\n';
				i = last_spc_pos + 1;
				last_spc_pos = -1;
				count = 0;
			}
		}
	}

	while(!end)
	{
		std::string str;

		size_t eol_pos = tmp.find('\n');
		if(eol_pos == std::string::npos)
		{
			str = tmp;
			end = true;
		}
		else
		{
			if(eol_pos != 0)
			{
				str = tmp.substr(0,eol_pos);
			}
			else
			{
				y++;
			}

			tmp = tmp.substr(eol_pos+1);
		}

		int X1 = GetX() + TextToGraphX( x );
		int Y1 = GetY() + TextToGraphY( y );

		GUI_DrawText( X1, Y1, str.c_str(), font, bgColor, fgColor );

		y++;
	}
}

void CWindow::DrawTextCentered( int y, const char* buf )
{
	DrawTextCentered( y, buf, GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_TXT ) );
}

void CWindow::DrawTextCentered( int y, const char* buf, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor )
{
	DrawTextCentered( 0, GetW()/GUI_CharW(), y, buf, font, bgColor, fgColor );
}

void CWindow::DrawTextCentered( int x1, int x2, int y, const char* buf )
{
	DrawTextCentered( x1, x2, y, buf, GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_TXT ) );
}

void CWindow::DrawTextCentered( int x1, int x2, int y, const char* buf, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor )
{
	GUI_Freeze_Locker lock;

	std::string tmp = buf;
	bool end = false;

	int count = 0;
	int last_spc_pos = -1;

	for( unsigned int i = 0; i < tmp.size(); i++ )
	{
		if(tmp[i] != '\n')
		{
			count++;
		}

		if(tmp[i] == ' ')
		{
			last_spc_pos = i;
		}

		if( count >= client_area_text.W )
		{
			if(last_spc_pos != -1)
			{
				tmp[last_spc_pos] = '\n';
				i = last_spc_pos + 1;
				last_spc_pos = -1;
				count = 0;
			}
		}
	}

	while(!end)
	{
		std::string str;

		size_t eol_pos = tmp.find('\n');
		if(eol_pos == std::string::npos)
		{
			str = tmp;
			end = true;
		}
		else
		{
			if(eol_pos != 0)
			{
				str = tmp.substr(0,eol_pos);
			}
			else
			{
				y++;
			}

			tmp = tmp.substr(eol_pos+1);
		}

		int X1 = GetX() + TextToGraphX( x1 );
		int X2 = GetX() + TextToGraphX( x2 );
		int Y1 = GetY() + TextToGraphY( y );

		GUI_DrawTextCentered( X1, X2, Y1, str.c_str(), font, bgColor, fgColor );

		y++;
	}
}


//---------------------------------------------------------------------------
// Stampa una stringa formattata nella finestra
// INPUT:   fmt     : formattatore
//          x,y     : coordinate relative alla finestra
//          centered: 1=centrato,0=non centrato
//---------------------------------------------------------------------------
void CWindow::DrawTextF( int x, int y, int centered, const char* fmt, ... )
{
	char buf[1024];
	va_list args;
	va_start( args, fmt );
	vsnprintf( buf, 1024, fmt, args );
	va_end( args );

	DrawText( x, y, buf );
}

//---------------------------------------------------------------------------
// Stampa una stringa formattata nella finestra specificando gli attributi
// INPUT:   fmt     : formattatore
//          x,y     : coordinate relative alla finestra
//          font    : puntatore al tipo font da utilizzare
//          bkColor : colore di sfondo
//          fgColor : colore testo
//          centered: 1=centrato,0=non centrato
//---------------------------------------------------------------------------
void CWindow::DrawTextF( int x, int y, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor, int centered, const char* fmt, ... )
{
	char buf[1024];
	va_list args;
	va_start( args, fmt );
	vsnprintf( buf, 1024, fmt, args );
	va_end( args );

	DrawText( x, y, buf, font, bgColor, fgColor );
}

//---------------------------------------------------------------------------
// Visualizza sottotitolo
//---------------------------------------------------------------------------
void CWindow::DrawSubTitle( int y, const char* text )
{
	if( !text )
		return;

	if( strlen( text ) )
	{
		GUI_Freeze_Locker lock;

		RectI r;
		r.X = GetX();
		r.Y = GetY() + TextToGraphY( y );
		r.W = GetW();
		r.H = GUI_CharH();

		GUI_FillRect( r, GUI_color( WIN_COL_SUBTITLE ) );
		GUI_DrawTextCentered( r.X, r.X+r.W, r.Y, text, GUI_DefaultFont, GUI_color( WIN_COL_TXT_SUBTITLE ) );
	}
}

//---------------------------------------------------------------------------
// Visualizza pannello
//---------------------------------------------------------------------------
void CWindow::DrawPanel( const RectI& area )
{
	DrawPanel( area, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_BORDER ) );
}

void CWindow::DrawPanel( const RectI& area, const GUI_color& in, const GUI_color& out )
{
	GUI_Freeze_Locker lock;

	RectI r( GetX() + TextToGraphX( area.X ), GetY() + TextToGraphY( area.Y ), TextToGraphX( area.W ), TextToGraphY( area.H ) );
	GUI_FillRect( r, in );
	GUI_Rect( r, out );
}

//---------------------------------------------------------------------------
// Visualizza gruppo
//---------------------------------------------------------------------------
void CWindow::DrawGroup( const RectI& area, const char* text )
{
	DrawGroup( area, text, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_TXT ) );
}

void CWindow::DrawGroup( const RectI& area, const char* text, const GUI_color& bg, const GUI_color& fg )
{
	GUI_Freeze_Locker lock;

	RectI r( GetX() + TextToGraphX( area.X ) + GUI_CharW()/2, GetY() + TextToGraphY( area.Y ) + GUI_CharH()/2, TextToGraphX( area.W ) - GUI_CharW(), TextToGraphY( area.H ) - GUI_CharH() );
	GUI_Rect( r, fg );

	if( strlen( text ) )
	{
		int X1 = GetX() + TextToGraphX( area.X + 2 );
		int Y1 = GetY() + TextToGraphY( area.Y );

		GUI_DrawText( X1, Y1, text, GUI_DefaultFont, bg, fg );
	}
}

//---------------------------------------------------------------------------
// Disegna un indicatore circolare a torta
//---------------------------------------------------------------------------
void CWindow::DrawPie( const PointI& center, int radius, int degree )
{
	GUI_Freeze_Locker lock;

	GUI_FillCircle( center, radius+1, GUI_color(GR_BLACK) );
	degree = -degree;
	GUI_FillPie( center, radius+3, degree-45, degree+48, GUI_color(GR_LIGHTRED) );
	GUI_Circle( center, radius+3, GUI_color(GR_WHITE) );
	GUI_Circle( center, radius+2, GUI_color(GR_DARKGRAY) );

	GUI_HLine( center.X-radius-6, center.X-radius-4, center.Y, GUI_color(GR_WHITE));
	GUI_HLine( center.X-radius-6, center.X-radius-4, center.Y+1, GUI_color(GR_BLACK));
	GUI_HLine( center.X+radius+6, center.X+radius+4, center.Y, GUI_color(GR_WHITE));
	GUI_HLine( center.X+radius+6, center.X+radius+4, center.Y+1, GUI_color(GR_BLACK));
	GUI_VLine( center.X, center.Y-radius-6, center.Y-radius-4, GUI_color(GR_BLACK));
	GUI_VLine( center.X+1, center.Y-radius-6, center.Y-radius-4, GUI_color(GR_WHITE));
	GUI_VLine( center.X, center.Y+radius+6, center.Y+radius+4, GUI_color(GR_BLACK));
	GUI_VLine( center.X+1, center.Y+radius+6, center.Y+radius+4, GUI_color(GR_WHITE));

	GUI_HLine( center.X-2, center.X+2, center.Y, GUI_color(GR_WHITE) );
	GUI_VLine( center.X, center.Y-2, center.Y+2, GUI_color(GR_WHITE) );
}

//---------------------------------------------------------------------------
// Disegna un rettangolo
//---------------------------------------------------------------------------
void CWindow::DrawRectangle( const RectI& area, const GUI_color& color )
{
	GUI_Freeze_Locker lock;

	RectI r( GetX() + TextToGraphX( area.X ), GetY() + TextToGraphY( area.Y ), TextToGraphX( area.W ), TextToGraphY( area.H ) );
	GUI_Rect( r, color );
}
