//---------------------------------------------------------------------------
//
// Name:        tws_stepperDefs.h
// Author:      Gabriel Ferri
// Created:     17/02/2011
// Description: StepperModule commands
//
//---------------------------------------------------------------------------

#ifndef __STEPPERMODULE_DEFS_H
#define __STEPPERMODULE_DEFS_H

// Comandi ricevuti dalla seriale
#define READREGISTER_CMD       1
#define WRITEREGISTER_CMD      2
#define ENABLEDRIVER_CMD       3
#define DISABLEDRIVER_CMD      4
#define FREEROTATION_CMD       5
#define STOPROTATION_CMD       6
#define ROTATIONDIR_CMD        7
#define MICROSTEPPING_CMD      8
#define DECAYMODE_CMD          9
#define RELATIVEMOVE_CMD       10
#define LOWCURRENT_CMD         11
#define MAXCURRENT_CMD         12
#define ZEROPOSITION_CMD       13
#define ABSOLUTEMOVE_CMD       14
#define GETVERSION_CMD         15
#define LIMITSCHECK_CMD        16
#define CWLIMITINPUT_CMD       17
#define CCWLIMITINPUT_CMD      18
#define ALARMRESET_CMD         19
#define MICRORESET_CMD         20
#define SUSPENDSERIAL_CMD      21
#define LIMITSLEVEL_CMD        22
#define ACTUALPOS_CMD			23 //n.u.
#define LOGTRIGUPDATE_CMD      	24
#define LOGPRESCALUPDATE_CMD   25
#define GETLOG1L_CMD       		26
#define GETLOG1H_CMD       		27
#define GETLOG2L_CMD       		28
#define GETLOG2H_CMD       		29
#define GETLOG3L_CMD       		30
#define GETLOG3H_CMD       		31
#define GETLOG4L_CMD       		32
#define GETLOG4H_CMD       		33
#define LOWTOHIGHTRIG_CMD		34
#define HIGHTOLOWTRIG_CMD		35
#define HOMESEARCH_CMD			36
#define SET_OUTPUT_LOW_CMD		37
#define SET_OUTPUT_HIGH_CMD		38
#define GETSENSORTEMP_CMD   	39
#define GETVPWR_CMD   			40
#define HOMESENSORMODE_CMD		41

#endif // __STEPPERMODULE_DEFS_H
