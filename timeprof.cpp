//---------------------------------------------------------------------------
//
// Name:        timeprof.cpp
// Author:      Gabriel Ferri
// Created:     23/10/2008 11.58
// Description: timeprof class implementation
//
//---------------------------------------------------------------------------
#include "timeprof.h"

#include <stdarg.h>

#include <mss.h>


CTimeProfiling::CTimeProfiling( const std::string& filename )
{
	_f = 0;
	_filename = filename;
	_prevTime = 0;
}

CTimeProfiling::~CTimeProfiling()
{
	stop();
}

int CTimeProfiling::stop()
{
	if( _f )
	{
		measure( "Stop profiling" );
		fprintf( _f, "\n" );
		if( _filename.size() )
		{
			fclose( _f );
			_f = NULL;
		}
	}

	return 1;
}

int CTimeProfiling::start( const char* fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	
	stop();
	
	if( _filename.size() == 0 )
	{
		return -1;
	}

	_f = fopen( _filename.c_str(), "a" );
	if( _f == NULL )
	{
		return -1;
	}

	if( fmt == NULL )
	{
		fprintf( _f, "Start profiling\n" );
	}
	else
	{
		fprintf( _f,"Start profiling : " );
		vfprintf( _f, fmt, args );
		fprintf( _f, "\n" );
	}

	_timer.start();
	_prevTime = 0;

	return 1;
}

int CTimeProfiling::clear()
{
	stop();
	remove( _filename.c_str() );

	return 1;
}

float  CTimeProfiling::measure( const char* fmt, ... )
{
	float elapsedTime = _timer.getElapsedTimeInMicroSec() / 1000.f;
	
	if( _f != NULL && fmt != NULL )
	{
		va_list args;
		va_start(args,fmt);
		vfprintf( _f, fmt, args );
		
		fprintf( _f, " : %5.2f; %5.2f\n", elapsedTime - _prevTime, elapsedTime );
	}
	
	_prevTime = elapsedTime;
	return elapsedTime;
}

float  CTimeProfiling::measure()
{
	return measure( NULL );
}
