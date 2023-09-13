//---------------------------------------------------------------------------
// Name:        q_inifile.cpp
// Author:      Gabriel Ferri
// Created:     11/07/2012
// Description:
//---------------------------------------------------------------------------
#include "q_inifile.h"

#include <stdio.h>

#include "q_cost.h"
#include "strutils.h"

#include <mss.h>


	//-----------------------------//
	// Variabili di configurazione //
	//-----------------------------//

bool use_extcam        = true;
bool show_assemblytime = false;
bool windowed_mode     = false;
bool use_net           = true;
bool use_reed          = true;
bool use_cam           = true;
bool use_finec         = true;
bool use_fox           = true;
bool fox_log           = false;
bool use_cpu           = true;
bool use_feederdir     = false;
bool discard_log       = false;
bool use_qmode         = false;

#ifdef __SNIPER
bool use_sniper[2]     = { true, true };
#endif

bool use_motorhead     = true;
bool check_uart        = true;
bool use_brush[2]      = { true, true };
bool use_step[2]       = { true, true };
bool use_tools20       = false;
bool error_log         = false;
bool demo_mode         = false;
bool asciiqid_mode     = false;
bool motorhead_on_uart = false;
bool use_steppers      = true;
bool res_1600          = false;
bool res_1920          = false;
bool use_shutter 	   = true;
bool working_log 	   = false;
int  fox_current 	   = 0;
int  fox_time 	   	   = 0;

//---------------------------------------------------------------------------
// Legge file di configurazione software (.ini)
//---------------------------------------------------------------------------
int ReadIniFile()
{
	FILE* finit = fopen( INIFILE, "r" );
	if( finit == NULL )
		return 0;

	char buf[80];
	char *p;

	while(!feof(finit))
	{
		fgets( buf, sizeof(buf), finit );
		strupr( buf );

		char* p = strchr(buf,'\n');

		if(p!=NULL)
		{
			*p='\0';
		}

		p=strchr(buf,'\r');

		if(p!=NULL)
		{
			*p='\0';
		}

		DelSpcR(buf);

		if( !strcmp(buf,"NOREED") )
		{
			use_reed = false;
			continue;
		}

		if( !strcmp(buf,"NOUART") )
		{
			check_uart = false;

			#ifdef __SNIPER
			use_sniper[0] = use_sniper[1] = false;
			#endif

			use_fox = false;
			use_cpu = false;
			continue;
		}

		#ifdef __SNIPER
		if(!strcmp(buf, "NOSNIPER"))
		{
			use_sniper[0] = use_sniper[1] = false;
			continue;
		}

		if(!strcmp(buf, "NOSNIPER1"))
		{
			use_sniper[0] = false;
			continue;
		}

		if(!strcmp(buf, "NOSNIPER2"))
		{
			use_sniper[1] = false;
			continue;
		}
		#endif

		if(!strcmp(buf, "NOCPU"))
		{
			use_cpu = false;
			continue;
		}

		if(!strcmp(buf, "NOMOTORHEAD"))
		{
			use_motorhead = 0;
			continue;
		}

		if(!strcmp(buf, "NOFOX"))
		{
			use_fox = false;
			continue;
		}

		if(!strcmp(buf, "NOCAM"))
		{
			use_cam = false;
			continue;
		}

		if(!strcmp(buf, "NOBRUSH1"))
		{
			use_brush[0] = false;
			continue;
		}

		if(!strcmp(buf, "NOBRUSH2"))
		{
			use_brush[1] = false;
			continue;
		}

		if(!strcmp(buf, "NOSTEP1"))
		{
			use_step[0] = false;
			continue;
		}

		if(!strcmp(buf, "NOSTEP2"))
		{
			use_step[1] = false;
			continue;
		}

		if(!strcmp(buf, "FOXLOG"))
		{
			fox_log = true;
			continue;
		}

		if(!strcmp(buf, "NOFINEC"))
		{
			use_finec = false;
			continue;
		}

		if(!strcmp(buf, "NOEXTCAM"))
		{
			use_extcam = false;
			continue;
		}

		if(!strcmp(buf, "NONET"))
		{
			use_net = false;
			continue;
		}

		if(!strcmp(buf, "DEMO"))
		{
			demo_mode = true;

			check_uart = false;
			use_fox = false;
			use_motorhead = false;
			use_cpu = false;

			#ifdef __SNIPER
			use_sniper[0] = use_sniper[1] = false;
			#endif
			continue;
		}

		if(!strcmp(buf, "ASSTIME"))
		{
			show_assemblytime = true;
			continue;
		}

		if(!strcmp(buf, "FEEDERDIR"))
		{
			use_feederdir = true;
			continue;
		}

		if(!strcmp(buf, "DISCARDLOG"))
		{
			discard_log = true;
			continue;
		}

		#ifdef __SNIPER
		if(!strcmp(buf, "QMODE"))
		{
			use_qmode = true;
			continue;
		}
		#endif

		if(!strcmp(buf, "WINDOWED"))
		{
			windowed_mode = true;
			continue;
		}

		if(!strcmp(buf, "TOOLS20"))
		{
			use_tools20 = true;
			continue;
		}

		if(!strcmp(buf, "ERRORLOG"))
		{
			error_log = true;
			continue;
		}

		if(!strcmp(buf, "ASCIIQID"))
		{
			asciiqid_mode = true;
			continue;
		}

		if(!strcmp(buf, "MHONUART"))
		{
			motorhead_on_uart = true;
			continue;
		}

		if(!strcmp(buf, "NOSTEPPERS"))
		{
			use_steppers = false;
			continue;
		}

		if(!strcmp(buf, "RES1600"))
		{
			res_1600 = true;
			continue;
		}

		if(!strcmp(buf, "RES1920"))
		{
			res_1920 = true;
			continue;
		}

		if(!strcmp(buf, "NOSHUTTER"))
		{
			use_shutter = false;
			continue;
		}

		if(!strcmp(buf, "WORKINGLOG"))
		{
			working_log = true;
			continue;
		}

		p = strstr( buf, "_FOXMAXCURRENT" );
		if( p != NULL )
		{
			fox_current = atoi( buf );
			continue;
		}

		p = strstr( buf, "_FOXMAXTIME" );
		if( p != NULL )
		{
			fox_time = atoi( buf );
			continue;
		}

	}

	fclose( finit );
	return 1;
}




