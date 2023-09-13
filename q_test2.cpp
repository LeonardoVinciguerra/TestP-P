/*
>>>> Q_TEST2.CPP

Gestione dei test hardware CARICATORI.
Gestione dei test hardware CAMBIO UGELLO.
Gestione dei test hardware MOVIMENTO ASSI.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++    Modificato da TWS Simone 06.08.96  - W042000
++++    Modificato da LCM Simone 08.08.01  - S080801 >> (user int. a classi)

*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "msglist.h"
#include "q_cost.h"
#include "q_packages.h"
#include "q_graph.h"
#include "q_gener.h"
#include "q_help.h"
#include "q_tabe.h"
#include "q_wind.h"
#include "q_param.h"
#include "q_conf.h"
#include "q_files.h"
#include "q_oper.h"
#include "q_test2.h"
#include "q_grcol.h"
#include "q_feedersel.h"
#include "q_ugeobj.h"

#ifdef __SNIPER
#include "tws_sniper.h"
#endif

#include "q_fox.h"
#include "q_assem.h"
#include "q_opert.h"
#include "strutils.h"

#include "lnxdefs.h"
#include "keyutils.h"
#include "datetime.h"
#include "q_vision.h"
#include <time.h>

#include "gui_defs.h"
#include "c_win_par.h"
#include "c_pan.h"

#include "centering_thread.h"
#include "q_files_new.h"

#ifdef __USE_MOTORHEAD
#include "tws_brushless.h"
#include "motorhead.h"
#endif

#include "q_conveyor.h"

#include <mss.h>


#define WAITCARIC_TEST	100


extern SMachineInfo MachineInfo;
extern SPackageData currentLibPackages[MAXPACK];

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;

//Struttura parametri test hw centraggio - DANY191102
struct CentTestParam centtest_param;


char TClass_StrVect[]={"LMCD"};

extern struct CfgParam QParam;
extern struct CfgHeader QHeader;
extern struct cur_data CurDat;
extern FeederFile* CarFile;
FeederFile* feederFile;


//-----------------------------------------------------------------------
// Test caricatori

const char* CarAvType_StrVect_Domes[] = {"C","M","L","D","C1","M1","L1","D1","DM"};
//record corrente caricatori
int cartest_nrec = 0;

// codici vassoi - test non implementato !
#define MAXCAR_TEST          15*8

struct NewCaric
{
	int K_Ccode;          // Cod. caricatore
	int K_Ctempo;         // Tempo di avanzamento (C/M/L)
	int K_selez;          // Flag di selez. per sequenza

}  NN_Caric[MAXCAR];


// Setta il campo codice caricatore con relativa combin. ( car.+ rot) ).
// Pacchetto 1/15  - Caric. 1/8
void Set_cc_car()
{
	int contacar = 0;
	for(int ploop = 1; ploop <= 15; ploop++ )
	{
		for(int cloop = 1; cloop <= 8; cloop++ )
		{
			NN_Caric[contacar].K_Ccode = (ploop*10)+cloop;
			NN_Caric[contacar].K_Ctempo = 0;
			NN_Caric[contacar].K_selez = 0;
			contacar++;
		}
	}
}


// Sequenza caricatori
int KCAR_seq(void)
{
	Pan_WaitTest(1);
	
	FeederClass* feederClass = new FeederClass( feederFile );

	// toglie temporanamente DemoMode altrimenti la routine di avanzamento non viene eseguita
	int tmpDemoMode = QParam.DemoMode;
	QParam.DemoMode = 0;

	flushKeyboardBuffer();

	while(1)
	{
		for( int sloop = 0; sloop <= 119; sloop++ )
		{
			if( keyRead() )
			{
				Pan_WaitTest(0);
				delete feederClass;
				QParam.DemoMode=tmpDemoMode; //SMOD250703
				return(0);
			}
		
			if(NN_Caric[sloop].K_selez)
			{
				feederClass->SetRec(sloop);
				feederClass->DecNComp();
				feederClass->Avanza(NN_Caric[sloop].K_Ctempo,1);
				feederClass->WaitReady();
		
				delay(WAITCARIC_TEST);
			}
		}
	}
	
	QParam.DemoMode = tmpDemoMode;
}

void KCAR_Enter()
{
	// toglie temporanamente DemoMode altrimenti la routine di avanzamento non viene eseguita
	int tmpDemoMode = QParam.DemoMode;
	QParam.DemoMode = 0;

	FeederClass* feederClass = new FeederClass( feederFile );
	feederClass->SetRec(cartest_nrec);
	feederClass->DecNComp();
	feederClass->Avanza(NN_Caric[cartest_nrec].K_Ctempo,1);
	feederClass->WaitReady();
	delete feederClass;

	QParam.DemoMode = tmpDemoMode;
}


//---------------------------------------------------------------------------
// finestra: Feeder test
//---------------------------------------------------------------------------
class FeederTestUI : public CWindowParams
{
public:
	FeederTestUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 60, 14 );
		SetTitle( MsgGetString(Msg_00148) );

		Set_cc_car();
	}

	typedef enum
	{
		FDR_CODE,
		ACT_TIME,
		SEL_SEQ
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[FDR_CODE] = new C_Combo( 8, 1, MsgGetString(Msg_00149), 3, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[ACT_TIME] = new C_Combo( 8, 3, MsgGetString(Msg_00150), 2, CELL_TYPE_TEXT );
		m_combos[SEL_SEQ]  = new C_Combo( 8, 4, MsgGetString(Msg_00151), 4, CELL_TYPE_YN );

		// set params
		#ifndef __DOME_FEEDER
		m_combos[ACT_TIME]->SetLegalStrings( 8, (char **)CarAvType_StrVect );
		#else
		m_combos[ACT_TIME]->SetLegalStrings( 9, (char **)CarAvType_StrVect_Domes );
		#endif

		// add to combo list
		m_comboList->Add( m_combos[FDR_CODE], 0, 0 );
		m_comboList->Add( m_combos[ACT_TIME], 1, 0 );
		m_comboList->Add( m_combos[SEL_SEQ],  2, 0 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 7, GetW()/GUI_CharW() - 4, 6 ) );
		DrawTextCentered( 8, MsgGetString(Msg_00323), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
		DrawTextCentered( 9, MsgGetString(Msg_00319), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
		DrawTextCentered( 10, MsgGetString(Msg_01478), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	}

	void onRefresh()
	{
		m_combos[FDR_CODE]->SetTxt( NN_Caric[cartest_nrec].K_Ccode );
		m_combos[ACT_TIME]->SetStrings_Pos( NN_Caric[cartest_nrec].K_Ctempo );
		m_combos[SEL_SEQ]->SetTxtYN( NN_Caric[cartest_nrec].K_selez );
	}

	void onEdit()
	{
		NN_Caric[cartest_nrec].K_Ctempo = m_combos[ACT_TIME]->GetStrings_Pos();
		NN_Caric[cartest_nrec].K_selez = m_combos[SEL_SEQ]->GetYN();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00455), K_F3, 0, NULL, KCAR_seq ); // Esecuzione in sequenza
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				KCAR_seq();
				return true;

			case K_CTRL_PAGEUP:
				if( cartest_nrec < MAXNREC_FEED )
					cartest_nrec -= 7;
			case K_PAGEUP:
				if( cartest_nrec > 0 )
					cartest_nrec--;
				else
					cartest_nrec = MAXCAR_TEST-1;
				return true;

			case K_CTRL_PAGEDOWN:
				if( cartest_nrec < MAXNREC_FEED )
					cartest_nrec += 7;
			case K_PAGEDOWN:
				if( cartest_nrec < MAXCAR_TEST-1 )
					cartest_nrec++;
				else
					cartest_nrec = 0;
				return true;

			case K_ENTER:
				KCAR_Enter();
				return true;

			default:
				break;
		}

		return false;
	}
};

int fn_FeederTest()
{
	feederFile = new FeederFile( QHeader.Conf_Default );
	if( !feederFile->opened )
	{
		delete feederFile;
		feederFile = NULL;
	}

	FeederTestUI win;
	win.Show();
	win.Hide();

	delete feederFile;
	feederFile = NULL;

	return 1;
}

// Fine test caricatori
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Test cambio ugello >> S080801

int UG_punta;  // punta cambio ugello
int UG_prel[2];   // ugello da prelevare


void SelectNextNozzle()
{
	UG_prel[UG_punta-1] = Ugelli->GetInUse( UG_punta );

	int start = UG_prel[UG_punta-1];
	if( start == -1 )
	{
		start = 'A';
	}

	CfgUgelli nozzleData;

	do
	{
		if( UG_prel[UG_punta-1] == -1 || UG_prel[UG_punta-1] == 'Z' )
		{
			UG_prel[UG_punta-1] = 'A';
		}
		else
		{
			UG_prel[UG_punta-1]++;
		}

		if( UG_prel[UG_punta-1] == start )
		{
			bipbip();
			break;
		}

		Ugelli->ReadRec( nozzleData, UG_prel[UG_punta-1]-'A' );

	} while( (nozzleData.NozzleAllowed & UG_punta) == 0 );
}

void ExecuteNozzleChange()
{
	if( UG_prel[UG_punta-1] != -1 )
	{
		CfgUgelli nozzleData;
		Ugelli->ReadRec( nozzleData, UG_prel[UG_punta-1]-'A' );

		if( (nozzleData.NozzleAllowed & UG_punta) == 0 )
		{
			bipbip();
			return;
		}
	}

	if( UG_punta == 1 )
		Ugelli->SetFlag(1,0);
	else
		Ugelli->SetFlag(0,1);

	if( UG_punta == 1 && UG_prel[0] == -1 )
	{
		Ugelli->Depo(1);
	}
	else if( UG_punta == 2 && UG_prel[1] == -1 )
	{
		Ugelli->Depo(2);
	}
	else
	{
		Ugelli->DoOper( UG_prel );
	}

	SelectNextNozzle();
}


//---------------------------------------------------------------------------
// finestra: Tool change test
//---------------------------------------------------------------------------
class ToolChangeTestUI : public CWindowParams
{
public:
	ToolChangeTestUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU | WIN_STYLE_EDITMODE_ON );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 54, 10 );

		char buf[80];
		snprintf( buf, 80, MsgGetString(Msg_00152), UG_punta );
		SetTitle( buf );
	}

	typedef enum
	{
		TOOL_PLACE,
		TOOL_PICK
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[TOOL_PLACE] = new C_Combo( 15, 1, MsgGetString(Msg_00154), 1, CELL_TYPE_TEXT , CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[TOOL_PICK]  = new C_Combo( 15, 2, MsgGetString(Msg_00155), 1, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE );

		// set params
		m_combos[TOOL_PICK]->SetLegalChars( "*" CHARSET_TOOL );

		// add to combo list
		m_comboList->Add( m_combos[TOOL_PLACE], 0, 0 );
		m_comboList->Add( m_combos[TOOL_PICK],  1, 0 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 5, GetW()/GUI_CharW() - 4, 4 ) );
		DrawTextCentered( 6, MsgGetString(Msg_00318), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
		DrawTextCentered( 7, MsgGetString(Msg_00323), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	void onRefresh()
	{
		if( Ugelli->GetInUse( UG_punta ) == -1 )
			m_combos[TOOL_PLACE]->SetTxt( '*' );
		else
			m_combos[TOOL_PLACE]->SetTxt( (char)Ugelli->GetInUse( UG_punta ) );

		if( UG_prel[UG_punta-1] == -1 )
			m_combos[TOOL_PICK]->SetTxt( '*' );
		else
			m_combos[TOOL_PICK]->SetTxt( (char)UG_prel[UG_punta-1] );
	}

	void onEdit()
	{
		if( m_combos[TOOL_PICK]->GetChar() == '*' )
			UG_prel[UG_punta-1] = -1;
		else
			UG_prel[UG_punta-1] = m_combos[TOOL_PICK]->GetChar();
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_PAGEUP:
				if( UG_punta > 1 )
				{
					UG_punta--;

					char buf[80];
					snprintf( buf, 79, MsgGetString(Msg_00152), UG_punta );
					SetTitle( buf );
					SelectNextNozzle();
					return true;
				}
				break;

			case K_PAGEDOWN:
				if( UG_punta < 2 )
				{
					UG_punta++;

					char buf[80];
					snprintf( buf, 79, MsgGetString(Msg_00152), UG_punta );
					SetTitle( buf );
					SelectNextNozzle();
					return true;
				}
				break;

			case K_ENTER:
				ExecuteNozzleChange();
				return true;

			default:
				break;
		}

		return false;
	}
};

int fn_ToolChangeTest()
{
	UG_punta = 1;

	for( int i = 0; i < 2; i++ )
	{
		UG_prel[i] = Ugelli->GetInUse( UG_punta );

		if( UG_prel[i] == -1 || UG_prel[i] == 'Z' )
		{
			UG_prel[i] = 'A';
		}
		else
		{
			UG_prel[i]++;
		}
	}

	ToolChangeTestUI win;
	win.Show();
	win.Hide();

	return 1;
}

// Fine test cambio ugello
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Test movimento assi

SAxesMovement AxesMovement;

//-----------------------------------------------------------------------
// Resetta valori test movimento assi
//-----------------------------------------------------------------------
int MOV_reset()
{
	for( int i = 0; i < AXESMOVEMENT_LEN; i++ )
	{
		AxesMovement.speed[i] = QHeader.xyMaxSpeed;
		AxesMovement.pos[i].X = 0.f;
		AxesMovement.pos[i].Y = 0.f;
	}

	return 1;
}

//-----------------------------------------------------------------------
// Setta valori di default test movimento assi
//-----------------------------------------------------------------------
int MOV_set_default()
{
	AxesMovement.pos[0].X = 450.f;   AxesMovement.pos[0].Y =   0.f;   AxesMovement.speed[0] = QHeader.xyMaxSpeed;
	AxesMovement.pos[1].X =   0.f;   AxesMovement.pos[1].Y = 400.f;   AxesMovement.speed[1] = QHeader.xyMaxSpeed;
	AxesMovement.pos[2].X =   0.f;   AxesMovement.pos[2].Y =   0.f;   AxesMovement.speed[2] = QHeader.xyMaxSpeed;
	AxesMovement.pos[3].X = 400.f;   AxesMovement.pos[3].Y = 400.f;   AxesMovement.speed[3] = QHeader.xyMaxSpeed;
	AxesMovement.pos[4].X = 300.f;   AxesMovement.pos[4].Y =   0.f;   AxesMovement.speed[4] = QHeader.xyMaxSpeed;
	AxesMovement.pos[5].X =   0.f;   AxesMovement.pos[5].Y = 300.f;   AxesMovement.speed[5] = QHeader.xyMaxSpeed;

	return 1;
}

//-----------------------------------------------------------------------
// Carica valori test movimento assi da file
//-----------------------------------------------------------------------
int MOV_Load()
{
	AxesMovement_Read( AxesMovement );
	return 1;
}

//-----------------------------------------------------------------------
// Salva valori test movimento assi su file
//-----------------------------------------------------------------------
int MOV_Save()
{
	if( W_Deci( 1, AREYOUSURETXT ) )
		AxesMovement_Write( AxesMovement );

	return 1;
}

//---------------------------------------------------------------------------------
// displayInfoMOV
// Mostra messaggio in movimento assi
//---------------------------------------------------------------------------------
#define VIDEO_CALLBACK_Y      29

void displayInfoMOV( void* parentWin )
{
	GUI_Freeze_Locker lock;

	CWindow* parent = (CWindow*)parentWin;
	parent->DrawTextCentered( VIDEO_CALLBACK_Y + 2, MsgGetString(Msg_00338) );
	parent->DrawTextCentered( VIDEO_CALLBACK_Y + 4, MsgGetString(Msg_00339) );
}


// Sequenza movimento assi
int MOV_seq()
{
	// setta parametri telecamera
	SetImgBrightCont( CurDat.HeadBright, CurDat.HeadContrast );

	Set_Tv_Title( MsgGetString(Msg_00156) );
	Set_Tv( 1, CAMERA_HEAD, displayInfoMOV );

	Set_Finec(ON);	   // abilita protezione tramite finecorsa

	// measure machine WorkTime
	Timer timer;
	timer.start();

	//GF_TEMP
	ASSEMBLY_PROFILER_CLEAR();
	ASSEMBLY_PROFILER_START("Start TEST");

	int counter = 0;
	bool stop = false;

	while( !stop )
	{
		for( int sloop = 0; sloop < AXESMOVEMENT_LEN; sloop++ )
		{
			if( keyRead() )
			{
				stop = true;
				break;
			}

			PuntaXYAccSpeed( QHeader.xyMaxAcc, AxesMovement.speed[sloop] );

			ASSEMBLY_PROFILER_MEASURE("Movement [%d]", sloop);

			if( !NozzleXYMove( AxesMovement.pos[sloop].X, AxesMovement.pos[sloop].Y ) )
			{
				stop = true;
				break;
			}

			ASSEMBLY_PROFILER_MEASURE("MOVE");

			if( !Wait_PuntaXY() )
			{
				stop = true;
				break;
			}

			ASSEMBLY_PROFILER_MEASURE("WAIT");

			if( Get_OnFile() )
			{
				stop = true;
				break;
			}
		}

		counter++;
		if( counter == 10 )
		{
			counter = 0;
			// measure machine WorkTime
			timer.stop();
			MachineInfo.WorkTime += timer.getElapsedTimeInSec();
			MachineInfo_Write( MachineInfo );
			timer.start();
		}
	}
	
	ASSEMBLY_PROFILER_STOP();
	
	// measure machine WorkTime
	timer.stop();
	MachineInfo.WorkTime += timer.getElapsedTimeInSec();
	MachineInfo_Write( MachineInfo );

	Set_Finec(OFF);	   // disabilita protezione tramite finecorsa

	Set_Tv(0);
	return 1;
}

// funzione handler tasto Enter
void MOV_Enter( int row )
{
	Pan_WaitTest(1);

	Set_Finec(ON);  // abilita protezione tramite finecorsa

	PuntaXYAccSpeed( QHeader.xyMaxAcc, AxesMovement.speed[row] );
	NozzleXYMove( AxesMovement.pos[row].X, AxesMovement.pos[row].Y );
	Wait_PuntaXY();

	Set_Finec(OFF); // disabilita protezione tramite finecorsa

	Pan_WaitTest(0);
}


//---------------------------------------------------------------------------
// finestra: Axes move test
//---------------------------------------------------------------------------
class AxesMoveTestUI : public CWindowParams
{
public:
	AxesMoveTestUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 60, 14 );
		SetTitle( MsgGetString(Msg_00156) );
	}

protected:
	void onInit()
	{
		for( int i = 0; i < AXESMOVEMENT_LEN; i++ )
		{
			// create combos
			m_combos[i*3]   = new C_Combo( 14, i+2, "-> X:", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
			m_combos[1+i*3] = new C_Combo( 30, i+2, "Y:", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
			m_combos[2+i*3] = new C_Combo( 43, i+2, MsgGetString(Msg_00158), 7, CELL_TYPE_UINT );

			// set params
			m_combos[i*3]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
			m_combos[1+i*3]->SetVMinMax( QParam.LY_mincl, QParam.LY_maxcl );
			m_combos[2+i*3]->SetVMinMax( 100, QHeader.xyMaxSpeed );

			// add to combo list
			m_comboList->Add( m_combos[i*3],   i, 0 );
			m_comboList->Add( m_combos[1+i*3], i, 1 );
			m_comboList->Add( m_combos[2+i*3], i, 2 );
		}
	}

	void onShow()
	{
		for( int i = 0; i < AXESMOVEMENT_LEN; i++ )
		{
			DrawText( 2, i+2,  MsgGetString(Msg_00157) );
		}

		DrawPanel( RectI( 2, 10, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 11, MsgGetString(Msg_00323), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	void onRefresh()
	{
		for( int i = 0; i < AXESMOVEMENT_LEN; i++ )
		{
			m_combos[i*3]->SetTxt( AxesMovement.pos[i].X );
			m_combos[1+i*3]->SetTxt( AxesMovement.pos[i].Y );
			m_combos[2+i*3]->SetTxt( int(AxesMovement.speed[i]) );
		}
	}

	void onEdit()
	{
		for( int i = 0; i < AXESMOVEMENT_LEN; i++ )
		{
			AxesMovement.pos[i].X = m_combos[i*3]->GetFloat();
			AxesMovement.pos[i].Y = m_combos[1+i*3]->GetFloat();
			AxesMovement.speed[i]  = m_combos[2+i*3]->GetInt();
		}
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00455), K_F3,  0, NULL, MOV_seq );         // Apprendimento posizione caricatore
		m_menu->Add( MsgGetString(Msg_00249), K_F9,  0, NULL, MOV_Load );        // Load from file
		m_menu->Add( MsgGetString(Msg_00248), K_F10, 0, NULL, MOV_Save );        // Save to file
		m_menu->Add( MsgGetString(Msg_06020), K_F11, 0, NULL, MOV_set_default ); // Set to default
		m_menu->Add( MsgGetString(Msg_06021), K_F12, 0, NULL, MOV_reset );       // Reset
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				MOV_seq();
				return true;

			case K_F9:
				MOV_Load();
				return true;

			case K_F10:
				MOV_Save();
				return true;

			case K_F11:
				MOV_set_default();
				return true;

			case K_F12:
				MOV_reset();
				return true;

			case K_ENTER:
				MOV_Enter( m_comboList->GetCurRow() );
				return true;

			default:
				break;
		}

		return false;
	}
};

int fn_AxesMoveTest()
{
	MOV_reset();

	AxesMoveTestUI win;
	win.Show();
	win.Hide();

	return 1;
}


// Fine test movimento assi
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Test movimento convogliatore

float startConv, stopConv;

// Sequenza movimento convogliatore
int Conv_seq()
{
	if( !Get_UseConveyor() )
	{
		W_Mess( MsgGetString(Msg_00928) );
		return 0;
	}

	bool stop = false;

	CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00927) );

	while( !stop )
	{
		if( keyRead() )
		{
			stop = true;
			break;
		}

		MoveConveyorAndWait( startConv );

		if( keyRead() )
		{
			stop = true;
			break;
		}

		MoveConveyorAndWait( stopConv );
	}

	delete pan;

	return 1;
}

// Movimento convogliatore a start
int Conv_start()
{
	if( !Get_UseConveyor() )
	{
		W_Mess( MsgGetString(Msg_00928) );
		return 0;
	}

	CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00927) );

	MoveConveyorAndWait( startConv );

	delete pan;

	return 1;
}

// Movimento convogliatore a stop
int Conv_stop()
{
	if( !Get_UseConveyor() )
	{
		W_Mess( MsgGetString(Msg_00928) );
		return 0;
	}

	CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00927) );

	MoveConveyorAndWait( stopConv );

	delete pan;

	return 1;
}

//---------------------------------------------------------------------------
// finestra: Conveyor move test
//---------------------------------------------------------------------------
class ConveyorMoveTestUI : public CWindowParams
{
public:
	ConveyorMoveTestUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 40, 5 );
		SetTitle( MsgGetString(Msg_01000) );
	}

protected:
	void onInit()
	{
		struct conv_data convVal;
		Read_Conv( &convVal );

		// create combos
		m_combos[0] = new C_Combo( 3, 2, MsgGetString(Msg_00520), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[1] = new C_Combo( 21, 2, MsgGetString(Msg_00521), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[0]->SetVMinMax( convVal.minPos, convVal.maxPos );
		m_combos[1]->SetVMinMax( convVal.minPos, convVal.maxPos );

		// add to combo list
		m_comboList->Add( m_combos[0], 0, 0 );
		m_comboList->Add( m_combos[1], 0, 1 );

		startConv = convVal.minPos;
		stopConv  = convVal.maxPos;
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 8, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 9, MsgGetString(Msg_00323), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	void onRefresh()
	{
		m_combos[0]->SetTxt( startConv );
		m_combos[1]->SetTxt( stopConv );
	}

	void onEdit()
	{
		startConv = m_combos[0]->GetFloat();
		stopConv  = m_combos[1]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00455), K_F3,  0, NULL, Conv_seq );         // movimento sequenziale
		m_menu->Add( MsgGetString(Msg_01001), K_F4,  0, NULL, Conv_start );       // movimento a start
		m_menu->Add( MsgGetString(Msg_01002), K_F5,  0, NULL, Conv_stop );        // movimento a stop
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				Conv_seq();
				return true;

			case K_F4:
				Conv_start();
				return true;

			case K_F5:
				Conv_stop();
				return true;

			default:
				break;
		}

		return false;
	}
};

int fn_ConveyorTest()
{
	ConveyorMoveTestUI win;
	win.Show();
	win.Hide();

	return 1;
}


// Fine test convogliatore
//-----------------------------------------------------------------------

//------------------------------------------------------------------------
// Test centraggio
#define ENCBUFLEN 265

int ct_nozzle = 1;
class FeederClass* ct_feeder = 0;
int ct_feederSelected = 0;
int ct_pidCent = 0;
int ct_pidPlace = 0;
SPackageData ct_pack;
struct CentTestParam ct_param;
int ct_numPick = 1;
float ct_prelErrX = 0;
float ct_prelErrY = 0;

//---------------------------------------------------------------------------------
// displayInfoStart
// Mostra informazioni per funzioni: StartLas, StartCent
//---------------------------------------------------------------------------------
void displayInfoStart( void* parentWin )
{
	GUI_Freeze_Locker lock;

	CWindow* parent = (CWindow*)parentWin;

	parent->DrawText( 21, 30, MsgGetString(Msg_01239) );
	parent->DrawText( 21, 31, MsgGetString(Msg_01240) );
	parent->DrawText( 21, 32, MsgGetString(Msg_01241) );
	parent->DrawText( 21, 33, MsgGetString(Msg_01238) );
}

int CT_SelectFeeder()
{
	int carcode = 11;

	FeederSelect* TT_FSel = new FeederSelect( 0 );
	carcode = TT_FSel->Activate();
	delete TT_FSel;

	if( carcode > 0 )
	{
		bool isCarOk = true;
		FeederClass* new_car = new FeederClass( feederFile );
		new_car->SetCode(carcode);

		if(new_car->GetDataConstRef().C_comp[0]==0 || new_car->GetDataConstRef().C_Package[0]==0)
		{
			isCarOk = false;
			bipbip();
		}
		else
		{
			if( !new_car->Get_AssociatedPack(ct_pack) )
			{
				isCarOk = false;
				bipbip();
			}
		}

		if( isCarOk )
		{
			if( ct_feeder != NULL )
			{
				delete ct_feeder;
			}

			ct_feeder = new_car;

			ct_feederSelected = 1;
			ct_pidCent = ct_pack.centeringPID;
			ct_pidPlace = ct_pack.placementPID;

			centtest_param.cam_dx = 0;
			centtest_param.cam_dy = 0;
			return 1;
		}
		else
		{
			delete new_car;
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

int CT_StartTest()
{
	int other_noz = ct_nozzle ^ 3;
	struct CfgBrush brushPar;
	float xlas,ylas,zmmprel;
	int n_ugello[2];
	int c=0,depo=0;
	int idx;
	int perso=0,prel_flag=0;
	float errX=0, errY=0;

	if( !ct_feederSelected )
	{
		bipbip();
		return 0;
	}

	/*
	int noise_torque = 0;
	int noise_pos = 0;

	FoxHead->SetTorqueNoise(BRUSH1,noise_torque);
	FoxHead->SetPositionNoise(BRUSH1,noise_pos);

	FoxHead->SetTorqueNoise(BRUSH2,noise_torque);
	FoxHead->SetPositionNoise(BRUSH2,noise_pos);
	*/

	//setta parametri classe Ugelli
	if(ct_nozzle==1)
	{
		Ugelli->SetFlag(1,0);
	}
	else
	{
		Ugelli->SetFlag(0,1);
	}

	n_ugello[ct_nozzle-1] = ct_pack.tools[0];

	if(n_ugello[ct_nozzle-1]==0)
	{
		n_ugello[ct_nozzle-1]='A';
	}

	//cambia ugelli se necessario
	Ugelli->DoOper(n_ugello);

	if(ct_feeder->GetCode() < FIRSTTRAY)
	{
		zmmprel = GetZCaricPos(ct_nozzle) - ct_feeder->GetDataConstRef().C_offprel;
	}
	else
	{
		zmmprel = GetPianoZPos(ct_nozzle) - ct_feeder->GetDataConstRef().C_offprel;
	}

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	struct PrelStruct preldata = PRELSTRUCT_DEFAULT;

	for( int i = 0; i < ct_numPick; i++ )
	{
		#ifdef __SNIPER
		PuntaRotDeg(0,ct_nozzle);
		Wait_EncStop(ct_nozzle);
		ct_nozzle == 1 ? Sniper1->Zero_Cmd() : Sniper2->Zero_Cmd();
		#endif

		//attiva contropressione prima di prelevare il comp.
		Prepick_Contro(1);

		// Se non e' il primo ciclo, tolgo le correzioni precedenti...
		if( i != 0 )
		{
			ct_feeder->GetDataRef().C_xcar -= errX;
			ct_feeder->GetDataRef().C_ycar -= errY;
		}

		errX = ( (ct_numPick/2 - i)/(ct_numPick/2.0) ) * ct_prelErrX;
		errY = ( (ct_numPick/2 - i)/(ct_numPick/2.0) ) * ct_prelErrY;

		ct_feeder->GetDataRef().C_xcar += errX;
		ct_feeder->GetDataRef().C_ycar += errY;

		preldata.punta = ct_nozzle;
		preldata.stepF=NULL;
		preldata.zprel=zmmprel;
		preldata.zup=-(ct_pack.z/2 + ct_pack.snpZOffset);
		preldata.caric = ct_feeder;
		preldata.package = &ct_pack;

		perso=0;

		do
		{
			ct_feeder->GoPos( ct_nozzle );

			//SMOD300103
			PuntaRotDeg(Get_PickThetaPosition( ct_feeder->GetCode(), ct_pack), ct_nozzle);
			while( !Check_PuntaRot( ct_nozzle ) )
			{
				FoxPort->PauseLog();
			}
			FoxPort->RestartLog();

			Wait_PuntaXY();

			ct_feeder->WaitReady();

			Set_Finec(ON);
			PrelComp( preldata );
			Set_Finec(OFF);

			ct_feeder->DecNComp();
			ct_feeder->Avanza();

			if( ct_pack.checkPick == 'X' || (ct_pack.checkPick=='V' && !QParam.AT_vuoto) )
			{
				W_Mess( MsgGetString(Msg_00243) );
				break;
			}
			else
			{
				prel_flag = Check_CompIsOnNozzle( ct_nozzle, ct_pack.checkPick );
				if( prel_flag )
				{
					perso++;
					PuntaZSecurityPos(1);
					PuntaZSecurityPos(2);
					PuntaZPosWait(1);
					PuntaZPosWait(2);
					ScaricaComp( ct_nozzle );

					if(perso>=QParam.N_try)
					{
						if(!NotifyPrelError( ct_nozzle,prel_flag,ct_pack,""))
						{
							/*
							FoxHead->SetTorqueNoise(BRUSH1,0);
							FoxHead->SetPositionNoise(BRUSH1,0);

							FoxHead->SetTorqueNoise(BRUSH2,0);
							FoxHead->SetPositionNoise(BRUSH2,0);
							*/
							return 0;
						}
						else
						{
							perso=0;
						}
					}
				}
				else
				{
					perso=0;
					break;
				}
			}
		} while(1);

		//DANY171002
		//Setta i parametri del PID basandosi sui dai del package
		BrushDataRead( brushPar, ct_pidCent );

		if( ct_nozzle == 1 )
		{
			FoxHead->SetPID(BRUSH1,brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0]);
		}
		else
		{
			FoxHead->SetPID(BRUSH2,brushPar.p[1],brushPar.i[1],brushPar.d[1],brushPar.clip[1]);
		}

		//set speeds
		SetNozzleXYSpeed_Index( ct_pack.speedXY );
		SetNozzleZSpeed_Index( ct_nozzle, ct_pack.speedPick );
		SetNozzleRotSpeed_Index( ct_nozzle, ct_pack.speedRot );

		// setta parametri telecamera
		SetImgBrightCont( CurDat.HeadBright, CurDat.HeadContrast );

		SetExtCam_Shutter(2);
		SetExtCam_Gain(DEFAULT_EXT_CAM_GAIN);

		// salva parametri correnti
		int prevBright = GetImageBright();
		int prevContrast = GetImageContrast();


		// Attiva la camera ext
		Set_Tv_Title( MsgGetString(Msg_01242) );
		Set_Tv( 1, CAMERA_EXT, displayInfoStart );

		delay(500);

		do
		{
			Wait_EncStop( ct_nozzle );

			PuntaZPosMm(other_noz,QHeader.LaserOut);
			PuntaZPosWait(other_noz);

			CenteringResultData centeringResult;

			#ifdef __SNIPER
			StartCenteringThread();
			StartCentering( ct_nozzle, 0, &ct_pack );

			// avanza lo stato di centraggio sniper fino a quando non e' completato
			//--------------------------------------------------------------------------
			Timer timeoutTimer;
			timeoutTimer.start();

			while( !IsCenteringCompleted( ct_nozzle ) )
			{
				delay( 5 );

				if( timeoutTimer.getElapsedTimeInMilliSec() > WAITPUP_TIMEOUT )
				{
					W_Mess( "Error - Sniper Centering Timeout !");
					return 0;
				}
			}

			GetCenteringResult( ct_nozzle, centeringResult );

			StopCenteringThread();
			#endif

			PuntaZSecurityPos(other_noz);
			PuntaZPosWait(other_noz);

			if( centeringResult.Result == STATUS_OK )
			{
				#ifdef __SNIPER
				xlas = centeringResult.Position1;
				ylas = centeringResult.Position2;
				#endif

				Set_Finec(ON);	   // abilita protezione tramite finecorsa

				NozzleXYMove( xlas + (QParam.AuxCam_X[ct_nozzle-1] + centtest_param.cam_dx), ylas + (QParam.AuxCam_Y[ct_nozzle-1] + centtest_param.cam_dy), ct_nozzle );
				Wait_PuntaXY();
				Set_Finec(OFF);	   // disabilita protezione tramite finecorsa

				//porta punta a quota telecamera
				PuntaZPosMm( ct_nozzle, QParam.AuxCam_Z[ct_nozzle-1] );

				Wait_EncStop( ct_nozzle );
				PuntaZPosWait( ct_nozzle );

				SetExtCam_Light(1);

				float start_x_pos = QParam.AuxCam_X[ct_nozzle-1];
				float start_y_pos = QParam.AuxCam_Y[ct_nozzle-1];
				float x = start_x_pos;
				float y = start_y_pos;

				do
				{
					bool move = false;

					c=Handle();
					switch(c)
					{
						case K_SHIFT_F5:
							SetExtCam_Light(0,-1);
							break;

						case K_SHIFT_F6:
							SetExtCam_Light(0,1);
							break;

						case K_SHIFT_F7:
							SetExtCam_Shutter(0,-1);
							break;

						case K_SHIFT_F8:
							SetExtCam_Shutter(0,1);
							break;

						case K_SHIFT_F9:
							SetExtCam_Gain(0,-1);
							break;

						case K_SHIFT_F10:
							SetExtCam_Gain(0,+1);
							break;

						case K_PAGEUP:
							PuntaRotStep(1, ct_nozzle,BRUSH_REL);
							break;
						case K_PAGEDOWN:
							PuntaRotStep(-1, ct_nozzle,BRUSH_REL);
							break;
						case K_CTRL_PAGEUP:
							PuntaRotDeg(1, ct_nozzle,BRUSH_REL);
							break;
						case K_CTRL_PAGEDOWN:
							PuntaRotDeg(-1, ct_nozzle,BRUSH_REL);
							break;
						case K_HOME:
							PuntaRotDeg(180, ct_nozzle,BRUSH_REL);
							break;
						case K_END:
							PuntaRotDeg(-180, ct_nozzle,BRUSH_REL);
							break;

						case K_RIGHT:
							x += 0.02;
							move = true;
							break;
						case K_SHIFT_RIGHT:
							x += 0.1;
							move = true;
							break;
						case K_CTRL_RIGHT:
							x += 1;
							move = true;
							break;
						case K_ALT_RIGHT:
							x += 10;
							move = true;
							break;
						case K_LEFT:
							x -= 0.02;
							move = true;
							break;
						case K_SHIFT_LEFT:
							x -= 0.1;
							move = true;
							break;
						case K_CTRL_LEFT:
							x -= 1;
							move = true;
							break;
						case K_ALT_LEFT:
							x -= 10;
							move = true;
							break;
						case K_UP:
							y += 0.02;
							move = true;
							break;
						case K_SHIFT_UP:
							y += 0.1;
							move = true;
							break;
						case K_CTRL_UP:
							y += 1;
							move = true;
							break;
						case K_ALT_UP:
							y += 10;
							move = true;
							break;
						case K_DOWN:
							y -= 0.02;
							move = true;
							break;
						case K_SHIFT_DOWN:
							y -= 0.1;
							move = true;
							break;
						case K_CTRL_DOWN:
							y -= 1;
							move = true;
							break;
						case K_ALT_DOWN:
							y -= 10;
							move = true;
							break;

						case K_F3:
							setCross( CROSS_NEXT_MODE );
							break;

						case K_F4:
							set_bright(0);    //default
							set_contrast(0);  //default
							break;

						case K_F5:
							set_bright(-512);
							break;

						case K_F6:
							set_bright(512);
							break;

						case K_F7:
							set_contrast(-512);
							break;

						case K_F8:
							set_contrast(512);
							break;

						case K_ENTER:
							{
								float cur_x = GetLastXPos();
								float cur_y = GetLastYPos();

								centtest_param.cam_dx = cur_x - start_x_pos;
								centtest_param.cam_dy = cur_y - start_y_pos;
								Mod_CentTest(centtest_param);
							}
					}

					if( move )
					{
						Set_Finec(ON);
						NozzleXYMove( x + xlas + centtest_param.cam_dx, y + ylas + centtest_param.cam_dy, ct_nozzle );
						Wait_PuntaXY();
						Set_Finec(OFF);
					}
				} while(c!=K_ESC);

				SetExtCam_Light(0);

				char buf[160];
				snprintf( buf, sizeof(buf), "X = %5.3f mm\nY = %5.3f mm\n \n%s",xlas,ylas,MsgGetString(Msg_01088));
				if(!W_Deci(1,buf))
				{
					depo=1;
					break;
				}
			}
			else
			{
				Set_Finec(ON);
				NozzleXYMove( QParam.AuxCam_X[ct_nozzle-1], QParam.AuxCam_Y[ct_nozzle-1], ct_nozzle );
				Wait_PuntaXY();
				Set_Finec(OFF);

				if(!W_Deci(1, MsgGetString(Msg_01087) ))
				{
					break;
				}
			}

			PuntaZPosMm( ct_nozzle,0,ABS_MOVE);
			PuntaZPosWait( ct_nozzle ); //attende fine movimento

			//SMOD310103
			PuntaRotDeg(Get_PickThetaPosition( ct_feeder->GetCode(), ct_pack ), ct_nozzle );

			//SMOD250903
			while(!Check_PuntaRot( ct_nozzle ))
			{
				FoxPort->PauseLog();
			}
			FoxPort->RestartLog();

			Wait_PuntaXY();

			PuntaZPosMm( ct_nozzle,preldata.zup,ABS_MOVE);
			PuntaZPosWait( ct_nozzle ); //attende fine movimento

		} while(1);

		// Disattiva la camera
		Set_Tv( 0 );
		SetExtCam_Light(0);

		// ripristina vecchi valori
		SetImgBrightCont( prevBright, prevContrast );


		if( i < ct_numPick - 1 )
		{
			depo = 0;
		}

		if(depo)
		{
			PuntaZPosMm( ct_nozzle,0,ABS_MOVE);
			PuntaZPosWait( ct_nozzle );

			//porta punta alle coordinate di deposito
			Set_Finec(ON);
			NozzleXYMove_N( ct_param.xcomp+xlas,ct_param.ycomp+ylas, ct_nozzle );
			Wait_PuntaXY();
			Set_Finec(OFF);

			//SMOD250903
			while(!Check_PuntaRot( ct_nozzle ))
			{
				FoxPort->PauseLog();
			}
			FoxPort->RestartLog();

			BrushDataRead( brushPar, ct_pidPlace );

			if( ct_nozzle == 1 )
			{
				FoxHead->SetPID(BRUSH1,brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0]);
			}
			else
			{
				FoxHead->SetPID(BRUSH2,brushPar.p[1],brushPar.i[1],brushPar.d[1],brushPar.clip[1]);
			}

			//deposita componente
			PuntaZPosMm( ct_nozzle, GetPianoZPos(ct_nozzle)-ct_pack.z-ct_param.zcomp );
			FoxHead->LogOn((ct_nozzle-1)*2);
			PuntaZPosWait(ct_nozzle);

			CPan* pan = new CPan( 22, 1, MsgGetString(Msg_01238) );

			//attende pressione ESC
			c = 0;
			while( c != K_ESC )
			{
				SetConfirmRequiredBeforeNextXYMovement(true);
				c = Handle();
			}

			delete pan;

			idx=FoxHead->LogOff((ct_nozzle-1)*2);
			FoxHead->LogGraph((ct_nozzle-1)*2,idx,ENCBUFLEN,1);

			//attiva contropressione punta
			Set_Contro(ct_nozzle,1);
			// attesa contropress.
			delay(QHeader.TContro_Comp);
			//disattiva contropressione punta
			Set_Contro(ct_nozzle,0);
		}

		//reset z acc e vel a default
		SetNozzleZSpeed_Index( ct_nozzle, ACC_SPEED_DEFAULT );
		SetNozzleRotSpeed_Index( ct_nozzle, ACC_SPEED_DEFAULT );

		PuntaZSecurityPos(1);
		PuntaZSecurityPos(2);
		PuntaZPosWait(1);
		PuntaZPosWait(2);

		if(!depo)
		{
			ScaricaComp(ct_nozzle);
		}

		BrushDataRead(brushPar,0);
		if(ct_nozzle==1)
		{
			FoxHead->SetPID(BRUSH1,brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0]);
		}
		else
		{
			FoxHead->SetPID(BRUSH2,brushPar.p[1],brushPar.i[1],brushPar.d[1],brushPar.clip[1]);
		}

		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

		PuntaRotDeg(0,ct_nozzle,BRUSH_ABS);    //porta la punta a zero theta

		//SMOD250903
		while(!Check_PuntaRot(ct_nozzle))
		{
			FoxPort->PauseLog();
		}
		FoxPort->RestartLog();
	}
	/*
	FoxHead->SetTorqueNoise(BRUSH1,0);
	FoxHead->SetPositionNoise(BRUSH1,0);

	FoxHead->SetTorqueNoise(BRUSH2,0);
	FoxHead->SetPositionNoise(BRUSH2,0);
	*/
	return 1;
}

