//---------------------------------------------------------------------------
//
// Name:        motorhead.h
// Author:      Gabriel Ferri
// Created:     21/06/2011
// Description: MotorheadClass definition
//
//---------------------------------------------------------------------------

#ifndef __MOTORHEAD_H
#define __MOTORHEAD_H

#ifdef __USE_MOTORHEAD

#include "tws_brushless.h"


// QDVC brushless module errors
#define ERR_NOERROR				0x00000000
#define ERR_DRIVER				0x00000001
#define ERR_OVERCURRENT			0x00000002
#define ERR_MOVETIMEOUT			0x00000004
#define ERR_OVERSPEED			0x00000008
#define ERR_NOENC				0x00000010
#define ERR_LIMITSWITCH			0x00000020
#define ERR_SECURITYSTOP		0x00000040
#define ERR_STEADYPOSITION		0x00000080


struct MotorheadLogData
{
	MotorheadLogData()
	{
		trigger = 0.05f;
		trigger_mode = LOWTOHIGH_TRIG;
		prescaler = 50;

		channel_on[0] = false;
		channel_on[1] = true;
		channel_on[2] = false;
		channel_on[3] = false;

		channel_type[0] = LOGGER_POSREF;
		channel_type[1] = LOGGER_MECHTHETA;
		channel_type[2] = LOGGER_SPEED;
		channel_type[3] = LOGGER_SPEEDREF;
	};

	float trigger;
	int trigger_mode;
	int prescaler;

	bool channel_on[LOG_CH_NUM];
	int channel_type[LOG_CH_NUM];

	float channel_data[LOG_CH_NUM][LOG_BUF_SIZE];
};


enum MotorModulesEnum
{
	HEADX_ID = 0,
	HEADY_ID,
	HEADALL_ID,
	MOTOR_NUM // numero di motori
};


class MotorheadClass
{
public:
	MotorheadClass( void* ComIntRef );
	MotorheadClass();
	~MotorheadClass();

	bool IsEnabled() { return enabled; };
	void Disable() { enabled = false; }; // disabilita funzionalita' classe
	void Enable() { enabled = true; };   // abilita funzionalita' classe

	int GetError() { return error; };     // returns last error code
	int GetErrorID() { return errorId; }; // returns module ID that generates last error
	bool ResetAlarms();

	char* Version( int module ) { return &versionStr[module][0]; };

	bool GetStatus( int module, int& status );
	bool GetPIDStatus( int module, int& status );
	bool GetEncoderStatus( int module, int& status );
	bool GetLimitSwitchInput( int module, int& state );

	bool Init( int module );
	bool SetHome();

	bool RestartAfterImmediateStop();

	bool Move( float x, float y, float xNorm, bool force = false );
	bool Wait();

	bool EnableMotors();
	void DisableMotors();

	void SetSpeed( float speed ) { lastSpeed = speed; }
	float GetSpeed() { return lastSpeed; };
	void SetAcc( float acc ) { lastAcc = acc; }
	float GetAcc() { return lastAcc; };
	void SetSpeedAcc( float speed, float acc ) { lastSpeed = speed; lastAcc = acc; }

	bool ReadEncoderMm( float& x, float& y );
	bool ReadRefPosition( float& x, float& y );

	//GF_TEMP_ENC_TEST
	float GetEncoder( int module );
	float GetEncoder2( int module );
	float GetEncoderInterp( int module );
	int GetEncoderPulses( int module );
	int GetEncoder2Pulses( int module );
	float GetSpeedBoard();
	float GetAccBoard();
	float GetDecBoard();

	void ReloadPIDParams();
	bool GetPIDs( int module );
	void ReloadParameters();

	bool SetLogParams( int module );
	bool ReadLog( int module, int channel );

	bool ZeroSearch();
	bool WaitOrigin( int module );

	bool EnableLimits( bool state );

	bool SetStartSpeeds( int module );

private:
	bool _disableDriver( int module );
	bool _readVersion( int module );
	bool _setPIDs( int module );
	bool _wait( int module );

	bool enabled; // abilita/disabilita le funzionalita' della classe
	              // (utilizzata per le applicazioni stand-alone senza hw dedicato)

	int error;   // last error code
	int errorId; // contains the module ID that generates last error

	char versionStr[MOTOR_NUM][10];
	unsigned char ver[MOTOR_NUM];
	unsigned char rev[MOTOR_NUM];

	float currentX, currentY; // X, Y current position

	float lastSpeed; // last setted speed (m/s)
	float lastAcc;   // last setted acc (m/s^2)
};


extern MotorheadClass* Motorhead;


//GF_TEMP
void Read_HeadParams_File( const char* filename, int module );
void Read_PIDParams_File( const char* filename );

void Motorhead_ShowStatus();
void Motorhead_ShowPIDStatus();
void Motorhead_ReadPIDs();

//GF_TEMP - poi togliere
void Motorhead_ActualPosition();
//GF_TEMP_ENC_TEST
void Motorhead_ActualEncoder();
void Motorhead_ActualSpeedAcc();

float Motorhead_GetEncoderDiff();

void Motorhead_GetEncoderMM( float& x, float& y );

void Motorhead_ShowLog( int module );

int UpdateMotorhead();

#endif //__USE_MOTORHEAD

#endif //__MOTORHEAD_H
