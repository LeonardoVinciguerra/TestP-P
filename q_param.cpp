/*
>>>> Q_PARAM.CPP

Gestione dei parametri di lavoro macchina.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++
++++    mod. by Walter 10/97 per nuova config. comandi - W09
++++    Mod.: W042000


++++    Mod Simone S090801 >> Interfaccia utente a classi

        Patch - ##P##SMOD200902 - Quota di prelievo ugelli diversa
                                  per le due punte.

*/
#include <iomanip>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "msglist.h"
#include "q_cost.h"
#include "q_graph.h"
#include "q_gener.h"
#include "q_help.h"
#include "q_tabe.h"
#include "q_wind.h"
#include "q_files.h"
#include "q_tabe.h"
#include "q_carint.h"
#include "q_oper.h"
#include "q_conf.h"
#include "q_init.h"
#include "q_fox.h"
#include "q_param.h"
#include "datetime.h"
#include "q_camera.h"

#include "c_win_par.h"
#include "c_pan.h"
#include "gui_desktop.h"
#include "gui_defs.h"

#include "q_param_new.h"
#include "keyutils.h"

#include <mss.h>


int fn_SniperCenteringModes( CWindow* parent );
int fn_SecurityReservedParameters( CWindow* parent );
int fn_SniperCenteringReservedParametersUI( CWindow* parent, int nsniper );


extern GUI_DeskTop* guiDeskTop;
extern struct CfgHeader QHeader;
extern struct CfgParam  QParam;
extern struct CfgTeste MapTeste;



