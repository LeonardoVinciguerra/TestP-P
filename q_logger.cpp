//---------------------------------------------------------------------------
//
// Name:        q_logger.cpp
// Author:      Gabriel Ferri
// Created:     20/01/2011
// Description: CLogger class implementation
//
//---------------------------------------------------------------------------
#include "q_logger.h"

#include <stdarg.h>
#include "datetime.h"
#include "strutils.h"
#include "q_help.h"
#include "msglist.h"
#include "tv.h"
#include "c_win_par.h"

#include <mss.h>
#include <fstream>

#define USER_LEN      	12
#define CAUSE_LEN     	48

#ifdef __LOG_ERROR
extern CLogger QuadraLogger;
#endif

#define LOGGER_LINES  	30

int fn_LoggerAskBox( char* error, char* user, char* cause );


CLogger::CLogger( const std::string& filename, const std::string& description )
{
	_filename = filename;
	_description = description;
	_enable = true;
	_showMsg = false;
	_printDate = true;
	_printTime = true;
	if( exists() )
		_empty = false;
	else
		_empty = true;
}

CLogger::~CLogger()
{
}

//---------------------------------------------------------------------------
// Erase log file
//---------------------------------------------------------------------------
int CLogger::Clear()
{
	if( exists() )
	{
		remove( _filename.c_str() );
	}

	return 1;
}

//---------------------------------------------------------------------------
// Log string
//---------------------------------------------------------------------------
bool CLogger::Log( const char* fmt, ... )
{
	if( !_enable )
	{
		return true;
	}

	if( !exists() )
	{
		if( !create( _description ) )
		{
			return false;
		}
	}

	FILE* _f = fopen( _filename.c_str(), "a" );
	if( _f == NULL )
	{
		if( _showMsg )
		{
			char buf[160];
			snprintf( buf, 160, MsgGetString(Msg_07029), _filename.c_str() );
			W_Mess( buf );
		}
		return false;
	}

	if( _printDate )
	{
		struct date _date;
		getdate( &_date );
		fprintf( _f, "%02d/%02d/%04d ", _date.da_day, _date.da_mon, _date.da_year );
	}

	if( _printTime )
	{
		struct time _time;
		gettime( &_time );
		fprintf( _f, "%02d:%02d:%02d ", _time.ti_hour, _time.ti_min, _time.ti_sec );
	}

	if( fmt != NULL )
	{
		fprintf( _f, ": " );
		va_list args;
		va_start( args, fmt );
		vfprintf( _f, fmt, args );
	}

	fprintf( _f, "\n" );
	fclose( _f );

	_empty = false;

	return true;
}

//---------------------------------------------------------------------------
// Log string and ask cause
//---------------------------------------------------------------------------
bool CLogger::LogBox( const char* fmt, ... )
{
	if( !_enable )
	{
		return true;
	}

	int cam = pauseLiveVideo();

	char buf[160];
	buf[0] = '\0';

	if( fmt != NULL )
	{
		va_list args;
		va_start( args, fmt );
		vsnprintf( buf, 160, fmt, args );
	}

	char user[USER_LEN];
	char cause[CAUSE_LEN];

	fn_LoggerAskBox( buf, user, cause );
	bool retVal = Log( "%s - [User: %s  Cause: %s]", buf, user, cause );

	playLiveVideo( cam );

	return retVal;
}

//---------------------------------------------------------------------------
// Check file existence
//---------------------------------------------------------------------------
bool CLogger::exists()
{
	return access( _filename.c_str(), F_OK ) == 0 ? true : false;
}

//---------------------------------------------------------------------------
// Create file
// If a file with the same name already exists its content is erased and
// the file is treated as a new empty file.
//---------------------------------------------------------------------------
bool CLogger::create( const std::string& description )
{
	FILE* _f = fopen( _filename.c_str(), "w" );
	if( _f == NULL )
	{
		if( _showMsg )
		{
			char buf[160];
			snprintf( buf, 160, MsgGetString(Msg_07030), _filename.c_str() );
			W_Mess( buf );
		}

		return false;
	}

	if( description.size() )
	{
		fprintf( _f, "//-------------------------------------------------\n" );
		fprintf( _f, "// %s\n", description.c_str() );
		fprintf( _f, "//-------------------------------------------------\n" );
	}

	fclose( _f );

	return true;
}

