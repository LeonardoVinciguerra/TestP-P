//---------------------------------------------------------------------------
//
// Name:        tws_stepper.cpp
// Author:      Daniele Belloni, Gabriel Ferri
// Created:     07/05/2008
// Description: StepperModule class implementation
//
//---------------------------------------------------------------------------
#include "tws_stepper.h"
#include "tws_stepperDefs.h"
#include "tws_sc.h"

#include <mss.h>


StepperModule::StepperModule( void* comInt, int address )
: MotorModule( comInt, address )
{
}

StepperModule::~StepperModule()
{
}

//-----------------------------------------------------------------------------
// Name: Read_Reg_Micro
// Desc: Legge un registro del micro
//-----------------------------------------------------------------------------
int StepperModule::Read_Reg_Micro( unsigned char RegAdd, unsigned short* Value )
{
	unsigned char rx_buffer[2];
	unsigned short rx_len = 0;
	
	if( TWSBus1_Send_HD( m_comInt, m_Address, READREGISTER_CMD, &RegAdd, 1, rx_buffer, &rx_len ) == 1 )
		return 1;
	
	*Value = 256 * rx_buffer[0] + rx_buffer[1];
	return 0;
}

//-----------------------------------------------------------------------------
// Name: Write_Reg_Micro()
// Desc: Scrive un registro del micro
//-----------------------------------------------------------------------------
int StepperModule::Write_Reg_Micro( unsigned char RegAdd, unsigned short Value )
{
	unsigned char tx_buffer[3];
	tx_buffer[0] = RegAdd;				// slave address
	tx_buffer[1] = Value >> 8;			// Value (H)
	tx_buffer[2] = Value;				// Value (L)
	
	return TWSBus1_Send_HD( m_comInt, m_Address, WRITEREGISTER_CMD, tx_buffer, 3 );
}

