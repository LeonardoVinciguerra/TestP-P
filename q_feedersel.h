//---------------------------------------------------------------------------
//
// Name:        q_feedersel.h
// Author:      
// Created:     23/05/2012
// Description: Finestra selezione caricatore
//
//---------------------------------------------------------------------------

#ifndef __Q_FEEDERSEL_H
#define __Q_FEEDERSEL_H

#include "c_window.h"
#include "c_combo.h"
#include "c_combolist.h"
#include "q_feederfile.h"


class FeederSelect
{
public:
	FeederSelect( CWindow* parent, char* pack = 0 );
	~FeederSelect();

	int Activate();

private:
	CWindow* Window;
	FeederFile* file;
	char packtxt[21];
	int rec;
	int nrec;
	int firstrec;
	int maxrec;
	int err;
	CarDat car;
	C_Combo* ComboCar;
	C_Combo* ComboPack;
	C_Combo* ComboTipo;
	CComboList* ComboSet;

	void FSel_PGUP(void);
	void FSel_PGDN(void);
	void FSel_CtrlPGUP(void);
	void FSel_CtrlPGDN(void);
	void Read_Show(int rec);
};

#endif
