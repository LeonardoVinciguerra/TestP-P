//---------------------------------------------------------------------------
// Name:        q_caric.cpp
// Author:      Walter Moretti / Simone
// Created:     
// Description: Gestione della tabella dei caricatori.
//---------------------------------------------------------------------------
#include "q_caric.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <unistd.h>
#include "keyutils.h"
#include "msglist.h"
#include "q_cost.h"
#include "q_gener.h"
#include "q_wind.h"
#include "q_tabe.h"
#include "q_help.h"
#include "q_files.h"
#include "q_prog.h"
#include "q_oper.h"
#include "q_carint.h"
#include "q_assem.h"
#include "q_dosat.h"
#include "q_carobj.h"
#include "q_ugeobj.h"
#include "q_net.h"
#include "q_init.h"
#include "q_packages.h"
#include "q_feeders.h"
#include "q_decode.h"

#include "q_conf.h"
#include "lnxdefs.h"
#include "strutils.h"
#include "fileutils.h"

#include "c_inputbox.h"
#include "c_win_imgpar.h"
#include "c_win_par.h"
#include "gui_desktop.h"

#include <mss.h>


// Max. n. di record in display
#define MAXRCAR			8


extern GUI_DeskTop* guiDeskTop;
extern FeederFile* CarFile;
extern FeederClass* Caricatore;
extern SPackageData currentLibPackages[MAXPACK];


extern char auto_text1[50];
extern char auto_text2[50];
extern char auto_text3[50];

bool feedersConfigRefresh = false;


int errDB=0;  //flag di errore per disallineamento configurazione-database

// Le strutture di dati per I/O su file sono definite in q_tabe.h.
// Dichiarazioni public per il modulo.

int car_open;                        // flag: check stato file caricatori
struct CarDat KK_Caric[MAXRCAR+1];  // struct dati caricatori
struct CarDat carCurData;           // struttura dati selezionata
extern struct CfgHeader QHeader;          // struct memo parametri vari.
extern struct CfgParam  QParam;
extern struct CarInt_data* CarList;
int Csequenza;					  // memo sequenza caricatori

int carCurRow;
int carCurRecord;

extern int ConfDBLink[MAXMAG];

//legge i record della tabella a partire da roffset
void TCaric_ReadRecs(int roffset);

void CInfo_EndAll(void);


//-----------  FINESTRE DATI CARICATORI - START  ----------------------------

CWindowParams* pWinFeederData = 0;


int CheckFeederUsed( struct CarDat caric );
int TCaric_UpdateDB( int isDel = 0);
int CInfo_AppZOff_Auto();
int CInfo_AppZOff_Man();
int CInfo_Av();
int FeederPosTeaching();


//---------------------------------------------------------------------------
// finestra: Feeders info (normal)
//---------------------------------------------------------------------------
class FeedersInfoNormalUI : public CWindowParams
{
public:
	FeedersInfoNormalUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 6 );
		SetClientAreaSize( 53, 7 );
		SetTitle( MsgGetString(Msg_00888) );

		SM_ZTeach = new GUI_SubMenu();
		SM_ZTeach->Add( MsgGetString(Msg_00731), K_F1, 0, NULL, CInfo_AppZOff_Auto ); // Automatico
		SM_ZTeach->Add( MsgGetString(Msg_00730), K_F2, 0, NULL, CInfo_AppZOff_Man ); // Manuale
	}

	~FeedersInfoNormalUI()
	{
		delete SM_ZTeach;
	}

	typedef enum
	{
		POS_X,
		POS_Y,
		PICK_HEIGHT,
		QUANTITY,
		CHECK_POS,
		CHECK_NUM
	} combo_labels;

protected:
	void onInit()
	{
		char buf[32];

		// create combos
		snprintf( buf, 32, "%s X :", MsgGetString(Msg_00503) );
		m_combos[POS_X] =       new C_Combo(  3, 1, buf, 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Y] =       new C_Combo( 36, 1, "Y :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[PICK_HEIGHT] = new C_Combo(  3, 2, MsgGetString(Msg_00462), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[QUANTITY] =    new C_Combo(  3, 3, MsgGetString(Msg_01043), 8, CELL_TYPE_UINT );
		m_combos[CHECK_POS] =   new C_Combo(  3, 4, MsgGetString(Msg_00034), 4, CELL_TYPE_YN );
		m_combos[CHECK_NUM] =   new C_Combo(  3, 5, MsgGetString(Msg_00035), 8, CELL_TYPE_UINT );

		// set params
		m_combos[POS_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[POS_Y]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[PICK_HEIGHT]->SetVMinMax( -50.f, 50.f );
		m_combos[QUANTITY]->SetVMinMax( 0, 500000 );
		m_combos[CHECK_NUM]->SetVMinMax( 0, 100 );

		// add to combo list
		m_comboList->Add( m_combos[POS_X],       0, 0 );
		m_comboList->Add( m_combos[POS_Y],       0, 1 );
		m_comboList->Add( m_combos[PICK_HEIGHT], 1, 0 );
		m_comboList->Add( m_combos[QUANTITY],    2, 0 );
		m_comboList->Add( m_combos[CHECK_POS],   3, 0 );
		m_comboList->Add( m_combos[CHECK_NUM],   4, 0 );
	}

	void onRefresh()
	{
		m_combos[POS_X]->SetTxt( KK_Caric[carCurRow].C_xcar );
		m_combos[POS_Y]->SetTxt( KK_Caric[carCurRow].C_ycar );
		m_combos[PICK_HEIGHT]->SetTxt( KK_Caric[carCurRow].C_offprel );
		m_combos[QUANTITY]->SetTxt( KK_Caric[carCurRow].C_quant );
		m_combos[CHECK_POS]->SetTxtYN( KK_Caric[carCurRow].C_checkPos );
		m_combos[CHECK_NUM]->SetTxt( int(KK_Caric[carCurRow].C_checkNum) );
	}

	void onEdit()
	{
		//TODO: controllare da altra parte ???
		if( CheckFeederUsed( KK_Caric[carCurRow] ) == MAG_USEDBY_OTHERS )
		{
			bipbip();
			W_Mess( ERRMAG_USEDBY_OTHERS );
			return;
		}

		KK_Caric[carCurRow].C_xcar     = m_combos[POS_X]->GetFloat();
		KK_Caric[carCurRow].C_ycar     = m_combos[POS_Y]->GetFloat();
		KK_Caric[carCurRow].C_offprel  = m_combos[PICK_HEIGHT]->GetFloat();
		KK_Caric[carCurRow].C_quant    = m_combos[QUANTITY]->GetInt();
		KK_Caric[carCurRow].C_checkPos = m_combos[CHECK_POS]->GetYN();
		KK_Caric[carCurRow].C_checkNum = m_combos[CHECK_NUM]->GetInt();

		//TODO: salvare da altra parte ???
		TCaric_UpdateDB();
		CarFile->SaveRecX( carCurRecord, KK_Caric[carCurRow] );
		CarFile->SaveFile();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01424), K_F3, 0, SM_ZTeach, NULL ); // Apprendimento quota prelievo
		m_menu->Add( MsgGetString(Msg_00026), K_F4, 0, NULL, FeederPosTeaching ); // Apprendimento posizione prelievo
		m_menu->Add( MsgGetString(Msg_00440), K_SHIFT_F4, 0, NULL, CInfo_Av );  // Avanzamento caricatore
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				SM_ZTeach->Show();
				return true;

			case K_F4:
				FeederPosTeaching();
				return true;

			case K_SHIFT_F4:
				CInfo_Av();
				return true;

			default:
				break;
		}

		return false;
	}

	GUI_SubMenu* SM_ZTeach; // sub menu apprendimento z caricatori
};


//---------------------------------------------------------------------------
// finestra: Feeders info (double)
//---------------------------------------------------------------------------
class FeedersInfoDoubleUI : public CWindowParams
{
public:
	FeedersInfoDoubleUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 6 );
		SetClientAreaSize( 53, 8 );
		SetTitle( MsgGetString(Msg_00888) );

		SM_ZTeach = new GUI_SubMenu();
		SM_ZTeach->Add( MsgGetString(Msg_00731), K_F1, 0, NULL, CInfo_AppZOff_Auto ); // Automatico
		SM_ZTeach->Add( MsgGetString(Msg_00730), K_F2, 0, NULL, CInfo_AppZOff_Man );  // Manuale
	}

	~FeedersInfoDoubleUI()
	{
		delete SM_ZTeach;
	}

	typedef enum
	{
		POS_X,
		POS_Y,
		PICK_HEIGHT,
		INC_X,
		INC_Y,
		QUANTITY,
		CHECK_POS,
		CHECK_NUM
	} combo_labels;

