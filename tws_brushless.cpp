//---------------------------------------------------------------------------
//
// File:     tws_brushless.cpp
// Info:     Low level functions for Brushless controller module
// Created:  Daniele Belloni, Gabriel Ferri [24Sep07]
//
//---------------------------------------------------------------------------
#include "tws_brushless.h"
#include "tws_brushlessDefs.h"
#include "tws_sc.h"

#include <mss.h>


BrushlessModule::BrushlessModule( void* comInt, int address )
: MotorModule ( comInt, address )
{
	ver = 0;
	rev = 0;
}

BrushlessModule::~BrushlessModule()
{
}

//-----------------------------------------------------------------------------
// Desc: Legge un registro del micro
//-----------------------------------------------------------------------------
int BrushlessModule::Read_Reg_Micro( unsigned char RegAdd, unsigned short* Value )
{
	if( m_Address == 0 )
	{
		return 1;
	}

	unsigned char rx_buffer[2];
	unsigned short rx_len = 0;

	if( (errorCode = TWSBus1_Send_HD( m_comInt, m_Address, READREGISTER_CMD, &RegAdd, 1, rx_buffer, &rx_len )) )
	{
		return 1;
	}

	*Value = (rx_buffer[0]<<8) + rx_buffer[1];
	return 0;
}

//-----------------------------------------------------------------------------
// Desc: Scrive un registro del micro
//-----------------------------------------------------------------------------
int BrushlessModule::Write_Reg_Micro( unsigned char RegAdd, unsigned short Value )
{
	if( m_Address == 0 )
	{
		return 1;
	}

	unsigned char tx_buffer[3];
	tx_buffer[0] = RegAdd;			// slave address
	tx_buffer[1] = Value >> 8;		// Value (H)
	tx_buffer[2] = Value;			// Value (L)
	
	return (errorCode = TWSBus1_Send_HD( m_comInt, m_Address, WRITEREGISTER_CMD, tx_buffer, 3 ));
}

/*----------------------------------------------------------------------
Function used to read the board firmware version
OUTPUT:     Version:
				- Bytes[1]: Version
				- Bytes[0]: Revision
RETURN:     Version if OK, -1 if errors
----------------------------------------------------------------------*/
unsigned short BrushlessModule::GetVersion()
{
	unsigned short val;

	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETVERSION_CMD, m_Data)) )
	{
		return MOTOR_ERROR;
	}
	val = (m_Data[0]<<8) + m_Data[1];

	ver = m_Data[0];
	rev = m_Data[1];

	return val;
}

/*----------------------------------------------------------------------
Function used to set the motor's nominal current.
INPUT:      val: Current value in mA
RETURN:     MOTOR_OK if OK, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::SetNominalCurrent( int val )
{
	if( Write_Reg_Micro( 135, val) )
	{
		return MOTOR_ERROR;
	}

	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, UPDATECURRENT_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
INFO:       Function used to get the motor's nominal current.
RETURN:     Current value in mA, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetNominalCurrent()
{
	unsigned short val = 0;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	if( Read_Reg_Micro( 135, &val) )
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
INFO:       Function used to stop the motor
INPUT:      ramp: left for compatibility...not used
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::StopRotation( int ramp )
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, IMMEDIATESTOP_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ActualPosition()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's actual reference position
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor actual position in IQ24
----------------------------------------------------------------------*/
int BrushlessModule::ActualPosition()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	if (Read_Reg_Micro( 5, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 6, &valL))
	{
		return MOTOR_ERROR;
	}

	return (valH << 16) + valL;
}

/*----------------------------------------------------------------------
Set the motor's home position
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::Home()
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, HOME_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Move the motor (absolute motion)
INPUT:      pos: number of steps
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::GotoPos0( int pos )
{
    // Setta il numero di passi (32 bit)
	if( Write_Reg_Micro( 5, pos >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 6, pos & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, POSUPDATE_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int MinimumFreq( int mFreq )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor start/stop speed
INPUT:      mFreq: start/stop speed in m/s (IQ24)
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::MinimumFreq( int mFreq )
{
    // Setta la velocita' di start/stop (32 bit)
	if( Write_Reg_Micro( 79, mFreq >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 80, mFreq & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int MaximumFreq( int MFreq )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor speed
INPUT:      MFreq: speed in m/s (IQ24)
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::MaximumFreq( int MFreq )
{
    // Setta la velocita' (32 bit)
	if( Write_Reg_Micro( 3, MFreq >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 4, MFreq & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int Aceleration( int acc )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor aceleration.
INPUT:      acc: aceleration in m/s2 (IQ24)
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::Aceleration( int acc )
{
	if( Write_Reg_Micro( 81, acc >> 16) )
	{
		return MOTOR_ERROR;
	}
    if( Write_Reg_Micro( 82, acc & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int Deceleration( int dec )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor deceleration.
INPUT:      dec: deceleration in m/s2 (IQ24)
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::Deceleration( int dec )
{
	if( Write_Reg_Micro( 103, dec >> 16) )
	{
		return MOTOR_ERROR;
	}
    if( Write_Reg_Micro( 104, dec & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ActualVelocity()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's actual setted velocity
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor actual setted velocity in m/s, -1 if errors
----------------------------------------------------------------------*/
int BrushlessModule::ActualVelocity()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	if (Read_Reg_Micro( 3, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 4, &valL))
	{
		return MOTOR_ERROR;
	}

	return (valH << 16) + valL;
}

/*----------------------------------------------------------------------
FUNCTION:   int MotorStatus()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's status
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor status, -1 if errors
----------------------------------------------------------------------*/
int BrushlessModule::MotorStatus()
{
	unsigned short val = 0;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	if( Read_Reg_Micro( 74, &val) )
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
Read the driver's actual digital inputs
RETURN:     Driver actual inputs status, -1 if errors
----------------------------------------------------------------------*/
int BrushlessModule::ActualInputs()
{
	unsigned short val = 0;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	if( Read_Reg_Micro( 1, &val) )
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
Set the motor limits check
INPUT:      limitLevel: Set limit check level high or low (LIMITLEVEL_HIGH,
                        LIMITLEVEL_LOW)
            limit: Activate or deactivate limits check (LIMITCHECK_ON,
                   LIMITCHECK_OFF)
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetLimitsCheck( int limit, int limitLevel )
{
	unsigned short val = 0;

	// Read mode register
	if( Read_Reg_Micro( 77, &val) )
	{
		return MOTOR_ERROR;
	}

	if( limit == LIMITCHECK_ON )
	{
		// Setta il flag di check limiti a true
		val |= (1<<MODE_LIMITS);
	}
	else
	{
		// Setta il flag di check limiti a false
		val &= ~(1<<MODE_LIMITS);
	}

	if( limitLevel == LIMITLEVEL_HIGH )
	{
		// Setta il livello di check limiti alto
		val |= (1<<MODE_LIMITSLEVEL);
	}
	else
	{
		// Setta il livello di check limiti basso
		val &= ~(1<<MODE_LIMITSLEVEL);
	}

	// Write mode register
	if( Write_Reg_Micro( 77, val) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Reset driver alarms
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::ResetAlarms()
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, ALARMRESET_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Enable motor control
INPUT:      command: action
                     MOTOR_OFF:    disable motor
                     MOTOR_ON:     enable motor (speed and position loop)
                     MOTOR_SPD_ON: enable motor (speed loop)
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::MotorEnable( int command )
{
	if( command == MOTOR_OFF )
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, LOOPDISABLE_CMD, m_Data);
	}
	else if( command == MOTOR_ON )
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, LOOPENABLE_CMD, m_Data);
	}
	else if( command == MOTOR_SPD_ON )
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, LOOPSPDENABLE_CMD, m_Data);
	}
	else
	{
		return MOTOR_ERROR;
	}

	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Reset the drive
