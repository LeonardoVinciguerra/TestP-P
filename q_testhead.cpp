//---------------------------------------------------------------------------
//
// Name:        q_testhead.h
// Author:      Walter Moretti - Carrara - Italy 1995
//              Simone S090801 >> Interfaccia utente a classi
// Created:
// Description: Definizioni delle funzioni esterne per test hardware
//
//---------------------------------------------------------------------------

#include "q_testhead.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#include "msglist.h"
#include "q_cost.h"
#include "q_graph.h"
#include "q_gener.h"
#include "q_help.h"
#include "q_tabe.h"
#include "q_wind.h"
#include "q_param.h"
#include "q_conf.h"
#include "q_conf_new.h"
#include "q_files.h"
#include "q_oper.h"
#include "q_carobj.h"
#include "q_grcol.h"

#ifdef __SNIPER
#include "tws_sniper.h"
#include "q_snprt.h"
#endif

#include "q_fox.h"
#include "q_ugeobj.h"
#include "q_opert.h"
#include "lnxdefs.h"
#include "keyutils.h"

#include "gui_functions.h"
#include "gui_defs.h"
#include "c_inputbox.h"
#include "c_pan.h"
#include "motorhead.h"

#include <mss.h>


// Istanza oggetto finestra test TESTE.
#define K_VACUO_MEDIA 10


int TTest_AutoAppMode=0;


extern struct CfgParam QParam;
extern struct CfgHeader QHeader;

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;

// Variabili public nel modulo.
float K_xpos, K_ypos;
int   K_puntar, K_puntap, K_puntav, K_puntac, K_puntaz;
int   K_gradi, K_mappa, K_corsa, K_tipo, K_classe,K_giri;
int   K_azionep, K_azionev, K_azionec;
int   K_gradiz[2]={0,0};
int   K_vuoto;
float K_zmm;




#ifdef __SERIAL
int   K_sogliav[2],K_MinVuoto[2], K_classev=4; // L2505
#endif
// Servizio per KTE_All_Def - Parametri allo start test teste.
void KTE_V_Def(void)
{
	K_xpos=K_ypos=0;
	K_azionep=K_azionev=K_azionec=0;
	K_puntar=K_puntap=K_puntav=K_puntac=K_puntaz=1;
	K_gradi=0; //DANY191102
	K_giri=0;
	K_corsa=0;
	K_zmm=0;
	K_mappa=0; //DANY191102
	K_tipo=0;
	K_classe=1;
	K_sogliav[0]=0;
	K_sogliav[1]=0;
}


// Setta un campo al default - test teste.
void KTE_Default(int X_Campo, int Y_Campo)
{
	switch(Y_Campo)
	{
		case 0:
			switch(X_Campo)
			{
				case 0:
					K_xpos = MID( QParam.LX_mincl, K_xpos, QParam.LX_maxcl );
					break;
				case 1:
					K_ypos = MID( QParam.LY_mincl, K_ypos, QParam.LY_maxcl );
					break;
			}
			break;
		case 1:
			switch(X_Campo)
			{
				case 0:
					K_puntar = MID( 1, K_puntar, 2 );
					break;
				case 1:
					K_gradi = MID( 1, K_gradi, 360 );
					break;
			}
			break;
		case 2:
			switch(X_Campo)
			{
				case 0:
					K_puntap = MID( 1, K_puntap, 2 );
					break;
				case 2:
					K_corsa = MID( 0, K_corsa, 2 );
					break;
			}
			break;
		case 3:
			switch(X_Campo)
			{ 	
				case 1:
					K_puntav = MID( 1, K_puntav, 2 );
					break;
	
				case 2:
					K_classev = MID( 1, K_classev, 9 );
					break;	
			}
			break;
		case 4:
			if(!X_Campo)
			{
				K_puntac = MID( 1, K_puntac, 2 );
			}
			break;
		case 5:
			switch(X_Campo)
			{
				case 0:
					K_puntaz = MID( 1, K_puntaz, 2 );
					break;
				case 1:
					K_tipo = MID( 0, K_tipo, 5 );
					break;
				case 2:
					K_classe = MID( 1, K_classe, 4 );
					break;
			}
			break;
	}
}