int CT_PlacePosTeaching()
{
	float xc = centtest_param.xcomp;
	float yc = centtest_param.ycomp;

	if( ManualTeaching(&xc,&yc,MsgGetString(Msg_01357)) )
	{
		ct_param.xcomp = xc;
		ct_param.ycomp = yc;
		centtest_param.xcomp = xc;
		centtest_param.ycomp = yc;
		Mod_CentTest(centtest_param);
	}
	return 1;
}


//---------------------------------------------------------------------------
// finestra: Centering test
//---------------------------------------------------------------------------
class CenteringTestUI : public CWindowParams
{
public:
	CenteringTestUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaPos( 0, 6 );
		SetClientAreaSize( 50, 20 );
		SetTitle( MsgGetString(Msg_01089) );
	}

	typedef enum
	{
		NOZZLE,
		COMP_CODE,
		COMP_PACK,
		COMP_X,
		COMP_Y,
		COMP_Z,
		COMP_ROT,
		ENCODER,
		PID_CENT,
		PID_PLACE,
		PICK_NUM,
		PICK_ERR_X,
		PICK_ERR_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[NOZZLE]     = new C_Combo( 4,  1, MsgGetString(Msg_01104), 1, CELL_TYPE_UINT );
		m_combos[COMP_CODE]  = new C_Combo( 4,  3, MsgGetString(Msg_01105), 25, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[COMP_PACK]  = new C_Combo( 4,  4, MsgGetString(Msg_01106), 25, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[COMP_X]     = new C_Combo( 4,  6, MsgGetString(Msg_01130), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[COMP_Y]     = new C_Combo( 4,  7, MsgGetString(Msg_01131), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[COMP_Z]     = new C_Combo( 4,  8, MsgGetString(Msg_01132), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[COMP_ROT]   = new C_Combo( 4,  9, MsgGetString(Msg_01172), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[ENCODER]    = new C_Combo( 4, 11, MsgGetString(Msg_01110), 7, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[PID_CENT]   = new C_Combo( 4, 13, MsgGetString(Msg_00840), 7, CELL_TYPE_UINT );
		m_combos[PID_PLACE]  = new C_Combo( 4, 14, MsgGetString(Msg_00841), 7, CELL_TYPE_UINT );
		m_combos[PICK_NUM]   = new C_Combo( 4, 16, MsgGetString(Msg_05005), 7, CELL_TYPE_UINT );
		m_combos[PICK_ERR_X] = new C_Combo( 4, 17, MsgGetString(Msg_05006), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[PICK_ERR_Y] = new C_Combo( 4, 18, MsgGetString(Msg_05007), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[NOZZLE]->SetVMinMax( 1, 2 );
		m_combos[COMP_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[COMP_Y]->SetVMinMax( QParam.LY_mincl, QParam.LY_maxcl );
		m_combos[COMP_Z]->SetVMinMax( -20.f, 30.f );
		m_combos[COMP_ROT]->SetVMinMax( -360.f, 360.f );
		m_combos[PID_CENT]->SetVMinMax( 0, 9 );
		m_combos[PID_PLACE]->SetVMinMax( 0, 9 );
		m_combos[PICK_NUM]->SetVMinMax( 1, 5 );
		m_combos[PICK_ERR_X]->SetVMinMax( -3.f, 3.f );
		m_combos[PICK_ERR_Y]->SetVMinMax( -3.f, 3.f );

		// add to combo list
		m_comboList->Add( m_combos[NOZZLE]    ,  0, 0 );
		m_comboList->Add( m_combos[COMP_CODE] ,  1, 0 );
		m_comboList->Add( m_combos[COMP_PACK] ,  2, 0 );
		m_comboList->Add( m_combos[COMP_X]    ,  3, 0 );
		m_comboList->Add( m_combos[COMP_Y]    ,  4, 0 );
		m_comboList->Add( m_combos[COMP_Z]    ,  5, 0 );
		m_comboList->Add( m_combos[COMP_ROT]  ,  6, 0 );
		m_comboList->Add( m_combos[ENCODER]   ,  7, 0 );
		m_comboList->Add( m_combos[PID_CENT]  ,  8, 0 );
		m_comboList->Add( m_combos[PID_PLACE] ,  9, 0 );
		m_comboList->Add( m_combos[PICK_NUM]  , 10, 0 );
		m_comboList->Add( m_combos[PICK_ERR_X], 11, 0 );
		m_comboList->Add( m_combos[PICK_ERR_Y], 12, 0 );

		centtest_param.cam_dx = 0;
		centtest_param.cam_dy = 0;
	}

	void onRefresh()
	{
		m_combos[NOZZLE]->SetTxt( ct_nozzle );

		if( ct_feeder )
		{
			m_combos[COMP_CODE]->SetTxt( ct_feeder->GetDataConstRef().C_comp );
			m_combos[COMP_PACK]->SetTxt( ct_feeder->GetDataConstRef().C_Package );
		}
		else
		{
			m_combos[COMP_CODE]->SetTxt( "" );
			m_combos[COMP_PACK]->SetTxt( "" );
		}

		m_combos[COMP_X]->SetTxt( ct_param.xcomp );
		m_combos[COMP_Y]->SetTxt( ct_param.ycomp );
		m_combos[COMP_Z]->SetTxt( ct_param.zcomp );
		m_combos[COMP_ROT]->SetTxt( ct_param.rcomp );
		m_combos[ENCODER]->SetTxt( (int)(FoxHead->ReadEncoder((ct_nozzle-1)*2)) );
		m_combos[PID_CENT]->SetTxt( ct_pidCent );
		m_combos[PID_PLACE]->SetTxt( ct_pidPlace );
		m_combos[PICK_NUM]->SetTxt( ct_numPick );
		m_combos[PICK_ERR_X]->SetTxt( ct_prelErrX );
		m_combos[PICK_ERR_Y]->SetTxt( ct_prelErrY );
	}

	void onEdit()
	{
		ct_nozzle = m_combos[NOZZLE]->GetInt();

		ct_param.xcomp = m_combos[COMP_X]->GetFloat();
		ct_param.ycomp = m_combos[COMP_Y]->GetFloat();
		ct_param.zcomp = m_combos[COMP_Z]->GetFloat();
		ct_param.rcomp = m_combos[COMP_ROT]->GetFloat();

		ct_pidCent = m_combos[PID_CENT]->GetInt();
		ct_pidPlace = m_combos[PID_PLACE]->GetInt();

		ct_numPick = m_combos[PICK_NUM]->GetInt();
		ct_prelErrX = m_combos[PICK_ERR_X]->GetFloat();
		ct_prelErrY = m_combos[PICK_ERR_Y]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00464), K_F3,  0, NULL, CT_SelectFeeder );       // Seleziona caricatore
		m_menu->Add( MsgGetString(Msg_01115), K_F4,  0, NULL, CT_StartTest );          // Start test
		m_menu->Add( MsgGetString(Msg_01510), K_F5,  0, NULL, CT_PlacePosTeaching );   // Appr. pos. deposito
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				CT_SelectFeeder();
				return true;

			case K_F4:
				CT_StartTest();
				return true;

			case K_F5:
				CT_PlacePosTeaching();
				return true;

			case K_CTRL_HOME:
				HeadHomeMov();
				break;

			case K_CTRL_END:
				HeadEndMov();
				break;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
	}
};

int fn_CenteringTest()
{
	//Letura parametri da file
	if( !Read_CentTest(centtest_param) )
	{
		//Creazione file dei parametri test centr
		Create_CentTest();
		Read_CentTest(centtest_param);

		centtest_param.xcomp = 0.f;
		centtest_param.ycomp = 0.f;
		centtest_param.zcomp = 0.f;
		centtest_param.rcomp = 0.f;

		//Aggiornamento file parametri test centr
		Mod_CentTest(centtest_param);
	}

	ct_param = centtest_param;

	ct_nozzle = 1;
	ct_feederSelected = 0;

	ct_numPick = 1;
	ct_prelErrX = 0;
	ct_prelErrY = 0;

	// open del file packages
	PackagesLib_Load( QHeader.Lib_Default );

	feederFile = new FeederFile( QHeader.Conf_Default );

	ct_pidCent = 0;
	ct_pidPlace = 0;

	if(centtest_param.carcode==0)
	{
		centtest_param.carcode = 11;
	}

	ct_feeder = new FeederClass( feederFile, centtest_param.carcode );
	if( ct_feeder->GetDataConstRef().C_comp[0]==0 || ct_feeder->GetDataConstRef().C_Package[0]==0 )
	{
		delete ct_feeder;
		ct_feeder = NULL;
		ct_feederSelected = 0;
	}
	else
	{
		if( ct_feeder->Get_AssociatedPack(ct_pack) )
		{
			ct_feederSelected = 1;
			ct_pidCent = ct_pack.centeringPID;
			ct_pidPlace = ct_pack.placementPID;
		}
		else
		{
			delete ct_feeder;
			ct_feeder = NULL;
			ct_feederSelected = 0;
		}
	}


	CenteringTestUI win;
	win.Show();
	win.Hide();

	//Aggiornamento file par. laser
	centtest_param.xcomp = ct_param.xcomp;
	centtest_param.ycomp = ct_param.ycomp;
	centtest_param.zcomp = ct_param.zcomp;
	centtest_param.rcomp = ct_param.rcomp;

	if( ct_feederSelected )
	{
		centtest_param.carcode = ct_feeder->GetCode();
	}

	Mod_CentTest(centtest_param);

	delete feederFile;

	if( ct_feeder )
	{
		delete ct_feeder;
		ct_feeder = NULL;
	}

	return 1;
}

//-------------------------------------------------------------------------------


float ZAutoApprend(int punta,float avvpos,float xpos,float ypos,float coarse_step,float fine_step)
{

	int vacuo,c=0,count=0;
	float vacset[2048];
	float deriv[2048],max=-500,min=500;
	
	PuntaZSecurityPos(1);  //porta entrambe le punte in posizione di sicurezza
	PuntaZSecurityPos(2);
	
	Set_Finec(ON);
	NozzleXYMove_N( xpos, ypos, punta );
	Wait_PuntaXY();
	Set_Finec(OFF);
	
	Set_Vacuo(punta,1);
	PuntaZPosMm(punta,avvpos);
	PuntaZPosWait(punta);
	delay(100);
	vacuo=Get_Vacuo(punta);
	
	CWindow* Q_ZAuto = new CWindow(20,10,60,15,"");
	C_Combo CZAuto( 1, 1, "", 4, CELL_TYPE_UINT );

	Q_ZAuto->Show();
	CZAuto.Show( Q_ZAuto->GetX(), Q_ZAuto->GetY() );

	deriv[0]=0;

	do
	{
		vacuo=Get_Vacuo(punta);
		CZAuto.SetTxt(vacuo);
	
		SetConfirmRequiredBeforeNextXYMovement(true);	

		c = Handle( false );

		switch(c)
		{
			case K_UP:
				PuntaZPosMm(punta,-fine_step,REL_MOVE);
				PuntaZPosWait(punta);
				vacuo=Get_Vacuo(punta);
				avvpos-=fine_step;
				if(count>0)
				{
					deriv[count]=vacuo-vacset[count-1];
					if(deriv[count]>max)
					{
						max=deriv[count];
					}
					if(deriv[count]<min)
					{
						min=deriv[count];
					}
				}
				vacset[count]=(float)vacuo;
				count++;
				break;

			case K_DOWN:
				PuntaZPosMm(punta,fine_step,REL_MOVE);
				PuntaZPosWait(punta);
				avvpos+=fine_step;
				if(count>0)
				{
					deriv[count]=vacuo-vacset[count-1];
					if(deriv[count]>max)
					{
						max=deriv[count];
					}
					if(deriv[count]<min)
					{
						min=deriv[count];
					}
				}
				vacset[count]=(float)vacuo;
				count++;
			break;
		}
	} while(c!=K_ESC);

	//if(count!=0)
	//	G_drawgraph(count,min,max,deriv,(float)25,"Test");

	Set_Vacuo(punta,0);
	
	PuntaZSecurityPos(1);  //porta entrambe le punte in posizione di sicurezza
	PuntaZSecurityPos(2);
	
	return(avvpos);
}


//--------------------------------------------------------------------------


int ZCheck_Verify( int nozzle )
{
	PuntaRotDeg(0,nozzle);

	while(!Check_PuntaRot(nozzle))
	{
		FoxPort->PauseLog();
	}
	FoxPort->RestartLog();

	doZCheck(nozzle);
	return 1;
}

//---------------------------------------------------------------------------
// finestra: Feeders default position
//---------------------------------------------------------------------------
class EarRingVerifyUI : public CWindowParams
{
public:
	EarRingVerifyUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 50, 9 );

		nozzle = 1;

		char buf[80];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01223), nozzle );
		SetTitle( buf );
	}

	typedef enum
	{
		POSITION
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[POSITION] = new C_Combo( 10, 2, MsgGetString(Msg_01220), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[POSITION]->SetVMinMax( QHeader.Min_NozHeight[nozzle-1], QHeader.Max_NozHeight[nozzle-1] );

		// add to combo list
		m_comboList->Add( m_combos[POSITION], 0, 0 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 5, GetW()/GUI_CharW() - 4, 3 ) );
		DrawTextCentered( 6, MsgGetString(Msg_00318), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	}

	void onRefresh()
	{
		// set params
		m_combos[POSITION]->SetVMinMax( QHeader.Min_NozHeight[nozzle-1], QHeader.Max_NozHeight[nozzle-1] );

		float val = MID( QHeader.Min_NozHeight[nozzle-1], QParam.ZCheckPos[nozzle-1], QHeader.Max_NozHeight[nozzle-1] );
		m_combos[POSITION]->SetTxt( val );
	}

	void onEdit()
	{
		QParam.ZCheckPos[nozzle-1] = m_combos[POSITION]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_01219), K_F3, 0, NULL, boost::bind( &EarRingVerifyUI::onVerifyPosition, this ) ); // Verifica posizione
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				onVerifyPosition();
				return true;

			case K_PAGEUP:
				if( nozzle > 1 )
				{
					nozzle--;

					char buf[80];
					snprintf( buf, sizeof(buf), MsgGetString(Msg_01223), nozzle );
					SetTitle( buf );
					return true;
				}
				break;

			case K_PAGEDOWN:
				if( nozzle < 2 )
				{
					nozzle++;

					char buf[80];
					snprintf( buf, sizeof(buf), MsgGetString(Msg_01223), nozzle );
					SetTitle( buf );
					return true;
				}
				break;

			default:
				break;
		}

		return false;
	}

private:
	int onVerifyPosition()
	{
		return ZCheck_Verify( nozzle );
	}

	int nozzle;
};


int fn_EarRingVerifyUI()
{
	EarRingVerifyUI win( 0 );
	win.Show();
	win.Hide();

	Mod_Par( QParam );
	return 1;
}



#ifdef __USE_MOTORHEAD

//---------------------------------------------------------------------------
// finestra: Motorhead log params
//---------------------------------------------------------------------------

const char* Channel1_StrVect[] = { "POS_REF" };
const char* Channel2_StrVect[] = { "MECH_THETA", "POS_ERR" , "PID_ID_REF", "PID_ID_ERR", "PID_IQ_REF", "PID_IQ_ERR", "SPEED_ERR", "SPEED_REF_NO_POS", "V_POWER" };
const char* Channel3_StrVect[] = { "MECH_THETA", "SPEED" , "PHASE_A" , "PHASE_B" , "PID_ID_OUT" , "PID_ID_FDB" , "PID_IQ_OUT" , "PID_IQ_FDB", "V_POWER" };
const char* Channel4_StrVect[] = { "MECH_THETA", "SPEED" , "PHASE_A" , "PHASE_B" , "PID_ID_OUT" , "PID_ID_FDB" , "PID_IQ_OUT" , "PID_IQ_FDB", "SPEED_REF", "V_POWER" };

extern MotorheadLogData motorheadLogData;


class MotorheadLogParamsUI : public CWindowParams
{
public:
	MotorheadLogParamsUI() : CWindowParams( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 54, 10 );
		SetTitle( MsgGetString(Msg_00098) );
	}

	typedef enum
	{
		TRIGGER,
		TRIGGER_MODE,
		PRESCALER,
		CH1_TYPE,
		CH2_TYPE,
		CH3_TYPE,
		CH4_TYPE,
		CH1_EN,
		CH2_EN,
		CH3_EN,
		CH4_EN
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[TRIGGER]      = new C_Combo( 5, 1, "Trigger   :", 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TRIGGER_MODE] = new C_Combo( 33, 1, "Low->High :", 4, CELL_TYPE_YN );
		m_combos[PRESCALER]    = new C_Combo( 5, 2, "Prescaler :", 5, CELL_TYPE_UINT );
		m_combos[CH1_TYPE]     = new C_Combo( 5, 4, "Channel1  :", 20, CELL_TYPE_TEXT, CELL_STYLE_READONLY );
		m_combos[CH2_TYPE]     = new C_Combo( 5, 5, "Channel2  :", 20, CELL_TYPE_TEXT );
		m_combos[CH3_TYPE]     = new C_Combo( 5, 6, "Channel3  :", 20, CELL_TYPE_TEXT );
		m_combos[CH4_TYPE]     = new C_Combo( 5, 7, "Channel4  :", 20, CELL_TYPE_TEXT );
		//m_combos[CH1_EN]       = new C_Combo( 40, 4, "En :", 4, CELL_TYPE_YN, CELL_STYLE_READONLY );
		m_combos[CH2_EN]       = new C_Combo( 40, 5, "En :", 4, CELL_TYPE_YN );
		m_combos[CH3_EN]       = new C_Combo( 40, 6, "En :", 4, CELL_TYPE_YN );
		m_combos[CH4_EN]       = new C_Combo( 40, 7, "En :", 4, CELL_TYPE_YN );

		// set params
		m_combos[TRIGGER]->SetVMinMax( -999.99f, 999.99f );
		m_combos[PRESCALER]->SetVMinMax( 1, 1000 );
		m_combos[CH1_TYPE]->SetLegalStrings( 1, (char**)Channel1_StrVect );
		m_combos[CH2_TYPE]->SetLegalStrings( 9, (char**)Channel2_StrVect );
		m_combos[CH3_TYPE]->SetLegalStrings( 9, (char**)Channel3_StrVect );
		m_combos[CH4_TYPE]->SetLegalStrings( 10, (char**)Channel4_StrVect );

		// add to combo list
		m_comboList->Add( m_combos[TRIGGER],      0, 0 );
		m_comboList->Add( m_combos[TRIGGER_MODE], 0, 1 );
		m_comboList->Add( m_combos[PRESCALER],    1, 0 );
		m_comboList->Add( m_combos[CH1_TYPE],     2, 0 );
		m_comboList->Add( m_combos[CH2_TYPE],     3, 0 );
		m_comboList->Add( m_combos[CH3_TYPE],     4, 0 );
		m_comboList->Add( m_combos[CH4_TYPE],     5, 0 );
		//m_comboList->Add( m_combos[CH1_EN],       2, 1 );
		m_comboList->Add( m_combos[CH2_EN],       3, 1 );
		m_comboList->Add( m_combos[CH3_EN],       4, 1 );
		m_comboList->Add( m_combos[CH4_EN],       5, 1 );
	}

	void onShow()
	{
		DrawText( 25, 1, "mm" );
		DrawText( 40, 4, "Trigger ch" );
	}

	void onRefresh()
	{
		m_combos[TRIGGER]->SetTxt( motorheadLogData.trigger );
		m_combos[TRIGGER_MODE]->SetTxtYN( motorheadLogData.trigger_mode == LOWTOHIGH_TRIG );
		m_combos[PRESCALER]->SetTxt( motorheadLogData.prescaler );

		int value2, value3, value4;

		// channel 1
		//LOGGER_POSREF

		// channel 2
		switch( motorheadLogData.channel_type[1] )
		{
		case LOGGER_MECHTHETA:
			value2 = 0;
			break;

		case LOGGER_POSERR:
			value2 = 1;
			break;

		case LOGGER_PIDIDREF:
			value2 = 2;
			break;

		case LOGGER_PIDIDERR:
			value2 = 3;
			break;

		case LOGGER_PIDIQREF:
			value2 = 4;
			break;

		case LOGGER_PIDIQERR:
			value2 = 5;
			break;

		case LOGGER_SPEEDERR:
			value2 = 6;
			break;

		case LOGGER_SPEEDREFNOPOS:
			value2 = 7;
			break;

		case LOGGER_VPWR:
			value2 = 8;
			break;

		default: //LOGGER_MECHTHETA
			value2 = 0;
			break;
		}

		// channel 3
		switch( motorheadLogData.channel_type[2] )
		{
		case LOGGER_MECHTHETA:
			value3 = 0;
			break;

		case LOGGER_SPEED:
			value3 = 1;
			break;

		case LOGGER_CLARKA:
			value3 = 2;
			break;

		case LOGGER_CLARKB:
			value3 = 3;
			break;

		case LOGGER_PIDIDOUT:
			value3 = 4;
			break;

		case LOGGER_PIDIDFDB:
			value3 = 5;
			break;

		case LOGGER_PIDIQOUT:
			value3 = 6;
			break;

		case LOGGER_PIDIQFDB:
			value3 = 7;
			break;

		case LOGGER_VPWR:
			value3 = 8;
			break;

		default: //LOGGER_SPEED
			value3 = 1;
			break;
		}

		// channel 4
		switch( motorheadLogData.channel_type[3] )
		{
		case LOGGER_MECHTHETA:
			value4 = 0;
			break;

		case LOGGER_SPEED:
			value4 = 1;
			break;

		case LOGGER_CLARKA:
			value4 = 2;
			break;

		case LOGGER_CLARKB:
			value4 = 3;
			break;

		case LOGGER_PIDIDOUT:
			value4 = 4;
			break;

		case LOGGER_PIDIDFDB:
			value4 = 5;
			break;

		case LOGGER_PIDIQOUT:
			value4 = 6;
			break;

		case LOGGER_PIDIQFDB:
			value4 = 7;
			break;

		case LOGGER_SPEEDREF:
			value4 = 8;
			break;

		case LOGGER_VPWR:
			value4 = 9;
			break;

		default: //LOGGER_SPEEDREF
			value4 = 8;
			break;
		}

		m_combos[CH1_TYPE]->SetStrings_Pos( 0 );
		m_combos[CH2_TYPE]->SetStrings_Pos( value2 );
		m_combos[CH3_TYPE]->SetStrings_Pos( value3 );
		m_combos[CH4_TYPE]->SetStrings_Pos( value4 );

		//m_combos[CH1_EN]->SetTxtYN( motorheadLogData.channel_on[0] );
		m_combos[CH2_EN]->SetTxtYN( motorheadLogData.channel_on[1] );
		m_combos[CH3_EN]->SetTxtYN( motorheadLogData.channel_on[2] );
		m_combos[CH4_EN]->SetTxtYN( motorheadLogData.channel_on[3] );
	}

	void onEdit()
	{
		motorheadLogData.trigger = m_combos[TRIGGER]->GetFloat();
		motorheadLogData.trigger_mode = m_combos[TRIGGER_MODE]->GetYN() ? LOWTOHIGH_TRIG : HIGHTOLOW_TRIG;
		motorheadLogData.prescaler = m_combos[PRESCALER]->GetInt();

		// channel 2
		switch( m_combos[CH2_TYPE]->GetStrings_Pos() )
		{
		case 0:
			motorheadLogData.channel_type[1] = LOGGER_MECHTHETA;
			break;

		case 1:
			motorheadLogData.channel_type[1] = LOGGER_POSERR;
			break;

		case 2:
			motorheadLogData.channel_type[1] = LOGGER_PIDIDREF;
			break;

		case 3:
			motorheadLogData.channel_type[1] = LOGGER_PIDIDERR;
			break;

		case 4:
			motorheadLogData.channel_type[1] = LOGGER_PIDIQREF;
			break;

		case 5:
			motorheadLogData.channel_type[1] = LOGGER_PIDIQERR;
			break;

		case 6:
			motorheadLogData.channel_type[1] = LOGGER_SPEEDERR;
			break;

		case 7:
			motorheadLogData.channel_type[1] = LOGGER_SPEEDREFNOPOS;
			break;

		case 8:
			motorheadLogData.channel_type[1] = LOGGER_VPWR;
			break;

		default:
			break;
		}

		// channel 3
		switch( m_combos[CH3_TYPE]->GetStrings_Pos() )
		{
		case 0:
			motorheadLogData.channel_type[2] = LOGGER_MECHTHETA;
			break;

		case 1:
			motorheadLogData.channel_type[2] = LOGGER_SPEED;
			break;

		case 2:
			motorheadLogData.channel_type[2] = LOGGER_CLARKA;
			break;

		case 3:
			motorheadLogData.channel_type[2] = LOGGER_CLARKB;
			break;

		case 4:
			motorheadLogData.channel_type[2] = LOGGER_PIDIDOUT;
			break;

		case 5:
			motorheadLogData.channel_type[2] = LOGGER_PIDIDFDB;
			break;

		case 6:
			motorheadLogData.channel_type[2] = LOGGER_PIDIQOUT;
			break;

		case 7:
			motorheadLogData.channel_type[2] = LOGGER_PIDIQFDB;
			break;

		case 8:
			motorheadLogData.channel_type[2] = LOGGER_VPWR;
			break;

		default:
			break;
		}

		// channel 3
		switch( m_combos[CH4_TYPE]->GetStrings_Pos() )
		{
		case 0:
			motorheadLogData.channel_type[3] = LOGGER_MECHTHETA;
			break;

		case 1:
			motorheadLogData.channel_type[3] = LOGGER_SPEED;
			break;

		case 2:
			motorheadLogData.channel_type[3] = LOGGER_CLARKA;
			break;

		case 3:
			motorheadLogData.channel_type[3] = LOGGER_CLARKB;
			break;

		case 4:
			motorheadLogData.channel_type[3] = LOGGER_PIDIDOUT;
			break;

		case 5:
			motorheadLogData.channel_type[3] = LOGGER_PIDIDFDB;
			break;

		case 6:
			motorheadLogData.channel_type[3] = LOGGER_PIDIQOUT;
			break;

		case 7:
			motorheadLogData.channel_type[3] = LOGGER_PIDIQFDB;
			break;

		case 8:
			motorheadLogData.channel_type[3] = LOGGER_SPEEDREF;
			break;

		case 9:
			motorheadLogData.channel_type[3] = LOGGER_VPWR;
			break;

		default:
			break;
		}

		motorheadLogData.channel_on[1] = m_combos[CH2_EN]->GetYN();
		motorheadLogData.channel_on[2] = m_combos[CH3_EN]->GetYN();
		motorheadLogData.channel_on[3] = m_combos[CH4_EN]->GetYN();
	}

	void onClose()
	{
		Motorhead->SetLogParams( HEADX_ID );
		Motorhead->SetLogParams( HEADY_ID );
	}
};

int fn_MotorheadLogParams()
{
	MotorheadLogParamsUI win;
	win.Show();
	win.Hide();

	return 1;
}

#endif
