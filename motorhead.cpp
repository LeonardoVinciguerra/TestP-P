//---------------------------------------------------------------------------
//
// Name:        motorhead.cpp
// Author:      Gabriel Ferri
// Created:     21/06/2011
// Description: MotorheadClass implementation
//
//---------------------------------------------------------------------------
#include "motorhead.h"


#ifdef __USE_MOTORHEAD

#include <stdio.h>
#include <math.h>
#include "iqmath.h"
#include "lnxdefs.h"
#include "q_files_new.h"
#include "commclass.h"
#include "comaddr.h"
#include "q_debug.h"
#include "q_oper.h"
#include "q_grcol.h"
#include "q_help.h"

#include <mss.h>


#define HEAD_X_ADDRESS		1
#define HEAD_Y_ADDRESS		2
#define HEAD_ALL_ADDRESS	0

#define MINIMUM_FREQ		0.05
#define MINIMUM_ACC			0.5

#define MOTOR_RETRY			3
#define MOTOR_STATUS_DELAY	5

#define AXIS_LIMIT_INPUT	2


//GF_TEMP
#define NUM_PID				7

struct MotorParamsStruct_100
{
	int BoardMaxCurrent;
	int MaxCurrent;
	int Poles;
	int PPR;
	int LineEnc1;
	int QepEncMode;
	int SerEncMode;
	int LineEnc2;
	int PulsesToMm;
	float TargetError;
	float TargetSpeed;
	float ProximityGap;
	float PIDPosActivationGap;
	float FollowingError;
	int SpeedFilter;
	int StartTicks;
	float SearchSpeedMax;
	float SearchSpeedMin;
	int Jerk;
	float CFriction;
	float SpeedFFWD;
	float AccFFWD;
	int CalAngle;
	int PID_SpdDef;
	int PID_Spd0;
	int PID_Spd;
	int PID_PosDef;
	int PID_Pos0;
	int PID_Id;
	int PID_Iq;
};

struct PIDParamsStruct_100
{
	float KP;
	float KI;
	float KD;
	float KC;
	float OMax;
	float OMin;
};



	//-------------------//
	//    Global Vars    //
	//-------------------//

MotorheadClass* Motorhead = 0;

BrushlessModule* head[MOTOR_NUM];
MotorParamsStruct_100 headParams[MOTOR_NUM];
PIDParamsStruct_100 PIDParams[MOTOR_NUM*NUM_PID];

MotorheadLogData motorheadLogData;

extern SMachineInfo MachineInfo;
double MI_XMovement_m = 0.0;
double MI_YMovement_m = 0.0;



	//------------------------//
	//    Member Functions    //
	//------------------------//

//---------------------------------------------------------------------------------
// Costruttore
//---------------------------------------------------------------------------------
MotorheadClass::MotorheadClass( void* comIntRef )
{
	enabled = true;
	error = ERR_NOERROR;

	currentX = 0.f;
	currentY = 0.f;

	for( int i = 0; i < MOTOR_NUM; i++ )
		versionStr[i][0] = '\0';

	lastSpeed = 1.0; // dafault speed (m/s)
	lastAcc = 3.0;   // dafault acc (m/s^2)

	head[HEADX_ID] = new BrushlessModule( comIntRef, HEAD_X_ADDRESS );
	head[HEADY_ID] = new BrushlessModule( comIntRef, HEAD_Y_ADDRESS );
	head[HEADALL_ID] = new BrushlessModule( comIntRef, HEAD_ALL_ADDRESS );

	// disable all
	DisableMotors();

	Enable();

	// read version
	_readVersion( HEADX_ID );
	_readVersion( HEADY_ID );
	// build version string
	snprintf( &versionStr[HEADX_ID][0], sizeof(versionStr[HEADX_ID]), "%d.%d", ver[HEADX_ID], rev[HEADX_ID] );
	snprintf( &versionStr[HEADY_ID][0], sizeof(versionStr[HEADY_ID]), "%d.%d", ver[HEADY_ID], rev[HEADY_ID] );

	if( error != ERR_NOERROR )
		enabled = false;
}

//---------------------------------------------------------------------------------
// Costruttore
//---------------------------------------------------------------------------------
MotorheadClass::MotorheadClass()
{
	enabled = false;
	error = ERR_NOERROR;

	currentX = 0.f;
	currentY = 0.f;

	for( int i = 0; i < MOTOR_NUM; i++ )
	{
		versionStr[i][0] = '\0';
		head[i] = 0;
	}
}

//---------------------------------------------------------------------------------
// Distruttore
//---------------------------------------------------------------------------------
MotorheadClass::~MotorheadClass()
{
	for( int i = 0; i < MOTOR_NUM; i++ )
	{
		if( head[i] )
		{
			_disableDriver( i );

			delete head[i];
			head[i] = 0;
		}
	}
}

