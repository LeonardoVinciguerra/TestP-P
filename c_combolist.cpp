//---------------------------------------------------------------------------
//
// Name:        c_combolist.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: CComboList class implementation
//
//---------------------------------------------------------------------------
#include "c_combolist.h"

#include "c_win_par.h"
#include "q_cost.h"
#include "q_oper.h" //SetConfirmRequiredBeforeNextXYMovement
#include "keyutils.h"

#include <mss.h>


//--------------------------------------------------------------------------
// Costruttore classe
//--------------------------------------------------------------------------
CComboList::CComboList( CWindow* parentWin )
{
	parent = parentWin;
	editFlag = 0;
}

//--------------------------------------------------------------------------
// Distruttore classe
//--------------------------------------------------------------------------
CComboList::~CComboList()
{
	entries.clear();
}

//--------------------------------------------------------------------------
// Inserisce combo nella lista
// INPUT:	combo: combo box da inserire nella lista
//			row, col : Riga e colonna del nuovo combo
//--------------------------------------------------------------------------
void CComboList::Add( C_Combo* combo, int row, int col )
{
	combo->SetParent( this );
	
	ComboList_Entry entry;
	entry.pCombo = combo;
	entry.row = row;
	entry.col = col;

	entries.push_back( entry );

	curCombo = entries.size() - 1;
}

//--------------------------------------------------------------------------
// Visualizza la lista di combo
//--------------------------------------------------------------------------
void CComboList::Show()
{
	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		if( parent != NULL )
			entries[i].pCombo->Show( parent->GetX(), parent->GetY() );
		else
			entries[i].pCombo->Show( 0, 0 );
	}

	curCombo = -1;
	SelectFirst();
}

//--------------------------------------------------------------------------
// Seleziona primo combo disponibile
//--------------------------------------------------------------------------
void CComboList::SelectFirst()
{
	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		// Cerca il primo combo selezionabile...
		if( !(entries[i].pCombo->GetStyle() & CELL_STYLE_NOSEL) )
		{
			curCombo = i;
			break;
		}
	}

	CurSelect();
}

//--------------------------------------------------------------------------
// Seleziona combo corrente
//--------------------------------------------------------------------------
void CComboList::CurSelect()
{
	if( curCombo != -1 )
	{
		entries[curCombo].pCombo->Select();
	}
}

//--------------------------------------------------------------------------
// Deseleziona combo corrente
//--------------------------------------------------------------------------
void CComboList::CurDeselect()
{
	if( curCombo != -1 )
	{
		entries[curCombo].pCombo->Deselect();
	}
}

//--------------------------------------------------------------------------
// Edita il combo corrente
// INPUT:	key: Codice del carattere da inserire
//--------------------------------------------------------------------------
int CComboList::CurEdit( int key )
{
	int ret = 0;

	if( curCombo != -1 )
	{
		ret = entries[curCombo].pCombo->Edit( key );
	}

	return ret;
}

//--------------------------------------------------------------------------
//Sposta la selezione verticalmente di una posizione
// INPUT:	direction: Direzione lungo la quale spostare la selezione
//				   0: Spostamento in basso
//				   1: Spostamento in alto
//--------------------------------------------------------------------------
void CComboList::VSelect( char direction )
{
	if( curCombo == -1 )
		return;
	
	int new_row = entries[curCombo].row;
	int new_combo = -1;
	
	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		if( entries[i].col == entries[curCombo].col && !(entries[i].pCombo->GetStyle() & CELL_STYLE_NOSEL) )
		{
			if( direction == 0 )
			{
				if( entries[i].row > entries[curCombo].row )
				{
					if( new_combo == -1 || entries[i].row < new_row )
					{
						new_row = entries[i].row;
						new_combo = i;
					}
				}
			}
			else
			{
				if( entries[i].row < entries[curCombo].row )
				{
					if( new_combo == -1 || entries[i].row > new_row )
					{
						new_row = entries[i].row;
						new_combo = i;
					}
				}
			}
		}
	}
	
	if( new_combo != -1 )
	{
		CurDeselect();
		curCombo = new_combo;
		CurSelect();
		return;
	}
	
	bipbip();
}