RETURN:     n.u. left for compatibility
----------------------------------------------------------------------*/
int BrushlessModule::ResetDrive()
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, MICRORESET_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the encoder's actual position
RETURN:     Encoder actual position in IQ24
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoderActualPosition()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETENCODER_CMD, m_Data)) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Activate the encoder's motor control mode
INPUT:      mode: working mode
				  ENCODER_NORMAL: normal
				  ENCODER_INVERTED: inverted
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::EncoderMode( int type, int mode )
{
	if( type == ENCODER_QEP )
	{
		if( mode == ENCODER_NORMAL )
		{
			errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, SETENCODERQEPNORM_CMD, m_Data);
		}
		else
		{
			errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, SETENCODERQEPINV_CMD, m_Data);
		}
	}
	else // ENCODER_SER
	{
		if( mode == ENCODER_NORMAL )
		{
			errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, SETENCODERSERNORM_CMD, m_Data);
		}
		else
		{
			errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, SETENCODERSERINV_CMD, m_Data);
		}
	}

	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the encoder control mode
RETURN:     encoder mode (0: normal, 1: inverted), MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoderMode( int type )
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, (type == ENCODER_QEP) ? GETENCODERQEPMODE_CMD : GETENCODERSERMODE_CMD, m_Data );
	if( errorCode )
	{
		return MOTOR_ERROR;
	}

	return (int)m_Data[0];
}

