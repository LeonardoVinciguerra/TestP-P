//---------------------------------------------------------------------------
// Name:        q_carint.h
// Author:      
// Created:     
// Description: Gestione caricatori intelligenti.
//---------------------------------------------------------------------------

#ifndef __Q_CARINT_H
#define __Q_CARINT_H

#include <map>
#include "q_tabe.h"
#include "q_cost.h"
#include "c_win_table.h"
#include "c_win_select.h"
#include "c_combolist.h"


//#define __DBNET_DEBUG

#define CARINT_MAXERR 3

#define CARINT_AUTO_OK              1
#define CARINT_AUTO_OKMANUAL        2
#define CARINT_AUTO_NOFOUND         3
#define CARINT_AUTO_NOCONF          4
#define CARINT_AUTO_WRONGPOS        5

#define CARINT_UPDATE_INITMNT 1
#define CARINT_UPDATE_NOINIT  2
#define CARINT_UPDATE_FULL    4

#define CARINT_ERR_WRONGPOS_TXT MsgGetString(Msg_02016)
#define CARINT_ERR_NOCONF_TXT   MsgGetString(Msg_02015)
#define CARINT_ERR_NOFOUND_TXT  MsgGetString(Msg_02017)
#define CARINT_OK_MANUAL_TXT    MsgGetString(Msg_00488)

#define CARINT_VALIDRECS_NUM "0123456789"
#define CARINT_VALIDRECS_STR "ABCDEFGHI"

#define CARINTAUTO_REPPOS    2,2,78,24
#define CARINT_AUTOREP_MSG   MsgGetString(Msg_01803)
#define CARINT_AUTOREP_ASK   MsgGetString(Msg_01856)
#define CARINT_AUTOSTAT_MSG1 MsgGetString(Msg_01851)
#define CARINT_AUTOSTAT_MSG2 MsgGetString(Msg_01852)
#define CARINT_AUTOSTAT_MSG3 MsgGetString(Msg_01853)
#define CARINT_AUTOSTAT_MSG4 MsgGetString(Msg_01854)
#define CARINT_AUTODUP_MSG   MsgGetString(Msg_01855)


#define CARINT_NOFOUND_ERR MsgGetString(Msg_01775)

#define CARINT_ERRCOMM     MsgGetString(Msg_01779)

#define FREE_RECORD   0
#define NEW_RECORD    0xFFFFFE

#define CARINT_NOSERIAL 0xFFFFFFFF

#define CARINT_LOCAL  0
#define CARINT_REMOTE 1

#define CARINT_TAPESER_OFFSET 		'T'
#define CARINT_GENERICSER_OFFSET 	'G'
#define CARINT_AIRSER_OFFSET  		'A'
#define CARINT_PROTCAP_OFFSET		'P' //tappo di protezione

#define NUMCIFRE  3
#define NUMELEM   7

#define CARINT_AUTOALL_MASK 0x7FFF

// Finestra tabella magazzini

// Max. n. di record in display
#define	MAXCARINT	5

#define CARINT_SOGLIA_TIP  MsgGetString(Msg_01628)
#define CARINT_SOGLIA_TXT1 MsgGetString(Msg_01629)
#define CARINT_SOGLIA_TXT2 MsgGetString(Msg_01631)
#define CARINT_SOGLIA_END  MsgGetString(Msg_01530)


// ** Menu apprendimento magazzini

#define MENUCARINTIMPH     911
#define MENUCARINTIMP_POS  29,8,45,11,MENUCARINTIMPH,1
#define MENUCARINTIMP_TXT1 MsgGetString(Msg_01809),999
#define MENUCARINTIMP_TXT2 MsgGetString(Msg_01810),999

#define FINDSER_ERR    MsgGetString(Msg_01350)
#define FINDMAG_ERR    MsgGetString(Msg_01287)
#define FINDNOTE_ERR   MsgGetString(Msg_01346)
#define FINDCAR_ERR    MsgGetString(Msg_01355)

#define MORE_NOTES  MsgGetString(Msg_01348)

// Cod magazzino gia' presente
#define DUPMAG_ERR   MsgGetString(Msg_01284)

#define CARINT_AUTODUP MsgGetString(Msg_01780)

#define  ERRCARINT	MsgGetString(Msg_01277)

// Max numero di records
#define  MAXCARINTALL   999


// Numero dei caric intell
#define  MAXCROCI     15

// Richiesta di update della libreria attuale dei caricatori
#define	ASK_UPDATECAR  MsgGetString(Msg_01304)
#define	ASK_UPDATECAR2 MsgGetString(Msg_01823)
#define	ASK_UPDATECAR3 MsgGetString(Msg_01859)

#define CARINT_RESET_WARN  MsgGetString(Msg_01655)

