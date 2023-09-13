//---------------------------------------------------------------------------
// Name:        c_table.h
// Author:      Gabriel Ferri
// Created:     25/01/2012
// Description: CTable class definition
//---------------------------------------------------------------------------

#ifndef __C_TABLE_H
#define __C_TABLE_H

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "gui_table.h"
#include "c_cell.h"
#include "c_window.h"


// Table's row style
#define TABLEROW_STYLE_DEFAULT           0x0000
#define TABLEROW_STYLE_HIGHLIGHT_RED     0x0001
#define TABLEROW_STYLE_HIGHLIGHT_GREEN   0x0002
#define TABLEROW_STYLE_SELECTED_GREEN    0x0004


typedef boost::function<int(unsigned int,unsigned int)> CTableSelectCallback;


struct CTable_Col
{
	CCell* cell;                    // Ciascuna colonna si basa su una casella tipo che vale per tutta la colonna
	std::vector<std::string> data;  // Dati contenuti in ciascuna riga della colonna
	int X;                          // Coordinata inizio colonna
};


class CTable : public GUITable
{
public:
	CTable( int x, int y, unsigned int numRows, unsigned int style = TABLE_STYLE_DEFAULT, CWindow* parentWin = NULL );
	~CTable();

	void AddCol( const std::string& label, int width, int type = CELL_TYPE_TEXT, int style = CELL_STYLE_DEFAULT );

	void Show();
	int ManageKey( int key );

	bool SetColMinMax( unsigned int n, int min, int max );
	bool SetColMinMax( unsigned int n, float min, float max );
	bool SetColLegalChars( unsigned int n, const char* set );
	bool SetColLegalStrings( unsigned int n, int nSet, char** set );
	bool SetColEditable( unsigned int n, bool mode );

	bool SetRowStyle( unsigned int n, unsigned int style );

	// text
	bool SetText( unsigned int row, unsigned int col, const char* text );
	const char* GetText( unsigned int row, unsigned int col );
	// int
	bool SetText( unsigned int row, unsigned int col, int ival );
	int GetInt( unsigned int row, unsigned int col );
	// float
	bool SetText( unsigned int row, unsigned int col, float fval );
	float GetFloat( unsigned int row, unsigned int col );
	// string set
	bool SetStrings_Pos( unsigned int row, unsigned int col, int pos );
	int GetStrings_Pos( unsigned int row, unsigned int col );
	// Y/N
	bool SetTextYN( unsigned int row, unsigned int col, int val );
	int GetYN( unsigned int row, unsigned int col );

	int GetCurRow() { return curR; }
	int GetCurCol() { return curC; }

	void SetOnSelectCellCallback( CTableSelectCallback func ) { fnOnSelectCellCallback = func; }

	void SetEdit( bool mode );
	bool GetEdit() { return editable; }

	bool Select( unsigned int row, unsigned int col );
	void Deselect();

	void CopyToClipboard();
	void PasteFromClipboard();

	// overloaded member
	void SetBgColor( GUI_color color );

protected:
	int X, Y; // Coordinate della tabella (text)

	CWindow* parent;

	// Funzione di callback dell'evento selezione
	CTableSelectCallback fnOnSelectCellCallback;

	std::vector<CTable_Col> cols;
	std::vector<unsigned int> rows_style;  // Stile delle righe

	bool visible;
	bool editable;
	bool selected;
	int curR, curC; // Riga e colonna correnti della tabella

	void selCell( int r, int c, bool show_row = true );
	void deselCell( int r, int c, bool show_row = true );
	void refreshCell( int r, int c );

	int editCell( int r, int c, int key );

	void vSelect( int key );
	void hSelect( int key );

	void showRow( int x, int y, int r, bool highlight );
	void setCellStyle( int r, int c, GUI_color bgColor );
};

#endif

