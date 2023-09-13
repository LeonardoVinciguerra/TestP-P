//---------------------------------------------------------------------------
//
// Name:        q_feeders.h
// Author:      Gabriel Ferri
// Created:     13/03/2012
// Description: Quadra feeders manager
//
//---------------------------------------------------------------------------

#ifndef __Q_FEEDERS_H
#define __Q_FEEDERS_H

#include <string>



//************************************************************************
// file: configurazione caricatori
//************************************************************************
/*
#pragma pack(1)

struct SFeederData
{
	short code;           // cod. caricatore
	char notes[25];

	char type;            // tipo caric. 0 = tape, 1 = air, 2 = thFeeder
	char activation;      // attivazione: C/M/L
	char address;         // indirizzo caricatore tipo thFeeder

	int serial;           // numero seriale del magazzino a cui appartiene

	float x;              // coord. x caricatore
	float y;              // coord. y caricatore
	float pickOffset;     // offset posizione di prelievo
	float xMeasure;       // coord. x di misura
	float yMeasure;       // coord. y di misura

	unsigned short numX;  // n. componenti asse x
	unsigned short numY;  // n. componenti asse y
	float incX;           // increm. asse x
	float incY;           // increm. asse y

	char component[26];   // tipo componente
	unsigned short tot;   // contatore comp. totali su caricatore
	short left;           // contatore componenti ancora da prelevare

	char checkPos;        // flag abilitazione controllo posizione componente
	char checkNum;        // ogni quanti avanzamenti eseguire il controllo
	char checkCount;      // avanzamenti mancanti al prossimo controllo

	char package[22];     // package (stringa)
	short packageIndex;   // index package associato

	char spare[27];
};

#pragma pack()
*/

bool FeedersConfig_Load( const std::string& confname );
bool FeedersConfig_Save( const std::string& confname );
bool FeedersConfig_Create( const std::string& confname );



//************************************************************************
// file: dati default caricatori
//************************************************************************
#pragma pack(1)

struct SFeederDefault
{
	short code;
	float x;
	float y;
};

#pragma pack()


bool FeedersDefault_Read( SFeederDefault* data );
bool FeedersDefault_Write( const SFeederDefault* data );

#endif
