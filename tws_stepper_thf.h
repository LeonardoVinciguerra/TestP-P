//---------------------------------------------------------------------------
//
// Name:        tws_stepper_thf.h
// Author:      Gabriel Ferri
// Created:     21/06/2011
// Description: StepperModuleTHF class declaration
//
//---------------------------------------------------------------------------
#ifndef __STEPPERMODULE_THF_H
#define __STEPPERMODULE_THF_H

#include "tws_stepper.h"

class StepperModuleTHF : public StepperModule
{
public:
	StepperModuleTHF( void* comInt, int address );
	~StepperModuleTHF();

	virtual int  FeederAdvance( int steps );
	virtual int  FeederUnlock();
	virtual int  FeederReady();
};

#endif
