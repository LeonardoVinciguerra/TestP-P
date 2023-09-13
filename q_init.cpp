#include "q_init.h"

#include <string>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "q_inifile.h"
#include "gui_desktop.h"

#include "q_cost.h"
#include "msglist.h"
#include "commclass.h"
#include "q_graph.h"
#include "motorhead.h"
#include "q_camera.h"

#ifdef __SNIPER
#include "sniper.h"
#include "tws_sniper.h"
#include "q_snprt.h"
#endif

#include "stepper_modules.h"

#include "q_net.h"
#include "q_fox.h"
#include "q_oper.h"
#include "q_tabe.h"
#include "q_vision.h"
#include "q_help.h"
#include "q_ser.h"
#include "q_carobj.h"
#include "q_carint.h"
#include "q_dosat.h"
#include "q_gener.h"
#include "q_wind.h"
#include "q_files.h"  //DANY210103
#include "filefn.h"
#include "keyutils.h"
#include "strutils.h"
#include "fileutils.h"
#include "lnxdefs.h"
#include "comaddr.h"

//GF_COM_TEST
#include "tws_brushless.h"
#include "Timer.h"

#include "c_pan.h"
#include "q_functions.h"
#include "q_files_new.h"
#include "q_conveyor.h"

#include <mss.h>


void NozzleZZeroSearch();

// Messaggio per mancanza pressione allo start.
#define	NOPRESS		    MsgGetString(Msg_00001)
// Msg. per passaggio in modalita' file.
#define	MODFILE		    MsgGetString(Msg_00002)
//Testo finestra attesa azzeramento teste
#define  WAITNOZZLERESET MsgGetString(Msg_00715)
//msg. di errore init scheda fox
#define  ERRINIT_FOX   MsgGetString(Msg_00881)
//brushless in protezione
#define  ERRBRUSH       MsgGetString(Msg_00956)

extern struct vis_data Vision;
extern struct CfgParam  QParam;
extern struct CfgHeader QHeader;     // struct memo configuraz.
extern struct CfgTeste MapTeste;
extern struct cur_data  CurDat;

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;


SSpeedsTable SpeedsTable;
SMachineInfo MachineInfo;
SQuickStart QuickStartList;

bool zeroMachineTeach = false;

int lastZTheta[2]={0,0};

#ifdef HWTEST_RELEASE
int autozero = 0;
#endif


#ifdef __DISP2
bool singleDispPar = false;

bool Get_SingleDispenserPar()
{
	return singleDispPar;
}

void Set_SingleDispenserPar( bool s )
{
	singleDispPar = s;
}
#endif



GUI_DeskTop* guiDeskTop = 0;

void EnterGraphicMode()
{
	guiDeskTop = new GUI_DeskTop();    // Istanza del desktop
	guiDeskTop->Show();
	guiDeskTop->ShowBanner();
}

void ExitGraphicMode()
{
	if( guiDeskTop != NULL )
	{
		guiDeskTop->Quit();
		delete guiDeskTop;
		guiDeskTop = NULL;
	 }
}

void ShowInitError(const char* msg)
{
	char buf[160];
	snprintf( buf, sizeof(buf), "%s !!\n%s\n%s", msg, MSG_INIT_QUIT, MSG_INIT_KEY );
	W_MsgBox msgbox = W_MsgBox( MsgGetString(Msg_00289), buf, 0, MSGBOX_ANYKEY, 0, MSGBOX_YLOW );
	msgbox.Activate();
}

void ShowInitErrorNotCritical(const char* msg)
{
	char buf[160];
	snprintf( buf, sizeof(buf), "%s !!\n%s",msg,MSG_INIT_KEY);
	W_MsgBox msgbox = W_MsgBox( MsgGetString(Msg_00289), buf, 0, MSGBOX_ANYKEY, 0, MSGBOX_YLOW );
	msgbox.Activate();
}


