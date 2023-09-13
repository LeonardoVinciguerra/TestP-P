//---------------------------------------------------------------------------
//
// Name:        commclass.cpp
// Author:      Gabriel Ferri
// Created:     21/10/2008
// Description: serial communication functions implementation
//
//---------------------------------------------------------------------------
#include "commclass.h"

#include <unistd.h>
#include "Timer.h"
#include "q_inifile.h"

#include <mss.h>


#define CC_TIMEOUT               100
#define CC_MAX_CHARS_TO_FLUSH    1000


using namespace LibSerial;

CommClass::CommClass( const char* port_name, unsigned int baud_rate )
{
	strncpy( portname, port_name , 64);
	baudrate = baud_rate;

	enabled = 0;
	open();

	timeout_ms = CC_TIMEOUT;

	ResetPerformanceIndex();
}

int CommClass::open()
{
	if(!enabled)
	{
		serial_port.Open( portname );
		if( !serial_port.good() )
			return -1;

		serial_port.SetBaudRate( SerialStreamBuf::BaudRateEnum(baudrate) );
		if ( !serial_port.good() )
			return -2;

		serial_port.SetCharSize( SerialStreamBuf::CHAR_SIZE_8 );
		if( !serial_port.good())
			return -3;

		serial_port.SetParity( SerialStreamBuf::PARITY_NONE );
		if ( !serial_port.good() )
			return -4;

		serial_port.SetNumOfStopBits( 1 );
		if ( !serial_port.good() )
			return -5;

		serial_port.SetFlowControl( SerialStreamBuf::FLOW_CONTROL_NONE );
		if ( !serial_port.good() )
			return -6;

		enabled = 1;
	}
	return 0;
}

void CommClass::flush()
{
	char c;
	int i = 0;
	while( serial_port.rdbuf()->in_avail() > 0 && i < CC_MAX_CHARS_TO_FLUSH )
	{
		serial_port.get( c );
		i++;
	}
}

void CommClass::flush( int max_chars, char* string, int& len )
{
	char c;
	int i = 0;
	while( serial_port.rdbuf()->in_avail() > 0 && i < max_chars )
	{
		serial_port.get( c );
		string[i] = c;
		i++;
	}
	len = i;
}

//-----------------------------------------------------------------------------
// Invia un byte
//-----------------------------------------------------------------------------
void CommClass::putbyte( const char c )
{
	serial_port.write( &c, 1 );
}

//-----------------------------------------------------------------------------
// Invia N bytes
//-----------------------------------------------------------------------------
bool CommClass::putstring( char* string, int len )
{
	if( len > 0 )
	{
		serial_port.write( string, len );
		return !serial_port.bad();
	}
	return true;
}

//-----------------------------------------------------------------------------
// Legge un byte
//-----------------------------------------------------------------------------
unsigned int CommClass::getbyte( void )
{
	if( !enabled )
		return SERIAL_ERROR_TIMEOUT;

	Timer timeoutTimer;
	timeoutTimer.start();
	
	char c;
	while( 1 )
	{
		if( serial_port.rdbuf()->in_avail() > 0  )
		{
			serial_port.get( c );
			return ((unsigned char)c);
		}

		if( timeoutTimer.getElapsedTimeInMilliSec() > timeout_ms )
		{
			break;
		}

		if( !Get_MotorheadOnUart() )
			usleep( 250 ); // usec
	}

	return SERIAL_ERROR_TIMEOUT;
}

//-----------------------------------------------------------------------------
// Legge N bytes
//-----------------------------------------------------------------------------
unsigned int CommClass::getstring( char* string, int len )
{
	if( !enabled )
		return SERIAL_ERROR_TIMEOUT;

	Timer timeoutTimer;
	timeoutTimer.start();

	while( 1 )
	{
		int avail = serial_port.rdbuf()->in_avail();

		int num = std::min( avail, len );
		if( num > 0 )
		{
			for( int i = 0; i < num; i++ )
			{
				serial_port.get( string[i] );
			}
			return num;
		}

		if( timeoutTimer.getElapsedTimeInMilliSec() > timeout_ms )
		{
			break;
		}

		if( !Get_MotorheadOnUart() )
			usleep( 250 ); // usec
	}

	return SERIAL_ERROR_TIMEOUT;
}


CommClass* ComPortCPU;
CommClass* ComPortFox;
CommClass* ComPortExt;
CommClass* ComPortSniper;
CommClass* ComPortStepperAux;
CommClass* ComPortMotorhead;
