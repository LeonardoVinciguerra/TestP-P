/*
 * q_conveyor.cpp
 *
 *  Created on: 15/ott/2014
 *      Author: dbelloni
 */

#include "q_help.h"
#include "msglist.h"
#include "commclass.h"
#include "comaddr.h"
#include "q_conveyor.h"
#include "q_tabe.h"
#include "lnxdefs.h"
#include "stepper_modules.h"
#include "q_inifile.h"

#define MOTOR_RETRY			3
#define MOTOR_STATUS_DELAY	5

bool isConveyorZero = false;
bool isConveyorInit = false;
float actConveyorPos = 0.0;

// Verifica se il convogliatore e' stato abilitato
bool Get_ConveyorEnabled()
{
	bool ret = false;

	struct conv_data convVal;
	Read_Conv( &convVal );

	if( convVal.enabled )
		ret = true;

	return ret;
}

// Inizializza il convogliatore
bool InitConveyor()
{
	if( !Get_UseSteppers() )
	{
		isConveyorInit = true;

		return true;
	}

	struct conv_data convVal;
	Read_Conv( &convVal );

	isConveyorInit = false;

	if( Conveyor->ResetDrive() == MOTOR_ERROR )
		return false;

	delay( 3000 );

	if( Conveyor->MicroStepping( CONV_MICROSTEP ) == MOTOR_ERROR )
		return false;

	if( Conveyor->MaximumFreq( convVal.speed ) == MOTOR_ERROR )
		return false;

	if( Conveyor->Aceleration( convVal.accDec ) == MOTOR_ERROR )
		return false;

	if( Conveyor->Deceleration( convVal.accDec ) == MOTOR_ERROR )
		return false;

	if( Conveyor->SetMinCurrent( convVal.minCurr ) == MOTOR_ERROR )
		return false;

	if( Conveyor->SetMaxCurrent( convVal.maxCurr ) == MOTOR_ERROR )
		return false;

	if( Conveyor->SetDecay( FAST_DECAY ) == MOTOR_ERROR )
		return false;

	if( Conveyor->MotorEnable( MOTOR_ON ) == MOTOR_ERROR )
		return false;

	actConveyorPos = 0.0;
	isConveyorInit = true;

	return true;
}

// Verifica se il convogliatore e' stato correttamente inizializzato
bool Get_ConveyorInit()
{
	return isConveyorInit;
}

// Verifica se lo zero del convogliatore e' stato correttamente trovato e pronto per essere utilizzato
bool Get_UseConveyor()
{
	return isConveyorZero;
}

// Ricerca lo zero del convogliatore e lo rende utilizzabile
bool SearchConveyorZero()
{
	if( !Get_UseSteppers() )
	{
		isConveyorZero = true;
		actConveyorPos = 0.0;
		return true;
	}

	struct conv_data convVal;
	Read_Conv( &convVal );

	isConveyorZero = false;

	if( Conveyor->HomeSensorInput( convVal.zero ) == MOTOR_ERROR )
		return false;

	if( Conveyor->SetHomingSpeed( HOME_SLOW, CONV_SLOWSEARCHSPEED_DEF ) == MOTOR_ERROR )
		return false;

	if( Conveyor->SetHomingSpeed( HOME_FAST, CONV_FASTSEARCHSPEED_DEF ) == MOTOR_ERROR )
		return false;

	if( Conveyor->SetHomingMovement( convVal.maxPos*convVal.stepsMm ) == MOTOR_ERROR )
		return false;

	if( Conveyor->SearchPos0( ZEROSEARCH_POS ) == MOTOR_ERROR )
		return false;
	if( !WaitConveyorOrigin() )
		return false;

	isConveyorZero = true;
	actConveyorPos = 0.0;

	return isConveyorZero;
}

// Attende la fine della ricerca zero del convogliatore
bool WaitConveyorOrigin()
{
	if( !Get_UseSteppers() )
		return true;

	int status = 0;
	int endMove = 0;
	int retry = 0;

	while( !endMove )
	{
		status = Conveyor->MotorStatus();

		if( status == MOTOR_ERROR )
		{
			retry++;
			if( retry >= MOTOR_RETRY )
			{
				return false;
			}
		}

		if( status & MOTOR_PROCERROR )
			return false;

		if( !(status & MOTOR_ZERO) )
			endMove = 1;

		delay( MOTOR_STATUS_DELAY );
	}

	return true;
}


