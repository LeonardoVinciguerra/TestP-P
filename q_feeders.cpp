//---------------------------------------------------------------------------
//
// Name:        q_feeders.cpp
// Author:      Gabriel Ferri
// Created:     13/03/2012
// Description: Quadra feeders manager
//
//---------------------------------------------------------------------------
#include "q_feeders.h"

#include "q_feederfile.h"
#include "q_cost.h"
#include "q_gener.h"
#include "q_help.h"
#include "q_init.h"
#include "q_tabe.h"
#include "msglist.h"
#include "filemanager.h"


//************************************************************************
// file: configurazione caricatori
//************************************************************************

CarDat currentFeedersConfig[MAXCAR];


	//---------------//
	// Gestione file //
	//---------------//

#define FEEDERSCONFIG_VERSION     0


void FeedersConfig_Default( CarDat* data )
{
	data->C_codice      = 0;
	data->C_comp[0]     = 0;
	data->C_tipo        = 0;
	data->C_thFeederAdd = 0;
	data->C_att         = 0;
	data->C_xcar        = 0;
	data->C_ycar        = 0;
	data->C_nx          = 1;
	data->C_ny          = 1;
	data->C_incx        = 0;
	data->C_incy        = 0;
	data->C_offprel     = 0;
	data->C_quant       = 0;
	data->C_avan        = 0;
	data->C_Ncomp       = 1;
	data->C_sermag      = 0;
	data->C_PackIndex   = 0;
	data->C_Package[0]  = 0;
	data->C_note[0]     = 0;
	data->C_checkPos    = 0;
	data->C_checkNum    = 0;
}

bool FeedersConfig_Read( const std::string& filename, CarDat* data )
{
	CFileManager fm( filename );
	int retVal = fm.open( FEEDERSCONFIG_VERSION, false );

	if( retVal == -4 )
	{
		// update
	}
	else if( retVal != 1 )
	{
		return false;
	}

	for( int i = 0; i < MAXCAR; i++ )
	{
		if( !fm.readRec( data, sizeof(CarDat), i ) )
			return false;

		data++;
	}
	return true;
}

bool FeedersConfig_Write( const std::string& filename, CarDat* data )
{
	CFileManager fm( filename );
	if( fm.open( FEEDERSCONFIG_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( FEEDERSCONFIG_VERSION ) )
			return false;
	}

	for( int i = 0; i < MAXCAR; i++ )
	{
		if( !fm.writeRec( data, sizeof(CarDat), i ) )
			return false;

		data++;
	}
	return true;
}



	//----------------------------//
	// Interfaccia: Gestione file //
	//----------------------------//

//---------------------------------------------------------------------------
// Carica libreria package
//---------------------------------------------------------------------------
bool FeedersConfig_Load( const std::string& confname )
{
	if( confname.empty() )
		return false;

	char filename[MAXNPATH];

	// open packages lib
	CarPath( filename, confname.c_str() );
	if( !FeedersConfig_Read( filename, currentFeedersConfig ) )
	{
		char buf[MAXNPATH];
		snprintf( buf, MAXNPATH, MsgGetString(Msg_00923), confname.c_str() );
		W_Mess( buf );
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------
// Salva libreria package
//---------------------------------------------------------------------------
bool FeedersConfig_Save( const std::string& confname )
{
	if( confname.empty() )
		return false;

	char filename[MAXNPATH];
	bool retVal = true;

	CarPath( filename, confname.c_str() );
	if( !FeedersConfig_Write( filename, currentFeedersConfig ) )
	{
		retVal = false;
	}
	return retVal;
}

//---------------------------------------------------------------------------
// Crea nuova libreria package
//---------------------------------------------------------------------------
bool FeedersConfig_Create( const std::string& confname )
{
	if( confname.empty() )
		return false;

	char filename[MAXNPATH];

	CarDat feedersConfig[MAXCAR];

	SFeederDefault cardef[MAXCAR];
	FeedersDefault_Read( cardef );

	for( int i = 0; i < MAXCAR; i++ )
	{
		FeedersConfig_Default( &feedersConfig[i] );

		feedersConfig[i].C_codice = GetCarCode(i);
		feedersConfig[i].C_xcar = cardef[i].x;
		feedersConfig[i].C_ycar = cardef[i].y;
		feedersConfig[i].C_offprel = feedersConfig[i].C_codice < FIRSTTRAY ? 0 : 10; //mm
	}

	bool retVal = true;

	CarPath( filename, confname.c_str() );
	if( !FeedersConfig_Write( filename, feedersConfig ) )
	{
		retVal = false;
	}
	return retVal;
}



//************************************************************************
// file: dati default caricatori
//************************************************************************

	//---------------//
	// Gestione file //
	//---------------//

#define FEEDERSDEFAULT_FILE       STDFILENAME".fdef"
#define FEEDERSDEFAULT_VERSION    0

void FeedersDefault_Default( SFeederDefault* data )
{
	for( int i = 0; i < MAXCAR; i++ )
	{
		data[i].code = GetCarCode( i );
		data[i].x = 0.f;
		data[i].y = 0.f;
	}
}

bool FeedersDefault_Read( SFeederDefault* data )
{
	CFileManager fm( FEEDERSDEFAULT_FILE );
	int retVal = fm.open( FEEDERSDEFAULT_VERSION, true );

	if( retVal == -1 )
	{
		// create new file
		if( !fm.create( FEEDERSDEFAULT_VERSION ) )
			return false;

		FeedersDefault_Default( data );

		for( int i = 0; i < MAXCAR; i++ )
		{
			if( !fm.writeRec( &data[i], sizeof(SFeederDefault), i ) )
				return false;
		}

		char buf[160];
		snprintf( buf, 160, MsgGetString(Msg_07027), FEEDERSDEFAULT_FILE );
		W_Mess( buf );
		return true;
	}
	else if( retVal == -4 )
	{
		// update
	}
	else if( retVal != 1 )
	{
		return false;
	}

	for( int i = 0; i < MAXCAR; i++ )
	{
		if( !fm.readRec( &data[i], sizeof(SFeederDefault), i ) )
			return false;
	}
	return true;
}

bool FeedersDefault_Write( const SFeederDefault* data )
{
	CFileManager fm( FEEDERSDEFAULT_FILE );
	if( fm.open( FEEDERSDEFAULT_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( FEEDERSDEFAULT_VERSION ) )
			return false;
	}

	for( int i = 0; i < MAXCAR; i++ )
	{
		if( !fm.writeRec( &data[i], sizeof(SFeederDefault), i ) )
			return false;
	}
	return true;
}
