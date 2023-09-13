//---------------------------------------------------------------------------
//
// Name:        q_files.h
// Author:      Gabriel Ferri
// Created:     17/01/2011
// Description: Gestione files di programma
//
//---------------------------------------------------------------------------

#ifndef __Q_FILES_H
#define __Q_FILES_H

#include "mathlib.h"


//************************************************************************
// file: informazioni macchina
//************************************************************************
#pragma pack(1)
struct SMachineInfo
{
	unsigned int WorkTime;   // tempo di lavoro della macchina in sec (assemblaggio + test assi )

	long long int XMovement;  // movimento totale asse X in m (64bit)
	long long int YMovement;  // movimento totale asse Y in m (64bit)
};
#pragma pack()

bool MachineInfo_Read( SMachineInfo& data );
bool MachineInfo_Write( const SMachineInfo& data );


//************************************************************************
// file: test movimento assi
//************************************************************************
#define AXESMOVEMENT_LEN     6

#pragma pack(1)
struct SAxesMovement
{
	PointF pos[AXESMOVEMENT_LEN];
	float speed[AXESMOVEMENT_LEN];
};
#pragma pack()

bool AxesMovement_Read( SAxesMovement& data );
bool AxesMovement_Write( const SAxesMovement& data );


//************************************************************************
// file: avvio veloce
//************************************************************************
#define QUICKSTARTLIST_NUM   8
#define QUICKSTARTTXT_LEN    12

struct SQuickStart
{
	char list[QUICKSTARTLIST_NUM*4][QUICKSTARTTXT_LEN];
};

bool QuickStart_Read( SQuickStart& data );
bool QuickStart_Write( const SQuickStart& data );


//************************************************************************
// file: controlli telecamera
//************************************************************************
#define CAMERACONTROLS_NUM   100
#define CAMERACONTROLS_EMPTY 0
#define CAMERACONTROLS_HEAD  1
#define CAMERACONTROLS_EXT   2

struct SCameraControls
{
	int cam[CAMERACONTROLS_NUM];
	int id[CAMERACONTROLS_NUM];
	int value[CAMERACONTROLS_NUM];
};

bool CameraControls_Read( SCameraControls& data, bool create_if_not_exist );
bool CameraControls_Write( const SCameraControls& data );


//************************************************************************
// file: velocita' assi
//************************************************************************
enum eSpeedID
{
	SPEED_XY_L = 0,
	SPEED_XY_M,
	SPEED_XY_H,
	SPEED_Z_L,
	SPEED_Z_M,
	SPEED_Z_H,
	SPEED_R_L,
	SPEED_R_M,
	SPEED_R_H,
	SPEED_NUM
};

struct SSpeedsTableEntry
{
	unsigned int a; // acceleration
	unsigned int v; // speed
	unsigned int s; // start/stop speed
};

struct SSpeedsTable
{
	SSpeedsTableEntry entries[SPEED_NUM];
};

bool SpeedsTable_Read( SSpeedsTable& data );
bool SpeedsTable_Write( const SSpeedsTable& data );

#endif