//---------------------------------------------------------------------------
// Legge files di configurazione macchina
//---------------------------------------------------------------------------
int ReadMachineConfigFiles()
{
	SpeedsTable_Read( SpeedsTable );
	MachineInfo_Read( MachineInfo );
	QuickStart_Read( QuickStartList );

	//TODO: fare tutti nuovi files :)

	Read_Map( MapTeste );
	Read_Cfg( QHeader );
	Read_Par( QParam );

	Init_off();             // Carica mappatura offset teste
	load_visionpar();       //CARICA PARAMETRI VISIONE

	CurData_Read( CurDat );
	CurDat.U_p1=-1;
	CurDat.U_p2=-1;
	//CurData_Write( CurDat );

	LoadNetPar( nwpar );

	#ifdef __USE_MOTORHEAD
	//TODO - sistemare
	Read_HeadParams_File( "headX.txt", HEADX_ID );
	Read_HeadParams_File( "headY.txt", HEADY_ID );
	Read_PIDParams_File( "headPID.txt" );
	#endif

	return 1;
}

//---------------------------------------------------------------------------
// Scrive files di configurazione macchina
//---------------------------------------------------------------------------
int WriteMachineConfigFiles()
{
	MachineInfo_Write( MachineInfo );

	return 1;
}


int InitProc()
{
	int i,foxver;

	EnableTimerConfirmRequiredBeforeNextXYMovement(true);

	// finestra di init
	CWindow initWindow = CWindow( 0 );
	initWindow.SetClientAreaPos( 0, 2 );
	initWindow.SetClientAreaSize( 32, 17 );
	initWindow.SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED_X );

	initWindow.Show();

	initWindow.DrawTextCentered( 2, MSG_INIT );
	int initWinRow = 5;

	DBRemote=CarIntFile(CARINT_REMOTE);



	//GF_COM_TEST
	/*
	CommClass* ComPortMotorheadXXX = new CommClass( "/dev/ttyUSB0", B460800 );
	BrushlessModule* m_board = new BrushlessModule( ComPortMotorheadXXX, 1 );
	
	Timer testTimer;

	while( !Esc_Press() )
	{
		testTimer.start();
		
		//m_board->ResetAlarms();
		//m_board->GotoPos0_Multi( 0, 0, 0, 0, 0, 0, 0 );
		if( m_board->MotorStatus() == MOTOR_ERROR )
		{
			return 0;
		}
		
		testTimer.stop();
		
		//print_debug( "time:  %5.3f\n", float(testTimer.getElapsedTimeInMicroSec())/1000.0 );
		
		delay( 10 );
	}
	
	delete m_board;
	ComPortMotorheadXXX->close();
	delete ComPortMotorheadXXX;
	return 0;
	*/



	#ifdef __USE_MOTORHEAD
	if( Get_UseMotorhead() )
	{
		ComPortMotorhead = new CommClass( getMotorheadComPort(), MOTORHEAD_BAUD );

		Motorhead = new MotorheadClass( ComPortMotorhead );

		if( Motorhead->IsEnabled() )
		{
			initWindow.DrawTextF( 1, initWinRow++, 0, MSG_INIT_MOTORHEADX_VERS, Motorhead->Version( HEADX_ID ) );
			initWindow.DrawTextF( 1, initWinRow++, 0, MSG_INIT_MOTORHEADY_VERS, Motorhead->Version( HEADY_ID ) );
		}
		else
		{
			ShowInitError( MSG_INIT_MOTORHEAD_FAIL );
			delete Motorhead;

			ComPortMotorhead->close();
			delete ComPortMotorhead;
			return 0;
		}
	}
	else
	{
		ComPortMotorhead = NULL;
		
		Motorhead = new MotorheadClass();
	}
	#endif //__USE_MOTORHEAD
	
	
	ComPortCPU = new CommClass( CPU_COM_PORT, CPU_320_BAUD );
	if(!Get_UseCpu())
	{
		ComPortCPU->enabled=0;
	}


	if( Get_UseCpu() )
	{
		initWindow.DrawTextF( 1, initWinRow++, 0, MSG_INIT_CPU_OK, ComPortCPU->GetPort() );

		int CpuVer,CpuRev;

		if( GetCPUVersion(CpuVer,CpuRev) )
		{
			if(CpuVer!=3)
			{
				ShowInitError(MSG_INIT_CPU_FAIL);
			
				#ifdef __USE_MOTORHEAD
				delete Motorhead;
				if( ComPortMotorhead )
				{
					ComPortMotorhead->close();
					delete ComPortMotorhead;
				}
				#endif
				
				delete ComPortCPU;
				return(0);
			}
			else
			{
				if(CpuRev < 10)
				{
					initWindow.DrawTextF(1,initWinRow++,0,MSG_INIT_CPU_VERS1,CpuVer,CpuRev);
				}
				else
				{
					initWindow.DrawTextF(1,initWinRow++,0,MSG_INIT_CPU_VERS2,CpuVer,CpuRev);
				}
			}
		}
		else
		{
			ShowInitError(MSG_INIT_CPU_FAIL);

			#ifdef __USE_MOTORHEAD
			delete Motorhead;
			if( ComPortMotorhead )
			{
				ComPortMotorhead->close();
				delete ComPortMotorhead;
			}
			#endif
			
			delete ComPortCPU;
			return(0);
		}
		delay(100);
	}

	if( Get_UseFox() )
	{
		ComPortFox = new CommClass( FOX_COM_PORT, FOX_BAUD );
		FoxPort=new FoxCommClass( ComPortFox );
		FoxHead=new FoxClass(FoxPort);

		if( Get_FoxLog() )
		{
			FoxPort->StartLog();
		}

		for(i=0;i<3;i++)
		{
			if((foxver=FoxHead->GetVersion(FOX_ERROROFF)))
			{
				break;
			}
		}

		if(i==3)
		{
			ShowInitError(MSG_INIT_FOX_FAIL);

			delete FoxHead;
			delete FoxPort;

			#ifdef __USE_MOTORHEAD
			delete Motorhead;
			if( ComPortMotorhead )
			{
				ComPortMotorhead->close();
				delete ComPortMotorhead;
			}
			#endif

			delete ComPortCPU;
			return(0);
		}

		// spegne luci telecamera
		Set_HeadCameraLight( -1 );

		initWindow.DrawTextF(1,initWinRow++,0,MSG_INIT_FOX_OK,FoxHead->GetPort());
		initWindow.DrawTextF(1,initWinRow++,0,MSG_INIT_FOX_VERS,foxver & 0xFF,(foxver & 0xFF00) >> 8);

		delay(100);
	}
	else
	{
		FoxPort = new FoxCommClass();
		FoxPort->enabled = 0;

		FoxHead=new FoxClass(FoxPort);
		FoxHead->Disable(STEP1);
		FoxHead->Disable(BRUSH1);
		FoxHead->Disable(STEP2);
		FoxHead->Disable(BRUSH2);
	}

	#ifdef __SNIPER
	if( Get_CheckUart() )
	{
		bool sniperErr = false;

		// Apertura porta seriale per comunicazione con moduli Sniper
		ComPortSniper = new CommClass( SNIPER_COM_PORT, SNIPER_BAUD );

		if( !Get_UseSniper( 1 ) )
		{
			Sniper1 = new SniperModule();
		}
		else
		{
			Sniper1 = new SniperModule( ComPortSniper, SNIPER1_ADDR );

			if( Sniper1->IsEnabled() )
			{
				initWindow.DrawTextF(1,initWinRow++,0, MsgGetString(Msg_00122), Sniper1->GetVersion() );
			}
			else
			{
				W_Mess( MsgGetString(Msg_05009) );

				sniperErr = true;
			}
		}

		if( !Get_UseSniper( 2 ) )
		{
			Sniper2 = new SniperModule();
		}
		else
		{
			Sniper2 = new SniperModule( ComPortSniper, SNIPER2_ADDR );

			if( Sniper2->IsEnabled() )
			{
				initWindow.DrawTextF(1,initWinRow++,0, MsgGetString(Msg_00123), Sniper2->GetVersion() );
			}
			else
			{
				W_Mess( MsgGetString(Msg_05004) );

				sniperErr = true;
			}
		}

		delay(1000);

		if( sniperErr )
		{
			ShowInitError( MsgGetString(Msg_05003) );
			delete FoxHead;
			delete FoxPort;

			#ifdef __USE_MOTORHEAD
			delete Motorhead;
			if( ComPortMotorhead )
			{
				ComPortMotorhead->close();
				delete ComPortMotorhead;
			}
			#endif

			delete ComPortCPU;
			if( Sniper1 )
				delete Sniper1;
			if( Sniper2 )
				delete Sniper2;
			ComPortSniper->close();
			delete ComPortSniper;
			return 0;
		}

	}
	else
	{
		ComPortSniper = NULL;
		
		Sniper1 = new SniperModule();
		Sniper2 = new SniperModule();
	}
	#endif
	
	//THFEEDER e CONVOGLIATORE
	if( Get_CheckUart() )
	{
		// Apertura porta seriale per comunicazione con stepper ausiliari (caricatore TH o convogliatore)
		ComPortStepperAux = new CommClass( getStepperAuxComPort(), STEPPERAUX_BAUD );
		
		ThFeeder = new StepperModuleTHF( ComPortStepperAux, 1 );

		Conveyor = new StepperModule( ComPortStepperAux, 15 );
	}

	if( Get_UseCam() && Get_UseExtCam() )
	{
		unsigned int extcamport = InitExtCamPort();

		if( extcamport )
		{
			initWindow.DrawTextF(1,initWinRow++,0,MSG_INIT_EXTCAM_OK,GetExtCamPorName());
		}
		else
		{
			ShowInitErrorNotCritical(MSG_INIT_EXTCAM_FAIL);
			Reset_UseExtCam();
		}
	}

	#ifdef HWTEST_RELEASE
	useextcam=1;
	#endif


	initWindow.Hide();


	//DANY160103
	if( !Get_UseBrush( 0 ) )
	{
		FoxHead->Disable(BRUSH1);
		FoxHead->MotorDisable(BRUSH1);
	}
	if( !Get_UseBrush( 1 ) )
	{
		FoxHead->Disable(BRUSH2);
		FoxHead->MotorDisable(BRUSH2);
	}

	if( !Get_UseStep( 0 ) )
	{
		FoxHead->Disable(STEP1);
		FoxHead->MotorDisable(STEP1);
	}
	if( !Get_UseStep( 1 ) )
	{
		FoxHead->Disable(STEP2);
		FoxHead->MotorDisable(STEP2);
	}
	
	#ifdef __SNIPER
	if( !Get_UseSniper( 1 ) )
		Sniper1->Disable();
	if( !Get_UseSniper( 2 ) )
		Sniper2->Disable();
	#endif
	
	//Verifica presenza directories per cliente di default - DANY210103
	DirVerify(QHeader.Cli_Default);
	
	//SMOD180208
	if(Get_UseCommonFeederDir())
	{
		if(!CheckDirectory(CARDIR))
		{
			mkdir(CARDIR,DIR_CREATION_FLAG);
		}
	}
	
	if(( !Get_UseCpu() || !Get_UseFox() ) && FoxPort->enabled )
	{
		Set_OnFile(0);
	}
	
	return 1;
}


