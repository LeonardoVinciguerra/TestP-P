/*
>>>> Q_MENU.CPP

Gestione del desktop, definizione dei menu e del popup, chiamate dei menu
alle funzioni di gestione.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++    Modificato da WALTER 16.11.96
++++    Modificato da WALTER 02.04.97 ** W0204
++++    >>SER >>MAP >>OFF
++++	>>UGE
++++	Mod. ottobre 97 - W09
++++    Mod. 31/07/1998 by Walter - mark: W3107

*/
#include "q_menu.h"
#include "q_menu_new.h"

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "filefn.h"
#include "tv.h"
#include "q_cost.h"
#include "msglist.h"
#include "q_gener.h"

#include "q_help.h"
#include "q_wind.h"
#include "q_prog.h"
#include "q_files.h"
#include "q_param.h"
#include "q_conf.h"
#include "q_conf_new.h"
#include "q_test2.h"
#include "q_fox.h"
#include "q_tabe.h"
#include "q_oper.h"
#include "q_init.h"
#include "q_inifile.h"
#include "q_zeri.h"
#include "q_assem.h"
#include "q_testh.h"
#include "q_oper.h"
#include "q_manage.h"
#include "q_functions.h"
#include "q_conveyor.h"

#ifdef __SNIPER
#include "sniper.h"
#include "tws_sniper.h"
#include "las_scan.h"
#include "q_snprt.h"
#endif

#include "stepper_modules.h"
#include "motorhead.h"

#include "q_ugeobj.h"
#include "q_vision.h" //VISIONE MASSIMONE 991018
#include "q_ser.h"
#include "q_dosat.h"  //wmd0
#include "q_oper.h"
#include "commclass.h"
#include "q_carint.h"
#include "q_net.h"
#include "lnxdefs.h"
#include "strutils.h"
#include "keyutils.h"

#include "gui_mainmenu.h"
#include "gui_desktop.h"

#include <mss.h>

#ifdef __LOG_ERROR
#include "q_inifile.h"
#include "q_logger.h"
extern CLogger BackupLogger;
#endif

extern GUI_DeskTop* guiDeskTop;

using namespace std;

// Dichiarazioni public per tutto il modulo
extern int fox_exclude;
extern int las_exclude;

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;

//SMOD260903
extern int *ComponentList;
extern unsigned int NComponent;
extern unsigned int PlacedNComp;

extern void ass_lasprerot(int punta,int orientamento,float degree_angle);

vector<string> menu_usb_mountpoints;

char buf1[30];
char buf2[30];

int Quit;      //flag di uscita programma
int restart;
int test_timeout;
float ftest;
extern int G_Prg(void);

//definizioni prototipi
void GestKcr(int);
void GestOff(void);
void GestUge(void);
void GestCali(void);
void GestZmac(void);
void GestScaric(void);
void GestCacen(void);
void GestDosPar(void);
void GestOdos(void);
void GestVisionPar(void);
void GestStd(int);

int Call_ConfComand(void);
int Call_ParamBackupMkMenu(void);

#ifdef __SNIPER
int Call_HTest_ImgLas1(void);
int Call_HTest_ImgLas2(void);
#endif

int Call_HTest(void);
int Call_HTest_ExtCam(void);
int Call_HTest_Las1(void);
int Call_HTest_Las2(void);

#ifndef __DISP2
int Call_CfgDosTime(void);
int Call_CfgDosOff(void);
#else
int Call_CfgDosTime1(void);
int Call_CfgDosTime2(void);
int Call_CfgDosOff1(void);
int Call_CfgDosOff2(void);
#endif

int Call_CfgZCalManual1(void);
int Call_CfgZCalManual2(void);

int Call_DoNetBkp(void);
int Call_DoNetRestore(void);
int Call_DoLocalDBCopy(void);

int Call_DoUsbBkpMkMenu(void);
int Call_DoUsbRestoreMkMenu(void);

int Call_CfgVuotoMax(void);


// Creazione delle istanze per gli oggetti dei menu e barra popup
extern struct CfgHeader QHeader;          // struct memo configuraz.
extern struct CfgParam  QParam;           // struct memo parametri vari

GUI_MainMenu* guiMainMenu;

#ifdef __DISP2
GUI_SubMenu *Q_MenuSubDosaPar;
GUI_SubMenu *Q_MenuSubDosaOff;
#endif

