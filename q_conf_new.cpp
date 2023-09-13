//---------------------------------------------------------------------------
//
// Name:        q_conf.cpp
// Author:      Gabriel Ferri
// Created:     15/09/2011
// Description: Quadra configuration parameters
//
//---------------------------------------------------------------------------
#include "q_conf_new.h"

#include "opencv2/imgproc/imgproc_c.h"

#include "c_win_par.h"
#include "c_inputbox.h"
#include "c_win_imgpar.h"
#include "c_win_table.h"
#include "gui_functions.h"
#include "gui_desktop.h"
#include "gui_defs.h"

#include "msglist.h"
#include "q_tabe.h"
#include "q_net.h"
#include "q_oper.h"
#include "q_help.h"
#include "q_carint.h"
#include "q_init.h"
#include "q_mapping.h"
#include "q_prog.h"
#include "q_conf.h"
#include "q_ser.h"
#include "q_ugeobj.h"
#include "q_fox.h"
#include "strutils.h"
#include "q_vision.h"
#include "q_files_new.h"
#include "q_feeders.h"
#include "q_dosat.h"
#include "keyutils.h"

#include <mss.h>


//TEMP //TODO: definite anche in q_ugeobj.cpp
#define DELTAUGECHECK_MIN   0.1
#define DELTAUGECHECK_MAX   4.0
#define DELTAUGECHECK_DEF   0.6


// limiti minimi coordinate ugelli
#define	XMINUGE        -9000.f
#define	YMINUGE        -9000.f


// limiti campo di lavoro
#define LXCAMPO_MAX     9900.f
#define LXCAMPO_MIN     -900.f
#define LYCAMPO_MAX     9900.f
#define LYCAMPO_MIN     -900.f

// limiti posizione fine scheda
#define ZXLIMIT_MAX      999.f
#define ZXLIMIT_MIN     -500.f
#define ZYLIMIT_MAX      999.f
#define ZYLIMIT_MIN     -500.f

// limiti passi in correzione rotazione teste
#define ADJROT_MINSTEP  -36
#define ADJROT_MAXSTEP   36

// limiti movimento punte in centratore esterno
#define EXTCAM_ZMIN     -10.f
#define EXTCAM_ZMAX      28.f

// limiti dimensioni parti ugello
#define UGEDIM_A_MIN    0.f
#define UGEDIM_A_MAX    15.f
#define UGEDIM_B_MIN    0.f
#define UGEDIM_B_MAX    15.f
#define UGEDIM_C_MIN    0.f
#define UGEDIM_C_MAX    15.f
#define UGEDIM_D_MIN    0.f
#define UGEDIM_D_MAX    15.f
#define UGEDIM_E_MIN    0.f
#define UGEDIM_E_MAX    15.f
#define UGEDIM_F_MIN    0.f
#define UGEDIM_F_MAX    15.f
#define UGEDIM_G_MIN    0.f
#define UGEDIM_G_MAX    15.f

// limiti dimensioni parti ugello
#define XTEL1_MIN       -9000.f
#define XTEL1_MAX       10000.f


extern GUI_DeskTop* guiDeskTop;
extern CfgParam QParam;
extern CfgHeader QHeader;
extern struct vis_data Vision;
extern struct CfgDispenser CC_Dispenser;

extern int mapTestDebug;
extern int mapTestLight;
extern int Test_TeachPosition( int image_type );



//---------------------------------------------------------------------------
// finestra: Feeders default position
//---------------------------------------------------------------------------
extern struct SFeederDefault CC_Caric[MAXCAR];
extern int car_rec;

extern int Caric_AutoApp();
extern int Caric_Avanz();

class FeedersDefPositionUI : public CWindowParams
{
public:
	FeedersDefPositionUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 60, 12 );
		SetTitle( MsgGetString(Msg_00239) );
	}

	typedef enum
	{
		FDR_CODE,
		FDR_POS_X,
		FDR_POS_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[FDR_CODE] =  new C_Combo( 15, 2, MsgGetString(Msg_00270), 3, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[FDR_POS_X] = new C_Combo( 15, 3, MsgGetString(Msg_00271), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[FDR_POS_Y] = new C_Combo( 15, 4, MsgGetString(Msg_00272), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		
		// set params
		m_combos[FDR_POS_X]->SetVMinMax( -999.99f, 999.99f );
		m_combos[FDR_POS_Y]->SetVMinMax( -999.99f, 999.99f );
		
		// add to combo list
		m_comboList->Add( m_combos[FDR_CODE],  0, 0 );
		m_comboList->Add( m_combos[FDR_POS_X], 1, 0 );
		m_comboList->Add( m_combos[FDR_POS_Y], 2, 0 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 7, GetW()/GUI_CharW() - 4, 4 ) );
		DrawTextCentered( 8, MsgGetString(Msg_00319), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
		DrawTextCentered( 9, MsgGetString(Msg_01478), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	}

	void onRefresh()
	{
		m_combos[FDR_CODE]->SetTxt( CC_Caric[car_rec].code );
		m_combos[FDR_POS_X]->SetTxt( CC_Caric[car_rec].x );
		m_combos[FDR_POS_Y]->SetTxt( CC_Caric[car_rec].y );
	}

	void onEdit()
	{
		CC_Caric[car_rec].x = m_combos[FDR_POS_X]->GetFloat();
		CC_Caric[car_rec].y = m_combos[FDR_POS_Y]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00026), K_F4, 0, NULL, Caric_AutoApp ); // Apprendimento posizione caricatore
		m_menu->Add( MsgGetString(Msg_00440), K_SHIFT_F4, 0, NULL, Caric_Avanz );  // Avanzamento caricatore
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F4:
				Caric_AutoApp();
				return true;
			
			case K_SHIFT_F4:
				Caric_Avanz();
				return true;
			
			case K_CTRL_PAGEUP:
				if( car_rec < MAXNREC_FEED )
					car_rec -= 7;
			case K_PAGEUP:
				if( car_rec > 0 )
					car_rec--;
				else
					car_rec = MAXCAR-1;
				return true;
			
			case K_CTRL_PAGEDOWN:
				if( car_rec < MAXNREC_FEED )
					car_rec += 7;
			case K_PAGEDOWN:
				if( car_rec < MAXCAR-1 )
					car_rec++;
				else
					car_rec = 0;
				return true;
			
			default:
				break;
		}
	
		return false;
	}
};


int fn_FeedersDefPosition()
{
	car_rec = 0;
	FeedersDefault_Read( CC_Caric );

	FeedersDefPositionUI win;
	win.Show();
	win.Hide();

	FeedersDefault_Write( CC_Caric );

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Speed and acceleration
//---------------------------------------------------------------------------
class SpeedAccelerationUI : public CWindowParams
{
public:
	SpeedAccelerationUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 58, 16 );
		SetTitle( MsgGetString(Msg_07023) );
	}

	typedef enum
	{
		AXES_SPEED_FACTOR,
		NOZZLE_START_STOP_1,
		NOZZLE_MAX_SPEED_1,
		NOZZLE_MAX_ACC_1,
		NOZZLE_ROT_SPEED_1,
		NOZZLE_ROT_ACC_1,
		NOZZLE_START_STOP_2,
		NOZZLE_MAX_SPEED_2,
		NOZZLE_MAX_ACC_2,
		NOZZLE_ROT_SPEED_2,
		NOZZLE_ROT_ACC_2
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[AXES_SPEED_FACTOR]   = new C_Combo( 4, 2, MsgGetString(Msg_00240), 7, CELL_TYPE_UINT );

		m_combos[NOZZLE_START_STOP_1] = new C_Combo( 4, 9, MsgGetString(Msg_00993), 7, CELL_TYPE_UINT );
		m_combos[NOZZLE_MAX_SPEED_1]  = new C_Combo( 4, 10, MsgGetString(Msg_00241), 7, CELL_TYPE_UINT );
		m_combos[NOZZLE_MAX_ACC_1]    = new C_Combo( 4, 11, MsgGetString(Msg_00242), 7, CELL_TYPE_UINT );
		m_combos[NOZZLE_ROT_SPEED_1]  = new C_Combo( 4, 13, MsgGetString(Msg_00997), 7, CELL_TYPE_UINT );
		m_combos[NOZZLE_ROT_ACC_1]    = new C_Combo( 4, 14, MsgGetString(Msg_00998), 7, CELL_TYPE_UINT );
		
		m_combos[NOZZLE_START_STOP_2] = new C_Combo( 46, 9, "", 7, CELL_TYPE_UINT );
		m_combos[NOZZLE_MAX_SPEED_2]  = new C_Combo( 46, 10, "", 7, CELL_TYPE_UINT );
		m_combos[NOZZLE_MAX_ACC_2]    = new C_Combo( 46, 11, "", 7, CELL_TYPE_UINT );
		m_combos[NOZZLE_ROT_SPEED_2]  = new C_Combo( 46, 13, "", 7, CELL_TYPE_UINT );
		m_combos[NOZZLE_ROT_ACC_2]    = new C_Combo( 46, 14, "", 7, CELL_TYPE_UINT );
		
		// set params
		m_combos[AXES_SPEED_FACTOR]->SetVMinMax( 1, 100 );

		m_combos[NOZZLE_START_STOP_1]->SetVMinMax( ZVELMIN, 100 );
		m_combos[NOZZLE_MAX_SPEED_1]->SetVMinMax( ZVELMIN, QHeader.zMaxSpeed );
		m_combos[NOZZLE_MAX_ACC_1]->SetVMinMax( ZACCMIN, QHeader.zMaxAcc );
		m_combos[NOZZLE_ROT_SPEED_1]->SetVMinMax( RVMIN, QHeader.rotMaxSpeed );
		m_combos[NOZZLE_ROT_ACC_1]->SetVMinMax( RAMIN, QHeader.rotMaxAcc );
		
		m_combos[NOZZLE_START_STOP_2]->SetVMinMax( ZVELMIN, 100 );
		m_combos[NOZZLE_MAX_SPEED_2]->SetVMinMax( ZVELMIN, QHeader.zMaxSpeed );
		m_combos[NOZZLE_MAX_ACC_2]->SetVMinMax( ZACCMIN, QHeader.zMaxAcc );
		m_combos[NOZZLE_ROT_SPEED_2]->SetVMinMax( RVMIN, QHeader.rotMaxSpeed );
		m_combos[NOZZLE_ROT_ACC_2]->SetVMinMax( RAMIN, QHeader.rotMaxAcc );
		
		// add to combo list
		m_comboList->Add( m_combos[AXES_SPEED_FACTOR]  , 0, 0 );
		m_comboList->Add( m_combos[NOZZLE_START_STOP_1], 2, 0 );
		m_comboList->Add( m_combos[NOZZLE_MAX_SPEED_1] , 3, 0 );
		m_comboList->Add( m_combos[NOZZLE_MAX_ACC_1]   , 4, 0 );
		m_comboList->Add( m_combos[NOZZLE_ROT_SPEED_1] , 5, 0 );
		m_comboList->Add( m_combos[NOZZLE_ROT_ACC_1]   , 6, 0 );
		m_comboList->Add( m_combos[NOZZLE_START_STOP_2], 2, 1 );
		m_comboList->Add( m_combos[NOZZLE_MAX_SPEED_2] , 3, 1 );
		m_comboList->Add( m_combos[NOZZLE_MAX_ACC_2]   , 4, 1 );
		m_comboList->Add( m_combos[NOZZLE_ROT_SPEED_2] , 5, 1 );
		m_comboList->Add( m_combos[NOZZLE_ROT_ACC_2]   , 6, 1 );
	}

	void onShow()
	{
		DrawSubTitle( 0, MsgGetString(Msg_00025) ); // assi
		DrawSubTitle( 6, MsgGetString(Msg_00044) ); // punte
		
		DrawText( 36, 8, MsgGetString(Msg_00042) );
		DrawText( 47, 8, MsgGetString(Msg_00043) );
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_07024), K_F3, 0, NULL, fn_SpeedAccelerationTable ); // Tabelle velocita'
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				fn_SpeedAccelerationTable();
				return true;

			default:
				break;
		}

		return false;
	}

	void onRefresh()
	{
		m_combos[AXES_SPEED_FACTOR]->SetTxt( QParam.xySpeedFactor );
		//TODO messaggi
		char buf[80];
		snprintf( buf, 80, "Speed        : %d", int(QHeader.xyMaxSpeed * QParam.xySpeedFactor / 100.f) );
		DrawText( 10, 3, buf );
		snprintf( buf, 80, "Acceleration : %d", int(QHeader.xyMaxAcc * QParam.xySpeedFactor / 100.f) );
		DrawText( 10, 4, buf );

		m_combos[NOZZLE_START_STOP_1]->SetTxt( QParam.Trasl_VelMin1 );
		m_combos[NOZZLE_MAX_SPEED_1]->SetTxt( QParam.velp1 );
		m_combos[NOZZLE_MAX_ACC_1]->SetTxt( QParam.accp1 );
		m_combos[NOZZLE_ROT_SPEED_1]->SetTxt( int(QParam.prot1_vel) );
		m_combos[NOZZLE_ROT_ACC_1]->SetTxt( int(QParam.prot1_acc) );

		m_combos[NOZZLE_START_STOP_2]->SetTxt( QParam.Trasl_VelMin2 );
		m_combos[NOZZLE_MAX_SPEED_2]->SetTxt( QParam.velp2 );
		m_combos[NOZZLE_MAX_ACC_2]->SetTxt( QParam.accp2 );
		m_combos[NOZZLE_ROT_SPEED_2]->SetTxt( int(QParam.prot2_vel) );
		m_combos[NOZZLE_ROT_ACC_2]->SetTxt( int(QParam.prot2_acc) );
	}

	void onEdit()
	{
		QParam.xySpeedFactor = m_combos[AXES_SPEED_FACTOR]->GetInt();
		
		QParam.Trasl_VelMin1 = m_combos[NOZZLE_START_STOP_1]->GetInt();
		QParam.velp1 = m_combos[NOZZLE_MAX_SPEED_1]->GetInt();
		QParam.accp1 = m_combos[NOZZLE_MAX_ACC_1]->GetInt();
		QParam.prot1_vel = m_combos[NOZZLE_ROT_SPEED_1]->GetInt();
		QParam.prot1_acc = m_combos[NOZZLE_ROT_ACC_1]->GetInt();
		
		QParam.Trasl_VelMin2 = m_combos[NOZZLE_START_STOP_2]->GetInt();
		QParam.velp2 = m_combos[NOZZLE_MAX_SPEED_2]->GetInt();
		QParam.accp2 = m_combos[NOZZLE_MAX_ACC_2]->GetInt();
		QParam.prot2_vel = m_combos[NOZZLE_ROT_SPEED_2]->GetInt();
		QParam.prot2_acc = m_combos[NOZZLE_ROT_ACC_2]->GetInt();
	}

	void onClose()
	{
		Mod_Par( QParam );

		// Legge i parametri di funzionamento da disco fisso e setta
		// accelerazione e velocita' di start/stop della testa
		ForceSpeedsSetting();
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
		SetNozzleZSpeed_Index( 1, ACC_SPEED_DEFAULT );
		SetNozzleZSpeed_Index( 2, ACC_SPEED_DEFAULT );
		SetNozzleRotSpeed_Index( 1, ACC_SPEED_DEFAULT );
		SetNozzleRotSpeed_Index( 2, ACC_SPEED_DEFAULT );

		// Ricarica solo parametri azionamenti motori punte
		OpenRotation( ROT_RELOAD );
	}
};

int fn_SpeedAcceleration()
{
	SpeedAccelerationUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Speed and acceleration Tables
//---------------------------------------------------------------------------
class SpeedAccelerationTableUI : public CWindowParams
{
public:
	SpeedAccelerationTableUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 70, 16 );
		SetTitle( MsgGetString(Msg_07024) );
	}

	typedef enum
	{
		XY_MAX_SPEED_L,
		XY_MAX_ACC_L,
		ROT_MAX_SPEED_L,
		ROT_MAX_ACC_L,
		Z_START_STOP_L,
		Z_MAX_SPEED_L,
		Z_MAX_ACC_L,
		XY_MAX_SPEED_M,
		XY_MAX_ACC_M,
		ROT_MAX_SPEED_M,
		ROT_MAX_ACC_M,
		Z_START_STOP_M,
		Z_MAX_SPEED_M,
		Z_MAX_ACC_M,
		XY_MAX_SPEED_H,
		XY_MAX_ACC_H,
		ROT_MAX_SPEED_H,
		ROT_MAX_ACC_H,
		Z_START_STOP_H,
		Z_MAX_SPEED_H,
		Z_MAX_ACC_H
	} combo_labels;

private:
	SSpeedsTable data;