//abbandona procedura
#define CARUPDATE_ABORT       0
//update riga completato
#define CARUPDATE_OKROW       1
//skip di una riga
#define CARUPDATE_SKIPROW     2
//update riga completato (prosegui per tutte le altre righe senza richiesta di conferma)
#define CARUPDATE_CONFIRMALL  3
//skip di tutte le righe
#define CARUPDATE_SKIPALL     4


// Dimensioni delle immagini grabbate per le etichette
#define LABELXDIM       380
#define LABELYDIM       160

// Distanza tra riferimento e centro dell'etichetta
#define LABELOFFSET     21.8

// Richiesta di aggiornamento magazzini montati a seguito del riconoscimento
#define UPDATEMAG  MsgGetString(Msg_01305)

#define AUTOMAG_SINGLE_NOTINDB   MsgGetString(Msg_01735)

// Copia locale del data base magazzini non trovata
#define NOLOCALCARMSG  MsgGetString(Msg_01308)

// Copia remota del data base magazzini non trovata
#define NOREMOTECARMSG  MsgGetString(Msg_01309)

#define READDB_NO    0
#define READDB_YES   1

#define DBLOCAL      0
#define DBREMOTE     1

#define ASK_CONFIRM_UNMOUNT MsgGetString(Msg_02018)

#define ASK_CONFIRMDELETE  MsgGetString(Msg_01784)
#define ASK_CONFIRMDELETE2 MsgGetString(Msg_01785)

#define ASK_COPYDBLOCAL    MsgGetString(Msg_01659)
#define ASK_COPYDBLOCAL2   MsgGetString(Msg_02012)

#define ASK_UPDATEMODLOCAL MsgGetString(Msg_01658)

#define ASK_UPDATELOCAL    MsgGetString(Msg_01310)
#define ASK_UPDATEREMOTE   MsgGetString(Msg_01311)

#define ERR_NETDBUPDATE    MsgGetString(Msg_01826)

//abbandona procedura
#define DBUPDATE_ABORT       0
//update riga completato
#define DBUPDATE_OKROW       1
//skip di una riga
#define DBUPDATE_SKIPROW     2
//update riga completato (prosegui per tutte le altre righe senza richiesta di conferma)
#define DBUPDATE_CONFIRMALL  3
//skip di tutte le righe
#define DBUPDATE_SKIPALL     4

#define DBMOD_UPDATE         1
#define DBMOD_NOUPDATE       2
#define DBMOD_ABORT          0

#define CHKSUMFIX_ABORT      0
#define CHKSUMFIX_YES        1
#define CHKSUMFIX_NO         2
#define CHKSUMFIX_ALL        3

#define CARINT_CHKSUMREC      '0'
#define CARINT_WRITE_ERRMSG   MsgGetString(Msg_01774)

#define CARINT_SEARCH_NORMAL  		0
#define CARINT_SEARCH_PROTECTIONS  	1

#define CARINT_TYPE_TAPE   		0
#define CARINT_TYPE_AIR    		1
#define CARINT_TYPE_GENERIC 	2
#define CARINT_TYPE_PROTCAP		3

#define CHECKMAGA             MsgGetString(Msg_01312)

#define NOMAGACOMP            MsgGetString(Msg_01313)

#define POPMAG_ON    1
#define POPMAG_OFF   0

#define CARINT_NONET_CHANGED  1

#define WAITAUTOMAG_POS       20,10,60,15
#define WAITAUTOMAG_BARPOS    2,3,36,1
#define WAITAUTOMAG_POS2      16,10,64,14

#define WAITAUTOMAG_TXT2      MsgGetString(Msg_01770)
#define CARINT_AUTOABORT      MsgGetString(Msg_01771)
#define CARINT_CHKSUM_ERRMSG  MsgGetString(Msg_01772)

#define WAITCOMPACTDB_TXT     MsgGetString(Msg_01740)
#define WAITLOADDB_TXT        MsgGetString(Msg_01741)

//tempo limite in sec. oltre il quale non tentare la riapertura del file
//database in caso di errore.
#define CARINT_WAITOPEN_TMR 5.0

#define CARINTFILE_ERR_SHOW          0x01                        //in caso di errore mostra avviso
#define CARINTFILE_ERR_ASKRETRY      (0x03 | CARINTFILE_ERR_SHOW)//in caso di errore chiede ritentativo
#define CARINTFILE_ERR_DISABLENET    0x04                        //in caso di errore disabilita rete
#define CARINTFILE_ERR_ASKDISABLENET 0x0C                        //in caso di errore chiede se disabilitare rete

#define TXTREMOTE                      MsgGetString(Msg_01662)
#define TXTLOCAL                       MsgGetString(Msg_01663)
#define TXTRETRY_REMOTE                MsgGetString(Msg_01664)
#define TXTRETRY_LOCAL                 MsgGetString(Msg_01665)
#define CARINTFILE_ERR                 MsgGetString(Msg_01656)

