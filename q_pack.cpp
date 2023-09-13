/*
>>>> Q_PACK.CPP

Gestione dei dati packages (lista+parametri)

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1998    ++++
++++    Integrazione Simone Navari LCM 2001

*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "q_cost.h"
#include "q_carobj.h"
#include "q_wind.h"
#include "q_tabe.h"
#include "q_gener.h"
#include "msglist.h"
#include "q_files.h"
#include "q_prog.h"
#include "q_progt.h"
#include "q_help.h"
#include "q_conf.h"
#include "q_dosat.h"
#include "q_packages.h"
#include "q_ugeobj.h"
#include "q_oper.h"
#include "q_opert.h"

#include "q_packages.h"

#ifdef __SNIPER
#include "tws_sniper.h"
#include "las_scan.h"
#endif

#include "q_fox.h"
#include "bitmap.h"
#include "tv.h"
#include "q_vision.h"
#include "q_init.h"
#include "q_carint.h"
#include "q_assem.h"

#include "strutils.h"
#include "fileutils.h"
#include "lnxdefs.h"
#include "keyutils.h"

#include "c_inputbox.h"
#include "c_win_par.h"
#include "c_win_imgpar.h"
#include "c_win_imgpar_pack.h"
#include "c_pan.h"
#include "gui_desktop.h"
#include "gui_defs.h"

#include "centering_thread.h"
#include "q_feedersel.h"
#include "mathlib.h"

#include <mss.h>


//TEMP - //TODO togliere
#define PACKOFF_MIN             -5.f
#define PACKOFF_MAX             5.f
#define PACKOFF_TMIN            -36
#define PACKOFF_TMAX            36

extern GUI_DeskTop* guiDeskTop;

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;

//array elementi tabella packages
extern SPackageData currentLibPackages[MAXPACK];
extern SPackageOffsetData currentLibOffsetPackages[MAXPACK];

extern struct img_data imgData;
extern struct vis_data Vision;

extern struct CfgHeader QHeader;
extern struct CfgParam  QParam;
extern struct cur_data  CurDat;


//SMOD290506 - PackOffset
extern char auto_text1[50];
extern char auto_text2[50];
extern char auto_text3[50];


extern int CheckToolOnNozzle( int nozzle, char* toolsList );

int PackCompPrel( int nozzle, CarDat& car, SPackageData& pack, int* prePrelQuant, float* prePrelRot, float* postPrelRot );


//--------------------------------------------------------------------------
// Gestione edit tabella packages - main

int TPack_Colla( CWindow* parent, int nozzle, int index )
{
	if( QParam.Dispenser )
	{
		if(!Dosatore->OpenPackData( nozzle, QHeader.Lib_Default))
		{
			W_Mess( ERR_DISPPKGFILE );
			return 0;
		}
	}

	PackageDispensingUI win( parent, index, nozzle-1 );
	win.Show();
	win.Hide();

	Dosatore->ClosePackData( nozzle );
	return 1;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int Pack_ZOffManApp( int packIndex )
{
	struct CarDat car;
	SPackageData pack = currentLibPackages[packIndex];

	int prePrelQuant = 0;
	float prePrelRot = 0;
	float postPrelRot = 0;
	int nozzle = 1;

	// controlla se possibile prelevare ugello
	int useP1 = CheckToolOnNozzle( 1, pack.tools );
	int useP2 = CheckToolOnNozzle( 2, pack.tools );
	
	if( !useP1 && !useP2 )
	{
		char sbuf[160];
		snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_02120), pack.name );
		W_Mess( sbuf );
		return 0;
	}

	if( nozzle == 1 && !useP1 )
	{
		char sbuf[160];
		snprintf( sbuf, sizeof(sbuf), ERRCHECK_UGEPACK, 1, pack.name );
		strcat( sbuf, "\n" );
		char sbuf2[64];
		snprintf( sbuf2, sizeof(sbuf2), MsgGetString(Msg_02119), 2 );
		strcat( sbuf, sbuf2 );
		
		if( W_Deci( 1, sbuf ) )
		{
			nozzle = 2;
		}
		else
			return 0;
	}
	else if( nozzle == 2 && !useP2 )
	{
		char sbuf[160];
		snprintf( sbuf, sizeof(sbuf), ERRCHECK_UGEPACK, 2, pack.name );
		W_Mess( sbuf );
		return 0;
	}

	#ifdef __SNIPER
	PuntaRotDeg( 0, nozzle, BRUSH_ABS );
	Wait_EncStop(nozzle);
	nozzle == 1 ? Sniper1->Zero_Cmd() : Sniper2->Zero_Cmd();
	#endif

	int ret = PackCompPrel( nozzle, car, pack, &prePrelQuant, &prePrelRot, &postPrelRot );

	if( ret == 1 )
	{
		PuntaZPosWait(nozzle);

		PuntaRotDeg(angleconv(car.C_codice,LASANGLE(pack.orientation)+postPrelRot),nozzle,BRUSH_REL);
		Wait_EncStop(nozzle);
		
		float delta;
		int ret = ScanTest(nozzle,LASSCAN_BIGWIN,LASSCAN_DEFZRANGE,0,0,&delta);

		if(car.C_codice >= FIRSTTRAY)
		{
			FeederFile *file=new FeederFile(QHeader.Conf_Default);
			FeederClass *caric=new FeederClass(file);

			caric->SetCode(car.C_codice);

			//PuntaRotDeg_component_safe(-angleconv(car.C_codice,LASANGLE(pack.lasangle)),nozzle);
			ScaricaCompOnTray(caric,nozzle,prePrelQuant);

			delete caric;
			delete file;
		}
		else
		{
			ScaricaComp(nozzle);
		}

		PuntaRotDeg(0,nozzle);
		Wait_EncStop(nozzle);

		if(ret)
		{
			pack.snpZOffset += delta;
			currentLibPackages[packIndex] = pack;
			PackagesLib_Save( QHeader.Lib_Default );
		}
	
		PuntaZSecurityPos(nozzle);
		PuntaZPosWait(nozzle);
	}

	PuntaRotDeg( 0, nozzle );
	Wait_EncStop( nozzle );

	return 1;
}


int Pack_AngLaserApp( int packIndex )
{
	int m[2]={1,1};
	float x[2],y[2];

	struct CarDat car;
	SPackageData pack = currentLibPackages[packIndex];

	int prePrelQuant = 0;
	float prePrelRot = 0;
	int nozzle = 1;

	// controlla se possibile prelevare ugello
	int useP1 = CheckToolOnNozzle( 1, pack.tools );
	int useP2 = CheckToolOnNozzle( 2, pack.tools );
	
	if( !useP1 && !useP2 )
	{
		char sbuf[160];
		snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_02120), pack.name );
		W_Mess( sbuf );
		return 0;
	}

	if( nozzle == 1 && !useP1 )
	{
		char sbuf[160];
		snprintf( sbuf, sizeof(sbuf), ERRCHECK_UGEPACK, 1, pack.name );
		strcat( sbuf, "\n" );
		char sbuf2[64];
		snprintf( sbuf2, sizeof(sbuf2), MsgGetString(Msg_02119), 2 );
		strcat( sbuf, sbuf2 );
		
		if( W_Deci( 1, sbuf ) )
		{
			nozzle = 2;
		}
		else
			return 0;
	}
	else if( nozzle == 2 && !useP2 )
	{
		char sbuf[160];
		snprintf( sbuf, sizeof(sbuf), ERRCHECK_UGEPACK, 2, pack.name );
		W_Mess( sbuf );
		return 0;
	}

	#ifdef __SNIPER
	PuntaRotDeg( 0, nozzle, BRUSH_ABS );
	Wait_EncStop(nozzle);
	nozzle == 1 ? Sniper1->Zero_Cmd() : Sniper2->Zero_Cmd();
	#endif

	int ret = PackCompPrel( nozzle, car, pack, &prePrelQuant, &prePrelRot, 0 );

	if( ret == 1 )
	{
		int debug2Tmp=QHeader.debugMode2;
	
		QHeader.debugMode2=QHeader.debugMode2 & (~DEBUG2_LASERERR);
		Mod_Cfg(QHeader);
	

		float tmp_tolerance = pack.snpTolerance;
	
		//si aumenta per far si che il controllo di tolleranza sulle dimensioni
		//non fallisca all'interno di GetResult
		pack.snpTolerance = 500;

		for( int i = 0; i < 2; i++ )
		{
			pack.orientation = i*90;

			Wait_EncStop(nozzle);

			#ifdef __SNIPER
			StartCenteringThread();
			StartCentering( nozzle, 0, &pack );

			// avanza lo stato di centraggio sniper fino a quando non e' completato
			//--------------------------------------------------------------------------
			Timer timeoutTimer;
			timeoutTimer.start();

			while( !IsCenteringCompleted( nozzle ) )
			{
				delay( 5 );

				if( timeoutTimer.getElapsedTimeInMilliSec() > WAITPUP_TIMEOUT )
				{
					W_Mess( "Error - Sniper Centering Timeout !");
					return 0;
				}
			}

			CenteringResultData centeringResult;
			GetCenteringResult( nozzle, centeringResult );

			StopCenteringThread();

			if( centeringResult.Result == STATUS_OK )
			{
				x[i] = centeringResult.Shadow1 / 1000.f;
				y[i] = centeringResult.Shadow2 / 1000.f;

				if( fabs(MIN(x[i],y[i])-pack.snpX) > tmp_tolerance ||
					fabs(MAX(x[i],y[i])-pack.snpY) > tmp_tolerance )
				{
					m[i]=0;
				}
			}
			else
			{
				m[i]=0;
				x[i]=0;
				y[i]=0;
			}
			#endif

			if( i == 0 )
			{
				//ritorna a zero theta
				PuntaRotDeg(0,nozzle);
				Wait_EncStop(nozzle);
			}
		} //for end

		//ripristina valore corretto di tolerance
		pack.snpTolerance=tmp_tolerance;

		if(m[0] && !m[1])
		{
			pack.orientation=0.0;
		}

		if(!m[0] && m[1])
		{
			pack.orientation=90.0;
		}

		if(m[0] && m[1])
		{
			if(x[0]<x[1])
			{
				pack.orientation=0.0;
			}
			else
			{
				pack.orientation=90.0;
			}
		}

		if(!(m[0] || m[1]))
		{
			W_Mess(MsgGetString(Msg_01495));
		}
		else
		{
			currentLibPackages[packIndex] = pack;
			PackagesLib_Save( QHeader.Lib_Default );
		}

		SetNozzleRotSpeed_Index( nozzle, ACC_SPEED_DEFAULT );

		if(car.C_codice >= FIRSTTRAY)
		{
			FeederFile *file=new FeederFile(QHeader.Conf_Default);
			FeederClass *caric=new FeederClass(file);

			caric->SetCode(car.C_codice);

			//PuntaRotDeg_component_safe(-angleconv(car.C_codice,LASANGLE(pack.lasangle)),nozzle);
			ScaricaCompOnTray(caric,nozzle,prePrelQuant);

			delete caric;
			delete file;
		}
		else
		{
			ScaricaComp(nozzle);
		}

		// restore debug flag
		QHeader.debugMode2 = debug2Tmp;
		Mod_Cfg(QHeader);
	}

	PuntaRotDeg( 0, nozzle );
	PuntaZSecurityPos( nozzle );
	Wait_EncStop( nozzle );
	PuntaZPosWait( nozzle );

	return 1;
}

int PackCompPrel( int nozzle, CarDat& car, SPackageData& pack, int* prePrelQuant, float* prePrelRot, float* postPrelRot )
{
	if( nozzle < 1 || nozzle > 2 )
	{
		nozzle = 1;
	}

	FeederSelect* fsel = new FeederSelect( 0, pack.name );
	int carcode = fsel->Activate();
	delete fsel;

	if( carcode <= 0 )
	{
		// abort by user
		return -1;
	}

	if( postPrelRot )
	{
		*postPrelRot = 0.f;

		CInputBox inbox( 0, 6, "", MsgGetString(Msg_01567), 7, CELL_TYPE_UDEC, 2 );
		inbox.SetText( *postPrelRot );
		inbox.SetVMinMax( 0.f, 360.f );

		inbox.Show();

		if( inbox.GetExitCode() != WIN_EXITCODE_ESC )
		{
			*postPrelRot = inbox.GetFloat();
		}
	}


	FeederFile* file = new FeederFile(QHeader.Conf_Default);

	FeederClass* caric = new FeederClass(file);
	caric->SetCode(carcode);            //seleziona caricatore carcode per oggetto
	car=caric->GetData();               //estrai struttura dati caricatore da oggetto

	//prendi/cambia ugello
	int new_uge[2] = { -1, -1 };

	if( nozzle == 1 )
	{
		Ugelli->SetFlag(1,0);
		new_uge[0] = pack.tools[0];
	}
	else
	{
		Ugelli->SetFlag(0,1);
		new_uge[1] = pack.tools[0];
	}
	
	if( !Ugelli->DoOper(new_uge) )
	{
		return 0;
	}

	float pick_theta_pos = Get_PickThetaPosition( car.C_codice, pack );

	if(prePrelRot!=NULL)
	{
		*prePrelRot = pick_theta_pos;
	}


	PuntaRotDeg(pick_theta_pos,nozzle);

	struct PrelStruct preldata=PRELSTRUCT_DEFAULT;

	preldata.punta = nozzle;
	preldata.package = &pack;
	preldata.caric = caric;

	//attiva contropressione prima di prelevare il comp.
	Prepick_Contro(nozzle);

	if(car.C_codice < FIRSTTRAY)          //se non vassoi
	{
		preldata.zprel=GetZCaricPos(nozzle)-car.C_offprel;  // n. corsa prelievo
	}
	else
	{
		preldata.zprel=GetPianoZPos(nozzle)-car.C_offprel;  // n. corsa prelievo
		//attiva vuoto
		Set_Vacuo(nozzle,ON);
	}
	
	caric->GoPos(nozzle);
	
	if( pack.centeringMode == CenteringMode::SNIPER )
	{
		preldata.zup = -(pack.z/2 + pack.snpZOffset);
	}
	else
	{
		preldata.zup = GetComponentSafeUpPosition(pack);
	}
	
	Wait_EncStop(nozzle);
	Wait_PuntaXY();

	SetNozzleRotSpeed_Index( nozzle, pack.speedRot );

	Set_Finec(ON);
	PrelComp(preldata);
	Set_Finec(OFF);

	if( prePrelQuant )
	{
		*prePrelQuant = caric->GetData().C_quant;
	}

	caric->DecNComp();
	DecMagaComp(carcode);

	caric->Avanza();

	delete caric;
	delete file;

	return 1;
}


void Pack_DimApp( int nozzle, int packIndex )
{
	struct CarDat car;
	SPackageData pack = currentLibPackages[packIndex];
	CenteringResultData centeringResult;
	centeringResult.Result = 0; // un qualsiasi valore diverso da STATUS_OK

	int prePrelQuant=0;
	float prePrelRot=0;
	
	if((nozzle<1) || (nozzle>2))
	{
		nozzle = 1;
	}
	
	// controlla se possibile prelevare ugello
	int useP1 = CheckToolOnNozzle( 1, pack.tools );
	int useP2 = CheckToolOnNozzle( 2, pack.tools );
	
	if( !useP1 && !useP2 )
	{
		char sbuf[160];
		snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_02120), pack.name );
		W_Mess( sbuf );
		return;
	}

	if( nozzle == 1 && !useP1 )
	{
		char sbuf[160];
		snprintf( sbuf, sizeof(sbuf), ERRCHECK_UGEPACK, 1, pack.name );
		strcat( sbuf, "\n" );
		char sbuf2[64];
		snprintf( sbuf2, sizeof(sbuf2), MsgGetString(Msg_02119), 2 );
		strcat( sbuf, sbuf2 );
		
		if( W_Deci( 1, sbuf ) )
		{
			nozzle = 2;
		}
		else
			return;
	}
	else if( nozzle == 2 && !useP2 )
	{
		char sbuf[160];
		snprintf( sbuf, sizeof(sbuf), ERRCHECK_UGEPACK, 2, pack.name );
		W_Mess( sbuf );
		return;
	}
	
	#ifdef __SNIPER
	PuntaRotDeg( 0, nozzle, BRUSH_ABS );
	Wait_EncStop(nozzle);
	nozzle == 1 ? Sniper1->Zero_Cmd() : Sniper2->Zero_Cmd();
	#endif

	int ret = PackCompPrel( nozzle, car, pack, &prePrelQuant, &prePrelRot, 0 );

	if( ret == 1 )
	{
		SPackageData pdta;
		
		// In questa fase si abilita sempre il centraggio con 4 lati
		pdta = pack;
		pdta.snpTolerance = 500;
		pdta.snpX = 50;
		pdta.snpY = 50;

		#ifdef __SNIPER
		StartCenteringThread();
		StartCentering( nozzle, 0, &pdta );

		// avanza lo stato di centraggio sniper fino a quando non e' completato
		//--------------------------------------------------------------------------
		Timer timeoutTimer;
		timeoutTimer.start();

		while( !IsCenteringCompleted( nozzle ) )
		{
			delay( 5 );

			if( timeoutTimer.getElapsedTimeInMilliSec() > WAITPUP_TIMEOUT )
			{
				W_Mess( "Error - Sniper Centering Timeout !");
				return;
			}
		}

		GetCenteringResult( nozzle, centeringResult );

		StopCenteringThread();
		#endif
	
		bool ok = false;

		#ifdef __SNIPER
		if( centeringResult.Result == STATUS_OK )
		{
			pack.snpX = centeringResult.Shadow1 / 1000.f;
			pack.snpY = centeringResult.Shadow2 / 1000.f;

			ok = true;
		}
		#endif

		if(ok)
		{
			currentLibPackages[packIndex] = pack;
			PackagesLib_Save( QHeader.Lib_Default );
		}

		SetNozzleRotSpeed_Index( nozzle, ACC_SPEED_DEFAULT );
	
		PuntaRotDeg(0,nozzle,BRUSH_ABS);
	
		if(car.C_codice>=FIRSTTRAY)
		{
			FeederFile *file=new FeederFile(QHeader.Conf_Default);
			FeederClass *caric=new FeederClass(file);
		
			caric->SetCode(car.C_codice);
		
			//PuntaRotDeg_component_safe(-angleconv(car.C_codice,LASANGLE(pack.lasangle)),nozzle);
			ScaricaCompOnTray(caric,nozzle,prePrelQuant);
		
			delete caric;
			delete file;
		}
		else
		{
			ScaricaComp(nozzle);
		}

		PuntaRotDeg(0,nozzle);
	
		//SMOD250903
		while(!Check_PuntaRot(nozzle))
		{
			FoxPort->PauseLog();
		}
		FoxPort->RestartLog();
	}

	PuntaRotDeg( 0, nozzle );
	Wait_EncStop( nozzle );

	if( ret != -1 )
		W_Mess( centeringResult.Result == STATUS_OK ? MsgGetString(Msg_00141) : MsgGetString(Msg_05157) );
}

int PackImageCaptureSave( unsigned short packCode, char* libname, int imageType )
{
	char imageName[MAXNPATH];
	char filename[MAXNPATH];
	
	int error=1;
	int sizex, sizey;

	//TODO: rivedere titoli
	char title[80];
	switch( imageType )
	{
	case PACKAGEVISION_LEFT_1:
	case PACKAGEVISION_LEFT_2:
		strcpy( title, PACKLLEFT_TITLE );
		break;

	case PACKAGEVISION_RIGHT_1:
	case PACKAGEVISION_RIGHT_2:
		strcpy( title, PACKLRIGHT_TITLE );
		break;
	}

	GetImageName( imageName, imageType, packCode, libname );
	
	delay(Vision.image_time);
	
	int cam = pauseLiveVideo();
	
	// cattura l'immagine dalla VGA
	bitmap captured( PKG_BRD_IMG_MAXX, PKG_BRD_IMG_MAXY, getFrameWidth()/2 ,getFrameHeight()/2 );
	
	strcpy( filename, imageName );
	AppendImageMode( filename, imageType, IMAGE );
	
	error = !captured.save(filename); // salva immagine catturata
	

	strcpy( filename, imageName );
	AppendImageMode( filename, imageType, DATA );

	img_data imgData;
	ImgDataLoad( filename, &imgData );
	
	//elabora immagine creando il pattern
	// carica dati immagine o crea default.
	strcpy( filename, imageName );
	ImagePack_Elabora( filename, imageType );
	
	sizex = PACK_PATTERN_MAXX;
	sizey = PACK_PATTERN_MAXY;
	
	// seleziona la dimensione dell'immagine del pattern per la ricerca
	if(!ChoosePattern( GetVideoCenterX(), GetVideoCenterY(), sizex, sizey,2,PATTERN_MINX,PATTERN_MINY,PACK_PATTERN_MAXX,PACK_PATTERN_MAXY ))
	{
		return(1);
	}
	
	// salva la nuova diemensione del pattern
	imgData.pattern_x=sizex;
	imgData.pattern_y=sizey;

	// salva luminosita' e contrasto correnti
	imgData.bright = GetImageBright() / 655;
	imgData.contrast = GetImageContrast() / 655;

	imgData.atlante_x=PKG_BRD_IMG_MAXX;
	imgData.atlante_y=PKG_BRD_IMG_MAXY;
	
	imgData.match_iter=1;
	
	// salva dati immagine
	strcpy( filename, imageName );
	AppendImageMode( filename, imageType, DATA );
	
	ImgDataSave( filename, &imgData );
	
	strcpy( filename, imageName );
	ImagePack_Elabora( filename, imageType );
	
	playLiveVideo( cam );
	
	return error;
}

void PackVisFile_Remove( int pack_code, char* libname )
{
	char path[13][MAXNPATH];

	//dati package
	PackVisData_GetFilename( path[0], pack_code, libname );

	//immagine originale
	SetImageName( path[1], PACKAGEVISION_LEFT_1, IMAGE, pack_code, libname );
	SetImageName( path[2], PACKAGEVISION_RIGHT_1, IMAGE, pack_code, libname );

	//dati immagine
	SetImageName( path[3], PACKAGEVISION_LEFT_1, DATA, pack_code, libname );
	SetImageName( path[4], PACKAGEVISION_RIGHT_1, DATA, pack_code, libname );

	//immagine elaborata
	SetImageName( path[5], PACKAGEVISION_LEFT_1, ELAB, pack_code, libname );
	SetImageName( path[6], PACKAGEVISION_RIGHT_1, ELAB, pack_code, libname );

	//immagine originale
	SetImageName( path[7], PACKAGEVISION_LEFT_2, IMAGE, pack_code, libname );
	SetImageName( path[8], PACKAGEVISION_RIGHT_2, IMAGE, pack_code, libname );

	//dati immagine
	SetImageName( path[9], PACKAGEVISION_LEFT_2, DATA, pack_code, libname );
	SetImageName( path[10], PACKAGEVISION_RIGHT_2, DATA, pack_code, libname );

	//immagine elaborata
	SetImageName( path[11], PACKAGEVISION_LEFT_2, ELAB, pack_code, libname );
	SetImageName( path[12], PACKAGEVISION_RIGHT_2, ELAB, pack_code, libname );

	for( int i = 0; i < 13; i++ )
	{
		if( !access(path[i],F_OK) )
		{
			remove( path[i] );
		}
	}
}

void G_PackImageApprend( int nozzle, int packIndex )
{
	#ifdef __DISP2
		#ifndef __DISP2_CAM
		if( !Get_SingleDispenserPar() )
		{
			return;
		}
		#endif
	#endif

	SPackageData pack = currentLibPackages[packIndex];
	struct CarDat car;
	int appmode;
	PointF center;

	int prePrelQuant = 0;
	float prePrelRot = 0;

	if( PackCompPrel( nozzle, car, pack, &prePrelQuant, &prePrelRot, 0 ) != 1 )
	{
		return;
	}

	char NameFile[MAXNPATH+1];
	PackVisData_GetFilename( NameFile, pack.code, QHeader.Lib_Default );

	// Controlla l'esistenza del file dati immagine package
	bool exists = access( NameFile, F_OK ) == 0 ? true : false;

	PackVisData_Open( NameFile, pack.name );

	struct PackVisData packvdat;
	PackVisData_Read( packvdat );

	if( packvdat.zoff == 0 )
	{
		packvdat.zoff = -pack.z;
	}

	snprintf( packvdat.name, sizeof(packvdat.name), "%s", pack.name );
	PackVisData_Write( packvdat );

	float px[4], py[4];
	float xpos, ypos;

	SetExtCam_Light( packvdat.light[nozzle-1] );
	SetExtCam_Shutter( packvdat.shutter[nozzle-1] );
	SetExtCam_Gain(DEFAULT_EXT_CAM_GAIN);

	PuntaRotDeg_component_safe( nozzle, -90 );
	Wait_EncStop( nozzle );

	// controlla dimensione componente in base ad angolo di centraggio
	float pack_x = pack.x;
	float pack_y = pack.y;

	// se angolo di orientamento e' 90 o 270 invertiamo dimensioni
	if( pack.orientation == 90.f || pack.orientation == 270.f )
	{
		// swap values
		float temp = pack_x;
		pack_x = pack_y;
		pack_y = temp;
	}

	// se angolo centraggio e' 90 o 270 invertiamo dimensioni
	if( pack.extAngle == 90.f || pack.extAngle == 270.f )
	{
		// swap values
		float temp = pack_x;
		pack_x = pack_y;
		pack_y = temp;
	}

	//------------------------------------------------------------------------
	int prevBright = GetImageBright();
	int prevContrast = GetImageContrast();

	// setta i parametri della telecamera
	SetImageName( NameFile, (nozzle == 1) ? PACKAGEVISION_RIGHT_1 : PACKAGEVISION_RIGHT_2, DATA, pack.code, QHeader.Lib_Default );
	if( access(NameFile,0) == 0 )
	{
		struct img_data imgData;
		ImgDataLoad( NameFile, &imgData );

		// set brightess and contrast
		SetImgBrightCont( imgData.bright*655, imgData.contrast*655 );
	}
	else
	{
		SetImgBrightCont( CurDat.HeadBright, CurDat.HeadContrast );
	}

	Set_Tv( 2, CAMERA_EXT );  //attiva visualizzazione senza interruzioni

	bool first = true;
	int loop = 0;
	while( 1 )
	{
		for(int i=0;i<4;i++)
		{
			//TODO: si potrebbe fare piu' elaborato in modo da considerare spostamenti ecc...
			if( loop == 0 )
			{
				switch(i)
				{
					case 0:
						px[0] = +pack_x/2;
						py[0] = -pack_y/2;
						break;
					case 1:
						px[1] = px[0] - pack_x;
						py[1] = py[0];
						break;
					case 2:
						px[2] = px[1];
						py[2] = py[1] + pack_y;
						break;
					case 3:
						px[3] = px[0];
						py[3] = py[2];
						break;
				}
			}

			xpos = QParam.AuxCam_X[nozzle-1] - px[i];
			ypos = QParam.AuxCam_Y[nozzle-1] - py[i];

			SetNozzleXYSpeed_Index( pack.speedXY );
			SetNozzleZSpeed_Index( nozzle, pack.speedPick );

			int mode = 0;
			appmode = AUTOAPP_NOEXITRESET | AUTOAPP_NOZMOVE_LONGXY | AUTOAPP_CONTROLCAM3 | AUTOAPP_CALLBACK_PACK1 | AUTOAPP_NOSTART_ZSECURITY;
			appmode |= (nozzle == 1) ? AUTOAPP_PUNTA1ON : AUTOAPP_PUNTA2ON;
			char titolo[80];

			if( first )
			{
				MoveComponentUpToSafePosition(1);
				MoveComponentUpToSafePosition(2);
				PuntaZPosWait(2);
				PuntaZPosWait(1);

				snprintf( titolo, 80, "%s", MsgGetString(Msg_01573) );
			}
			else
			{
				mode |= AUTOAPP_NOZSECURITY;
				appmode |= AUTOAPP_NOZSECURITY;

				if( i == 0 )
				{
					snprintf( titolo, 80, "%s", MsgGetString(Msg_01573) );
				}
				else if( i == 1 )
				{
					snprintf( titolo, 80, "%s", MsgGetString(Msg_01572) );
				}
				else if( i == 2 )
				{
					snprintf( titolo, 80, "%s", MsgGetString(Msg_01581) );
				}
				else
				{
					snprintf( titolo, 80, "%s", MsgGetString(Msg_01582) );
				}
			}

			NozzleXYMove( xpos, ypos, mode );
			Wait_PuntaXY();

			// porta componente in quota di centraggio
			if( first )
			{
				PuntaZPosMm( nozzle, packvdat.zoff+QParam.AuxCam_Z[nozzle-1] );
				PuntaZPosWait( nozzle );

				first = false;
			}

			if( !ManualTeaching( &xpos, &ypos, titolo, appmode, CAMERA_EXT, nozzle ) )
			{
				if(!exists)
				{
					PackVisData_Remove( pack.code, QHeader.Lib_Default );
				}
				goto end;
			}

			// posizione delgi spigoli relativa al centro della telecamera
			px[i] = QParam.AuxCam_X[nozzle-1] - xpos;
			py[i] = QParam.AuxCam_Y[nozzle-1] - ypos;
		}

		packvdat.zoff = GetXYApp_LastZPos( nozzle ) - QParam.AuxCam_Z[nozzle-1];

		float a1 = -atan2(py[0]-py[1],px[0]-px[1])*180/PI;
		float a2 = -atan2(py[3]-py[2],px[3]-px[2])*180/PI;

		float angle = (a1+a2)/2;

		PuntaRotDeg( angle, nozzle, BRUSH_REL );
		Wait_EncStop( nozzle );

		// dopo il secondo ciclo chiede se continuare
		if( loop )
		{
			if( !W_Deci(1,MsgGetString(Msg_01614)) )
			{
				break;
			}
		}

		loop++;
	}

	//------------------------------------------------------------------------

	// calcolo posizione teorica della punta (considerando un prelievo esatto al centro del componente)
	// riferita al centro della telecamera
	center.X = (px[0]+px[1])/2;
	center.Y = (py[3]+py[0])/2;

	//------------------------------------------------------------------------

	// posizione primo punto (basso-dx)
	xpos = QParam.AuxCam_X[nozzle-1] - px[0];
	ypos = QParam.AuxCam_Y[nozzle-1] - py[0];

	NozzleXYMove( xpos, ypos, AUTOAPP_NOZSECURITY );
	Wait_PuntaXY();

	PuntaZPosMm( nozzle, packvdat.zoff+QParam.AuxCam_Z[nozzle-1] );
	PuntaZPosWait( nozzle );

	appmode = AUTOAPP_NOEXITRESET | AUTOAPP_NOZMOVE_LONGXY | AUTOAPP_PACKBOX | AUTOAPP_CONTROLCAM3 | AUTOAPP_NOSTART_ZSECURITY | AUTOAPP_CALLBACK_PACK1;
	appmode |= (nozzle == 1) ? AUTOAPP_PUNTA1ON : AUTOAPP_PUNTA2ON;

	if( !ManualTeaching( &xpos, &ypos, MsgGetString(Msg_01573), appmode, CAMERA_EXT, nozzle ) )
	{
		if(!exists)
		{
			PackVisFile_Remove( pack.code, QHeader.Lib_Default );
		}
		goto end;
	}

	if( PackImageCaptureSave( pack.code, QHeader.Lib_Default, (nozzle == 1) ? PACKAGEVISION_RIGHT_1 : PACKAGEVISION_RIGHT_2 ) )
	{
		if(!exists)
		{
			PackVisFile_Remove( pack.code, QHeader.Lib_Default );
		}
		goto end;
	}

	//DB270117
	//packvdat.xp[nozzle-1][0] = QParam.AuxCam_X[nozzle-1] - xpos - center.X;
	//packvdat.yp[nozzle-1][0] = QParam.AuxCam_Y[nozzle-1] - ypos - center.Y;
	packvdat.xp[nozzle-1][0] = QParam.AuxCam_X[nozzle-1] - xpos;
	packvdat.yp[nozzle-1][0] = QParam.AuxCam_Y[nozzle-1] - ypos;

	//------------------------------------------------------------------------

	// posizione secondo punto (basso-sx)
	xpos = QParam.AuxCam_X[nozzle-1] - px[1];

	NozzleXYMove( xpos, ypos, AUTOAPP_NOZSECURITY );
	Wait_PuntaXY();
	
	PuntaZPosMm( nozzle, packvdat.zoff+QParam.AuxCam_Z[nozzle-1] );
	PuntaZPosWait( nozzle );

	appmode = AUTOAPP_NOEXITRESET | AUTOAPP_NOZMOVE_LONGXY | AUTOAPP_PACKBOX | AUTOAPP_CONTROLCAM3 | AUTOAPP_NOSTART_ZSECURITY | AUTOAPP_CALLBACK_PACK2;
	appmode |= (nozzle == 1) ? AUTOAPP_PUNTA1ON : AUTOAPP_PUNTA2ON;

	if( !ManualTeaching( &xpos, &ypos, MsgGetString(Msg_01572), appmode, CAMERA_EXT, nozzle ) )
	{
		if(!exists)
		{
			PackVisFile_Remove( pack.code, QHeader.Lib_Default );
		}
		goto end;
	}

	if(PackImageCaptureSave( pack.code, QHeader.Lib_Default, (nozzle == 1) ? PACKAGEVISION_LEFT_1 : PACKAGEVISION_LEFT_2 ))
	{
		if(!exists)
		{
			PackVisFile_Remove( pack.code, QHeader.Lib_Default );
		}
		goto end;
	}

	//DB270117
	//packvdat.xp[nozzle-1][1] = QParam.AuxCam_X[nozzle-1] - xpos - center.X;
	//packvdat.yp[nozzle-1][1] = QParam.AuxCam_Y[nozzle-1] - ypos - center.Y;
	packvdat.xp[nozzle-1][1] = QParam.AuxCam_X[nozzle-1] - xpos;
	packvdat.yp[nozzle-1][1] = QParam.AuxCam_Y[nozzle-1] - ypos;

	packvdat.centerX[nozzle-1] = center.X;
	packvdat.centerY[nozzle-1] = center.Y;

	//------------------------------------------------------------------------

	packvdat.light[nozzle-1] = GetExtCam_Light();
	packvdat.shutter[nozzle-1] = GetExtCam_Shutter();

	PackVisData_Write(packvdat);

end:

	struct CfgBrush brushPar; //DANY151102
	BrushDataRead(brushPar,0);
	FoxHead->SetPID( (nozzle==1) ? BRUSH1 : BRUSH2, brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0] );

	Set_Tv( 3 ); //disattiva visualizzazione senza interruzioni

	// ripristina vecchi valori
	SetImgBrightCont( prevBright, prevContrast );

	SetExtCam_Light(0);
	SetExtCam_Shutter(DEFAULT_EXT_CAM_SHUTTER);
	SetExtCam_Gain(DEFAULT_EXT_CAM_GAIN );

	PackVisData_Close();

	MoveComponentUpToSafePosition(1);
	MoveComponentUpToSafePosition(2);
	PuntaZPosWait(2);
	PuntaZPosWait(1);

	if(car.C_codice>=FIRSTTRAY)
	{
		FeederFile* file = new FeederFile(QHeader.Conf_Default);
		FeederClass* caric = new FeederClass(file);

		caric->SetCode( car.C_codice );
		ScaricaCompOnTray( caric, nozzle, prePrelQuant );

		delete caric;
		delete file;    
	}
	else
	{
		ScaricaComp( nozzle );
	}

	PuntaRotDeg( 0, nozzle );
	while( !Check_PuntaRot(nozzle) );
}

//-----------------------------------------------------------------------------------

int G_MapPackOffsetXY( int packIndex )
{
	int nc = Get_LastRecMount();
	if( nc == -1 )
	{
		W_Mess(NOCALIBPRG);
		return 0;
	}

	FeederFile* CarFile=new FeederFile(QHeader.Conf_Default);
	
	if(!CarFile->opened)
	{
		delete CarFile;
		CarFile=NULL;
		return 0;
	}

	TPrgFile* TPrg=new TPrgFile(QHeader.Prg_Default,PRG_ASSEMBLY);
	
	if(!TPrg->Open(SKIPHEADER))
	{
		delete TPrg;
		delete CarFile;
		return 0;
	}

	ZerFile *zer=new ZerFile(QHeader.Prg_Default);
	
	if(!zer->Open())                   // open del file zeri
	{
		bipbip();
		W_Mess(NOZSCHFILE);
		delete zer;
		delete TPrg;
		delete CarFile;
		return 0;
	}

	struct Zeri ZerData[2];
	zer->Read(ZerData[0],0);
	
	SPackageOffsetData PackOff = currentLibOffsetPackages[packIndex];

	// setta i parametri della telecamera
	SetImgBrightCont( CurDat.HeadBright, CurDat.HeadContrast );

	Set_Tv( 2 ); //attiva visualizzazione senza interruzioni
	
	Set_Finec(ON);
	
	int abort=0;
	
	int done=0;
	
	struct TabPrg TabRec,tmpTabRec;

	for(int i=nc;i>=0;i--)
	{
		if(done==0x0f)
		{
			break;
		}
	
		if(!TPrg->Read(TabRec,i))
		{
			break;
		}
	
		// skip se flag non montare
		if(!(TabRec.status & MOUNT_MASK))
		{
			continue;
		}

		zer->Read(ZerData[1],TabRec.scheda);     // read board associata al comp.
	
		if(!ZerData[1].Z_ass)                    // skip se board non da montare
		{
			continue;
		}
		
	
		DelSpcR(TabRec.pack_txt);

		int do_calib=-1;

		if(!strncmp(TabRec.pack_txt,currentLibPackages[packIndex].name,20))
		{
			for(int j=0;j<4;j++)
			{
				if(done & (1 << j))
				{
					continue;
				}
		
				float astart=-45+(j*90);
				float aend=45+(j*90);
				
				if(TabRec.Rotaz<0)
				{
					TabRec.Rotaz+=360;
				}
	
				if(astart<aend)
				{
					if((TabRec.Rotaz>=astart) && (TabRec.Rotaz<=aend))
					{
						do_calib=j;
						break;
					}
				}
				else
				{
				}
			}

			float xoffs=0;
			float yoffs=0;
		
			float px,py;
			
			if(do_calib!=-1)
			{
				SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
		
				float dummy_angle;
				float ox,oy;
		
				TabRec.Punta = '1'; //assegnazione di comodo per non dover allocare
								    //un vettore di 3 anziche di 2 per ZerData
				
				//fornisce le coordinate per il posizionamento della punta
				Get_PosData( ZerData, TabRec, &currentLibPackages[packIndex], 0, 0, ox, oy, dummy_angle, 0, 0 );

				//si esegue la correzione per ottenere le coordinate per il posizionamento
				//della telecamera sul punto
				ox-=QParam.CamPunta1Offset_X;
				oy-=QParam.CamPunta1Offset_Y;
				
				int exit_loop=0;
		
				int auto_ic=0;
		
				do
				{
					px=ox+xoffs;
					py=oy+yoffs;

					snprintf( auto_text1, sizeof(auto_text1), "   %s %3.2f",MsgGetString(Msg_00565),TabRec.Rotaz);

					char buf[20];
					strncpyQ(buf,currentLibPackages[packIndex].name,18);
					snprintf( auto_text2, sizeof(auto_text2),"   %s %s",MsgGetString(Msg_01962),buf);

					snprintf( auto_text3, sizeof(auto_text3),"   %s %3.2f / %3.2f",MsgGetString(Msg_01963),PackOff.offX[do_calib]-xoffs,PackOff.offY[do_calib]-yoffs);
			
					NozzleXYMove( px, py );
			
					int ret_val = ManualTeaching(&px,&py, MsgGetString(Msg_00872), AUTOAPP_COMP | AUTOAPP_ONLY1KEY | AUTOAPP_NOUPDATE); //one shot

					switch(ret_val)
					{
						case K_RIGHT:
							if(xoffs+QHeader.PassoX<PACKOFF_MAX)
							{
								xoffs+=QHeader.PassoX;
							}
							break;
						case K_LEFT:
							if(xoffs-QHeader.PassoX>PACKOFF_MIN)
							{
								xoffs-=QHeader.PassoX;
							}
							break;
			
						case K_UP:
							if(yoffs+QHeader.PassoY<PACKOFF_MAX)
							{
								yoffs+=QHeader.PassoY;
							}
							break;
						case K_DOWN:
							if(yoffs-QHeader.PassoY>PACKOFF_MIN)
							{
								yoffs-=QHeader.PassoY;
							}
							break;

						case K_CTRL_RIGHT:
							if(xoffs+1<PACKOFF_MAX)
							{
								xoffs+=1;
							}
							break;
						case K_CTRL_LEFT:
							if(xoffs-1>PACKOFF_MIN)
							{
								xoffs-=1;
							}
							break;
			
						case K_CTRL_UP:
							if(yoffs+1<PACKOFF_MAX)
							{
								yoffs+=1;
							}
							break;
						case K_CTRL_DOWN:
							if(yoffs-1>PACKOFF_MIN)
							{
								yoffs-=1;
							}
							break;

						case K_SHIFT_F4:
							if(!auto_ic)
							{
								if(Prg_Autocomp(AUTO_IC_MAPOFFSET,i,TPrg,&tmpTabRec))  // autoapp. compon. / C.I.
								{
									xoffs=tmpTabRec.XMon-TabRec.XMon;
									yoffs=tmpTabRec.YMon-TabRec.YMon;
				
									auto_ic=1;
								}
							}
							else
							{
								bipbip();
							}
							break;

						case 1:
							PackOff.offX[do_calib]-=xoffs;
							PackOff.offY[do_calib]-=yoffs;
							exit_loop=1;
							abort=0;
							break;
						case 0:
							exit_loop=1;
							abort=1;
							break;
					}

				} while(!exit_loop);

				done|=1 << do_calib;
			}
		}
	}

	Set_Tv( 3 ); //disattiva visualizzazione senza interruzioni
	
	if(!done)
	{
		char buf[80];
		snprintf( buf, sizeof(buf),MsgGetString(Msg_01964),currentLibPackages[packIndex].name);
		W_Mess(buf);
	}
	else
	{
		if(!abort)
		{
			currentLibOffsetPackages[packIndex] = PackOff;
		}
	}

	Set_Finec(OFF);
	
	delete zer;
	delete TPrg;
	delete CarFile;

	return 1;
}