protected:
	void onInit()
	{
		SpeedsTable_Read( data );

		// create combos
		m_combos[XY_MAX_SPEED_L]  = new C_Combo( 4, 3, MsgGetString(Msg_00241), 7, CELL_TYPE_UINT );
		m_combos[XY_MAX_ACC_L]    = new C_Combo( 4, 4, MsgGetString(Msg_00242), 7, CELL_TYPE_UINT );
		m_combos[XY_MAX_SPEED_M]  = new C_Combo( 46, 3, "", 7, CELL_TYPE_UINT );
		m_combos[XY_MAX_ACC_M]    = new C_Combo( 46, 4, "", 7, CELL_TYPE_UINT );
		m_combos[XY_MAX_SPEED_H]  = new C_Combo( 57, 3, "", 7, CELL_TYPE_UINT );
		m_combos[XY_MAX_ACC_H]    = new C_Combo( 57, 4, "", 7, CELL_TYPE_UINT );
		m_combos[ROT_MAX_SPEED_L]  = new C_Combo( 4, 9, MsgGetString(Msg_00997), 7, CELL_TYPE_UINT );
		m_combos[ROT_MAX_ACC_L]    = new C_Combo( 4, 10, MsgGetString(Msg_00998), 7, CELL_TYPE_UINT );
		m_combos[ROT_MAX_SPEED_M]  = new C_Combo( 46, 9, "", 7, CELL_TYPE_UINT );
		m_combos[ROT_MAX_ACC_M]    = new C_Combo( 46, 10, "", 7, CELL_TYPE_UINT );
		m_combos[ROT_MAX_SPEED_H]  = new C_Combo( 57, 9, "", 7, CELL_TYPE_UINT );
		m_combos[ROT_MAX_ACC_H]    = new C_Combo( 57, 10, "", 7, CELL_TYPE_UINT );
		m_combos[Z_START_STOP_L]  = new C_Combo( 4, 12, MsgGetString(Msg_00993), 7, CELL_TYPE_UINT );
		m_combos[Z_MAX_SPEED_L]  = new C_Combo( 4, 13, MsgGetString(Msg_00241), 7, CELL_TYPE_UINT );
		m_combos[Z_MAX_ACC_L]    = new C_Combo( 4, 14, MsgGetString(Msg_00242), 7, CELL_TYPE_UINT );
		m_combos[Z_START_STOP_M]  = new C_Combo( 46, 12, "", 7, CELL_TYPE_UINT );
		m_combos[Z_MAX_SPEED_M]  = new C_Combo( 46, 13, "", 7, CELL_TYPE_UINT );
		m_combos[Z_MAX_ACC_M]    = new C_Combo( 46, 14, "", 7, CELL_TYPE_UINT );
		m_combos[Z_START_STOP_H]  = new C_Combo( 57, 12, "", 7, CELL_TYPE_UINT );
		m_combos[Z_MAX_SPEED_H]  = new C_Combo( 57, 13, "", 7, CELL_TYPE_UINT );
		m_combos[Z_MAX_ACC_H]    = new C_Combo( 57, 14, "", 7, CELL_TYPE_UINT );

		// set params
		m_combos[XY_MAX_SPEED_L]->SetVMinMax( XYVMIN, QHeader.xyMaxSpeed );
		m_combos[XY_MAX_ACC_L]->SetVMinMax( XYAMIN, QHeader.xyMaxAcc );
		m_combos[XY_MAX_SPEED_M]->SetVMinMax( XYVMIN, QHeader.xyMaxSpeed );
		m_combos[XY_MAX_ACC_M]->SetVMinMax( XYAMIN, QHeader.xyMaxAcc );
		m_combos[XY_MAX_SPEED_H]->SetVMinMax( XYVMIN, QHeader.xyMaxSpeed );
		m_combos[XY_MAX_ACC_H]->SetVMinMax( XYAMIN, QHeader.xyMaxAcc );
		m_combos[ROT_MAX_SPEED_L]->SetVMinMax( RVMIN, QHeader.rotMaxSpeed );
		m_combos[ROT_MAX_ACC_L]->SetVMinMax( RAMIN, QHeader.rotMaxAcc );
		m_combos[ROT_MAX_SPEED_M]->SetVMinMax( RVMIN, QHeader.rotMaxSpeed );
		m_combos[ROT_MAX_ACC_M]->SetVMinMax( RAMIN, QHeader.rotMaxAcc );
		m_combos[ROT_MAX_SPEED_H]->SetVMinMax( RVMIN, QHeader.rotMaxSpeed );
		m_combos[ROT_MAX_ACC_H]->SetVMinMax( RAMIN, QHeader.rotMaxAcc );
		m_combos[Z_START_STOP_L]->SetVMinMax( ZVELMIN, 100 );
		m_combos[Z_MAX_SPEED_L]->SetVMinMax( ZVELMIN, QHeader.zMaxSpeed );
		m_combos[Z_MAX_ACC_L]->SetVMinMax( ZACCMIN, QHeader.zMaxAcc );
		m_combos[Z_START_STOP_M]->SetVMinMax( ZVELMIN, 100 );
		m_combos[Z_MAX_SPEED_M]->SetVMinMax( ZVELMIN, QHeader.zMaxSpeed );
		m_combos[Z_MAX_ACC_M]->SetVMinMax( ZACCMIN, QHeader.zMaxAcc );
		m_combos[Z_START_STOP_H]->SetVMinMax( ZVELMIN, 100 );
		m_combos[Z_MAX_SPEED_H]->SetVMinMax( ZVELMIN, QHeader.zMaxSpeed );
		m_combos[Z_MAX_ACC_H]->SetVMinMax( ZACCMIN, QHeader.zMaxAcc );

		// add to combo list
		m_comboList->Add( m_combos[XY_MAX_SPEED_L] , 1, 0 );
		m_comboList->Add( m_combos[XY_MAX_ACC_L]   , 2, 0 );
		m_comboList->Add( m_combos[XY_MAX_SPEED_M] , 1, 1 );
		m_comboList->Add( m_combos[XY_MAX_ACC_M]   , 2, 1 );
		m_comboList->Add( m_combos[XY_MAX_SPEED_H] , 1, 2 );
		m_comboList->Add( m_combos[XY_MAX_ACC_H]   , 2, 2 );
		m_comboList->Add( m_combos[ROT_MAX_SPEED_L] , 3, 0 );
		m_comboList->Add( m_combos[ROT_MAX_ACC_L]   , 4, 0 );
		m_comboList->Add( m_combos[ROT_MAX_SPEED_M] , 3, 1 );
		m_comboList->Add( m_combos[ROT_MAX_ACC_M]   , 4, 1 );
		m_comboList->Add( m_combos[ROT_MAX_SPEED_H] , 3, 2 );
		m_comboList->Add( m_combos[ROT_MAX_ACC_H]   , 4, 2 );
		m_comboList->Add( m_combos[Z_START_STOP_L], 5, 0 );
		m_comboList->Add( m_combos[Z_MAX_SPEED_L] , 6, 0 );
		m_comboList->Add( m_combos[Z_MAX_ACC_L]   , 7, 0 );
		m_comboList->Add( m_combos[Z_START_STOP_M], 5, 1 );
		m_comboList->Add( m_combos[Z_MAX_SPEED_M] , 6, 1 );
		m_comboList->Add( m_combos[Z_MAX_ACC_M]   , 7, 1 );
		m_comboList->Add( m_combos[Z_START_STOP_H], 5, 2 );
		m_comboList->Add( m_combos[Z_MAX_SPEED_H] , 6, 2 );
		m_comboList->Add( m_combos[Z_MAX_ACC_H]   , 7, 2 );
	}

	void onShow()
	{
		DrawSubTitle( 0, MsgGetString(Msg_00025) ); // assi
		DrawSubTitle( 6, MsgGetString(Msg_00044) ); // punte

		DrawText( 36, 2, MsgGetString(Msg_00062) );
		DrawText( 47, 2, MsgGetString(Msg_00063) );
		DrawText( 58, 2, MsgGetString(Msg_00064) );

		DrawText( 36, 8, MsgGetString(Msg_00062) );
		DrawText( 47, 8, MsgGetString(Msg_00063) );
		DrawText( 58, 8, MsgGetString(Msg_00064) );
	}

	void onRefresh()
	{
		m_combos[XY_MAX_SPEED_L]->SetTxt( int(data.entries[SPEED_XY_L].v) );
		m_combos[XY_MAX_ACC_L]->SetTxt( int(data.entries[SPEED_XY_L].a) );
		m_combos[XY_MAX_SPEED_M]->SetTxt( int(data.entries[SPEED_XY_M].v) );
		m_combos[XY_MAX_ACC_M]->SetTxt( int(data.entries[SPEED_XY_M].a) );
		m_combos[XY_MAX_SPEED_H]->SetTxt( int(data.entries[SPEED_XY_H].v) );
		m_combos[XY_MAX_ACC_H]->SetTxt( int(data.entries[SPEED_XY_H].a) );

		m_combos[ROT_MAX_SPEED_L]->SetTxt( int(data.entries[SPEED_R_L].v) );
		m_combos[ROT_MAX_ACC_L]->SetTxt( int(data.entries[SPEED_R_L].a) );
		m_combos[ROT_MAX_SPEED_M]->SetTxt( int(data.entries[SPEED_R_M].v) );
		m_combos[ROT_MAX_ACC_M]->SetTxt( int(data.entries[SPEED_R_M].a) );
		m_combos[ROT_MAX_SPEED_H]->SetTxt( int(data.entries[SPEED_R_H].v) );
		m_combos[ROT_MAX_ACC_H]->SetTxt( int(data.entries[SPEED_R_H].a) );

		m_combos[Z_START_STOP_L]->SetTxt( int(data.entries[SPEED_Z_L].s) );
		m_combos[Z_MAX_SPEED_L]->SetTxt( int(data.entries[SPEED_Z_L].v) );
		m_combos[Z_MAX_ACC_L]->SetTxt( int(data.entries[SPEED_Z_L].a) );
		m_combos[Z_START_STOP_M]->SetTxt( int(data.entries[SPEED_Z_M].s) );
		m_combos[Z_MAX_SPEED_M]->SetTxt( int(data.entries[SPEED_Z_M].v) );
		m_combos[Z_MAX_ACC_M]->SetTxt( int(data.entries[SPEED_Z_M].a) );
		m_combos[Z_START_STOP_H]->SetTxt( int(data.entries[SPEED_Z_H].s) );
		m_combos[Z_MAX_SPEED_H]->SetTxt( int(data.entries[SPEED_Z_H].v) );
		m_combos[Z_MAX_ACC_H]->SetTxt( int(data.entries[SPEED_Z_H].a) );
	}

	void onEdit()
	{
		data.entries[SPEED_XY_L].v = m_combos[XY_MAX_SPEED_L]->GetInt();
		data.entries[SPEED_XY_L].a = m_combos[XY_MAX_ACC_L]->GetInt();
		data.entries[SPEED_XY_M].v = m_combos[XY_MAX_SPEED_M]->GetInt();
		data.entries[SPEED_XY_M].a = m_combos[XY_MAX_ACC_M]->GetInt();
		data.entries[SPEED_XY_H].v = m_combos[XY_MAX_SPEED_H]->GetInt();
		data.entries[SPEED_XY_H].a = m_combos[XY_MAX_ACC_H]->GetInt();

		data.entries[SPEED_R_L].v = m_combos[ROT_MAX_SPEED_L]->GetInt();
		data.entries[SPEED_R_L].a = m_combos[ROT_MAX_ACC_L]->GetInt();
		data.entries[SPEED_R_M].v = m_combos[ROT_MAX_SPEED_M]->GetInt();
		data.entries[SPEED_R_M].a = m_combos[ROT_MAX_ACC_M]->GetInt();
		data.entries[SPEED_R_H].v = m_combos[ROT_MAX_SPEED_H]->GetInt();
		data.entries[SPEED_R_H].a = m_combos[ROT_MAX_ACC_H]->GetInt();

		data.entries[SPEED_Z_L].s = m_combos[Z_START_STOP_L]->GetInt();
		data.entries[SPEED_Z_L].v = m_combos[Z_MAX_SPEED_L]->GetInt();
		data.entries[SPEED_Z_L].a = m_combos[Z_MAX_ACC_L]->GetInt();
		data.entries[SPEED_Z_M].s = m_combos[Z_START_STOP_M]->GetInt();
		data.entries[SPEED_Z_M].v = m_combos[Z_MAX_SPEED_M]->GetInt();
		data.entries[SPEED_Z_M].a = m_combos[Z_MAX_ACC_M]->GetInt();
		data.entries[SPEED_Z_H].s = m_combos[Z_START_STOP_H]->GetInt();
		data.entries[SPEED_Z_H].v = m_combos[Z_MAX_SPEED_H]->GetInt();
		data.entries[SPEED_Z_H].a = m_combos[Z_MAX_ACC_H]->GetInt();
	}

	void onClose()
	{
		SpeedsTable_Write( data );
	}
};

