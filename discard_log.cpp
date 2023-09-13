//---------------------------------------------------------------------------
//
// Name:        discard_log.cpp
// Author:      Gabriel Ferri
// Created:     07/12/2011
// Description: Log dei componenti scartati
//
//---------------------------------------------------------------------------
#include "discard_log.h"

#include "filemanager.h"
#include "q_cost.h"
#include "q_packages.h"
#include "tws_sniper.h"


//---------------------------------------------------------------------------
// Files names

#define DISCARD_LOG_FILE          "discard.log"
#define DISCARD_LOG_FILE_CVS      "discard.txt"


//---------------------------------------------------------------------------
// Files current version

#define DISCARD_LOG_VERSION       0


extern SPackageData currentLibPackages[MAXPACK];



CDiscardLog::CDiscardLog()
{
	pLogData = NULL;
}

CDiscardLog::~CDiscardLog()
{
	if( pLogData )
	{
		delete [] pLogData;
	}
}

//--------------------------------------------------------------------------
// Carica dati report errori assemblaggio
//--------------------------------------------------------------------------
bool CDiscardLog::Load()
{
	if( pLogData )
	{
		delete [] pLogData;
		pLogData = 0;
	}

	pLogData = new DiscardLogStruct[MAXPACK];



	CFileManager fm( DISCARD_LOG_FILE );
	int retVal = fm.open( DISCARD_LOG_VERSION, true );

	if( retVal == -1 )
	{
		// create new file
		if( !fm.create( DISCARD_LOG_VERSION ) )
			return false;

		memset( pLogData, 0, MAXPACK*sizeof(DiscardLogStruct) );

		for( int i = 0; i < MAXPACK; i++ )
		{
			if( !fm.writeRec( pLogData+i, sizeof(DiscardLogStruct), i ) )
				return false;
		}
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

	for( int i = 0; i < MAXPACK; i++ )
	{
		if( !fm.readRec( pLogData+i, sizeof(DiscardLogStruct), i ) )
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------
// Salva dati report errori assemblaggio
//--------------------------------------------------------------------------
int CDiscardLog::Save()
{
	if( !pLogData )
		return 1;

	CFileManager fm( DISCARD_LOG_FILE );
	if( fm.open( DISCARD_LOG_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( DISCARD_LOG_VERSION ) )
			return false;
	}

	for( int i = 0; i < MAXPACK; i++ )
	{
		if( !fm.writeRec( pLogData+i, sizeof(DiscardLogStruct), i ) )
			return false;
	}
	return true;
}

int CDiscardLog::SaveCVS()
{
	if( !pLogData )
		return 1;

	CFileManager fm( DISCARD_LOG_FILE_CVS );
	if( fm.open( DISCARD_LOG_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( DISCARD_LOG_VERSION ) )
			return false;
	}

	FILE* pFile = fm.GetFD();
	fprintf( pFile, "\n" );

	DiscardLogStruct* pPack = pLogData;

	for( int i = 0; i < MAXPACK; i++ )
	{
		if( currentLibPackages[i].name[0] != 0 )
		{
			fprintf( pFile, "%s;", currentLibPackages[i].name );
			fprintf( pFile, "P1;%d;%d;", pPack->assembled[0], pPack->eTotal[0] );
			fprintf( pFile, "E1;%d;%d;%d;%d;%d;%d;%d;%d;",
					pPack->eEmpty[0], pPack->eBlocked[0], pPack->eNoMin[0],
					pPack->eBufferFull[0], pPack->eEncoder[0], pPack->eTooBig[0],
					pPack->eTolerance[0], pPack->eOther[0] );
			fprintf( pFile, "P2;%d;%d;", pPack->assembled[1], pPack->eTotal[1] );
			fprintf( pFile, "E2;%d;%d;%d;%d;%d;%d;%d;%d;",
					pPack->eEmpty[1], pPack->eBlocked[1], pPack->eNoMin[1],
					pPack->eBufferFull[1], pPack->eEncoder[1], pPack->eTooBig[1],
					pPack->eTolerance[1], pPack->eOther[1] );
			fprintf( pFile, "\n" );
		}

		pPack++;
	}

	return 1;
}

//--------------------------------------------------------------------------
// Reset dati report errori assemblaggio
//--------------------------------------------------------------------------
int CDiscardLog::Reset()
{
	if( !pLogData )
		return 1;

	memset( pLogData, 0, MAXPACK*sizeof(DiscardLogStruct) );
	Save();
	return 1;
}


int CDiscardLog::Log( int nozzle, int errorCode, int packNum )
{
	if( !pLogData )
		return 1;

	pLogData[packNum].assembled[nozzle-1]++;

	if( errorCode != CEN_ERR_NONE )
	{
		pLogData[packNum].eTotal[nozzle-1]++;

		switch( errorCode )
		{
		case CEN_ERR_EMPTY:
			pLogData[packNum].eEmpty[nozzle-1]++;
			break;

		case CEN_ERR_L_BLOCKED:
		case CEN_ERR_R_BLOCKED:
		case CEN_ERR_B_BLOCKED:
			pLogData[packNum].eBlocked[nozzle-1]++;
			break;

		case CEN_ERR_NOMIN:
			pLogData[packNum].eNoMin[nozzle-1]++;
			break;

		case CEN_ERR_BUF_FULL:
			pLogData[packNum].eBufferFull[nozzle-1]++;
			break;

		case CEN_ERR_ENCODER:
			pLogData[packNum].eEncoder[nozzle-1]++;
			break;

		case CEN_ERR_TOO_BIG:
			pLogData[packNum].eTooBig[nozzle-1]++;
			break;

		case CEN_ERR_DIMX:
			pLogData[packNum].eTolerance[nozzle-1]++;
			break;

		case CEN_ERR_DIMY:
			pLogData[packNum].eTolerance[nozzle-1]++;
			break;

		default:
			pLogData[packNum].eOther[nozzle-1]++;
			break;
		}
	}

	return 1;
}
