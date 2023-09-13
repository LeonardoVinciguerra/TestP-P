/*

>>>> Q_PROG.CPP

Gestione della tabella del programma di assemblaggio

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++  	Modificato da TWS Simone 06.08.96
++++     Modificato da Walter 02.04.97 per fix bugs in edit ** W0204
++++     Modif. W042000

*/
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>

#include "filefn.h"
#include "msglist.h"
#include "q_cost.h"
#include "q_debug.h"
#include "q_net.h"
#include "q_wind.h"
#include "q_fox.h"
#include "q_carint.h" //SMOD290703-CARINT
#include "q_prog.h"
#include "q_progt.h"
#include "q_tabe.h"
#include "q_tabet.h"
#include "q_help.h"
#include "q_oper.h"
#include "q_files.h"
#include "q_assem.h"
#include "q_caric.h"
#include "q_carobj.h"
#include <unistd.h>
#include "q_zerit.h"
#include "q_zeri.h"
#include "q_conf.h"
#include "q_conf_new.h"
#include "q_packages.h"
#include "q_feeders.h"
#include "q_grcol.h"
#include "q_conveyor.h"

#ifdef __SNIPER
#include "sniper.h"
#endif

#include "q_ugeobj.h"
#include "q_dosat.h"
#include "q_opt2.h"
#include "q_init.h"
#include "q_inifile.h"
#include "strutils.h"
#include "keyutils.h"
#include "lnxdefs.h"

#include "c_inputbox.h"
#include "c_win_imgpar.h"
#include "gui_defs.h"
#include "gui_functions.h"
#include "gui_desktop.h"

#include "fileutils.h"

#include <mss.h>



#define  PRG_COLOR_MOUNT_BG         249,254,130
#define  PRG_COLOR_DOSA_BG          0,255,255


#define  PHASE_TXT      MsgGetString(Msg_01069)


extern GUI_DeskTop* guiDeskTop;

bool programTableRefresh = false;
ProgramTableUI* programWin = 0;

extern struct cur_data CurDat;

extern int   nomountP2;
extern int   lastmountP1;
extern int   lastmountP2;

void Prg_SwitchTable(int mode);
int  CreateDosaFile(void);


extern FeederFile* CarFile;
extern SPackageData currentLibPackages[MAXPACK];
extern struct CarInt_data* CarList;

int nboard, ncomp;

// Le strutture di dati per I/O su file sono definite in q_tabe.h.
GUI_SubMenu *Q_MenuSSearch;

GUI_SubMenu *Q_MenuAut;    // Zeri scheda manuale aut. - VISIONE MASSIMONE 991015
GUI_SubMenu *Q_MenuAsse;   // Assemblaggio
GUI_SubMenu *Q_MenuRio[2]; // Riordino righe prog.
GUI_SubMenu *Q_SubMenuRio; // Sotto menu riordino (ottimizzazione)

//GF_14_07_2011
GUI_SubMenu* Q_MenuMountAll; // Sottomenu "Assembla tutto"
GUI_SubMenu* Q_MenuDispAll;  // Sottomenu "Dispensa tutto"

GUI_SubMenu* Q_MenuFiducials; // Sottomenu "Fiducials"

GUI_SubMenu* Q_MenuSplit; // Sottomenu "Split"

GUI_SubMenu* Q_MenuChangeCoord; // Sottomenu "Change coordinates"

#ifdef __DISP2
GUI_SubMenu *Q_MenuDas1;
GUI_SubMenu *Q_MenuDist1;
GUI_SubMenu *Q_MenuDas2;
GUI_SubMenu *Q_MenuDist2;
GUI_SubMenu *Q_MenuD1D2As;
GUI_SubMenu *Q_MenuD1AsD2;
#endif
GUI_SubMenu *Q_MenuDas;    // Dosaggio & Assemb. - W3107
GUI_SubMenu *Q_MenuCarSeq; // Apprendimento sequenziale caricatori
GUI_SubMenu *Q_SubMenuCarZSeq; // Apprendimento sequenziale caricatori Z
GUI_SubMenu *Q_MenuDist;   // Dosaggio
GUI_SubMenu *Q_MenuZPos;   // check origine z
GUI_SubMenu *Q_MenuZTheta; // check origine theta
GUI_SubMenu *Q_MenuCheckOri; //check origini


struct CarDat PP_Caric;  	          // struct dati caricatori
struct TabPrg ATab[MAXRECS];         // Array di struct per display records
extern struct CfgHeader QHeader;     // structs memo parametri vari.
extern struct CfgParam  QParam;

//SMOD260903
extern int* ComponentList;
extern unsigned int NComponent;
extern unsigned int PlacedNComp;

extern int lastZTheta[2];

int CurRow;
int CurRecord;
int AFlag=0;
int lastCodCompFound = -1;

int Reccount;                   // N. tot. di records del programma
int Prg_Flag=0;

int dosaAss=0;                  //flag dosaggio e assemblaggio in corso

#ifdef __DISP2
int dosa12Ass=0;
int Dosa12Ass_phase=0;
tDosAssIdx Dosa12Ass_seqvector[3]={DOSASS_IDX_NONE,DOSASS_IDX_NONE,DOSASS_IDX_NONE};
#endif

CWindow* checkprg_wait = NULL;

int Prg_OpenFiles( bool check );
void Prg_CloseFiles(void);    //##SMOD011002
int check_data(int mode);
int CheckZeriProg(void);      //SMOD021003
int UpdateAssemblyFile(GUI_ProgressBar_OLD *progress=NULL);
int CheckDupCompcod(void);
int CheckSomethingToMount(void);
int AddAssemblyRec(struct TabPrg newrec);
int Check_OptFile(void);
int Prg_OptSort(int mode,int numPunte);

TPrgFile* TPrg = 0;
TPrgFile* prevTPrg = 0;

TPrgFile* TPrgExp = 0;
TPrgFile* TPrgDosa = 0;
TPrgFile* TPrgNormal = 0;

int menuIdx;

int updateDosa=0;

//flag richiesta conferma ad ogni modifica riscontrata
int prgupdateAskFlag=1;

// Legge i records dal file di programma e carica l'array di structs.
void ReadRecs(int Roffset)
{
	register int floop = 0;   // counter - record on file

	while (floop<=MAXRECS-1)
	{
		if(Roffset+floop>=Reccount)
		{
			break;
		}

		if(!TPrg->Read(ATab[floop],Roffset+floop))
		{
			break; // if eof()
		}

		if(ATab[floop].Caric>LASTTRAY)
		{
			ATab[floop].Caric=0;
			TPrg->Write(ATab[floop],Roffset+floop,FLUSHON);
		}

		floop++;
	}
}



//---------------------------------------------------------------------------
//* Funzioni di apprendimento componenti

// apprendimento sequenziale posizione componenti
void Prg_SeqTeachPosition()
{
	int x_count=CurRecord;
	struct TabPrg tabrec;
	
	if( TPrg == TPrgDosa )
	{
		bipbip();
		return;
	}

	// setta i parametri della telecamera
	SetImgBrightCont( CurDat.HeadBright, CurDat.HeadContrast );

	Set_Tv(2);         //attiva visualizzazione senza interruzioni
	
	//EnableTimerConfirmRequiredBeforeNextXYMovement(true);
	
	while(x_count<Reccount)
	{
		TPrg->Read(tabrec,x_count);
		if((tabrec.status & MOUNT_MASK) && (!(tabrec.status & NOMNTBRD_MASK)))
		{
			if(!Prg_Autocomp(AUTO_COMPREC,x_count,TPrg))
			{
				break;
			}
		}
	
		x_count++;
	}
	
	//EnableTimerConfirmRequiredBeforeNextXYMovement(false);
	
	Set_Tv(3);  //disattiva visualizzazione se presente
	
	if(TPrg==TPrgNormal)
	{
		UpdateAssemblyFile();
	}
	
	ReadRecs(CurRecord-CurRow);
	programTableRefresh = true;
}
//---------------------------------------------------------------------------


// Check del campo chiave tipo componente
// ritorna 0 per campo vuoto ( blanks only )
int chk_tipo(int num)
{
	if( strcmp(ATab[num].TipCom,"              ") == 0)
	{
		return 0;
	}
	return 1;
}

// Scrolla i valori dell'array di structs di una riga in Direction.
// 0 up / 1 down
// mod. x package - wmd0
void DatScroll(int Direction)
{
    int Indent = 0;
    if(Direction) Indent = 1;
    memmove(&ATab[Indent],&ATab[!Indent],sizeof(ATab)-sizeof(ATab[0]));
}

//---------------------------------------------------------------------------
// * Init/Reset dei dati

// deseleziona tutti i record
void Prg_Desel(void)
{
	struct TabPrg X_Tab;
	int r_loop;
	for(r_loop=0;r_loop<Reccount;r_loop++)
   {
		TPrg->Read(X_Tab,r_loop);
		if(X_Tab.status & (RDEL_MASK | RMOV_MASK))
      {
			X_Tab.status&=~(RDEL_MASK | RMOV_MASK);
			TPrg->Write(X_Tab,r_loop,FLUSHON); // aggiorno il record preced.
		}
	}
}


// Inizializza l'elemento RecNum nell'array di visualizzazione della tabella
// con i valori di default. Setta elemento riga con riga specificata
void Prg_InitAll(int RecNum, int riga)
{
	// RecNum == N. del record  /  riga == N. della riga di programma
	ATab[RecNum].Riga = riga;
	*ATab[RecNum].CodCom=0;
	*ATab[RecNum].TipCom=0;
	ATab[RecNum].XMon = 0;
	ATab[RecNum].YMon = 0;
	ATab[RecNum].Rotaz = 0;
	ATab[RecNum].Punta='1';
	ATab[RecNum].Caric=0;

	ATab[RecNum].status=1;

	ATab[RecNum].scheda=1;

	*ATab[RecNum].NoteC=0;

	ATab[RecNum].pack_txt[0]=0;
	ATab[RecNum].Uge=-1;
}

//azzera tutta la tabella di progrramma (in memoria)
void Prg_ClearAll(void)
{
	for(int xx=0;xx<MAXRECS;xx++)
	{
		Prg_InitAll(xx,0);              // init di tutte le righe con valore di riga=0
	}
}

//ricarica dall'inizio il file corrente
void Prg_Reload(void)
{
	Reccount = TPrg->Count();         // n. di records
	Prg_ClearAll();
	ReadRecs(0);  				         // lettura dell'array di structs
}

//---------------------------------------------------------------------------

// Ritorna il n�. del primo record evidenziato con CR
// (ritorna -1 per nessun record trovato).
int Prg_FindSel(int mask,int start=0)
{

	struct TabPrg X_Tab[1];
	int r_loop;


	for(r_loop=start;r_loop<Reccount;r_loop++) {
		TPrg->Read(X_Tab[0],r_loop);
		if(X_Tab[0].status & mask) return(r_loop);
	}
	return(-1);
}

//---------------------------------------------------------------------------
// * Operazioni in tabella

//cancella il record correte
//DANY261102
void Prg_Delete()
{
	GUI_Freeze_Locker lock;

	int del_rec=CurRecord;
	int pointer=0;
	struct TabPrg tabrec;

	if(CurRecord<Reccount && Reccount>0)        // se non � un record append...
	{
		if(W_Deci(0, MsgGetString(Msg_00183) ))
		{
			TPrg->Read(tabrec,del_rec);
			TPrg->DelRec(del_rec);               // elimina record

			pointer=TPrgExp->Search(pointer,tabrec,PRG_CODSEARCH);
			while(pointer!=-1)
			{
				TPrgExp->Read(tabrec,pointer);
				tabrec.status|=RDEL_MASK;
				TPrgExp->Write(tabrec,pointer,FLUSHON);
				pointer++;
				pointer=TPrgExp->Search(pointer,tabrec,PRG_CODSEARCH);
			}

			UpdateAssemblyFile();

			Reccount = TPrg->Count();                // n. di records

			Prg_ClearAll();                          //svuota tabella

			if(Reccount==0)                          //se eliminati tutti i records
				Prg_InitAll(0,1);                      //setta riga di default
			else
			{
				// se record da elim=ultimo rec ed e' presente piu di un record
				if((CurRecord==Reccount) && (Reccount>0))
				{
					CurRecord--;                       // decrementa indici di posione su file e
					CurRow--;                          // in tabella
				}

				if(CurRow==-1)
				{
					//CurRecord--;
					CurRow=0;
				}
			}

			ReadRecs(CurRecord-CurRow); 		       //legge i records
			programTableRefresh = true;
		}
	}
}

// cancella tutti i record selezionati
//DANY261102
int Prg_DelBlk(void)
{
	GUI_Freeze_Locker lock;

	struct TabPrg tabDat;
	int pointer=0;
	int doUpdate=0; //SMOD120503

	if(W_Deci(0, MsgGetString(Msg_00183) ))
	{
		//Ricerca degli codici da cancellare
	
		for(int r_loop=0;r_loop<TPrg->Count();r_loop++)
		{
			TPrg->Read(tabDat,r_loop);
			if(tabDat.status & RDEL_MASK)//se riga da eliminare
			{
				pointer=0;
				while((pointer=TPrgExp->Search(pointer,tabDat,PRG_CODSEARCH))!=-1)
				{
					TPrgExp->Read(tabDat,pointer);
					tabDat.status|=RDEL_MASK;
					TPrgExp->Write(tabDat,pointer,FLUSHON);
					pointer++;
					doUpdate=1; //SMOD120503
				}
			}
		}
	
		if(doUpdate) //SMOD120503
			UpdateAssemblyFile();
	
		Reccount=TPrg->DelSel();
	
		Prg_ClearAll();             //svuota tabella
		if(Reccount==0)             //se eliminati tutti i records
		Prg_InitAll(0,1);             //setta riga di default
		ReadRecs(0);                //legge e mostra tabella dall'inizio
		CurRow=0;
		CurRecord=0;
		programTableRefresh = true;
	
		return 1;
	}

	return 0;
}

bool Prg_onEnter()
{
	GUI_Freeze_Locker lock;

	if( CurRow < Reccount && !AFlag )
	{
		if(ATab[CurRow].status & (RDEL_MASK | RMOV_MASK))
		{
			ATab[CurRow].status&=~(RDEL_MASK | RMOV_MASK);
		}
			else
		{
			if(TPrg==TPrgNormal)
			{
				ATab[CurRow].status|=RDEL_MASK | RMOV_MASK;
			}
			else
			{
				ATab[CurRow].status|=RMOV_MASK;
			}
		}

		TPrg->Write(ATab[CurRow],CurRecord);
		programTableRefresh = true;
		return true;
	}
	return false;
}


// Duplica in basso un record in file e recupera il n. di riga progressivo.
//DANY131202
int Prg_DupRec()
{
	GUI_Freeze_Locker lock;

	int src_rec=CurRecord;     // n�. record selezionato
	int src_rig=CurRow;        // n�. riga selezionata (elem. di array)
	struct TabPrg tabDat;
	
	if(src_rec>=Reccount)
		return 0;      // dup. su riga append vuota

	if(!guiDeskTop->GetEditMode() || TPrg!=TPrgNormal)
	{
		bipbip();
		return 0;
	}
	
	// Se c'e' gia' un componente con codice nullo, non fa il duplica record
	for(int r_loop=0;r_loop<Reccount;r_loop++)
	{
		TPrg->Read(tabDat,r_loop);
		if( tabDat.CodCom[0] == 0 )
		{
			W_Mess(INVALID_DUP);
			return 0;
		}
	}
	
	TPrg->Read(tabDat,src_rec);
	TPrg->DupRec(src_rec);
	tabDat.CodCom[0]='\0';
	
	//UpdateAssemblyFile();
	AddAssemblyRec(tabDat);
	
	Reccount++;
	ReadRecs(src_rec-src_rig);     // lettura dell'array di structs
	programTableRefresh = true;                 // mostra l'array letto
	
	CurRecord=src_rec;         // n�. record selezionato
	CurRow=src_rig;            // n�. riga selezionata (elem. di array)

	return 1;
}

// Sposta un record da una posizione all'altra.
int Prg_MoveRec(void)
{
	GUI_Freeze_Locker lock;

	int source=Prg_FindSel(RMOV_MASK);
	int dest  =CurRecord;
	int d_riga=CurRow;
	struct TabPrg X_Tab[1];

	if(!guiDeskTop->GetEditMode() || TPrg==TPrgDosa)
	{
		bipbip();
		return 0;
	}

	if(source==-1)                 // nessun record selezionato
	{
		W_Mess( MsgGetString(Msg_00187) );
		return 0;
	}
	if(source==dest)               // move sul solito record
	{
		W_Mess( MsgGetString(Msg_00188) );
		return 0;
	}

	if(dest>=Reccount)
		return 0;      // dup. su riga append vuota

	TPrg->Read(X_Tab[0],source);
	TPrg->MoveRec(source,dest);

	ReadRecs(dest-d_riga);     // lettura dell'array di structs
	programTableRefresh = true;

	return 1;
}


// Copia il valore di un campo nel record successivo
int Prg_CopyField( int n_campo )
{
	GUI_Freeze_Locker lock;

	struct TabPrg X_Tab[2];

	int src_rec=CurRecord;
	int src_rig=CurRow;

	if( !guiDeskTop->GetEditMode() || n_campo==ProgramTableUI::COL_LINE || n_campo==ProgramTableUI::COL_CODE || n_campo==ProgramTableUI::COL_FEEDER || TPrg!=TPrgNormal )
	{
		bipbip();
		return 0;
	}

	if(src_rec>=Reccount-1)
	{
		return 0;      // copia da riga append/ultima
	}

	TPrg->Read(X_Tab[0],src_rec);
	TPrg->Read(X_Tab[1],src_rec+1);

	switch(n_campo)
	{
		case ProgramTableUI::COL_COMP:
			strcpy(X_Tab[1].TipCom,X_Tab[0].TipCom);
			X_Tab[1].Changed|=TIPO_FIELD;
			break;
		case ProgramTableUI::COL_ROT:
			X_Tab[1].Rotaz=X_Tab[0].Rotaz;
			X_Tab[1].Changed|=ROT_FIELD;
			break;
		case ProgramTableUI::COL_NOZZLE:
			X_Tab[1].Punta=X_Tab[0].Punta;
			X_Tab[1].Changed|=PUNTA_FIELD;
			break;
		case ProgramTableUI::COL_ASSEM:
			X_Tab[1].status|=(X_Tab[0].status & MOUNT_MASK);
			X_Tab[1].Changed|=MOUNT_FIELD;
			break;
		case ProgramTableUI::COL_X:
			X_Tab[1].XMon=X_Tab[0].XMon;
			X_Tab[1].Changed|=PX_FIELD;
			break;
		case ProgramTableUI::COL_Y:
			X_Tab[1].YMon=X_Tab[0].YMon;
			X_Tab[1].Changed|=PY_FIELD;
			break;
		case ProgramTableUI::COL_NOTES:
			strcpy(X_Tab[1].NoteC,X_Tab[0].NoteC);
			X_Tab[1].Changed|=NOTE_FIELD;
			break;
		case ProgramTableUI::COL_DISP:
			#ifndef __DISP2
			X_Tab[1].status|=(X_Tab[0].status & DODOSA_MASK);
			#else
			X_Tab[1].status|=(X_Tab[0].status & (DODOSA_MASK | DODOSA2_MASK));
			#endif
			X_Tab[1].Changed|=DOSA_FIELD;
			break;
	}

	TPrg->Write(X_Tab[1],src_rec+1,FLUSHON);

	UpdateAssemblyFile();

	ReadRecs(src_rec-src_rig);     // lettura dell'array di structs
	programTableRefresh = true;
	return 1;
}


//GF_14_07_2011
//--------------------------------------------------------------------------
// Modifica tutti i record del programma
// se n_campo = CAMPO_MOUNT -> value puo' valere: Y, N
//    n_campo = CAMPO_DOSA  -> value puo' valere: 1, 2, A, N
//--------------------------------------------------------------------------
void Prg_ChangeAllRows( int n_campo, char value )
{
	bool error = false;
	
	if( TPrg!=TPrgNormal )
		error = true;

	if( n_campo != ProgramTableUI::COL_ASSEM && n_campo != ProgramTableUI::COL_DISP )
		error = true;

	if( n_campo == ProgramTableUI::COL_ASSEM && value != 'Y' && value != 'N' )
		error = true;

	#ifndef __DISP2
	if( n_campo == ProgramTableUI::COL_DISP && value != 'Y' && value != 'N' )
	#else
	if( n_campo == ProgramTableUI::COL_DISP && value != '1' && value != '2' && value != 'A' && value != 'N' )
	#endif
		error = true;

	if( error )
	{
		bipbip();
		return;
	}

	struct TabPrg tabDat;

	for( int i = 0; i < Reccount; i++ )
	{
		TPrg->Read( tabDat, i );
		
		switch( n_campo )
		{
			case ProgramTableUI::COL_ASSEM:
				switch( value )
				{
					case 'Y':
						tabDat.status |= MOUNT_MASK; // mount all
						break;
					case 'N':
						tabDat.status &= ~MOUNT_MASK; // mount none
						break;
				}
				tabDat.Changed |= MOUNT_FIELD;
				break;
			case ProgramTableUI::COL_DISP:
				#ifndef __DISP2
				switch( value )
				{
					case 'Y':
						tabDat.status |= DODOSA_MASK; // dispense all
						break;
					case 'N':
						tabDat.status &= ~DODOSA_MASK; // dispense none
						break;
				}
				#else
				switch( value )
				{
					case '1':
						tabDat.status |= DODOSA_MASK; // dispense all 1
						tabDat.status &= ~DODOSA2_MASK;
						break;
					case '2':
						tabDat.status &= ~DODOSA_MASK; // dispense all 2
						tabDat.status |= DODOSA2_MASK;
						break;
					case 'A':
						tabDat.status |= DODOSA_MASK; // dispense all A
						tabDat.status |= DODOSA2_MASK;
						break;
					case 'N':
						tabDat.status &= ~DODOSA_MASK; // dispense none
						tabDat.status &= ~DODOSA2_MASK;
						break;
				}
				#endif
				tabDat.Changed |= DOSA_FIELD;
				break;
		}
		
		TPrg->Write( tabDat, i );
	}

	UpdateAssemblyFile();

	ReadRecs( 0 );
	programTableRefresh = true;
}


// Ordinamento del database programma
#define SORT_ASKCONFIRM    1
#define SORT_NOASKCONFIRM  0
void Prg_Sort(int sort_field,int mode = SORT_NOASKCONFIRM )
{
  int type, i;
  int nrec=TPrg->Count();

  if(nrec<2)
    return;

  if((mode==SORT_ASKCONFIRM) && (Get_AssemblingFlag() || Get_DosaFlag()))
  {
    if(!W_Deci(0,WARN_ASSEMBLING1))   //ask conferma
      return;
  }

  struct TabPrg *recsetMount=new struct TabPrg[nrec];
  struct TabPrg *recsetNoMount=new struct TabPrg[nrec];
  
  struct TabPrg tmpRec;

  unsigned int dataMount;
  unsigned int baseMount=(mem_pointer)recsetMount;
  unsigned int nMount=0;

  for(i=0;i<nrec;i++)
  {
    TPrg->Read(tmpRec,i);
     recsetMount[nMount++]=tmpRec;
  }

  switch(sort_field)
  {
    case SORT_TIPOCOM:
      dataMount=(mem_pointer)&recsetMount[0].TipCom;
      type=SORTFIELDTYPE_STRING;
    break;

    case SORT_CODCOM:
      dataMount=(mem_pointer)&recsetMount[0].CodCom;
      type=SORTFIELDTYPE_STRING;
    break;
    case SORT_CARIC:
      dataMount=(mem_pointer)&recsetMount[0].Caric;
      type=SORTFIELDTYPE_INT16;
      break;
    case SORT_PUNTA:
      dataMount=(mem_pointer)&recsetMount[0].Punta;
      type=SORTFIELDTYPE_CHAR;
      break;      
    case SORT_XMON:
      dataMount=(mem_pointer)&recsetMount[0].XMon;
      type=SORTFIELDTYPE_FLOAT32;
      break;
    case SORT_PUNTAUGE:
      for(int i=0;i<nrec;i++)
      {
        recsetMount[i].spareSort[0]=recsetMount[i].Punta;
        recsetMount[i].spareSort[1]=recsetMount[i].Uge;
        recsetMount[i].spareSort[2]=0;
      }
      
      dataMount=(mem_pointer)&recsetMount[0].spareSort;
      type=SORTFIELDTYPE_STRING;
      break;
      
	 case SORT_NUMSCHEDA:
      dataMount=(mem_pointer)&recsetMount[0].scheda;
      type=SORTFIELDTYPE_INT16;
      break;

    case SORT_NOTE:
      dataMount=(mem_pointer)&recsetMount[0].NoteC;
      type=SORTFIELDTYPE_STRING;
      break;
  }

  SortData((void *)recsetMount,type,nMount,dataMount-baseMount,sizeof(struct TabPrg));

  for(i=0;i<nMount;i++)
  {
    recsetMount[i].Riga=i+1;
    TPrg->Write(recsetMount[i],i);
  }

  ReadRecs(0);
  CurRow=0;
  CurRecord=0;
  programTableRefresh = true;

  delete[] recsetMount;
  delete[] recsetNoMount;
}

//---------------------------------------------------------------------------

void SwitchToMasterTable()
{
	TPrg=TPrgNormal;
	if(TPrg->GetHandle()==0)
		TPrg->Open(SKIPHEADER);
	Prg_Reload();
	menuIdx=0;

	CurRecord=0;
	CurRow=0;

	Prg_ClearAll();
	ReadRecs(CurRecord-CurRow);
	
	if(Reccount==0)
		Prg_InitAll(0,1);
	
	programTableRefresh = true;
}

void SwitchToAssemblyTable()
{
	TPrg=TPrgExp;
	if(TPrg->GetHandle()==0)
		TPrg->Open(SKIPHEADER);
	Prg_Reload();
	menuIdx=1;

	CurRecord=Get_LastRecMount();
	//se nessun comp. assemblato o assemblaggio terminato
	if(CurRecord==-1 || !Get_AssemblingFlag())
		//tabella ad inizio file
		CurRecord=0;

	CurRow=0;
	Prg_ClearAll();
	ReadRecs(CurRecord-CurRow);
	programTableRefresh = true;
}

//DANY270103
void SwitchToDosaTable(void)
{
	TPrg=TPrgDosa;

	if(!TPrg->IsOnDisk() || updateDosa)
	{
		if(!CreateDosaFile())
			return;
	}

	if(TPrg->GetHandle()==0)
		TPrg->Open(SKIPHEADER);

	Prg_Reload();

	menuIdx=2;

	CurRecord=Get_LastRecDosa();

	//se nessun comp. dosato o dosaggio terminato
	if(CurRecord==-1 || !Get_DosaFlag())
	//tabella ad inizio file
	CurRecord=0;

	CurRow=0;

	Prg_ClearAll();
	ReadRecs(CurRecord-CurRow);

	programTableRefresh = true;
}

// Lancio alla gestione assemblaggio
// rip_ass:  0 (default) assemblaggio da inizio / 1 completa assemblaggio
//           2 check databases only
// Mod. W09
int GestAss(int rip_ass,int stepmode,int nrip=-1,int asknewAss=1,int autoref=1,int enableContAss=1)
{
	char G_NomeFile[MAXNPATH];
	CarPath(G_NomeFile,QHeader.Conf_Default);
	if( access(G_NomeFile,0) != 0 )
	{
		W_Mess(NOCONF);
		return 0;
	}

	if( !CheckDupCompcod() )
	{
		return 0;
	}

	if( !CheckSomethingToMount() )
	{
		return 0;
	}

	Prg_SwitchTable( PRG_ASSEMBLY );

	int retval = check_data( CHECKPRG_FULL );

	if( retval )
	{
		if( (!rip_ass) && Check_OptFile() )
		{
			if( W_Deci(0, MsgGetString(Msg_01434) ) )
			{
				PrgOptimize2* optimize = new PrgOptimize2( QHeader.Prg_Default, QParam.AutoOptimize );

				retval = optimize->InitOptimize();
				if( retval )
				{
					retval = optimize->DoOptimize_NN();
					if( retval )
					{
						if( !optimize->WriteOptimize() )
						{
							W_Mess( "Error on WriteOptimize !" );
						}
					}
				}

				delete optimize;
			}
		}
	}


	if( retval )
	{
		int expcount = TPrgExp->Count();
	
		Prg_CloseFiles();
	
		//SMOD260903
		if( ComponentList == NULL )
		{
			if(expcount!=0)
			{
				ComponentList=new int[expcount];
				for(int i=0;i<expcount;i++)
				{
					ComponentList[i]=0;
				}
				PlacedNComp=0;
				NComponent=expcount;
			}
		}
		else
		{
			if(NComponent<expcount)
			{
				int *tmpptr=new int[expcount];
				memcpy(tmpptr,ComponentList,NComponent*sizeof(int));
				memset(tmpptr+NComponent,(char)0,expcount-NComponent);
				NComponent=expcount;
				delete[] ComponentList;
				ComponentList=tmpptr;
			}
			else
			{
				if(NComponent>expcount)
				{
					int *tmpptr=new int[NComponent];
					memcpy(tmpptr,ComponentList,NComponent*sizeof(int));
					NComponent=expcount;
					delete [] ComponentList;
					ComponentList=tmpptr;
				}
			}
		}

		SetAlarmMode(ALARM_ENABLED);
		retval = InitAss(rip_ass,stepmode,nrip,asknewAss,autoref,enableContAss);
		SetAlarmMode(ALARM_DISABLED);

		Prg_OpenFiles( false );

		TPrg = NULL;
		Prg_SwitchTable(PRG_ASSEMBLY);
	}

	ReadRecs(CurRecord-CurRow);                    // reload dei record
	programTableRefresh = true;

	return retval;
}

// Lancio alla gestione DOSAGGIO ( in q_assem )
#ifndef __DISP2
int GestDos(int rip,int stepmode,int nrip,int interruptable,int asknewDosa,int autoref,int enableContDosa,int disableStartPoints)
#else
int GestDos(int ndisp,int rip,int stepmode,int nrip,int interruptable,int asknewDosa,int autoref,int enableContDosa,int disableStartPoints)
#endif
{
	#ifndef __DISP2
	int ndisp = 1;
	#endif
	
	if(!Dosatore->IsConfLoaded(ndisp))
	{
		W_Mess(NODOSACONF_LOADED);
		return 0;
	}
	
	TPrgFile *tmp=TPrg;
	
	int ret=0;
	
	TPrg=TPrgExp;
	int retval=check_data(CHECKPRG_DISPFULL);
	
	if(retval)
	{
		if(!CreateDosaFile())
		{
			TPrg=tmp;
			return 0;
		}
	
		Prg_SwitchTable(PRG_DOSAT);
	
		if(TPrg!=TPrgDosa)
		{
			TPrg=tmp;
			return 0;
		}
	
		//--- Distribuition start ---
	
		TPrgDosa->Close();
	
		SetAlarmMode(ALARM_ENABLED);
		#ifndef __DISP2
		ret=Dosaggio(rip,stepmode,nrip,interruptable,asknewDosa,autoref,enableContDosa,disableStartPoints);
		#else
		ret=Dosaggio(ndisp,rip,stepmode,nrip,interruptable,asknewDosa,autoref,enableContDosa);
		#endif
		SetAlarmMode(ALARM_DISABLED);
	
		TPrgDosa->Open();
		
		//---  Distribuition end  ---
	
		TPrg=NULL;
		Prg_SwitchTable(PRG_DOSAT);
	
	}
	
	if(TPrg!=TPrgDosa)
	{
		TPrg=tmp;
	}
	
	return(ret);
} // GestDos



// rip      =ripartenza Si(1)/No(0)    (default=0)
// stepmode =1: modalita' passo passo, 0: modalita' normale (default)
#ifndef __DISP2
int GestDosAss(int rip,int stepmode,int asknewDosa,int disableStartPoints)
#else
int GestDosAss(int ndisp,int rip,int stepmode,int asknewDosa,int disableStartPoints)
#endif
{
	unsigned int assrip_component=0,dosrip_component=0,fine=0;

	#ifndef __DISP2
	int ndisp = 1;
	#endif	
	
	if(!Dosatore->IsConfLoaded(ndisp))
	{
		W_Mess(NODOSACONF_LOADED);
		return 0;
	}

	do
	{
		if(rip)
		{
			if(!dosaAss)
			{
				W_Mess(NO_DASRESTART);
				dosrip_component=0;
				assrip_component=0;
		
				if(!Set_RipData(dosrip_component,TPrgDosa->Count(),RIP_DOSAASS_TITLE))
				return 0;
		
				dosaAss=1;
		
				#ifndef __DISP2
				if(!GestDos(rip,stepmode,dosrip_component,0,0,0,0,disableStartPoints))
				#else
				if(!GestDos(ndisp,rip,stepmode,dosrip_component,0,0,0,0))
				#endif
				{
					return 0;
				}
				
				if(!GestAss(0,stepmode,0,0,0,0))
				{
					return 0;
				}
			
			}
			else
			{
				if(Get_DosaFlag())
				{
					dosrip_component=Get_LastRecDosa()+1;
					if(!Set_RipData(dosrip_component,TPrgDosa->Count(),RIP_DOSAASS_TITLE))
					{
						return 0;
					}
		
					dosaAss=1;
			
					#ifndef __DISP2
					if(!GestDos(rip,stepmode,dosrip_component,0,0,0,0,disableStartPoints))
					#else
					if(!GestDos(ndisp,rip,stepmode,dosrip_component,0,0,0,0))
					#endif
					{
						return 0;
					}
					if(!GestAss(0,stepmode,0,0,0,0))
					{
						return 0;
					}				
				}
				else
				{
					if(Get_AssemblingFlag())
					{
						assrip_component=Get_LastRecMount(1)+1; //SMOD110403
			
						int prev_assrip=assrip_component;
			
						if(!Set_RipData(assrip_component,TPrgExp->Count(),RIP_DOSAASS_TITLE))
						{
							return 0;
						}
			
						if(assrip_component!=prev_assrip)
						{
							nomountP2=0; //SMOD251104
							lastmountP1=-1;
							lastmountP2=-1;
						}              
					
					}

				dosaAss=1;
		
				if(!GestAss(rip,stepmode,assrip_component,0,0,0))
				{
					return 0;
				}
        	}
      	}
    }
    else
    {
		dosaAss=1;
	
		#ifndef __DISP2
		if(!GestDos(rip,stepmode,dosrip_component,0,0,1,0,disableStartPoints))
		#else
		if(!GestDos(ndisp,rip,stepmode,dosrip_component,0,0,1,0))
		#endif
		{
			return 0;
		}
	
		if(!GestAss(rip,stepmode,assrip_component,0,0,0))
		{
			return 0;
		}
    }

    if(!Get_DosaFlag() && !Get_AssemblingFlag())
    {
		dosaAss=0;
		if(!QParam.AS_cont)
		{
			if(asknewDosa)
			{
				wait_to_cont_ass_internal_state = protection_state_wait_transition;
				int ret = W_Deci(1,DAS_END,MSGBOX_YCENT,0,0,GENERIC_ALARM);
				/*
				int ret = W_Deci(1,DAS_END,MSGBOX_YCENT,wait_to_cont_ass_protection_check_polling_loop,0,GENERIC_ALARM);
				if(ret)
				{
					if(wait_to_cont_ass_internal_state == protection_state_wait_transition)
					{
						ret = CheckProtectionIsWorking();
					}
				}
				*/

				if(!ret)
				{
					fine = 1;
				}
			}
			else
				fine = 1;
		}
    }

    rip=0;
    dosrip_component=0;
    assrip_component=0;
  } while(!fine);
  
  return 1;
} // GestDosAss

#ifdef __DISP2
int GestDos12Ass(tDosAssIdx seq1,tDosAssIdx seq2,tDosAssIdx seq3,int rip,int stepmode,int asknewDosa,int disableStartPoints)
{
	if((!Dosatore->IsConfLoaded(1)) || (!Dosatore->IsConfLoaded(2)))
	{
		W_Mess(NODOSACONF_LOADED);
		return 0;
	}

	unsigned int ass_start_component=0;
	unsigned int dos_start_component=0;
	int fine=0;

  	//...per sicurezza anche se e' un caso che non dovrebbe mai accadere
	if(Dosa12Ass_phase>=3)
	{
		Dosa12Ass_phase=0;
	}


	if(!rip)
	{
		Dosa12Ass_phase=0;
		dos_start_component=0;
		ass_start_component=0;

		Dosa12Ass_seqvector[0]=seq1;
		Dosa12Ass_seqvector[1]=seq2;
		Dosa12Ass_seqvector[2]=seq3;
    
	}
	else
	{
    	//ripartenza richiesta

		if(((!dosa12Ass) || (Dosa12Ass_phase==2)) || (Dosa12Ass_seqvector[Dosa12Ass_phase+1]==DOSASS_IDX_NONE))
		{
			//il ciclo di dosaggio e assemblaggio non risulta in corso oppure tutte le fasi
			//sono state comunque completate

      		//notifica dosaggio+assemblaggio non interrotto
			W_Mess(NO_DASRESTART);
			dos_start_component=0;
			ass_start_component=0;

			if(!Set_RipData(dos_start_component,TPrgDosa->Count(),RIP_DOSAASS_TITLE))
			{
				return 0;
			}

			Dosa12Ass_phase=0;
			Dosa12Ass_seqvector[0]=seq1;
			Dosa12Ass_seqvector[1]=seq2;
			Dosa12Ass_seqvector[2]=seq3;

		}
		else
		{
			int restart_phase;

			if(!Get_DosaFlag() && !Get_AssemblingFlag())
			{
				//il ciclo di dosaggio e assemblaggio risulta in corso ma dosaggio e
				//assemblaggio risultano non in esecuizione: la fase precedente e'
				//stata completata ma il ciclo di dosaggio e assemblaggio non e' ancora
				//concluso: si deve passare alla prossima fase

				restart_phase=Dosa12Ass_phase+1;
				dos_start_component=0;
				ass_start_component=0;
			}
			else
			{
        		//la fase precedente era stata interrotta
				restart_phase=Dosa12Ass_phase;
				dos_start_component=Get_LastRecDosa()+1;
				ass_start_component=Get_LastRecMount(1)+1;
			}

			switch(Dosa12Ass_seqvector[restart_phase])
			{
				case DOSASS_IDX_DOSA1:
				case DOSASS_IDX_DOSA2:

					if(!Set_RipData(dos_start_component,TPrgDosa->Count(),RIP_DOSAASS_TITLE))
					{
						return 0;
					}

					break;
          
				case DOSASS_IDX_ASSEM:
					ass_start_component=0;

					int prev=ass_start_component;

					if(!Set_RipData(ass_start_component,TPrgExp->Count(),RIP_DOSAASS_TITLE))
					{
						return 0;
					}

					if(ass_start_component!=prev)
					{
						nomountP2=0; //SMOD251104
						lastmountP1=-1;
						lastmountP2=-1;
					}
          
					break;
			}

			Dosa12Ass_phase=restart_phase;
		}
	}

	do
	{
		dosa12Ass=1;
		
		int search_refs = 1;

		while(Dosa12Ass_phase!=3)
		{
			switch(Dosa12Ass_seqvector[Dosa12Ass_phase])
			{
				case DOSASS_IDX_DOSA1:
					if(!GestDos(1,rip,stepmode,dos_start_component,0,0,search_refs,0))
					{
						return 0;
					}
					rip=0;
					search_refs=0;
					break;
				case DOSASS_IDX_ASSEM:
					if(!GestAss(rip,stepmode,ass_start_component,0,search_refs,0))
					{
						return 0;
					}
					rip=0;
					search_refs=0;
					break;
				case DOSASS_IDX_DOSA2:
					if(!GestDos(2,rip,stepmode,dos_start_component,0,0,search_refs,0))
					{
						return 0;
					}
					rip=0;
					search_refs=0;					
					break;
			}

			Dosa12Ass_phase++;
		}

		dosa12Ass=0;
		Dosa12Ass_phase=0;

		if(!QParam.AS_cont)
		{
			wait_to_cont_ass_internal_state = protection_state_wait_transition;
			int ret = W_Deci(1,DAS_END,MSGBOX_YCENT,0,0,GENERIC_ALARM);
			/*
			int ret = W_Deci(1,DAS_END,MSGBOX_YCENT,wait_to_cont_ass_protection_check_polling_loop,0,GENERIC_ALARM);
			if(ret)
			{
				if(wait_to_cont_ass_internal_state == protection_state_wait_transition)
				{
					ret = CheckProtectionIsWorking();		
				}				
			}
			*/
							
			if(!ret)
			{
				fine = 1;
			}						
		}

		rip=0;
		dos_start_component=0;
		ass_start_component=0;
    
	} while(!fine);

	return 1;
}
#endif


