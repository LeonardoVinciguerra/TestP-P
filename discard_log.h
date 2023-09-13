//---------------------------------------------------------------------------
//
// Name:        discard_log.h
// Author:      Gabriel Ferri
// Created:     07/12/2011
// Description: Log dei componenti scartati (attivabile da debug)
//
//---------------------------------------------------------------------------

#ifndef __DISCARD_LOG_H
#define __DISCARD_LOG_H


struct DiscardLogStruct
{
	unsigned int assembled[2];
	unsigned int eTotal[2];
	// error type
	unsigned int eEmpty[2];
	unsigned int eBlocked[2];
	unsigned int eNoMin[2];
	unsigned int eBufferFull[2];
	unsigned int eEncoder[2];
	unsigned int eTooBig[2];
	unsigned int eTolerance[2];
	unsigned int eOther[2];
};


class CDiscardLog
{
public:
	CDiscardLog();
	~CDiscardLog();

	int Log( int nozzle, int errorCode, int packNum );

	bool Load();
	int Save();
	int SaveCVS();
	int Reset();

private:
	DiscardLogStruct* pLogData;
};

#endif