/*----------------------------------------------------------------------
Set motion controllers inputs setting
INPUT:      val: inputs setting DWORD
				- Bytes[3..2]: CW limit switch input
				- Bytes[1..0]: CCW limit switch input
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::InputsSetting( int val )
{
	if( Write_Reg_Micro( 75, (val&0xFFFF0000)>>16) )
	{
		return MOTOR_ERROR;
	}

	if( Write_Reg_Micro( 76, (val&0x0000FFFF)) )
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
int BrushlessModule::SearchPos0( int dir )
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, HOMESEARCH_CMD, dir, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

//----------------------------------------------------------------------
// Function used to suspend driver during firmware update
// RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::SuspendDrive()
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, SUSPENDSERIAL_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Set Iq current PID params
INPUT:      PID coefficients
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPIDIqParams( int KP, int KI, int KD, int KC, int OMax, int OMin )
{
	// Setta il KP
	if( Write_Reg_Micro( 43, KP >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 44, KP & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KI
	if( Write_Reg_Micro( 45, KI >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 46, KI & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KD
	if( Write_Reg_Micro( 47, KD >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 48, KD & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KC
	if( Write_Reg_Micro( 49, KC >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 50, KC & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta OutMax
	if( Write_Reg_Micro( 51, OMax >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 52, OMax & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta OutMin
	if( Write_Reg_Micro( 53, OMin >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 54, OMin & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori del PID
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PIDIQUPDATE_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Get Iq current PID params
OUTPUT:     PID coefficients
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDIqParams( int* KP, int* KI, int* KD, int* KC, int* OMax, int* OMin )
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	// KP
	if (Read_Reg_Micro( 43, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 44, &valL))
	{
		return MOTOR_ERROR;
	}
	*KP = (valH << 16) + valL;

	// KI
	if (Read_Reg_Micro( 45, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 46, &valL))
	{
		return MOTOR_ERROR;
	}
	*KI = (valH << 16) + valL;

	// KD
	if (Read_Reg_Micro( 47, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 48, &valL))
	{
		return MOTOR_ERROR;
	}
	*KD = (valH << 16) + valL;

	// KC
	if (Read_Reg_Micro( 49, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 50, &valL))
	{
		return MOTOR_ERROR;
	}
	*KC = (valH << 16) + valL;

	// OutMax
	if (Read_Reg_Micro( 51, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 52, &valL))
	{
		return MOTOR_ERROR;
	}
	*OMax = (valH << 16) + valL;

	// OutMin
	if (Read_Reg_Micro( 53, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 54, &valL))
	{
		return MOTOR_ERROR;
	}
	*OMin = (valH << 16) + valL;

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Set Id current PID params
INPUT:      PID coefficients
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPIDIdParams( int KP, int KI, int KD, int KC, int OMax, int OMin )
{
	// Setta il KP
	if( Write_Reg_Micro( 31, KP >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 32, KP & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KI
	if( Write_Reg_Micro( 33, KI >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 34, KI & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KD
	if( Write_Reg_Micro( 35, KD >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 36, KD & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KC
	if( Write_Reg_Micro( 37, KC >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 38, KC & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta OutMax
	if( Write_Reg_Micro( 39, OMax >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 40, OMax & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta OutMin
	if( Write_Reg_Micro( 41, OMin >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 42, OMin & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori del PID
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PIDIDUPDATE_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Get Id current PID params
OUTPUT:     PID coefficients
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDIdParams( int* KP, int* KI, int* KD, int* KC, int* OMax, int* OMin )
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	// KP
	if (Read_Reg_Micro( 31, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 32, &valL))
	{
		return MOTOR_ERROR;
	}
	*KP = (valH << 16) + valL;

	// KI
	if (Read_Reg_Micro( 33, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 34, &valL))
	{
		return MOTOR_ERROR;
	}
	*KI = (valH << 16) + valL;

	// KD
	if (Read_Reg_Micro( 35, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 36, &valL))
	{
		return MOTOR_ERROR;
	}
	*KD = (valH << 16) + valL;

	// KC
	if (Read_Reg_Micro( 37, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 38, &valL))
	{
		return MOTOR_ERROR;
	}
	*KC = (valH << 16) + valL;

	// OutMax
	if (Read_Reg_Micro( 39, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 40, &valL))
	{
		return MOTOR_ERROR;
	}
	*OMax = (valH << 16) + valL;

	// OutMin
	if (Read_Reg_Micro( 41, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 42, &valL))
	{
		return MOTOR_ERROR;
	}
	*OMin = (valH << 16) + valL;

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Set speed PID params
INPUT:      PID set id and PID coefficients
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPIDSpdParams( unsigned short set, int KP, int KI, int KD, int KC, int OMax, int OMin )
{
	unsigned char KP_add, KI_add, KD_add, KC_add;

	switch( set )
	{
	case PIDSET_DEF:
		KP_add = 7;
		KI_add = 9;
		KD_add = 11;
		KC_add = 13;
		break;

	case PIDSET_0:
		KP_add = 83;
		KI_add = 85;
		KD_add = 87;
		KC_add = 89;
		break;

	case PIDSET_SPD:
		KP_add = 121;
		KI_add = 123;
		KD_add = 125;
		KC_add = 127;
		break;
			
	default:
		KP_add = 7;
		KI_add = 9;
		KD_add = 11;
		KC_add = 13;
		break;
	}

	// Setta il KP
	if( Write_Reg_Micro( KP_add, KP >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( KP_add+1, KP & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KI
	if( Write_Reg_Micro( KI_add, KI >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( KI_add+1, KI & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KD
	if( Write_Reg_Micro( KD_add, KD >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( KD_add+1, KD & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KC
	if( Write_Reg_Micro( KC_add, KC >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( KC_add+1, KC & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta OutMax
	if( Write_Reg_Micro( 15, OMax >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 16, OMax & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta OutMin
	if( Write_Reg_Micro( 17, OMin >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 18, OMin & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori del PID (se aggiornato il set per motore fermo...)
	if( set == PIDSET_0)
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PIDSPDUPDATE_CMD, m_Data);
		if( errorCode )
		{
			return MOTOR_ERROR;
		}
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetPIDSpdParams()
AUTHOR:     Daniele Belloni
INFO:       Function used to get speed PID params
INPUT:      PID set id
OUTPUT:     PID coefficients
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDSpdParams( unsigned short set, int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin )
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	unsigned char KP_add, KI_add, KD_add, KC_add;

	switch( set )
	{
	case PIDSET_DEF:
		KP_add = 7;
		KI_add = 9;
		KD_add = 11;
		KC_add = 13;
		break;

	case PIDSET_0:
		KP_add = 83;
		KI_add = 85;
		KD_add = 87;
		KC_add = 89;
		break;

	case PIDSET_SPD:
		KP_add = 121;
		KI_add = 123;
		KD_add = 125;
		KC_add = 127;
		break;

	default:
		KP_add = 7;
		KI_add = 9;
		KD_add = 11;
		KC_add = 13;
		break;
	}

	// KP
	if (Read_Reg_Micro( KP_add, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( KP_add+1, &valL))
	{
		return MOTOR_ERROR;
	}
	*KP = (valH << 16) + valL;

	// KI
	if (Read_Reg_Micro( KI_add, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( KI_add+1, &valL))
	{
		return MOTOR_ERROR;
	}
	*KI = (valH << 16) + valL;

	// KD
	if (Read_Reg_Micro( KD_add, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( KD_add+1, &valL))
	{
		return MOTOR_ERROR;
	}
	*KD = (valH << 16) + valL;

	// KC
	if (Read_Reg_Micro( KC_add, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( KC_add+1, &valL))
	{
		return MOTOR_ERROR;
	}
	*KC = (valH << 16) + valL;

	// OutMax
	if (Read_Reg_Micro( 15, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 16, &valL))
	{
		return MOTOR_ERROR;
	}
	*OMax = (valH << 16) + valL;

	// OutMin
	if (Read_Reg_Micro( 17, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 18, &valL))
	{
		return MOTOR_ERROR;
	}
	*OMin = (valH << 16) + valL;

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetPIDPosParams()
AUTHOR:     Daniele Belloni
INFO:       Function used to set position PID params
INPUT:      PID set id and PID coefficients
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPIDPosParams( unsigned short set, int KP, int KI, int KD, int KC, int OMax, int OMin )
{
	unsigned char KP_add, KI_add, KD_add, KC_add;

	switch( set )
	{
	case PIDSET_DEF:
		KP_add = 19;
		KI_add = 21;
		KD_add = 23;
		KC_add = 25;
		break;

	case PIDSET_0:
		KP_add = 91;
		KI_add = 93;
		KD_add = 95;
		KC_add = 97;
		break;

	default:
		KP_add = 19;
		KI_add = 21;
		KD_add = 23;
		KC_add = 25;
		break;
	}

	// Setta il KP
	if( Write_Reg_Micro( KP_add, KP >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( KP_add+1, KP & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KI
	if( Write_Reg_Micro( KI_add, KI >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( KI_add+1, KI & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KD
	if( Write_Reg_Micro( KD_add, KD >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( KD_add+1, KD & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta il KC
	if( Write_Reg_Micro( KC_add, KC >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( KC_add+1, KC & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta OutMax
	if( Write_Reg_Micro( 27, OMax >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 28, OMax & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

    // Setta OutMin
	if( Write_Reg_Micro( 29, OMin >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 30, OMin & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori del PID (se aggiornato il set per motore fermo...)
	if( set == PIDSET_0 )
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PIDPOSUPDATE_CMD, m_Data);
		if( errorCode )
		{
			return MOTOR_ERROR;
		}
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Get position PID params
INPUT:      PID set id
OUTPUT:     PID coefficients
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDPosParams( unsigned short set, int *KP, int *KI, int *KD, int *KC, int *OMax, int *OMin )
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	unsigned char KP_add, KI_add, KD_add, KC_add;

	switch( set )
	{
	case PIDSET_DEF:
		KP_add = 19;
		KI_add = 21;
		KD_add = 23;
		KC_add = 25;
		break;

	case PIDSET_0:
		KP_add = 91;
		KI_add = 93;
		KD_add = 95;
		KC_add = 97;
		break;

	default:
		KP_add = 19;
		KI_add = 21;
		KD_add = 23;
		KC_add = 25;
		break;
	}

	// KP
	if (Read_Reg_Micro( KP_add, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( KP_add+1, &valL))
	{
		return MOTOR_ERROR;
	}
	*KP = (valH << 16) + valL;

	// KI
	if (Read_Reg_Micro( KI_add, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( KI_add+1, &valL))
	{
		return MOTOR_ERROR;
	}
	*KI = (valH << 16) + valL;

	// KD
	if (Read_Reg_Micro( KD_add, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( KD_add+1, &valL))
	{
		return MOTOR_ERROR;
	}
	*KD = (valH << 16) + valL;

	// KC
	if (Read_Reg_Micro( KC_add, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( KC_add+1, &valL))
	{
		return MOTOR_ERROR;
	}
	*KC = (valH << 16) + valL;

	// OutMax
	if (Read_Reg_Micro( 27, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 28, &valL))
	{
		return MOTOR_ERROR;
	}
	*OMax = (valH << 16) + valL;

	// OutMin
	if (Read_Reg_Micro( 29, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 30, &valL))
	{
		return MOTOR_ERROR;
	}
	*OMin = (valH << 16) + valL;

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetLineEncoder()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's encoder line number
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor line encoder
----------------------------------------------------------------------*/
int BrushlessModule::GetLineEncoder()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	if (Read_Reg_Micro( 55, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 56, &valL))
	{
		return MOTOR_ERROR;
	}

	return (valH << 16) + valL;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetLineEncoder( int lines )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor's encoder line number
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetLineEncoder( int lines )
{
	if( Write_Reg_Micro( 55, lines >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 56, lines & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, LINEENCUPDATE_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetPulsesPerRev()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's encoder pulses per revolution
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor encoder pulses per revolution
----------------------------------------------------------------------*/
int BrushlessModule::GetPulsesPerRev()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	if( Read_Reg_Micro( 138, &valH) )
	{
		return MOTOR_ERROR;
	}
	if( Read_Reg_Micro( 139, &valL) )
	{
		return MOTOR_ERROR;
	}

	return (valH << 16) + valL;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetPulsesPerRev( int ppr )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor's encoder pulses per revolution
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPulsesPerRev( int ppr )
{
	if( Write_Reg_Micro( 138, ppr >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 139, ppr & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PPRUPDATE_CMD, m_Data);
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the motor's poles
RETURN:     Motor encoder poles
----------------------------------------------------------------------*/
int BrushlessModule::GetPoles()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 58, &val))
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
Set the motor's poles
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPoles( int poles )
{
	if( Write_Reg_Micro( 58, poles) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, ENCPARAMUPDATE_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the motor's calibrated angle
RETURN:     Motor calibrated angle
----------------------------------------------------------------------*/
int BrushlessModule::GetCalibratedAngle()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 59, &val))
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
Set the motor's calibrated angle
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetCalibratedAngle( int angle )
{
	if( Write_Reg_Micro( 59, angle) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, CALANGUPDATE_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the logger trigger level
RETURN:     Logger trigger
----------------------------------------------------------------------*/
int BrushlessModule::GetLoggerTrigger()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 60, &val))
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetLoggerTrigger( int trig )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the logger trigger level
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetLoggerTrigger( int trig )
{
	if( Write_Reg_Micro( 60, trig) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, LOGTRIGUPDATE_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetLoggerPrescaler()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the logger prescaler
OUTPUT:     -
GLOBAL:     -
RETURN:     Logger prescaler
----------------------------------------------------------------------*/
int BrushlessModule::GetLoggerPrescaler()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 61, &val))
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetLoggerPrescaler( int prescal )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the logger prescaler
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetLoggerPrescaler( int prescal )
{
	if( Write_Reg_Micro( 61, prescal) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, LOGPRESCALUPDATE_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetLoggerChannelData()
AUTHOR:     Daniele Belloni
INFO:       Function used to get the logger channel data
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::GetLoggerChannelData( int channel )
{
	unsigned char cmd1, cmd2;
	switch( channel )
	{
	case LOG_CH1:
		cmd1 = GETLOG1L_CMD;
		cmd2 = GETLOG1H_CMD;
		break;

	case LOG_CH2:
		cmd1 = GETLOG2L_CMD;
		cmd2 = GETLOG2H_CMD;
		break;

	case LOG_CH3:
		cmd1 = GETLOG3L_CMD;
		cmd2 = GETLOG3H_CMD;
		break;

	case LOG_CH4:
		cmd1 = GETLOG4L_CMD;
		cmd2 = GETLOG4H_CMD;
		break;

	default:
		return MOTOR_ERROR;
		break;
	}

	// Legge la parte bassa del buffer
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, cmd1, m_Data )) )
	{
		return MOTOR_ERROR;
	}
	for( int i=0; i<50; i++ )
	{
		bufDataLog[channel][i] = (m_Data[i*2]<<8) + m_Data[i*2+1];
	}

	// Legge la parte alta del buffer
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, cmd2, m_Data )) )
	{
		return MOTOR_ERROR;
	}
	for( int i=0; i<50; i++ )
	{
		bufDataLog[channel][i+50] = (m_Data[i*2]<<8) + m_Data[i*2+1];
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Set the logger channels input
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetLoggerChannelInput( int channel, int input )
{
	if( Write_Reg_Micro( 62+channel, input) )
	{
		return MOTOR_ERROR;
	}
	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the encoder's actual speed
INPUT:      mode: action
                     SPEED_PU:  speed in p.u.
                     SPEED_RPM: speed in RPM
                     SPEED_MS:  speed in m/s
RETURN:     Encoder actual speed
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoderActualSpeed( int mode )
{
	int val;

	if( mode == SPEED_PU )
	{
		if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETSPEEDPU_CMD, m_Data )) )
		{
			return MOTOR_ERROR;
		}
	}
	else if( mode == SPEED_RPM )
	{
		if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETSPEEDRPM_CMD, m_Data )) )
		{
			return MOTOR_ERROR;
		}
	}
	else if( mode == SPEED_MS )
	{
		if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETSPEEDMS_CMD, m_Data )) )
		{
			return MOTOR_ERROR;
		}
	}
	val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];

	return val;
}