//-----------------------------------------------------------------------
// Ricerca codici caricatori del programma
//  mode: 0 -> inizio da riga corrente programma
//        1 -> continua
// Ritorna: 0 -> ricerca finita
//          1 -> componente non montato
//          codice caricatore altrimenti
//-----------------------------------------------------------------------
int Search_car( int mode )
{
	struct TabPrg PTab;
	static int Nsequenza;

	if( mode == 0 )
	{
		Nsequenza = CurRecord;
	}

	if( Nsequenza < Reccount )
	{
		TPrg->Read( PTab, Nsequenza );
		Nsequenza++;

		if( (PTab.status & MOUNT_MASK) != 0 )
		{
			return PTab.Caric; // codice caricatore
		}

		return 1; //componente non montato
	}

	return 0; //ricerca finita
}


//GF_TEMP -> //TODO: sostituisce SetImageName
void GetImageName( char* filename, int type, int num, char* libname )
{
	switch( type )
	{
		case ZEROIMG:        //immagine zero scheda originale
		case RIFEIMG:        // punto di riferimento originale
			strcpy(filename,CLIDIR);
			strcat(filename,"/");
			strcat(filename,QHeader.Cli_Default);
			strcat(filename,"/");
			strcat(filename,VISIONDIR);
			strcat(filename,QHeader.Prg_Default);
			break;
		
		case ZEROMACHIMG: // zero macchina originale
			strcpy( filename, VISIONDIR );
			strcat( filename, ZEROMACHFILE );
			break;
		
		case EXTCAM_NOZ_IMG: // punta da telecamera esterna
			strcpy( filename, VISIONDIR );
			strcat( filename, EXCAMNOZ_FILE );
			break;
		
		case HEADCAM_SCALE_IMG: // settaggio scala telecamera testa
			strcpy( filename, VISIONDIR );
			strcat( filename, SETSCALEFILE );
			break;
		
		case MAPPING_IMG:
			strcpy( filename, VISIONDIR );
			strcat( filename, MAPPING_FILE );
			break;

		//CCCP
		case CARCOMP_IMG:    // componente su caricatore
			strcpy( filename, VISIONDIR );
			strcat( filename, CARCOMP_FILE );
			sprintf( filename, "%s%03d", filename, num );
			break;

		case RECTCOMP_IMG:
			strcpy( filename, VISIONDIR );
			strcat( filename, RECTCOMP_FILE );
			break;

		case INKMARK_IMG:
			strcpy( filename, VISIONDIR );
			strcat( filename, INKMARK_FILE );
			break;

		case PACKAGEVISION_LEFT_1:
		case PACKAGEVISION_RIGHT_1:
			if( libname == 0 )
			{
				filename[0] = 0;
				break;
			}

			strcpy( filename, VISPACKDIR );
			strcat( filename, libname );
			DelSpcR( filename );
			sprintf( filename, "%s/%d_n1", filename, num );
			break;
		
		case PACKAGEVISION_LEFT_2:
		case PACKAGEVISION_RIGHT_2:
			if( libname == 0 )
			{
				filename[0] = 0;
				break;
			}

			strcpy( filename, VISPACKDIR );
			strcat( filename, libname );
			DelSpcR( filename );
			sprintf( filename, "%s/%d_n2", filename, num );
			break;

		case EXTCAM_SCALE_IMG:
			strcpy( filename, VISIONDIR );
			strcat( filename, SETSCALEAUXFILE );
			break;
		
		case UGEIMG:
			strcpy( filename, VISIONDIR );
			strcat( filename, UGEREFFILE );
			break;
		
		default:
			strcpy( filename, "image" );
			break;
	}
}

//GF_TEMP -> //TODO: sostituisce vecchio SetImageName
void AppendImageMode( char* filename, int type, int mode )
{
	switch( type )
	{
		case ZEROIMG:
			strcat( filename, ".z" );
			break;
		
		case RIFEIMG:
			strcat( filename, ".r" );
			break;

		case PACKAGEVISION_LEFT_1:
		case PACKAGEVISION_RIGHT_1:
		case PACKAGEVISION_LEFT_2:
		case PACKAGEVISION_RIGHT_2:
			strcat( filename, ".p" );
			break;
		
		default:
			strcat( filename, ".i" );
			break;
	}

	switch( mode )
	{
		case IMAGE:   //immagine originale
			if( type == PACKAGEVISION_LEFT_1 || type == PACKAGEVISION_LEFT_2 )
			{
				strcat( filename, "os" );
				break;
			}
			if( type == PACKAGEVISION_RIGHT_1 || type == PACKAGEVISION_RIGHT_2 )
			{
				strcat( filename, "od" );
				break;
			}
			
			strcat( filename, "or" );
			break;
		
		case DATA:    //dati immagine
			if( type == PACKAGEVISION_LEFT_1 || type == PACKAGEVISION_LEFT_2 )
			{
				strcat( filename, "ds" );
				break;
			}
			if( type == PACKAGEVISION_RIGHT_1 || type == PACKAGEVISION_RIGHT_2 )
			{
				strcat( filename, "dd" );
				break;
			}
			
			strcat( filename, "dt" );
			break;
		
		case ELAB:    //immagine elaborata
			if( type == PACKAGEVISION_LEFT_1 || type == PACKAGEVISION_LEFT_2 )
			{
				strcat( filename, "es" );
				break;
			}
			if( type == PACKAGEVISION_RIGHT_1 || type == PACKAGEVISION_RIGHT_2 )
			{
				strcat( filename, "ed" );
				break;
			}
			
			strcat( filename, "el" );
			break;
		
		case BOARD:   //immagine con n. board
			sprintf( filename, "%s%02d", filename, int(nboard % 100) );
			break;
		
		default:
			strcat( filename, "mg" );
			break;
	}
}

// Fornisce il nome del file per salvare dati immagine
void SetImageName( char* filename, int type, int mode, int num, char* libname )
{
	GetImageName( filename, type, num, libname );
	AppendImageMode( filename, type, mode );
}


//---------------------------------------------------------------------------
//* Funzioni di settaggio altezza PCB x programma di assemblaggio

struct dta_data* DtaVal;

int GDtaM_ClearNb()
{
	DtaVal->n_board = 0;
	DtaVal->n_comp = 0;
	return 1;
}


//---------------------------------------------------------------------------
// finestra: Program data
//---------------------------------------------------------------------------
class ProgramDataUI : public CWindowParams
{
public:
	ProgramDataUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 4 );
		#ifndef __DISP2
		SetClientAreaSize( 52, 12 );
		#else
		SetClientAreaSize( 62, 12 );
		#endif
		SetTitle( MsgGetString(Msg_00124) );

		#ifdef __DISP2
		SM_DispPoints = new GUI_SubMenu();
		SM_DispPoints->Add( MsgGetString(Msg_05113), K_F1, 0, NULL, boost::bind( &ProgramDataUI::onPColla1, this ) ); // dispenser 1
		SM_DispPoints->Add( MsgGetString(Msg_05114), K_F2, 0, NULL, boost::bind( &ProgramDataUI::onPColla2, this ) ); // dispenser 2
		#endif

		DtaVal = new dta_data;
		Read_Dta( DtaVal );
	}

	~ProgramDataUI()
	{
		#ifdef __DISP2
		delete SM_DispPoints;
		#endif
		delete DtaVal;
	}

	typedef enum
	{
		PCB_H,
		DISP_X1,
		DISP_Y1,
		#ifdef __DISP2
		DISP_X2,
		DISP_Y2,
		#endif
		MAX_COMP_H,
		NUM_COMPS,
		NUM_BOARDS
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[PCB_H]      = new C_Combo(  4, 1, MsgGetString(Msg_00989), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		#ifndef __DISP2
		m_combos[DISP_X1]    = new C_Combo(  4, 3, MsgGetString(Msg_00990), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[DISP_Y1]    = new C_Combo(  4, 4, MsgGetString(Msg_00578), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		#else
		m_combos[DISP_X1]    = new C_Combo(  4, 3, MsgGetString(Msg_05115), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[DISP_Y1]    = new C_Combo(  4, 4, MsgGetString(Msg_05116), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[DISP_X2]    = new C_Combo( 50, 3, "", 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[DISP_Y2]    = new C_Combo( 50, 4, "", 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		#endif
		m_combos[MAX_COMP_H] = new C_Combo(  4, 6, MsgGetString(Msg_01433), 7, CELL_TYPE_SDEC, CELL_STYLE_NOSEL, 2 );
		m_combos[NUM_COMPS]  = new C_Combo(  4, 7, MsgGetString(Msg_00991), 7, CELL_TYPE_UINT, CELL_STYLE_NOSEL );
		m_combos[NUM_BOARDS] = new C_Combo(  4, 8, MsgGetString(Msg_00742), 7, CELL_TYPE_UINT, CELL_STYLE_NOSEL );

		// set params
		m_combos[PCB_H]->SetVMinMax( PCB_H_MIN, PCB_H_MAX );
		m_combos[DISP_X1]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[DISP_Y1]->SetVMinMax( QParam.LY_mincl, QParam.LY_maxcl );
		#ifdef __DISP2
		m_combos[DISP_X2]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[DISP_Y2]->SetVMinMax( QParam.LY_mincl, QParam.LY_maxcl );
		#endif

		// add to combo list
		m_comboList->Add( m_combos[PCB_H]     , 0, 0 );
		m_comboList->Add( m_combos[DISP_X1]   , 1, 0 );
		m_comboList->Add( m_combos[DISP_Y1]   , 2, 0 );
		#ifdef __DISP2
		m_comboList->Add( m_combos[DISP_X2]   , 1, 1 );
		m_comboList->Add( m_combos[DISP_Y2]   , 2, 1 );
		#endif
		m_comboList->Add( m_combos[MAX_COMP_H], 4, 0 );
		m_comboList->Add( m_combos[NUM_COMPS] , 5, 0 );
		m_comboList->Add( m_combos[NUM_BOARDS], 6, 0 );
	}

	void onRefresh()
	{
		m_combos[PCB_H]->SetTxt( DtaVal->PCBh );
		m_combos[DISP_X1]->SetTxt( DtaVal->dosa_x );
		m_combos[DISP_Y1]->SetTxt( DtaVal->dosa_y );
		#ifdef __DISP2
		m_combos[DISP_X2]->SetTxt( DtaVal->dosa_x2 );
		m_combos[DISP_Y2]->SetTxt( DtaVal->dosa_y2 );
		#endif
		m_combos[NUM_COMPS]->SetTxt( int(DtaVal->n_comp) );
		m_combos[NUM_BOARDS]->SetTxt( int(DtaVal->n_board) );

		#ifdef __SNIPER
		float sniper_body = SNIPER_BODY_DELTA;
		if( QHeader.Z12_Zero_delta < 0 )
		{
			//sniper 1 e' piu in alto dello sniper 2
			sniper_body -= QHeader.Z12_Zero_delta;
		}

		m_combos[MAX_COMP_H]->SetTxt( (float)(QHeader.Zero_Piano-DtaVal->PCBh-sniper_body-SNIPER_MAXCOMP_DELTA) );
		#endif
	}

	void onEdit()
	{
		DtaVal->PCBh = m_combos[PCB_H]->GetFloat();
		if( fabs(DtaVal->PCBh) < PCB_H_ABS_MIN )
		{
			DtaVal->PCBh = 0.f;
		}

		DtaVal->dosa_x = m_combos[DISP_X1]->GetFloat();
		DtaVal->dosa_y = m_combos[DISP_Y1]->GetFloat();

		#ifdef __DISP2
		DtaVal->dosa_x2 = m_combos[DISP_X2]->GetFloat();
		DtaVal->dosa_y2 = m_combos[DISP_Y2]->GetFloat();
		#endif
	}

	void onShowMenu()
	{
		#ifndef __DISP2
		m_menu->Add( MsgGetString(Msg_01324), K_F3, 0, NULL, boost::bind( &ProgramDataUI::onPColla1, this ) );   // Initial dispensing points
		#else
		m_menu->Add( MsgGetString(Msg_01324), K_F3, 0, SM_DispPoints, NULL );
		#endif

		m_menu->Add( MsgGetString(Msg_00728), K_F12, 0, NULL, GDtaM_ClearNb ); // Reset counters
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				#ifndef __DISP2
				onPColla1();
				#else
				SM_DispPoints->Show();
				#endif
				return true;

			case K_F12:
				GDtaM_ClearNb();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Save_Dta( *DtaVal );
	}

private:
	int onPColla1()
	{
		Save_Dta( *DtaVal );
		fn_DispenserPointsPosition( this, 1, PCOLLA_INITIALPOINT );
		Read_Dta( DtaVal );
		return 1;
	}

	int onPColla2()
	{
		Save_Dta( *DtaVal );
		fn_DispenserPointsPosition( this, 2, PCOLLA_INITIALPOINT );
		Read_Dta( DtaVal );
		return 1;
	}

	#ifdef __DISP2
	GUI_SubMenu* SM_DispPoints;    // sub menu punti dosaggio
	#endif
};

int fn_ProgramData( CWindow* parent )
{
	ProgramDataUI win( parent );
	win.Show();
	win.Hide();

	return 1;
}

//---------------------------------------------------------------------------
// finestra: Conveyor
//---------------------------------------------------------------------------
extern std::string fn_Select( CWindow* parent, const std::string& title, const std::vector<std::string>& items );

struct conv_data* convVal;

class ConveyorAdvancedUI : public CWindowParams
{
public:
	ConveyorAdvancedUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 6 );
		SetClientAreaSize( 46, 8 );
		SetTitle( MsgGetString(Msg_00915) );
	}

	~ConveyorAdvancedUI()
	{
	}

	typedef enum
	{
		SPEED,
		ACCDEC,
		STEPS,
		ZERO,
		LIMIT,
		MINCURR,
		MAXCURR,
		MINPOS,
		MAXPOS
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[SPEED]  	= new C_Combo(  3, 1, MsgGetString(Msg_00908), 7, CELL_TYPE_UDEC );
		m_combos[ACCDEC]  	= new C_Combo(  3, 2, MsgGetString(Msg_00909), 7, CELL_TYPE_UDEC );
		m_combos[STEPS]  	= new C_Combo(  3, 3, MsgGetString(Msg_00910), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 4 );
		m_combos[ZERO]  	= new C_Combo(  3, 4, MsgGetString(Msg_00911), 7, CELL_TYPE_UDEC );
		m_combos[LIMIT]  	= new C_Combo(  30, 4, "", 7, CELL_TYPE_UDEC );
		m_combos[MINCURR]  	= new C_Combo(  3, 5, MsgGetString(Msg_00912), 7, CELL_TYPE_UDEC );
		m_combos[MAXCURR]  	= new C_Combo(  30, 5, "", 7, CELL_TYPE_UDEC );
		m_combos[MINPOS]    = new C_Combo(  3, 6, MsgGetString(Msg_00914), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[MAXPOS]    = new C_Combo(  30, 6, "", 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

		// set params
		m_combos[SPEED]  	->SetVMinMax( 100, 3000 );
		m_combos[ACCDEC]  	->SetVMinMax( 2000, 20000 );
		m_combos[STEPS]  	->SetVMinMax( (float)1.0, (float)100.0 );
		m_combos[ZERO]  	->SetVMinMax( 1, 3 );
		m_combos[LIMIT]  	->SetVMinMax( 1, 3 );
		m_combos[MINCURR]  	->SetVMinMax( 50, 500 );
		m_combos[MAXCURR]  	->SetVMinMax( 50, 4000 );
		m_combos[MINPOS]    ->SetVMinMax( (float)-50.0, (float)50.0 );
		m_combos[MAXPOS]    ->SetVMinMax( (float)0.0, (float)1200.0 );

		// add to combo list
		m_comboList->Add( m_combos[SPEED]  	, 0, 0 );
		m_comboList->Add( m_combos[ACCDEC]  , 1, 0 );
		m_comboList->Add( m_combos[STEPS]  	, 2, 0 );
		m_comboList->Add( m_combos[ZERO]  	, 3, 0 );
		m_comboList->Add( m_combos[LIMIT]  	, 3, 1 );
		m_comboList->Add( m_combos[MINCURR] , 4, 0 );
		m_comboList->Add( m_combos[MAXCURR] , 4, 1 );
		m_comboList->Add( m_combos[MINPOS]  , 5, 0 );
		m_comboList->Add( m_combos[MAXPOS]  , 5, 1 );
	}

	void onRefresh()
	{
		m_combos[SPEED]->SetTxt( (int)convVal->speed );
		m_combos[ACCDEC]->SetTxt( (int)convVal->accDec );
		m_combos[STEPS]->SetTxt( float(convVal->stepsMm) );
		m_combos[ZERO]->SetTxt( (int)convVal->zero );
		m_combos[LIMIT]->SetTxt( (int)convVal->limit );
		m_combos[MINCURR]->SetTxt( (int)convVal->minCurr );
		m_combos[MAXCURR]->SetTxt( (int)convVal->maxCurr );
		m_combos[MINPOS]->SetTxt( float(convVal->minPos) );
		m_combos[MAXPOS]->SetTxt( float(convVal->maxPos) );
	}

	void onEdit()
	{
		convVal->speed   = m_combos[SPEED]->GetInt();
		convVal->accDec  = m_combos[ACCDEC]->GetInt();
		convVal->stepsMm = m_combos[STEPS]->GetFloat();
		convVal->zero    = m_combos[ZERO]->GetInt();
		convVal->limit   = m_combos[LIMIT]->GetInt();
		convVal->minCurr = m_combos[MINCURR]->GetInt();
		convVal->maxCurr = m_combos[MAXCURR]->GetInt();
		convVal->minPos  = m_combos[MINPOS]->GetFloat();
		convVal->maxPos  = m_combos[MAXPOS]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00977), K_F3, 0, NULL, boost::bind( &ConveyorAdvancedUI::onStepsMmTeach, this ) );
		m_menu->Add( MsgGetString(Msg_00988), K_F12, 0, NULL, boost::bind( &ConveyorAdvancedUI::onReset, this ) );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				onStepsMmTeach();
				return true;

			case K_F12:
				onReset();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		if( Get_UseConveyor() )
			UpdateConveyorSpeedAcc( convVal->speed, convVal->accDec );
	}

private:
	int onStepsMmTeach()
	{
		if( !Get_UseConveyor() )
		{
			W_Mess( MsgGetString(Msg_00928) );
			return 0;
		}

		float x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;
		int c1 = 0, c2 = 0;

		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00916) );
		MoveConveyorPosition( CONV_HOME );
		delete pan;

		// Messaggio apprendimento punto riferimento prima della movimentazione convogliatore
		W_Mess( MsgGetString(Msg_00978) );

		Set_Tv(2); // predispone per non richiudere immagine su video

		if( !ManualTeaching( &x1, &y1, MsgGetString(Msg_00977), AUTOAPP_CONVEYOR ) )
		{
			Set_Tv(3); // chiude immagine su video
			return 0;
		}

		Set_Tv(3); // chiude immagine su video

		x2 = x1;
		y2 = y1;

		// Posizione attuale del convogliatore
		c1 = GetConveyorActualSteps();

		// Messaggio apprendimento punto riferimento prima della movimentazione convogliatore
		W_Mess( MsgGetString(Msg_00984) );

		Set_Tv(2); // predispone per non richiudere immagine su video

		if( !ManualTeaching( &x2, &y2, MsgGetString(Msg_00977), AUTOAPP_CONVEYOR ) )    // autoappr. posizione zero
		{
			Set_Tv(3); // chiude immagine su video
			return 0;
		}

		Set_Tv(3); // chiude immagine su video

		// Posizione attuale del convogliatore
		c2 = GetConveyorActualSteps();

		// Calcola e salva nuova costante passi/mm
		convVal->stepsMm = (float)(abs(c2 - c1)) / fabs(x2 - x1);
		Mod_Conv( *convVal );

		onRefresh();

		return 1;
	}

	int onReset()
	{
		if( !Get_UseConveyor() )
		{
			W_Mess( MsgGetString(Msg_00928) );
			return 0;
		}

		CPan* wait = new CPan( -1, 1, MsgGetString(Msg_00916) );

		if( !InitConveyor() )
		{
			W_Mess( MsgGetString(Msg_00920) );
		}
		else
		{
			// Ricerca zero convogliatore
			if( !SearchConveyorZero() )
			{
				W_Mess( MsgGetString(Msg_00917) );
			}

			// Abilitazione limiti
			if( !ConveyorLimitsEnable( true ) )
			{
				W_Mess( MsgGetString(Msg_00917) );
			}
		}

		delete wait;

		return 1;
	}

};

class ConveyorUI : public CWindowParams
{
public:
	ConveyorUI( CWindow* parent ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 4 );
		SetClientAreaSize( 56, 17 );
		SetTitle( MsgGetString(Msg_00899) );

		convVal = new conv_data;
		Read_Conv( convVal );

		SM_Step1 = new GUI_SubMenu(); // sub menu step1
		SM_Step1->Add( MsgGetString(Msg_00903), K_F3, 0, NULL, boost::bind( &ConveyorUI::onLoadStep1, this ) );
		SM_Step1->Add( MsgGetString(Msg_00907), K_F4, 0, NULL, boost::bind( &ConveyorUI::onTeachStep1, this ) );
		SM_Step1->Add( MsgGetString(Msg_00905), K_F5, 0, NULL, boost::bind( &ConveyorUI::onDisableStep1, this ) );

		SM_Step2 = new GUI_SubMenu(); // sub menu step2
		SM_Step2->Add( MsgGetString(Msg_00903), K_F3, 0, NULL, boost::bind( &ConveyorUI::onLoadStep2, this ) );
		SM_Step2->Add( MsgGetString(Msg_00907), K_F4, 0, NULL, boost::bind( &ConveyorUI::onTeachStep2, this ) );
		SM_Step2->Add( MsgGetString(Msg_00905), K_F5, 0, NULL, boost::bind( &ConveyorUI::onDisableStep2, this ) );

		SM_Step3 = new GUI_SubMenu(); // sub menu step3
		SM_Step3->Add( MsgGetString(Msg_00903), K_F3, 0, NULL, boost::bind( &ConveyorUI::onLoadStep3, this ) );
		SM_Step3->Add( MsgGetString(Msg_00907), K_F4, 0, NULL, boost::bind( &ConveyorUI::onTeachStep3, this ) );
		SM_Step3->Add( MsgGetString(Msg_00905), K_F5, 0, NULL, boost::bind( &ConveyorUI::onDisableStep3, this ) );
	}

	~ConveyorUI()
	{
		delete convVal;
		delete SM_Step1;
		delete SM_Step2;
		delete SM_Step3;
	}

	typedef enum
	{
		ENABLE,
		STEP1_CUST,
		STEP1_PROG,
		STEP1_FEEDER,
		STEP1_PACKAGE,
		STEP1_MOVE,
		STEP2_CUST,
		STEP2_PROG,
		STEP2_FEEDER,
		STEP2_PACKAGE,
		STEP2_MOVE,
		STEP3_CUST,
		STEP3_PROG,
		STEP3_FEEDER,
		STEP3_PACKAGE,
		STEP3_MOVE
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[ENABLE]  		= new C_Combo(  6, 1, MsgGetString(Msg_00891), 4, CELL_TYPE_YN );
		m_combos[STEP1_CUST]    = new C_Combo(  3, 4, MsgGetString(Msg_00886), 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP1_PROG]    = new C_Combo(  3, 5, MsgGetString(Msg_00895), 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP1_FEEDER]  = new C_Combo(  3, 6, MsgGetString(Msg_00896), 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP1_PACKAGE] = new C_Combo(  3, 7, MsgGetString(Msg_00897), 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP1_MOVE]    = new C_Combo(  3, 8, MsgGetString(Msg_00898), 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 2 );
		m_combos[STEP2_CUST]    = new C_Combo(  31, 4, "", 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP2_PROG]    = new C_Combo(  31, 5, "", 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP2_FEEDER]  = new C_Combo(  31, 6, "", 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP2_PACKAGE] = new C_Combo(  31, 7, "", 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP2_MOVE]    = new C_Combo(  31, 8, "", 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 2 );
		m_combos[STEP3_CUST]    = new C_Combo(  42, 4, "", 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP3_PROG]    = new C_Combo(  42, 5, "", 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP3_FEEDER]  = new C_Combo(  42, 6, "", 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP3_PACKAGE] = new C_Combo(  42, 7, "", 9, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		m_combos[STEP3_MOVE]    = new C_Combo(  42, 8, "", 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY | CELL_STYLE_NOSEL, 2 );

		// set params
		m_combos[STEP1_MOVE]->SetVMinMax( convVal->minPos, convVal->maxPos );
		m_combos[STEP2_MOVE]->SetVMinMax( convVal->minPos, convVal->maxPos );
		m_combos[STEP3_MOVE]->SetVMinMax( convVal->minPos, convVal->maxPos );

		m_combos[STEP1_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP1_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP1_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP1_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP1_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );

		if( convVal->enabled )
		{
			if( convVal->step1enabled )
			{
				m_combos[STEP1_CUST]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP1_PROG]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP1_FEEDER]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP1_PACKAGE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP1_MOVE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			}

			if( convVal->step2enabled )
			{
				m_combos[STEP2_CUST]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP2_PROG]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP2_FEEDER]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP2_PACKAGE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP2_MOVE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			}

			if( convVal->step3enabled )
			{
				m_combos[STEP3_CUST]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP3_PROG]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP3_FEEDER]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP3_PACKAGE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP3_MOVE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			}
		}

		// add to combo list
		m_comboList->Add( m_combos[ENABLE]       , 0, 0 );
		m_comboList->Add( m_combos[STEP1_CUST]   , 1, 0 );
		m_comboList->Add( m_combos[STEP1_PROG]   , 2, 0 );
		m_comboList->Add( m_combos[STEP1_FEEDER] , 3, 0 );
		m_comboList->Add( m_combos[STEP1_PACKAGE], 4, 0 );
		m_comboList->Add( m_combos[STEP1_MOVE]   , 5, 0 );
		m_comboList->Add( m_combos[STEP2_CUST]   , 1, 1 );
		m_comboList->Add( m_combos[STEP2_PROG]   , 2, 1 );
		m_comboList->Add( m_combos[STEP2_FEEDER] , 3, 1 );
		m_comboList->Add( m_combos[STEP2_PACKAGE], 4, 1 );
		m_comboList->Add( m_combos[STEP2_MOVE]   , 5, 1 );
		m_comboList->Add( m_combos[STEP3_CUST]   , 1, 2 );
		m_comboList->Add( m_combos[STEP3_PROG]   , 2, 2 );
		m_comboList->Add( m_combos[STEP3_FEEDER] , 3, 2 );
		m_comboList->Add( m_combos[STEP3_PACKAGE], 4, 2 );
		m_comboList->Add( m_combos[STEP3_MOVE]   , 5, 2 );
	}

	void onShow()
	{
		DrawText( 21, 3, MsgGetString(Msg_00892), GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_TXT ) );
		DrawText( 32, 3, MsgGetString(Msg_00893), GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_TXT ) );
		DrawText( 43, 3, MsgGetString(Msg_00894), GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_TXT ) );

		DrawPanel( RectI( 2, 10, GetW()/GUI_CharW() - 4, 6 ) );
		DrawTextCentered( 11, MsgGetString(Msg_00926), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawTextCentered( 12, MsgGetString(Msg_00900), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawTextCentered( 13, MsgGetString(Msg_00901), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawTextCentered( 14, MsgGetString(Msg_00902), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
	}

	void onRefresh()
	{
		m_combos[ENABLE]->SetTxtYN( convVal->enabled );
		m_combos[STEP1_CUST]->SetTxt( convVal->cust1 );
		m_combos[STEP1_PROG]->SetTxt( convVal->prog1 );
		m_combos[STEP1_FEEDER]->SetTxt( convVal->conf1 );
		m_combos[STEP1_PACKAGE]->SetTxt( convVal->lib1 );
		m_combos[STEP1_MOVE]->SetTxt( float(convVal->move1) );
		m_combos[STEP2_CUST]->SetTxt( convVal->cust2 );
		m_combos[STEP2_PROG]->SetTxt( convVal->prog2 );
		m_combos[STEP2_FEEDER]->SetTxt( convVal->conf2 );
		m_combos[STEP2_PACKAGE]->SetTxt( convVal->lib2 );
		m_combos[STEP2_MOVE]->SetTxt( float(convVal->move2) );
		m_combos[STEP3_CUST]->SetTxt( convVal->cust3 );
		m_combos[STEP3_PROG]->SetTxt( convVal->prog3 );
		m_combos[STEP3_FEEDER]->SetTxt( convVal->conf3 );
		m_combos[STEP3_PACKAGE]->SetTxt( convVal->lib3 );
		m_combos[STEP3_MOVE]->SetTxt( float(convVal->move3) );
	}

	void onEdit()
	{
		convVal->enabled = m_combos[ENABLE]->GetYN();
		if( convVal->enabled )
		{
			if( convVal->step1enabled )
			{
				m_combos[STEP1_CUST]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP1_PROG]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP1_FEEDER]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP1_PACKAGE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP1_MOVE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			}

			if( convVal->step2enabled )
			{
				m_combos[STEP2_CUST]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP2_PROG]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP2_FEEDER]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP2_PACKAGE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP2_MOVE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			}

			if( convVal->step3enabled )
			{
				m_combos[STEP3_CUST]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP3_PROG]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP3_FEEDER]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP3_PACKAGE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
				m_combos[STEP3_MOVE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			}
		}
		else
		{
			m_combos[STEP1_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP1_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP1_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP1_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP1_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP2_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP2_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP2_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP2_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP2_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP3_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP3_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP3_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP3_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP3_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		}

		convVal->move1 = m_combos[STEP1_MOVE]->GetFloat();
		convVal->move2 = m_combos[STEP2_MOVE]->GetFloat();
		convVal->move3 = m_combos[STEP3_MOVE]->GetFloat();
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00930), K_F3, 0, NULL, boost::bind( &ConveyorUI::onResetParams, this ) );
		m_menu->Add( MsgGetString(Msg_00892), K_F4, 0, SM_Step1, NULL );
		m_menu->Add( MsgGetString(Msg_00893), K_F5, 0, SM_Step2, NULL );
		m_menu->Add( MsgGetString(Msg_00894), K_F6, 0, SM_Step3, NULL );
		m_menu->Add( MsgGetString(Msg_00915), K_F7, 0, NULL, boost::bind( &ConveyorUI::onAdvancedParams, this ) );
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				onResetParams();
				return true;

			case K_F4:
				SM_Step1->Show();
				return true;

			case K_F5:
				SM_Step2->Show();
				return true;

			case K_F6:
				SM_Step3->Show();
				return true;

			case K_F7:
				onAdvancedParams();
				return true;

			case K_CTRL_HOME:
				onHomePosition();
				return true;

			case K_CTRL_1:
				onStep1Position();
				return true;

			case K_CTRL_2:
				onStep2Position();
				return true;

			case K_CTRL_3:
				onStep3Position();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		Mod_Conv( *convVal );
	}

private:
	int onResetParams()
	{
		convVal->zeroPos = 0.0;
		convVal->refPos = 0.0;
		convVal->step1enabled = 0;
		strcpy( convVal->cust1, "" );
		strcpy( convVal->prog1, "" );
		strcpy( convVal->conf1, "" );
		strcpy( convVal->lib1, "" );
		convVal->move1 = CONV_MOVE_DEF;
		convVal->step2enabled = 0;
		strcpy( convVal->cust2, "" );
		strcpy( convVal->prog2, "" );
		strcpy( convVal->conf2, "" );
		strcpy( convVal->lib2, "" );
		convVal->move2 = CONV_MOVE_DEF;
		convVal->step3enabled = 0;
		strcpy( convVal->cust3, "" );
		strcpy( convVal->prog3, "" );
		strcpy( convVal->conf3, "" );
		strcpy( convVal->lib3, "" );
		convVal->move3 = CONV_MOVE_DEF;

		m_combos[STEP1_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP1_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP1_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP1_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP1_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP2_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		m_combos[STEP3_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );

		onRefresh();

		return 1;
	}

	int getClientProgramName( char* cli, char* prog, char* fullpath )
	{
		std::vector<std::string> nameList;
		char path[MAXNPATH];

		// selezione cliente
		FindFiles( CLIDIR, 0, nameList );

		if( nameList.size() == 0 )
		{
			//TODO: messaggio di errore
			return 0;
		}

		std::string selectedCustomer = fn_Select( this, MsgGetString(Msg_00070), nameList );
		if( selectedCustomer.empty() )
		{
			return 0;
		}

		// selezione programma
		snprintf( path, MAXNPATH, "%s/%s/%s", CLIDIR, selectedCustomer.c_str(), PRGDIR );

		nameList.clear();
		FindFiles( path, "*" PRGEXT, nameList );

		if( nameList.size() == 0 )
		{
			W_Mess( MsgGetString(Msg_00090) );
		}

		std::string selectedProgram = fn_Select( this, MsgGetString(Msg_00229), nameList );
		if( selectedProgram.empty() )
		{
			return 0;
		}

		strcpy( cli, selectedCustomer.c_str() );
		strcpy( prog, selectedProgram.c_str() );

		char* buffer = fullpath;
		strcpy(buffer,CLIDIR);
		strcat(buffer,"/");
		strcat(buffer,cli);
		strcat(buffer,"/");
		strcat(buffer,PRGDIR);
		strcat(buffer,"/");
		strcat(buffer,prog);
		strcat(buffer,DTAEXT);

		return 1;
	}

	int onLoadStep1()
	{
		char cli[9];
		char prog[9];
		char path[MAXNPATH];

		if( getClientProgramName( cli, prog, path ) )
		{
			struct dta_data DtaVal;
			Read_Dta(&DtaVal, path);

			strcpy( convVal->cust1, cli );
			strcpy( convVal->prog1, prog );
			strcpy( convVal->conf1, DtaVal.lastconf );
			strcpy( convVal->lib1, DtaVal.lastlib );

			onRefresh();

			return 1;
		}

		return 0;
	}

	int onTeachStep1()
	{
		if( !Get_UseConveyor() )
		{
			W_Mess( MsgGetString(Msg_00928) );
			return 0;
		}

		float c_ax = 0.0, c_ay = 0.0;

		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00925) );
		MoveConveyorAndWait( convVal->move1 );
		delete pan;

		Set_Tv(2); // predispone per non richiudere immagine su video

		if( ManualTeaching( &c_ax, &c_ay, MsgGetString(Msg_00907), AUTOAPP_CONVEYOR ) )    // autoappr. posizione convogliatore
		{
			convVal->move1 = GetConveyorPosition();
		}

		Set_Tv(3); // chiude immagine su video

		onRefresh();

		return 1;
	}

	int onDisableStep1()
	{
		convVal->step1enabled = !convVal->step1enabled;

		if( convVal->step1enabled )
		{
			m_combos[STEP1_CUST]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP1_PROG]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP1_FEEDER]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP1_PACKAGE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP1_MOVE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
		}
		else
		{
			m_combos[STEP1_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP1_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP1_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP1_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP1_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		}

		return 1;
	}

	int onLoadStep2()
	{
		char cli[9];
		char prog[9];
		char path[MAXNPATH];

		if( getClientProgramName( cli, prog, path ) )
		{
			struct dta_data DtaVal;
			Read_Dta(&DtaVal, path);

			strcpy( convVal->cust2, cli );
			strcpy( convVal->prog2, prog );
			strcpy( convVal->conf2, DtaVal.lastconf );
			strcpy( convVal->lib2, DtaVal.lastlib );

			onRefresh();

			return 1;
		}

		return 0;
	}

	int onTeachStep2()
	{
		if( !Get_UseConveyor() )
		{
			W_Mess( MsgGetString(Msg_00928) );
			return 0;
		}

		float c_ax = 0.0, c_ay = 0.0;

		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00925) );
		MoveConveyorAndWait( convVal->move2 );
		delete pan;

		Set_Tv(2); // predispone per non richiudere immagine su video

		if( ManualTeaching( &c_ax, &c_ay, MsgGetString(Msg_00907), AUTOAPP_CONVEYOR ) )    // autoappr. posizione convogliatore
		{
			convVal->move2 = GetConveyorPosition();
		}

		Set_Tv(3); // chiude immagine su video

		onRefresh();

		return 1;
	}

	int onDisableStep2()
	{
		convVal->step2enabled = !convVal->step2enabled;

		if( convVal->step2enabled )
		{
			m_combos[STEP2_CUST]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP2_PROG]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP2_FEEDER]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP2_PACKAGE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP2_MOVE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
		}
		else
		{
			m_combos[STEP2_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP2_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP2_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP2_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP2_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		}

		return 1;
	}

	int onLoadStep3()
	{
		char cli[9];
		char prog[9];
		char path[MAXNPATH];

		if( getClientProgramName( cli, prog, path ) )
		{
			struct dta_data DtaVal;
			Read_Dta(&DtaVal, path);

			strcpy( convVal->cust3, cli );
			strcpy( convVal->prog3, prog );
			strcpy( convVal->conf3, DtaVal.lastconf );
			strcpy( convVal->lib3, DtaVal.lastlib );

			onRefresh();

			return 1;
		}

		return 0;
	}

	int onTeachStep3()
	{
		if( !Get_UseConveyor() )
		{
			W_Mess( MsgGetString(Msg_00928) );
			return 0;
		}

		float c_ax = 0.0, c_ay = 0.0;

		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00925) );
		MoveConveyorAndWait( convVal->move3 );
		delete pan;

		Set_Tv(2); // predispone per non richiudere immagine su video

		if( ManualTeaching( &c_ax, &c_ay, MsgGetString(Msg_00907), AUTOAPP_CONVEYOR ) )    // autoappr. posizione convogliatore
		{
			convVal->move3 = GetConveyorPosition();
		}

		Set_Tv(3); // chiude immagine su video

		onRefresh();

		return 1;
	}

	int onDisableStep3()
	{
		convVal->step3enabled = !convVal->step3enabled;

		if( convVal->step3enabled )
		{
			m_combos[STEP3_CUST]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP3_PROG]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP3_FEEDER]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP3_PACKAGE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
			m_combos[STEP3_MOVE]->SetReadOnlyBgColor( GUI_color( CB_COL_EDIT_BG ) );
		}
		else
		{
			m_combos[STEP3_CUST]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP3_PROG]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP3_FEEDER]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP3_PACKAGE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
			m_combos[STEP3_MOVE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
		}

		return 1;
	}

	int onHomePosition()
	{
		if( !Get_UseConveyor() )
		{
			W_Mess( MsgGetString(Msg_00928) );
			return 0;
		}

		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00927) );
		MoveConveyorAndWait( 0 );
		delete pan;

		return 1;
	}

	int onStep1Position()
	{
		if( !Get_UseConveyor() )
		{
			W_Mess( MsgGetString(Msg_00928) );
			return 0;
		}

		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00927) );
		MoveConveyorAndWait( convVal->move1 );
		delete pan;

		return 1;
	}

	int onStep2Position()
	{
		if( !Get_UseConveyor() )
		{
			W_Mess( MsgGetString(Msg_00928) );
			return 0;
		}

		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00927) );
		MoveConveyorAndWait( convVal->move2 );
		delete pan;

		return 1;
	}

	int onStep3Position()
	{
		if( !Get_UseConveyor() )
		{
			W_Mess( MsgGetString(Msg_00928) );
			return 0;
		}

		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00927) );
		MoveConveyorAndWait( convVal->move3 );
		delete pan;

		return 1;
	}

	int onAdvancedParams()
	{
		ConveyorAdvancedUI win( this );
		win.Show();
		win.Hide();

		return 1;
	}

	GUI_SubMenu* SM_Step1;
	GUI_SubMenu* SM_Step2;
	GUI_SubMenu* SM_Step3;
};

