//---------------------------------------------------------------------------
//
// Name:        c_combolist.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: CComboList class definition
//
//---------------------------------------------------------------------------

#ifndef __C_COMBOLIST_H
#define __C_COMBOLIST_H

#include <vector>
#include "c_combo.h"
#include "c_window.h"

using namespace std;


struct ComboList_Entry
{
	C_Combo* pCombo;			// Puntatore alla combo box corrente
	int row;
	int col;
};


// Classe C_ComboList per la gestione di una lista di caselle di immissione testo
class CComboList
{
	public:
		CComboList( CWindow* parentWin );
		~CComboList();
		
		void Add( C_Combo* InitCombo, int row, int col );
		
		void SetEdit( bool mode );
		bool IsEditOn() { return editFlag; }
		
		void Show();
		
		int CurEdit( int key );

		void SelectFirst();
		void CurSelect();
		void CurDeselect();
		void LastRow();
		void FirstRow();
		
		int ManageKey( int key );

		int GetCurRow() { return entries[curCombo].row; }
		int GetCurCol() { return entries[curCombo].col; }

		C_Combo* GetCurCombo() { return entries[curCombo].pCombo; }

	protected:
		void HSelect( char direction );
		void VSelect( char direction );
		
		CWindow* parent;	// Handler della finestra associata alla combo list
		bool editFlag;		// Flag che abilita l'editing
		
		int curCombo;		// Combo corrente della lista
		vector<ComboList_Entry> entries;
};

#endif
