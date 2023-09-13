//---------------------------------------------------------------------------
//
// Name:        q_menu_new.cpp
// Author:      Gabriel Ferri
// Created:     07/09/2011
// Description:
//
//---------------------------------------------------------------------------
#include "q_menu_new.h"

#include "gui_defs.h"
#include "gui_submenu.h"
#include "msglist.h"
#include "q_tabe.h"
#include "q_net.h"
#include "q_cost.h"
#include "q_inifile.h"
#include "q_test2.h"
#include "q_help.h"
#include "q_testhead.h"
#include "q_conf_new.h"
#include "q_manage.h"
#include "q_conveyor.h"
#include "q_menu.h"

#include <mss.h>


#define MyDELETE(p)       if( (p) != NULL ) { delete p; p = NULL; }


extern CfgHeader QHeader;          // struct memo configuraz.



	//-------------------------------------
	// SM Level 1: Production
	//-------------------------------------

extern int G_Prg();



	//-------------------------------------
	// SM Level 1: Management
	//-------------------------------------

GUI_SubMenu* sm_Management;

void sm_Management_Create()
{
	sm_Management = new GUI_SubMenu();
	sm_Management->Add( MsgGetString(Msg_00228), 0, 0, NULL, fn_CustomersManager );    // customers
	sm_Management->Add( MsgGetString(Msg_00229), 0, 0, NULL, fn_ProgramsManager );     // programs
	sm_Management->Add( MsgGetString(Msg_00230), 0, 0, NULL, fn_FeederConfigManager ); // feeders configuration
	sm_Management->Add( MsgGetString(Msg_00231), 0, 0, NULL, fn_PackagesLibManager );  // packages library
	sm_Management->Add( MsgGetString(Msg_00661), 0, 0, NULL, fn_QuickLoadPrograms );   // quick load
}

void sm_Management_Destroy()
{
	MyDELETE( sm_Management );
}



	//-------------------------------------
	// SM Level 1: Utility
	//-------------------------------------

GUI_SubMenu* sm_Utility;

		//-------------------------------------
		// SM Level 2: Hardware Test
		//-------------------------------------

extern int Call_HTest();
extern int Call_HTest_ExtCam();
extern int Call_HTest_ImgLas1();
extern int Call_HTest_ImgLas2();
extern GUI_SubMenu* Q_SubLasTest;

GUI_SubMenu* sm2_HardwareTest;

void sm2_HardwareTest_Create()
{
	sm2_HardwareTest = new GUI_SubMenu();
	sm2_HardwareTest->Add( MsgGetString(Msg_00426), 0, 0, NULL, fn_HeadTest );       // Heads test
	sm2_HardwareTest->Add( MsgGetString(Msg_00427), 0, 0, NULL, fn_FeederTest );     // Feeders test
	sm2_HardwareTest->Add( MsgGetString(Msg_00428), 0, 0, NULL, fn_ToolChangeTest ); // Tool exchange
	sm2_HardwareTest->Add( MsgGetString(Msg_00429), 0, 0, NULL, fn_AxesMoveTest );   // Axes movement
	sm2_HardwareTest->Add( MsgGetString(Msg_01113), 0, 0, NULL, fn_CenteringTest );  // Centering test
	sm2_HardwareTest->Add( MsgGetString(Msg_00605), 0, 0, NULL, Call_HTest );        // Board hardware test
	sm2_HardwareTest->Add( MsgGetString(Msg_01864), 0, Get_UseExtCam() ? 0 : SM_GRAYED, NULL, Call_HTest_ExtCam ); // External camera test

	#ifdef __SNIPER
	sm2_HardwareTest->Add( MsgGetString(Msg_00965), 0, 0, Q_SubLasTest, NULL );      // Sniper test
	#endif

	#ifdef __USE_MOTORHEAD
	sm2_HardwareTest->Add( MsgGetString(Msg_00098), 0, 0, NULL, fn_MotorheadLogParams );
	#endif

	sm2_HardwareTest->Add( MsgGetString(Msg_00999), 0, Get_ConveyorEnabled() ? 0 : SM_GRAYED, NULL, fn_ConveyorTest ); // Conveyor test
}

void sm2_HardwareTest_Destroy()
{
	MyDELETE( sm2_HardwareTest );
}


	//-------------------------------------
	// SM Level 1: Utility
	//-------------------------------------