int fn_ConveyorWizard()
{
	char mess[512];
	char newName[9];
	struct conv_data convVal;

	if( !Get_UseConveyor() )
	{
		W_Mess( MsgGetString(Msg_00928) );
		return 0;
	}

	if( !W_Deci( 1, MsgGetString(Msg_00952) ) )
		return 0;

	// Reset dati convogliatore
	Read_Conv( &convVal );
	convVal.zeroPos = 0.0;
	convVal.refPos = 0.0;
	convVal.step1enabled = 0;
	strcpy( convVal.cust1, "" );
	strcpy( convVal.prog1, "" );
	strcpy( convVal.conf1, "" );
	strcpy( convVal.lib1, "" );
	convVal.move1 = CONV_MOVE_DEF;
	convVal.step2enabled = 0;
	strcpy( convVal.cust2, "" );
	strcpy( convVal.prog2, "" );
	strcpy( convVal.conf2, "" );
	strcpy( convVal.lib2, "" );
	convVal.move2 = CONV_MOVE_DEF;
	convVal.step3enabled = 0;
	strcpy( convVal.cust3, "" );
	strcpy( convVal.prog3, "" );
	strcpy( convVal.conf3, "" );
	strcpy( convVal.lib3, "" );
	convVal.move3 = CONV_MOVE_DEF;
	Mod_Conv( convVal );

	ZerFile *zer=new ZerFile(QHeader.Prg_Default);
	if(!zer->Open())
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		return 0;
	}

	if(zer->GetNRecs()<1)
	{
		W_Mess(NOZSCHEDA);
		delete zer;
		return 0;
	}
	delete zer;

	float xPos = 0.0, yPos = 0.0;
	float xZero = 0.0, yZero = 0.0;
	float deltaXz = 0.0, deltaYz = 0.0;
	float Xz = 0.0, Yz = 0.0;
	float Xr = 0.0, Yr = 0.0;
	float x1 = 0.0, x2 = 0.0;

	CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00916) );
	MoveConveyorPosition( CONV_HOME );
	delete pan;

	// Messaggio apprendimento zero
	W_Mess( MsgGetString(Msg_00961) );

	Set_Tv(2); // predispone per non richiudere immagine su video

	if( !ManualTeaching( &xZero, &yZero, MsgGetString(Msg_00015), AUTOAPP_CONVEYOR ) )    // autoappr. posizione zero
	{
		Set_Tv(3); // chiude immagine su video
		return 0;
	}

	Set_Tv(3); // chiude immagine su video

	// Setta posizioni di zero e step1 del convogliatore come attuali
	SetConveyorPosition( CONV_STEP1 );

	// Messaggio apprendimento posizione di split 1
	snprintf( mess, sizeof(mess), MsgGetString(Msg_00962), 1 );
	W_Mess( mess );

	Set_Tv(2); // predispone per non richiudere immagine su video

	xPos = xZero;
	yPos = yZero;
	if( !ManualTeaching( &xPos, &yPos, MsgGetString(Msg_00969) ) )    // autoappr. posizione X split 1
	{
		Set_Tv(3); // chiude immagine su video
		return 0;
	}
	x1 = xPos + GetConveyorPosition( CONV_ACT );

	Set_Tv(3); // chiude immagine su video

	// Crea il programma per lo step 1
	snprintf( mess, sizeof(mess), MsgGetString(Msg_00972), 1 );
	if( !PrgM_CreateNewFromX( newName, -999.9, x1, mess ) )
		return 0;

	// Associa il programma allo step 1 e lo abilita
	char newProg[MAXNPATH];
	struct dta_data DtaValDest;
	PrgPath( newProg, newName, PRG_DATA );
	Read_Dta(&DtaValDest, newProg);
	strcpy( convVal.cust1, QHeader.Cli_Default );
	strcpy( convVal.prog1, newName );
	strcpy( convVal.conf1, DtaValDest.lastconf );
	strcpy( convVal.lib1, DtaValDest.lastlib );
	convVal.step1enabled = 1;
	Mod_Conv( convVal );

	// Messaggio apprendimento posizione convogliatore e nuovo zero per step 2
	snprintf( mess, sizeof(mess), MsgGetString(Msg_00973), 2 );
	W_Mess( mess );

	Set_Tv(2); // predispone per non richiudere immagine su video

	if( !ManualTeaching( &xPos, &yPos, MsgGetString(Msg_00015), AUTOAPP_CONVEYOR ) )    // autoappr. posizione zero
	{
		Set_Tv(3); // chiude immagine su video
		return 0;
	}

	Set_Tv(3); // chiude immagine su video

	// Coordinate dello zero dello step 2
	Xz = xPos;
	Yz = yPos;

	// Coordinate da sottrarre ai componenti dello step 2
	deltaXz = GetConveyorPosition( CONV_ACT ) + xPos - xZero;
	deltaYz = yPos - yZero;

	// Setta posizione di step 2 del convogliatore come attuale
	SetConveyorPosition( CONV_STEP2 );

	// Messaggio apprendimento posizione nuovo riferimento per step 2
	snprintf( mess, sizeof(mess), MsgGetString(Msg_00974), 2 );
	W_Mess( mess );

	Set_Tv(2); // predispone per non richiudere immagine su video

	if( !ManualTeaching( &xPos, &yPos, MsgGetString(Msg_00016) ) )    // autoappr. posizione riferimento
	{
		Set_Tv(3); // chiude immagine su video
		return 0;
	}

	Set_Tv(3); // chiude immagine su video

	// Coordinate del riferimento dello step 2
	Xr = xPos;
	Yr = yPos;

	// Messaggio apprendimento posizione di split 2
	snprintf( mess, sizeof(mess), MsgGetString(Msg_00962), 2 );
	W_Mess( mess );

	Set_Tv(2); // predispone per non richiudere immagine su video

	if( !ManualTeaching( &xPos, &yPos, MsgGetString(Msg_00969) ) )    // autoappr. posizione X split 2
	{
		Set_Tv(3); // chiude immagine su video
		return 0;
	}
	x2 = xPos + GetConveyorPosition( CONV_ACT );

	Set_Tv(3); // chiude immagine su video

	// Crea il programma per lo step 2
	snprintf( mess, sizeof(mess), MsgGetString(Msg_00972), 2 );
	if( !PrgM_CreateNewFromX( newName, x1, x2, mess, deltaXz, deltaYz, Xz, Yz, Xr, Yr ) )
		return 0;

	// Associa il programma allo step 2 e lo abilita
	PrgPath( newProg, newName, PRG_DATA );
	Read_Dta(&DtaValDest, newProg);
	Read_Conv( &convVal );
	strcpy( convVal.cust2, QHeader.Cli_Default );
	strcpy( convVal.prog2, newName );
	strcpy( convVal.conf2, DtaValDest.lastconf );
	strcpy( convVal.lib2, DtaValDest.lastlib );
	convVal.step2enabled = 1;
	Mod_Conv( convVal );

	if( W_Deci( 1, MsgGetString(Msg_00976) ) )
	{
		// Messaggio apprendimento posizione convogliatore e nuovo zero per step 3
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00973), 3 );
		W_Mess( mess );

		Set_Tv(2); // predispone per non richiudere immagine su video

		if( !ManualTeaching( &xPos, &yPos, MsgGetString(Msg_00015), AUTOAPP_CONVEYOR ) )    // autoappr. posizione zero
		{
			Set_Tv(3); // chiude immagine su video
			return 0;
		}

		Set_Tv(3); // chiude immagine su video

		// Coordinate dello zero dello step 3
		Xz = xPos;
		Yz = yPos;

		// Coordinate da sottrarre ai componenti dello step 3
		deltaXz = GetConveyorPosition( CONV_ACT ) + xPos - xZero;
		deltaYz = yPos - yZero;

		// Setta posizione di step 3 del convogliatore come attuale
		SetConveyorPosition( CONV_STEP3 );

		// Messaggio apprendimento posizione nuovo riferimento per step 3
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00974), 3 );
		W_Mess( mess );

		Set_Tv(2); // predispone per non richiudere immagine su video

		if( !ManualTeaching( &xPos, &yPos, MsgGetString(Msg_00016) ) )    // autoappr. posizione riferimento
		{
			Set_Tv(3); // chiude immagine su video
			return 0;
		}

		Set_Tv(3); // chiude immagine su video

		// Coordinate del riferimento dello step 3
		Xr = xPos;
		Yr = yPos;

		// Crea il programma per lo step 3
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00972), 3 );
		if( !PrgM_CreateNewFromX( newName, x2, 999.9, mess, deltaXz, deltaYz, Xz, Yz, Xr, Yr ) )
			return 0;

		// Associa il programma allo step 3 e lo abilita
		PrgPath( newProg, newName, PRG_DATA );
		Read_Dta(&DtaValDest, newProg);
		Read_Conv( &convVal );
		strcpy( convVal.cust3, QHeader.Cli_Default );
		strcpy( convVal.prog3, newName );
		strcpy( convVal.conf3, DtaValDest.lastconf );
		strcpy( convVal.lib3, DtaValDest.lastlib );
		convVal.step3enabled = 1;
		Mod_Conv( convVal );
	}

	//convogliatore a home
	if( Get_ConveyorEnabled() )
	{
		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00916) );
		MoveConveyorPosition( CONV_HOME );
		delete pan;
	}

	return 1;
}

int fn_ConveyorData( CWindow* parent )
{
	ConveyorUI win( parent );
	win.Show();
	win.Hide();

	return 1;
}


//abbandona procedura
#define PRGUPDATE_ABORT       0
//update riga completato
#define PRGUPDATE_OKROW       1
//skip di una riga
#define PRGUPDATE_SKIPROW     2
//update riga completato (prosegui per tutte le altre righe senza richiesta di conferma)
#define PRGUPDATE_CONFIRMALL  3

//update di un singolo componente
#define PRGUPDATE_ONE         0
//update di tutti i componenti
#define PRGUPDATE_ALL         1


//aggiunge un record in tabella espansa (uno per scheda)
int AddAssemblyRec(struct TabPrg newrec)
{
	struct Zeri zero;

	ZerFile *zer=new ZerFile(QHeader.Prg_Default);   //apre file zeri
	if(!zer->Open())
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		return 0;
	}
	
	int zer_count=zer->GetNRecs();
	
	int rec = TPrgExp->Count();
	
	for(int j=1;j<zer_count;j++)
	{
		zer->Read(zero,j);
	
		if(!zero.Z_ass)
		{
			newrec.status|=NOMNTBRD_MASK; //SMOD021003
		}
	
		newrec.Riga=rec+1;
		newrec.scheda=j;
		newrec.status&=~(RDEL_MASK | RMOV_MASK);
		
		TPrgExp->Write(newrec,rec,FLUSHON);
		
		rec++;
	}
	
	updateDosa=1;
	
	delete zer;

	return 1;
}

//cancella una board se presente dal programma espanso
int DelAssemblyBoard(int board)
{
	int nrecs=TPrgExp->Count();
	int flag=0;
	struct TabPrg rec;

	for(int i=0;i<nrecs;i++)
	{
		TPrgExp->Read(rec,i);
		if(rec.scheda==board)
		{
			rec.status|=RDEL_MASK;
			TPrgExp->Write(rec,i);
			flag=1;
		}
	}

	if(flag)
	{
		updateDosa=1;
		TPrgExp->DelSel();
	}

	return 1;
}

//aggiunge una board nel programma espanso
int AddAssemblyBoard(int board,int mount)
{
  int i,prg_count,rec;
  struct TabPrg *tab;

  //cancella se presente la board indicata
  DelAssemblyBoard(board);

  // ** READ DATA
  prg_count=TPrgNormal->Count();

  tab=new TabPrg[prg_count];

  for(i=0;i<prg_count;i++)
  {
	  TPrgNormal->Read(tab[i],i);
  }

  rec=TPrgExp->Count();

  for(i=0;i<prg_count;i++)
  {
    tab[i].Riga=rec+1;
    tab[i].scheda=board;
    tab[i].status&=~(RDEL_MASK | RMOV_MASK);
    if(!mount)
      tab[i].status|=NOMNTBRD_MASK;
    else
      tab[i].status&=~NOMNTBRD_MASK;

    TPrgExp->Write(tab[i],rec,FLUSHON);
    rec++;
  }

  updateDosa=1;

  delete[] tab;

  return 1;

}

//SMOD080403
int CreateDosaFile(void)
{
	int k,i,n,openExp=0,dosaClose=0,count=0;
	unsigned int offset;
	struct TabPrg *rec,*AllRecs;

	float px,py,angle;
	struct Zeri   ZerData[2];
	
	if(!TPrgExp->IsOnDisk() || TPrgExp->Count()<=0)
	{
		W_Mess( MsgGetString(Msg_01209) );
		return 0;
	}

	if(TPrgExp->GetHandle()==0)
	{
		TPrgExp->Open(SKIPHEADER);
		openExp=1;
	}

	n=TPrgExp->Count();
	
	if(n==0)
	{
		if(openExp)
		{
			TPrgExp->Close();
		}
		return 0;
	}
  
	ZerFile *zer=new ZerFile(QHeader.Prg_Default);
	if(!zer->Open())
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		return 0;
	}

	AllRecs=new TabPrg[n];
	
	for(i=0;i<n;i++)
	{
		TPrgExp->Read(AllRecs[i],i);

		#ifndef __DISP2
		if((!(AllRecs[i].status & DODOSA_MASK)) || (AllRecs[i].status & NOMNTBRD_MASK))
		#else
		if(!(AllRecs[i].status & (DODOSA_MASK | DODOSA2_MASK)) || (AllRecs[i].status & NOMNTBRD_MASK))
		#endif
		{
			continue;
		}

		count++;
	}

	if(openExp)
	{
		TPrgExp->Close();
	}
	
	//SMOD290403 START
	if(!count)
	{
		delete[] AllRecs;
		delete zer;
		return 0;
	}
	//SMOD290403 END

	rec=new TabPrg[count];

	k=0;

	zer->Read(ZerData[0],0); //read master
	
	struct Zeri *AllZeri=new struct Zeri[zer->GetNRecs()];
	for(i=0;i<zer->GetNRecs();i++)
	{
		AllZeri[i].Z_scheda=-1;
	}
	
	for(i=0;i<n;i++)
	{
		#ifndef __DISP2
		if((AllRecs[i].status & DODOSA_MASK) && (!(AllRecs[i].status & NOMNTBRD_MASK)))
		#else
		if((AllRecs[i].status & (DODOSA_MASK | DODOSA2_MASK)) && (!(AllRecs[i].status & NOMNTBRD_MASK)))
		#endif
		{
			rec[k]=AllRecs[i];

			if(AllZeri[AllRecs[i].scheda].Z_scheda==-1)
			{
				zer->Read(AllZeri[AllRecs[i].scheda],AllRecs[i].scheda);
			}
		
			ZerData[1]=AllZeri[AllRecs[i].scheda];

			Get_PosData(ZerData,rec[k],NULL,0,0,px,py,angle,1);

			rec[k].XMon=px;
			rec[k].YMon=py;

			k++;
		}
  	}

	delete[] AllZeri;
	
	if(TPrgDosa->GetHandle()!=0)
	{
		TPrgDosa->Close();
		dosaClose=1;
	}
	
	TPrgDosa->Create();
	
	TPrgDosa->Open();

	#ifndef __LINEDISP
	offset = (mem_pointer)&rec[0].XMon-(mem_pointer)rec;
	
	SortData((void *)rec,SORTFIELDTYPE_FLOAT32,count,offset,sizeof(struct TabPrg));
	#endif

	for(i=0;i<count;i++)
	{
		rec[i]=AllRecs[rec[i].Riga-1];
		rec[i].Riga=i+1;
		TPrgDosa->Write(rec[i],i);
	}

	k=0;

	delete[] rec;
	
	rec=new TabPrg[n-count];

	for(i=0;i<n;i++)
	{
		#ifndef __DISP2
		if((!(AllRecs[i].status & DODOSA_MASK)) || (AllRecs[i].status & NOMNTBRD_MASK))
		#else
		if((!(AllRecs[i].status & (DODOSA_MASK | DODOSA2_MASK))) || (AllRecs[i].status & NOMNTBRD_MASK))
		#endif
		{
			rec[k++]=AllRecs[i];
		}
	}

	for(i=0;i<k;i++)
	{
		rec[i].Riga=count+i+1;
		TPrgDosa->Write(rec[i],count+i);
	}

	delete[] AllRecs;
	delete[] rec;

	delete zer; //SMOD110403

	updateDosa=0;

	TPrgDosa->Close();

	if(dosaClose)
	{
		TPrgDosa->Open();
	}

	return 1;

}


//DANY281102
int CreateAssemblyFile(int mode=CREATEASS_REFRESH)
{
	char buf[80];
	struct Zeri zero;
	int prg_count,zer_count;
	int i,j,rec=0,exp_flag=0;

	if(Get_AssemblingFlag())            //se assemblaggio in corso
	{
		if(!W_Deci(0,WARN_ASSEMBLING1))   //ask conferma
		{
			return 0;
		}
	}

	// ** READ DATA
	prg_count=TPrgNormal->Count();

	struct TabPrg *tabTmp=new TabPrg[prg_count];

	for(i=0;i<prg_count;i++)
	{
		TPrgNormal->Read(tabTmp[i],i);
		//SMOD291107
	}

	if(TPrgExp->GetHandle()!=0)    //se assembly file gia aperto
	{
		exp_flag=1;
		TPrgExp->Close();            //chiudi
	}

	TPrgExp->Create();             //crea nuovo/sovrascrivi vecchio
	TPrgExp->Open(SKIPHEADER);     //apre file

	ZerFile *zer=new ZerFile(QHeader.Prg_Default);   //apre file zeri
	if(!zer->Open())
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		return 0;
	}
	zer_count=zer->GetNRecs();

	int err=0;

	//** WRITE DATA
	for(j=1;j<zer_count;j++)
	{
		zer->Read(zero,j);

		for(i=0;i<prg_count;i++)
		{
			if(TPrgNormal->Search(i+1,tabTmp[i],PRG_CODSEARCH)!=-1)
			{
				snprintf(buf, sizeof(buf),ERRDUPCOMP,tabTmp[i].CodCom);
				W_Mess(buf);
				err=1;
				break;
			}

			tabTmp[i].Riga=rec+1;
			tabTmp[i].scheda=j;
			tabTmp[i].status&=~(RDEL_MASK | RMOV_MASK);
			tabTmp[i].Changed=0;

			if(!zero.Z_ass)
			{
				tabTmp[i].status|=NOMNTBRD_MASK;
			}
			else
			{
				tabTmp[i].status&=~NOMNTBRD_MASK;
			}

      		TPrgExp->Write(tabTmp[i],rec,FLUSHON);

     		 rec++;
		}

		if(err)
		{
			break;
		}
  	}

	if(!exp_flag)               //se assembly file non era aperto
	{
		TPrgExp->Close();         //chiusi file
	}
	
	if(!err)
	{
		updateDosa=1;
	}

	delete zer;

	delete[] tabTmp;

	Prg_ClearAll();
	CurRecord=0;
	CurRow=0;

	ReadRecs(CurRecord-CurRow);                    // reload dei record

	if(mode==CREATEASS_REFRESH)
	{
		programTableRefresh = true;
	}

  return 1;

}

//DANY161202
void UpdateAssemblyBoards(int mode,int prevNZeri)
{
	int i,j,nz,np;
	struct TabPrg rec;
	struct Zeri *zerList=NULL;

	if(!mode)
	{
		return;
	}
	
	if(mode & ZER_NEWMASTER)          //se nuovo master
	{
		//SMOD110403
		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00116) );
		CreateAssemblyFile(CREATEASS_NOREFRESH); //ricrea programma di assemblaggio
		delete pan;
		programTableRefresh = true;
		return;
	}
	else                              //altrimenti: solo update
	{
		ZerFile *zer=new ZerFile(QHeader.Prg_Default);
		if(!zer->Open())
		{
			W_Mess(NOZSCHFILE);
			delete zer;
			return;
		}
		nz=zer->GetNRecs();
		//leggi l'intera nuova struttura degli zeri
		zerList=new struct Zeri[nz];
		zer->ReadAll(zerList,nz);
		delete zer;

		//se aggiunte o rimosse delle schede, update del programma di assemblaggio
		if(nz!=prevNZeri)
		{
			if( nz < prevNZeri ) //Se eliminate delle schede, si ricrea il prog di assemb.
			{
				CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00116) );
				CreateAssemblyFile();           //ricrea programma di assemblaggio
				delete pan;
			}
			else //Aggiunte schede -> si aggiungono al prog di assemb.
			{
				//aggiungi le nuove schede nel programma di assemblaggio
				CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00116) );
				for(i=prevNZeri;i<nz;i++)
				{
					AddAssemblyBoard(zerList[i].Z_scheda,zerList[i].Z_ass);
				}
				delete pan;
			}
    	}

		//SMOD180303

		//update del flag monta SI/NO
		CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00116) );

		for(j=0;j<nz;j++)
		{
			np=TPrgExp->Count();
			for(i=0;i<np;i++) //ciclo update programma di assemblaggio
			{
				TPrgExp->Read(rec,i);
				//se numero di scheda non corrispondente
				if(rec.scheda!=zerList[j].Z_scheda)
				{
					continue;   //skip
				}

				//se scheda da non montare
				if(!zerList[j].Z_ass)
				{
					if(!(rec.status & NOMNTBRD_MASK))
					{
						rec.status|=NOMNTBRD_MASK;
						updateDosa=1;
					}
				}
				else
				{ 
					if(rec.status & NOMNTBRD_MASK)
					{
						rec.status&=~NOMNTBRD_MASK;
						updateDosa=1;
					}
				}

        		TPrgExp->Write(rec,i,FLUSHON);

			} //fine ciclo lettura programma di assemblaggio
		} //fine ciclo lettura schede

    	delete pan;
	}
	
	if(zerList!=NULL)
	{
		delete[] zerList;
	}

}


int Prg_AskUpdate(struct TabPrg rec)
{ char buf[260];
  int c;

  snprintf(buf, sizeof(buf),ASK_UPDATEASSF,rec.scheda,rec.CodCom);
  W_MsgBox *box=new W_MsgBox(MsgGetString(Msg_00289),buf,3,MSGBOX_GESTKEY);

  box->AddButton(FAST_YES,0);
  box->AddButton(FAST_NO,1);
  box->AddButton(FAST_ALL,0);

  c=box->Activate();
  delete box;
  return(c);
}

//DANY041202
int Prg_Update(TPrgFile *file,struct TabPrg search,struct TabPrg newtab)
{
	int status = PRGUPDATE_OKROW;
	int test_array[7]={PUNTA_FIELD,PX_FIELD|PY_FIELD,ROT_FIELD,MOUNT_FIELD,DOSA_FIELD,NOTE_FIELD,VERSION_FIELD};

	ZerFile* zer=new ZerFile(QHeader.Prg_Default);
	if(!zer->Open())
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		return 0;
	}

	int pointer = 0;
	while( (pointer=file->Search(pointer,search,PRG_CODSEARCH,0)) != -1 )
	{
		if(!newtab.Changed)
		{
			pointer++;
			continue;
		}

		struct TabPrg intab;
    	file->Read(intab,pointer);

		status=PRGUPDATE_OKROW;

		for( int i = 0; i < 7; i++ )
		{
			if(newtab.Changed & test_array[i])
			{
				if(prgupdateAskFlag && (intab.Changed & test_array[i]))
				{
					status=Prg_AskUpdate(intab);
				}
				else
				{
					status=PRGUPDATE_OKROW;
				}

				switch(status)
				{
					case PRGUPDATE_ABORT:
						return(status);
						break;
					case PRGUPDATE_SKIPROW:
						continue;
					case PRGUPDATE_CONFIRMALL:
						prgupdateAskFlag=0;
						break;
				}

				intab.Changed&=~test_array[i];

				switch(i)
				{
					case 0:
						intab.Punta=newtab.Punta;
						break;
					case 1:
						intab.XMon=newtab.XMon;
						intab.YMon=newtab.YMon;
						break;
					case 2:
						intab.Rotaz=newtab.Rotaz;
						break;
					case 3:
						intab.status=(intab.status & ~MOUNT_MASK) | (newtab.status & MOUNT_MASK);
						break;
					case 4:
						#ifndef __DISP2
						intab.status=(intab.status & ~DODOSA_MASK) | (newtab.status & DODOSA_MASK);
						#else
						intab.status=(intab.status & ~(DODOSA_MASK | DODOSA2_MASK)) | (newtab.status & (DODOSA_MASK | DODOSA2_MASK));
						#endif
						break;
					case 5:
						strcpy(intab.NoteC,newtab.NoteC);
						break;
					case 6:
						intab.status=(intab.status & ~VERSION_MASK) | (newtab.status & VERSION_MASK);
						break;
				}
			}
		}

		strcpy(intab.CodCom,newtab.CodCom);
		strcpy(intab.TipCom,newtab.TipCom);
		intab.Caric=newtab.Caric;
		strcpy(intab.pack_txt,newtab.pack_txt);

		file->Write(intab,pointer);
		pointer++;
	}

	delete zer;
	return status;
}

//aggiorna record in tabella espansa
int UpdateAssemblyRec(struct TabPrg prevtab,struct TabPrg newtab)
{
	int ret = 0;
	if(TPrgExp->IsOnDisk())
	{
		ret = Prg_Update(TPrgExp,prevtab,newtab);
		if( ret )
		{
			updateDosa = 1;
		}
	}
	return ret;
}


//aggiorna il programma espanso
//DANY131202
int UpdateAssemblyFile(GUI_ProgressBar_OLD *progress)
{
  int i,prg_count;
  int flag=0,ret;
  struct TabPrg tab;

  if(!TPrgExp->IsOnDisk())
    return 0;

  TPrgExp->InitSearchBuf();

  TPrgExp->DelSel();

  prg_count=TPrgNormal->Count();

  prgupdateAskFlag=1;

  for(i=0;i<prg_count;i++)
  {
    if(progress!=NULL)
    {
      progress->Increment(1);
    }

    TPrgNormal->Read(tab,i);
    if(tab.Changed)// && !(tab.status & DUP_MASK))
    {
      ret=UpdateAssemblyRec(tab,tab); //update di un record preesistente

      if(ret==PRGUPDATE_ABORT)
      {
        TPrgExp->DestroySearchBuf();
        return 0;
      }
      else
      {
        if(ret!=PRGUPDATE_SKIPROW)
        {
          flag|=1;
        }
      }

      tab.Changed=0;
      TPrgNormal->Write(tab,i,FLUSHON);
    }
  }

  TPrgExp->DestroySearchBuf();

  return(flag);
}


//---------------------------------------------------------------------------
//* Funzioni Menu

// appr. zero sch./punto rifer.
int PrgM_ZAuto(void)
{
	// Come prima cosa verifico se il master e' stato appreso.
	// In caso contrario, richiamo il pannello di composizione!
	ZerFile *zer=new ZerFile(QHeader.Prg_Default,ZER_ADDPATH,true);
	if( !zer->Open() )
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		return 0;
	}

	//get numero record in file Zeri
	int prevnz = zer->GetNRecs();

	if( prevnz == 0 )
	{
		W_Mess(NOZSCHEDA);

		zer->Close();
		delete zer;
		return PrgM_Zeri();
	}
	else
	{
		if(!AskConfirm_ZRefBoardMode())
		{
			return 0;
		}

		struct TabPrg tmpPrg;
		TPrg->Read(tmpPrg,CurRow);

		Z_scheda(tmpPrg.scheda,ZER_APP_ZEROREF,ZER_APP_MANUAL);

		/*
		if((prevnz==0) && (zer->GetNRecs()!=0))
		{
			UpdateAssemblyBoards(ZER_NEWMASTER,prevnz);

			if(TPrg==TPrgExp)
			{
				Reccount = TPrg->Count();         // n. di records
				ReadRecs(CurRecord-CurRow);
				programTableRefresh = true;
			}
		}
		*/

		delete zer;
		return 1;
	}
}

int PrgM_ZAuto2(void)
{
	//SMOD260603
	if(Reccount==0)
	{
		W_Mess(ERRNORECS);
		return 0;
	}  
	
	if(!AskConfirm_ZRefBoardMode())
	{
		return 0;
	}  
	
	ZerFile *zer=new ZerFile(QHeader.Prg_Default,ZER_ADDPATH,true);
	if(!zer->Open(SKIPHEADER))                   
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		return 0;
	}

	if(zer->GetNRecs()>1)
	{
		struct TabPrg tmpPrg;
		TPrg->Read(tmpPrg,CurRow);
	
		Z_scheda(tmpPrg.scheda,ZER_APP_ZEROREF,ZER_APP_AUTO);
	}
	else
	{
		W_Mess(NOZSCHEDA);
	}
	
	delete zer;
	return 1;
}



//---------------------------------------------------------------------------
// finestra: Board zero position
//---------------------------------------------------------------------------
class BoardZeroPositionUI : public CWindowParams
{
public:
	BoardZeroPositionUI( CWindow* parent, float x, float y ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU | WIN_STYLE_EDITMODE_ON );
		SetClientAreaPos( 0, 7 );
		SetClientAreaSize( 40, 5 );
		SetTitle( MsgGetString(Msg_01178) );

		m_x = x;
		m_y = y;
	}

	int GetExitCode() { return m_exitCode; }
	float GetPositionOnX() { return m_x; }
	float GetPositionOnY() { return m_y; }

	typedef enum
	{
		NUM_X,
		NUM_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[NUM_X] = new C_Combo( 8, 1, "X (mm) :", 8, CELL_TYPE_SDEC );
		m_combos[NUM_Y] = new C_Combo( 8, 3, "Y (mm) :", 8, CELL_TYPE_SDEC );

		// set params
		m_combos[NUM_X]->SetVMinMax( QParam.LX_mincl, QParam.LX_maxcl );
		m_combos[NUM_Y]->SetVMinMax( QParam.LY_mincl, QParam.LY_maxcl );

		// add to combo list
		m_comboList->Add( m_combos[NUM_X], 0, 0 );
		m_comboList->Add( m_combos[NUM_Y], 1, 0 );
	}

	void onShow()
	{
		tips = new CPan( 20, 2, MsgGetString(Msg_00296), MsgGetString(Msg_00297) );
	}

	void onRefresh()
	{
		m_combos[NUM_X]->SetTxt( m_x );
		m_combos[NUM_Y]->SetTxt( m_y );
	}

	void onEdit()
	{
		m_x = m_combos[NUM_X]->GetFloat();
		m_y = m_combos[NUM_Y]->GetFloat();
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_ESC:
				m_exitCode = WIN_EXITCODE_ESC;
				break;

			case K_ENTER:
				forceExit();
				m_exitCode = WIN_EXITCODE_ENTER;
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		delete tips;
	}

	CPan* tips;
	float m_exitCode;
	float m_x, m_y;
};


int PrgM_RedefineZero(void) //SMOD310303
{
	float dx,dy;
	int count,i;
	struct Zeri zdata_new,zdata_prev;
	
	struct TabPrg tabdat;
	
	ZerFile *zer=new ZerFile(QHeader.Prg_Default);
	if(!zer->Open())                   
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		return 0;
	}
	
	if(zer->GetNRecs()<1)
	{
		W_Mess(NOZSCHEDA);
		delete zer;
		return 0;
	}

	//SMOD260603
	if(Reccount==0)
	{
		W_Mess(ERRNORECS);
		delete zer;
		return 0;
	}    
	
	zer->Read(zdata_prev,0);
	
	if(Get_AssemblingFlag())    //se assemblaggio in corso
	{
		if(!W_Deci(0,WARN_ASSEMBLING1))   //ask conferma
		{
			delete zer;
			return 0;
		}
	}

	if(!W_Deci(0,MSG_REDEFZERI))
	{
		delete zer;
		return 0;
	}

	if(!AskConfirm_ZRefBoardMode())
	{
		delete zer;
		return 0;
	}
	
	if(!Z_scheda(0,ZER_APP_ZERO,ZER_APP_MANUAL))
	{
		delete zer;
		return 0;
	}

	zer->Read(zdata_new,0);
	
	zer->Close();
	zer->Create();
	zer->Open();


	BoardZeroPositionUI inputBox( 0, zdata_new.Z_xzero, zdata_new.Z_yzero );
	inputBox.Show();
	inputBox.Hide();
	if( inputBox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		delete zer;
		return 0;
	}

	zdata_new.Z_xzero = inputBox.GetPositionOnX();
	zdata_new.Z_yzero = inputBox.GetPositionOnY();

	
	//se lo zero ed il riferimento precedenti all'operazione di ridifinizione
	//dello zero erano coincidenti: sposta anche il riferimento per mantenerli
	//coincidenti.
	if((fabs(zdata_prev.Z_xzero-zdata_prev.Z_xrif)<0.02) && (fabs(zdata_prev.Z_yzero-zdata_prev.Z_yrif)<0.02))
	{
		zdata_new.Z_xrif=zdata_new.Z_xzero;
		zdata_new.Z_yrif=zdata_new.Z_yzero;
		zdata_new.Z_rifangle=0;
	}
	else
	{
		//altrimenti mantieni vecchio riferimento
		zdata_new.Z_xrif=zdata_prev.Z_xrif;
		zdata_new.Z_yrif=zdata_prev.Z_yrif;
		zdata_new.Z_rifangle=atan2((zdata_new.Z_yrif-zdata_new.Z_yzero),(zdata_new.Z_xrif-zdata_new.Z_xzero));
	}

	dx=zdata_new.Z_xzero-zdata_prev.Z_xzero;
	dy=zdata_new.Z_yzero-zdata_prev.Z_yzero;
	
	//SMOD310303
	zer->Write( zdata_new, 0 );
	zdata_new.Z_scheda=1;
	zdata_new.Z_ass=1;
	zer->Write( zdata_new, 1 );

	count=TPrgNormal->Count();
	for(i=0;i<count;i++)
	{ 
		TPrgNormal->Read(tabdat,i);
		tabdat.XMon-=dx;
		tabdat.YMon-=dy;
		TPrgNormal->Write(tabdat,i,FLUSHON);
	}
    
	if(TPrgExp->GetHandle()!=0)
	{ 
		count=TPrgExp->Count();
		for(i=0;i<count;i++)
		{ 
			TPrgExp->Read(tabdat,i);
			tabdat.XMon-=dx;
			tabdat.YMon-=dy;
			TPrgExp->Write(tabdat,i,FLUSHON);
		}
	}

  	Set_AssemblingFlag(0);
  	CreateAssemblyFile();

	delete zer;
	
	Prg_Reload();
	programTableRefresh = true;
	return 1;
}


// parametri immagine
int PrgM_ImgPar(void)
{
	ShowImgParams( ZEROIMG );         // Selezione parametri immagine.
	ShowImgParams( RIFEIMG );         // Selezione parametri immagine.
	return 1;
}