protected:
	void onInit()
	{
		char buf[32];

		// create combos
		snprintf( buf, 32, "%s X :", MsgGetString(Msg_00503) );
		m_combos[POS_X] =       new C_Combo(  3, 1, buf, 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Y] =       new C_Combo( 36, 1, "Y :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[PICK_HEIGHT] = new C_Combo(  3, 2, MsgGetString(Msg_00462), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		snprintf( buf, 32, "%s X :", MsgGetString(Msg_00516) );
		m_combos[INC_X] =       new C_Combo(  3, 3, buf, 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[INC_Y] =       new C_Combo( 36, 3, "Y :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[QUANTITY] =    new C_Combo(  3, 4, MsgGetString(Msg_01043), 8, CELL_TYPE_UINT );
		m_combos[CHECK_POS] =   new C_Combo(  3, 5, MsgGetString(Msg_00034), 4, CELL_TYPE_YN );
		m_combos[CHECK_NUM] =   new C_Combo(  3, 6, MsgGetString(Msg_00035), 8, CELL_TYPE_UINT );

		// set params
		m_combos[POS_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[POS_Y]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[PICK_HEIGHT]->SetVMinMax( -50.f, 50.f );
		m_combos[INC_X]->SetVMinMax( -100.f, 100.f );
		m_combos[INC_Y]->SetVMinMax( -100.f, 100.f );
		m_combos[QUANTITY]->SetVMinMax( 0, 500000 );
		m_combos[CHECK_NUM]->SetVMinMax( 0, 100 );

		// add to combo list
		m_comboList->Add( m_combos[POS_X],       0, 0 );
		m_comboList->Add( m_combos[POS_Y],       0, 1 );
		m_comboList->Add( m_combos[PICK_HEIGHT], 1, 0 );
		m_comboList->Add( m_combos[INC_X],       2, 0 );
		m_comboList->Add( m_combos[INC_Y],       2, 1 );
		m_comboList->Add( m_combos[QUANTITY],    3, 0 );
		m_comboList->Add( m_combos[CHECK_POS],   4, 0 );
		m_comboList->Add( m_combos[CHECK_NUM],   5, 0 );
	}

	void onRefresh()
	{
		m_combos[POS_X]->SetTxt( KK_Caric[carCurRow].C_xcar );
		m_combos[POS_Y]->SetTxt( KK_Caric[carCurRow].C_ycar );
		m_combos[PICK_HEIGHT]->SetTxt( KK_Caric[carCurRow].C_offprel );
		m_combos[INC_X]->SetTxt( KK_Caric[carCurRow].C_incx );
		m_combos[INC_Y]->SetTxt( KK_Caric[carCurRow].C_incy );
		m_combos[QUANTITY]->SetTxt( KK_Caric[carCurRow].C_quant );
		m_combos[CHECK_POS]->SetTxtYN( KK_Caric[carCurRow].C_checkPos );
		m_combos[CHECK_NUM]->SetTxt( int(KK_Caric[carCurRow].C_checkNum) );
	}

	void onEdit()
	{
		//TODO: controllare da altra parte ???
		if( CheckFeederUsed( KK_Caric[carCurRow] ) == MAG_USEDBY_OTHERS )
		{
			bipbip();
			W_Mess( ERRMAG_USEDBY_OTHERS );
			return;
		}

		KK_Caric[carCurRow].C_xcar     = m_combos[POS_X]->GetFloat();
		KK_Caric[carCurRow].C_ycar     = m_combos[POS_Y]->GetFloat();
		KK_Caric[carCurRow].C_offprel  = m_combos[PICK_HEIGHT]->GetFloat();
		KK_Caric[carCurRow].C_incx     = m_combos[INC_X]->GetFloat();
		KK_Caric[carCurRow].C_incy     = m_combos[INC_Y]->GetFloat();
		KK_Caric[carCurRow].C_quant    = m_combos[QUANTITY]->GetInt();
		KK_Caric[carCurRow].C_checkPos = m_combos[CHECK_POS]->GetYN();
		KK_Caric[carCurRow].C_checkNum = m_combos[CHECK_NUM]->GetInt();

		//TODO: salvare da altra parte ???
		TCaric_UpdateDB();
		CarFile->SaveRecX( carCurRecord, KK_Caric[carCurRow] );
		CarFile->SaveFile();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01424), K_F3, 0, SM_ZTeach, NULL ); // Apprendimento prelievo Z
		m_menu->Add( MsgGetString(Msg_00026), K_F4, 0, NULL, FeederPosTeaching ); // Apprendimento posizione prelievo
		m_menu->Add( MsgGetString(Msg_00440), K_SHIFT_F4, 0, NULL, CInfo_Av );  // Avanzamento caricatore
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				SM_ZTeach->Show();
				return true;

			case K_F4:
				FeederPosTeaching();
				return true;

			case K_SHIFT_F4:
				CInfo_Av();
				return true;

			default:
				break;
		}

		return false;
	}

	GUI_SubMenu* SM_ZTeach; // sub menu apprendimento z caricatori
};


//---------------------------------------------------------------------------
// finestra: Feeders info (tray)
//---------------------------------------------------------------------------
class FeedersInfoTrayUI : public CWindowParams
{
public:
	FeedersInfoTrayUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 6 );
		SetClientAreaSize( 53, 7 );
		SetTitle( MsgGetString(Msg_00889) );
	}

	typedef enum
	{
		POS_X,
		POS_Y,
		PICK_HEIGHT,
		INC_X,
		INC_Y,
		NUM_X,
		NUM_Y,
		QUANTITY
	} combo_labels;

private:
	int onMenuExit()
	{
		onRefresh();

		return 1;
	}

protected:
	void onInit()
	{
		char buf[32];

		// create combos
		snprintf( buf, 32, "%s X :", MsgGetString(Msg_00503) );
		m_combos[POS_X] =       new C_Combo(  3, 1, buf, 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Y] =       new C_Combo( 36, 1, "Y :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[PICK_HEIGHT] = new C_Combo(  3, 2, MsgGetString(Msg_00462), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		snprintf( buf, 32, "%s X :", MsgGetString(Msg_00516) );
		m_combos[INC_X] =       new C_Combo(  3, 3, buf, 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[INC_Y] =       new C_Combo( 36, 3, "Y :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[NUM_X] =       new C_Combo(  3, 4, MsgGetString(Msg_00514), 8, CELL_TYPE_UINT );
		m_combos[NUM_Y] =       new C_Combo( 36, 4, "Y :", 8, CELL_TYPE_UINT );
		m_combos[QUANTITY] =    new C_Combo(  3, 5, MsgGetString(Msg_01043), 8, CELL_TYPE_UINT );

		// set params
		m_combos[POS_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[POS_Y]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[PICK_HEIGHT]->SetVMinMax( -50.f, 50.f );
		m_combos[INC_X]->SetVMinMax( -100.f, 100.f );
		m_combos[INC_Y]->SetVMinMax( -100.f, 100.f );
		m_combos[NUM_X]->SetVMinMax( 0, 9999 );
		m_combos[NUM_Y]->SetVMinMax( 0, 9999 );
		m_combos[QUANTITY]->SetVMinMax( 0, 500000 );

		// add to combo list
		m_comboList->Add( m_combos[POS_X],       0, 0 );
		m_comboList->Add( m_combos[POS_Y],       0, 1 );
		m_comboList->Add( m_combos[PICK_HEIGHT], 1, 0 );
		m_comboList->Add( m_combos[INC_X],       2, 0 );
		m_comboList->Add( m_combos[INC_Y],       2, 1 );
		m_comboList->Add( m_combos[NUM_X],       3, 0 );
		m_comboList->Add( m_combos[NUM_Y],       3, 1 );
		m_comboList->Add( m_combos[QUANTITY],    4, 0 );
	}

	void onRefresh()
	{
		m_combos[POS_X]->SetTxt( KK_Caric[carCurRow].C_xcar );
		m_combos[POS_Y]->SetTxt( KK_Caric[carCurRow].C_ycar );
		m_combos[PICK_HEIGHT]->SetTxt( KK_Caric[carCurRow].C_offprel );
		m_combos[INC_X]->SetTxt( KK_Caric[carCurRow].C_incx );
		m_combos[INC_Y]->SetTxt( KK_Caric[carCurRow].C_incy );
		m_combos[NUM_X]->SetTxt( KK_Caric[carCurRow].C_nx );
		m_combos[NUM_Y]->SetTxt( KK_Caric[carCurRow].C_ny );
		m_combos[QUANTITY]->SetTxt( KK_Caric[carCurRow].C_quant );
	}

	void onEdit()
	{
		KK_Caric[carCurRow].C_xcar     = m_combos[POS_X]->GetFloat();
		KK_Caric[carCurRow].C_ycar     = m_combos[POS_Y]->GetFloat();
		KK_Caric[carCurRow].C_offprel  = m_combos[PICK_HEIGHT]->GetFloat();
		KK_Caric[carCurRow].C_incx     = m_combos[INC_X]->GetFloat();
		KK_Caric[carCurRow].C_incy     = m_combos[INC_Y]->GetFloat();
		KK_Caric[carCurRow].C_nx       = m_combos[NUM_X]->GetInt();
		KK_Caric[carCurRow].C_ny       = m_combos[NUM_Y]->GetInt();
		KK_Caric[carCurRow].C_quant    = m_combos[QUANTITY]->GetInt();

		if( KK_Caric[carCurRow].C_quant > KK_Caric[carCurRow].C_nx * KK_Caric[carCurRow].C_ny )
		{
			KK_Caric[carCurRow].C_quant = KK_Caric[carCurRow].C_nx * KK_Caric[carCurRow].C_ny;

			m_combos[QUANTITY]->SetTxt( KK_Caric[carCurRow].C_quant );
			bipbip();
		}

		//TODO: salvare da altra parte ???
		CarFile->SaveRecX( carCurRecord, KK_Caric[carCurRow] );
		CarFile->SaveFile();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01424), K_F3, 0, NULL, CInfo_AppZOff_Man ); // Apprendimento quota prelievo
		m_menu->Add( MsgGetString(Msg_00026), K_F4, 0, NULL, FeederPosTeaching ); // Apprendimento posizione prelievo
		m_menu->Add( MsgGetString(Msg_00325), K_F6, 0, NULL, auto_incr, boost::bind( &FeedersInfoTrayUI::onMenuExit, this ) );  // Apprendimento incrementi vassoio
		m_menu->Add( MsgGetString(Msg_00440), K_SHIFT_F4, 0, NULL, CInfo_Av );  // Avanzamento caricatore
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				CInfo_AppZOff_Man();
				return true;

			case K_F4:
				FeederPosTeaching();
				return true;

			case K_F6:
				auto_incr();
				onRefresh();
				return true;

			case K_SHIFT_F4:
				CInfo_Av();
				return true;

			default:
				break;
		}

		return false;
	}
};


//---------------------------------------------------------------------------
// finestra: Feeders info (TH)
//---------------------------------------------------------------------------
class FeedersInfoTHUI : public CWindowParams
{
public:
	FeedersInfoTHUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 6 );
		SetClientAreaSize( 53, 8 );
		SetTitle( MsgGetString(Msg_00888) );
	}

	typedef enum
	{
		POS_X,
		POS_Y,
		PICK_HEIGHT,
		QUANTITY,
		CHECK_POS,
		CHECK_NUM,
		ADDRESS
	} combo_labels;