extern GUI_SubMenu* Q_MenuBackup;
extern GUI_SubMenu* Q_SubNet;

void sm_Utility_Create()
{
	sm2_HardwareTest_Create();

	sm_Utility = new GUI_SubMenu();
	sm_Utility->Add( MsgGetString(Msg_00411), 0, 0, sm2_HardwareTest, NULL ); // Hardware  test
	sm_Utility->Add( MsgGetString(Msg_01802), 0, 0, Q_MenuBackup, NULL );     // Backup
	sm_Utility->Add( MsgGetString(Msg_01634), 0, IsNetEnabled() ? 0 : SM_GRAYED, Q_SubNet, NULL ); // Network operations
	sm_Utility->Add( MsgGetString(Msg_00089), 0, 0, NULL, fn_MachineInfo );   // Info
}

void sm_Utility_Destroy()
{
	sm2_HardwareTest_Destroy();
	
	MyDELETE( sm_Utility );
}



	//-------------------------------------
	// SM Level 1: Configuration
	//-------------------------------------

GUI_SubMenu* sm_Configuration;

		//-------------------------------------
		// SM Level 2: Working parameters
		//-------------------------------------

extern int G_C2Off();
extern GUI_SubMenu* Q_SubAxCal;
extern int Call_CfgDosTime();
extern int Call_CfgDosOff();
extern GUI_SubMenu* Q_MenuSubDosaPar;
extern GUI_SubMenu* Q_MenuSubDosaOff;

GUI_SubMenu* sm2_WorkParams;

void sm2_WorkParams_Create()
{
	sm2_WorkParams = new GUI_SubMenu();
	sm2_WorkParams->Add( MsgGetString(Msg_00415), 0, 0, NULL, fn_FeedersDefPosition );        // Feeder position
	sm2_WorkParams->Add( MsgGetString(Msg_07023), 0, 0, NULL, fn_SpeedAcceleration );         // speed and acceleration
	sm2_WorkParams->Add( MsgGetString(Msg_00417), 0, 0, NULL, fn_ToolsParams );               // Tools parameters
	sm2_WorkParams->Add( MsgGetString(Msg_00418), 0, 0, NULL, fn_NozzleOffsetCalibration );   // Camera-nozzle offset
	sm2_WorkParams->Add( MsgGetString(Msg_00113), 0, 0, NULL, fn_PlacementMapping );          // Placement mapping
	sm2_WorkParams->Add( MsgGetString(Msg_00238), 0, 0, NULL, fn_WorkingModes );       // Operating modes
	sm2_WorkParams->Add( MsgGetString(Msg_01521), 0, 0, NULL, fn_ControlParams );      // Control parameters
	sm2_WorkParams->Add( MsgGetString(Msg_00421), 0, 0, Q_SubAxCal, NULL );            // Axes calibration
	sm2_WorkParams->Add( MsgGetString(Msg_00422), 0, 0, NULL, fn_UnitOriginPosition ); // Unit origin calibration
	sm2_WorkParams->Add( MsgGetString(Msg_01227), 0, 0, NULL, fn_PlacementAreaCalib ); // Placement area z-calibr.
	#ifdef __SNIPER
	sm2_WorkParams->Add( MsgGetString(Msg_05023), 0, 0, NULL, fn_DeltaZCalibration );  // Snipers delta z
	#endif
	sm2_WorkParams->Add( MsgGetString(Msg_07025), 0, 0, NULL, fn_LimitsPositions );    // limits and positions
	#ifndef __DISP2
	sm2_WorkParams->Add( MsgGetString(Msg_00125), 0, QParam.Dispenser ? 0 : SM_GRAYED, NULL, Call_CfgDosTime ); // Distribution parameters
	sm2_WorkParams->Add( MsgGetString(Msg_00656), 0, QParam.Dispenser ? 0 : SM_GRAYED, NULL, Call_CfgDosOff );  // Camera-dispenser offset
	#else
	sm2_WorkParams->Add( MsgGetString(Msg_00125), 0, QParam.Dispenser ? 0 : SM_GRAYED, Q_MenuSubDosaPar, NULL ); // Distribution parameters
	sm2_WorkParams->Add( MsgGetString(Msg_00656), 0, QParam.Dispenser ? 0 : SM_GRAYED, Q_MenuSubDosaOff, NULL ); // Camera-dispenser offset
	#endif
	sm2_WorkParams->Add( MsgGetString(Msg_00425), 0, 0, NULL, fn_CenteringCameraParams ); // Centering camera
	sm2_WorkParams->Add( MsgGetString(Msg_00755), 0, 0, NULL, fn_VisionParams );       // Vision parameters
	sm2_WorkParams->Add( MsgGetString(Msg_01902), 0, 0, NULL, fn_WarmUpParams );       // Warm-Up parameters
	sm2_WorkParams->Add( MsgGetString(Msg_01635), 0, 0, NULL, fn_MachineID );          // machine id
}