// apprendimento componente selezionato
int PrgM_AutoComp()
{
	// setta i parametri della telecamera
	SetImgBrightCont( CurDat.HeadBright, CurDat.HeadContrast );

	Prg_Autocomp(AUTO_COMP,CurRecord,TPrg);
	
	if(TPrg==TPrgNormal)
	{
		UpdateAssemblyFile();
	}
	ReadRecs(CurRecord-CurRow);
	programTableRefresh = true;
	return 1;
}

int PrgM_RioUge(void)
{
	Prg_Sort(SORT_PUNTAUGE,SORT_ASKCONFIRM);
	return 1;
}

int PrgM_RioTipo(void)
{
	Prg_Sort(SORT_TIPOCOM,SORT_ASKCONFIRM);        // riordino per tipo comp.
	return 1;
}

int PrgM_RioCodComp(void)
{
	Prg_Sort(SORT_CODCOM,SORT_ASKCONFIRM);         // riordino per codice componente
	return 1;
}

int PrgM_RioCodCar(void)
{
	Prg_Sort(SORT_CARIC,SORT_ASKCONFIRM);                   // riordino per cod. caric.
	return 1;
}

int PrgM_RioPunta(void)
{
	Prg_Sort(SORT_PUNTA,SORT_ASKCONFIRM);                   // riordino per punta
	return 1;
}

int PrgM_RioScheda(void)
{
	Prg_Sort(SORT_NUMSCHEDA,SORT_ASKCONFIRM);              // riordino per scheda
	return 1;
}

int PrgM_RioNote(void)
{
	Prg_Sort(SORT_NOTE,SORT_ASKCONFIRM);              // riordino per note
	return 1;
}

int PrgM_RioXCoord(void)
{
	Prg_Sort(SORT_XMON,SORT_ASKCONFIRM);              // riordino per note
	return 1;
}

void _PrgM_MenuSortOptNorm( int nozzle_mode )
{
	if( !check_data( CHECKPRG_FULL ) )
	{
		return;
	}

	PrgOptimize2* optimize = new PrgOptimize2( QHeader.Prg_Default, nozzle_mode );
	
	if( optimize->InitOptimize() )
	{
		bool optiOk = true;

		#ifdef __OPTI_DEBUG
			//utilizzata per debug
			FILE *aaa_sets = fopen( "aaa_sets.txt" , "wt" );
			optiOk = optimize->DoOptimize_NN( aaa_sets );
			fclose( aaa_sets );
		
			FILE *aaa_opt = fopen( "aaa_opt.txt" , "wt" );
			optimize->PrintOptimize( aaa_opt );
			fclose( aaa_opt );
		#else
			optiOk = optimize->DoOptimize_NN();
		#endif

		if( optiOk )
		{
			if( !optimize->WriteOptimize() )
			{
				W_Mess( "Error on WriteOptimize !" );
			}
		}
	}

	delete optimize;

	ReadRecs(CurRecord-CurRow); // reload dei record
	programTableRefresh = true;
}

int PrgM_MenuSortOptNorm(void)
{
	_PrgM_MenuSortOptNorm(3);
	return 1;
}

int PrgM_MenuSortOptNorm1(void)
{
	_PrgM_MenuSortOptNorm(1);
	return 1;
}

int PrgM_MenuSortOptNorm2(void)
{
	_PrgM_MenuSortOptNorm(2);
	return 1;
}

//GF_14_07_2011 - START
int PrgM_MountAll_Y(void)
{
	if( W_Deci( 1, AREYOUSURETXT ) )
		Prg_ChangeAllRows( ProgramTableUI::COL_ASSEM, 'Y' ); // Assembla tutto

	return 1;
}

int PrgM_MountAll_N(void)
{
	if( W_Deci( 1, AREYOUSURETXT ) )
		Prg_ChangeAllRows( ProgramTableUI::COL_ASSEM, 'N' ); // Assembla niente

	return 1;
}

#ifndef __DISP2
int PrgM_DispAll_Y(void)
{
	if( W_Deci( 1, AREYOUSURETXT ) )
		Prg_ChangeAllRows( ProgramTableUI::COL_DISP, 'Y' ); // Dispensa tutto

	return 1;
}
#else
int PrgM_DispAll_1(void)
{
	Prg_ChangeAllRows( ProgramTableUI::COL_DISP, '1' ); // Dispensa tutto 1

	return 1;
}

int PrgM_DispAll_2(void)
{
	if( W_Deci( 1, AREYOUSURETXT ) )
		Prg_ChangeAllRows( ProgramTableUI::COL_DISP, '2' ); // Dispensa tutto 2

	return 1;
}

int PrgM_DispAll_A(void)
{
	if( W_Deci( 1, AREYOUSURETXT ) )
		Prg_ChangeAllRows( ProgramTableUI::COL_DISP, 'A' ); // Dispensa tutto A

	return 1;
}
#endif

int PrgM_DispAll_N(void)
{
	if( W_Deci( 1, AREYOUSURETXT ) )
		Prg_ChangeAllRows( ProgramTableUI::COL_DISP, 'N' ); // Dispensa niente

	return 1;
}
//GF_14_07_2011 - END

int PrgM_SetFiducial1(void)
{
	GUI_Freeze_Locker lock;

	struct TabPrg X_Tab;

	int src_rec=CurRecord;
	int src_rig=CurRow;

	if( TPrg!=TPrgNormal )
	{
		bipbip();
		return 0;
	}

	TPrg->Read(X_Tab,src_rec);

	strcpy(X_Tab.pack_txt,FIDUCIAL1_TEXT);
	X_Tab.Changed|=PACK_FIELD;
	#ifndef __DISP2
	X_Tab.status&=~DODOSA_MASK;
	#else
	X_Tab.status&=~(DODOSA_MASK | DODOSA2_MASK);
	#endif
	X_Tab.Changed|=DOSA_FIELD;
	X_Tab.status&=~MOUNT_MASK;
	X_Tab.Changed|=MOUNT_FIELD;
	X_Tab.status|=FID1_MASK;
	X_Tab.status&=~FID2_MASK;

	TPrg->Write(X_Tab,src_rec,FLUSHON);

	UpdateAssemblyFile();

	ReadRecs(src_rec-src_rig);     // lettura dell'array di structs
	programTableRefresh = true;

	return 1;
}

int PrgM_SetFiducial2(void)
{
	GUI_Freeze_Locker lock;

	struct TabPrg X_Tab;

	int src_rec=CurRecord;
	int src_rig=CurRow;

	if( TPrg!=TPrgNormal )
	{
		bipbip();
		return 0;
	}

	TPrg->Read(X_Tab,src_rec);

	strcpy(X_Tab.pack_txt,FIDUCIAL2_TEXT);
	X_Tab.Changed|=PACK_FIELD;
	#ifndef __DISP2
	X_Tab.status&=~DODOSA_MASK;
	#else
	X_Tab.status&=~(DODOSA_MASK | DODOSA2_MASK);
	#endif
	X_Tab.Changed|=DOSA_FIELD;
	X_Tab.status&=~MOUNT_MASK;
	X_Tab.Changed|=MOUNT_FIELD;
	X_Tab.status|=FID2_MASK;
	X_Tab.status&=~FID1_MASK;

	TPrg->Write(X_Tab,src_rec,FLUSHON);

	UpdateAssemblyFile();

	ReadRecs(src_rec-src_rig);     // lettura dell'array di structs
	programTableRefresh = true;

	return 1;
}

int PrgM_ResetFiducial(void)
{
	GUI_Freeze_Locker lock;

	struct TabPrg X_Tab;

	int src_rec=CurRecord;
	int src_rig=CurRow;

	if( TPrg!=TPrgNormal )
	{
		bipbip();
		return 0;
	}

	TPrg->Read(X_Tab,src_rec);

	strcpy(X_Tab.pack_txt,"");
	X_Tab.Changed|=PACK_FIELD;
	X_Tab.status&=~(FID1_MASK | FID2_MASK);

	TPrg->Write(X_Tab,src_rec,FLUSHON);

	UpdateAssemblyFile();

	ReadRecs(src_rec-src_rig);     // lettura dell'array di structs
	programTableRefresh = true;

	return 1;
}

int PrgM_SplitSel(void)
{
	struct TabPrg tabDat;
	int pointer=0;
	int doUpdate=0; //SMOD120503

	if(W_Deci(1, MsgGetString(Msg_05186) ))
	{
		char newName[9];
		char newProg[MAXNPATH];

		CInputBox inbox( 0, 8, MsgGetString(Msg_00475), MsgGetString(Msg_00983), 8, CELL_TYPE_TEXT );
		inbox.SetLegalChars( CHARSET_FILENAME );
		inbox.Show();

		if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
		{
			return 0;
		}

		snprintf( newName, 9, "%s", inbox.GetText() );
		strlwr( newName );

		PrgPath( newProg, newName );

		if( CheckFile( newProg ) )
		{
			W_Mess( MsgGetString(Msg_00093) ); // already present
			return 0;
		}

		// crea programma
		TPrgFile* newPrg = new TPrgFile( newName, PRG_NORMAL );
		int ret = newPrg->Create();

		if( !ret )
		{
			W_Mess( MsgGetString(Msg_05187) );
			delete newPrg;
			return 0;
		}

		newPrg->Open();

		//Ricerca delle righe da dividere per eliminarle nell'espanso dell'attuale e crearle nel programma nuovo
		int numRec = 0;
		for(int r_loop=0;r_loop<TPrg->Count();r_loop++)
		{
			TPrg->Read(tabDat,r_loop);
			if(tabDat.status & RDEL_MASK)//se riga da eliminare/dividere
			{
				// Crea il record nel programma nuovo (prima marcandolo come non selezionato...)
				tabDat.status &=~ (RDEL_MASK | RMOV_MASK);
				newPrg->Write(tabDat,numRec++,FLUSHON);
				tabDat.status |= (RDEL_MASK | RMOV_MASK);

				// Elimina i record dall'eventuale espanso dell'attuale
				pointer=0;
				while((pointer=TPrgExp->Search(pointer,tabDat,PRG_CODSEARCH))!=-1)
				{
					TPrgExp->Read(tabDat,pointer);
					tabDat.status|=RDEL_MASK;
					TPrgExp->Write(tabDat,pointer,FLUSHON);
					pointer++;
					doUpdate=1; //SMOD120503
				}
			}
		}

		newPrg->Close();

		// Copia nel nuovo file di programma la conf. caricatori e la lib. package attuali
		struct dta_data DtaValAct;
		struct dta_data DtaValDest;
		PrgPath( newProg, newName, PRG_DATA );
		Read_Dta(&DtaValAct);
		Read_Dta(&DtaValDest, newProg);
		strcpy( DtaValDest.lastconf, DtaValAct.lastconf );
		strcpy( DtaValDest.lastlib, DtaValAct.lastlib );
		Save_Dta(DtaValDest, newProg);

		// Copia il file degli zeri
		char sFile[MAXNPATH];
		char dFile[MAXNPATH];
		snprintf( sFile, MAXNPATH, "%s/%s/%s/%s%s", CLIDIR, QHeader.Cli_Default, PRGDIR, QHeader.Prg_Default, ZEREXT );
		snprintf( dFile, MAXNPATH, "%s/%s/%s/%s%s", CLIDIR, QHeader.Cli_Default, PRGDIR, newName, ZEREXT );
		CopyFile( dFile, sFile );

		// Se non c'erano righe selezionate, avviso che il programma e' stato creato vuoto!
		if( numRec == 0 )
		{
			W_Mess( MsgGetString(Msg_05188) );
		}

		if(doUpdate) //SMOD120503
			UpdateAssemblyFile();

		// Elimina i record dal programma attuale
		Reccount=TPrg->DelSel();

		Prg_ClearAll();             //svuota tabella
		if(Reccount==0)             //se eliminati tutti i records
		Prg_InitAll(0,1);             //setta riga di default
		ReadRecs(0);                //legge e mostra tabella dall'inizio
		CurRow=0;
		CurRecord=0;
		programTableRefresh = true;

		delete newPrg;

		return 1;
	}

	return 0;
}

int PrgM_SplitX(void)
{
	struct TabPrg tabDat;
	int pointer=0;
	int doUpdate=0; //SMOD120503

	if(W_Deci(1, MsgGetString(Msg_05186) ))
	{
		char newName[9];
		char newProg[MAXNPATH];

		CInputBox inbox( 0, 8, MsgGetString(Msg_00475), MsgGetString(Msg_00983), 8, CELL_TYPE_TEXT );
		inbox.SetLegalChars( CHARSET_FILENAME );
		inbox.Show();

		if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
		{
			return 0;
		}

		snprintf( newName, 9, "%s", inbox.GetText() );
		strlwr( newName );

		PrgPath( newProg, newName );

		if( CheckFile( newProg ) )
		{
			W_Mess( MsgGetString(Msg_00093) ); // already present
			return 0;
		}

		// crea programma
		TPrgFile* newPrg = new TPrgFile( newName, PRG_NORMAL );
		int ret = newPrg->Create();

		if( !ret )
		{
			W_Mess( MsgGetString(Msg_05187) );
			delete newPrg;
			return 0;
		}

		float xcoord = 0.0;

		CInputBox inboxf( 0, 8, MsgGetString(Msg_05184), MsgGetString(Msg_05189), 8, CELL_TYPE_SDEC );
		inboxf.Show();

		if( inboxf.GetExitCode() == WIN_EXITCODE_ESC )
		{
			return 0;
		}

		xcoord = inboxf.GetFloat();

		newPrg->Open();

		//Ricerca delle righe da dividere per eliminarle nell'espanso dell'attuale e crearle nel programma nuovo
		int numRec = 0;
		for(int r_loop=0;r_loop<TPrg->Count();r_loop++)
		{
			TPrg->Read(tabDat,r_loop);
			if(tabDat.XMon > xcoord)//se riga da eliminare/dividere
			{
				// Crea il record nel programma nuovo
				newPrg->Write(tabDat,numRec++,FLUSHON);

				// Marca il record nel programma attuale come da eliminare
				tabDat.status |= RDEL_MASK;
				TPrg->Write(tabDat,r_loop,FLUSHON);

				// Elimina i record dall'eventuale espanso dell'attuale
				pointer=0;
				while((pointer=TPrgExp->Search(pointer,tabDat,PRG_CODSEARCH))!=-1)
				{
					TPrgExp->Read(tabDat,pointer);
					tabDat.status|=RDEL_MASK;
					TPrgExp->Write(tabDat,pointer,FLUSHON);
					pointer++;
					doUpdate=1; //SMOD120503
				}
			}
		}

		newPrg->Close();

		// Copia nel nuovo file di programma la conf. caricatori e la lib. package attuali
		struct dta_data DtaValAct;
		struct dta_data DtaValDest;
		PrgPath( newProg, newName, PRG_DATA );
		Read_Dta(&DtaValAct);
		Read_Dta(&DtaValDest, newProg);
		strcpy( DtaValDest.lastconf, DtaValAct.lastconf );
		strcpy( DtaValDest.lastlib, DtaValAct.lastlib );
		Save_Dta(DtaValDest, newProg);

		// Copia il file degli zeri
		char sFile[MAXNPATH];
		char dFile[MAXNPATH];
		snprintf( sFile, MAXNPATH, "%s/%s/%s/%s%s", CLIDIR, QHeader.Cli_Default, PRGDIR, QHeader.Prg_Default, ZEREXT );
		snprintf( dFile, MAXNPATH, "%s/%s/%s/%s%s", CLIDIR, QHeader.Cli_Default, PRGDIR, newName, ZEREXT );
		CopyFile( dFile, sFile );

		// Se non c'erano righe selezionate, avviso che il programma e' stato creato vuoto!
		if( numRec == 0 )
		{
			W_Mess( MsgGetString(Msg_05188) );
		}

		if(doUpdate) //SMOD120503
			UpdateAssemblyFile();

		// Elimina i record dal programma attuale
		Reccount=TPrg->DelSel();

		Prg_ClearAll();             //svuota tabella
		if(Reccount==0)             //se eliminati tutti i records
		Prg_InitAll(0,1);             //setta riga di default
		ReadRecs(0);                //legge e mostra tabella dall'inizio
		CurRow=0;
		CurRecord=0;
		programTableRefresh = true;

		delete newPrg;

		return 1;
	}

	return 0;
}

int PrgM_CreateNewFromX( char* newName, float xcoordInf, float xcoordSup, const char* title, float deltaZeroX, float deltaZeroY, float zerX, float zerY, float rifX, float rifY )
{
	struct TabPrg tabDat;

	char newProg[MAXNPATH];

	CInputBox inbox( 0, 8, title, MsgGetString(Msg_00983), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	PrgPath( newProg, newName );

	if( CheckFile( newProg ) )
	{
		W_Mess( MsgGetString(Msg_00093) ); // already present
		return 0;
	}

	// crea programma master e espanso
	TPrgFile* newPrgM = new TPrgFile( newName, PRG_NORMAL );
	TPrgFile* newPrgE = new TPrgFile( newName, PRG_ASSEMBLY );
	int ret = newPrgM->Create();
	if( !ret )
	{
		W_Mess( MsgGetString(Msg_05187) );
		delete newPrgM;
		return 0;
	}
	ret = newPrgE->Create();
	if( !ret )
	{
		W_Mess( MsgGetString(Msg_05187) );
		delete newPrgE;
		return 0;
	}

	newPrgM->Open();
	newPrgE->Open();

	//Ricerca delle righe da creare nel programma nuovo
	int numRec = 0;

	for(int r_loop=0;r_loop<TPrg->Count();r_loop++)
	{
		TPrg->Read(tabDat,r_loop);
		if((tabDat.XMon >= xcoordInf) && (tabDat.XMon < xcoordSup))//se riga da eliminare/dividere
		{
			// Crea il record nel programma nuovo
			// Se richiesto, modifico le coordinate di assemblaggio
			if( (deltaZeroX != 0.0) || (deltaZeroY != 0.0) )
			{
				tabDat.XMon -= deltaZeroX;
				tabDat.YMon -= deltaZeroY;
			}
			newPrgM->Write(tabDat,numRec++,FLUSHON);
			newPrgE->Write(tabDat,numRec++,FLUSHON);
		}
	}

	newPrgM->Close();
	newPrgE->Close();

	// Copia nel nuovo file di programma la conf. caricatori e la lib. package attuali
	struct dta_data DtaValAct;
	struct dta_data DtaValDest;
	PrgPath( newProg, newName, PRG_DATA );
	Read_Dta(&DtaValAct);
	Read_Dta(&DtaValDest, newProg);
	strcpy( DtaValDest.lastconf, DtaValAct.lastconf );
	strcpy( DtaValDest.lastlib, DtaValAct.lastlib );
	Save_Dta(DtaValDest, newProg);

	// Copia il file degli zeri
	char sFile[MAXNPATH];
	char dFile[MAXNPATH];
	snprintf( sFile, MAXNPATH, "%s/%s/%s/%s%s", CLIDIR, QHeader.Cli_Default, PRGDIR, QHeader.Prg_Default, ZEREXT );
	snprintf( dFile, MAXNPATH, "%s/%s/%s/%s%s", CLIDIR, QHeader.Cli_Default, PRGDIR, newName, ZEREXT );
	CopyFile( dFile, sFile );

	// Se richiesto, modifico le coordinate di zero e riferimento
	if( (deltaZeroX != 0.0) || (deltaZeroY != 0.0) )
	{
		struct Zeri zdata_new;

		ZerFile *zer=new ZerFile(newName);
		if(!zer->Open())
		{
			W_Mess(NOZSCHFILE);
			delete zer;
			return 0;
		}

		zer->Read(zdata_new,0);

		zer->Close();
		zer->Create();
		zer->Open();

		zdata_new.Z_xzero = zerX;
		zdata_new.Z_yzero = zerY;
		zdata_new.Z_xrif = rifX;
		zdata_new.Z_yrif = rifY;
		if((fabs(zdata_new.Z_xzero-zdata_new.Z_xrif)<0.02) && (fabs(zdata_new.Z_yzero-zdata_new.Z_yrif)<0.02))
		{
			zdata_new.Z_xrif=zdata_new.Z_xzero;
			zdata_new.Z_yrif=zdata_new.Z_yzero;
			zdata_new.Z_rifangle=0;
		}
		else
		{
			//altrimenti mantieni vecchio riferimento
			zdata_new.Z_rifangle=atan2((zdata_new.Z_yrif-zdata_new.Z_yzero),(zdata_new.Z_xrif-zdata_new.Z_xzero));
		}

		zer->Write( zdata_new, 0 );
		zdata_new.Z_scheda=1;
		zdata_new.Z_ass=1;
		zer->Write( zdata_new, 1 );
	}

	delete newPrgM;
	delete newPrgE;

	return 1;
}

int PrgM_CoordX(void)
{
	struct TabPrg tabDat;
	int pointer=0;

	float xcoord = 0.0;

	CInputBox inboxf( 0, 8, MsgGetString(Msg_05192), MsgGetString(Msg_05189), 8, CELL_TYPE_SDEC );
	inboxf.Show();

	if( inboxf.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	xcoord = inboxf.GetFloat();

	// Modifica valori
	for(int r_loop=0;r_loop<TPrg->Count();r_loop++)
	{
		TPrg->Read(tabDat,r_loop);
		tabDat.XMon += xcoord;

		TPrg->Write(tabDat,r_loop,FLUSHON);

		// Elimina i record dall'eventuale espanso dell'attuale
		pointer=0;
		while((pointer=TPrgExp->Search(pointer,tabDat,PRG_CODSEARCH))!=-1)
		{
			TPrgExp->Read(tabDat,pointer);
			tabDat.XMon += xcoord;
			TPrgExp->Write(tabDat,pointer,FLUSHON);
			pointer++;
		}
	}

	Prg_ClearAll();             //svuota tabella
	if(Reccount==0)             //se eliminati tutti i records
		Prg_InitAll(0,1);             //setta riga di default
	ReadRecs(0);                //legge e mostra tabella dall'inizio
	CurRow=0;
	CurRecord=0;
	programTableRefresh = true;

	return 1;
}

int PrgM_CoordY(void)
{
	struct TabPrg tabDat;
	int pointer=0;

	float ycoord = 0.0;

	CInputBox inboxf( 0, 8, MsgGetString(Msg_05192), MsgGetString(Msg_05193), 8, CELL_TYPE_SDEC );
	inboxf.Show();

	if( inboxf.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	ycoord = inboxf.GetFloat();

	// Modifica valori
	for(int r_loop=0;r_loop<TPrg->Count();r_loop++)
	{
		TPrg->Read(tabDat,r_loop);
		tabDat.YMon += ycoord;

		TPrg->Write(tabDat,r_loop,FLUSHON);

		// Elimina i record dall'eventuale espanso dell'attuale
		pointer=0;
		while((pointer=TPrgExp->Search(pointer,tabDat,PRG_CODSEARCH))!=-1)
		{
			TPrgExp->Read(tabDat,pointer);
			tabDat.YMon += ycoord;
			TPrgExp->Write(tabDat,pointer,FLUSHON);
			pointer++;
		}
	}

	Prg_ClearAll();             //svuota tabella
	if(Reccount==0)             //se eliminati tutti i records
		Prg_InitAll(0,1);             //setta riga di default
	ReadRecs(0);                //legge e mostra tabella dall'inizio
	CurRow=0;
	CurRecord=0;
	programTableRefresh = true;

	return 1;
}

int PrgM_FindZerom(void)
{
	ZeroMaccManSearch();         // zero macchina manuale
	return 1;
}

//massimo errore di ripetibilita nella ricerca dello zero theta
#define MAXERR_THETA 2


int CheckZTheta( int nozzle )
{
	if( Get_OnFile() )
	{
		return 0;
	}

	Ugelli->Depo( nozzle );

	// pre-ruota punta per scansione
	#ifdef __SNIPER
	PuntaRotDeg( -90-10, nozzle, BRUSH_ABS );
	#endif
	Wait_EncStop( nozzle );

	CPan* pan = new CPan( -1, 1, MsgGetString(Msg_01040) );

	#ifdef __SNIPER
	int ret = Sniper_FindZeroTheta( nozzle, 1 );
	#endif
	delete pan;

	Wait_EncStop( nozzle );

	if(ret)
	{
		//segno invertito perche la Fox conta gli angoli in senso opposto
		int pos = -FoxHead->ReadBrushPosition( nozzle == 1 ? BRUSH1 : BRUSH2 );
		if(pos>2048)
		{
			pos=4096-pos;
		}

		unsigned short enc_step = nozzle == 1 ? QHeader.Enc_step1 : QHeader.Enc_step2;
		if((abs(pos))>(float)MAXERR_THETA/360.0*enc_step)
		{
			char buf[160];
			snprintf( buf, sizeof(buf), MsgGetString(Msg_00966), fabs((pos*360.0)/enc_step) );
			if( W_Deci( 0, buf ) )
			{
				PuntaRotDeg( 0, nozzle, BRUSH_RESET );
			}
		}
		else
		{
			W_Mess(MsgGetString(Msg_00967));
		}
	}
	else
	{
		W_Mess(MsgGetString(Msg_00968));
	}

	PuntaRotStep( 0, nozzle, BRUSH_ABS );
	PuntaZSecurityPos( nozzle );
	Wait_EncStop( nozzle );
	PuntaZPosWait( nozzle );
	return 1;
}



int PrgM_CheckZThetaP1()
{
	return CheckZTheta( 1 );
}

int PrgM_CheckZThetaP2()
{
	return CheckZTheta( 2 );
}


int PrgM_CheckZPosP1(void)
{
  if(Get_OnFile())
    return 0;

  //SMOD210503
  PuntaZPosWait(1);
  CheckZAxis(1);
  return 1;
}

int PrgM_CheckZPosP2(void)
{
  if(Get_OnFile())
    return 0;

  //SMOD210503
  PuntaZPosWait(2);
  CheckZAxis(2);
  return 1;
}


int PrgM_appCI(void)
{
	Prg_Autocomp(AUTO_IC,CurRecord,TPrg); // appr. posiz. IC

	if( TPrg == TPrgNormal )
	{
		UpdateAssemblyFile();
	}
	ReadRecs(CurRecord-CurRow);
	programTableRefresh = true;
	return 1;
}

int PrgM_AutoCarSeq(void)
{
	if(!check_data(CHECKPRG_FULL))
	{
		return 0;
	}

	Feeder_SeqPickPosition(); // appr. sequenz. caricatori
	return 1;
}


int PrgM_AutoCarZSeq_Auto()
{
	if(!check_data(CHECKPRG_FULL))
	{
		return 0;
	}

	// Appr. sequenziale (AUTO) quota prelievo caricatori
	FeederZSeq_Auto();
	return 1;
}

int PrgM_AutoCarZSeq_Man()
{
	if(!check_data(CHECKPRG_FULL))
	{
		return 0;
	}

	// Appr. sequenziale (MANUAL) quota prelievo caricatori
	FeederZSeq_Man();
	return 1;
}

int PrgCaricGraph()
{
	if( check_data(CHECKPRG_COMPONENTS) )
	{
		CaricHistogram* graph = new CaricHistogram();
		delete graph;
		ReadRecs(CurRecord-CurRow);
		programTableRefresh = true;
	}
	return 1;
}

int PrgM_AutoCompSeq()
{
	// appr. sequenziale componenti
	Prg_SeqTeachPosition();
	return 1;
}

int PrgM_Check(void)
{
	if( check_data(CHECKPRG_FULL) )  // check prog./configuraz
	{
		W_Mess( MsgGetString(Msg_01116) );
	}
	else
	{
		W_Mess( MsgGetString(Msg_01117) );
	}
	
	ReadRecs(CurRecord-CurRow);     // reload dei record
	programTableRefresh = true;
	return 1;
}

//SMOD290703-CARINT
int PrgM_CarIntSearch(void)
{
	CInputBox inbox( programWin, 6, MsgGetString(Msg_00398), MsgGetString(Msg_00103), 40, CELL_TYPE_TEXT );
	//inbox.SetLegalChar( "0123456789ABCDEFGHILMNOPQRSTUVZXYWKJabcdefghijklmnopqrstuvwxyz~!#$%&*()=+:-_/.,\\ " );
	inbox.SetText( ATab[CurRow].TipCom );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	char searchtxt[41];
	snprintf( searchtxt, 41, "%s", inbox.GetText() );


	programWin->Deselect();

	if( UpdateDBData(CARINT_UPDATE_FULL) )
	{
		FeederAdvancedSearchUI win( 0 );
		if( win.Search( searchtxt ) )
		{
			win.Show();
			win.Hide();
		}
	}

	programWin->Select();
	return 1;
}

int PrgM_CarInt(void)
{
	programWin->Deselect();

	CarInt(0,ATab[CurRow].Caric);

	programWin->Select();
	return 1;
}

int PrgM_SelectVers(void)
{
	CInputBox inbox( programWin, 6, MsgGetString(Msg_00245), MsgGetString(Msg_02032), 1, CELL_TYPE_UINT );
	inbox.SetVMinMax( 0, 9 );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	int vers = inbox.GetInt();


	programWin->Deselect();

	int nrec=TPrgNormal->Count();
	TabPrg tabrec;

	int nmounted=0;
	int nversioned=0;

	for(int i=0;i<nrec;i++)
	{
		TPrgNormal->Read(tabrec,i);

		if(!(tabrec.status & VERSION_MASK))
		{
		  //non includere nella ricerca le righe non versionate: lascia
		  //inalterato il campo mount
		  continue;
		}

		char* ptr=strchr(tabrec.CodCom,SELECTVERS_SPECIALCHAR);

		if(ptr==NULL)
		{
		  //lascia inalterato il campo mounted
		  //tabrec.status&=~MOUNT_MASK;
		}
		else
		{
		  nversioned++;

		  for(;*ptr!='\0';ptr++)
		  {
			if((*ptr-'0')==vers)
			{
			  tabrec.status|=MOUNT_MASK;
			  tabrec.Changed|=MOUNT_FIELD;
			  nmounted++;
			  break;
			}
		  }

		  if(*ptr=='\0')
		  {
			tabrec.status&=~MOUNT_MASK;
			tabrec.Changed|=MOUNT_FIELD;
		  }
		}

		if(tabrec.Changed)
		{
		  UpdateAssemblyRec(tabrec,tabrec); //update in espanso
		  tabrec.Changed&=~MOUNT_FIELD;
		  TPrgNormal->Write(tabrec,i,FLUSHON);
		}
	}

	if(nversioned)
	{
		if(nmounted==0)
		{
			char buf[160];
			snprintf(buf, sizeof(buf),SELECTVERS_NOFOUND,vers);
			W_Mess(buf);
		}
		else
		{
			char buf[160];
			snprintf(buf, sizeof(buf),SELECTVERS_DONE,nmounted,vers);
			W_Mess(buf);
		}

		CurRecord=0;
		CurRow=0;
		Prg_ClearAll();
		ReadRecs(CurRecord);
		programTableRefresh = true;
	}
	else
	{
		W_Mess(SELECTVERS_NOVERS);
	}

	programWin->Select();
	return 1;
}

int PrgM_FindComp(void)
{
	CInputBox inbox( programWin, 6, MsgGetString(Msg_00398), MsgGetString(Msg_01161), 16, CELL_TYPE_TEXT );
	//inbox.SetLegalChar( "0123456789ABCDEFGHILMNOPQRSTUVZXYWKJabcdefghijklmnopqrstuvwxyz~!#$%&*()=+:-_/.,\\ " );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	char searchtxt[20];
	snprintf( searchtxt, sizeof(searchtxt), "%s", inbox.GetText() );


	struct TabPrg rec;
	int ret;

	strcpy(rec.CodCom,searchtxt);

	int start_rec;
	
	if(CurRecord == lastCodCompFound)
	{
		start_rec = CurRecord + 1;
	}
	else
	{
		start_rec = CurRecord;
	}

	ret=TPrg->Search(start_rec,rec,PRG_CODSEARCH);
	if(ret!=-1)
	{
		lastCodCompFound = ret;
		CurRecord=ret;
		CurRow=0;
		Prg_ClearAll();
		ReadRecs(CurRecord);
		programTableRefresh = true;
	}
	else
	{
		W_Mess(FINDCOMP_ERR);
	}
	return 1;
}


#ifndef __DISP2
int PrgM_DispParams()
{
	return fn_DispenserParams( programWin, 1 );
}

int PrgM_Dosat(void)
{
	return GestDos(0,0);                     // Dosaggio totale
}

int PrgM_DosStep(void)
{
	return GestDos(0,1);                     // Dosaggio totale
}

int PrgM_DosRestart(void)
{
	return GestDos(1,0);                     // ripart. dosaggio
}

int PrgM_DosStepRestart(void)
{
	return GestDos(1,1);                     // ripart. dosaggio
}

int PrgM_DosAss(void)
{
	GestDosAss();                     // Dosaggio & assemblaggio totale
	return 1;
}

int PrgM_DosAssStep(void)
{
	return GestDosAss(0,1);                  // Dosaggio & assembl. passo passo
}

int PrgM_DosAssRestart(void)
{
	return GestDosAss(1);                     // ripart. dosaggio & assembl.
}

int PrgM_DosAssStepRestart(void)
{
	return GestDosAss(1,1);                     // ripart. dosaggio & assembl.
}
#else
int PrgM_DispParams1()
{
	return fn_DispenserParams( programWin, 1 );
}

int PrgM_DispParams2()
{
	return fn_DispenserParams( programWin, 2 );
}

int PrgM_Dosat1(void)
{
	GestDos(1,0,0);                     // Dosaggio totale
}

int PrgM_DosStep1(void)
{
	GestDos(1,0,1);                     // Dosaggio totale
}

int PrgM_DosRestart1(void)
{
	GestDos(1,1,0);                     // ripart. dosaggio
}

int PrgM_DosStepRestart1(void)
{
	GestDos(1,1,1);                     // ripart. dosaggio
}

int PrgM_Dosat2(void)
{
	GestDos(2,0,0);                     // Dosaggio totale
}

int PrgM_DosStep2(void)
{
	GestDos(2,0,1);                     // Dosaggio totale
}

int PrgM_DosRestart2(void)
{
	GestDos(2,1,0);                     // ripart. dosaggio
}

int PrgM_DosStepRestart2(void)
{
	GestDos(2,1,1);                     // ripart. dosaggio
}

int PrgM_DosAss1(void)
{
	GestDosAss(1);                     // Dosaggio & assemblaggio totale
}

int PrgM_DosAssStep1(void)
{
	GestDosAss(1,0,1);                  // Dosaggio & assembl. passo passo
}

int PrgM_DosAssRestart1(void)
{
	GestDosAss(1,1);                     // ripart. dosaggio & assembl.
}

int PrgM_DosAssStepRestart1(void)
{
	GestDosAss(1,1,1);                     // ripart. dosaggio & assembl.
}

int PrgM_DosAss2(void)
{
	GestDosAss(2);                     // Dosaggio & assemblaggio totale
}

int PrgM_DosAssStep2(void)
{
	GestDosAss(2,0,1);                  // Dosaggio & assembl. passo passo
}

int PrgM_DosAssRestart2(void)
{
	GestDosAss(2,1);                     // ripart. dosaggio & assembl.
}

int PrgM_DosAssStepRestart2(void)
{
	GestDosAss(2,1,1);                     // ripart. dosaggio & assembl.
}

int PrgM_Dos1Dos2Ass(void)
{
	GestDos12Ass(DOSASS_IDX_DOSA1,DOSASS_IDX_DOSA2,DOSASS_IDX_ASSEM);
}

int PrgM_Dos1Dos2AssStep(void)
{
	GestDos12Ass(DOSASS_IDX_DOSA1,DOSASS_IDX_DOSA2,DOSASS_IDX_ASSEM,0,1);
}

int PrgM_Dos1Dos2AssRestart(void)
{
	GestDos12Ass(DOSASS_IDX_DOSA1,DOSASS_IDX_DOSA2,DOSASS_IDX_ASSEM,1);
}

int PrgM_Dos1Dos2AssStepRestart(void)
{
	GestDos12Ass(DOSASS_IDX_DOSA1,DOSASS_IDX_DOSA2,DOSASS_IDX_ASSEM,1,1);
}

int PrgM_Dos1AssDos2(void)
{
	GestDos12Ass(DOSASS_IDX_DOSA1,DOSASS_IDX_ASSEM,DOSASS_IDX_DOSA2);
}

int PrgM_Dos1AssDos2Step(void)
{
	GestDos12Ass(DOSASS_IDX_DOSA1,DOSASS_IDX_ASSEM,DOSASS_IDX_DOSA2,0,1);
}

int PrgM_Dos1AssDos2Restart(void)
{
	GestDos12Ass(DOSASS_IDX_DOSA1,DOSASS_IDX_ASSEM,DOSASS_IDX_DOSA2,1);
}

int PrgM_Dos1AssDos2StepRestart(void)
{
	GestDos12Ass(DOSASS_IDX_DOSA1,DOSASS_IDX_ASSEM,DOSASS_IDX_DOSA2,1,1);
}

#endif

int PrgM_Pkg()
{
	fn_PackagesTable( programWin, ATab[CurRow].pack_txt );

	programTableRefresh = true;
	return 1;
}

int PrgM_DosaTableSwitch(void)
{
	Prg_SwitchTable(PRG_DOSAT);
	return 1;
}

int PrgM_Caric()
{
	char tipcomp[21];
	char packtxt[21];

	programWin->Deselect();

	int caric = (ATab[CurRow].Caric != 0) ? ATab[CurRow].Caric : 11;
	caric = G_TCaric( caric, tipcomp, packtxt );

	programWin->Select();

	if( caric != -1 )
	{
		ATab[CurRow].Caric = caric;
		strcpy(ATab[CurRow].TipCom,tipcomp);
		strcpy(ATab[CurRow].pack_txt,packtxt);
		if((AFlag) || ((CurRecord==0) && (Reccount==0)))
		{
			TPrg->Write(ATab[CurRow],CurRecord);
			Reccount++;
			AFlag=0;
		}
		else
		{
			TPrg->Write(ATab[CurRow],CurRecord);
		}
	}
	programTableRefresh = true;
	return 1;
}

int PrgM_Zeri(void)
{
	int prevnz;
	//SMOD080403 START
	
	//apre il file degli zeri
	
	struct dta_data DtaVal;
	Read_Dta(&DtaVal);

	ZerFile *zer=new ZerFile(QHeader.Prg_Default);
	if(zer->Open())
	{
		//get numero record in file Zeri
		prevnz=zer->GetNRecs();
	}
	else
	{
		prevnz=0;
	}
	
	//carica tutti i record della tabella degli zeri
	struct Zeri *zdat_prev=NULL;
	if(prevnz!=0)
	{
		zdat_prev=new struct Zeri[prevnz];
		zer->ReadAll(zdat_prev,prevnz);
	}

	delete zer;


	programWin->Deselect();

	//gestione tabella zeri
	int mode=G_Zeri();
	
	programWin->Select();

	//se nuovo numero elementi in tabella zeri e' uguale al precedente
	int newzeri=zer->GetNRecs();
	
	if((newzeri==prevnz) && prevnz!=0)
	{
		ZerFile* zer = new ZerFile(QHeader.Prg_Default);
		if(!zer->Open())                   
		{
			W_Mess(NOZSCHFILE);
			delete zer;
			delete [] zdat_prev;
			return 0;
		}
	
		struct Zeri *zdat_new=new struct Zeri[newzeri];
		zer->ReadAll(zdat_new,newzeri);
	
		//se sono avvenute modifiche in tabella zeri
		if(memcmp((char *)zdat_new,(char *)zdat_prev,sizeof(struct Zeri)*newzeri)!=0)
		{
			//aggiorna tabella di dosaggio
			updateDosa=1;
		}

		delete zer;
		
		delete[] zdat_new;
	}
	else
	{ 
		//se e' cambiato il numero degli zeri
		if(prevnz!=0)
		{
			updateDosa=1; //aggiorna tabella di dosaggio
		}
	}

	if(zdat_prev!=NULL)
	{
		delete [] zdat_prev;
	}
	
	
	if(Reccount!=0)
	{
		UpdateAssemblyBoards(mode,prevnz);

		CheckZeriProg();      //SMOD021003

		if(TPrg==TPrgExp)
		{
			Reccount = TPrg->Count();         // n. di records
			ReadRecs(CurRecord-CurRow);
			programTableRefresh = true;
		}
	}

	return 1;
}

//------------------ Funzioni assemblaggio normale ------------------------------------//
int PrgM_AssTot(void)
{
	GestAss(0,0);                    //assemblaggio totale
	return 1;
}

int PrgM_AssStep(void)
{
	GestAss(0,1);                  // gestione assemblaggio
	return 1;
}

int PrgM_AssRestart(void)
{
	GestAss(1,0);                  // Ripartenza programma
	return 1;
}

int PrgM_AssRestartStep(void)
{
	GestAss(1,1);                  // Ripartenza programma in modo passo-passo
	return 1;
}


//------------------ Funzioni assemblaggio con convogliatore --------------------------//

int MoveToStepPosition( int step )
{
	struct conv_data convVal;
	bool ret = true;

	Read_Conv( &convVal );

	if( !Get_UseConveyor() )
	{
		W_Mess( MsgGetString(Msg_00928) );
		return 0;
	}

	CPan* pan = new CPan( -1, 1, MsgGetString(Msg_00927) );
	switch( step )
	{
		case CONVEYOR_STEP1:
			ret = MoveConveyorAndWait( convVal.move1 );
		break;

		case CONVEYOR_STEP2:
			ret = MoveConveyorAndWait( convVal.move2 );
		break;

		case CONVEYOR_STEP3:
			ret = MoveConveyorAndWait( convVal.move3 );
		break;
	}

	delete pan;

	if( ret )
		return 1;
	else
		return 0;
}

int Refresh_Prg( int mode )
{
	char X_NomeFile[MAXNPATH];

	menuIdx=0;
	updateDosa=0;

	AFlag=0;

	lastCodCompFound = -1;

	Prg_CloseFiles();

	delete TPrgExp;
	delete TPrgNormal;
	delete TPrgDosa;

	PrgPath(X_NomeFile,QHeader.Prg_Default);
	TPrgNormal=new TPrgFile(X_NomeFile,PRG_NOADDPATH);

	PrgPath(X_NomeFile,QHeader.Prg_Default,PRG_ASSEMBLY);
	TPrgExp=new TPrgFile(X_NomeFile,PRG_NOADDPATH);

	PrgPath(X_NomeFile,QHeader.Prg_Default,PRG_DOSAT);
	TPrgDosa=new TPrgFile(X_NomeFile,PRG_NOADDPATH);

	TPrg=TPrgNormal;

	if( !Prg_OpenFiles( true ) )
	{
		return 0;
	}

	if( mode == PRG_DOSAT )
	{
		int retval=check_data(CHECKPRG_DISPFULL);

		if(retval)
		{
			if(!CreateDosaFile())
			{
				TPrg=TPrgNormal;
				return 0;
			}

			Prg_SwitchTable(PRG_DOSAT);

			if(TPrg!=TPrgDosa)
			{
				TPrg=TPrgNormal;
				return 0;
			}
		}
	}

	// visualizza dati sulle barre
	ShowCurrentData();

	// check file zeri
	ZerFile* fz = new ZerFile( QHeader.Prg_Default );
	if( !fz->IsOnDisk() )
	{
		fz->Create();
	}
	delete fz;

	TPrg = NULL;
	Prg_SwitchTable( mode );

	programWin->onRefresh();

	return 1;
}

int fn_LoadProgramFromConveyorStep( int step )
{
	struct conv_data convVal;
	char cli[9], prg[9], conf[9], lib[9];

	Read_Conv( &convVal );

	switch( step )
	{
		case CONVEYOR_STEP1:
			strcpy( cli, convVal.cust1 );
			strcpy( prg, convVal.prog1 );
			strcpy( conf, convVal.conf1 );
			strcpy( lib, convVal.lib1 );
		break;

		case CONVEYOR_STEP2:
			strcpy( cli, convVal.cust2 );
			strcpy( prg, convVal.prog2 );
			strcpy( conf, convVal.conf2 );
			strcpy( lib, convVal.lib2 );
		break;

		case CONVEYOR_STEP3:
			strcpy( cli, convVal.cust3 );
			strcpy( prg, convVal.prog3 );
			strcpy( conf, convVal.conf3 );
			strcpy( lib, convVal.lib3 );
		break;
	}

	if( strncmp( QHeader.Prg_Default, prg, 8 ) )
	{
		if( ComponentList != NULL )
		{
			delete [] ComponentList;
			ComponentList = NULL;
		}
		NComponent = 0;
		PlacedNComp = 0;

		ri_reset();
	}

	strcpy( QHeader.Cli_Default, cli );
	strcpy( QHeader.Prg_Default, prg );
	strcpy( QHeader.Conf_Default, conf );
	strcpy( QHeader.Lib_Default, lib );

	Mod_Cfg(QHeader);

	return 1;
}

int Conv_LoadStep1(void) 		// Caricamento rapido programma step 1
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	strncpyQ(tmp1,QHeader.Conf_Default,8);
	strncpyQ(tmp2,QHeader.Cli_Default,8);

	if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP1 ) )
	{
		if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
		{
			if(UpdateDBData(CARINT_UPDATE_FULL))
			{
				ConfImport(0);
			}
		}

		// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
		if( !Refresh_Prg( PRG_NORMAL ) )
		{
			strcpy( QHeader.Cli_Default, cli );
			strcpy( QHeader.Prg_Default, prg );
			strcpy( QHeader.Conf_Default, conf );
			strcpy( QHeader.Lib_Default, lib );

			Refresh_Prg( PRG_NORMAL );

			return 0;
		}
	}

	// Muovere convogliatore?
	if( W_Deci( 1, MsgGetString(Msg_00879) ) )
	{
		if( !MoveToStepPosition( CONVEYOR_STEP1 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}
	}

	return 1;
}

int Conv_LoadStep2(void) 		// Caricamento rapido programma step 2
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	strncpyQ(tmp1,QHeader.Conf_Default,8);
	strncpyQ(tmp2,QHeader.Cli_Default,8);

	if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP2 ) )
	{
		if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
		{
			if(UpdateDBData(CARINT_UPDATE_FULL))
			{
				ConfImport(0);
			}
		}

		// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
		if( !Refresh_Prg( PRG_NORMAL ) )
		{
			strcpy( QHeader.Cli_Default, cli );
			strcpy( QHeader.Prg_Default, prg );
			strcpy( QHeader.Conf_Default, conf );
			strcpy( QHeader.Lib_Default, lib );

			Refresh_Prg( PRG_NORMAL );

			return 0;
		}
	}

	// Muovere convogliatore?
	if( W_Deci( 1, MsgGetString(Msg_00879) ) )
	{
		if( !MoveToStepPosition( CONVEYOR_STEP2 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}
	}

	return 1;
}

