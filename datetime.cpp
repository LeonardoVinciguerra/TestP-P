//---------------------------------------------------------------------------
//
// Name:        datetime.h
// Author:      Gabriel Ferri
// Created:     15/10/2008 9.43.20
// Description: date and time functions declaration
//
//---------------------------------------------------------------------------
#include "datetime.h"

#include <time.h>
#include <stdio.h>

#include <mss.h>


void getdate( struct date *d )
{
	time_t ms = time( NULL );
	tm *tempo = localtime( &ms );

	d->da_year	= tempo->tm_year + 1900;
	d->da_day	= tempo->tm_mday;
	d->da_mon	= tempo->tm_mon + 1;
}

void gettime( struct time *t )
{
	time_t ms = time( NULL );
	tm *tempo = localtime( &ms );

	t->ti_min	= tempo->tm_min;
	t->ti_hour	= tempo->tm_hour;
	t->ti_sec	= tempo->tm_sec;
}

//--------------------------------------------------------------------------
// Ritorna la data del giorno formattata GG.MM.AAAA
//--------------------------------------------------------------------------
void getdate_formatted( char* date )
{
	struct date _date;
	getdate( &_date );
	sprintf( date, "%02d.%02d.%04d", _date.da_day, _date.da_mon, _date.da_year );
}
