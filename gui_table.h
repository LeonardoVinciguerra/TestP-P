//---------------------------------------------------------------------------
// Name:        gui_table.h
// Author:      Gabriel Ferri
// Created:     26/01/2012
// Description: GUITable class definition
//---------------------------------------------------------------------------

#ifndef __GUI_TABLE_H
#define __GUI_TABLE_H

#include <string>
#include <vector>
#include "gui_functions.h"


// Table style
#define TABLE_STYLE_DEFAULT         0x0000
#define TABLE_STYLE_NO_LABELS       0x0001
#define TABLE_STYLE_HIGHLIGHT_ROW   0x0002


struct GUITable_Col
{ 
	std::string Label;  // Intestazione colonna
	unsigned short W;   // Larghezza della colonna
};


class GUITable
{
public:
	GUITable( unsigned int numRows, unsigned int style = TABLE_STYLE_DEFAULT );
	~GUITable();

	void SetStyle( unsigned int style ) { table_style = style; }
	unsigned int GetStyle() { return table_style; }

	unsigned int GetRows() { return rows; }
	int GetColWidth( unsigned int n );

	void Show( int X, int Y );
	void HighlightRow( unsigned int row, GUI_color color );

	void SetBgColor( GUI_color color ) { bgColor = color; }
	void SetFgColor( GUI_color color ) { fgColor = color; }
	GUI_color GetBgColor() { return bgColor; }
	GUI_color GetFgColor() { return fgColor; }

protected:
	void AddCol( const std::string& label, int width );

private:
	int x, y; // Coordinate della tabella (pixels)
	unsigned int table_style;

	std::vector<GUITable_Col> cols;
	unsigned int rows;    // Numero di righe da visualizzare

	GUI_color bgColor;
	GUI_color fgColor;
};

#endif
