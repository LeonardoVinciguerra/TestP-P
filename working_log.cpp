//---------------------------------------------------------------------------
//
// Name:        working_log.cpp
// Author:      Daniele Belloni
// Created:     27/05/2019
// Description: Log del lavoro della macchina
//
//---------------------------------------------------------------------------
#include "working_log.h"

#include <stdarg.h>
#include "datetime.h"
#include "strutils.h"
#include "c_win_par.h"
#include "q_help.h"
#include "q_inifile.h"
#include <mss.h>
#include <fstream>
#include "msglist.h"
#include "q_tabe.h"
#include "q_gener.h"

//---------------------------------------------------------------------------
// Files names

#define WORKING_LOG_FILE_NAME_END          "_job.log"



CWorkingLog::CWorkingLog()
{

}

CWorkingLog::~CWorkingLog()
{

}

//---------------------------------------------------------------------------
// Check file existence
//---------------------------------------------------------------------------
bool CWorkingLog::exists()
{
	return access( logFile, F_OK ) == 0 ? true : false;
}

//---------------------------------------------------------------------------
// Create file
//
//---------------------------------------------------------------------------
bool CWorkingLog::create()
{
	if( !Get_WorkingLog() || !IsNetEnabled() )
	{
		return true;
	}

	FILE* _f = fopen( logFile, "w" );
	if( _f == NULL )
	{
		char buf[160];
		snprintf( buf, 160, MsgGetString(Msg_07030), logFile );
		Reset_WorkingLog();
		W_Mess( buf );

		return false;
	}

	struct date _date;
	getdate( &_date );
	fprintf( _f, "%02d/%02d/%04d;", _date.da_day, _date.da_mon, _date.da_year );

	struct time _time;
	gettime( &_time );
	fprintf( _f, "%02d:%02d:%02d;", _time.ti_hour, _time.ti_min, _time.ti_sec );

	fprintf( _f, "IDLE\n" );

	fclose( _f );

	return true;
}

bool CWorkingLog::Load()
{
	if( !Get_WorkingLog() || !IsNetEnabled() )
	{
		return true;
	}

	snprintf( logFile, MAXNPATH, "%s/%s%s", REMOTEDISK, nwpar.NetID, WORKING_LOG_FILE_NAME_END );
	if( !exists() )
	{
		create();

		char buf[160];
		snprintf( buf, 160, MsgGetString(Msg_07033) );
		W_Mess( buf );
	}

	return true;
}

//---------------------------------------------------------------------------
// Log status
//---------------------------------------------------------------------------
bool CWorkingLog::LogStatus( int status, int errorCode )
{
	if( !Get_WorkingLog() || !IsNetEnabled() )
	{
		return true;
	}

	if( !exists() )
	{
		create();

		char buf[160];
		snprintf( buf, 160, MsgGetString(Msg_07033) );
		W_Mess( buf );
	}

	FILE* _f = fopen( logFile, "r+" );
	if( _f == NULL )
	{
		char buf[160];
		snprintf( buf, 160, MsgGetString(Msg_07029), logFile );
		Reset_WorkingLog();
		W_Mess( buf );

		return false;
	}

	struct date _date;
	getdate( &_date );
	fprintf( _f, "%02d/%02d/%04d;", _date.da_day, _date.da_mon, _date.da_year );

	struct time _time;
	gettime( &_time );
	fprintf( _f, "%02d:%02d:%02d;", _time.ti_hour, _time.ti_min, _time.ti_sec );

	switch( status )
	{
	case STATUS_IDLE:
		fprintf( _f, "IDLE    \n" );
		break;

	case STATUS_WORKING:
		fprintf( _f, "WORKING \n" );
		break;

	case STATUS_ERROR:
		fprintf( _f, "ERROR   \n" );
		break;

	case STATUS_OFF:
		fprintf( _f, "OFF     \n" );
		break;
	}

	fclose( _f );

	return true;
}

//---------------------------------------------------------------------------
// Log production
//---------------------------------------------------------------------------
bool CWorkingLog::LogProduction( int status, char* prodName, int boardNum )
{
	if( !Get_WorkingLog() || !IsNetEnabled() )
	{
		return true;
	}

	if( !exists() )
	{
		create();

		char buf[160];
		snprintf( buf, 160, MsgGetString(Msg_07033) );
		W_Mess( buf );
	}

	struct FileHeader header;
	TPrgFile* fp = new TPrgFile( prodName, PRG_NORMAL );
	if( fp->Open( NOSKIPHEADER ) )
	{
		fp->ReadHeader( header );
	}

	delete fp;

	FILE* _f = fopen( logFile, "a" );
	if( _f == NULL )
	{
		char buf[160];
		snprintf( buf, 160, MsgGetString(Msg_07029), logFile );
		Reset_WorkingLog();
		W_Mess( buf );

		return false;
	}

	struct date _date;
	getdate( &_date );
	fprintf( _f, "%02d/%02d/%04d;", _date.da_day, _date.da_mon, _date.da_year );

	struct time _time;
	gettime( &_time );
	fprintf( _f, "%02d:%02d:%02d;", _time.ti_hour, _time.ti_min, _time.ti_sec );

	switch( status )
	{
	case PRODUCTION_START:
		fprintf( _f, "START;%s;%s;%d\n", prodName, trimwhitespace(header.F_note), boardNum );
		break;

	case PRODUCTION_END:
		fprintf( _f, "END;%s;%s;%d\n", prodName, trimwhitespace(header.F_note), boardNum );
		break;
	}

	fclose( _f );

	return true;
}