/*----------------------------------------------------------------------
FUNCTION:   unsigned short GetVersion()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the board firmware version
INPUT:      -
OUTPUT:     Version:
				- Bytes[1]: Version
				- Bytes[0]: Revision
GLOBAL:     -
RETURN:     Version on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
unsigned short StepperModule::GetVersion()
{
	unsigned short val;

	if( TWSBus1_Send_Command_HD( this->m_comInt, m_Address, GETVERSION_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	val = ( m_Data[0] << 8 ) + m_Data[1];
	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetMinCurrent( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor's reduced current
INPUT:      val: Current value
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::SetMinCurrent( int val )
{
	if( Write_Reg_Micro( 9, val ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetMaxCurrent( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor's current when running at
            constant speed
INPUT:      val: Current value
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::SetMaxCurrent( int val )
{
	if( Write_Reg_Micro( 10, val ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int FreeRotation( int sensoRot )
AUTHOR:     Daniele Belloni
INFO:       Function used to send the motor an infinite motion command
INPUT:      sensoRot: Rotation versus (0=CW, 1=CCW)
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::FreeRotation( int sensoRot )
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, FREEROTATION_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int StopRotation( int ramp )
AUTHOR:     Daniele Belloni
INFO:       Function used to stop the motor
INPUT:      ramp: stop with or without ramp (0 without, 1 with)
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::StopRotation( int ramp )
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, STOPROTATION_CMD, ramp, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int MicroStepping( int Fraction )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor steps per revolution
INPUT:      Fraction: motor step fraction (1..128)
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::MicroStepping( int Fraction )
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, MICROSTEPPING_CMD, Fraction, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ActualPosition()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's actual position
INPUT:      -
GLOBAL:     -
RETURN:     Motor actual position, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::ActualPosition()
{
	unsigned short valH = 0, valL = 0;
	if( Read_Reg_Micro( 14, &valH ) )
	{
		return MOTOR_ERROR;
	}
	if( Read_Reg_Micro( 15, &valL ) )
	{
		return MOTOR_ERROR;
	}

	return ( valH << 16 ) + valL;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetActualPosition( int pos )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor's actual position
INPUT:      pos: motor position
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::SetActualPosition( int pos )
{
	if (Write_Reg_Micro( 14, pos >> 16 ))
	{
		return MOTOR_ERROR;
	}
	if (Write_Reg_Micro( 15, pos & 0xFFFF ))
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int Home()
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor's home position
INPUT:      -
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::Home()
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, ZEROPOSITION_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GotoPosRel( int pos )
AUTHOR:     Daniele Belloni
INFO:       Function used to move the motor (relative motion)
INPUT:      pos: number of steps
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::GotoPosRel( int pos )
{
	// Setta il numero di passi (32 bit)
	if( Write_Reg_Micro( 7, pos >> 16 ) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 8, pos & 0xFFFF ) )
	{
		return MOTOR_ERROR;
	}

	// Movimento relativo
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, RELATIVEMOVE_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GotoPos0( int pos )
AUTHOR:     Daniele Belloni
INFO:       Function used to move the motor (absolute motion)
INPUT:      pos: number of steps
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::GotoPos0( int pos )
{
    // Setta il numero di passi (32 bit)
	if( Write_Reg_Micro( 7, pos >> 16 ) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 8, pos & 0xFFFF ) )
	{
		return MOTOR_ERROR;
	}

	// Movimento assoluto
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, ABSOLUTEMOVE_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int MaximumFreq( int MFreq )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor working frequency in ms.
            (1 to MaximumAccFreq)
INPUT:      MFreq: frequency
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::MaximumFreq( int MFreq )
{
	if( Write_Reg_Micro( 2, (unsigned short)MFreq ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int Aceleration( int acc )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor aceleration time in ms.
            (1 to 10000)
INPUT:      acc: aceleration time
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::Aceleration( int acc )
{
	if( Write_Reg_Micro( 3, (unsigned short)acc ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int Deceleration( int dec )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor deceleration time in ms.
            (1 to 10000)
INPUT:      dec: deceleration time
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::Deceleration( int dec )
{
    if( Write_Reg_Micro( 4, (unsigned short)dec ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SlopeValue( int slopeFactor )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor aceleration and deceleration
            time in ms. (1 to 10000)
INPUT:      slopeFactor: aceleration and deceleration time
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::SlopeValue( int slopeFactor )
{
	if( Write_Reg_Micro( 3, (unsigned short)slopeFactor ) )
	{
		return MOTOR_ERROR;
	}
    if( Write_Reg_Micro( 4, (unsigned short)slopeFactor ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int MotorStatus()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's status
            Used with masks:
            MOTOR_RUNNING: the motor is moving
            MOTOR_OVERRUN: limit switch reached
INPUT:      -
GLOBAL:     -
RETURN:     Motor status, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::MotorStatus()
{
	unsigned short val = 0;

	if( Read_Reg_Micro( 13, &val) )
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int ActualInputs()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the driver's actual digital inputs
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     Driver actual inputs status, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::ActualInputs()
{
	unsigned short val = 0;

	if( Read_Reg_Micro( 16, &val) )
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetOutputs( int outId, int outVal )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the driver's digital outputs
INPUT:      outId: Output id (OUT0, OUT1, OUT2)
            outVal: Output val (LOW, HIGH)
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::SetOutputs( int outId, int outVal )
{
	unsigned short val = 0;

	// Read currently setted outputs
	if( Read_Reg_Micro( 17, &val) )
	{
		return MOTOR_ERROR;
	}

	switch( outId )
	{
		case OUT0:
			if( outVal == LOW )
				val &= 0xFFFE;
			else
				val |= 0x0001;
			break;

		case OUT1:
			if( outVal == LOW )
				val &= 0xFFFD;
			else
				val |= 0x0002;
			break;

		case OUT2:
			if( outVal == LOW )
				val &= 0xFFFB;
			else
				val |= 0x0004;
			break;
	}

	if( Write_Reg_Micro( 17, val) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetLimitsCheck( int limit, int limitLevel )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor limits check.
INPUT:      limitLevel: Set limit check level high or low (LIMITLEVEL_HIGH,
                        LIMITLEVEL_LOW)
            limit: Activate or deactivate limits check (LIMITCHECK_ON,
                   LIMITCHECK_OFF)
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::SetLimitsCheck( int limit, int limitLevel )
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, LIMITSCHECK_CMD, limit, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, LIMITSLEVEL_CMD, limitLevel, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ResetAlarms()
AUTHOR:     Daniele Belloni
INFO:       Function used to reset driver alarms.
INPUT:      -
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::ResetAlarms()
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, ALARMRESET_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int MotorEnable( int command )
AUTHOR:     Daniele Belloni
INFO:       Function used to reset driver alarms.
INPUT:      command: action
                     MOTOR_OFF:    disable motor
                     MOTOR_ON:     enable motor
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::MotorEnable( int command )
{
	if( command == MOTOR_OFF )
	{
		if( TWSBus1_Send_Command_HD( m_comInt, m_Address, DISABLEDRIVER_CMD, m_Data ) )
		{
			return MOTOR_ERROR;
		}
	}
	else if( command == MOTOR_ON )
	{
		if( TWSBus1_Send_Command_HD( m_comInt, m_Address, ENABLEDRIVER_CMD, m_Data ) )
		{
			return MOTOR_ERROR;
		}
	}
	else
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ResetDrive()
AUTHOR:     Daniele Belloni
INFO:       Function used reset the drive
INPUT:      -
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::ResetDrive()
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, MICRORESET_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int InputsSetting( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set motion controllers inputs setting
INPUT:      val: inputs setting DWORD
				- Bytes[3..2]: CW limit switch input
				- Bytes[1..0]: CCW limit switch input
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::InputsSetting( int val )
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, CWLIMITINPUT_CMD, (val&0xFFFF0000)>>16, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, CCWLIMITINPUT_CMD, (val&0x0000FFFF), m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetDecay( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set motor's decay
INPUT:      val: decays
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::SetDecay( int val )
{
	if( Write_Reg_Micro( 6, val) )
	{
		return MOTOR_ERROR;
	}
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, DECAYMODE_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SuspendDrive()
AUTHOR:     Daniele Belloni
INFO:       Function used to suspend the drive serial communication
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::SuspendDrive()
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, SUSPENDSERIAL_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ExpandAddress( bool state )
AUTHOR:     Gabriel Ferri
INFO:       Function used to expand the address
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on OK, MOTOR_ERROR on error
----------------------------------------------------------------------*/
int StepperModule::ExpandAddress( bool state ) // solo ver. 2.xx
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, state ? 23 : 24, m_Data ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int HomeSensorInput( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set motion controllers home sensor input
INPUT:      val: home sensor input
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int StepperModule::HomeSensorInput( int val )
{
	if( Write_Reg_Micro( 28, val ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetHomingSpeed( int speed )
AUTHOR:     Daniele Belloni
INFO:       Function used to get homing speeds (fast and slow)
INPUT:      speed: action
                     HOME_SLOW: read homing slow speed
                     HOME_FAST: read homing fast speed
OUTPUT:     -
GLOBAL:     -
RETURN:     Homing speeds, -1 if errors
----------------------------------------------------------------------*/
int StepperModule::GetHomingSpeed( int speed )
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;

	if( speed == HOME_SLOW )
	{
		if (Read_Reg_Micro( 30, &val))
		{
			return MOTOR_ERROR;
		}
	}
	else
	{
		if (Read_Reg_Micro( 29, &val))
		{
			return MOTOR_ERROR;
		}
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetHomingSpeed( int speed, int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set homing speeds (fast and slow)
INPUT:      speed: action
                     HOME_SLOW: read homing slow speed
                     HOME_FAST: read homing fast speed
		    val: speed value
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int StepperModule::SetHomingSpeed( int speed, int val )
{
	if( speed == HOME_SLOW )
	{
		if( Write_Reg_Micro( 30, val ) )
		{
			return MOTOR_ERROR;
		}
	}
	else
	{
		if( Write_Reg_Micro( 29, val ) )
		{
			return MOTOR_ERROR;
		}
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetHomingMovement( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set homing movement
INPUT:      val: movement in steps
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int StepperModule::SetHomingMovement( int val )
{
	if( Write_Reg_Micro( 37, val >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 38, val & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Search motor's home position
INPUT:      dir: search direction (ZEROSEARCH_POS, ZEROSEARCH_NEG)
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int StepperModule::SearchPos0( int dir )
{
	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, HOMESEARCH_CMD, dir, m_Data) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}