#define CARINT_NOREF                   MsgGetString(Msg_01706)

#define CONFIMPORT_ABORT          MsgGetString(Msg_01807)
#define CONFIMPORT_OK             MsgGetString(Msg_01808)

#define CARINT_FIELD_TIPCOM       1
#define CARINT_FIELD_PACKAGE      2
#define CARINT_FIELD_AVTYPE       4
#define CARINT_FIELD_TYPE         8
#define CARINT_FIELD_QUANT       16
#define CARINT_FIELD_NOTE        32

#define MAG_USEDBY_NONE           0
#define MAG_USEDBY_ME             1
#define MAG_USEDBY_OTHERS         2


struct CarInt_data
{
	int idx;                  // Numero del record
	int mounted;              // Se diverso da 0, indica la posizione in cui e' montato (1..15)
	unsigned char type;       // Tipo caricatore (normal,electronic,vision)
	unsigned char smart; 	  // Caricatore intelligente si/no	
	unsigned short dummy;	
	char elem[8][26];         // Lista di tipi di componenti associati al caricatore
	char pack[8][21];         // Nome del package
	char noteCar[8][21];      // Note dei singoli caricatori
	unsigned short tipoCar[8];// tipo caric. 0 = tape, 1 = air
	unsigned short tavanz[8]; // attivazione: C/M/L
	int packIdx[8];           // Indice del package
	int num[8];               // Numero di componenti della bobina/stecca
	char note[43];            // Nota per il magazzino caricatori
	int serialUser;
	int serial;               // Numero seriale
	unsigned char changed;    // flag modificato a rete disabilitata
	unsigned char usedBy;
	unsigned short int checksum;  //Checksum dei dati caricatore   
};


// Struttura della posizione di default dei riferimenti dei caric intell
struct CfgCaricInt
{
	int C_cod;
	float C_posx_bob;
	float C_posy_bob;
	float C_posx_air;
	float C_posy_air;
};

struct CarIntAuto_res
{
  int ser[MAXMAG];
  unsigned int chksum[MAXMAG];
  int type[MAXMAG];
  int swver[MAXMAG];
};

class CarIntFile
{
  protected:
    int type;

    int lockOpen;

    char name[MAXNPATH];

    int HandleCar;
    int Caroffset;

    int GetLen(void);

    char err_askretry[256];
    char err_show[256];

    int TypeFlagOk;
    int _CheckTypeFlagDone(void);
    int _SetTypeFlagDone(void);
   
  public:
    CarIntFile(void);
    CarIntFile(int _type);
    ~CarIntFile();

    int IsOpen(void);

    void SetDefault(struct CarInt_data &car);

    int LockOpen(int errmode=0);
    int UnLockClose(void);

    int Open(int errmode=0);
    int OpenCur(void);

    int Create(int errmode=0);
    int Close(void);

    int Count(void);

    int Resize(int nRec);

    int InitList(struct CarInt_data **list,int **flist,int &count,int &count2);

    int ReadAll(struct CarInt_data *Car_Tab,int errmode=0);
    int Read(struct CarInt_data &Car_Tab, int RecNum,int errmode=0);

    int WriteAll(struct CarInt_data *Car_Tab,int errmode=0);
    int Write(struct CarInt_data Car_Tab, int RecNum, int Flush,int errmode=0);

    #ifdef __DBNET_DEBUG
    void Check_Log(struct CarInt_data Car_Tab,int RecNum,int mode);
    void WriteLogMsg(char *msg);
    #endif

    int CheckTypeFlagDone(int errmode=0);
    int SetTypeFlagDone(int errmode=0);
    int UpdateTypeFlag(int errmode=0);
};


#define ERRMAG_USEDBY_OTHERS  MsgGetString(Msg_02010)
#define ERRMAG_USEDBY_OTHERS2 MsgGetString(Msg_02011)
#define ERRMAG_USEDBY_OTHERS3 MsgGetString(Msg_02009)

#define ERRMAG_USEDBY_OTHERS_CONFIRM1 MsgGetString(Msg_02043)
#define ERRMAG_USEDBY_OTHERS_CONFIRM2 MsgGetString(Msg_02044)


extern CarIntFile DBLocal;
extern CarIntFile DBRemote;

//Riconoscimento dei magazzini
void CarIntAutoMag(int mode,int mask,struct CarIntAuto_res &results);

int CarIntAuto_ElabResults(struct CarIntAuto_res results,int elabmask,int mode=1);

//Ritorna l'indice dell'elemento del database che risulta montato
//nella posizione specificata
int GetConfDBLink(int mag);