GUI_SubMenu *Q_MenuMachineBackupTo;
GUI_SubMenu *Q_MenuMachineBkpRestoreFrom;
GUI_SubMenu *Q_MenuMachineBackup;
GUI_SubMenu *Q_MenuBackup;
GUI_SubMenu *Q_MenuMachineBackupToUSB;
GUI_SubMenu *Q_MenuMachineRestoreFromUSB;

GUI_SubMenu *Q_SubNet;

GUI_SubMenu *Q_SubLasTest;
GUI_SubMenu *Q_SubLTest_Cent;

GUI_SubMenu *Q_SubZCal;

GUI_SubMenu *Q_SubAxCal;
GUI_SubMenu *Q_SubTethaCal;
GUI_SubMenu *Q_SubTethaCal_Prerot;

GUI_SubMenu* Q_MenuParamBkpTo;


void InitMenuInstance(void)
{
	#ifdef __DISP2
	Q_MenuSubDosaPar = new GUI_SubMenu();
	Q_MenuSubDosaOff = new GUI_SubMenu();
	#endif

	Q_SubNet = new GUI_SubMenu();

	Q_SubLasTest = new GUI_SubMenu();     // Sottomenu test scansione laser
	
	Q_SubLTest_Cent = new GUI_SubMenu();

	Q_SubZCal = new GUI_SubMenu();     // Sottomenu calibrazione asse Z

	Q_SubAxCal = new GUI_SubMenu();     // Sottomenu calibrazione assi

	Q_SubTethaCal = new GUI_SubMenu(); // Sottomenu calibrazione asse tetha
	Q_SubTethaCal_Prerot = new GUI_SubMenu();
}

// >> Costruzione degli oggetti menu <<

// Creazione menu utilita' >> Mod S010801
void MenuUti()
{
	Q_MenuMachineBackupTo=new GUI_SubMenu(MENU_MACHINE_BACKUP_TO);
	Q_MenuMachineBackupTo->Add( MsgGetString(Msg_05053),999,IsNetEnabled() ? 0 : SM_GRAYED, NULL, Call_DoNetBkp );
	Q_MenuMachineBackupTo->Add( MsgGetString(Msg_02105),999,0,NULL,Call_DoUsbBkpMkMenu );

	Q_MenuMachineBkpRestoreFrom=new GUI_SubMenu(MENU_MACHINE_RESTORE_FROM);
	Q_MenuMachineBkpRestoreFrom->Add(MsgGetString(Msg_05053),999,IsNetEnabled() ? 0 : SM_GRAYED,NULL,Call_DoNetRestore );
	Q_MenuMachineBkpRestoreFrom->Add(MsgGetString(Msg_02105),999,0,NULL,Call_DoUsbRestoreMkMenu);

	Q_MenuMachineBackup=new GUI_SubMenu(MENU_MACHINE_BACKUP);
	Q_MenuMachineBackup->Add( MsgGetString(Msg_00479), 0, 0, Q_MenuMachineBackupTo, NULL );
	Q_MenuMachineBackup->Add( MsgGetString(Msg_00480), 0, 0, Q_MenuMachineBkpRestoreFrom, NULL );
	
	Q_MenuBackup=new GUI_SubMenu(MENU_BACKUP);
	Q_MenuBackup->Add(MENU_BACKUP1,0,NULL,Call_ParamBackupMkMenu);
	Q_MenuBackup->Add(MENU_BACKUP2,0,Q_MenuMachineBackup,NULL);
}

void MenuSubNet()
{
	Q_SubNet->Add(MsgGetString(Msg_01660),999,0,NULL,Call_DoLocalDBCopy);
}

void MenuSubZCal()
{
	Q_SubZCal->Add( MsgGetString(Msg_00042),999,0,NULL,Call_CfgZCalManual1);
	Q_SubZCal->Add( MsgGetString(Msg_00043),999,0,NULL,Call_CfgZCalManual2);
}


int Call_FindPrerotZeroTheta1()
{
	Sniper_FindPrerotZeroTheta( 1 );
	return 1;
}

int Call_FindPrerotZeroTheta2()
{
	Sniper_FindPrerotZeroTheta( 2 );
	return 1;
}