protected:
	void onInit()
	{
		char buf[32];

		// create combos
		snprintf( buf, 32, "%s X :", MsgGetString(Msg_00503) );
		m_combos[POS_X]       = new C_Combo(  3, 1, buf, 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Y]       = new C_Combo( 36, 1, "Y :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[PICK_HEIGHT] = new C_Combo(  3, 2, MsgGetString(Msg_00462), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[QUANTITY]    = new C_Combo(  3, 3, MsgGetString(Msg_01043), 8, CELL_TYPE_UINT );
		m_combos[CHECK_POS]   = new C_Combo(  3, 4, MsgGetString(Msg_00034), 4, CELL_TYPE_YN );
		m_combos[CHECK_NUM]   = new C_Combo(  3, 5, MsgGetString(Msg_00035), 8, CELL_TYPE_UINT );
		m_combos[ADDRESS]     = new C_Combo(  3, 6, MsgGetString(Msg_02118), 8, CELL_TYPE_UINT );

		// set params
		m_combos[POS_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[POS_Y]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[PICK_HEIGHT]->SetVMinMax( -50.f, 50.f );
		m_combos[QUANTITY]->SetVMinMax( 0, 500000 );
		m_combos[CHECK_NUM]->SetVMinMax( 0, 100 );
		m_combos[ADDRESS]->SetVMinMax( 1, 15 );

		// add to combo list
		m_comboList->Add( m_combos[POS_X],       0, 0 );
		m_comboList->Add( m_combos[POS_Y],       0, 1 );
		m_comboList->Add( m_combos[PICK_HEIGHT], 1, 0 );
		m_comboList->Add( m_combos[QUANTITY],    2, 0 );
		m_comboList->Add( m_combos[CHECK_POS],   3, 0 );
		m_comboList->Add( m_combos[CHECK_NUM],   4, 0 );
		m_comboList->Add( m_combos[ADDRESS],     5, 0 );
	}

	void onRefresh()
	{
		m_combos[POS_X]->SetTxt( KK_Caric[carCurRow].C_xcar );
		m_combos[POS_Y]->SetTxt( KK_Caric[carCurRow].C_ycar );
		m_combos[PICK_HEIGHT]->SetTxt( KK_Caric[carCurRow].C_offprel );
		m_combos[QUANTITY]->SetTxt( KK_Caric[carCurRow].C_quant );
		m_combos[CHECK_POS]->SetTxtYN( KK_Caric[carCurRow].C_checkPos );
		m_combos[CHECK_NUM]->SetTxt( int(KK_Caric[carCurRow].C_checkNum) );
		m_combos[ADDRESS]->SetTxt( int(KK_Caric[carCurRow].C_thFeederAdd) );
	}

	void onEdit()
	{
		//TODO: controllare da altra parte ???
		if( CheckFeederUsed( KK_Caric[carCurRow] ) == MAG_USEDBY_OTHERS )
		{
			bipbip();
			W_Mess( ERRMAG_USEDBY_OTHERS );
			return;
		}

		KK_Caric[carCurRow].C_xcar        = m_combos[POS_X]->GetFloat();
		KK_Caric[carCurRow].C_ycar        = m_combos[POS_Y]->GetFloat();
		KK_Caric[carCurRow].C_offprel     = m_combos[PICK_HEIGHT]->GetFloat();
		KK_Caric[carCurRow].C_quant       = m_combos[QUANTITY]->GetInt();
		KK_Caric[carCurRow].C_checkPos    = m_combos[CHECK_POS]->GetYN();
		KK_Caric[carCurRow].C_checkNum    = m_combos[CHECK_NUM]->GetInt();
		KK_Caric[carCurRow].C_thFeederAdd = m_combos[ADDRESS]->GetInt();

		//TODO: salvare da altra parte ???
		TCaric_UpdateDB();
		CarFile->SaveRecX( carCurRecord, KK_Caric[carCurRow] );
		CarFile->SaveFile();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01424), K_F3, 0, NULL, CInfo_AppZOff_Man ); // Apprendimento quota prelievo
		m_menu->Add( MsgGetString(Msg_00026), K_F4, 0, NULL, FeederPosTeaching ); // Apprendimento posizione prelievo
		m_menu->Add( MsgGetString(Msg_00440), K_SHIFT_F4, 0, NULL, CInfo_Av );  // Avanzamento caricatore
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				CInfo_AppZOff_Man();
				return true;

			case K_F4:
				FeederPosTeaching();
				return true;

			case K_SHIFT_F4:
				CInfo_Av();
				return true;

			default:
				break;
		}

		return false;
	}
};

//-----------  FINESTRE DATI CARICATORI - END  ------------------------------

//---------------------------------------------------------------------------
// finestra: Tray increments dialog box
//---------------------------------------------------------------------------
class TrayIncrementsUI : public CWindowParams
{
public:
	TrayIncrementsUI( CWindow* parent, int x, int y ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU | WIN_STYLE_EDITMODE_ON );
		SetClientAreaPos( 0, 7 );
		SetClientAreaSize( 32, 5 );
		SetTitle( MsgGetString(Msg_01143) );

		m_x = x;
		m_y = y;
	}

	int GetExitCode() { return m_exitCode; }
	int GetIncrementsX() { return m_x; }
	int GetIncrementsY() { return m_y; }

	typedef enum
	{
		INC_X,
		INC_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[INC_X] = new C_Combo( 4, 1, MsgGetString(Msg_01144), 3, CELL_TYPE_UINT );
		m_combos[INC_Y] = new C_Combo( 4, 3, MsgGetString(Msg_01145), 3, CELL_TYPE_UINT );

		// set params
		m_combos[INC_X]->SetVMinMax( 1, 100 );
		m_combos[INC_Y]->SetVMinMax( 1, 100 );

		// add to combo list
		m_comboList->Add( m_combos[INC_X], 0, 0 );
		m_comboList->Add( m_combos[INC_Y], 1, 0 );
	}

	void onShow()
	{
		tips = new CPan( 20, 2, MsgGetString(Msg_00296), MsgGetString(Msg_00297) );
	}

	void onRefresh()
	{
		m_combos[INC_X]->SetTxt( m_x );
		m_combos[INC_Y]->SetTxt( m_y );
	}

	void onEdit()
	{
		m_x = m_combos[INC_X]->GetInt();
		m_y = m_combos[INC_Y]->GetInt();
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_ESC:
				m_exitCode = WIN_EXITCODE_ESC;
				break;

			case K_ENTER:
				forceExit();
				m_exitCode = WIN_EXITCODE_ENTER;
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		delete tips;
	}

	CPan* tips;
	int m_exitCode;
	int m_x, m_y;
};




