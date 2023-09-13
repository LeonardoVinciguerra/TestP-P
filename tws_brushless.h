//---------------------------------------------------------------------------
//
// File:     BrushlessModule.h
// Info:     Definitions for the Brushless control module BrushlessModule.cpp
// Created:  Daniele Belloni
//
//---------------------------------------------------------------------------

#ifndef __BrushlessMODULE_H
#define __BrushlessMODULE_H

#include "tws_motor.h"

// Tipo encoder (quadratura o seriale)
#define ENCODER_QEP			0
#define ENCODER_SER			1

// Modalita' di funzionamento dell'encoder (normale o invertito)
#define ENCODER_NORMAL		0
#define ENCODER_INVERTED	1

// Modalita' di generazione angolo elettrico con encoder
#define ELECTTHETA_ON		0
#define ELECTTHETA_OFF		1

// Logger channel IDs
enum eLogChannels
{
	LOG_CH1 = 0,
	LOG_CH2,
	LOG_CH3,
	LOG_CH4,
	LOG_CH_NUM
};

#define LOG_BUF_SIZE		100

// Livello del trigger del logger
#define LOWTOHIGH_TRIG 		0
#define HIGHTOLOW_TRIG 		1

// Logger channel IDs
#define LOGGER_CH1			0
#define LOGGER_CH2			1
#define LOGGER_CH3			2
#define LOGGER_CH4			3

// Ingressi canali logger
#define LOGGER_MECHTHETA		0
#define LOGGER_SPEED			1
#define LOGGER_CLARKA			2
#define LOGGER_CLARKB			3
#define LOGGER_PIDIDOUT			4
#define LOGGER_PIDIQOUT			5
#define LOGGER_PIDIDFDB			6
#define LOGGER_PIDIQFDB			7
#define LOGGER_PIDIDREF			8
#define LOGGER_PIDIQREF			9
#define LOGGER_SPEEDREF			10
#define LOGGER_POSREF			11
#define LOGGER_POSERR			12
#define LOGGER_PIDIDERR			13
#define LOGGER_PIDIQERR			14
#define LOGGER_SPEEDERR			15
#define LOGGER_SPEEDREFNOPOS	16
#define LOGGER_ELECERROR		17
#define LOGGER_ELECRAMP			18
#define LOGGER_VPWR				19

// Indici fasi motore
#define PHASE1_INDEX		0
#define PHASE2_INDEX		1
#define PHASE3_INDEX		2

// Attivazione e disattivazione dei PWM delle fasi del motore
#define PWM_OFF				0
#define PWM_ON				1

// Attivazione e disattivazione della modalita' calibrazione ADC
#define ADCCAL_OFF			0
#define ADCCAL_ON			1

// Parametri usati durante la calibrazione dello zero elettrico
#define CAL_ANGLE_CURRENT	0.75
#define CAL_PHASE_ROT_DELAY	2000
#define CAL_ANGLE_DELAY		2000
#define CAL_PARKANGLE		0.1
#define CAL_ERROR			40

// Canali ADC
#define ADC_CHANNEL0		0
#define ADC_CHANNEL1		1

// Tipo encoder
#define ROTATIVE_ENC		0
#define LINEAR_ENC			1

// Bits di modalita' del modulo (relativi alla variabile work_vars dedicata...)
#define MODE_LIMITS         0
#define MODE_LIMITSLEVEL    1
#define MODE_RAMP           2 //n.u.
#define MODE_SECURITY       3
#define MODE_SECURITYLEVEL  4

// Attivazione/disattivazione rampe accelerazione/decelerazione
#define RAMP_ON  			0
#define RAMP_OFF  			1

// Lettura velocita'
#define SPEED_PU			0
#define SPEED_RPM			1
#define SPEED_MS			2

// Set di parametri del PID di velocita'
#define PIDSET_DEF			0
#define PIDSET_0			1
#define PIDSET_SPD			2

// Attivazione/disattivazione compensazione decelerazione
#define DECCOMP_ON  		0
#define DECCOMP_OFF  		1