void MenuSubTethaCal()
{
	Q_SubTethaCal_Prerot->Add( MsgGetString(Msg_00042), 0, 0, NULL, Call_FindPrerotZeroTheta1 );
	Q_SubTethaCal_Prerot->Add( MsgGetString(Msg_00043), 0, 0, NULL, Call_FindPrerotZeroTheta2 );

	Q_SubTethaCal->Add( MsgGetString(Msg_00420), 0, 0, NULL, fn_RotationCenterCalibration );
	Q_SubTethaCal->Add( MsgGetString(Msg_01133), 0, 0, NULL, fn_NozzleRotationAdjustment );
	Q_SubTethaCal->Add( MsgGetString(Msg_00162), 0, 0, Q_SubTethaCal_Prerot, NULL );
}

void MenuSubAxCal()
{
	Q_SubAxCal->Add(MENUAXCAL1,999,0,NULL,fn_AxesCalibration);
	Q_SubAxCal->Add(MENUAXCAL2,999,0,Q_SubZCal,NULL);
	Q_SubAxCal->Add(MENUAXCAL3,999,0,Q_SubTethaCal,NULL);
}

#ifdef __DISP2
void MenuSubDosaPar()
{
	Q_MenuSubDosaPar->Add(MsgGetString(Msg_05113),999,0,NULL,Call_CfgDosTime1);
	Q_MenuSubDosaPar->Add(MsgGetString(Msg_05114),999,0,NULL,Call_CfgDosTime2);
}

void MenuSubDosaOff()
{
	Q_MenuSubDosaOff->Add(MsgGetString(Msg_05113),999,0,NULL,Call_CfgDosOff1);
	Q_MenuSubDosaOff->Add(MsgGetString(Msg_05114),999,0,NULL,Call_CfgDosOff2);
}
#endif


//* Creazione sub-menu test laser
extern int fn_EarRingVerifyUI();

void MenuSubLasTest()
{
	Q_SubLasTest->Add( MsgGetString(Msg_01111), 0, 0, NULL, Call_HTest_Las1 );
	Q_SubLasTest->Add( MsgGetString(Msg_01112), 0, 0, NULL, Call_HTest_Las2 );

	Q_SubLasTest->Add( MsgGetString(Msg_01222), 0, 0, NULL, fn_EarRingVerifyUI );

	Q_SubLasTest->Add( MsgGetString(Msg_05001), 0, 0, NULL, Call_HTest_ImgLas1 ); // Sniper1 sensor image test
	Q_SubLasTest->Add( MsgGetString(Msg_05002), 0, 0, NULL, Call_HTest_ImgLas2 ); // Sniper2 sensor image test
}

// >> Fine creazione degli oggetti menu <<


int Call_ParamBackup(void)
{
	guiDeskTop->ShowStatusBars( false );

	int n = Q_MenuParamBkpTo->GetSelectionIndex();
	DataBackup(menu_usb_mountpoints.at(n).c_str());

	guiDeskTop->HideStatusBars();
	return 1;
}

int Call_ParamBackupMkMenu()
{
	int n = getAllUsbMountPoints(menu_usb_mountpoints);
	if(n)
	{
		Q_MenuParamBkpTo = new GUI_SubMenu(MENU_PARAM_BACKUP);
		
		for(int i = 0; i < n; i++)
		{
			Q_MenuParamBkpTo->Add(getMountPointName(menu_usb_mountpoints.at(i).c_str()),1,0,NULL,Call_ParamBackup);
		}
		
		Q_MenuParamBkpTo->Show();
		
		delete Q_MenuParamBkpTo;
		Q_MenuParamBkpTo = NULL;
		Q_MenuBackup->HideAll();
	}
	else
	{
		Q_MenuBackup->HideAll();
		bipbip();
		W_Mess(MSG_NO_REMOVABLE_STORAGE_DEVICE);
	}
	return 1;
}


//Richiama funzioni Configurazioni Comandi
int Call_ConfComand(void)
{
	if( Ask_Password( CONFIG_PSW ) )
	{
		fn_ReservedParams();

		//ricarica parametri azionamenti
		OpenRotation(ROT_RELOAD);
		// e velocita' assi
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
		return 1;
	}

	return 0;
}