// Gestione apprendimento incremento vassoi
// Con check per un solo compon. in X o in Y * W2403 *
//DANY201102
int auto_incr(void)
{
	float old_1x, old_1y, old_2x, old_2y;
	float p_x1, p_y1, p_x2, p_y2, s_x1, s_x2, s_y1, s_y2;
	float centro_1x, centro_1y, centro_2x, centro_2y;

	int nc_riga=carCurRow; // riga in selezione
	int nc_rec =carCurRecord;        // record in selezione
	int ritorno;

	if( KK_Caric[nc_riga].C_codice < 160 )
	{
		return 0;
	}

	TrayIncrementsUI inputBox( 0, KK_Caric[carCurRow].C_nx,  KK_Caric[carCurRow].C_ny );
	inputBox.Show();
	inputBox.Hide();
	if( inputBox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	KK_Caric[carCurRow].C_nx = inputBox.GetIncrementsX();
	KK_Caric[carCurRow].C_ny = inputBox.GetIncrementsY();
	KK_Caric[carCurRow].C_quant = KK_Caric[nc_riga].C_nx * KK_Caric[nc_riga].C_ny;


	//TODO_TV
	Set_Tv(2);         //attiva visualizzazione senza interruzioni

	float c_ax = KK_Caric[nc_riga].C_xcar;
	float c_ay = KK_Caric[nc_riga].C_ycar;


	auto_text1[0]=0;
	snprintf( auto_text2, sizeof(auto_text2), "%s %s", MsgGetString(Msg_00310),KK_Caric[nc_riga].C_comp );
	auto_text3[0]=0;

	char title[180];

	/* app. punto basso sx, integr. basso sx */
	snprintf( title, sizeof(title), "%s - %s", MsgGetString(Msg_00032), MsgGetString(Msg_00326) );
	if( ManualTeaching( &c_ax, &c_ay, title, AUTOAPP_COMP ) )
	{
		ritorno=1;
		p_x1 = c_ax;
		p_y1 = c_ay;
		old_1x=p_x1;
		old_1y=p_y1;
	}
	else
	{
		ritorno=0;
	}

	/* app. punto alto dx, integr. basso sx */
	if(ritorno)
	{
		snprintf( title, sizeof(title), "%s - %s", MsgGetString(Msg_00033), MsgGetString(Msg_00326) );
		if( ManualTeaching( &c_ax, &c_ay, title, AUTOAPP_COMP ) )
		{
			ritorno=1;
			p_x2 = c_ax;
			p_y2 = c_ay;
			old_2x=p_x2;
			old_2y=p_y2;

		}
		else
		{
			ritorno=0;
		}
	}

	/* calcolo centro integr. basso sx, posizion. su integr. alto dx */
	if(ritorno)
	{
		centro_1x=(p_x2-p_x1)/2+p_x1;
		centro_1y=(p_y2-p_y1)/2+p_y1;

		c_ax=(KK_Caric[nc_riga].C_incx*(KK_Caric[nc_riga].C_nx-1))+
		       old_1x;
		c_ay=(KK_Caric[nc_riga].C_incy*(KK_Caric[nc_riga].C_ny-1))+
			  old_1y;
	}

	/* app. punto basso sx, integr. alto dx */
	if(ritorno)
	{
		snprintf( title, sizeof(title), "%s - %s", MsgGetString(Msg_00032), MsgGetString(Msg_00327) );
		if( ManualTeaching( &c_ax, &c_ay, title, AUTOAPP_COMP ) )
		{
			ritorno=1;
			s_x1 = c_ax;
			s_y1 = c_ay;
		}
		else
		{
			ritorno=0;
		}
	}

	/* app. punto alto dx, integr. alto dx */
	if(ritorno)
	{
		c_ax += ( old_2x - old_1x);
		c_ay += ( old_2y - old_1y);

		snprintf( title, sizeof(title), "%s - %s", MsgGetString(Msg_00033), MsgGetString(Msg_00327) );
		if( ManualTeaching( &c_ax, &c_ay, title, AUTOAPP_COMP ) )
		{
			ritorno=1;
			s_x2 = c_ax;
			s_y2 = c_ay;
		}
		else
		{
			ritorno=0;
		}
	}
	if(ritorno)
	{
		centro_2x=(s_x2-s_x1)/2+s_x1;
		centro_2y=(s_y2-s_y1)/2+s_y1;

		/* check per un solo comp. in X o in Y * W2403 */
		if(KK_Caric[nc_riga].C_nx>1)
		{
			KK_Caric[nc_riga].C_incx = (centro_2x-centro_1x) / (KK_Caric[nc_riga].C_nx-1);
		}
		else
		{
			KK_Caric[nc_riga].C_incx = 0; // 1 solo comp. in X
		}

		if(KK_Caric[nc_riga].C_ny>1)
		{
			KK_Caric[nc_riga].C_incy = (centro_2y-centro_1y) / (KK_Caric[nc_riga].C_ny-1);
		}
		else
		{
			KK_Caric[nc_riga].C_incy = 0; // 1 solo comp. in Y
		}

		//SMOD140405
		//Setta come posizione vassoio il centro del primo componente
		KK_Caric[nc_riga].C_xcar=centro_1x;
		KK_Caric[nc_riga].C_ycar=centro_1y;

		CarFile->SaveRecX(nc_rec,KK_Caric[nc_riga]);
		CarFile->SaveFile();
	}

	Set_Tv(3);         //disattiva visualizzazione senza interruzioni

	return(ritorno);
}


//---------------------------------------------------------------------------
// Apprendimento posizione caricatore
//---------------------------------------------------------------------------
int FeederPosTeaching()
{
	Caricatore->SetData( KK_Caric[carCurRow] );

	int prevBright = GetImageBright();
	int prevContrast = GetImageContrast();

	if( !Caricatore->TeachPosition() )
	{
		SetImgBrightCont( prevBright, prevContrast );
	}

	TCaric_ReadRecs(carCurRecord-carCurRow);
	feedersConfigRefresh = true;
	return 1;
}

// Avanzamento caricatore selezionato
void av_caric(int code)
{
	struct CarDat dat;
	int open=0;
	
	// non eseguire alcuna operazione se si tratta di un vassoio
	if( code >= FIRSTTRAY )
	{
		return;
	}
	
	//TODO_TV: sequenza Set_tv -> use_tv da controllare
	Set_Tv(1);
	
	if(CarFile==NULL)
	{
		CarFile=new FeederFile(QHeader.Conf_Default);
		open=1;
	}
	
	if(!CarFile->opened)
	{
		if(open)
		{
			delete CarFile;
			CarFile=NULL;
		}
		return;
	}
	
	FeederClass *car=new FeederClass(CarFile);
	car->SetCode(code);
	dat=car->GetData();
	
	playLiveVideo( CAMERA_HEAD );
	
	delay(80);
	car->SetNComp(0);
	car->Avanza();
	
	dat=car->GetData();
	
	if(dat.C_quant!=0)
	{
		dat.C_quant--;
	}
	
	DecMagaComp(dat.C_codice);
	
	car->WaitReady();
	car->UpdateStatus();
	//delay(250);
	Set_Tv(0);
	
	delete car;
	
	if(open)
	{
		delete CarFile;
		CarFile=NULL;
	}
} // av_cari


//------------------------------------------------------------------------------------
// Apprendimento sequenziale posizione di prelievo caricatori in uso
//------------------------------------------------------------------------------------
void Feeder_SeqPickPosition()
{
	bool open = false;
	if( !CarFile )
	{
		CarFile = new FeederFile( QHeader.Conf_Default );
		open = true;
	}

	if( !CarFile->opened )
	{
		if( open )
		{
			delete CarFile;
			CarFile = 0;
		}
		return;
	}
	FeederClass* car = new FeederClass( CarFile );

	int car_flag[MAXCAR];
	for( int i = 0; i < MAXCAR; i++ )
	{
		car_flag[i] = 0;
	}

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	int code = Search_car( 0 );
	while( code != 0 )
	{
		if( code > 1 )
		{
			if( IsValidCarCode(code) )
			{
				car->SetCode(code);
				int crec = GetCarRec(code);
		
				if( !car_flag[crec] )
				{
					car_flag[crec] = 1;

					// Chiedi avanzamento solo per i caricatori (no vassoi)
					if( car->GetData().C_codice < FIRSTTRAY )
					{
						car->GoPos( 0 );

						if( W_Deci(1,MsgGetString(Msg_00663)) )
						{
							Wait_PuntaXY();
							av_caric(car->GetData().C_codice);

							// Ricarica dati per oggetto caricatore
							car->SetCode(code);
						}
						else
						{
							Wait_PuntaXY();
						}
					}

					if( !car->TeachPosition() )
					{
						break;
					}
				}
			}
		}

		code = Search_car(1);
	}

	delete car;

	if( open )
	{
		delete CarFile;
		CarFile = 0;
	}

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
}


//------------------------------------------------------------------------------------
// Apprendimento z offset caricatore
//   nrec: numero record in tabella caricatori
//   uge : ugello da utilizzare (se=0 chiede quale ugello utilizzare)
//------------------------------------------------------------------------------------
int Feeder_AppZOff( int nrec, char tool, bool return_xyzero )
{
	struct CarDat carRec;
	SPackageData packRec;

	CarFile->ReadRec( nrec, carRec );

	if( (carRec.C_PackIndex != 0) && (carRec.C_PackIndex <= MAXPACK) )
	{
		packRec = currentLibPackages[carRec.C_PackIndex-1];
		if( ischar_legal( CHARSET_TOOL, packRec.tools[0]) )
		{
			tool = packRec.tools[0];
		}
	}

	int nozzle = 1;

	int new_uge[2] = { -1, -1 };

	if( tool == 0 )
	{
		tool = 'A';
		if( !ToolNozzleSelectionWin( 0, tool, nozzle ) )
		{
			return 0;
		}
	}
	else
	{
		CfgUgelli toolData;
		Ugelli->ReadRec( toolData, tool-'A' ); // lettura del record

		if( toolData.NozzleAllowed == UG_P2 )
		{
			nozzle = 2;
		}
	}

	if( nozzle == 1 )
	{
		Ugelli->SetFlag( 1, 0 );
		new_uge[0] = tool;
	}
	else
	{
		Ugelli->SetFlag( 0, 1 );
		new_uge[1] = tool;
	}

	if( !Ugelli->DoOper(new_uge) )
	{
		return 0;
	}

	if( (carRec.C_PackIndex != 0) && (carRec.C_PackIndex <= MAXPACK) )
	{
		PuntaRotDeg( Get_PickThetaPosition( carRec.C_codice, packRec ), nozzle );
	}


	float zPos;
	float xPos,yPos;

	if(carRec.C_codice<FIRSTTRAY)
	{
		xPos = carRec.C_xcar;
		yPos = carRec.C_ycar;
		zPos=(GetZCaricPos(nozzle)-carRec.C_offprel);
	}
	else
	{
		FeederClass feeder(CarFile,carRec.C_codice);
		feeder.GetTrayPosition(xPos,yPos);
		zPos=(GetPianoZPos(nozzle)-carRec.C_offprel);
	}

	while(!Check_PuntaRot(nozzle));

	int mode=APPRENDZPOS_NOVACUO;

	if(!return_xyzero)
	{
		mode|=APPRENDZPOS_NOXYZERORET;
	}

	if( AutoAppZPosMm(MsgGetString(Msg_01424), nozzle, xPos, yPos, zPos, mode, 0, 0, &carRec) )
	{
		if(carRec.C_codice<FIRSTTRAY)
		{
			carRec.C_offprel=-(zPos-GetZCaricPos(nozzle));
		}
		else
		{
			carRec.C_offprel=-(zPos-GetPianoZPos(nozzle));
		}

		CarFile->SaveRecX(nrec,carRec);
		CarFile->SaveFile();
		PuntaRotDeg(0,nozzle);

		while(!Check_PuntaRot(nozzle));
	}
	else
	{
		PuntaRotDeg(0,nozzle);

		while(!Check_PuntaRot(nozzle));

		return 0;
	}

	return 1;
}



//---------------------------------------------------------------------------------
// Apprendimento quota di prelievo caricatore tramite la misurazione del valore del vuoto.
// Parametri di ingresso:
//
// Valore di ritorno:
//    0 se ricerca fallita
//   -1 interrotto dall'utente
//    1 se Ok
//----------------------------------------------------------------------------------
#define ZV_PRE_MOVE_MM              3.f
#define ZV_POST_MOVE_MM             4.f
#define ZV_DELTA_STEPS              3
#define ZV_DELTA_DIM_MM             5.f
#define ZV_DELTA_POS1_MM            1.5f
#define ZV_DELTA_POS2_MM            2.5f

int Feeder_AppZOff_Vacuo( int nrec, char tool, bool return_xyzero )
{
	struct CarDat carRec;
	CarFile->ReadRec( nrec, carRec );
	if( carRec.C_PackIndex == 0 || carRec.C_codice >= FIRSTTRAY )
	{
		W_Mess( MsgGetString(Msg_00077) ); //TODO: decidere se fare anche in vassoio o meno
		return 1;
	}

	int nozzle = 1;
	int new_uge[2] = { -1, -1 };

	if( nozzle == 1 )
	{
		Ugelli->SetFlag(1,0);
		new_uge[0] = tool;
	}
	else
	{
		Ugelli->SetFlag(0,1);
		new_uge[1] = tool;
	}

	if( !Ugelli->DoOper(new_uge) )
	{
		return 0;
	}


	SPackageData packRec = currentLibPackages[carRec.C_PackIndex-1];

	// comp isn't on tray
	float xPos = carRec.C_xcar;
	float yPos = carRec.C_ycar;

	// calcola posizione di misura
	float delta_measure = (packRec.orientation == 0) ? packRec.x / 2.f : packRec.y / 2.f;
	if( delta_measure <= 0.02f )
	{
		char buf[120];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_00075), carRec.C_comp );
		W_Mess( buf );
		return 0;
	}

	if( delta_measure > ZV_DELTA_DIM_MM )
		delta_measure += ZV_DELTA_POS2_MM;
	else
		delta_measure += ZV_DELTA_POS1_MM;

	int mag = carRec.C_codice/10;
	if( mag < 4 )
	{
		xPos -= delta_measure;
	}
	else if( mag < 8 )
	{
		yPos += delta_measure;
	}
	else if( mag < 12 )
	{
		xPos += delta_measure;
	}
	else
	{
		yPos -= delta_measure;
	}

	int retVal = 1;

	while( 1 )
	{
		// setta valori
		snprintf( auto_text1, sizeof(auto_text1), "   %s %d", MsgGetString(Msg_00311), carRec.C_codice );
		snprintf( auto_text2, sizeof(auto_text2), "   %s %s", MsgGetString(Msg_00310), carRec.C_comp );
		*auto_text3 = 0;

		// chiede conferma (o modifica) della posizione di misura
		if( !ManualTeaching( &xPos, &yPos, MsgGetString(Msg_01424), AUTOAPP_COMP ) )
		{
			// interrotto dall'utente
			retVal = -1;
			break;
		}

		// porta punta in posizione
		Set_Finec(ON);
		NozzleXYMove_N( xPos, yPos, nozzle );
		Wait_PuntaXY();
		Set_Finec(OFF);

		// esegue misurazione
		float zPos = GetZCaricPos(nozzle) - carRec.C_offprel - ZV_PRE_MOVE_MM;
		int steppos = zPos * QHeader.Step_Trasl1;
		int stepposLim = (zPos + ZV_PRE_MOVE_MM + ZV_POST_MOVE_MM) * QHeader.Step_Trasl1;
		retVal = VacuoFindZPosition( nozzle, ZV_DELTA_STEPS, steppos, stepposLim );

		if( retVal == -1 )
		{
			// interrotto dall'utente
			break;
		}
		else if( retVal == 0 )
		{
			// ricerca fallita (superati limiti)
			//TODO - chiedere utente cosa fare
			W_Mess( MsgGetString(Msg_00076) );
			break;
		}
		else if( retVal == 1 )
		{
			#ifdef __DEBUG
			char str[32];
			snprintf( str, sizeof(str), "%.3f -> %.3f", GetZCaricPos(nozzle) - carRec.C_offprel, ((float)steppos / QHeader.Step_Trasl1 + QHeader.automaticPickHeightCorrection) );
			W_Mess( str );
			#endif

			carRec.C_offprel = GetZCaricPos(nozzle) - ((float)steppos / QHeader.Step_Trasl1 + QHeader.automaticPickHeightCorrection);
			CarFile->SaveRecX( nrec, carRec );
			break;
		}
	}

	CarFile->SaveFile();

	PuntaZSecurityPos( nozzle );
	PuntaZPosWait( nozzle );

	if( return_xyzero )
	{
		Set_Finec(ON);
		NozzleXYMove( 0, 0 );
		Wait_PuntaXY();
		Set_Finec(OFF);
	}

	return retVal;
}

