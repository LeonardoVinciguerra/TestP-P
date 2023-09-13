/*
>>>> Q_GENER.H

Dichiarazione delle funzioni di utilit� generale pubbliche ai moduli

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

*/


#if !defined(__Q_GENER_)
#define	__Q_GENER_

#include <vector>
#include <string>

#include <stdio.h>
#include <assert.h>

#include "q_cost.h"

#include "c_window.h"
#include "gui_progressbar.h"

#include "comaddr.h"

#include <mss.h>


#ifdef __SNIPER
#define LASANGLE(x) ((x==0) ? -90 : 0)
#endif

//#define assert2(exp,failop) if(!(exp)){ failop; waitKeyPress();} assert(exp)
#define assert2(exp,failop) if(!(exp)){ failop; } assert(exp)


#define SORTFIELDTYPE_CHAR       0
#define SORTFIELDTYPE_INT16      1
#define SORTFIELDTYPE_INT32      2
#define SORTFIELDTYPE_FLOAT32    3
#define SORTFIELDTYPE_STRING     4

#define SORTFIELDTYPE_SIGNED     0x00
#define SORTFIELDTYPE_UNSIGNED   0x10

#define BEEPLONG  0
#define BEEPSHORT 1


struct VertexType
{
	float cx,cy;
	float x[4];
	float y[4];
	float z;
	unsigned int enable;
	void* AuxData;
};

extern int package_in_clipboard;

// Emette un bip
void bip( int type = BEEPSHORT );
// Emette un suono bitonale di avvertimento
void bipbip();
// Emette uno squillo di avviso
void beep();

// Attende pressione tasto e converte in codice esteso
int Handle( bool wait = true );

//* Aggiunto tipo per gestire estensioni diverse (dati relativi al programma).
extern void PrgPath(char *D_File_Name, const char *S_File_Name,int tipo=0);
//* Ritorna il nomefile caricatori compreso di path.
extern void CarPath(char *D_File_Name, const char *S_File_Name);
//* Ritorna il nomefile packages (dati dosatore) compreso di path.
#ifndef __DISP2
extern void PackDosPath(char *D_File_Name,const char *S_File_Name);
#else
extern void PackDosPath(int ndisp,char *D_File_Name,const char *S_File_Name);
#endif

//ritorna true se il carattere appartiene all'insieme di caratteri legal
int ischar_legal(const char *legal,int c);

// centra la stringa nella lunghezza len. //L140999
void Centre_string(char *s, int len);

// Integr. Loris
bool Esc_Press(void);

void InsSpc(int len,char *txt);
void InsSpc(int len,std::string& txt,int pos=-1);
void Pad(char *str,int n,int ch);
void AllignR(int len,char *txt);
char* trimwhitespace(char *str);

int GetCarRec(int ck_code);
int GetCarCode(int rec_num);
int IsValidCarCode(int code);

unsigned int Hex2Val( char* hexStr );
void Val2Hex(unsigned int d_num, int n_bytes, char *ex_num,int endstring=1); //##SMOD060902
int Potenza(int x,int y);


float angleconv(int ncaric,float angle);

int Deg2Step( float deg, int nozzle );
float Step2Deg( int step, int nozzle );

//----------------------------------------------------------------------------

void SortData( void *recset,int type,int nrec,int offset,int dim,int showwait=1 );

int Set_RipData(unsigned int &ripcomp,int numerorecs,const char *title);


char* RemovePathString(char *path);

void CheckBackupDir(char *cust=NULL);

#define CHECKDIR_CREATE 1
#define CHECKDIR_NORMAL 0

//conta il numero dei file all'interno di una directory con recursione
int DirCount(const char *dir);

//copia di una directory
int DirCopy(const char *dest,const char *orig,GUI_ProgressBar_OLD *progbar=NULL);

//check esistenza di una directory generica
int CheckDir(const char *dir,int mode=CHECKDIR_NORMAL);

//confronto di una stringa con una maschera di riferimento
int strcmp_wildcard(char *str,char *mask);

//dati tre punti ritorna 1 se sono posizionati in senso antiorario
//-1 se in senso orario
int PointCCW(float x1,float y1,float x2,float y2,float x3,float y3);

//dati due segmenti ritorna 1 se si intersecano, 0 altrimenti
int SegmentIntersect(float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4);

//Calcola i giorni trascorsi tra due date
int DiffDate(struct date date1,struct date date2);
int FirstDate(struct date d1,struct date d2);

void copyTextToClipboard( const char* txt, int type = 0 );
const char* pasteTextFromClipboard();
int getClipboardTextType();


float CalcMoveTime(float s,float vmin,float vmax,float acc);
float CalcSpaceInTime(float t,float smax,float vmin,float vmax,float acc);

bool is_mount_ready(const char* mnt);
int getAllUsbMountPoints(std::vector<std::string>& m);
const char* getMountPointName(const char* mnt);

bool isDeviceRemovable(char* dev);


bool IsSharedDirMounted();


int accessQ(const char* path, int type, bool ignore_case = false);



int ToolSelectionWin( CWindow* parent, char& tool, int nozzle );
int ToolNozzleSelectionWin( CWindow* parent, char& tool, int& nozzle );

// Stampa dati sulle barre di stato
void ShowCurrentData();

const char* getMotorheadComPort();
const char* getStepperAuxComPort();

#endif