/*----------------------------------------------------------------------
Enable motor PWM generation
INPUT:      command: action
                     PWM_OFF:    disable pwm
                     PWM_ON:     enable pwm
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::PWMEnable( int command )
{
	if (command == PWM_OFF)
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PWMDISABLE_CMD, m_Data );
	}
	else if (command == PWM_ON)
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PWMENABLE_CMD, m_Data );
	}

	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Enable single motor phase
INPUT:      phase:	 phase to activate/deactivate
			command: action
                     PWM_OFF:    disable pwm
                     PWM_ON:     enable pwm
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::PhaseEnable( int phase, int command )
{
	if (command == PWM_OFF)
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PHASEDISABLE_CMD, phase, m_Data );
	}
	else if (command == PWM_ON)
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PHASEENABLE_CMD, phase, m_Data );
	}

	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Enable ADC Calibration mode
INPUT:      command: action
                     ADCCAL_OFF:    disable ADC cal
                     ADCCAL_ON:     enable ADC cal
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::ADCCalEnable( int command )
{
	if (command == ADCCAL_OFF)
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, ADCCALDISABLE_CMD, m_Data );
	}
	else if (command == ADCCAL_ON)
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, ADCCALENABLE_CMD, m_Data );
	}

	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the Clark module As input
RETURN:     Clarke As component in IQ24
----------------------------------------------------------------------*/
int BrushlessModule::GetClarkeAs()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETCLARKEA_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}
	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];

	return val;
}

/*----------------------------------------------------------------------
Read the Clark module Bs input
RETURN:     Clarke Bs component in IQ24
----------------------------------------------------------------------*/
int BrushlessModule::GetClarkeBs()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETCLARKEB_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}
	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetADC0Offset( int channel )
AUTHOR:     Daniele Belloni
INFO:       Function used to get ADC offset
INPUT:      channel: action
                     ADC_CHANNEL0:  read ADC channel 0
                     ADC_CHANNEL1:  read ADC channel 1
OUTPUT:     -
GLOBAL:     -
RETURN:     ADCx offset
----------------------------------------------------------------------*/
int BrushlessModule::GetADCOffset( int channel )
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if( channel == ADC_CHANNEL0 )
	{
		if (Read_Reg_Micro( 66, &valH))
		{
			return MOTOR_ERROR;
		}
		if (Read_Reg_Micro( 67, &valL))
		{
			return MOTOR_ERROR;
		}
	}
	else if( channel == ADC_CHANNEL1 )
	{
		if (Read_Reg_Micro( 68, &valH))
		{
			return MOTOR_ERROR;
		}
		if (Read_Reg_Micro( 69, &valL))
		{
			return MOTOR_ERROR;
		}
	}
	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
