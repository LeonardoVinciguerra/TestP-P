//---------------------------------------------------------------------------
// Name:        c_win_select.cpp
// Author:      Gabriel Ferri
// Created:     18/11/2011
// Description: CWindowSelect class implementation
//---------------------------------------------------------------------------
#include "c_win_select.h"

#include <boost/algorithm/string.hpp>
#include "c_pan.h"
#include "q_cost.h"
#include "gui_functions.h"
#include "gui_defs.h"
#include "keyutils.h"

#include <mss.h>



//---------------------------------------------------------------------------
// Costruttore/Distruttore
//---------------------------------------------------------------------------
CWindowSelect::CWindowSelect( CWindow* parent, int tx, int ty, unsigned int rows, unsigned int width )
: CWindowTable( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_EDIT );
	SetClientAreaPos( 0, 5 );
	SetClientAreaSize( tx*2+width, ty*2+rows+1 );

	m_x = tx;
	m_y = ty;
	m_numRows = rows;
	m_width = width;

	m_start_item = -1;

	m_exitCode = WIN_EXITCODE_ESC;
}

CWindowSelect::~CWindowSelect()
{
}

//---------------------------------------------------------------------------
// Inserisce un elemento in fondo alla lista
//---------------------------------------------------------------------------
void CWindowSelect::AddItem( const char* item )
{
	m_items.push_back( item );
}

//---------------------------------------------------------------------------
// Ripulisce la lista
//---------------------------------------------------------------------------
void CWindowSelect::ClearItems()
{
	m_items.clear();
}

//---------------------------------------------------------------------------
// Ordina in ordine alfabetico la lista
//---------------------------------------------------------------------------
void CWindowSelect::Sort()
{
	m_items.sort( boost::bind( &CWindowSelect::compareNoCase, this, _1, _2 ) );
}

//---------------------------------------------------------------------------
// Visualizza elementi della lista (partendo dal primo)
//---------------------------------------------------------------------------
void CWindowSelect::ShowItems()
{
	m_start_item = -1;
	showItems( 0 );
}

