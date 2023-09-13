//---------------------------------------------------------------------------
// Name:        c_table.cpp
// Author:      Gabriel Ferri
// Created:     25/01/2012
// Description: CTable class implementation
//---------------------------------------------------------------------------
#include "c_table.h"

#include "q_cost.h"
#include "q_graph.h"
#include "msglist.h"
#include "q_oper.h"
#include "strutils.h"

#include "gui_defs.h"
#include "gui_functions.h"
#include "gui_desktop.h"
#include "keyutils.h"

#include <mss.h>

#define  ROW_COLOR_SELECT_BG             232,242,254
#define  ROW_COLOR_HIGHLIGHT_RED         255,0,0
#define  ROW_COLOR_HIGHLIGHT_GREEN       0,150,0
#define  ROW_COLOR_SELECTED_GREEN        COL_HIGHLIGHT


//---------------------------------------------------------------------------
// Costruttore
//---------------------------------------------------------------------------
CTable::CTable( int x, int y, unsigned int numRows, unsigned int style, CWindow* parentWin )
: GUITable( numRows, style )
{
	X = x;
	Y = y;

	parent = parentWin;

	visible = false;
	editable = false;
	selected = true;
	curC = curR = -1;

	fnOnSelectCellCallback = 0;

	// setta stille delle righe
	for( unsigned int i = 0; i < numRows; i++ )
	{
		rows_style.push_back( TABLEROW_STYLE_DEFAULT );
	}
}

//---------------------------------------------------------------------------
// Distruttore
//---------------------------------------------------------------------------
CTable::~CTable()
{
	for( unsigned int i = 0; i < cols.size(); i++ )
	{
		delete cols[i].cell;
	}
}

//---------------------------------------------------------------------------
// Aggiunge una colonna alla tabella
//---------------------------------------------------------------------------
void CTable::AddCol( const std::string& label, int width, int type, int style )
{
	CTable_Col c;
	c.cell = new CCell( "", width, type, style | CELL_STYLE_FONT_DEFAULT );
	for( unsigned int i = 0; i < GetRows(); i++ )
	{
		c.data.push_back( "" );
	}

	if( cols.size() )
	{
		c.X = cols[cols.size()-1].X + GetColWidth( cols.size()-1 ) + 1;
	}
	else
	{
		c.X = 0;
	}

	cols.push_back( c );

	GUITable::AddCol( label, width );
}

//---------------------------------------------------------------------------
// Visualizza la tabella
//---------------------------------------------------------------------------
void CTable::Show()
{
	GUI_Freeze_Locker lock;

	int x = TextToGraphX( X );
	int y = TextToGraphY( Y );

	if( parent )
	{
		x += parent->GetX();
		y += parent->GetY();
	}

	GUITable::Show( x, y );

	// refresh data
	for( unsigned int r = 0; r < GetRows(); r++ )
	{
		showRow( x, y, r, false );
	}

	visible = true;

	curC = curR = 0;
	selCell( 0, 0 );
}

//--------------------------------------------------------------------------
// Visualizza dati intera riga
//--------------------------------------------------------------------------
void CTable::showRow( int x, int y, int r, bool highlight )
{
	GUI_Freeze_Locker lock;

	GUI_color color = ( highlight && (GetStyle() & TABLE_STYLE_HIGHLIGHT_ROW) ) ? GUI_color(ROW_COLOR_SELECT_BG) : GetBgColor();

	HighlightRow( r, color );

	for( unsigned int c = 0; c < cols.size(); c++ )
	{
		// set style
		setCellStyle( r, c, color );

		cols[c].cell->Visible( false );
		cols[c].cell->SetTxt( cols[c].data[r].c_str() );
		cols[c].cell->Show( x + cols[c].X*GUI_CharW(), y + r*GUI_CharH() );
	}
}

//--------------------------------------------------------------------------
// Gestione dei tasti della tabella
//--------------------------------------------------------------------------
int CTable::ManageKey( int key )
{
	SetConfirmRequiredBeforeNextXYMovement(true);

	switch( key )
	{
		case K_CTRL_C:
			CopyToClipboard();
			break;

		case K_CTRL_V:
			PasteFromClipboard();
			return 1;

		case K_DOWN:
		case K_UP:
			vSelect( key );
			break;

		case K_RIGHT:
		case K_LEFT:
			hSelect( key );
			break;

		default:
			if( editable )
			{
				return editCell( curR, curC, key ); //TODO: controllo sul valore di curCell
			}
			break;
	}

	return 0;
}

//---------------------------------------------------------------------------
// Setta valori minimo e  massimo per la colonna
//---------------------------------------------------------------------------
bool CTable::SetColMinMax( unsigned int n, int min, int max )
{
	if( n < cols.size() )
	{
		cols[n].cell->SetVMinMax( min, max );
		return true;
	}
	return false;
}