void sm2_WorkParams_Destroy()
{
	MyDELETE( sm2_WorkParams );
}

	//-------------------------------------
	// SM Level 1: Configuration
	//-------------------------------------

extern int Call_ConfComand();

void sm_Configuration_Create()
{
	sm2_WorkParams_Create();
	
	sm_Configuration = new GUI_SubMenu();
	sm_Configuration->Add( MsgGetString(Msg_00237), 0, 0, sm2_WorkParams, NULL );  // working parameters
	sm_Configuration->Add( MsgGetString(Msg_00128), 0, 0, NULL, Call_ConfComand ); // reserved parameters
}

void sm_Configuration_Destroy()
{
	sm2_WorkParams_Destroy();
	
	MyDELETE( sm_Configuration );
}



	//-------------------------------------
	// SM Level 1: Exit
	//-------------------------------------

int fn_Exit()
{
#ifdef __UBUNTU18
	int exitcode = W_DeciYNE( 1, MsgGetString(Msg_00133) );
	if( exitcode != NO )
	{
		Call_Exit(exitcode);
	}
#elif __UBUNTU16
	int exitcode = W_DeciYNE( 1, MsgGetString(Msg_00133) );
	if( exitcode != NO )
	{
		Call_Exit(exitcode);
	}
#else
	if( W_Deci( 1, MsgGetString(Msg_00133) ) )
	{
		Call_Exit();
	}
#endif

	return 1;
}



	//-------------------------------------
	// MAIN MENU
	//-------------------------------------

#define Y_INC		(3*GUI_CharH())
#define Y_START		(Y_INC)

// Aggiunta delle voci al menu principale
void MainMenu_Create( GUI_MainMenu** mainMenu )
{
	// create sub menu
	sm_Management_Create();
	sm_Utility_Create();
	sm_Configuration_Create();
	
	// create main menu
	*mainMenu = new GUI_MainMenu();

	// production
	GUIMenuEntry* e1 = new GUIMenuEntry( MsgGetString(Msg_00279) );
	e1->SetCallback( G_Prg );
	e1->SetPos( MM_MENU_POS_X, Y_START );

	// management
	GUIMenuEntry* e2 = new GUIMenuEntry( MsgGetString(Msg_00280) );
	e2->SetSubMenu( sm_Management );
	e2->SetPos( MM_MENU_POS_X, Y_START + Y_INC );

	// utility
	GUIMenuEntry* e3 = new GUIMenuEntry( MsgGetString(Msg_00281) );
	e3->SetSubMenu( sm_Utility );
	e3->SetPos( MM_MENU_POS_X, Y_START + 2*Y_INC );

	// configuration
	GUIMenuEntry* e4 = new GUIMenuEntry( MsgGetString(Msg_00282) );
	e4->SetSubMenu( sm_Configuration );
	e4->SetPos( MM_MENU_POS_X, Y_START + 3*Y_INC );

	// exit
	GUIMenuEntry* e5 = new GUIMenuEntry( MsgGetString(Msg_00283) );
	e5->SetCallback( fn_Exit );
	e5->SetPos( MM_MENU_POS_X, int((GUI_ScreenH()-Y_START)/Y_INC)*Y_INC );
	e5->SetMode( MM_MODE_SQUARE );
	
	(*mainMenu)->Add( e1 );
	(*mainMenu)->Add( e2 );
	(*mainMenu)->Add( e3 );
	(*mainMenu)->Add( e4 );
	(*mainMenu)->Add( e5 );
}

void MainMenu_Destroy( GUI_MainMenu** mainMenu )
{
	MyDELETE( *mainMenu );
	
	sm_Management_Destroy();
	sm_Utility_Destroy();
	sm_Configuration_Destroy();
}
