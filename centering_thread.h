//---------------------------------------------------------------------------
//
// Name:        centering_thread.h
// Author:      Gabriel Ferri
// Created:     23/11/2011
// Description:
//
//---------------------------------------------------------------------------

#ifndef __CENTERING_THREAD_H
#define __CENTERING_THREAD_H

struct CenteringResultData
{
	int Result;
	float Position1;
	float Position2;
	float Shadow1;
	float Shadow2;
};

void StartCenteringThread();
void StopCenteringThread();

bool StartCentering( int nozzle, int placeAngle, const struct SPackageData* package );
bool IsCenteringCompleted( int nozzle );
int GetCenteringResult( int nozzle, CenteringResultData& data );

#endif