// Muove il convogliatore
bool MoveConveyor( float mm, int mode )
{
	if( !Get_UseSteppers() )
		return true;

	// Prima di effettuare un movimento, verifico se ci sono stati errori in precedenza...
	int status = Conveyor->MotorStatus();
	if( (status == MOTOR_ERROR) || (status & MOTOR_OVERRUN) || (status & MOTOR_PROCERROR) || (status & MOTOR_TIMEOUT) )
	{
		W_Mess( MsgGetString(Msg_00876) );
		return false;
	}

	struct conv_data convVal;
	Read_Conv( &convVal );

	float pos = mm;
	if( mode == REL_MOVE )
		pos = actConveyorPos + mm;

	// Check limiti sw
	if( (pos < convVal.minPos) || (pos > convVal.maxPos) )
		return false;

	if( Conveyor->GotoPos0( (int)(pos*convVal.stepsMm) ) == MOTOR_ERROR )
		return false;

	actConveyorPos = pos;

	return true;
}

// Attende la fine del movimento del convogliatore
bool WaitConveyor()
{
	if( !Get_UseSteppers() )
		return true;

	int status = 0;
	int endMove = 0;
	int retry = 0;

	while( !endMove )
	{
		status = Conveyor->MotorStatus();

		if( status == MOTOR_ERROR )
		{
			retry++;
			if( retry >= MOTOR_RETRY )
			{
				return false;
			}
		}

		if( status & MOTOR_OVERRUN )
			return false;

		if( !(status & MOTOR_RUNNING) )
			endMove = 1;

		delay( MOTOR_STATUS_DELAY );
	}

	return true;
}

// Muove il convogliatore ed attende la fine del movimento
bool MoveConveyorAndWait( float mm, int mode )
{
	if( !MoveConveyor( mm, mode ) )
		return false;

	if( !WaitConveyor() )
		return false;

	return true;
}

// Ritorna la posizione del convogliatore
float GetConveyorPosition( int pos )
{
	float val = 0.0;

	struct conv_data convVal;
	Read_Conv( &convVal );

	if( pos == CONV_ZERO )
		val = convVal.zeroPos;
	else if( pos == CONV_REF )
		val = convVal.refPos;
	else if( pos == CONV_ACT )
		val = actConveyorPos;

	return val;
}

// Setta la posizione del convogliatore come l'attuale
bool SetConveyorPosition( int pos )
{
	struct conv_data convVal;
	Read_Conv( &convVal );

	if( pos == CONV_ZERO )
		convVal.zeroPos = actConveyorPos;
	else if( pos == CONV_REF )
		convVal.refPos = actConveyorPos;
	else if( pos == CONV_STEP1 )
		convVal.move1 = actConveyorPos;
	else if( pos == CONV_STEP2 )
		convVal.move2 = actConveyorPos;
	else if( pos == CONV_STEP3 )
		convVal.move3 = actConveyorPos;

	Mod_Conv( convVal );

	return true;
}

// Porta il convogliatore a zero/riferimento
bool MoveConveyorPosition( int pos )
{
	struct conv_data convVal;
	Read_Conv( &convVal );

	if( pos == CONV_ZERO )
		return MoveConveyorAndWait( convVal.zeroPos );
	else if( pos == CONV_REF )
		return MoveConveyorAndWait( convVal.refPos );
	else if( pos == CONV_HOME )
		return MoveConveyorAndWait( 0.0 );

	return true;
}

// Aggiorna le velocita'
bool UpdateConveyorSpeedAcc( int speed, int accDec )
{
	if( !Get_UseSteppers() )
		return true;

	struct conv_data convVal;
	Read_Conv( &convVal );

	if( Conveyor->MaximumFreq( speed ) == MOTOR_ERROR )
		return false;

	if( Conveyor->Aceleration( accDec ) == MOTOR_ERROR )
		return false;

	if( Conveyor->Deceleration( accDec ) == MOTOR_ERROR )
		return false;

	return true;
}

// Ritorna la posizione attuale in passi del motore del convogliatore
int GetConveyorActualSteps()
{
	return Conveyor->ActualPosition();
}

