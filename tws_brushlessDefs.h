//---------------------------------------------------------------------------
//
// Name:        BrushlessModuleDefs.h
// Author:      Gabriel Ferri
// Created:     17/02/2011
// Description: BrushlessModule commands
//
//---------------------------------------------------------------------------

#ifndef __BRUSHLESSMODULE_DEFS_H
#define __BRUSHLESSMODULE_DEFS_H

// Comandi ricevuti dalla seriale
#define READREGISTER_CMD        	1
#define WRITEREGISTER_CMD       	2
#define GETVERSION_CMD          	3
#define MICRORESET_CMD          	4
#define SUSPENDSERIAL_CMD       	5
#define LOOPENABLE_CMD          	6
#define LOOPDISABLE_CMD         	7
#define GETENCODER_CMD          	8
#define PIDIQUPDATE_CMD       		9
#define PIDIDUPDATE_CMD       		10
#define PIDSPDUPDATE_CMD       		11
#define PIDPOSUPDATE_CMD       		12
#define ENCPARAMUPDATE_CMD      	13
#define CALANGUPDATE_CMD       		14
#define PPRUPDATE_CMD       		15
#define LINEENCUPDATE_CMD			16
#define LOGTRIGUPDATE_CMD      		17
#define LOGPRESCALUPDATE_CMD    	18
#define GETLOG1L_CMD       			19
#define GETLOG1H_CMD       			20
#define GETLOG2L_CMD       			21
#define GETLOG2H_CMD       			22
#define GETLOG3L_CMD       			23
#define GETLOG3H_CMD       			24
#define GETLOG4L_CMD				25
#define GETLOG4H_CMD       			26
#define LOOPSPDENABLE_CMD       	27
#define GETSPEEDPU_CMD				28
#define RESETENCODER_CMD       		29
#define PWMENABLE_CMD       		30
#define PWMDISABLE_CMD  	     	31
#define PHASEENABLE_CMD				32
#define PHASEDISABLE_CMD			33
#define ADCCALENABLE_CMD			34
#define ADCCALDISABLE_CMD			35
#define GETCLARKEA_CMD				36
#define GETCLARKEB_CMD				37
#define GETPIDIQOUT_CMD				38
#define GETPIDIDOUT_CMD			    39
#define GETPIDSPDOUT_CMD			40
#define GETPIDPOSOUT_CMD			41
#define GETENCODERTYPE_CMD			42
#define SETENCODERTYPE_CMD  	    43
#define GETPIDIQFDB_CMD				44
#define GETPIDIDFDB_CMD				45
#define GETPIDSPDFDB_CMD			46
#define GETPIDPOSFDB_CMD			47
#define GETSPEEDRPM_CMD				48
#define SETENCODERQEPNORM_CMD		49
#define SETENCODERQEPINV_CMD		50
#define GETENCODERQEPMODE_CMD		51
#define ENCODERDISABLE_CMD			52
#define ENCODERENABLE_CMD			53
#define POSUPDATE_CMD				54
#define DECCOMPDISABLE_CMD			55
#define DECCOMPENABLE_CMD			56
#define SETACTPIDSPD_CMD			57
#define SETACTPIDPOS_CMD			58
#define LOWTOHIGHTRIG_CMD			59
#define HIGHTOLOWTRIG_CMD			60
#define GETENCODERPULSES_CMD		61
#define PTMUPDATE_CMD				62
#define GETSPEEDMS_CMD				63
#define IMMEDIATESTOP_CMD			64
#define ALARMRESET_CMD				65
#define DANGERDISABLE_CMD			66
#define DANGERENABLE_CMD			67
#define HOME_CMD					68
#define HOMESEARCH_CMD				69
#define SETREFACTUALPOS_CMD			70
#define PIDSPDRESET_CMD				71
#define PIDPOSRESET_CMD				72
#define UPDATECURRENT_CMD			73
#define GETPHASEC_CMD				74
#define GETENCODER360_CMD			75
#define GETELECTHETA_CMD			76
#define MULTIMOVE_CMD				79
#define RESET_MULTIMOVE_CMD			80
#define GETENCODER2_CMD				81
#define GETENCODER2PULSES_CMD		82
#define GETENCODERTRANSITIONS_CMD	83
#define LINEENC2UPDATE_CMD			84
#define SETENCODERSERNORM_CMD		85
#define SETENCODERSERINV_CMD		86
#define GETENCODERSERMODE_CMD		87
#define GETENCODERINTERP_CMD		88
#define GETSTATUS_RESETMM_CMD		89
#define ROTATEPHASE_CMD         	90
#define GETCOMPLEXSTATUS_CMD    	91
#define UPDATESPDFILTER_CMD     	92
// from 4.10
#define GETVPWR_CMD             	93
// from 4.11
#define GETSENSORTEMP_CMD			94
// from 4.15
#define UPDATEOVERSPEED_CMD			96
#define OVERSPEEDCHECK_CMD			97
// from 5.1
#define RESETSPEED_CMD				99


#endif // __BRUSHLESSMODULE_DEFS_H