// Init generale hardware
bool HW_Init()
{
	if( Get_UseSniper( 1 ) || Get_UseSniper( 2 ) )
	{
		// CONTROLLO ATTIVAZIONE
		if( Sniper1->IsExpired() || Sniper2->IsExpired() )
		{
			W_Mess( "Periodo affitto terminato !" );
			
			Set_OnFile( 1 );
			return false;
		}
		else
		{
			if( Sniper1->GetDaysLeft() != -1 || Sniper2->GetDaysLeft() != -1 )
			{
				char str[128];
				sprintf( str, "Stato attivo.\nGiorni mancanti %d", MIN(Sniper1->GetDaysLeft(),Sniper2->GetDaysLeft()) );
				W_Mess( str );
			}
		}
	}

	Set_OnFile(QParam.US_file);

	//GF_10_06_2011 - Disattiva vuoto (potrebbe esser rimasto attivo in caso di errato spegnimento)
	//TODO: vedere perche' non funziona piu'
	/*
	Set_Vacuo( 1, OFF );
	Set_Vacuo( 2, OFF );
	*/


	CPan* wait = new CPan( -1,2,WAITTEXT,WAITELAB );

	cpu_cmd(2,(char*)"SR1");            // reset elettronica esterna
	cpu_cmd(2,(char*)"SR0");            // azzera reset flag

	make_car();                  // Init tempi caricatori in CPU

	cpu_cmd(2,(char*)"SF1");            // abilita assi

	#ifndef HWTEST_RELEASE
	Set_Finec(ON);               // abilita finecorsa
	#endif
	
	zeroMachineTeach = false;
	ResetXYVariables();  //resetta variabili di controllo assi x/y
	ResetXYStatus();
	
	delete wait;

	// controllo pressione
	while( !Check_Press() )
	{
		if(!W_Deci(1,NOPRESS))
		{
			W_Mess(MODFILE);
			Set_OnFile(1); // flag modal. file on
			break;
		}
	}

	#ifdef HWTEST_RELEASE
	autozero=0;
	
	if(W_Deci(1,"Azzeramento automatico ?"))
		autozero=1;
	#endif

	bool centering_dev_err = false;

	//se init fallito
	#ifdef __SNIPER
	if( Get_UseSniper( 1 ) )
	{
		if( !Get_OnFile() && Sniper1->IsOnError() )
		{
			W_Mess("Sniper1 Reset Error");
			centering_dev_err=true;
		}
	}

	if( Get_UseSniper( 2 ) )
	{
		if( !Get_OnFile() && Sniper2->IsOnError() )
		{
			W_Mess("Sniper2 Reset Error");
			centering_dev_err=true;
		}
	}
	#endif
	
	bool doNozzleReset = false;

	if( !centering_dev_err )
	{
		#ifdef HWTEST_RELEASE
		if((!Get_OnFile() && FoxPort->enabled) && autozero)
		#else
		if(!Get_OnFile() && FoxPort->enabled)
		#endif
		{
			#ifdef __SNIPER
			if(Sniper1->IsEnabled() && Sniper2->IsEnabled())
			{
				doNozzleReset = true;
			}
			#endif
		}
		
	
		if(doNozzleReset)
		{
			// azzeramento teste in Z
			wait = new CPan( -1, 1, WAITNOZZLERESET );
		
			//porta le punte a quota di sicurezza
			InitPuntaUP(1);
			InitPuntaUP(2);
		
			NozzleZZeroSearch();

			#ifdef __SNIPER
			Sniper_Calibrate( 1 );
			Sniper_Calibrate( 2 );
			#endif

			delete wait;
		}
	}

	// spegne luci telecamera
	Set_HeadCameraLight( -1 );

	QHeader.FoxPos_HeadSens[0]=FoxHead->ReadPos(STEP1)*P1_ZDIRMOD;
	QHeader.FoxPos_HeadSens[1]=FoxHead->ReadPos(STEP2)*P2_ZDIRMOD;
	Mod_Cfg(QHeader);

	Init_VacuoReading();

	#ifdef __USE_MOTORHEAD
	// init dei motori
	if( !Get_OnFile() && Motorhead->IsEnabled() )
	{
		wait = new CPan( -1, 1, MSG_WAIT_MOTORHEAD_RESET );

		if( !Motorhead->Init( HEADX_ID ) )
		{
			W_Mess( "Unable to init X motor driver !" );
		}
		else if( !Motorhead->Init( HEADY_ID ) )
		{
			W_Mess( "Unable to init Y motor driver !" );
		}
		else if( !SetHome_PuntaXY() )
		{
			Motorhead->Disable();
		}

		delete wait;
	}
	#endif

	// carica parametri telecamere
	CameraControls_Load( false );

	if( !Get_DemoMode() && !Get_OnFile() )
	{
		//un piccolo trucco per inizializzare una prima volta il timer in modo
		//che il primo movimento effettuato non richieda la conferma al movimento
		//tramite pressione dei due tasti shift
		IsConfirmRequiredBeforeNextXYMovement();

		bool manualSearch = true;

		// se ricerca automatica dei riferimenti abilitata
#ifdef __DISP2
	#ifndef __DOME_FEEDER
		if( QParam.AZ_tassi && Get_SingleDispenserPar() )
	#else
		if( QParam.AZ_tassi )
	#endif
#else
	if( QParam.AZ_tassi )
#endif
		{
			if( W_DeciManAuto( MsgGetString(Msg_00548), 10 ) )
			{
				// zero macchina automatico
				ResetTimerConfirmRequiredBeforeNextXYMovement();
				if( ZeroMaccAutoSearch() )
				{
					manualSearch = false;
					zeroMachineTeach = true;
				}
			}
		}

		if( manualSearch )
		{
			ResetTimerConfirmRequiredBeforeNextXYMovement();
			zeroMachineTeach = ZeroMaccManSearch();
		}
	}

	#ifndef HWTEST_RELEASE
	Set_Finec(OFF); // disabilita finecorsa
	#endif

	if(doNozzleReset)
	{
		wait = new CPan( -1, 1, WAITNOZZLERESET );
		
		//azzeramento in theta
		#ifdef __SNIPER
		char buf[80];
		if(	!Sniper_FindZeroTheta( 1 ) )
		{
			snprintf( buf, sizeof(buf), MsgGetString(Msg_00721), 1 );
			if( W_Deci( 1, buf ) )
			{
				Set_OnFile( 1 );
			}
		}
		if( !Sniper_FindZeroTheta( 2 ) )
		{
			snprintf( buf, sizeof(buf), MsgGetString(Msg_00721), 2 );
			if( W_Deci( 1, buf ) )
			{
				Set_OnFile( 1 );
			}
		}
		#endif

		lastZTheta[0]=FoxHead->ReadEncoder(BRUSH1);
		lastZTheta[1]=FoxHead->ReadEncoder(BRUSH2);

		//porta le punte a posizione di sicurezza
		PuntaZSecurityPos(1);
		PuntaZSecurityPos(2);
		PuntaZPosWait(2);
		PuntaZPosWait(1);

		delete wait;
	}

	//GF_30_05_2011
	// se immagine riferimento non presente salta la procedura di ricerca posizione telecamera esterna
	if( Get_UseCam() && Get_UseExtCam() && zeroMachineTeach )
	{
		if( ExtCamPositionSearch() != -1 )
		{
			// riporta punta a zero
			NozzleXYMove( 0.f, 0.f );
		}
	}

	// init convogliatore (se abilitato)
	if( Get_ConveyorEnabled() )
	{
		// Disabilito la modalita' prediscesa
		QParam.ZPreDownMode = 0;
		Mod_Par( QParam );

		wait = new CPan( -1, 1, MsgGetString(Msg_00916) );

		if( !InitConveyor() )
		{
			W_Mess( MsgGetString(Msg_00920) );
		}
		else
		{
			// Ricerca zero convogliatore
			if( !SearchConveyorZero() )
			{
				W_Mess( MsgGetString(Msg_00917) );
			}

			// Abilitazione limiti
			if( !ConveyorLimitsEnable( true ) )
			{
				W_Mess( MsgGetString(Msg_00917) );
			}
		}

		delete wait;
	}

	return true;
}