//---------------------------------------------------------------------------------
// Disabilita driver motore
//---------------------------------------------------------------------------------
bool MotorheadClass::_disableDriver( int module )
{
	if( !enabled )
		return true;

	if( head[module]->ResetDrive() == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	return true;
}

//---------------------------------------------------------------------------------
// Legge la versione del firmware
//---------------------------------------------------------------------------------
bool MotorheadClass::_readVersion( int module )
{
	ver[module] = 0;
	rev[module] = 0;

	if( !enabled )
		return true;

	unsigned short version = head[module]->GetVersion();
	ver[module] = version >> 8;
	rev[module] = version;

	if( ver[module] == 255 && rev[module] == 255 )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------
// GetStatus
//----------------------------------------------------------------------------------
bool MotorheadClass::GetStatus( int module, int& status )
{
	if( !enabled )
	{
		status = 0;
		return true;
	}

	int res = head[module]->MotorStatus();
	if( res == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	status = res;
	return true;
}

//----------------------------------------------------------------------------------
// GetPIDStatus
//----------------------------------------------------------------------------------
bool MotorheadClass::GetPIDStatus( int module, int& status )
{
	if( !enabled )
	{
		status = 0;
		return true;
	}

	int res = head[module]->PIDStatus();
	if( res == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	status = res;
	return true;
}

//----------------------------------------------------------------------------------
// GetEncoderStatus
//----------------------------------------------------------------------------------
bool MotorheadClass::GetEncoderStatus( int module, int& status )
{
	if( !enabled )
	{
		status = 0;
		return true;
	}

	int res = head[module]->EncoderStatus();
	if( res == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	status = res;
	return true;
}

//----------------------------------------------------------------------------------
// Legge valore ingresso del limite del movimento motore (limit switch)
//----------------------------------------------------------------------------------
bool MotorheadClass::GetLimitSwitchInput( int module, int& state )
{
	if( !enabled )
	{
		state = false;
		return true;
	}

	int res = head[module]->ActualInputs();
	if( res == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	state = res & AXIS_LIMIT_INPUT ? 1 : 0;
	return true;
}

//---------------------------------------------------------------------------------
// Legge la posizione (mm) data dagli encoders
//---------------------------------------------------------------------------------
bool MotorheadClass::ReadEncoderMm( float& x, float& y )
{
	if( !enabled )
	{
		x = 0.f;
		y = 0.f;
		return true;
	}

	int posX = head[HEADX_ID]->GetEncoderActualPosition();
	int posY = head[HEADY_ID]->GetEncoderActualPosition();

	x = (_IQ24ToF( posX ) - 0.5) * headParams[HEADX_ID].LineEnc1 / headParams[HEADX_ID].PulsesToMm;
	y = (_IQ24ToF( posY ) - 0.5) * headParams[HEADY_ID].LineEnc1 / headParams[HEADY_ID].PulsesToMm;

	return true;
}

//---------------------------------------------------------------------------------
// Legge la posizione di riferimento
//---------------------------------------------------------------------------------
bool MotorheadClass::ReadRefPosition( float& x, float& y )
{
	if( !enabled )
	{
		x = 0.f;
		y = 0.f;
		return true;
	}

	int posX = head[HEADX_ID]->ActualPosition();
	int posY = head[HEADY_ID]->ActualPosition();

	x = _IQ24ToF( posX );
	y = _IQ24ToF( posY );

	return true;
}

//---------------------------------------------------------------------------------
// Setta valori PID del modulo
//---------------------------------------------------------------------------------
bool MotorheadClass::_setPIDs( int module )
{
	if( !enabled )
		return true;

	MotorParamsStruct_100* pHead = &headParams[module];

	// Set PIDs parameters
	if( head[module]->SetPIDIqParams( _IQ24(PIDParams[pHead->PID_Iq].KP), _IQ24(PIDParams[pHead->PID_Iq].KI), _IQ24(PIDParams[pHead->PID_Iq].KD), _IQ24(PIDParams[pHead->PID_Iq].KC), _IQ24(PIDParams[pHead->PID_Iq].OMax), _IQ24(PIDParams[pHead->PID_Iq].OMin) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	if( head[module]->SetPIDIdParams( _IQ24(PIDParams[pHead->PID_Id].KP), _IQ24(PIDParams[pHead->PID_Id].KI), _IQ24(PIDParams[pHead->PID_Id].KD), _IQ24(PIDParams[pHead->PID_Id].KC), _IQ24(PIDParams[pHead->PID_Id].OMax), _IQ24(PIDParams[pHead->PID_Id].OMin) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	if( head[module]->SetPIDSpdParams( PIDSET_DEF, _IQ24(PIDParams[pHead->PID_SpdDef].KP), _IQ24(PIDParams[pHead->PID_SpdDef].KI), _IQ24(PIDParams[pHead->PID_SpdDef].KD), _IQ24(PIDParams[pHead->PID_SpdDef].KC), _IQ24(PIDParams[pHead->PID_SpdDef].OMax), _IQ24(PIDParams[pHead->PID_SpdDef].OMin) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	if( head[module]->SetPIDSpdParams( PIDSET_0, _IQ24(PIDParams[pHead->PID_Spd0].KP), _IQ24(PIDParams[pHead->PID_Spd0].KI), _IQ24(PIDParams[pHead->PID_Spd0].KD), _IQ24(PIDParams[pHead->PID_Spd0].KC), _IQ24(PIDParams[pHead->PID_Spd0].OMax), _IQ24(PIDParams[pHead->PID_Spd0].OMin) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	if( head[module]->SetPIDSpdParams( PIDSET_SPD, _IQ24(PIDParams[pHead->PID_Spd].KP), _IQ24(PIDParams[pHead->PID_Spd].KI), _IQ24(PIDParams[pHead->PID_Spd].KD), _IQ24(PIDParams[pHead->PID_Spd].KC), _IQ24(PIDParams[pHead->PID_Spd].OMax), _IQ24(PIDParams[pHead->PID_Spd].OMin) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	if( head[module]->SetPIDPosParams( PIDSET_DEF, _IQ24(PIDParams[pHead->PID_PosDef].KP), _IQ24(PIDParams[pHead->PID_PosDef].KI), _IQ24(PIDParams[pHead->PID_PosDef].KD), _IQ24(PIDParams[pHead->PID_PosDef].KC), _IQ24(PIDParams[pHead->PID_PosDef].OMax), _IQ24(PIDParams[pHead->PID_PosDef].OMin) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	if( head[module]->SetPIDPosParams( PIDSET_0, _IQ24(PIDParams[pHead->PID_Pos0].KP), _IQ24(PIDParams[pHead->PID_Pos0].KI), _IQ24(PIDParams[pHead->PID_Pos0].KD), _IQ24(PIDParams[pHead->PID_Pos0].KC), _IQ24(PIDParams[pHead->PID_Pos0].OMax), _IQ24(PIDParams[pHead->PID_Pos0].OMin) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Setta valori PID del modulo
//---------------------------------------------------------------------------------
bool MotorheadClass::GetPIDs( int module )
{
	if( !enabled )
		return true;

	MotorParamsStruct_100* pHead = &headParams[module];

	// Get PIDs parameters
	int kp, ki, kd, kc, omax, omin;

	if( head[module]->GetPIDIqParams( &kp, &ki, &kd, &kc, &omax, &omin ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	PIDParams[pHead->PID_Iq].KP = _IQ24ToF( kp );
	PIDParams[pHead->PID_Iq].KI = _IQ24ToF( ki );
	PIDParams[pHead->PID_Iq].KD = _IQ24ToF( kd );
	PIDParams[pHead->PID_Iq].KC = _IQ24ToF( kc );
	PIDParams[pHead->PID_Iq].OMax = _IQ24ToF( omax );
	PIDParams[pHead->PID_Iq].OMin = _IQ24ToF( omin );


	if( head[module]->GetPIDIdParams( &kp, &ki, &kd, &kc, &omax, &omin ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	PIDParams[pHead->PID_Id].KP = _IQ24ToF( kp );
	PIDParams[pHead->PID_Id].KI = _IQ24ToF( ki );
	PIDParams[pHead->PID_Id].KD = _IQ24ToF( kd );
	PIDParams[pHead->PID_Id].KC = _IQ24ToF( kc );
	PIDParams[pHead->PID_Id].OMax = _IQ24ToF( omax );
	PIDParams[pHead->PID_Id].OMin = _IQ24ToF( omin );


	if( head[module]->GetPIDSpdParams( PIDSET_DEF, &kp, &ki, &kd, &kc, &omax, &omin ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	PIDParams[pHead->PID_SpdDef].KP = _IQ24ToF( kp );
	PIDParams[pHead->PID_SpdDef].KI = _IQ24ToF( ki );
	PIDParams[pHead->PID_SpdDef].KD = _IQ24ToF( kd );
	PIDParams[pHead->PID_SpdDef].KC = _IQ24ToF( kc );
	PIDParams[pHead->PID_SpdDef].OMax = _IQ24ToF( omax );
	PIDParams[pHead->PID_SpdDef].OMin = _IQ24ToF( omin );


	if( head[module]->GetPIDSpdParams( PIDSET_0, &kp, &ki, &kd, &kc, &omax, &omin ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	PIDParams[pHead->PID_Spd0].KP = _IQ24ToF( kp );
	PIDParams[pHead->PID_Spd0].KI = _IQ24ToF( ki );
	PIDParams[pHead->PID_Spd0].KD = _IQ24ToF( kd );
	PIDParams[pHead->PID_Spd0].KC = _IQ24ToF( kc );
	PIDParams[pHead->PID_Spd0].OMax = _IQ24ToF( omax );
	PIDParams[pHead->PID_Spd0].OMin = _IQ24ToF( omin );


	if( head[module]->GetPIDSpdParams( PIDSET_SPD, &kp, &ki, &kd, &kc, &omax, &omin ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	PIDParams[pHead->PID_Spd].KP = _IQ24ToF( kp );
	PIDParams[pHead->PID_Spd].KI = _IQ24ToF( ki );
	PIDParams[pHead->PID_Spd].KD = _IQ24ToF( kd );
	PIDParams[pHead->PID_Spd].KC = _IQ24ToF( kc );
	PIDParams[pHead->PID_Spd].OMax = _IQ24ToF( omax );
	PIDParams[pHead->PID_Spd].OMin = _IQ24ToF( omin );


	if( head[module]->GetPIDPosParams( PIDSET_DEF, &kp, &ki, &kd, &kc, &omax, &omin ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	PIDParams[pHead->PID_PosDef].KP = _IQ24ToF( kp );
	PIDParams[pHead->PID_PosDef].KI = _IQ24ToF( ki );
	PIDParams[pHead->PID_PosDef].KD = _IQ24ToF( kd );
	PIDParams[pHead->PID_PosDef].KC = _IQ24ToF( kc );
	PIDParams[pHead->PID_PosDef].OMax = _IQ24ToF( omax );
	PIDParams[pHead->PID_PosDef].OMin = _IQ24ToF( omin );


	if( head[module]->GetPIDPosParams( PIDSET_0, &kp, &ki, &kd, &kc, &omax, &omin ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	PIDParams[pHead->PID_Pos0].KP = _IQ24ToF( kp );
	PIDParams[pHead->PID_Pos0].KI = _IQ24ToF( ki );
	PIDParams[pHead->PID_Pos0].KD = _IQ24ToF( kd );
	PIDParams[pHead->PID_Pos0].KC = _IQ24ToF( kc );
	PIDParams[pHead->PID_Pos0].OMax = _IQ24ToF( omax );
	PIDParams[pHead->PID_Pos0].OMin = _IQ24ToF( omin );

	return true;
}

//---------------------------------------------------------------------------------
// Inizializza il modulo per il controllo motore
//---------------------------------------------------------------------------------
bool MotorheadClass::Init( int module )
{
	if( !enabled )
		return true;

	// just in case
	errorId = module;

	_iq val;

	// set board max current
	if( head[module]->SetMaxReadCurrent( headParams[module].BoardMaxCurrent ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Set current (nominal current)
	if( head[module]->SetNominalCurrent( headParams[module].MaxCurrent ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Set PIDs parameters
	if( _setPIDs( module ) == false )
		return false;

	// QEP Mode (NORMAL/INVERTED)
	if( head[module]->EncoderMode( ENCODER_QEP, headParams[module].QepEncMode ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// SER Mode (NORMAL/INVERTED)
	if( head[module]->EncoderMode( ENCODER_SER, headParams[module].SerEncMode ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Line Encoder 1
	if( head[module]->SetLineEncoder( headParams[module].LineEnc1 ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Line Encoder 2
	if( head[module]->SetLineEncoder2( headParams[module].LineEnc2 ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Pulses Per Revolution
	if( head[module]->SetPulsesPerRev( headParams[module].PPR ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Poles
	if( head[module]->SetPoles( headParams[module].Poles ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Pulses To Mm
	if( head[module]->SetPulsesToMm( headParams[module].PulsesToMm ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Calibrated Angle
	if( head[module]->SetCalibratedAngle( headParams[module].CalAngle ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Set cw and ccw limit
	int flag = (AXIS_LIMIT_INPUT << 16) + AXIS_LIMIT_INPUT;
	if( head[module]->InputsSetting( flag ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	if( head[module]->MaximumFreq( _IQ24(0.5) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// setta percentuale overspeed
	if( head[module]->SetOverSpeed( 30 ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// aggiorna valore overspeed
	if( head[module]->UpdateOverSpeed() == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// ADC calibration
	if( head[module]->SetIdRef( 0 ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}
	if( head[module]->SetIqRef( 0 ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Reset speed module
	if( head[module]->ResetSpeed() == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	for( int i=0; i<5; i++ )
	{
		if( head[module]->ADCCalEnable( ADCCAL_ON ) == MOTOR_ERROR )
		{
			error |= ERR_DRIVER;
			return false;
		}

		delay( 200 );

		if( head[module]->ADCCalEnable( ADCCAL_OFF ) == MOTOR_ERROR )
		{
			error |= ERR_DRIVER;
			return false;
		}

		delay( 200 );
	}

	// Angle calibration
	if( head[module]->ElecThetaGeneration( ELECTTHETA_OFF ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	val = _IQ24( CAL_ANGLE_CURRENT/4.0 );
	if( head[module]->SetIdRef( val ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	if( head[module]->SetPhaseRotationNumber( 2 ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	if( head[module]->SetPhaseRotation( PHASEROT_POS ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	delay( 2*CAL_PHASE_ROT_DELAY );

	if( head[module]->SetPhaseRotationNumber( 1 ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	if( head[module]->SetPhaseRotation( PHASEROT_NEG ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	delay( CAL_PHASE_ROT_DELAY );

	// setta velocita' iniziali
	if( !SetStartSpeeds( module ) )
	{
		return false;
	}

	// aggiorna valore overspeed
	if( head[module]->UpdateOverSpeed() == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	if( head[module]->ElecThetaGeneration( ELECTTHETA_OFF ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	val = _IQ24( CAL_ANGLE_CURRENT );
	if( head[module]->SetIdRef( val ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	val = _IQ24( 0 );
	if( head[module]->SetIParkAngle( val ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	delay( CAL_ANGLE_DELAY );

	if( head[module]->ResetEncoder() == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	if( head[module]->SetIdRef( 0 ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	if( head[module]->ElecThetaGeneration( ELECTTHETA_ON ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Setta il proximity gap
	if( head[module]->SetProximityGap( _IQ24( headParams[module].ProximityGap * headParams[module].PulsesToMm / headParams[module].LineEnc1 ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Setta il PID pos activation gap
	if( head[module]->SetPIDPosActivationGap( _IQ24( headParams[module].PIDPosActivationGap ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Setta il following error
	if( head[module]->SetFollowingError( _IQ24( headParams[module].FollowingError * headParams[module].PulsesToMm / headParams[module].LineEnc1 ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Setta speed filter
	if( ver[module] > 4 || ( ver[module] == 4 && rev[module] > 6 ) )
	{
		if( head[module]->SetSpeedFilter( headParams[module].SpeedFilter ) == MOTOR_ERROR )
		{
			error |= ERR_DRIVER;
			return false;
		}
	}

	// Setta start ticks
	if( head[module]->SetStartTicks( headParams[module].StartTicks ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// Abilita il ciclo di controllo di posizione
	if( head[module]->MotorEnable( MOTOR_ON ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// attiva controllo velocita'
	if( head[module]->OverSpeedCheck( 1 ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	if( head[module]->SetEndMovementDelta( _IQ24( headParams[module].TargetError * headParams[module].PulsesToMm / headParams[module].LineEnc1 ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	if( head[module]->SetEndMovementSpeed( _IQ24( headParams[module].TargetSpeed ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	// setta jerk
	if( head[module]->SetMaxJerk( headParams[module].Jerk ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// setta Coulomb friction
	if( head[module]->SetCFriction( _IQ24( headParams[module].CFriction ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// setta speed feed-forward
	if( head[module]->SetSpdFFWD( _IQ24( headParams[module].SpeedFFWD ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// setta acc feed-forward
	if( head[module]->SetAccFFWD( _IQ24( headParams[module].AccFFWD ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	// setta parametri logger
	if( SetLogParams( module ) == false )
		return false;

	return true;
}

//---------------------------------------------------------------------------------
// Setta posizione di zero
//---------------------------------------------------------------------------------
bool MotorheadClass::SetHome()
{
	if( !enabled )
		return true;

	if( head[HEADX_ID]->Home() == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return false;
	}

	if( head[HEADY_ID]->Home() == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Ripristina il ciclo di controllo dopo uno arresto immediato
//---------------------------------------------------------------------------------
bool MotorheadClass::RestartAfterImmediateStop()
{
	if( !enabled )
		return true;

	ResetAlarms();

	// setta posizione di riferimento
	if( head[HEADX_ID]->SetReferenceActualPos() == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return false;
	}
	if( head[HEADY_ID]->SetReferenceActualPos() == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
		return false;
	}

	// abilita motori
	if( !EnableMotors() )
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Muove gli assi nella posizione richiesta (XY contemporaneamente)
//---------------------------------------------------------------------------------
bool MotorheadClass::Move( float x, float y, float xNorm, bool force )
{
	if( !enabled )
	{
		return true;
	}

	bool xFlag = (x != currentX || force) ? false : true;
	bool yFlag = (y != currentY || force) ? false : true;
	if( xFlag && yFlag )
	{
		return true;
	}

	int retry = 0;
	for( ; retry < MOTOR_RETRY; retry++ )
	{
		unsigned char xAdd = !xFlag ? head[HEADX_ID]->GetAddress() : 0;
		unsigned char yAdd = !yFlag ? head[HEADY_ID]->GetAddress() : 0;

		int ret = head[HEADALL_ID]->GotoPos0_Multi(
			xAdd, _IQ24( x * headParams[HEADX_ID].PulsesToMm / headParams[HEADX_ID].LineEnc1 + 0.5 ),
			yAdd, _IQ24( y * headParams[HEADY_ID].PulsesToMm / headParams[HEADY_ID].LineEnc1 + 0.5 ),
			_IQ24( lastSpeed ),
			_IQ24( lastAcc ),
			_IQ24( headParams[HEADX_ID].ProximityGap * headParams[HEADX_ID].PulsesToMm / headParams[HEADX_ID].LineEnc1 ),
			_IQ24( headParams[HEADY_ID].ProximityGap * headParams[HEADY_ID].PulsesToMm / headParams[HEADY_ID].LineEnc1 ) );

		if( ret == MOTOR_ERROR )
		{
			error |= ERR_DRIVER;
			errorId = HEADALL_ID;
			return false;
		}

		// Aggiunto perche' il comando precedente non attende nessuna risposta
		delay( 20 );

		if( !xFlag )
		{
			int camXStatus = head[HEADX_ID]->GetStatus_ResetFlagMulti();
			if( camXStatus == MOTOR_ERROR )
			{
				error |= ERR_DRIVER;
				errorId = HEADX_ID;
				return false;
			}
			if( camXStatus & MOTOR_MULTIMOVE )
			{
				xFlag = true;
			}
		}

		if( !yFlag )
		{
			int camYStatus = head[HEADY_ID]->GetStatus_ResetFlagMulti();
			if( camYStatus == MOTOR_ERROR )
			{
				error |= ERR_DRIVER;
				errorId = HEADY_ID;
				return false;
			}
			if( camYStatus & MOTOR_MULTIMOVE )
			{
				yFlag = true;
			}
		}

		if( xFlag && yFlag )
		{
			break;
		}

		delay( MOTOR_STATUS_DELAY );
	}

	if( retry == MOTOR_RETRY )
	{
		error |= ERR_DRIVER;
		errorId = HEADALL_ID;
		return false;
	}


	// log machine movements
	MachineInfo.XMovement -= (unsigned int)MI_XMovement_m;
	MachineInfo.YMovement -= (unsigned int)MI_YMovement_m;

	MI_XMovement_m += fabs(currentX - x)/1000.f;
	MI_YMovement_m += fabs(currentY - y)/1000.f;

	MachineInfo.XMovement += (unsigned int)MI_XMovement_m;
	MachineInfo.YMovement += (unsigned int)MI_YMovement_m;

	currentX = x;
	currentY = y;

	return true;
}

//---------------------------------------------------------------------------------
// Attende fine movimento
//---------------------------------------------------------------------------------
bool MotorheadClass::Wait()
{
	if( !enabled )
	{
		return true;
	}

	int movementX = 0, movementY = 0;
	while( 1 )
	{
		error = 0;

		if( movementX == 0 )
		{
			if( _wait( HEADX_ID ) )
			{
				movementX = 1;
			}
			else if( error )
			{
				return false;
			}
		}

		if( movementY == 0 )
		{
			if( _wait( HEADY_ID ) )
			{
				movementY = 1;
			}
			else if( error )
			{
				return false;
			}
		}

		if( movementX && movementY )
		{
			break;
		}

		delay( MOTOR_STATUS_DELAY );
	}

	return true;
}

bool MotorheadClass::_wait( int module )
{
	bool movementEnd = false;

	int	status = head[module]->MotorStatus();
	if( status == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	//TODO - mettere un flag per distinguere il tipo di attesa
	if( !(status & MOTOR_RUNNING) ) //MOTOR_NEARTARGET
	{
		movementEnd = true;
	}

	if( status & MOTOR_OVERRUN )
	{
		error |= ERR_LIMITSWITCH;
		errorId = module;
		return false;
	}
	if( status & MOTOR_OVERCURRENT )
	{
		error |= ERR_OVERCURRENT;
		errorId = module;
		return false;
	}
	if( status & MOTOR_TIMEOUT )
	{
		error |= ERR_MOVETIMEOUT;
		errorId = module;
		return false;
	}
	if( status & MOTOR_DANGER )
	{
		error |= ERR_LIMITSWITCH;
		errorId = module;
		return false;
	}
	if( status & MOTOR_OVERSPEED )
	{
		error |= ERR_OVERSPEED;
		errorId = module;
		return false;
	}
	if( status & MOTOR_PROCERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	if( status & MOTOR_NOENC )
	{
		error |= ERR_NOENC;
		errorId = module;
		return false;
	}
	if( status & MOTOR_SECURITY )
	{
		error |= ERR_SECURITYSTOP;
		errorId = module;
		return false;
	}
	if( status & MOTOR_STEADYPOS )
	{
		error |= ERR_STEADYPOSITION;
		errorId = module;
		return false;
	}

	return movementEnd;
}

//---------------------------------------------------------------------------------
// Abilita motori
//---------------------------------------------------------------------------------
bool MotorheadClass::EnableMotors()
{
	if( !enabled )
		return true;

	if( head[HEADX_ID]->MotorEnable( MOTOR_ON ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return false;
	}
	if( head[HEADY_ID]->MotorEnable( MOTOR_ON ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Disabilita motori
//---------------------------------------------------------------------------------
void MotorheadClass::DisableMotors()
{
	_disableDriver( HEADX_ID );
	delay(1200);
	_disableDriver( HEADY_ID );
	delay(1200);
	Disable();
}

//---------------------------------------------------------------------------------
// Resetta allarmi driver motori
//---------------------------------------------------------------------------------
bool MotorheadClass::ResetAlarms()
{
	error = ERR_NOERROR;
	
	if( head[HEADX_ID]->ResetAlarms() )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
	}
	if( head[HEADY_ID]->ResetAlarms() )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
	}
	
	return error == ERR_NOERROR ? true : false;
}

//---------------------------------------------------------------------------------
// Rcarica parametri PIDs
//---------------------------------------------------------------------------------
void MotorheadClass::ReloadPIDParams()
{
	Read_HeadParams_File( "headX.txt", HEADX_ID );
	Read_HeadParams_File( "headY.txt", HEADY_ID );
	Read_PIDParams_File( "headPID.txt" );
	
	_setPIDs( HEADX_ID );
	_setPIDs( HEADY_ID );

	// setta jerk
	head[HEADX_ID]->SetMaxJerk( headParams[HEADX_ID].Jerk );
	head[HEADY_ID]->SetMaxJerk( headParams[HEADY_ID].Jerk );
	// setta Coulomb friction
	head[HEADX_ID]->SetCFriction( _IQ24( headParams[HEADX_ID].CFriction ) );
	head[HEADY_ID]->SetCFriction( _IQ24( headParams[HEADY_ID].CFriction ) );
	// setta speed feed-forward
	head[HEADX_ID]->SetSpdFFWD( _IQ24( headParams[HEADX_ID].SpeedFFWD ) );
	head[HEADY_ID]->SetSpdFFWD( _IQ24( headParams[HEADY_ID].SpeedFFWD ) );
	// setta acc feed-forward
	head[HEADX_ID]->SetAccFFWD( _IQ24( headParams[HEADX_ID].AccFFWD ) );
	head[HEADY_ID]->SetAccFFWD( _IQ24( headParams[HEADY_ID].AccFFWD ) );
}

//---------------------------------------------------------------------------------
// Ricarica parametri
//---------------------------------------------------------------------------------
void MotorheadClass::ReloadParameters()
{
	Read_HeadParams_File( "headX.txt", HEADX_ID );
	Read_HeadParams_File( "headY.txt", HEADY_ID );
	
	// Calibrated Angle
	if( head[HEADX_ID]->SetCalibratedAngle( headParams[HEADX_ID].CalAngle ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return;
	}
	if( head[HEADY_ID]->SetCalibratedAngle( headParams[HEADY_ID].CalAngle ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return;
	}

	// Target error
	if( head[HEADX_ID]->SetEndMovementDelta( _IQ24( headParams[HEADX_ID].TargetError * headParams[HEADX_ID].PulsesToMm / headParams[HEADX_ID].LineEnc1 ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return;
	}
	if( head[HEADY_ID]->SetEndMovementDelta( _IQ24( headParams[HEADY_ID].TargetError * headParams[HEADY_ID].PulsesToMm / headParams[HEADY_ID].LineEnc1 ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return;
	}

	if( head[HEADX_ID]->SetEndMovementSpeed( _IQ24( headParams[HEADX_ID].TargetSpeed ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return;
	}
	if( head[HEADY_ID]->SetEndMovementSpeed( _IQ24( headParams[HEADY_ID].TargetSpeed ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
		return;
	}

	// Start ticks
	if( head[HEADX_ID]->SetStartTicks( headParams[HEADX_ID].StartTicks ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return;
	}
	if( head[HEADY_ID]->SetStartTicks( headParams[HEADY_ID].StartTicks ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
		return;
	}
}

//---------------------------------------------------------------------------------
// Setta parametri per logger
//---------------------------------------------------------------------------------
bool MotorheadClass::SetLogParams( int module )
{
	if( !enabled )
		return true;

	// trigger
	_iq val = _IQ15( motorheadLogData.trigger * headParams[module].PulsesToMm / headParams[module].LineEnc1 + 0.5 );
	if( head[module]->SetLoggerTrigger( val ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	// trigger mode
	if( head[module]->LoggerTriggerMode( motorheadLogData.trigger_mode ) )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	// prescaler
	if( head[module]->SetLoggerPrescaler( motorheadLogData.prescaler ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	for( int ch = 0; ch < LOG_CH_NUM; ch++ )
	{
		if( motorheadLogData.channel_on[ch] )
		{
			if( head[module]->SetLoggerChannelInput( ch, motorheadLogData.channel_type[ch] ) == MOTOR_ERROR )
			{
				error |= ERR_DRIVER;
				errorId = module;
				return false;
			}
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Legge il buffer di log
//---------------------------------------------------------------------------------
bool MotorheadClass::ReadLog( int module, int channel )
{
	if( !enabled )
		return true;

	if( head[module]->GetLoggerChannelData( channel ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Ricerca origini
//---------------------------------------------------------------------------------
bool MotorheadClass::ZeroSearch()
{
	if( !enabled )
		return true;

	// setta velocita' iniziali
	if( !SetStartSpeeds( HEADX_ID ) )
	{
		return false;
	}

	if( !SetStartSpeeds( HEADY_ID ) )
	{
		return false;
	}

	if( head[HEADX_ID]->HomeSensorInput( AXIS_LIMIT_INPUT ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return false;
	}
	if( head[HEADY_ID]->HomeSensorInput( AXIS_LIMIT_INPUT ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
		return false;
	}

	// ricerca zero asse X
	if( head[HEADX_ID]->SearchPos0( ZEROSEARCH_POS ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return false;
	}
	if( !WaitOrigin( HEADX_ID ) )
	{
		return false;
	}
	if( head[HEADX_ID]->MotorEnable( MOTOR_ON ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return false;
	}
	// muove la testa verso destra di 10 mm per evitare contatto tra switch e bordo macchina
	if( head[HEADX_ID]->GotoPos0( _IQ24( 10.0f * headParams[HEADX_ID].PulsesToMm / headParams[HEADX_ID].LineEnc1 + 0.5 ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return false;
	}
	Wait();

	// ricerca zero asse Y
	if( head[HEADY_ID]->SearchPos0( ZEROSEARCH_POS ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
		return false;
	}
	if( !WaitOrigin( HEADY_ID ) )
	{
		return false;
	}
	if( head[HEADY_ID]->MotorEnable( MOTOR_ON ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
		return false;
	}

	return SetHome();
}

//---------------------------------------------------------------------------------
// Attende fine ricerca origini
//---------------------------------------------------------------------------------
bool MotorheadClass::WaitOrigin( int module )
{
	if( !enabled )
		return true;

	error = 0;

	int retry = 0;

	while( 1 )
	{
		int camStatus = head[module]->MotorStatus();
		if( camStatus == MOTOR_ERROR )
		{
			retry++;
			if( retry >= MOTOR_RETRY )
			{
				error |= ERR_DRIVER;
				errorId = module;
				return false;
			}
		}
		else
		{
			if( !(camStatus & MOTOR_ZERO) )
			{
				return true;
			}

			if( camStatus & MOTOR_PROCERROR )
			{
				error |= ERR_DRIVER;
				errorId = module;
				return false;
			}
		}

		delay( MOTOR_STATUS_DELAY );
	}

	return true;
}

//---------------------------------------------------------------------------------
// Abilita/disabilita limiti finecorsa
//---------------------------------------------------------------------------------
bool MotorheadClass::EnableLimits( bool state )
{
	if( !enabled )
		return true;

	if( head[HEADX_ID]->SetLimitsCheck( state ? LIMITCHECK_ON : LIMITCHECK_OFF, LIMITLEVEL_HIGH ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADX_ID;
		return false;
	}
	if( head[HEADY_ID]->SetLimitsCheck( state ? LIMITCHECK_ON : LIMITCHECK_OFF, LIMITLEVEL_HIGH ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = HEADY_ID;
		return false;
	}
	return true;
}

//---------------------------------------------------------------------------------
// Setta velocita' iniziali (minime)
//---------------------------------------------------------------------------------
bool MotorheadClass::SetStartSpeeds( int module )
{
	if( head[module]->MinimumFreq( _IQ24(MINIMUM_FREQ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	if( head[module]->MaximumFreq( _IQ24(MINIMUM_FREQ) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}
	if( head[module]->Aceleration( _IQ24(MINIMUM_ACC) ) == MOTOR_ERROR )
	{
		error |= ERR_DRIVER;
		errorId = module;
		return false;
	}

	return true;
}


//GF_TEMP_ENC_TEST - START
float MotorheadClass::GetEncoder( int module )
{
	int val = head[module]->GetEncoderActualPosition();
	return _IQ24ToF( val );
}

float MotorheadClass::GetEncoder2( int module )
{
	int val = head[module]->GetEncoder2ActualPosition();
	return _IQ24ToF( val );
}

float MotorheadClass::GetEncoderInterp( int module )
{
	int val = head[module]->GetEncoderInterpActualPosition();
	return _IQ24ToF( val );
}

int MotorheadClass::GetEncoderPulses( int module )
{
	int val = head[module]->GetEncoderActualPulses();
	return val;// / 2048.f;
}

int MotorheadClass::GetEncoder2Pulses( int module )
{
	int val = head[module]->GetEncoder2ActualPulses();
	return val;// / 100.f;
}


float MotorheadClass::GetSpeedBoard()
{
	int val = head[HEADY_ID]->ActualVelocity();
	return _IQ24ToF( val );
}

float MotorheadClass::GetAccBoard()
{
	int val = head[HEADY_ID]->ActualAceleration();
	return _IQ24ToF( val );
}

float MotorheadClass::GetDecBoard()
{
	int val = head[HEADY_ID]->ActualDeceleration();
	return _IQ24ToF( val );
}
//GF_TEMP_ENC_TEST - END


//GF_TEMP
#include "filefn.h"
#include "q_gener.h"
#include "strutils.h"

void Read_HeadParams_File( const char* filename, int module )
{
	FILE* fparams = FilesFunc_fopen( filename, "r" );

	if( fparams == 0 )
		return;

	memset( &headParams[module], 0, sizeof(MotorParamsStruct_100) );

	while( !feof( fparams ) )
	{
		char buf[80];
		fgets( buf, 80, fparams );
		strupr( buf );

		char *p = strchr( buf, '\n' );
		if( p != NULL )
		{
			*p = '\0';
		}

		p = strchr( buf, '\r' );
		if( p != NULL )
		{
			*p = '\0';
		}

		DelSpcR( buf );

		p = strstr( buf, "BOARDMAXCURRENT" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].BoardMaxCurrent = v;
			continue;
		}

		p = strstr( buf, "MAXCURRENT" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].MaxCurrent = v;
			continue;
		}
		
		p = strstr( buf, "POLES" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].Poles = v;
			continue;
		}
		
		p = strstr( buf, "PPR" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].PPR = v;
			continue;
		}
		
		p = strstr( buf, "LINEENC1" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].LineEnc1 = v;
			continue;
		}
		
		p = strstr( buf, "QEPENCMODE" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].QepEncMode = v;
			continue;
		}
		
		p = strstr( buf, "SERENCMODE" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].SerEncMode = v;
			continue;
		}
		
		p = strstr( buf, "LINEENC2" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].LineEnc2 = v;
			continue;
		}
		
		p = strstr( buf, "PULSESTOMM" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].PulsesToMm = v;
			continue;
		}
		
		p = strstr( buf, "TARGETERROR" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].TargetError = v;
			continue;
		}
		
		p = strstr( buf, "TARGETSPEED" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].TargetSpeed = v;
			continue;
		}

		p = strstr( buf, "PROXIMITYGAP" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].ProximityGap = v;
			continue;
		}
		
		p = strstr( buf, "PIDPOSACTIVATIONGAP" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].PIDPosActivationGap = v;
			continue;
		}
		
		p = strstr( buf, "FOLLOWINGERROR" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].FollowingError = v;
			continue;
		}

		p = strstr( buf, "SPEEDFILTER" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].SpeedFilter = v;
			continue;
		}

		p = strstr( buf, "STARTTICKS" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].StartTicks = v;
			continue;
		}

		p = strstr( buf, "SEARCHSPEEDMAX" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].SearchSpeedMax = v;
			continue;
		}
		
		p = strstr( buf, "SEARCHSPEEDMIN" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].SearchSpeedMin = v;
			continue;
		}
		
		p = strstr( buf, "JERK" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].Jerk = v;
			continue;
		}
		
		p = strstr( buf, "CFRICTION" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].CFriction = v;
			continue;
		}
		
		p = strstr( buf, "SPEEDFFWD" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].SpeedFFWD = v;
			continue;
		}
		
		p = strstr( buf, "ACCFFWD" );
		if( p != NULL )
		{
			float v = atof( buf );
			headParams[module].AccFFWD = v;
			continue;
		}
		
		p = strstr( buf, "CALANGLE" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].CalAngle = v;
			continue;
		}
		
		p = strstr( buf, "PID_SPDDEF" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].PID_SpdDef = v;
			continue;
		}
		
		p = strstr( buf, "PID_SPD0" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].PID_Spd0 = v;
			continue;
		}
		
		p = strstr( buf, "PID_SPD" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].PID_Spd = v;
			continue;
		}
		
		p = strstr( buf, "PID_POSDEF" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].PID_PosDef = v;
			continue;
		}
		
		p = strstr( buf, "PID_POS0" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].PID_Pos0 = v;
			continue;
		}
		
		p = strstr( buf, "PID_ID" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].PID_Id = v;
			continue;
		}
		
		p = strstr( buf, "PID_IQ" );
		if( p != NULL )
		{
			int v = atoi( buf );
			headParams[module].PID_Iq = v;
			continue;
		}
	}

	FilesFunc_fclose( fparams );
}


void Read_PIDParams_File( const char* filename )
{
	FILE* fparams = FilesFunc_fopen( filename, "r" );

	if( fparams == 0 )
		return;

	int index = -1;
	while( !feof( fparams ) )
	{
		char buf[80];
		fgets( buf, 80, fparams );
		strupr( buf );

		char *p = strchr( buf, '\n' );
		if( p != NULL )
		{
			*p = '\0';
		}

		p = strchr( buf, '\r' );
		if( p != NULL )
		{
			*p = '\0';
		}

		DelSpcR( buf );

		p = NULL;
		for( int i = 0; i < NUM_PID && p == NULL; i++ )
		{
			switch( i )
			{
				case 0:
					p = strstr( buf, "PID_SPDDEF" );
					break;
				case 1:
					p = strstr( buf, "PID_SPD0" );
					break;
				case 2:
					p = strstr( buf, "PID_SPD" );
					break;
				case 3:
					p = strstr( buf, "PID_POSDEF" );
					break;
				case 4:
					p = strstr( buf, "PID_POS0" );
					break;
				case 5:
					p = strstr( buf, "PID_ID" );
					break;
				case 6:
					p = strstr( buf, "PID_IQ" );
					break;
			}
		}

		if( p != NULL )
		{
			index++;
			continue;
		}

		if( index < 0 || index >= MOTOR_NUM*NUM_PID )
			continue;

		//-------------//
		// Read Values //
		//-------------//
		
		p = strstr( buf, "KP" );
		if( p != NULL )
		{
			float v = atof( buf );
			PIDParams[index].KP = v;
			continue;
		}
		
		p = strstr( buf, "KI" );
		if( p != NULL )
		{
			float v = atof( buf );
			PIDParams[index].KI = v;
			continue;
		}
		
		p = strstr( buf, "KD" );
		if( p != NULL )
		{
			float v = atof( buf );
			PIDParams[index].KD = v;
			continue;
		}
		
		p = strstr( buf, "KC" );
		if( p != NULL )
		{
			float v = atof( buf );
			PIDParams[index].KC = v;
			continue;
		}
		
		p = strstr( buf, "OMAX" );
		if( p != NULL )
		{
			float v = atof( buf );
			PIDParams[index].OMax = v;
			continue;
		}
		
		p = strstr( buf, "OMIN" );
		if( p != NULL )
		{
			float v = atof( buf );
			PIDParams[index].OMin = v;
			continue;
		}
	}
	
	FilesFunc_fclose( fparams );
}


#include "q_help.h"

void Motorhead_ShowStatus()
{
	if( Motorhead == 0 )
		return;

	int statusX = 0;
	int statusY = 0;
	Motorhead->GetStatus( HEADX_ID, statusX );
	Motorhead->GetStatus( HEADY_ID, statusY );

	char buf[64];
	char buf_out[2048];
	buf_out[0] = '\0';

	strcat( buf_out, "    Flag       X  Y\n" );
	strcat( buf_out, "--------------------\n" );
	snprintf( buf, sizeof(buf), " Running     : %d  %d\n", statusX & MOTOR_RUNNING ? 1 : 0, statusY & MOTOR_RUNNING ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Overcurrent : %d  %d\n", statusX & MOTOR_OVERCURRENT ? 1 : 0, statusY & MOTOR_OVERCURRENT ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Proc. Error : %d  %d\n", statusX & MOTOR_PROCERROR ? 1 : 0, statusY & MOTOR_PROCERROR ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Timeout     : %d  %d\n", statusX & MOTOR_TIMEOUT ? 1 : 0, statusY & MOTOR_TIMEOUT ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Zero        : %d  %d\n", statusX & MOTOR_ZERO ? 1 : 0, statusY & MOTOR_ZERO ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Overrun     : %d  %d\n", statusX & MOTOR_OVERRUN ? 1 : 0, statusY & MOTOR_OVERRUN ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Danger      : %d  %d\n", statusX & MOTOR_DANGER ? 1 : 0, statusY & MOTOR_DANGER ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Overspeed   : %d  %d\n", statusX & MOTOR_OVERSPEED ? 1 : 0, statusY & MOTOR_OVERSPEED ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " No Encoder  : %d  %d\n", statusX & MOTOR_NOENC ? 1 : 0, statusY & MOTOR_NOENC ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " PID Spd     : %d  %d\n", statusX & MOTOR_PIDSPDON ? 1 : 0, statusY & MOTOR_PIDSPDON ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " PID Pos     : %d  %d\n", statusX & MOTOR_PIDPOSON ? 1 : 0, statusY & MOTOR_PIDPOSON ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Near Target : %d  %d\n", statusX & MOTOR_NEARTARGET ? 1 : 0, statusY & MOTOR_NEARTARGET ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Multi Move  : %d  %d\n", statusX & MOTOR_MULTIMOVE ? 1 : 0, statusY & MOTOR_MULTIMOVE ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " No Follow   : %d  %d\n", statusX & MOTOR_NO_FOLLOW ? 1 : 0, statusY & MOTOR_NO_FOLLOW ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Security    : %d  %d\n", statusX & MOTOR_SECURITY ? 1 : 0, statusY & MOTOR_SECURITY ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Steady Pos. : %d  %d\n", statusX & MOTOR_STEADYPOS ? 1 : 0, statusY & MOTOR_STEADYPOS ? 1 : 0 );
	strcat( buf_out, buf );


	int encStatusX = 0;
	int encStatusY = 0;
	Motorhead->GetEncoderStatus( HEADX_ID, encStatusX );
	Motorhead->GetEncoderStatus( HEADY_ID, encStatusY );

	strcat( buf_out, "\n" );
	strcat( buf_out, "--- No Encoder FLAGS ---\n" );
	snprintf( buf, sizeof(buf), " Mag Field   : %d  %d\n", encStatusX & ENC_MF_ERROR ? 1 : 0, encStatusY & ENC_MF_ERROR ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Overrun     : %d  %d\n", encStatusX & ENC_OVERRUN_ERROR ? 1 : 0, encStatusY & ENC_OVERRUN_ERROR ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Frame       : %d  %d\n", encStatusX & ENC_FRAME_ERROR ? 1 : 0, encStatusY & ENC_FRAME_ERROR ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " No Recept.  : %d  %d\n", encStatusX & ENC_NORECEPTIONS ? 1 : 0, encStatusY & ENC_NORECEPTIONS ? 1 : 0 );
	strcat( buf_out, buf );


	W_Mess( buf_out, MSGBOX_YUPPER );
}

void Motorhead_ShowPIDStatus()
{
	if( Motorhead == 0 )
		return;

	int statusX = 0;
	int statusY = 0;
	Motorhead->GetPIDStatus( HEADX_ID, statusX );
	Motorhead->GetPIDStatus( HEADY_ID, statusY );

	char buf[64];
	char buf_out[1024];
	buf_out[0] = '\0';

	snprintf( buf, sizeof(buf), "    PIDs       X  Y\n" );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), "--------------------\n" );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Spd Def     : %d  %d\n", statusX & PID_SPDDEF ? 1 : 0, statusY & PID_SPDDEF ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Spd 0       : %d  %d\n", statusX & PID_SPD0 ? 1 : 0, statusY & PID_SPD0 ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Spd Spd     : %d  %d\n", statusX & PID_SPDSPD ? 1 : 0, statusY & PID_SPDSPD ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Pos Def     : %d  %d\n", statusX & PID_POSDEF ? 1 : 0, statusY & PID_POSDEF ? 1 : 0 );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " Pos 0       : %d  %d\n", statusX & PID_POS0 ? 1 : 0, statusY & PID_POS0 ? 1 : 0 );
	strcat( buf_out, buf );

	W_Mess( buf_out, MSGBOX_YUPPER );
}

void Motorhead_ReadPIDs()
{
	char buf[64];
	char buf_out[1024];
	buf_out[0] = '\0';

	Motorhead->GetPIDs( HEADX_ID );
	Motorhead->GetPIDs( HEADY_ID );

	for( int i = 0; i < MOTOR_NUM*NUM_PID; i++ )
	{
		buf_out[0] = '\0';
		
		snprintf( buf, sizeof(buf), " PID #%d\n", i );
		strcat( buf_out, buf );
		snprintf( buf, sizeof(buf), "--------------------\n" );
		strcat( buf_out, buf );
		snprintf( buf, sizeof(buf), " KP   : %.3f\n", PIDParams[i].KP );
		strcat( buf_out, buf );
		snprintf( buf, sizeof(buf), " KI   : %.3f\n", PIDParams[i].KI );
		strcat( buf_out, buf );
		snprintf( buf, sizeof(buf), " KD   : %.3f\n", PIDParams[i].KD );
		strcat( buf_out, buf );
		snprintf( buf, sizeof(buf), " KC   : %.3f\n", PIDParams[i].KC );
		strcat( buf_out, buf );
		snprintf( buf, sizeof(buf), " OMax : %.3f\n", PIDParams[i].OMax );
		strcat( buf_out, buf );
		snprintf( buf, sizeof(buf), " OMin : %.3f\n", PIDParams[i].OMin );
		strcat( buf_out, buf );
		
		W_Mess( buf_out, MSGBOX_YUPPER );
	}
}


//GF_TEMP - poi togliere
void Motorhead_ActualPosition()
{
	float px, py;
	Motorhead->ReadRefPosition( px, py );
	print_debug( "Pos: %.6f, %.6f\n", px, py );
}


//GF_TEMP_ENC_TEST - START
void Motorhead_ActualEncoder()
{
	char buf[128];
	char buf_out[1024];
	buf_out[0] = '\0';

	float p1X = Motorhead->GetEncoder( HEADX_ID );
	int p1Xcount = Motorhead->GetEncoderPulses( HEADX_ID );

	float p1Y = Motorhead->GetEncoder( HEADY_ID );
	int p1Ycount = Motorhead->GetEncoderPulses( HEADY_ID );

	float x = (p1X - 0.5) * headParams[HEADX_ID].LineEnc1 / headParams[HEADX_ID].PulsesToMm;
	float y = (p1Y - 0.5) * headParams[HEADY_ID].LineEnc1 / headParams[HEADY_ID].PulsesToMm;

	snprintf( buf, sizeof(buf), " [Encoder X]   Val: %.2f - p.u.: %.6f - Counter: %d\n", x, p1X, p1Xcount );
	strcat( buf_out, buf );
	snprintf( buf, sizeof(buf), " [Encoder Y]   Val: %.2f - p.u.: %.6f - Counter: %d\n", y, p1Y, p1Ycount );
	strcat( buf_out, buf );

	W_Mess( buf_out, MSGBOX_YUPPER );
}

void Motorhead_ActualSpeedAcc()
{
	float s = Motorhead->GetSpeedBoard();
	float a = Motorhead->GetAccBoard();
	float d = Motorhead->GetDecBoard();
	print_debug( "Speed: %.2f  Acc: %.2f  Dec: %.2f\n", s, a, d );
}

float Motorhead_GetEncoderDiff()
{
	float p1mm = Motorhead->GetEncoderPulses( HEADY_ID );
	float p2mm = Motorhead->GetEncoder2Pulses( HEADY_ID );
	return p1mm - p2mm;
}

void Motorhead_GetEncoderMM( float& x, float& y )
{
	x = (Motorhead->GetEncoder( HEADX_ID ) - 0.5) * headParams[HEADX_ID].LineEnc1 / headParams[HEADX_ID].PulsesToMm;
	y = (Motorhead->GetEncoder( HEADY_ID ) - 0.5) * headParams[HEADY_ID].LineEnc1 / headParams[HEADY_ID].PulsesToMm;
}



//TODO: spostare in altro file
#include "q_graph.h"
#include "keyutils.h"

#define LOG_SPD_FACTOR		2

#define RGND                3000
#define RVPWR               68000

void Motorhead_ShowLog( int module )
{
	float min_y[LOG_CH_NUM];
	float max_y[LOG_CH_NUM];

	for( int ch = 0; ch < LOG_CH_NUM; ch++ )
	{
		min_y[ch] = max_y[ch] = 0.f;
	}

	int cam = pauseLiveVideo();

	for( int ch = 0; ch < LOG_CH_NUM; ch++ )
	{
		if( motorheadLogData.channel_on[ch] )
		{
			if( !Motorhead->ReadLog( module, ch ) )
			{
				W_Mess( "Errore lettura dati log! " );
				playLiveVideo( cam );
				return;
			}

			for( int i = 0; i < LOG_BUF_SIZE; i++ )
			{
				motorheadLogData.channel_data[ch][i] = _IQ15ToF( head[module]->bufDataLog[ch][i] );

				// change data scale
				if( motorheadLogData.channel_type[ch] == LOGGER_SPEED ||
					motorheadLogData.channel_type[ch] == LOGGER_SPEEDREF ||
					motorheadLogData.channel_type[ch] == LOGGER_SPEEDREFNOPOS )
				{
					motorheadLogData.channel_data[ch][i] *= LOG_SPD_FACTOR;
				}
				else if( motorheadLogData.channel_type[ch] == LOGGER_MECHTHETA )
				{
					motorheadLogData.channel_data[ch][i] = (motorheadLogData.channel_data[ch][i] - 0.5) * headParams[module].LineEnc1 / headParams[module].PulsesToMm;
				}
				else if( motorheadLogData.channel_type[ch] == LOGGER_VPWR )
				{
					motorheadLogData.channel_data[ch][i] = ((motorheadLogData.channel_data[ch][i] * 3.3) / RGND) * (RGND + RVPWR);
				}

				//
				if( i == 0 )
				{
					min_y[ch] = max_y[ch] = motorheadLogData.channel_data[ch][i];
				}
				else
				{
					if( motorheadLogData.channel_data[ch][i] > max_y[ch] )
					{
						//TODO: mettere linea verticale
						if( fabs(motorheadLogData.channel_data[ch][i] - max_y[ch]) < 100.f )
							max_y[ch] = motorheadLogData.channel_data[ch][i];
					}
					else if( motorheadLogData.channel_data[ch][i] < min_y[ch] )
					{
						min_y[ch] = motorheadLogData.channel_data[ch][i];
					}
				}
			}
		}
	}

	// asse X: tempi in ms
	float dataX[LOG_BUF_SIZE];
	for( int i = 0; i < LOG_BUF_SIZE; i++ )
	{
		dataX[i] = i * 0.1f * motorheadLogData.prescaler;
	}

	char title[20];
	snprintf( title, sizeof(title), "Motorhead %c", module == HEADX_ID ? 'X' : 'Y' );
	C_Graph* graph = new C_Graph( 1, 1, 91, 30, title, GRAPH_NUMTYPEY_FLOAT | GRAPH_NUMTYPEX_FLOAT | GRAPH_AXISTYPE_XY | GRAPH_DRAWTYPE_LINE | GRAPH_SHOWPOSTXT, LOG_CH_NUM );

	//TODO: per il momento sono tutti scalati in base al primo grafico (il secondo dei 4 disponibili)
	graph->SetVMinY( min_y[1] );
	graph->SetVMaxY( max_y[1] );
	graph->SetVMinX( dataX[0] );
	graph->SetVMaxX( dataX[LOG_BUF_SIZE-1] );

	// set data
	for( int ch = 0; ch < LOG_CH_NUM; ch++ )
	{
		if( motorheadLogData.channel_on[ch] )
		{
			if( ch != 1 )
			{
				float dRef = max_y[1] - min_y[1];
				float dCur = max_y[ch] - min_y[ch];

				float m = dRef / dCur;
				float q = min_y[1] - min_y[ch] * m;

				for( int i = 0; i < LOG_BUF_SIZE; i++ )
				{
					motorheadLogData.channel_data[ch][i] = motorheadLogData.channel_data[ch][i] * m + q;
				}
			}

			graph->SetNData( LOG_BUF_SIZE, ch );
			graph->SetDataY( motorheadLogData.channel_data[ch], ch );
			graph->SetDataX( dataX, ch );
			graph->SetGraphScale( dataX[0], dataX[LOG_BUF_SIZE-1], min_y[ch], max_y[ch], ch );

			if( ch == 1 )
				graph->SetGraphColor( GUI_color( GR_YELLOW ), ch );
			else if( ch == 2 )
				graph->SetGraphColor( GUI_color( GR_CYAN ), ch );
			else if( ch == 3 )
				graph->SetGraphColor( GUI_color( GR_MAGENTA ), ch );
		}
	}

	graph->Show();

	//TODO: stampare nomi grafici

	do
	{
		int c = Handle();
		if(c != K_ESC)
		{
			graph->GestKey( c );
		}
		else
		{
			break;
		}
	} while( 1 );

	delete graph;

	playLiveVideo( cam );
}

//GF_TEMP_ENC_TEST - END



//---------------------------------------------------------------------------------
// Aggiorna firmware modulo Motorhead
// Parametri di ingresso:
//    filename: nome del file contenente il firmware
// Ritorna 0 se errore, 1 altrimenti
//---------------------------------------------------------------------------------
#define RETRYTIMES              3
#define MAX_FAKE_CHARS          5
#define TX_OK                   0x11
#define ERROR_Crc               0x22
#define ERROR_Line              0x33
#define ERROR_Signature         0x44
#define ERROR_OverFlow          0x55
#define ERROR_TopModule         0x66

int Motorhead_BootLoader( int motorID, const char* filename )
{
	unsigned int inByte, outByte;
	int i;
	int retCode = 1;

	BrushlessModule* driver = head[motorID];
	CommClass* com = ComPortMotorhead;

	// ignore any received bytes (flush input buffer)
	com->flush();

	// open selected file
	FILE* firmware = fopen( filename , "rb" );

	if( firmware == NULL )
	{
		return -1;
	}

	// reset drive
	driver->ResetDrive();

	// set 10secs timeout
	com->settimeout ( 10000000 );

	// looking for 'a' character
	for ( i = 0; i < MAX_FAKE_CHARS; i++ )
	{
		inByte = com->getbyte();
		// check for timeout
		if( inByte == SERIAL_ERROR_TIMEOUT )
		{
			retCode = -4;
			break;
		}

		if( inByte == 'a' )
		{
			outByte = 'b';
			com->putbyte( outByte );

			// read echo
			inByte = com->getbyte();
			// check for timeout
			if( inByte == SERIAL_ERROR_TIMEOUT )
			{
				retCode = -4;
				break;
			}
			break;
		}
	}

	if( i == MAX_FAKE_CHARS )
	{
		retCode = -3;
	}

	if( retCode != 1 )
	{
		fclose( firmware );
		return retCode;
	}

	// looking for 'b' character
	inByte = com->getbyte();
	// check for timeout
	if( inByte == SERIAL_ERROR_TIMEOUT )
	{
		fclose( firmware );
		return -4;
	}

	if( inByte != 'b' )
	{
		fclose( firmware );
		return -5;
	}

	// data transfer
	printf( "Transfering data...\n" );

	// get the file size
	fseek( firmware, 0, SEEK_END );
	int firmwareLenght = ftell( firmware );
	rewind( firmware );

	// non si considerano gli ultimi byte (carattere terminatore + spazi + CR + LF)
	firmwareLenght -= 4;
	int byteIndex = 0;
	unsigned char buffer[2];

	// start char
	fread( &outByte, sizeof( char ), 1, firmware );
	byteIndex++;
	buffer[0] = outByte;

	com->putbyte( outByte );

	// read echo
	inByte = com->getbyte();
	// check for timeout
	if( inByte == SERIAL_ERROR_TIMEOUT )
	{
		fclose( firmware );
		return -4;
	}

	// looking for target response
	inByte = com->getbyte();
	// check for timeout
	if( inByte == SERIAL_ERROR_TIMEOUT )
	{
		fclose( firmware );
		return -4;
	}

	if( outByte != inByte )
	{
		fclose( firmware );
		return -5;
	}


	while( !feof( firmware ) && byteIndex <= firmwareLenght )
	{
		// first byte
		fread( &outByte, sizeof( char ), 1, firmware );
		byteIndex++;
		if( outByte == 32 || outByte == 13 || outByte == 10 )
			continue;
		buffer[0] = Hex2Dec( outByte ) * 16;

		// second byte
		fread( &outByte, sizeof( char ), 1, firmware );
		byteIndex++;
		if( outByte == 32 || outByte == 13 || outByte == 10 )
			continue;
		buffer[0] += Hex2Dec( outByte );

		// update progress
		if( byteIndex % 250 == 0 )
		{
			printf( "." );
			fflush( stdout );
		}

		// send byte
		com->putbyte( buffer[0] );

		// read echo
		inByte = com->getbyte();
		// check for timeout
		if( inByte == SERIAL_ERROR_TIMEOUT )
		{
			retCode = -4;
			break;
		}

		// looking for target response
		inByte = com->getbyte();
		// check for timeout
		if( inByte == SERIAL_ERROR_TIMEOUT )
		{
			retCode = -4;
			break;
		}

		if( buffer[0] != inByte )
		{
			retCode = -5;
			break;
		}
	}

	fclose( firmware );
	return retCode;
}


//---------------------------------------------------------------------------------
// Interfaccia testuale aggiornamento firmware Sniper
//---------------------------------------------------------------------------------
int UpdateMotorhead()
{
	printf( "\n\tVersion %s\n", SOFT_VER );
	printf( "\tBuild date %s\n\n", __DATE__ );

	// Apertura porta seriale per comunicazione con moduli Brushless
	ComPortMotorhead = new CommClass( getMotorheadComPort(), MOTORHEAD_BAUD );

	head[HEADX_ID] = new BrushlessModule( ComPortMotorhead, HEAD_X_ADDRESS );
	head[HEADY_ID] = new BrushlessModule( ComPortMotorhead, HEAD_Y_ADDRESS );

	// read version
	unsigned short version;
	unsigned char ver[2];
	unsigned char rev[2];

	version = head[HEADX_ID]->GetVersion();
	ver[HEADX_ID] = version >> 8;
	rev[HEADX_ID] = version;

	if( ver[HEADX_ID] != 255 && rev[HEADX_ID] != 255 )
	{
		printf( " Motorhead X   v. %d.%d\n", ver[HEADX_ID], rev[HEADX_ID] );
	}

	version = head[HEADY_ID]->GetVersion();
	ver[HEADY_ID] = version >> 8;
	rev[HEADY_ID] = version;

	if( ver[HEADY_ID] != 255 && rev[HEADY_ID] != 255 )
	{
		printf( " Motorhead Y   v. %d.%d\n", ver[HEADY_ID], rev[HEADY_ID] );
	}

	int exit_flag = 0;
	while( !exit_flag )
	{
		delay(1000);
		int ret = 0;

		printf("\n");
		printf(" 1 - Update firmware - Motorhead X\n");
		printf(" 2 - Update firmware - Motorhead Y\n");
		printf(" q (Q) - Quit\n");
		printf("-> ");

		int c = getchar();

		// flush
		int c_dummy = c;
		while( c_dummy != 10 )
			c_dummy = getchar();

		if( c == 'q' || c == 'Q' )
			exit_flag = 1;
		else if( c >= '1' && c <= '4' )
		{
			// confirm
			printf("\n");
			printf("\n You choose to ");
			if( c == '1' )
				printf("update firmware - Motorhead X\n");
			else if( c == '2' )
				printf("update firmware - Motorhead Y\n");
			printf(" c (C) - Continue\n");
			printf(" other key - Abort\n");
			printf("-> ");

			int cc = getchar();

			// flush
			int c_dummy = cc;
			while( c_dummy != 10 )
				c_dummy = getchar();

			if( cc == 'c' || cc == 'C' )
			{
				if( c == '1' )
				{
					printf("\n\n\t MOTORHEAD - BOOTLOADER\n\n");
					printf("Wait...\n" );

					head[HEADY_ID]->SuspendDrive();
					ret = Motorhead_BootLoader( HEADX_ID, "motorx.tix" );
				}
				else if( c == '2' )
				{
					printf("\n\n\t MOTORHEAD - BOOTLOADER\n\n");
					printf("Wait...\n" );

					head[HEADX_ID]->SuspendDrive();
					ret = Motorhead_BootLoader( HEADY_ID, "motory.tix" );
				}

				if( c == '1' || c == '2' )
				{
					switch( ret )
					{
					case -1:
						printf("\nERROR - Firmware file (*.tix) doesn't exist !!!\n" );
						break;
					case -2:
						printf("\nERROR - Target device not found !!!\n");
						break;
					case -3:
						printf("\nERROR - MAX_FAKE_CHARS reached !!!\n");
						break;
					case -4:
						printf("\nERROR - Timeout occurred !!!\n");
						break;
					case -5:
						printf("\nERROR - Unexpected character !!!\n");
						break;
					case -6:
						printf("\nERROR - File damaged (Bad CRC) !!!\n");
						break;
					case -7:
						printf("\nERROR - Check connection !!!\n");
						break;
					case -8:
						printf("\nERROR - Invalid file !!!\n");
						break;
					case -9:
						printf("\nERROR - Communication out of sync !!!\n");
						break;
					case 1:
						printf("\nTransfering successfully completed !\n");
						break;
					default:
						break;
					}

					if( ret == 1 && c == '1' )
					{
						delay(1000);
						version = head[HEADX_ID]->GetVersion();
						ver[HEADX_ID] = version >> 8;
						rev[HEADX_ID] = version;

						if( ver[HEADX_ID] != 255 && rev[HEADX_ID] != 255 )
						{
							printf( "\n\n Motorhead X   v. %d.%d\n", ver[HEADX_ID], rev[HEADX_ID] );
						}
					}

					if( ret == 1 && c == '2' )
					{
						delay(1000);
						version = head[HEADY_ID]->GetVersion();
						ver[HEADY_ID] = version >> 8;
						rev[HEADY_ID] = version;

						if( ver[HEADY_ID] != 255 && rev[HEADY_ID] != 255 )
						{
							printf( "\n\n Motorhead Y   v. %d.%d\n", ver[HEADY_ID], rev[HEADY_ID] );
						}
					}
				}
			}
		}
	}

	delete head[HEADX_ID];
	delete head[HEADY_ID];
	delete ComPortMotorhead;

	return 1;
}

#endif //__USE_MOTORHEAD
