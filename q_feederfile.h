//---------------------------------------------------------------------------
// Name:        q_feederfile.h
// Author:      
// Created:     
// Description: Gestione file configurazione caricatori.
//---------------------------------------------------------------------------

#ifndef __Q_FEEDERFILE_H
#define __Q_FEEDERFILE_H

#include <string>

#include "q_cost.h"

//tipi di avanzamento caricatori
#define CARAVA_NORMAL   			0
#define CARAVA_MEDIUM   			1
#define CARAVA_LONG     			2
#define CARAVA_DOUBLE   			3
#define CARAVA_NORMAL1  			4
#define CARAVA_MEDIUM1  			5
#define CARAVA_LONG1    			6
#define CARAVA_DOUBLE1  			7
#define CARAVA_DOMES				8
#define CARAVA_DOMES_FORCED_UP		9
#define CARAVA_DOMES_FORCED_DOWN	10

#define ERRFCONF          MsgGetString(Msg_00923)

//errore nessuna associazione caricatore-package
#define NOCARPACK         MsgGetString(Msg_01046)

#define PACKNOTFOUND1     MsgGetString(Msg_01717)
#define PACKNOTFOUND2     MsgGetString(Msg_01718)

#define CAR_SEARCHDIR_UP  0
#define CAR_SEARCHDIR_DW  1
#define CAR_SEARCHFAIL    -1

#ifdef __GNUC__
	#pragma pack(1)
#endif

struct CarDat
{
	short C_codice;         // cod. caricatore
	unsigned short C_quant; // contatore comp. totali su caricatore
	char C_comp[26];        // tipo componente
	char C_tipo;            // tipo caric. 0 = tape, 1 = air, 2 = thFeeder
	char C_thFeederAdd;     // indirizzo caricatore tipo thFeeder
	short C_att;            // attivazione: C/M/L
	float C_xcar;           // coord. x caricatore
	float C_ycar;           // coord. y caricatore
	unsigned short C_nx;    // n. componenti asse x
	unsigned short C_ny;    // n. componenti asse y
	float C_incx;           // increm. asse x
	float C_incy;           // increm. asse y
	int  C_sermag;          // numero seriale del magazzino a cui appartiene

	char spare;

	//CCCP
	char C_checkPos;        // flag abilitazione controllo posizione componente
	char C_checkNum;        // ogni quanti avanzamenti eseguire il controllo
	char C_checkCount;      // avanzamenti mancanti al prossimo controllo
	
	short C_avan;           // flag: 1 - avanzam. eseguito
	char C_Package[21];     // package (stringa)
	short C_PackIndex;      // index package associato
	short C_Ncomp;          // contatore componenti ancora da prelevare
	float C_offprel;        // quota di prelievo

	char C_note[25];        // note
};


#ifdef __GNUC__
	#pragma pack()
#endif

class FeederFile
{
public:
	FeederFile( char* name, bool check = true );
	~FeederFile(void);
	
	int opened;  //flag file aperto
	int Read(int code,CarDat &car);
	int SaveX(int code,CarDat car);
	int ReadRec(int nrec,CarDat &car);
	int SaveRecX(int nrec,CarDat car);
	
	void SetDefValues( int code, float posx, float posy );

	int Search(char *txt,struct CarDat &car_ret,int start=0,int dir=CAR_SEARCHDIR_DW);
	int FindNext(struct CarDat &car_ret);
	int FindPrev(struct CarDat &car_ret);
	int SearchPack(char *comp,char *pack,struct CarDat &car_ret,int start=0,int dir=CAR_SEARCHDIR_DW);
	int SearchPack(char *pack,struct CarDat &car_ret,int start=0,int dir=CAR_SEARCHDIR_DW);
	int FindNextPack(struct CarDat &car_ret);
	int FindPrevPack(struct CarDat &car_ret);

	int GetRec_AssociatedPack( int nrec, struct SPackageData& pack );
	int Get_AssociatedPack( int car, struct SPackageData& pack );

	bool SaveFile();

private:
	std::string filename;

	int lastfind;        //indice ultimo record trovato con Search
	int lastfind_pkg;    //indice ultimo record trovato con SearchPack
	int lastfind_okpkg;  //indice ultimo record trovato correttamente con SearchPack
	char lastsearch[26]; //ultimo codice componente cercato con Search
	char lastsearch_comp[26]; //ultimo codice componente cercato con SearchPack
	char lastsearch_pack[21]; //ultimo package cercato con SearchPack

	int  CheckFeederFile(void);
};

void SetFeederNCompDef(CarDat &car);

#endif