// Attivazione/disattivazione limiti di pericolosita'
#define DANGERLIMIT_ON  	0
#define DANGERLIMIT_OFF  	1


class BrushlessModule : public MotorModule
{
public:
	short bufDataLog[LOG_CH_NUM][LOG_BUF_SIZE];

	BrushlessModule( void* comInt, int address );
	~BrushlessModule();

	virtual unsigned short GetVersion();
	virtual int  SetNominalCurrent( int val );
	virtual int  GetNominalCurrent();
	virtual int  StopRotation( int ramp );
	virtual int ActualPosition();
	virtual int  Home();
	virtual int  GotoPos0( int pos );
	virtual int  MinimumFreq( int mFreq );
	virtual int  MaximumFreq( int MFreq );
	virtual int  Aceleration( int acc );
	virtual int  Deceleration( int dec );
	virtual int ActualVelocity();
	virtual int  MotorStatus();
	virtual int  ActualInputs();
	virtual int  SetLimitsCheck( int limit, int limitLevel );
	virtual int  ResetAlarms();
	virtual int  MotorEnable( int command );
	virtual int  ResetDrive();
	virtual int GetEncoderActualPosition();
	virtual int  EncoderMode( int type, int mode );
	virtual int  GetEncoderMode( int type );
	virtual int  InputsSetting( int val );
	virtual int  SearchPos0( int dir );
	virtual int  SuspendDrive();
	virtual int  SetPIDIqParams( int KP, int KI, int KD, int KC, int OMax, int OMin );
	virtual int  GetPIDIqParams( int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin );
	virtual int  SetPIDIdParams( int KP, int KI, int KD, int KC, int OMax, int OMin );
	virtual int  GetPIDIdParams( int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin );
	virtual int  SetPIDSpdParams( unsigned short set, int KP, int KI, int KD, int KC, int OMax, int OMin );
	virtual int  GetPIDSpdParams( unsigned short set, int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin );
	virtual int  SetPIDPosParams( unsigned short set, int KP, int KI, int KD, int KC, int OMax, int OMin );
	virtual int  GetPIDPosParams( unsigned short set, int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin );
	virtual int GetLineEncoder();
	virtual int  SetLineEncoder( int lines );
	virtual int GetPulsesPerRev();
	virtual int  SetPulsesPerRev( int ppr );
	virtual int  GetPoles();
	virtual int  SetPoles( int poles );
	virtual int  GetCalibratedAngle();
	virtual int  SetCalibratedAngle( int angle );
	virtual int  GetLoggerTrigger();
	virtual int  SetLoggerTrigger( int trig );
	virtual int  GetLoggerPrescaler();
	virtual int  SetLoggerPrescaler( int prescal );
	virtual int  GetLoggerChannelData( int channel );
	virtual int  SetLoggerChannelInput( int channel, int input );
	virtual int GetEncoderActualSpeed( int mode );
	virtual int  PWMEnable( int command );
	virtual int  PhaseEnable( int phase, int command );
	virtual int  ADCCalEnable( int command );
	virtual int GetClarkeAs();
	virtual int GetClarkeBs();
	virtual int GetADCOffset( int channel );
	virtual int  SetADCOffset( int channel, int val );
	virtual int GetPIDIqOut();
	virtual int GetPIDIdOut();
	virtual int GetPIDSpdOut();
	virtual int GetPIDPosOut();
	virtual int GetPIDIqFdb();
	virtual int GetPIDIdFdb();
	virtual int GetPIDSpdFdb();
	virtual int GetPIDPosFdb();
	virtual int GetIqRef();
	virtual int  SetIqRef( int val );
	virtual int GetIdRef();
	virtual int  SetIdRef( int val );
	virtual int  GetEncoderType();
	virtual int  SetEncoderType( int type );
	virtual int  ElecThetaGeneration( int mode );
	virtual int  GetMaxSpeed();
	virtual int  SetMaxSpeed( int spd );
	virtual int  GetRamps();
	virtual int  SetRamps( int mode );
	virtual int ActualStartStopVelocity();
	virtual int ActualAceleration();
	virtual int ActualDeceleration();
	virtual int  SetActualPIDSpd( unsigned short set );
	virtual int  SetActualPIDPos( unsigned short set );
	virtual int  LoggerTriggerMode( int mode );
	virtual int GetEncoderActualPulses();
	virtual int  SetPhaseCurrent( int val );
	virtual int GetPhaseCurrent();
	virtual int  GetDecelerationAdvance();
	virtual int  SetDecelerationAdvance( int val );
	virtual int  GetPulsesToMm();
	virtual int  SetPulsesToMm( int ptm );
	virtual int  DecCompensationMode( int mode );
	virtual int  SetProximityGap( int val );
	virtual int GetProximityGap();
	virtual int  SetPIDPosActivationGap( int val );
	virtual int GetPIDPosActivationGap();
	virtual int  SetIParkAngle( int val );
	virtual int GetIParkAngle();
	virtual int GetDangerLimit( int limit );
	virtual int  SetDangerLimit( int limit, int val );
	virtual int  DangerLimitMode( int mode );
	virtual int  ResetEncoder();
	virtual int  HomeSensorInput( int val );
	virtual int GetHomingSpeed( int speed );
	virtual int  SetHomingSpeed( int speed, int val );
	virtual int  GetOverSpeed();
	virtual int  SetOverSpeed( int val );
	virtual int  SetReferenceActualPos();
	virtual int  ResetPIDSpd();
	virtual int  ResetPIDPos();
	virtual int  PIDStatus();
	virtual int  GetMaxReadCurrent();
	virtual int  SetMaxReadCurrent( int val );
	virtual int GetPhaseC();
	virtual int GetEncoder360ActualPosition();
	virtual int GetElecTheta();
	