//---------------------------------------------------------------------------
// finestra: Reserved parameters
//---------------------------------------------------------------------------
class ReservedParamsUI : public CWindowParams
{
public:
	ReservedParamsUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 60, 16 );
		SetTitle( MsgGetString(Msg_00128) );


		SM_SniperPar = new GUI_SubMenu();
		SM_SniperPar->Add( "Sniper 1", K_F1, 0, NULL, boost::bind( &ReservedParamsUI::onSniper1Par, this ) );
		SM_SniperPar->Add( "Sniper 2", K_F2, 0, NULL, boost::bind( &ReservedParamsUI::onSniper2Par, this ) );

		SM_AdvCam = new GUI_SubMenu();
		SM_AdvCam->Add( "Camera HEAD", K_F1, 0, NULL, fn_CameraTeaching_Head );
		SM_AdvCam->Add( "Camera EXT", K_F2, 0, NULL, fn_CameraTeaching_Ext );
	}

	~ReservedParamsUI()
	{
		delete SM_SniperPar;
		delete SM_AdvCam;
	}

	typedef enum
	{
		DISCHARGE_TIME,
		VACUUM_READ_TIME,
		C_PRESSURE_COMP,
		C_PRESSURE_TOOLS,
		ZERO_THETA_PROT_1,
		ZERO_THETA_PROT_2,
		TOOLS_REF_POINT_X1,
		TOOLS_REF_POINT_Y1,
		TOOLS_REF_POINT_X2,
		TOOLS_REF_POINT_Y2,
		STEP_VALUE_X,
		STEP_VALUE_Y,
		DEBUG_1,
		DEBUG_2,
		PRE_MOVE_DOWN
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[DISCHARGE_TIME]     = new C_Combo(  4, 2, MsgGetString(Msg_00619), 8, CELL_TYPE_SINT );
		m_combos[VACUUM_READ_TIME]   = new C_Combo(  4, 3, MsgGetString(Msg_00620), 8, CELL_TYPE_SINT );
		m_combos[C_PRESSURE_COMP]    = new C_Combo(  4, 4, MsgGetString(Msg_00139), 8, CELL_TYPE_SINT );
		m_combos[C_PRESSURE_TOOLS]   = new C_Combo(  4, 5, MsgGetString(Msg_00137), 8, CELL_TYPE_SINT );
		m_combos[ZERO_THETA_PROT_1]  = new C_Combo(  4, 7, MsgGetString(Msg_00622), 8, CELL_TYPE_SINT );
		m_combos[ZERO_THETA_PROT_2]  = new C_Combo( 47, 7, "", 8, CELL_TYPE_SINT );
		m_combos[TOOLS_REF_POINT_X1] = new C_Combo(  4, 8, MsgGetString(Msg_00623), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 3 );
		m_combos[TOOLS_REF_POINT_Y1] = new C_Combo( 47, 8, "", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 3 );
		m_combos[TOOLS_REF_POINT_X2] = new C_Combo(  4, 9, MsgGetString(Msg_05143), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 3 );
		m_combos[TOOLS_REF_POINT_Y2] = new C_Combo( 47, 9, "", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 3 );
		m_combos[STEP_VALUE_X]       = new C_Combo(  4, 10, MsgGetString(Msg_00143), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 3 );
		m_combos[STEP_VALUE_Y]       = new C_Combo( 47, 10, "", 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 3 );
		m_combos[DEBUG_1]            = new C_Combo(  4, 12, MsgGetString(Msg_00144), 8, CELL_TYPE_UINT );
		m_combos[DEBUG_2]            = new C_Combo( 47, 12, "", 8, CELL_TYPE_UINT );
		m_combos[PRE_MOVE_DOWN]      = new C_Combo(  4, 13, MsgGetString(Msg_01423), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 3 );

		// set params
		m_combos[DISCHARGE_TIME]->SetVMinMax( 0, 900 );
		m_combos[VACUUM_READ_TIME]->SetVMinMax( 0, 900 );
		m_combos[C_PRESSURE_COMP]->SetVMinMax( 0, 250 );
		m_combos[C_PRESSURE_TOOLS]->SetVMinMax( 0, 250 );
		m_combos[ZERO_THETA_PROT_1]->SetVMinMax( -360, 360 );
		m_combos[ZERO_THETA_PROT_2]->SetVMinMax( -360, 360 );
		m_combos[TOOLS_REF_POINT_X1]->SetVMinMax( -20.f, 20.f );
		m_combos[TOOLS_REF_POINT_Y1]->SetVMinMax( -20.f, 20.f );
		m_combos[TOOLS_REF_POINT_X2]->SetVMinMax( -20.f, 20.f );
		m_combos[TOOLS_REF_POINT_Y2]->SetVMinMax( -20.f, 20.f );
		m_combos[STEP_VALUE_X]->SetVMinMax( 0.f, 1.f );
		m_combos[STEP_VALUE_Y]->SetVMinMax( 0.f, 1.f );
		m_combos[DEBUG_1]->SetVMinMax( 0, 0xFFFF );
		m_combos[DEBUG_2]->SetVMinMax( 0, 0xFFFF );
		m_combos[PRE_MOVE_DOWN]->SetVMinMax( 0.5f, 4.f );

		// add to combo list
		m_comboList->Add( m_combos[DISCHARGE_TIME]    , 0, 0 );
		m_comboList->Add( m_combos[VACUUM_READ_TIME]  , 1, 0 );
		m_comboList->Add( m_combos[C_PRESSURE_COMP]   , 2, 0 );
		m_comboList->Add( m_combos[C_PRESSURE_TOOLS]  , 3, 0 );
		m_comboList->Add( m_combos[ZERO_THETA_PROT_1] , 5, 0 );
		m_comboList->Add( m_combos[ZERO_THETA_PROT_2] , 5, 1 );
		m_comboList->Add( m_combos[TOOLS_REF_POINT_X1], 6, 0 );
		m_comboList->Add( m_combos[TOOLS_REF_POINT_Y1], 6, 1 );
		m_comboList->Add( m_combos[TOOLS_REF_POINT_X2], 7, 0 );
		m_comboList->Add( m_combos[TOOLS_REF_POINT_Y2], 7, 1 );
		m_comboList->Add( m_combos[STEP_VALUE_X]      , 8, 0 );
		m_comboList->Add( m_combos[STEP_VALUE_Y]      , 8, 1 );
		m_comboList->Add( m_combos[DEBUG_1]           , 9, 0 );
		m_comboList->Add( m_combos[DEBUG_2]           , 9, 1 );
		m_comboList->Add( m_combos[PRE_MOVE_DOWN]     , 10, 0 );
	}

	void onRefresh()
	{
		GUI_Freeze_Locker lock;

		m_combos[DISCHARGE_TIME]->SetTxt( QHeader.Dis_vuoto );
		m_combos[VACUUM_READ_TIME]->SetTxt( QHeader.D_vuoto );
		m_combos[C_PRESSURE_COMP]->SetTxt( QHeader.TContro_Comp );
		m_combos[C_PRESSURE_TOOLS]->SetTxt( QHeader.TContro_Uge );
		m_combos[ZERO_THETA_PROT_1]->SetTxt( QHeader.zertheta_Prerot[0] );
		m_combos[ZERO_THETA_PROT_2]->SetTxt( QHeader.zertheta_Prerot[1] );
		m_combos[TOOLS_REF_POINT_X1]->SetTxt( QHeader.uge_offs_x[0] );
		m_combos[TOOLS_REF_POINT_Y1]->SetTxt( QHeader.uge_offs_y[0] );
		m_combos[TOOLS_REF_POINT_X2]->SetTxt( QHeader.uge_offs_x[1] );
		m_combos[TOOLS_REF_POINT_Y2]->SetTxt( QHeader.uge_offs_y[1] );
		m_combos[STEP_VALUE_X]->SetTxt( QHeader.PassoX );
		m_combos[STEP_VALUE_Y]->SetTxt( QHeader.PassoY );
		m_combos[DEBUG_1]->SetTxt( QHeader.debugMode1 );
		m_combos[DEBUG_2]->SetTxt( QHeader.debugMode2 );
		m_combos[PRE_MOVE_DOWN]->SetTxt( QHeader.predownDelta );
	}

	void onEdit()
	{
		QHeader.Dis_vuoto = m_combos[DISCHARGE_TIME]->GetInt();
		QHeader.D_vuoto = m_combos[VACUUM_READ_TIME]->GetInt();
		QHeader.TContro_Comp = m_combos[C_PRESSURE_COMP]->GetInt();
		QHeader.TContro_Uge = m_combos[C_PRESSURE_TOOLS]->GetInt();
		QHeader.zertheta_Prerot[0] = m_combos[ZERO_THETA_PROT_1]->GetInt();
		QHeader.zertheta_Prerot[1] = m_combos[ZERO_THETA_PROT_2]->GetInt();
		QHeader.uge_offs_x[0] = m_combos[TOOLS_REF_POINT_X1]->GetFloat();
		QHeader.uge_offs_y[0] = m_combos[TOOLS_REF_POINT_Y1]->GetFloat();
		QHeader.uge_offs_x[1] = m_combos[TOOLS_REF_POINT_X2]->GetFloat();
		QHeader.uge_offs_y[1] = m_combos[TOOLS_REF_POINT_Y2]->GetFloat();
		QHeader.PassoX = m_combos[STEP_VALUE_X]->GetFloat();
		QHeader.PassoY = m_combos[STEP_VALUE_Y]->GetFloat();
		QHeader.debugMode1 = m_combos[DEBUG_1]->GetInt();
		QHeader.debugMode2 = m_combos[DEBUG_2]->GetInt();
		QHeader.predownDelta = m_combos[PRE_MOVE_DOWN]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01121), K_F3, 0, NULL, boost::bind( &ReservedParamsUI::onPIDBrush, this ) );
		#ifdef __SNIPER
		m_menu->Add( MsgGetString(Msg_00906), K_F4, 0, NULL, boost::bind( &ReservedParamsUI::onSniperCenteringModes, this ) );
		#endif
		m_menu->Add( MsgGetString(Msg_00824), K_F5, 0, NULL, boost::bind( &ReservedParamsUI::onOtherPsrams, this ) );
		m_menu->Add( MsgGetString(Msg_01836), K_F6, 0, NULL, boost::bind( &ReservedParamsUI::onFeederTimings, this ) );
		m_menu->Add( MsgGetString(Msg_05088), K_F7, 0, SM_AdvCam, NULL );
		m_menu->Add( MsgGetString(Msg_05087), K_F8, 0, NULL, boost::bind( &ReservedParamsUI::onExtCameraAdvancedParams, this ) );
		m_menu->Add( MsgGetString(Msg_05098), K_F9, 0, SM_SniperPar,NULL);
		m_menu->Add( MsgGetString(Msg_00128), K_F10, 0, NULL, boost::bind( &ReservedParamsUI::onSecurityReservedParameters, this ) );
		m_menu->Add( MsgGetString(Msg_00080), K_F11, 0, NULL, boost::bind( &ReservedParamsUI::onSpeedLimits, this ) );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				onPIDBrush();
				return true;

			#ifdef __SNIPER
			case K_F4:
				onSniperCenteringModes();
				return true;
			#endif

			case K_F5:
				onOtherPsrams();
				return true;

			case K_F6:
				onFeederTimings();
				return true;

			case K_F7:
				SM_AdvCam->Show();
				return true;

			case K_F8:
				onExtCameraAdvancedParams();
				return true;

			case K_F9:
				SM_SniperPar->Show();
				return true;

			case K_F10:
				onSecurityReservedParameters();
				return true;

			case K_F11:
				onSpeedLimits();
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