// Abilita i limiti del convogliatore
bool ConveyorLimitsEnable( bool state )
{
	if( !Get_UseSteppers() )
		return true;

	struct conv_data convVal;
	Read_Conv( &convVal );

	// Set cw and ccw limit
	int flag = (convVal.limit << 16) + convVal.zero;
	if( Conveyor->InputsSetting( flag ) == MOTOR_ERROR )
		return false;

	if( Conveyor->SetLimitsCheck( state ? LIMITCHECK_ON : LIMITCHECK_OFF, LIMITLEVEL_HIGH ) == MOTOR_ERROR )
		return false;

	return true;
}

//---------------------------------------------------------------------------------
// Aggiorna firmware modulo Conveyor
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

int Conveyor_BootLoader( const char* filename )
{
	unsigned int inByte, outByte;
	int i;
	int retCode = 1;

	// open selected file
	FILE* firmware = fopen( filename , "rb" );

	if( firmware == NULL )
	{
		return -1;
	}

	// reset drive
	Conveyor->ResetDrive();

	// ATTENZIONE! per il bootloader bisogna riaprire la porta COM con il corretto baudrate
	// dal momento che per il bootloader le schede stepper nuove vogliono 460800!!!
	delete ComPortStepperAux;
	delete Conveyor;
	CommClass* com = new CommClass( getStepperAuxComPort(), MOTORHEAD_BAUD );

	// ignore any received bytes (flush input buffer)
	com->flush();

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
		delete com;
		return retCode;
	}

	// looking for 'b' character
	inByte = com->getbyte();
	// check for timeout
	if( inByte == SERIAL_ERROR_TIMEOUT )
	{
		fclose( firmware );
		delete com;
		return -4;
	}

	if( inByte != 'b' )
	{
		fclose( firmware );
		delete com;
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
		delete com;
		return -4;
	}

	// looking for target response
	inByte = com->getbyte();
	// check for timeout
	if( inByte == SERIAL_ERROR_TIMEOUT )
	{
		fclose( firmware );
		delete com;
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
	delete com;
	return retCode;
}

//---------------------------------------------------------------------------------
// Interfaccia testuale aggiornamento firmware convogliatore
//---------------------------------------------------------------------------------
int UpdateConveyor()
{
	printf( "\n\tVersion %s\n", SOFT_VER );
	printf( "\tBuild date %s\n\n", __DATE__ );

	// Apertura porta seriale per comunicazione con modulo convogliatore
	ComPortStepperAux = new CommClass( getStepperAuxComPort(), STEPPERAUX_BAUD );

	Conveyor = new StepperModule( ComPortStepperAux, 15 );

	// read version
	unsigned short version;
	unsigned char ver;
	unsigned char rev;

	version = Conveyor->GetVersion();
	ver = version >> 8;
	rev = version;

	if( ver != 255 && rev != 255 )
	{
		printf( " Conveyor   v. %d.%d\n", ver, rev );
	}

	int exit_flag = 0;
	while( !exit_flag )
	{
		delay(1000);
		int ret = 0;

		printf("\n");
		printf(" 1 - Update firmware - Conveyor\n");
		printf(" q (Q) - Quit\n");
		printf("-> ");

		int c = getchar();

		// flush
		int c_dummy = c;
		while( c_dummy != 10 )
			c_dummy = getchar();

		if( c == 'q' || c == 'Q' )
			exit_flag = 1;
		else if( c == '1' )
		{
			// confirm
			printf("\n");
			printf("\n Confirm updating conveyor firmware ?\n");
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
				ret = Conveyor_BootLoader( "conveyor.tix" );

				// ATTENZIONE! alla fine del bootloader bisogna riaprire la porta COM con il corretto baudrate
				// dal momento che per il bootloader le schede stepper nuove vogliono 460800!!!
				ComPortStepperAux = new CommClass( getStepperAuxComPort(), STEPPERAUX_BAUD );
				Conveyor = new StepperModule( ComPortStepperAux, 15 );

				if( c == '1' )
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
						delay(2000);
						version = Conveyor->GetVersion();
						ver = version >> 8;
						rev = version;

						if( ver != 255 && rev != 255 )
						{
							printf( "\n\n Conveyor   v. %d.%d\n\n\n", ver, rev );
							exit_flag = 1;
						}
					}
				}
			}
		}
	}

	delete Conveyor;
	delete ComPortStepperAux;

	return 1;
}
