/***************************************************************************
 *   Copyright (C) 2008 by TWS Automation                                  *
 *                                                                         *
 *  Gestione del ciclo main                                                *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#include <signal.h>

#include "q_cost.h"
#include "filefn.h"
#include "msglist.h"
#include "q_oper.h"
#include "q_init.h"
#include "q_inifile.h"
#include "sniper.h"
#include "motorhead.h"
#include "q_conveyor.h"
#include "working_log.h"

#ifdef __LOG_ERROR
#include "q_logger.h"
CLogger QuadraLogger( "qerror.log", "Quadra Errors Log File" );
CLogger BackupLogger( "qbackup.log", "Quadra Backup Errors Log File" );
#endif

extern CWorkingLog workingLog;

#include <mss.h>


extern int StartProc();


//---------------------------------------------------------------------------
// Signal handler: backtrace
//---------------------------------------------------------------------------
void sig_handler( int sig_num )
{
	#ifdef __DEBUG
	void* array[10];

	// get void*'s for all entries on the stack
	size_t size = backtrace( array, 10 );

	// print out all the frames to stdout
	fprintf( stdout, "signal %d (%s)\n", sig_num, strsignal(sig_num) );
	backtrace_symbols_fd( array, size, 1 );
	#endif

	exit(EXIT_FAILURE);
}



int main( int argc, char** argv )
{
	// install our handler
	signal( SIGSEGV, sig_handler );

	ReadIniFile();

	argc--;
	argv++;
	while( argc > 0 )
	{
		if(!strcmp(*argv,"/sniper"))
		{
			UpdateSniper();
			return 0;
		}

		if(!strcmp(*argv,"/activation"))
		{
			ActivateSniper();
			return 0;
		}

		if(!strcmp(*argv,"/motorhead"))
		{
			UpdateMotorhead();
			return 0;
		}

		if(!strcmp(*argv,"/conveyor"))
		{
			UpdateConveyor();
			return 0;
		}

		#ifdef __DISP2
		if(!strcasecmp(*argv,"/d1"))
		{
			Set_SingleDispenserPar(1);
		}
		#endif		

		argv++;
		argc--;
	}

	#ifdef __DISP2
		#ifndef __DISP2_CAM
		if( !Get_SingleDispenserPar() )
		{
			// in configurazione doppio dispenser la telecamera esterna non e' raggiungibile
			Reset_UseExtCam();
		}
		#endif
	#endif

	EnterGraphicMode();

	ReadLanguageFile();
	ReadMachineConfigFiles();


	//TODO: dividire InitProc in piu' funzioni in base al tipo di init che fanno
	int ret = 0;
	if( InitProc() )
	{
		open_tv();

		ret = StartProc();

		workingLog.LogStatus( STATUS_OFF );

		close_tv();
	}

	WriteMachineConfigFiles();

	ExitGraphicMode();

	if( ret == YES )
	{
		char buf[100];
		snprintf( buf, sizeof(buf),"shutdown -h now" );
		system( buf );
	}
}