int DBToMemList_Idx(int idx);

int IsConfDBLinkOk(void);


//controlla e allinea i database locale e remoto
int  UpdateDBData(int mode=0);
int  UpdateCaricLib(void);

//Importa da configurazione
int ConfImport(int mode=1);

// Decrementa il numero di componenti del caricatore associato al magazzino
void DecMagaComp(int code);
void UpdateMagaComp(void);

// Inizializza il vettore di magazzini montati sulla macchina attuale
int CarInt(int mode=0,int initsel_mag=-1);

//Alloca e inizalizza strutture dati
int InitCarList(void);

//Dealloca strutture dati
void DeAllocCarInt(void);

//copia database locale su database remoto
void CopyLocalDB(void);

int CarInt_GetSerial(int caric,int &serial,unsigned int &chksum,int &type,int &swver);

int CarInt_SetData(unsigned int caric,char record,char *data);
int CarInt_SetData(unsigned int caric,char record,unsigned int data);

unsigned short int CarInt_CalcChecksum(struct CarInt_data rec);

#ifdef __DBNET_DEBUG
void DBNet_DebugOpen(void);
void DBNet_DebugClose(void);
#endif

int CarList_GetNRec(void);

void DisableDB(void);

int CarInt_SetAsUsed(int n,struct CarInt_data *loc_update=NULL);
int CarInt_SetAsUsed(CarInt_data &mag);
int CarInt_SetAsUnused(CarInt_data &mag,int forced=0);
int CarInt_SetAllAsUnused(void);
int CarInt_GetUsedState(CarInt_data &mag,int remote_idx=-99);


void EnableFeederDB();
void DisableFeederDB();

int FeedersTest();



//---------------------------------------------------------------------------
// finestra: Caricatore corrente (associato al magazzino)
//---------------------------------------------------------------------------
class CurrentFeederUI : public CWindowTable
{
public:
	CurrentFeederUI( CWindow* parent );

	void SelectFirstCell();
	void DeselectCells();
	void UpdateFeeder();

protected:
	void onInit();
	void onRefresh();
	void onEdit();
	void onShowMenu();
	bool onKeyPress( int key );

private:
	int OnDelete();
	int OnPackagesLib();
};


//---------------------------------------------------------------------------
// finestra: Feeders database
//---------------------------------------------------------------------------
class FeedersDatabaseUI : public CWindowTable
{
public:
	FeedersDatabaseUI( CWindow* parent, bool select );
	~FeedersDatabaseUI();

	int GetExitCode() { return m_exitCode; }

protected:
	void onInit();
	void onShow();
	void onRefresh();
	void onEdit();
	void onShowMenu();
	bool onKeyPress( int key );
	void onClose();

private:
	int onEditOverride();
	int onFeederData();
	int onFindSerial();
	int toSharedFolder();
	int toUSBDevice();

	bool exportInventory( char* filename );
	void forceRefresh();
	bool vSelect( int key );
	void showUsedBy();

	struct _inventoryItem
	{
		char name[26];
		char pack[26];
		unsigned int quant;
	};

	int m_exitCode;
	bool selectMode;

	CurrentFeederUI* assFeederWin;

	GUI_SubMenu* SM_IventoryExport; // sub menu esporta inventario
};


//---------------------------------------------------------------------------
// finestra: Feeder advanced search
//---------------------------------------------------------------------------
class FeederAdvancedSearchUI : public CWindowTable
{
public:
	FeederAdvancedSearchUI( CWindow* parent );
	~FeederAdvancedSearchUI();

	bool Search( char* mask );

protected:
	void onInit();
	void onShow();
	void onRefresh();
	bool onKeyPress( int key );

private:
	bool onKeyDown();
	bool onKeyUp();
	void showUsedBy();

	struct _searchNode
	{
		struct _searchNode* prev;
		struct _searchNode* next;
		int nruota;
		CarInt_data* data;
	};

    int m_totQuant;
    struct CarInt_data* m_db;
    struct _searchNode* m_Results;
    struct _searchNode* m_ResultsEnd;
    struct _searchNode* m_curSel;
    struct _searchNode* m_TabStart;
};


//---------------------------------------------------------------------------
// finestra: Programs manager
//---------------------------------------------------------------------------
class FeederImportUI : public CWindowSelect
{
public:
	FeederImportUI( CWindow* parent, int num );
	~FeederImportUI();

	typedef enum
	{
		FEEDER_PACK,
		FEEDER_NOTES
	} combo_labels;

protected:
	virtual void onInitSomething();
	virtual void onShowSomething();

private:
	std::map<int,C_Combo*> combos;
	CComboList* comboList;
	CTable* infoTable;
	int feederPackNumber;

	int onSelectionChange( unsigned int row, unsigned int col );
};


#endif
