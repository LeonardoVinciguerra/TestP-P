//---------------------------------------------------------------------------
//
// Name:        q_param_new.cpp
// Author:      Gabriel Ferri
// Created:     15/09/2011
// Description: Quadra configuration parameters
//
//---------------------------------------------------------------------------
#include "q_param_new.h"

#include "c_win_par.h"
#include "c_win_table.h"
#include "c_pan.h"
#include "gui_defs.h"

#include "msglist.h"
#include "q_init.h"
#include "q_carint.h"
#include "q_carobj.h"
#include "keyutils.h"

#include <mss.h>


// velocita' e acc.
#define XY_SPEED_LIM     100, 5000
#define XY_ACC_LIM       100, 20000
#define Z_SPEED_LIM      5, 4000
#define Z_ACC_LIM        1700, 30000
#define ROT_SPEED_LIM    1000, 200000
#define ROT_ACC_LIM      10000, 200000


extern CfgHeader QHeader;



//---------------------------------------------------------------------------
// finestra: PID brushless motor params
//---------------------------------------------------------------------------
class PIDBrushlessParamsUI : public CWindowTable
{
public:
	PIDBrushlessParamsUI( CWindow* parent ) : CWindowTable( parent )
	{
		nozzle = 1;
		firstTime = true;

		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 50, 17 );

		char buf[80];
		snprintf( buf, 80, MsgGetString(Msg_01120), nozzle );
		SetTitle( buf );

		BrushDataRead( brushData );
	}

