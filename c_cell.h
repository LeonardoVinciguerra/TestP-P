//---------------------------------------------------------------------------
// Name:        c_cell.h
// Author:      Gabriel Ferri
// Created:     24/01/2012
// Description: CCell class definition
//---------------------------------------------------------------------------

#ifndef __C_CELL_H
#define __C_CELL_H

#include <string>
#include "gui_functions.h"


// Cell style
#define CELL_STYLE_DEFAULT      0x0000
#define CELL_STYLE_READONLY     0x0001
#define CELL_STYLE_NOSEL        0x0002
#define CELL_STYLE_UPPERCASE    0x0004
#define CELL_STYLE_LOWERCASE    0x0008
#define CELL_STYLE_OVERWRITE    0x0010
#define CELL_STYLE_FONT_DEFAULT 0x0020
#define CELL_STYLE_CENTERED     0x0040

// Cell type
#define CELL_TYPE_FIRST         0

#define CELL_TYPE_TEXT          0
#define CELL_TYPE_UINT          1
#define CELL_TYPE_SINT          2
#define CELL_TYPE_UDEC          3
#define CELL_TYPE_SDEC          4
#define CELL_TYPE_YN            5

#define CELL_TYPE_LAST          5


class CCell
{
public:
	CCell( const char* text, int width, int type = CELL_TYPE_TEXT, int style = CELL_STYLE_DEFAULT );
	~CCell();

	void Show( int x, int y ) { X = x; Y = y; visible = 1; Refresh(); }
	void Visible( bool state ) { visible = state; }

	void SetStyle( unsigned int style ) { cell_style = style; }
	unsigned int GetStyle() { return cell_style; }
	unsigned int GetType() { return cell_type; }

	void SetEdit( bool state );

	void Select();
	void Deselect();
	bool IsSelected() { return selected; }

	// text
	void SetTxt( const char* text );
	char* GetTxt() { return txt; }
	// int
	void SetTxt( int val );
	int GetInt();
	// int
	//void SetTxt( long val );
	//int GetLong();
	// float
	void SetTxt( float val );
	float GetFloat();
	// char
	void SetTxt( char val );
	char GetChar();
	// Y/N
	void SetTxtYN( int val );
	int GetYN();

	void SetBgColor( const GUI_color& col ) { bgColor = col; }
	void SetFgColor( const GUI_color& col ) { fgColor = col; }

	void SetReadOnlyBgColor( const GUI_color& col ) { bgReadOnlyColor = col; }

	void SetLegalChars( const char* set );

	void SetDecimals( int dec ) { decdgt = dec; }
	void SetVMinMax( float min, float max );
	void SetVMinMax( int min, int max );

	void SetLegalStrings( int nSet, char** set );
	void SetStrings_Pos( int pos );
	void SetStrings_Text( const char* text );
	int GetStrings_Pos();

	void Refresh();
	int Edit( int key );

	void CopyToClipboard();
	void PasteFromClipboard();

protected:
	int X, Y;           // posizione in pixels

	unsigned int cell_style;
	unsigned int cell_type;

	bool visible;
	bool selected;
	bool editable;

	char* txt;          // Testo contenuto nella casella
	int W;              // Larghezza della casella
	int decdgt;			// Numero di cifre decimali

	// colors
	GUI_color bgColor;
	GUI_color fgColor;
	GUI_color bgSelColor;
	GUI_color fgSelColor;
	GUI_color bgEditColor;
	GUI_color fgEditColor;
	GUI_color bgReadOnlyColor;

	std::string legalset;

	float maxval;       // Massimo valore ammesso
	float minval;       // Minimo valore ammesso

	char** txtSet;      // Set di stringhe
	int txtSet_count;   // Numero di stringhe
	int txtSet_pos;     // Posizione corrente nel set di stringhe

	char manageChar( char key );
	int checkLegalChars( int key );
	int editString( char key, char* s );
	void clearTxtSet();
	bool checkClipboardText();
	void checkMinMax();
};

#endif