//---------------------------------------------------------------------------
// Get number of logged line
// (first 3 lines are excluded, they are a comment to the file usage)
//---------------------------------------------------------------------------
int CLogger::GetLineNumber()
{
	if( !exists() )
	{
		return -1;
	}

	int number_of_lines = 0;

    string line;
    ifstream _f(_filename.c_str() );

    if(_f.is_open())
    {
        while(!_f.eof())
        {
            getline(_f,line);

            number_of_lines++;
        }
        _f.close();
    }

    if( number_of_lines < 3 )
    	return -1;
    else
    	return (number_of_lines-4);
}

//---------------------------------------------------------------------------
// Get logger last num lines
//---------------------------------------------------------------------------
bool CLogger::GetLastLines( char* buf, int num )
{
	int number_of_lines = GetLineNumber();

	if( number_of_lines == -1 )
	{
		return false;
	}

	int start_line = number_of_lines - num;
	if( start_line < 0 )
		start_line = 0;

    string line;
    ifstream _f(_filename.c_str() );

    int current_line = 0;
    buf[0] = '\0';
    if(_f.is_open())
    {
        while(!_f.eof())
        {
            getline(_f,line);

            current_line++;
            if( current_line > (start_line+3) )
            {
            	strcat( buf, line.c_str() );
            	strcat( buf, "\n" );
            }
        }
        _f.close();
    }
}

//---------------------------------------------------------------------------
// finestra: Logger ask box
//---------------------------------------------------------------------------
class LoggerAskBoxUI : public CWindowParams
{
public:
	LoggerAskBoxUI( const std::string& description ) : CWindowParams( 0 )
	{
		_description = description;

		SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU | WIN_STYLE_EDITMODE_ON );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 60, 7 );
		SetTitle( MsgGetString(Msg_00633) );

		user[0] = '\0';
		cause[0] = '\0';
	}

	char* GetUser() { return user; }
	char* GetCause() { return cause; }

	typedef enum
	{
		USER_NAME,
		ERR_CAUSE
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[USER_NAME] = new C_Combo( 2, 3, MsgGetString(Msg_06015), USER_LEN, CELL_TYPE_TEXT  );
		m_combos[ERR_CAUSE] = new C_Combo( 2, 5, MsgGetString(Msg_06025), CAUSE_LEN, CELL_TYPE_TEXT  );

		// add to combo list
		m_comboList->Add( m_combos[USER_NAME], 0, 0 );
		m_comboList->Add( m_combos[ERR_CAUSE], 1, 0 );
	}

	void onShow()
	{
		DrawSubTitle( 1, _description.c_str() );
	}

	void onRefresh()
	{
		m_combos[USER_NAME]->SetTxt( user );
		m_combos[ERR_CAUSE]->SetTxt( cause );
	}

	void onEdit()
	{
		strncpy( user, m_combos[USER_NAME]->GetTxt(), USER_LEN );
		strncpy( cause, m_combos[ERR_CAUSE]->GetTxt(), CAUSE_LEN );
	}

	/*
	bool onKeyPress( int key )
	{
		switch( key )
		{
			case ESC:
				if( !strlen( user ) || !strlen( cause ) )
				{
					W_Mess( "Riempire tutti i campi!" ); //TODO: messaggio
				}
				return true;

			default:
				break;
		}

		return false;
	}
	*/

	std::string _description;
	char user[USER_LEN];
	char cause[CAUSE_LEN];
};

int fn_LoggerAskBox( char* error, char* user, char* cause )
{
	LoggerAskBoxUI win( error );
	win.Show();
	win.Hide();

	strncpyQ( user, win.GetUser(), USER_LEN-1 );
	strncpyQ( cause, win.GetCause(), CAUSE_LEN-1 );

	DelSpcR( user );
	DelSpcR( cause );

	return 1;
}

void ShowLogger()
{
#ifdef __LOG_ERROR

	char buf_out[4096];

	QuadraLogger.GetLastLines( buf_out, LOGGER_LINES );

	W_Mess( buf_out, MSGBOX_YUPPER );

#endif
}