bool CTable::SetColMinMax( unsigned int n, float min, float max )
{
	if( n < cols.size() )
	{
		cols[n].cell->SetVMinMax( min, max );
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
// Setta insieme di caratteri legali per la colonna
//    NOTE: Esegue un overide dei caratteri validi per il modo di edit
//          corrente
//--------------------------------------------------------------------------
bool CTable::SetColLegalChars( unsigned int n, const char* set )
{
	if( n < cols.size() )
	{
		cols[n].cell->SetLegalChars( set );
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
// Setta insieme di stringhe ammesse per la colonna
//--------------------------------------------------------------------------
bool CTable::SetColLegalStrings( unsigned int n, int nSet, char** set )
{
	if( n < cols.size() )
	{
		cols[n].cell->SetLegalStrings( nSet, set );
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
// Setta la colonna editabile o no
//--------------------------------------------------------------------------
bool CTable::SetColEditable( unsigned int n, bool mode )
{
	if( n < cols.size() )
	{
		if( mode )
		{
			cols[n].cell->SetStyle( cols[n].cell->GetStyle() & ~CELL_STYLE_READONLY );
			cols[n].cell->SetEdit( editable );
		}
		else
		{
			cols[n].cell->SetStyle( cols[n].cell->GetStyle() | CELL_STYLE_READONLY );
			cols[n].cell->SetEdit( false );
		}
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
// Setta stile della riga
//--------------------------------------------------------------------------
bool CTable::SetRowStyle( unsigned int n, unsigned int style )
{
	if( n < GetRows() )
	{
		rows_style[n] = style;
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
// Setta/Restituisce testo di una cella
//--------------------------------------------------------------------------
bool CTable::SetText( unsigned int row, unsigned int col, const char* text )
{
	if( row < GetRows() && col < cols.size() )
	{
		cols[col].data[row] = text;

		if( row == curR && col == curC && selected )
			selCell( row, col, false );
		else
			deselCell( row, col, false );
		return true;
	}
	return false;
}

const char* CTable::GetText( unsigned int row, unsigned int col )
{
	if( row < GetRows() && col < cols.size() )
	{
		return cols[col].data[row].c_str();
	}
	return 0;
}

//--------------------------------------------------------------------------
// Setta/Restituisce testo di una cella INT
//--------------------------------------------------------------------------
bool CTable::SetText( unsigned int row, unsigned int col, int val )
{
	if( row < GetRows() && col < cols.size() )
	{
		cols[col].cell->Visible( false );
		cols[col].cell->SetTxt( val );
		cols[col].data[row] = cols[col].cell->GetTxt();

		if( row == curR && col == curC && selected )
			selCell( row, col, false );
		else
			deselCell( row, col, false );
		return true;
	}
	return false;
}

int CTable::GetInt( unsigned int row, unsigned int col )
{
	if( row < GetRows() && col < cols.size() )
	{
		cols[col].cell->Visible( false );
		cols[col].cell->SetTxt( cols[col].data[row].c_str() );
		return cols[col].cell->GetInt();
	}
	return 0;
}

//--------------------------------------------------------------------------
// Setta/Restituisce testo di una cella FLOAT
//--------------------------------------------------------------------------
bool CTable::SetText( unsigned int row, unsigned int col, float val )
{
	if( row < GetRows() && col < cols.size() )
	{
		cols[col].cell->Visible( false );
		cols[col].cell->SetTxt( val );
		cols[col].data[row] = cols[col].cell->GetTxt();

		if( row == curR && col == curC && selected )
			selCell( row, col, false );
		else
			deselCell( row, col, false );
		return true;
	}
	return false;
}

float CTable::GetFloat( unsigned int row, unsigned int col )
{
	if( row < GetRows() && col < cols.size() )
	{
		cols[col].cell->Visible( false );
		cols[col].cell->SetTxt( cols[col].data[row].c_str() );
		return cols[col].cell->GetFloat();
	}
	return 0;
}

//--------------------------------------------------------------------------
// Setta/Restituisce testo di una cella STRING SET
//--------------------------------------------------------------------------
bool CTable::SetStrings_Pos( unsigned int row, unsigned int col, int pos )
{
	if( row < GetRows() && col < cols.size() )
	{
		cols[col].cell->Visible( false );
		cols[col].cell->SetStrings_Pos( pos );
		cols[col].data[row] = cols[col].cell->GetTxt();

		if( row == curR && col == curC && selected )
			selCell( row, col, false );
		else
			deselCell( row, col, false );
		return true;
	}
	return false;
}

int CTable::GetStrings_Pos( unsigned int row, unsigned int col )
{
	if( row < GetRows() && col < cols.size() )
	{
		cols[col].cell->Visible( false );
		cols[col].cell->SetStrings_Text( cols[col].data[row].c_str() );
		return cols[col].cell->GetStrings_Pos();
	}
	return 0;
}

//--------------------------------------------------------------------------
// Setta/Restituisce testo YES/NO
//--------------------------------------------------------------------------
bool CTable::SetTextYN( unsigned int row, unsigned int col, int val )
{
	if( row < GetRows() && col < cols.size() )
	{
		cols[col].cell->Visible( false );
		cols[col].cell->SetTxtYN( val );
		cols[col].data[row] = cols[col].cell->GetTxt();

		if( row == curR && col == curC && selected )
			selCell( row, col, false );
		else
			deselCell( row, col, false );
		return true;
	}
	return false;
}

int CTable::GetYN( unsigned int row, unsigned int col )
{
	if( row < GetRows() && col < cols.size() )
	{
		cols[col].cell->Visible( false );
		cols[col].cell->SetTxt( cols[col].data[row].c_str() );
		return cols[col].cell->GetYN();
	}
	return 0;
}

//--------------------------------------------------------------------------
// Setta edit mode
//--------------------------------------------------------------------------
void CTable::SetEdit( bool mode )
{
	editable = mode;
	if( cols[curC].cell->IsSelected() )
	{
		selCell( curR, curC );
	}
}

//--------------------------------------------------------------------------
// Seleziona la cella alle coordinate passate
//--------------------------------------------------------------------------
bool CTable::Select( unsigned int row, unsigned int col )
{
	selected = true;
	if( row < GetRows() && col < cols.size() )
	{
		if( curR != row || curC != col )
		{
			deselCell( curR, curC );
			curR = row;
			curC = col;
		}

		selCell( curR, curC );
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
// Deseleziona la cella corrente
//--------------------------------------------------------------------------
void CTable::Deselect()
{
	selected = false;
	deselCell( curR, curC );
}

//--------------------------------------------------------------------------
// Seleziona la casella alle coordinate passate
//--------------------------------------------------------------------------
void CTable::selCell( int r, int c, bool show_row )
{
	GUI_Freeze_Locker lock;

	if( visible && show_row )
	{
		int x = TextToGraphX( X );
		int y = TextToGraphY( Y );

		if( parent )
		{
			x += parent->GetX();
			y += parent->GetY();
		}

		showRow( x, y, r, true );
	}

	cols[c].cell->Visible( false );
	cols[c].cell->SetEdit( editable );
	cols[c].cell->Select();
	refreshCell( r, c );

	if( visible && fnOnSelectCellCallback )
	{
		// execute callback function
		fnOnSelectCellCallback( r, c );
	}
}

//--------------------------------------------------------------------------
// Deseleziona la casella alle coordinate passate
//--------------------------------------------------------------------------
void CTable::deselCell( int r, int c, bool show_row )
{
	GUI_Freeze_Locker lock;

	cols[c].cell->Visible( false );
	cols[c].cell->Deselect();
	refreshCell( r, c );

	if( visible && show_row )
	{
		int x = TextToGraphX( X );
		int y = TextToGraphY( Y );

		if( parent )
		{
			x += parent->GetX();
			y += parent->GetY();
		}

		showRow( x, y, curR, false );
	}
}

//--------------------------------------------------------------------------
// Aggiorna la casella alle coordinate passate
//--------------------------------------------------------------------------
void CTable::refreshCell( int r, int c )
{
	if( !visible )
		return;

	int x = TextToGraphX( X );
	int y = TextToGraphY( Y );

	if( parent )
	{
		x += parent->GetX();
		y += parent->GetY();
	}

	// set style
	setCellStyle( r, c, GetBgColor() );

	cols[c].cell->Visible( false );
	cols[c].cell->SetTxt( cols[c].data[r].c_str() );
	cols[c].cell->Show( x + cols[c].X*GUI_CharW(), y + r*GUI_CharH() );
}

//--------------------------------------------------------------------------
// Edit della casella alle coordinate passate
//--------------------------------------------------------------------------
int CTable::editCell( int r, int c, int key )
{
	if( !visible )
		return 0;

	int x = TextToGraphX( X );
	int y = TextToGraphY( Y );

	if( parent )
	{
		x += parent->GetX();
		y += parent->GetY();
	}

	cols[c].cell->Visible( false );
	cols[c].cell->SetTxt( cols[c].data[r].c_str() );
	cols[c].cell->Show( x + cols[c].X*GUI_CharW(), y + r*GUI_CharH() );

	int ret = cols[c].cell->Edit( key );
	if( ret )
	{
		cols[c].data[r] = cols[c].cell->GetTxt();
	}

	return ret;
}

//--------------------------------------------------------------------------
// Sposta la selezione verticalmente di una posizione
//--------------------------------------------------------------------------
void CTable::vSelect( int key )
{
	if( curC == -1 )
		return;

	GUI_Freeze_Locker lock;

	int x = TextToGraphX( X );
	int y = TextToGraphY( Y );

	if( parent )
	{
		x += parent->GetX();
		y += parent->GetY();
	}

	// muovendosi in verticale la cella in cui andiamo e' sicuramente selezionabile
	if( key == K_DOWN )
	{
		if( curR < GetRows() - 1 )
		{
			deselCell( curR, curC );
			curR++;
			selCell( curR, curC );
		}
	}
	else if( key == K_UP )
	{
		if( curR > 0 )
		{
			deselCell( curR, curC );
			curR--;
			selCell( curR, curC );
		}
	}
}

//--------------------------------------------------------------------------
// Sposta la selezione orizzontalmente di una posizione
//--------------------------------------------------------------------------
void CTable::hSelect( int key )
{
	if( curC == -1 )
		return;

	GUI_Freeze_Locker lock;

	int x = TextToGraphX( X );
	int y = TextToGraphY( Y );

	if( parent )
	{
		x += parent->GetX();
		y += parent->GetY();
	}

	int col = curC;

	if( key == K_RIGHT )
	{
		while( col < cols.size() - 1 )
		{
			col++;
			if( !(cols[col].cell->GetStyle() & CELL_STYLE_NOSEL) )
			{
				deselCell( curR, curC, false );
				curC = col;
				selCell( curR, curC );
				break;
			}
		}
	}
	else if( key == K_LEFT )
	{
		while( curC > 0 )
		{
			col--;
			if( !(cols[col].cell->GetStyle() & CELL_STYLE_NOSEL) )
			{
				deselCell( curR, curC, false );
				curC = col;
				selCell( curR, curC );
				break;
			}

			if( col == 0 )
			{
				break;
			}
		}
	}
}

//--------------------------------------------------------------------------
// Imposta stile della riga
//--------------------------------------------------------------------------
void CTable::setCellStyle( int r, int c, GUI_color bgColor )
{
	// set row bg color
	cols[c].cell->SetBgColor( bgColor );
	cols[c].cell->SetReadOnlyBgColor( bgColor );

	// set row fg color
	if( rows_style[r] & TABLEROW_STYLE_HIGHLIGHT_RED )
	{
		cols[c].cell->SetFgColor( GUI_color( ROW_COLOR_HIGHLIGHT_RED ) );
	}
	else if( rows_style[r] & TABLEROW_STYLE_HIGHLIGHT_GREEN )
	{
		cols[c].cell->SetFgColor( GUI_color( ROW_COLOR_HIGHLIGHT_GREEN ) );
	}
	else if( rows_style[r] & TABLEROW_STYLE_SELECTED_GREEN )
	{
		cols[c].cell->SetFgColor( GetFgColor() );
		cols[c].cell->SetBgColor( GUI_color( ROW_COLOR_SELECTED_GREEN ) );
		cols[c].cell->SetReadOnlyBgColor( GUI_color( ROW_COLOR_SELECTED_GREEN ) );
	}
	else // default
	{
		cols[c].cell->SetFgColor( GetFgColor() );
	}
}

//--------------------------------------------------------------------------
// Copia il testo della cella correntemente selezionata in memoria
//--------------------------------------------------------------------------
void CTable::CopyToClipboard()
{
	if( curC == -1 || curR == -1 )
		return;

	copyTextToClipboard( cols[curC].data[curR].c_str(), cols[curC].cell->GetType() );
}

//--------------------------------------------------------------------------
// Incolla il testo dalla memoria nella cella correntemente selezionata
//--------------------------------------------------------------------------
void CTable::PasteFromClipboard(void)
{
	if( !editable || curC == -1 || curR == -1 )
	{
		return;
	}

	cols[curC].cell->Visible( false );
	cols[curC].cell->SetTxt( cols[curC].data[curR].c_str() );
	cols[curC].cell->PasteFromClipboard();
	cols[curC].data[curR] = cols[curC].cell->GetTxt();

	if( selected )
		selCell( curR, curC );
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void CTable::SetBgColor( GUI_color color )
{
	//TODO: ridisegnare sfondo tabella
	GUITable::SetBgColor( color );

	for( unsigned int c = 0; c < cols.size(); c++ )
	{
		cols[c].cell->SetBgColor( color );
		cols[c].cell->SetReadOnlyBgColor( color );
	}

	Show();
}