//Richiama funzioni test Hardware
int Call_HTest(void)
{
	guiDeskTop->ShowStatusBars( false );
	Test_hardw();
	guiDeskTop->HideStatusBars();
	return 1;
}

int Call_HTest_ExtCam(void)
{
	guiDeskTop->ShowStatusBars( false );
	TestExtCam();
	guiDeskTop->HideStatusBars();
	return 1;
}


#ifdef __SNIPER
//GF -START-
int Call_HTest_ImgLas1(void)
{
	Sniper_ImageTest( 1 );
	return 1;
}

int Call_HTest_ImgLas2(void)
{
	Sniper_ImageTest( 2 );
	return 1;
}
//GF -END-

void Call_HTest_Las( int punta )
{
	PuntaZPosMm(punta,0);
	PuntaZPosWait(punta);

	StdScan( punta );

	PuntaZSecurityPos(punta);
	PuntaZPosWait(punta);
}
#endif


int Call_HTest_Las1()
{
	Call_HTest_Las(1);
	return 1;
}

int Call_HTest_Las2()
{
	Call_HTest_Las(2);
	return 1;
}


//richiama funzioni di uscita
int Call_Exit(int code)
{
	Quit=code;
	restart=0;
	return 1;
}


#ifndef __DISP2
int Call_CfgDosTime(void)
{
	if( QParam.Dispenser )
	{
		fn_DispenserParams( 0, 1 );
	}
	return 1;
}

int Call_CfgDosOff(void)
{
	if( QParam.Dispenser )
	{
		fn_DispenserOffset( 0, 1 );
	}
	return 1;
}
#else
int Call_CfgDosTime1(void)
{
	if( QParam.Dispenser )
	{
		fn_DispenserParams( 0, 1 );
	}
	return 1;
}

int Call_CfgDosTime2(void)
{
	if( QParam.Dispenser )
	{
		fn_DispenserParams( 0, 2 );
	}
	return 1;
}

int Call_CfgDosOff1()
{
	if( QParam.Dispenser )
	{
		fn_DispenserOffset( 0, 1 );
	}
	return 1;
}

int Call_CfgDosOff2()
{
	if( QParam.Dispenser )
	{
		fn_DispenserOffset( 0, 2 );
	}
	return 1;
}
#endif


int Call_CfgZCalManual1(void)
{
	guiDeskTop->ShowStatusBars( false );
	G_ZCalManual( 1 );
	guiDeskTop->HideStatusBars();
	return 1;
}

int Call_CfgZCalManual2(void)
{
	guiDeskTop->ShowStatusBars( false );
	G_ZCalManual( 2 );
	guiDeskTop->HideStatusBars();
	return 1;
}


int Call_DoNetBkp(void)
{
	guiDeskTop->ShowStatusBars( false );
	DoBkp(BKP_COMPLETE,bkp::Net);
	guiDeskTop->HideStatusBars();
	return 1;
}

int Call_DoNetRestore(void)
{
	guiDeskTop->ShowStatusBars( false );
	DoBkpRestore(bkp::Net);
	guiDeskTop->HideStatusBars();
	return 1;
}

int Call_DoUsbBkp(void)
{
	guiDeskTop->ShowStatusBars( false );
	int n = Q_MenuMachineBackupToUSB->GetSelectionIndex();
	DoBkp(BKP_COMPLETE,bkp::USB,menu_usb_mountpoints.at(n).c_str());
	guiDeskTop->HideStatusBars();
	return 1;
}

int Call_DoUsbBkpMkMenu(void)
{
	int n = getAllUsbMountPoints(menu_usb_mountpoints);
	if(n)
	{
		Q_MenuMachineBackupToUSB = new GUI_SubMenu(MENU_MACHINE_BKP_TO_USB);
		
		for(int i = 0; i < n; i++)
		{
			Q_MenuMachineBackupToUSB->Add(getMountPointName(menu_usb_mountpoints.at(i).c_str()),1,0,NULL,Call_DoUsbBkp);
		}
		
		Q_MenuMachineBackupToUSB->Show();
		
		delete Q_MenuMachineBackupToUSB;
		Q_MenuMachineBackupToUSB = NULL;
		Q_MenuMachineBackupTo->HideAll();
	}
	else
	{
		Q_MenuMachineBackupTo->HideAll();
		bipbip();
		W_Mess(MSG_NO_REMOVABLE_STORAGE_DEVICE);
	}
	return 1;
}