int Conv_LoadStep3(void) 		// Caricamento rapido programma step 3
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	strncpyQ(tmp1,QHeader.Conf_Default,8);
	strncpyQ(tmp2,QHeader.Cli_Default,8);

	if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP3 ) )
	{
		if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
		{
			if(UpdateDBData(CARINT_UPDATE_FULL))
			{
				ConfImport(0);
			}
		}

		// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
		if( !Refresh_Prg( PRG_NORMAL ) )
		{
			strcpy( QHeader.Cli_Default, cli );
			strcpy( QHeader.Prg_Default, prg );
			strcpy( QHeader.Conf_Default, conf );
			strcpy( QHeader.Lib_Default, lib );

			Refresh_Prg( PRG_NORMAL );

			return 0;
		}
	}

	// Muovere convogliatore?
	if( W_Deci( 1, MsgGetString(Msg_00879) ) )
	{
		if( !MoveToStepPosition( CONVEYOR_STEP3 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}
	}

	return 1;
}

int Conv_AssTot(void) 		// Assemblaggio totale di tutti gli step attivi
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	struct conv_data convVal;

	Read_Conv( &convVal );

	if( !convVal.step1enabled && !convVal.step2enabled && !convVal.step3enabled )
	{
		W_Mess( MsgGetString(Msg_00885) );
		return 0;
	}

	// Assemblaggio totale step 1 (se abilitato)
	if( convVal.step1enabled )
	{
	// Salvataggio della conf attuale (da ripristinare se errori)
		strcpy( cli, QHeader.Cli_Default );
		strcpy( prg, QHeader.Prg_Default );
		strcpy( conf, QHeader.Conf_Default );
		strcpy( lib, QHeader.Lib_Default );

		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP1 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP1 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		if( !GestAss(0,0,-1,0) )
			return 0;
	}

	// Assemblaggio totale step 2 (se abilitato)
	if( convVal.step2enabled )
	{
		// Salvataggio della conf attuale (da ripristinare se errori)
		strcpy( cli, QHeader.Cli_Default );
		strcpy( prg, QHeader.Prg_Default );
		strcpy( conf, QHeader.Conf_Default );
		strcpy( lib, QHeader.Lib_Default );

		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP2 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP2 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		if( !GestAss(0,0,-1,0) )
			return 0;
	}

	// Assemblaggio totale step 3 (se abilitato)
	if( convVal.step3enabled )
	{
		// Salvataggio della conf attuale (da ripristinare se errori)
		strcpy( cli, QHeader.Cli_Default );
		strcpy( prg, QHeader.Prg_Default );
		strcpy( conf, QHeader.Conf_Default );
		strcpy( lib, QHeader.Lib_Default );

		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP3 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP3 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		if( !GestAss(0,0,-1,0) )
			return 0;
	}

	return 1;
}

int Conv_AssStep1(void)		// Assemblaggio solo step 1 (se attivo)
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	char mess[512];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	if( convVal.step1enabled )
	{
		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP1 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP1 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		GestAss(0,0,-1,1);
	}
	else
	{
		// Step non abilitato
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00875), 1 );
		W_Mess( mess );
	}

	return 1;
}

int Conv_AssStep2(void)		// Assemblaggio solo step 2 (se attivo)
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	char mess[512];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	if( convVal.step2enabled )
	{
		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP2 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP2 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		GestAss(0,0,-1,1);
	}
	else
	{
		// Step non abilitato
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00875), 2 );
		W_Mess( mess );
	}

	return 1;
}

int Conv_AssStep3(void)		// Assemblaggio solo step 3 (se attivo)
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	char mess[512];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	if( convVal.step3enabled )
	{
		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP3 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP3 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		GestAss(0,0,-1,1);
	}
	else
	{
		// Step non abilitato
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00875), 3 );
		W_Mess( mess );
	}

	return 1;
}

int Conv_DispTot(void) 		// Dosaggio totale di tutti gli step attivi
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	struct conv_data convVal;

	Read_Conv( &convVal );

	if( !convVal.step1enabled && !convVal.step2enabled && !convVal.step3enabled )
	{
		W_Mess( MsgGetString(Msg_00885) );
		return 0;
	}

	// Dosaggio totale step 1 (se abilitato)
	if( convVal.step1enabled )
	{
		// Salvataggio della conf attuale (da ripristinare se errori)
		strcpy( cli, QHeader.Cli_Default );
		strcpy( prg, QHeader.Prg_Default );
		strcpy( conf, QHeader.Conf_Default );
		strcpy( lib, QHeader.Lib_Default );

		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP1 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_DOSAT ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_DOSAT );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP1 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		if( !GestDos(0,0,-1,-1,0,1,1,0) )
			return 0;
	}

	// Dosaggio totale step 2 (se abilitato)
	if( convVal.step2enabled )
	{
		// Salvataggio della conf attuale (da ripristinare se errori)
		strcpy( cli, QHeader.Cli_Default );
		strcpy( prg, QHeader.Prg_Default );
		strcpy( conf, QHeader.Conf_Default );
		strcpy( lib, QHeader.Lib_Default );

		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP2 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_DOSAT ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_DOSAT );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP2 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		if( !GestDos(0,0,-1,-1,0,1,1,1) )
			return 0;
	}

	// Dosaggio totale step 3 (se abilitato)
	if( convVal.step3enabled )
	{
		// Salvataggio della conf attuale (da ripristinare se errori)
		strcpy( cli, QHeader.Cli_Default );
		strcpy( prg, QHeader.Prg_Default );
		strcpy( conf, QHeader.Conf_Default );
		strcpy( lib, QHeader.Lib_Default );

		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP3 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_DOSAT ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_DOSAT );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP3 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		if( !GestDos(0,0,-1,-1,0,1,1,1) )
			return 0;
	}

	return 1;
}

int Conv_DispStep1(void)		// Dosaggio solo step 1 (se attivo)
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	char mess[512];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	if( convVal.step1enabled )
	{
		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP1 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_DOSAT ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_DOSAT );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP1 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		GestDos(0,0,-1,-1,1,1,1,0);
	}
	else
	{
		// Step non abilitato
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00875), 1 );
		W_Mess( mess );
	}

	return 1;
}

int Conv_DispStep2(void)		// Dosaggio solo step 2 (se attivo)
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	char mess[512];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	if( convVal.step2enabled )
	{
		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP2 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_DOSAT ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_DOSAT );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP2 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		GestDos(0,0,-1,-1,1,1,1,0);
	}
	else
	{
		// Step non abilitato
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00875), 2 );
		W_Mess( mess );
	}

	return 1;
}

int Conv_DispStep3(void)		// Dosaggio solo step 3 (se attivo)
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	char mess[512];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	if( convVal.step3enabled )
	{
		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP3 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_DOSAT ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_DOSAT );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP3 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		GestDos(0,0,-1,-1,1,1,1,0);
	}
	else
	{
		// Step non abilitato
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00875), 3 );
		W_Mess( mess );
	}

	return 1;
}

int Conv_DispAssTot(void) 		// Dosaggio e Assemblaggio totale di tutti gli step attivi
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	struct conv_data convVal;

	Read_Conv( &convVal );

	if( !convVal.step1enabled && !convVal.step2enabled && !convVal.step3enabled )
	{
		W_Mess( MsgGetString(Msg_00885) );
		return 0;
	}

	// Dosaggio e Assemblaggio totale step 1 (se abilitato)
	if( convVal.step1enabled )
	{
		// Salvataggio della conf attuale (da ripristinare se errori)
		strcpy( cli, QHeader.Cli_Default );
		strcpy( prg, QHeader.Prg_Default );
		strcpy( conf, QHeader.Conf_Default );
		strcpy( lib, QHeader.Lib_Default );

		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP1 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP1 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		if( !GestDosAss(0,0,0,0) )
			return 0;
	}

	// Dosaggio e Assemblaggio totale step 2 (se abilitato)
	if( convVal.step2enabled )
	{
		// Salvataggio della conf attuale (da ripristinare se errori)
		strcpy( cli, QHeader.Cli_Default );
		strcpy( prg, QHeader.Prg_Default );
		strcpy( conf, QHeader.Conf_Default );
		strcpy( lib, QHeader.Lib_Default );

		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP2 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP2 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		if( !GestDosAss(0,0,0,1) )
			return 0;
	}

	// Dosaggio e Assemblaggio totale step 3 (se abilitato)
	if( convVal.step3enabled )
	{
		// Salvataggio della conf attuale (da ripristinare se errori)
		strcpy( cli, QHeader.Cli_Default );
		strcpy( prg, QHeader.Prg_Default );
		strcpy( conf, QHeader.Conf_Default );
		strcpy( lib, QHeader.Lib_Default );

		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP3 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP3 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		if( !GestDosAss(0,0,0,1) )
			return 0;
	}

	return 1;
}

int Conv_DispAssStep1(void)		// Dosaggio e Assemblaggio solo step 1 (se attivo)
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	char mess[512];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	if( convVal.step1enabled )
	{
		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP1 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP1 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		GestDosAss(0,0,1,0);
	}
	else
	{
		// Step non abilitato
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00875), 1 );
		W_Mess( mess );
	}

	return 1;
}

int Conv_DispAssStep2(void)		// Dosaggio e Assemblaggio solo step 2 (se attivo)
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	char mess[512];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	if( convVal.step2enabled )
	{
		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP2 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP2 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		GestDosAss(0,0,1,0);
	}
	else
	{
		// Step non abilitato
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00875), 2 );
		W_Mess( mess );
	}

	return 1;
}

int Conv_DispAssStep3(void)		// Dosaggio e Assemblaggio solo step 3 (se attivo)
{
	char tmp1[9],tmp2[9];
	char cli[9], prg[9], conf[9], lib[9];
	char mess[512];
	struct conv_data convVal;

	// Salvataggio della conf attuale (da ripristinare se errori)
	strcpy( cli, QHeader.Cli_Default );
	strcpy( prg, QHeader.Prg_Default );
	strcpy( conf, QHeader.Conf_Default );
	strcpy( lib, QHeader.Lib_Default );

	Read_Conv( &convVal );

	if( convVal.step3enabled )
	{
		strncpyQ(tmp1,QHeader.Conf_Default,8);
		strncpyQ(tmp2,QHeader.Cli_Default,8);

		if( fn_LoadProgramFromConveyorStep( CONVEYOR_STEP3 ) )
		{
			if((QHeader.modal & ENABLE_CARINT) && ((strcasecmpQ(QHeader.Conf_Default,tmp1)) || (strcasecmpQ(QHeader.Cli_Default,tmp2))))
			{
				if(UpdateDBData(CARINT_UPDATE_FULL))
				{
					ConfImport(0);
				}
			}

			// Aggiorna la tabella coi dati di programma dello step. Se problemi, ricarica il programma precedente
			if( !Refresh_Prg( PRG_ASSEMBLY ) )
			{
				strcpy( QHeader.Cli_Default, cli );
				strcpy( QHeader.Prg_Default, prg );
				strcpy( QHeader.Conf_Default, conf );
				strcpy( QHeader.Lib_Default, lib );

				Refresh_Prg( PRG_ASSEMBLY );

				return 0;
			}
		}

		// Convogliatore alla posizione richiesta
		if( !MoveToStepPosition( CONVEYOR_STEP3 ) )
		{
			W_Mess( MsgGetString(Msg_00876) );
			return 0;
		}

		GestDosAss(0,0,1,0);
	}
	else
	{
		// Step non abilitato
		snprintf( mess, sizeof(mess), MsgGetString(Msg_00875), 3 );
		W_Mess( mess );
	}

	return 1;
}

//---------------------------------------------------------------------------


void Prg_MenuCheckOrigin()
{
	Q_MenuZTheta=new GUI_SubMenu( 20, 13 );
	Q_MenuZTheta->Add(MENUZTHETA1,0,NULL,PrgM_CheckZThetaP1);
	Q_MenuZTheta->Add(MENUZTHETA2,0,NULL,PrgM_CheckZThetaP2);

	Q_MenuZPos=new GUI_SubMenu( 20, 14 );
	Q_MenuZPos->Add(MENUZPOS1,0,NULL,PrgM_CheckZPosP1);
	Q_MenuZPos->Add(MENUZPOS2,0,NULL,PrgM_CheckZPosP2);

	Q_MenuCheckOri=new GUI_SubMenu( 42, 12 );
	Q_MenuCheckOri->Add(MENUCHKORI1,0,NULL,PrgM_FindZerom);
	Q_MenuCheckOri->Add(MENUCHKORI2,0,Q_MenuZTheta,NULL);
	Q_MenuCheckOri->Add(MENUCHKORI3,0,Q_MenuZPos,NULL);
}

void Prg_MenuSSearch(void)
{
	Q_MenuSSearch = new GUI_SubMenu();
	Q_MenuSSearch->Add( MsgGetString(Msg_01777), K_F1, 0, NULL, PrgM_FindComp );
	Q_MenuSSearch->Add( MsgGetString(Msg_01778), K_F2, (QHeader.modal & ENABLE_CARINT) ? 0 : SM_GRAYED, NULL, PrgM_CarIntSearch );
}

void Prg_MenuAut()
{
	Q_MenuAut=new GUI_SubMenu( 25, 10 );   // Scelte in programmaz.
	Q_MenuAut->Add(MENUAUT1,0,NULL,PrgM_ZAuto);
	if( !Get_DemoMode() )
	{
		Q_MenuAut->Add(MENUAUT2,0,NULL,PrgM_ZAuto2);
		Q_MenuAut->Add(MENUAUT3,0,NULL,PrgM_RedefineZero);
		Q_MenuAut->Add(MENUAUT4,0,NULL,PrgM_ImgPar);
	}
	else
	{
		Q_MenuAut->Add(MENUAUT2,SM_GRAYED,NULL,PrgM_ZAuto2);
		Q_MenuAut->Add(MENUAUT3,SM_GRAYED,NULL,PrgM_RedefineZero);
		Q_MenuAut->Add(MENUAUT4,SM_GRAYED,NULL,PrgM_ImgPar);
	}
}

// Creazione menu di assemblaggio
void Prg_MenuAsse()
{
	Q_MenuAsse = new GUI_SubMenu(MENUASSEPOS);

	Q_MenuAsse->Add(MENUASSE2,0,NULL,PrgM_AssTot);
	Q_MenuAsse->Add(MENUASSE4,0,NULL,PrgM_AssStep);
	Q_MenuAsse->Add(MENUASSE5,0,NULL,PrgM_AssRestart);
	Q_MenuAsse->Add(MENUASSE6,0,NULL,PrgM_AssRestartStep);
}


// Creazione menu di riordino programma
void Prg_MenuRio()
{
	Q_SubMenuRio=new GUI_SubMenu(MENUSUBRIOPOS_NORM);
	Q_SubMenuRio->Add(MENUSUBRIONORM_1,0,NULL,PrgM_MenuSortOptNorm);
	Q_SubMenuRio->Add(MENUSUBRIONORM_2,0,NULL,PrgM_MenuSortOptNorm1);
	Q_SubMenuRio->Add(MENUSUBRIONORM_3,0,NULL,PrgM_MenuSortOptNorm2);
		
	Q_MenuRio[1] = new GUI_SubMenu(MENURIOPOS_ASSEM);
	Q_MenuRio[1]->Add(MENURIO0,0,NULL,PrgM_RioNote);
	Q_MenuRio[1]->Add(MENURIO1,0,NULL,PrgM_RioTipo);
	Q_MenuRio[1]->Add(MENURIO2,0,NULL,PrgM_RioCodComp);
	Q_MenuRio[1]->Add(MENURIO3,0,NULL,PrgM_RioCodCar);
	Q_MenuRio[1]->Add(MENURIO4,0,NULL,PrgM_RioPunta);
	Q_MenuRio[1]->Add(MENURIO6,0,NULL,PrgM_RioUge);
	Q_MenuRio[1]->Add(MENURIO7,0,NULL,PrgM_RioScheda);
	Q_MenuRio[1]->Add(MENURIO5,0,Q_SubMenuRio,NULL);

	Q_MenuRio[0] = new GUI_SubMenu(MENURIOPOS_MASTER);
	Q_MenuRio[0]->Add(MENURIO0,0,NULL,PrgM_RioNote);
	Q_MenuRio[0]->Add(MENURIO1,0,NULL,PrgM_RioTipo);
	Q_MenuRio[0]->Add(MENURIO2,0,NULL,PrgM_RioCodComp);
	Q_MenuRio[0]->Add(MENURIO3,0,NULL,PrgM_RioCodCar);
	Q_MenuRio[0]->Add(MENURIO4,0,NULL,PrgM_RioPunta);
	Q_MenuRio[0]->Add(MENURIO8,0,NULL,PrgM_RioXCoord);
}

// Creazione menu operaz. in tabella
//GF_14_07_2011
void Prg_MenuTab()
{
	Q_MenuMountAll = new GUI_SubMenu(MENUMOUNTPOS);
	Q_MenuMountAll->Add(MENUMOUNT1,0,NULL,PrgM_MountAll_Y);
	Q_MenuMountAll->Add(MENUMOUNT2,0,NULL,PrgM_MountAll_N);

	Q_MenuFiducials = new GUI_SubMenu(MENUFIDPOS);
	Q_MenuFiducials->Add(MENUFID1,0,NULL,PrgM_SetFiducial1);
	Q_MenuFiducials->Add(MENUFID2,0,NULL,PrgM_SetFiducial2);
	Q_MenuFiducials->Add(MENUFID3,0,NULL,PrgM_ResetFiducial);

	Q_MenuDispAll = new GUI_SubMenu(MENUDISPPOS);
	#ifndef __DISP2
	Q_MenuDispAll->Add(MENUDISP1,0,NULL,PrgM_DispAll_Y);
	Q_MenuDispAll->Add(MENUDISP2,0,NULL,PrgM_DispAll_N);
	#else
	Q_MenuDispAll->Add(MENUDISP1,0,NULL,PrgM_DispAll_1);
	Q_MenuDispAll->Add(MENUDISP2,0,NULL,PrgM_DispAll_2);
	Q_MenuDispAll->Add(MENUDISP3,0,NULL,PrgM_DispAll_A);
	Q_MenuDispAll->Add(MENUDISP4,0,NULL,PrgM_DispAll_N);
	#endif

	Q_MenuSplit = new GUI_SubMenu(MENUSPLITPOS);
	Q_MenuSplit->Add(MENUSPLIT1,0,NULL,PrgM_SplitSel);
	Q_MenuSplit->Add(MENUSPLIT2,0,NULL,PrgM_SplitX);

	Q_MenuChangeCoord = new GUI_SubMenu(MENUCOORDPOS);
	Q_MenuChangeCoord->Add(MENUCOORD1,0,NULL,PrgM_CoordX);
	Q_MenuChangeCoord->Add(MENUCOORD2,0,NULL,PrgM_CoordY);
}

// Creazione menu di dosaggio & assemblaggio - W3107
void Prg_MenuDas()
{
	#ifndef __DISP2
	Q_MenuDas = new GUI_SubMenu(MENUDASPOS);
	Q_MenuDas->Add(MENUDAS1,0,NULL,PrgM_DosAss);
	Q_MenuDas->Add(MENUDAS2,0,NULL,PrgM_DosAssStep);
	Q_MenuDas->Add(MENUDAS3,0,NULL,PrgM_DosAssRestart);
	Q_MenuDas->Add(MENUDAS4,0,NULL,PrgM_DosAssStepRestart);
	#else
	Q_MenuDas1 = new GUI_SubMenu(MENUDASPOS1);
	Q_MenuDas1->Add(MENUDAS1,0,NULL,PrgM_DosAss1);
	Q_MenuDas1->Add(MENUDAS2,0,NULL,PrgM_DosAssStep1);
	Q_MenuDas1->Add(MENUDAS3,0,NULL,PrgM_DosAssRestart1);
	Q_MenuDas1->Add(MENUDAS4,0,NULL,PrgM_DosAssStepRestart1);

	Q_MenuDas2 = new GUI_SubMenu(MENUDASPOS2);
	Q_MenuDas2->Add(MENUDAS1,0,NULL,PrgM_DosAss2);
	Q_MenuDas2->Add(MENUDAS2,0,NULL,PrgM_DosAssStep2);
	Q_MenuDas2->Add(MENUDAS3,0,NULL,PrgM_DosAssRestart2);
	Q_MenuDas2->Add(MENUDAS4,0,NULL,PrgM_DosAssStepRestart2);

	Q_MenuD1D2As = new GUI_SubMenu(MENUDASPOS_D1D2AS);
	Q_MenuD1D2As->Add(MENUDAS1,0,NULL,PrgM_Dos1Dos2Ass);
	Q_MenuD1D2As->Add(MENUDAS2,0,NULL,PrgM_Dos1Dos2AssStep);
	Q_MenuD1D2As->Add(MENUDAS3,0,NULL,PrgM_Dos1Dos2AssRestart);
	Q_MenuD1D2As->Add(MENUDAS4,0,NULL,PrgM_Dos1Dos2AssStepRestart);

	Q_MenuD1AsD2 = new GUI_SubMenu(MENUDASPOS_D1ASD2);
	Q_MenuD1AsD2->Add(MENUDAS1,0,NULL,PrgM_Dos1AssDos2);
	Q_MenuD1AsD2->Add(MENUDAS2,0,NULL,PrgM_Dos1AssDos2Step);
	Q_MenuD1AsD2->Add(MENUDAS3,0,NULL,PrgM_Dos1AssDos2Restart);
	Q_MenuD1AsD2->Add(MENUDAS4,0,NULL,PrgM_Dos1AssDos2StepRestart);

	Q_MenuDas = new GUI_SubMenu(MENUDASPOS);
	Q_MenuDas->Add(MENUDAS_D1ASS,0,Q_MenuDas1,NULL);
	Q_MenuDas->Add(MENUDAS_D2ASS,0,Q_MenuDas2,NULL);
	Q_MenuDas->Add(MENUDAS_D1D2ASS,0,Q_MenuD1D2As,NULL);
	Q_MenuDas->Add(MENUDAS_D1ASSD2,0,Q_MenuD1AsD2,NULL);
	
	#endif
	
	return;
} // MenuDas

// Creazione menu di dosaggio - W0298
void Prg_MenuDist()
{
	#ifndef __DISP2
	Q_MenuDist = new GUI_SubMenu(MENUDISTPOS);
	Q_MenuDist->Add( MsgGetString(Msg_00125), K_F1, 0, NULL, PrgM_DispParams ); // Distribution parameters
	Q_MenuDist->Add(MENUDIST1,0,NULL,PrgM_Dosat);
	Q_MenuDist->Add(MENUDIST2,0,NULL,PrgM_DosStep);
	Q_MenuDist->Add(MENUDIST3,0,NULL,PrgM_DosRestart);
	Q_MenuDist->Add(MENUDIST4,0,NULL,PrgM_DosStepRestart);
	#else
	Q_MenuDist1 = new GUI_SubMenu(MENUDISTPOS1);
	Q_MenuDist1->Add( MsgGetString(Msg_00125), K_F1, 0, NULL, PrgM_DispParams1 ); // Distribution parameters
	Q_MenuDist1->Add(MENUDIST1,0,NULL,PrgM_Dosat1);
	Q_MenuDist1->Add(MENUDIST2,0,NULL,PrgM_DosStep1);
	Q_MenuDist1->Add(MENUDIST3,0,NULL,PrgM_DosRestart1);
	Q_MenuDist1->Add(MENUDIST4,0,NULL,PrgM_DosStepRestart1);

	Q_MenuDist2 = new GUI_SubMenu(MENUDISTPOS2);
	Q_MenuDist2->Add( MsgGetString(Msg_00125), K_F1, 0, NULL, PrgM_DispParams2 ); // Distribution parameters
	Q_MenuDist2->Add(MENUDIST1,0,NULL,PrgM_Dosat2);
	Q_MenuDist2->Add(MENUDIST2,0,NULL,PrgM_DosStep2);
	Q_MenuDist2->Add(MENUDIST3,0,NULL,PrgM_DosRestart2);
	Q_MenuDist2->Add(MENUDIST4,0,NULL,PrgM_DosStepRestart2);

	Q_MenuDist = new GUI_SubMenu(MENUDISTPOS);
	Q_MenuDist->Add(MENUDIST_SELN1,0,Q_MenuDist1,NULL);
	Q_MenuDist->Add(MENUDIST_SELN2,0,Q_MenuDist2,NULL);
	#endif
}

void Prg_MenuCarSeq()
{
	Q_SubMenuCarZSeq = new GUI_SubMenu();
	Q_SubMenuCarZSeq->Add( MsgGetString(Msg_00731), K_F1, 0, NULL, PrgM_AutoCarZSeq_Auto ); // Automatico
	Q_SubMenuCarZSeq->Add( MsgGetString(Msg_00730), K_F2, 0, NULL, PrgM_AutoCarZSeq_Man );  // Manuale

	Q_MenuCarSeq = new GUI_SubMenu(42,17);
	Q_MenuCarSeq->Add( MsgGetString(Msg_00368), K_F5, 0, NULL, PrgM_AutoCarSeq );
	Q_MenuCarSeq->Add( MsgGetString(Msg_02103), K_F6, 0, Q_SubMenuCarZSeq, NULL );
}



bool Prg_onVersion()
{
	if( TPrg == TPrgNormal )
	{
		ATab[CurRow].status ^= VERSION_MASK;
		ATab[CurRow].Changed |= VERSION_FIELD;
		UpdateAssemblyRec(ATab[CurRow],ATab[CurRow]); //update in espanso
		ATab[CurRow].Changed &= ~VERSION_FIELD;
		TPrgNormal->Write(ATab[CurRow],CurRecord);
		programTableRefresh = true;
		return true;
	}
	return false;
}


//--------------------------------------------------------------------------
//* Funzioni di visualizzazione e gestione tabella

bool Prg_IncRow()
{
	int oldriga=ATab[CurRow].Riga;
	struct TabPrg tabDat;
	int found = 0;

	if(AFlag || Reccount==0)
	{
		bipbip();
		return false;
	}

	// Cerca eventuali componenti con codice nullo
	for(int r_loop=0;r_loop<Reccount;r_loop++)
	{
		TPrg->Read(tabDat,r_loop);
		if( tabDat.CodCom[0] == 0 )
		{
			found = 1;
		}
	}


	CurRow++;
	CurRecord++;

	//if( (CurRecord==Reccount) && (ATab[CurRow-1].CodCom[0]!=0) )
	if( (CurRecord==Reccount) && !found )
	{
		if(TPrg==TPrgExp || TPrg==TPrgDosa)
		{
			CurRecord=Reccount-1;
			CurRow--;
		}
		else
		{
			AFlag=1;
		}
	}
	else
	{
		//if( (ATab[CurRow-1].CodCom[0]==0) && (CurRecord==Reccount) )
		if( found && (CurRecord==Reccount) )
		{
			CurRecord=Reccount-1;
			CurRow--;
			W_Mess(INVALID_CODE);
		}
	}

	if(CurRow==MAXRECS)
	{
		CurRow=MAXRECS-1;
		DatScroll(0);
		if(!AFlag)
		{
			TPrg->Read(ATab[MAXRECS-1],CurRecord); // legge un nuovo records
			programTableRefresh = true;
		}
	}
	if(AFlag)
	{
		Prg_InitAll(CurRow,oldriga+1);
		ATab[CurRow].pack_txt[0]=0;
		ATab[CurRow].Riga=oldriga+1;
		programTableRefresh = true;
	}

	return true;
}

void Prg_DecRow(void)
{
	if(AFlag && TPrg==TPrgNormal)
	{
		ATab[CurRow].Riga=0;
		programTableRefresh = true;
		AFlag=0;
	}
	CurRecord--;
	CurRow--;
	if(CurRecord<0)
	{
		CurRecord=0;
		CurRow++;
		return;
	}
	if(CurRow<0)
	{
		CurRow=0;
		DatScroll(1);
		TPrg->Read(ATab[0],CurRecord); // legge un nuovo records
		programTableRefresh = true;
	}
}

void Prg_CtrlPGDown()
{
	if( AFlag || CurRecord == (Reccount-1) || Reccount == 0 )
	{
		return;
	}

	CurRecord=Reccount-1;
	CurRow=(CurRecord % (MAXRECS-1));

	Prg_ClearAll();
	ReadRecs(CurRecord-CurRow);

	programTableRefresh = true;
}

void Prg_CtrlPGUp()
{
	if(AFlag)
	{
		ATab[CurRow].Riga=0;
		programTableRefresh = true;
		AFlag=0;
	}
	if( CurRecord == 0 )
	{
		return;
	}
	CurRecord=0;
	CurRow=0;

	Prg_ClearAll();
	ReadRecs(CurRecord-CurRow);

	programTableRefresh = true;
}

void Prg_PGDown()
{
	if( AFlag || CurRecord == (Reccount-1) || Reccount == 0 )
	{
		return;
	}

	if(CurRecord+(MAXRECS-1)>=Reccount)
	{
		CurRecord=Reccount-1;
		CurRow=(CurRecord % (MAXRECS-1));
	}
	else
		CurRecord=CurRecord+(MAXRECS-1);

	Prg_ClearAll();
	ReadRecs(CurRecord-CurRow);

	programTableRefresh = true;
}

void Prg_PGUp()
{
	if(AFlag)
	{
		ATab[CurRow].Riga=0;
		programTableRefresh = true;
		AFlag=0;
	}
	if(CurRecord==0)
	{
		return;
	}
	if(CurRecord-(MAXRECS-1)<0)
	{
		CurRecord=0;
		CurRow=0;
	}
	else
	CurRecord=CurRecord-(MAXRECS-1);

	if(CurRecord-CurRow<0)
	{
		CurRecord=0;
		CurRow=0;
	}

	Prg_ClearAll();
	ReadRecs(CurRecord-CurRow);

	programTableRefresh = true;
}


