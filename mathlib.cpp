//---------------------------------------------------------------------------
//
// Name:        mathlib.cpp
// Author:      Gabriel Ferri
// Created:     03/12/2010
// Description:
//
//---------------------------------------------------------------------------
#include "mathlib.h"

#include <mss.h>


int ftoi( double x )
{
	int X = int( x );
	if( x >= 0.0 )
		return ( x - X > 0.5 ) ? X + 1 : X;
	else
		return ( x - X > -0.5 ) ? X : X - 1;
	/*
	if( x > 0.0 )
		return (int)(x+0.5);
	else
		return (int)(x-0.5);
	*/
}


//---------------------------------------------------------------------------------
// Converte da esadecimale a decimale
//---------------------------------------------------------------------------------
unsigned char Hex2Dec( unsigned char hex )
{
	// number
	if ( hex >= '0' && hex <= '9' )
		return hex - '0';

	// upper alpha
	if ( hex >= 'A' && hex <= 'F' )
		return hex - 'A' + 10;

	// lower alpha
	if ( hex >= 'a' && hex <= 'f' )
		return hex - 'a' + 10;

	return 0;
}
