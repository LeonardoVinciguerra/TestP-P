//---------------------------------------------------------------------------
//
// Name:        commclass.h
// Author:      Gabriel Ferri
// Created:     21/10/2008
// Description: serial communication functions declaration
//
//---------------------------------------------------------------------------

#ifndef __COMMCLASS_H
#define __COMMCLASS_H

#include <SerialStream.h>
#include <SerialStreamBuf.h>

#define SERIAL_ERROR_DISABLE      2056
#define SERIAL_ERROR_TIMEOUT      2057


class CommClass
{
public:
	int enabled;
	CommClass( const char* port_name, unsigned int baud_rate );

	int open();
	void close() {};

	void flush();
	void flush( int max_chars, char* string, int& len );

	void putbyte( const char c );
	bool putstring( char* string, int len );
	unsigned int getbyte();
	unsigned int getstring( char* string, int len );

	void settimeout( unsigned int ms ) { timeout_ms = ms; };

	const char* GetPort(void) { return portname; };

	void ResetPerformanceIndex() { num_comms = 0; num_errors = 0; }
	double GetPerformanceIndex() { return (num_comms && num_errors) ? double(num_errors)*100.0/double(num_comms) : 0.0; }
	unsigned int GetFailures() { return num_errors; }
	void CommunicationFailure() { num_comms++; num_errors++; }
	void CommunicationOK() { num_comms++; }

private:
	LibSerial::SerialStream serial_port;
	char portname[64];
	unsigned int baudrate;
	unsigned int timeout_ms;

	unsigned int num_comms;
	unsigned int num_errors;
};

extern CommClass* ComPortCPU;
extern CommClass* ComPortFox;
extern CommClass* ComPortExt;
extern CommClass* ComPortSniper;
extern CommClass* ComPortStepperAux;
extern CommClass* ComPortMotorhead;

#endif
