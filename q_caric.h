//---------------------------------------------------------------------------
// Name:        q_caric.h
// Author:      Walter Moretti / Simone
// Created:     
// Description: Gestione della tabella dei caricatori.
//---------------------------------------------------------------------------

#ifndef __Q_CARIC_H
#define __Q_CARIC_H

#include "c_win_table.h"

int G_TCaric( int carstart, char* tc, char* pk );

void Feeder_SeqPickPosition();
// Gestione autoapprendimento sequenziale Z offset dei caricatori in uso
void FeederZSeq_Auto();
void FeederZSeq_Man();


//---------------------------------------------------------------------------
// finestra: Feeders configuration
//---------------------------------------------------------------------------
class FeedersConfigUI : public CWindowTable
{
public:
	FeedersConfigUI( CWindow* parent );
	~FeedersConfigUI();

protected:
	void onInit();
	void onShow();
	void onRefresh();
	void onEdit();
	void onShowMenu();
	bool onKeyPress( int key );
	void onClose();

private:
	int onEditOverride();
	int onFeederData();
	int onPackageTable();
	int csvToSharedFolder();
	int csvToUSBDevice();
	int onMenuExit();

	bool vSelect( int key );

	GUI_SubMenu* SM_PickH;
	GUI_SubMenu* SM_ExportTo;
};


#endif