protected:
	void onInit()
	{
		// create table
		m_table = new CTable( 5, 2, 10, TABLE_STYLE_DEFAULT, this );

		// add columns
		m_table->AddCol( "", 4, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
		m_table->AddCol( "P", 8, CELL_TYPE_UINT );
		m_table->AddCol( "I", 8, CELL_TYPE_UINT );
		m_table->AddCol( "D", 8, CELL_TYPE_UINT );
		m_table->AddCol( "Clip", 8, CELL_TYPE_UINT );

		// set params
		m_table->SetColMinMax( 0, 0, 9999 );
		m_table->SetColMinMax( 1, 0, 9999 );
		m_table->SetColMinMax( 2, 0, 9999 );
		m_table->SetColMinMax( 3, 0, 9999 );
		m_table->SetColMinMax( 3, 0, 4096 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 13, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 14, MsgGetString(Msg_00318), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	void onRefresh()
	{
		if( firstTime )
		{
			firstTime = false;
			m_table->Select( 0, 1 );
		}

		for( int i = 0; i < 10; i++ )
		{
			m_table->SetText( i, 0, i );
			m_table->SetText( i, 1, brushData[i].p[nozzle-1] );
			m_table->SetText( i, 2, brushData[i].i[nozzle-1] );
			m_table->SetText( i, 3, brushData[i].d[nozzle-1] );
			m_table->SetText( i, 4, brushData[i].clip[nozzle-1] );
		}
	}

	void onEdit()
	{
		for( int i = 0; i < 10; i++ )
		{
			brushData[i].p[nozzle-1] = m_table->GetInt( i, 1 );
			brushData[i].i[nozzle-1] = m_table->GetInt( i, 2 );
			brushData[i].d[nozzle-1] = m_table->GetInt( i, 3 );
			brushData[i].clip[nozzle-1] = m_table->GetInt( i, 4 );
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
					snprintf( buf, 80, MsgGetString(Msg_01120), nozzle );
					SetTitle( buf );
					return true;
				}
				break;

			case K_PAGEDOWN:
				if( nozzle < 2 )
				{
					nozzle++;

					char buf[80];
					snprintf( buf, 80, MsgGetString(Msg_01120), nozzle );
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
		BrushDataSave( brushData );

		//ricarica parametri azionamenti
		OpenRotation( ROT_RELOAD );
	}

	int nozzle;
	CfgBrush brushData[10];
	bool firstTime;
};

int fn_PIDBrushlessParams( CWindow* parent )
{
	PIDBrushlessParamsUI win( parent );
	win.Show();
	win.Hide();

	return 1;
}


//---------------------------------------------------------------------------
// finestra: Speed limits
//---------------------------------------------------------------------------
class SpeedLimitsUI : public CWindowParams
{
public:
	SpeedLimitsUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 50, 16 );
		SetTitle( MsgGetString(Msg_00080) );
	}

	typedef enum
	{
		XY_MAX_SPEED,
		XY_MAX_ACC,
		Z_MAX_SPEED,
		Z_MAX_ACC,
		ROT_MAX_SPEED,
		ROT_MAX_ACC
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[XY_MAX_SPEED]  = new C_Combo( 4,  2, MsgGetString(Msg_00241), 7, CELL_TYPE_UINT );
		m_combos[XY_MAX_ACC]    = new C_Combo( 4,  3, MsgGetString(Msg_00242), 7, CELL_TYPE_UINT );
		m_combos[Z_MAX_SPEED]   = new C_Combo( 4,  7, MsgGetString(Msg_00241), 7, CELL_TYPE_UINT );
		m_combos[Z_MAX_ACC]     = new C_Combo( 4,  8, MsgGetString(Msg_00242), 7, CELL_TYPE_UINT );
		m_combos[ROT_MAX_SPEED] = new C_Combo( 4, 12, MsgGetString(Msg_00997), 7, CELL_TYPE_UINT );
		m_combos[ROT_MAX_ACC]   = new C_Combo( 4, 13, MsgGetString(Msg_00998), 7, CELL_TYPE_UINT );

		// set params
		m_combos[XY_MAX_SPEED]->SetVMinMax( XY_SPEED_LIM );
		m_combos[XY_MAX_ACC]->SetVMinMax( XY_ACC_LIM );
		m_combos[Z_MAX_SPEED]->SetVMinMax( Z_SPEED_LIM );
		m_combos[Z_MAX_ACC]->SetVMinMax( Z_ACC_LIM );
		m_combos[ROT_MAX_SPEED]->SetVMinMax( ROT_SPEED_LIM );
		m_combos[ROT_MAX_ACC]->SetVMinMax( ROT_ACC_LIM );

		// add to combo list
		m_comboList->Add( m_combos[XY_MAX_SPEED] , 0, 0 );
		m_comboList->Add( m_combos[XY_MAX_ACC]   , 1, 0 );
		m_comboList->Add( m_combos[Z_MAX_SPEED]  , 2, 0 );
		m_comboList->Add( m_combos[Z_MAX_ACC]    , 3, 0 );
		m_comboList->Add( m_combos[ROT_MAX_SPEED], 4, 0 );
		m_comboList->Add( m_combos[ROT_MAX_ACC]  , 5, 0 );
	}

	void onShow()
	{
		DrawSubTitle(  0, MsgGetString(Msg_00025) );
		//TODO messaggi
		DrawSubTitle(  5, "Punte Z" );
		DrawSubTitle( 10, "Punte rotazione" );
	}

	void onRefresh()
	{
		m_combos[XY_MAX_SPEED]->SetTxt( QHeader.xyMaxSpeed );
		m_combos[XY_MAX_ACC]->SetTxt( QHeader.xyMaxAcc );
		m_combos[Z_MAX_SPEED]->SetTxt( QHeader.zMaxSpeed );
		m_combos[Z_MAX_ACC]->SetTxt( QHeader.zMaxAcc );
		m_combos[ROT_MAX_SPEED]->SetTxt( QHeader.rotMaxSpeed );
		m_combos[ROT_MAX_ACC]->SetTxt( QHeader.rotMaxAcc );
	}

	void onEdit()
	{
		QHeader.xyMaxSpeed = m_combos[XY_MAX_SPEED]->GetInt();
		QHeader.xyMaxAcc = m_combos[XY_MAX_ACC]->GetInt();
		QHeader.zMaxSpeed = m_combos[Z_MAX_SPEED]->GetInt();
		QHeader.zMaxAcc = m_combos[Z_MAX_ACC]->GetInt();
		QHeader.rotMaxSpeed = m_combos[ROT_MAX_SPEED]->GetInt();
		QHeader.rotMaxAcc = m_combos[ROT_MAX_ACC]->GetInt();
	}
};

int fn_SpeedLimits( CWindow* parent )
{
	SpeedLimitsUI win( parent );
	win.Show();
	win.Hide();

	return 1;
}


//---------------------------------------------------------------------------
// finestra: Feeder timings
//---------------------------------------------------------------------------
class FeederTimingsUI : public CWindowParams
{
public:
	FeederTimingsUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 60, 14 );
		SetTitle( MsgGetString(Msg_01836) );
	}

	typedef enum
	{
		FEEDING_TYPE,
		FEEDER_COIL_S,
		FEEDER_COIL_D,
		MOTOR_ADV_S,
		MOTOR_ADV_D,
		MOTOR_INV_S,
		MOTOR_INV_D,
		MOTOR_WAIT
	} combo_labels;