Set ADC offset
INPUT:      channel: action
                     ADC_CHANNEL0:  set ADC channel 0
                     ADC_CHANNEL1:  set ADC channel 1
		    val: value to assign to ADC channel
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetADCOffset( int channel, int val )
{
	if( channel == ADC_CHANNEL0 )
	{
		if( Write_Reg_Micro( 66, val >> 16) )
		{
			return MOTOR_ERROR;
		}
		if( Write_Reg_Micro( 67, val & 0xFFFF) )
		{
			return MOTOR_ERROR;
		}
	}
	else if( channel == ADC_CHANNEL1 )
	{
		if( Write_Reg_Micro( 68, val >> 16) )
		{
			return MOTOR_ERROR;
		}
		if( Write_Reg_Micro( 69, val & 0xFFFF) )
		{
			return MOTOR_ERROR;
		}
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Get PID Iq out (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDIqOut()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETPIDIQOUT_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Get PID Id out (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDIdOut()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETPIDIDOUT_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Get PID speed out (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDSpdOut()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETPIDSPDOUT_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Get PID position out (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDPosOut()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETPIDPOSOUT_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Get PID Iq feedback (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDIqFdb()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETPIDIQFDB_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Get PID Id feedback (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDIdFdb()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETPIDIDFDB_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Get PID speed feedback (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDSpdFdb()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETPIDSPDFDB_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Get PID position feedback (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDPosFdb()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETPIDPOSFDB_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Get Iq reference current (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetIqRef()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if (Read_Reg_Micro( 70, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 71, &valL))
	{
		return MOTOR_ERROR;
	}

	int val = (valH << 16) + valL;
	return val;
}

/*----------------------------------------------------------------------
Set Iq reference
INPUT:      val: value to assign to Iq reference
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetIqRef( int val )
{
	if( Write_Reg_Micro( 70, val >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 71, val & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Get Id reference current (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetIdRef()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if (Read_Reg_Micro( 72, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 73, &valL))
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
Set Id reference
INPUT:      val: value to assign to Id reference
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetIdRef( int val )
{
	if( Write_Reg_Micro( 72, val >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 73, val & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Get the encoder type
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoderType()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETENCODERTYPE_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	return (int)m_Data[0];
}

/*----------------------------------------------------------------------
Set the encoder type
INPUT:      type: action
                     ROTATIVE_ENC:  rotative encoder
                     LINEAR_ENC:    linear encoder
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetEncoderType( int type )
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, SETENCODERTYPE_CMD, type, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Activate/deactivate the encoder's elec theta generation
INPUT:      mode: working mode 
				  ELECTTHETA_ON
				  ELECTTHETA_OFF
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::ElecThetaGeneration( int mode )
{
	if( mode == ELECTTHETA_ON )
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, ENCODERENABLE_CMD, m_Data );
	}
	else
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, ENCODERDISABLE_CMD, m_Data );
	}

	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetMaxSpeed()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's max speed
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor max speed (RPM)
----------------------------------------------------------------------*/
int BrushlessModule::GetMaxSpeed()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 78, &val))
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
Set the motor's max speed (in RPM)
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetMaxSpeed( int spd )
{
	if( Write_Reg_Micro( 78, spd) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, ENCPARAMUPDATE_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the acc/dec ramps mode (n.u.)
RETURN:     Motor acc/dec ramp mode
----------------------------------------------------------------------*/
int BrushlessModule::GetRamps()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 77, &val))
	{
		return MOTOR_ERROR;
	}

	return (val>>MODE_RAMP)&0x0001;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetRamps( int mode )
AUTHOR:     Daniele Belloni
INFO:       Function used to activate/deactivate the acc/dec ramps (n.u.)
INPUT:      mode: working mode 
				  RAMP_ON 
				  RAMP_OFF
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetRamps( int mode )
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 77, &val))
	{
		return MOTOR_ERROR;
	}

	if( mode == RAMP_ON )
	{
		val &= !1<<MODE_RAMP;
	}
	else
	{
		val |= 1<<MODE_RAMP;
	}

	if( Write_Reg_Micro( 77, val) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ActualStartStopVelocity()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's actual setted start/stop velocity
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor actual setted start/stop velocity, -1 if errors
----------------------------------------------------------------------*/
int BrushlessModule::ActualStartStopVelocity()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	if (Read_Reg_Micro( 79, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 80, &valL))
	{
		return MOTOR_ERROR;
	}

	return (valH << 16) + valL;
}

/*----------------------------------------------------------------------
FUNCTION:   int ActualAceleration()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor aceleration.
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     aceleration (in m/s), -1 if errors
----------------------------------------------------------------------*/
int BrushlessModule::ActualAceleration()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	if (Read_Reg_Micro( 81, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 82, &valL))
	{
		return MOTOR_ERROR;
	}

	return (valH << 16) + valL;
}

/*----------------------------------------------------------------------
FUNCTION:   int ActualDeceleration()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor deceleration.
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     deceleration (in m/s), -1 if errors
----------------------------------------------------------------------*/
int BrushlessModule::ActualDeceleration()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	if (Read_Reg_Micro( 103, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 104, &valL))
	{
		return MOTOR_ERROR;
	}

	return (valH << 16) + valL;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetActualPIDSpd( unsigned short set )
AUTHOR:     Daniele Belloni
INFO:       Function used to set actual speed PID
INPUT:      PID set id
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetActualPIDSpd( unsigned short set )
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, SETACTPIDSPD_CMD, set, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetActualPIDPos( unsigned short set )
AUTHOR:     Daniele Belloni
INFO:       Function used to set actual position PID
INPUT:      PID set id
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetActualPIDPos( unsigned short set )
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, SETACTPIDPOS_CMD, set, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int LoggerTriggerMode( int mode )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the trigger level
INPUT:      mode: working mode 
				  LOWTOHIGH_TRIG
				  HIGHTOLOW_TRIG
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::LoggerTriggerMode( int mode )
{
	if( mode == LOWTOHIGH_TRIG )
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, LOWTOHIGHTRIG_CMD, m_Data );
	}
	else
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, HIGHTOLOWTRIG_CMD, m_Data );
	}

	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetEncoderActualPulses()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the encoder's counter register
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     Encoder actual counter register
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoderActualPulses()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETENCODERPULSES_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetPhaseCurrent( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set phase current (used during phases generation deactivation)
INPUT:      val: phase current value
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPhaseCurrent( int val )
{
	if( Write_Reg_Micro( 99, val >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 100, val & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetPhaseCurrent()
AUTHOR:     Daniele Belloni
INFO:       Function used to get phase current
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     phase current
----------------------------------------------------------------------*/
int BrushlessModule::GetPhaseCurrent()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if (Read_Reg_Micro( 99, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 100, &valL))
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetDecelerationAdvance()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's deceleration advance
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor's deceleration advance (%)
----------------------------------------------------------------------*/
int BrushlessModule::GetDecelerationAdvance()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 101, &val))
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetDecelerationAdvance( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor's deceleration advance (in %)
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetDecelerationAdvance( int val )
{
	if( Write_Reg_Micro( 101, val) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetPulsesToMm()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's encoder pulses to mm constant
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor encoder pulses per revolution
----------------------------------------------------------------------*/
int BrushlessModule::GetPulsesToMm()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 102, &val))
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetPulsesToMm( int ptm )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor's encoder pulses to mm constant
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPulsesToMm( int ptm )
{
	if( Write_Reg_Micro( 102, ptm) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PTMUPDATE_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int DecCompensationMode( int mode )
AUTHOR:     Daniele Belloni
INFO:       Function used to activate/deactivate the deceleration compensation mode
INPUT:      mode: working mode 
				  DECCOMP_ON
				  DECCOMP_OFF
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::DecCompensationMode( int mode )
{
	if( mode == DECCOMP_ON )
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, DECCOMPENABLE_CMD, m_Data );
	}
	else
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, DECCOMPDISABLE_CMD, m_Data );
	}

	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetProximityGap( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set proximity gap
INPUT:      val: proximity value in p.u.
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetProximityGap( int val )
{
	if( Write_Reg_Micro( 105, val >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 106, val & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetProximityGap()
AUTHOR:     Daniele Belloni
INFO:       Function used to get proximity gap
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     proximity gap in p.u.
----------------------------------------------------------------------*/
int BrushlessModule::GetProximityGap()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if (Read_Reg_Micro( 105, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 106, &valL))
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetPIDPosActivationGap( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set PID Pos activation gap
INPUT:      val: PID Pos activation gap value in p.u.
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPIDPosActivationGap( int val )
{
	if( Write_Reg_Micro( 107, val >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 108, val & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetPIDPosActivationGap()
AUTHOR:     Daniele Belloni
INFO:       Function used to get PID Pos activation gap
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     PID Pos activation gap in p.u.
----------------------------------------------------------------------*/
int BrushlessModule::GetPIDPosActivationGap()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if (Read_Reg_Micro( 107, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 108, &valL))
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetIParkAngle( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set iPark angle during calibration
INPUT:      val: iPark angle value in p.u.
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetIParkAngle( int val )
{
	if( Write_Reg_Micro( 109, val >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 110, val & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetIParkAngle()
AUTHOR:     Daniele Belloni
INFO:       Function used to get iPark angle during calibration
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     iPark angle in p.u.
----------------------------------------------------------------------*/
int BrushlessModule::GetIParkAngle()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if (Read_Reg_Micro( 109, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 110, &valL))
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetDangerLimit( int limit )
AUTHOR:     Daniele Belloni
INFO:       Function used to get the danger limits
INPUT:      limit: action
                     LOW:  low danger limit
                     HIGH: high danger limit
OUTPUT:     -
GLOBAL:     -
RETURN:     Danger limits
----------------------------------------------------------------------*/
int BrushlessModule::GetDangerLimit( int limit )
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if( limit == LOW )
	{
		if (Read_Reg_Micro( 111, &valH))
		{
			return MOTOR_ERROR;
		}
		if (Read_Reg_Micro( 112, &valL))
		{
			return MOTOR_ERROR;
		}
	}
	else
	{
		if (Read_Reg_Micro( 113, &valH))
		{
			return MOTOR_ERROR;
		}
		if (Read_Reg_Micro( 114, &valL))
		{
			return MOTOR_ERROR;
		}
	}
	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetDangerLimit( int limit, int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the danger limits
INPUT:      limit: action
                     LOW:  low danger limit
                     HIGH: high danger limit
		    val: value to assign to the selected limit
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetDangerLimit( int limit, int val )
{
	if( limit == LOW )
	{
		if( Write_Reg_Micro( 111, val >> 16) )
		{
			return MOTOR_ERROR;
		}
		if( Write_Reg_Micro( 112, val & 0xFFFF) )
		{
			return MOTOR_ERROR;
		}
	}
	else
	{
		if( Write_Reg_Micro( 113, val >> 16) )
		{
			return MOTOR_ERROR;
		}
		if( Write_Reg_Micro( 114, val & 0xFFFF) )
		{
			return MOTOR_ERROR;
		}
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int DangerLimitMode( int mode )
AUTHOR:     Daniele Belloni
INFO:       Function used to activate/deactivate the danger limits
INPUT:      mode: working mode 
				  DANGERLIMIT_ON
				  DANGERLIMIT_OFF
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::DangerLimitMode( int mode )
{
	if( mode == DANGERLIMIT_ON )
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, DANGERENABLE_CMD, m_Data );
	}
	else
	{
		errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, DANGERDISABLE_CMD, m_Data );
	}

	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ResetEncoder()
AUTHOR:     Daniele Belloni
INFO:       Function used to reset the motor's encoder position
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::ResetEncoder()
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, RESETENCODER_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
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
int BrushlessModule::HomeSensorInput( int val )
{
	if( Write_Reg_Micro( 115, val ) )
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
int BrushlessModule::GetHomingSpeed( int speed )
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if( speed == HOME_SLOW )
	{
		if (Read_Reg_Micro( 118, &valH))
		{
			return MOTOR_ERROR;
		}
		if (Read_Reg_Micro( 119, &valL))
		{
			return MOTOR_ERROR;
		}
	}
	else
	{
		if (Read_Reg_Micro( 116, &valH))
		{
			return MOTOR_ERROR;
		}
		if (Read_Reg_Micro( 117, &valL))
		{
			return MOTOR_ERROR;
		}
	}
	val = (valH << 16) + valL;

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
int BrushlessModule::SetHomingSpeed( int speed, int val )
{
	if( speed == HOME_SLOW )
	{
		if( Write_Reg_Micro( 118, val >> 16) )
		{
			return MOTOR_ERROR;
		}
		if( Write_Reg_Micro( 119, val & 0xFFFF) )
		{
			return MOTOR_ERROR;
		}
	}
	else
	{
		if( Write_Reg_Micro( 116, val >> 16) )
		{
			return MOTOR_ERROR;
		}
		if( Write_Reg_Micro( 117, val & 0xFFFF) )
		{
			return MOTOR_ERROR;
		}
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetOverSpeed()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's speed overspeed percentage
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor's speed overspeed (%)
----------------------------------------------------------------------*/
int BrushlessModule::GetOverSpeed()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short val = 0;
	if (Read_Reg_Micro( 120, &val))
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetOverSpeed( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the motor's speed overspeed percentage
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetOverSpeed( int val )
{
	if( Write_Reg_Micro( 120, val) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetReferenceActualPos()
AUTHOR:     Daniele Belloni
INFO:       Function used to set actual position as the reference one
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetReferenceActualPos()
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, SETREFACTUALPOS_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ResetPIDSpd()
AUTHOR:     Daniele Belloni
INFO:       Function used to reset speed PID output.
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::ResetPIDSpd()
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PIDSPDRESET_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int ResetPIDPos()
AUTHOR:     Daniele Belloni
INFO:       Function used to reset position PID output.
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::ResetPIDPos()
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, PIDPOSRESET_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int PIDStatus()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the PIDs status
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     PIDs status, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::PIDStatus()
{
	unsigned short val = 0;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	if( Read_Reg_Micro( 133, &val ) )
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetMaxReadCurrent()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the board max read current
OUTPUT:     -
GLOBAL:     -
RETURN:     Board max current (mA), MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetMaxReadCurrent()
{
	unsigned short val = 0;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	if (Read_Reg_Micro( 134, &val))
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetMaxReadCurrent( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the board max read current (mA)
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK if OK, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::SetMaxReadCurrent( int val )
{
	if( Write_Reg_Micro( 134, val) )
	{
		return MOTOR_ERROR;
	}

	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, UPDATECURRENT_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetPhaseC()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the phase C current
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     phase C current in IQ24, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetPhaseC()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETPHASEC_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetEncoder360ActualPosition()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the encoder's actual position in 360 degrees
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     Encoder actual position in IQ24, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoder360ActualPosition()
{
	int val;

	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, GETENCODER360_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}
	val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetElecTheta()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the encoder's electrical theta
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     Encoder electrical theta in IQ24, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetElecTheta()
{
	int val;

	if( TWSBus1_Send_Command_HD( m_comInt, m_Address, GETELECTHETA_CMD, m_Data ) )
	{
		return MOTOR_ERROR;
	}
	val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];

	return val;
}

//----------------------------------------------------------------------
// FUNCTION:   GetAccFFWD
// INFO:       Function used to get the acc feed-forward
// RETURN:     acc feed-forward (IQ24) if OK, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::GetAccFFWD()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if( Read_Reg_Micro( 147, &valH ) )
	{
		return MOTOR_ERROR;
	}
	if( Read_Reg_Micro( 148, &valL ) )
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

//----------------------------------------------------------------------
// FUNCTION:   SetAccFFWD
// INFO:       Function used to set the acc feed-forward
// INPUT:      val: acc feed-forward (IQ24)
// RETURN:     MOTOR_OK if OK, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::SetAccFFWD( int val )
{
	if( Write_Reg_Micro( 147, val >> 16 ) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 148, val & 0xFFFF ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

//----------------------------------------------------------------------
// FUNCTION:   GetSpdFFWD
// INFO:       Function used to get the speed feed-forward
// RETURN:     speed feed-forward (IQ24) if OK, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::GetSpdFFWD()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if( Read_Reg_Micro( 149, &valH ) )
	{
		return MOTOR_ERROR;
	}
	if( Read_Reg_Micro( 150, &valL ) )
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

//----------------------------------------------------------------------
// FUNCTION:   SetSpdFFWD
// INFO:       Function used to set the speed feed-forward
// INPUT:      val: speed feed-forward (IQ24)
// RETURN:     MOTOR_OK if OK, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::SetSpdFFWD( int val )
{
	if( Write_Reg_Micro( 149, val >> 16 ) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 150, val & 0xFFFF ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

//----------------------------------------------------------------------
// FUNCTION:   GetMaxJerk
// INFO:       Function used to get the maximum jerk
// RETURN:     maximum jerk if OK, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::GetMaxJerk()
{
	unsigned short val = 0;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	if( Read_Reg_Micro( 144, &val) )
	{
		return MOTOR_ERROR;
	}

	return val;
}

//----------------------------------------------------------------------
// FUNCTION:   SetMaxJerk
// INFO:       Function used to set the maximum jerk
// INPUT:      val: maximum jerk
// RETURN:     MOTOR_OK if OK, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::SetMaxJerk( int val )
{
	if( Write_Reg_Micro( 144, val) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}


//----------------------------------------------------------------------
// FUNCTION:   GetCFriction
// INFO:       Function used to get Coulomb friction constant
// RETURN:     Coulomb friction constant (IQ24) if OK, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::GetCFriction()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if( Read_Reg_Micro( 153, &valH ) )
	{
		return MOTOR_ERROR;
	}
	if( Read_Reg_Micro( 154, &valL ) )
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

//----------------------------------------------------------------------
// FUNCTION:   SetCFriction
// INFO:       Function used to set Coulomb friction constant
// INPUT:      val: Coulomb friction constant (IQ24)
// RETURN:     MOTOR_OK if OK, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::SetCFriction( int val )
{
	if( Write_Reg_Micro( 153, val >> 16 ) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 154, val & 0xFFFF ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetEndMovementSpeed()
AUTHOR:     Daniele Belloni
INFO:       Function used to get the speed for end movement
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     speed in m/s IQ24 if OK, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetEndMovementSpeed()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if( Read_Reg_Micro( 145, &valH ) )
	{
		return MOTOR_ERROR;
	}
	if( Read_Reg_Micro( 146, &valL ) )
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetEndMovementSpeed( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the speed for end movement
INPUT:      val: speed in m/s IQ24
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK if OK, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::SetEndMovementSpeed( int val )
{
	if( Write_Reg_Micro( 145, val >> 16 ) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 146, val & 0xFFFF ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetEndMovementDelta()
AUTHOR:     Daniele Belloni
INFO:       Function used to get the encoder delta for end movement
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     delta in pulses if OK, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetEndMovementDelta()
{
	unsigned short val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if( Read_Reg_Micro( 142, &valH ) )
	{
		return MOTOR_ERROR;
	}
	if( Read_Reg_Micro( 143, &valL ) )
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetEndMovementDelta( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the encoder delta for end movement
INPUT:      val: delta in pulses
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK if OK, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::SetEndMovementDelta( int val )
{
	if( Write_Reg_Micro( 142, val >> 16 ) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 143, val & 0xFFFF ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

//----------------------------------------------------------------------
// Mmove multiple motors (absolute motion) with specified speed/acc
// INPUT:      addX, posX: address and absolute position for each motor
//             maxFreq: movement speed
//             acc: movement acceleration
//             posXnorm: X normalized position (double encoder)
// RETURN:     MOTOR_OK if OK, MOTOR_ERROR otherwise
//----------------------------------------------------------------------
int BrushlessModule::GotoPos0_Multi( unsigned char addX, int posX, unsigned char addY, int posY, int maxFreq, int acc, int proximityDeltaX, int proximityDeltaY )
{
	unsigned char tx_buffer[26];
	tx_buffer[0] = addX; // X slave address
	tx_buffer[1] = posX >> 24; // X position
	tx_buffer[2] = posX >> 16;
	tx_buffer[3] = posX >> 8;
	tx_buffer[4] = posX;
	tx_buffer[5] = proximityDeltaX >> 24; // X proximity delta
	tx_buffer[6] = proximityDeltaX >> 16;
	tx_buffer[7] = proximityDeltaX >> 8;
	tx_buffer[8] = proximityDeltaX;
	tx_buffer[9] = addY; // Y slave address
	tx_buffer[10] = posY >> 24; // Y position
	tx_buffer[11] = posY >> 16;
	tx_buffer[12] = posY >> 8;
	tx_buffer[13] = posY;
	tx_buffer[14] = proximityDeltaY >> 24; // Y proximity delta
	tx_buffer[15] = proximityDeltaY >> 16;
	tx_buffer[16] = proximityDeltaY >> 8;
	tx_buffer[17] = proximityDeltaY;
	tx_buffer[18] = maxFreq >> 24; // speed
	tx_buffer[19] = maxFreq >> 16;
	tx_buffer[20] = maxFreq >> 8;
	tx_buffer[21] = maxFreq;
	tx_buffer[22] = acc >> 24; // acceleration
	tx_buffer[23] = acc >> 16;
	tx_buffer[24] = acc >> 8;
	tx_buffer[25] = acc;


	errorCode = TWSBus1_Send_HD( m_comInt, m_Address, MULTIMOVE_CMD, tx_buffer, 26 );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Reset multiple motors command flag
RETURN:     MOTOR_OK if OK, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::ResetFlag_Multi()
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, RESET_MULTIMOVE_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the encoder's actual position (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoder2ActualPosition()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETENCODER2_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Read the encoder's counter register
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoder2ActualPulses()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETENCODER2PULSES_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetLineEncoder2()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the motor's encoder line number
OUTPUT:     -
GLOBAL:     -
RETURN:     Motor line encoder
----------------------------------------------------------------------*/
int BrushlessModule::GetLineEncoder2()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;
	if (Read_Reg_Micro( 140, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 141, &valL))
	{
		return MOTOR_ERROR;
	}

	return (valH << 16) + valL;
}

/*----------------------------------------------------------------------
Set the motor's encoder line number
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetLineEncoder2( int lines )
{
	if( Write_Reg_Micro( 140, lines >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 141, lines & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	// Aggiorna i valori
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, LINEENC2UPDATE_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Read the encoder's actual position (in IQ24)
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoderInterpActualPosition()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETENCODERINTERP_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Read the motor's status and reset multimove flag
RETURN:     Motor status, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetStatus_ResetFlagMulti()
{
	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETSTATUS_RESETMM_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<8) + m_Data[1];
	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetFollowingError( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set following error
INPUT:      val: following error value in p.u.
OUTPUT:     -
GLOBAL:     -
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetFollowingError( int val )
{
	if( Write_Reg_Micro( 162, val >> 16 ) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 163, val & 0xFFFF ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int GetFollowingError()
AUTHOR:     Daniele Belloni
INFO:       Function used to get following error
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     following error in p.u.
----------------------------------------------------------------------*/
int BrushlessModule::GetFollowingError()
{
	int val;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	unsigned short valH = 0, valL = 0;

	if( Read_Reg_Micro( 162, &valH ) )
	{
		return MOTOR_ERROR;
	}
	if( Read_Reg_Micro( 163, &valL ) )
	{
		return MOTOR_ERROR;
	}

	val = (valH << 16) + valL;

	return val;
}

/*----------------------------------------------------------------------
FUNCTION:   int EncoderStatus()
AUTHOR:     Daniele Belloni
INFO:       Function used to read the encoder status
INPUT:      -
OUTPUT:     -
GLOBAL:     -
RETURN:     encoder status, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::EncoderStatus()
{
	unsigned short val = 0;

	if( m_Address == 0 )
	{
		return MOTOR_ERROR;
	}

	if( Read_Reg_Micro( 164, &val ) )
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
Set the start ticks
INPUT:      ticks: start ticks
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetStartTicks( int ticks )
{
	if( Write_Reg_Micro( 2, ticks & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Set the phase rotation
INPUT:      value: phase rotation
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPhaseRotation( int value )
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, ROTATEPHASE_CMD, value, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetPhaseRotationNumber( int value )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the phase rotation number
INPUT:      value: phase rotation number
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetPhaseRotationNumber( int value )
{
	if( Write_Reg_Micro( 57, value ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Set the speed filter window
INPUT:      window: speed filter window in ms
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetSpeedFilter( unsigned short window )
{
	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, UPDATESPDFILTER_CMD, window, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
Set the motor security check.
INPUT:      limitLevel: Set security check level high or low (SECURITYLEVEL_HIGH,
                        SECURITYLEVEL_LOW)
            limit: Activate or deactivate security check (SECURITYCHECK_ON,
                   SECURITYCHECK_OFF)
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetSecurityCheck( int limit, int limitLevel )
{
	unsigned short val = 0;

	// Read mode register
	if( Read_Reg_Micro( 77, &val) )
	{
		return MOTOR_ERROR;
	}

	if( limit == SECURITYCHECK_ON )
	{
		// Setta il flag di check limiti a true
		val |= (1<<MODE_SECURITY);
	}
	else
	{
		// Setta il flag di check limiti a false
		val &= ~(1<<MODE_SECURITY);
	}

	if( limitLevel == SECURITYLEVEL_HIGH )
	{
		// Setta il livello di check limiti alto
		val |= (1<<MODE_SECURITYLEVEL);
	}
	else
	{
		// Setta il livello di check limiti basso
		val &= ~(1<<MODE_SECURITYLEVEL);
	}

	// Write mode register
	if( Write_Reg_Micro( 77, val) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
FUNCTION:   int SetSecurityInput( int val )
AUTHOR:     Daniele Belloni
INFO:       Function used to set the security check input
INPUT:      val:
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetSecurityInput( int val )
{
	if( Write_Reg_Micro( 166, (val&0x0000FFFF)) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
Get the supplied voltage
RETURN:     Voltage if OK, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::GetVPower()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETVPWR_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<24) + (m_Data[1]<<16) + (m_Data[2]<<8) + m_Data[3];
	return val;
}

/*----------------------------------------------------------------------
Get the sensor temperature
RETURN:     Temperature if OK, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::GetSensorTemp()
{
	if( (errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, GETSENSORTEMP_CMD, m_Data )) )
	{
		return MOTOR_ERROR;
	}

	int val = (m_Data[0]<<8) + m_Data[1];
	return val;
}

/*----------------------------------------------------------------------
INFO:       Function used to set the steady position error
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetSteadyPosition( int pos )
{
	if( Write_Reg_Micro( 167, pos >> 16) )
	{
		return MOTOR_ERROR;
	}
	if( Write_Reg_Micro( 168, pos & 0xFFFF) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
INFO:       Function used to get the steady position error
RETURN:     Current delta if ok, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetSteadyPosition()
{
	unsigned short valH = 0, valL = 0;
	if (Read_Reg_Micro( 167, &valH))
	{
		return MOTOR_ERROR;
	}
	if (Read_Reg_Micro( 168, &valL))
	{
		return MOTOR_ERROR;
	}

	return (valH << 16) + valL;
}

/*----------------------------------------------------------------------
INFO:       Function used to set the encoder spike delta
RETURN:     MOTOR_OK on success, MOTOR_ERROR otherwise
----------------------------------------------------------------------*/
int BrushlessModule::SetEncoderSpikeDelta( unsigned short delta )
{
	if( Write_Reg_Micro( 169, delta ) )
	{
		return MOTOR_ERROR;
	}

	return MOTOR_OK;
}

/*----------------------------------------------------------------------
INFO:       Function used to get the encoder spike delta
RETURN:     Current delta if ok, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::GetEncoderSpikeDelta()
{
	unsigned short val = 0;

	if( Read_Reg_Micro( 169, &val ) )
	{
		return MOTOR_ERROR;
	}

	return val;
}

/*----------------------------------------------------------------------
INFO:       Function used to update the overspeed check value
RETURN:     MOTOR_OK if ok, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::UpdateOverSpeed()
{
	// controllo versione
	if( ver < 4 || ( ver == 4 && rev < 15 ) )
	{
		return MOTOR_OK;
	}

	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, UPDATEOVERSPEED_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
INFO:       Function used to Enable/disable the overspeed check
RETURN:     MOTOR_OK if ok, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::OverSpeedCheck( int enable )
{
	// controllo versione
	if( ver < 4 || ( ver == 4 && rev < 15 ) )
	{
		return MOTOR_OK;
	}

	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, OVERSPEEDCHECK_CMD, enable, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}

/*----------------------------------------------------------------------
INFO:       Function used to reset speed module
RETURN:     MOTOR_OK if ok, MOTOR_ERROR if errors
----------------------------------------------------------------------*/
int BrushlessModule::ResetSpeed()
{
	// controllo versione
	if( ver < 5 || ( ver == 5 && rev < 1 ) )
	{
		return MOTOR_OK;
	}

	errorCode = TWSBus1_Send_Command_HD( m_comInt, m_Address, RESETSPEED_CMD, m_Data );
	return errorCode ? MOTOR_ERROR : MOTOR_OK;
}
