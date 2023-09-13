//---------------------------------------------------------------------------
//
// Name:        timeprof.h
// Author:      Gabriel Ferri
// Created:     23/10/2008 11.58
// Description: timeprof class definitions
//
//---------------------------------------------------------------------------

#ifndef __TIMEPROF_H
#define __TIMEPROF_H

#include <stdio.h>
#include <string>
#include "Timer.h"


class CTimeProfiling
{
public:
	CTimeProfiling( const std::string& filename );
	~CTimeProfiling();
	
	int clear();
	int start( const char* fmt, ... );
	int stop();
	float measure( const char* fmt, ... );
	float measure();

protected:
	FILE* _f;
	std::string _filename;
	Timer _timer;

	float _prevTime;
};

#endif