	// get/set the acceleration feed-forward
	virtual int GetAccFFWD();
	virtual int  SetAccFFWD( int val );
	
	// get/set the speed feed-forward
	virtual int GetSpdFFWD();
	virtual int  SetSpdFFWD( int val );

	virtual int  GetMaxJerk();
	virtual int  SetMaxJerk( int val );

	virtual int GetCFriction();
	virtual int  SetCFriction( int val );
	
	virtual int GetEndMovementSpeed();
	virtual int  SetEndMovementSpeed( int val );
	virtual int GetEndMovementDelta();
	virtual int  SetEndMovementDelta( int val );
	
	// move multiple motors (absolute motion)
	virtual int  GotoPos0_Multi( unsigned char addX, int posX, unsigned char addY, int posY, int maxFreq, int acc, int proximityDeltaX, int proximityDeltaY );
	virtual int  ResetFlag_Multi();
	
	virtual int GetEncoder2ActualPosition();
	virtual int GetEncoder2ActualPulses();
	virtual int GetLineEncoder2();
	virtual int  SetLineEncoder2( int lines );
	virtual int GetEncoderInterpActualPosition();
	virtual int  GetStatus_ResetFlagMulti();
	virtual int  SetFollowingError( int val );
	virtual int GetFollowingError();
	virtual int  EncoderStatus();
	virtual int SetStartTicks( int ticks );
	virtual int SetPhaseRotation( int value );
	virtual int SetPhaseRotationNumber( int value );
	virtual int SetSpeedFilter( unsigned short window );
	virtual int SetSecurityCheck( int limit, int limitLevel );
	virtual int SetSecurityInput( int val );

	// from 4.10
	virtual int GetVPower();

	// from 4.11
	virtual int GetSensorTemp();
	virtual int SetSteadyPosition( int pos );
	virtual int GetSteadyPosition();
	virtual int SetEncoderSpikeDelta( unsigned short delta );
	virtual int GetEncoderSpikeDelta();

	// from 4.15
	virtual int UpdateOverSpeed();
	virtual int OverSpeedCheck( int enable );

	// from 5.1
	virtual int ResetSpeed();

protected:
	int Read_Reg_Micro( unsigned char RegAdd, unsigned short* Value );
	int Write_Reg_Micro( unsigned char RegAdd, unsigned short Value );

	unsigned char ver;
	unsigned char rev;
};

#endif
