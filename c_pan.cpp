//---------------------------------------------------------------------------
// Name:        c_pan.cpp
// Author:      Gabriel Ferri
// Created:     16/11/2011
// Description: CPan class implementation
//---------------------------------------------------------------------------
#include "c_pan.h"

#include <stdarg.h>

#include <mss.h>

#define CPAN_MIN_WIDTH		44


//---------------------------------------------------------------------------
// Costruttore
//    y: -1 centra verticalmente la finestra
//---------------------------------------------------------------------------
CPan::CPan( int y, int n, ... )
: CWindow( 0 )
{
	if( y < 0 )
		SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED );
	else
		SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED_X );

	SetClientAreaPos( 0, y );

	// calcolo dimensioni finestra
	int len_max = 0;
	int n_row = 0;

	va_list ap;
	va_start( ap, n );

	for( int i = 0; i < n; i++ )
	{
		char* row = va_arg( ap, char* );

		if( row != NULL )
		{
			int len = strlen(row);
			if( len > len_max )
				len_max = len;

			n_row++;
		}
	}

	va_end( ap );

	len_max += 4;
	if( len_max < CPAN_MIN_WIDTH )
		len_max = CPAN_MIN_WIDTH;

	n_row += 2;

	SetClientAreaSize( len_max, n_row );

	// visualizzo messaggi
	GUI_Freeze_Locker lock;

	Show();

	va_start( ap, n );

	for( int i = 0; i < n; i++ )
	{
		char* row = va_arg( ap, char* );

		if( row != NULL )
		{
			DrawTextCentered( i + 1, row );
		}
	}

	va_end(ap);
}
