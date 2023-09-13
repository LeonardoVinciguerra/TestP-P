//---------------------------------------------------------------------------
// Name:        gui_table.cpp
// Author:      Gabriel Ferri
// Created:     26/01/2012
// Description: GUITable class implementation
//---------------------------------------------------------------------------
#include "gui_table.h"

#include "gui_defs.h"
#include "gui_functions.h"

#include <mss.h>


//---------------------------------------------------------------------------
// Costruttore
//---------------------------------------------------------------------------
GUITable::GUITable( unsigned int numRows, unsigned int style )
{
	x = 0;
	y = 0;

	rows = numRows;

	table_style = style;

	bgColor = GUI_color( GRID_COL_BG );
	fgColor = GUI_color( GRID_COL_FG );
}

//---------------------------------------------------------------------------
// Distruttore
//---------------------------------------------------------------------------
GUITable::~GUITable()
{
}

//---------------------------------------------------------------------------
// Aggiunge una colonna alla tabella
//---------------------------------------------------------------------------
void GUITable::AddCol( const std::string& label, int width )
{
	GUITable_Col c;
	c.Label = label;
	c.W = width;

	cols.push_back( c );
}

//---------------------------------------------------------------------------
// Ritorna larghezza colonna
//---------------------------------------------------------------------------
int GUITable::GetColWidth( unsigned int n )
{
	if( n < cols.size() )
	{
		return cols[n].W;
	}
	return -1;
}

//---------------------------------------------------------------------------
// Visualizza la tabella
//---------------------------------------------------------------------------
void GUITable::Show( int X, int Y )
{
	x = X;
	y = Y;

	GUI_Freeze_Locker lock;

	GUI_color in_border( GRID_COL_IN_BORDER );
	GUI_color out_border( GRID_COL_OUT_BORDER );

	GUI_color head_bg( GRID_COL_LABEL_BG );
	GUI_color head_fg( GRID_COL_LABEL_FG );

	if( cols.empty() || rows < 1 )
		return;

	int total_size = 0;
	for( unsigned int i = 0; i < cols.size(); i++ )
	{
		total_size += cols[i].W;
	}
	total_size += cols.size();

	RectI r( x, y, (total_size-1)*GUI_CharW(), rows*GUI_CharH() );

	// draw background
	GUI_FillRect( RectI( r.X-4, r.Y-1, r.W+8, r.H-1 ), bgColor );

	// draw out box
	int yOut = (table_style & TABLE_STYLE_NO_LABELS) ? r.Y-4 : r.Y-GUI_CharH()-8;
	int hOut = (table_style & TABLE_STYLE_NO_LABELS) ? r.H+5 : r.H+GUI_CharH()+9;
	GUI_Rect( RectI( r.X-7, yOut, r.W+14, hOut ), out_border );

	// draw in box - columns
	int offset = -GUI_CharW()/2 + 2;
	int total = 0; // somma caratteri

	for( unsigned int i = 0; i < cols.size(); i++ )
	{
		GUI_VLine( r.X-2+offset, r.Y-1, r.Y+r.H-3, in_border );

		if( !(table_style & TABLE_STYLE_NO_LABELS) )
		{
			int cX = r.X - 2 + offset;
			int cY = r.Y - GUI_CharH() - 4;
			int cW = (cols[i].W+1)*GUI_CharW() + 1;
			int cH = GUI_CharH() + 3;

			GUI_FillRect( RectI(cX, cY-2, cW, cH), head_bg );
			GUI_Rect( RectI(cX, cY-2, cW, cH), out_border );

			if( !cols[i].Label.empty() )
			{
				int ty = cY + (cH-GUI_CharH())/2;
				GUI_DrawTextCentered( cX, cX+cW, ty, cols[i].Label.c_str(), GUI_DefaultFont, head_fg );
			}
		}

		total += cols[i].W + 1;
		offset = total*GUI_CharW() - GUI_CharW()/2 + 1;
	}
	// last vertical line
	GUI_VLine( r.X-2+offset, r.Y-1, r.Y+r.H-3, in_border );


	// Disegna le righe
	for( unsigned int i = 0; i <= rows; i++ )
	{
		GUI_HLine( r.X-4, r.X+r.W+3, r.Y+(GUI_CharH()*i)-2, in_border );
	}
}

//---------------------------------------------------------------------------
// Visualizza la tabella
//---------------------------------------------------------------------------
void GUITable::HighlightRow( unsigned int row, GUI_color color )
{
	if( row >= rows )
	{
		return;
	}

	int X = x-4;

	for( unsigned int i = 0; i < cols.size(); i++ )
	{
		if( i == 0 )
		{
			GUI_FillRect( RectI( X, y+row*GUI_CharH()-1, (cols[i].W+1)*GUI_CharW()-2, GUI_CharH()-1 ), color );
		}
		else
		{
			GUI_FillRect( RectI( X-1, y+row*GUI_CharH()-1, (cols[i].W+1)*GUI_CharW()-1, GUI_CharH()-1 ), color );
		}

		X += (cols[i].W+1)*GUI_CharW();
	}
}