// Controlla i valori di cella e controlla la validita' degli inputs
// Ritorna 1 se il campo e' stato modificato con successo.
int test( int row, int col, char* ValoreStr )
{
	int Modific = 0;
	char old_value[6];
	int ok_code, s_scan;

	struct TabPrg tmptab;
	memcpy(&tmptab,ATab+row,sizeof(tmptab));

	switch( col )
	{
		//codice componente (solo per programma master)
		case ProgramTableUI::COL_CODE:
			if(chk_tipo(row) && TPrg!=TPrgExp)
			{
				strcpy(ATab[row].CodCom,strupr(ValoreStr));
				if(TPrg->Search(0,ATab[row],PRG_CODSEARCH,ATab[row].scheda)==-1)
				{
					if(strcasecmpQ(tmptab.CodCom,strupr(ValoreStr))!=0)
					{
						ATab[row].Changed|=COD_FIELD;
					}

					DelSpcR(ATab[row].CodCom);
					Modific=1;
				}
				else
				{
					Modific=0;
				}
			}
			else
			{
				Modific=0;
			}
			break;

		case ProgramTableUI::COL_COMP:
		//tipo componente (solo per programma master)
			s_scan=0;
			ok_code=0;
			for(s_scan=0;s_scan<25;s_scan++)
			{
				if(ValoreStr[s_scan]==0)
				{
					break;
				}

				if(ValoreStr[s_scan]!=' ')
				{
					ok_code=1;
					break;
				}
			}

			if(ok_code && TPrg!=TPrgExp)
			{
				if(strcasecmpQ(ATab[row].TipCom,ValoreStr)!=0)
				{
					ATab[row].Changed|=TIPO_FIELD;
					strcpy(ATab[row].TipCom,ValoreStr);
				}

				Modific=1;

			}
			else
			{
				Modific=0;
			}
			break;

		case ProgramTableUI::COL_ROT:
			//rotazione
			if(chk_tipo(row))
			{
				Modific=1;
				float ValoreFloat = atof( ValoreStr );

				if(ValoreFloat>=(float)360)
				{
					ValoreFloat=(float)360;
				}
				if(ValoreFloat<=(float)-360)
				{
					ValoreFloat=(float)-360;
				}

				if(ATab[row].Rotaz!=ValoreFloat)
				{
					ATab[row].Changed|=ROT_FIELD;
					ATab[row].Rotaz=ValoreFloat;
				}
			}
			break;

		case ProgramTableUI::COL_NOZZLE:
			//punta
			if(chk_tipo(row))
			{
				*old_value=ATab[row].Punta;
				ATab[row].Punta=*strupr(ValoreStr);

				if(ATab[row].Punta<'1' || ATab[row].Punta>'2')
				{
					Modific=0;
				}
				else
				{
					if(ATab[row].Punta!=*old_value)
					{
						ATab[row].Changed|=PUNTA_FIELD;
					}
					Modific=1;
				}

				if(!Modific)
				{
					ATab[row].Punta=*old_value;
				}
			}
			break;

		case ProgramTableUI::COL_ASSEM:
			//monta si/no
			if( TPrg==TPrgNormal && (chk_tipo(row) && (!(ATab[row].status & NOMNTBRD_MASK))) )
			{
				Modific = 1;

				if( strcmp( ValoreStr, SHORT_NO ) != 0 )
				{
					ATab[row].status |= MOUNT_MASK;
				}
				else
				{
					ATab[row].status &= ~MOUNT_MASK;
				}

				if(ATab[row].status!=tmptab.status)
				{
					ATab[row].Changed |= MOUNT_FIELD;
				}
			}
			break;

		case ProgramTableUI::COL_DISP:
			//dosa si/no
			if(TPrg==TPrgNormal && (chk_tipo(row) && (!(ATab[row].status & NOMNTBRD_MASK))))
			{
				Modific = 1;

				#ifndef __DISP2
				if( strcmp( ValoreStr, SHORT_NO ) != 0 ) // strings are different
				{
					ATab[row].status |= DODOSA_MASK;
				}
				else
				{
					ATab[row].status &= ~DODOSA_MASK;
				}
				#else
				ATab[row].status&=~(DODOSA_MASK | DODOSA2_MASK);

				if( strcmp( ValoreStr, SHORT_NO ) != 0 ) // strings are different
				{
					if(!strcmp(ValoreStr,DOSA_ALL_TXT))
					{
						ATab[row].status|=(DODOSA_MASK | DODOSA2_MASK);
					}
					else
					{
						switch(toupper(ValoreStr[0]))
						{
							case '1':
								ATab[row].status|=DODOSA_MASK;
								break;
							case '2':
								ATab[row].status|=DODOSA2_MASK;
								break;
						}
					}
				}
				#endif

				if(ATab[row].status!=tmptab.status)
				{
					ATab[row].Changed|=DOSA_FIELD;
				}
			}
			break;

		case ProgramTableUI::COL_X:
			//coordinata X
			if(chk_tipo(row))
			{
				Modific=1;
				float ValoreFloat = atof( ValoreStr );
				if(ValoreFloat>QParam.LX_maxcl)
				{
					ValoreFloat=QParam.LX_maxcl;
				}
				if(ValoreFloat<-QParam.LX_maxcl)
				{
					ValoreFloat=-QParam.LX_maxcl;
				}

				if(ATab[row].XMon!=ValoreFloat)
				{
					ATab[row].Changed|=(PX_FIELD|PY_FIELD);
					ATab[row].XMon=ValoreFloat;
				}
			}
			break;

		case ProgramTableUI::COL_Y:
			//coordinata Y
			if(chk_tipo(row))
			{
				Modific=1;
				float ValoreFloat = atof( ValoreStr );
				if(ValoreFloat>QParam.LY_maxcl)
				{
					ValoreFloat=QParam.LY_maxcl;
				}

				if(ValoreFloat<-QParam.LY_maxcl)
				{
					ValoreFloat=-QParam.LY_maxcl;
				}

				if(ATab[row].YMon!=ValoreFloat)
				{
					ATab[row].Changed|=(PX_FIELD|PY_FIELD);
					ATab[row].YMon=ValoreFloat;
				}
			}
	 		break;

		case ProgramTableUI::COL_FEEDER:
			//numero caricatore
			Modific=0;
			break;

		case ProgramTableUI::COL_NOTES:
			//note
			if(chk_tipo(row))
			{
				Modific=1;
				if(strcasecmpQ(ATab[row].NoteC,ValoreStr)!=0)
				{
					strcpy(ATab[row].NoteC,ValoreStr);
					ATab[row].Changed|=NOTE_FIELD;
				}
			}
			else
			{
				Modific=0;
			}
			break;
	}

	if(!Modific)
	{
		memcpy(ATab+row,&tmptab,sizeof(tmptab));
	}

	return(Modific);
}


//---------------------------------------------------------------------------
// finestra: Nozzle rotation adjustment
//---------------------------------------------------------------------------
#define PROG_TAB_Y     4

ProgramTableUI::ProgramTableUI() : CWindowTable( 0 )
{
	SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_EDIT );
	SetClientAreaPos( 0, 3 );
	SetClientAreaSize( 143, PROG_TAB_Y + MAXRECS + 3 );

	prevTPrg = 0;
	m_start_item = ATab[CurRow].Riga;

	SM_TableOp = new GUI_SubMenu();
	SM_TableOp->Add( MsgGetString(Msg_05176), K_F3, 0, Q_MenuFiducials, NULL );
	SM_TableOp->Add( MsgGetString(Msg_05192), K_F4, 0, Q_MenuChangeCoord, NULL );
	SM_TableOp->Add( MsgGetString(Msg_05169), K_F7, 0, Q_MenuMountAll, NULL );
	SM_TableOp->Add( MsgGetString(Msg_05170), K_F8, 0, Q_MenuDispAll, NULL );
	SM_TableOp->Add( MsgGetString(Msg_05185), K_F9, 0, Q_MenuSplit, NULL );
	SM_TableOp->Add( MsgGetString(Msg_00383), K_F10, 0, NULL, boost::bind( &ProgramTableUI::onCopyField, this ) );
	SM_TableOp->Add( MsgGetString(Msg_00384), K_F11, 0, NULL, Prg_DupRec );
	SM_TableOp->Add( MsgGetString(Msg_00385), K_F12, 0, NULL, Prg_MoveRec );

	SM_ConvAss = new GUI_SubMenu(); // sub menu assembla
	SM_ConvAss->Add( MsgGetString(Msg_00386), K_F8, 0, NULL, Conv_AssTot );
	SM_ConvAss->Add( MsgGetString(Msg_00892), K_F9, 0, NULL, Conv_AssStep1 );
	SM_ConvAss->Add( MsgGetString(Msg_00893), K_F10, 0, NULL, Conv_AssStep2 );
	SM_ConvAss->Add( MsgGetString(Msg_00894), K_F11, 0, NULL, Conv_AssStep3 );

	SM_ConvDisp = new GUI_SubMenu(); // sub menu dosa
	SM_ConvDisp->Add( MsgGetString(Msg_00386), K_F8, 0, NULL, Conv_DispTot );
	SM_ConvDisp->Add( MsgGetString(Msg_00892), K_F9, 0, NULL, Conv_DispStep1 );
	SM_ConvDisp->Add( MsgGetString(Msg_00893), K_F10, 0, NULL, Conv_DispStep2 );
	SM_ConvDisp->Add( MsgGetString(Msg_00894), K_F11, 0, NULL, Conv_DispStep3 );

	SM_ConvDispAss = new GUI_SubMenu(); // sub menu dosa e assembla
	SM_ConvDispAss->Add( MsgGetString(Msg_00386), K_F8, 0, NULL, Conv_DispAssTot );
	SM_ConvDispAss->Add( MsgGetString(Msg_00892), K_F9, 0, NULL, Conv_DispAssStep1 );
	SM_ConvDispAss->Add( MsgGetString(Msg_00893), K_F10, 0, NULL, Conv_DispAssStep2 );
	SM_ConvDispAss->Add( MsgGetString(Msg_00894), K_F11, 0, NULL, Conv_DispAssStep3 );

	SM_ConvQL = new GUI_SubMenu(); // sub menu caricamento rapido step
	SM_ConvQL->Add( MsgGetString(Msg_00892), K_F1, 0, NULL, Conv_LoadStep1 );
	SM_ConvQL->Add( MsgGetString(Msg_00893), K_F2, 0, NULL, Conv_LoadStep2 );
	SM_ConvQL->Add( MsgGetString(Msg_00894), K_F3, 0, NULL, Conv_LoadStep3 );

	SM_MenuConveyor = new GUI_SubMenu();
	if( Get_ConveyorEnabled() )
		SM_MenuConveyor->Add( MsgGetString(Msg_00948), K_F1, 0, NULL, boost::bind( &ProgramTableUI::onConveyorWizard, this ) );
	else
		SM_MenuConveyor->Add( MsgGetString(Msg_00948), K_F1, SM_GRAYED, NULL, boost::bind( &ProgramTableUI::onConveyorWizard, this ) );
	SM_MenuConveyor->Add( MsgGetString(Msg_00899), K_F2, 0, NULL, boost::bind( &ProgramTableUI::onConveyorData, this ) );
	SM_MenuConveyor->Add( MsgGetString(Msg_00880), K_F3, 0, SM_ConvQL, NULL );
	if( Get_ConveyorEnabled() )
		SM_MenuConveyor->Add( MsgGetString(Msg_00352), K_F7, 0, SM_ConvAss, NULL );
	else
		SM_MenuConveyor->Add( MsgGetString(Msg_00352), K_F7, SM_GRAYED, SM_ConvAss, NULL );
	if( Get_ConveyorEnabled() )
		SM_MenuConveyor->Add( MsgGetString(Msg_00638), K_F8, 0, SM_ConvDisp, NULL );
	else
		SM_MenuConveyor->Add( MsgGetString(Msg_00638), K_F8, SM_GRAYED, SM_ConvDisp, NULL );
	if( Get_ConveyorEnabled() )
		SM_MenuConveyor->Add( MsgGetString(Msg_00677), K_F9, 0, SM_ConvDispAss, NULL );
	else
		SM_MenuConveyor->Add( MsgGetString(Msg_00677), K_F9, SM_GRAYED, SM_ConvDispAss, NULL );

}

ProgramTableUI::~ProgramTableUI()
{
	// delete combos
	for( unsigned int i = 0; i < m_combos.size(); i++ )
	{
		if( m_combos[i] )
		{
			delete m_combos[i];
		}
	}

	delete SM_TableOp;
	delete SM_ConvAss;
	delete SM_ConvDisp;
	delete SM_ConvDispAss;
	delete SM_ConvQL;
	delete SM_MenuConveyor;
}

void ProgramTableUI::onInit()
{
	// create combos
	m_combos[NUM_BOARDS] = new C_Combo(  3, 1, MsgGetString(Msg_01234), 8, CELL_TYPE_UINT );
	m_combos[NUM_COMPS] = new C_Combo( 28, 1, MsgGetString(Msg_01235), 8, CELL_TYPE_UINT );

	// create table
	m_table = new CTable( 3, PROG_TAB_Y, MAXRECS, TABLE_STYLE_HIGHLIGHT_ROW, this );

	// add columns
	m_table->AddCol( MsgGetString(Msg_00177),  5, CELL_TYPE_UINT, CELL_STYLE_READONLY ); //riga
	m_table->AddCol( MsgGetString(Msg_00523), 16, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE ); //codice
	m_table->AddCol( MsgGetString(Msg_00519),  3, CELL_TYPE_UINT, CELL_STYLE_READONLY ); //scheda
	m_table->AddCol( MsgGetString(Msg_00524), 25, CELL_TYPE_TEXT ); //tipo componente
	m_table->AddCol( MsgGetString(Msg_00678), 20, CELL_TYPE_TEXT, CELL_STYLE_READONLY ); //package
	m_table->AddCol( MsgGetString(Msg_00526),  7, CELL_TYPE_SDEC ); //rotazione
	m_table->AddCol( MsgGetString(Msg_00527),  1, CELL_TYPE_TEXT ); //punta
	m_table->AddCol( MsgGetString(Msg_00531),  1, CELL_TYPE_YN ); //monta
	m_table->AddCol( MsgGetString(Msg_00522),  1, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE ); //dosa
	m_table->AddCol( "X",  7, CELL_TYPE_SDEC ); //x
	m_table->AddCol( "Y",  7, CELL_TYPE_SDEC ); //y
	m_table->AddCol( MsgGetString(Msg_00529),  3, CELL_TYPE_SINT, CELL_STYLE_READONLY ); //caricatore
	m_table->AddCol( MsgGetString(Msg_00060), 29, CELL_TYPE_TEXT ); //note

	// set params
	m_table->SetColLegalChars( COL_CODE, CHARSET_TEXT_NO_SPACE ); //codice
	m_table->SetColLegalChars( COL_NOZZLE, "12" ); //punta
	#ifndef __DISP2
	char buf[10];
	strcpy( buf, SHORT_NO );
	strcat( buf, SHORT_YES );
	buf[2] = 0;
	m_table->SetColLegalChars( COL_DISP, buf ); //dosa
	#else
	char buf[10];
	strcpy( buf, SHORT_NO );
	strcat( buf, "12" );
	strcat( buf, DOSA_ALL_TXT );
	buf[4] = 0;
	m_table->SetColLegalChars( COL_DISP, buf ); //dosa
	#endif
	m_table->SetColMinMax( COL_ROT, -360.f, 360.f ); //rotazione
	m_table->SetColMinMax( COL_X, -QParam.LX_maxcl, QParam.LX_maxcl ); //x
	m_table->SetColMinMax( COL_Y, -QParam.LX_maxcl, QParam.LX_maxcl ); //y
}

void ProgramTableUI::onShow()
{
	// show combos
	for( unsigned int i = 0; i < m_combos.size(); i++ )
	{
		m_combos[i]->Show( GetX(), GetY() );
	}

	// show table
	forceRefresh();

	m_start_item = ATab[0].Riga;
}

void ProgramTableUI::onRefresh()
{
	if( programTableRefresh )
	{
		programTableRefresh = false;
		forceRefresh();
	}
}

void ProgramTableUI::onEdit()
{
	char str[64];
	struct TabPrg tmpRec;

	int col = m_table->GetCurCol();
	snprintf( str, 64, "%s", m_table->GetText( CurRow, col ) );

	if( test( CurRow, col, str ) )
	{
		TPrg->Read( tmpRec, CurRecord ); //get del contenuto attuale

		if( TPrg == TPrgNormal )
		{
			if(AFlag || (CurRecord==0 && Reccount==0))  //se nuovo record
			{
				//check validita' e univocita' del codice componente, prima di aggiungere la riga
				if(AFlag && ((ATab[CurRow].CodCom[0]=='\0') || (TPrg->Search(0,ATab[CurRow],PRG_CODSEARCH,ATab[CurRow].scheda)!=-1)))
				{
					Prg_InitAll(CurRow,ATab[CurRow].Riga);
					programTableRefresh = true;
					bipbip();
					return;
				}

				AddAssemblyRec(ATab[CurRow]);
				AFlag=0;
				ATab[CurRow].Changed=0;
				Reccount++;
			}
			else
			{
				if(ATab[CurRow].Changed)   //se cambiamento riscontrato
				{
					prgupdateAskFlag = 1;

					UpdateAssemblyRec( tmpRec, ATab[CurRow] );   //update espanso

					ATab[CurRow].Changed=0;
				}
			}

			TPrg->Write(ATab[CurRow],CurRecord);
		}
		else
		{
			TPrgExp->Write(ATab[CurRow],CurRecord);
			updateDosa = 1;
		}

		ReadRecs(CurRecord-CurRow);

		TPrg->WriteLast(CurRecord);
	}

	programTableRefresh = true;
}

void ProgramTableUI::onShowMenu()
{
	//TODO: i sottomenu vanno inseriti nella classe

	if( TPrg != TPrgDosa )
	{
		m_menu->Add( MsgGetString(Msg_00124), K_F1, 0, NULL, boost::bind( &ProgramTableUI::onProgramData, this ) ); // Program data
		m_menu->Add( MsgGetString(Msg_00247), K_F2, 0, NULL, boost::bind( &ProgramTableUI::onEditOverride, this ) ); // edit
		m_menu->Add( MsgGetString(Msg_00348), K_F3, 0, Q_MenuAut, NULL ); // board reference
		m_menu->Add( MsgGetString(Msg_00349), K_F4, 0, NULL, PrgM_AutoComp ); // placement pos teaching
		m_menu->Add( MsgGetString(Msg_00350), K_F5, 0, NULL, PrgM_Caric ); // feeder config
		m_menu->Add( MsgGetString(Msg_00351), K_F6, 0, NULL, PrgM_Zeri ); // panel composition
		m_menu->Add( MsgGetString(Msg_00352), K_F7, 0, Q_MenuAsse, NULL ); // assembling
		m_menu->Add( MsgGetString(Msg_00353), K_F8, 0, Q_MenuRio[menuIdx], NULL ); //
		m_menu->Add( MsgGetString(Msg_00354), K_F9, (TPrg == TPrgNormal) ? 0 : SM_GRAYED, SM_TableOp, NULL ); // table operation
		m_menu->Add( MsgGetString(Msg_00638), K_F10, QParam.Dispenser ? 0 : SM_GRAYED, Q_MenuDist, NULL ); // dispensing
		m_menu->Add( MsgGetString(Msg_01142), K_F11, 0, NULL, PrgM_Pkg ); // packages library
		m_menu->Add( MsgGetString(Msg_01176), K_F12, 0, NULL, boost::bind( &ProgramTableUI::onTableSwitch, this ) ); // Master/Mounting
		m_menu->Add( MsgGetString(Msg_01553), K_SHIFT_F1, 0, NULL, PrgCaricGraph ); // usage graph
		m_menu->Add( MsgGetString(Msg_00112), K_SHIFT_F2, 0, Q_MenuCheckOri, NULL ); // check zeroes
		m_menu->Add( MsgGetString(Msg_00398), K_SHIFT_F3, 0, Q_MenuSSearch, NULL ); // find component
		m_menu->Add( MsgGetString(Msg_00358), K_SHIFT_F4, 0, NULL, PrgM_appCI ); // IC placement pos teaching
		m_menu->Add( MsgGetString(Msg_00662), K_SHIFT_F5, 0, Q_MenuCarSeq, NULL ); // feeder pos seq teaching
		m_menu->Add( MsgGetString(Msg_00360), K_SHIFT_F6, 0, NULL, PrgM_AutoCompSeq ); // placement pos seq teaching
		m_menu->Add( MsgGetString(Msg_00361), K_SHIFT_F7, 0, NULL, boost::bind( &ProgramTableUI::onCongruenceCheck, this ) ); // congruence check
		m_menu->Add( MsgGetString(Msg_01302), K_SHIFT_F8, (QHeader.modal & ENABLE_CARINT) ? 0 : SM_GRAYED, NULL, PrgM_CarInt ); // feeder packs database
		m_menu->Add( MsgGetString(Msg_00245), K_SHIFT_F9, 0, NULL, PrgM_SelectVers ); // select prog version
		m_menu->Add( MsgGetString(Msg_00677), K_SHIFT_F10, QParam.Dispenser ? 0 : SM_GRAYED, Q_MenuDas, NULL ); // dispensing & assembling
		m_menu->Add( MsgGetString(Msg_01212), K_SHIFT_F11, QParam.Dispenser ? 0 : SM_GRAYED, NULL, boost::bind( &ProgramTableUI::onTableSwitch_Dispenser, this ) ); // dispensing table
		m_menu->Add( MsgGetString(Msg_00949), K_SHIFT_F12, 0, SM_MenuConveyor, NULL ); // Conveyor data
	}
	else
	{
		m_menu->Add( MsgGetString(Msg_00638), K_F10, QParam.Dispenser ? 0 : SM_GRAYED, Q_MenuDist, NULL ); // dispensing
		m_menu->Add( MsgGetString(Msg_01142), K_F11, 0, NULL, PrgM_Pkg ); // packages library
		m_menu->Add( MsgGetString(Msg_01176), K_F12, 0, NULL, boost::bind( &ProgramTableUI::onTableSwitch, this ) ); // Master/Mounting
		m_menu->Add( MsgGetString(Msg_00112), K_SHIFT_F2, 0, Q_MenuCheckOri, NULL ); // check zeroes
		m_menu->Add( MsgGetString(Msg_00398), K_SHIFT_F3, 0, Q_MenuSSearch, NULL ); // find component
		m_menu->Add( MsgGetString(Msg_00677), K_SHIFT_F10, QParam.Dispenser ? 0 : SM_GRAYED, Q_MenuDas, NULL ); // dispensing & assembling
	}
}

bool ProgramTableUI::onKeyPress( int key )
{
	switch( key )
	{
		case K_F1:
			onProgramData();
			return true;

		case K_F2:
			onEditOverride();
			return true;

		case K_F3:
			if( TPrg != TPrgDosa )
			{
				Q_MenuAut->Show();
				return true;
			}
			break;

		case K_F4:
			if( TPrg != TPrgDosa )
			{
				PrgM_AutoComp();
				return true;
			}
			break;

		case K_F5:
			if( TPrg != TPrgDosa )
			{
				PrgM_Caric();
				return true;
			}
			break;

		case K_F6:
			if( TPrg != TPrgDosa )
			{
				PrgM_Zeri();
				return true;
			}
			break;

		case K_F7:
			if( TPrg != TPrgDosa )
			{
				Q_MenuAsse->Show();
				return true;
			}
			break;

		case K_F8:
			if( TPrg != TPrgDosa )
			{
				Q_MenuRio[menuIdx]->Show();
				return true;
			}
			break;

		case K_F9:
			if( TPrg == TPrgNormal )
			{
				SM_TableOp->Show();
				return true;
			}
			break;

		case K_F10:
			if( QParam.Dispenser )
			{
				Q_MenuDist->Show();
				return true;
			}
			break;

		case K_F11:
			PrgM_Pkg();
			return true;

		case K_F12:
			onTableSwitch();
			return true;

		case K_SHIFT_F1:
			if( TPrg != TPrgDosa )
			{
				PrgCaricGraph();
				return true;
			}
			break;

		case K_SHIFT_F2:
			Q_MenuCheckOri->Show();
			return true;

		case K_SHIFT_F3:
			Q_MenuSSearch->Show();
			return true;

		case K_SHIFT_F4:
			if( TPrg != TPrgDosa )
			{
				PrgM_appCI();
				return true;
			}
			break;

		case K_SHIFT_F5:
			if( TPrg != TPrgDosa )
			{
				Q_MenuCarSeq->Show();
				return true;
			}
			break;

		case K_SHIFT_F6:
			if( TPrg != TPrgDosa )
			{
				PrgM_AutoCompSeq();
				return true;
			}
			break;

		case K_SHIFT_F7:
			if( TPrg != TPrgDosa )
			{
				onCongruenceCheck();
				return true;
			}
			break;

		case K_SHIFT_F8:
			if( TPrg != TPrgDosa )
			{
				if( QHeader.modal & ENABLE_CARINT )
				{
					PrgM_CarInt();
					return true;
				}
			}
			break;

		case K_SHIFT_F9:
			if( TPrg != TPrgDosa )
			{
				PrgM_SelectVers();
				return true;
			}
			break;

		case K_SHIFT_F10:
			if( QParam.Dispenser )
			{
				Q_MenuDas->Show();
				return true;
			}
			break;

		case K_SHIFT_F11:
			if( TPrg != TPrgDosa )
			{
				if( QParam.Dispenser )
				{
					onTableSwitch_Dispenser();
					return true;
				}
			}
			break;

		case K_SHIFT_F12:
			SM_MenuConveyor->Show();
			return true;

		case K_ENTER:
			if( Prg_onEnter() )
			{
				showRow( CurRow );
				return true;
			}
			break;

		case K_ALT_V:
			if( Prg_onVersion() )
			{
				showRow( CurRow );
				return true;
			}
			break;

		case K_DEL:
			if( TPrg == TPrgNormal && guiDeskTop->GetEditMode() )
			{
				Prg_Delete();
				m_table->Select( CurRow, m_table->GetCurCol() );
				return true;
			}
			break;

		case K_CTRL_DEL:
			if( TPrg == TPrgNormal && guiDeskTop->GetEditMode() )
			{
				Prg_DelBlk();
				forceRefresh();
				m_table->Select( CurRow, m_table->GetCurCol() );
				return true;
			}
			break;

		case K_CTRL_HOME: // posizionamento rapido zero macchina
			HeadHomeMov();
			return true;

		case K_CTRL_END: // posizionamento rapido fine scheda
			HeadEndMov();
			return true;

		case K_DOWN:
		case K_UP:
		case K_PAGEDOWN:
		case K_PAGEUP:
		case K_CTRL_PAGEDOWN:
		case K_CTRL_PAGEUP:
			return vSelect( key );

		case K_END:
			m_table->Select( CurRow, COL_TOT-1 );
			return true;

		case K_HOME:
			m_table->Select( CurRow, 0 );
			return true;

		default:
			break;
	}

	return false;
}

void ProgramTableUI::onClose()
{
}

int ProgramTableUI::onProgramData()
{
	fn_ProgramData( this );
	showProgramData();
	return 1;
}

int ProgramTableUI::onConveyorWizard()
{
	fn_ConveyorWizard();
	return 1;
}

int ProgramTableUI::onConveyorData()
{
	fn_ConveyorData( this );
	return 1;
}

int ProgramTableUI::onEditOverride()
{
	guiDeskTop->SetEditMode( true );
	m_table->SetEdit( true );
	return 1;
}

int ProgramTableUI::onCongruenceCheck()
{
	PrgM_Check();

	forceRefresh();
	return 1;
}

int ProgramTableUI::onTableSwitch()
{
	if( TPrg == TPrgExp )
	{
		Prg_SwitchTable(PRG_NORMAL);
	}
	else
	{
		Prg_SwitchTable(PRG_ASSEMBLY);
	}

	m_start_item = ATab[CurRow].Riga;
	return 1;
}

int ProgramTableUI::onTableSwitch_Dispenser()
{
	PrgM_DosaTableSwitch();

	m_start_item = ATab[CurRow].Riga;

	m_table->Select( CurRow, m_table->GetCurCol() );
	return 1;
}

int ProgramTableUI::onCopyField()
{
	return Prg_CopyField( m_table->GetCurCol() );
}


//---------------------------------------------------------------------------
// Setta lo stile della tabella
//---------------------------------------------------------------------------
void ProgramTableUI::setStyle()
{
	if( prevTPrg == TPrg )
	{
		return;
	}

	prevTPrg = TPrg;

	// set table params
	if( TPrg == TPrgNormal )
	{
		SetTitle( MsgGetString(Msg_00003) );
		m_table->SetBgColor( GUI_color( GRID_COL_BG ) );
	}
	else if( TPrg == TPrgExp )
	{
		SetTitle( MsgGetString(Msg_00182) );
		m_table->SetBgColor( GUI_color( PRG_COLOR_MOUNT_BG ) );
	}
	else
	{
		SetTitle( MsgGetString(Msg_01212) );
		m_table->SetBgColor( GUI_color( PRG_COLOR_DOSA_BG ) );
	}

	// set cols editable
	for( int i = 0; i < COL_TOT; i++ )
	{
		if( i == COL_LINE || i == COL_BOARD || i == COL_PACK || i == COL_FEEDER )
			continue;

		m_table->SetColEditable( i, ( TPrg == TPrgNormal ) ? true : false );
	}
}

//---------------------------------------------------------------------------
// Forza il refresh della tabella
//---------------------------------------------------------------------------
void ProgramTableUI::forceRefresh()
{
	GUI_Freeze_Locker lock;

	setStyle();

	int start = m_start_item;
	m_start_item = -1;
	showItems( start );

	showProgramData();
}

//---------------------------------------------------------------------------
// Visualizza una riga della tabella
//---------------------------------------------------------------------------
void ProgramTableUI::showRow( int row )
{
	GUI_Freeze_Locker lock;

	// row mark
	int markX1 = GetX() + GUI_CharW();
	int markX2 = GetX() + GetW() - 2*GUI_CharW();
	int markY = GetY() + (row + 4)*GUI_CharH() + 4;

	if( ATab[row].status & VERSION_MASK && ATab[row].Riga != 0 )
	{
		GUI_DrawGlyph( markX1, markY, '>', GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_GREEN ) );
		GUI_DrawGlyph( markX2, markY, '<', GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_GREEN ) );
	}
	else
	{
		GUI_DrawText( markX1, markY, " ", GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_GREEN ) );
		GUI_DrawText( markX2, markY, " ", GUI_DefaultFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_GREEN ) );
	}

	// empty row
	if( ATab[row].Riga == 0 )
	{
		for( int i = 0; i < COL_TOT; i++ )
		{
			m_table->SetText( row, i, "" );
		}
		return;
	}

	// check riga selezionata
	if( ATab[row].status & (RDEL_MASK | RMOV_MASK) )
	{
		m_table->SetRowStyle( row, TABLEROW_STYLE_HIGHLIGHT_RED );
	}
	else
	{
		m_table->SetRowStyle( row, TABLEROW_STYLE_DEFAULT );
	}

	m_table->SetText( row, COL_LINE, ATab[row].Riga );
	m_table->SetText( row, COL_CODE, ATab[row].CodCom );
	m_table->SetText( row, COL_BOARD, ATab[row].scheda );
	m_table->SetText( row, COL_COMP, ATab[row].TipCom );
	m_table->SetText( row, COL_PACK, ATab[row].pack_txt );
	m_table->SetText( row, COL_ROT, ATab[row].Rotaz );
	m_table->SetText( row, COL_NOZZLE, ATab[row].Punta == '1' ? "1" : "2" );
	m_table->SetText( row, COL_X, ATab[row].XMon );
	m_table->SetText( row, COL_Y, ATab[row].YMon );
	m_table->SetText( row, COL_FEEDER, ATab[row].Caric );
	m_table->SetText( row, COL_NOTES, ATab[row].NoteC );

	if( (ATab[row].status & MOUNT_MASK) && !(ATab[row].status & NOMNTBRD_MASK) )
	{
		m_table->SetText( row, COL_ASSEM, SHORT_YES );
	}
	else
	{
		m_table->SetText( row, COL_ASSEM, SHORT_NO );
	}

	#ifndef __DISP2
	if( (ATab[row].status & DODOSA_MASK)  && !(ATab[row].status & NOMNTBRD_MASK) )
	{
		m_table->SetText( row, COL_DISP, SHORT_YES );
	}
	else
	{
		m_table->SetText( row, COL_DISP, SHORT_NO );
	}
	#else
	switch( ATab[row].status & (DODOSA_MASK | DODOSA2_MASK) )
	{
		case DODOSA_MASK:
			m_table->SetText( row, COL_DISP, "1" );
			break;

		case DODOSA2_MASK:
			m_table->SetText( row, COL_DISP, "2" );
			break;

		case (DODOSA_MASK | DODOSA2_MASK):
			m_table->SetText( row, COL_DISP, DOSA_ALL_TXT );
			break;

		default:
			m_table->SetText( row, COL_DISP, SHORT_NO );
			break;
	}
	#endif
}

//---------------------------------------------------------------------------
// Visualizza i dati in tabella
//---------------------------------------------------------------------------
void ProgramTableUI::showItems( int start_item )
{
	if( start_item == m_start_item )
		return;

	GUI_Freeze_Locker lock;

	for( int row = 0; row < MAXRECS; row++ )
	{
		showRow( row );
	}
	showSelectedRowData();

	m_start_item = start_item;

	m_table->Select( CurRow, m_table->GetCurCol() );
}

//---------------------------------------------------------------------------
// Visualizza i dati associati alla riga corrente
//---------------------------------------------------------------------------
void ProgramTableUI::showSelectedRowData()
{
	GUI_Freeze_Locker lock;

	if( TPrg == TPrgExp && (ATab[CurRow].status & MOUNT_MASK) && !(ATab[CurRow].status & NOMNTBRD_MASK) )
	{
		char tool = ATab[CurRow].Uge;
		if( tool < 'A' || tool > 'Z' )
		{
			tool = ' ';
		}

		char buf[80];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01044), ATab[CurRow].Punta, tool );
		DrawText( 3, PROG_TAB_Y + MAXRECS + 1, buf );
	}
	else
	{
		int x = GetX() + 3*GUI_CharW();
		int y = GetY() + (PROG_TAB_Y + MAXRECS + 1)*GUI_CharH();
		GUI_FillRect( RectI( x, y, 30*GUI_CharW(), GUI_CharH() ), GUI_color( WIN_COL_CLIENTAREA ) );
	}
}

//---------------------------------------------------------------------------
// Visualizza i dati del programma
//---------------------------------------------------------------------------
void ProgramTableUI::showProgramData()
{
	// schede e componenti
	Read_nboard( nboard, ncomp );

	GUI_Freeze();
	m_combos[NUM_BOARDS]->SetTxt( int(nboard) );
	m_combos[NUM_COMPS]->SetTxt( int(ncomp) );
	GUI_Thaw();
}

//--------------------------------------------------------------------------
// Sposta la selezione verticalmente
//--------------------------------------------------------------------------
bool ProgramTableUI::vSelect( int key )
{
	if( m_table->GetCurRow() < 0 )
		return false;

	GUI_Freeze_Locker lock;

	if( key == K_DOWN )
	{
		if( CurRecord == (Reccount-1) || Reccount == 0 )
		{
			// aggiungo una riga
			Prg_IncRow();
			// aggiorno la tabella
			return true;
		}

		if( CurRow < m_table->GetRows() - 1 )
		{
			if( Prg_IncRow() )
			{
				showSelectedRowData();
				// lascio eseguire la selezione al parent
				return false;
			}
			return true;
		}
		else // sono nell'ultima riga
		{
			Prg_IncRow();
			// aggiorno la tabella
			return true;
		}
	}
	else if( key == K_UP )
	{
		if( CurRecord == 0 )
		{
			// niente altro da selezionare
			return true;
		}

		if( CurRow > 0 )
		{
			int a = AFlag;
			Prg_DecRow();
			if( a != AFlag )
			{
				forceRefresh();
				showSelectedRowData();
				return true;
			}

			showSelectedRowData();
			// lascio eseguire la selezione al parent
			return false;
		}
		else // sono nella prima riga
		{
			Prg_DecRow();
			// aggiorno la tabella
			return true;
		}
	}
	else if( key == K_PAGEDOWN )
	{
		Prg_PGDown();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_PAGEUP )
	{
		Prg_PGUp();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_CTRL_PAGEDOWN )
	{
		Prg_CtrlPGDown();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_CTRL_PAGEUP )
	{
		Prg_CtrlPGUp();
		// aggiorno la tabella
		return true;
	}

	return false;
}


void Prg_SwitchTable(int mode)
{
	switch(mode)
	{
	case PRG_NORMAL:
		if(TPrg!=TPrgNormal)
		{
			SwitchToMasterTable();
		}
		break;

	case PRG_ASSEMBLY:
		if(TPrg!=TPrgExp)
		{
			if(TPrgExp->Count()>0)
			{
				SwitchToAssemblyTable();
			}
			else
			{
				W_Mess( MsgGetString(Msg_01209) );
			}
		}
		break;

	case PRG_DOSAT:
		if(TPrg!=TPrgDosa)
		{
			if(TPrgDosa->Count()>0)
			{
				SwitchToDosaTable();
			}
			else
			{
				W_Mess( MsgGetString(Msg_01214) );
			}
		}
		break;
	}
}


//##SMOD011002 START
int Prg_OpenFiles( bool check )
{
	if(!TPrgNormal->Open(SKIPHEADER))
	{
		W_Mess( MsgGetString(Msg_00184) );
		return 0;
	}

	if(!TPrgExp->IsOnDisk())
		TPrgExp->Create();

	TPrgExp->Open(SKIPHEADER);

	TPrgDosa->Open();

	if( !PackagesLib_Load( QHeader.Lib_Default ) )
	{
		W_Mess(NOPACK);
		return 0;
	}

	Prg_Reload();

	CurRecord=0;
	CurRow=0;

	//SMOD280403 START
	char X_NomeFile[MAXNPATH];
	CarPath(X_NomeFile,QHeader.Conf_Default);

	if(access(X_NomeFile,0)!=0)
	{
		W_Mess(NOCONF);
		return 0;
	}
	else
	{
		CarFile = new FeederFile( QHeader.Conf_Default, check );

		if( !CarFile->opened )
		{
			delete CarFile;
			CarFile=NULL;
			return 0;
		}

		delete CarFile;
		CarFile=NULL;
	}
	//SMOD280403 END

	return 1;
}

void Prg_CloseFiles()
{
	TPrgNormal->Close();
	TPrgExp->Close();
	TPrgDosa->Close();
}
//##SMOD011002 END