void Set_sogliav(int type)
{
	int MaxVuoto;

	if (type)
	{ 
		if(K_azionec) 
		{
			Set_Contro(K_puntac,OFF);	// disaattiv. contropressione
		}

	  	Set_Vacuo(K_puntav,ON);			// attiv. vuoto punta selezionata
	  	// Wait vacuo
	  	delay(QHeader.D_vuoto);     	// aspetta regime vuoto

	  	if(K_puntav==1)
		{
			MaxVuoto=QParam.vuoto_1;
		}
	  	else 
		{
			MaxVuoto=QParam.vuoto_1;
		}

		K_MinVuoto[K_puntav-1]=Get_Vacuo(K_puntav);

		K_sogliav[K_puntav-1]=K_MinVuoto[K_puntav-1]+(MaxVuoto-K_MinVuoto[K_puntav-1])*K_classev/10;

		if(!K_azionev)
		{
			Set_Vacuo(K_puntav,OFF);	// disattiv. vuoto punta selezionata
		}
		
		if(K_azionec) 
		{
			Set_Contro(K_puntac,ON);	// riattiv. contropressione
		}
	}
	else
	{ 
		if(K_puntav==1)
		{
			MaxVuoto=QParam.vuoto_1;
		}
		else 
		{
			MaxVuoto=QParam.vuoto_1;
		}
		K_sogliav[K_puntav-1]=K_MinVuoto[K_puntav-1]+(MaxVuoto-K_MinVuoto[K_puntav-1])*K_classev/10;
	}
} // Set_sogliav


// Default di tutti i valori - Test teste.
void KTE_All_Def(void)
{
	int xloop, yloop;
	for(xloop=0;xloop<=5;xloop++)
	{
		for (yloop=0;yloop<=2;yloop++)
		{
			KTE_Default(xloop,yloop);
		}
	}
} // KTE_All_Def


void KTE_go_NEW( int row )
{
	int tmpZPos[2];

	switch( row )
	{
		case 0:
			tmpZPos[0]=PuntaZPosStep(1,0,RET_POS);
			tmpZPos[1]=PuntaZPosStep(2,0,RET_POS);

			PuntaZSecurityPos(1);
			PuntaZSecurityPos(2);
			PuntaZPosWait(2);
			PuntaZPosWait(1);

			Set_Finec(ON);	   // abilita protezione tramite finecorsa
			NozzleXYMove( K_xpos, K_ypos );
			Wait_PuntaXY();
			Set_Finec(OFF);	   // disabilita protezione tramite finecorsa

			PuntaZPosStep(1,tmpZPos[0]);
			PuntaZPosStep(2,tmpZPos[1]);
			PuntaZPosWait(2);
			PuntaZPosWait(1);
			break;
		case 1:
			/* rotazione di test a vel. max in classe 0 */
			Wait_EncStop(K_puntar);

			if( abs(4096*(K_giri*360+K_gradi)/360) < MAXSTEPVAL )
			{
				K_gradi += K_giri*360;
				PuntaRotDeg(K_gradi,K_puntar);
			}
	 		break;
		case 2:
			if(K_azionep==0)
			{
				PuntaZPosMm(K_puntap,0,ABS_MOVE);
			}
			else
			{
				if(K_zmm>GetPianoZPos(K_puntap))
				{
					bipbip();
					if(W_Deci(0,MsgGetString(Msg_00259)))
					{
						PuntaZPosMm(K_puntap,K_zmm,ABS_MOVE);
					}
				}
				else
				{
					PuntaZPosMm(K_puntap,K_zmm,ABS_MOVE);
				}
			}
	 		break;
		case 3:
			Set_Vacuo(K_puntav,K_azionev);
			break;
		case 4:
			Set_Contro(K_puntac,K_azionec);
			break;
	}
}

//INTEGR.V1.2k start
#define ROTATION_TEST_DELTA_DEFAULT 1.f

