//---------------------------------------------------------------------------
//
// Name:        tws_stepper.h
// Author:      Daniele Belloni, Gabriel Ferri
// Created:     07/05/2008
// Description: StepperModule class declaration
//
//---------------------------------------------------------------------------
#ifndef __STEPPERMODULE_H
#define __STEPPERMODULE_H

#include "tws_motor.h"

class StepperModule : public MotorModule
{
public:
	StepperModule( void* comInt, int address );
	~StepperModule();
 
	virtual unsigned short GetVersion();
	virtual int  SetMinCurrent( int val );
	virtual int  SetMaxCurrent( int val );
	virtual int  FreeRotation( int sensoRot );
	virtual int  StopRotation( int ramp );
	virtual int  MicroStepping( int frazione );
	virtual int ActualPosition();
	virtual int  SetActualPosition( int pos );
	virtual int  Home();
	virtual int  GotoPosRel( int pos );
	virtual int  GotoPos0( int pos );
	virtual int  MaximumFreq( int MFreq );
	virtual int  Aceleration( int acc );
	virtual int  Deceleration( int dec );
	virtual int  SlopeValue( int slopeFactor );
	virtual int  MotorStatus();
	virtual int  ActualInputs();
	virtual int  SetOutputs( int outId, int outVal );
	virtual int  SetLimitsCheck( int limit, int limitLevel = LIMITLEVEL_HIGH );
	virtual int  ResetAlarms();
	virtual int  MotorEnable( int command );
	virtual int  ResetDrive();
	virtual int  InputsSetting( int val );
	virtual int  SetDecay( int val );
	virtual int  SuspendDrive();
	virtual int  ExpandAddress( bool state );
	virtual int  HomeSensorInput( int val );
	virtual int GetHomingSpeed( int speed );
	virtual int  SetHomingSpeed( int speed, int val );
	virtual int  SetHomingMovement( int val );
	virtual int  SearchPos0( int dir );

protected:
	int Read_Reg_Micro( unsigned char RegAdd, unsigned short* Value );
	int Write_Reg_Micro( unsigned char RegAdd, unsigned short Value );
};

#endif