//--------------------------------------------------------------------------
// Sposta la selezione orizzontalmente di una posizione
// INPUT:	direction: Direzione lungo la quale spostare la selezione
//				   0: Spostamento a destra
//				   1: Spostamento a sinistra
//--------------------------------------------------------------------------
void CComboList::HSelect( char direction )
{
	if( curCombo == -1 )
		return;
	
	int new_col = entries[curCombo].col;
	int new_combo = -1;
	
	for( int i = 0; i < entries.size(); i++ )
	{
		if( entries[i].row == entries[curCombo].row && !(entries[i].pCombo->GetStyle() & CELL_STYLE_NOSEL) )
		{
			if( direction == 0 )
			{
				if( entries[i].col > entries[curCombo].col )
				{
					if( new_combo == -1 || entries[i].col < new_col )
					{
						new_col = entries[i].col;
						new_combo = i;
					}
				}
			}
			else
			{
				if( entries[i].col < entries[curCombo].col )
				{
					if( new_combo == -1 || entries[i].col > new_col )
					{
						new_col = entries[i].col;
						new_combo = i;
					}
				}
			}
		}
	}
	
	if( new_combo != -1 )
	{
		CurDeselect();
		curCombo = new_combo;
		CurSelect();
		return;
	}

	bipbip();
}

//--------------------------------------------------------------------------
// Sposta la selezione all'ultima riga nella colonna corrente
//--------------------------------------------------------------------------
void CComboList::LastRow()
{
	if( curCombo == -1 )
		return;
	
	int new_row = entries[curCombo].row;
	int new_combo = -1;
	
	for( int i = 0; i < entries.size(); i++ )
	{
		if( entries[i].row > new_row && entries[i].col == entries[curCombo].col && !(entries[i].pCombo->GetStyle() & CELL_STYLE_NOSEL) )
		{
			new_row = entries[i].row;
			new_combo = i;
		}
	}
	
	if( new_combo != -1 )
	{
		CurDeselect();
		curCombo = new_combo;
		CurSelect();
	}
}

//--------------------------------------------------------------------------
// Sposta la selezione alla prima riga nella colonna corrente
//--------------------------------------------------------------------------
void CComboList::FirstRow()
{
	if( curCombo == -1 )
		return;
	
	int new_row = entries[curCombo].row;
	int new_combo = -1;
	
	for( int i = 0; i < entries.size(); i++ )
	{
		if( entries[i].row < new_row && entries[i].col == entries[curCombo].col && !(entries[i].pCombo->GetStyle() & CELL_STYLE_NOSEL) )
		{
			new_row = entries[i].row;
			new_combo = i;
		}
	}
	
	if( new_combo != -1 )
	{
		CurDeselect();
		curCombo = new_combo;
		CurSelect();
	}
}

//--------------------------------------------------------------------------
// Gestione dei tasti della lista di combo boxes
//--------------------------------------------------------------------------
int CComboList::ManageKey( int key )
{
	SetConfirmRequiredBeforeNextXYMovement(true);

	switch( key )
	{
		case K_CTRL_C:
			if( curCombo != -1 )
			{
				entries[curCombo].pCombo->CopyToClipboard();
			}
			break;

		case K_CTRL_V:
			if( curCombo != -1 )
			{
				entries[curCombo].pCombo->PasteFromClipboard();
				return 1;
			}
			break;

		case K_DOWN:
			VSelect(0);
			break;

		case K_UP:
			VSelect(1);
			break;

		case K_RIGHT:
			HSelect(0);
			break;

		case K_LEFT:
			HSelect(1);
			break;

		case K_PAGEUP:
			FirstRow();
			break;

		case K_PAGEDOWN:
			LastRow();
			break;

		default:
			if( editFlag )
			{
				return CurEdit( key );
			}
			break;
	}

	return 0;
}

//--------------------------------------------------------------------------
// Setta la modalita' di editing per la lista
// INPUT:	editmod: Modalita' di editing
//				 0: Non editabile
//				 1: Editabile
//--------------------------------------------------------------------------
void CComboList::SetEdit( bool mode )
{
	editFlag = mode;
	
	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		entries[i].pCombo->SetEdit( editFlag );
	}
	
	if( curCombo != -1 )
	{
		if( entries[curCombo].pCombo->IsSelected() )
		{
			CurSelect();
		}
	}
}