protected:
	void onInit()
	{
		cur_rec = 0;
		
		// create combos
		m_combos[FEEDING_TYPE]  = new C_Combo(  3, 1, MsgGetString(Msg_01830), 2, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[FEEDER_COIL_S] = new C_Combo(  3, 3, MsgGetString(Msg_01831), 4, CELL_TYPE_UINT );
		m_combos[FEEDER_COIL_D] = new C_Combo( 49, 3, "", 4, CELL_TYPE_UINT );
		m_combos[MOTOR_ADV_S]   = new C_Combo(  3, 4, MsgGetString(Msg_01832), 4, CELL_TYPE_UINT );
		m_combos[MOTOR_ADV_D]   = new C_Combo( 49, 4, "", 4, CELL_TYPE_UINT );
		m_combos[MOTOR_INV_S]   = new C_Combo(  3, 5, MsgGetString(Msg_01833), 4, CELL_TYPE_UINT );
		m_combos[MOTOR_INV_D]   = new C_Combo( 49, 5, "", 4, CELL_TYPE_UINT );
		m_combos[MOTOR_WAIT]    = new C_Combo(  3, 7, MsgGetString(Msg_01834), 4, CELL_TYPE_UINT );
		
		// set params
		m_combos[FEEDING_TYPE]->SetVMinMax( 0, 999 );
		m_combos[FEEDER_COIL_S]->SetVMinMax( 0, 999 );
		m_combos[FEEDER_COIL_D]->SetVMinMax( 0, 999 );
		m_combos[MOTOR_ADV_S]->SetVMinMax( 0, 999 );
		m_combos[MOTOR_ADV_D]->SetVMinMax( 0, 999 );
		m_combos[MOTOR_INV_S]->SetVMinMax( 0, 999 );
		m_combos[MOTOR_INV_D]->SetVMinMax( 0, 999 );
		m_combos[MOTOR_WAIT]->SetVMinMax( 0, 999 );
		
		// add to combo list
		m_comboList->Add( m_combos[FEEDING_TYPE],  0, 0 );
		m_comboList->Add( m_combos[FEEDER_COIL_S], 1, 0 );
		m_comboList->Add( m_combos[FEEDER_COIL_D], 1, 1 );
		m_comboList->Add( m_combos[MOTOR_ADV_S],   2, 0 );
		m_comboList->Add( m_combos[MOTOR_ADV_D],   2, 1 );
		m_comboList->Add( m_combos[MOTOR_INV_S],   3, 0 );
		m_comboList->Add( m_combos[MOTOR_INV_D],   3, 1 );
		m_comboList->Add( m_combos[MOTOR_WAIT],    4, 0 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 10, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 11, MsgGetString(Msg_01892), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	void onRefresh()
	{
		struct CarTimeData dta;
		CarTime_Read( dta, cur_rec );
		
		m_combos[FEEDING_TYPE]->SetTxt( dta.name );
		m_combos[FEEDER_COIL_S]->SetTxt( dta.start_ele );
		m_combos[FEEDER_COIL_D]->SetTxt( dta.end_ele );
		m_combos[MOTOR_ADV_S]->SetTxt( dta.start_motor );
		m_combos[MOTOR_ADV_D]->SetTxt( dta.end_motor );
		m_combos[MOTOR_INV_S]->SetTxt( dta.start_inv );
		m_combos[MOTOR_INV_D]->SetTxt( dta.end_inv );
		m_combos[MOTOR_WAIT]->SetTxt( dta.wait );
	}

	void onEdit()
	{
		struct CarTimeData dta;
		strncpy( dta.name, m_combos[FEEDING_TYPE]->GetTxt(), sizeof(dta.name) );
		
		dta.start_ele = m_combos[FEEDER_COIL_S]->GetInt();
		dta.end_ele = m_combos[FEEDER_COIL_D]->GetInt();
		dta.start_motor = m_combos[MOTOR_ADV_S]->GetInt();
		dta.end_motor = m_combos[MOTOR_ADV_D]->GetInt();
		dta.start_inv = m_combos[MOTOR_INV_S]->GetInt();
		dta.end_inv = m_combos[MOTOR_INV_D]->GetInt();
		dta.wait = m_combos[MOTOR_WAIT]->GetInt();
		
		CarTime_Write( dta, cur_rec );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_PAGEDOWN:
				if( cur_rec == CarTime_NRec()-1 )
					bipbip();
				else
					cur_rec++;
				return true;

			case K_PAGEUP:
				if( cur_rec == 0 )
					bipbip();
				else
					cur_rec--;
				return true;

			default:
				break;
		}
	
		return false;
	}

	int cur_rec;
};

int fn_FeederTimings( CWindow* parent )
{
	if( !CarTime_Open() )
	{
		return 0;
	}
	
	FeederTimingsUI win( parent );
	win.Show();
	win.Hide();
	
	make_car();

	CarTime_Close();

	return 1;
}


//---------------------------------------------------------------------------
// finestra: External camera advanced parameters
//---------------------------------------------------------------------------
class ExtCameraAdvancedParamsUI : public CWindowParams
{
public:
	ExtCameraAdvancedParamsUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 56, 7 );
		SetTitle( MsgGetString(Msg_05087) );

		CCenteringReservedParameters::inst().readData( m_data );
	}

	typedef enum
	{
		D1,
		D2,
		Z1,
		Z2,
		Z3,
		ZR
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[D1] = new C_Combo(  3, 1, MsgGetString(Msg_05081), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[Z1] = new C_Combo( 46, 1, "", 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[D2] = new C_Combo(  3, 2, MsgGetString(Msg_05082), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[Z2] = new C_Combo( 46, 2, "", 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[Z3] = new C_Combo(  3, 3, MsgGetString(Msg_05083), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[ZR] = new C_Combo(  3, 4, MsgGetString(Msg_05084), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[D1]->SetVMinMax( 1.0f, 60.0f );
		m_combos[D2]->SetVMinMax( 1.0f, 60.0f );
		m_combos[Z1]->SetVMinMax( -4.0f, 10.0f );
		m_combos[Z2]->SetVMinMax( -4.0f, 10.0f );
		m_combos[Z3]->SetVMinMax( -4.0f, 10.0f );
		m_combos[ZR]->SetVMinMax( -4.0f, 10.0f );

		// add to combo list
		m_comboList->Add( m_combos[D1], 0, 0 );
		m_comboList->Add( m_combos[Z1], 0, 1 );
		m_comboList->Add( m_combos[D2], 1, 0 );
		m_comboList->Add( m_combos[Z2], 1, 1 );
		m_comboList->Add( m_combos[Z3], 2, 1 );
		m_comboList->Add( m_combos[ZR], 3, 1 );
	}

	void onRefresh()
	{
		m_combos[D1]->SetTxt( m_data.opt_cent_d1 );
		m_combos[D2]->SetTxt( m_data.opt_cent_d2 );
		m_combos[Z1]->SetTxt( m_data.opt_cent_z1 );
		m_combos[Z2]->SetTxt( m_data.opt_cent_z2 );
		m_combos[Z3]->SetTxt( m_data.opt_cent_z3 );
		m_combos[ZR]->SetTxt( m_data.opt_cent_zrot );
	}

	void onEdit()
	{
		m_data.opt_cent_d1 = m_combos[D1]->GetFloat();
		m_data.opt_cent_d2 = m_combos[D2]->GetFloat();
		m_data.opt_cent_z1 = m_combos[Z1]->GetFloat();
		m_data.opt_cent_z2 = m_combos[Z2]->GetFloat();
		m_data.opt_cent_z3 = m_combos[Z3]->GetFloat();
		m_data.opt_cent_zrot = m_combos[ZR]->GetFloat();
	}

	void onClose()
	{
		CCenteringReservedParameters::inst().writeData( m_data );
	}

	CenteringReservedParameters m_data;
};


int fn_ExtCameraAdvancedParams( CWindow* parent )
{
	ExtCameraAdvancedParamsUI win( parent );
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Other advanced parameters
//---------------------------------------------------------------------------
class OtherAdvancedParamsUI : public CWindowParams
{
public:
	OtherAdvancedParamsUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 84, 21 );
		SetTitle( MsgGetString(Msg_00128) );
	}

	typedef enum
	{
		ENC_1,
		ENC_2,
		Z_STEPS_1,
		Z_STEPS_2,
		WORK_PL_Z,
		FEEDER_Z,
		TOOLS_Z_1,
		TOOLS_Z_2,
		INK_Z,
		DELTA_Z,
		INTER_Z,
		INTER_F_Z,
		DEBUG,
		OUT_Z,
		THETA_P_1,
		THETA_P_2,
		POLES_1,
		POLES_2,
		MAX_CUR,
		MAX_TIME,
		THETA_Z_1,
		THETA_Z_2,
		THETA_O_1,
		THETA_O_2,
		V_DELAY,
		UGE_Z_S,
		UGE_Z_V,
		UGE_Z_A,
		UGE_TIME,
		MAX_P1,
		MIN_P1,
		MAX_P2,
		MIN_P2
	} combo_labels;

protected:
	void onInit()
	{
		char buf[80];
		const char* Encoder_StrVect[]={"2048","4096"};
		const char* CurrMax_StrVect[]={"100","200","400","600","800","1000","1200","1400"};
		const char* CurrTime_StrVect[]={"150","300","600","1250","2500","5000"};
		const char* BrushlessNPoles_StrVect[] = { "2","4" };

		// create combos
		m_combos[ENC_1]     = new C_Combo(  3,  1, MsgGetString(Msg_00825), 6, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[ENC_2]     = new C_Combo(  3,  2, MsgGetString(Msg_00826), 6, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[Z_STEPS_1] = new C_Combo(  3,  3, MsgGetString(Msg_00827), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[Z_STEPS_2] = new C_Combo(  3,  4, MsgGetString(Msg_00828), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );

		m_combos[WORK_PL_Z] = new C_Combo(  3,  6, MsgGetString(Msg_00829), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[FEEDER_Z]  = new C_Combo(  3,  7, MsgGetString(Msg_00830), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOOLS_Z_1] = new C_Combo(  3,  8, MsgGetString(Msg_00831), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOOLS_Z_2] = new C_Combo(  3,  9, MsgGetString(Msg_00812), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[INK_Z]     = new C_Combo(  9, 10, MsgGetString(Msg_00832), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		#ifdef __SNIPER
		m_combos[DELTA_Z]   = new C_Combo(  3, 11, MsgGetString(Msg_00838), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		#endif

		m_combos[INTER_Z]   = new C_Combo(  3, 13, MsgGetString(Msg_01128), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[INTER_F_Z] = new C_Combo(  3, 14, MsgGetString(Msg_01129), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[DEBUG]     = new C_Combo(  3, 15, MsgGetString(Msg_00960), 6, CELL_TYPE_UINT );
		m_combos[OUT_Z]     = new C_Combo(  3, 16, MsgGetString(Msg_00843), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[THETA_P_1] = new C_Combo(  3, 17, MsgGetString(Msg_00868), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[THETA_P_2] = new C_Combo(  3, 18, MsgGetString(Msg_00869), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		snprintf( buf, sizeof(buf), MsgGetString(Msg_05175), 1 );
		m_combos[POLES_1]   = new C_Combo( 47,  1, buf, 6, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		snprintf( buf, sizeof(buf), MsgGetString(Msg_05175), 2 );
		m_combos[POLES_2]   = new C_Combo( 47,  2, buf, 6, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[MAX_CUR]   = new C_Combo( 47,  3, MsgGetString(Msg_00849), 6, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
		m_combos[MAX_TIME]  = new C_Combo( 47,  4, MsgGetString(Msg_00850), 6, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );

		m_combos[THETA_Z_1] = new C_Combo( 47,  6, MsgGetString(Msg_00873), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[THETA_Z_2] = new C_Combo( 47,  7, MsgGetString(Msg_00874), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[THETA_O_1] = new C_Combo( 47,  8, MsgGetString(Msg_05011), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[THETA_O_2] = new C_Combo( 47,  9, MsgGetString(Msg_05012), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		#ifdef __DISP2
		m_combos[V_DELAY]   = new C_Combo( 47, 10, MsgGetString(Msg_00950), 6, CELL_TYPE_UINT );
		#endif
		m_combos[UGE_Z_S]   = new C_Combo( 47, 11, MsgGetString(Msg_00711), 6, CELL_TYPE_UINT );
		m_combos[UGE_Z_V]   = new C_Combo( 47, 12, MsgGetString(Msg_01019), 6, CELL_TYPE_UINT );
		m_combos[UGE_Z_A]   = new C_Combo( 47, 13, MsgGetString(Msg_01020), 6, CELL_TYPE_UINT );
		m_combos[UGE_TIME]  = new C_Combo( 47, 14, MsgGetString(Msg_01021), 6, CELL_TYPE_UINT );

		m_combos[MAX_P1]    = new C_Combo( 47, 16, MsgGetString(Msg_01200), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[MIN_P1]    = new C_Combo( 47, 17, MsgGetString(Msg_01201), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[MAX_P2]    = new C_Combo( 47, 18, MsgGetString(Msg_01232), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[MIN_P2]    = new C_Combo( 47, 19, MsgGetString(Msg_01233), 6, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[ENC_1]->SetLegalStrings( 2, (char**)Encoder_StrVect );
		m_combos[ENC_2]->SetLegalStrings( 2, (char**)Encoder_StrVect );
		m_combos[Z_STEPS_1]->SetVMinMax( 0.f, 700.f );
		m_combos[Z_STEPS_2]->SetVMinMax( 0.f, 700.f );

		m_combos[WORK_PL_Z]->SetVMinMax( 0.f, 50.f );
		m_combos[FEEDER_Z]->SetVMinMax( 0.f, 50.f );
		m_combos[TOOLS_Z_1]->SetVMinMax( 0.f, 50.f );
		m_combos[TOOLS_Z_2]->SetVMinMax( 0.f, 50.f );
		m_combos[INK_Z]->SetVMinMax( 0.f, 40.f );
		#ifdef __SNIPER
		m_combos[DELTA_Z]->SetVMinMax( -10.f, 10.f );
		#endif

		m_combos[INTER_Z]->SetVMinMax( -360.f, 360.f ); //TODO ???
		m_combos[INTER_F_Z]->SetVMinMax( -360.f, 360.f ); //TODO ???
		m_combos[DEBUG]->SetVMinMax( 0, 15 );
		m_combos[OUT_Z]->SetVMinMax( -10.f, 10.f );
		m_combos[THETA_P_1]->SetVMinMax( -360.f, 360.f );
		m_combos[THETA_P_2]->SetVMinMax( -360.f, 360.f );

		m_combos[POLES_1]->SetLegalStrings( 2, (char**)BrushlessNPoles_StrVect );
		m_combos[POLES_2]->SetLegalStrings( 2, (char**)BrushlessNPoles_StrVect );
		m_combos[MAX_CUR]->SetLegalStrings( 8, (char**)CurrMax_StrVect );
		m_combos[MAX_TIME]->SetLegalStrings( 6, (char**)CurrTime_StrVect );

		m_combos[THETA_Z_1]->SetVMinMax( 0.f, 50.f );
		m_combos[THETA_Z_2]->SetVMinMax( 0.f, 50.f );
		m_combos[THETA_O_1]->SetVMinMax( -360.f, 360.f );
		m_combos[THETA_O_2]->SetVMinMax( -360.f, 360.f );
		#ifdef __DISP2
		m_combos[V_DELAY]->SetVMinMax( 0, 10000 );
		#endif
		m_combos[UGE_Z_S]->SetVMinMax( 5, QHeader.zMaxSpeed );
		m_combos[UGE_Z_V]->SetVMinMax( 5, QHeader.zMaxSpeed );
		m_combos[UGE_Z_A]->SetVMinMax( 10, QHeader.zMaxAcc );
		m_combos[UGE_TIME]->SetVMinMax( 0, 9999 );

		m_combos[MAX_P1]->SetVMinMax( 0.f, 99.f );
		m_combos[MIN_P1]->SetVMinMax( -99.f, 0.f );
		m_combos[MAX_P2]->SetVMinMax( 0.f, 99.f );
		m_combos[MIN_P2]->SetVMinMax( -99.f, 0.f );

		// add to combo list
		m_comboList->Add( m_combos[ENC_1]    ,  0, 0 );
		m_comboList->Add( m_combos[ENC_2]    ,  1, 0 );
		m_comboList->Add( m_combos[Z_STEPS_1],  2, 0 );
		m_comboList->Add( m_combos[Z_STEPS_2],  3, 0 );

		m_comboList->Add( m_combos[WORK_PL_Z],  5, 0 );
		m_comboList->Add( m_combos[FEEDER_Z] ,  6, 0 );
		m_comboList->Add( m_combos[TOOLS_Z_1],  7, 0 );
		m_comboList->Add( m_combos[TOOLS_Z_2],  8, 0 );
		m_comboList->Add( m_combos[INK_Z]    ,  9, 0 );
		#ifdef __SNIPER
		m_comboList->Add( m_combos[DELTA_Z]  , 10, 0 );
		#endif

		m_comboList->Add( m_combos[INTER_Z]  , 12, 0 );
		m_comboList->Add( m_combos[INTER_F_Z], 13, 0 );
		m_comboList->Add( m_combos[DEBUG]    , 14, 0 );
		m_comboList->Add( m_combos[OUT_Z]    , 15, 0 );
		m_comboList->Add( m_combos[THETA_P_1], 16, 0 );
		m_comboList->Add( m_combos[THETA_P_2], 17, 0 );

		m_comboList->Add( m_combos[POLES_1]  ,  0, 1 );
		m_comboList->Add( m_combos[POLES_2]  ,  1, 1 );
		m_comboList->Add( m_combos[MAX_CUR]  ,  2, 1 );
		m_comboList->Add( m_combos[MAX_TIME] ,  3, 1 );

		m_comboList->Add( m_combos[THETA_Z_1],  5, 1 );
		m_comboList->Add( m_combos[THETA_Z_2],  6, 1 );
		m_comboList->Add( m_combos[THETA_O_1],  7, 1 );
		m_comboList->Add( m_combos[THETA_O_2],  8, 1 );
		#ifdef __DISP2
		m_comboList->Add( m_combos[V_DELAY]  ,  9, 1 );
		#endif
		m_comboList->Add( m_combos[UGE_Z_S]  , 10, 1 );
		m_comboList->Add( m_combos[UGE_Z_V]  , 11, 1 );
		m_comboList->Add( m_combos[UGE_Z_A]  , 12, 1 );
		m_comboList->Add( m_combos[UGE_TIME] , 13, 1 );

		m_comboList->Add( m_combos[MAX_P1]   , 15, 1 );
		m_comboList->Add( m_combos[MIN_P1]   , 16, 1 );
		m_comboList->Add( m_combos[MAX_P2]   , 17, 1 );
		m_comboList->Add( m_combos[MIN_P2]   , 18, 1 );
	}

	void onRefresh()
	{
		m_combos[ENC_1]->SetStrings_Pos( QHeader.Enc_step1 == 2048 ? 0 : 1 );
		m_combos[ENC_2]->SetStrings_Pos( QHeader.Enc_step2 == 2048 ? 0 : 1 );
		m_combos[Z_STEPS_1]->SetTxt( QHeader.Step_Trasl1 );
		m_combos[Z_STEPS_2]->SetTxt( QHeader.Step_Trasl2 );

		m_combos[WORK_PL_Z]->SetTxt( QHeader.Zero_Piano );
		m_combos[FEEDER_Z]->SetTxt( QHeader.Zero_Caric );
		m_combos[TOOLS_Z_1]->SetTxt( QHeader.Zero_Ugelli );
		m_combos[TOOLS_Z_2]->SetTxt( QHeader.Zero_Ugelli2 );
		m_combos[INK_Z]->SetTxt( QHeader.InkZPos );
		#ifdef __SNIPER
		m_combos[DELTA_Z]->SetTxt( QHeader.Z12_Zero_delta );
		#endif

		m_combos[INTER_Z]->SetTxt( QHeader.interfNorm );
		m_combos[INTER_F_Z]->SetTxt( QHeader.interfFine );
		m_combos[DEBUG]->SetTxt( int((QHeader.modal & DEBUGVAL_MASK) >> 12) );
		m_combos[OUT_Z]->SetTxt( QHeader.LaserOut );
		m_combos[THETA_P_1]->SetTxt( QHeader.thoff_uge1 );
		m_combos[THETA_P_2]->SetTxt( QHeader.thoff_uge2 );

		m_combos[POLES_1]->SetStrings_Pos( int(QHeader.brushlessNPoles[0]) == 2 ? 0 : 1 );
		m_combos[POLES_2]->SetStrings_Pos( int(QHeader.brushlessNPoles[1]) == 2 ? 0 : 1 );
		m_combos[MAX_CUR]->SetStrings_Pos( QHeader.brushMaxCurrent );
		m_combos[MAX_TIME]->SetStrings_Pos( QHeader.brushMaxCurrTime );

		m_combos[THETA_Z_1]->SetTxt( QHeader.zth_level1 );
		m_combos[THETA_Z_2]->SetTxt( QHeader.zth_level2 );
		m_combos[THETA_O_1]->SetTxt( QHeader.sniper1_thoffset );
		m_combos[THETA_O_2]->SetTxt( QHeader.sniper2_thoffset );
		#ifdef __DISP2
		m_combos[V_DELAY]->SetTxt( QHeader.DispVacuoGen_Delay );
		#endif
		m_combos[UGE_Z_S]->SetTxt( QHeader.uge_zmin );
		m_combos[UGE_Z_V]->SetTxt( QHeader.uge_zvel );
		m_combos[UGE_Z_A]->SetTxt( QHeader.uge_zacc );
		m_combos[UGE_TIME]->SetTxt( QHeader.uge_wait );

		m_combos[MAX_P1]->SetTxt( QHeader.Max_NozHeight[0] );
		m_combos[MIN_P1]->SetTxt( QHeader.Min_NozHeight[0] );
		m_combos[MAX_P2]->SetTxt( QHeader.Max_NozHeight[1] );
		m_combos[MIN_P2]->SetTxt( QHeader.Min_NozHeight[1] );
	}

	void onEdit()
	{
		QHeader.Enc_step1 = m_combos[ENC_1]->GetInt();
		QHeader.Enc_step2 = m_combos[ENC_2]->GetInt();
		QHeader.Step_Trasl1 = m_combos[Z_STEPS_1]->GetFloat();
		QHeader.Step_Trasl2 = m_combos[Z_STEPS_2]->GetFloat();

		QHeader.Zero_Piano = m_combos[WORK_PL_Z]->GetFloat();
		QHeader.Zero_Caric = m_combos[FEEDER_Z]->GetFloat();
		QHeader.Zero_Ugelli = m_combos[TOOLS_Z_1]->GetFloat();
		QHeader.Zero_Ugelli2 = m_combos[TOOLS_Z_2]->GetFloat();
		QHeader.InkZPos = m_combos[INK_Z]->GetFloat();
		#ifdef __SNIPER
		QHeader.Z12_Zero_delta = m_combos[DELTA_Z]->GetFloat();
		#endif

		QHeader.interfNorm = m_combos[INTER_Z]->GetFloat();
		QHeader.interfFine = m_combos[INTER_F_Z]->GetFloat();
		QHeader.modal = ((QHeader.modal & ~DEBUGVAL_MASK) | ((m_combos[DEBUG]->GetInt() << 12) & DEBUGVAL_MASK));
		QHeader.LaserOut = m_combos[OUT_Z]->GetFloat();
		QHeader.thoff_uge1 = m_combos[THETA_P_1]->GetFloat();
		QHeader.thoff_uge2 = m_combos[THETA_P_2]->GetFloat();

		QHeader.brushlessNPoles[0] = m_combos[POLES_1]->GetInt();
		QHeader.brushlessNPoles[1] = m_combos[POLES_2]->GetInt();
		QHeader.brushMaxCurrent = m_combos[MAX_CUR]->GetStrings_Pos();
		QHeader.brushMaxCurrTime = m_combos[MAX_TIME]->GetStrings_Pos();

		QHeader.zth_level1 = m_combos[THETA_Z_1]->GetFloat();
		QHeader.zth_level2 = m_combos[THETA_Z_2]->GetFloat();
		QHeader.sniper1_thoffset = m_combos[THETA_O_1]->GetFloat();
		QHeader.sniper2_thoffset = m_combos[THETA_O_2]->GetFloat();
		#ifdef __DISP2
		QHeader.DispVacuoGen_Delay = m_combos[V_DELAY]->GetInt();
		#endif
		QHeader.uge_zmin = m_combos[UGE_Z_S]->GetInt();
		QHeader.uge_zvel = m_combos[UGE_Z_V]->GetInt();
		QHeader.uge_zacc = m_combos[UGE_Z_A]->GetInt();
		QHeader.uge_wait = m_combos[UGE_TIME]->GetInt();

		QHeader.Max_NozHeight[0] = m_combos[MAX_P1]->GetFloat();
		QHeader.Min_NozHeight[0] = m_combos[MIN_P1]->GetFloat();
		QHeader.Max_NozHeight[1] = m_combos[MAX_P2]->GetFloat();
		QHeader.Min_NozHeight[1] = m_combos[MIN_P2]->GetFloat();
	}

	void onClose()
	{
		Mod_Cfg( QHeader );
	}
};


int fn_OtherAdvancedParams( CWindow* parent )
{
	OtherAdvancedParamsUI win( parent );
	win.Show();
	win.Hide();

	return 1;
}