//---------------------------------------------------------------------------
// Apprendimento sequenziale quota prelievo (misurazione del vuoto)
//---------------------------------------------------------------------------
void FeederZSeq_Auto()
{
	int open = 0;

	if( CarFile == NULL )
	{
		CarFile = new FeederFile(QHeader.Conf_Default);
		open = 1;
	}

	if( !CarFile->opened )
	{
		if( open )
		{
			delete CarFile;
			CarFile = NULL;
		}
		return;
	}

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	std::vector<int> used_feeders;
	int code = Search_car(0);

	while( code != 0 )
	{
		if( code > 1 )
		{
			if( IsValidCarCode(code) )
			{
				int already_found = 0;
				int nrec = GetCarRec(code);

				for( unsigned int i = 0 ; i < used_feeders.size(); i++ )
				{
					if( used_feeders[i] == nrec )
					{
						already_found = 1;
						break;
					}
				}

				if( !already_found )
				{
					used_feeders.push_back( nrec );
				}
			}
		}

		code = Search_car(1);
	}

	//EnableTimerConfirmRequiredBeforeNextXYMovement(true);

	if( used_feeders.size() != 0 )
	{
		SortData((void *)&used_feeders.at(0),SORTFIELDTYPE_INT32,used_feeders.size(),0,sizeof(used_feeders.at(0)),0);

		// esegue misurazioni
		char tool = 'E';

		if( ToolSelectionWin( 0, tool, 1 ) )
		{
			for( unsigned int i = 0 ; i < used_feeders.size() ; i++ )
			{
				if( Feeder_AppZOff_Vacuo( used_feeders[i], tool, ( i != used_feeders.size()-1 ) ? false : true ) != 1 )
				{
					break;
				}
			}
		}
	}
	else
	{
		//TODO - messaggio
		W_Mess( "No feeder to teach !" );
	}

	//EnableTimerConfirmRequiredBeforeNextXYMovement(false);

	if( open )
	{
		delete CarFile;
		CarFile = NULL;
	}
}


typedef struct
{
	int nrec;
	int uge;
} t_used_feeders;

//---------------------------------------------------------------------------------
// Apprendimento sequenziale (MANUAL) quota prelievo caricatori
//---------------------------------------------------------------------------------
void FeederZSeq_Man(void)
{
	int open = 0;

	if( CarFile == NULL )
	{
		CarFile = new FeederFile(QHeader.Conf_Default);
		open = 1;
	}

	if( !CarFile->opened )
	{
		if( open )
		{
			delete CarFile;
			CarFile = NULL;
		}
		return;
	}

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	std::vector<t_used_feeders> used_feeders;
	int code = Search_car(0);
	
	while( code != 0 )
	{
		if( code > 1 )
		{
			if( IsValidCarCode(code) )
			{
				int already_found = 0;
				int nrec = GetCarRec(code);

				for( unsigned int i = 0 ; i < used_feeders.size(); i++ )
				{
					if(used_feeders[i].nrec == nrec)
					{
						already_found = 1;
						break;
					}
				}

				if( !already_found )
				{
					t_used_feeders f;

					f.nrec = nrec;

					struct CarDat carRec;
					CarFile->ReadRec( f.nrec, carRec );

					f.uge = currentLibPackages[carRec.C_PackIndex-1].tools[0];

					used_feeders.push_back(f);
				}
			}
		}
	
		code = Search_car(1);
	}

	//EnableTimerConfirmRequiredBeforeNextXYMovement(true);
	
	if(used_feeders.size() != 0)
	{
		SortData((void *)&used_feeders.at(0),SORTFIELDTYPE_INT32,used_feeders.size(),((mem_pointer)&(used_feeders.at(0).uge))-((mem_pointer)&(used_feeders.at(0))),sizeof(used_feeders.at(0)),0);
	
		for( unsigned int i = 0 ; i< used_feeders.size() ; i++ )
		{
			char tool = used_feeders[i].uge;

			if( !Feeder_AppZOff( used_feeders[i].nrec, tool, ( i != used_feeders.size()-1 ) ? false : true ) )
			{
				break;
			}
		}
	}

	//EnableTimerConfirmRequiredBeforeNextXYMovement(false);

	if( open )
	{
		delete CarFile;
		CarFile = NULL;
	}
}


//---------------------------------------------------------------------------
// * Gestione tabella caricatori >> S310801