void RotationTest(int punta)
{
	int c,fine=0;

	char buf[80];
	snprintf( buf, 80, MsgGetString(Msg_01459), punta );

	CInputBox inbox( 0, 6, buf, MsgGetString(Msg_01460), 7, CELL_TYPE_UDEC, 2 );
	inbox.SetText( ROTATION_TEST_DELTA_DEFAULT );

	if( punta == 1 )
	{
		inbox.SetVMinMax( (float)360.0/QHeader.Enc_step1, 90.f );
	}
	else
	{
		inbox.SetVMinMax( (float)360.0/QHeader.Enc_step2, 90.f );
	}

	inbox.Show();
	float step = inbox.GetFloat();


	GUI_Thaw();

	CWindow* Q_RotTest = new CWindow( 15, 10, 65, 20, buf );
	Q_RotTest->Show();

	C_Combo c1( 5, 1, MsgGetString(Msg_01458), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	c1.Show( Q_RotTest->GetX(), Q_RotTest->GetY() );

	GUI_ProgressBar* progbar = new GUI_ProgressBar( Q_RotTest, RectI( 4*GUI_CharW(), 3*GUI_CharH(), Q_RotTest->GetW() - 8*GUI_CharW(), GUI_CharH() ), 360*2 );
	progbar->Show();

	Q_RotTest->DrawPanel( RectI( 2, 6, Q_RotTest->GetW()/GUI_CharW() - 4, 3 ) );
	Q_RotTest->DrawTextCentered( 7, MsgGetString(Msg_00297), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );

	GUI_Thaw();

	do
	{
		for(int i=0;((i<2) && (!fine));i++)
		{
			for(float curpos=step;curpos<360;curpos+=step)
			{
				c = Handle( false );
				if( c == K_ESC )
				{
					fine=1;
					break;
				}

				PuntaRotDeg(0,punta);
		
				if(!Wait_EncStop(punta))
				{
					if(!W_Deci(1,MsgGetString(Msg_01487)))
					{
						fine=1;
						break;
					}
				}

				progbar->SetValue( curpos + i*360 );

				if(i==0)
				{
					c1.SetTxt(curpos);
					PuntaRotDeg(curpos,punta);
				}
				else
				{
					c1.SetTxt(-curpos);
					PuntaRotDeg(-curpos,punta);
				}
        
				if(!Wait_EncStop(punta))
				{
					if(!W_Deci(1,MsgGetString(Msg_01487)))
					{
						fine=1;
						break;
					}				
				}        
			}
    	}
	} while(!fine);

	delete progbar;
	delete Q_RotTest;

	PuntaRotDeg(0,punta);
	Wait_EncStop(punta);
}



//-----------------------------------------------------------------------
// Test movimento teste >> S090801


// Reset degli attuatori all'uscita dal test
void t_reset(void)
{
	Set_Vacuo(1,OFF);
	Set_Vacuo(2,OFF);
	Set_Contro(1,OFF);
	Set_Contro(2,OFF);
	
	PuntaZSecurityPos(1);
	PuntaZSecurityPos(2);
	
	PuntaRotDeg(0,1,BRUSH_ABS);
	PuntaRotDeg(0,2,BRUSH_ABS);
	
	//SMOD250903
	while (!Check_PuntaRot(2))
	{
		FoxPort->PauseLog();
	}
	FoxPort->RestartLog();
	
	while (!Check_PuntaRot(1))
	{
		FoxPort->PauseLog();
	}
	FoxPort->RestartLog();
}


//---------------------------------------------------------------------------
// finestra: Head test window
//---------------------------------------------------------------------------
class HeadTestUI : public CWindowParams
{
public:
	HeadTestUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 6 );
		SetClientAreaSize( 71, 22 );
		SetTitle( MsgGetString(Msg_00163) );

		tips = 0;

		v_count = 0;
		v_sum[0] = 0.f;
		v_sum[1] = 0.f;

		SM_CheckRot = new GUI_SubMenu();
		SM_CheckRot->Add( MsgGetString(Msg_00042), K_F1, 0, NULL, boost::bind( &HeadTestUI::onRotationTest1, this ) ); // punta 1
		SM_CheckRot->Add( MsgGetString(Msg_00043), K_F2, 0, NULL, boost::bind( &HeadTestUI::onRotationTest2, this ) ); // punta 2
	}

	~HeadTestUI()
	{
		delete SM_CheckRot;
	}

	typedef enum
	{
		POS_X,
		POS_Y,
		ROT_NOZ,
		ROT_DEG,
		ROT_MAP,
		NOZ_NOZ,
		NOZ_ACT,
		NOZ_POS,
		VAC_NOZ,
		VAC_ACT,
		VAC_CLA,
		CNT_NOZ,
		CNT_ACT,
		ENC_1,
		ENC_2,
		#ifdef __SNIPER
		SNIPER1,
		SNIPER2,
		#endif
		VAC_1,
		VAC_2,
		TRIG_1,
		TRIG_2,
		ENC_X,
		ENC_Y,
		ENC_X_STATUS,
		ENC_Y_STATUS
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[POS_X]   = new C_Combo( 23, 1, "X :", 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[POS_Y]   = new C_Combo( 39, 1, "Y :", 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[ROT_NOZ] = new C_Combo( 18, 2, MsgGetString(Msg_00541), 1, CELL_TYPE_UINT );
		m_combos[ROT_DEG] = new C_Combo( 34, 2, MsgGetString(Msg_00542), 4, CELL_TYPE_SINT );
		m_combos[ROT_MAP] = new C_Combo( 53, 2, MsgGetString(Msg_00543), 4, CELL_TYPE_YN );
		m_combos[NOZ_NOZ] = new C_Combo( 18, 3, MsgGetString(Msg_00541), 1, CELL_TYPE_UINT );
		m_combos[NOZ_ACT] = new C_Combo( 34, 3, MsgGetString(Msg_00544), 4, CELL_TYPE_YN );
		m_combos[NOZ_POS] = new C_Combo( 52, 3, MsgGetString(Msg_00545), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[VAC_NOZ] = new C_Combo( 18, 4, MsgGetString(Msg_00541), 1, CELL_TYPE_UINT );
		m_combos[VAC_ACT] = new C_Combo( 34, 4, MsgGetString(Msg_00544), 4, CELL_TYPE_YN );
		m_combos[VAC_CLA] = new C_Combo( 52, 4, MsgGetString(Msg_00588), 1, CELL_TYPE_UINT );
		m_combos[CNT_NOZ] = new C_Combo( 18, 5, MsgGetString(Msg_00541), 1, CELL_TYPE_UINT );
		m_combos[CNT_ACT] = new C_Combo( 34, 5, MsgGetString(Msg_00544), 4, CELL_TYPE_YN );


		char buf[40];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01033), 1 );
		m_combos[ENC_1]   =  new C_Combo( 10,  8, buf, 5, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01033), 2 );
		m_combos[ENC_2]   =  new C_Combo( 38,  8, buf, 5, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );

		#ifdef __SNIPER
		snprintf( buf, sizeof(buf), MsgGetString(Msg_05010), 1 );
		m_combos[SNIPER1] =  new C_Combo(  7,  9, buf, 5, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		snprintf( buf, sizeof(buf), MsgGetString(Msg_05010), 2 );
		m_combos[SNIPER2] =  new C_Combo( 35,  9, buf, 5, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		#endif

		snprintf( buf, sizeof(buf), MsgGetString(Msg_00587), 1 );
		m_combos[VAC_1]   =  new C_Combo( 11, 11, buf, 5, CELL_TYPE_SINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		snprintf( buf, sizeof(buf), MsgGetString(Msg_00587), 2 );
		m_combos[VAC_2]   =  new C_Combo( 11, 12, buf, 5, CELL_TYPE_SINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );

		snprintf( buf, sizeof(buf), MsgGetString(Msg_00669), 1 );
		m_combos[TRIG_1]  =  new C_Combo( 35, 11, buf, 5, CELL_TYPE_SINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		snprintf( buf, sizeof(buf), MsgGetString(Msg_00669), 2 );
		m_combos[TRIG_2]  =  new C_Combo( 35, 12, buf, 5, CELL_TYPE_SINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );

		snprintf( buf, sizeof(buf), "Encoder X :" );
		m_combos[ENC_X]   =  new C_Combo( 10,  14, buf, 5, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		snprintf( buf, sizeof(buf), "Encoder Y :" );
		m_combos[ENC_Y]   =  new C_Combo( 38,  14, buf, 5, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );

		snprintf( buf, sizeof(buf), "Enc status X :" );
		m_combos[ENC_X_STATUS]   =  new C_Combo( 7,  15, buf, 5, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		snprintf( buf, sizeof(buf), "Enc status Y :" );
		m_combos[ENC_Y_STATUS]   =  new C_Combo( 35,  15, buf, 5, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );

		// set params
		m_combos[POS_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[POS_Y]->SetVMinMax( QParam.LY_mincl, QParam.LY_maxcl );
		m_combos[ROT_NOZ]->SetVMinMax( 1, 2 );
		m_combos[ROT_DEG]->SetVMinMax( -360, 360 );
		m_combos[NOZ_NOZ]->SetVMinMax( 1, 2 );
		m_combos[NOZ_POS]->SetVMinMax( -100.f, 100.f );
		m_combos[VAC_NOZ]->SetVMinMax( 1, 2 );
		m_combos[VAC_CLA]->SetVMinMax( 1, 9 );
		m_combos[CNT_NOZ]->SetVMinMax( 1, 2 );

		// add to combo list
		m_comboList->Add( m_combos[POS_X]  , 0, 0 );
		m_comboList->Add( m_combos[POS_Y]  , 0, 1 );
		m_comboList->Add( m_combos[ROT_NOZ], 1, 0 );
		m_comboList->Add( m_combos[ROT_DEG], 1, 1 );
		m_comboList->Add( m_combos[ROT_MAP], 1, 2 );
		m_comboList->Add( m_combos[NOZ_NOZ], 2, 0 );
		m_comboList->Add( m_combos[NOZ_ACT], 2, 1 );
		m_comboList->Add( m_combos[NOZ_POS], 2, 2 );
		m_comboList->Add( m_combos[VAC_NOZ], 3, 0 );
		m_comboList->Add( m_combos[VAC_ACT], 3, 1 );
		m_comboList->Add( m_combos[VAC_CLA], 3, 2 );
		m_comboList->Add( m_combos[CNT_NOZ], 4, 0 );
		m_comboList->Add( m_combos[CNT_ACT], 4, 1 );
		m_comboList->Add( m_combos[ENC_1]  , 5, 0 );
		m_comboList->Add( m_combos[ENC_2]  , 5, 1 );
		#ifdef __SNIPER
		m_comboList->Add( m_combos[SNIPER1], 6, 0 );
		m_comboList->Add( m_combos[SNIPER2], 6, 1 );
		#endif
		m_comboList->Add( m_combos[VAC_1]  , 7, 0 );
		m_comboList->Add( m_combos[VAC_2]  , 8, 0 );
		m_comboList->Add( m_combos[TRIG_1] , 7, 1 );
		m_comboList->Add( m_combos[TRIG_2] , 8, 1 );
		m_comboList->Add( m_combos[ENC_X]  , 9, 0 );
		m_comboList->Add( m_combos[ENC_Y]  , 9, 1 );
		m_comboList->Add( m_combos[ENC_X_STATUS]  , 10, 0 );
		m_comboList->Add( m_combos[ENC_Y_STATUS]  , 10, 1 );
	}

	void onShow()
	{
		for( int i = 0; i < 5; i++ )
		{
			DrawPanel( RectI( 3, 1+i, 14, 1) );
		}

		DrawText( 4, 1, MsgGetString(Msg_00164), GUI_SmallFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 4, 2, MsgGetString(Msg_00165), GUI_SmallFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 4, 3, MsgGetString(Msg_00166), GUI_SmallFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 4, 4, MsgGetString(Msg_00167), GUI_SmallFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 4, 5, MsgGetString(Msg_00168), GUI_SmallFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );

		for( int i = 0; i < 5; i++ )
		{
			DrawRectangle( RectI( 3, 1+i, 14, 1), GUI_color( WIN_COL_BORDER ) );
		}

		DrawPanel( RectI( 2, 17, GetW()/GUI_CharW() - 4, 4 ) );
		DrawText( 18, 18, MsgGetString(Msg_00323), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 18, 19, MsgGetString(Msg_00670), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
	}

	void onRefresh()
	{
		m_combos[POS_X]->SetTxt( K_xpos );
		m_combos[POS_Y]->SetTxt( K_ypos );
		m_combos[ROT_NOZ]->SetTxt( K_puntar );
		m_combos[ROT_DEG]->SetTxt( K_gradi % 360 );
		m_combos[ROT_MAP]->SetTxtYN( K_mappa );
		m_combos[NOZ_NOZ]->SetTxt( K_puntap );
		m_combos[NOZ_ACT]->SetTxtYN( K_azionep );
		m_combos[NOZ_POS]->SetTxt( K_zmm );
		m_combos[VAC_NOZ]->SetTxt( K_puntav );
		m_combos[VAC_ACT]->SetTxtYN( K_azionev );
		m_combos[VAC_CLA]->SetTxt( K_classev );
		m_combos[CNT_NOZ]->SetTxt( K_puntac );
		m_combos[CNT_ACT]->SetTxtYN( K_azionec );
	}

	void onEdit()
	{
		K_xpos = m_combos[POS_X]->GetFloat();
		K_ypos = m_combos[POS_Y]->GetFloat();
		K_puntar = m_combos[ROT_NOZ]->GetInt();
		K_gradi = m_combos[ROT_DEG]->GetInt();
		K_mappa = m_combos[ROT_MAP]->GetYN();
		K_puntap = m_combos[NOZ_NOZ]->GetInt();
		K_azionep = m_combos[NOZ_ACT]->GetYN();
		K_zmm = m_combos[NOZ_POS]->GetFloat();
		K_puntav = m_combos[VAC_NOZ]->GetInt();
		K_azionev = m_combos[VAC_ACT]->GetYN();

		int v = K_classev;
		K_classev = m_combos[VAC_CLA]->GetInt();
		if( K_sogliav[K_puntav-1] && v != K_classev )
		{
			Set_sogliav(0);
		}

		K_puntac = m_combos[CNT_NOZ]->GetInt();
		K_azionec = m_combos[CNT_ACT]->GetYN();

		KTE_go_NEW( m_comboList->GetCurRow() );
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00709), K_F3, 0, NULL, boost::bind( &HeadTestUI::onTeaching, this ) ); // Select tool type
		m_menu->Add( MsgGetString(Msg_01474), K_F4, 0, SM_CheckRot, NULL );
	}

	void onIdle()
	{
		GUI_Freeze_Locker lock;

		// show vacuum
		for( int i = 0; i < 2; i++ )
		{
			if( v_count == K_VACUO_MEDIA )
			{
				C_Combo* cV = m_combos[ ( i == 0 ) ? VAC_1 : VAC_2 ];
				C_Combo* cT = m_combos[ ( i == 0 ) ? TRIG_1 : TRIG_2 ];

				int media = ftoi( v_sum[i] / K_VACUO_MEDIA );

				if( (media > K_sogliav[i]) && (K_sogliav[i]>0) )
				{
					cV->SetBgColor( GUI_color( GR_RED ) );
				}
				else
				{
					cV->SetBgColor( GUI_color( GR_WHITE ) );
				}
				cV->SetTxt( -media );

				if( K_sogliav[i] )
				{
					cT->SetTxt( -K_sogliav[i] );
				}
				else
				{
					cT->SetTxt( "---" );
				}

				v_sum[i] = 0;

				if( i == 1 )
				{
					v_count = 0;
				}
			}

			v_sum[i] += Get_Vacuo(i+1);
		}

		v_count++;

		// show encoder
		m_combos[ENC_1]->SetTxt( FoxHead->ReadEncoder(BRUSH1) );
		m_combos[ENC_2]->SetTxt( FoxHead->ReadEncoder(BRUSH2) );

		#ifdef __SNIPER
		m_combos[SNIPER1]->SetTxt( Sniper1->GetEncoder() );
		m_combos[SNIPER2]->SetTxt( Sniper2->GetEncoder() );
		#endif

		if( Motorhead->IsEnabled() )
		{
			int xcount = Motorhead->GetEncoderPulses( HEADX_ID );
			int ycount = Motorhead->GetEncoderPulses( HEADY_ID );
			if( (xcount == MOTOR_ERROR) || (ycount == MOTOR_ERROR) )
				W_Mess( "Error reading encoder!" );
			m_combos[ENC_X]->SetTxt( xcount );
			m_combos[ENC_Y]->SetTxt( ycount );

			int encStatusX = 0;
			int encStatusY = 0;
			if( !Motorhead->GetEncoderStatus( HEADX_ID, encStatusX ) || !Motorhead->GetEncoderStatus( HEADY_ID, encStatusY ) )
				W_Mess( "Error reading encoder status!" );
			m_combos[ENC_X_STATUS]->SetTxt( encStatusX );
			m_combos[ENC_Y_STATUS]->SetTxt( encStatusY );
		}

		delay(1);
	}

	bool onKeyPress( int key )
	{
		if( TTest_AutoAppMode )
		{
			if( AppGestKey( key ) )
			{
				return true;
			}
		}

		switch( key )
		{
			case K_F3:
				onTeaching();
				return true;

			case K_F4:
				SM_CheckRot->Show();
				return true;

			case K_F7:
				onNozzleZTest( 1 );
				return true;

			case K_F8:
				onNozzleZTest( 2 );
				return true;

			case K_ENTER:
				KTE_go_NEW( m_comboList->GetCurRow() );
				return true;

			case K_INSERT:
				onInsKey();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		if( tips )
		{
			delete tips;
			tips = 0;
		}
	}

private:
	int onTeaching()
	{
		if( TTest_AutoAppMode )
		{
			TTest_AutoAppMode = 0;

			if( tips )
			{
				delete tips;
				tips = 0;
			}
		}
		else
		{
			TTest_AutoAppMode = 1;

			if( !tips )
			{
				tips = new CPan( 28, 1, MsgGetString(Msg_01167) );
			}

			curx = K_xpos;
			cury = K_ypos;
			curz = K_zmm;
			curdeg = K_gradi;
		}
		return 1;
	}

	int onRotationTest1()
	{
		Deselect();
	  	RotationTest(1);
	  	Select();
	  	return 1;
	}

	int onRotationTest2()
	{
		Deselect();
		RotationTest(2);
		Select();
		return 1;
	}

	int onNozzleZTest( int nozzle )
	{
		Deselect();

		CPan pan( -1, 1, MsgGetString(Msg_00291) ); // Please wait ...

		float oldpos = PuntaZPosMm( nozzle, 0, RET_POS );

		while( keyRead() != K_ESC )
		{
			PuntaZPosMm( nozzle, K_zmm, ABS_MOVE );
			PuntaZPosWait( nozzle );
			PuntaZPosMm( nozzle, 0, ABS_MOVE );
			PuntaZPosWait( nozzle );
		}

		PuntaZPosMm( nozzle, oldpos, ABS_MOVE );
		PuntaZPosWait( nozzle );

		Select();
		return 1;
	}

	int onInsKey()
	{
		if( m_comboList->GetCurRow() >= 2 )
		{
			Set_sogliav(1);
		}
		else
		{
			Mod_Par( QParam );
			fn_SpeedAcceleration();
		}
		return 1;
	}

	bool AppGestKey( int key )
	{
		int xymove = 0;
		int xysmall = 0;

		SetConfirmRequiredBeforeNextXYMovement(true);

		switch( m_comboList->GetCurRow() )
		{
			case 0: //Movimenti in x,y
				switch( key )
				{
					case K_RIGHT:
						if((K_xpos+QHeader.PassoX)<QParam.LX_maxcl)
						{
							K_xpos+=QHeader.PassoX;
							xymove=1;
							xysmall=1;
						}
						break;
					case K_CTRL_RIGHT:
						if((K_xpos+1)<QParam.LX_maxcl)
						{
							xymove=1;
							xysmall=0;
							K_xpos+=1;
						}
						break;
					case K_ALT_RIGHT:
						if((K_xpos+10)<QParam.LX_maxcl)
						{
							K_xpos+=10;
							xymove=1;
							xysmall=0;
						}
						break;
					case K_LEFT:
						if((K_xpos-QHeader.PassoX)>QParam.LX_mincl)
						{
							K_xpos-=QHeader.PassoX;
							xymove=1;
							xysmall=1;
						}
						break;
					case K_CTRL_LEFT:
						if((K_xpos-1)>QParam.LX_mincl)
						{
							K_xpos-=1;
							xymove=1;
							xysmall=0;
						}
						break;
					case K_ALT_LEFT:
						if((K_xpos-10)>QParam.LX_mincl)
						{
							K_xpos-=10;
							xymove=1;
							xysmall=0;
						}
						break;
					case K_UP:
						if((K_ypos+QHeader.PassoY)<QParam.LY_maxcl)
						{
							K_ypos+=QHeader.PassoY;
							xymove=1;
							xysmall=1;
						}
						break;
					case K_CTRL_UP:
						if((K_ypos+1)<QParam.LY_maxcl)
						{
							K_ypos+=1;
							xymove=1;
							xysmall=0;
						}
						break;
					case K_ALT_UP:
						if((K_ypos+10)<QParam.LY_maxcl)
						{
							K_ypos+=10;
							xymove=1;
							xysmall=0;
						}
						break;
					case K_DOWN:
						if((K_ypos-QHeader.PassoY)>QParam.LY_mincl)
						{
							K_ypos-=QHeader.PassoY;
							xymove=1;
							xysmall=1;
						}
						break;
					case K_CTRL_DOWN:
						if((K_ypos-1)>QParam.LY_mincl)
						{
							K_ypos-=1;
							xymove=1;
							xysmall=0;
						}
						break;
					case K_ALT_DOWN:
						if((K_ypos-10)>QParam.LY_mincl)
						{
							K_ypos-=10;
							xymove=1;
							xysmall=0;
						}
						break;
					case K_ENTER:
						curx=K_xpos;
						cury=K_ypos;
						bipbip();
						return true;
					case K_ESC:
						xymove=1;

						if((fabs(K_xpos-curx)>QHeader.PassoX) || (fabs(K_ypos-cury)>QHeader.PassoY))
						{
							xysmall=0;
						}
						else
						{
							xysmall=1;
						}

						K_xpos=curx;
						K_ypos=cury;
						break;
				}

				if(xymove)
				{
					int tmpZPos[2];

					if(!xysmall)
					{
						tmpZPos[0]=PuntaZPosStep(1,0,RET_POS);
						tmpZPos[1]=PuntaZPosStep(2,0,RET_POS);

						PuntaZSecurityPos(1);
						PuntaZSecurityPos(2);
						PuntaZPosWait(2);
						PuntaZPosWait(1);
					}

					Set_Finec(ON);	   // abilita protezione tramite finecorsa
					NozzleXYMove( K_xpos, K_ypos, AUTOAPP_NOZSECURITY );
					Wait_PuntaXY();
					Set_Finec(OFF);   // disabilita protezione tramite finecorsa

					if(!xysmall)
					{
						PuntaZPosStep(1,tmpZPos[0]);
						PuntaZPosStep(2,tmpZPos[1]);
						PuntaZPosWait(2);
						PuntaZPosWait(1);
					}

					return true;
				}
	      		break;

			case 1: //Rotazioni
				switch( key )
				{
					case K_RIGHT:
						if(abs(4096*(K_gradi+1)/360)<MAXSTEPVAL)
						{
							K_gradi+=1;
							K_giri=abs(K_gradi)/360;
							Wait_EncStop(K_puntar);
							PuntaRotDeg(K_gradi,K_puntar,BRUSH_ABS);
						}
						return true;
					case K_CTRL_RIGHT:
						if(abs(4096*(K_gradi+45)/360)<MAXSTEPVAL)
						{
							K_gradi+=45;
							K_giri=abs(K_gradi)/360;
							Wait_EncStop(K_puntar);
							PuntaRotDeg(K_gradi,K_puntar,BRUSH_ABS);
						}
						return true;
					case K_ALT_RIGHT:
						if(abs(4096*(K_gradi+180)/360)<MAXSTEPVAL)
						{
							K_gradi+=180;
							K_giri=abs(K_gradi)/360;
							Wait_EncStop(K_puntar);
							PuntaRotDeg(K_gradi,K_puntar,BRUSH_ABS);
						}
						return true;
					case K_LEFT:
						if(abs(4096*(K_gradi-1)/360)<MAXSTEPVAL)
						{
							K_gradi-=1;
							K_giri=abs(K_gradi)/360;
							Wait_EncStop(K_puntar);
							PuntaRotDeg(K_gradi,K_puntar,BRUSH_ABS);
						}
						return true;
					case K_CTRL_LEFT:
						if(abs(4096*(K_gradi-45)/360)<MAXSTEPVAL)
						{
							K_gradi-=45;
							K_giri=abs(K_gradi)/360;
							Wait_EncStop(K_puntar);
							PuntaRotDeg(K_gradi,K_puntar,BRUSH_ABS);
						}
						return true;
					case K_ALT_LEFT:
						if(abs(4096*(K_gradi-180)/360)<MAXSTEPVAL)
						{
							K_gradi-=180;
							K_giri=abs(K_gradi)/360;
							Wait_EncStop(K_puntar);
							PuntaRotDeg(K_gradi,K_puntar,BRUSH_ABS);
						}
						return true;
					case K_ENTER:
						curdeg=K_gradi;
						return true;
					case K_ESC:
						K_gradi=curdeg;
						PuntaRotDeg(K_gradi,K_puntar,BRUSH_ABS);
						return true;
				}
	     		break;

			case 2: //Movimenti in z
				switch( key )
				{
					case K_UP:
						K_zmm-=0.1;
						PuntaZPosMm(K_puntap,K_zmm,ABS_MOVE);
						return true;
					case K_CTRL_UP:
						K_zmm-=1;
						PuntaZPosMm(K_puntap,K_zmm,ABS_MOVE);
						return true;
					case K_ALT_UP:
						K_zmm-=5;
						PuntaZPosMm(K_puntap,K_zmm,ABS_MOVE);
						return true;
					case K_DOWN:
						K_zmm+=0.1;
						PuntaZPosMm(K_puntap,K_zmm,ABS_MOVE);
						return true;
					case K_CTRL_DOWN:
						K_zmm+=1;
						PuntaZPosMm(K_puntap,K_zmm,ABS_MOVE);
						return true;
					case K_ALT_DOWN:
						K_zmm+=5;
						PuntaZPosMm(K_puntap,K_zmm,ABS_MOVE);
						return true;
					case K_ENTER:
						curz=K_zmm;
						return true;
					case K_ESC:
						K_zmm=curz;
						PuntaZPosMm(K_puntap,K_zmm,ABS_MOVE);
						return true;
				}
				break;
		}

		// blocked keys
		switch( key )
		{
			case K_RIGHT:
			case K_CTRL_RIGHT:
			case K_ALT_RIGHT:
			case K_SHIFT_RIGHT:
			case K_LEFT:
			case K_CTRL_LEFT:
			case K_ALT_LEFT:
			case K_SHIFT_LEFT:
			case K_UP:
			case K_CTRL_UP:
			case K_ALT_UP:
			case K_SHIFT_UP:
			case K_DOWN:
			case K_CTRL_DOWN:
			case K_ALT_DOWN:
			case K_SHIFT_DOWN:
			case K_PAGEUP:
			case K_PAGEDOWN:
			case K_CTRL_C:
			case K_CTRL_V:
			case K_ENTER:
			case K_ESC:
				return true;
		}

		return false;
	}

	float curx, cury, curz;
	int curdeg;

	int v_count;
	float v_sum[2];
	int test_state[2];

	GUI_SubMenu* SM_CheckRot;
	CPan* tips;
};


// Main test teste
int fn_HeadTest()
{
	TTest_AutoAppMode=0;

	#ifdef __SNIPER
	Sniper1->Zero_Cmd();
	Sniper2->Zero_Cmd();
	#endif

	KTE_V_Def();
	KTE_All_Def();

	PuntaZPosMm(1,0,ABS_MOVE);
	PuntaZPosWait(1);
	PuntaZPosMm(2,0,ABS_MOVE);
	PuntaZPosWait(2);

	HeadTestUI win( 0 );
	win.Show();
	win.Hide();

	ResetNozzles();

	Mod_Par(QParam);

	t_reset(); // resetta tutto
	return 1;
}

