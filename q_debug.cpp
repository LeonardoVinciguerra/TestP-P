//---------------------------------------------------------------------------
// Name:        q_debug.cpp
// Author:      Gabriel Ferri
// Created:     13/11/2012
// Description: Quadra debug functions
//---------------------------------------------------------------------------
#include "q_debug.h"

#include <stdio.h>
#include <stdarg.h>

#include <mss.h>


//--------------------------------------------------------------------------
// Stampa stringa di debug sullo standard output
//--------------------------------------------------------------------------
void print_debug( const char* fmt, ... )
{
	#ifdef __DEBUG
	va_list argptr;
	va_start( argptr, fmt );
	vfprintf( stdout, fmt, argptr );
	va_end( argptr );
	fflush( stdout ) ;
	#endif
}