int fn_SpeedAccelerationTable()
{
	SpeedAccelerationTableUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Tools database
//---------------------------------------------------------------------------
class ToolsDatabaseUI : public CWindowTable
{
public:
	ToolsDatabaseUI( CWindow* parent, int start_item ) : CWindowTable( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 9 );
		SetClientAreaSize( 77, 18 );
		SetTitle( MsgGetString(Msg_01531) );

		m_start_item = start_item;
		loadDatabase();
	}

	int GetExitCode()
	{
		return m_exitCode;
	}

	//---------------------------------------------------------------------------
	// Ritorna il nome del tipo ugello selezionato
	//---------------------------------------------------------------------------
	char* GetToolName()
	{
		int selectedRow = getSelectedRow();
		if( selectedRow == -1 )
			return 0;

		return m_items[selectedRow].name;
	}

	//---------------------------------------------------------------------------
	// Ritorna il codice del tipo ugello selezionato
	//---------------------------------------------------------------------------
	int GetToolCode()
	{
		int selectedRow = getSelectedRow();
		if( selectedRow == -1 )
			return -1;

		//return m_items[selectedRow].index;
		return selectedRow;
	}


protected:
	void onInit()
	{
		// create table
		m_table = new CTable( 3, 2, 10, TABLE_STYLE_DEFAULT, this );

		// add columns
		m_table->AddCol( MsgGetString(Msg_01532), 4, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
		m_table->AddCol( MsgGetString(Msg_01533), 24, CELL_TYPE_TEXT );
		m_table->AddCol( "A", 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT );
		m_table->AddCol( "B", 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT );
		m_table->AddCol( "C", 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT );
		m_table->AddCol( "D", 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT );
		m_table->AddCol( "E", 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT );
		m_table->AddCol( "F", 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT );
		m_table->AddCol( "G", 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT );

		// set params
		m_table->SetColMinMax( 2, UGEDIM_A_MIN, UGEDIM_A_MAX );
		m_table->SetColMinMax( 3, UGEDIM_B_MIN, UGEDIM_B_MAX );
		m_table->SetColMinMax( 4, UGEDIM_C_MIN, UGEDIM_C_MAX );
		m_table->SetColMinMax( 5, UGEDIM_D_MIN, UGEDIM_D_MAX );
		m_table->SetColMinMax( 6, UGEDIM_E_MIN, UGEDIM_E_MAX );
		m_table->SetColMinMax( 7, UGEDIM_F_MIN, UGEDIM_F_MAX );
		m_table->SetColMinMax( 8, UGEDIM_G_MIN, UGEDIM_G_MAX );
	}

	void onShow()
	{
		// serve per assicurare il refresh della tabella
		int start = m_start_item;
		m_start_item = -1;
		showItems( start );

		DrawPanel( RectI( 3, 14, GetW()/GUI_CharW() - 6, 3 ) );
		DrawTextCentered( 15, MsgGetString(Msg_00097), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	}

	void onRefresh()
	{
	}

	void onEdit()
	{
		for( unsigned int i = 0; i + m_start_item < MAXUGEDIM && i < m_table->GetRows(); i++ )
		{
			CfgUgeDim* row = &m_items[i + m_start_item];

			snprintf( row->name, sizeof(row->name), "%s", m_table->GetText( i, 1 ) );
			row->a = m_table->GetFloat( i, 2 );
			row->b = m_table->GetFloat( i, 3 );
			row->c = m_table->GetFloat( i, 4 );
			row->d = m_table->GetFloat( i, 5 );
			row->e = m_table->GetFloat( i, 6 );
			row->f = m_table->GetFloat( i, 7 );
			row->g = m_table->GetFloat( i, 8 );
		}
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_DOWN:
			case K_UP:
			case K_PAGEDOWN:
			case K_PAGEUP:
				return vSelect( key );

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
		//TODO: manca controllo scrittura ok
		UgeDimWriteAll( m_items );
	}

private:
	//---------------------------------------------------------------------------
	// Carica i dati degli ugelli
	//---------------------------------------------------------------------------
	void loadDatabase()
	{
		//TODO: manca controllo lettura ok
		UgeDimReadAll( m_items );
	}

	//---------------------------------------------------------------------------
	// Visualizza i dati in tabella a partire da start_item
	//---------------------------------------------------------------------------
	void showItems( int start_item )
	{
		start_item = MAX( 0, start_item );
		if( start_item == m_start_item || start_item >= MAXUGEDIM )
			return;

		unsigned int i = 0;
		while( i + start_item < MAXUGEDIM && i < m_table->GetRows() )
		{
			CfgUgeDim* row = &m_items[i + start_item];

			m_table->SetText( i, 0, int(i + start_item + 1) );
			m_table->SetText( i, 1, row->name );
			m_table->SetText( i, 2, row->a );
			m_table->SetText( i, 3, row->b );
			m_table->SetText( i, 4, row->c );
			m_table->SetText( i, 5, row->d );
			m_table->SetText( i, 6, row->e );
			m_table->SetText( i, 7, row->f );
			m_table->SetText( i, 8, row->g );
			++i;
		}
		while( i < m_table->GetRows() )
		{
			m_table->SetText( i, 0, "" );
			m_table->SetText( i, 1, "" );
			m_table->SetText( i, 2, "" );
			m_table->SetText( i, 3, "" );
			m_table->SetText( i, 4, "" );
			m_table->SetText( i, 5, "" );
			m_table->SetText( i, 6, "" );
			m_table->SetText( i, 7, "" );
			m_table->SetText( i, 8, "" );
			++i;
		}

		m_start_item = start_item;
	}

	//---------------------------------------------------------------------------
	// Restituisce il numero della riga corrente
	//---------------------------------------------------------------------------
	int getSelectedRow()
	{
		int selectedRow = m_start_item + m_table->GetCurRow();
		if( selectedRow < 0 || selectedRow >= MAXUGEDIM )
			return -1;
		return selectedRow;
	}

	//--------------------------------------------------------------------------
	// Sposta la selezione verticalmente
	//--------------------------------------------------------------------------
	bool vSelect( int key )
	{
		int curR = m_table->GetCurRow();
		if( curR < 0 )
			return false;

		if( key == K_DOWN )
		{
			if( curR + m_start_item >= MAXUGEDIM - 1 )
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
			if( curR + m_start_item >= MAXUGEDIM - 1 )
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

	int m_exitCode;
	CfgUgeDim m_items[MAXUGEDIM];
	unsigned int m_start_item;
};

int fn_ToolsDatabase( CWindow* parent, int start_item, std::string& name )
{
	if( !UgeDimOpen() )
	{
		return -1;
	}

	ToolsDatabaseUI win( parent, start_item );
	win.Show();
	win.Hide();

	UgeDimClose();

	if( win.GetExitCode() == WIN_EXITCODE_ENTER )
	{
		name = win.GetToolName();
		return win.GetToolCode();
	}

	return -1;
}



//---------------------------------------------------------------------------
// finestra: Tools parameters
//---------------------------------------------------------------------------
extern UgelliClass* Ugelli;
extern unsigned char u_curnoz;
extern FoxCommClass* FoxPort;
extern CfgUgelli DatiUge;
extern int uge_rec;

extern int Uge_AutoAppSeqP1();
extern int Uge_AutoAppSeqP2();
extern int Uge_AutoAppP1();
extern int Uge_AutoAppP2();
extern int Uge_ZAppP1();
extern int Uge_ZAppP2();
extern int G_UgeCamSearch();
extern int Uge_CamAppRef();
extern int Uge_CamImgPar();


class ToolsParamsUI : public CWindowParams
{
public:
	ToolsParamsUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 58, 18 );
		SetTitle( MsgGetString(Msg_00417) );


		SM_SeqTeach = new GUI_SubMenu();
		SM_SeqTeach->Add( MsgGetString(Msg_00042), K_F1, 0, NULL, Uge_AutoAppSeqP1 ); // punta 1
		SM_SeqTeach->Add( MsgGetString(Msg_00043), K_F2, 0, NULL, Uge_AutoAppSeqP2 ); // punta 2

		SM_Teach = new GUI_SubMenu();
		SM_Teach->Add( MsgGetString(Msg_00042), K_F1, 0, NULL, Uge_AutoAppP1 ); // punta 1
		SM_Teach->Add( MsgGetString(Msg_00043), K_F2, 0, NULL, Uge_AutoAppP2 ); // punta 2

		SM_TeachZ = new GUI_SubMenu();
		SM_TeachZ->Add( MsgGetString(Msg_00042), K_F1, 0, NULL, Uge_ZAppP1 ); // punta 1
		SM_TeachZ->Add( MsgGetString(Msg_00043), K_F2, 0, NULL, Uge_ZAppP2 ); // punta 2

		SM_FidTeach = new GUI_SubMenu();
		SM_FidTeach->Add( MsgGetString(Msg_01708), K_F1, 0, NULL, G_UgeCamSearch ); // Teaching
		SM_FidTeach->Add( MsgGetString(Msg_01712), K_F2, 0, NULL, Uge_CamAppRef );  // Teach reference
		SM_FidTeach->Add( MsgGetString(Msg_00744), K_F3, 0, NULL, Uge_CamImgPar );  // Image parameters
	}

	~ToolsParamsUI()
	{
		delete SM_SeqTeach;
		delete SM_Teach;
		delete SM_TeachZ;
		delete SM_FidTeach;
	}

	typedef enum
	{
		TOOL_CODE,
		TOOL_TYPE,
		TOOL_NOZZ,
		TOOL_P1_X,
		TOOL_P1_Y,
		TOOL_P1_Z,
		TOOL_P2_X,
		TOOL_P2_Y,
		TOOL_P2_Z,
		TOOL_NOTE
	} combo_labels;

protected:
	void onInit()
	{
		uge_rec = 0;
		Ugelli->ReadRec( DatiUge, uge_rec ); // lettura del record

		// create combos
		m_combos[TOOL_CODE] = new C_Combo(  5, 1, MsgGetString(Msg_00261), 1, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[TOOL_TYPE] = new C_Combo(  5, 2, MsgGetString(Msg_00262), 25, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[TOOL_NOZZ] = new C_Combo(  5, 3, MsgGetString(Msg_01006), 1, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE );
		m_combos[TOOL_P1_X] = new C_Combo( 22, 5, "X:", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOOL_P1_Y] = new C_Combo( 36, 5, "Y:", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOOL_P1_Z] = new C_Combo( 15, 6, MsgGetString(Msg_00987), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOOL_P2_X] = new C_Combo( 22, 8, "X:", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOOL_P2_Y] = new C_Combo( 36, 8, "Y:", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOOL_P2_Z] = new C_Combo( 15, 9, MsgGetString(Msg_00987), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOOL_NOTE] = new C_Combo(  5, 11, MsgGetString(Msg_00175), 25, CELL_TYPE_TEXT );

		// set params
		m_combos[TOOL_NOZZ]->SetLegalChars( "12B-" );
		m_combos[TOOL_P1_X]->SetVMinMax( XMINUGE, QParam.LX_maxcl );
		m_combos[TOOL_P1_Y]->SetVMinMax( YMINUGE, QParam.LY_maxcl );
		m_combos[TOOL_P1_Z]->SetVMinMax( QHeader.DMm_HeadSens[0], 20.f );
		m_combos[TOOL_P2_X]->SetVMinMax( XMINUGE, QParam.LX_maxcl );
		m_combos[TOOL_P2_Y]->SetVMinMax( YMINUGE, QParam.LY_maxcl );
		m_combos[TOOL_P2_Z]->SetVMinMax( QHeader.DMm_HeadSens[0], 20.f );

		// add to combo list
		m_comboList->Add( m_combos[TOOL_CODE], 0, 0 );
		m_comboList->Add( m_combos[TOOL_TYPE], 1, 0 );
		m_comboList->Add( m_combos[TOOL_NOZZ], 2, 0 );
		m_comboList->Add( m_combos[TOOL_P1_X], 3, 0 );
		m_comboList->Add( m_combos[TOOL_P1_Y], 3, 1 );
		m_comboList->Add( m_combos[TOOL_P1_Z], 4, 0 );
		m_comboList->Add( m_combos[TOOL_P2_X], 5, 0 );
		m_comboList->Add( m_combos[TOOL_P2_Y], 5, 1 );
		m_comboList->Add( m_combos[TOOL_P2_Z], 6, 0 );
		m_comboList->Add( m_combos[TOOL_NOTE], 7, 0 );
	}

	void onShow()
	{
		DrawText( 5, 5, MsgGetString(Msg_00042) );
		DrawText( 5, 8, MsgGetString(Msg_00043) );

		DrawPanel( RectI( 2, 14, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 15, MsgGetString(Msg_00317), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	void onRefresh()
	{
		m_combos[TOOL_CODE]->SetTxt( DatiUge.U_code );
		m_combos[TOOL_TYPE]->SetTxt( DatiUge.U_tipo );

		switch( DatiUge.NozzleAllowed )
		{
		case UG_P1:
			m_combos[TOOL_NOZZ]->SetTxt( '1' );
			break;
		case UG_P2:
			m_combos[TOOL_NOZZ]->SetTxt( '2' );
			break;
		case UG_ALL:
			m_combos[TOOL_NOZZ]->SetTxt( 'B' );
			break;
		default:
			m_combos[TOOL_NOZZ]->SetTxt( '-' );
			break;
		}

		m_combos[TOOL_P1_X]->SetTxt( DatiUge.X_ugeP1 );
		m_combos[TOOL_P1_Y]->SetTxt( DatiUge.Y_ugeP1 );
		m_combos[TOOL_P1_Z]->SetTxt( DatiUge.Z_offset[0] );
		m_combos[TOOL_P2_X]->SetTxt( DatiUge.X_ugeP2 );
		m_combos[TOOL_P2_Y]->SetTxt( DatiUge.Y_ugeP2 );
		m_combos[TOOL_P2_Z]->SetTxt( DatiUge.Z_offset[1] );
		m_combos[TOOL_NOTE]->SetTxt( DatiUge.U_note );
	}

	void onEdit()
	{
		snprintf( DatiUge.U_tipo, 24, "%s", m_combos[TOOL_TYPE]->GetTxt() );

		switch( m_combos[TOOL_NOZZ]->GetChar() )
		{
		case '1':
			DatiUge.NozzleAllowed = UG_P1;
			break;
		case '2':
			DatiUge.NozzleAllowed = UG_P2;
			break;
		case 'B':
			DatiUge.NozzleAllowed = UG_ALL;
			break;
		default:
			DatiUge.NozzleAllowed = 0;
			break;
		}

		DatiUge.X_ugeP1 = m_combos[TOOL_P1_X]->GetFloat();
		DatiUge.Y_ugeP1 = m_combos[TOOL_P1_Y]->GetFloat();
		DatiUge.Z_offset[0] = m_combos[TOOL_P1_Z]->GetFloat();
		DatiUge.X_ugeP2 = m_combos[TOOL_P2_X]->GetFloat();
		DatiUge.Y_ugeP2 = m_combos[TOOL_P2_Y]->GetFloat();
		DatiUge.Z_offset[1] = m_combos[TOOL_P2_Z]->GetFloat();
		strncpyQ( DatiUge.U_note, m_combos[TOOL_NOTE]->GetTxt(), 20 );
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00434), K_F3, 0, SM_SeqTeach, NULL ); // Tool position sequential teaching
		m_menu->Add( MsgGetString(Msg_00435), K_F4, 0, SM_Teach, NULL );    // Tool position single teaching
		m_menu->Add( MsgGetString(Msg_00979), K_F5, 0, SM_TeachZ, NULL );   // Teaching Z-offset
		m_menu->Add( MsgGetString(Msg_01534), K_F6, 0, NULL, boost::bind( &ToolsParamsUI::selectToolType, this ) ); // Select tool type
		m_menu->Add( MsgGetString(Msg_01713), K_F7, 0, SM_FidTeach, NULL ); // Tool position teaching with fiducials
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				SM_SeqTeach->Show();
				return true;

			case K_F4:
				SM_Teach->Show();
				return true;

			case K_F5:
				SM_TeachZ->Show();
				return true;

			case K_F6:
				selectToolType();
				return true;

			case K_F7:
				SM_FidTeach->Show();
				return true;

			case K_PAGEUP:
				if( uge_rec > 0 )
				{
					Ugelli->SaveRec( DatiUge, uge_rec );
					if( u_curnoz != 0 )
					{
						resetNozzle( u_curnoz );
					}
					uge_rec--;
					Ugelli->ReadRec( DatiUge, uge_rec );
					return true;
				}
				break;

			case K_PAGEDOWN:
				if( uge_rec < 25 )
				{
					Ugelli->SaveRec( DatiUge, uge_rec );
					if( u_curnoz != 0 )
					{
						resetNozzle( u_curnoz );
					}
					uge_rec++;
					Ugelli->ReadRec( DatiUge, uge_rec );
					return true;
				}
				break;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Ugelli->SaveRec( DatiUge, uge_rec );
		Ugelli->ReloadCurUge();

		resetNozzle( 1 );
		resetNozzle( 2 );
	}

private:
	void resetNozzle( int nozzle )
	{
		PuntaZSecurityPos( nozzle );
		PuntaZPosWait( nozzle );

		PuntaRotDeg( 0, nozzle );

		while( !Check_PuntaRot( nozzle ) )
		{
			FoxPort->PauseLog();
		}
		FoxPort->RestartLog();
	}

	int selectToolType()
	{
		std::string toolName;
		int toolType = fn_ToolsDatabase( this, DatiUge.utype, toolName );

		if( toolType != -1 )
		{
			snprintf( DatiUge.U_tipo, sizeof(DatiUge.U_tipo), "%s", toolName.c_str() );
			DatiUge.utype = toolType;

			Ugelli->SaveRec( DatiUge, uge_rec );
			Ugelli->ReloadCurUge();
		}

		return 1;
	}


	GUI_SubMenu* SM_SeqTeach; // sub menu apprendimento sequenziale
	GUI_SubMenu* SM_Teach;    // sub menu apprendimento singolo
	GUI_SubMenu* SM_TeachZ;   // sub menu apprendimento Z offset
	GUI_SubMenu* SM_FidTeach; // sub menu apprendimento fiduciali
};

int fn_ToolsParams()
{
	u_curnoz = 0;
	Set_UgeBlock( OFF );

	ToolsParamsUI win;
	win.Show();
	win.Hide();

	CheckNozzlesUp();

	Set_UgeBlock( OFF );
	return 1;
}



//---------------------------------------------------------------------------
// finestra: Nozzle offset calibration
//---------------------------------------------------------------------------
int offCal_nozzle = 1;
char offCal_tool = 'Z';
float offCal_pcb = 0.f;

extern int POff_InkPosition();
extern int POff_InkCalibration();
extern int POff_InkDeltaHeights();

class NozzleOffsetCalibrationUI : public CWindowParams
{
public:
	NozzleOffsetCalibrationUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 9 );
		SetClientAreaSize( 46, 16 );

		offCal_nozzle = 1;

		char buf[80];
		snprintf( buf, 79, MsgGetString(Msg_00232), offCal_nozzle );
		SetTitle( buf );
	}

	typedef enum
	{
		PCB_H,
		TOOL,
		MARK_X,
		MARK_Y,
		OFFSET_X,
		OFFSET_Y,
		INK_Z
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[PCB_H]    = new C_Combo( 5,  1, MsgGetString(Msg_00212), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOOL]     = new C_Combo( 5,  2, MsgGetString(Msg_00953), 1, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE );
		m_combos[MARK_X]   = new C_Combo( 5,  4, MsgGetString(Msg_00853), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[MARK_Y]   = new C_Combo( 5,  5, MsgGetString(Msg_00854), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[OFFSET_X] = new C_Combo( 5,  7, MsgGetString(Msg_00864), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[OFFSET_Y] = new C_Combo( 5,  8, MsgGetString(Msg_00865), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[INK_Z]    = new C_Combo( 5, 10, MsgGetString(Msg_00832), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[PCB_H]->SetVMinMax( PCB_H_MIN, PCB_H_MAX );
		m_combos[TOOL]->SetLegalChars( CHARSET_TOOL );
		m_combos[MARK_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[MARK_Y]->SetVMinMax( QParam.LY_mincl, QParam.LY_maxcl );
		m_combos[OFFSET_X]->SetVMinMax( -80.f, 80.f );
		m_combos[OFFSET_Y]->SetVMinMax( -80.f, 80.f );
		m_combos[INK_Z]->SetVMinMax( 0.f, GetPianoZPos(offCal_nozzle) + 5.f );

		// add to combo list
		m_comboList->Add( m_combos[PCB_H],    1, 0 );
		m_comboList->Add( m_combos[TOOL],     2, 0 );
		m_comboList->Add( m_combos[MARK_X],   3, 0 );
		m_comboList->Add( m_combos[MARK_Y],   4, 0 );
		m_comboList->Add( m_combos[OFFSET_X], 5, 0 );
		m_comboList->Add( m_combos[OFFSET_Y], 6, 0 );
		m_comboList->Add( m_combos[INK_Z],    7, 0 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 12, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 13, MsgGetString(Msg_00318), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	void onRefresh()
	{
		m_combos[PCB_H]->SetTxt( offCal_pcb );
		m_combos[TOOL]->SetTxt( offCal_tool );
		m_combos[MARK_X]->SetTxt( QParam.OFFX_mark );
		m_combos[MARK_Y]->SetTxt( QParam.OFFY_mark );

		if( offCal_nozzle == 1 )
		{
			m_combos[OFFSET_X]->SetTxt( QParam.CamPunta1Offset_X );
			m_combos[OFFSET_Y]->SetTxt( QParam.CamPunta1Offset_Y );
		}
		else
		{
			m_combos[OFFSET_X]->SetTxt( QParam.CamPunta2Offset_X );
			m_combos[OFFSET_Y]->SetTxt( QParam.CamPunta2Offset_Y );
		}

		m_combos[INK_Z]->SetTxt( QHeader.InkZPos );
	}

	void onEdit()
	{
		offCal_pcb = m_combos[PCB_H]->GetFloat();
		offCal_tool = m_combos[TOOL]->GetChar();
		QParam.OFFX_mark = m_combos[MARK_X]->GetFloat();
		QParam.OFFY_mark = m_combos[MARK_Y]->GetFloat();

		if( offCal_nozzle == 1 )
		{
			QParam.CamPunta1Offset_X = m_combos[OFFSET_X]->GetFloat();
			QParam.CamPunta1Offset_Y = m_combos[OFFSET_Y]->GetFloat();
		}
		else
		{
			QParam.CamPunta2Offset_X = m_combos[OFFSET_X]->GetFloat();
			QParam.CamPunta2Offset_Y = m_combos[OFFSET_Y]->GetFloat();
		}

		QHeader.InkZPos = m_combos[INK_Z]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00859), K_F3, 0, NULL, POff_InkPosition );     // ink pos teaching
		m_menu->Add( MsgGetString(Msg_00862), K_F4, 0, NULL, POff_InkCalibration );  // mark position
		m_menu->Add( MsgGetString(Msg_00263), K_F5, 0, NULL, POff_InkDeltaHeights ); //
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				POff_InkPosition();
				return true;

			case K_F4:
				POff_InkCalibration();
				return true;

			case K_F5:
				POff_InkDeltaHeights();
				return true;

			case K_PAGEUP:
				if( offCal_nozzle > 1 )
				{
					offCal_nozzle--;

					char buf[80];
					snprintf( buf, 79, MsgGetString(Msg_00232), offCal_nozzle );
					SetTitle( buf );
					return true;
				}
				break;

			case K_PAGEDOWN:
				if( offCal_nozzle < 2 )
				{
					offCal_nozzle++;

					char buf[80];
					snprintf( buf, 79, MsgGetString(Msg_00232), offCal_nozzle );
					SetTitle( buf );
					return true;
				}
				break;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Mod_Par(QParam);
	}
};

int fn_NozzleOffsetCalibration()
{
	NozzleOffsetCalibrationUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Components placement recognition
//---------------------------------------------------------------------------
extern int Test_RectangularComponent();
extern int ComponentsPlacing_Check();

class ComponentsRecoParamsUI : public CWindowParams
{
public:
	ComponentsRecoParamsUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 9 );
		SetClientAreaSize( 40, 12 );
		SetTitle( "Components recognition params" );
	}

	typedef enum
	{
		RECT_X,
		RECT_Y,
		RECT_TOLERANCE,
		FILTER_SMOOTH_DIM,
		FILTER_BIN_THR_MIN,
		FILTER_BIN_THR_MAX,
		FILTER_APPROX,
		LIGHTER_ON,
		SHOW_DEBUG
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[RECT_X]             = new C_Combo( 5,  1, "Component X       :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[RECT_Y]             = new C_Combo( 5,  2, "Component Y       :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[RECT_TOLERANCE]     = new C_Combo( 5,  3, "Tolerance         :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[FILTER_SMOOTH_DIM]  = new C_Combo( 5,  4, "Filter smooth dim :", 7, CELL_TYPE_UINT );
		m_combos[FILTER_BIN_THR_MIN] = new C_Combo( 5,  5, "Filter bin thr min:", 7, CELL_TYPE_UINT );
		m_combos[FILTER_BIN_THR_MAX] = new C_Combo( 5,  6, "Filter bin thr max:", 7, CELL_TYPE_UINT );
		m_combos[FILTER_APPROX]      = new C_Combo( 5,  7, "Filter approx     :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[LIGHTER_ON]         = new C_Combo( 5,  9, "Camera led on     :", 4, CELL_TYPE_YN );
		m_combos[SHOW_DEBUG]         = new C_Combo( 5, 10, "Show debug        :", 5, CELL_TYPE_UINT );


		// set params
		m_combos[RECT_X]->SetVMinMax( 0.f, 100.f );
		m_combos[RECT_Y]->SetVMinMax( 0.f, 100.f );
		m_combos[RECT_TOLERANCE]->SetVMinMax( 0.f, 1.f );
		m_combos[FILTER_SMOOTH_DIM]->SetVMinMax( 1, 29 );
		m_combos[FILTER_BIN_THR_MIN]->SetVMinMax( 1, 255 );
		m_combos[FILTER_BIN_THR_MAX]->SetVMinMax( 1, 255 );
		m_combos[FILTER_APPROX]->SetVMinMax( 0.01f, 100.f );
		m_combos[SHOW_DEBUG]->SetVMinMax( 0, 256 );


		// add to combo list
		m_comboList->Add( m_combos[RECT_X],             0, 0 );
		m_comboList->Add( m_combos[RECT_Y],             1, 0 );
		m_comboList->Add( m_combos[RECT_TOLERANCE],     2, 0 );
		m_comboList->Add( m_combos[FILTER_SMOOTH_DIM],  3, 0 );
		m_comboList->Add( m_combos[FILTER_BIN_THR_MIN], 4, 0 );
		m_comboList->Add( m_combos[FILTER_BIN_THR_MAX], 5, 0 );
		m_comboList->Add( m_combos[FILTER_APPROX],      6, 0 );
		m_comboList->Add( m_combos[LIGHTER_ON],         7, 0 );
		m_comboList->Add( m_combos[SHOW_DEBUG],         8, 0 );


		dimX = Vision.rectX / 100.f;
		dimY = Vision.rectY / 100.f;
		tolerance = Vision.rectTolerance / 100.f;
		approx = Vision.rectFApprox / 100.f;
	}

	void onShow()
	{
		DrawText( 33, 1, "mm" );
		DrawText( 33, 2, "mm" );
		DrawText( 33, 3, "mm" );
	}

	void onRefresh()
	{
		m_combos[RECT_X]->SetTxt( dimX );
		m_combos[RECT_Y]->SetTxt( dimY );
		m_combos[RECT_TOLERANCE]->SetTxt( tolerance );
		m_combos[FILTER_SMOOTH_DIM]->SetTxt( Vision.rectFSmoothDim );
		m_combos[FILTER_BIN_THR_MIN]->SetTxt( Vision.rectFBinThrMin );
		m_combos[FILTER_BIN_THR_MAX]->SetTxt( Vision.rectFBinThrMax );
		m_combos[FILTER_APPROX]->SetTxt( approx );
		m_combos[LIGHTER_ON]->SetTxtYN( mapTestLight );
		m_combos[SHOW_DEBUG]->SetTxt( mapTestDebug );
	}

	void onEdit()
	{
		dimX = m_combos[RECT_X]->GetFloat();
		dimY = m_combos[RECT_Y]->GetFloat();
		tolerance = m_combos[RECT_TOLERANCE]->GetFloat();
		Vision.rectFSmoothDim = m_combos[FILTER_SMOOTH_DIM]->GetInt();
		Vision.rectFBinThrMin = m_combos[FILTER_BIN_THR_MIN]->GetInt();
		Vision.rectFBinThrMax = m_combos[FILTER_BIN_THR_MAX]->GetInt();
		approx = m_combos[FILTER_APPROX]->GetFloat();
		mapTestLight = m_combos[LIGHTER_ON]->GetYN();
		mapTestDebug = m_combos[SHOW_DEBUG]->GetInt();

		Vision.rectX = cvRound( dimX * 100.f );
		Vision.rectY = cvRound( dimY * 100.f );
		Vision.rectTolerance = cvRound( tolerance * 100.f );
		Vision.rectFApprox = cvRound( approx * 100.f );
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01361), K_F3, 0, NULL, boost::bind( &ComponentsRecoParamsUI::OnTeachPosition, this ) );
		m_menu->Add( MsgGetString(Msg_00176), K_F4, 0, NULL, Test_RectangularComponent );
		m_menu->Add( "Esegui verifica", K_F5, 0, NULL, ComponentsPlacing_Check );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				OnTeachPosition();
				return true;

			case K_F4:
				Test_RectangularComponent();
				return true;

			case K_F5:
				ComponentsPlacing_Check();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		VisDataSave( Vision );
	}

private:
	int OnTeachPosition()
	{
		Test_TeachPosition( RECTCOMP_IMG );
		return 1;
	}

	float dimX;
	float dimY;
	float tolerance;
	float approx;
};


//---------------------------------------------------------------------------
// finestra: Ink mark recognition parameters
//---------------------------------------------------------------------------
extern int Test_InkMark();
extern int InkMarks_Check();

class InkMarksRecoParamsUI : public CWindowParams
{
public:
	InkMarksRecoParamsUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 9 );
		SetClientAreaSize( 40, 9 );
		SetTitle( "Ink mark recognition params" );
	}

	typedef enum
	{
		INK_DIAMETER,
		INK_TOLERANCE,
		FILTER_SMOOTH_DIM,
		FILTER_EDGE_THR,
		INK_ACCUM,
		SHOW_DEBUG
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[INK_DIAMETER]      = new C_Combo( 5, 1, "Ink mark diameter :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[INK_TOLERANCE]     = new C_Combo( 5, 2, "Tolerance         :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[FILTER_SMOOTH_DIM] = new C_Combo( 5, 3, "Filter smooth dim :", 7, CELL_TYPE_UINT );
		m_combos[FILTER_EDGE_THR]   = new C_Combo( 5, 4, "Filter edge thr   :", 7, CELL_TYPE_UINT );
		m_combos[INK_ACCUM]         = new C_Combo( 5, 5, "Accumulator       :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[SHOW_DEBUG]        = new C_Combo( 5, 7, "Show debug        :", 5, CELL_TYPE_UINT );


		// set params
		m_combos[INK_DIAMETER]->SetVMinMax( 0.01f, 10.f );
		m_combos[INK_TOLERANCE]->SetVMinMax( 0.f, 1.f );
		m_combos[FILTER_SMOOTH_DIM]->SetVMinMax( 1, 29 );
		m_combos[FILTER_EDGE_THR]->SetVMinMax( 1, 255 );
		m_combos[INK_ACCUM]->SetVMinMax( 0.01f, 0.99f );
		m_combos[SHOW_DEBUG]->SetVMinMax( 0, 256 );


		// add to combo list
		m_comboList->Add( m_combos[INK_DIAMETER],      0, 0 );
		m_comboList->Add( m_combos[INK_TOLERANCE],     1, 0 );
		m_comboList->Add( m_combos[FILTER_SMOOTH_DIM], 2, 0 );
		m_comboList->Add( m_combos[FILTER_EDGE_THR],   3, 0 );
		m_comboList->Add( m_combos[INK_ACCUM],         4, 0 );
		m_comboList->Add( m_combos[SHOW_DEBUG],        5, 0 );


		diameter = Vision.circleDiameter / 100.f;
		tolerance = Vision.circleTolerance / 100.f;
	}

	void onShow()
	{
		DrawText( 33, 1, "mm" );
		DrawText( 33, 2, "mm" );
	}

	void onRefresh()
	{
		m_combos[INK_DIAMETER]->SetTxt( diameter );
		m_combos[INK_TOLERANCE]->SetTxt( tolerance );
		m_combos[FILTER_SMOOTH_DIM]->SetTxt( Vision.circleFSmoothDim );
		m_combos[FILTER_EDGE_THR]->SetTxt( Vision.circleFEdgeThr );
		m_combos[INK_ACCUM]->SetTxt( Vision.circleFAccum );
		m_combos[SHOW_DEBUG]->SetTxt( mapTestDebug );
	}

	void onEdit()
	{
		diameter = m_combos[INK_DIAMETER]->GetFloat();
		tolerance = m_combos[INK_TOLERANCE]->GetFloat();
		Vision.circleFSmoothDim = m_combos[FILTER_SMOOTH_DIM]->GetInt();
		Vision.circleFEdgeThr = m_combos[FILTER_EDGE_THR]->GetInt();
		Vision.circleFAccum = m_combos[INK_ACCUM]->GetFloat();
		mapTestDebug = m_combos[SHOW_DEBUG]->GetInt();

		Vision.circleDiameter = cvRound( diameter * 100 );
		Vision.circleTolerance = cvRound( tolerance * 100 );
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01361), K_F3, 0, NULL, boost::bind( &InkMarksRecoParamsUI::OnTeachPosition, this ) );
		m_menu->Add( MsgGetString(Msg_00176), K_F4, 0, NULL, Test_InkMark );
		m_menu->Add( "Esegui verifica", K_F5, 0, NULL, InkMarks_Check );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				OnTeachPosition();
				return true;

			case K_F4:
				Test_InkMark();
				return true;

			case K_F5:
				InkMarks_Check();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		VisDataSave( Vision );
	}

private:
	int OnTeachPosition()
	{
		Test_TeachPosition( INKMARK_IMG );
		return 1;
	}

	float diameter;
	float tolerance;
};


//---------------------------------------------------------------------------
// finestra: Placement mapping
//---------------------------------------------------------------------------
class PlacementMappingUI : public CWindowTable
{
public:
	PlacementMappingUI( CWindow* parent ) : CWindowTable( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 64, 16 );

		SetTitle( MsgGetString(Msg_00113) );
	}

protected:
	void onInit()
	{
		// create table
		m_table = new CTable( 2, 4, 8, TABLE_STYLE_DEFAULT, this );

		// add columns
		m_table->AddCol( MsgGetString(Msg_00532),  8, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_CENTERED );
		m_table->AddCol( "X", 12, CELL_TYPE_SINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
		m_table->AddCol( "Y", 12, CELL_TYPE_SINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
		m_table->AddCol( "X", 12, CELL_TYPE_SINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
		m_table->AddCol( "Y", 12, CELL_TYPE_SINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );

		// set params
		m_table->SetText( 0, 0, "  0" );
		m_table->SetText( 1, 0, " 90" );
		m_table->SetText( 2, 0, "180" );
		m_table->SetText( 3, 0, "270" );
		m_table->SetText( 4, 0, "  0" );
		m_table->SetText( 5, 0, " 90" );
		m_table->SetText( 6, 0, "180" );
		m_table->SetText( 7, 0, "270" );

		// set callback function
		m_table->SetOnSelectCellCallback( boost::bind( &PlacementMappingUI::onSelectionChange, this, _1, _2 ) );
	}

	void onShow()
	{
		DrawTextCentered( 11, 36, 1, MsgGetString(Msg_00042) );
		DrawTextCentered( 37, 62, 1, MsgGetString(Msg_00043) );
	}

	void onRefresh()
	{
		int sx, sy;

		// legge i passi di correzione dal vettore di mappatura
		for( int i = 0; i < 4; i++ )
		{
			Read_off( sx, sy, i );
			m_table->SetText( i, 1, sx );
			m_table->SetText( i, 2, sy );

			Read_off( sx, sy, i+4 );
			m_table->SetText( i, 3, sx );
			m_table->SetText( i, 4, sy );
		}
		for( int i = 4; i < 8; i++ )
		{
			Read_off( sx, sy, i+4 );
			m_table->SetText( i, 1, sx );
			m_table->SetText( i, 2, sy );

			Read_off( sx, sy, i+8 );
			m_table->SetText( i, 3, sx );
			m_table->SetText( i, 4, sy );
		}
	}

	void onEdit()
	{
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00732), K_F3, 0, NULL, boost::bind( &PlacementMappingUI::onExecute, this ) );
		m_menu->Add( MsgGetString(Msg_00819), K_F4, 0, NULL, boost::bind( &PlacementMappingUI::onDelete, this ) );
		m_menu->Add( MsgGetString(Msg_00147), K_F8, 0, NULL, boost::bind( &PlacementMappingUI::onComponentsReco, this ) );
		m_menu->Add( "Ink marks verify", K_F9, 0, NULL, boost::bind( &PlacementMappingUI::onInkMarksReco, this ) ); //TODO: mettere sottomenu in F8 con scelta componente/inchiostro
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				onExecute();
				return true;

			case K_F4:
				onDelete();
				return true;

			case K_F8:
				onComponentsReco();
				return true;

			case K_F9:
				onInkMarksReco();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
	}

private:
	int onExecute()
	{
		M_offset();
		return 1;
	}

	int onDelete()
	{
		// reset mappatura offset ?
		if( W_Deci( 0, MsgGetString(Msg_00564) ) )
		{
			Reset_off();
		}
		return 1;
	}

	int onComponentsReco()
	{
		ComponentsRecoParamsUI win( this );
		win.Show();
		win.Hide();
		return 1;
	}

	int onInkMarksReco()
	{
		InkMarksRecoParamsUI win( this );
		win.Show();
		win.Hide();
		return 1;
	}

	int onSelectionChange( unsigned int row, unsigned int col )
	{
		DrawPie( PointI( GetX()+GetW()/2, GetY()+14*GUI_CharH() ), 18, (row+1)*90 );
		return 1;
	}
};

int fn_PlacementMapping()
{
	PlacementMappingUI win( 0 );
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Working modes
//---------------------------------------------------------------------------
class WorkingModesUI : public CWindowParams
{
public:
	WorkingModesUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 83, 14 );
		SetTitle( MsgGetString(Msg_00238) );
	}
	
	typedef enum
	{
		OUTPUT_ON_FILE,
		CONTINUOUS_ASSEMBLING,
		SNIPER_CENTERING,
		VACUUM_SENSOR,
		PRESSURE_SENSOR,
		CHECK_COMP_SNIPER,
		AUTO_RESET_XY,
		DISPENSER_ON,
		AUTO_CHECK_REF,
		SNIPER_SCAN_PICK,
		SNIPER_SCAN_ERROR,
		DEMO_MODE,
		Z_PRE_MOVE,
		NUM_RETRY,
		OPTIMIZATION_MODE,
		ENABLE_MAP_XY,
		LOG_MODE,
		ENABLE_FEEDER_DB,
		ENABLE_LAN,
		SMART_FEEDER_ONLY,
		ENABLE_ALARM,
		ASK_ON_PACK_CHANGE
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[OUTPUT_ON_FILE] =        new C_Combo( 2, 1, MsgGetString(Msg_00251), 4, CELL_TYPE_YN );
		m_combos[CONTINUOUS_ASSEMBLING] = new C_Combo( 2, 2, MsgGetString(Msg_00253), 4, CELL_TYPE_YN );
		m_combos[SNIPER_CENTERING] =      new C_Combo( 2, 3, MsgGetString(Msg_00839), 4, CELL_TYPE_YN );
		m_combos[VACUUM_SENSOR] =         new C_Combo( 2, 4, MsgGetString(Msg_00255), 4, CELL_TYPE_YN );
		m_combos[PRESSURE_SENSOR] =       new C_Combo( 2, 5, MsgGetString(Msg_00256), 4, CELL_TYPE_YN );
		m_combos[CHECK_COMP_SNIPER] =     new C_Combo( 2, 6, MsgGetString(Msg_01512), 4, CELL_TYPE_YN );
		m_combos[AUTO_RESET_XY] =         new C_Combo( 2, 7, MsgGetString(Msg_00257), 4, CELL_TYPE_YN );
		m_combos[DISPENSER_ON] =          new C_Combo( 2, 8, MsgGetString(Msg_00740), 4, CELL_TYPE_YN );
		m_combos[AUTO_CHECK_REF] =        new C_Combo( 2, 9, MsgGetString(Msg_00758), 4, CELL_TYPE_YN );
		m_combos[SNIPER_SCAN_PICK] =      new C_Combo( 2, 10, MsgGetString(Msg_00970), 4, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[SNIPER_SCAN_ERROR] =     new C_Combo( 2, 11, MsgGetString(Msg_00971), 4, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		
		m_combos[DEMO_MODE] =             new C_Combo( 42, 1, MsgGetString(Msg_01179), 4, CELL_TYPE_YN );
		m_combos[Z_PRE_MOVE] =            new C_Combo( 42, 2, MsgGetString(Msg_01206), 4, CELL_TYPE_YN );
		m_combos[NUM_RETRY] =             new C_Combo( 42, 3, MsgGetString(Msg_00741), 4, CELL_TYPE_UINT );
		m_combos[OPTIMIZATION_MODE] =     new C_Combo( 42, 4, MsgGetString(Msg_00254), 4, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[ENABLE_MAP_XY] =         new C_Combo( 42, 5, MsgGetString(Msg_01565), 4, CELL_TYPE_YN );
		m_combos[LOG_MODE] =              new C_Combo( 42, 6, MsgGetString(Msg_01604), 4, CELL_TYPE_UINT );
		m_combos[ENABLE_FEEDER_DB] =      new C_Combo( 42, 7, MsgGetString(Msg_01647), 4, CELL_TYPE_YN );
		m_combos[ENABLE_LAN] =            new C_Combo( 42, 8, MsgGetString(Msg_01646), 4, CELL_TYPE_YN );
		m_combos[SMART_FEEDER_ONLY] =     new C_Combo( 42, 9, MsgGetString(Msg_01857), 4, CELL_TYPE_YN );
		m_combos[ENABLE_ALARM] =          new C_Combo( 42, 10, MsgGetString(Msg_01891), 4, CELL_TYPE_YN );
		m_combos[ASK_ON_PACK_CHANGE] =    new C_Combo( 42, 11, MsgGetString(Msg_05099), 4, CELL_TYPE_YN );
		
		// set params
		m_combos[SNIPER_SCAN_PICK]->SetLegalStrings( 4, (char**)Nozzles_StrVect );
		m_combos[SNIPER_SCAN_ERROR]->SetLegalStrings( 4, (char**)Nozzles_StrVect );
		m_combos[NUM_RETRY]->SetVMinMax( 1, 999 );
		m_combos[OPTIMIZATION_MODE]->SetLegalStrings( 4, (char**)Nozzles_StrVect );
		m_combos[LOG_MODE]->SetVMinMax( 0, 255 );

		// add to combo list
		for( int p = 0, i = OUTPUT_ON_FILE; i <= SNIPER_SCAN_ERROR; p++, i++ )
		{
			m_comboList->Add( m_combos[i], p, 0 );
		}
		for( int p = 0, i = DEMO_MODE; i <= ASK_ON_PACK_CHANGE; p++, i++ )
		{
			m_comboList->Add( m_combos[i], p, 1 );
		}

#ifdef __DISP2
	#ifndef __DOME_FEEDER
		if( !Get_SingleDispenserPar() )
		{
			m_combos[AUTO_RESET_XY]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[AUTO_RESET_XY]->SetStyle( m_combos[AUTO_RESET_XY]->GetStyle() | CELL_STYLE_READONLY );
		}
	#endif
#endif
	}

	void onRefresh()
	{
		m_combos[OUTPUT_ON_FILE]->SetTxtYN( QParam.US_file );
		m_combos[CONTINUOUS_ASSEMBLING]->SetTxtYN( QParam.AS_cont );
		
		if( QParam.DemoMode )
		{
			m_combos[SNIPER_CENTERING]->SetTxt( MSG_NO );
			m_combos[VACUUM_SENSOR]->SetTxt( MSG_NO );
			m_combos[PRESSURE_SENSOR]->SetTxt( MSG_NO );
			m_combos[CHECK_COMP_SNIPER]->SetTxt( MSG_NO );
			
			m_combos[SNIPER_CENTERING]->SetStyle( m_combos[SNIPER_CENTERING]->GetStyle() | CELL_STYLE_NOSEL );
			m_combos[VACUUM_SENSOR]->SetStyle( m_combos[VACUUM_SENSOR]->GetStyle() | CELL_STYLE_NOSEL );
			m_combos[PRESSURE_SENSOR]->SetStyle( m_combos[PRESSURE_SENSOR]->GetStyle() | CELL_STYLE_NOSEL );
			m_combos[CHECK_COMP_SNIPER]->SetStyle( m_combos[CHECK_COMP_SNIPER]->GetStyle() | CELL_STYLE_NOSEL );
		}
		else
		{
			m_combos[SNIPER_CENTERING]->SetTxt( QHeader.modal & NOLASCENT_MASK ? MSG_NO : MSG_YES );
			m_combos[VACUUM_SENSOR]->SetTxtYN( QParam.AT_vuoto );
			m_combos[PRESSURE_SENSOR]->SetTxtYN( QParam.AT_press );
			m_combos[CHECK_COMP_SNIPER]->SetTxtYN( !QParam.Disable_LaserCompCheck );
			
			m_combos[SNIPER_CENTERING]->SetStyle( m_combos[SNIPER_CENTERING]->GetStyle() & ~CELL_STYLE_NOSEL );
			m_combos[VACUUM_SENSOR]->SetStyle( m_combos[VACUUM_SENSOR]->GetStyle() & ~CELL_STYLE_NOSEL );
			m_combos[PRESSURE_SENSOR]->SetStyle( m_combos[PRESSURE_SENSOR]->GetStyle() & ~CELL_STYLE_NOSEL );
			m_combos[CHECK_COMP_SNIPER]->SetStyle( m_combos[CHECK_COMP_SNIPER]->GetStyle() & ~CELL_STYLE_NOSEL );
		}
		
		m_combos[AUTO_RESET_XY]->SetTxtYN( QParam.AZ_tassi );
		m_combos[DISPENSER_ON]->SetTxtYN( QParam.Dispenser );
		m_combos[AUTO_CHECK_REF]->SetTxtYN( QParam.Autoref );
		m_combos[SNIPER_SCAN_PICK]->SetStrings_Pos( QHeader.modal & (SCANPREL1_MASK | SCANPREL2_MASK) );
		m_combos[SNIPER_SCAN_ERROR]->SetStrings_Pos( (QHeader.modal & (SCANERR1_MASK | SCANERR2_MASK)) >> 7 );
		
		m_combos[DEMO_MODE]->SetTxtYN( QParam.DemoMode );
		m_combos[Z_PRE_MOVE]->SetTxtYN( QParam.ZPreDownMode );
		m_combos[NUM_RETRY]->SetTxt( int( QParam.N_try ) );
		m_combos[OPTIMIZATION_MODE]->SetStrings_Pos( ( QParam.AutoOptimize >= 0 && QParam.AutoOptimize <= 3) ? QParam.AutoOptimize : 0 );
		m_combos[ENABLE_MAP_XY]->SetTxtYN( QHeader.modal & ENABLE_XYMAP );
		m_combos[LOG_MODE]->SetTxt( int( QParam.LogMode ) );
		m_combos[ENABLE_FEEDER_DB]->SetTxtYN( QHeader.modal & ENABLE_CARINT );
		m_combos[ENABLE_LAN]->SetTxtYN( nwpar.enabled );
		m_combos[SMART_FEEDER_ONLY]->SetTxtYN( QHeader.modal & ONLY_SMARTFEEDERS );
		m_combos[ENABLE_ALARM]->SetTxtYN( QHeader.modal & ENABLE_ALARMSIGNAL );
		m_combos[ASK_ON_PACK_CHANGE]->SetTxtYN( QHeader.modal & ASK_PACKAGE_CHANGE );
	}
	
	void onEdit()
	{
		QParam.US_file = m_combos[OUTPUT_ON_FILE]->GetYN();
		QParam.AS_cont = m_combos[CONTINUOUS_ASSEMBLING]->GetYN();
		
		if( !QParam.DemoMode )
		{
			QHeader.modal = m_combos[SNIPER_CENTERING]->GetYN() ? (QHeader.modal & (~NOLASCENT_MASK)) : (QHeader.modal | NOLASCENT_MASK);
			QParam.AT_vuoto = m_combos[VACUUM_SENSOR]->GetYN();
			QParam.AT_press = m_combos[PRESSURE_SENSOR]->GetYN();
			QParam.Disable_LaserCompCheck = !m_combos[CHECK_COMP_SNIPER]->GetYN();
		}
		
		QParam.AZ_tassi = m_combos[AUTO_RESET_XY]->GetYN();
		QParam.Dispenser = m_combos[DISPENSER_ON]->GetYN();
		QParam.Autoref = m_combos[AUTO_CHECK_REF]->GetYN();
		
		QHeader.modal &= ~(SCANPREL1_MASK | SCANPREL2_MASK);
		switch( m_combos[SNIPER_SCAN_PICK]->GetStrings_Pos() )
		{
			case 1:
				QHeader.modal |= SCANPREL1_MASK;
				break;
			case 2:
				QHeader.modal |= SCANPREL2_MASK;
				break;
			case 3:
				QHeader.modal |= (SCANPREL1_MASK | SCANPREL2_MASK);
				break;
		}
	
		QHeader.modal &= ~(SCANERR1_MASK | SCANERR2_MASK);
		switch( m_combos[SNIPER_SCAN_ERROR]->GetStrings_Pos() )
		{
			case 1:
				QHeader.modal |= SCANERR1_MASK;
				break;
			case 2:
				QHeader.modal |= SCANERR2_MASK;
				break;
			case 3:
				QHeader.modal |= (SCANERR1_MASK | SCANERR2_MASK);
				break;
		}
		
		
		QParam.DemoMode = m_combos[DEMO_MODE]->GetYN();
		QParam.ZPreDownMode = m_combos[Z_PRE_MOVE]->GetYN();
		QParam.N_try = m_combos[NUM_RETRY]->GetInt();
		QParam.AutoOptimize = m_combos[OPTIMIZATION_MODE]->GetStrings_Pos();
		QHeader.modal = m_combos[ENABLE_MAP_XY]->GetYN() ? (QHeader.modal | ENABLE_XYMAP) : (QHeader.modal & (~ENABLE_XYMAP));
		QParam.LogMode = m_combos[LOG_MODE]->GetInt();
		QHeader.modal = m_combos[ENABLE_FEEDER_DB]->GetYN() ? (QHeader.modal | ENABLE_CARINT) : (QHeader.modal & (~ENABLE_CARINT));
		nwpar.enabled = m_combos[ENABLE_LAN]->GetYN();
		QHeader.modal = m_combos[SMART_FEEDER_ONLY]->GetYN() ? (QHeader.modal | ONLY_SMARTFEEDERS) : (QHeader.modal & (~ONLY_SMARTFEEDERS));
		QHeader.modal = m_combos[ENABLE_ALARM]->GetYN() ? (QHeader.modal | ENABLE_ALARMSIGNAL) : (QHeader.modal & (~ENABLE_ALARMSIGNAL));
		QHeader.modal = m_combos[ASK_ON_PACK_CHANGE]->GetYN() ? (QHeader.modal | ASK_PACKAGE_CHANGE) : (QHeader.modal & (~ASK_PACKAGE_CHANGE));
	}

	void onClose()
	{
		Set_OnFile( QParam.US_file );

		Mod_Cfg( QHeader );
		Mod_Par( QParam );
		WriteNetPar( nwpar );
	}
};

int fn_WorkingModes()
{
	struct NetPar prevnetpar = nwpar;

	bool prevNetEnabled_GlobalFlag = IsNetEnabled();
	int prevcarint = (QHeader.modal & ENABLE_CARINT);


	WorkingModesUI win;
	win.Show();
	win.Hide();

	if( prevcarint != (QHeader.modal & ENABLE_CARINT) )
	{
		if( QHeader.modal & ENABLE_CARINT)
		{
			// abilita db precedentemente disattivo
			EnableFeederDB();
		}
		else
		{
			// disabilita db precedentemente attivo
	
			// se la rete era effettivamente attiva ed e' stato richiesta una disattivazione di questa
			if( prevNetEnabled_GlobalFlag && !nwpar.enabled )
			{
				DisableFeederDB();
			}
		}
	}

	if( prevnetpar.enabled != nwpar.enabled )
	{
		//la rete e' stata attivata o disattivata: richiesto riavvio del sw
		bipbip();
		W_Mess( MsgGetString(Msg_02113) ); // restart required
	}

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Control parameters
//---------------------------------------------------------------------------
class ControlParamsUI : public CWindowParams
{
public:
	ControlParamsUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 70, 18 );
		SetTitle( MsgGetString(Msg_01522) );
	}

	typedef enum
	{
		WAIT_STEADY_POS,
		TIMEOUT_STEADY_POS,
		DELTA1_STEADY_POS,
		DELTA2_STEADY_POS,
		DELTA_TOOLS_CHECK,
		COMP_PICKUP_TIME,
		VACUO_ACT_TIME,
		PICKUP_DELTA,
		PICKUP_SLOW_SPEED,
		VACUUM_MAX_1,
		VACUUM_MAX_2,
		PICK_HEIGHT_DELTA
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[WAIT_STEADY_POS]    = new C_Combo( 3, 1, MsgGetString(Msg_01523), 7, CELL_TYPE_UINT );
		m_combos[TIMEOUT_STEADY_POS] = new C_Combo( 3, 2, MsgGetString(Msg_01524), 7, CELL_TYPE_UINT );
		m_combos[DELTA1_STEADY_POS]  = new C_Combo( 3, 3, MsgGetString(Msg_01525), 7, CELL_TYPE_UINT );
		m_combos[DELTA2_STEADY_POS]  = new C_Combo( 59, 3, "", 7, CELL_TYPE_UINT );
		m_combos[DELTA_TOOLS_CHECK]  = new C_Combo( 3, 5, MsgGetString(Msg_01477), 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[COMP_PICKUP_TIME]   = new C_Combo( 3, 7, MsgGetString(Msg_01526), 7, CELL_TYPE_UINT );
		m_combos[VACUO_ACT_TIME]     = new C_Combo( 3, 9, MsgGetString(Msg_01491), 7, CELL_TYPE_UINT );
		m_combos[PICKUP_DELTA]       = new C_Combo( 3, 10, MsgGetString(Msg_02092), 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[PICKUP_SLOW_SPEED]  = new C_Combo( 3, 11, MsgGetString(Msg_02093), 8, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[VACUUM_MAX_1]       = new C_Combo( 3, 13, MsgGetString(Msg_00581), 7, CELL_TYPE_UINT );
		m_combos[VACUUM_MAX_2]       = new C_Combo( 59, 13, "", 7, CELL_TYPE_UINT );
		m_combos[PICK_HEIGHT_DELTA]  = new C_Combo( 3, 15, MsgGetString(Msg_00260), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[WAIT_STEADY_POS]->SetVMinMax( 0, 255 );
		m_combos[TIMEOUT_STEADY_POS]->SetVMinMax( 10, 2000 );
		m_combos[DELTA1_STEADY_POS]->SetVMinMax( 0, QHeader.Enc_step1 );
		m_combos[DELTA2_STEADY_POS]->SetVMinMax( 0, QHeader.Enc_step2 );
		m_combos[DELTA_TOOLS_CHECK]->SetVMinMax( (float)DELTAUGECHECK_MIN, (float)DELTAUGECHECK_MAX );
		m_combos[COMP_PICKUP_TIME]->SetVMinMax( 0, 999 );
		m_combos[VACUO_ACT_TIME]->SetVMinMax( 0, 999 );
		m_combos[PICKUP_DELTA]->SetVMinMax( 0.f,(float)MAX_SOFTPICK_DELTA );
		m_combos[PICKUP_SLOW_SPEED]->SetLegalStrings( 3, (char**)Speeds_StrVect );
		m_combos[VACUUM_MAX_1]->SetVMinMax( 60, 100 );
		m_combos[VACUUM_MAX_2]->SetVMinMax( 60, 100 );
		m_combos[PICK_HEIGHT_DELTA]->SetVMinMax( -5.f, 5.f );
		
		// add to combo list
		m_comboList->Add( m_combos[WAIT_STEADY_POS],    0, 0 );
		m_comboList->Add( m_combos[TIMEOUT_STEADY_POS], 1, 0 );
		m_comboList->Add( m_combos[DELTA1_STEADY_POS],  2, 0 );
		m_comboList->Add( m_combos[DELTA2_STEADY_POS],  2, 1 );
		m_comboList->Add( m_combos[DELTA_TOOLS_CHECK],  3, 0 );
		m_comboList->Add( m_combos[COMP_PICKUP_TIME],   4, 0 );
		m_comboList->Add( m_combos[VACUO_ACT_TIME],     5, 0 );
		m_comboList->Add( m_combos[PICKUP_DELTA],       6, 0 );
		m_comboList->Add( m_combos[PICKUP_SLOW_SPEED],  7, 0 );
		m_comboList->Add( m_combos[VACUUM_MAX_1],       8, 0 );
		m_comboList->Add( m_combos[VACUUM_MAX_2],       8, 1 );
		m_comboList->Add( m_combos[PICK_HEIGHT_DELTA],  9, 0 );
	}

	void onRefresh()
	{
		m_combos[WAIT_STEADY_POS]->SetTxt( QHeader.Time_1 );
		m_combos[TIMEOUT_STEADY_POS]->SetTxt( QHeader.Time_2 );
		m_combos[DELTA1_STEADY_POS]->SetTxt( int(QHeader.BrushSteady_Step[0]) );
		m_combos[DELTA2_STEADY_POS]->SetTxt( int(QHeader.BrushSteady_Step[1]) );
		m_combos[DELTA_TOOLS_CHECK]->SetTxt( QParam.DeltaUgeCheck );
		m_combos[COMP_PICKUP_TIME]->SetTxt( QHeader.ComponentDwellTime );
		m_combos[VACUO_ACT_TIME]->SetTxt( QHeader.PrelVacuoDelta );
		m_combos[PICKUP_DELTA]->SetTxt( QHeader.SoftPickDelta );
		m_combos[PICKUP_SLOW_SPEED]->SetStrings_Pos( QHeader.softPickSpeedIndex );
		m_combos[VACUUM_MAX_1]->SetTxt( QParam.vuoto_1 );
		m_combos[VACUUM_MAX_2]->SetTxt( QParam.vuoto_2 );
		m_combos[PICK_HEIGHT_DELTA]->SetTxt( QHeader.automaticPickHeightCorrection );
	}

	void onEdit()
	{
		QHeader.Time_1 = m_combos[WAIT_STEADY_POS]->GetInt();
		QHeader.Time_2 = m_combos[TIMEOUT_STEADY_POS]->GetInt();
		QHeader.BrushSteady_Step[0] = m_combos[DELTA1_STEADY_POS]->GetInt();
		QHeader.BrushSteady_Step[1] = m_combos[DELTA2_STEADY_POS]->GetInt();
		QParam.DeltaUgeCheck = m_combos[DELTA_TOOLS_CHECK]->GetFloat();
		QHeader.ComponentDwellTime = m_combos[COMP_PICKUP_TIME]->GetInt();
		QHeader.PrelVacuoDelta = m_combos[VACUO_ACT_TIME]->GetInt();
		QHeader.SoftPickDelta = m_combos[PICKUP_DELTA]->GetFloat();
		QHeader.softPickSpeedIndex = m_combos[PICKUP_SLOW_SPEED]->GetStrings_Pos();
		QParam.vuoto_1 = m_combos[VACUUM_MAX_1]->GetInt();
		QParam.vuoto_2 = m_combos[VACUUM_MAX_2]->GetInt();
		QHeader.automaticPickHeightCorrection = m_combos[PICK_HEIGHT_DELTA]->GetFloat();
	}

	void onClose()
	{
		Mod_Cfg( QHeader );
		Mod_Par( QParam );
	}
};

int fn_ControlParams()
{
	ControlParamsUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Pattern recognition parameters
//---------------------------------------------------------------------------
extern int Test_MapPattern();

class PatternRecoParamsUI : public CWindowParams
{
public:
	PatternRecoParamsUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 40, 9 );
		SetTitle( MsgGetString(Msg_06019) );
	}

	typedef enum
	{
		CIRCLE_DIAMETER,
		CIRCLE_TOLERANCE,
		FILTER_SMOOTH_DIM,
		FILTER_EDGE_THR,
		CIRCLE_ACCUM,
		SHOW_DEBUG
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[CIRCLE_DIAMETER]   = new C_Combo( 5, 1, "Circle Diameter   :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[CIRCLE_TOLERANCE]  = new C_Combo( 5, 2, "Tolerance         :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[FILTER_SMOOTH_DIM] = new C_Combo( 5, 3, "Filter smooth dim :", 7, CELL_TYPE_UINT );
		m_combos[FILTER_EDGE_THR]   = new C_Combo( 5, 4, "Filter edge thr   :", 7, CELL_TYPE_UINT );
		m_combos[CIRCLE_ACCUM]      = new C_Combo( 5, 5, "Accumulator       :", 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[SHOW_DEBUG]        = new C_Combo( 5, 7, "Show debug        :", 5, CELL_TYPE_UINT );


		// set params
		m_combos[CIRCLE_DIAMETER]->SetVMinMax( 0.01f, 10.f );
		m_combos[CIRCLE_TOLERANCE]->SetVMinMax( 0.f, 1.f );
		m_combos[FILTER_SMOOTH_DIM]->SetVMinMax( 1, 29 );
		m_combos[FILTER_EDGE_THR]->SetVMinMax( 1, 255 );
		m_combos[CIRCLE_ACCUM]->SetVMinMax( 0.01f, 0.99f );
		m_combos[SHOW_DEBUG]->SetVMinMax( 0, 256 );


		// add to combo list
		m_comboList->Add( m_combos[CIRCLE_DIAMETER],   0, 0 );
		m_comboList->Add( m_combos[CIRCLE_TOLERANCE],  1, 0 );
		m_comboList->Add( m_combos[FILTER_SMOOTH_DIM], 2, 0 );
		m_comboList->Add( m_combos[FILTER_EDGE_THR],   3, 0 );
		m_comboList->Add( m_combos[CIRCLE_ACCUM],      4, 0 );
		m_comboList->Add( m_combos[SHOW_DEBUG],        5, 0 );


		diameter = Vision.circleDiameter / 100.f;
		tolerance = Vision.circleTolerance / 100.f;
	}

	void onShow()
	{
		DrawText( 33, 1, "mm" );
		DrawText( 33, 2, "mm" );
	}

	void onRefresh()
	{
		m_combos[CIRCLE_DIAMETER]->SetTxt( diameter );
		m_combos[CIRCLE_TOLERANCE]->SetTxt( tolerance );
		m_combos[FILTER_SMOOTH_DIM]->SetTxt( Vision.circleFSmoothDim );
		m_combos[FILTER_EDGE_THR]->SetTxt( Vision.circleFEdgeThr );
		m_combos[CIRCLE_ACCUM]->SetTxt( Vision.circleFAccum );
		m_combos[SHOW_DEBUG]->SetTxt( mapTestDebug );
	}

	void onEdit()
	{
		diameter = m_combos[CIRCLE_DIAMETER]->GetFloat();
		tolerance = m_combos[CIRCLE_TOLERANCE]->GetFloat();
		Vision.circleFSmoothDim = m_combos[FILTER_SMOOTH_DIM]->GetInt();
		Vision.circleFEdgeThr = m_combos[FILTER_EDGE_THR]->GetInt();
		Vision.circleFAccum = m_combos[CIRCLE_ACCUM]->GetFloat();
		mapTestDebug = m_combos[SHOW_DEBUG]->GetInt();

		Vision.circleDiameter = cvRound( diameter * 100 );
		Vision.circleTolerance = cvRound( tolerance * 100 );
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01361), K_F3, 0, NULL, boost::bind( &PatternRecoParamsUI::OnTeachPosition, this ) );
		m_menu->Add( MsgGetString(Msg_00176), K_F4, 0, NULL, Test_MapPattern );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				OnTeachPosition();
				return true;

			case K_F4:
				Test_MapPattern();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		VisDataSave( Vision );
	}

private:
	int OnTeachPosition()
	{
		Test_TeachPosition( MAPPING_IMG );
		return 1;
	}

	float diameter;
	float tolerance;
};



//---------------------------------------------------------------------------
// finestra: Axes calibration
//---------------------------------------------------------------------------
#define WIN_AXESCALIBRATION_H   20


class AxesCalibrationUI : public CWindowParams
{
public:
	AxesCalibrationUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 71, WIN_AXESCALIBRATION_H );
		SetTitle( MsgGetString(Msg_00236) );

		SM_AxesOrtho = new GUI_SubMenu();
		SM_AxesOrtho->Add( MsgGetString(Msg_00862), K_F1, 0, NULL, boost::bind( &AxesCalibrationUI::onExecute_AxesOrtho, this ) );
		SM_AxesOrtho->Add( MsgGetString(Msg_00217), K_F2, 0, NULL, boost::bind( &AxesCalibrationUI::onCheck_AxesOrtho, this ) );

		SM_EncScale = new GUI_SubMenu();
		SM_EncScale->Add( MsgGetString(Msg_00862), K_F1, 0, NULL, boost::bind( &AxesCalibrationUI::onExecute_EncScale, this ) );
		SM_EncScale->Add( MsgGetString(Msg_00217), K_F2, 0, NULL, boost::bind( &AxesCalibrationUI::onCheck_EncScale, this ) );

		//TODO - messaggi
		SM_CheckMap = new GUI_SubMenu();
		SM_CheckMap->Add( "Check Y", K_F1, 0, NULL, boost::bind( &AxesCalibrationUI::onCheck_Y, this ) );
		SM_CheckMap->Add( "Check Y Neg", K_F2, 0, NULL, boost::bind( &AxesCalibrationUI::onCheck_Y_Neg, this ) );
		SM_CheckMap->Add( MsgGetString(Msg_00735), K_F4, 0, NULL, boost::bind( &AxesCalibrationUI::onCheck_Corr, this ) );
		SM_CheckMap->Add( MsgGetString(Msg_00736), K_F12, 0, NULL, boost::bind( &AxesCalibrationUI::onShowLastCheck, this ) );
	}

	~AxesCalibrationUI()
	{
		delete SM_AxesOrtho;
		delete SM_EncScale;
		delete SM_CheckMap;
	}

	typedef enum
	{
		FUNC_POS_X,
		FUNC_POS_Y,
		ORTHO_MOVE_X,
		ORTHO_MOVE_Y,
		ORTHO_ERROR,
		ENC_MOVE_X,
		ENC_MOVE_Y,
		ENC_SCALE_X,
		ENC_SCALE_Y,
		MAP_NUM_X,
		MAP_NUM_Y,
		MAP_OFF_X,
		MAP_OFF_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[FUNC_POS_X]   = new C_Combo(  3,  1, MsgGetString(Msg_00797), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[FUNC_POS_Y]   = new C_Combo(  3,  2, MsgGetString(Msg_00798), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		m_combos[ORTHO_MOVE_X] = new C_Combo(  3,  6, MsgGetString(Msg_06006), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[ORTHO_MOVE_Y] = new C_Combo(  3,  7, MsgGetString(Msg_06007), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[ORTHO_ERROR]  = new C_Combo( 37,  6, MsgGetString(Msg_06011), 11, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 8 );

		m_combos[ENC_MOVE_X]   = new C_Combo(  3, 11, MsgGetString(Msg_06006), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[ENC_MOVE_Y]   = new C_Combo(  3, 12, MsgGetString(Msg_06007), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[ENC_SCALE_X]  = new C_Combo( 37, 11, MsgGetString(Msg_06008), 11, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 8 );
		m_combos[ENC_SCALE_Y]  = new C_Combo( 37, 12, MsgGetString(Msg_06009), 11, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 8 );

		m_combos[MAP_NUM_X]    = new C_Combo(  3, 16, MsgGetString(Msg_00799), 7, CELL_TYPE_UINT );
		m_combos[MAP_NUM_Y]    = new C_Combo(  3, 17, MsgGetString(Msg_00800), 7, CELL_TYPE_UINT );
		m_combos[MAP_OFF_X]    = new C_Combo( 37, 16, MsgGetString(Msg_00801), 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[MAP_OFF_Y]    = new C_Combo( 37, 17, MsgGetString(Msg_00802), 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[FUNC_POS_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[FUNC_POS_Y]->SetVMinMax( QParam.LY_mincl, QParam.LY_maxcl );

		m_combos[ORTHO_MOVE_X]->SetVMinMax( -400.f, 440.0f );
		m_combos[ORTHO_MOVE_Y]->SetVMinMax( -400.f, 440.0f );
		m_combos[ORTHO_ERROR]->SetVMinMax( -0.05f, 0.05f );

		m_combos[ENC_MOVE_X]->SetVMinMax( -400.f, 440.0f );
		m_combos[ENC_MOVE_Y]->SetVMinMax( -400.f, 440.0f );
		m_combos[ENC_SCALE_X]->SetVMinMax( 0.5f, 1.5f );
		m_combos[ENC_SCALE_Y]->SetVMinMax( 0.5f, 1.5f );

		m_combos[MAP_NUM_X]->SetVMinMax( 0, 999 );
		m_combos[MAP_NUM_Y]->SetVMinMax( 0, 999 );
		m_combos[MAP_OFF_X]->SetVMinMax( 1.f, 100.f );
		m_combos[MAP_OFF_Y]->SetVMinMax( 1.f, 100.f );

		// add to combo list
		m_comboList->Add( m_combos[FUNC_POS_X],   0, 0 );
		m_comboList->Add( m_combos[FUNC_POS_Y],   1, 0 );

		m_comboList->Add( m_combos[ORTHO_MOVE_X], 2, 0 );
		m_comboList->Add( m_combos[ORTHO_MOVE_Y], 3, 0 );
		m_comboList->Add( m_combos[ORTHO_ERROR],  2, 1 );

		m_comboList->Add( m_combos[ENC_MOVE_X],   4, 0 );
		m_comboList->Add( m_combos[ENC_MOVE_Y],   5, 0 );
		m_comboList->Add( m_combos[ENC_SCALE_X],  4, 1 );
		m_comboList->Add( m_combos[ENC_SCALE_Y],  5, 1 );

		m_comboList->Add( m_combos[MAP_NUM_X],    6, 0 );
		m_comboList->Add( m_combos[MAP_NUM_Y],    7, 0 );
		m_comboList->Add( m_combos[MAP_OFF_X],    6, 1 );
		m_comboList->Add( m_combos[MAP_OFF_Y],    7, 1 );
	}

	void onShow()
	{
		DrawText( 30, 1, "mm" );
		DrawText( 30, 2, "mm" );

		DrawSubTitle( 4, MsgGetString(Msg_00215) ); // Axes orthogonality parameters
		DrawText( 30, 6, "mm" );
		DrawText( 30, 7, "mm" );

		DrawSubTitle( 9, MsgGetString(Msg_00216) ); // Encoder scale parameters
		DrawText( 30, 11, "mm" );
		DrawText( 30, 12, "mm" );

		DrawSubTitle( 14, MsgGetString(Msg_00759) ); // Mapping parameter
		DrawText( 64, 16, "mm" );
		DrawText( 64, 17, "mm" );
	}

	void onRefresh()
	{
		m_combos[FUNC_POS_X]->SetTxt( Vision.pos_x );
		m_combos[FUNC_POS_Y]->SetTxt( Vision.pos_y );

		m_combos[ORTHO_MOVE_X]->SetTxt( QParam.OrthoXY_Movement_x );
		m_combos[ORTHO_MOVE_Y]->SetTxt( QParam.OrthoXY_Movement_y );
		m_combos[ORTHO_ERROR]->SetTxt( QParam.OrthoXY_Error );

		m_combos[ENC_MOVE_X]->SetTxt( QParam.EncScale_Movement_x );
		m_combos[ENC_MOVE_Y]->SetTxt( QParam.EncScale_Movement_y );
		m_combos[ENC_SCALE_X]->SetTxt( QParam.EncScale_x );
		m_combos[ENC_SCALE_Y]->SetTxt( QParam.EncScale_y );

		m_combos[MAP_NUM_X]->SetTxt( Vision.mapnum_x );
		m_combos[MAP_NUM_Y]->SetTxt( Vision.mapnum_y );
		m_combos[MAP_OFF_X]->SetTxt( Vision.mapoff_x );
		m_combos[MAP_OFF_Y]->SetTxt( Vision.mapoff_y );

		if( Vision.mapnum_x == 0 || Vision.mapnum_y == 0 )
		{
			DrawText( 6, 19, "TWS Automation default map table" );
		}
		else
		{
			DrawText( 6, 19, "Custom map area                 " );
		}
	}

	void onEdit()
	{
		Vision.pos_x = m_combos[FUNC_POS_X]->GetFloat();
		Vision.pos_y = m_combos[FUNC_POS_Y]->GetFloat();

		QParam.OrthoXY_Movement_x = m_combos[ORTHO_MOVE_X]->GetFloat();
		QParam.OrthoXY_Movement_y = m_combos[ORTHO_MOVE_Y]->GetFloat();
		QParam.OrthoXY_Error = m_combos[ORTHO_ERROR]->GetFloat();

		QParam.EncScale_Movement_x = m_combos[ENC_MOVE_X]->GetFloat();
		QParam.EncScale_Movement_y = m_combos[ENC_MOVE_Y]->GetFloat();
		QParam.EncScale_x = m_combos[ENC_SCALE_X]->GetFloat();
		QParam.EncScale_y = m_combos[ENC_SCALE_Y]->GetFloat();

		Vision.mapnum_x = m_combos[MAP_NUM_X]->GetInt();
		Vision.mapnum_y = m_combos[MAP_NUM_Y]->GetInt();
		Vision.mapoff_x = m_combos[MAP_OFF_X]->GetFloat();
		Vision.mapoff_y = m_combos[MAP_OFF_Y]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00215), K_F3, 0, SM_AxesOrtho, NULL ); // Axes orthogonality
		m_menu->Add( MsgGetString(Msg_00216), K_F4, 0, SM_EncScale, NULL );  // Encoder scale
		m_menu->Add( MsgGetString(Msg_00732), K_F5, 0, NULL, boost::bind( &AxesCalibrationUI::onErrormap_Create, this ) ); // Create error map
		m_menu->Add( MsgGetString(Msg_00733), K_F6, 0, SM_CheckMap, NULL );     // Verify error map
		m_menu->Add( MsgGetString(Msg_00819), K_F7, 0, NULL, Errormap_Delete ); // Delete error map
		m_menu->Add( MsgGetString(Msg_06019), K_F8, 0, NULL, boost::bind( &AxesCalibrationUI::onPatternRecoParams, this ) );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				SM_AxesOrtho->Show();
				return true;

			case K_F4:
				SM_EncScale->Show();
				return true;

			case K_F5:
				onErrormap_Create();
				return true;

			case K_F6:
				SM_CheckMap->Show();
				return true;

			case K_F7:
				Errormap_Delete();
				return true;

			case K_F8:
				onPatternRecoParams();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Mod_Par( QParam );
		VisDataSave( Vision );
	}

private:
	int onExecute_AxesOrtho()
	{
		Deselect();
		OrthogonalityXY_Calibrate( true );
		Select();
		return 1;
	}

	int onCheck_AxesOrtho()
	{
		Deselect();
		OrthogonalityXY_Calibrate( false );
		Select();
		return 1;
	}

	int onExecute_EncScale()
	{
		Deselect();
		EncoderScale_Calibrate( true );
		Select();
		return 1;
	}

	int onCheck_EncScale()
	{
		Deselect();
		EncoderScale_Calibrate( false );
		Select();
		return 1;
	}

	int onErrormap_Create()
	{
		Deselect();
		Errormap_Create( 0 ); // dir Y
		Select();
		return 1;
	}

	int onCheck_Y()
	{
		Deselect();
		Errormap_Check_XY( 0 ); // dir Y pos
		Select();
		return 1;
	}

	int onCheck_Y_Neg()
	{
		Deselect();
		Errormap_Check_XY( 1 ); // dir Y neg
		Select();
		return 1;
	}

	int onCheck_Corr()
	{
		Deselect();
		Errormap_Check_Correction( 0 );
		Select();
		return 1;
	}

	int onShowLastCheck()
	{
		Deselect();
		Show_Errormap_Check( this );
		Select();
		return 1;
	}

	int onPatternRecoParams()
	{
		PatternRecoParamsUI win( this );
		win.Show();
		win.Hide();
		return 1;
	}

	GUI_SubMenu* SM_AxesOrtho; // sub menu ortogonalita' assi
	GUI_SubMenu* SM_EncScale; // sub menu scala encoder
	GUI_SubMenu* SM_CheckMap; // sub menu verifica mappatura
};

int fn_AxesCalibration()
{
	AxesCalibrationUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Rotation center calibration
//---------------------------------------------------------------------------
extern struct CfgTeste MapTeste;

extern int CCal_Calc();
extern RotCenterCalibStruct rotCenterData;

class RotationCenterCalibrationUI : public CWindowParams
{
public:
	RotationCenterCalibrationUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 9 );
		SetClientAreaSize( 46, 14 );

		char buf[80];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01053), rotCenterData.nozzle );
		SetTitle( buf );
	}

	typedef enum
	{
		POS_Z_1,
		POS_Z_2,
		ROT_CENTER_1,
		ROT_CENTER_2,
		DELTA_POS_1,
		DELTA_POS_2,
		PARAM1,
		PARAM2
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[POS_Z_1]      = new C_Combo(  3,  1, MsgGetString(Msg_01698), 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		m_combos[POS_Z_2]      = new C_Combo( 35,  1, "", 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		m_combos[ROT_CENTER_1] = new C_Combo(  3,  2, MsgGetString(Msg_01731), 7, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		m_combos[ROT_CENTER_2] = new C_Combo( 35,  2, "", 7, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		m_combos[DELTA_POS_1]  = new C_Combo(  3,  3, MsgGetString(Msg_01062), 7, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		m_combos[DELTA_POS_2]  = new C_Combo( 35,  3, "", 7, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		m_combos[PARAM1]       = new C_Combo(  3,  6, MsgGetString(Msg_05072), 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY, 3 );
		m_combos[PARAM2]       = new C_Combo(  3,  7, MsgGetString(Msg_05073), 7, CELL_TYPE_UDEC, CELL_STYLE_READONLY, 3 );

		// set params
		m_combos[PARAM1]->SetVMinMax( -2.f, 2.f );
		m_combos[PARAM2]->SetVMinMax( 2.f, 4.f );

		// add to combo list
		m_comboList->Add( m_combos[POS_Z_1],      1, 0 );
		m_comboList->Add( m_combos[POS_Z_2],      1, 1 );
		m_comboList->Add( m_combos[ROT_CENTER_1], 2, 0 );
		m_comboList->Add( m_combos[ROT_CENTER_2], 2, 1 );
		m_comboList->Add( m_combos[DELTA_POS_1],  3, 0 );
		m_comboList->Add( m_combos[DELTA_POS_2],  3, 1 );
		m_comboList->Add( m_combos[PARAM1],       4, 0 );
		m_comboList->Add( m_combos[PARAM2],       5, 0 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 10, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 11, MsgGetString(Msg_00318), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	void onRefresh()
	{
		m_combos[POS_Z_1]->SetTxt( rotCenterData.pos_z[rotCenterData.nozzle-1][0] );
		m_combos[POS_Z_2]->SetTxt( rotCenterData.pos_z[rotCenterData.nozzle-1][1] );
		m_combos[ROT_CENTER_1]->SetTxt( rotCenterData.rot_center[rotCenterData.nozzle-1][0] );
		m_combos[ROT_CENTER_2]->SetTxt( rotCenterData.rot_center[rotCenterData.nozzle-1][1] );
		m_combos[DELTA_POS_1]->SetTxt( rotCenterData.delta_pos[rotCenterData.nozzle-1][0] );
		m_combos[DELTA_POS_2]->SetTxt( rotCenterData.delta_pos[rotCenterData.nozzle-1][1] );

		m_combos[PARAM1]->SetTxt( MapTeste.ccal_z_cal_m[rotCenterData.nozzle-1] );
		m_combos[PARAM2]->SetTxt( MapTeste.ccal_z_cal_q[rotCenterData.nozzle-1] );
	}

	void onEdit()
	{
		MapTeste.ccal_z_cal_m[rotCenterData.nozzle-1] = m_combos[PARAM1]->GetFloat();
		MapTeste.ccal_z_cal_q[rotCenterData.nozzle-1] = m_combos[PARAM2]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01983), K_F3, 0, NULL, CCal_Calc ); // execute calibration
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				CCal_Calc();
				return true;

			case K_PAGEUP:
				if( rotCenterData.nozzle > 1 )
				{
					rotCenterData.nozzle--;

					char buf[80];
					snprintf( buf, sizeof(buf), MsgGetString(Msg_01053), rotCenterData.nozzle );
					SetTitle( buf );
					return true;
				}
				break;

			case K_PAGEDOWN:
				if( rotCenterData.nozzle < 2 )
				{
					rotCenterData.nozzle++;

					char buf[80];
					snprintf( buf, sizeof(buf), MsgGetString(Msg_01053), rotCenterData.nozzle );
					SetTitle( buf );
					return true;
				}
				break;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Mod_Map( MapTeste );
	}
};

int fn_RotationCenterCalibration()
{
	RotationCenterCalibrationUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Nozzle rotation adjustment
//---------------------------------------------------------------------------
class NozzleRotationAdjustmentUI : public CWindowTable
{
public:
	NozzleRotationAdjustmentUI() : CWindowTable( 0 )
	{
		nozzle = 1;

		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 46, 14 );

		char buf[80];
		snprintf( buf, 79, MsgGetString(Msg_00233), nozzle );
		SetTitle( buf );
	}

protected:
	void onInit()
	{
		// create table
		m_table = new CTable( 2, 2, 4, TABLE_STYLE_DEFAULT, this );

		// add columns
		m_table->AddCol( MsgGetString(Msg_00285), 16, CELL_TYPE_SINT, CELL_STYLE_CENTERED );
		m_table->AddCol( MsgGetString(Msg_00532),  8, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
		m_table->AddCol( MsgGetString(Msg_00533), 16, CELL_TYPE_SINT, CELL_STYLE_CENTERED );

		// set params
		m_table->SetColMinMax( 0, ADJROT_MINSTEP, ADJROT_MAXSTEP );
		m_table->SetColMinMax( 2, ADJROT_MINSTEP, ADJROT_MAXSTEP );

		m_table->SetText( 0, 1, "  0" );
		m_table->SetText( 1, 1, " 90" );
		m_table->SetText( 2, 1, "180" );
		m_table->SetText( 3, 1, "270" );

		// set callback function
		m_table->SetOnSelectCellCallback( boost::bind( &NozzleRotationAdjustmentUI::onSelectionChange, this, _1, _2 ) );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 10, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 11, MsgGetString(Msg_00318), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	void onRefresh()
	{
		if( nozzle == 1)
		{
			m_table->SetText( 0, 0, MapTeste.T1_360[TABLE_CORRECT_SNIPER] );
			m_table->SetText( 0, 2, MapTeste.T1_360[TABLE_CORRECT_EXTCAM] );

			m_table->SetText( 1, 0, MapTeste.T1_90[TABLE_CORRECT_SNIPER] );
			m_table->SetText( 1, 2, MapTeste.T1_90[TABLE_CORRECT_EXTCAM] );

			m_table->SetText( 2, 0, MapTeste.T1_180[TABLE_CORRECT_SNIPER] );
			m_table->SetText( 2, 2, MapTeste.T1_180[TABLE_CORRECT_EXTCAM] );

			m_table->SetText( 3, 0, MapTeste.T1_270[TABLE_CORRECT_SNIPER] );
			m_table->SetText( 3, 2, MapTeste.T1_270[TABLE_CORRECT_EXTCAM] );
		}
		else
		{
			m_table->SetText( 0, 0, MapTeste.T2_360[TABLE_CORRECT_SNIPER] );
			m_table->SetText( 0, 2, MapTeste.T2_360[TABLE_CORRECT_EXTCAM] );

			m_table->SetText( 1, 0, MapTeste.T2_90[TABLE_CORRECT_SNIPER] );
			m_table->SetText( 1, 2, MapTeste.T2_90[TABLE_CORRECT_EXTCAM] );

			m_table->SetText( 2, 0, MapTeste.T2_180[TABLE_CORRECT_SNIPER] );
			m_table->SetText( 2, 2, MapTeste.T2_180[TABLE_CORRECT_EXTCAM] );

			m_table->SetText( 3, 0, MapTeste.T2_270[TABLE_CORRECT_SNIPER] );
			m_table->SetText( 3, 2, MapTeste.T2_270[TABLE_CORRECT_EXTCAM] );
		}
	}

	void onEdit()
	{
		if( nozzle == 1)
		{
			MapTeste.T1_360[TABLE_CORRECT_SNIPER] = m_table->GetInt( 0, 0 );
			MapTeste.T1_360[TABLE_CORRECT_EXTCAM] = m_table->GetInt( 0, 2 );

			MapTeste.T1_90[TABLE_CORRECT_SNIPER] = m_table->GetInt( 1, 0 );
			MapTeste.T1_90[TABLE_CORRECT_EXTCAM] = m_table->GetInt( 1, 2 );

			MapTeste.T1_180[TABLE_CORRECT_SNIPER] = m_table->GetInt( 2, 0 );
			MapTeste.T1_180[TABLE_CORRECT_EXTCAM] = m_table->GetInt( 2, 2 );

			MapTeste.T1_270[TABLE_CORRECT_SNIPER] = m_table->GetInt( 3, 0 );
			MapTeste.T1_270[TABLE_CORRECT_EXTCAM] = m_table->GetInt( 3, 2 );
		}
		else
		{
			MapTeste.T2_360[TABLE_CORRECT_SNIPER] = m_table->GetInt( 0, 0 );
			MapTeste.T2_360[TABLE_CORRECT_EXTCAM] = m_table->GetInt( 0, 2 );

			MapTeste.T2_90[TABLE_CORRECT_SNIPER] = m_table->GetInt( 1, 0 );
			MapTeste.T2_90[TABLE_CORRECT_EXTCAM] = m_table->GetInt( 1, 2 );

			MapTeste.T2_180[TABLE_CORRECT_SNIPER] = m_table->GetInt( 2, 0 );
			MapTeste.T2_180[TABLE_CORRECT_EXTCAM] = m_table->GetInt( 2, 2 );

			MapTeste.T2_270[TABLE_CORRECT_SNIPER] = m_table->GetInt( 3, 0 );
			MapTeste.T2_270[TABLE_CORRECT_EXTCAM] = m_table->GetInt( 3, 2 );
		}
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_PAGEUP:
				if( nozzle > 1 )
				{
					nozzle--;

					char buf[80];
					snprintf( buf, 79, MsgGetString(Msg_00233), nozzle );
					SetTitle( buf );
					return true;
				}
				break;

			case K_PAGEDOWN:
				if( nozzle < 2 )
				{
					nozzle++;

					char buf[80];
					snprintf( buf, 79, MsgGetString(Msg_00233), nozzle );
					SetTitle( buf );
					return true;
				}
				break;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Mod_Map( MapTeste );
	}

	int onSelectionChange( unsigned int row, unsigned int col )
	{
		DrawPie( PointI( GetX()+GetW()/2, GetY()+8*GUI_CharH() ), 18, (row+1)*90 );
		return 1;
	}

	int nozzle;
};

int fn_NozzleRotationAdjustment()
{
	NozzleRotationAdjustmentUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Unit origin position
//---------------------------------------------------------------------------
int _teachUnitOrigin()
{
	if( ZeroMaccCal( QParam.X_zmacc, QParam.Y_zmacc ) )
	{
		return SetHome_PuntaXY();
	}
	return 1;
}

int _imageUnitOrigin()
{
	ShowImgParams( ZEROMACHIMG );
	return 1;
}

class UnitOriginPositionUI : public CWindowParams
{
public:
	UnitOriginPositionUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 41, 5 );
		SetTitle( MsgGetString(Msg_00422) );
	}

	typedef enum
	{
		ORIGIN_X,
		ORIGIN_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[ORIGIN_X] = new C_Combo(  3, 2, "X :", 9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[ORIGIN_Y] = new C_Combo( 22, 2, "Y :", 9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		
		// set params
		m_combos[ORIGIN_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[ORIGIN_Y]->SetVMinMax( QParam.LY_mincl, QParam.LY_maxcl );
		
		// add to combo list
		m_comboList->Add( m_combos[ORIGIN_X], 0, 0 );
		m_comboList->Add( m_combos[ORIGIN_Y], 0, 1 );
	}

	void onShow()
	{
		DrawText( 17, 2, "mm" );
		DrawText( 36, 2, "mm" );
	}

	void onRefresh()
	{
		m_combos[ORIGIN_X]->SetTxt( QParam.X_zmacc );
		m_combos[ORIGIN_Y]->SetTxt( QParam.Y_zmacc );
	}

	void onEdit()
	{
		QParam.X_zmacc = m_combos[ORIGIN_X]->GetFloat();
		QParam.Y_zmacc = m_combos[ORIGIN_Y]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00422), K_F3, 0, NULL, _teachUnitOrigin ); // autoapp. zero macchina
		m_menu->Add( MsgGetString(Msg_00754), K_F4, 0, NULL, _imageUnitOrigin ); // parametri immagine zero
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				_teachUnitOrigin();
				return true;
			
			case K_F4:
				_imageUnitOrigin();
				return true;
			
			default:
				break;
		}
	
		return false;
	}

	void onClose()
	{
		Mod_Par( QParam );
	}
};

int fn_UnitOriginPosition()
{
	UnitOriginPositionUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Placement area calibration
//---------------------------------------------------------------------------
extern int PianoCal_Manual();
extern int PianoCal_PosAuto();
extern float xPianoCal;
extern float yPianoCal;

class PlacementAreaCalibrationUI : public CWindowParams
{
public:
	PlacementAreaCalibrationUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 60, 4 );
		SetTitle( MsgGetString(Msg_01227) );
	}

	typedef enum
	{
		PLATE_H
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[PLATE_H] = new C_Combo( 4, 1, MsgGetString(Msg_01228), 9, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		
		// set params
		m_combos[PLATE_H]->SetVMinMax( 0.f, QHeader.Max_NozHeight[0] );
		
		// add to combo list
		m_comboList->Add( m_combos[PLATE_H], 0, 0 );
	}

	void onRefresh()
	{
		m_combos[PLATE_H]->SetTxt( QHeader.Zero_Piano );
	}

	void onEdit()
	{
		QHeader.Zero_Piano = m_combos[PLATE_H]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00862), K_F3, 0, NULL, PianoCal_Manual );  // esegui procedura
		m_menu->Add( MsgGetString(Msg_01038), K_F4, 0, NULL, PianoCal_PosAuto ); // posizione calibrazione
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				PianoCal_Manual();
				return true;
		
			case K_F4:
				PianoCal_PosAuto();
				return true;
		
			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Mod_Cfg( QHeader );
	}
};

int fn_PlacementAreaCalib()
{
	xPianoCal = QParam.LX_mincl+(QParam.LX_maxcl-QParam.LX_mincl)/2;
	yPianoCal = QParam.LY_mincl+(QParam.LY_maxcl-QParam.LY_mincl)/2;
	
	PlacementAreaCalibrationUI win;
	win.Show();
	win.Hide();

	PuntaZSecurityPos( 1 );
	PuntaZPosWait( 1 );
	
	return 1;
}



//---------------------------------------------------------------------------
// finestra: Sniper delta Z calibration
//---------------------------------------------------------------------------
#ifdef __SNIPER

extern int DeltaZSniper_Calibrate();
extern int DeltaZSniper_PosAuto();
extern float xDeltaZSniper;
extern float yDeltaZSniper;

class DeltaZCalibrationUI : public CWindowParams
{
public:
	DeltaZCalibrationUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 60, 4 );
		SetTitle( MsgGetString(Msg_05017) );
	}

	typedef enum
	{
		DELTA_Z
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[DELTA_Z] = new C_Combo( 4, 1, MsgGetString(Msg_05021), 9, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		
		// set params
		m_combos[DELTA_Z]->SetVMinMax( 0.f, QHeader.Max_NozHeight[0] );
		
		// add to combo list
		m_comboList->Add( m_combos[DELTA_Z], 0, 0 );
	}

	void onRefresh()
	{
		m_combos[DELTA_Z]->SetTxt( QHeader.Z12_Zero_delta );
	}

	void onEdit()
	{
		QHeader.Z12_Zero_delta = m_combos[DELTA_Z]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01968), K_F3, 0, NULL, DeltaZSniper_Calibrate ); // calibrazione delta z
		m_menu->Add( MsgGetString(Msg_01038), K_F4, 0, NULL, DeltaZSniper_PosAuto );   // posizione calibrazione
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				DeltaZSniper_Calibrate();
				return true;
	
			case K_F4:
				DeltaZSniper_PosAuto();
				return true;
	
			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Mod_Cfg( QHeader );
	}
};

int fn_DeltaZCalibration()
{
	xDeltaZSniper = QParam.LX_mincl+(QParam.LX_maxcl-QParam.LX_mincl)/2;
	yDeltaZSniper = QParam.LY_mincl+(QParam.LY_maxcl-QParam.LY_mincl)/2;
	
	DeltaZCalibrationUI win;
	win.Show();
	win.Hide();
	
	PuntaZSecurityPos(1);
	PuntaZSecurityPos(2);
	PuntaZPosWait(1);
	PuntaZPosWait(2);
	
	return 1;
}

#endif



//---------------------------------------------------------------------------
// finestra: Limits and positions
//---------------------------------------------------------------------------
class LimitsPositionsUI : public CWindowParams
{
public:
	LimitsPositionsUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 44, 17 );
		SetTitle( MsgGetString(Msg_07025) );
	}

	typedef enum
	{
		AXIS_X_MIN,
		AXIS_X_MAX,
		AXIS_Y_MIN,
		AXIS_Y_MAX,
		DIS_COMP_X,
		DIS_COMP_Y,
		END_POS_X,
		END_POS_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[AXIS_X_MIN] = new C_Combo(  9, 3, "X :", 9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[AXIS_X_MAX] = new C_Combo( 24, 3, "",    9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[AXIS_Y_MIN] = new C_Combo(  9, 4, "Y :", 9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[AXIS_Y_MAX] = new C_Combo( 24, 4, "",    9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		m_combos[DIS_COMP_X] = new C_Combo(  9, 8, "X :", 9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[DIS_COMP_Y] = new C_Combo(  9, 9, "Y :", 9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		m_combos[END_POS_X] = new C_Combo(  9, 13, "X :", 9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[END_POS_Y] = new C_Combo(  9, 14, "Y :", 9, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[AXIS_X_MIN]->SetVMinMax( LXCAMPO_MIN, LXCAMPO_MAX );
		m_combos[AXIS_X_MAX]->SetVMinMax( LXCAMPO_MIN, LXCAMPO_MAX );
		m_combos[AXIS_Y_MIN]->SetVMinMax( LYCAMPO_MIN, LYCAMPO_MAX );
		m_combos[AXIS_Y_MAX]->SetVMinMax( LYCAMPO_MIN, LYCAMPO_MAX );

		m_combos[DIS_COMP_X]->SetVMinMax( QParam.LX_mincl-QParam.CamPunta1Offset_X, QParam.LX_maxcl-QParam.CamPunta2Offset_X );
		m_combos[DIS_COMP_Y]->SetVMinMax( QParam.LY_mincl-QParam.CamPunta1Offset_Y, QParam.LY_maxcl-QParam.CamPunta2Offset_Y );

		m_combos[END_POS_X]->SetVMinMax( ZXLIMIT_MIN, ZXLIMIT_MAX );
		m_combos[END_POS_Y]->SetVMinMax( ZYLIMIT_MIN, ZYLIMIT_MAX );

		// add to combo list
		m_comboList->Add( m_combos[AXIS_X_MIN], 0, 0 );
		m_comboList->Add( m_combos[AXIS_X_MAX], 0, 1 );
		m_comboList->Add( m_combos[AXIS_Y_MIN], 1, 0 );
		m_comboList->Add( m_combos[AXIS_Y_MAX], 1, 1 );

		m_comboList->Add( m_combos[DIS_COMP_X], 2, 0 );
		m_comboList->Add( m_combos[DIS_COMP_Y], 3, 0 );

		m_comboList->Add( m_combos[END_POS_X], 4, 0 );
		m_comboList->Add( m_combos[END_POS_Y], 5, 0 );
	}

	void onShow()
	{
		DrawSubTitle( 0, MsgGetString(Msg_00423) ); // limiti del campo lavoro
		DrawText( 16, 2, "Min" );
		DrawText( 28, 2, "Max" );

		DrawSubTitle( 6, MsgGetString(Msg_00424) ); // posizione scarico

		DrawSubTitle( 11, MsgGetString(Msg_00589) ); // posizione scarico
	}

	void onRefresh()
	{
		m_combos[AXIS_X_MIN]->SetTxt( QParam.LX_mincl );
		m_combos[AXIS_X_MAX]->SetTxt( QParam.LX_maxcl );
		m_combos[AXIS_Y_MIN]->SetTxt( QParam.LY_mincl );
		m_combos[AXIS_Y_MAX]->SetTxt( QParam.LY_maxcl );

		m_combos[DIS_COMP_X]->SetTxt( QParam.PX_scaric );
		m_combos[DIS_COMP_Y]->SetTxt( QParam.PY_scaric );

		m_combos[END_POS_X]->SetTxt( QParam.X_endbrd );
		m_combos[END_POS_Y]->SetTxt( QParam.Y_endbrd );
	}

	void onEdit()
	{
		QParam.LX_mincl = m_combos[AXIS_X_MIN]->GetFloat();
		QParam.LX_maxcl = m_combos[AXIS_X_MAX]->GetFloat();
		QParam.LY_mincl = m_combos[AXIS_Y_MIN]->GetFloat();
		QParam.LY_maxcl = m_combos[AXIS_Y_MAX]->GetFloat();

		QParam.PX_scaric = m_combos[DIS_COMP_X]->GetFloat();
		QParam.PY_scaric = m_combos[DIS_COMP_Y]->GetFloat();

		QParam.X_endbrd = m_combos[END_POS_X]->GetFloat();
		QParam.Y_endbrd = m_combos[END_POS_Y]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_07026), K_F3, 0, NULL, Sca_auto ); // app. posizione scarico
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				Sca_auto();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Mod_Par( QParam );
	}
};

int fn_LimitsPositions()
{
	LimitsPositionsUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Dispenser points position
//---------------------------------------------------------------------------
extern int Col_auto();
extern int Colla_test();
extern int colla_test;
extern float X_colla, Y_colla;
extern int PColla_ndisp;

class DispenserPointsPositionUI : public CWindowParams
{
public:
	DispenserPointsPositionUI( CWindow* parent, int ndisp ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 46, 5 );

		#ifdef __DISP2
		char buf[80];
		snprintf( buf, 80, "%s - %d", MsgGetString(Msg_00699), ndisp );
		SetTitle( buf );
		#else
		SetTitle( MsgGetString(Msg_00699) );
		#endif
	}

	typedef enum
	{
		POS_X,
		POS_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[POS_X] = new C_Combo(  6, 2, "X :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Y] = new C_Combo( 25, 2, "Y :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[POS_X]->SetVMinMax( -9000.f, QParam.LX_maxcl );
		m_combos[POS_Y]->SetVMinMax( -9000.f, QParam.LY_maxcl );

		// add to combo list
		m_comboList->Add( m_combos[POS_X], 0, 0 );
		m_comboList->Add( m_combos[POS_Y], 0, 1 );
	}

	void onShow()
	{
	}

	void onRefresh()
	{
		m_combos[POS_X]->SetTxt( X_colla );
		m_combos[POS_Y]->SetTxt( Y_colla );
	}

	void onEdit()
	{
		X_colla = m_combos[POS_X]->GetFloat();
		Y_colla = m_combos[POS_Y]->GetFloat();

		if( colla_test )
		{
			CC_Dispenser.X_colla = X_colla;
			CC_Dispenser.Y_colla = Y_colla;

			Dosatore->WriteConfig(PColla_ndisp,CC_Dispenser,QHeader.CurDosaConfig[PColla_ndisp-1]);
			Dosatore->ReadCurConfig();
		}
		else
		{
			#ifndef __DISP2
			Save_pcolla(X_colla, Y_colla);
			#else
			Save_pcolla(PColla_ndisp,X_colla, Y_colla);
			#endif
		}
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00698), K_F3, 0, NULL, Col_auto );
		m_menu->Add( MsgGetString(Msg_00708), K_F4, 0, NULL, Colla_test );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				Col_auto();
				return true;

			case K_F4:
				Colla_test();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
	}
};

int fn_DispenserPointsPosition( CWindow* parent, int ndisp, int test_point )
{
	#ifndef __DISP2
	PColla_ndisp = 1;
	#else
	PColla_ndisp = ndisp;
	#endif

	if(!Dosatore->IsConfLoaded(PColla_ndisp))
	{
		W_Mess( MsgGetString(Msg_01746) );
		return 0;
	}

	colla_test = test_point;

	if( colla_test )
	{
		//punto colla in parametri dosatore

		Dosatore->ReadCurConfig(); //set di dati correnti dosatore
		Dosatore->GetConfig(PColla_ndisp,CC_Dispenser);

		X_colla = CC_Dispenser.X_colla;
		Y_colla = CC_Dispenser.Y_colla;
	}
	else
	{
		//punto colla programma di assemblaggio
		#ifndef __DISP2
		Read_pcolla(X_colla, Y_colla);
		#else
		Read_pcolla(PColla_ndisp,X_colla, Y_colla);
		#endif
	}

	DispenserPointsPositionUI win( parent, ndisp );
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Feeders default position
//---------------------------------------------------------------------------
const char* DispType_StrVect[] = { MsgGetString(Msg_01726), MsgGetString(Msg_01727) };
extern void DosaSet_PGUp();
extern void DosaSet_PGDown();
extern void Dosa_DelRec();

extern int DosaSet_ndisp;
extern int DosaSet_CurRec;
extern int DosaSet_NewRec;

class DispenserParamsUI : public CWindowParams
{
public:
	DispenserParamsUI( CWindow* parent, int ndisp ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 7 );
		SetClientAreaSize( 80, 20 );

		#ifndef __DISP2
		SetTitle( MsgGetString(Msg_00125) );
		#else
		char buf[80];
		snprintf( buf, sizeof(buf), "%s %d", MsgGetString(Msg_00125), ndisp );
		SetTitle( buf );
		#endif
	}

	typedef enum
	{
		NAME,
		DIS_TYPE,
		NOTES,
		XY_SPEED,
		VISCOSITY,
		POINTS,
		ACT_TIME,
		OFFSET,
		STOP_TIME,
		DN_TIME,
		UP_TIME,
		POST_TIME,
		PINV_TIME,
		INV_TIME,
		#ifdef __DISP2
		IMP_TIME,
		PULS_TIME,
		WAIT_TIME
		#endif
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[NAME]     = new C_Combo( 2, 1, MsgGetString(Msg_01734), 8, CELL_TYPE_TEXT );
		#ifndef __DISP2
		m_combos[DIS_TYPE] = new C_Combo( 2, 2, MsgGetString(Msg_01724), 18, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		#else
		if( DosaSet_ndisp == VOLUMETRIC_NDISPENSER )
		{
			//sul dispenser 2 possono essere collegati sia il dispenser tempo pressione che quello volumetrico
			m_combos[DIS_TYPE] = new C_Combo( 2, 2, MsgGetString(Msg_01724), 18, CELL_TYPE_TEXT );
		}
		else
		{
			//sul dispenser 1 puo' essere montato solo il dispenser tempo-pressione
			m_combos[DIS_TYPE] = new C_Combo( 2, 2, MsgGetString(Msg_01724), 18, CELL_TYPE_TEXT, CELL_STYLE_NOSEL | CELL_STYLE_READONLY );
		}
		#endif

		m_combos[NOTES]     = new C_Combo( 2, 3, MsgGetString(Msg_01725), 18, CELL_TYPE_TEXT );
		m_combos[XY_SPEED]  = new C_Combo( 2, 5, MsgGetString(Msg_01186),  8, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[VISCOSITY] = new C_Combo( 2, 7, MsgGetString(Msg_00696), 5, CELL_TYPE_UINT );
		m_combos[POINTS]    = new C_Combo( 2, 9, MsgGetString(Msg_00706), 5, CELL_TYPE_UINT );
		m_combos[ACT_TIME]  = new C_Combo( 2, 10, MsgGetString(Msg_00654), 5, CELL_TYPE_UINT );
		m_combos[OFFSET]    = new C_Combo( 2, 11, MsgGetString(Msg_05166), 5, CELL_TYPE_UINT );
		m_combos[STOP_TIME] = new C_Combo( 42,  5, MsgGetString(Msg_00653), 5, CELL_TYPE_UINT );
		m_combos[DN_TIME]   = new C_Combo( 42,  6, MsgGetString(Msg_00667), 5, CELL_TYPE_UINT );
		m_combos[UP_TIME]   = new C_Combo( 42,  7, MsgGetString(Msg_01788), 5, CELL_TYPE_UINT );
		#ifndef __LINEDISP
		m_combos[POST_TIME] = new C_Combo( 42,  8, MsgGetString(Msg_00666), 5, CELL_TYPE_UINT );
		#else
		m_combos[POST_TIME] = new C_Combo( 42,  8, MsgGetString(Msg_01862), 5, CELL_TYPE_UINT );
		#endif

		#ifndef __DISP2
		m_combos[PINV_TIME] = new C_Combo( 42,  9, MsgGetString(Msg_01723), 5, CELL_TYPE_UINT );
		#else
		m_combos[PINV_TIME] = new C_Combo( 42,  9, MsgGetString(Msg_05128), 5, CELL_TYPE_UINT );
		#endif

		m_combos[INV_TIME]  = new C_Combo( 42, 10, MsgGetString(Msg_01722), 5, CELL_TYPE_UINT );
		#ifdef __DISP2
		m_combos[IMP_TIME]  = new C_Combo( 42, 11, MsgGetString(Msg_05123), 5, CELL_TYPE_UINT );
		m_combos[PULS_TIME] = new C_Combo( 42, 12, MsgGetString(Msg_05129), 5, CELL_TYPE_UINT );
		m_combos[WAIT_TIME] = new C_Combo( 42, 13, MsgGetString(Msg_05130), 5, CELL_TYPE_UINT );
		#endif

		// set params
		m_combos[DIS_TYPE]->SetLegalStrings( 2, (char**)DispType_StrVect );
		m_combos[XY_SPEED]->SetLegalStrings( 3, (char**)Speeds_StrVect );
		m_combos[VISCOSITY]->SetVMinMax( 10, 999 );
		m_combos[POINTS]->SetVMinMax( 0, 15 );
		m_combos[ACT_TIME]->SetVMinMax( 0, 999 );
		m_combos[OFFSET]->SetVMinMax( 0, 5 );
		m_combos[STOP_TIME]->SetVMinMax( 2, 999 );
		m_combos[DN_TIME]->SetVMinMax( 0, 999 );
		m_combos[UP_TIME]->SetVMinMax( 0, 999 );
		m_combos[POST_TIME]->SetVMinMax( 0, 999 );
		m_combos[PINV_TIME]->SetVMinMax( 0, 999 );
		m_combos[INV_TIME]->SetVMinMax( 0, 999 );
		#ifdef __DISP2
		m_combos[IMP_TIME]->SetVMinMax( 0, 999 );
		m_combos[PULS_TIME]->SetVMinMax( 0, 999 );
		m_combos[WAIT_TIME]->SetVMinMax( 0, 999 );
		#endif

		// add to combo list
		m_comboList->Add( m_combos[NAME]     , 0, 0 );
		m_comboList->Add( m_combos[DIS_TYPE] , 1, 0 );
		m_comboList->Add( m_combos[NOTES]    , 2, 0 );
		m_comboList->Add( m_combos[XY_SPEED] , 3, 0 );
		m_comboList->Add( m_combos[VISCOSITY], 5, 0 );

		m_comboList->Add( m_combos[POINTS]   , 7, 0 );
		m_comboList->Add( m_combos[ACT_TIME] , 8, 0 );
		m_comboList->Add( m_combos[OFFSET]   , 9, 0 );

		m_comboList->Add( m_combos[STOP_TIME], 3, 1 );
		m_comboList->Add( m_combos[DN_TIME]  , 4, 1 );
		m_comboList->Add( m_combos[UP_TIME]  , 5, 1 );
		m_comboList->Add( m_combos[POST_TIME], 6, 1 );
		m_comboList->Add( m_combos[PINV_TIME], 7, 1 );
		m_comboList->Add( m_combos[INV_TIME] , 8, 1 );

		#ifdef __DISP2
		m_comboList->Add( m_combos[IMP_TIME] , 9, 1 );
		m_comboList->Add( m_combos[PULS_TIME], 10, 1 );
		m_comboList->Add( m_combos[WAIT_TIME], 11, 1 );
		#endif
	}

	void onShow()
	{
		DrawPanel( RectI( 3, 14, GetW()/GUI_CharW() - 6, 5 ) );
		DrawTextCentered( 15, MsgGetString(Msg_01728), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
		DrawTextCentered( 16, MsgGetString(Msg_01729), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
		DrawTextCentered( 17, MsgGetString(Msg_01730), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	}

	void onRefresh()
	{
		#ifdef __DISP2
		if( DosaSet_ndisp != VOLUMETRIC_NDISPENSER )
		{
			CC_Dispenser.tipoDosat = 0;
		}
		#endif

		m_combos[NAME]->SetTxt( CC_Dispenser.name );

		#ifndef __DISP2
		m_combos[DIS_TYPE]->SetStrings_Pos( CC_Dispenser.tipoDosat );
		#else
		if( DosaSet_ndisp == VOLUMETRIC_NDISPENSER )
		{
			m_combos[DIS_TYPE]->SetStrings_Pos( CC_Dispenser.tipoDosat );
		}
		else
		{
			m_combos[DIS_TYPE]->SetTxt( MsgGetString(Msg_01726) );
		}
		#endif

		m_combos[NOTES]->SetTxt( CC_Dispenser.Note );
		m_combos[XY_SPEED]->SetStrings_Pos( CC_Dispenser.speedIndex );
		m_combos[VISCOSITY]->SetTxt( CC_Dispenser.Viscosity );
		m_combos[POINTS]->SetTxt( int(CC_Dispenser.NPoint) );
		m_combos[ACT_TIME]->SetTxt( CC_Dispenser.GlueOut_Time );
		m_combos[OFFSET]->SetTxt( CC_Dispenser.TestPointsOffset );

		m_combos[STOP_TIME]->SetTxt( CC_Dispenser.DosaMov_Time );
		m_combos[DN_TIME]->SetTxt( CC_Dispenser.DosaZMovDown_Time );
		m_combos[UP_TIME]->SetTxt( CC_Dispenser.DosaZMovUp_Time );
		m_combos[POST_TIME]->SetTxt( CC_Dispenser.GlueEnd_Time );
		#ifndef __DISP2
		m_combos[PINV_TIME]->SetTxt( CC_Dispenser.PreInversion_Time );
		#else
		m_combos[PINV_TIME]->SetTxt( CC_Dispenser.AntiDropStart_Time );
		#endif
		m_combos[INV_TIME]->SetTxt( CC_Dispenser.Inversion_Time );

		#ifdef __DISP2
		m_combos[IMP_TIME]->SetTxt( CC_Dispenser.VacuoPulse_Time );
		m_combos[PULS_TIME]->SetTxt( CC_Dispenser.VacuoPulseFinal_Time );
		m_combos[WAIT_TIME]->SetTxt( CC_Dispenser.VacuoWait_Time );
		#endif

		#ifndef __DISP2
		if( CC_Dispenser.tipoDosat )
		{
			m_combos[PINV_TIME]->SetStyle( m_combos[PINV_TIME]->GetStyle() & ~CELL_STYLE_READONLY );
			m_combos[INV_TIME]->SetStyle( m_combos[INV_TIME]->GetStyle() & ~CELL_STYLE_READONLY );
		}
		else
		{
			m_combos[PINV_TIME]->SetStyle( m_combos[PINV_TIME]->GetStyle() | CELL_STYLE_READONLY );
			m_combos[INV_TIME]->SetStyle( m_combos[INV_TIME]->GetStyle() | CELL_STYLE_READONLY );
		}
		#else
		if(CC_Dispenser.tipoDosat)
		{
			m_combos[INV_TIME]->SetStyle( m_combos[PINV_TIME]->GetStyle() & ~CELL_STYLE_READONLY );
		}
		else
		{
			m_combos[INV_TIME]->SetStyle( m_combos[PINV_TIME]->GetStyle() | CELL_STYLE_READONLY );
		}
		#endif
	}

	void onEdit()
	{
		char buf[20];
		snprintf( buf, sizeof(buf), "%s", m_combos[NAME]->GetTxt() );
		DelSpcR(buf);
		if( strlen(buf) == 0 )
		{
			W_Mess( MsgGetString(Msg_00569) );
			return;
		}

		snprintf( CC_Dispenser.name, sizeof(CC_Dispenser.name), "%s", buf );

		#ifndef __DISP2
		CC_Dispenser.tipoDosat = m_combos[DIS_TYPE]->GetStrings_Pos();
		#else
		if( DosaSet_ndisp == VOLUMETRIC_NDISPENSER )
		{
			CC_Dispenser.tipoDosat = m_combos[DIS_TYPE]->GetStrings_Pos();
		}
		else
		{
			CC_Dispenser.tipoDosat = 0;
		}
		#endif

		snprintf( CC_Dispenser.Note, sizeof(CC_Dispenser.Note), "%s", m_combos[NOTES]->GetTxt() );

		CC_Dispenser.speedIndex = m_combos[XY_SPEED]->GetStrings_Pos();
		CC_Dispenser.Viscosity = m_combos[VISCOSITY]->GetInt();
		CC_Dispenser.NPoint = m_combos[POINTS]->GetInt();
		CC_Dispenser.GlueOut_Time = m_combos[ACT_TIME]->GetInt();
		CC_Dispenser.TestPointsOffset = m_combos[OFFSET]->GetInt();
		CC_Dispenser.DosaMov_Time = m_combos[STOP_TIME]->GetInt();
		CC_Dispenser.DosaZMovDown_Time = m_combos[DN_TIME]->GetInt();
		CC_Dispenser.DosaZMovUp_Time = m_combos[UP_TIME]->GetInt();
		CC_Dispenser.GlueEnd_Time = m_combos[POST_TIME]->GetInt();

		#ifndef __DISP2
		CC_Dispenser.PreInversion_Time = m_combos[PINV_TIME]->GetInt();
		#else
		CC_Dispenser.AntiDropStart_Time = m_combos[PINV_TIME]->GetInt();
		#endif

		if( CC_Dispenser.tipoDosat )
		{
			#ifndef __DISP2
			CC_Dispenser.PreInversion_Time = m_combos[PINV_TIME]->GetInt();
			#else
			CC_Dispenser.AntiDropStart_Time = m_combos[PINV_TIME]->GetInt();
			#endif
			CC_Dispenser.Inversion_Time = m_combos[INV_TIME]->GetInt();
		}

		#ifdef __DISP2
		CC_Dispenser.VacuoPulse_Time = m_combos[IMP_TIME]->GetInt();
		CC_Dispenser.VacuoPulseFinal_Time = m_combos[PULS_TIME]->GetInt();
		CC_Dispenser.VacuoWait_Time = m_combos[WAIT_TIME]->GetInt();
		#endif

		Dosatore->WriteConfig(DosaSet_ndisp,CC_Dispenser,DosaSet_CurRec);

		if(DosaSet_NewRec)
		{
			DosaSet_NewRec=0;
		}
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00707), K_F3, 0, NULL, boost::bind( &DispenserParamsUI::onDispensePoints, this ) ); // Punti test dosaggio
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				onDispensePoints();
				return true;

			case K_PAGEUP:
				DosaSet_PGUp();
				return true;

			case K_PAGEDOWN:
				DosaSet_PGDown();
				return true;

			case K_DEL:
				if( guiDeskTop->GetEditMode() )
				{
					Dosa_DelRec();
					return true;
				}
				break;

			case K_ENTER:
				if( DosaSet_NewRec )
				{
					W_Mess( MsgGetString(Msg_00569) );
					return true;
				}

				QHeader.CurDosaConfig[DosaSet_ndisp-1] = DosaSet_CurRec;
				Mod_Cfg( QHeader );

				forceExit();
				return true;

			default:
				break;
		}

		return false;
	}

private:
	int onDispensePoints()
	{
		return fn_DispenserPointsPosition( this, DosaSet_ndisp, PCOLLA_TESTPOINT );
	}
};

int fn_DispenserParams( CWindow* parent, int ndisp )
{
	#ifndef __DISP2
	DosaSet_ndisp = 1;
	#else
	DosaSet_ndisp = ndisp;
	#endif

	if(Dosatore->GetConfigNRecs(DosaSet_ndisp)==0)
	{
		DosaSet_CurRec=0;
		DosaSet_NewRec=1;

		memset(&CC_Dispenser,(char)0,sizeof(CC_Dispenser));
	}
	else
	{
		DosaSet_CurRec=QHeader.CurDosaConfig[DosaSet_ndisp-1];
		DosaSet_NewRec=0;

		Dosatore->ReadConfig(DosaSet_ndisp,CC_Dispenser,DosaSet_CurRec);
	}

	DispenserParamsUI win( parent, ndisp );
	win.Show();
	win.Hide();

	Dosatore->ReadCurConfig();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Dispenser offset
//---------------------------------------------------------------------------
extern int DosaOffs_ndisp;
extern int Odos_auto();

class DispenserOffsetUI : public CWindowParams
{
public:
	DispenserOffsetUI( CWindow* parent, int ndisp ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 46, 5 );

		#ifdef __DISP2
		char buf[80];
		snprintf( buf, 80, "%s - %d", MsgGetString(Msg_00660), ndisp );
		SetTitle( buf );
		#else
		SetTitle( MsgGetString(Msg_00656) );
		#endif
	}

	typedef enum
	{
		POS_X,
		POS_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[POS_X] = new C_Combo(  6, 2, "X :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Y] = new C_Combo( 25, 2, "Y :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[POS_X]->SetVMinMax( XTEL1_MIN, XTEL1_MAX );
		m_combos[POS_Y]->SetVMinMax( XTEL1_MIN, XTEL1_MAX );

		// add to combo list
		m_comboList->Add( m_combos[POS_X], 0, 0 );
		m_comboList->Add( m_combos[POS_Y], 0, 1 );
	}

	void onShow()
	{
	}

	void onRefresh()
	{
		m_combos[POS_X]->SetTxt( CC_Dispenser.CamOffset_X );
		m_combos[POS_Y]->SetTxt( CC_Dispenser.CamOffset_Y );
	}

	void onEdit()
	{
		CC_Dispenser.CamOffset_X = m_combos[POS_X]->GetFloat();
		CC_Dispenser.CamOffset_Y = m_combos[POS_Y]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00447), K_F3, 0, NULL, Odos_auto );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				Odos_auto();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
	}
};

int fn_DispenserOffset( CWindow* parent, int ndisp )
{
	DosaOffs_ndisp = ndisp;

	Dosatore->GetConfig( ndisp, CC_Dispenser );

	DispenserOffsetUI win( parent, ndisp );
	win.Show();
	win.Hide();

	Dosatore->WriteConfig( ndisp, CC_Dispenser, QHeader.CurDosaConfig[ndisp-1] );
	Dosatore->ReadCurConfig();
	return 1;
}



//---------------------------------------------------------------------------
// finestra: Centering camera parameters
//---------------------------------------------------------------------------
extern int AuxCam_ParamImg();
extern int AuxCam_DeleteParamImg();
int AuxCam_App1() { return AuxCam_App(1); }
int AuxCam_App2() { return AuxCam_App(2); }
int AuxCam_Scale1() { return ExtCam_Scale(1); }
int AuxCam_Scale2() { return ExtCam_Scale(2); }

class CenteringCameraParamsUI : public CWindowParams
{
public:
	CenteringCameraParamsUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 50, 10 );
		SetTitle( MsgGetString(Msg_00425) );

		SM_PosTeaching = new GUI_SubMenu();
		SM_PosTeaching->Add( MsgGetString(Msg_00042), K_F1, 0, NULL, AuxCam_App1 ); // punta 1
		SM_PosTeaching->Add( MsgGetString(Msg_00043), K_F2, 0, NULL, AuxCam_App2 ); // punta 2

		SM_ScaleCalib = new GUI_SubMenu();
		SM_ScaleCalib->Add( MsgGetString(Msg_00042), K_F1, 0, NULL, AuxCam_Scale1 ); // punta 1
		SM_ScaleCalib->Add( MsgGetString(Msg_00043), K_F2, 0, NULL, AuxCam_Scale2 ); // punta 2
	}

	~CenteringCameraParamsUI()
	{
		delete SM_PosTeaching;
		delete SM_ScaleCalib;
	}

	typedef enum
	{
		POS_X_1,
		POS_Y_1,
		POS_Z_1,
		SCALE_X_1,
		SCALE_Y_1,
		POS_X_2,
		POS_Y_2,
		POS_Z_2,
		SCALE_X_2,
		SCALE_Y_2
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[POS_X_1] =   new C_Combo( 15, 2, "X (mm) :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_X_2] =   new C_Combo( 35, 2, "", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Y_1] =   new C_Combo( 15, 3, "Y (mm) :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Y_2] =   new C_Combo( 35, 3, "", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Z_1] =   new C_Combo( 15, 4, "Z (mm) :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Z_2] =   new C_Combo( 35, 4, "", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[SCALE_X_1] = new C_Combo(  3, 6, MsgGetString(Msg_01600), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 6 );
		m_combos[SCALE_X_2] = new C_Combo( 35, 6, "", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 6 );
		m_combos[SCALE_Y_1] = new C_Combo(  3, 7, MsgGetString(Msg_01601), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 6 );
		m_combos[SCALE_Y_2] = new C_Combo( 35, 7, "", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 6 );

		// set params
		m_combos[POS_X_1]->SetVMinMax( LXCAMPO_MIN, LXCAMPO_MAX );
		m_combos[POS_X_2]->SetVMinMax( LXCAMPO_MIN, LXCAMPO_MAX );
		m_combos[POS_Y_1]->SetVMinMax( LYCAMPO_MIN, LYCAMPO_MAX );
		m_combos[POS_Y_2]->SetVMinMax( LYCAMPO_MIN, LYCAMPO_MAX );
		m_combos[POS_Z_1]->SetVMinMax( EXTCAM_ZMIN, EXTCAM_ZMAX );
		m_combos[POS_Z_2]->SetVMinMax( EXTCAM_ZMIN, EXTCAM_ZMAX );
		m_combos[SCALE_X_1]->SetVMinMax( (float)(EXTCAM_MMPIX_MIN), (float)(EXTCAM_MMPIX_MAX) );
		m_combos[SCALE_X_2]->SetVMinMax( (float)(EXTCAM_MMPIX_MIN), (float)(EXTCAM_MMPIX_MAX) );
		m_combos[SCALE_Y_1]->SetVMinMax( (float)(EXTCAM_MMPIX_MIN), (float)(EXTCAM_MMPIX_MAX) );
		m_combos[SCALE_Y_2]->SetVMinMax( (float)(EXTCAM_MMPIX_MIN), (float)(EXTCAM_MMPIX_MAX) );

		// add to combo list
		m_comboList->Add( m_combos[POS_X_1], 0, 0 );
		m_comboList->Add( m_combos[POS_X_2], 0, 1 );
		m_comboList->Add( m_combos[POS_Y_1], 1, 0 );
		m_comboList->Add( m_combos[POS_Y_2], 1, 1 );
		m_comboList->Add( m_combos[POS_Z_1], 2, 0 );
		m_comboList->Add( m_combos[POS_Z_2], 2, 1 );
		m_comboList->Add( m_combos[SCALE_X_1], 3, 0 );
		m_comboList->Add( m_combos[SCALE_X_2], 3, 1 );
		m_comboList->Add( m_combos[SCALE_Y_1], 4, 0 );
		m_comboList->Add( m_combos[SCALE_Y_2], 4, 1 );
	}

	void onShow()
	{
		DrawText( 24, 1, MsgGetString(Msg_00042) );
		DrawText( 36, 1, MsgGetString(Msg_00043) );
	}

	void onRefresh()
	{
		m_combos[POS_X_1]->SetTxt( QParam.AuxCam_X[0] );
		m_combos[POS_Y_1]->SetTxt( QParam.AuxCam_Y[0] );
		m_combos[POS_Z_1]->SetTxt( QParam.AuxCam_Z[0] );
		m_combos[SCALE_X_1]->SetTxt( QParam.AuxCam_Scale_x[0] );
		m_combos[SCALE_Y_1]->SetTxt( QParam.AuxCam_Scale_y[0] );
		m_combos[POS_X_2]->SetTxt( QParam.AuxCam_X[1] );
		m_combos[POS_Y_2]->SetTxt( QParam.AuxCam_Y[1] );
		m_combos[POS_Z_2]->SetTxt( QParam.AuxCam_Z[1] );
		m_combos[SCALE_X_2]->SetTxt( QParam.AuxCam_Scale_x[1] );
		m_combos[SCALE_Y_2]->SetTxt( QParam.AuxCam_Scale_y[1] );
	}

	void onEdit()
	{
		QParam.AuxCam_X[0] = m_combos[POS_X_1]->GetFloat();
		QParam.AuxCam_Y[0] = m_combos[POS_Y_1]->GetFloat();
		QParam.AuxCam_Z[0] = m_combos[POS_Z_1]->GetFloat();
		QParam.AuxCam_Scale_x[0] = m_combos[SCALE_X_1]->GetFloat();
		QParam.AuxCam_Scale_y[0] = m_combos[SCALE_Y_1]->GetFloat();
		QParam.AuxCam_X[1] = m_combos[POS_X_2]->GetFloat();
		QParam.AuxCam_Y[1] = m_combos[POS_Y_2]->GetFloat();
		QParam.AuxCam_Z[1] = m_combos[POS_Z_2]->GetFloat();
		QParam.AuxCam_Scale_x[1] = m_combos[SCALE_X_2]->GetFloat();
		QParam.AuxCam_Scale_y[1] = m_combos[SCALE_Y_2]->GetFloat();
	}

	void onShowMenu()
	{
		#ifdef __SNIPER
		m_menu->Add( MsgGetString(Msg_01361), K_F3, 0, SM_PosTeaching, NULL );        // app. posizione
		m_menu->Add( MsgGetString(Msg_01251), K_F4, 0, SM_ScaleCalib, NULL );         // scale calibration
		#endif
		m_menu->Add( MsgGetString(Msg_05158), K_F8, 0, NULL, AuxCam_ParamImg );       // nozzle image params
		m_menu->Add( MsgGetString(Msg_05167), K_F9, 0, NULL, AuxCam_DeleteParamImg ); // delete nozzle image
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				SM_PosTeaching->Show();
				return true;

			case K_F4:
				SM_ScaleCalib->Show();
				return true;

			case K_F8:
				AuxCam_ParamImg();
				return true;

			case K_F9:
				AuxCam_DeleteParamImg();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Mod_Par( QParam );
	}

	GUI_SubMenu* SM_PosTeaching;
	GUI_SubMenu* SM_ScaleCalib;
};

int fn_CenteringCameraParams()
{
	CenteringCameraParamsUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Vision parameters
//---------------------------------------------------------------------------
extern int HeadCam_Scale();

class VisionParamsUI : public CWindowParams
{
public:
	VisionParamsUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 75, 10 );
		SetTitle( MsgGetString(Msg_00756) );
	}

	typedef enum
	{
		VIS_SPEED,
		MATCH_SPEED,
		MATCH_ERROR,
		STOP_TIME,
		IMAGE_TIME,
		MMPIX_X_CAM1,
		MMPIX_Y_CAM1,
		OFFSET_SCALE,
		VIS_DEBUG
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[VIS_SPEED]   = new C_Combo( 3, 1, MsgGetString(Msg_00785), 8, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[MATCH_SPEED] = new C_Combo( 3, 2, MsgGetString(Msg_00786), 8, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[MATCH_ERROR] = new C_Combo( 3, 4, MsgGetString(Msg_00789), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[STOP_TIME]   = new C_Combo( 3, 6, MsgGetString(Msg_00791), 8, CELL_TYPE_UINT );
		m_combos[IMAGE_TIME]  = new C_Combo( 3, 7, MsgGetString(Msg_00792), 8, CELL_TYPE_UINT );
		
		m_combos[MMPIX_X_CAM1] = new C_Combo( 43, 1, MsgGetString(Msg_01600), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 5 );
		m_combos[MMPIX_Y_CAM1] = new C_Combo( 43, 2, MsgGetString(Msg_01601), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 5 );
		m_combos[OFFSET_SCALE] = new C_Combo( 41, 4, MsgGetString(Msg_00795), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[VIS_DEBUG]    = new C_Combo( 41, 7, MsgGetString(Msg_00796), 8, CELL_TYPE_UINT );
		
		// set params
		m_combos[VIS_SPEED]->SetLegalStrings( 3, (char**)Speeds_StrVect );
		m_combos[MATCH_SPEED]->SetLegalStrings( 3, (char**)Speeds_StrVect );
		m_combos[MATCH_ERROR]->SetVMinMax( 0.1f, 5.f );
		m_combos[STOP_TIME]->SetVMinMax( 10, 1000 );
		m_combos[IMAGE_TIME]->SetVMinMax( 10, 1000 );
		m_combos[MMPIX_X_CAM1]->SetVMinMax( 0.01f, 0.9f );
		m_combos[MMPIX_Y_CAM1]->SetVMinMax( 0.01f, 0.9f );
		m_combos[OFFSET_SCALE]->SetVMinMax( 0.01f, 5.f );
		m_combos[VIS_DEBUG]->SetVMinMax( 0, 60000 );
		
		// add to combo list
		m_comboList->Add( m_combos[VIS_SPEED],   0, 0 );
		m_comboList->Add( m_combos[MATCH_SPEED], 1, 0 );
		m_comboList->Add( m_combos[MATCH_ERROR], 3, 0 );
		m_comboList->Add( m_combos[STOP_TIME],   5, 0 );
		m_comboList->Add( m_combos[IMAGE_TIME],  6, 0 );
		m_comboList->Add( m_combos[MMPIX_X_CAM1],   0, 1 );
		m_comboList->Add( m_combos[MMPIX_Y_CAM1],   1, 1 );
		m_comboList->Add( m_combos[OFFSET_SCALE],   3, 1 );
		m_comboList->Add( m_combos[VIS_DEBUG],      6, 1 );
	}

	void onRefresh()
	{
		m_combos[VIS_SPEED]->SetStrings_Pos( Vision.visionSpeedIndex );
		m_combos[MATCH_SPEED]->SetStrings_Pos( Vision.matchSpeedIndex );
		m_combos[MATCH_ERROR]->SetTxt( Vision.match_err );
		m_combos[STOP_TIME]->SetTxt( Vision.wait_time );
		m_combos[IMAGE_TIME]->SetTxt( Vision.image_time );
		m_combos[MMPIX_X_CAM1]->SetTxt( Vision.mmpix_x );
		m_combos[MMPIX_Y_CAM1]->SetTxt( Vision.mmpix_y );
		m_combos[OFFSET_SCALE]->SetTxt( Vision.scale_off );
		m_combos[VIS_DEBUG]->SetTxt( Vision.debug );
	}

	void onEdit()
	{
		Vision.visionSpeedIndex = m_combos[VIS_SPEED]->GetStrings_Pos();
		Vision.matchSpeedIndex = m_combos[MATCH_SPEED]->GetStrings_Pos();
		Vision.match_err = m_combos[MATCH_ERROR]->GetFloat();
		Vision.wait_time = m_combos[STOP_TIME]->GetInt();
		Vision.image_time = m_combos[IMAGE_TIME]->GetInt();
		Vision.mmpix_x = m_combos[MMPIX_X_CAM1]->GetFloat();
		Vision.mmpix_y = m_combos[MMPIX_Y_CAM1]->GetFloat();
		Vision.scale_off = m_combos[OFFSET_SCALE]->GetFloat();
		Vision.debug = m_combos[VIS_DEBUG]->GetInt();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01251), K_F3, 0, NULL, HeadCam_Scale ); // calibrazione scala telecamera
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				HeadCam_Scale();
				return true;
			
			default:
				break;
		}
	
		return false;
	}

	void onClose()
	{
		VisDataSave( Vision );
	}
};

int fn_VisionParams()
{
	VisionParamsUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Warm-up parameters
//---------------------------------------------------------------------------
class WarmUpParamsUI : public CWindowParams
{
public:
	WarmUpParamsUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 50, 11 );
		SetTitle( MsgGetString(Msg_01902) );
	}

	typedef enum
	{
		MAC_TEMP,
		THR_TEMP,
		AXES_ACC,
		DURATION,
		ENABLE_WU
	} combo_labels;

protected:
	void onInit()
	{
		WarmUpParams_Open();
		WarmUpParams_Read( data );
		
		// create combos
		m_combos[MAC_TEMP]  = new C_Combo( 5, 1, MsgGetString(Msg_01897), 6, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[THR_TEMP]  = new C_Combo( 5, 2, MsgGetString(Msg_01898), 6, CELL_TYPE_UINT );
		m_combos[AXES_ACC]  = new C_Combo( 5, 4, MsgGetString(Msg_01899), 6, CELL_TYPE_UINT );
		m_combos[DURATION]  = new C_Combo( 5, 6, MsgGetString(Msg_01900), 6, CELL_TYPE_UINT );
		m_combos[ENABLE_WU] = new C_Combo( 5, 8, MsgGetString(Msg_01901), 4, CELL_TYPE_YN );
		
		// set params
		m_combos[MAC_TEMP]->SetVMinMax( -40, 40 );
		m_combos[THR_TEMP]->SetVMinMax( 5, 30 );
		m_combos[AXES_ACC]->SetVMinMax( 100, QHeader.xyMaxAcc );
		m_combos[DURATION]->SetVMinMax( 1, 15 );
	
		// add to combo list
		m_comboList->Add( m_combos[MAC_TEMP],  0, 0 );
		m_comboList->Add( m_combos[THR_TEMP],  1, 0 );
		m_comboList->Add( m_combos[AXES_ACC],  2, 0 );
		m_comboList->Add( m_combos[DURATION],  3, 0 );
		m_comboList->Add( m_combos[ENABLE_WU], 4, 0 );
		
		n = 0;
		temp = 0.f;
	}

	void onRefresh()
	{
		m_combos[THR_TEMP]->SetTxt( data.threshold );
		m_combos[AXES_ACC]->SetTxt( data.acceleration );
		m_combos[DURATION]->SetTxt( data.duration );
		m_combos[ENABLE_WU]->SetTxtYN( data.enable );
	}

	void onEdit()
	{
		data.threshold = m_combos[THR_TEMP]->GetInt();
		data.acceleration = m_combos[AXES_ACC]->GetInt();
		data.duration = m_combos[DURATION]->GetInt();
		data.enable = m_combos[ENABLE_WU]->GetYN();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01896), K_F3, 0, NULL, boost::bind( &WarmUpParamsUI::onDoWarmup, this ) );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				onDoWarmup();
				return true;
		
			default:
				break;
		}

		return false;
	}

	void onIdle()
	{
		if( n == 10 )
		{
			m_combos[MAC_TEMP]->SetTxt( temp/n );
			n = 0;
			temp = 0.f;
		}
		else
		{
			float _temp;
			GetMachineTemperature( _temp );
			temp += _temp;
			n++;
		}
	}

	void onClose()
	{
		WarmUpParams_Write( data );
		WarmUpParams_Close();
	}

	int onDoWarmup()
	{
		WarmUpParams_Write( data );
		return DoWarmUpCycle( this );
	}

	int n;
	float temp;
	char cpubuf[40];
	struct WarmUpParams data;
};

int fn_WarmUpParams()
{
	WarmUpParamsUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Machine info
//---------------------------------------------------------------------------
extern GUI_DeskTop* guiDeskTop;
extern SMachineInfo MachineInfo;

class MachineInfoUI : public CWindowParams
{
public:
	MachineInfoUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
		SetClientAreaSize( 53, 12 );
		SetTitle( MsgGetString(Msg_00089) );
	}

	typedef enum
	{
		WORK_TIME,
		X_MOVE,
		Y_MOVE,
		COM_MHEAD,
		COM_MHEAD_ERR,
		COM_SNIPER,
		COM_SNIPER_ERR
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[WORK_TIME]      = new C_Combo(  5,  1, MsgGetString(Msg_00234), 7, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[X_MOVE]         = new C_Combo(  5,  5, "X (Km):", 9, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 1 );
		m_combos[Y_MOVE]         = new C_Combo( 30,  5, "Y (Km):", 9, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 1 );
		m_combos[COM_MHEAD]      = new C_Combo(  5,  9, "Motorhead (%):", 9, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 6 );
		m_combos[COM_MHEAD_ERR]  = new C_Combo( 31,  9, "(tot):", 9, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[COM_SNIPER]     = new C_Combo(  5, 10, "Sniper    (%):", 9, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 6 );
		m_combos[COM_SNIPER_ERR] = new C_Combo( 31, 10, "(tot):", 9, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );

		// add to combo list
		m_comboList->Add( m_combos[WORK_TIME]     , 0, 0 );
		m_comboList->Add( m_combos[X_MOVE]        , 1, 0 );
		m_comboList->Add( m_combos[Y_MOVE]        , 1, 1 );
		m_comboList->Add( m_combos[COM_MHEAD]     , 2, 0 );
		m_comboList->Add( m_combos[COM_MHEAD_ERR] , 2, 1 );
		m_comboList->Add( m_combos[COM_SNIPER]    , 3, 0 );
		m_comboList->Add( m_combos[COM_SNIPER_ERR], 3, 1 );
	}

	void onShow()
	{
		DrawSubTitle( 3, MsgGetString(Msg_00429) ); // Movimento assi
		DrawSubTitle( 7, "Errori di comunicazione" );
	}

	void onRefresh()
	{
		m_combos[WORK_TIME]->SetTxt( int(MachineInfo.WorkTime/3600.f) );
		m_combos[X_MOVE]->SetTxt( float(MachineInfo.XMovement/1000.f) );
		m_combos[Y_MOVE]->SetTxt( float(MachineInfo.YMovement/1000.f) );
		if( ComPortMotorhead )
		{
			m_combos[COM_MHEAD]->SetTxt( float(ComPortMotorhead->GetPerformanceIndex()) );
			m_combos[COM_MHEAD_ERR]->SetTxt( int(ComPortMotorhead->GetFailures()) );
		}
		if( ComPortSniper )
		{
			m_combos[COM_SNIPER]->SetTxt( float(ComPortSniper->GetPerformanceIndex()) );
			m_combos[COM_SNIPER_ERR]->SetTxt( int(ComPortSniper->GetFailures()) );
		}
	}
};

int fn_MachineInfo()
{
	MachineInfoUI win;
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Machine ID
//---------------------------------------------------------------------------
int fn_MachineID()
{
	char tmp[NETID_TXT_LEN];
	
	int err;
	
	// visualizza le barre di stato e "pulisce il desktop"
	guiDeskTop->ShowStatusBars( false );
	
	do
	{
		err = 0;

		CInputBox* inbox = new CInputBox( 0, 8, MsgGetString(Msg_01635), MsgGetString(Msg_01639), NETID_TXT_LEN-1, CELL_TYPE_TEXT );
		inbox->SetText( nwpar.NetID );
		inbox->Show();

		if( inbox->GetExitCode() == WIN_EXITCODE_ESC )
		{
			delete inbox;

			// nasconde le barre di stato
			guiDeskTop->HideStatusBars();

			return 0;
		}

		snprintf( tmp, NETID_TXT_LEN, "%s", nwpar.NetID );
		snprintf( nwpar.NetID, NETID_TXT_LEN, "%s", inbox->GetText() );

		delete inbox;
	
		DelSpcR( nwpar.NetID );
		strupr( nwpar.NetID );
	
		if( strlen(nwpar.NetID) == 0 )
		{
			bipbip();
			W_Mess( MsgGetString(Msg_01653) );
			strcpy( nwpar.NetID, tmp );
			err = 1;
		}
		else
		{
			if( strcmp(nwpar.NetID,tmp) )
			{
				// prova ad aggiungere il nome macchina alla lista macchine in rete
				// (se la rete e' disattivata ritorna immediatamente senza errori)
				int add_ret = NewNetMachineName(nwpar.NetID);
	
				if( add_ret <= 0 )
				{
					if( add_ret == -1 )
					{
						bipbip();
						W_Mess( NETACCESS_ERR );
					}
					err = 1;
					strcpy( nwpar.NetID, tmp );
				}
				else
				{
					nwpar.NetID_Idx = add_ret;
				}
			}
		}
	} while( err );
	
	// nasconde le barre di stato
	guiDeskTop->HideStatusBars();
	
	WriteNetPar( nwpar );
	return 1;
}