//DANY080103
int G_Prg(void)
{
	char X_NomeFile[MAXNPATH];
	
	menuIdx=0;
	updateDosa=0;
	
	AFlag=0;
	
	lastCodCompFound = -1;
	
	Read_PrgCFile(QHeader.Lib_Default,QHeader.Conf_Default);
	Mod_Cfg(QHeader);

	PrgPath(X_NomeFile,QHeader.Prg_Default);
	TPrgNormal=new TPrgFile(X_NomeFile,PRG_NOADDPATH);
	
	PrgPath(X_NomeFile,QHeader.Prg_Default,PRG_ASSEMBLY);
	TPrgExp=new TPrgFile(X_NomeFile,PRG_NOADDPATH);
	
	PrgPath(X_NomeFile,QHeader.Prg_Default,PRG_DOSAT);
	TPrgDosa=new TPrgFile(X_NomeFile,PRG_NOADDPATH);
	
	TPrg=TPrgNormal;

	if( !Prg_OpenFiles( true ) )
	{
		delete TPrgExp;
		delete TPrgNormal;
		delete TPrgDosa;
		return 0;
	}

	// check file zeri
	ZerFile* fz = new ZerFile( QHeader.Prg_Default );
	if( !fz->IsOnDisk() )
	{
		fz->Create();
	}
	delete fz;

	Prg_Flag=1;

	Prg_MenuSSearch();
	Prg_MenuAut();
	Prg_MenuAsse();
	Prg_MenuRio();
	Prg_MenuTab();
	Prg_MenuDas();
	Prg_MenuCarSeq();
	Prg_MenuDist();
	Prg_MenuCheckOrigin();

	TPrg = NULL;
	Prg_SwitchTable( PRG_NORMAL );

	programWin = new ProgramTableUI;
	programWin->Show();
	programWin->Hide();
	delete programWin;
	programWin = 0;

	if(updateDosa && (TPrgExp->IsOnDisk() && TPrgExp->Count()>0))
	{
		CreateDosaFile();
	}

	Prg_Desel();
	
	delete Q_MenuSSearch;

	delete Q_MenuAut;
	delete Q_MenuAsse;
	delete Q_MenuRio[0];
	delete Q_MenuRio[1];
	delete Q_SubMenuRio;

	delete Q_MenuMountAll;
	delete Q_MenuDispAll;
	delete Q_MenuFiducials;
	delete Q_MenuSplit;
	delete Q_MenuChangeCoord;

	#ifdef __DISP2
	delete Q_MenuDas1;
	delete Q_MenuDist1;
	delete Q_MenuDas2;
	delete Q_MenuDist2;
	delete Q_MenuD1D2As;
	delete Q_MenuD1AsD2;
	#endif
	
	delete Q_MenuDas;
	delete Q_MenuDist;
	delete Q_SubMenuCarZSeq;
	delete Q_MenuCarSeq;
	delete Q_MenuZPos;
	delete Q_MenuZTheta;
	delete Q_MenuCheckOri;

	delete TPrgExp;
	delete TPrgNormal;
	delete TPrgDosa;

	Prg_Flag=0;

	return 1;
}

//------------------------------------------------------------------------
int CheckAssemblyPrgPresent()
{
	if( TPrgExp->Count() == 0 )
	{
		W_Mess( MsgGetString(Msg_01887) );
		return 0;
	}

	return 1;
}

int CheckSomethingToMount()
{
	for( int i = 0; i < TPrgExp->Count(); i++ )
	{
		struct TabPrg rec;
		TPrgExp->Read( rec, i );
		if( (rec.status & MOUNT_MASK) && (!(rec.status & NOMNTBRD_MASK)) )
		{
			return 1;
		}
	}

	W_Mess( MsgGetString(Msg_01956) ); // nothing to mount
	return 0;
}

//TODO: controllare per ottimizzazione check di congruenza
int CheckDupCompcod(void)
{
	print_debug("CheckDupCompcod\n");

	int nrec = TPrgNormal->Count();
	struct TabPrg* rec = new struct TabPrg[nrec];

	for( int i = 0; i < nrec; i++ )
	{
		TPrgNormal->Read(rec[i],i);
	}

	for( int i = 0; i < nrec; i++ )
	{
		for( int j = i+1; j < nrec; j++ )
		{
			if(!strcasecmpQ(rec[i].CodCom,rec[j].CodCom))
			{
				char buf[120];
				snprintf( buf, sizeof(buf), ERRDUPCODE, rec[i].CodCom );
				W_Mess(buf);
				delete [] rec;
				return 0;
			}
		}
	}

	delete [] rec;

	return 1;
}

// check assembly data
//===============================================================================
int check_data( int mode )
{
	print_debug("check_data( %d )\n", mode );

	CarFile = new FeederFile(QHeader.Conf_Default);
	if( !CarFile->opened )
	{
		delete CarFile;
		CarFile = NULL;
		return 0;
	}

	checkprg_wait = new CWindow( 0 );
	checkprg_wait->SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU );
	checkprg_wait->SetClientAreaPos( 0, 10 );
	checkprg_wait->SetClientAreaSize( 50, 6 );

	checkprg_wait->Show();
	checkprg_wait->DrawTextCentered( 1, MsgGetString(Msg_00517) );

	//check congruenza dei flag assembla si/no tra tab di programma e tabella degli zeri
	int retval = CheckZeriProg();
	if( retval )
	{
		if( QHeader.modal & ENABLE_CARINT )
		{
			if(!IsConfDBLinkOk())
			{
				retval=UpdateDBData(CARINT_UPDATE_FULL);

				if(retval==1)
				{
					retval=ConfImport(0);
				}
			}

			if(retval==1) //SMOD070705
			{
				if( QHeader.modal & ENABLE_CARINT )
				{
					struct CarIntAuto_res results;
			
					CarIntAutoMag(CARINT_SEARCH_NORMAL, CARINT_AUTOALL_MASK, results);
			
					if(QHeader.modal & ENABLE_CARINT)
					{
						retval=CarIntAuto_ElabResults(results,CARINT_AUTOALL_MASK,0);
					}
				}
			}

			//aggiorna configurazione con i magazzini riconosciuti (se necessario)
			if((retval==1) && (QHeader.modal & ENABLE_CARINT))
			{
				retval=UpdateCaricLib();
			}
		}

		if(retval==1)
		{
			CCheckPrg check(checkprg_wait,mode);
			retval = check.Start();
		}
	}
	
	delete CarFile;
	CarFile=NULL;

	if(checkprg_wait!=NULL)
	{
		delete checkprg_wait;
		checkprg_wait=NULL;
	}

	return retval;
}


CCheckPrg::CCheckPrg( CWindow* _wait_window, int _mode )
: wait_window(_wait_window),
  mode(_mode),
  progress_tot_phases(0),
  progress_phase(0),
  progress_size(0),
  checkprg_progbar(NULL),
  tprg_assembly_toupdate(0),
  tprg_normal_nrec(0),
  tprg_exp_nrec(0),
  tprg_normal(NULL),
  tprg_exp(NULL)
{

	for( int i = 0; i < MAXCAR; i++ )
	{
		CarFile->ReadRec(i,feeders[i]);
	}

	tprg_normal_nrec = TPrgNormal->Count();

	tprg_normal = new struct TabPrg[tprg_normal_nrec];

	for(int i=0; i<tprg_normal_nrec; i++)
	{
		TPrgNormal->Read(tprg_normal[i],i);
	}
	
	if(mode & CHECKPRG_COMPONENTS)
	{
		progress_size+=2*tprg_normal_nrec;
		progress_tot_phases+=2;
	}
	
	if(mode & CHECKPRG_ALIGN_TABLES)
	{
		progress_size+=tprg_normal_nrec;
		progress_tot_phases++;
	}
	
	if(mode & CHECKPRG_PREASSEMBLY)
	{
		progress_size+=tprg_normal_nrec;
		progress_tot_phases++;
	}
	
	if(mode & CHECKPRG_FEEDERDB)
	{
		progress_size+=tprg_normal_nrec;
		progress_tot_phases++;
	}

	//update assembly file
	progress_size+=tprg_normal_nrec;
	
	checkprg_progbar = new GUI_ProgressBar_OLD( wait_window, 2, 4, 46, 1, progress_size );
}

CCheckPrg::~CCheckPrg(void)
{
	if(tprg_normal!=NULL)
	{
		delete[] tprg_normal;
	}
	
	if(tprg_exp!=NULL)
	{
		delete[] tprg_exp;
	}
	
	if(checkprg_progbar!=NULL)
	{
		delete checkprg_progbar;
	}
}


int CCheckPrg::SearchPackage(char *name)
{
	for( int i = 0; i < MAXPACK; i++ )
	{
		if( !strncasecmpQ( currentLibPackages[i].name, name, 20 ) )
		{
			return i;
		}
	}

	return -1;
}

int CCheckPrg::Start(void)
{
	//TODO: togliere
	print_debug( "CCheckPrg::Start\n" );

	int ok = 0;

	if( mode & CHECKPRG_COMPONENTS )
	{
		char sbuf[160];
		int first_err = -1;
		int err = ComponentsCheck(first_err);

		switch(err)
		{
		case COMPCHECK_NOERR:
			ok = 1;
			break;

		case COMPCHECK_TRAYHEIGHT:
		case COMPCHECK_ERRFEEDER:
			return 0;
			break;

		case COMPCHECK_NOCOMPTYPE:
			snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_00140), first_err+1 );
			W_Mess(sbuf);
			break;

		case COMPCHECK_NOCONF:
			snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_00224), first_err+1 );
			W_Mess(sbuf);
			break;
		}
	}
	
	if(tprg_assembly_toupdate)
	{
		UpdateAssemblyFile(checkprg_progbar);
	}
	
	if( ok && (mode & CHECKPRG_ALIGN_TABLES) )
	{
		if(!AlignMasterAssemblyTable())
		{
			ok=0;
		}
	}
	
	if( ok && (mode & CHECKPRG_PREASSEMBLY) )
	{
		if(!PreAssemblyCheck())
		{
			ok=0;
		}
	}
	
	if( ok && (mode & CHECKPRG_FEEDERDB) && (QHeader.modal & ENABLE_CARINT) )
	{
		if(!FeederDBCheck())
		{
			ok=0;
		}
	}
	
	return(ok);
}

int CCheckPrg::AssignToolHelper( int nrec )
{
	if( nrec >= tprg_exp_nrec )
	{
		return 1;
	}
	
	if(tprg_exp==NULL)
	{
		return 1;
	}
	
	char buff[80];
	
	SPackageData* pack = &currentLibPackages[feeders[GetCarRec(tprg_exp[nrec].Caric)].C_PackIndex-1];
	
	int len = strlen(pack->tools);
	
	switch(tprg_exp[nrec].Punta)
	{
	case '1':
		if((tprg_exp[nrec].Uge>='A') && (tprg_exp[nrec].Uge<='Z'))
		{
			if(strchr( pack->tools, tprg_exp[nrec].Uge)!=NULL)
			{
				CfgUgelli udat;
				Ugelli->ReadRec(udat,tprg_exp[nrec].Uge-'A');
				if(udat.NozzleAllowed & UG_P1)
				{
					break;
				}
			}
		}

		if(len>=1)
		{
			tprg_exp[nrec].Uge=pack->tools[0];
			TPrgExp->Write(tprg_exp[nrec],nrec,FLUSHON);
		}
		else
		{
			snprintf(buff, sizeof(buff),"%s %s.",MsgGetString(Msg_01036),pack->name);
			W_Mess(buff);
			return 0;
		}
		break;
		
	case '2':
	
		if((tprg_exp[nrec].Uge>='A') && (tprg_exp[nrec].Uge<='Z'))
		{
		if(strchr(pack->tools,tprg_exp[nrec].Uge)!=NULL)
		{
			CfgUgelli udat;
			Ugelli->ReadRec(udat,tprg_exp[nrec].Uge-'A');
			if(udat.NozzleAllowed & UG_P2)
			{
			break;
			}          
		}
		}
	
		if(len>=2)
		{
		tprg_exp[nrec].Uge=pack->tools[1];
		TPrgExp->Write(tprg_exp[nrec],nrec,FLUSHON);
		}
		else
		{
		if(len==1)
		{
			tprg_exp[nrec].Uge=pack->tools[0];
			TPrgExp->Write(tprg_exp[nrec],nrec,FLUSHON);
		}
		else
		{
			snprintf(buff, sizeof(buff),"%s %s.",MsgGetString(Msg_01036),pack->name);
			W_Mess(buff);
			return 0;
		}
		break;
		}
	
	}
	
	return 1;
}



int CCheckPrg::FeederCheck(struct CarDat& car)
{
	char sbuf[160];
	std::string packName;

	int npack_ret,ret;

	W_MsgBox *WAsk=NULL;

	DelSpcR(car.C_Package);

	//se il caricatore specificato ha un package
	if((car.C_PackIndex!=0) && (car.C_PackIndex<=MAXPACK) && (*car.C_Package!=0))
	{
		npack_ret=SearchPackage(car.C_Package);

		npack_ret++;

		if(npack_ret!=0)
		{
			//package esiste
			if(npack_ret!=car.C_PackIndex)
			{
				//correggi indice package in tab. caricatori
				car.C_PackIndex = npack_ret;

				CarFile->SaveX(car.C_codice,car);
				CarFile->SaveFile();
			}
			else
			{
				//nessuna operazione necessaria
				return 1;
			}
		}
		else
		{
			//package esiste solo in tab. caricatori
			snprintf( sbuf, 160, MsgGetString(Msg_01719) , car.C_codice, car.C_Package );
			char msg[160];
			snprintf( msg, 160, "%s\n%s", sbuf, MsgGetString(Msg_00929) );


		  WAsk=new W_MsgBox( MsgGetString(Msg_00361), msg,2,MSGBOX_GESTKEY);
		  WAsk->AddButton(FAST_YES);
		  WAsk->AddButton(FAST_NO,1);
		  ret=WAsk->Activate();
		  delete WAsk;

		  if(ret==1)
		  {
			if( fn_PackagesTableSelect( 0, "", npack_ret, packName ) )
			{
			  car.C_PackIndex=npack_ret;
			  strcpy(car.C_Package,packName.c_str());

			  //se non e' un vassoio
			  if((car.C_codice/10)<=MAXMAG)
			  {
				int dbIdx=GetConfDBLink(car.C_codice/10);

				//e risulta montato
				if(dbIdx!=-1)
				{
				  //aggiorna database

				  int f=DBToMemList_Idx(dbIdx);

				  if(f!=-1)
				  {
					int nc=(car.C_codice-(car.C_codice/10)*10)-1;

					CarList[f].packIdx[nc]=npack_ret;
					strcpy(CarList[f].pack[nc],packName.c_str());

					CarList[f].changed=CARINT_NONET_CHANGED;

					if(IsNetEnabled())
					{
					  if(DBRemote.Write(CarList[f],dbIdx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
					  {
						CarList[f].changed=0;
					  }
					}

					print_debug("check_confrec %d %d\n",dbIdx,CarList[f].idx);
					DBLocal.Write(CarList[f],dbIdx,FLUSHON);
				  }
				}
			  }

			  CarFile->SaveX(car.C_codice,car);
			  CarFile->SaveFile();
			}
			else
			{
			  return 0;
			}
		  }
		  else
		  {
			//non correggere ed esci
			return 0;
		  }
		}
	}
	else
	{
		//caricatore non ha package
		snprintf( sbuf, 160, "%s", car.C_comp );
		DelSpcL(sbuf);

		if( sbuf[0] != 0 )
		{
			//se caricatore ha associato tipo componente

			snprintf( sbuf, 160, "%s %d %s", MsgGetString(Msg_00986), car.C_codice, MsgGetString(Msg_00929) );

		  WAsk=new W_MsgBox( MsgGetString(Msg_00361), sbuf,2,MSGBOX_GESTKEY);
		  WAsk->AddButton(FAST_YES);
		  WAsk->AddButton(FAST_NO,1);
		  ret=WAsk->Activate();

		  delete WAsk;

		  if(ret==1)
		  {
			//correggi:seleziona package per il caricatore
			if( fn_PackagesTableSelect( 0, "", npack_ret, packName ) )
			{
			  car.C_PackIndex=npack_ret;
			  strcpy(car.C_Package,packName.c_str());

			  //se non e' un vassoio
			  if((car.C_codice/10)<=MAXMAG)
			  {
				int dbIdx=GetConfDBLink(car.C_codice/10);

				//e risulta montato
				if(dbIdx!=-1)
				{
				  //aggiorna database

				  int f=DBToMemList_Idx(dbIdx);

				  if(f!=-1)
				  {

					//struct CarInt_data dbData;
					//DBLocal.Read(dbData,dbIdx);

					int nc=(car.C_codice-(car.C_codice/10)*10)-1;

					CarList[f].packIdx[nc]=npack_ret;
					strcpy(CarList[f].pack[nc],packName.c_str());

					CarList[f].changed=CARINT_NONET_CHANGED;

					if(IsNetEnabled())
					{
					  if(DBRemote.Write(CarList[f],dbIdx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
					  {
						CarList[f].changed=0;
					  }
					}

					print_debug("check_confrec2 %d %d\n",dbIdx,CarList[f].idx);
					DBLocal.Write(CarList[f],dbIdx,FLUSHON);
				  }
				}
			  }

			  CarFile->SaveX(car.C_codice,car);
			  CarFile->SaveFile();
			}
			else
			{
			  //non correggere ed esci
			  return 0;
			}
		  }
		  else
		  {
			return 0;
		  }
		}
	}

	return 1;
}


int CCheckPrg::ComponentsCheck(int& first_err)
{
	//TODO: togliere
	print_debug( "CCheckPrg::ComponentsCheck\n" );

	char sbuf[160];
	char buf[26];
	
	first_err=-1;
	int err_type=COMPCHECK_NOERR;
	
	tprg_assembly_toupdate=0;
	
	InsSpc( strlen(PHASE_TXT)+8, sbuf );
	checkprg_wait->DrawText( 3, 3, sbuf );
	snprintf( sbuf, sizeof(sbuf), "%s %d / %d",PHASE_TXT,++progress_phase,progress_tot_phases);
	checkprg_wait->DrawText( 3, 3, sbuf );

	for( int i = 0; i < tprg_normal_nrec; i++ )
	{
		// se dispensing, check anche se solo dosatura
		if( mode & CHECKPRG_DISP )
		{
			if( !((tprg_normal[i].status & DODOSA_MASK) == DODOSA_MASK) && !((tprg_normal[i].status & DODOSA2_MASK) == DODOSA2_MASK) )
			{
				tprg_normal[i].Caric = 0;
				TPrgNormal->Write(tprg_normal[i],i,FLUSHON);

				checkprg_progbar->Increment(1);
				continue;
			}
		}
		else
		{
			// se componente non assemblato mette caricatore a zero e continua
			if( !(tprg_normal[i].status & MOUNT_MASK) )
			{
				tprg_normal[i].Caric = 0;
				TPrgNormal->Write(tprg_normal[i],i,FLUSHON);

				checkprg_progbar->Increment(1);
				continue;
			}
		}

		//controlla tipo componente specificato nel programma
		strncpyQ(buf,tprg_normal[i].TipCom,25); //SMOD210503
		DelSpcR(buf);

		if(*buf=='\0')
		{
			//SMOD280408
			tprg_normal[i].Caric=0;
			tprg_normal[i].Changed=COD_FIELD;
			TPrgNormal->Write(tprg_normal[i],i,FLUSHON);
			
			if(first_err==-1)
			{
				first_err=i;
				err_type=COMPCHECK_NOCOMPTYPE;
			}
			
			continue;
		}

		//controlla esistenza in configurazione caricatori dei tipi componenti specificati nel programma
		int nfeeder_found=-1;
		int trayflag[MAXTRAY];

		memset(trayflag,0,sizeof(trayflag));

		for(int j=0;j<MAXCAR;j++)
		{
			if(!strcasecmpQ(feeders[j].C_comp,tprg_normal[i].TipCom))
			{
				//codice componente trovato
				if( feeders[j].C_codice>=FIRSTTRAY && !trayflag[(feeders[j].C_codice-FIRSTTRAY)/10] && feeders[j].C_offprel==0 )
				{
					snprintf( sbuf, sizeof(sbuf), ERR_TRAYZEROHEIGHT,feeders[j].C_codice);
					if( !W_Deci(1,sbuf) )
					{
						return(COMPCHECK_TRAYHEIGHT);
					}

					trayflag[(feeders[j].C_codice-FIRSTTRAY)/10]=1;
				}
			
				nfeeder_found=j;
				break;
			}
		}

		//copia temporanea del record tab. programma da aggiornare
		struct TabPrg tmpRec=tprg_normal[i];
	

		if( nfeeder_found < 0 )
		{
			//se tipo componente non trovato in configurazione caricatori
			tprg_normal[i].Caric = 0;
			if( first_err == -1 )
			{
				first_err = i;
				err_type = COMPCHECK_NOCONF;
			}
		}
		else
		{
			//controlla che il caricatore da associare sia valido
			if(!FeederCheck(feeders[nfeeder_found]))
			{
				return COMPCHECK_ERRFEEDER;
			}

			//check passato->associa dati
			if(QHeader.modal & ASK_PACKAGE_CHANGE)
			{
				if(strlen(tprg_normal[i].pack_txt))
				{
					if(strcasecmpQ(tprg_normal[i].pack_txt,feeders[nfeeder_found].C_Package))
					{
						char buff[256];
						snprintf(buff, sizeof(buff),COMPCHECK_ASK_PACKAGE_CHANGE,tprg_normal[i].Riga,tprg_normal[i].pack_txt,feeders[nfeeder_found].C_Package);
						if(!W_Deci(0,buff))
						{
							return COMPCHECK_DIFF_PACKAGE;
						}
					}
				}
			}

			tprg_normal[i].Caric=feeders[nfeeder_found].C_codice;
			strncpyQ(tprg_normal[i].pack_txt,feeders[nfeeder_found].C_Package,20);
		}

		if(memcmp(&tprg_normal[i],&tmpRec,sizeof(struct TabPrg))!=0)
		{
			//trick: package e caricatore vengono sempre aggiornati da UpdateAssemblyRec a patto che un campo in Changed sia attivo
			tprg_normal[i].Changed=COD_FIELD;
			tprg_assembly_toupdate=1;
		
			TPrgNormal->Write(tprg_normal[i],i,FLUSHON);
		}

		checkprg_progbar->Increment(1);
	}

	return err_type;
}

int CCheckPrg::AlignMasterAssemblyTable()
{
	//TODO: togliere
	print_debug( "CCheckPrg::AlignMasterAssemblyTable\n" );

	if(!CheckAssemblyPrgPresent())
	{
		return 0;
	}

	int tprg_exp_null=0;

	if(tprg_exp==NULL)
	{
		tprg_exp_nrec=TPrgExp->Count();
		tprg_exp=new struct TabPrg[tprg_exp_nrec];
		tprg_exp_null=1;
	}

	int tprg_exp_nrec=TPrgExp->Count();    

  	// Verifica se il numero dei componenti del programma di montaggio e' uguale
  	// al numero di componenti del programma Master * numero degli zeri

	ZerFile *zer=new ZerFile(QHeader.Prg_Default);   //apre file zeri
	if(!zer->Open())                   
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		delete[] tprg_exp;
		tprg_exp=NULL;			
		return 0;
	}
	
	int zer_count = zer->GetNRecs()-1;
	delete zer;

	if(tprg_exp_nrec!=(tprg_normal_nrec*zer_count))
	{
		W_Mess(ERR_NUMCOMP);
		return 0;
	}

	char sbuf[160];

	InsSpc(strlen(PHASE_TXT)+8,sbuf);
	checkprg_wait->DrawText( 3, 3, sbuf );
	snprintf( sbuf, 160, "%s %d / %d",PHASE_TXT,++progress_phase,progress_tot_phases);
	checkprg_wait->DrawText( 3, 3, sbuf );

  	//SMOD141107
	TPrgNormal->InitSearchBuf();

	for(int i=0;i<tprg_exp_nrec;i++)
	{    
		if(tprg_exp_null)
		{
			TPrgExp->Read(tprg_exp[i],i);
		}

		int mrec=TPrgNormal->Search(0,tprg_exp[i],PRG_CODSEARCH,0);

    	//SMOD061108 START
		TabPrg tmaster;

		if(mrec!=-1)
		{
			TPrgNormal->Read(tmaster,mrec);

			int mount_exp=(tprg_exp[i].status & MOUNT_MASK);
			int mount_master=(tmaster.status & MOUNT_MASK);

			if(mount_exp!=mount_master)
			{
				snprintf( sbuf, sizeof(sbuf), ERR_MOUNT_MASTER,tmaster.Riga);
				W_Mess(sbuf);

				TPrgNormal->DestroySearchBuf();        
				return 0;
			}
		}
		else
		{
			snprintf( sbuf, sizeof(sbuf), ERR_COMP_MISSING,tprg_exp[i].CodCom);
			W_Mess(sbuf);

			TPrgNormal->DestroySearchBuf();
			return 0;
		}

		// se dispensing, check solo dosatura
		if( mode & CHECKPRG_DISP )
		{
			if( !((tprg_exp[i].status & DODOSA_MASK) == DODOSA_MASK) && !((tprg_exp[i].status & DODOSA2_MASK) == DODOSA2_MASK) || (tprg_exp[i].status & NOMNTBRD_MASK) )
			{
				tprg_exp[i].Caric=0;
				TPrgExp->Write(tprg_exp[i],i,FLUSHON);
				checkprg_progbar->Increment(1);
				continue;
			}
			else
			{
				if(tprg_exp[i].Caric!=tmaster.Caric)
				{
					tprg_exp[i].Caric=tmaster.Caric;
					TPrgExp->Write(tprg_exp[i],i,FLUSHON);
				}
			}
		}
		else
		{
			if((!(tprg_exp[i].status & MOUNT_MASK)) || (tprg_exp[i].status & NOMNTBRD_MASK))
			{
				tprg_exp[i].Caric=0;
				TPrgExp->Write(tprg_exp[i],i,FLUSHON);
				checkprg_progbar->Increment(1);
				continue;
			}
			else
			{
				if(tprg_exp[i].Caric!=tmaster.Caric)
				{
					tprg_exp[i].Caric=tmaster.Caric;
					TPrgExp->Write(tprg_exp[i],i,FLUSHON);
				}
			}
		}
    	//SMOD061108 END
	}

	return 1;
}


//TODO: controllare quante volte esegue i soliti check
int CCheckPrg::PreAssemblyCheck(void)
{
	//TODO: togliere
	print_debug( "PreAssemblyCheck\n" );

	if(!CheckAssemblyPrgPresent())
	{
		return 0;
	}

	int tprg_exp_null = 0;

	if(tprg_exp==NULL)
	{
		tprg_exp_nrec=TPrgExp->Count();
		tprg_exp=new struct TabPrg[tprg_exp_nrec];
		tprg_exp_null=1;
	}

	int tprg_exp_nrec = TPrgExp->Count();
	
	// Verifica se il numero dei componenti del programma di montaggio e' uguale
	// al numero di componenti del programma Master * numero degli zeri
	
	ZerFile *zer=new ZerFile(QHeader.Prg_Default);   //apre file zeri
	if(!zer->Open())
	{
		W_Mess( NOZSCHFILE );
		delete zer;
		delete[] tprg_exp;
		tprg_exp=NULL;
		return 0;
	}
	int zer_count = zer->GetNRecs()-1;
	delete zer;
	
	if(tprg_exp_nrec!=(tprg_normal_nrec*zer_count))
	{
		W_Mess( ERR_NUMCOMP );
		return 0;
	}

	char sbuf[160];
	
	InsSpc(strlen(PHASE_TXT)+8,sbuf);
	checkprg_wait->DrawText( 3, 3, sbuf );
	snprintf( sbuf, 160, "%s %d / %d",PHASE_TXT,++progress_phase,progress_tot_phases);
	checkprg_wait->DrawText( 3, 3, sbuf );

	//SMOD141107
	TPrgNormal->InitSearchBuf();
	
	for(int i=0;i<tprg_exp_nrec;i++)
	{
		if(tprg_exp_null)
		{
			TPrgExp->Read(tprg_exp[i],i);
		}

		int mrec=TPrgNormal->Search(0,tprg_exp[i],PRG_CODSEARCH,0);
	
		if(mrec!=-1)
		{
			TabPrg tmaster;
			TPrgNormal->Read(tmaster,mrec);
		
			int mount_exp=(tprg_exp[i].status & MOUNT_MASK);
			int mount_master=(tmaster.status & MOUNT_MASK);
	
			if(mount_exp!=mount_master)
			{
				snprintf( sbuf, 160, ERR_MOUNT_MASTER,tmaster.Riga);
				W_Mess(sbuf);
		
				TPrgNormal->DestroySearchBuf();
				return 0;
			}
		}
		else
		{
			snprintf( sbuf, 160, ERR_COMP_MISSING,tprg_exp[i].CodCom);
			W_Mess(sbuf);
		
			TPrgNormal->DestroySearchBuf();
			return 0;
		}

		if((!(tprg_exp[i].status & MOUNT_MASK)) || (tprg_exp[i].status & NOMNTBRD_MASK))
		{
			tprg_exp[i].Caric=0;
			TPrgExp->Write(tprg_exp[i],i,FLUSHON);
			checkprg_progbar->Increment(1);
			continue;
		}

		//TODO controllo dati package
		// fare controllo unico con classe package

		SPackageData* pack = &currentLibPackages[feeders[GetCarRec(tprg_exp[i].Caric)].C_PackIndex-1];

		//controlla che il package indichi almeno un ugello prelevabile
		int nozzleUsed = 0;

		for( unsigned int j = 0; j < strlen(pack->tools); j++ )
		{
			int packuge = pack->tools[j];

			struct CfgUgelli ugerec;
			Ugelli->ReadRec( ugerec, packuge-'A' );

			if( ugerec.NozzleAllowed != 0 )
			{
				nozzleUsed |= (ugerec.NozzleAllowed & UG_ALL );
			}

			if( nozzleUsed == UG_ALL )
			{
				break;
			}
		}

		// se nessun ugello prelevabile nel pack ...
		if( nozzleUsed == 0 )
		{
			snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_02120), pack->name );
			W_Mess( sbuf );
			TPrgNormal->DestroySearchBuf();
			return 0;
		}

		// se centraggio con camera esterna: controllo immagini package
		if( pack->centeringMode == CenteringMode::EXTCAM )
		{
			#ifdef __DISP2
				#ifndef __DISP2_CAM
				if( !Get_SingleDispenserPar() )
				{
					// centraggio camera esterna non consentito
					snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_00246), pack->name );
					W_Mess( sbuf );

					TPrgNormal->DestroySearchBuf();
					return 0;
				}
				#endif
			#endif

			int ret = CheckPackageVisionData( *pack, QHeader.Lib_Default, false );

			bool imgError = false;

			if( ret == 1 )
			{
				// ok
			}
			else if( nozzleUsed == UG_ALL || ret == 0 )
			{
				imgError = true;
			}
			else if( nozzleUsed == UG_P1 && ret == -1 )
			{
				imgError = true;
			}
			else if( nozzleUsed == UG_P2 && ret == -2 )
			{
				imgError = true;
			}

			if( imgError )
			{
				// apprendere immagini package
				snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_00244), pack->name );
				W_Mess( sbuf );

				TPrgNormal->DestroySearchBuf();
				return 0;
			}
		}


		// controlla validita dell'ugello assegnato e cambia se necessario
		if(!AssignToolHelper(i))
		{
			TPrgNormal->DestroySearchBuf();
			return 0;
		}

		//controlla se il componente puo' essere montato con la punta specificata dal programma
		int retval = CheckPackageNozzle( pack, tprg_exp[i].Punta-'0' );

		sbuf[0] = '\0';
	
		switch(retval)
		{
			case PACK_TOOBIG:
				snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_05057), pack->name, tprg_exp[i].Punta-'0' );
				break;
			case PACK_TOOHIGH:
				snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_01564), pack->name, tprg_exp[i].Punta-'0' );
				break;
			case PACK_NOTOOLS:
				//TODO: questo controllo va attivato solo dopo aver messo il controllo che modifica il numero di punta nel master
				/*
				snprintf( sbuf, sizeof(sbuf), MsgGetString(Msg_00118), pack->name );
				break;
				*/
			case PACK_DIMOK:
				break;
			default:
				snprintf( sbuf, sizeof(sbuf), "Package %s Unknown Error !", pack->name );
				break;
		}

		if( *sbuf != '\0' )
		{
			//trovato errore: mostra messaggio ed esci
			char errorMsg[200];
			snprintf( errorMsg, sizeof(errorMsg), MsgGetString(Msg_00102), tprg_exp[i].Riga );
			strncat( errorMsg, "\n", sizeof(errorMsg) );
			strncat( errorMsg, sbuf, sizeof(errorMsg) );

			W_Mess( errorMsg );
			TPrgNormal->DestroySearchBuf();
			return 0;
		}

		checkprg_progbar->Increment(1);
	}

	TPrgNormal->DestroySearchBuf();

	return 1;
}

int CCheckPrg::FeederDBCheck(void)
{
	//TODO: togliere
	print_debug( "FeederDBCheck\n" );

	char sbuf[160];
	
	InsSpc(strlen(PHASE_TXT)+8,sbuf);
	checkprg_wait->DrawText( 3, 3, sbuf );
	snprintf( sbuf, 160, "%s %d / %d",PHASE_TXT,++progress_phase,progress_tot_phases);
	checkprg_wait->DrawText( 3, 3, sbuf );
	
	if(!(QHeader.modal & ENABLE_CARINT))
	{
		checkprg_progbar->Increment(tprg_exp_nrec);
		return 1;
	}

	int checked_mag[MAXMAG];
	
	for(int i=0;i<MAXMAG;i++)
	{
		checked_mag[i]=0;
	}

	int tprg_exp_null=0;
	
	if(tprg_exp==NULL)
	{
		tprg_exp_nrec=TPrgExp->Count();
		tprg_exp=new struct TabPrg[tprg_exp_nrec];
		tprg_exp_null=1;
	}

	for(int i=0;i<tprg_exp_nrec;i++)
	{
		if(tprg_exp_null)
		{
			TPrgExp->Read(tprg_exp[i],i);
		}
	
		if((!(tprg_exp[i].status & MOUNT_MASK)) || (tprg_exp[i].status & NOMNTBRD_MASK))
		{
			checkprg_progbar->Increment(1);
			continue;
		}

		int m = (tprg_exp[i].Caric/10)-1;

		// continua se componente su vassoio
		if( m >= MAXMAG )
		{
			checkprg_progbar->Increment(1);
			continue;
		}

		// continua se caricatore già controllato
		if( checked_mag[m] )
		{
			checkprg_progbar->Increment(1);
			continue;
		}

		int idx = GetConfDBLink(m+1);
	
		if( idx == -1 )
		{
			snprintf( sbuf, sizeof(sbuf), ERR_NOMOUNTEDCAR,m+1);
			W_Mess(sbuf);
			return 0;
		}

		int f = DBToMemList_Idx(idx);

		//TEMP - cosa succede se uguale a -1 ???
		if(f!=-1)
		{
			if(CarInt_GetUsedState(CarList[f])!=MAG_USEDBY_ME)
			{
				switch((CarList[f].serial >> 24) & 0xFF)
				{
					case CARINT_TAPESER_OFFSET:
						snprintf( sbuf, sizeof(sbuf), ERRMAG_USEDBY_OTHERS3,'T',CarList[f].serial & 0xFFFFFF);
						break;
					case CARINT_AIRSER_OFFSET:
						snprintf( sbuf, sizeof(sbuf), ERRMAG_USEDBY_OTHERS3,'A',CarList[f].serial & 0xFFFFFF);
						break;
					case CARINT_GENERICSER_OFFSET:
						snprintf( sbuf, sizeof(sbuf), ERRMAG_USEDBY_OTHERS3,'G',CarList[f].serial & 0xFFFFFF);
						break;
					default:
						snprintf( sbuf, sizeof(sbuf), ERRMAG_USEDBY_OTHERS3,'X',CarList[f].serial & 0xFFFFFF);
						break;
				}
			
				W_Mess(sbuf);
				return 0;
			}

		}

		checked_mag[m] = 1;

		checkprg_progbar->Increment(1);
	}

	return 1;
}

//check ottimizzazione da eseguire
//ritorna 1 se si 0 altrimenti
//SMOD080503
int Check_OptFile()
{
	int retval=0;

	if(QParam.AutoOptimize==0)
	{
		return 0;
	}

	struct TabPrg recExp,recOpt;

	int nExp=TPrgExp->Count();

	char lastOptPath[MAXNPATH];
	PrgPath(lastOptPath,QHeader.Prg_Default,PRG_LASTOPT);
	TPrgFile *TPrgOpt=new TPrgFile(lastOptPath,PRG_NOADDPATH);
	if(!TPrgOpt->Open())
	{
		delete TPrgOpt;
		return 1;
	}

	int nOpt=TPrgOpt->Count();

	if(nOpt!=nExp)
	{
		delete TPrgOpt;
		return 1;
	}

	int first=1,foundBothNozzle=0;
	char firstNozzle;

	for(int i=0;i<nExp;i++)
	{
		TPrgExp->Read(recExp,i);
		TPrgOpt->Read(recOpt,i);

		//SMOD090503
		if(((recExp.status & MOUNT_MASK)!=(recOpt.status & MOUNT_MASK)) || ((recExp.status & NOMNTBRD_MASK)!=(recOpt.status & NOMNTBRD_MASK)))
		{
			retval=1;
			break;
		}

		if(first && (recExp.status & MOUNT_MASK) && !(recExp.status & NOMNTBRD_MASK))
		{
			firstNozzle=recExp.Punta;
			first=0;
		}

		if((firstNozzle!=recExp.Punta) && !foundBothNozzle)
		{
			foundBothNozzle=1;
		}

		if((fabs(recExp.XMon-recOpt.XMon)>SOGLIA_AUTOOPT) || (fabs(recExp.YMon-recOpt.YMon)>SOGLIA_AUTOOPT) || (strcasecmpQ(recExp.TipCom,recOpt.TipCom)) || ((recExp.status & MOUNT_MASK)!=(recOpt.status & MOUNT_MASK)) || ((recExp.status & NOMNTBRD_MASK)!=(recOpt.status & NOMNTBRD_MASK)))
		{
			retval=1;
			break;
		}
	}

	/*
	//se il programma attuale utilizza entrambe le punte o solo la punta 2
	//ma l'ottimizzazione e' a singola punta (1): esegui ottimizzazione
	//se il programma attuale utilizza solo la punta 1 ma l'ottimizzazione automatica
	//e' a doppia punta: esegui ottimizzazione
	if( ((foundBothNozzle || (!foundBothNozzle && (firstNozzle=='2'))) && (QParam.AutoOptimize==1))
		|| (!foundBothNozzle && (firstNozzle=='1') && (QParam.AutoOptimize==2)))
		retval=1;
	*/

	delete TPrgOpt;

	return retval;
}


