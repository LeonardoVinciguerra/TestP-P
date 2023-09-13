//---------------------------------------------------------------------------
//
// Name:        gui_statusbar.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_StatusBar class implementation
//
//---------------------------------------------------------------------------
#include "gui_statusbar.h"

#include <stdio.h>

#include "gui_defs.h"
#include "gui_functions.h"

#include <mss.h>


//---------------------------------------------------------------------------
// Costruttore della classe
//---------------------------------------------------------------------------
GUI_StatusBar::GUI_StatusBar( int y, int h )
{
	Y = y;
	H = h;
}

//---------------------------------------------------------------------------
// Distruttore della classe
//---------------------------------------------------------------------------
GUI_StatusBar::~GUI_StatusBar()
{
	for( unsigned int i = 0; i < items.size(); i++ )
	{
		if( items[i]->label )
			delete [] items[i]->label;
		delete items[i];
	}
	items.clear();
}

//--------------------------------------------------------------------------
// Funzione che aggiunge una voce alla barra
//--------------------------------------------------------------------------
void GUI_StatusBar::Add( int width, int type, const char* txt, int show )
{
	GUI_StatusData* data = new GUI_StatusData;
	
	data->width = width;
	data->type = type;
	data->show = show;

	data->label = new char[width+1];
	snprintf( data->label, width, "%s", txt );

	items.push_back( data );
}

//--------------------------------------------------------------------------
// Aggiorna il testo di un elemento della barra
//--------------------------------------------------------------------------
void GUI_StatusBar::SetLabel( int n, const char* label, bool show )
{
	if( n >= items.size() )
		return;

	snprintf( items[n]->label, items[n]->width, "%s", label );

	if( show )
		showItems();
}

//--------------------------------------------------------------------------
// Modifica il flag di stato per l'elemento specificato
//--------------------------------------------------------------------------
void GUI_StatusBar::SetStatus( int n, int show )
{
	if( n >= items.size() )
		return;
	
	if( show != items[n]->show )
	{
		items[n]->show = show;
		showItems();
	}
}

//--------------------------------------------------------------------------
// Modifica il tipo dell'elemento specificato
//--------------------------------------------------------------------------
void GUI_StatusBar::SetType( int n, int type )
{
	if( n >= items.size() )
		return;

	if( items[n]->type == type )
		return;

	items[n]->type = type;
	if( items[n]->show )
	{
		showItems();
	}
}

//--------------------------------------------------------------------------
// Visualizza la barra di stato
//--------------------------------------------------------------------------
void GUI_StatusBar::Show()
{
	GUI_Freeze_Locker lock;

	GUI_FillRect( RectI( 0, Y, GUI_ScreenW(), Y + H ), GUI_color( SB_COL_BACKGROUND ) );

	isShown = true;
	
	showItems();
}

//--------------------------------------------------------------------------
// Attiva la barra di stato
//--------------------------------------------------------------------------
void GUI_StatusBar::showItems()
{
	if( !isShown )
		return;

	GUI_Freeze_Locker lock;

	GUI_color fg_color( SB_COL_TXT );
	int dx = GUI_CharW() / 2;
	int dy = (H - GUI_CharH()) / 2;
	
	int xpos = GUI_CharW();
	int ypos = Y + dy;
	
	for( unsigned int i = 0; i < items.size(); i++ )
	{
		if( items[i]->type != SB_EMPTY )
			// clear item background
			GUI_FillRect( RectI( xpos-dx, ypos, items[i]->width*GUI_CharW()+2*dx, H-2*dy ), GUI_color( SB_COL_ITEM_BG ) );
		
		if( items[i]->show )
		{
			if( items[i]->type == SB_GRAPH_1 )
			{
				int box_s = SB_BOX_DIM;
				int box_dy = (H - box_s) / 2;
				
				int box_x = xpos;
				int box_y = Y+box_dy;

				GUI_FillRect( RectI( box_x, box_y, box_s, box_s ), GUI_color( SB_COL_ITEM_BORDER_1 ) );
				GUI_FillRect( RectI( box_x + 2, box_y + 2, box_s - 4, box_s - 4 ), GUI_color( SB_BOX_COL ) );

				GUI_DrawText( box_x + box_s + 10, ypos, items[i]->label, GUI_DefaultFont, fg_color );
			}
			else if( items[i]->type == SB_GRAPH_2 )
			{
				int box_s = SB_BOX_DIM;
				int box_dy = (H - box_s) / 2;

				int box_x = xpos;
				int box_y = Y+box_dy;

				GUI_FillRect( RectI( box_x-2, box_y-2, box_s+4, box_s+4 ), GUI_color( SB_COL_ITEM_BORDER_2 ) );
				GUI_FillRect( RectI( box_x, box_y, box_s, box_s ), GUI_color( SB_COL_ITEM_BORDER_3 ) );
				GUI_FillRect( RectI( box_x + 2, box_y + 2, box_s - 4, box_s - 4 ), GUI_color( SB_BOX_COL ) );

				GUI_DrawText( box_x + box_s + 10, ypos, items[i]->label, GUI_DefaultFont, fg_color );
			}
			else if( items[i]->type == SB_TEXT )
			{
				GUI_Rect( RectI( xpos-dx, ypos, items[i]->width*GUI_CharW()+2*dx+1, H-2*dy+1 ), GUI_color( SB_COL_ITEM_BORDER_1 ) );
				GUI_DrawText( xpos, ypos, items[i]->label, GUI_DefaultFont, fg_color );
			}
		}
		
		xpos += (items[i]->width+2) * GUI_CharW();
	}
}