int TCaric_UpdateDB( int isDel )
{
	if((!(QHeader.modal & ENABLE_CARINT)) || errDB)
	{
		return(1);
	}
	
	if((!isDel) && (KK_Caric[carCurRow].C_comp[0]=='\0'))
	{
		return(1);
	}
	
	int mag=(KK_Caric[carCurRow].C_codice/10);
	
	if(mag>MAXMAG)
	{
		return(1);
	}
	
	int dbIdx=GetConfDBLink(mag);
	
	if(dbIdx==-1)
	{
		return(1);
	}
	
	int f=DBToMemList_Idx(dbIdx);
	
	if(f==-1)
	{
		return(1);
	}

	//DBLocal.Read(dbRec,dbIdx);
	
	int n=(KK_Caric[carCurRow].C_codice-mag*10)-1;
	
	if(isDel)
	{
		CarList[f].elem[n][0]='\0';
		CarList[f].pack[n][0]='\0';
		CarList[f].noteCar[n][0]='\0';
	
		CarList[f].packIdx[n]=-1;
		CarList[f].num[n]=0;
		CarList[f].tipoCar[n]=0;
		CarList[f].tavanz[n]=0;
	
		int all_empty=1;
	
		for(int i=0;i<8;i++)
			{
			if(CarList[f].elem[i][0]!='\0')
			{
				all_empty=0;
				break;
			}
		}
		
		if(all_empty)
		{
			//magazzino nullo: smonta dalla configurazione
			CarList[f].mounted=0;
		}
	}
	else
	{
		strcpy(CarList[f].elem[n],KK_Caric[carCurRow].C_comp);
		strcpy(CarList[f].pack[n],KK_Caric[carCurRow].C_Package);
		strncpyQ(CarList[f].noteCar[n],KK_Caric[carCurRow].C_note,20);
	
		CarList[f].packIdx[n]=KK_Caric[carCurRow].C_PackIndex;
		CarList[f].num[n]=KK_Caric[carCurRow].C_quant;
		CarList[f].tipoCar[n]=KK_Caric[carCurRow].C_tipo;
		CarList[f].tavanz[n]=KK_Caric[carCurRow].C_att;
	}

	#ifdef CARINT_CHECKSUM
	CarList[f].checksum=CarInt_CalcChecksum(CarList[f]);
	
	CarInt_SetData(CarList[f].mounted-1,CARINT_CHKSUMREC,CarList[f].checksum);
	#endif

	CarList[f].changed=CARINT_NONET_CHANGED;
	
	if(IsNetEnabled())
	{
		if(DBRemote.Write(CarList[f],CarList[f].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
		{
			CarList[f].changed=0;
		}
	}
	
	DBLocal.Write(CarList[f],CarList[f].idx,FLUSHON);
	
	return(1);
}

int CheckFeederUsed(struct CarDat caric)
{
  int mag=(KK_Caric[carCurRow].C_codice/10);

  if(mag>MAXMAG)
  {
    return(-1);
  }

  int dbIdx=GetConfDBLink(mag);

  if(dbIdx==-1)
  {
    return(-2);
  }  

  int f=DBToMemList_Idx(dbIdx);

  if(f==-1)
  {
    return(-3);
  }

  return(CarInt_GetUsedState(CarList[f]));
}


int TCaric_DBSelection(void)
{
	if(!(QHeader.modal & ENABLE_CARINT))
	{
		return 1;
	}

	CarInt(KK_Caric[carCurRow].C_codice/10);

	TCaric_ReadRecs(carCurRecord-carCurRow);
	feedersConfigRefresh = true;

	return 1;
}

int TCaric_RemoveMag(void)
{
	if(!(QHeader.modal & ENABLE_CARINT))
	{
		return(1);
	}
	
	char buf[80];
	
	int mag=KK_Caric[carCurRow].C_codice/10;
	
	snprintf( buf, sizeof(buf), MsgGetString(Msg_01860), mag );
	
	if(!W_Deci(0,buf))
	{
		return(1);
	}

	struct CarDat carRec;
	SFeederDefault cardef[MAXCAR];
	FeedersDefault_Read( cardef );

	for(int i=1;i<=8;i++)
	{
		memset(&carRec,(char)0,sizeof(carRec));
		carRec.C_codice=(mag*10)+i;
		
		carRec.C_xcar = cardef[((mag-1)*8)+i-1].x;
		carRec.C_ycar = cardef[((mag-1)*8)+i-1].y;
		
		carRec.C_nx = 1;
		carRec.C_ny = 1;
		
		CarFile->SaveX( (mag*10)+i, carRec );
	}
	CarFile->SaveFile();
	
	int dbIdx=GetConfDBLink(mag);
	
	if(dbIdx!=-1)
	{
		int f=DBToMemList_Idx(dbIdx);
	
		if(f==-1)
		{
			return(1);
		}

		ConfDBLink[CarList[f].mounted-1]=-1;

		CarList[f].mounted=0;
		CarInt_SetAsUnused(CarList[f]);

		CarList[f].changed=CARINT_NONET_CHANGED;
	
		if(IsNetEnabled())
		{
			if(DBRemote.Write(CarList[f],CarList[f].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
			{
				CarList[f].changed=0;
			}
		}
	
		DBLocal.Write(CarList[f],CarList[f].idx,FLUSHON);
	}

	TCaric_ReadRecs(carCurRecord-carCurRow);
	feedersConfigRefresh = true;

	return 1;
}


int TCaric_AppZOff_Auto()
{
	char tool = 'E';
	if( ToolSelectionWin( 0, tool, 1 ) )
	{
		Feeder_AppZOff_Vacuo( GetCarRec(KK_Caric[carCurRow].C_codice), tool, true );
	}

	TCaric_ReadRecs(carCurRecord-carCurRow);
	feedersConfigRefresh = true;

	return 1;
}

int TCaric_AppZOff_Man()
{
	Feeder_AppZOff( GetCarRec(KK_Caric[carCurRow].C_codice), 0, true );

	TCaric_ReadRecs(carCurRecord-carCurRow);
	feedersConfigRefresh = true;
	return 1;
}


int TCaric_ShowCompImage()
{
	if( !KK_Caric[carCurRow].C_checkPos )
	{
		bipbip();
		return 1;
	}
	
	ShowImgParams( CARCOMP_IMG, KK_Caric[carCurRow].C_codice );
	return 1;
}

int TCaric_autoapp()
{
	FeederPosTeaching();
	return 1;
}

int TCaric_AvCar()
{
	//GF_13_06_2011  -  notifica modo demo attivo
	if( QParam.DemoMode )
		W_Mess( MsgGetString(Msg_05165) );
	
	//SMOD301106 Porta la telecamera sul caricatore prima di avanzare
	Caricatore->SetData(KK_Caric[carCurRow]);
	Caricatore->GoPos( 0 );
	Wait_PuntaXY();
	
	av_caric(KK_Caric[carCurRow].C_codice);
	TCaric_ReadRecs(carCurRecord-carCurRow);
	feedersConfigRefresh = true;

	return 1;
}

//DOMES
#ifdef __DOME_FEEDER
int get_domes_feeder_ncycles(int up_dir)
{
	const char* title;

	if( up_dir )
	{
		title = MsgGetString(Msg_05163);
	}
	else
	{
		title = MsgGetString(Msg_05164);
	}

	CInputBox inbox( 0, 8, title, MsgGetString(Msg_05162), NETID_TXT_LEN-1, CELL_TYPE_UINT );
	inbox.SetText( "1" );
	inbox.SetVMinMax( 1, 99 );

	inbox.Show();

	if( inbox.GetExitCode() != WIN_EXITCODE_ESC )
	{
		return inbox.GetInt();
	}

	return 0;
}

int TCaric_dome_up(void)
{
	int ncycles = get_domes_feeder_ncycles(1);
	
	if(ncycles)
	{
		FeederClass car = FeederClass(CarFile);
		car.SetCode(KK_Caric[carCurRow].C_codice);
		
		flushKeyboardBuffer();
		
		for(int i = 0; i < ncycles; i++)
		{
			if( Handle( false ) == K_ESC )
			{
				break;
			}
			
			car.DomesForcedUp();
			car.WaitReady();
		}
	}
	return 1;
}

int TCaric_dome_down(void)
{
	int ncycles = get_domes_feeder_ncycles(0);
	
	if(ncycles)
	{
		FeederClass car = FeederClass(CarFile);
		car.SetCode(KK_Caric[carCurRow].C_codice);
		
		flushKeyboardBuffer();

		for(int i = 0; i < ncycles; i++)
		{
			if( Handle( false ) == K_ESC )
			{
				break;
			}
		
			car.DomesForcedDown();
			car.WaitReady();
		}
	}
	return 1;
}
#endif

void TCaric_ReadRecs(int roffset)
{
	int floop = 0;   // counter - record on file
	while (floop<=MAXRCAR-1)
	{
		if(!CarFile->ReadRec(roffset+floop,KK_Caric[floop]))
		{
			break;
		}
		floop++;
	}
}


// Scrolla i valori dell'array di structs di una riga in Direction.
// 0 up / 1 down
void dat_scroll(int Direction)
{
    int Indent = 0;
    if(Direction) Indent = 1;
    memmove(&KK_Caric[Indent],&KK_Caric[!Indent],sizeof(KK_Caric)-sizeof(KK_Caric[0]));
}

void carIncRow(void)
{
  carCurRow++;
  carCurRecord++;
  if(carCurRecord==MAXCAR)
  { carCurRecord=MAXCAR-1;
    carCurRow--;
    bipbip();
    return;
  }
  if(carCurRow==MAXRCAR)
  {
    carCurRow=MAXRCAR-1;
  	 dat_scroll(0);
  	 CarFile->ReadRec(carCurRecord,KK_Caric[MAXRCAR-1]);
  }

  feedersConfigRefresh = true;
}

void carDecRow(void)
{
  carCurRecord--;
  carCurRow--;
  if(carCurRecord<0)
  { carCurRecord=0;
    carCurRow++;
    bipbip();
    return;
  }
  if(carCurRow<0)
  { carCurRow=0;

    dat_scroll(1);
    CarFile->ReadRec(carCurRecord,KK_Caric[0]);
  }

  feedersConfigRefresh = true;
}

void TCaric_PgDN()
{
  if(((carCurRecord+MAXRCAR)>=MAXCAR)||((carCurRecord+2*(MAXRCAR))>=MAXCAR))
  {
    carCurRecord = MAXCAR-1;

    carCurRow=MAXRCAR-1;
  }
  else
  {
    carCurRecord+=MAXRCAR;
  }

  TCaric_ReadRecs(carCurRecord-carCurRow);
  memcpy(&carCurData,KK_Caric+carCurRow,sizeof(struct CarDat));

  feedersConfigRefresh = true;
}

void TCaric_PgUP()
{
  if(carCurRecord-2*(MAXRCAR)<0)
  {
	  carCurRecord=0;
	  carCurRow=0;
  }
  else
    carCurRecord-=MAXRCAR;

  TCaric_ReadRecs(carCurRecord-carCurRow);
  memcpy(&carCurData,KK_Caric+carCurRow,sizeof(struct CarDat));

  feedersConfigRefresh = true;
}

void TCaric_CtrlPgUP()
{
	carCurRow=0;
	carCurRecord=0;
	TCaric_ReadRecs(0);
	feedersConfigRefresh = true;
}

void TCaric_CtrlPgDN()
{
	carCurRow=MAXRCAR-1;
	carCurRecord=MAXCAR-1;
	TCaric_ReadRecs(carCurRecord-(MAXRCAR-1));
	feedersConfigRefresh = true;
}


void TCaric_InfoShow(void)
{
	int mode = 0;

	if( carCurData.C_codice >= 160 )
	{
		mode = 2;
	}
	else if( carCurData.C_tipo == CARTYPE_THFEEDER )
	{
		mode = 3;
	}
	else if( carCurData.C_att != 3 )
	{
		mode = 0;
	}
	else
	{
		mode = 1;
	}

	if( pWinFeederData )
	{
		pWinFeederData->Hide();
		delete pWinFeederData;
		pWinFeederData = 0;
	}

	if( mode == 0 )
	{
		pWinFeederData = new FeedersInfoNormalUI( 0 );
	}
	else if( mode == 1 )
	{
		pWinFeederData = new FeedersInfoDoubleUI( 0 );
	}
	else if( mode == 2 )
	{
		pWinFeederData = new FeedersInfoTrayUI( 0 );
	}
	else if( mode == 3 )
	{
		pWinFeederData = new FeedersInfoTHUI( 0 );
	}

	if( pWinFeederData )
	{
		pWinFeederData->Show( false, false );
	}
}


int G_TCaric( int carstart, char* tc, char* pk )
{
	if((QHeader.modal & ENABLE_CARINT) && (!IsConfDBLinkOk()))
	{
		if(UpdateDBData(CARINT_UPDATE_FULL))
		{
			ConfImport(0);
		}
	}
	
	int retval=-1;

	carCurRecord=GetCarRec(carstart);
	if(carCurRecord<0)
	{
		carCurRecord=0;
	}
	carCurRow=0;

	ShowCurrentData();

	//init classe file caricatori
	CarFile=new FeederFile(QHeader.Conf_Default);
	//init classe caricatore selezionato
	Caricatore=new FeederClass(CarFile);
	
	if(CarFile->opened)
		car_open=1;
	else
		car_open=0;

	if(car_open)
	{
	  carstart=GetCarRec(carstart);
		if(carstart<0)
			carstart=0;
	
		if((carstart+MAXRCAR-1)>=MAXCAR)
		{
			carstart=MAXCAR-MAXRCAR;
			carCurRow=MAXRCAR-1-(MAXCAR-1-carCurRecord);
		}

		TCaric_ReadRecs(carstart); //>>CAR
	
		memcpy(&carCurData,KK_Caric,sizeof(struct CarDat));


		FeedersConfigUI win(0);
		win.Show();
		win.Hide();

	
		delete Caricatore;
		delete CarFile;
		CarFile=NULL;
	}

	CInfo_EndAll();

	return retval;
}


//---------------------------------------------------------------------------

void CInfo_EndAll()
{
	if( pWinFeederData )
	{
		pWinFeederData->Hide();
		delete pWinFeederData;
		pWinFeederData = 0;
	}
}

int CInfo_AppZOff_Auto()
{
	char tool = 'E';
	if( ToolSelectionWin( 0, tool, 1 ) )
	{
		Feeder_AppZOff_Vacuo( GetCarRec(KK_Caric[carCurRow].C_codice), tool, true );
	}

	TCaric_ReadRecs(carCurRecord-carCurRow);
	return 1;
}

int CInfo_AppZOff_Man()
{
	Feeder_AppZOff( GetCarRec(KK_Caric[carCurRow].C_codice), 0, true );

	TCaric_ReadRecs(carCurRecord-carCurRow);
	return 1;
}

int CInfo_Av(void)
{
	//GF_13_06_2011  -  notifica modo demo attivo
	if( QParam.DemoMode )
		W_Mess( MsgGetString(Msg_05165) );
	
	// avanzamento caricatore selez.
	av_caric(KK_Caric[carCurRow].C_codice);
	return 1;
}


//---------------------------------------------------------------------------
// finestra: Feeders database
//---------------------------------------------------------------------------
FeedersConfigUI::FeedersConfigUI( CWindow* parent ) : CWindowTable( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_EDIT );
	SetClientAreaPos( 0, 18 );
	SetClientAreaSize( 87, 11 );
	SetTitle( MsgGetString(Msg_00350) );

	SM_PickH = new GUI_SubMenu();
	SM_PickH->Add( MsgGetString(Msg_00731), K_F1, 0, NULL, TCaric_AppZOff_Auto ); // Automatico
	SM_PickH->Add( MsgGetString(Msg_00730), K_F2, 0, NULL, TCaric_AppZOff_Man );  // Manuale

	SM_ExportTo = new GUI_SubMenu();
	SM_ExportTo->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &FeedersConfigUI::csvToSharedFolder, this ) );
	SM_ExportTo->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &FeedersConfigUI::csvToUSBDevice, this ) );
}