int Call_DoUsbRestore(void)
{
	guiDeskTop->ShowStatusBars( false );
	int n = Q_MenuMachineRestoreFromUSB->GetSelectionIndex();
	DoBkpRestore(bkp::USB,menu_usb_mountpoints.at(n).c_str());
	guiDeskTop->HideStatusBars();
	return 1;
}

int Call_DoUsbRestoreMkMenu(void)
{
	int n = getAllUsbMountPoints(menu_usb_mountpoints);
	if(n)
	{
		Q_MenuMachineRestoreFromUSB = new GUI_SubMenu(MENU_MACHINE_RESTORE_FROM_USB);
		
		for(int i = 0; i < n; i++)
		{
			Q_MenuMachineRestoreFromUSB->Add(getMountPointName(menu_usb_mountpoints.at(i).c_str()),1,0,NULL,Call_DoUsbRestore);
		}
		
		Q_MenuMachineRestoreFromUSB->Show();
		
		delete Q_MenuMachineRestoreFromUSB;
		Q_MenuMachineRestoreFromUSB = NULL;
		Q_MenuMachineBkpRestoreFrom->HideAll();
	}
	else
	{
		Q_MenuMachineBkpRestoreFrom->HideAll();
		bipbip();
		W_Mess(MSG_NO_REMOVABLE_STORAGE_DEVICE);
	}
	return 1;
}

int Call_DoLocalDBCopy(void)
{
	guiDeskTop->ShowStatusBars( false );
	CopyLocalDB();
	guiDeskTop->HideStatusBars();
	return 1;
}


// Gestione della programmazione in autoapprendimento

//          *** Creazione del desktop e lancio dei menu ***

void UgeTest1(void)
{
	int c;
	int n_ugello[2]={0,'A'};
	Ugelli->SetFlag(0,1);
	do
	{
		Ugelli->DoOper(n_ugello);
		n_ugello[1]++;
		if(n_ugello[1]=='C')
		n_ugello[1]='A';
		c=Handle();
	} while(c!=K_ESC);
}


