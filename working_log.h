//---------------------------------------------------------------------------
//
// Name:        working_log.h
// Author:      Daniele Belloni
// Created:     27/05/2019
// Description: Log del lavoro della macchina
//
//---------------------------------------------------------------------------

#ifndef __WORKING_LOG_H
#define __WORKING_LOG_H

#include <stdio.h>
#include <string>
#include "q_net.h"

#define STATUS_IDLE			0
#define STATUS_WORKING		1
#define STATUS_ERROR		2
#define STATUS_OFF			3

#define PRODUCTION_START	0
#define PRODUCTION_END		1

class CWorkingLog
{
public:
	CWorkingLog();
	~CWorkingLog();

	bool Load();
	bool LogStatus( int status, int errorCode=0 );
	bool LogProduction( int status, char* prodName, int boardNum );

protected:
	char logFile[MAXNPATH];
	bool exists();
	bool create();
};

#endif