//---------------------------------------------------------------------------
// Restituisce il numero della riga corrente
//---------------------------------------------------------------------------
int CWindowSelect::GetSelectedItemIndex()
{
	int selectedRow = m_start_item + m_table->GetCurRow();
	if( selectedRow < 0 || selectedRow >= m_items.size() )
		return -1;
	return selectedRow;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void CWindowSelect::onInit()
{
	// create table
	m_table = new CTable( m_x, m_y, m_numRows, TABLE_STYLE_NO_LABELS, this );

	// add column
	m_table->AddCol( "", m_width, CELL_TYPE_TEXT, CELL_STYLE_READONLY );

	// init delle classi derivate
	onInitSomething();
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void CWindowSelect::onShow()
{
	ShowItems();

	// show tips per le classi derivate
	tips = 0;
	onShowSomething();
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bool CWindowSelect::onKeyPress( int key )
{
	switch( key )
	{
		case K_DOWN:
		case K_UP:
		case K_PAGEDOWN:
		case K_PAGEUP:
			if( !m_search.empty() )
			{
				m_search.clear();
				showSearch();
			}
			return vSelect( key );

		case K_BACKSPACE:
			if( !m_search.empty() )
			{
				m_search.erase( m_search.end()-1, m_search.end() );
				searchItem();
			}
			break;

		case K_ESC:
			if( !m_search.empty() )
			{
				m_search.clear();
				showSearch();
				return true;
			}
			m_exitCode = WIN_EXITCODE_ESC;
			break;

		case K_ENTER:
			if( !m_search.empty() )
			{
				m_search.clear();
				showSearch();
			}
			// ENTER classi derivate
			onEnter();
			forceExit();
			m_exitCode = WIN_EXITCODE_ENTER;
			return true;

		default:
			if( onHotKey( key ) )
			{
				if( !m_search.empty() )
				{
					m_search.clear();
					showSearch();
				}
				return true;
			}
			else if( key < 256 )
			{
				if( strchr( CHARSET_FILENAME, key ) != NULL && m_search.size() < m_width )
				{
					m_search.push_back( key );
					searchItem();
				}
			}
			break;
	}

	return false;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void CWindowSelect::onClose()
{
	if( tips )
	{
		delete tips;
		tips = 0;
	}
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void CWindowSelect::showItems( int start_item )
{
	GUI_Freeze_Locker lock;

	start_item = MAX( 0, start_item );
	if( start_item == m_start_item || (start_item >= m_items.size() && m_items.size() > 0 ) )
		return;

	std::list<std::string>::iterator it = m_items.begin();
	for( int i = 0; i < start_item; i++)
		++it;

	unsigned int i = 0;
	while( i + start_item < m_items.size() && i < m_table->GetRows() )
	{
		m_table->SetText( i, 0, (*it).c_str() );
		++i;
		++it;
	}
	while( i < m_table->GetRows() )
	{
		m_table->SetText( i, 0, "" );
		++i;
	}

	if( m_items.size() > 0 )
		m_start_item = start_item;
	else
		m_start_item = -1;
}

//--------------------------------------------------------------------------
// Sposta la selezione verticalmente
//--------------------------------------------------------------------------
bool CWindowSelect::vSelect( int key )
{
	int curR = m_table->GetCurRow();
	if( curR < 0 )
		return false;

	if( key == K_DOWN )
	{
		if( curR + m_start_item >= m_items.size() - 1 )
		{
			// niente altro da selezionare
			return true;
		}

		if( curR < m_table->GetRows() - 1 )
		{
			// lascio eseguire la selezione al parent
			return false;
		}
		else // sono nell'ultima riga
		{
			// aggiorno la tabella
			showItems( m_start_item + 1 );
			return true;
		}
	}
	else if( key == K_UP )
	{
		if( curR + m_start_item == 0 )
		{
			// niente altro da selezionare
			return true;
		}

		if( curR > 0 )
		{
			// lascio eseguire la selezione al parent
			return false;
		}
		else // sono nella prima riga
		{
			// aggiorno la tabella
			showItems( m_start_item - 1 );
			return true;
		}
	}
	else if( key == K_PAGEDOWN )
	{
		if( curR + m_start_item >= m_items.size() - 1 )
		{
			// niente altro da selezionare
			return true;
		}

		// aggiorno la tabella
		showItems( m_start_item + m_table->GetRows() );
		m_table->Select( 0, 0 );
		return true;
	}
	else if( key == K_PAGEUP )
	{
		if( curR + m_start_item == 0 )
		{
			// niente altro da selezionare
			return true;
		}

		// aggiorno la tabella
		showItems( m_start_item - m_table->GetRows() );
		m_table->Select( 0, 0 );
		return true;
	}

	return false;
}

//--------------------------------------------------------------------------
// Ricerca elemento nella lista
//--------------------------------------------------------------------------
#define SEARCH_MATCH_OK       0
#define SEARCH_MATCH_NEXT     1
#define SEARCH_MATCH_END      2

void CWindowSelect::searchItem()
{
	unsigned int c_match = 0;
	unsigned int match_result = SEARCH_MATCH_OK;

	unsigned int pos = -1;
	unsigned int pos_match = -1;
	for( std::list<std::string>::iterator it = m_items.begin(); it != m_items.end(); ++it )
	{
		unsigned int c = 0;
		while( c < m_search.size() && c < (*it).size() )
		{
			if( tolower(m_search[c]) < tolower((*it)[c]) )
			{
				match_result = SEARCH_MATCH_END;
				break;
			}
			else if( tolower(m_search[c]) > tolower((*it)[c]) )
			{
				match_result = SEARCH_MATCH_NEXT;
				break;
			}

			match_result = SEARCH_MATCH_OK;
			++c;
		}

		if( c > c_match )
		{
			c_match = c;
			pos_match = pos;
		}
		else if( match_result == SEARCH_MATCH_END || c == m_search.size() )
		{
			break;
		}

		++pos;
	}

	if( c_match )
	{
		showItems( pos_match+1 );
		m_table->Select( 0, 0 );
	}

	showSearch( c_match < m_search.size() ? true : false );
}

//---------------------------------------------------------------------------
// Visualizza/Nasconde box di ricerca
//---------------------------------------------------------------------------
void CWindowSelect::showSearch( bool error )
{
	GUI_Freeze_Locker lock;

	RectI r;
	r.X = GetX() + TextToGraphX( m_x ) - 2;
	r.Y = GetY() + TextToGraphY( m_y + m_numRows ) - 2;
	r.W = TextToGraphX( m_width ) + 4;
	r.H = GUI_CharH() + 2;

	if( m_search.empty() )
	{
		GUI_FillRect( r, GUI_color( WIN_COL_CLIENTAREA ) );
		GUI_HLine( r.X, r.X+r.W, r.Y, GUI_color( GRID_COL_IN_BORDER ) );
		GUI_HLine( r.X, r.X+r.W, r.Y+2, GUI_color( GRID_COL_OUT_BORDER ) );
	}
	else
	{
		GUI_Rect( r, GUI_color( WIN_COL_BORDER ) );
		GUI_FillRect( RectI( r.X+1, r.Y+1, r.W-2, r.H-2 ), error ? GUI_color( GRID_COL_BG_ERROR ) : GUI_color( 255,255,255 ) );

		int X = GetX() + TextToGraphX( m_x );
		int Y = GetY() + TextToGraphY( m_y + m_numRows ) - 1;
		GUI_DrawText( X, Y, m_search.c_str(), GUI_SmallFont, error ? GUI_color( GRID_COL_BG_ERROR ) : GUI_color( 255,255,255 ), error ? GUI_color( GRID_COL_FG_ERROR ) : GUI_color( 0, 0, 0 ) );
	}
}

//---------------------------------------------------------------------------
// Compara due stringhe (usata per il sort)
//---------------------------------------------------------------------------
bool CWindowSelect::compareNoCase( std::string str1, std::string str2 )
{
	return boost::ilexicographical_compare( str1, str2 );
}
