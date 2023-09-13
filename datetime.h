//---------------------------------------------------------------------------
//
// Name:        datetime.h
// Author:      Gabriel Ferri
// Created:     15/10/2008 9.43.20
// Description: date and time functions declaration
//
//---------------------------------------------------------------------------

#ifndef __DATETIME_H
#define __DATETIME_H

#include <time.h>

struct date
{
	short da_year;
	char  da_day;
	char  da_mon;
};

void getdate( struct date * );

struct time
{
	unsigned char ti_min;
	unsigned char ti_hour;
	//unsigned char ti_hund;
	unsigned char ti_sec;
};

void gettime( struct time * );

void getdate_formatted( char* date );

#endif
