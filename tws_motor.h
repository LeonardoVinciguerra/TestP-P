//---------------------------------------------------------------------------
//
// Name:        tws_motor.h
// Author:      Gabriel Ferri
// Created:     07/05/2008
// Description: MotorModule class declaration
//
//---------------------------------------------------------------------------
#ifndef __MOTORMODULE_H
#define __MOTORMODULE_H


#define MOTOR_DATA_LEN		512

#define MOTOR_ERROR			-1
#define MOTOR_OK			0

#define SLOW_DECAY			0
#define FAST_DECAY			1

#define CW					0x01
#define CCW					0x00

#define IO0					0x01
#define IO1					0x02
#define IO2					0x04
#define IO3					0x08
#define DIO0				0x10
#define DIO1				0x20
#define DIO2				0x40

#define IOIN				0
#define IOOUT				1

#define LOW					0
#define HIGH				1

#define NORAMP				0
#define RAMP				1
#define OUT0				0
#define OUT1				1
#define OUT2				2
#define OUT3				3
#define OUT4				4
#define OUT5				5
#define OUT6				6
#define OUT7				7
#define OUT8				8

// Maschere per la funzione MotorStatus
#define MOTOR_RUNNING		0x0001		// Il motore si sta muovendo
#define MOTOR_OVERCURRENT	0x0002		// Overcurrent
#define MOTOR_PROCERROR		0x0004		// Errore nella procedura (usato in ricerca zero)
#define MOTOR_TIMEOUT		0x0008		// Timeout nel movimento
#define MOTOR_ZERO			0x0010		// Ricerca origini in corso
#define MOTOR_OVERRUN		0x0020		// Raggiunto un finecorsa
#define MOTOR_DANGER		0x0040		// Raggiunto il limite software di movimento
#define MOTOR_OVERSPEED		0x0080		// Overspeed
#define MOTOR_NOENC			0x0100		// Nessuna variazione encoder rilevata
#define MOTOR_PIDSPDON		0x0200		// PID di velocita' attivo
#define MOTOR_PIDPOSON		0x0400		// PID di posizione attivo
#define MOTOR_NEARTARGET	0x0800		// Il motore e' in prossimita della posizione obiettivo
#define MOTOR_MULTIMOVE		0x1000		// Il motore ha eseguito un movimento "multi-motore"
#define MOTOR_NO_FOLLOW		0x2000		// Errore inseguimento traiettoria "eccessivo"
#define MOTOR_SECURITY		0x4000		// Sensore di sicurezza attivo
#define MOTOR_STEADYPOS		0x8000		// Errore mantenimento posizione


// Maschere per la funzione PIDStatus
#define PID_SPDDEF			0x0001		// Parametri di default PID di velocita' attivi
#define PID_SPD0			0x0002		// Parametri di regime PID di velocita' attivi
#define PID_SPDSPD			0x0004		// Parametri di loop di velocita' PID di velocita' attivi
#define PID_POSDEF			0x0008		// Parametri di default PID di posizione attivi
#define PID_POS0			0x0010		// Parametri di regime PID di posizione attivi

// Maschere per la funzione EncoderStatus
#define ENC_MF_ERROR        0x0001		// Magnetic field error
#define ENC_OVERRUN_ERROR   0x0002		// Overrun error
#define ENC_FRAME_ERROR	    0x0004		// Frame error
#define ENC_NORECEPTIONS    0x0008		// No receptions

// Attivazione o disattivazione check dei limiti dei movimenti
#define LIMITCHECK_OFF		0
#define LIMITCHECK_ON		1
// Livello check limiti (attivi alti o bassi)
#define LIMITLEVEL_LOW		0
#define LIMITLEVEL_HIGH		1

// Attivazione o disattivazione check del sensore di sicurezza
#define SECURITYCHECK_OFF   0
#define SECURITYCHECK_ON    1
// Livello check sensore sicurezza (attivo alto o basso)
#define SECURITYLEVEL_LOW   0
#define SECURITYLEVEL_HIGH  1

// Attivazione, disattivazione o stato dei movimenti del motore
#define MOTOR_OFF			0
#define MOTOR_ON			1
#define MOTOR_SPD_ON		2

// Modalita' di funzionamento dei moduli (con o senza encoder) //$$$ENC
#define NORMAL_MODE			0
#define ENCODER_MODE		1

// Modalita' di rotazione senza encoder
#define PHASEROT_DISABLE    0
#define PHASEROT_POS        1
#define PHASEROT_NEG        2

// Direzione ricerca zero
#define ZEROSEARCH_POS  	0
#define ZEROSEARCH_NEG  	1

// Velocita' di homing
#define HOME_SLOW  			0
#define HOME_FAST  			1