private:
	int onPIDBrush()
	{
		return fn_PIDBrushlessParams( this );
	}

	int onSniperCenteringModes()
	{
		return fn_SniperCenteringModes( this );
	}

	int onOtherPsrams()
	{
		return fn_OtherAdvancedParams( this );
	}

	int onSecurityReservedParameters()
	{
		return fn_SecurityReservedParameters( this );
	}

	int onSniper1Par()
	{
		return fn_SniperCenteringReservedParametersUI( this, 1 );
	}

	int onSniper2Par()
	{
		return fn_SniperCenteringReservedParametersUI( this, 2 );
	}

	int onFeederTimings()
	{
		return fn_FeederTimings( this );
	}

	int onSpeedLimits()
	{
		return fn_SpeedLimits( this );
	}

	int onExtCameraAdvancedParams()
	{
		return fn_ExtCameraAdvancedParams( this );
	}

	GUI_SubMenu* SM_SniperPar;
	GUI_SubMenu* SM_AdvCam;
};

int fn_ReservedParams()
{
	ReservedParamsUI win;
	win.Show();
	win.Hide();

	return 1;
}



//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------

#ifdef __SNIPER

class SniperCenteringModesUI : public CWindowParams
{
public:
	SniperCenteringModesUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 56, 14 );
		SetTitle( MsgGetString(Msg_00906) );
	}

	typedef enum
	{
		CODE,
		NAME,
		ROT_SPEED,
		ROT_ACC,
		ROT_ANGLE,
		PRE_ANGLE,
		D_FRAMES,
		ALGO_NUM
	} combo_labels;

