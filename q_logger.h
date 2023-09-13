//---------------------------------------------------------------------------
//
// Name:        q_logger.h
// Author:      Gabriel Ferri
// Created:     20/01/2011
// Description: CLogger class definitions
//
//---------------------------------------------------------------------------

#ifndef __Q_LOGGER_H
#define __Q_LOGGER_H

#include <stdio.h>
#include <string>

void ShowLogger();

class CLogger
{
public:
	CLogger( const std::string& filename, const std::string& description );
	~CLogger();

	void PrintDate( bool enable ) { _printDate = enable; }
	void PrintTime( bool enable ) { _printTime = enable; }

	void EnableMessageBox( bool enable ) { _showMsg = enable; }
	void EnableLog( bool enable ) { _enable = enable; }

	int Clear();
	bool IsEmpty() { return _empty; }
	bool Log( const char* fmt, ... );
	bool LogBox( const char* fmt, ... );

	int GetLineNumber();
	bool GetLastLines( char* buf, int num );

protected:
	std::string _filename;
	std::string _description;
	bool _enable;
	bool _showMsg;
	bool _printDate;
	bool _printTime;
	bool _empty;

	bool exists();
	bool create( const std::string& description );
};

#endif
