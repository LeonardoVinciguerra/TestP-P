//---------------------------------------------------------------------------
//
// Name:        q_files.cpp
// Author:      Gabriel Ferri
// Created:     17/01/2011
// Description: Gestione files di programma
//
//---------------------------------------------------------------------------

#include "q_files_new.h"

#include "filemanager.h"
#include "q_help.h"
#include "q_cost.h"
#include "msglist.h"


//---------------------------------------------------------------------------
// Files names

#define MACHINEINFO_FILE          STDFILENAME".nfo"
#define AXESMOVEMENT_FILE         STDFILENAME".mov"
#define QUICKSTART_FILE           STDFILENAME".qqs"
#define CAMERACONTROLS_FILE       STDFILENAME".qcc"
#define SPEEDS_FILE               STDFILENAME".spd"


//---------------------------------------------------------------------------
// Files current version

#define MACHINEINFO_VERSION       0
#define AXESMOVEMENT_VERSION      0
#define QUICKSTART_VERSION        0
#define CAMERACONTROLS_VERSION    0
#define SPEEDS_VERSION            0



//************************************************************************
// file: informazioni macchina
//************************************************************************
void MachineInfo_Default( SMachineInfo& data )
{
	data.WorkTime = 0;
	data.XMovement = 0;
	data.YMovement = 0;
}

bool MachineInfo_Read( SMachineInfo& data )
{
	CFileManager fm( MACHINEINFO_FILE );
	int retVal = fm.open( MACHINEINFO_VERSION, true );

	if( retVal == -1 )
	{
		// create new file
		if( !fm.create( MACHINEINFO_VERSION ) )
			return false;

		MachineInfo_Default( data );
		if( fm.writeRec( &data, sizeof(data), 0 ) )
		{
			char buf[160];
			snprintf( buf, 160, MsgGetString(Msg_07027), MACHINEINFO_FILE );
			W_Mess( buf );
			return true;
		}
		return false;
	}
	else if( retVal == -4 )
	{
		// update
	}
	else if( retVal != 1 )
	{
		return false;
	}

	return fm.readRec( &data, sizeof(data), 0 );
}

bool MachineInfo_Write( const SMachineInfo& data )
{
	CFileManager fm( MACHINEINFO_FILE );
	if( fm.open( MACHINEINFO_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( MACHINEINFO_VERSION ) )
			return false;
	}

	return fm.writeRec( &data, sizeof(data), 0 );
}


//************************************************************************
// file: test movimento assi
//************************************************************************
void AxesMovement_Default( SAxesMovement& data )
{
	for( int i = 0; i < AXESMOVEMENT_LEN; i++ )
	{
		data.pos[i].X = 0.f;
		data.pos[i].Y = 0.f;
		data.speed[i] = 0.f;
	}
}

bool AxesMovement_Read( SAxesMovement& data )
{
	CFileManager fm( AXESMOVEMENT_FILE );
	int retVal = fm.open( AXESMOVEMENT_VERSION, true );

	if( retVal == -1 )
	{
		// create new file
		if( !fm.create( AXESMOVEMENT_VERSION ) )
			return false;

		AxesMovement_Default( data );
		if( fm.writeRec( &data, sizeof(data), 0 ) )
		{
			char buf[160];
			snprintf( buf, 160, MsgGetString(Msg_07027), AXESMOVEMENT_FILE );
			W_Mess( buf );
			return true;
		}
		return false;
	}
	else if( retVal == -4 )
	{
		// update
	}
	else if( retVal != 1 )
	{
		return false;
	}

	return fm.readRec( &data, sizeof(data), 0 );
}

bool AxesMovement_Write( const SAxesMovement& data )
{
	CFileManager fm( AXESMOVEMENT_FILE );
	if( fm.open( AXESMOVEMENT_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( AXESMOVEMENT_VERSION ) )
			return false;
	}

	return fm.writeRec( &data, sizeof(data), 0 );
}


//************************************************************************
// file: avvio veloce
//************************************************************************
void QuickStart_Default( SQuickStart& data )
{
	for( int i = 0; i < QUICKSTARTLIST_NUM*4; i++ )
	{
		data.list[i][0] = '\0';
	}
}

bool QuickStart_Read( SQuickStart& data )
{
	CFileManager fm( QUICKSTART_FILE );
	int retVal = fm.open( QUICKSTART_VERSION, true );

	if( retVal == -1 )
	{
		// create new file
		if( !fm.create( QUICKSTART_VERSION ) )
			return false;

		QuickStart_Default( data );
		if( fm.writeRec( &data, sizeof(data), 0 ) )
		{
			char buf[160];
			snprintf( buf, 160, MsgGetString(Msg_07027), QUICKSTART_FILE );
			W_Mess( buf );
			return true;
		}
		return false;
	}
	else if( retVal == -4 )
	{
		// update
	}
	else if( retVal != 1 )
	{
		return false;
	}

	return fm.readRec( &data, sizeof(data), 0 );
}