protected:
	void onInit()
	{
		//TODO: rifare come tutte le altre funzioni che operano su files
		if( SniperModes::inst().checkOk() )
		{
			//throw fileerr;
		}

		m_nrec = 0;
		SniperModes::inst().getRecord( m_nrec, m_rec );

		// create combos
		m_combos[CODE]      = new C_Combo( 3, 1, MsgGetString(Msg_05029), 2, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[NAME]      = new C_Combo( 3, 2, MsgGetString(Msg_05028), 15, CELL_TYPE_TEXT );
		m_combos[ROT_SPEED] = new C_Combo( 3, 3, MsgGetString(Msg_05030), 7, CELL_TYPE_UINT );
		m_combos[ROT_ACC]   = new C_Combo( 3, 4, MsgGetString(Msg_05031), 7, CELL_TYPE_UINT );
		m_combos[ROT_ANGLE] = new C_Combo( 3, 5, MsgGetString(Msg_05032), 7, CELL_TYPE_UINT );
		m_combos[PRE_ANGLE] = new C_Combo( 3, 6, MsgGetString(Msg_05033), 7, CELL_TYPE_UINT );
		m_combos[D_FRAMES]  = new C_Combo( 3, 7, MsgGetString(Msg_05034), 2, CELL_TYPE_UINT );
		m_combos[ALGO_NUM]  = new C_Combo( 3, 8, MsgGetString(Msg_05035), 2, CELL_TYPE_UINT );

		// set params
		m_combos[ROT_SPEED]->SetVMinMax( 10, MIN( QParam.prot1_vel, QParam.prot2_vel) );
		m_combos[ROT_ACC]->SetVMinMax( 10, MIN( QParam.prot1_acc, QParam.prot2_acc) );
		m_combos[ROT_ANGLE]->SetVMinMax( 0, 360 );
		m_combos[PRE_ANGLE]->SetVMinMax( 0, 90 );
		m_combos[D_FRAMES]->SetVMinMax( 0, 99 );
		m_combos[ALGO_NUM]->SetVMinMax( 0, 99 );

		// add to combo list
		m_comboList->Add( m_combos[CODE],      0, 0 );
		m_comboList->Add( m_combos[NAME],      1, 0 );
		m_comboList->Add( m_combos[ROT_SPEED], 2, 0 );
		m_comboList->Add( m_combos[ROT_ACC],   3, 0 );
		m_comboList->Add( m_combos[ROT_ANGLE], 4, 0 );
		m_comboList->Add( m_combos[PRE_ANGLE], 5, 0 );
		m_comboList->Add( m_combos[D_FRAMES],  6, 0 );
		m_comboList->Add( m_combos[ALGO_NUM],  7, 0 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 10, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 11, MsgGetString(Msg_05036), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	}

	void onRefresh()
	{
		GUI_Freeze_Locker lock;

		m_combos[CODE]->SetTxt( m_rec.idx );
		m_combos[NAME]->SetTxt( m_rec.name );
		m_combos[ROT_SPEED]->SetTxt( m_rec.speed );
		m_combos[ROT_ACC]->SetTxt( m_rec.acceleration );
		m_combos[ROT_ANGLE]->SetTxt( m_rec.search_angle );
		m_combos[PRE_ANGLE]->SetTxt( m_rec.prerotation );
		m_combos[D_FRAMES]->SetTxt( m_rec.discard_nframes );
		m_combos[ALGO_NUM]->SetTxt( m_rec.algorithm );
	}

	void onEdit()
	{
		strncpy( m_rec.name, m_combos[NAME]->GetTxt(), sizeof(m_rec.name)-1 );
		m_rec.speed = m_combos[ROT_SPEED]->GetInt();
		m_rec.acceleration = m_combos[ROT_ACC]->GetInt();
		m_rec.search_angle = m_combos[ROT_ANGLE]->GetInt();
		m_rec.prerotation = m_combos[PRE_ANGLE]->GetInt();
		m_rec.discard_nframes = m_combos[D_FRAMES]->GetInt();
		m_rec.algorithm = m_combos[ALGO_NUM]->GetInt();
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_PAGEUP:
				if( m_nrec > 0 )
				{
					SniperModes::inst().updateRecord( m_nrec, m_rec );
					m_nrec--;
					SniperModes::inst().getRecord( m_nrec, m_rec );
					return true;
				}
				break;

			case K_PAGEDOWN:
				if( m_nrec < SniperModes::inst().getNRecords() - 1 )
				{
					SniperModes::inst().updateRecord( m_nrec, m_rec );
					m_nrec++;
					SniperModes::inst().getRecord( m_nrec, m_rec );
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
		SniperModes::inst().updateRecord( m_nrec, m_rec );
	}

	SniperModeData m_rec;
	int m_nrec;
};


int fn_SniperCenteringModes( CWindow* parent )
{
	SniperCenteringModesUI win( parent );
	win.Show();
	win.Hide();

	return 1;
}


//---------------------------------------------------------------------------------------------
//TODO mettere pag up/down per selezionare punte
class SniperCenteringReservedParametersUI : public CWindowParams
{
public:
	SniperCenteringReservedParametersUI( CWindow* parent, unsigned int nsniper ) : CWindowParams( parent )
	{
		m_nsniper = nsniper;
		
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 62, 11 );
		
		char buf[80];
		sprintf( buf, MsgGetString(Msg_05089), m_nsniper );
		SetTitle( buf );

		CCenteringReservedParameters::inst().readData( m_data );
	}

	typedef enum
	{
		KMM_PIXEL,
		POS,
		MAX_PICK_ERR_TYPE1,
		MAX_PICK_ERR_TYPE2,
		SAFETY_MARGIN_TYPE1,
		SAFETY_MARGIN_TYPE2,
		MAXIMUM_TYPE1_COMP_SIZE,
		MAXIMUM_TYPE2_COMP_SIZE
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[KMM_PIXEL] = new C_Combo( 3, 1, MsgGetString(Msg_05090), 8, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 6 );
		m_combos[POS]       = new C_Combo( 3, 2, MsgGetString(Msg_05091), 6, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		
		m_combos[MAX_PICK_ERR_TYPE1] = new C_Combo( 3, 6, MsgGetString(Msg_05092), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 3 );
		m_combos[MAX_PICK_ERR_TYPE2] = new C_Combo( 50, 6, "", 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 3 );
		
		m_combos[SAFETY_MARGIN_TYPE1] = new C_Combo( 3, 7, MsgGetString(Msg_05093), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 3 );
		m_combos[SAFETY_MARGIN_TYPE2] = new C_Combo( 50, 7, "", 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 3 );
		
		m_combos[MAXIMUM_TYPE1_COMP_SIZE] = new C_Combo( 3, 8, MsgGetString(Msg_05094), 6, CELL_TYPE_UDEC,CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		m_combos[MAXIMUM_TYPE2_COMP_SIZE] = new C_Combo( 50, 8, "", 6, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		
		// set params
		m_combos[MAX_PICK_ERR_TYPE1]->SetVMinMax( 0.0f, 2.0f );
		m_combos[MAX_PICK_ERR_TYPE2]->SetVMinMax( 0.0f, 2.0f );
		m_combos[SAFETY_MARGIN_TYPE1]->SetVMinMax( 0.0f, 2.0f );
		m_combos[SAFETY_MARGIN_TYPE2]->SetVMinMax( 0.0f, 2.0f );
		
		// add to combo list
		m_comboList->Add( m_combos[KMM_PIXEL], 0, 0 );
		m_comboList->Add( m_combos[POS], 1, 0 );
		m_comboList->Add( m_combos[MAX_PICK_ERR_TYPE1], 2, 0 );
		m_comboList->Add( m_combos[MAX_PICK_ERR_TYPE2], 2, 1 );
		m_comboList->Add( m_combos[SAFETY_MARGIN_TYPE1], 3, 0 );
		m_comboList->Add( m_combos[SAFETY_MARGIN_TYPE2], 3, 1 );
		m_comboList->Add( m_combos[MAXIMUM_TYPE1_COMP_SIZE], 4, 0 );
		m_comboList->Add( m_combos[MAXIMUM_TYPE2_COMP_SIZE], 4, 1 );
	}

	void onShow()
	{
		char buf[80];
		sprintf( buf, MsgGetString(Msg_05095), 1 );
		DrawText( 40, 4, buf );
		sprintf( buf, MsgGetString(Msg_05095), 2 );
		DrawText( 50, 4, buf );
	}

	void onRefresh()
	{
		GUI_Freeze_Locker lock;

		m_combos[KMM_PIXEL]->SetTxt( QHeader.sniper_kpix_um[m_nsniper-1] );
		m_combos[POS]->SetTxt( MapTeste.ccal_z_cal_q[m_nsniper-1] );
	
		m_combos[MAX_PICK_ERR_TYPE1]->SetTxt( m_data.sniper_type1_components_max_pick_error );
		m_combos[MAX_PICK_ERR_TYPE2]->SetTxt( m_data.sniper_type2_components_max_pick_error );
	
		m_combos[SAFETY_MARGIN_TYPE1]->SetTxt( m_data.sniper_type1_safety_margin );
		m_combos[SAFETY_MARGIN_TYPE2]->SetTxt( m_data.sniper_type2_safety_margin );
		
		float type1_limit = getSniperType1ComponentLimit( m_nsniper, m_data );
		float type2_limit = getSniperType2ComponentLimit( m_nsniper, m_data );
		m_combos[MAXIMUM_TYPE1_COMP_SIZE]->SetTxt( type1_limit );
		m_combos[MAXIMUM_TYPE2_COMP_SIZE]->SetTxt( type2_limit );
	}

	void onEdit()
	{
		m_data.sniper_type1_components_max_pick_error = m_combos[MAX_PICK_ERR_TYPE1]->GetFloat();
		m_data.sniper_type2_components_max_pick_error = m_combos[MAX_PICK_ERR_TYPE2]->GetFloat();
		m_data.sniper_type1_safety_margin = m_combos[SAFETY_MARGIN_TYPE1]->GetFloat();
		m_data.sniper_type2_safety_margin = m_combos[SAFETY_MARGIN_TYPE2]->GetFloat();
	}

	void onClose()
	{
		CCenteringReservedParameters::inst().writeData( m_data );
	}

	CenteringReservedParameters m_data;
	unsigned int m_nsniper;
};

int fn_SniperCenteringReservedParametersUI( CWindow* parent, int nsniper )
{
	SniperCenteringReservedParametersUI win( parent, nsniper );
	win.Show();
	win.Hide();
	
	return 1;
}

#endif


//-----------------------------------------------------------------------

#define MOVEMENT_CONFIRM_TIMEOUT_MIN 		0
#define MOVEMENT_CONFIRM_TIMEOUT_MAX 		90

class SecurityReservedParameterUI : public CWindowParams
{
public:
	SecurityReservedParameterUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 30, 8 );
		SetTitle( MsgGetString(Msg_00128) );
	}

	typedef enum
	{
		OPEN_PROTECTION_MOV_ENABLED,
		EMERGENCY_STOP_DISABLED,
		END_ASSEMBLY_CHECK_DISABLED,
		MOV_CONFIRM_DISABLED,
		MOV_CONFIRM_TIMEOUT
	} combo_labels;

protected:
	void onInit()
	{
		if( CSecurityReservedParametersFile::inst().is_open() )
		{
			CSecurityReservedParametersFile::inst().close();
			CSecurityReservedParametersFile::inst().open();
		}
		CSecurityReservedParametersFile::inst().readParameters( m_parameters );
		
		// create combos
		m_combos[OPEN_PROTECTION_MOV_ENABLED] = new C_Combo( 3, 1, MsgGetString(Msg_05137), 4, CELL_TYPE_YN );
		m_combos[EMERGENCY_STOP_DISABLED] =     new C_Combo( 3, 2, MsgGetString(Msg_05138), 4, CELL_TYPE_YN );
		m_combos[END_ASSEMBLY_CHECK_DISABLED] = new C_Combo( 3, 3, MsgGetString(Msg_05139), 4, CELL_TYPE_YN );
		m_combos[MOV_CONFIRM_DISABLED] =        new C_Combo( 3, 4, MsgGetString(Msg_05140), 4, CELL_TYPE_YN );
		m_combos[MOV_CONFIRM_TIMEOUT] =         new C_Combo( 3, 5, MsgGetString(Msg_05141), 4, CELL_TYPE_UINT );
		
		// set params
		m_combos[MOV_CONFIRM_TIMEOUT]->SetVMinMax( MOVEMENT_CONFIRM_TIMEOUT_MIN, MOVEMENT_CONFIRM_TIMEOUT_MAX );

		// add to combo list
		m_comboList->Add( m_combos[OPEN_PROTECTION_MOV_ENABLED], 0, 0 );
		m_comboList->Add( m_combos[EMERGENCY_STOP_DISABLED], 1, 0 );
		m_comboList->Add( m_combos[END_ASSEMBLY_CHECK_DISABLED], 2, 0 );
		m_comboList->Add( m_combos[MOV_CONFIRM_DISABLED], 3, 0 );
		m_comboList->Add( m_combos[MOV_CONFIRM_TIMEOUT], 4, 0 );
	}

	void onRefresh()
	{
		GUI_Freeze_Locker lock;

		m_combos[OPEN_PROTECTION_MOV_ENABLED]->SetTxtYN( m_parameters.flags.bits.open_protection_mov_enabled );
		m_combos[EMERGENCY_STOP_DISABLED]->SetTxtYN( m_parameters.flags.bits.emergency_stop_disabled );
		m_combos[END_ASSEMBLY_CHECK_DISABLED]->SetTxtYN( m_parameters.flags.bits.end_assembly_protection_check_disabled );
		m_combos[MOV_CONFIRM_DISABLED]->SetTxtYN( m_parameters.flags.bits.mov_confirm_disabled );
		m_combos[MOV_CONFIRM_TIMEOUT]->SetTxt( m_parameters.mov_confirm_timeout );
	}
	
	void onEdit()
	{
		s_security_reserved_params tmp;
		memset( &tmp, 0, sizeof(s_security_reserved_params) );
		
		tmp.flags.bits.open_protection_mov_enabled = m_combos[OPEN_PROTECTION_MOV_ENABLED]->GetYN();
		tmp.flags.bits.emergency_stop_disabled = m_combos[EMERGENCY_STOP_DISABLED]->GetYN();
		tmp.flags.bits.end_assembly_protection_check_disabled = m_combos[END_ASSEMBLY_CHECK_DISABLED]->GetYN();
		tmp.flags.bits.mov_confirm_disabled = m_combos[MOV_CONFIRM_DISABLED]->GetYN();
		tmp.mov_confirm_timeout = m_combos[MOV_CONFIRM_TIMEOUT]->GetInt();
		
		if( !log(tmp) )
		{
			printf("FAIL LOG\n");
			bipbip();
			return;
		}
		
		m_parameters = tmp;
		CSecurityReservedParametersFile::inst().writeParameters( m_parameters );
	}

	bool log( const s_security_reserved_params& new_param )
	{
		if( access(RESERVED_PARAMTERS_LOG_FILE,F_OK) )
		{
			if( !create_log_file() )
			{
				return false;
			}
		}
	
		if( !m_log_file.is_open() )
		{
			m_log_file.open( RESERVED_PARAMTERS_LOG_FILE, std::ios::app | std::ios::out );
			if( !m_log_file.is_open() )
			{
				return false;
			}
		}
		
		struct date today;
		struct time now;
	
		getdate( &today );
		gettime( &now );
		
		m_log_file << int(today.da_day) << '/' << int(today.da_mon) << '/' << today.da_year;
		
		m_log_file << ';' << int(now.ti_hour) << ':' << int(now.ti_min) << ':' << int(now.ti_sec); 
		m_log_file << ";0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << new_param.flags.mask << std::nouppercase;
		m_log_file << ';' << std::dec << new_param.mov_confirm_timeout << std::endl;
		
		m_log_file.flush();
		
		return true;
	}

	bool create_log_file()
	{
		if( m_log_file.is_open() )
		{
			m_log_file.close();
		}
		
		m_log_file.open( RESERVED_PARAMTERS_LOG_FILE, std::ios::trunc | std::ios::out );
		
		bool r = m_log_file.is_open();
		
		m_log_file.close();
		
		return r;
	}

	s_security_reserved_params m_parameters;
	std::ofstream m_log_file;
};

int fn_SecurityReservedParameters( CWindow* parent )
{
	SecurityReservedParameterUI win( parent );
	win.Show();
	win.Hide();

	return 1;
}
