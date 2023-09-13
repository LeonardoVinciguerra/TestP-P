//-----------------------------------------------------------------------------
// File: Sniper.h
//
// Desc: Header file for the Sniper class
//
//-----------------------------------------------------------------------------

#ifndef __SNIPER_H
#define __SNIPER_H

void Snipers_Enable();
int Sniper_Calibrate( int nozzle );
int Sniper_CheckComp( int nozzle );
int Sniper_ZeroCheck( int nozzle );
int Sniper_FindNozzleHeight( int nozzle );
int Sniper_FindZeroTheta( int nozzle, int check = 0 );
int Sniper_FindPrerotZeroTheta( int nozzle );
void Sniper_ImageTest( int nozzle );
void Sniper_ImageTestDetailed( int nozzle );
void Sniper_PlotFrames( int nozzle );


int UpdateSniper();
int ActivateSniper();

#endif