class MotorModule
{
public:
	MotorModule( void* comInt, int Address ) { m_comInt = comInt; m_Address = Address; }
	~MotorModule() {}
	
	void ChangeAddress( int Address ) { m_Address = Address; }
	int GetAddress() { return m_Address; }
	int GetErrorCode() { return errorCode; }

	virtual unsigned short GetVersion() { return 0; }
	virtual int  SetMinCurrent( int val ) { return MOTOR_OK; }
	virtual int  SetMaxCurrent( int val ) { return MOTOR_OK; }
	virtual int  SetBoostCurrent( int val ) { return MOTOR_OK; }
	virtual int  SetNominalCurrent( int val ) { return MOTOR_OK; }
	virtual int  GetNominalCurrent() { return MOTOR_OK; }
	virtual int  FreeRotation( int sensoRot ) { return MOTOR_OK; }
	virtual int  StopRotation( int ramp ) { return MOTOR_OK; }
	virtual int  MicroStepping( int frazione ) { return MOTOR_OK; }
	virtual int ActualPosition() { return 0; }
	virtual int  SetActualPosition( int pos ) { return MOTOR_OK; }
	virtual int  Home() { return MOTOR_OK; }
	virtual int  GotoPosRel( int pos ) { return MOTOR_OK; }
	virtual int  GotoPos0( int pos ) { return MOTOR_OK; }
	virtual int  MinimumFreq( int mFreq ) { return MOTOR_OK; }
	virtual int  MaximumAccFreq( int MFreq ) { return MOTOR_OK; }
	virtual int  MaximumFreq( int MFreq ) { return MOTOR_OK; }
	virtual int  Aceleration( int acc ) { return MOTOR_OK; }
	virtual int  Deceleration( int dec ) { return MOTOR_OK; }
	virtual int  SlopeValue( int slopeFactor ) { return MOTOR_OK; }
	virtual int  ActualCurrent() { return 0; }
	virtual int ActualVelocity() { return 0; }
	virtual int  ActualVoltage() { return 0; }
	virtual int  ActualTemperature() { return 0; }
	virtual int  MotorStatus() { return 0; }
	virtual int  ActualInputs() { return 0; }
	virtual int  SetOutputs( unsigned short outId, int outVal ) { return MOTOR_OK; }
	virtual int  SetLimitsCheck( int limit, int limitLevel = LIMITLEVEL_HIGH ) { return MOTOR_OK; }
	virtual int  Disable_FW_Output() { return MOTOR_OK; }
	virtual int  ResetAlarms() { return MOTOR_OK; }
	virtual int  MotorEnable( int command ) { return MOTOR_OK; }
	virtual int  ResetDrive() { return MOTOR_OK; }
	virtual int GetEncoderActualPosition() { return 0; }
	virtual int  SetEncoderActualPosition( int pos ) { return MOTOR_OK; }
	virtual int EncoderActualFrequency() { return 0; }
	virtual int  MotorToEncoderRatio( float ratio ) { return MOTOR_OK; }
	virtual int  MaxEncoderControlError( int err ) { return MOTOR_OK; }
	virtual int  EncoderMode( int type, int mode ) { return MOTOR_OK; }
	virtual int  GetEncoderMode( int type ) { return 0; }
	virtual int  EncoderTargetPosition( int pulse ) { return MOTOR_OK; }
	virtual int  EncoderTargetError( int err ) { return MOTOR_OK; }
	virtual int  StoreParameters() { return MOTOR_OK; }
	virtual int  RestoreParameters() { return MOTOR_OK; }
	virtual int  InputsSetting( int val ) { return MOTOR_OK; }
	virtual int  SearchPos0( int dir ) { return MOTOR_OK; }
	virtual int  SetDecay( int val ) { return MOTOR_OK; }
	virtual int  SuspendDrive() { return MOTOR_OK; }
	virtual int  SetPIDIqParams( int KP, int KI, int KD, int KC, int OMax, int OMin ) { return MOTOR_OK; }
	virtual int  GetPIDIqParams( int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin ) { return MOTOR_OK; }
	virtual int  SetPIDIdParams( int KP, int KI, int KD, int KC, int OMax, int OMin ) { return MOTOR_OK; }
	virtual int  GetPIDIdParams( int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin ) { return MOTOR_OK; }
	virtual int  SetPIDSpdParams( unsigned short set, int KP, int KI, int KD, int KC, int OMax, int OMin ) { return MOTOR_OK; }
	virtual int  GetPIDSpdParams( unsigned short set, int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin ) { return MOTOR_OK; }
	virtual int  SetPIDPosParams( unsigned short set, int KP, int KI, int KD, int KC, int OMax, int OMin ) { return MOTOR_OK; }
	virtual int  GetPIDPosParams( unsigned short set, int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin ) { return MOTOR_OK; }
	virtual int GetLineEncoder() { return MOTOR_OK; }
	virtual int  SetLineEncoder( int lines ) { return MOTOR_OK; }
	virtual int GetPulsesPerRev() { return MOTOR_OK; }
	virtual int  SetPulsesPerRev( int ppr ) { return MOTOR_OK; }
	virtual int  GetPoles() { return MOTOR_OK; }
	virtual int  SetPoles( int poles ) { return MOTOR_OK; }
	virtual int  GetCalibratedAngle() { return MOTOR_OK; }
	virtual int  SetCalibratedAngle( int angle ) { return MOTOR_OK; }
	virtual int  GetLoggerTrigger() { return MOTOR_OK; }
	virtual int  SetLoggerTrigger( int trig ) { return MOTOR_OK; }
	virtual int  GetLoggerPrescaler() { return MOTOR_OK; }
	virtual int  SetLoggerPrescaler( int prescal ) { return MOTOR_OK; }
	virtual int  GetLoggerChannelData( int channel ) { return MOTOR_OK; }
	virtual int  SetLoggerChannelInput( int channel, int input ) { return MOTOR_OK; }
	virtual int GetEncoderActualSpeed( int mode ) { return MOTOR_OK; }
	virtual int  PWMEnable( int command ) { return MOTOR_OK; }
	virtual int  PhaseEnable( int phase, int command ) { return MOTOR_OK; }
	virtual int  ADCCalEnable( int command ) { return MOTOR_OK; }
	virtual int GetClarkeAs() { return MOTOR_OK; }
	virtual int GetClarkeBs() { return MOTOR_OK; }
	virtual int GetADCOffset( int channel ) { return MOTOR_OK; }
	virtual int  SetADCOffset( int channel, int val ) { return MOTOR_OK; }
	virtual int GetPIDIqOut() { return MOTOR_OK; }
	virtual int GetPIDIdOut() { return MOTOR_OK; }
	virtual int GetPIDSpdOut() { return MOTOR_OK; }
	virtual int GetPIDPosOut() { return MOTOR_OK; }
	virtual int GetPIDIqFdb() { return MOTOR_OK; }
	virtual int GetPIDIdFdb() { return MOTOR_OK; }
	virtual int GetPIDSpdFdb() { return MOTOR_OK; }
	virtual int GetPIDPosFdb() { return MOTOR_OK; }
	virtual int GetIqRef() { return MOTOR_OK; }
	virtual int  SetIqRef( int val ) { return MOTOR_OK; }
	virtual int GetIdRef() { return MOTOR_OK; }
	virtual int  SetIdRef( int val ) { return MOTOR_OK; }
	virtual int  GetEncoderType() { return MOTOR_OK; }
	virtual int  SetEncoderType( int type ) { return MOTOR_OK; }
	virtual int  ElecThetaGeneration( int mode ) { return MOTOR_OK; }
	virtual int  GetMaxSpeed() { return MOTOR_OK; }
	virtual int  SetMaxSpeed( int spd ) { return MOTOR_OK; }
	virtual int  GetRamps() { return MOTOR_OK; }
	virtual int  SetRamps( int mode ) { return MOTOR_OK; }
	virtual int ActualStartStopVelocity() { return MOTOR_OK; }
	virtual int ActualAceleration() { return MOTOR_OK; }
	virtual int ActualDeceleration() { return MOTOR_OK; }
	virtual int  SetActualPIDSpd( unsigned short set ) { return MOTOR_OK; }
	virtual int  SetActualPIDPos( unsigned short set ) { return MOTOR_OK; }
	virtual int  LoggerTriggerMode( int mode ) { return MOTOR_OK; }
	virtual int GetEncoderActualPulses() { return MOTOR_OK; }
	virtual int  SetPhaseCurrent( int val ) { return MOTOR_OK; }
	virtual int GetPhaseCurrent() { return MOTOR_OK; }
	virtual int  GetDecelerationAdvance() { return MOTOR_OK; }
	virtual int  SetDecelerationAdvance( int val ) { return MOTOR_OK; }
	virtual int  GetPulsesToMm() { return MOTOR_OK; }
	virtual int  SetPulsesToMm( int ptm ) { return MOTOR_OK; }
	virtual int  DecCompensationMode( int mode ) { return MOTOR_OK; }
	virtual int  SetProximityGap( int val ) { return MOTOR_OK; }
	virtual int GetProximityGap() { return MOTOR_OK; }
	virtual int  SetPIDPosActivationGap( int val ) { return MOTOR_OK; }
	virtual int GetPIDPosActivationGap() { return MOTOR_OK; }
	virtual int  SetIParkAngle( int val ) { return MOTOR_OK; }
	virtual int GetIParkAngle() { return MOTOR_OK; }
	virtual int GetDangerLimit( int limit ) { return MOTOR_OK; }
	virtual int  SetDangerLimit( int limit, int val ) { return MOTOR_OK; }
	virtual int  DangerLimitMode( int mode ) { return MOTOR_OK; }
	virtual int  ResetEncoder() { return MOTOR_OK; }
	virtual int  HomeSensorInput( int val ) { return MOTOR_OK; }
	virtual int GetHomingSpeed( int speed ) { return MOTOR_OK; }
	virtual int  SetHomingSpeed( int speed, int val ) { return MOTOR_OK; }
	virtual int  SetHomingMovement( int val ) { return MOTOR_OK; }
	virtual int  GetOverSpeed() { return MOTOR_OK; }
	virtual int  SetOverSpeed( int val ) { return MOTOR_OK; }
	virtual int  SetReferenceActualPos() { return MOTOR_OK; }
	virtual int  ResetPIDSpd() { return MOTOR_OK; }
	virtual int  ResetPIDPos() { return MOTOR_OK; }
	virtual int  PIDStatus() { return MOTOR_OK; }
	virtual int  GetMaxReadCurrent() { return MOTOR_OK; }
	virtual int  SetMaxReadCurrent( int val ) { return MOTOR_OK; }
	virtual int GetPhaseC() { return MOTOR_OK; }
	virtual int GetEncoder360ActualPosition() { return MOTOR_OK; }
	virtual int GetElecTheta() { return MOTOR_OK; }
	virtual int GetAccFFWD() { return MOTOR_OK; }
	virtual int  SetAccFFWD( int val ) { return MOTOR_OK; }
	virtual int GetSpdFFWD() { return MOTOR_OK; }
	virtual int  SetSpdFFWD( int val ) { return MOTOR_OK; }
	virtual int  GetMaxJerk() { return MOTOR_OK; }
	virtual int  SetMaxJerk( int val ) { return MOTOR_OK; }
	virtual int GetCFriction() { return MOTOR_OK; }
	virtual int  SetCFriction( int val ) { return MOTOR_OK; }
	virtual int GetEndMovementSpeed() { return MOTOR_OK; }
	virtual int  SetEndMovementSpeed( int val ) { return MOTOR_OK; }
	virtual int GetEndMovementDelta() { return MOTOR_OK; }
	virtual int  SetEndMovementDelta( int val ) { return MOTOR_OK; }
	virtual int  GotoPos0_Multi( unsigned char addX, int posX, unsigned char addY, int posY, int maxFreq, int acc, int posXnorm ) { return MOTOR_OK; }
	virtual int  ResetFlag_Multi() { return MOTOR_OK; }
	virtual int GetEncoder2ActualPosition() { return MOTOR_OK; }
	virtual int GetEncoder2ActualPulses() { return MOTOR_OK; }
	virtual int GetLineEncoder2() { return MOTOR_OK; }
	virtual int  SetLineEncoder2( int lines ) { return MOTOR_OK; }
	virtual int GetEncoderInterpActualPosition() { return MOTOR_OK; }
	virtual int  GetStatus_ResetFlagMulti() { return MOTOR_OK; }
	virtual int  SetFollowingError( int val ) { return MOTOR_OK; }
	virtual int GetFollowingError() { return MOTOR_OK; }
	virtual int  EncoderStatus() { return MOTOR_OK; }
	virtual int SetStartTicks( int ticks ) { return MOTOR_OK; }
	virtual int SetPhaseRotation( int value ) { return MOTOR_OK; }
	virtual int SetPhaseRotationNumber( int value ) { return MOTOR_OK; }
	virtual int SetSpeedFilter( unsigned short window ) { return MOTOR_OK; }
	virtual int SetSecurityCheck( int limit, int limitLevel ) { return MOTOR_OK; }
	virtual int SetSecurityInput( int val ) { return MOTOR_OK; }
	virtual int GetVPower() { return MOTOR_OK; }
	virtual int GetSensorTemp() { return MOTOR_OK; }
	virtual int SetSteadyPosition( int pos ) { return MOTOR_OK; }
	virtual int GetSteadyPosition() { return MOTOR_OK; }
	virtual int SetEncoderSpikeDelta( unsigned short delta ) { return MOTOR_OK; }
	virtual int GetEncoderSpikeDelta() { return MOTOR_OK; }
	virtual int UpdateOverSpeed() { return MOTOR_OK; }
	virtual int OverSpeedCheck( int enable ) { return MOTOR_OK; }
	virtual int ResetSpeed() { return MOTOR_OK; }

protected:
	void* m_comInt;
	int m_Address;
	unsigned char m_Data[MOTOR_DATA_LEN];
	int errorCode;
};

#endif