FeedersConfigUI::~FeedersConfigUI()
{
	delete SM_PickH;
}

void FeedersConfigUI::onInit()
{
	// create table
	m_table = new CTable( 3, 2, MAXRCAR, TABLE_STYLE_HIGHLIGHT_ROW, this );

	// add columns
	m_table->AddCol( MsgGetString(Msg_00490),  3, CELL_TYPE_UINT, CELL_STYLE_READONLY ); //num. caricatore
	m_table->AddCol( MsgGetString(Msg_00510), 25, CELL_TYPE_TEXT ); //tipo componente
	m_table->AddCol( MsgGetString(Msg_00678), 21, CELL_TYPE_TEXT, CELL_STYLE_READONLY ); //package
	m_table->AddCol( MsgGetString(Msg_01034), 24, CELL_TYPE_TEXT ); //note
	m_table->AddCol( MsgGetString(Msg_01622),  2, CELL_TYPE_TEXT ); //tipo avanzamento
	m_table->AddCol( MsgGetString(Msg_01623),  1, CELL_TYPE_TEXT ); //tipo caricatore

	// set params
	m_table->SetColLegalStrings( 4, 8, (char**)CarAvType_StrVect );
	#ifndef __DOME_FEEDER
	m_table->SetColLegalStrings( 5, 3, (char**)CarType_StrVect );
	#else
	m_table->SetColLegalStrings( 5, 4, (char**)CarType_StrVect );
	#endif
}

void FeedersConfigUI::onShow()
{
	feedersConfigRefresh = true;
}

void FeedersConfigUI::onRefresh()
{
	if( !feedersConfigRefresh )
	{
		return;
	}

	GUI_Freeze_Locker lock;

	feedersConfigRefresh = false;

	for( int i = 0; i < MAXRCAR; i++ )
	{
		m_table->SetText( i, 0, KK_Caric[i].C_codice );
		m_table->SetText( i, 1, KK_Caric[i].C_comp );
		m_table->SetText( i, 2, KK_Caric[i].C_Package );
		m_table->SetText( i, 3, KK_Caric[i].C_note );

		#ifdef __DOME_FEEDER
		if( KK_Caric[i].C_tipo == CARTYPE_DOME )
		{
			m_table->SetText( i, 4, "-" );
		}
		else
		#endif
		{
			if( KK_Caric[i].C_att < 8 ) //Laser test
			{
				m_table->SetStrings_Pos( i, 4, KK_Caric[i].C_att );
			}
			else
			{
				m_table->SetStrings_Pos( i, 4, 0 );
			}
		}

		m_table->SetStrings_Pos( i, 5, KK_Caric[i].C_tipo );
	}

	m_table->Select( carCurRow, m_table->GetCurCol() );

	memcpy( &carCurData, KK_Caric+carCurRow, sizeof(struct CarDat) );

	//mostra finestra info
	TCaric_InfoShow();
}

void FeedersConfigUI::onEdit()
{
	if( CheckFeederUsed(KK_Caric[carCurRow]) == MAG_USEDBY_OTHERS )
	{
		W_Mess( MsgGetString(Msg_02010) ); // feeder already used
		return;
	}

	char str[32];
	int curCol = m_table->GetCurCol();

	if( curCol == 1 )
	{
		snprintf( str, 32, "%s", m_table->GetText( carCurRow, curCol ) );
		DelSpcR( str );

		struct CarDat tmpCar;
		if( CarFile->Search( str, tmpCar, 0 ) == CAR_SEARCHFAIL )
		{
			snprintf( KK_Caric[carCurRow].C_comp, 26, "%s", str );
		}
	}
	else if( curCol == 3 )
	{
		snprintf( KK_Caric[carCurRow].C_note, 25, "%s", m_table->GetText( carCurRow, curCol ) );
	}
	else if( curCol == 4 )
	{
		KK_Caric[carCurRow].C_att = m_table->GetStrings_Pos( carCurRow, curCol );
	}
	else if( curCol == 5 )
	{
		KK_Caric[carCurRow].C_tipo = m_table->GetStrings_Pos( carCurRow, curCol );
	}


	TCaric_UpdateDB();
	CarFile->SaveRecX(carCurRecord,KK_Caric[carCurRow]);
	CarFile->SaveFile();

	feedersConfigRefresh = true;
}

void FeedersConfigUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_00888), K_F1, 0, NULL, boost::bind( &FeedersConfigUI::onFeederData, this ) ); // Feeder data
	m_menu->Add( MsgGetString(Msg_00247), K_F2, 0, NULL, boost::bind( &FeedersConfigUI::onEditOverride, this ) ); // edit
	m_menu->Add( MsgGetString(Msg_01424), K_F3, 0, SM_PickH, NULL ); // Picking height teaching
	m_menu->Add( MsgGetString(Msg_00026), K_F4, 0, NULL, TCaric_autoapp ); // Picking position teaching

	if( carCurData.C_codice < FIRSTTRAY )
	{
		m_menu->Add( MsgGetString(Msg_01624), K_F5, (QHeader.modal & ENABLE_CARINT) ? 0 : SM_GRAYED, NULL, TCaric_DBSelection ); // Select from db
		m_menu->Add( MsgGetString(Msg_01861), K_F6, (QHeader.modal & ENABLE_CARINT) ? 0 : SM_GRAYED, NULL, TCaric_RemoveMag ); // Remove
		m_menu->Add( MsgGetString(Msg_00744), K_F8, (KK_Caric[carCurRow].C_checkPos) ? 0 : SM_GRAYED, NULL, TCaric_ShowCompImage ); // Image parameters
		m_menu->Add( MsgGetString(Msg_01142), K_F11, 0, NULL, boost::bind( &FeedersConfigUI::onPackageTable, this ) ); // Packages library
		m_menu->Add( MsgGetString(Msg_00009), K_F12, 0, SM_ExportTo, NULL );
		m_menu->Add( MsgGetString(Msg_00440), K_SHIFT_F4, 0, NULL, TCaric_AvCar ); // Component feeding

		#ifdef __DOME_FEEDER
		if( KK_Caric[carCurRow].C_tipo == CARTYPE_DOME )
		{
			m_menu->Add( MsgGetString(Msg_05160), K_SHIFT_F5, 0, NULL, TCaric_dome_up );
			m_menu->Add( MsgGetString(Msg_05161), K_SHIFT_F6, 0, NULL, TCaric_dome_down );
		}
		#endif
	}
	else
	{
		m_menu->Add( MsgGetString(Msg_00325), K_F7, 0, NULL, auto_incr, boost::bind( &FeedersConfigUI::onMenuExit, this ) ); // Tray pitch teaching
		m_menu->Add( MsgGetString(Msg_01142), K_F11, 0, NULL, boost::bind( &FeedersConfigUI::onPackageTable, this ) ); // Packages library
		m_menu->Add( MsgGetString(Msg_00009), K_F12, 0, SM_ExportTo, NULL );
	}
}

bool FeedersConfigUI::onKeyPress( int key )
{
	switch( key )
	{
		case K_F1:
			return onFeederData();

		case K_F2:
			return onEditOverride();

		case K_F3:
			SM_PickH->Show();
			return true;

		case K_F4:
			TCaric_autoapp();
			return true;

		case K_F5:
			if( carCurData.C_codice < FIRSTTRAY )
			{
				TCaric_DBSelection();
				return true;
			}
			break;

		case K_F6:
			if( carCurData.C_codice < FIRSTTRAY )
			{
				TCaric_RemoveMag();
				return true;
			}
			break;

		case K_F7:
			if( carCurData.C_codice >= FIRSTTRAY )
			{
				auto_incr();

				feedersConfigRefresh = true;
				onRefresh();

				return true;
			}
			break;

		case K_F8:
			if( carCurData.C_codice < FIRSTTRAY )
			{
				TCaric_ShowCompImage();
				return true;
			}
			break;

		case K_F11:
			return onPackageTable();

		case K_F12:
			SM_ExportTo->Show();
			return true;

		case K_SHIFT_F4:
			if( carCurData.C_codice < FIRSTTRAY )
			{
				TCaric_AvCar();
				return true;
			}
			break;

		#ifdef __DOME_FEEDER
		case K_SHIFT_F5:
			if( carCurData.C_codice < FIRSTTRAY && KK_Caric[carCurRow].C_tipo == CARTYPE_DOME )
			{
				TCaric_dome_up();
				return true;
			}
			break;

		case K_SHIFT_F6:
			if( carCurData.C_codice < FIRSTTRAY && KK_Caric[carCurRow].C_tipo == CARTYPE_DOME )
			{
				TCaric_dome_down();
				return true;
			}
			break;
		#endif

		case K_DEL:
			if( guiDeskTop->GetEditMode() )
			{
				if(CheckFeederUsed(KK_Caric[carCurRow])==MAG_USEDBY_OTHERS)
				{
					W_Mess(ERRMAG_USEDBY_OTHERS);
					break;
				}

				if( W_Deci(0,MsgGetString(Msg_00142)) )
				{
					SFeederDefault cardef[MAXCAR];
					FeedersDefault_Read( cardef );

					int code = GetCarCode(carCurRecord);
					CarFile->SetDefValues( code, cardef[carCurRecord].x, cardef[carCurRecord].y );
					CarFile->SaveFile();

					TCaric_UpdateDB(1);

					TCaric_ReadRecs(carCurRecord-carCurRow);
					feedersConfigRefresh = true;
				}
				return true;
			}
			break;

		case K_DOWN:
		case K_UP:
		case K_PAGEDOWN:
		case K_PAGEUP:
		case K_CTRL_PAGEUP:
		case K_CTRL_PAGEDOWN:
			return vSelect( key );

			//TODO
			/*
		case CTRLC:
			cTab->GestKey(c);
			if(cTab->GetCurCol()==3)
			{
				package_in_clipboard = KK_Caric[carCurRow].C_PackIndex;
			}
			break;

		case CTRLV:
			if(cTab->GetCurCol()==3)
			{
				if(package_in_clipboard)
				{
					KK_Caric[carCurRow].C_PackIndex = package_in_clipboard;
					strcpy(KK_Caric[carCurRow].C_Package,get_cliptext());

					CarFile->SaveRecX(carCurRecord,KK_Caric[carCurRow]);
					CarFile->SaveFile();
					TCaric_UpdateDB();

					feedersConfigRefresh = true;
				}
				else
				{
					bipbip();
				}
			}
			else
			{
				cTab->GestKey(c);
			}
			break;
			*/

		default:
			break;
	}

	return false;
}

void FeedersConfigUI::onClose()
{
}

int FeedersConfigUI::onEditOverride()
{
	guiDeskTop->SetEditMode( true );
	m_table->SetEdit( true );

	feedersConfigRefresh = true;
	return 1;
}

int FeedersConfigUI::onFeederData()
{
	if( pWinFeederData )
	{
		GUI_Freeze();
		Deselect();
		m_table->SetRowStyle( m_table->GetCurRow(), TABLEROW_STYLE_SELECTED_GREEN );
		m_table->Deselect();
		pWinFeederData->SelectFirstCell();
		GUI_Thaw();

		pWinFeederData->SetFocus();

		GUI_Freeze();
		pWinFeederData->DeselectCells();
		m_table->SetRowStyle( m_table->GetCurRow(), TABLEROW_STYLE_DEFAULT );
		m_table->Select( m_table->GetCurRow(), m_table->GetCurCol() );
		Select();

		if( !(GetStyle() & WIN_STYLE_NO_MENU) && !(GetStyle() & WIN_STYLE_NO_EDIT) )
		{
			m_table->SetEdit( guiDeskTop->GetEditMode() );
		}
		GUI_Thaw();
	}
	return 1;
}

int FeedersConfigUI::onPackageTable()
{
	int pack_index;
	std::string packName;

	if( fn_PackagesTableSelect( this, KK_Caric[carCurRow].C_Package, pack_index, packName ) )
	{
		if(CheckFeederUsed(KK_Caric[carCurRow])==MAG_USEDBY_OTHERS)
		{
			bipbip();
			W_Mess(ERRMAG_USEDBY_OTHERS);
		}
		else
		{
			KK_Caric[carCurRow].C_PackIndex=pack_index;
			strcpy(KK_Caric[carCurRow].C_Package,packName.c_str());
			CarFile->SaveRecX(carCurRecord,KK_Caric[carCurRow]);
			CarFile->SaveFile();
			TCaric_UpdateDB();
		}
	}

	TCaric_ReadRecs(carCurRecord-carCurRow);

	feedersConfigRefresh = true;
	return 1;
}

int FeedersConfigUI::csvToSharedFolder()
{
	char newName[9];

	CInputBox inbox( this, 8, MsgGetString(Msg_00009), MsgGetString(Msg_00170), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );


	// check file
	char csvFile[MAXNPATH];
	snprintf( csvFile, MAXNPATH, "%s/%s%s", SHAREDIR, newName, CSV_EXT );

	if( !CheckDir(SHAREDIR,CHECKDIR_CREATE) )
	{
		W_Mess( MsgGetString(Msg_01841) );
		return 0;
	}

	if( CheckFile( csvFile ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00086) ) )
		{
			return 0;
		}
	}

	// export file
	if( !FeederConfig_ExportCSV( QHeader.Conf_Default, csvFile ) )
	{
		W_Mess( MsgGetString(Msg_00067) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00222) );
	return 1;
}

extern std::string fn_Select( CWindow* parent, const std::string& title, const std::vector<std::string>& items );

int FeedersConfigUI::csvToUSBDevice()
{
	// choose device
	std::vector<std::string> usb_mountpoints;
	if( !getAllUsbMountPoints( usb_mountpoints ) )
	{
		W_Mess( MsgGetString(Msg_01757) );
		return 0;
	}

	std::string selectedDevice = fn_Select( this, MsgGetString(Msg_02105), usb_mountpoints );
	if( selectedDevice.empty() )
	{
		return 0;
	}

	char newName[9];

	CInputBox inbox( this, 8, MsgGetString(Msg_00009), MsgGetString(Msg_00170), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	// check file
	char csvFile[MAXNPATH];
	snprintf( csvFile, MAXNPATH, "%s/%s%s", selectedDevice.c_str(), newName, CSV_EXT );

	if( CheckFile( csvFile ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00086) ) )
		{
			return 0;
		}
	}

	// export file
	if( !FeederConfig_ExportCSV( QHeader.Conf_Default, csvFile ) )
	{
		W_Mess( MsgGetString(Msg_00067) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00222) );
	return 1;
}

int FeedersConfigUI::onMenuExit()
{
	feedersConfigRefresh = true;
	onRefresh();

	return 1;
}

//--------------------------------------------------------------------------
// Sposta la selezione verticalmente
//--------------------------------------------------------------------------
bool FeedersConfigUI::vSelect( int key )
{
	if( m_table->GetCurRow() < 0 )
		return false;

	if( key == K_DOWN )
	{
		carIncRow();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_UP )
	{
		carDecRow();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_PAGEDOWN )
	{
		TCaric_PgDN();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_PAGEUP )
	{
		TCaric_PgUP();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_CTRL_PAGEDOWN )
	{
		TCaric_CtrlPgDN();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_CTRL_PAGEUP )
	{
		TCaric_CtrlPgUP();
		// aggiorno la tabella
		return true;
	}

	return false;
}