// Attenzione! Inserire di seguito, nel ciclo SWITCH, le chiamate alle
// funzioni relative, a seconda del relativo codice composto (Handle+N.Barra)
int StartProc()
{
	int THandle;
	restart=0;
	Quit=0;

	Ugelli=new UgelliClass();
	Ugelli->Open();

	ComponentList=NULL; //SMOD260903
	NComponent=0;
	PlacedNComp=0;

	InitMenuInstance();

	Dosatore = new DosatClass(FoxHead);

	if(!QParam.Dispenser)
	{
		Dosatore->Disable();
	}

	// Creazione degli oggetti menu collegati al popup
	MenuUti();

	#ifdef __DISP2
	MenuSubDosaPar();
	MenuSubDosaOff();
	#endif

	MenuSubNet();
	MenuSubTethaCal();
	MenuSubZCal();
	MenuSubAxCal();
	MenuSubLasTest();

	MainMenu_Create( &guiMainMenu );

	OpenRotation();

	CheckMachineID();

	if(QHeader.modal & ENABLE_CARINT)
	{
		InitCarList();
		UpdateDBData(CARINT_UPDATE_FULL | CARINT_UPDATE_INITMNT); //esegue anche ConfImport
	}

	#ifdef __DBNET_DEBUG
	DBNet_DebugOpen();
	#endif

	Set_OnFile(0); // reset flag disabilit. hardware - W09

	comm_init();  // com port init

	HW_Init();    // Init generale hardware

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	// Init file di log backup
#ifdef __LOG_ERROR
	BackupLogger.Clear();
#endif

	FList_Read();

	if(IsNetEnabled())
	{
		CheckBkp(CHKBKP_FIRST,bkp::Net);
	}
	
	PuntaZSecurityPos(1);
	PuntaZSecurityPos(2);
	
	PuntaZPosWait(1);
	PuntaZPosWait(2);
	
	if(Get_UseCpu())
	{
		CheckWarmUp();
	}
	
	CheckUgeDim();
	
	guiMainMenu->Activate(0); //seleziona primo popup
	guiMainMenu->Show();

	char tmp1[9],tmp2[9];
	do
	{
		THandle=Handle();
		switch(THandle)
		{
			case K_CTRL_HOME:
				HeadHomeMov();
				break;

			case K_CTRL_END:
				HeadEndMov();
				break;
			
			case K_SHIFT_F2:
				if( !Get_DemoMode() )
				{
					ZeroMaccManSearch(); // Verifica zero macchina
				}
				else
				{
					bipbip();
				}
				break;
			
			case K_F1:
			case K_F2:
			case K_F3:
			case K_F4:
			case K_F5:
			case K_F6:
			case K_F7:
			case K_F8:
				strncpyQ(tmp1,QHeader.Conf_Default,8);
				strncpyQ(tmp2,QHeader.Cli_Default,8);

				if( fn_LoadProgramFromQuickList( THandle-K_F1 ) )
				{
					if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
					{
						if(UpdateDBData(CARINT_UPDATE_FULL))
						{
							ConfImport(0);
						}
					}
					
					G_Prg();
				}
				break;

			case K_F12:
				fn_QuickLoadPrograms();
				break;

			default:
				guiMainMenu->GestKey( THandle );
		}
	
		guiDeskTop->SetEditMode( false );
		
	} while( Quit == 0 );
	
	ResetZAxis(1);
	ResetZAxis(2);
	
	Ugelli->DepoAll();
	
	NozzleXYMove( 0, 0 );
	Wait_PuntaXY();

	//cpu_cmd(2,"SR1");      // reset comandi CPU rack esterno
	cpu_cmd(2,(char*)"SF0"); // disabilita assi

	FoxHead->MoveRel(STEP1,4);
	FoxHead->MoveRel(STEP2,4);
	PuntaZPosWait(1);
	PuntaZPosWait(2);
	
	delete Ugelli;
	
	if( !restart )
	{
		//UpdateDBData(CARINT_UPDATE_FULL);
		if(QHeader.modal & ENABLE_CARINT)
		{
			DisableDB();
		}

		int backup_done=0;

		if(IsNetEnabled())
		{
			backup_done=CheckBkp(CHKBKP_NORMAL,bkp::Net);
		}

		if(!backup_done)
		{
			FList_Save();
			FList_Flush();
		}
		else
		{
			FList_Flush();
			FList_Save();
		}
	
		if(ComponentList!=NULL)
		{
			delete[] ComponentList;
		}
		
		FoxHead->MotorDisAll();
	
		FoxHead->ArmWatchdog();
		delay(10);
		FoxHead->Crash();
	
		#ifdef __SNIPER
		delete Sniper1;
		delete Sniper2;

		if( ComPortSniper )
		{
			ComPortSniper->close();
			delete ComPortSniper;
		}
		#endif
		
		// se convogliatore inizializzato, resetto il driver
		if( Get_ConveyorInit() && Get_UseSteppers() )
		{
			MoveConveyorPosition( CONV_HOME );
			Conveyor->ResetDrive();
		}

		//THFEEDER
		delete ThFeeder;
		//CONVOGLIATORE
		delete Conveyor;
		ComPortStepperAux->close();
		delete ComPortStepperAux;

		#ifdef __USE_MOTORHEAD
		delete Motorhead;
		if( ComPortMotorhead )
		{
			ComPortMotorhead->close();
			delete ComPortMotorhead;
		}
		#endif
		
		delete ComPortCPU;
		delete FoxHead;
		delete FoxPort;

		MainMenu_Destroy( &guiMainMenu );

		#ifdef __DISP2
		delete Q_MenuSubDosaPar;
		delete Q_MenuSubDosaOff;
		#endif

		delete Q_MenuMachineBackupTo;
		delete Q_MenuMachineBkpRestoreFrom;
		delete Q_MenuMachineBackup;
		delete Q_MenuBackup;

		delete Q_SubLasTest;

		delete Q_SubLTest_Cent;

		delete Q_SubZCal;

		delete Q_SubAxCal;

		delete Q_SubTethaCal;
		delete Q_SubTethaCal_Prerot;

		delete Q_SubNet;

		if(Dosatore!=NULL)
		delete Dosatore;

		CloseExtCamPort();
	}
	else
	{
		OpenRotation();
	}

	#ifdef __DBNET_DEBUG
	DBNet_DebugClose();
	#endif

	return Quit;
}
