//---------------------------------------------------------------------------
// Name:        gui_window.cpp
// Author:      Gabriel Ferri
// Created:     15/11/2011
// Description: GUIWindow class implementation
//---------------------------------------------------------------------------
#include "gui_window.h"

#include "gui_defs.h"
#include "gui_functions.h"

#include <mss.h>


//---------------------------------------------------------------------------
// Costruttore
//---------------------------------------------------------------------------
GUIWindow::GUIWindow()
: GUIObject()
{
	win_title = "";
	win_style = WIN_STYLE_DEFAULT;

	is_shown = false;
	is_selected = false;

	bg_buf_index = -1;
	fg_buf_index = -1;
}

GUIWindow::GUIWindow( const RectI& clientArea, const std::string& title, unsigned int style )
: GUIObject()
{
	client_area = clientArea;
	win_title = title;
	win_style = style;

	is_shown = false;
	is_selected = false;

	bg_buf_index = -1;
	fg_buf_index = -1;
}

//---------------------------------------------------------------------------
// Distruttore
//---------------------------------------------------------------------------
GUIWindow::~GUIWindow()
{
	if( is_shown )
	{
		RestoreScreen( bg_buf_index );
	}
}

//---------------------------------------------------------------------------
// Setta titolo della finestra
//---------------------------------------------------------------------------
void GUIWindow::SetTitle( const std::string& title )
{
	win_title = title;

	if( is_shown )
	{
		drawCaption( is_selected );
	}
}

//---------------------------------------------------------------------------
// Visualizza la finestra
//---------------------------------------------------------------------------
void GUIWindow::Show( bool select )
{
	if( !is_shown )
	{
		GUI_Freeze();
		if( fg_buf_index == -1 )
		{
			// calc area with frame
			graph_area.X = client_area.X - WIN_BORDER;
			graph_area.Y = client_area.Y - WIN_BORDER;
			graph_area.W = client_area.W + 2 * WIN_BORDER;
			graph_area.H = client_area.H + 2 * WIN_BORDER;

			if( !(win_style & WIN_STYLE_NO_CAPTION) )
			{
				graph_area.Y -= (WIN_CAPTION_HEIGHT + WIN_CAPTION_LOW_BORDER);
				graph_area.H += (WIN_CAPTION_HEIGHT + WIN_CAPTION_LOW_BORDER);
			}

			bg_buf_index = SaveScreen();

			draw();
		}
		else
		{
			RestoreScreen( fg_buf_index );
			fg_buf_index = -1;
		}

		drawCaption( select );
		GUI_Thaw();

		is_selected = select;
		is_shown = true;
	}
}

//---------------------------------------------------------------------------
// Nasconde la finestra
//---------------------------------------------------------------------------
void GUIWindow::Hide( bool saveForeground )
{
	if( is_shown )
	{
		if( saveForeground )
			fg_buf_index = SaveScreen();

		RestoreScreen( bg_buf_index );
		is_shown = false;
	}
}

//---------------------------------------------------------------------------
// Seleziona la finestra
//---------------------------------------------------------------------------
void GUIWindow::Select()
{
	if( is_shown && !is_selected )
	{
		drawCaption( true );
	}
	is_selected = true;
}

//---------------------------------------------------------------------------
// Deseleziona la finestra
//---------------------------------------------------------------------------
void GUIWindow::Deselect()
{
	if( is_shown && is_selected )
	{
		drawCaption( false );
	}
	is_selected = false;
}

//---------------------------------------------------------------------------
// Disegna la finestra
//---------------------------------------------------------------------------
void GUIWindow::draw()
{
	GUI_Freeze();

	// frame
	GUI_FillRect( graph_area, GUI_color( WIN_COL_BORDER ) );
	// client area
	GUI_FillRect( client_area, GUI_color( WIN_COL_CLIENTAREA ) );

	GUI_Thaw();
}

//---------------------------------------------------------------------------
// Disegna la barra del titolo
//---------------------------------------------------------------------------
void GUIWindow::drawCaption( bool select )
{
	if( win_style & WIN_STYLE_NO_CAPTION )
	{
		return;
	}

	RectI rect( graph_area.X + WIN_BORDER, graph_area.Y + WIN_BORDER, client_area.W, WIN_CAPTION_HEIGHT );

	GUI_Freeze();

	if( select )
		GUI_FillRect( rect, GUI_color( WIN_COL_SELECTED ) );
	else
		GUI_FillRect( rect, GUI_color( WIN_COL_DESELECTED ) );

	if( win_title.length() > 0)
	{
		int X1 = rect.X;
		int X2 = X1 + rect.W - 1;

		if( select )
			GUI_DrawTextCentered( X1, X2, rect.Y, win_title.c_str(), GUI_DefaultFont, GUI_color( WIN_COL_SELECTED ), GUI_color( WIN_COL_TXT_SELECTED ) );
		else
			GUI_DrawTextCentered( X1, X2, rect.Y, win_title.c_str(), GUI_DefaultFont, GUI_color( WIN_COL_DESELECTED ), GUI_color( WIN_COL_TXT_DESELECTED ) );
	}

	GUI_Thaw();
}