#define MAX_FOXRES_ERR 5 //massimo numero di errori in ricerca scheda Fox
//check presenza scheda Fox
int Check_Fox(void)
{
	int err_count=0;
	int fine=0;
	
	do
	{
		if(!FoxHead->GetVersion())  //get versione scheda Fox
			err_count++;              //se errore incrementa contatore errori
		else
			fine=1;                     //altrimenti termina check presenza Fox
		
		if(err_count>MAX_FOXRES_ERR)//se superato limite massimo di tentativi
		{
			W_Mess(ERRINIT_FOX);      //errore
			break;                    //esci da ciclo
		}
	} while(!fine);                 //ripeti ciclo se scheda non trovata
	
	//fine=1 se scheda trovata, 0 altrimenti
	return(fine);
}


/*Inizializza azionamenti motori punte (brushless+stepper)*/
//mode=ROT_RESET      reset+ricarica parametri azionamenti
//mode=ROT_RELOAD     ricarica solo parametri azionamenti
void OpenRotation(int mode)
{
	struct CfgBrush brushPar;

	if(FoxPort->enabled )  //se porta di comunicazione fox abilitata
	{
		if(mode==ROT_RESET)
		{
			if(!Check_Fox())
			{
				FoxHead->Disable();  //se scheda non trovata: disabilita classe Fox
				bipbip();
			}
			else
			{
				if(!FoxPort->enabled)
					return;
		
				delay(100);

				//seleziona tipo di encoder
				if(QHeader.Enc_step1==4096)
				{
					if(QHeader.brushlessNPoles[0] != 2)
					{
						FoxHead->SetEncoderType(BRUSH1,ENC4096_4P);
					}
					else
					{
						FoxHead->SetEncoderType(BRUSH1,ENC4096_2P);
					}
				}
				else
				{
					FoxHead->SetEncoderType(BRUSH1,ENC2048_2P);
				}
				int fox_current, fox_time;
				fox_current = Get_FoxCurrent();
				fox_time = Get_FoxTime();
				if( fox_current == 0 )
					fox_current = I1000m;
				if( fox_time == 0 )
					fox_time = T1250ms;
				FoxHead->SetMaxCurrent(BRUSH1,fox_current,fox_time); //1.0A, con t=1.25s (init)

				//seleziona tipo di encoder
				if(QHeader.Enc_step2==4096)
				{
					if(QHeader.brushlessNPoles[1] != 2)
					{
						FoxHead->SetEncoderType(BRUSH2,ENC4096_4P);
					}
					else
					{
						FoxHead->SetEncoderType(BRUSH2,ENC4096_2P);
					}
				}
				else
				{
					FoxHead->SetEncoderType(BRUSH2,ENC2048_2P);
				}
				FoxHead->SetMaxCurrent(BRUSH2,fox_current,fox_time); //1.0A, con t=1.25s (init)


				if(FoxHead->IsEnabled(STEP1))
					FoxHead->MotorEnable(STEP1); //abilita stepper

				if(FoxHead->IsEnabled(STEP2))
					FoxHead->MotorEnable(STEP2);


				if(FoxHead->IsEnabled(BRUSH1))
				{
					FoxHead->MotorDisable(BRUSH1); //disabilita brushless
		
					if(!FoxPort->enabled)
						return;
					
					delay(100);
					FoxHead->MotorEnable(BRUSH1);
		
					if(!FoxPort->enabled)
						return;
					
					delay(1500);
				}

				if(FoxHead->IsEnabled(BRUSH2))
				{
					FoxHead->MotorDisable(BRUSH2);              //disabilita brushless

					if(!FoxPort->enabled)
						return;

					delay(100);
					FoxHead->MotorEnable(BRUSH2);

					if(!FoxPort->enabled)
						return;

					delay(1500);
				}
		
				//seleziona corrente max
				FoxHead->SetMaxCurrent(BRUSH1,QHeader.brushMaxCurrent,QHeader.brushMaxCurrTime);
				FoxHead->SetMaxCurrent(BRUSH2,QHeader.brushMaxCurrent,QHeader.brushMaxCurrTime);
			}
		}


		//setta vel. max e start/stop per stepper
		SetNozzleZSpeed_Index( 1, ACC_SPEED_DEFAULT );
		SetNozzleZSpeed_Index( 2, ACC_SPEED_DEFAULT );
		//setta timer di riduzione della corrente
		FoxHead->SetLowCurrent_Timer(STEP1,5);
		FoxHead->SetLowCurrent_Timer(STEP2,5);

		BrushDataRead(brushPar,0);

		FoxHead->SetPID(BRUSH1,brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0]);
		FoxHead->SetPID(BRUSH2,brushPar.p[1],brushPar.i[1],brushPar.d[1],brushPar.clip[1]);

		//setta vel/acc. di rotazione
		SetNozzleRotSpeed_Index( 1, ACC_SPEED_DEFAULT );
		SetNozzleRotSpeed_Index( 2, ACC_SPEED_DEFAULT );

		PuntaZPosWait(2); //check punte ferme
		PuntaZPosWait(1);
	}
}

