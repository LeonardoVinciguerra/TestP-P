//---------------------------------------------------------------------------
//
// Name:        tws_stepper_thf.cpp
// Author:      Gabriel Ferri
// Created:     21/06/2011
// Description: StepperModuleTHF class implementation
//
//---------------------------------------------------------------------------
#include "tws_stepper_thf.h"
#include "tws_stepper_thfDefs.h"
#include "tws_sc.h"

#include <mss.h>


StepperModuleTHF::StepperModuleTHF( void* comInt, int address )
: StepperModule( comInt, address )
{
}

StepperModuleTHF::~StepperModuleTHF()
{
}


/*----------------------------------------------------------------------
FUNCTION:   int FeederAdvance()
AUTHOR:     Gabriel Ferri
INFO:       Function used to advance the feeder
INPUT:      steps: number of steps (32 bit)
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModuleTHF::FeederAdvance( int steps )
{
	// Setta il numero di passi (32 bit)
	if( Write_Reg_Micro( 7, steps >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 8, steps & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, FEEDER_ADVANCE_CMD, m_Data) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int FeederUnlock()
AUTHOR:     Gabriel Ferri
INFO:       Function used to unlock the component on the feeder
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModuleTHF::FeederUnlock()
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, FEEDER_UNLOCK_CMD, m_Data) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int FeederReady()
AUTHOR:     Gabriel Ferri
INFO:       Function used to read the feeder status
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     Feeder status, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModuleTHF::FeederReady()
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, FEEDER_READY_CMD, m_Data) )
	{
		return MOTOR_ERROR;
	}

	return m_Data[0];
}
