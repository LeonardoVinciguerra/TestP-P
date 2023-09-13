/*
>>>> Q_CONF.CPP

Gestione dei parametri di lavoro macchina 2.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++  	Modificato da TWS Simone 06.08.96 
++++	Modif. Walter 02.04.97 **W0204
++++	>>UGE >>VUO W140797 - >>GRAPH W042000

++++ Modif. Simone LCM S020801-S080801 > interfaccia utente a classi
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <math.h>

#include "q_ugeobj.h"
#include "q_fox.h"
#include "q_cost.h"
#include "msglist.h"
#include "q_cost.h"
#include "q_gener.h"
#include "q_help.h"
#include "q_tabe.h"
#include "q_wind.h"
#include "q_files.h"
#include "q_param.h"
#include "q_oper.h"
#include "q_conf.h"
#include "q_conf_new.h"
#include "q_dosat.h" //L0709
#include "q_assem.h" //L0709
#include "q_prog.h"
#include "q_vision.h"
#include "q_mapping.h"
#include <unistd.h>  //GNU
#include "q_zerit.h" //Integr. Loris
#include "q_packages.h"
#include "q_feeders.h"
#include "q_vision.h"
#include "q_carobj.h"
#include "q_feedersel.h"

#ifdef __SNIPER
#include "tws_sniper.h"
#include "las_scan.h"
#endif

#include "motorhead.h"
#include "q_init.h"
#include "q_inifile.h"
#include "q_net.h"
#include "q_carint.h"
#include "q_ser.h"
#include "q_param.h"
#include "q_filest.h"

#include "keyutils.h"
#include "strutils.h"
#include "fileutils.h"
#include "lnxdefs.h"

#include "c_inputbox.h"
#include "c_waitbox.h"
#include "c_win_imgpar.h"
#include "c_pan.h"
#include "gui_desktop.h"

#include <mss.h>

#ifdef __LOG_ERROR
#include "q_logger.h"
extern CLogger QuadraLogger;
#endif


extern GUI_DeskTop* guiDeskTop;


extern char auto_text1[50];
extern char auto_text2[50];
extern char auto_text3[50];

extern FeederFile* CarFile;
extern FeederClass* Caricatore;
extern SPackageData currentLibPackages[MAXPACK];

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;

extern struct vis_data Vision;
extern struct CfgTeste MapTeste;
extern struct img_data imgData;

//record corrente configurazione caricatori
int car_rec;


int G_UgeZLas( int punta, int ugello );


// Le definizioni delle strutture a seguire sono in q_tab.h (i/o su disco).

// Dich. struct parametri public nel modulo.
struct CfgDispenser CC_Dispenser; // L1905
// Dich. struct parametri public nel modulo.
extern struct CfgParam QParam;
// Dich. struct config. parametri public nel modulo.
extern struct CfgHeader QHeader;

// Dich. puntatore a struct configuraz. ugelli.
struct CfgUgelli CC_Ugelli;

// Dich. struct posizione default caricatori.
SFeederDefault CC_Caric[MAXCAR];

// posizione punto colla corrente L0709
float X_colla, Y_colla;
// posizione fiduciale Integr. Loris
float X_fiduc, Y_fiduc;
// flag punto colla test o programma
int colla_test;

//variabili globali calibrazione assi
unsigned char CalAx_asse;      //asse in calibrazione 1=X/0=Y
unsigned int x_passi, y_passi;
float distanza;

//---------------------------------------------------------------------------------


//-----------------------------------------------------------------------
// Configurazione ugelli >> S020801

CfgUgelli DatiUge;
//punta corrente su posizione ugello
unsigned char u_curnoz;
int uge_rec;


#define UREF_COLS           4
#define UREF_ROWS           3

#define UREF_COLS_TOOLS20   4
#define UREF_ROWS_TOOLS20   5

#define UREF_OFF_X          0.0f
#define UREF_OFF_Y          8.0f

#define UREF_OFF_X_TOOLS20  0.0f
#define UREF_OFF_Y_TOOLS20  -9.0f


const PointF uref_def[][UREF_COLS] =
{
	//row1
	{
		PointF( -30.0f, -20.0f ),
		PointF( -15.0f, -20.0f ),
		PointF(  15.0f, -20.0f ),
		PointF(  30.0f, -20.0f )
	},
	//row2
	{
		PointF( -30.0f, -41.0f ),
		PointF( -15.0f, -41.0f ),
		PointF(  15.0f, -41.0f ),
		PointF(  30.0f, -41.0f )
	},
	//row3
	{
		PointF( -30.0f, -62.0f ),
		PointF( -15.0f, -62.0f ),
		PointF(  15.0f, -62.0f ),
		PointF(  30.0f, -62.0f )
	}
};

const PointF uref_def_tools20[][UREF_COLS] =
{
	//row1
	{
		PointF( -31.5f, -1.0f ),
		PointF( -15.0f, -1.0f ),
		PointF(  15.0f, -1.0f ),
		PointF(  31.5f, -1.0f )
	},
	//row2
	{
		PointF( -31.5f, -18.5f ),
		PointF( -15.0f, -18.5f ),
		PointF(  15.0f, -18.5f ),
		PointF(  31.5f, -18.5f )
	},
	//row3
	{
		PointF( -31.5f, -35.0f ),
		PointF( -15.0f, -35.0f ),
		PointF(  15.0f, -35.0f ),
		PointF(  31.5f, -35.0f )
	},
	//row4
	{
		PointF( -31.5f, -51.5f ),
		PointF( -15.0f, -51.5f ),
		PointF(  15.0f, -51.5f ),
		PointF(  31.5f, -51.5f )
},
	//row5
	{
		PointF( -31.5f, -68.0f ),
		PointF( -15.0f, -68.0f ),
		PointF(  15.0f, -68.0f ),
		PointF(  31.5f, -68.0f )
	}
};


int G_UgeCamSearch(void)
{
	if( !Get_Tools20() )
	{
		Set_UgeBlock(1);

		if(!WaitReedUge(1))
		{
			Set_UgeBlock(0);
			return 0;
		}
	}

	CPan pan( -1, 1, MsgGetString(Msg_05040) ); // Fiducial teaching please wait ...

	PuntaZSecurityPos(1);
	PuntaZSecurityPos(2);
	PuntaZPosWait(1);
	PuntaZPosWait(2);

	// build position vector
	int ncols, nrows;

	if( Get_Tools20() )
	{
		ncols = UREF_COLS_TOOLS20;
		nrows = UREF_ROWS_TOOLS20;
	}
	else
	{
		ncols = UREF_COLS;
		nrows = UREF_ROWS;
	}

	std::vector<PointF> refs_pos;

	for( int r = 0; r < nrows; r++ )
	{
		for( int c = 0; c < ncols; c++ )
		{
			if( Get_Tools20() )
			{
				refs_pos.push_back( uref_def_tools20[r][c] );
			}
			else
			{
				refs_pos.push_back( uref_def[r][c] );
			}
		}
	}

	Set_Tv_Title( MsgGetString(Msg_01707) ); // search tool reference point
	Set_HeadCameraLight( 1 );

	float dx = refs_pos[nrows*ncols-1].X - refs_pos[0].X;
	float dy = refs_pos[nrows*ncols-1].Y - refs_pos[0].Y;
	double a_theorical = atan2(dy,dx); //angolo teorico

	for( int r = 0; r < nrows; r++ )
	{
		for( int c = 0; c < ncols; c++ )
		{
			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			PointF& pos = refs_pos[r*ncols+c];
			
			if( !Image_match( &pos.X, &pos.Y, UGEIMG ) )
			{
				W_Mess( MsgGetString(Msg_01709) );
				
				if( !ManualTeaching( &pos.X, &pos.Y, MsgGetString(Msg_01707) ) )
				{
					if( !Get_Tools20() )
					{
						Set_UgeBlock(0);
					}
					Set_HeadCameraLight( 1 );
					return 0;
				}
			}
		}
	}

	Set_HeadCameraLight( 0 );

	Set_Tv(2);

	// verifica
	for( int r = 0; r < nrows; r++ )
	{
		for( int c = 0; c < ncols; c++ )
		{
			PointF& pos = refs_pos[r*ncols+c];
			if( !ManualTeaching( &pos.X, &pos.Y, MsgGetString(Msg_01707) ) )
			{
				break;
			}
		}
	}

	dx = refs_pos[nrows*ncols-1].X - refs_pos[0].X;
	dy = refs_pos[nrows*ncols-1].Y - refs_pos[0].Y;
	double a_real = atan2(dy,dx); //angolo reale
	
	double da = a_real - a_theorical;

	//ruota vettore teorico tra riferimento e posizione ugello
	for( int i = 0 ; i < 2; i++ )
	{
		float offs_x = QHeader.uge_offs_x[i] * cos(da) - QHeader.uge_offs_y[i] * sin(da);
		float offs_y = QHeader.uge_offs_x[i] * sin(da) + QHeader.uge_offs_y[i] * cos(da);

		QHeader.uge_offs_x[i] = offs_x;
		QHeader.uge_offs_y[i] = offs_y;
	}

	int n = 0;
	for( int r = 0; r < nrows; r++ )
	{
		for( int c = 0; c < ncols; c++ )
		{
			struct CfgUgelli data;
	
			Ugelli->ReadRec(data,n);

			for(int k = 0; k < 2 ; k++)
			{
				float ux = refs_pos[r*ncols+c].X + QHeader.uge_offs_x[k];
				float uy = refs_pos[r*ncols+c].Y + QHeader.uge_offs_y[k];

				if(k == 0)
				{
					data.X_ugeP1 = ux + QParam.CamPunta1Offset_X;
					data.Y_ugeP1 = uy + QParam.CamPunta1Offset_Y;
				}
				else
				{
					data.X_ugeP2 = ux + QParam.CamPunta2Offset_X;
					data.Y_ugeP2 = uy + QParam.CamPunta2Offset_Y;
				}
			}
			
			Ugelli->SaveRec(data,n);
			n++;
		}
	}

	Set_Tv(3);

	if( !Get_Tools20() )
	{
		Set_UgeBlock(0);
	}

	Ugelli->ReadRec(DatiUge,uge_rec);
	return 1;
}

int Uge_CamImgPar(void)
{
	return ShowImgParams( UGEIMG );
}

int Uge_CamAppRef(void)
{
	Set_Tv(2); // predispone per non richiudere immagine su video

	if( !Get_Tools20() )
	{
		Set_UgeBlock(1);
	}

	PointF refs_pos;

	if( Get_Tools20() )
	{
		refs_pos = uref_def_tools20[0][0];
	}
	else
	{
		refs_pos = uref_def[0][0];
	}

	if( ManualTeaching( &refs_pos.X, &refs_pos.Y, MsgGetString(Msg_01710) ) )
	{
		if( ImageCaptureSave( UGEIMG, YESCHOOSE ) )
		{
			// errore
			Set_Tv(3);
			Set_UgeBlock(0);
			return 0;
		}
	}

	Set_Tv(3);

	if( !Get_Tools20() )
	{
		Set_UgeBlock(0);
	}
	return 1;
}

int Uge_AutoAppP1(void)
{
	if(DatiUge.NozzleAllowed==2)
	{
		bipbip();
		return 0;
	}

	UgeClass* UgeObj = new UgeClass(DatiUge);
	UgeObj->AutoApp(1);
	UgeObj->GetRec(&DatiUge);
	Ugelli->SaveRec(DatiUge,uge_rec);
	delete UgeObj;

	Ugelli->ReadRec(DatiUge,uge_rec);
	return 1;
}

int Uge_AutoAppSeqP1(void)
{
	if(DatiUge.NozzleAllowed==2)
	{
		bipbip();
		return 0;
	}

	Ugelli->AutoApp_Seq(1);
	Ugelli->ReadRec(DatiUge,uge_rec);
	return 1;
}

int Uge_ZAppP1()
{
	if( DatiUge.NozzleAllowed == 2 )
	{
		bipbip();
		return 0;
	}

	G_UgeZLas( 1, uge_rec );
	return 1;
}

int Uge_ZAppP2()
{
	if( DatiUge.NozzleAllowed == 1 )
	{
		bipbip();
		return 0;
	}

	G_UgeZLas( 2, uge_rec );
	return 1;
}

int Uge_AutoAppP2(void)
{
	if(DatiUge.NozzleAllowed==1)
	{
		bipbip();
		return 0;
	}

	UgeClass* UgeObj = new UgeClass(DatiUge);
	UgeObj->AutoApp(2);
	UgeObj->GetRec(&DatiUge);
	Ugelli->SaveRec(DatiUge,uge_rec);
	delete UgeObj;

	Ugelli->ReadRec(DatiUge,uge_rec);
	return 1;
}

int Uge_AutoAppSeqP2(void)
{
	if(DatiUge.NozzleAllowed==1)
	{
		bipbip();
		return 0;
	}

	Ugelli->AutoApp_Seq(2);
	Ugelli->ReadRec(DatiUge,uge_rec);
	return 1;
}


// Fine configurazione ugelli >> S020801
//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
// Posizione default caricatori >> S010801

// Gestione autoapprend. posizione caricatori
int Caric_AutoApp(void)
{
	auto_text1[0]=0;
	auto_text3[0]=0;
	sprintf(auto_text2,"   %s  %d", MsgGetString(Msg_00311) ,CC_Caric[car_rec].code);

	float c_ax = CC_Caric[car_rec].x;
	float c_ay = CC_Caric[car_rec].y;
	
	if( ManualTeaching( &c_ax, &c_ay, MsgGetString(Msg_00026), AUTOAPP_COMP ) )
	{
		CC_Caric[car_rec].x = c_ax;
		CC_Caric[car_rec].y = c_ay;
		return 1;
	}
	return 0;
}

int Caric_Avanz(void)
{
	//GF_13_06_2011  -  notifica modo demo attivo
	if( QParam.DemoMode )
		W_Mess( MsgGetString(Msg_05165) );

	// Avanzamento corto caricatore selezionato (senza flag "avanzato").
	//THFEEDER - da rivedere per ora passo tipo 0 = tape/air
	CaricMov( CC_Caric[car_rec].code, 0, 0, 0, 0 );
	CaricWait(0,0); // attesa ready

	return 1;
}
// Fine Posizione default caricatori
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------

//GF_14_07_2011 - Rimuove parametri immagine
void Remove_image_par( int ImageType )
{
	char NameFile[MAXNPATH];

	SetImageName( NameFile, ImageType, DATA );
	ImgDataDelete( NameFile );

	SetImageName( NameFile, ImageType, IMAGE );
	ImgDataDelete( NameFile );

	SetImageName( NameFile, ImageType, ELAB );
	ImgDataDelete( NameFile );
} // Remove_image_par

// Fine gestione parametri immagine
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Gestione posizione punto colla >> S070801

int PColla_ndisp;

// Gestione autoapprend. posizione punto colla SMOD090403
int Col_auto()
{
	float c_ax=X_colla;
	float c_ay=Y_colla;

	#ifndef __DISP2
	// autoappr. pos. p. colla
	if( ManualTeaching(&c_ax,&c_ay,MsgGetString(Msg_00699)) )
	#else
	char titolo[80];
	snprintf( titolo, 80, "%s - %d", MsgGetString(Msg_00699), PColla_ndisp );
	if( ManualTeaching( &c_ax, &c_ay, titolo ) )
	#endif
	{
		X_colla=c_ax;
		Y_colla=c_ay;

		if(colla_test)
		{
			CC_Dispenser.X_colla=X_colla;
			CC_Dispenser.Y_colla=Y_colla;

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

	return 1;
}

// test punti colla iniziali - SMOD090403
int Colla_test(void)
{
	Dosatore->GetConfig(PColla_ndisp,CC_Dispenser);

	if(colla_test)
	{
		X_colla=CC_Dispenser.X_colla;
		Y_colla=CC_Dispenser.Y_colla;
	}
	else
	{
		#ifndef __DISP2
		Read_pcolla(X_colla, Y_colla);
		#else
		Read_pcolla(PColla_ndisp,X_colla, Y_colla);
		#endif
	}

	#ifndef __DISP2
	Dosa_steady(X_colla, Y_colla);
	#else
	Dosa_steady(PColla_ndisp,X_colla, Y_colla);
	#endif

	return 1;
}

// Fine gestione posizione punto colla
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Gestione tempi di dosaggio >> S070801
int DosaSet_ndisp;
int DosaSet_CurRec;
int DosaSet_NewRec;


void Dosa_DelRec(void)
{
	if(DosaSet_CurRec==QHeader.CurDosaConfig[DosaSet_ndisp-1])
	{
		W_Mess( MsgGetString(Msg_01745) );
		return;
	}

	if(!W_Deci(0,MsgGetString(Msg_01744)))
	{
		return;
	}

	if(DosaSet_CurRec<QHeader.CurDosaConfig[DosaSet_ndisp-1])
	{
		QHeader.CurDosaConfig[DosaSet_ndisp-1]--;
		Mod_Cfg(QHeader);
	}

	int nrec=Dosatore->GetConfigNRecs(DosaSet_ndisp);

	struct CfgDispenser *data=new struct CfgDispenser[nrec];

	for(int i=0;i<nrec;i++)
	{
		Dosatore->ReadConfig(DosaSet_ndisp,data[i],i);
	}

	Dosatore->CloseConfig(DosaSet_ndisp);

	Dosatore->CreateConfig(DosaSet_ndisp);

	Dosatore->OpenConfig(DosaSet_ndisp);

	int count=0;

	for(int i=0;i<nrec;i++)
	{
		if(i!=DosaSet_CurRec)
		{
			Dosatore->WriteConfig(DosaSet_ndisp,data[i],count++);
		}
	}

	delete[] data;

	DosaSet_CurRec--;

	if(DosaSet_CurRec<0)
	{
		DosaSet_CurRec=0;
	}

	Dosatore->ReadConfig(DosaSet_ndisp,CC_Dispenser,DosaSet_CurRec);
}

void DosaSet_PGUp(void)
{
	if(DosaSet_CurRec==0)
	{
		bipbip();
		return;
	}

	DosaSet_CurRec--;
	Dosatore->ReadConfig(DosaSet_ndisp,CC_Dispenser,DosaSet_CurRec);

	if(DosaSet_NewRec)
	{
		DosaSet_NewRec=0;
	}
}

void DosaSet_PGDown(void)
{
	if(DosaSet_NewRec)
	{
		bipbip();
		return;
	}

	DosaSet_CurRec++;

	if(DosaSet_CurRec==Dosatore->GetConfigNRecs(DosaSet_ndisp))
	{
		DosaSet_NewRec=1;
		memset(&CC_Dispenser,(char)0,sizeof(CC_Dispenser));
	}
	else
	{
		Dosatore->ReadConfig(DosaSet_ndisp,CC_Dispenser,DosaSet_CurRec);
	}
}


// Fine gestione tempi di dosaggio
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Gestione configurazione offset dosatore >> S070801
int DosaOffs_ndisp;

// Gestione autoapprendimento offset dosatore
int Odos_auto()
{
	float c_ax, c_ay;
	float dos_x, dos_y;
	float xp_tlc, yp_tlc;

	c_ax=0;
	c_ay=0;

	CPan* panOff = new CPan( 22, 3, MsgGetString(Msg_00643), MsgGetString(Msg_00644), MsgGetString(Msg_00645) );

	// autoappr. posizione dosatore
	#ifndef __DISP2
	if( ManualTeaching(&c_ax,&c_ay,MsgGetString(Msg_00658), AUTOAPP_NOCAM | AUTOAPP_DOSAT ) )
	#else
	char titolo[80];
	snprintf( titolo, 80, "%s - %d", MsgGetString(Msg_00658), DosaOffs_ndisp );
	Dosatore->SelectCurrentDisp( DosaOffs_ndisp );
	if( ManualTeaching(&c_ax,&c_ay, titolo, AUTOAPP_NOCAM | AUTOAPP_DOSAT ) )
	#endif
	{
		dos_x = c_ax;
		dos_y = c_ay;

		c_ax = dos_x-CC_Dispenser.CamOffset_X;
		c_ay = dos_y-CC_Dispenser.CamOffset_Y;

		delete panOff;

		// autoappr. posizione telec.
		if( ManualTeaching(&c_ax,&c_ay,MsgGetString(Msg_00028)) )
		{
			xp_tlc=c_ax;
			yp_tlc=c_ay;

			if( W_Deci( 0, MsgGetString(Msg_00250) ) )
			{
				CC_Dispenser.CamOffset_X=dos_x-xp_tlc;
				CC_Dispenser.CamOffset_Y=dos_y-yp_tlc;
			}
	  	}
	}
   	else
	{
   		delete panOff;
	}

	return 1;
}

// Fine gestione configurazione offset dosatore
//-----------------------------------------------------------------------



//---------------------------------------------------------------------------------
// Calcola la costante PIXEL-MILLIMETRI per la telecamera sulla testa
// Ritorna 1 se ok, 0 altrimenti
//---------------------------------------------------------------------------------
int HeadCam_Scale()
{
	// apprendimento posizione
	float px = Vision.scalaXCoord;
	float py = Vision.scalaYCoord;

	Set_Finec(ON);
	NozzleXYMove( px, py );

	// setta i parametri della telecamera
	char NameFile[MAXNPATH];
	SetImageName( NameFile, HEADCAM_SCALE_IMG, DATA );
	if( access(NameFile,0) == 0 )
	{
		img_data imgData;
		ImgDataLoad( NameFile, &imgData );

		// set brightess and contrast
		SetImgBrightCont( imgData.bright*655, imgData.contrast*655 );
	}

	Wait_PuntaXY();

	Set_Tv( 2, CAMERA_HEAD );

	bool abort = false;
	bool imageFound = false;
	float deltaXpix = 0.f;
	float deltaYpix = 0.f;

	if( ManualTeaching( &px, &py, MsgGetString(Msg_01251), AUTOAPP_NOEXITRESET, CAMERA_HEAD ) )
	{
		// save PATTERN
		//ImageCaptureSave( HEADCAM_SCALE_IMG, NOCHOOSE, CAMERA_HEAD );
		ImageCaptureSave( HEADCAM_SCALE_IMG, YESCHOOSE, CAMERA_HEAD );

		Set_Tv(3);

		Vision.scalaXCoord = px;
		Vision.scalaYCoord = py;

		deltaXpix = px + Vision.scale_off;
		deltaYpix = py + Vision.scale_off;

		// porta la testa in posizione di test ed esegue match
		SetNozzleXYSpeed_Index( Vision.matchSpeedIndex );

		imageFound = Image_match( &deltaXpix, &deltaYpix, HEADCAM_SCALE_IMG );
	}
	else
	{
		abort = true;
	}

	Set_Tv(3);
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	Set_Finec(OFF);

	if( abort )
	{
		return 0;
	}

	// calcolo scala
	if( !imageFound || fabs(deltaXpix) < 1.f || fabs(deltaYpix) < 1.f )
	{
		Vision.mmpix_x = HEADCAM_MMPIX_DEF;
		Vision.mmpix_y = HEADCAM_MMPIX_DEF;
		VisDataSave( Vision );

		W_Mess( MsgGetString(Msg_00068) );
		return 0;
	}

	//CONVERSIONE RISULTATI
	Vision.mmpix_x = (float)Vision.scale_off/fabs(deltaXpix);
	Vision.mmpix_y = (float)Vision.scale_off/fabs(deltaYpix);

	if( Vision.mmpix_x <= HEADCAM_MMPIX_MIN || Vision.mmpix_x >= HEADCAM_MMPIX_MAX ||
		Vision.mmpix_y <= HEADCAM_MMPIX_MIN || Vision.mmpix_y >= HEADCAM_MMPIX_MAX )
	{
		Vision.mmpix_x = HEADCAM_MMPIX_DEF;
		Vision.mmpix_y = HEADCAM_MMPIX_DEF;
		VisDataSave( Vision );

		W_Mess( MsgGetString(Msg_00068) );
		return 0;
	}

	VisDataSave( Vision );

	W_Mess( MsgGetString(Msg_00069) );
	return 1;
}



//-----------------------------------------------------------------------
// // Gestione Offset Punta-Telecamera Integr. S160801

extern int offCal_nozzle;
extern char offCal_tool;
extern float offCal_pcb;

int POff_InkPosition()
{
	float x = QParam.OFFX_ink;
	float y = QParam.OFFY_ink;

	if( ManualTeaching( &x, &y, MsgGetString(Msg_00861) ) )
	{
		QParam.OFFX_ink = x;
		QParam.OFFY_ink = y;
		Mod_Par(QParam);
	}
	return 1;
}

int POff_InkCalibration()
{
	int nozzle = offCal_nozzle;
	char tool = offCal_tool;

	struct CfgUgelli udat;
	Ugelli->ReadRec( udat, tool-'A' );

	if(!(udat.NozzleAllowed & nozzle))
	{
		bipbip();
		return 0;
	}

	// apprendimento punto di mark
	PointF mark( QParam.OFFX_mark, QParam.OFFY_mark );
	if( !ManualTeaching( &mark.X, &mark.Y, MsgGetString(Msg_00860) ) )
	{
		return 0;
	}

	float ofx, ofy;
	int uge_code[2];

	if( nozzle == 1 )
	{
		ofx = QParam.CamPunta1Offset_X;
		ofy = QParam.CamPunta1Offset_Y;
		Ugelli->SetFlag(1,0);
		uge_code[0] = tool;
	}
	else
	{
		ofx = QParam.CamPunta2Offset_X;
		ofy = QParam.CamPunta2Offset_Y;
		Ugelli->SetFlag(0,1);
		uge_code[1] = tool;
	}

	// prelievo ugello
	Ugelli->DoOper(uge_code);
	Set_Vacuo(nozzle,1);


	for( int rot = 0; rot < 4; rot++ )
	{
		// inchiostratura ugello
		Set_Finec(ON);
		NozzleXYMove( QParam.OFFX_ink + ofx, QParam.OFFY_ink + ofy );
		Wait_PuntaXY();
		Set_Finec(OFF);

		for( int i = 0; i < 2; i++ )
		{
			PuntaZPosMm(nozzle,GetPianoZPos(nozzle)-QHeader.InkZPos);
			PuntaZPosWait(nozzle);
			delay(500);
			PuntaZSecurityPos(nozzle);
			PuntaZPosWait(nozzle);
		}

		if(!CheckNozzlesUp())
		{
			return 0;
		}

		// marcatura punto
		float x = mark.X + ofx + rot * 2; // sposta i punti 2mm sulla destra
		float y = mark.Y + ofy;

		Set_Finec(ON);
		NozzleXYMove( x, y );
		Wait_PuntaXY();
		Set_Finec(OFF);

		PuntaRotDeg( rot*90, nozzle );
		Wait_EncStop( nozzle );

		for( int i = 0; i < 2; i++ )
		{
			PuntaZPosMm(nozzle,GetPianoZPos(nozzle)-offCal_pcb);
			PuntaZPosWait(nozzle);
			delay(400);
			PuntaZSecurityPos(nozzle);
			PuntaZPosWait(nozzle);
		}
	}

	Set_Vacuo(nozzle,0);

	if( !CheckNozzlesUp() )
	{
		return 0;
	}

	Set_Finec(ON);


	float dx[4], dy[4];
	int ret = 1;

	for( int rot = 0; rot < 4; rot++ )
	{
		// apprendimento punto
		float x = mark.X + rot * 2; // sposta i punti 2mm sulla destra
		float y = mark.Y;

		if( !ManualTeaching( &x, &y, (offCal_nozzle==1) ? MsgGetString(Msg_00029) : MsgGetString(Msg_00030)) )
		{
			ret = 0;
			break;
		}

		// calcolo delta
		dx[rot] = x - (mark.X + rot * 2);
		dy[rot] = y - mark.Y;
	}

	if( ret )
	{
		// calcolo centro della marcatura (punto medio)
		float xMean = 0;
		float yMean = 0;
		for( int rot = 0; rot < 4; rot++ )
		{
			xMean += dx[rot];
			yMean += dy[rot];
		}
		xMean /= 4;
		yMean /= 4;

		// aggiorno offset
		float ox = ofx - xMean;
		float oy = ofy - yMean;

		if( offCal_nozzle == 1 )
		{
			QParam.CamPunta1Offset_X = ox;
			QParam.CamPunta1Offset_Y = oy;
		}
		else
		{
			QParam.CamPunta2Offset_X = ox;
			QParam.CamPunta2Offset_Y = oy;
		}

		QParam.OFFX_mark = mark.X;
		QParam.OFFY_mark = mark.Y;

		Mod_Par(QParam);

		W_Mess( MsgGetString(Msg_00114) );
	}

	Set_Finec(OFF);

	// deposito ugello
	if( W_Deci( 1, MsgGetString(Msg_00200) ) )
	{
		Ugelli->Depo( nozzle );
	}

	return ret;
}


int POff_InkDeltaHeights()
{
	int nozzle = offCal_nozzle;
	char tool = offCal_tool;

	struct CfgUgelli udat;
	Ugelli->ReadRec( udat, tool-'A' );

	if(!(udat.NozzleAllowed & nozzle))
	{
		bipbip();
		return 0;
	}

	// apprendimento punto di mark
	PointF mark( QParam.OFFX_mark, QParam.OFFY_mark );
	if( !ManualTeaching( &mark.X, &mark.Y, MsgGetString(Msg_00860) ) )
	{
		return 0;
	}

	float ofx, ofy;
	int uge_code[2];

	if( nozzle == 1 )
	{
		ofx = QParam.CamPunta1Offset_X;
		ofy = QParam.CamPunta1Offset_Y;
		Ugelli->SetFlag(1,0);
		uge_code[0] = tool;
	}
	else
	{
		ofx = QParam.CamPunta2Offset_X;
		ofy = QParam.CamPunta2Offset_Y;
		Ugelli->SetFlag(0,1);
		uge_code[1] = tool;
	}

	// prelievo ugello
	Ugelli->DoOper(uge_code);
	Set_Vacuo(nozzle,1);

	// inchiostratura ugello
	Set_Finec(ON);
	NozzleXYMove( QParam.OFFX_ink + ofx, QParam.OFFY_ink + ofy );
	Wait_PuntaXY();
	Set_Finec(OFF);

	for( int i = 0; i < 2; i++ )
	{
		PuntaZPosMm(nozzle,GetPianoZPos(nozzle)-QHeader.InkZPos);
		PuntaZPosWait(nozzle);
		delay(500);
		PuntaZSecurityPos(nozzle);
		PuntaZPosWait(nozzle);
	}

	if(!CheckNozzlesUp())
	{
		return 0;
	}

	// marcatura punto
	Set_Finec(ON);
	NozzleXYMove( mark.X + ofx, mark.Y + ofy );
	Wait_PuntaXY();
	Set_Finec(OFF);

	for( int i = 0; i < 2; i++ )
	{
		PuntaZPosMm(nozzle,GetPianoZPos(nozzle)-offCal_pcb);
		PuntaZPosWait(nozzle);
		delay(400);
		PuntaZSecurityPos(nozzle);
		PuntaZPosWait(nozzle);
	}

	Set_Vacuo(nozzle,0);

	if( !CheckNozzlesUp() )
	{
		return 0;
	}

	// apprendimento punto
	Set_Finec(ON);
	PointF pos = mark;

	if( ManualTeaching( &pos.X, &pos.Y, (offCal_nozzle==1) ? MsgGetString(Msg_00029) : MsgGetString(Msg_00030)) )
	{
		Set_Finec(OFF);

		// move head to end position
		HeadEndMov();

		//
		if( !W_Deci( 1, MsgGetString(Msg_00264) ) )
		{
			return 0;
		}

		//TODO - verificare se necessari messaggi
		// ripete marcatura e verifica (INSERIRE PIANO PIU' ALTO)

		Set_Vacuo(nozzle,1);

		// inchiostratura ugello
		Set_Finec(ON);
		NozzleXYMove( QParam.OFFX_ink + ofx, QParam.OFFY_ink + ofy );
		Wait_PuntaXY();
		Set_Finec(OFF);

		for( int i = 0; i < 2; i++ )
		{
			PuntaZPosMm(nozzle,GetPianoZPos(nozzle)-QHeader.InkZPos);
			PuntaZPosWait(nozzle);
			delay(500);
			PuntaZSecurityPos(nozzle);
			PuntaZPosWait(nozzle);
		}

		if(!CheckNozzlesUp())
		{
			return 0;
		}

		// marcatura punto
		Set_Finec(ON);
		NozzleXYMove( mark.X + ofx, mark.Y + ofy );
		Wait_PuntaXY();
		Set_Finec(OFF);

		for( int i = 0; i < 2; i++ )
		{
			PuntaZPosMm(nozzle,GetPianoZPos(nozzle)-offCal_pcb);
			PuntaZPosWait(nozzle);
			delay(400);
			PuntaZSecurityPos(nozzle);
			PuntaZPosWait(nozzle);
		}


		Set_Vacuo(nozzle,0);

		if( !CheckNozzlesUp() )
		{
			return 0;
		}

		// apprendimento punto
		Set_Finec(ON);
		ManualTeaching( &pos.X, &pos.Y, (offCal_nozzle==1) ? MsgGetString(Msg_00029) : MsgGetString(Msg_00030));
	}
	Set_Finec(OFF);

	// deposito ugello
	if( W_Deci( 1, MsgGetString(Msg_00200) ) )
	{
		Ugelli->Depo( nozzle );
	}

	return 1;
}


//-------------------------------------------------------------------------
// Calibrazione asse Z

int pos_zcal[2] = {0,0};
float delta_zcal = 0.f;


//---------------------------------------------------------------------------
// finestra: Z axis calibration
//---------------------------------------------------------------------------
class ZAxisCalibrationUI : public CWindowParams
{
public:
	ZAxisCalibrationUI( CWindow* parent, int nozzle ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU | WIN_STYLE_EDITMODE_ON );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 46, 5 );
		SetTitle( MsgGetString(Msg_01208) );

		m_nozzle = nozzle;
	}

	int GetExitCode()
	{
		return m_exitCode;
	}

	typedef enum
	{
		Z_CONST,
		D_STEPS,
		D_MM
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[Z_CONST] = new C_Combo( 5, 1, MsgGetString(Msg_01610), 8, CELL_TYPE_UDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 3 );
		m_combos[D_STEPS] = new C_Combo( 5, 2, MsgGetString(Msg_00963), 6, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[D_MM]    = new C_Combo( 5, 3, MsgGetString(Msg_00964), 6, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[D_MM]->SetVMinMax( 0.f, 50.f );
		//
		m_combos[Z_CONST]->SetTxt( (m_nozzle==1) ? QHeader.Step_Trasl1 : QHeader.Step_Trasl2 );
		m_combos[D_STEPS]->SetTxt( pos_zcal[0]-pos_zcal[1] );

		// add to combo list
		m_comboList->Add( m_combos[Z_CONST], 0, 0 );
		m_comboList->Add( m_combos[D_STEPS], 1, 0 );
		m_comboList->Add( m_combos[D_MM]   , 2, 0 );
	}

	void onShow()
	{
		tips = new CPan( 22, 2, MsgGetString(Msg_00296), MsgGetString(Msg_00297) );
	}

	void onRefresh()
	{
		m_combos[D_MM]->SetTxt( delta_zcal );
	}

	void onEdit()
	{
		delta_zcal = m_combos[D_MM]->GetFloat();
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_ESC:
				m_exitCode = WIN_EXITCODE_ESC;
				break;

			case K_ENTER:
				if( delta_zcal != 0.f )
				{
					forceExit();
					m_exitCode = WIN_EXITCODE_ENTER;
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
		delete tips;
	}

	CPan* tips;
	int m_exitCode;
	int m_nozzle;
};


void G_ZCalManual( int nozzle )
{
	int err = 0;

	Set_Finec(ON);
	NozzleXYMove( QParam.LX_mincl+(QParam.LX_maxcl-QParam.LX_mincl)/2, QParam.LY_mincl+(QParam.LY_maxcl-QParam.LY_mincl)/2 );
	Wait_PuntaXY();
	Set_Finec(OFF);

	while( 1 )
	{
		for(int i=0;i<2;i++)
		{
			char tipbuf[80];

			if(i==0)
			{
				strncpy(tipbuf, MsgGetString(Msg_01999), 80 );
			}
			else
			{
				strncpy( tipbuf, MsgGetString(Msg_02000), 80 );
			}

			if(!AutoAppZPosStep(MsgGetString(Msg_01208), tipbuf, nozzle, QParam.LX_mincl+(QParam.LX_maxcl-QParam.LX_mincl)/2,QParam.LY_mincl+(QParam.LY_maxcl-QParam.LY_mincl)/2,pos_zcal[i],APPRENDZPOS_VACUO))
			{
				err=1;
				break;
			}

			PuntaZSecurityPos( nozzle );
			PuntaZPosWait( nozzle );
		}

		if(err)
		{
			break;
		}
		else
		{
			if(pos_zcal[0]<=pos_zcal[1])
			{
				W_Mess( MsgGetString(Msg_02001) );
			}
			else
			{
				break;
			}
    	}

	}

	PuntaZSecurityPos( nozzle );
	PuntaZPosWait( nozzle );

	if(err)
	{
		return;
	}

	ZAxisCalibrationUI win( 0, nozzle );
	win.Show();
	win.Hide();

	if( win.GetExitCode() == WIN_EXITCODE_ENTER )
	{
		float zstep_trasl=((float)(pos_zcal[0]-pos_zcal[1]))/delta_zcal;

		char buf[120];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01609), zstep_trasl );
		if( W_Deci( 1, buf, MSGBOX_YLOW ) )
		{
			if( nozzle == 1 )
			{
				QHeader.Step_Trasl1 = zstep_trasl;
			}
			else
			{
				QHeader.Step_Trasl2 = zstep_trasl;
			}

			Mod_Cfg(QHeader);
		}
	}
}


//-------------------------------------------------------------------------
//Regolazione fine Z offset ugello (con laser) S110202

int car;
int ncar;
char zuge_punta = 0;
struct CarDat record_caric;
SPackageData record_pack;
extern int ass_caricat (int	_cd, int	_res = 0,int punta=1);
extern int ass_ugelli(int *n_ugello,int *o_ugello,int *mount_flag);


int G_UgeZLas( int punta, int ugello )
{
	char files_open=0;
	float uge_err;
	int new_uge[2];

	int prePrelQuant=-1;
	
	guiDeskTop->ShowMenuItem( false );
	
	CarFile=new FeederFile(QHeader.Conf_Default);
	Caricatore=new FeederClass(CarFile);
	
	if(CarFile->opened)
	{
		files_open=1;
	}
	else
	{
		files_open=0;
	}

	if(files_open)
	{
		if( !PackagesLib_Load( QHeader.Lib_Default ) )
		{
			delete Caricatore;
			delete CarFile;
			CarFile=NULL;
			guiDeskTop->ShowMenuItem( true );
			return(0);
		}
	}
	else
	{
		guiDeskTop->ShowMenuItem( true );
		return(0);
	}

	bool found = false;
	for( ncar = 0; ncar < MAXCAR; ncar++ )
	{
		CarFile->ReadRec(ncar,record_caric);

		if( (record_caric.C_PackIndex != 0) && (record_caric.C_PackIndex <= MAXPACK) )
		{
			if( CarFile->GetRec_AssociatedPack(ncar,record_pack) )
			{
				if( strchr( record_pack.tools, char(ugello+65) ) )
				{
					found = true;
					break;
				}
			}
		}
  	}

	if( !found )
	{
		//nessun caricatore assegnato a un package con ugello specificato
		car = 11;
		ncar=0;
		CarFile->ReadRec(ncar,record_caric);

		if( (record_caric.C_PackIndex!=0) && (record_caric.C_PackIndex <= MAXPACK) )
		{
			if(!CarFile->GetRec_AssociatedPack(ncar,record_pack))
			{
				record_caric.C_PackIndex=0;
				record_pack.name[0]=0;
			}
		}
		else
		{
			record_pack.name[0]=0;
		}
	}
	else
	{
		car = record_caric.C_codice;
	}

	//TODO mettere codice caricatore di partenza in feeder select
	FeederSelect* TT_FSel = new FeederSelect( 0 );
	int carcode = TT_FSel->Activate();
	delete TT_FSel;

	if( carcode > 0 )
	{
		//estrai numero punta da combo
		zuge_punta = punta;

		Caricatore->SetCode( carcode );
		record_caric = Caricatore->GetData();

		if(zuge_punta==1)
		{
			Ugelli->SetFlag(1,0);
		}
		else
		{
			Ugelli->SetFlag(0,1);
		}

		new_uge[zuge_punta-1] = ugello+65; //estrai nome nuovo ugello da caricare

		Ugelli->SaveRec(DatiUge,new_uge[zuge_punta-1]-'A');
		Ugelli->DoOper(new_uge);      //prendi/cambia ugello
		Ugelli->ReadRec(DatiUge,new_uge[zuge_punta-1]-'A');

		struct PrelStruct preldata=PRELSTRUCT_DEFAULT;

		preldata.punta = zuge_punta;
		preldata.zup = 0;
		preldata.package = &record_pack;
		preldata.caric = Caricatore;

		if(record_caric.C_codice<160) //se non vassoi
		{
			preldata.zprel=GetZCaricPos(punta)-record_caric.C_offprel;  // n. corsa prelievo
		}
		else
		{
			preldata.zprel=GetPianoZPos(punta)-record_caric.C_offprel;  // n. corsa prelievo
		}

		Set_Finec(ON);
		Caricatore->GoPos(punta);   //vai a pos. di prelievo

		PuntaRotDeg(Get_PickThetaPosition(record_caric.C_codice,record_pack),zuge_punta);

		Wait_EncStop(zuge_punta);
		Wait_PuntaXY();
		Set_Finec(OFF);
		Caricatore->WaitReady();

		PrelComp(preldata);
		prePrelQuant = Caricatore->GetData().C_quant;

		PuntaRotDeg(angleconv(record_caric.C_codice,LASANGLE(record_pack.orientation)),zuge_punta,BRUSH_REL);
		Wait_EncStop(zuge_punta);

		Caricatore->DecNComp();
		DecMagaComp(record_caric.C_codice);

		Caricatore->Avanza();

		W_Mess( MsgGetString(Msg_00243) );

		if( ScanTest( zuge_punta, LASSCAN_ZEROCUR | LASSCAN_BIGWIN, 0, record_pack.z, 0, &uge_err ) )
		{
			//correggi Zoffset
			PuntaZPosMm(zuge_punta,-uge_err,REL_MOVE); //spostamento di uge_err passi
			//setta posizione attuale=posizione precedente al movimento
			PuntaZPosWait(zuge_punta);
			DatiUge.Z_offset[zuge_punta-1]+=uge_err;
			Ugelli->SaveRec(DatiUge,ugello);
			Ugelli->ReloadCurUge();
		}
	}

	PuntaZSecurityPos(1);
	PuntaZSecurityPos(2);
	PuntaZPosWait(2);
	PuntaZPosWait(1);

	if(zuge_punta)
	{
		if((record_caric.C_codice>=FIRSTTRAY) && (prePrelQuant!=-1))
		{
			//PuntaRotDeg_component_safe(zuge_punta,-angleconv(record_caric.C_codice,LASANGLE(record_pack.lasangle)));
			ScaricaCompOnTray(Caricatore,zuge_punta,prePrelQuant);
		}
		else
		{
			ScaricaComp(zuge_punta);          //scarica i componenti
		}

		PuntaRotDeg(0,zuge_punta);

		Set_Vacuo(zuge_punta,OFF);          //Vuoto OFF

		while(!Check_PuntaRot(zuge_punta));
	}

	//destroy oggetti caricatori
	delete Caricatore;
	delete CarFile;
	CarFile=NULL;

	guiDeskTop->ShowMenuItem( true );

	return 1;
}

//-------------------------------------------------------------------------
// Gestione CCal

#ifdef __SNIPER
//SMOD140203
int FindCCal(int punta,float& center,float& deltap)
{
	int k;
	float media=0;
	
	float min_position = 99999999;
	float max_position = 0;

	float *position=new float[512];
	float *angle=new float[512];

	SniperModule* sniper = (punta == 1) ? Sniper1 : Sniper2;

	PuntaRotStep(0,punta);
	
	//SMOD250903
	while(!Check_PuntaRot(punta))
	{
		FoxPort->PauseLog();
	}
	FoxPort->RestartLog();

	CWaitBox waitBox( 0, 15, MsgGetString(Msg_00291), 512 );
	waitBox.Show();
	
	int error = 0;
	int astep;
	
	if(punta == 1)
	{
		astep = QHeader.Enc_step1 / 512;
	}
	else
	{
		astep = QHeader.Enc_step2 / 512;
	}		

	for(k=0;k<512;k++)
	{
		PuntaRotStep(astep,punta,BRUSH_REL);

		angle[k] = (astep*k)*360.0/ ((punta==1) ? QHeader.Enc_step1 : QHeader.Enc_step2);

		//SMOD250903
		while(!Check_PuntaRot(punta))
		{
			FoxPort->PauseLog();
		}
		FoxPort->RestartLog();


		int measure_status;
		int measure_angle;
		float measure_position;
		float measure_shadow;
		sniper->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow );

		if( measure_status )
		{
			float tmpPosition;
			tmpPosition = measure_position / 1000.f;
		
			position[k] = (( SNIPER_RIGHT_USABLE - SNIPER_LEFT_USABLE + 1 ) * QHeader.sniper_kpix_um[punta-1] / 1000.0) - tmpPosition;
			
			media+=position[k];
			
			if(min_position > position[k])
			{
				min_position=position[k];
			}
		
			if(max_position < position[k])
			{
				max_position=position[k];
			}
		}
		else
		{
			bipbip();
			char buf[120];

			#ifdef __SNIPER
			switch( measure_status )
			{
				case STATUS_R_BLK:
					strncpy( buf, MsgGetString(Msg_00169), sizeof(buf) );
					break;	
				
				default:
					snprintf( buf, sizeof(buf), MsgGetString(Msg_00104), measure_status );
					break;
			}
			#endif

			W_Mess(buf);
			error = 1;
			break;
		}

		waitBox.Increment();
	}

	waitBox.Hide();
	
	if(!error)
	{
		center = media / 512;
		deltap = max_position - min_position;

		char buf[120];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01053), punta );

		C_Graph* graph=new C_Graph( 5,3,76,22, buf, GRAPH_NUMTYPEY_FLOAT | GRAPH_NUMTYPEX_FLOAT | GRAPH_AXISTYPE_XY | GRAPH_DRAWTYPE_LINE | GRAPH_SHOWPOSTXT,1);

		graph->SetVMinY(min_position);
		graph->SetVMaxY(max_position);
		graph->SetVMinX(0);
		graph->SetVMaxX(360.0f);
		graph->SetNData(512,0);
		graph->SetTick((float)0.01);

		graph->SetDataY(position,0);
		graph->SetDataX(angle,0);

		graph->Show();

		int c;
		do
		{
			c=Handle();
			if( c != K_ESC)
			{
				graph->GestKey(c);
			}
		} while( c != K_ESC);

		delete graph;
	}

	delete [] position;
	delete [] angle;

	return(!error);
}
#endif

//GF_TEMP
RotCenterCalibStruct rotCenterData;

int CCal_Calc()
{
	Ugelli->Depo( rotCenterData.nozzle );

	float deltap[2];
	float center[2];
	float zpos[2];
	int ugPrel[2] = { -1, -1 };
	
	int done = 1;
		
	for(int i = 0 ; i < 2 ; i++)
	{
		if( i == 1 )
		{
			char tool = 'C';
			if( !ToolSelectionWin( 0, tool, rotCenterData.nozzle ) )
			{
				done = 0;
				break;
			}

			if(rotCenterData.nozzle == 1)
			{
				Ugelli->SetFlag(1,0);
				ugPrel[0] = tool;
			}
			else
			{
				Ugelli->SetFlag(0,1);
				ugPrel[1] = tool;
			}
			if(!Ugelli->DoOper(ugPrel))
			{
				done = 0;
				break;
			}

			Set_Vacuo(rotCenterData.nozzle,ON);
		}		
		
		PuntaZPosMm(rotCenterData.nozzle,0);
		PuntaZPosWait(rotCenterData.nozzle);

		float delta;

		int ret=ScanTest(rotCenterData.nozzle,LASSCAN_BIGWIN,LASSCAN_DEFZRANGE,0,0,&delta);

		if(ret)
		{
			PuntaZPosMm(rotCenterData.nozzle,-delta);
			PuntaZPosWait(rotCenterData.nozzle);
			zpos[i] = GetPhysZPosMm(rotCenterData.nozzle);
		
			rotCenterData.pos_z[rotCenterData.nozzle-1][i] = zpos[i];
						
			if(!FindCCal(rotCenterData.nozzle,center[i],deltap[i]))
			{
				printf( "FindCCal: ERROR\n" );
				done = 0;
				break;
			}
			printf( "FindCCal: OK\n" );
			
			rotCenterData.delta_pos[rotCenterData.nozzle-1][i] = deltap[i];
			rotCenterData.rot_center[rotCenterData.nozzle-1][i] = center[i];
		}
		else
		{
			done = 0;
			break;
		}	
	}
	
	Set_Vacuo(rotCenterData.nozzle,OFF);
	 
	if(done)
	{
		MapTeste.ccal_z_cal_m[rotCenterData.nozzle-1] = (center[0] - center[1]) / (zpos[0] - zpos[1]);
		MapTeste.ccal_z_cal_q[rotCenterData.nozzle-1] = center[0] - MapTeste.ccal_z_cal_m[rotCenterData.nozzle-1] * zpos[0];

		Mod_Map(MapTeste);		
	}
	
	Ugelli->Depo(rotCenterData.nozzle);
	
	return done;
}

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
#define PIANOCAL_DELTA 5
#define PIANOCAL_STEP  1


float xPianoCal;
float yPianoCal;

int PianoCal_Manual()
{
	float zPos = QHeader.Zero_Piano - PIANOCAL_DELTA;
	
	if( AutoAppZPosMm( MsgGetString(Msg_01227), 1, xPianoCal, yPianoCal, zPos, APPRENDZPOS_VACUO ) )
	{
		QHeader.Zero_Piano = zPos;
	}

	PuntaZSecurityPos(1);
	PuntaZPosWait(1);
	return 1;
}

int PianoCal_PosAuto(void)
{
	float x = xPianoCal;
	float y = yPianoCal;
	
	if( ManualTeaching( &x, &y, MsgGetString(Msg_01318) ) )
	{
		xPianoCal = x;
		yPianoCal = y;
	}
}
//---------------------------------------------------------------------------

//GF_30_05_2011
int AuxCam_ParamImg()
{
	return ShowImgParams( EXTCAM_NOZ_IMG );
}

int AuxCam_DeleteParamImg()
{
	Remove_image_par( EXTCAM_NOZ_IMG );
	W_Mess( MsgGetString(Msg_05168) );
	return 1;
}

int AuxCam_App( int nozzle )
{
	#ifdef __DISP2
		#ifndef __DISP2_CAM
		if( !Get_SingleDispenserPar() )
		{
			return false;
		}
		#endif
	#endif

	float px = QParam.AuxCam_X[nozzle-1];
	float py = QParam.AuxCam_Y[nozzle-1];

	Set_Finec(ON);
	NozzleXYMove( px, py );

	// setta i parametri della telecamera
	char NameFile[MAXNPATH];
	SetImageName( NameFile, EXTCAM_NOZ_IMG, DATA );
	if( access(NameFile,0) == 0 )
	{
		img_data imgData;
		ImgDataLoad( NameFile, &imgData );

		// set brightess and contrast
		SetImgBrightCont( imgData.bright*655, imgData.contrast*655 );

		// setup ext cam
		SetExtCam_Light( imgData.filter_p1 );
		SetExtCam_Gain( imgData.filter_p2 );
		SetExtCam_Shutter( imgData.filter_p3 );
	}
	else
	{
		SetExtCam_Light(7);
		SetExtCam_Shutter(DEFAULT_EXT_CAM_SHUTTER);
		SetExtCam_Gain(DEFAULT_EXT_CAM_GAIN);
	}

	Wait_PuntaXY();

	PuntaZPosMm( nozzle, QParam.AuxCam_Z[nozzle-1] );
	PuntaZPosWait( nozzle );

	Set_Tv( 2, CAMERA_EXT );

	int mode = AUTOAPP_EXTCAM | AUTOAPP_CONTROLCAM3;
	mode |= (nozzle == 1) ? AUTOAPP_PUNTA1ON | AUTOAPP_NOEXITRESET: AUTOAPP_PUNTA2ON;

	int teached = 0;
	if( ManualTeaching( &px, &py, MsgGetString(Msg_01358), mode, CAMERA_EXT ) )
	{
		teached = 1;

		QParam.AuxCam_X[nozzle-1] = px;
		QParam.AuxCam_Y[nozzle-1] = py;
		QParam.AuxCam_Z[nozzle-1] = GetXYApp_LastZPos(nozzle);

		Mod_Par(QParam);

		// chiede se apprendere immagine di riferimento
		if( nozzle == 1 )
		{
			if( W_Deci(1,MEMIMAGEQ) )
			{
				ImageCaptureSave( EXTCAM_NOZ_IMG, YESCHOOSE, CAMERA_EXT );
			}
		}
	}

	Set_Tv(3);
	SetExtCam_Light(0);

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	// riporta punta in posizione di sicurezza
	PuntaZSecurityPos( nozzle );
	PuntaZPosWait( nozzle );

	Set_Finec(OFF);
	return teached;
}


//---------------------------------------------------------------------------------
// Calcola la costante PIXEL-MILLIMETRI per la telecamera esterna
// Ritorna TRUE se ok, FALSE altrimenti
//---------------------------------------------------------------------------------
int ExtCam_Scale( int nozzle )
{
	#ifdef __DISP2
		#ifndef __DISP2_CAM
		if( !Get_SingleDispenserPar() )
		{
			return false;
		}
		#endif
	#endif

	// apprendimento posizione
	float px = QParam.AuxCam_X[nozzle-1];
	float py = QParam.AuxCam_Y[nozzle-1];

	Set_Finec(ON);
	NozzleXYMove( px, py );

	// setta i parametri della telecamera
	char NameFile[MAXNPATH];
	SetImageName( NameFile, EXTCAM_NOZ_IMG, DATA );
	if( access(NameFile,0) == 0 )
	{
		img_data imgData;
		ImgDataLoad( NameFile, &imgData );

		// set brightess and contrast
		SetImgBrightCont( imgData.bright*655, imgData.contrast*655 );

		// setup ext cam
		SetExtCam_Light( imgData.filter_p1 );
		SetExtCam_Gain( imgData.filter_p2 );
		SetExtCam_Shutter( imgData.filter_p3 );
	}
	else
	{
		SetExtCam_Light( 7 );
		SetExtCam_Gain( DEFAULT_EXT_CAM_GAIN );
		SetExtCam_Shutter( DEFAULT_EXT_CAM_SHUTTER );
	}

	Wait_PuntaXY();

	PuntaZPosMm( nozzle, QParam.AuxCam_Z[nozzle-1] );
	PuntaZPosWait( nozzle );

	Set_Tv( 2, CAMERA_EXT );

	bool abort = false;
	bool imageFound = false;
	float deltaXpix = 0.f;
	float deltaYpix = 0.f;

	int mode = AUTOAPP_EXTCAM | AUTOAPP_CONTROLCAM3 | AUTOAPP_NOEXITRESET;
	mode |= (nozzle == 1) ? AUTOAPP_PUNTA1ON : AUTOAPP_PUNTA2ON;

	if( ManualTeaching( &px, &py, MsgGetString(Msg_01251), mode, CAMERA_EXT ) )
	{
		Set_Tv(3);

		// save PATTERN
		ImageCaptureSave( EXTCAM_SCALE_IMG, NOCHOOSE, CAMERA_EXT );

		// porta la testa in posizione di test ed esegue match
		SetNozzleXYSpeed_Index( Vision.matchSpeedIndex );
		NozzleXYMove( px + Vision.scale_off, py + Vision.scale_off, AUTOAPP_NOZSECURITY );
		Wait_PuntaXY();

		imageFound = ImageMatch_ExtCam( &deltaXpix, &deltaYpix, EXTCAM_SCALE_IMG, nozzle );
	}
	else
	{
		abort = true;
	}

	Set_Tv(3);
	Set_Vacuo( nozzle, 0 );
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	// riporta punta in posizione di sicurezza
	PuntaZSecurityPos( nozzle );
	PuntaZPosWait( nozzle );

	SetExtCam_Light( 0 );
	Set_Finec(OFF);

	if( abort )
	{
		return 0;
	}

	// calcolo scala
	if( !imageFound || fabs(deltaXpix) < 1.f || fabs(deltaYpix) < 1.f )
	{
		QParam.AuxCam_Scale_x[nozzle-1] = EXTCAM_MMPIX_DEF;
		QParam.AuxCam_Scale_y[nozzle-1] = EXTCAM_MMPIX_DEF;
		Mod_Par( QParam );

		W_Mess( MsgGetString(Msg_00068) );
		return 0;
	}

	QParam.AuxCam_Scale_x[nozzle-1] = (float)Vision.scale_off/fabs(deltaXpix);
	QParam.AuxCam_Scale_y[nozzle-1] = (float)Vision.scale_off/fabs(deltaYpix);

	if( QParam.AuxCam_Scale_x[nozzle-1] <= EXTCAM_MMPIX_MIN || QParam.AuxCam_Scale_x[nozzle-1] >= EXTCAM_MMPIX_MAX ||
		QParam.AuxCam_Scale_y[nozzle-1] <= EXTCAM_MMPIX_MIN || QParam.AuxCam_Scale_y[nozzle-1] >= EXTCAM_MMPIX_MAX )
	{
		QParam.AuxCam_Scale_x[nozzle-1] = EXTCAM_MMPIX_DEF;
		QParam.AuxCam_Scale_y[nozzle-1] = EXTCAM_MMPIX_DEF;
		Mod_Par( QParam );

		W_Mess( MsgGetString(Msg_00068) );
		return 0;
	}

	Mod_Par( QParam );

	W_Mess( MsgGetString(Msg_00069) );
	return 1;
}


//--------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


int CheckUgeDim(void)
{
  if(!UgeDimOpen())
  {
    return(0);
  }

  struct CfgUgeDim data;

  int zero=0;

  //se i primi tre record sono nulli si considera il database vuoto
  for(int i=0;i<3;i++)
  {
    UgeDimRead(data,i);

    if((((data.a==0) && (data.b==0)) && ((data.c==0) && (data.d==0))) && ((data.e==0) && (data.f==0)))
    {
      zero++;
    }
  }

  if(zero==3)
  {
    bipbip();

    if(W_Deci(1, MsgGetString(Msg_01733) ))
    {
      UgeDimClose();
      UgeDimCreate();
      UgeDimOpen();
      
      for(int i=0;i<DEF_NUGEDIM;i++)
      {
        memcpy(&data,&defUgeDim[i],sizeof(data));
        UgeDimWrite(data,i);
      }

      CfgUgelli ugedat;

      //A
      Ugelli->ReadRec(ugedat,0);
      ugedat.utype=defUgeDim[0].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[0].name,24);
      Ugelli->SaveRec(ugedat,0);

      //B
      Ugelli->ReadRec(ugedat,1);
      ugedat.utype=defUgeDim[1].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[1].name,24);
      Ugelli->SaveRec(ugedat,1);

      //C
      Ugelli->ReadRec(ugedat,2);
      ugedat.utype=defUgeDim[2].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[2].name,24);
      Ugelli->SaveRec(ugedat,2);

      //D
      Ugelli->ReadRec(ugedat,3);
      ugedat.utype=defUgeDim[2].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[2].name,24);
      Ugelli->SaveRec(ugedat,3);

      //E
      Ugelli->ReadRec(ugedat,4);
      ugedat.utype=defUgeDim[3].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[3].name,24);
      Ugelli->SaveRec(ugedat,4);

      //F
      Ugelli->ReadRec(ugedat,5);
      ugedat.utype=defUgeDim[4].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[4].name,24);
      Ugelli->SaveRec(ugedat,5);

      //G
      Ugelli->ReadRec(ugedat,6);
      ugedat.utype=defUgeDim[5].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[5].name,24);
      Ugelli->SaveRec(ugedat,6);

      //H
      Ugelli->ReadRec(ugedat,7);
      ugedat.utype=defUgeDim[5].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[5].name,24);
      Ugelli->SaveRec(ugedat,7);

      //I
      Ugelli->ReadRec(ugedat,8);
      ugedat.utype=defUgeDim[3].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[3].name,24);
      Ugelli->SaveRec(ugedat,8);
      
      //J
      Ugelli->ReadRec(ugedat,9);
      ugedat.utype=defUgeDim[4].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[4].name,24);
      Ugelli->SaveRec(ugedat,9);

      //K
      Ugelli->ReadRec(ugedat,10);
      ugedat.utype=defUgeDim[5].index;
      strncpyQ(ugedat.U_tipo,defUgeDim[5].name,24);
      Ugelli->SaveRec(ugedat,10);

      UgeDimClose();

      return(1);
    }

    UgeDimClose();

    return(0);
  }

  UgeDimClose();

  return(1);
}

//-------------------------------------------------------------------------


//---------------------------------------------------------------------------

int CheckAndCreateUSBMachinesFolder(const char* name,const char* mnt)
{
	char buf[MAXNPATH];
	
	if(is_mount_ready(mnt))
	{
		//sprintf(buf,USB_MOUNT "%s/" USB_MACHINE_ID_FOLDER,mnt);
		sprintf(buf,"%s/" USB_MACHINE_ID_FOLDER,mnt);
		if(!CheckDirectory(buf))
		{
			if(mkdir(buf,DIR_CREATION_FLAG))
			{
				return(0);
			}
		}
	
		sprintf(buf,"%s/%s",buf,name);
		if(!CheckDirectory(buf))
		{
			if(mkdir(buf,DIR_CREATION_FLAG))
			{
				return(0);
			}
		}
	
		char tmp[MAXNPATH];
		strcpy(tmp,buf);
		
		strcat(buf,"/" USB_BACKUP_FOLDER);
		if(!CheckDirectory(buf))
		{
			if(mkdir(buf,DIR_CREATION_FLAG))
			{
				return(0);
			}
		}
	
		strcpy(buf,tmp);
		
		strcat(buf,"/" USB_INSTALL_FOLDER);
		if(!CheckDirectory(buf))
		{
			if(mkdir(buf,DIR_CREATION_FLAG))
			{
				return(0);
			}
		}
	}
	else
	{
		bipbip();
		W_Mess(USB_NOT_READY);
	}
	
	return(1);
}


//---------------------------------------------------------------------------

#ifdef __SNIPER

float xDeltaZSniper;
float yDeltaZSniper;

int DeltaZSniper_Calibrate(void)
{
	float z1=0;
	float z2=0;

	if(AutoAppZPosMm( MsgGetString(Msg_05017),1,xDeltaZSniper,yDeltaZSniper,z1,APPRENDZPOS_VACUO | APPRENDZPOS_NOXYZERORET))
	{
		if(AutoAppZPosMm( MsgGetString(Msg_05017),2,xDeltaZSniper,yDeltaZSniper,z2,APPRENDZPOS_VACUO))
		{
			QHeader.Z12_Zero_delta = z2-z1;
			Mod_Cfg(QHeader);
		}
	}

	PuntaZSecurityPos(1);
	PuntaZSecurityPos(2);
	PuntaZPosWait(1);
	PuntaZPosWait(2);
	return 1;
}

int DeltaZSniper_PosAuto(void)
{
	float x=xDeltaZSniper;
	float y=yDeltaZSniper;
  
	if( ManualTeaching(&x,&y,MsgGetString(Msg_01318)) )
	{
		xDeltaZSniper=x;
		yDeltaZSniper=y;
	}
	return 1;
}

#endif