//disattivazione azionamenti testa
void CloseRotation(void)
{
	FoxHead->MotorDisAll(); //disabilita tutti i motori
}

//---------------------------------------------------------------------------------
// Azzeramento punte
//---------------------------------------------------------------------------------
#ifdef __SNIPER
void NozzleZZeroSearch()
{
	PuntaZPosStep(1,200,REL_MOVE,ZLIMITOFF);
	PuntaZPosStep(2,200,REL_MOVE,ZLIMITOFF);
	
	PuntaZPosWait(1);
	PuntaZPosWait(2);

	do
	{
		InitPuntaUP(1);            //porta le punte a quota di sicurezza
		InitPuntaUP(2);
		
		PuntaZPosMm(1,-4,REL_MOVE,ZLIMITOFF);
		PuntaZPosMm(2,-4,REL_MOVE,ZLIMITOFF);
		PuntaZPosWait(2);
		PuntaZPosWait(1);
		
		int measure_status;
		int measure_angle;
		float measure_position;
		float measure_shadow;
		
		Sniper1->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow );

		if( measure_status != CEN_ERR_EMPTY )
		{
			bipbip();
			if(!W_Deci(0,ZZEROERR_MSG))
			{
				Set_OnFile(1);
				W_Mess( MsgGetString(Msg_01317) );
				break;
			}
		}

		Sniper2->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow );

		if( measure_status != CEN_ERR_EMPTY )
		{
			bipbip();
			if(!W_Deci(0,ZZEROERR_MSG))
			{
				Set_OnFile(1);
				W_Mess( MsgGetString(Msg_01317) );
				break;
			}
		}
		else
			break;
	} while(1);

	Sniper_FindNozzleHeight( 1 ); //azzera terminale su zero laser
	Sniper_ZeroCheck( 1 );        //controlla azzeramento
	Sniper_FindNozzleHeight( 2 );
	Sniper_ZeroCheck( 2 );

	//ricerca e memorizza posizione di sicurezza per le due punte
	for(int i=0;i<2;i++)
	{
		if(!Get_OnFile())
		{
			float pos=InitPuntaUP(i+1)-DELTA_HEADSENS;
		
			CheckHeadSensPosChanged(pos,i+1);
		}
	}

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	for(int i = 0 ; i < 2 ; i++)
	{
		SetNozzleZSpeed_Index( i+1, ACC_SPEED_DEFAULT );
		SetNozzleRotSpeed_Index( i+1, ACC_SPEED_DEFAULT );
	}

	//porta le punte a posizione di sicurezza
	PuntaZSecurityPos(1);
	PuntaZSecurityPos(2);
	PuntaZPosWait(2);
	PuntaZPosWait(1);
}
#endif