//controlla congruenza dei flag assembla si/no tra programma di assemblaggio e tabella zeri
//controlla numero e duplicazione righe
//Ritorna 0 se controllo fallito, 1 altrimenti
//SMOD021003
int CheckZeriProg()
{
	ZerFile* zer = new ZerFile( QHeader.Prg_Default );

	if( !zer->Open() )
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		return 0;
	}

	int zerNRecs = zer->GetNRecs();
	if( zerNRecs == 0 )
	{
		delete zer;
		return 1;
	}  

	struct Zeri* zdat = new struct Zeri[zerNRecs];
	zer->ReadAll( zdat, zerNRecs );
	delete zer;

	// componenti attesi nel programma di assemblaggio
	int checkNumMaster = TPrgNormal->Count();
	int checkNum = checkNumMaster * (zerNRecs - 1);
	if( checkNum != TPrgExp->Count() )
	{
		delete [] zdat;
		W_Mess( MsgGetString(Msg_01887) );
		return 0;
	}
	//
	std::vector< std::string > checkComp;
	checkComp.resize( checkNum );

	int correct = 0;
	int ok = 1;

	for( int i = 0; i < TPrgExp->Count(); i++ )
	{
		struct TabPrg tdat;
		TPrgExp->Read( tdat, i );

		// controlla numero scheda (numerate a partire da 1)
		if( tdat.scheda < 1 || tdat.scheda >= zerNRecs )
		{
			W_Mess( MsgGetString(Msg_01887) ); //TODO: potrebbe esser fatto in automatico ???
			ok = 0;
			break;
		}

		// controlla duplicazione campi
		int checkIndex = checkNumMaster * (tdat.scheda-1);
		for( int j = 0; j < checkNumMaster; j++, checkIndex++ )
		{
			if( !checkComp[checkIndex].empty() )
			{
				if( checkComp[checkIndex].compare( tdat.CodCom ) == 0 )
				{
					ok = 0;
					break;
				}
			}
			else
			{
				checkComp[checkIndex] = std::string( tdat.CodCom );
				break;
			}
		}
		if( !ok )
		{
			W_Mess( MsgGetString(Msg_01887) );
			break;
		}


		// controlla incongruenza tra scheda nel file zeri e scheda in record programma assemblaggio
		if( (zdat[tdat.scheda].Z_ass && (tdat.status & NOMNTBRD_MASK)) || ( !zdat[tdat.scheda].Z_ass && !(tdat.status & NOMNTBRD_MASK)) )
		{
			if( !correct )
			{
				if( W_Deci(1,MsgGetString(Msg_01517)) )
				{
					correct = 1;
				}
				else
				{
					ok = 0;
					break;
				}
			}

			if(zdat[tdat.scheda].Z_ass)
			{
				tdat.status &= ~NOMNTBRD_MASK;
			}
			else
			{
				tdat.status |= NOMNTBRD_MASK;
			}

			TPrgExp->Write(tdat,i,FLUSHON);
		}
	}

	delete [] zdat;

	return ok;
}

//===========================================================================
int CaricHistogramMenu;


/*
-------------------------------------------------------------------------
SearchBestPosition
Cerca per un tipo componente la migliore posizione in configurazione caric.
Parametri di ingresso:
  tipocomp   : tipo componente per cui cercare la posiz. migliore
  allowed    : vettore dei caricatori nei quali e' possibile posizionare tipocomp
  _conf      : non utilizzato
Valori ritornati
  posizione trovata, -1 se nessuna posizione disponibile
-------------------------------------------------------------------------
*/
//SMOD131205: corretto numero totale record caricatori (=MAXCAR)
int CaricHistogram::SearchBestPosition(char *tipcomp,int *allowed,struct CarDat *_conf)
{
	float dist_min=1000000000;
	int dist_min_idx=-1;

	DelSpcR(tipcomp);

	//legge posizione caricatori di default
	SFeederDefault cardef[MAXCAR];
	FeedersDefault_Read( cardef );

	for(int j=0;j<MAXNREC_FEED;j++)
	{
		//controlla che la posizione sia disponibile
		if(!allowed[j])
			continue;

		float distance=0;

		//per la posizione in esame calcola la distanza totale tra componenti su scheda
		//e caricatore
		for(int i=0;i<nPrg;i++)
		{
			DelSpcR(AllPrg[i].TipCom);

			if((!strcasecmpQ(AllPrg[i].TipCom,tipcomp)) &&
			(!((AllPrg[i].status & NOMNTBRD_MASK) && ((AllPrg[i].status & MOUNT_MASK)))))
			{
				float dx=AllZer[AllPrg[i].scheda].Z_xzero+AllPrg[i].XMon-cardef[j].x;
				float dy=AllZer[AllPrg[i].scheda].Z_yzero+AllPrg[i].YMon-cardef[j].y;

				distance+=sqrt(dx*dx+dy*dy);
			}
		}

		//se distanza=minima: memorizza
		if(distance<dist_min)
		{
			dist_min=distance;
			dist_min_idx=j;  //nuova posizione migliore
		}
	}

	//ritorna posizione migliore o -1 se nessuna posizione trovata
	return(dist_min_idx);
}

/*
-------------------------------------------------------------------------
SearchBestMagPosition
Cerca la migliore posizione di un magazzino caricatori
Parametri di ingresso:
  nmag       : magazzino di cui ottimizzare la posizione
  allowed    : vettore dei caricatori nei quali e' possibile posizionare tipocomp
  _conf      : (opzionale) se specificato indica la struttura dati caricatore
               da utlizzare
Valori ritornati
  posizione trovata, -1 se nessuna posizione disponibile
-------------------------------------------------------------------------
*/
//SMOD131205: corretto numero totale record caricatori (=MAXCAR)
int CaricHistogram::SearchBestMagPosition(int nmag,int *allowed,struct CarDat *_conf/*=NULL*/)
{
  float dist_min=1000000000;
  int dist_min_idx=-1;

  //legge posizione caricatori di default
	SFeederDefault cardef[MAXCAR];
	FeedersDefault_Read( cardef );

  for(int j=0;j<MAXMAG;j++)
  {
    //controlla che la posizione sia disponibile
    if(!allowed[j])
    {
      continue;
    }

    float distance=0;

    //per la posizione in esame calcola la distanza totale tra i componenti
    //del magazzino (sulla scheda) e i caricatori corrispondenti
    for(int i=0;i<nPrg;i++)
    {
      if(AllPrg[i].Caric<160)
        MagUsed[AllPrg[i].Caric/10-1]++;

      if(((!(AllPrg[i].status & NOMNTBRD_MASK)) && (AllPrg[i].status & MOUNT_MASK))
         && ((AllPrg[i].Caric/10-1)==nmag))
      {
        int nf=(AllPrg[i].Caric % 10)-1;
        float dx=AllZer[AllPrg[i].scheda].Z_xzero+AllPrg[i].XMon-cardef[nf+j*8].x;
        float dy=AllZer[AllPrg[i].scheda].Z_yzero+AllPrg[i].YMon-cardef[nf+j*8].y;
      
        distance+=sqrt(dx*dx+dy*dy);
      }
    }

    //se distanza=minima: memorizza
    if(distance<dist_min)
    {
      dist_min=distance;
      dist_min_idx=j;  //nuova posizione migliore
    }
  }

  //ritorna posizione migliore o -1 se nessuna posizione trovata
  return(dist_min_idx);
}



/*
-------------------------------------------------------------------------
FindBestMagPosition
Calcola la migliore configurazione riposizionando i magazzini caricatori
Parametri di ingresso:
  nessuno
Valori ritornati
  nessuno
-------------------------------------------------------------------------
*/
//SMOD131205: corretto numero totale record caricatori (=MAXCAR)
int CaricHistogram::FindBestMagConfiguration(void)
{
  struct tUsedMag
  {
    int nmag;
    int nused;
  } UsedOrder[MAXMAG];

  int allowed[MAXMAG];


  //inizializza struttura per l'ordinamento dei magazzini in base al loro
  //utilizzo
  for(int i=0;i<MAXMAG;i++)
  {
    UsedOrder[i].nmag=i;
    UsedOrder[i].nused=MagUsed[i];
    allowed[i]=1;
  }

  struct CarDat newConf[MAXCAR];  

  //legge posizione caricatori di default
	SFeederDefault cardef[MAXCAR];
	FeedersDefault_Read( cardef );

  //inizializza nuova configurazione
  for(int i=0;i<MAXCAR;i++)
  {
    newConf[i].C_codice=GetCarCode(i);

    newConf[i].C_tipo   =CARTYPE_TAPE;
    newConf[i].C_att    =0;
    newConf[i].C_xcar   =cardef[i].x;
    newConf[i].C_ycar   =cardef[i].y;
    newConf[i].C_nx     =1;
    newConf[i].C_ny     =1;
    newConf[i].C_incx   =0;
    newConf[i].C_incy   =0;
    newConf[i].C_offprel=0;
    newConf[i].C_quant  =0;
    newConf[i].C_avan   =0;
    newConf[i].C_Ncomp  =1;
    newConf[i].C_PackIndex =0;
    newConf[i].C_Package[0]=NULL;
    newConf[i].C_note[0]=0;
    newConf[i].C_comp[0]=0;
  }  

  //ordina i magazzini in base al loro utilizzo in ordine crescente
  SortData((void *)UsedOrder,SORTFIELDTYPE_INT32,MAXMAG,(mem_pointer)&UsedOrder[0].nused-(mem_pointer)UsedOrder,sizeof(struct tUsedMag));

  int notUsed[MAXMAG];
  int count_notused=0;


  //parte dal magazzino piu utilizzato
  for(int i=MAXMAG-1;i>=0;i--)
  {
	//cerca per il magazzino la posizione migliore
	int found=SearchBestMagPosition(UsedOrder[i].nmag,allowed);

	if(found!=-1)
	{
		//rende la posizione trovata non disponibile per altri magazzini
		allowed[found]=0;

		//sposta il magazzino nella nuova posizione
		for(int j=0;j<8;j++)
		{
			newConf[j+found*8].C_att    =AllCar[j+UsedOrder[i].nmag*8].C_att;
			newConf[j+found*8].C_nx     =AllCar[j+UsedOrder[i].nmag*8].C_nx;
			newConf[j+found*8].C_ny     =AllCar[j+UsedOrder[i].nmag*8].C_ny;
			newConf[j+found*8].C_incx   =AllCar[j+UsedOrder[i].nmag*8].C_incx;
			newConf[j+found*8].C_incy   =AllCar[j+UsedOrder[i].nmag*8].C_incy;
			newConf[j+found*8].C_offprel=AllCar[j+UsedOrder[i].nmag*8].C_offprel;
			newConf[j+found*8].C_quant  =AllCar[j+UsedOrder[i].nmag*8].C_quant;
			newConf[j+found*8].C_avan   =AllCar[j+UsedOrder[i].nmag*8].C_avan;
			newConf[j+found*8].C_Ncomp  =AllCar[j+UsedOrder[i].nmag*8].C_Ncomp;
			newConf[j+found*8].C_tipo   =AllCar[j+UsedOrder[i].nmag*8].C_tipo;
			newConf[j+found*8].C_PackIndex =AllCar[j+UsedOrder[i].nmag*8].C_PackIndex;
			strcpy(newConf[j+found*8].C_Package,AllCar[j+UsedOrder[i].nmag*8].C_Package);
			strcpy(newConf[j+found*8].C_note,AllCar[j+UsedOrder[i].nmag*8].C_note);
			strcpy(newConf[j+found*8].C_comp,AllCar[j+UsedOrder[i].nmag*8].C_comp);
      }
    }
    else
    {
      //nessuna posizione trovata: magazzino non utilizzato      
      notUsed[count_notused++]=UsedOrder[i].nmag;
    }
  }

  count_notused=0;

  //riposiziona i caricatori non utilizzati
  for(int i=0;i<MAXMAG;i++)
  {
    if(allowed[i])
    {
      for(int j=0;j<8;j++)
      {
        newConf[j+i*8].C_att    =AllCar[j+notUsed[count_notused]*8].C_att;
        newConf[j+i*8].C_nx     =AllCar[j+notUsed[count_notused]*8].C_nx;
        newConf[j+i*8].C_ny     =AllCar[j+notUsed[count_notused]*8].C_ny;
        newConf[j+i*8].C_incx   =AllCar[j+notUsed[count_notused]*8].C_incx;
        newConf[j+i*8].C_incy   =AllCar[j+notUsed[count_notused]*8].C_incy;
        newConf[j+i*8].C_offprel=AllCar[j+notUsed[count_notused]*8].C_offprel;
        newConf[j+i*8].C_quant  =AllCar[j+notUsed[count_notused]*8].C_quant;
        newConf[j+i*8].C_avan   =AllCar[j+notUsed[count_notused]*8].C_avan;
        newConf[j+i*8].C_Ncomp  =AllCar[j+notUsed[count_notused]*8].C_Ncomp;
        newConf[j+i*8].C_tipo   =AllCar[j+notUsed[count_notused]*8].C_tipo;
        newConf[j+i*8].C_PackIndex =AllCar[j+notUsed[count_notused]*8].C_PackIndex;
        strcpy(newConf[j+i*8].C_Package,AllCar[j+notUsed[count_notused]*8].C_Package);
        strcpy(newConf[j+i*8].C_note,AllCar[j+notUsed[count_notused]*8].C_note);
        strcpy(newConf[j+i*8].C_comp,AllCar[j+notUsed[count_notused]*8].C_comp);
      }
      count_notused++;
    }
  }

	//lascia inalterati i vassoi
	for(int i=MAXNREC_FEED;i<MAXCAR;i++)
	{
		newConf[i]=AllCar[i];
	}


	CInputBox inbox( 0, 6, MsgGetString(Msg_01543), MsgGetString(Msg_01542), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	char newconf[9];
	snprintf( newconf, 9, "%s", inbox.GetText() );


	char newpathCar[MAXNPATH];

  CarPath(newpathCar,newconf);
  if(!access(newpathCar,F_OK))
  {
    char buf[80];
    snprintf(buf, sizeof(buf),OPTFEEDER_OVERWRITE,newconf);
    if(!W_Deci(0,buf))
      return 0;
  }

  FeedersConfig_Create( newconf );

  FeederFile* NewCar = new FeederFile(newconf);

  for(int i=0;i<MAXCAR;i++)
  {
    NewCar->SaveRecX(i,newConf[i]);
  }
  NewCar->SaveFile();

  delete NewCar;

  //chiede se si vuole caricare la nuova configurazione
  if(W_Deci(0,OPTFEEDER_ASKLOAD))
  {
    //si: aggiorna strutture dati
    strncpyQ(QHeader.Conf_Default,newconf,8);
    Mod_Cfg(QHeader);

    Save_PrgCFile(QHeader.Lib_Default,QHeader.Conf_Default);

    //esegue check di congruenza per associare la nuova configurazione
    //al programma
    TPrgFile* TPrgTmp = TPrg;

    TPrg = TPrgNormal;
    check_data(CHECKPRG_FULL);
    TPrg = TPrgTmp;

    if(curselected!=-1)
      SelectItem(curselected,0);

    //ricarica i dati di programma e configurazione
    LoadAllData();      

    //mostra il nuovo grafico
    int tmpcurgraph=curgraph;
    curgraph=-1;

    ShowGraph(tmpcurgraph);
  }

  return 1;
}


int CaricHistogram::ShowCarGraph()
{
	ShowGraph( 0 );
	return 1;
}

int CaricHistogram::ShowMagGraph()
{
	ShowGraph( 1 );
	return 1;
}


/*
-------------------------------------------------------------------------
FindBestMagPosition
Calcola la migliore configurazione riposizionando i caricatori
Parametri di ingresso:
  nessuno
Valori ritornati
  nessuno
-------------------------------------------------------------------------
*/
//SMOD131205: corretto numero totale record caricatori (=MAXCAR)
int CaricHistogram::FindBestConfiguration(void)
{
  struct tUsedOrder
  {
    int carrec;
    int ncomp;
  } UsedOrder[MAXNREC_FEED];

  for(int i=0;i<MAXNREC_FEED;i++)
  {
    if(AllCar[i].C_codice<160)
    {
      UsedOrder[i].carrec=i;
      UsedOrder[i].ncomp=CarUsed[i];
    }
  }

  SortData((void *)UsedOrder,SORTFIELDTYPE_INT32,MAXNREC_FEED,(mem_pointer)&UsedOrder[0].ncomp-(mem_pointer)UsedOrder,sizeof(struct tUsedOrder));

  int allowed[MAXCAR];
  struct CarDat newConf[MAXCAR];
  struct CarDat notUsed[MAXNREC_FEED];

  //legge caricatori di default
	SFeederDefault cardef[MAXCAR];
	FeedersDefault_Read( cardef );
  
  for(int i=0;i<MAXCAR;i++)
  {
    allowed[i]=1;
    newConf[i].C_codice=GetCarCode(i);

    newConf[i].C_tipo  =CARTYPE_TAPE;
    newConf[i].C_att    =0;
    newConf[i].C_xcar   =cardef[i].x;
    newConf[i].C_ycar   =cardef[i].y;
    newConf[i].C_nx     =1;
    newConf[i].C_ny     =1;
    newConf[i].C_incx   =0;
    newConf[i].C_incy   =0;
    newConf[i].C_offprel=0;
    newConf[i].C_quant  =0;
    newConf[i].C_avan   =0;
    newConf[i].C_Ncomp  =1;
    newConf[i].C_PackIndex =0;
    newConf[i].C_Package[0]=NULL;
    newConf[i].C_note[0]=0;
    newConf[i].C_comp[0]=0;
  }

  struct CarDat car;

  int count_notused=0;

  for(int i=MAXNREC_FEED-1;i>=0;i--)
  {
    car=AllCar[UsedOrder[i].carrec];

    int found=SearchBestPosition(car.C_comp,allowed,newConf);

    if(found==-1)
    {
      notUsed[count_notused++]=car;
      continue;
    }

    allowed[found]=0;

    newConf[found].C_att    =car.C_att;
    newConf[found].C_nx     =car.C_nx;
    newConf[found].C_ny     =car.C_ny;
    newConf[found].C_incx   =car.C_incx;
    newConf[found].C_incy   =car.C_incy;
    newConf[found].C_offprel=car.C_offprel;
    newConf[found].C_quant  =car.C_quant;
    newConf[found].C_avan   =car.C_avan;
    newConf[found].C_Ncomp  =car.C_Ncomp;
    newConf[found].C_tipo  =car.C_tipo;
    newConf[found].C_PackIndex =car.C_PackIndex;
    strcpy(newConf[found].C_Package,car.C_Package);
    strcpy(newConf[found].C_note,car.C_note);
    strcpy(newConf[found].C_comp,car.C_comp);

  }

  count_notused=0;

  //riposiziona i caricatori non utilizzati
  for(int i=0;i<MAXNREC_FEED;i++)
  {
    if(allowed[i])
    {
      newConf[i].C_att    =notUsed[count_notused].C_att;
      newConf[i].C_nx     =notUsed[count_notused].C_nx;
      newConf[i].C_ny     =notUsed[count_notused].C_ny;
      newConf[i].C_incx   =notUsed[count_notused].C_incx;
      newConf[i].C_incy   =notUsed[count_notused].C_incy;
      newConf[i].C_offprel=notUsed[count_notused].C_offprel;
      newConf[i].C_quant  =notUsed[count_notused].C_quant;
      newConf[i].C_avan   =notUsed[count_notused].C_avan;
      newConf[i].C_Ncomp  =notUsed[count_notused].C_Ncomp;
      newConf[i].C_tipo  =notUsed[count_notused].C_tipo;
      newConf[i].C_PackIndex =notUsed[count_notused].C_PackIndex;
      strcpy(newConf[i].C_Package,notUsed[count_notused].C_Package);
      strcpy(newConf[i].C_note,notUsed[count_notused].C_note);
      strcpy(newConf[i].C_comp,notUsed[count_notused].C_comp);

      newConf[i]=notUsed[count_notused];
      newConf[i].C_codice=GetCarCode(i);
      count_notused++;
    }
  }

	//lascia inalterati i vassoi
	for(int i=MAXNREC_FEED;i<MAXCAR;i++)
	{
		newConf[i]=AllCar[i];
	}


	CInputBox inbox( 0, 6, MsgGetString(Msg_01543), MsgGetString(Msg_01542), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	char newconf[9];
	snprintf( newconf, 9, "%s", inbox.GetText() );


	char newpathCar[MAXNPATH];

	CarPath(newpathCar,newconf);
	if(!access(newpathCar,F_OK))
	{
		char buf[80];
		snprintf(buf, sizeof(buf),OPTFEEDER_OVERWRITE,newconf);
		if(!W_Deci(0,buf))
			return 0;
	}

	FeedersConfig_Create( newconf );

	FeederFile* NewCar=new FeederFile(newconf);

	for(int i=0;i<MAXCAR;i++)
	{
		NewCar->SaveRecX(i,newConf[i]);
	}
	NewCar->SaveFile();

	delete NewCar;

	if(W_Deci(0,OPTFEEDER_ASKLOAD))
	{
		strncpyQ(QHeader.Conf_Default,newconf,8);
		Mod_Cfg(QHeader);
		Save_PrgCFile(QHeader.Lib_Default,QHeader.Conf_Default);

		TPrgFile *TPrgTmp=TPrg;

		TPrg=TPrgNormal;
		check_data(CHECKPRG_FULL);

		TPrg=TPrgTmp;

		if(curselected!=-1)
		SelectItem(curselected,0);

		LoadAllData();

		int tmpcurgraph=curgraph;
		curgraph=-1;
		ShowGraph(tmpcurgraph);
	}

	return 1;
}

//SMOD131205: corretto numero totale record caricatori (=MAXCAR)
int CaricHistogram::MoveCar()
{
	if(curselected==-1)
	{
		if(curgraph==0)
		{
			W_Mess( MsgGetString(Msg_01556) );
		}
		else
		{
			W_Mess( MsgGetString(Msg_01557) );
		}
		return 0;
	}

	if(((curselected>=MAXNREC_FEED) && (cur_caric<MAXNREC_FEED)) || ((curselected<MAXNREC_FEED) && (cur_caric>=MAXNREC_FEED)))
	{
		bipbip();
		return 0;
	}

  //legge caricatori di default
	SFeederDefault cardef[MAXCAR];
	FeedersDefault_Read( cardef );

  if(curgraph==0)
  {
    struct CarDat tmp=AllCar[curselected];
    AllCar[curselected]=AllCar[cur_caric];
    AllCar[cur_caric]=tmp;

    AllCar[curselected].C_codice=GetCarCode(curselected);
    AllCar[cur_caric].C_codice=GetCarCode(cur_caric);

    AllCar[cur_caric].C_xcar=cardef[cur_caric].x;
    AllCar[cur_caric].C_ycar=cardef[cur_caric].y;

    AllCar[curselected].C_xcar=cardef[curselected].x;
    AllCar[curselected].C_ycar=cardef[curselected].y;
    
    Car->SaveRecX(curselected,AllCar[curselected]);
    Car->SaveRecX(cur_caric,AllCar[cur_caric]);
  }
  else
  {
    for(int i=0;i<8;i++)
    {
      struct CarDat tmp=AllCar[curselected*8+i];
      AllCar[curselected*8+i]=AllCar[cur_caric*8+i];
      AllCar[cur_caric*8+i]=tmp;
      AllCar[curselected*8+i].C_codice=GetCarCode(curselected*8+i);
      AllCar[cur_caric*8+i].C_codice=GetCarCode(cur_caric*8+i);

      AllCar[cur_caric*8+i].C_xcar=cardef[cur_caric*8+i].x;
      AllCar[cur_caric*8+i].C_ycar=cardef[cur_caric*8+i].y;

      AllCar[curselected*8+i].C_xcar=cardef[curselected*8+i].x;
      AllCar[curselected*8+i].C_ycar=cardef[curselected*8+i].y;

      Car->SaveRecX(curselected*8+i,AllCar[curselected*8+i]);
      Car->SaveRecX(cur_caric*8+i,AllCar[cur_caric*8+i]);
    }
  }
  Car->SaveFile();

  //esegue check di congruenza per associare la nuova configurazione
  //al programma
  TPrgFile *TPrgTmp=TPrg;
    
  TPrg=TPrgNormal;
  check_data(CHECKPRG_FULL);

  TPrg=TPrgTmp;

  //ricarica i dati di programma e configurazione
  LoadAllData();

  int tmpcursor=cur_caric;

  if(curselected!=-1)
    SelectItem(curselected,0);

  //mostra il nuovo grafico
  int tmpcurgraph=curgraph;
  curgraph=-1;
  ShowGraph(tmpcurgraph);

  MoveCursorX(tmpcursor);
  
  return 1;
}

void CaricHistogram::ActivateMenu(void)
{
	GUI_SubMenu* Menu = new GUI_SubMenu();
	Menu->Add(OPTFEEDER_MENU2,0,NULL, boost::bind( &CaricHistogram::FindBestConfiguration, this ) );
	Menu->Add(OPTFEEDER_MENU3,0,NULL, boost::bind( &CaricHistogram::FindBestMagConfiguration, this ) );
	Menu->Add(OPTFEEDER_MENU4,0,NULL, boost::bind( &CaricHistogram::ShowCarGraph, this ) );
	Menu->Add(OPTFEEDER_MENU5,0,NULL, boost::bind( &CaricHistogram::ShowMagGraph, this ) );
	if( curgraph == 0)
		Menu->Add(OPTFEEDER_MENU6_1,0,NULL, boost::bind( &CaricHistogram::MoveCar, this ) );
	else
		Menu->Add(OPTFEEDER_MENU6_2,0,NULL,boost::bind( &CaricHistogram::MoveCar, this ) );

	Menu->Show();
	delete Menu;
}

void CaricHistogram::GestKey(int c)
{
  switch(c)
  {
    case K_LEFT:
      MoveCursorX(-1);
      break;
    case K_RIGHT:
      MoveCursorX(1);
      break;
    case K_CTRL_LEFT:
      MoveCursorX(-10);
      break;
    case K_CTRL_RIGHT:
      MoveCursorX(10);
      break;      
    case K_UP:
      MoveCursorY(1);
      break;
    case K_DOWN:
      MoveCursorY(-1);
      break;
    case K_CTRL_UP:
      MoveCursorY(10);
      break;
    case K_CTRL_DOWN:
      MoveCursorY(-10);
      break;
    case K_SPACE:
    case K_ENTER:
      if(curselected!=-1)
        SelectItem(curselected,0);
      ToggleSelectItem(cur_caric);
      break;
    case K_TAB:
    case K_ALT_M:
      ActivateMenu();
      break;
    case K_F2:
      FindBestConfiguration();
      break;
    case K_F3:
      FindBestMagConfiguration();
      break;
    case K_F4:
      ShowGraph(0);
      break;
    case K_F5:
      ShowGraph(1);
      break;
    case K_F6:
      MoveCar();
      break;
    default:
      bipbip();
      break;
  }
}


void CaricHistogram::MoveCursorX(int incX)
{
	GUI_Freeze_Locker lock;

	int maxcar = (curgraph==0) ? nCar : MAXMAG;

	if( (cur_caric+incX) >= 0 && (cur_caric+incX) < maxcar )
	{
		if( _vidbufX )
		{
			GUI_DrawSurface( PointI( gx1+int(scalex*cur_caric+2), gy1 ), _vidbufX );
			GUI_FreeSurface( &_vidbufX );
		}
		if( _vidbufY )
		{
			GUI_DrawSurface( PointI( gx1, gy2-int(scaley*cur_ypos) ), _vidbufY );
			GUI_FreeSurface( &_vidbufY );
		}

		cur_caric += incX;
		col = int(scalex*cur_caric+2);

		char buf[100];
		int txtY = py + h + 1;

		//pulisce
		if( Q_graph )
		{
			Q_graph->DrawPanel( RectI( 3, txtY, 65, 4 ), GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_CLIENTAREA ) );

			if( curgraph == 0 )
			{
				MoveCursorY(CarUsed[cur_caric]-cur_ypos);

				snprintf(buf, sizeof(buf),CARIC_HISTOGRAM_TXT1,AllCar[cur_caric].C_codice,AllCar[cur_caric].C_comp);
				Q_graph->DrawText( 3, txtY, buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_BLACK ) );
				snprintf(buf, sizeof(buf),CARIC_HISTOGRAM_TXT2,CarUsed[cur_caric]);
				Q_graph->DrawText( 3, txtY+1, buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_BLACK ) );

				if(CarUsed[cur_caric])
				{
					snprintf(buf, sizeof(buf),CARIC_HISTOGRAM_TXT3,CarDistance[cur_caric],CarDistance[cur_caric]/CarUsed[cur_caric]);
					Q_graph->DrawText( 3, txtY+2, buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_BLACK ) );
				}
			}
			else
			{
				MoveCursorY(MagUsed[cur_caric]-cur_ypos);

				snprintf(buf, sizeof(buf),MAG_HISTOGRAM_TXT1,cur_caric+1);
				Q_graph->DrawText( 3, txtY, buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_BLACK ) );
				snprintf(buf, sizeof(buf),CARIC_HISTOGRAM_TXT2,MagUsed[cur_caric]);
				Q_graph->DrawText( 3, txtY+1, buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_BLACK ) );

				if(MagUsed[cur_caric])
				{
					snprintf(buf, sizeof(buf),CARIC_HISTOGRAM_TXT3,MagDistance[cur_caric],MagDistance[cur_caric]/MagUsed[cur_caric]);
					Q_graph->DrawText( 3, txtY+2, buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( GR_BLACK ) );
				}
			}
		}
	}
}


void CaricHistogram::MoveCursorY(int incY)
{
	GUI_Freeze_Locker lock;

	if( (cur_ypos+incY) >= 0 && (cur_ypos+incY) <= max_carused )
	{
		if( _vidbufX )
		{
			GUI_DrawSurface( PointI( gx1+int(scalex*cur_caric+2), gy1 ), _vidbufX );
			GUI_FreeSurface( &_vidbufX );
		}
		if( _vidbufY )
		{
			GUI_DrawSurface( PointI( gx1, gy2-int(scaley*cur_ypos) ), _vidbufY );
			GUI_FreeSurface( &_vidbufY );
		}

		cur_ypos += incY;

		_vidbufX = GUI_SaveScreen( RectI( gx1+int(scalex*cur_caric+2), gy1, 1, gy2-gy1+1 ) );
		_vidbufY = GUI_SaveScreen( RectI( gx1, gy2-int(scaley*cur_ypos), gx2-gx1+1, 1 ) );

		row = int(scaley*cur_ypos);
		DrawCursors();
	}
}


int CaricHistogram::LoadAllData(void)
{
	if(AllPrg!=NULL)
	{
		delete[] AllPrg;
	}
	if(AllZer!=NULL)
	{
		delete[] AllZer;
	}

	nPrg=TPrg->Count();

	AllPrg=new struct TabPrg[nPrg];

	for(int i=0;i<nPrg;i++)
	{
		TPrg->Read(AllPrg[i],i);
	}

	char pathZer[MAXNPATH];

	PrgPath(pathZer,QHeader.Prg_Default,PRG_ZER);
	if(access(pathZer,F_OK))
	{
		W_Mess(NOZEROFILE);
		return 0;
	}

	ZerFile* Zer = new ZerFile(pathZer,ZER_NOADDPATH);
	if( !Zer->Open() )
	{
		W_Mess( NOZSCHFILE );
		delete Zer;
		return 0;
	}

	int nZer=Zer->GetNRecs();

	AllZer=new struct Zeri[nZer];
	for(int i=0;i<nZer;i++)
	{
		Zer->Read(AllZer[i],i);
	}
	delete Zer;

	char lib[9],conf[9];
	Read_PrgCFile(lib,conf);

	char pathCar[MAXNPATH];
	CarPath(pathCar,conf);

	if(Car!=NULL)
	{
		delete Car;
	}

	Car=new FeederFile(conf);
	nCar=MAXCAR;

	for(int i=0;i<nCar;i++)
	{
		Car->ReadRec(i,AllCar[i]);
		CarUsed[i]=0;
		CarDistance[i]=0;
	}

	for(int i=0;i<MAXMAG;i++)
	{
		MagUsed[i]=0;
		MagDistance[i]=0;
	}

  	return 1;
}

void CaricHistogram::CalcHistogram(struct TabPrg *AllPrg,struct CarDat *AllCar)
{
  max_carused=0;
  max_magused=0;

  for(int i=0;i<MAXCAR;i++)
  {
    CarUsed[i]=0;
    CarDistance[i]=0;
  }

  for(int i=0;i<MAXMAG;i++)
  {
    MagUsed[i]=0;
    MagDistance[i]=0;
  }

  for(int i=0;i<nPrg;i++)
  {
    if((!(AllPrg[i].status & NOMNTBRD_MASK)) && (AllPrg[i].status & MOUNT_MASK))
    {
      int carrec=GetCarRec(AllPrg[i].Caric);
      CarUsed[carrec]++;

      if(AllPrg[i].Caric<160)
      {
        MagUsed[AllPrg[i].Caric/10-1]++;
        if(MagUsed[AllPrg[i].Caric/10-1]>max_magused)
        {
          max_magused=MagUsed[AllPrg[i].Caric/10-1];
        }
      }

      float dx=AllZer[AllPrg[i].scheda].Z_xzero+AllPrg[i].XMon-AllCar[carrec].C_xcar;
      float dy=AllZer[AllPrg[i].scheda].Z_yzero+AllPrg[i].YMon-AllCar[carrec].C_ycar;

      CarDistance[carrec]+=sqrt(dx*dx+dy*dy);

      if(AllPrg[i].Caric<160)
      {
        MagDistance[AllPrg[i].Caric/10-1]+=CarDistance[carrec];
      }

      if(CarUsed[carrec]>max_carused)
      {
        max_carused=CarUsed[carrec];
      }

    }
  }

  max_carused+=10;
  max_magused+=10;
}

void CaricHistogram::ToggleSelectItem(int item)
{
  SelectItem(item,selected[curgraph][item] ^ 1);
}

void CaricHistogram::SelectItem(int item,int sel)
{
	int *data;

	if(curgraph==0)
	{
		data=CarUsed;
	}
	else
	{
		data=MagUsed;
	}

	int xstart=gx1+int(item*scalex);
	int xend=gx1+int((item+1)*scalex);

	int ypos=gy2-int(scaley*data[item]);

	selected[curgraph][item]=sel;


	if( _vidbufX )
	{
		GUI_DrawSurface( PointI( gx1+int(scalex*cur_caric+2), gy1 ), _vidbufX );
		GUI_FreeSurface( &_vidbufX );
	}
	if( _vidbufY )
	{
		GUI_DrawSurface( PointI( gx1, gy2-int(scaley*cur_ypos) ), _vidbufY );
		GUI_FreeSurface( &_vidbufY );
	}

	if(!sel)
	{
		curselected=-1;
		GUI_FillRect( RectI(xstart+1, ypos, xend-xstart-2, gy2-ypos), GUI_color(GR_YELLOW) );
	}
	else
	{
		curselected=item;
		GUI_Rect( RectI(xstart+1, ypos, xend-xstart-2, gy2-ypos), GUI_color(GR_BLACK) );
		GUI_FillRect( RectI(xstart+2, ypos, xend-xstart-4, gy2-ypos), GUI_color(GR_RED) );
	}

	_vidbufX = GUI_SaveScreen( RectI( gx1+int(scalex*cur_caric+2), gy1, 1, gy2-gy1+1 ) );
	_vidbufY = GUI_SaveScreen( RectI( gx1, gy2-int(scaley*cur_ypos), 1, 1 ) );

	DrawCursors();
}

void CaricHistogram::ShowGraph(int graphtype)
{
	if( graphtype == curgraph )
	{
		bipbip();
		return;
	}

	if(curselected!=-1)
		SelectItem(curselected,0);

	CalcHistogram(AllPrg,AllCar);

	curgraph = graphtype;

	if(!first)
	{
		ResetGraphData(1);
	}
	else
	{
		first=0;
	}

	SetVMinY(0);

	if(curgraph==0)
	{
		SetVMaxY(max_carused);
		SetNData(nCar,0);
		SetDataY(CarUsed,0);

		SetTitle( CARIC_HISTOGRAMTIT );
	}
	else
	{
		SetVMaxY(max_magused);
		SetNData(MAXMAG,0);
		SetDataY(MagUsed,0);

		SetTitle( MAG_HISTOGRAMTIT );
	}

	Show();

	cur_caric=0;
	cur_ypos=0;
	row=0;
	col=2;
	MoveCursorX(0);
	MoveCursorY(0);
}

CaricHistogram::CaricHistogram(void)
: C_Graph(CARIC_HISTOGRAMPOS,CARIC_HISTOGRAMTIT,GRAPH_NUMTYPEY_INT | GRAPH_AXISTYPE_NORMAL | GRAPH_DRAWTYPE_HISTOGRAM | GRAPH_NOSTART_SHOWCURSOR,1)
{
	max_carused=0;

	AllPrg=NULL;
	AllZer=NULL;
	Car=NULL;

	first=1;

	_vidbufX = 0;
	_vidbufY = 0;

	for(int i=0;i<2;i++)
	{
		for(int j=0;j<MAXCAR;j++)
		{
			selected[i][j]=0;
		}
	}

	curselected=-1;

	curgraph=-1;

	CaricHistogramMenu=-1;

	if(!LoadAllData())
	{
		return;
	}

	ShowGraph(0);

	int c = 0;
	while( c != K_ESC )
	{
		c = Handle();
		if(c!=K_ESC)
		{
			GestKey(c);
		}
	}

}

CaricHistogram::~CaricHistogram(void)
{
	if(AllPrg!=NULL)
	{
		delete[] AllPrg;
	}
	if(AllZer!=NULL)
	{
		delete[] AllZer;
	}

	if(Car!=NULL)
		delete Car;

	ReadRecs(CurRecord-CurRow);                    // reload dei record
	programTableRefresh = true;
}