bool Get_UseExtCam() { return use_extcam; }
bool Get_ShowAssemblyTime() { return show_assemblytime; }
bool Get_WindowedMode() { return windowed_mode; }

bool Get_UseNetwork()
{
#ifndef HWTEST_RELEASE
	return use_net;
#else
	return false;
#endif
}

bool Get_UseReed() { return use_reed; }
bool Get_CheckUart() { return check_uart; }
#ifdef __SNIPER
bool Get_UseSniper( int num ) { return use_sniper[num-1]; }
#endif
bool Get_Tools20() { return use_tools20; }
bool Get_WriteErrorLog() { return error_log; }
bool Get_WriteDiscardLog() { return discard_log; }
bool Get_UseCommonFeederDir() { return use_feederdir; }
bool Get_UseQMode() { return use_qmode; }
bool Get_UseCam() { return use_cam; }
bool Get_UseCpu() { return use_cpu; }
bool Get_UseFinec() { return use_finec; }
bool Get_UseMotorhead() { return use_motorhead; }
bool Get_UseFox() { return use_fox; }
bool Get_FoxLog() { return fox_log; }
bool Get_UseBrush( int num ) { return use_brush[num]; }
bool Get_UseStep( int num ) { return use_step[num]; }

bool Get_DemoMode()
{
#ifndef HWTEST_RELEASE
	return demo_mode;
#else
	return false;
#endif
}

bool Get_AsciiqIdMode() { return asciiqid_mode; }
bool Get_MotorheadOnUart() { return motorhead_on_uart; }

void Reset_UseExtCam() { use_extcam = false; }
bool Get_UseSteppers() { return use_steppers; }
bool Get_Res1600() { return res_1600; }
bool Get_Res1920() { return res_1920; }
bool Get_ShutterEnable() { return use_shutter; }
bool Get_WorkingLog() { return working_log; }
bool Reset_WorkingLog() { working_log = false; }
int Get_FoxCurrent() { return fox_current; }
int Get_FoxTime() { return fox_time; }