bool QuickStart_Write( const SQuickStart& data )
{
	CFileManager fm( QUICKSTART_FILE );
	if( fm.open( QUICKSTART_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( QUICKSTART_VERSION ) )
			return false;
	}

	return fm.writeRec( &data, sizeof(data), 0 );
}


//************************************************************************
// file: controlli telecamera
//************************************************************************
void CameraControls_Default( SCameraControls& data )
{
	for( int i = 0; i < CAMERACONTROLS_NUM; i++ )
	{
		data.cam[i] = CAMERACONTROLS_EMPTY;
	}
}

bool CameraControls_Read( SCameraControls& data, bool create_if_not_exist )
{
	CFileManager fm( CAMERACONTROLS_FILE );
	int retVal = fm.open( CAMERACONTROLS_VERSION, false );

	if( retVal == -1 )
	{
		if( create_if_not_exist )
		{
			// create new file
			if( !fm.create( CAMERACONTROLS_VERSION ) )
				return false;

			CameraControls_Default( data );
			if( fm.writeRec( &data, sizeof(data), 0 ) )
			{
				/*
				char buf[160];
				snprintf( buf, 160, MsgGetString(Msg_07027), CAMERACONTROLS_FILE );
				W_Mess( buf );
				*/
				return true;
			}
			return false;
		}

		return false;
	}
	else if( retVal == -4 )
	{
		// update
	}
	else if( retVal != 1 )
	{
		return false;
	}

	return fm.readRec( &data, sizeof(data), 0 );
}

bool CameraControls_Write( const SCameraControls& data )
{
	CFileManager fm( CAMERACONTROLS_FILE );
	if( fm.open( CAMERACONTROLS_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( CAMERACONTROLS_VERSION ) )
			return false;
	}

	return fm.writeRec( &data, sizeof(data), 0 );
}


//************************************************************************
// file: velocita' assi
//************************************************************************
void SSpeedsTable_Default( SSpeedsTable& data )
{
	data.entries[SPEED_XY_L].a = 3000;
	data.entries[SPEED_XY_L].v = 500;
	data.entries[SPEED_XY_L].s = 0;
	data.entries[SPEED_XY_M].a = 6500;
	data.entries[SPEED_XY_M].v = 1000;
	data.entries[SPEED_XY_M].s = 0;
	data.entries[SPEED_XY_H].a = 10000;
	data.entries[SPEED_XY_H].v = 1500;
	data.entries[SPEED_XY_H].s = 0;

	data.entries[SPEED_Z_L].a = 6000;
	data.entries[SPEED_Z_L].v = 600;
	data.entries[SPEED_Z_L].s = 17;
	data.entries[SPEED_Z_M].a = 13000;
	data.entries[SPEED_Z_M].v = 1300;
	data.entries[SPEED_Z_M].s = 17;
	data.entries[SPEED_Z_H].a = 20000;
	data.entries[SPEED_Z_H].v = 2000;
	data.entries[SPEED_Z_H].s = 17;

	data.entries[SPEED_R_L].a = 30000;
	data.entries[SPEED_R_L].v = 10000;
	data.entries[SPEED_R_L].s = 0;
	data.entries[SPEED_R_M].a = 70000;
	data.entries[SPEED_R_M].v = 50000;
	data.entries[SPEED_R_M].s = 0;
	data.entries[SPEED_R_H].a = 140000;
	data.entries[SPEED_R_H].v = 100000;
	data.entries[SPEED_R_H].s = 0;
}

bool SpeedsTable_Read( SSpeedsTable& data )
{
	CFileManager fm( SPEEDS_FILE );
	int retVal = fm.open( SPEEDS_VERSION, true );

	if( retVal == -1 )
	{
		// create new file
		if( !fm.create( SPEEDS_VERSION ) )
			return false;

		SSpeedsTable_Default( data );
		if( fm.writeRec( &data, sizeof(data), 0 ) )
		{
			char buf[160];
			snprintf( buf, 160, MsgGetString(Msg_07027), SPEEDS_FILE );
			W_Mess( buf );
			return true;
		}
		return false;
	}
	else if( retVal == -4 )
	{
		// update
	}
	else if( retVal != 1 )
	{
		return false;
	}

	return fm.readRec( &data, sizeof(data), 0 );
}

bool SpeedsTable_Write( const SSpeedsTable& data )
{
	CFileManager fm( SPEEDS_FILE );
	if( fm.open( SPEEDS_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( SPEEDS_VERSION ) )
			return false;
	}

	return fm.writeRec( &data, sizeof(data), 0 );
}

