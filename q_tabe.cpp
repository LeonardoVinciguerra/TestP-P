/*
>>>> Q_TABE.CPP

Gestione dei files di programma.
Include obbligatorio -> Q_TABET.H.
L'header dei files di programma pu� essere letto con TYPE,
o stampato con PRINT, direttamente da DOS.
Es. PRINT *.QPF  -> Stampa su carta tutti gli header dei files trovati.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

-> Mod. 01.96 - Centratore
-> Modificato da WALTER 16.11.96 (Ric. automatica zero macchina)
-> Modificato da WALTER 02.04.1997 (bug in ASCCreate) ** W0204
-> Modificato da WALTER 02.04.1997 bug 'disk protect' ** W0204
-> Modif. da W1296 (mappatura assi) -> MP
-> Modif. da W2604 (mappatura offset teste) 26.04.97
-> Modif. da W0707 (nuova gestione ugelli)
-> Modif. da Walter 10/97 - W09
-> Modif. da Walter (packages) 05/98 - wmd0
*/

// For general functions
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

// For open, ecc.
#include <fcntl.h>
#include <math.h>
#include <fstream>

// For internal definitions
#include "msglist.h"
#include "filefn.h"
#include "q_cost.h"
#include "q_gener.h"
#include "q_progt.h"
#include "q_tabe.h"
#include "q_tabet.h"
#include "q_help.h"
#include "q_oper.h"
#include "q_filest.h"
#include "q_vision.h"
#include <unistd.h>  //GNU

#include <errno.h>

#include "q_snprt.h"
#include "q_net.h"
#include "q_inifile.h"
#include "q_prog.h" //Integr. Loris
#include "q_carint.h"
#include "q_files.h"
#include "strutils.h"
#include "fileutils.h"
#include "lnxdefs.h"

#include <fcntl.h>
#include <sstream>

#include <mss.h>

//DB220313
#ifdef __LOG_ERROR
#include "q_inifile.h"
#include "q_logger.h"
extern CLogger QuadraLogger;
#endif

#define MAX_QFILES_HEADER_SIZE 240

struct CfgTeste MapTeste;
struct CfgHeader QHeader;
struct CfgParam QParam;
struct vis_data Vision;
struct cur_data CurDat;


int    FOpenedNRecs=0;
struct FOpenItem *FOpenedList=NULL;
struct FOpenItem *FOpenedLast=NULL;

// Dichiarazione variabili public per il modulo.
int HandleTab=0; // File handle per prg. di montaggio.

int HandleCfg=0;    // File handle per config. programma.
int HandlePar=0;    // File handle per memor. parametri.
int HandleUge=0;    // File handle per memor. tabella ugelli.
int HandleMap=0;    // File handle per mappatura rotaz. teste.
int HandleOpt=0;    // File handle per parametri di ottimizzazione - DANY101002
int HandleLas=0;    // File handle per parametri del test hw laser - DANY191102
int HandleAsc=0;    // File handle per export in file ascii
int HandleDta=0;    // File handle per dati aggiuntivi programma - L0709
int HandleTlas=0;   // File handle per tipi O_RDWRalgoritmi laser >>S181201
int HandlePrgTab=0; // File handle per tabella programma ass. >>NEW S211201
int HandleAssTab=0;
int HandleCurDat=0; // File handle per dati correnti
int HandleBrush=0;  // File handle per dati brushless
int HandleConv=0;   // File handle per parametri convogliatore

int O_HandleCar=0;    // File handle per caricatori.
int O_HandlePrgTab=0; // File handle per tabella programma di assemblaggio

int HandleAxs=0; // File handle per tabella mappatura assi   W1296 MP
int HandleOff=0; // File handle per tabella mappatura offset teste W2604

int Coffset;   //   "     "   tabella caricatori, in bytes
int Tlasoffset; //  "     "   tabella dati tipi algoritmi laser in bytes >>S181201
int Prgoffset;      // Offset dati in file di programma >>NEW S280802
int Assoffset;      // Offset dati in file assemblaggio
int BrushOffset;    // Offset dati brushless
int O_Coffset;

struct map_off  O_off[17];          // struct. dati mappatura offset teste W2604
int off_file_error;                 // flag: se 1, errore in apertura/creazione file


// Create new files - Funz. di supporto alle create programma/caricatori.
// Ritorna 0 per creazione fallita.
/*  con gestione errore hardware per disco protetto **W0204 */
int F_File_Create ( int &N_Handle, const char *NomeFile , bool lock)
{
	int dsk_error=0;

	//SMOD300503 - LOG FILES HANDLE
	if ((N_Handle = FilesFunc_open(NomeFile, O_RDWR | O_CREAT | O_TRUNC,
	S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,false,lock)) == -1) dsk_error=1;

	if( dsk_error )
	{
		dsk_error=0;
		return 0;
	}

	return 1;
}

//ritrorna versione e subversion di un file Quadra Laser
//SMOD230703-bridge
int Get_FileVersion_OLD(int Handle,unsigned int &version,unsigned int &subversion)
{
	int curpos = lseek( Handle, 0, SEEK_CUR );
	
	char buf[HEADER_LEN+1];
	
	lseek(Handle,0,SEEK_SET);
	
	version = 0;
	subversion = 0;
	
	int nread=read(Handle,buf,HEADER_LEN);
	
	buf[HEADER_LEN]=0;
	
	if(nread!=HEADER_LEN)
	{
		//TODO: controllare errore -> crash del sistema
		/*
		char tmp[80];
		sprintf(tmp,"NRead Err %d %s",nread,buf);
		W_Mess(buf);
		*/
	}

	lseek(Handle,curpos,SEEK_SET);
	
	subversion=buf[HEADER_LEN-2] - '0';
	version=buf[HEADER_LEN-4] - '0';
	
	return 1;
}


/*--------------------------------------------------------------------------
Mostra messaggio di errore per versione file errata
--------------------------------------------------------------------------*/
//SMOD230703-bridge
void Show_FileVersionErr(const char *name)
{
	char buf[80];
	snprintf( buf, sizeof(buf),"%s %s\n", MsgGetString(Msg_01187) ,name);
	W_Mess(buf);
}


// Open files - Funz. di supporto alle open programma/caricatori.
// Ritorna 0 per open fallito.
int F_File_Open ( int &N_Handle, const char *NomeFile,int mode/*=CHECK_HEADER*/, bool ignore_case,bool lock)
{
	//SMOD300503 - LOG FILES HANDLE
	if ((N_Handle = FilesFunc_open(NomeFile, O_RDWR , 0, ignore_case,lock)) == -1)
	{
		//print_debug("File %s open failed !\nError number:%d\n",NomeFile,errno);
		return 0;
	}
	
	//printf("File open: %d - %s\n", N_Handle, NomeFile); //DB190712


	unsigned int subv;
	unsigned int vers;
	int  vers_ok=0;
	
	FList_AddItem(NomeFile);
	
	if(mode==CHECK_HEADER)
	{
		//SMOD230703-bridge
		if(Get_FileVersion_OLD(N_Handle,vers,subv))
		{
			if(vers == FILES_VERSION[0] -'0')
			{
				vers_ok=1;
			}
		}
	
		if(!vers_ok)
		{
			Show_FileVersionErr(NomeFile);
			return 0;
		}
	}

   return 1;
} // F_File_Open


//********************************************************************

// Esegue una copia del file in uso in CP_Name.
// Il file sorgente deve essere gi� aperto e associato a relativo handle.
// CP_File_Handle = Handle del file sorgente.
// Ritorna 1 per successo, 0 per copia non possibile.
//SMOD110403
int CopyFileOLD(const char *CP_Name, int CP_File_Handle)
{
	int CP_Handle,nb;
	
	if(!F_File_Create(CP_Handle,CP_Name)) return 0;
	
	char buf[1024*1024];

	while(1)
	{
		nb=read(CP_File_Handle,buf,sizeof(buf));
		
		if(nb>0)
		{
			int nw = write(CP_Handle,buf,nb);
			if(nw == -1)
			{
				FilesFunc_close(CP_Handle);
				return 0;
			}
		}
		else
		{
			FilesFunc_close(CP_Handle);
		
			if(nb==0)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
	}
}

//TODO: rifare usando la nuova CopyFile in fileutils.h
int CopyFileOLD(const char *fdest,const char *forig)
{
	bool ignore_src_case = false;

   int fo,fd;

   if(!F_File_Open(fo,forig,NOCHECK_HEADER,ignore_src_case))
   {
     return 0;
   }

   char buf[1024*1024];

   const char *ptr=strchr(fdest,':');

   ptr=fdest;
   buf[0]='\0';

   do
   {
     char *ptr2=strchr( (char*)ptr,'/');
     
     if(ptr2==NULL) 
     {
       break;
     }
	 
	 if(ptr2!=ptr)
	 {
		strncpyQ(buf+strlen(buf),ptr,ptr2-ptr);
	
		if(!CheckDirectory(buf))
		{
			mkdir(buf,DIR_CREATION_FLAG);
		}
	 }

     ptr=ptr2+1;

     strcat(buf,"/");

   } while(1);

   if(!F_File_Create(fd,fdest))
   {
     return 0;
   }

   do
   {
     int nb=read(fo,buf,sizeof(buf));

     if(nb!=0)
     {
       if(write(fd,buf,nb)!=nb)
       {
         FilesFunc_close(fo);
         FilesFunc_close(fd);
         return 0;
       }
     }
     else
     {
       break;
     }
   } while(1);

   FilesFunc_close(fo);
   FilesFunc_close(fd);

   return 1;
}


// END MOD.

//************************************************************************
// Gestione file tabella programma ##SMOD280802
//************************************************************************

TPrgFile::TPrgFile(const char *file,int tipo)
{ 
  if(tipo & PRG_NOADDPATH)
    strcpy(NomeFile,file);
  else
    PrgPath(NomeFile,file,tipo);  // nome con path  

  HandlePrgTab=0;
  Prgoffset=0;
  hdrUpdateOnWrite=1;
  SearchBuf=NULL;
}

TPrgFile::~TPrgFile(void)
{
  if(SearchBuf!=NULL)
    delete[] SearchBuf;
  Close();
}

//check se il file esiste su disco
int TPrgFile::IsOnDisk(bool ignore_case)
{
	return(!accessQ(NomeFile,F_OK,ignore_case));
}

//inizializza il buffer di ricerca (velocizza le operazioni di ricerca)
void TPrgFile::InitSearchBuf(void)
{
  if(SearchBuf!=NULL)
  {
    delete[] SearchBuf;
  }

  int open=0;

  if(HandlePrgTab==0)
  {
    Open(SKIPHEADER);
    open=1;
  }

  int n=Count();

  SearchBuf=new struct TabPrg[n];

  for(int i=0;i<n;i++)
  {
    Read(SearchBuf[i],i);
  }

  if(open)
  {
    Close();
  }

}

//dealloca buffer di ricerca
void TPrgFile::DestroySearchBuf(void)
{
  delete[] SearchBuf;
  SearchBuf=NULL;
}

//cerca un occorrenza di codice componente specificato nel programma
//DANY221102
int TPrgFile::Search(int start,struct TabPrg tab,int mode,int scheda)
{
	int n,i,ret=-1,open=0,c1,c2,c3;

	if(!mode)
		return(-1);

	if(SearchBuf==NULL)
	{
		if(HandlePrgTab==0)
		{
			Open(SKIPHEADER);
			open=1;
		}
	}

	n=Count();

	if(n<=start)
		return(-1);
  
	DelSpcR(tab.CodCom);
	DelSpcR(tab.TipCom);
	DelSpcR(tab.pack_txt);

	for(i=start;i<n;i++)
	{
		struct TabPrg intab;
		if(SearchBuf==NULL)
		{
			Read(intab,i);
		}
		else
		{
			intab=SearchBuf[i];
		}

		DelSpcR(intab.CodCom);
		DelSpcR(intab.TipCom);
		DelSpcR(intab.pack_txt);

		if(scheda!=0 && intab.scheda!=scheda)
			continue;

		c1=!strcasecmpQ(intab.CodCom,tab.CodCom) && (mode & PRG_CODSEARCH);
		c2=!strcasecmpQ(intab.TipCom,tab.TipCom) && (mode & PRG_TIPSEARCH);
		c3=!strcasecmpQ(intab.pack_txt,tab.pack_txt) && (mode & PRG_PACKSEARCH);

		if(c1 || c2 || c3)
		{
			ret=i;
			break;
		}
	}

	if( open )
	{
		Close();
	}

	return ret;
}

// Chiude il file di programma di montaggio in uso.  ##SMOD
int TPrgFile::Close(void)
{  
	if(HandlePrgTab)
	{ 
		FilesFunc_close(HandlePrgTab);
	  	HandlePrgTab=0;
     	return 1;
	}
	else
	{
		return 0;
	}
} // PrgClose

// Apre il file del programma di montaggio, con il nome specif (con pathname)
// OpenType: SKIPHEADER  =salta header e controlla dimensione file
//           NOSKIPHEADER=apre e posiziona indice ad inizio file (non controlla dimensione file)
// Ritorna 1 se ok, 0 se fail
int TPrgFile::Open(int OpenType,bool ignore_case)  //##SMOD
{  
	int PrgFileLen;
	int c=0;

   if(HandlePrgTab!=0)
   {
	   return 0;
   }

   Prgoffset=0;

   if(!F_File_Open(HandlePrgTab,NomeFile,CHECK_HEADER,ignore_case))  //apre file
   {
	   return 0;
   }

	ReadHeader(PrgHdr);
	
	lseek(HandlePrgTab,0,SEEK_SET);
	
	if(OpenType==SKIPHEADER)
	{ 
		while(c!=EOFVAL)                // skip file header
		{ 
			read(HandlePrgTab,&c,1);
		 	Prgoffset++;
			
			if(Prgoffset > MAX_QFILES_HEADER_SIZE)
			{
				Close();
				return 0;
			}
	  	}
	  	Prgoffset+=6;     // +2 per memo ultimo record modificato, dopo eof.
					        // +4 per memo dimensione file

		read(HandlePrgTab,&c,1);           // skip last record data
		read(HandlePrgTab,&c,1);
	
		read(HandlePrgTab,&PrgFileLen,4);     //read dimensione file
		if(filelength(HandlePrgTab)!=PrgFileLen)
		{		
			char buf[80];
			char *ptr=NomeFile;
			char *ptr_last=NULL;
		
			while((ptr=strchr(ptr,'/'))!=NULL)
			{
				ptr_last=ptr;
				ptr++;
			}

			if(ptr_last)
			{
				sprintf(buf,PRGCORRUPTED,ptr_last+1);
			}
			else
			{
				sprintf(buf,PRGCORRUPTED,"");
			}			

			if(W_Deci(1,buf))
			{
				lseek(HandlePrgTab,-4,SEEK_CUR);
				PrgFileLen=filelength(HandlePrgTab);
				write(HandlePrgTab,&PrgFileLen,4);
			}
     	}
	}
   	return 1;
}


// Ritorna la lunghezza del file programma aperto in byte.
int TPrgFile::FLen(void)
{ if(HandlePrgTab!=0)
    return(filelength(HandlePrgTab));
  else
    return(-1);
}// L_Prog

// Ritorna il n. di records contenuti nel file programma. ##SMOD
int TPrgFile::Count(void)
{ int dimobj=sizeof(struct TabPrg);

  if(HandlePrgTab==0)
     return(-1);
     
  return((int)(filelength(HandlePrgTab)-Prgoffset)/dimobj);
} // Count

// Read di un record, dato il suo numero.
// ritorna numero di byte letti o 0 per eof. ##SMOD
int TPrgFile::Read(TabPrg &ITab, int RecNum)
{  int rnumero = RecNum;                        // record num.
	int odim    = sizeof(ITab);                  // dimension of objet
	int rwpos;

	int numrecords;

   if(HandlePrgTab==0)
     return 0;

	numrecords=Count();
	if(numrecords<1) return 0;

	if(rnumero>=numrecords) return 0;           // eof
	rwpos = (rnumero*odim)+Prgoffset;  // r/w pointer in file

	lseek(HandlePrgTab,rwpos,SEEK_SET);

	int ret = read(HandlePrgTab,(char *)&ITab, sizeof(ITab));
	return(ret);
}

//aggiorna l'header ad ogni scrittura su file
void TPrgFile::UpdateHeader_OnWrite(int flag)
{
  hdrUpdateOnWrite=flag;
}

// Crea un nuovo file tipo programma vuoto
int TPrgFile::Create(void)
{
	short int Modific=NOREC;
	int filedim;

	if(!F_File_Create(HandlePrgTab,NomeFile))
     return 0;

   //strncpyQ(PrgHdr.F_nome,QHeader.Cli_Default,8);
	PrgHdr.F_nome[0] = 0;
   getdate_formatted(PrgHdr.F_aggi);
   InsSpc(40,PrgHdr.F_note);

   WriteHeader(PrgHdr);

   lseek(HandlePrgTab,0,SEEK_END);

   // n. ultimo rec. modif.
	write(HandlePrgTab,(char *) &Modific, sizeof(short int));
   //get dimensione file
   filedim=lseek(HandlePrgTab,0,SEEK_CUR)+sizeof(int);
   // dimensione file
	write(HandlePrgTab,(char*) &filedim, sizeof(int));

	FilesFunc_close(HandlePrgTab);
   HandlePrgTab=0;

	return 1;
} // PrgCreate

// Ritorna il n. dell'ultimo record modificato in file
// (legge da header). ##SMOD
int TPrgFile::FindLast(void)
{
	short int Ultimo=0;

	if(HandlePrgTab==0)
		return 0;

	lseek(HandlePrgTab,Prgoffset-6,SEEK_SET);
	read(HandlePrgTab,(char *) &Ultimo,sizeof(Ultimo));
	return(Ultimo);
} // FindMark


// Tronca il file programma di n record.  ##SMOD
void TPrgFile::Reduce(int n)
{  int size = filelength(HandlePrgTab);
   size-=n*sizeof(struct TabPrg); //new size

   if(HandlePrgTab==0)
     return;

   //scrive nuova dimensione file in header
	lseek(HandlePrgTab,Prgoffset-sizeof(int),SEEK_SET);
   write(HandlePrgTab,(char *)&size,sizeof(int));
   //modifica dimensione file
   chsize(HandlePrgTab,size);
}

// Write di un record, dato il suo numero, in file tabella programma.
// ritorna 1 se scrive, 0 per error (disk full?).
// Per  Mode == 1  append dei dati.
// Per Flush == 1  flush del buffer DOS dopo la scrittura
int TPrgFile::Replace(TabPrg &ITab, int RecNum, int Flush,int Mode)
{
	int finefile = 1, handle;
	int rwpos;
	int l_file=filelength(HandlePrgTab);
	
	rwpos = (RecNum*sizeof(ITab))+Prgoffset;  // r/w pointer in file
	
	if(HandlePrgTab==0)
	{
		return 0;
	}
	
	if(Flush)
	{
		handle = FilesFunc_dup(HandlePrgTab);
	}
	
	if(Mode==WR_APPEND)
	{
		lseek(HandlePrgTab,0L,SEEK_END);     // seek a fine file
	}
	else
	{
		lseek(HandlePrgTab,rwpos,SEEK_SET);  // seek for replace
	}
	
	if(write(HandlePrgTab, (char *)&ITab, sizeof(ITab)) < sizeof(ITab))
	{
		finefile=0;
	}
	
	l_file+=sizeof(ITab);
	
	if(Mode==WR_APPEND)                //se aggiunto un record
	{ 
		lseek(HandlePrgTab,Prgoffset-4,SEEK_SET); //aggiorna dimensione file in header
		write(HandlePrgTab,(char *)&l_file,4);
	}
	
	lseek(HandlePrgTab,Prgoffset-6,SEEK_SET);   //aggiorna in header ultimo rec. aggiornato
	write(HandlePrgTab,(char *)&RecNum,2);
	
	if(Flush)
	{
		FilesFunc_close(handle);  // flush DOS buffer
	}
	
	if(hdrUpdateOnWrite)     //se richiesto
	{
		getdate_formatted(PrgHdr.F_aggi);
		WriteHeader(PrgHdr);   //aggiorna header
	}
		
	return(finefile);
} // Replace

int TPrgFile::Write(TabPrg tab,int RecNum,int flush)
{ int n_recs=Count();     // n. ultimo rec lato
  int ret;
  if(RecNum<n_recs)
    ret=Replace(tab,RecNum,flush,WR_REPLACE);
  else
    ret=Replace(tab,RecNum,flush,WR_APPEND);
  return(ret);
}


// duplica la riga di programma dup_rec
void TPrgFile::DupRec(int dup_rec)
{

   if(HandlePrgTab==0)
     return;

   int n_recs=Count();     // n. ultimo rec lato
   struct TabPrg tab;
   int i;

   for(i=n_recs-1;i>=dup_rec;i--) //inserisce riga
   {
     Read(tab,i);
     tab.Riga++;
     Write(tab,i+1,FLUSHON);
   }

   Read(tab,dup_rec+1);
   *tab.CodCom=0; //DANY131202
   tab.status|=DUP_MASK;
   Write(tab,dup_rec+1);

}

//inserisce un record copiandoci la struttura definita in tab
void TPrgFile::InsRec(int ins_rec,struct TabPrg tab)
{
  if(HandlePrgTab==0)
  {
    return;
  }
    
  DupRec(ins_rec);
  tab.Riga=ins_rec+1;
  Write(tab,ins_rec);

}


// cancella tutti i record selezionati
//DANY261102
int TPrgFile::DelSel(void)
{
	struct TabPrg tabDat;
	int r_loop;
	int pos,hn,delcount;
	int Reccount=Count();

	pos=0;
   hn=0;
   delcount=0;

   for(r_loop=0;r_loop<Reccount;r_loop++)
  	{
     Read(tabDat,r_loop);
     if(tabDat.status & RDEL_MASK)//se riga da eliminare
     {
       if(hn==0)                  //se non ci sono righe libere
       {
         pos=r_loop-delcount;     //riposiziona puntatore a prima riga libera
       }
       hn++;                      //incrementa contatore righe libere
       delcount++;                //incrementa contaore elemeni eliminati

     }
     else
     {
       if(pos!=r_loop)             //se spostamento richiesto
       { tabDat.Riga=pos+1;
         Write(tabDat,pos);  //sposta record nella prima riga libera
         pos++;                    //prossima riga libera
         hn--;                     //decrementa numero di righe libere
       }
       else
       {
         pos=r_loop+1;
       }
       
     }
   }

   Reduce(delcount);      //modifica dimensione del file

   return(Reccount-delcount);

}

void TPrgFile::DelRec(int del_rec)
{
  if(HandlePrgTab==0)
  {
    return;
  }

  int i;
  struct TabPrg tab;
  
  Read(tab,del_rec);        //legge record da modificare

  //aggiorna dati su file, eliminando record del_rec
  i=del_rec+1;
  while(Read(tab,i))
  {
    tab.Riga--;
    Write(tab,i-1,FLUSHON);
    i++;
  }
  
  Reduce();                       //accorcia il file di 1 elemento

}


// Scambia due record
void TPrgFile::Change(int ch_src, int ch_dest)
{
   int src_offset, dest_offset;
   struct TabPrg tab_src,tab_dest;

	if(HandlePrgTab==0 || (ch_src==ch_dest)) return;

   src_offset=Prgoffset+(ch_src*sizeof(tab_src));
	dest_offset=Prgoffset+(ch_dest*sizeof(tab_dest));

	lseek(HandlePrgTab,src_offset,SEEK_SET);
	read(HandlePrgTab,(char*)&tab_src,sizeof(tab_src));

	lseek(HandlePrgTab,dest_offset,SEEK_SET);
	read(HandlePrgTab,(char*)&tab_dest,sizeof(tab_dest));

	lseek(HandlePrgTab,dest_offset,SEEK_SET);
   tab_src.Riga=tab_dest.Riga;
   write(HandlePrgTab,(char*)&tab_src,sizeof(tab_src));

	lseek(HandlePrgTab,src_offset,SEEK_SET);
   tab_dest.Riga=tab_src.Riga;
	write(HandlePrgTab,(char*)&tab_dest,sizeof(tab_dest));

}

//sposta un record
void TPrgFile::MoveRec(int nrec,int newpos)
{
   struct TabPrg tab_s,tab_d;

   if(HandlePrgTab==0 || (nrec==newpos)) return;

   Read(tab_s,nrec);
   Read(tab_d,newpos);

   DelRec(nrec);

   if(nrec<newpos)
     InsRec(newpos-1,tab_d);
   else
     InsRec(newpos,tab_d);

   tab_s.Riga=newpos+1;
   Write(tab_s,newpos,FLUSHON);

   if(nrec>newpos)
   {
     tab_d.Riga=newpos+2;
     Write(tab_d,newpos+1,FLUSHON);
   }

}



// Scrive il n. dell'ultimo record modificato, subito dopo il primo eof
// e prima dei dati di prg., con flush.
// Aggiorna anche la data L704
int TPrgFile::WriteLast(short int Ultimo)
{

   if(HandlePrgTab==0)
     return 0;

   int handle = FilesFunc_dup(HandlePrgTab);

	lseek(HandlePrgTab,Prgoffset-6,SEEK_SET);
	write(HandlePrgTab,(char *) &Ultimo,sizeof(Ultimo));
	FilesFunc_close(handle);  // flush DOS buffer
	return 1;
} // WriteLast


//scrive l'header del file programma aperto
void TPrgFile::WriteHeader(FileHeader F_Header)
{
	int eof=EOFVAL;
	
	if(HandlePrgTab==0)
	{
		return;
	}
	
	int tmppos=lseek(HandlePrgTab,0,SEEK_CUR);
	
	lseek(HandlePrgTab,0,SEEK_SET);
	
	int duphandle=FilesFunc_dup(HandlePrgTab);
	
	WRITE_HEADER(HandlePrgTab,FILES_VERSION,PRG_SUBVERSION); //SMOD230703-bridge  
	
	write(HandlePrgTab,(char *) TXTHD1,strlen(TXTHD1));
	write(HandlePrgTab,(char *) TXTHD2,strlen(TXTHD2));
	write(HandlePrgTab,(char *) F_Header.F_nome,strlen(F_Header.F_nome));

	write(HandlePrgTab,"\n\r",2);
	write(HandlePrgTab,(char *) TXTHD3,strlen(TXTHD3));
	write(HandlePrgTab,(char *) F_Header.F_aggi,10);
	write(HandlePrgTab,"\n\r",2);
	write(HandlePrgTab,(char *) TXTHD4,strlen(TXTHD4));
	
	char buf[41];

	//GF: buf adesso e' 38 (non 40 come prima) perche' sono stati aggiunti 2 byte alla data di ultima modifica
	strncpyQ(buf,F_Header.F_note,38);
	if(strlen(F_Header.F_note)<38)
		InsSpc(38-strlen(F_Header.F_note),buf+strlen(F_Header.F_note));

	write(HandlePrgTab,(char *) buf,38);

	write(HandlePrgTab,"\n\r",2);
	
	write(HandlePrgTab,&eof,1);              // fine header ascii - eof
	
	FilesFunc_close(duphandle);
	
	lseek(HandlePrgTab,tmppos,SEEK_SET);
}


// Legge header del file programma nella struttura.
void TPrgFile::ReadHeader(FileHeader &F_Header)
{
	if( !HandlePrgTab )
	{
		return;
	}

	char Letto = 0;
	while( Letto != 13 )
	{
		if(Letto == EOFVAL) // v. x.14 exit if EOF
		{
			return;
		}

		read( HandlePrgTab, &Letto, 1 );
	}

	for( int row = 0; row < 3; row++ )
	{
		int count = 0;
		char Buffer[45];

		// skip row title (length 13)
		read( HandlePrgTab, Buffer, 13 );

		// reset buffer
		memset( Buffer, 0, sizeof(Buffer) );

		Letto = 0;
		while( Letto != 13 )
		{
			if(Letto == EOFVAL) // v. x.14 exit if EOF
			{
				return;
			}

			if( Letto != 0 && Letto != '\n' )
			{
				if( count < sizeof(Buffer) - 1 )
				{
					Buffer[count++] = Letto;
				}
			}

			read( HandlePrgTab, &Letto, 1 );
		}

		Buffer[count] = '\0';

		switch( row )
		{ 
			case 0:
				strncpyQ( F_Header.F_nome, Buffer, 8 );
				break;
			case 1:
				strcpy( F_Header.F_aggi, Buffer );
				break;
			case 2:
				strcpy( F_Header.F_note, Buffer );
				break;
		}
	}
}

//ritorna l'header del programma
void TPrgFile::GetHeader(FileHeader &F_Header)
{
	if( !HandlePrgTab )
	{
		return;
	}

	F_Header = PrgHdr;
}


//**************************************************************************
// Gestione file zeri scheda
//**************************************************************************

/*--------------------------------------------------------------------------
Costruttore clase file zeri scheda
INPUT :  NomeFile: nome del file senza pathname
GLOBAL:	-
RETURN:  -
NOTE:    Non apre il file
--------------------------------------------------------------------------*/
ZerFile::ZerFile(char *NomeFile,int mode,bool _create_if_not_exixts)
{
	if(mode & ZER_ADDPATH)
	{
		strcpy(pathname,CLIDIR);
		strcat(pathname,"/");
		strcat(pathname,QHeader.Cli_Default);
		strcat(pathname,"/");
		strcat(pathname,PRGDIR);
		strcat(pathname,"/");
		strcat(pathname,NomeFile);
		strcat(pathname,ZEREXT);
	}
	else
	{
		strcpy(pathname,NomeFile);
	}
	
	create_if_not_exixts=_create_if_not_exixts;
	
	Offset1=0;
	Offset2=0;
	Handle=0;
}

ZerFile::~ZerFile(void)
{
	if(Handle)
		FilesFunc_close(Handle);
}

/*--------------------------------------------------------------------------
Crea file zeri scheda
INPUT :  NomeFile: nome del file senza pathname
GLOBAL:	-
RETURN:  0 se fail, 1 altrimenti.
NOTE:
--------------------------------------------------------------------------*/
int ZerFile::Create(void)
{
	char eof = EOFVAL;
	
	struct Zeri zer_dat;
	
	if(!F_File_Create(Handle,pathname))
	{
		return 0;
	}
	
	WRITE_HEADER(Handle,FILES_VERSION,ZERFILE_SUBVERSION); //SMOD230703-bridge   
	write(Handle,(char *)TXTZER,strlen(TXTZER));
	write(Handle,&eof,1);
	
	memset(&zer_dat,0,sizeof(struct Zeri));
	
	write(Handle,(char *)&zer_dat,sizeof(struct Zeri));
	write(Handle,&eof,1);
	
	FilesFunc_close(Handle);
	Handle=0;
	return 1;
}// ZerCreate


/*--------------------------------------------------------------------------
Apre il file zeri scheda.
INPUT :  NomeFile: nome del file da aprise (con pathname completo)
         OpenType: SKIPHEADER
                   NOSKIPHEADER
GLOBAL:	-
RETURN:  0 se fail, 1 altrimenti.
NOTE:
--------------------------------------------------------------------------*/
int ZerFile::Open(int OpenType,bool ignore_case)
{
	char c=0;
	int NoHeader=0;
	
	if(Handle)
	{
		return 0;              // File is already open
	}
	
	if(create_if_not_exixts && !IsOnDisk())           //se il file non esiste crealo
	{
		Create();
	}
	
	if(!F_File_Open(Handle,pathname,CHECK_HEADER,ignore_case))
	{
		return 0;
	}
	
	if(OpenType==SKIPHEADER)
	{
		while(c!=EOFVAL)                 // skip file header
		{
			read(Handle,&c,1);
			NoHeader++;
			
			if(NoHeader > MAX_QFILES_HEADER_SIZE)
			{
				Close();
				return 0;
			}
		}

		Offset1=NoHeader;           // offset dati zeri scheda
		Offset2=Offset1+sizeof(struct Zeri);
	}
	
	// Check versione ed in caso aggiornamento...
	unsigned int version, subversion;
	GetVersion(version, subversion);
	Update(version, subversion);

	return 1;
} // ZerOpen

/*--------------------------------------------------------------------------
Lunghezza file zeri scheda correntemente aperto.
INPUT :  -
GLOBAL:	-
RETURN:  Ritorna lunghezza file / 0 se file non aperto
NOTE:
--------------------------------------------------------------------------*/
int ZerFile::GetLen(void)
{
	if(Handle)
	{
		return(filelength(Handle));
	}
	else
	{
		return 0;
	}
}// L_Zero


/*--------------------------------------------------------------------------
Check esistenza file su disco
INPUT :  -
GLOBAL:	-
RETURN:  Ritorna 1 se file esiste / 0 altrimenti
NOTE:
--------------------------------------------------------------------------*/
int ZerFile::IsOnDisk(void)
{
	return(!access(pathname,F_OK));
}// L_Zero


/*--------------------------------------------------------------------------
Handle file zeri
INPUT :  -
GLOBAL:	-
RETURN:  Ritorna handle del file o 0 se file non aperto
NOTE:
--------------------------------------------------------------------------*/
int ZerFile::GetHandle(void)
{
	return(Handle);
}

/*--------------------------------------------------------------------------
Chiude il file zeri scheda
INPUT :  -
GLOBAL:	-
RETURN:  -
NOTE:
-------------------------------------------------------------------------*/
void ZerFile::Close(void)
{
	if(Handle!=0) //SMOD100504
	{
		FilesFunc_close(Handle);
	}
	
	Handle=0;
} // ZerClose

/*--------------------------------------------------------------------------
Conta i record presenti nel file zeri scheda
INPUT :  -
GLOBAL:	-
RETURN:  -
NOTE:
-------------------------------------------------------------------------*/
int ZerFile::GetNRecs(void)
{
	if(Handle)
		return((filelength(Handle)-Offset2)/sizeof(Zeri));
	else
		return 0;
} // Zcount


/*--------------------------------------------------------------------------
Modifica le dimensioni dell'area dati nel file zeri aperto.
INPUT :  Size: nuova dimensione
GLOBAL:	-
RETURN:  1 se OK / 0 se FAIL (file non aperto)
NOTE:
-------------------------------------------------------------------------*/
int ZerFile::Reduce(int Size)
{
	if(Handle)
	{
		chsize(Handle,Size+Offset2);
		return 1;
	}
	else
		return 0;

} // ZerReduce


// Write dei dati di intestazione del file di zeri
// ritorna 1 se scrive, 0 per error (disk full?).
// Per Flush == 1  flush del buffer DOS dopo la scrittura
int ZerFile::DtaWrite(Zeri dat,int Flush)
{
	int finefile = 1, h;
	
	if(Flush)
		h = FilesFunc_dup(Handle);
	
	lseek(Handle,Offset1,SEEK_SET);            // seek for replace
	
	if(write(Handle,(char *)&dat,sizeof(struct Zeri)) < sizeof(struct Zeri))
		finefile=0;
	if(Flush)
		FilesFunc_close(h);  // flush DOS buffer
	return(finefile);
}

// Read dei dati di intestazione del file di zeri
// ritorna 1 se legge, 0 per eof.
int ZerFile::DtaRead(Zeri &dat)
{
	int flag = 1;
	
	lseek(Handle,Offset1,SEEK_SET);
	if(read(Handle,(char *)&dat,sizeof(struct Zeri))==0)
		flag=0;
	return(flag);
}


// Write di un record, dato il suo numero, in file zeri scheda.
// ritorna 1 se scrive, 0 per error (disk full?).
int ZerFile::Write( Zeri& ZTab, int RecNum )
{
	int finefile = 1;
	int odim = sizeof(ZTab);                   // dimension of objet
	int rwpos = (RecNum*odim)+Offset2;               // r/w pointer in file

	int h = FilesFunc_dup(Handle);
	lseek(h,rwpos,SEEK_SET);            // seek for replace

	if( write(h, (char *)&ZTab, sizeof(ZTab)) < sizeof(ZTab) )
		finefile = 0;

	FilesFunc_close(h);

	return finefile;
}

// Read di un record, dato il suo numero, da tabella zeri scheda.
// ritorna 1 se legge, 0 per eof.
int ZerFile::Read(Zeri &ZTab, int RecNum) {
	int flag = 1;
	int rnumero = RecNum;                        // record num.
	int odim    = sizeof(ZTab);                  // dimension of objet
	int rwpos;
   int n;

	rwpos = (rnumero*odim)+(int)Offset2;        // r/w pointer in file

	lseek(Handle,rwpos,SEEK_SET);
   n=read(Handle,(char *)&ZTab, odim);
   if(n==0)
     flag=0;
	return(flag);
} // Zread

//legge tutti i record
int ZerFile::ReadAll(struct Zeri *zerList,int n)
{
  int floop=0;

  while(floop<n)
  {
    if(!Read(zerList[floop],floop))
      return 0;

    floop++;
  }

  return 1;

}

//ritorna il numero di board da assemblare
int ZerFile::GetNAssBoard(void)
{
  int n=GetNRecs();
  int i,nb=0;
  struct Zeri zer;
  
  for(i=0;i<n;i++)
  { Read(zer,i);
    if(zer.Z_ass)
      nb++;
  }
  return(nb);
}

void ZerFile::GetVersion(unsigned int &version, unsigned int &subversion)
{
	if( !Handle )
	{
		return;
	}

	Get_FileVersion_OLD(Handle, version, subversion);
}

void ZerFile::Update(unsigned int actVer, unsigned int actSubver)
{
	if( !Handle )
	{
		return;
	}

	if( actVer == 1 )
	{
		if( actSubver == 0 )
		{
			int curpos = lseek( Handle, 0, SEEK_CUR );

			// Mi metto ad inizio file e scrivo l'header con la nuova versione
			lseek(Handle,0,SEEK_SET);

			//WRITE_HEADER(Handle,FILES_VERSION,ZERFILE_SUBVERSION);

			lseek(Handle,0,SEEK_SET);

			Zeri tmpZeri;
			Read(tmpZeri,0);
			tmpZeri.Z_scalefactor = 1.0;
			Write(tmpZeri,0);

			// Mi rimetto dov'ero all'inizio
			lseek(Handle,0,SEEK_SET);
		}
	}
}


//**************************************************************************
// Gestione file configurazione convogliatore
//**************************************************************************

// Crea un nuovo file di configurazione convogliatore
int ConvCreate(void)
{
	char eof = EOFVAL;

	if(!F_File_Create(HandleConv,CONVNAME))
	{
		return 0;
	}
	WRITE_HEADER(HandleConv,FILES_VERSION,CONV_SUBVERSION); //SMOD230703-bridge
	write(HandleConv,(char *)TXTCONV,strlen(TXTCONV));
	write(HandleConv,&eof,1);

	struct conv_data convVal;
	memset( &convVal, 0, sizeof(struct conv_data) );

	convVal.enabled = 0;
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
	convVal.speed = CONV_SPEED_DEF;
	convVal.accDec = CONV_ACC_DEF;
	convVal.stepsMm = CONV_STEPMM_DEF;
	convVal.zero = CONV_ZEROIN_DEF;
	convVal.limit = CONV_LIMITIN_DEF;
	convVal.minCurr = CONV_MINCURR_DEF;
	convVal.maxCurr = CONV_MAXCURR_DEF;
	convVal.minPos = CONV_MINPOS_DEF;
	convVal.maxPos = CONV_MAXPOS_DEF;

	write( HandleConv, &convVal, sizeof(struct conv_data) );
	FilesFunc_close(HandleConv);
	return 1;
}


//---------------------------------------------------------------------------------
// Legge i parametri dal file di configurazione convogliatore
//---------------------------------------------------------------------------------
int Read_Conv( struct conv_data* convVal )
{
	char Letto=0;

	if(access(CONVNAME,0))
	{
		if(!ConvCreate())
		{
			return 0;
		}
	}

	if(!F_File_Open(HandleConv,CONVNAME))
	{
		return 0;
	}

	unsigned int spare,subversion;
	Get_FileVersion_OLD(HandleConv,spare,subversion);

	if(subversion!=CONV_SUBVERSION[0] - '0')
	{
		/*
		if(subversion<=MAPROT_PRESUBVERSION[0] - '0')
		{
			//UpgradeMapTeste(HandleMap,subversion);
		}
		else
		{
			Show_FileVersionErr(MAPNAME);
			FilesFunc_close(HandleMap);
			return 0;
		}
		*/
		Show_FileVersionErr(CONVNAME);
		FilesFunc_close(HandleConv);
		return 0;
	}

	int NoHeader=0;
	while(Letto!=EOFVAL)
	{
		read(HandleConv,&Letto,1);          // & init. lines
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleConv);
			HandleConv = 0;
			return 0;
		}
	}
	Letto=NULL;
	read(HandleConv,convVal,sizeof(struct conv_data));

	FilesFunc_close(HandleConv);
	return 1;
}

// Modifica i dati nel file di configurazione convogliatore
int Mod_Conv( struct conv_data convVal )
{
	char Letto=0;

	if(access(CONVNAME,0))
	{
		if(!ConvCreate())
		{
			return 0;
		}
	}

	if(!F_File_Open(HandleConv,CONVNAME))
	{
		return 0;
	}

	int NoHeader=0;
	while(Letto!=EOFVAL)
	{
		read(HandleConv,&Letto,1);         // & init. lines
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleConv);
			HandleConv = 0;
			return 0;
		}
	}
	Letto=NULL;

	write(HandleConv,&convVal,sizeof(struct conv_data));   // scrive dati
	FilesFunc_close(HandleConv);
	return 1;
}


//**************************************************************************
// Gestione file dati programma di assemblaggio (DAT)
//**************************************************************************

/*--------------------------------------------------------------------------
Crea un file dati programma
INPUT :  NomeFile: nome del file completo di pathname
RETURN:  0 se fail, 1 altrimenti
-------------------------------------------------------------------------*/
int DtaCreate(char *NomeFile)
{
	char eof = EOFVAL;

	if(!F_File_Create(HandleDta, NomeFile))
	{
		return 0;
	}

	WRITE_HEADER(HandleDta,FILES_VERSION,PRGDTA_SUBVERSION); //SMOD230703-bridge
	write(HandleDta,(char *)TXTDTA,strlen(TXTDTA));
	write(HandleDta,&eof,1);

	struct dta_data dtaVal;
	memset( &dtaVal, 0, sizeof(dtaVal) );

	write( HandleDta, &dtaVal, sizeof(dtaVal) );
	FilesFunc_close(HandleDta);

	HandleDta=0;
	return 1;
}

/*--------------------------------------------------------------------------
Apre un file dati programma
INPUT :  NomeFile: nome del file completo di pathname
         OpenType: SKIPHEADER
                   NOSKIPHEADER
RETURN:  0 se fail, 1 altrimenti
-------------------------------------------------------------------------*/
int DtaOpen( char *NomeFile, int OpenType = SKIPHEADER, bool ignore_case = false )
{
	int NoHeader = 0;
	char c=0;
	
	if(accessQ(NomeFile,0,ignore_case))
	{ 
		if(!DtaCreate(NomeFile))
		{
			return 0;
		}
	}
	
	if(!F_File_Open(HandleDta,NomeFile,CHECK_HEADER,ignore_case))
	{
		return 0;
	}

	if(!OpenType)
	{ 
		while(c!=EOFVAL)                 // skip file header
		{
			if (read(HandleDta,&c,1)==0)
			{
				break;
			}
			
			NoHeader++;
			
			if(NoHeader > MAX_QFILES_HEADER_SIZE)
			{
				FilesFunc_close(HandleDta);
				HandleDta = 0;
				return 0;
			}				
		}
	}
	
	//init della parte finale del file con zeri se la lunghezza del file e'
	//minore di quella della struttura dati
	if((filelength(HandleDta)-NoHeader)<sizeof(struct dta_data))
	{
		int n=sizeof(dta_data)-(filelength(HandleDta)-NoHeader);
	
		lseek(HandleDta,0,SEEK_END);
	
		int data=0;
		
		while(n>=0)
		{
			n--;
			write(HandleDta,&data,1);
		}
	}	
	
  	return 1;
}

/*--------------------------------------------------------------------------
Salva file dati programma
INPUT :  val : struttuta dati da salvare
-------------------------------------------------------------------------*/
void Save_Dta(struct dta_data val,char *nomefile/*=NULL*/)
{
	char X_NomeFile[MAXNPATH];

	if(nomefile==NULL)
	{
		PrgPath(X_NomeFile,QHeader.Prg_Default,2);       // nome con path
	}
	else
	{
		strcpy(X_NomeFile,nomefile);   
	}

	if(DtaOpen(X_NomeFile))
	{
		write(HandleDta, &val, sizeof(struct dta_data));
		FilesFunc_close(HandleDta);
	}
}


/*--------------------------------------------------------------------------
Legge file dati programma
INPUT :  val : struttuta dati da salvae
-------------------------------------------------------------------------*/
void Read_Dta(struct dta_data *val,char *nomefile/*=NULL*/)
{
	char X_NomeFile[MAXNPATH];

	if(nomefile==NULL)
	{
		PrgPath(X_NomeFile,QHeader.Prg_Default,2);       // nome con path
	}
	else
	{
		strcpy(X_NomeFile,nomefile);
	}
	
	if( DtaOpen(X_NomeFile) )
	{
		read(HandleDta, val, sizeof(struct dta_data));
		FilesFunc_close(HandleDta);
	}
}

/*--------------------------------------------------------------------------
Legge informazioni altezza PCB
RETURN:  ritorna in PCB, l'altezza PCB letta.
-------------------------------------------------------------------------*/
void Read_PCBh( float& PCBh, char* nomefile/*=NULL*/ )
{
	struct dta_data dtaVal;
	Read_Dta( &dtaVal, (char*)nomefile );

	PCBh = dtaVal.PCBh;
}

/*--------------------------------------------------------------------------
Salva punto colla iniziale
INPUT :  X_colla: punto coordinata X
         Y_colla: punto coordinata Y
-------------------------------------------------------------------------*/
#ifndef __DISP2
void Save_pcolla( float X_colla, float Y_colla, char* nomefile/*=NULL*/ )
#else
void Save_pcolla( int ndisp, float X_colla, float Y_colla, char* nomefile/*=NULL*/ )
#endif
{
	struct dta_data dtaVal;
	Read_Dta( &dtaVal, (char*)nomefile );

	#ifndef __DISP2
	dtaVal.dosa_x = X_colla;
	dtaVal.dosa_y = Y_colla;
	#else
	if( ndisp == 1 )
	{
		dtaVal.dosa_x = X_colla;
		dtaVal.dosa_y = Y_colla;
	}
	else
	{
		dtaVal.dosa_x2 = X_colla;
		dtaVal.dosa_y2 = Y_colla;
	}
	#endif

	Save_Dta( dtaVal, (char*)nomefile );
}

/*--------------------------------------------------------------------------
Legge posiziona punto colla
Legge informazioni altezza PCB
RETURN:  ritorna in X_colla= punto coordinata X
                    Y_colla= punto coordinata Y
-------------------------------------------------------------------------*/
#ifndef __DISP2
void Read_pcolla( float& X_colla, float& Y_colla, char* nomefile/*=NULL*/ )
#else
void Read_pcolla( int ndisp, float& X_colla, float& Y_colla, char* nomefile/*=NULL*/ )
#endif
{
	struct dta_data dtaVal;
	Read_Dta( &dtaVal, (char*)nomefile );

	#ifndef __DISP2
	X_colla = dtaVal.dosa_x;
	Y_colla = dtaVal.dosa_y;
	#else
	if(ndisp==1)
	{
		X_colla = dtaVal.dosa_x;
		Y_colla = dtaVal.dosa_y;
	}
	else
	{
		X_colla = dtaVal.dosa_x2;
		Y_colla = dtaVal.dosa_y2;
	}   
	#endif
}


/*--------------------------------------------------------------------------
Legge libreria package e configurazione caricatori associate al prg. di assemblaggio
RETURN:  ritorna in lib : nome libreria package
                    conf: nome configurazione caricatori
-------------------------------------------------------------------------*/
void Read_PrgCFile(char *lib,char *conf,char *nomefile/*=NULL*/)
{
	struct dta_data dtaVal;
	Read_Dta( &dtaVal, (char*)nomefile );

	strcpy( lib, dtaVal.lastlib );
	strcpy( conf, dtaVal.lastconf );
}

/*--------------------------------------------------------------------------
Salva libreria package e configurazione caricatori associate al prg. di assemblaggio
INPUT :  lib = nome libreria package
         conf= nome configurazione caricatori
-------------------------------------------------------------------------*/
void Save_PrgCFile(const char *lib,const char *conf,const char *nomefile/*=NULL*/)
{
	struct dta_data dtaVal;
	Read_Dta( &dtaVal, (char*)nomefile );

	strncpyQ( dtaVal.lastlib, lib, 8 );
	strncpyQ( dtaVal.lastconf, conf, 8 );
	dtaVal.lastlib[8] = '\0';
	dtaVal.lastconf[8] = '\0';

	Save_Dta( dtaVal, (char*)nomefile );
}


/*--------------------------------------------------------------------------
Salva numero board e componenti del programma di assemblaggio
INPUT :  nboard: numero di board da salvare nel file
         ncomp : numero di componenti da salvare nel file
         -
RETURN:  -
GLOBAL:	-
NOTE  :  -
-------------------------------------------------------------------------*/
void Save_nboard(int nboard,int ncomp,char *nomefile/*=NULL*/)
{
	struct dta_data dtaVal;
	Read_Dta( &dtaVal, (char*)nomefile );

	dtaVal.n_board = nboard;
	dtaVal.n_comp = ncomp;

	Save_Dta( dtaVal, (char*)nomefile );
}

/*--------------------------------------------------------------------------
Legge numero board e componenti del programma di assemblaggio
INPUT :  -
RETURN:  nboard: numero di board
         ncomp : numero di componenti
GLOBAL:	-
NOTE  :  -
-------------------------------------------------------------------------*/
void Read_nboard(int &nboard,int &ncomp,char *nomefile/*=NULL*/)
{
	struct dta_data dtaVal;
	Read_Dta( &dtaVal, (char*)nomefile );

	nboard = dtaVal.n_board;
	ncomp = dtaVal.n_comp;
}

/*--------------------------------------------------------------------------
Salva modalita' di riconoscimento fiduciali del programma di assemblaggio
INPUT :  mode: modalita' di riconoscimento
				MATCH_CORRELATION
				MATCH_VECTORIAL
RETURN:  -
GLOBAL:	-
NOTE  :  -
-------------------------------------------------------------------------*/
void Save_matchingMode(char mode,char *nomefile)
{
	struct dta_data dtaVal;
	Read_Dta( &dtaVal, (char*)nomefile );

	dtaVal.matching_mode = mode;

	Save_Dta( dtaVal, (char*)nomefile );
}

/*--------------------------------------------------------------------------
Legge modalita' di riconoscimento fiduciali del programma di assemblaggio
INPUT :  -
RETURN:  mode: modalita' di riconoscimento
GLOBAL:	-
NOTE  :  -
-------------------------------------------------------------------------*/
void Read_matchingMode(char &mode,char *nomefile)
{
	struct dta_data dtaVal;
	Read_Dta( &dtaVal, (char*)nomefile );

	mode = dtaVal.matching_mode;
}

//**************************************************************************
// Gestione file mappatura offset
//**************************************************************************

/*--------------------------------------------------------------------------
Salva tutti i dati relativi alla mappatura offset teste.
I dati sono memorizzati nel vettore O_off
-------------------------------------------------------------------------*/
void Update_off(void)
{
	char c=0;
	int i;
	
	if(off_file_error) 
	{
		return;
	}
	
	//SMOD300503 - LOG FILES HANDLE
	if(!F_File_Open(HandleOff,OFFMAP))
	{
		off_file_error=1;
		return;
	}
	
	int NoHeader=0;
	while(c!=EOFVAL)
	{
		// skip file header
		read(HandleOff,&c,1);
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleOff);
			HandleOff = 0;
			return;
		}		
	}
	
	for(i=0;i<=15;i++)
	{
		write(HandleOff,(char *)&O_off[i],sizeof(O_off[i]));
	}
	FilesFunc_close(HandleOff);
	HandleOff=0;
	off_file_error=0;
}

/*--------------------------------------------------------------------------
Legge i dati relativi alla mappatura dell'offset teste nel vettore O_off.
-------------------------------------------------------------------------*/
int Init_off(void)
{
	int i;
	char eof=EOFVAL;
	char c=0;
	
	if(access(OFFMAP,0))  // file not present
	{
		if(!F_File_Create(HandleOff,OFFMAP)) 
		{
			off_file_error=1;
			return 0;
		}
		write(HandleOff,(char *)TXTOFF,strlen(TXTOFF));
		write(HandleOff,&eof,1);
	
		for(i=0;i<16;i++) 
		{
			O_off[i].x=0;
			O_off[i].y=0;
			write(HandleOff,(char *)&O_off[i],sizeof(O_off[i]));
		}
		FilesFunc_close(HandleOff);
		HandleOff=0;
		off_file_error=0;
		return 1;
	}
	else
	{
		// access
		//SMOD300503 - LOG FILES HANDLE
		if(!F_File_Open(HandleOff,OFFMAP))
		{
			off_file_error=1;
			return 0;
		}

		int NoHeader=0;
		while(c!=EOFVAL)
		{                // skip file header
			read(HandleOff,&c,1);
			NoHeader++;
			if(NoHeader > MAX_QFILES_HEADER_SIZE)
			{
				FilesFunc_close(HandleOff);
				HandleOff = 0;
				off_file_error=1;
				return 0;
			}
		}

		for(i=0;i<16;i++)
		{
			read(HandleOff,(char *)&O_off[i], sizeof(O_off[i]));
		}
		FilesFunc_close(HandleOff);
		HandleOff=0;
		off_file_error=0;
	}
	return 1;
} // Init_off

/*--------------------------------------------------------------------------
Ritorna i dati di un elemento della mappatura offset
INPUT : posizione: indice nel vettore mappatura
RETURN:	valore_x : valore mappatura in x
        valore_y : valore mappatura in y
-------------------------------------------------------------------------*/
int Read_off(int &valore_x, int &valore_y, int posizione)
{
	if( off_file_error )
	{
		valore_x = 0;
		valore_y = 0;
		return 0;
	}

	posizione = MID( 0, posizione, 15 );

	valore_x = O_off[posizione].x;
	valore_y = O_off[posizione].y;

	return 1;
}

int Read_offmmN( float &valore_x, float &valore_y, int nozzle, float rot, int centering )
{
	while( rot >= 360 )
	{
		rot -= 360;
	}
	while( rot < 0 )
	{
		rot += 360;
	}

	int idx;

	// CALCOLO INDICE VETTORE DI MAPPATURA OFFSET TESTE
	if(rot>=45 && rot<135)
	{
		idx=1;
	}
	else if (rot>=135 && rot<225)
	{
		idx=2;
	}
	else if(rot>=225 && rot<315)
	{
		idx=3;
	}
	else
	{
		idx=0;
	}
	
	if( nozzle == 2 )
	{
		idx+=4;
	}
	
	if( centering == 1 ) //centraggio con telecamera esterna
	{
		idx+=8;
	}

	// lettura valori in passi per mappatura offset teste
	int x = 0, y = 0;
	int ret = Read_off( x, y, idx );

	valore_x = QHeader.PassoX * x;
	valore_y = QHeader.PassoY * y;
	
	return ret;
}


/*--------------------------------------------------------------------------
Modifica un elemento nel vettore delle mappature offset
INPUT :  posizione: indice nel vettore mappatura
         valore_x : valore mappatura in x
         valore_y : valore mappatura in y
NOTE  :  Non salva su file
-------------------------------------------------------------------------*/
void Mod_off(int posizione, int valore_x, int valore_y)
{
	if(posizione<0) posizione=0;
	if(posizione>15) posizione=15;
	if(off_file_error) return;
	O_off[posizione].x=valore_x;
	O_off[posizione].y=valore_y;
	return;
}

/*--------------------------------------------------------------------------
Resetta su file i dati della mappatura offset teste
-------------------------------------------------------------------------*/
void Reset_off(void)
{
	int i;
	for(i=0;i<=15;i++)
	{
		Mod_off(i,0,0);
	}
	Update_off();
}


//**************************************************************************
// Gestione file visione
//**************************************************************************

// Creazione di un nuovo file dati parametri visione Integr. Loris
//DANY140103
int VisCreate(char *NomeFile)
{
	int handle = 0;

	struct vis_data vdat;
	
	if(!F_File_Create(handle, NomeFile))
	{
		return 0;
	}
	
	WRITE_HEADER(handle,FILES_VERSION,VISPAR_SUBVERSION); //SMOD230703-bridge
	write(handle,(char *)TXTVIS,strlen(TXTVIS));
	
	//TODO: rivedere costanti
	vdat.mmpix_x = 0.020;      // costante millimetri pixel cam head
	vdat.mmpix_y = 0.020;
	
	vdat.match_err=1;           // errore max match in millimetri
	vdat.scale_off=0.5;         // setscale offset X e Y
	vdat.matchSpeedIndex = 1;   // velocita' match: media
	vdat.visionSpeedIndex = 1;  // velocita' visione: media
	vdat.wait_time=150;         // tempo attesa assi fermi
	vdat.image_time=150;        // tempo acquisizione intera immagine (2 frame)
	vdat.debug=0;               // debug visione
	vdat.pos_x = 0.f;
	vdat.pos_y = 0.f;
	vdat.mapoff_x=20;           // offset punti mappatura
	vdat.mapoff_y=20;
	vdat.mapnum_x=24;           // n. punti asse x mappatura
	vdat.mapnum_y=29;           // n. punti asse y mappatura
	
	vdat.scalaXCoord=0;
	vdat.scalaYCoord=0;
	
	// parametri riconoscimento pattern mappatura
	vdat.circleDiameter = 100;
	vdat.circleTolerance = 10;
	vdat.circleFSmoothDim = 15;
	vdat.circleFEdgeThr = 40;
	vdat.circleFAccum = 0.25f;

	//TODO - scegliere parametri di default
	// parametri riconoscimento pattern mappatura
	vdat.rectX = 300;
	vdat.rectY = 200;
	vdat.rectTolerance = 10;
	vdat.rectFSmoothDim = 15;
	vdat.rectFBinThrMin = 120;
	vdat.rectFBinThrMax = 255;
	vdat.rectFApprox = 500;

	write(handle, &vdat, sizeof(vdat));
	FilesFunc_close(handle);
	return 1;
}


// Caricamento file dati parametri visione Integr. Loris
bool VisDataLoad(struct vis_data &vdat)
{
	int handle = 0;
	char Letto=0;
	char NomeFile[24];
	
	strcpy(NomeFile,VISDIR);
	strcat(NomeFile,"/");
	strcat(NomeFile,VISDATAFILE); //##SMOD240902

   	if(access(NomeFile,0))
	{
		if(!VisCreate(NomeFile))
		{
			return false;
		}
   	}
	
   	if(!F_File_Open(handle,NomeFile))
	{
	   return false;
   	}

	int NoHeader=0;
	while(Letto!=EOFVAL)
	{
		read(handle,&Letto,1);
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(handle);
			handle = 0;
			return false;
		}
	}
	
	read(handle, &vdat, sizeof(vdat));
	
	FilesFunc_close(handle);
	return true;
}

// Salvataggio file dati parametri visione Integr. Loris
bool VisDataSave(struct vis_data vdat)
{
	int handle = 0;
	char NomeFile[24];
	
	strcpy(NomeFile,VISDIR);
	strcat(NomeFile,"/");
	strcat(NomeFile,VISDATAFILE);

	if(!F_File_Create(handle, NomeFile))
	{
		return false;
	}

	WRITE_HEADER(handle,FILES_VERSION,VISPAR_SUBVERSION); //SMOD230703-bridge   
	write(handle,(char *)TXTVIS,strlen(TXTVIS));

	write(handle, &vdat, sizeof(vdat));
	FilesFunc_close(handle);
	return true;
}


//**************************************************************************
// Gestione file parametri immagine
//**************************************************************************

//------------------------------------------------------------------------------------------
// Creazione di un nuovo file dati parametri immagine
//------------------------------------------------------------------------------------------
int ImgCreate( char* filename, img_data* data )
{
	int handle = 0;
	
	if(!F_File_Create(handle, filename))
	{
		return 0;
	}
	
	WRITE_HEADER(handle,FILES_VERSION,IMPGPAR_SUBVERSION); //SMOD230703-bridge
	write(handle,(char *)TXTIMG,strlen(TXTIMG));
	
	data->pattern_x=80;
	data->pattern_y=80;
	data->atlante_x=200;
	data->atlante_y=200;
	data->match_iter=3;
	data->match_thr=0.65;
	data->filter_type=0;
	data->filter_p1=4;
	data->filter_p2=15;
	data->filter_p3=1;
	data->contrast=50;
	data->bright=50;
	data->vect_diameter = DEF_DIAMETER_VALUE;
	data->vect_tolerance = DEF_TOLERANCE_VALUE;
	data->vect_smooth = DEF_SMOOTH_VALUE;
	data->vect_edge = DEF_EDGE_VALUE;
	data->vect_accumulator = DEF_ACCUMULATOR_VALUE;
	data->vect_atlante_x = DEF_SEARCHX_VALUE;
	data->vect_atlante_y = DEF_SEARCHY_VALUE;

	write(handle, data, sizeof(*data));
	FilesFunc_close(handle);
	return 1;
}

//------------------------------------------------------------------------------------------
// Caricamento file dati parametri immagine
//------------------------------------------------------------------------------------------
int ImgDataLoad( char* filename, img_data* data )
{
	int handle = 0;
	char Letto=0;
	
	if(access(filename,F_OK))
	{
		if(!ImgCreate(filename, data))
		{
			return 0;
		}
	}

	if(!F_File_Open(handle,filename))
	{
		return 0;
	}
	
	int NoHeader=0;
	while(Letto!=EOFVAL)
	{
		read(handle,&Letto,1);
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(handle);
			handle = 0;
			return 0;
		}
	}

	read( handle, data, sizeof(img_data) );

	FilesFunc_close(handle);

	// Check data fields
	// If never setted or out of range, set to default values!
	data->bright = ( ((data->bright < MIN_BRIGHT_VALUE) || (data->bright > MAX_BRIGHT_VALUE)) ? DEF_BRIGHT_VALUE : data->bright);
	data->contrast = ( ((data->contrast < MIN_CONTRAST_VALUE) || (data->contrast > MAX_CONTRAST_VALUE)) ? DEF_CONTRAST_VALUE : data->contrast);
	data->atlante_x = ( ((data->atlante_x < MIN_ATLANTE_VALUE) || (data->atlante_x > PKG_BRD_IMG_MAXX)) ? DEF_ATLANTE_VALUE : data->atlante_x);
	data->atlante_y = ( ((data->atlante_y < MIN_ATLANTE_VALUE) || (data->atlante_y > PKG_BRD_IMG_MAXY)) ? DEF_ATLANTE_VALUE : data->atlante_y);
	data->match_iter = ( ((data->match_iter < MIN_ITER_VALUE) || (data->match_iter > MAX_ITER_VALUE)) ? DEF_ITER_VALUE : data->match_iter);
	data->match_thr = ( ((data->match_thr < MIN_THR_VALUE) || (data->match_thr > MAX_THR_VALUE)) ? DEF_THR_VALUE : data->match_thr);
	data->pattern_x = ( ((data->pattern_x < PATTERN_MINX) || (data->pattern_x > PACK_PATTERN_MAXX)) ? DEF_PATTERN_VALUE : data->pattern_x);
	data->pattern_y = ( ((data->pattern_y < PATTERN_MINY) || (data->pattern_y > PACK_PATTERN_MAXY)) ? DEF_PATTERN_VALUE : data->pattern_y);
	data->filter_type = ( ((data->filter_type < MIN_FTYPE_VALUE) || (data->filter_type > MAX_FTYPE_VALUE)) ? DEF_FTYPE_VALUE : data->filter_type);
	data->filter_p1 = ( ((data->filter_p1 < MIN_FP1_VALUE) || (data->filter_p1 > MAX_FP1_VALUE)) ? DEF_FP1_VALUE : data->filter_p1);
	data->filter_p2 = ( ((data->filter_p2 < MIN_FP2_VALUE) || (data->filter_p2 > MAX_FP2_VALUE)) ? DEF_FP2_VALUE : data->filter_p2);
	data->filter_p3 = ( ((data->filter_p3 < MIN_FP3_VALUE) || (data->filter_p3 > MAX_FP3_VALUE)) ? DEF_FP3_VALUE : data->filter_p3);

	data->vect_diameter = ( ((data->vect_diameter < MIN_DIAMETER_VALUE) || (data->vect_diameter > MAX_DIAMETER_VALUE)) ? DEF_DIAMETER_VALUE : data->vect_diameter);
	data->vect_tolerance = ( ((data->vect_tolerance < MIN_TOLERANCE_VALUE) || (data->vect_tolerance > MAX_TOLERANCE_VALUE)) ? DEF_TOLERANCE_VALUE : data->vect_tolerance);
	data->vect_smooth = ( ((data->vect_smooth < MIN_SMOOTH_VALUE) || (data->vect_smooth > MAX_SMOOTH_VALUE)) ? DEF_SMOOTH_VALUE : data->vect_smooth);
	data->vect_edge = ( ((data->vect_edge < MIN_EDGE_VALUE) || (data->vect_edge > MAX_EDGE_VALUE)) ? DEF_EDGE_VALUE : data->vect_edge);
	data->vect_accumulator = ( ((data->vect_accumulator < MIN_ACCUMULATOR_VALUE) || (data->vect_accumulator > MAX_ACCUMULATOR_VALUE)) ? DEF_ACCUMULATOR_VALUE : data->vect_accumulator);
	data->vect_atlante_x = ( ((data->vect_atlante_x < MIN_SEARCHX_VALUE) || (data->vect_atlante_x > MAX_SEARCHX_VALUE)) ? DEF_SEARCHX_VALUE : data->vect_atlante_x);
	data->vect_atlante_y = ( ((data->vect_atlante_y < MIN_SEARCHY_VALUE) || (data->vect_atlante_y > MAX_SEARCHY_VALUE)) ? DEF_SEARCHY_VALUE : data->vect_atlante_y);

	return 1;
}

//------------------------------------------------------------------------------------------
// Salvataggio file dati parametri immagine
//------------------------------------------------------------------------------------------
int ImgDataSave( char* filename, img_data* data )
{
	int handle = 0;

	if(!F_File_Create(handle, filename))
	{
		return 0;
	}

	WRITE_HEADER(handle,FILES_VERSION,IMPGPAR_SUBVERSION); //SMOD230703-bridge
	write( handle, (char *)TXTIMG, strlen(TXTIMG) );
	write( handle, data, sizeof(img_data) );

	FilesFunc_close(handle);
	return 1;
}

//------------------------------------------------------------------------------------------
// Elimina file dati parametri immagine
//------------------------------------------------------------------------------------------
bool ImgDataDelete( char* filename )
{
	if( access( filename, F_OK ) )
	{
		return true;
	}

	remove( filename );
	return true;
}


//**************************************************************************
// Gestione file parametri di lavoro
//**************************************************************************

// Crea un nuovo file di configurazione parametri 2.
int ParCreate(void)
{
	char eof = EOFVAL;

	if(!F_File_Create(HandlePar,PARNAME))
	{
		return 0;
	}
	
	WRITE_HEADER(HandlePar,FILES_VERSION,PARAM_SUBVERSION); //SMOD230703-bridge
	write(HandlePar,(char *)TXTPAR,strlen(TXTPAR));
	write(HandlePar,&eof,1);
	FilesFunc_close(HandlePar);
	return 1;
}


// Modifica i dati nel file di configurazione e parametri 2.
int Mod_Par(CfgParam &H_Par)
{
	char Letto=0;
	if(access(PARNAME,0))
	{
		if(!ParCreate()) 
		{
			return 0;
		}
	}
	if(!F_File_Open(HandlePar,PARNAME)) 
	{
		return 0;
	}

	int NoHeader=0;
	while(Letto!=EOFVAL) 
	{
		read(HandlePar,&Letto,1);         // & init. lines
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandlePar);
			HandlePar = 0;
			return 0;
		}
	}
	Letto=NULL;
	write(HandlePar,(char *)&H_Par,sizeof(H_Par));   // scrive dati
	FilesFunc_close(HandlePar);
	return 1;
} // Mod_Par

// Legge i parametri di configurazione da file parametri 2.
int Read_Par(CfgParam &H_Par)
{
	char Letto=0;
	int len;

	if(access(PARNAME,0))
	{
		return 0;
	}
	
	if(!F_File_Open(HandlePar,PARNAME)) 
	{
		return 0;
	}
	
	int NoHeader=0;
	while(Letto!=EOFVAL)
	{
		read(HandlePar,&Letto,1);          // & init. lines
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandlePar);
			HandlePar = 0;
			return 0;
		}
	}
	Letto=NULL;
	len=read(HandlePar,(char *)&H_Par,sizeof(H_Par));
	if(len!=sizeof(H_Par))
	{
	
	}
	FilesFunc_close(HandlePar);

	//SMOD240907
	if(H_Par.N_try<=0)
	{
		H_Par.N_try=1;
	}
	
	Mod_Par(H_Par);

	return 1;
}


//**************************************************************************
// Gestione file configurazione
//**************************************************************************

#define UREF_OFF_X          0.0f
#define UREF_OFF_Y          8.0f

// Crea un nuovo file di configurazione programma e parametri 1.
int CfgCreate()
{
	char eof = EOFVAL;
	
	if(!F_File_Create(HandleCfg,CFGNAME))
	{
		return 0;
	}
	
	WRITE_HEADER(HandleCfg,FILES_VERSION,CONFIG_SUBVERSION); //SMOD230703-bridge
	write(HandleCfg,(char *)TXTCF1,strlen(TXTCF1));
	write(HandleCfg,&eof,1);
	
	struct CfgHeader Cfg;

	Cfg.Cli_Default[0]='\0';
	Cfg.Prg_Default[0]='\0';

	Cfg.Zero_Ugelli2=ZEROUGELLI_DEF;
	
	Cfg.debugMode1=DEBUG1_DEF;
	Cfg.debugMode2=DEBUG2_DEF;
	
	Cfg.interfNorm=INTERFERNOR_DEFAULT;
	Cfg.interfFine=INTERFFINE_DEF;
	
	Cfg.Conf_Default[0]='\0';
	Cfg.Lib_Default[0]='\0';

	Cfg.xyMaxSpeed = SPEED_AXES_DEF;
	Cfg.xyMaxAcc = ACC_AXES_DEF;
	Cfg.zMaxSpeed = SPEED_Z_DEF;
	Cfg.zMaxAcc = ACC_Z_DEF;
	Cfg.rotMaxSpeed = SPEED_ROT_DEF;
	Cfg.rotMaxAcc = ACC_ROT_DEF;

	Cfg.CurDosaConfig[0] = 0;
	Cfg.CurDosaConfig[1] = 0;
	
	Cfg.Dis_vuoto=TSCARICCOMP_DEF;
	Cfg.TContro_Comp=TCONTROCOMP_DEF;

	Cfg.PassoX=PASSOX_DEF;
	Cfg.PassoY=PASSOX_DEF;

	Cfg.D_vuoto=TREADVACUO_DEF;
	
	Cfg.Time_1=TIME1_DEF;
	Cfg.Time_2=TIME2_DEF;

	Cfg.modal=MODAL_DEF;

	Cfg.Enc_step1=ENCSTEP1_DEF;
	Cfg.Enc_step2=ENCSTEP1_DEF;
	
	Cfg.Step_Trasl1=KSTEP1_DEF;
	Cfg.Step_Trasl2=KSTEP2_DEF;
	
	Cfg.Zero_Piano=ZPIANO_DEF;
	Cfg.Zero_Caric=ZCARIC_DEF;
	Cfg.Zero_Ugelli=ZEROUGELLI_DEF;
	
	Cfg.InkZPos=ZINK_DEF;

	Cfg.uge_offs_x[0] = UREF_OFF_X;
	Cfg.uge_offs_y[0] = UREF_OFF_Y;
	Cfg.uge_offs_x[1] = UREF_OFF_X;
	Cfg.uge_offs_y[1] = UREF_OFF_Y;

	Cfg.DispVacuoGen_Delay=DISPVACUOGEN_DEF;

	#ifdef __SNIPER
	Cfg.Z12_Zero_delta=0;
	#endif

	Cfg.SoftPickDelta = 0;
	Cfg.softPickSpeedIndex = 0; // bassa

	Cfg.FoxPos_HeadSens[0]=0;
	Cfg.FoxPos_HeadSens[1]=0;

	Cfg.LaserOut=LASOUT_DEF;
	Cfg.TContro_Uge=TCONTROUGE_DEF;

	Cfg.zertheta_Prerot[0]=0;
	Cfg.zertheta_Prerot[1]=0;

	Cfg.brushMaxCurrent=BRUSHMAXCUR_DEF;
	Cfg.brushMaxCurrTime=BRUSHMAXCURTIME_DEF;

	Cfg.brushlessNPoles[0] = 2;
	Cfg.brushlessNPoles[1] = 2;

	Cfg.thoff_uge1=THOFFUGE_DEF;
	Cfg.thoff_uge2=THOFFUGE_DEF;

	Cfg.zth_level1=ZTHSEARCH_DEF;
	Cfg.zth_level2=ZTHSEARCH_DEF;

	#ifdef __SNIPER
	Cfg.sniper1_thoffset = 0;
	Cfg.sniper2_thoffset = 0;
	#endif
	
	Cfg.uge_wait=UGEWAIT_DEF;
	Cfg.uge_zacc=UGEZACC_DEF;
	Cfg.uge_zvel=UGEZVEL_DEF;
	Cfg.uge_zmin=UGEZSTART_DEF;
	
	Cfg.DMm_HeadSens[0]=0;
	Cfg.DMm_HeadSens[1]=0;

	Cfg.Max_NozHeight[0]=MAXNOZH1_DEF;
	Cfg.Max_NozHeight[1]=MAXNOZH2_DEF;

	Cfg.Min_NozHeight[0]=MINNOZH1_DEF;
	Cfg.Min_NozHeight[1]=MINNOZH2_DEF;

	Cfg.DStep_HeadSens[0]=0;
	Cfg.DStep_HeadSens[1]=0;

	Cfg.predownDelta=PREDOWNDELTA_DEF;

	Cfg.automaticPickHeightCorrection = -0.6f;

	memset( Cfg.spare1, 0, sizeof(Cfg.spare1) );
	Cfg.spare2 = 0;
	memset( Cfg.spare3, 0, sizeof(Cfg.spare3) );
	memset( Cfg.spare4, 0, sizeof(Cfg.spare4) );
	memset( Cfg.spare6, 0, sizeof(Cfg.spare6) );
	memset( Cfg.spare7, 0, sizeof(Cfg.spare7) );
	memset( Cfg.spare9, 0, sizeof(Cfg.spare9) );
	memset( Cfg.spare10, 0, sizeof(Cfg.spare10) );
	memset( Cfg.spare11, 0, sizeof(Cfg.spare11) );
	memset( Cfg.spare12, 0, sizeof(Cfg.spare12) );
	memset( Cfg.spare13, 0, sizeof(Cfg.spare13) );
	memset( Cfg.spare14, 0, sizeof(Cfg.spare14) );
	memset( Cfg.spare15, 0, sizeof(Cfg.spare15) );
	memset( Cfg.spare16, 0, sizeof(Cfg.spare16) );
	memset( Cfg.spare17, 0, sizeof(Cfg.spare17) );

	write(HandleCfg,(char *)&Cfg,sizeof(Cfg));   // scrive dati

	FilesFunc_close(HandleCfg);
	return 1;
}

void UpgradeConfig(int from_subversion,CfgHeader* cfg=NULL)
{
	char buf[MAXNPATH];
	
	bool cfg_was_null=false;
	
	CheckBackupDir();
	strcpy(buf,BACKDIR "/" CFGNAME);
	CopyFileOLD(buf,HandleCfg);
	
	if(cfg==NULL)
	{
		cfg=new CfgHeader;
		cfg_was_null=true;
		
		lseek(HandleCfg,0,SEEK_SET);
		
		int NoHeader=0;
		
		do
		{
			int c=0;
			read(HandleCfg,&c,1);
			if(c==EOFVAL)
			{
				break;
			}
			NoHeader++;
			if(NoHeader > MAX_QFILES_HEADER_SIZE)
			{
				FilesFunc_close(HandleCfg);
				HandleCfg = 0;
				return;
			}
		} while(1);
		
		read(HandleCfg,(char *)cfg,sizeof(*cfg));
	}

	if(from_subversion < CFG_FIRST_VERSION_MULTIPOLE_BRUSHLESS[0] - '0')
	{
		cfg->brushlessNPoles[0] = 2;
		cfg->brushlessNPoles[1] = 2;
	}

	if(from_subversion < CFG_FIRST_VERSION_DUAL_DISPENSER[0] - '0')
	{
		cfg->CurDosaConfig[0] = 0;
		cfg->CurDosaConfig[1] = 0;
		cfg->DispVacuoGen_Delay = DISPVACUOGEN_DEF;
	}
	
	lseek(HandleCfg,0,SEEK_SET);
	
	int eof=EOFVAL;
	
	WRITE_HEADER(HandleCfg,FILES_VERSION,CONFIG_SUBVERSION); //SMOD230703-bridge
	write(HandleCfg,(char *)TXTCF1,strlen(TXTCF1));
	write(HandleCfg,&eof,1);
	
	write(HandleCfg,(char *)cfg,sizeof(*cfg));   // scrive dati
	
	if(cfg_was_null)
	{
		delete cfg;
	}
	
	FilesFunc_close(HandleCfg);
	
	F_File_Open(HandleCfg,CFGNAME);

}


// Modifica i dati nel file di configurazione e parametri 1.
int Mod_Cfg(CfgHeader &H_Cfg)
{
	char Letto=0;
	if(access(CFGNAME,0))
   	{
		bipbip();
		W_Mess(NOCFGNAME);
		if (!W_Deci(0,ASKCFGCREATE))
      	{
			Set_OnFile(1);
			return 0;
		}
      	else
		{	
			if(!CfgCreate())
			{
				return 0;
			}
		}
	}

	if(!F_File_Open(HandleCfg,CFGNAME))
	{
		return 0;
	}

	//SMOD250703-START
	unsigned int spare,subversion;
	Get_FileVersion_OLD(HandleCfg,spare,subversion);
	
	if(subversion!=CONFIG_SUBVERSION[0] - '0')
	{
		if(subversion<=CONFIG_PRESUBVERSION[0] - '0')
		{
			UpgradeConfig(subversion,&H_Cfg);
		}
		else
		{
			Show_FileVersionErr(CFGNAME);
			FilesFunc_close(HandleCfg);
			return 0;
		}
	}
	//SMOD250703-END

	int NoHeader=0;
	while(Letto!=EOFVAL)
	{
		// skip title QPF
		read(HandleCfg,&Letto,1);         // & init. lines
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleCfg);
			HandleCfg = 0;
			return 0;
		}			
	}

	write(HandleCfg,(char *)&H_Cfg,sizeof(H_Cfg));   // scrive dati
	FilesFunc_close(HandleCfg);
	return 1;
} // Mod_Cfg

// Legge i parametri di configurazione da file.
int Read_Cfg(CfgHeader &H_Cfg)
{

	char Letto=0;

	if(access(CFGNAME,0))
	{
		bipbip();
		W_Mess(NOCFGNAME);
		if (!W_Deci(0,ASKCFGCREATE))
		{
			Set_OnFile(1);
			return 0;
		}
		else
		{
			if(!CfgCreate())
			return 0;
		}
	}

	if(!F_File_Open(HandleCfg,CFGNAME)) 
	{
		return 0;
	}

	//SMOD250703-START
	unsigned int spare,subversion;
	Get_FileVersion_OLD(HandleCfg,spare,subversion);
	
	if(subversion!=CONFIG_SUBVERSION[0] - '0')
	{
		if(subversion<=CONFIG_PRESUBVERSION[0] - '0')
		{
			UpgradeConfig(subversion);
		}
		else
		{
			Show_FileVersionErr(CFGNAME);
			FilesFunc_close(HandleCfg);
			return 0;
		}
	}
   //SMOD250703-END


	int offset=0;
	
	while(Letto!=EOFVAL)
	{                                    // skip title QPF
		offset++;
		read(HandleCfg,&Letto,1);         // & init. lines
	}
	Letto=NULL;
	read(HandleCfg,(char *)&H_Cfg,sizeof(H_Cfg));
	
	//SMOD270503
	if((H_Cfg.predownDelta<=0) || (H_Cfg.predownDelta>4))
	{
		H_Cfg.predownDelta=0;
	}
	
	FilesFunc_close(HandleCfg);
	
	if(H_Cfg.SoftPickDelta<0)
	{
		H_Cfg.SoftPickDelta=0;
		Mod_Cfg(H_Cfg);
	}

	return 1;
}// Read_Cfg



//**************************************************************************
// Gestione file mappatura rotazione
//**************************************************************************

// Crea un nuovo file di mappatura rotazione
int MapCreate(void)
{
	char eof = EOFVAL;

	if(!F_File_Create(HandleMap,MAPNAME))
	{
		return 0;
	}
	WRITE_HEADER(HandleMap,FILES_VERSION,MAPROT_SUBVERSION); //SMOD230703-bridge
	write(HandleMap,(char *)TXTMAP,strlen(TXTMAP));
	write(HandleMap,&eof,1);
	FilesFunc_close(HandleMap);
	return 1;
}


//---------------------------------------------------------------------------------
// Legge i parametri dal file mappatura rotazione
//---------------------------------------------------------------------------------
int Read_Map( CfgTeste& H_Map )
{
	char Letto=0;
	
	if(access(MAPNAME,0)) 
	{
		if(!MapCreate()) 
		{
			return 0;
		}
	}
	
	if(!F_File_Open(HandleMap,MAPNAME)) 
	{
		return 0;
	}
	
	unsigned int spare,subversion;
	Get_FileVersion_OLD(HandleMap,spare,subversion);
	
	if(subversion!=MAPROT_SUBVERSION[0] - '0')
	{
		if(subversion<=MAPROT_PRESUBVERSION[0] - '0')
		{
			//UpgradeMapTeste(HandleMap,subversion);
		}
		else
		{
			Show_FileVersionErr(MAPNAME);
			FilesFunc_close(HandleMap);
			return 0;
		}
	}		
	
	int NoHeader=0;
	while(Letto!=EOFVAL) 
	{
		read(HandleMap,&Letto,1);          // & init. lines
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleMap);
			HandleMap = 0;
			return 0;
		}			
	}
	Letto=NULL;
	read(HandleMap,(char *)&H_Map,sizeof(H_Map));		
	
	FilesFunc_close(HandleMap);
	return 1;
}

// Modifica i dati nel file di mappatura teste.
int Mod_Map(CfgTeste &H_Map)
{
	char Letto=0;

	if(access(MAPNAME,0))
	{
		if(!MapCreate())
		{
			return 0;
		}
	}

	if(!F_File_Open(HandleMap,MAPNAME))
	{
		return 0;
	}

	int NoHeader=0;
	while(Letto!=EOFVAL)
	{
		read(HandleMap,&Letto,1);         // & init. lines
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleMap);
			HandleMap = 0;
			return 0;
		}
	}
	Letto=NULL;

	write(HandleMap,(char *)&H_Map,sizeof(H_Map));   // scrive dati
	FilesFunc_close(HandleMap);
	return 1;
}


//**************************************************************************
//
//**************************************************************************

int CurData_Create(void)
{ 
	int eof=EOFVAL;

	int FHandle;
	struct cur_data CurData;
	if(!F_File_Create(FHandle,CURFNAME))
	{
		return 0;
	}
	
	WRITE_HEADER(FHandle,FILES_VERSION,CURDATA_SUBVERSION); //SMOD230703-bridge
	write(FHandle,(char *)TXTDEF,strlen(TXTDEF));
	write(FHandle,&eof,1);
	
	memset(&CurData,0,sizeof(CurData));
	
	CurData.HeadBright = 128*655;
	CurData.HeadContrast = 128*655;
	CurData.Cross=0;
	CurData.U_p1=-1;
	CurData.U_p1=-1;
	CurData.extcam_light=EXTCAM_LIGHT_DEFAULT;
	CurData.extcam_gain=EXTCAM_GAIN_DEFAULT;
	CurData.extcam_shutter=EXTCAM_SHUTTER_DEFAULT;
	
	
	write(FHandle,(char *)(&CurData),sizeof(CurData));
	
	FilesFunc_close(FHandle);

  	return 1;
}

int CurData_Open(void)
{ 	
	int c=0;

	if(HandleCurDat!=0)
	{
		return 0;
	}
	
	if(access(CURFNAME,0))    // file not present
	if(!CurData_Create())
		return 0;
	
	//SMOD300503 - LOG FILES HANDLE
	if(!F_File_Open(HandleCurDat,CURFNAME))
	{
		return 0;
	}
	
	int NoHeader=0;
	while(c!=EOFVAL)                 // skip file header
	{
		read(HandleCurDat,&c,1);
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleCurDat);
			HandleCurDat = 0;
			return 0;
		}			
	}
	
	return 1;
}

int CurData_Close(void)
{ 
	if(HandleCurDat==0)
	{
		return 0;
	}
	FilesFunc_close(HandleCurDat);
	HandleCurDat=0;
	return 1;
}

int CurData_Read(struct cur_data& Dat)
{
	if(!CurData_Open())
	{
		return 0;
	}

  	read(HandleCurDat,&Dat,sizeof(struct cur_data));

  	return(CurData_Close());
}

int CurData_Write(struct cur_data Dat)
{
	if(!CurData_Open())
	{
		return 0;
	}
	
	write(HandleCurDat,(char *)&Dat,sizeof(struct cur_data));
	
	return(CurData_Close());
}

//**************************************************************************
// Gestione file assemblaggio SMOD081002
//**************************************************************************
/*
// Read di un record, dato il suo numero.
// ritorna numero di byte letti o 0 per eof. ##SMOD
int AssPrg_Read(TabPrg &ITab, int RecNum)
{  int rnumero = RecNum;                        // record num.
	int odim    = sizeof(ITab);                  // dimension of objet
	int rwpos;

	int numrecords;
	numrecords=TPrg_Count();

	if(numrecords<1)
     return 0;

	if(rnumero>=numrecords)
     return 0;           // eof

	rwpos = (rnumero*odim)+Assoffset;  // r/w pointer in file
	lseek(HandleAssTab,rwpos,SEEK_SET);
	return(read(HandleAssTab,(char *)&ITab, sizeof(ITab)));
}

// Apre il file del programma di montaggio, con il nome specif (con pathname)
// OpenType: SKIPHEADER  =salta header e controlla dimensione file
//           NOSKIPHEADER=apre e posiziona indice ad inizio file (non controlla dimensione file)
// Ritorna 1 se ok, 0 se fail
int AssPrg_Open(char *NomeFile, int OpenType)
{  int PrgFileLen;
   Prgoffset=0;
	int c=0;

   if(HandleAssTab) return 0;              // File is already open

   if(!F_File_Open(HandleAssTab,NomeFile))  //apre file
     return 0;

   if(OpenType==SKIPHEADER)
{ while(c!=EOFVAL)                // skip file header
     { read(HandleAssTab,&c,1);
		 Prgoffset++;
	  }
   }
   return 1;
}

// Crea un nuovo file tipo programma vuoto
int AssPrg_Create(char *NomeFile)
{
	char eof = EOFVAL;

	short int Modific=NOREC;
	int filedim;

	if(!F_File_Create(HandleAssTab,NomeFile))
     return 0;

	write(HandleAssTab,(char *) TXTASSPRG,strlen(TXTASSPRG));
	write(HandleAssTab,(char*) &eof, 1);

	FilesFunc_close(HandleAssTab);

	HandleAssTab=0;

	return 1;
}

// Chiude il file di programma di montaggio in uso.  ##SMOD
void AssPrg_Close(void)
{  if(HandleAssTab)
	{ FilesFunc_close(HandleAssTab);
	  HandleAssTab=0;
   }
}

int AssPrg_Write(TabPrg &ITab, int Flush)
{

	char eof = EOFVAL;
	int handle,finefile=1;

	if(Flush)
     handle = FilesFunc_dup(HandleAssTab);

   lseek(HandleAssTab,0L,SEEK_END);     // seek a fine file

   if(write(HandleAssTab, (char *)&ITab, sizeof(ITab)) < sizeof(ITab))
     finefile=0;

   if(Flush)
     FilesFunc_close(handle);  // flush DOS buffer
     
	return(finefile);
}                                                                           */


//**************************************************************************
// Gestione file configurazione Brushless
//**************************************************************************

/*--------------------------------------------------------------------------
Crea un file configurazione Brusheless
INPUT :  NomeFile: nome del file completo di pathname
GLOBAL:	-
RETURN:  0 se fail, 1 altrimenti
NOTE  :  -
-------------------------------------------------------------------------*/
int BrushDataCreate(void)
{	
	char eof = EOFVAL;
	struct CfgBrush data;
	int Handle,i;

	if(!F_File_Create(Handle, BRUSHFNAME))
	{
		return 0;
	}

	WRITE_HEADER(Handle,FILES_VERSION,BRUSHDATA_SUBVERSION); //SMOD230703-bridge
	write(Handle,(char *)TXTBRUSH,strlen(TXTBRUSH));
	write(Handle,&eof,1);
	
	memset(&data,0,sizeof(struct CfgBrush));
	for(i=0;i<10;i++)
	{
		write(Handle,(char *)&data,sizeof(struct CfgBrush));
	}
	
	FilesFunc_close(Handle);
	return 1;
}//BrushDataCreate

/*--------------------------------------------------------------------------
Chiude il file di configurazione brushless (se aperto)
INPUT :  -
GLOBAL:	-
RETURN:  0 se fail, 1 altrimenti
NOTE  :  -
-------------------------------------------------------------------------*/

void BrushDataClose(void)
{ 
	if(HandleBrush)
	{
		FilesFunc_close(HandleBrush);
	}
	HandleBrush=0;
}

/*--------------------------------------------------------------------------
Apre il file di configurazione brushless
INPUT :  -
GLOBAL:	-
RETURN:  0 se fail, 1 altrimenti
NOTE  :  -
-------------------------------------------------------------------------*/
int BrushDataOpen(void)
{
	char c=0;
	
	if(HandleBrush)
	{
		return 1;
	}
	
	BrushOffset=0;
	
	if(access(BRUSHFNAME,F_OK))
	{ 
		if(!BrushDataCreate())
		{
			return 0;
		}
	}
	if(!F_File_Open(HandleBrush,BRUSHFNAME))
	{
		return 0;
	}
	
	// skip file header
	while(c!=EOFVAL)
	{ 
		read(HandleBrush,&c,1);
		BrushOffset++;
		if(BrushOffset > MAX_QFILES_HEADER_SIZE)
		{
			BrushDataClose();
			return 0;
		}			
	}
	
	//print_debug("bo=%d\n",BrushOffset);
	
	return 1;
} // BrushDataOpen


/*--------------------------------------------------------------------------
Salva dati configurazione brushless
INPUT :  data : dati da salvare
         n    : numero del record da salvare (0/9)
GLOBAL:	-
RETURN:  -
NOTE  :  -
-------------------------------------------------------------------------*/
void BrushDataSave(struct CfgBrush data,int n)
{
   if(n<0)
   {
	   n=0;
   }
   else
   {
	   if(n>9)
	   {
		   n=9;
	   }
   }

   BrushDataOpen();
   lseek(HandleBrush,BrushOffset+n*sizeof(CfgBrush),SEEK_SET);
   write(HandleBrush,&data,sizeof(struct CfgBrush));
   BrushDataClose();
}

/*--------------------------------------------------------------------------
Legge dati configurazione brushless
INPUT :  data : struttura dove inserire i dati letti
         n    : numero del record da leggere (0/9)
GLOBAL:	-
RETURN:  -
NOTE  :  -
-------------------------------------------------------------------------*/
void BrushDataRead(struct CfgBrush &data,int n)
{
	n = MID( 0, n, 9 );
	
	BrushDataOpen();
	lseek(HandleBrush,BrushOffset+n*sizeof(CfgBrush),SEEK_SET);
	read(HandleBrush,&data,sizeof(struct CfgBrush));
	BrushDataClose();
	
	int flag=0;
	
	if(data.clip[0]==0)
	{
		data.clip[0]=4096;
		flag=1;
	}
	
	if(data.clip[1]==0)
	{
		data.clip[1]=4096;
		flag=1;
	}
	
	if(flag)
	{
		BrushDataSave(data,n);
	}
}


/*--------------------------------------------------------------------------
Salva tutti i dati configurazione brushless
INPUT :  data : vettore dei dati da salvare
GLOBAL:	-
RETURN:  -
NOTE  :  -
-------------------------------------------------------------------------*/
void BrushDataSave(struct CfgBrush *data)
{
   BrushDataOpen();
   write(HandleBrush,data,sizeof(struct CfgBrush)*10);
   BrushDataClose();
}

/*--------------------------------------------------------------------------
Legge dati configurazione brushless
INPUT :  data : vettore dove inserire i dati letti
GLOBAL:	-
RETURN:  -
NOTE  :  -
-------------------------------------------------------------------------*/
void BrushDataRead(struct CfgBrush *data)
{
   BrushDataOpen();
   read(HandleBrush,data,sizeof(struct CfgBrush)*10);
   BrushDataClose();

   int flag=0;

   for(int i=0;i<10;i++)
   {
     if(data[i].clip[0]==0)
     {
       data[i].clip[0]=4096;
       flag=1;
     }
     if(data[i].clip[1]==0)
     {
       data[i].clip[1]=4096;
       flag=1;
     }
   }

   if(flag)
   {
     BrushDataSave(data);
   }
   
}

//**************************************************************************
// Gestione parametri test hw centraggio DANY191102-SMOD150903
//**************************************************************************

// Legge i valori dal file dei parametri del test laser
int Read_CentTest(CentTestParam& param)
{
	char Letto=0;
   
	if(access(CENTTESTNAME,0))
	{
		if(!Create_CentTest())
		{
			return 0;
		}
	}

	int Handle;
	
	if(!F_File_Open(Handle,CENTTESTNAME))
	{
		return 0;
	}
      
	int NoHeader=0;
	while(Letto!=EOFVAL)
	{
		// skip title
		read(Handle,&Letto,1);          // & init. lines
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(Handle);
			Handle = 0;
			return 0;
		}			
	}
	Letto=NULL;

	read(Handle,(char *)&param,sizeof(param));

	FilesFunc_close(Handle);

	return 1;
}// Read_Las

// Crea un nuovo file di parametri del test hw laser
int Create_CentTest(void)
{
	char eof = EOFVAL;
	
	int Handle;
	
	if(!F_File_Create(Handle,CENTTESTNAME))
	{
		return 0;
	}
	WRITE_HEADER(Handle,FILES_VERSION,TESTHWCENTR_SUBVERSION); //SMOD230703-bridge
	write(Handle,(char *)TXTCENTRTEST,strlen(TXTCENTRTEST));
	write(Handle,&eof,1);
	
	CentTestParam param;
	memset(&param,(char)0,sizeof(param));          // init dati
	write(Handle,(char *)&param,sizeof(param));   // scrive dati
	
	FilesFunc_close(Handle);
	
	return 1;
}// Create_Las


// Modifica i dati nel file dei parametri del test hw laser
int Mod_CentTest(const CentTestParam &param)
{
	char Letto=0;
	
	if(access(CENTTESTNAME,0))
	{
		if(!Create_CentTest())
		{
			return 0;
		}
	}
	
	int Handle;
	
	if(!F_File_Open(Handle,CENTTESTNAME))
	{
		return 0;
	}
	
	int NoHeader=0;
	while(Letto!=EOFVAL)
	{
		read(Handle,&Letto,1);         // & init. lines
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(Handle);
			Handle = 0;
			return 0;
		}			
	}
	Letto=NULL;
	
	write(Handle,(char *)&param,sizeof(param));   // scrive dati
	
	FilesFunc_close(Handle);
	
	return 1;
} // Mod_Las


//==========================================================================


int UgeDimHandle=0;     //SMOD141003
int UgeDim_HeaderPos=0; //SMOD141003

/*--------------------------------------------------------------------------
Crea file dimensioni ugelli
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    SMOD141003
--------------------------------------------------------------------------*/
int UgeDimCreate(void)
{
	if(!F_File_Create(UgeDimHandle,UGEDIM_FILE))
	{
		UgeDimHandle=0;
		return 0;
	}
	
	int eof=EOFVAL;
	
	WRITE_HEADER(UgeDimHandle,FILES_VERSION,UGEDIM_SUBVERSION);
	write(UgeDimHandle,(char *)TXTUGEDIM,strlen(TXTUGEDIM));
	write(UgeDimHandle,&eof,1);
	
	UgeDim_HeaderPos=lseek(UgeDimHandle,0,SEEK_CUR);
	
	struct CfgUgeDim data;
	
	data.name[0]='\0';
	data.a=0;
	data.b=0;
	data.c=0;
	data.d=0;
	data.e=0;
	data.f=0;
	data.g=0;
	memset(&data.spare1,0,10*sizeof(float));
	
	for(int i=0;i<MAXUGEDIM;i++)
	{
		data.index=i;
		UgeDimWrite(data,i);
	}
	
	UgeDimClose();
	return 0;
}

/*--------------------------------------------------------------------------
Apre il file dimensioni ugelli
INPUT:   -
GLOBAL:	-
RETURN:  0 se apertura fallita, 1 altrimenti
NOTE:    SMOD141003
--------------------------------------------------------------------------*/
int UgeDimOpen(void)
{
	if(UgeDimHandle)
	{
		return 1;
	}
	
	if(access(UGEDIM_FILE,F_OK))
	{
		UgeDimCreate();
	}
	
	if(!F_File_Open(UgeDimHandle,UGEDIM_FILE))
	{
		UgeDimHandle=0;
		return 0;
	}
	
	UgeDim_HeaderPos=0;
	
	char c;
	
	do
	{
		read(UgeDimHandle,&c,1);
		UgeDim_HeaderPos++;
		if(UgeDim_HeaderPos > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(UgeDimHandle);
			UgeDimHandle = 0;
			return 0;
		}				
	} while(c!=EOFVAL);

  return 1;

}

/*--------------------------------------------------------------------------
Chiude il file dimensioni ugelli
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    SMOD141003
--------------------------------------------------------------------------*/
int UgeDimClose(void)
{
  FilesFunc_close(UgeDimHandle);
  UgeDimHandle=0;
}

/*--------------------------------------------------------------------------
Scrive un record nel file dimensioni ugelli
INPUT:   data : record da scrivere
         n    : numero del record da scrivere
GLOBAL:	-
RETURN:  0 in caso di errore, 1 altrimenti
NOTE:    SMOD141003
--------------------------------------------------------------------------*/
int UgeDimWrite(CfgUgeDim data,int n)
{
  if(!UgeDimHandle)
    return 0;

  lseek(UgeDimHandle,UgeDim_HeaderPos+n*sizeof(CfgUgeDim),SEEK_SET);

  if(write(UgeDimHandle,&data,sizeof(CfgUgeDim))!=sizeof(CfgUgeDim))
    return 0;
  else
    return 1;
}

/*--------------------------------------------------------------------------
Legge un record dal file dimensioni ugelli
INPUT:   data : record dove inserire il dato letto
         n    : numero del record da leggere
GLOBAL:	-
RETURN:  0 in caso di errore, 1 altrimenti
NOTE:    SMOD141003
--------------------------------------------------------------------------*/
int UgeDimRead(CfgUgeDim &data,int n)
{
  if(!UgeDimHandle)
    return 0;

  lseek(UgeDimHandle,UgeDim_HeaderPos+n*sizeof(CfgUgeDim),SEEK_SET);

  if(read(UgeDimHandle,&data,sizeof(CfgUgeDim))!=sizeof(CfgUgeDim))
    return 0;
  else
    return 1;

}

/*--------------------------------------------------------------------------
Scrive tutti i record nel  file dimensioni ugelli
INPUT:   data : vettore dei record da scrivere
GLOBAL:	-
RETURN:  0 in caso di errore, 1 altrimenti
NOTE:    SMOD141003
--------------------------------------------------------------------------*/
int UgeDimWriteAll(CfgUgeDim *data)
{
  if(!UgeDimHandle)
    return 0;

  lseek(UgeDimHandle,UgeDim_HeaderPos,SEEK_SET);

  if(write(UgeDimHandle,data,sizeof(CfgUgeDim)*MAXUGEDIM)!=sizeof(CfgUgeDim)*MAXUGEDIM)
    return 0;
  else
    return 1;
  
}

/*--------------------------------------------------------------------------
Legge tutti i record dal file dimensioni ugelli
INPUT:   data : vettore di destinazione dei record letti
GLOBAL:	-
RETURN:  0 in caso di errore, 1 altrimenti
NOTE:    SMOD141003
--------------------------------------------------------------------------*/
int UgeDimReadAll(CfgUgeDim *data)
{
  if(!UgeDimHandle)
    return 0;

  lseek(UgeDimHandle,UgeDim_HeaderPos,SEEK_SET);

  if(read(UgeDimHandle,data,sizeof(CfgUgeDim)*MAXUGEDIM)!=sizeof(CfgUgeDim)*MAXUGEDIM)
    return 0;
  else
    return 1;
}

int PackVis_Handle=0;
int PackVis_HeaderPos=0;

void PackVisData_GetFilename( char* filename, int pack_code, char* libname )
{
	strcpy( filename, VISPACKDIR );
	strcat( filename, libname );
	DelSpcR( filename );
	sprintf( filename, "%s/%d", filename, pack_code );
	strcat( filename, VISPACK_EXT );
}

/*--------------------------------------------------------------------------
Elimina file dati immagine package
INPUT:   path : pathname completo del file da eliminare
GLOBAL:	-
RETURN:  -
NOTE:    SMOD180304
--------------------------------------------------------------------------*/
int PackVisData_Remove( int pack_code, char* libname )
{
	char NameFile[MAXNPATH+1];
	PackVisData_GetFilename( NameFile, pack_code, libname );

	if(!access(NameFile,F_OK))
	{
		return remove(NameFile);
	}

	return 1;
}


/*--------------------------------------------------------------------------
Crea file dati immagine package
INPUT:   path : pathname completo del file da creare
GLOBAL:	-
RETURN:  -
NOTE:    SMOD180304
--------------------------------------------------------------------------*/
int PackVisData_Create(char *path,char *name)
{
	if(!F_File_Create(PackVis_Handle,path))
	{
		PackVis_Handle=0;
		return 0;
	}
	
	int eof=EOFVAL;
	
	WRITE_HEADER(PackVis_Handle,FILES_VERSION,PACKVIS_SUBVERSION);
	write(PackVis_Handle,(char *)TXTPACKVIS,strlen(TXTPACKVIS));
	write(PackVis_Handle,&eof,1);
	
	PackVis_HeaderPos=lseek(PackVis_Handle,0,SEEK_CUR);
	
	struct PackVisData d;

	memset(&d,(char)0,sizeof(PackVisData));

	d.light[0] = PKVIS_LIGHT_DEF;
	d.light[1] = PKVIS_LIGHT_DEF;
	d.shutter[0] = PKVIS_SHUTTER_DEF;
	d.shutter[1] = PKVIS_SHUTTER_DEF;

	d.match_thr = PKVIS_MATCH_THR_DEF;
	d.niteraz = PKVIS_ITERAZ_DEF;
	
	strncpyQ(d.name,name,20);
	
	PackVisData_Write(d);
	
	FilesFunc_close(PackVis_Handle);
	
	PackVis_Handle=0;
	
	return 0;

}

/*--------------------------------------------------------------------------
Apre il file dati immagine package
INPUT:   path : pathname completo del file da aprire
GLOBAL:	-
RETURN:  0 se apertura fallita, 1 altrimenti
NOTE:    SMOD180304
--------------------------------------------------------------------------*/
int PackVisData_Open( char* path, char* name )
{
	if(PackVis_Handle)
	{
		return 1;
	}
	
	if(access(path,F_OK))
	{
		if(name!=NULL)
		{
			PackVisData_Create(path,name);
		}
		else
		{
			return 0;
		}
	}
	
	if(!F_File_Open(PackVis_Handle,path))
	{
		PackVis_Handle=0;
		return 0;
	}
	
	PackVis_HeaderPos=0;
	
	char c;
	
	do
	{
		read(PackVis_Handle,&c,1);
		PackVis_HeaderPos++;
		if(PackVis_HeaderPos > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(PackVis_Handle);
			PackVis_Handle = 0;
			return 0;
		}			
	} while(c!=EOFVAL);
	
	return 1;

}


/*--------------------------------------------------------------------------
Chiude il file dati immagine package
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    SMOD180304
--------------------------------------------------------------------------*/
void PackVisData_Close(void)
{
	if(PackVis_Handle)
	{
		FilesFunc_close(PackVis_Handle);
		PackVis_Handle=0;
	}
}

/*--------------------------------------------------------------------------
Scrive dati su file dati immagine package
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    SMOD180304
--------------------------------------------------------------------------*/
void PackVisData_Write(struct PackVisData data)
{
	lseek(PackVis_Handle,PackVis_HeaderPos,SEEK_SET);
	write(PackVis_Handle,&data,sizeof(data));
}

/*--------------------------------------------------------------------------
Legge dati su file dati immagine package
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    SMOD180304
--------------------------------------------------------------------------*/
void PackVisData_Read(struct PackVisData &data)
{
	lseek(PackVis_Handle,PackVis_HeaderPos,SEEK_SET);
	read(PackVis_Handle,&data,sizeof(data));
}

struct FOpenItem *FList_GetHead(void)
{
  return(FOpenedList);
}

void FList_AddItem(const char *fname)
{
  struct FOpenItem *ptr=FOpenedList;

  if(!IsFileFuncEnabled())
  {
    return;
  }

  if(strstr(fname,":/"))
  {
    //non loggare i file aperti specificando il pathname completo di
    //nome disco
    return;
  }
  if(strstr(fname,REMOTEDISK))
  {
    //non loggare i file aperti gia' nel server
    return;
  }
  
  while(ptr!=NULL)
  {
    if(!strcasecmpQ(fname,ptr->name))
    {
      return;
    }
    ptr=ptr->next;
  }

  if(FOpenedList==NULL)
  {
    FOpenedList=new FOpenItem;
    FOpenedList->next=NULL;
    FOpenedList->prev=NULL;

    FOpenedLast=FOpenedList;
  }
  else
  {
    struct FOpenItem *tmp=new FOpenItem;

    tmp->prev=FOpenedLast;
    tmp->next=NULL;

    FOpenedLast->next=tmp;
    
    FOpenedLast=tmp;
  }

  strncpyQ(FOpenedLast->name,fname,MAXNPATH);

  FOpenedNRecs++;

  /*
  while(FOpenedNRecs>FOPENLIST_MAX_SIZE)
  {
    FOpenItem* tmp=FOpenedList;
    FOpenedList->next->prev=NULL;
    FOpenedList=FOpenedList->next;
    delete tmp;

    FOpenedNRecs--;
  }
  */
}

void FList_RemoveItem(char *fname)
{
  struct FOpenItem *ptr=FOpenedList;

  if(!IsFileFuncEnabled())
  {
    return;
  }

  while(ptr!=NULL)
  {
    if(!strcasecmpQ(fname,ptr->name))
    {
      break;
    }
    ptr=ptr->next;
  }

  if(ptr!=NULL)
  {
    if(ptr->prev!=NULL)
    {
      ptr->prev->next=ptr->next;
    }

    if(ptr->next)
    {
      ptr->next->prev=ptr->prev;
    }

    delete ptr;

  }

  if(FOpenedNRecs>=0)
  {
    FOpenedNRecs--;
  }

}

void FList_Save(void)
{
  FILE *f=fopen("bkplist","wb");

  struct FOpenItem *ptr=FOpenedList;

  if(f!=NULL)
  {
    while(ptr!=NULL)
    {
      fwrite(ptr->name,1,MAXNPATH+1,f);

      ptr=ptr->next;
    }
  }  

  fclose(f);
}

void FList_Read(void)
{
  //FList_Flush();

  FOpenedNRecs=0;

  FILE *f=fopen("bkplist","rb");

  if((f!=NULL) && (filelength(fileno(f))>0))
  {
    char buf[MAXNPATH+1];

    while(!feof(f))
    {
      fread(buf,1,MAXNPATH+1,f);
      FList_AddItem(buf);
    }
  }
  else
  {
    FList_Save();
  }
}


void FList_Flush(void)
{
  while(FOpenedLast!=NULL)
  {
    struct FOpenItem *tmp=FOpenedLast->prev;
    delete FOpenedLast;
    FOpenedLast=tmp;    
  }

  FOpenedLast=NULL;
  FOpenedList=NULL;

  FOpenedNRecs=0;
}

int FList_Count(void)
{
  int count=0;

  struct FOpenItem *ptr=FOpenedList;

  while(ptr!=NULL)
  {
    count++;
    ptr=ptr->next;
  }

  return(count);
}

//------------------------------------------------------------------------------

int HandleNetPar=0;

int CreateNetPar(void)
{
	if(!F_File_Create(HandleNetPar,NETPARNAME))
	{
		return 0;
	}
	
	int eof=EOFVAL;
	
	WRITE_HEADER(HandleNetPar,FILES_VERSION,NETPAR_SUBVERSION);
	
	write(HandleNetPar,(char *)TXTNETPAR,strlen(TXTNETPAR));
	write(HandleNetPar,&eof,1);
	
	nwpar.NetID[0]='\0';
	nwpar.enabled=0;
	nwpar.executed=0;
	
	write(HandleNetPar,(char *)&nwpar,sizeof(nwpar));
	
	FilesFunc_close(HandleNetPar);
	
	return 1;

}

int WriteNetPar(struct NetPar nwpar)
{
	if(!F_File_Open(HandleNetPar,NETPARNAME))
	{
		return 0;
	}
	
	char Letto;
	
	int NoHeader=0;
	
	do
	{
		read(HandleNetPar,&Letto,1);
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleNetPar);
			HandleNetPar = 0;
			return 0;
		}
	} while(Letto!=EOFVAL);
	
	write(HandleNetPar,(char *)&nwpar,sizeof(nwpar));
	
	FilesFunc_close(HandleNetPar);
	
	return 1;
}

int LoadNetPar(struct NetPar &nwpar)
{
	if(access(NETPARNAME,F_OK))
	{
		if(!CreateNetPar())
		{
			return 0;
		}
	}
	
	if(!F_File_Open(HandleNetPar,NETPARNAME))
	{
		return 0;
	}
	
	char Letto;
	
	int NoHeader=0;
	
	do
	{
		read(HandleNetPar,&Letto,1);
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleNetPar);
			HandleNetPar = 0;
			return 0;
		}
	} while(Letto!=EOFVAL);
	
	read(HandleNetPar,(char *)&nwpar,sizeof(nwpar));
	
	FilesFunc_close(HandleNetPar);
	
	if(!Get_UseNetwork())
	{
		nwpar.enabled=0;
	}
	
	return 1;

}

//===========================================================================


int HandleCarTime=0;
int CarTime_Offset=0;

int CarTime_WriteDefault(void)
{
	if(!HandleCarTime)
	{
		return 0;
	}
	
	lseek(HandleCarTime,CarTime_Offset,SEEK_SET);
	
	struct CarTimeData dta;
	
	memset(dta.spare1,(char)0,8);
	
	strcpy(dta.name,CARTIMEDEF0_NAME);
	
	dta.start_ele=CARTIMEDEF0_STRT_ELE;
	dta.end_ele=CARTIMEDEF0_END_ELE;
	
	dta.start_motor=CARTIMEDEF0_STRT_MOT;
	dta.end_motor=CARTIMEDEF0_END_MOT;
	
	dta.start_inv=CARTIMEDEF0_STRT_INV;
	dta.end_inv=CARTIMEDEF0_END_INV;
	
	dta.wait=CARTIMEDEF0_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------
	
	strcpy(dta.name,CARTIMEDEF1_NAME);
	
	dta.start_ele=CARTIMEDEF1_STRT_ELE;
	dta.end_ele=CARTIMEDEF1_END_ELE;
	
	dta.start_motor=CARTIMEDEF1_STRT_MOT;
	dta.end_motor=CARTIMEDEF1_END_MOT;
	
	dta.start_inv=CARTIMEDEF1_STRT_INV;
	dta.end_inv=CARTIMEDEF1_END_INV;
	
	dta.wait=CARTIMEDEF1_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------
	
	strcpy(dta.name,CARTIMEDEF2_NAME);
	
	dta.start_ele=CARTIMEDEF2_STRT_ELE;
	dta.end_ele=CARTIMEDEF2_END_ELE;
	
	dta.start_motor=CARTIMEDEF2_STRT_MOT;
	dta.end_motor=CARTIMEDEF2_END_MOT;
	
	dta.start_inv=CARTIMEDEF2_STRT_INV;
	dta.end_inv=CARTIMEDEF2_END_INV;
	
	dta.wait=CARTIMEDEF2_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------

	strcpy(dta.name,CARTIMEDEF3_NAME);
	
	dta.start_ele=CARTIMEDEF3_STRT_ELE;
	dta.end_ele=CARTIMEDEF3_END_ELE;
	
	dta.start_motor=CARTIMEDEF3_STRT_MOT;
	dta.end_motor=CARTIMEDEF3_END_MOT;
	
	dta.start_inv=CARTIMEDEF3_STRT_INV;
	dta.end_inv=CARTIMEDEF3_END_INV;
	
	dta.wait=CARTIMEDEF3_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------
	
	strcpy(dta.name,CARTIMEDEF4_NAME);
	
	dta.start_ele=CARTIMEDEF4_STRT_ELE;
	dta.end_ele=CARTIMEDEF4_END_ELE;
	
	dta.start_motor=CARTIMEDEF4_STRT_MOT;
	dta.end_motor=CARTIMEDEF4_END_MOT;
	
	dta.start_inv=CARTIMEDEF4_STRT_INV;
	dta.end_inv=CARTIMEDEF4_END_INV;
	
	dta.wait=CARTIMEDEF4_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------
	
	strcpy(dta.name,CARTIMEDEF5_NAME);
	
	dta.start_ele=CARTIMEDEF5_STRT_ELE;
	dta.end_ele=CARTIMEDEF5_END_ELE;
	
	dta.start_motor=CARTIMEDEF5_STRT_MOT;
	dta.end_motor=CARTIMEDEF5_END_MOT;
	
	dta.start_inv=CARTIMEDEF5_STRT_INV;
	dta.end_inv=CARTIMEDEF5_END_INV;
	
	dta.wait=CARTIMEDEF5_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------
	
	strcpy(dta.name,CARTIMEDEF6_NAME);
	
	dta.start_ele=CARTIMEDEF6_STRT_ELE;
	dta.end_ele=CARTIMEDEF6_END_ELE;
	
	dta.start_motor=CARTIMEDEF6_STRT_MOT;
	dta.end_motor=CARTIMEDEF6_END_MOT;
	
	dta.start_inv=CARTIMEDEF6_STRT_INV;
	dta.end_inv=CARTIMEDEF6_END_INV;
	
	dta.wait=CARTIMEDEF6_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------
	
	strcpy(dta.name,CARTIMEDEF7_NAME);
	
	dta.start_ele=CARTIMEDEF7_STRT_ELE;
	dta.end_ele=CARTIMEDEF7_END_ELE;
	
	dta.start_motor=CARTIMEDEF7_STRT_MOT;
	dta.end_motor=CARTIMEDEF7_END_MOT;
	
	dta.start_inv=CARTIMEDEF7_STRT_INV;
	dta.end_inv=CARTIMEDEF7_END_INV;
	
	dta.wait=CARTIMEDEF7_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	#ifdef __DOME_FEEDER
	strcpy(dta.name,CARTIMEDEF_DOME_NAME);
	
	dta.start_ele=CARTIMEDEF_DOME_STRT_ELE;
	dta.end_ele=CARTIMEDEF_DOME_END_ELE;
	
	dta.start_motor=CARTIMEDEF_DOME_STRT_MOT;
	dta.end_motor=CARTIMEDEF_DOME_END_MOT;
	
	dta.start_inv=CARTIMEDEF_DOME_STRT_INV;
	dta.end_inv=CARTIMEDEF_DOME_END_INV;
	
	dta.wait=CARTIMEDEF_DOME_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------
	
	strcpy(dta.name,CARTIMEDEF_DOME_UP_NAME);
	
	dta.start_ele=CARTIMEDEF_DOME_UP_STRT_ELE;
	dta.end_ele=CARTIMEDEF_DOME_UP_END_ELE;
	
	dta.start_motor=CARTIMEDEF_DOME_UP_STRT_MOT;
	dta.end_motor=CARTIMEDEF_DOME_UP_END_MOT;
	
	dta.start_inv=CARTIMEDEF_DOME_UP_STRT_INV;
	dta.end_inv=CARTIMEDEF_DOME_UP_END_INV;
	
	dta.wait=CARTIMEDEF_DOME_UP_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------
	
	strcpy(dta.name,CARTIMEDEF_DOME_DW_NAME);
	
	dta.start_ele=CARTIMEDEF_DOME_DW_STRT_ELE;
	dta.end_ele=CARTIMEDEF_DOME_DW_END_ELE;
	
	dta.start_motor=CARTIMEDEF_DOME_DW_STRT_MOT;
	dta.end_motor=CARTIMEDEF_DOME_DW_END_MOT;
	
	dta.start_inv=CARTIMEDEF_DOME_DW_STRT_INV;
	dta.end_inv=CARTIMEDEF_DOME_DW_END_INV;
	
	dta.wait=CARTIMEDEF_DOME_DW_WAIT;
	
	if(write(HandleCarTime,&dta,sizeof(dta))!=sizeof(dta))
	{
		return 0;
	}

	//-----------------------------------------------------------------
	
	#endif

	return 1;
}

void CarTime_Close(void)
{
	if(HandleCarTime)
	{
		FilesFunc_close(HandleCarTime);
		HandleCarTime=0;
	}
}

int CarTime_Create(void)
{
	if(!F_File_Create(HandleCarTime,CARTIME_FILE))
	{
		return 0;
	}
	
	int eof=EOFVAL;
	
	WRITE_HEADER(HandleCarTime,FILES_VERSION,CARTIME_SUBVERSION); //SMOD230703-bridge
	write(HandleCarTime,(char *)TXTCARTIME,strlen(TXTCARTIME));
	write(HandleCarTime,&eof,1);
	
	CarTime_Offset=lseek(HandleCarTime,0,SEEK_CUR);
	
	if(!CarTime_WriteDefault())
	{
		CarTime_Close();
		return 0;
	}
	
	CarTime_Close();
	
	return 1;
}

int CarTime_Open(void)
{
	if(HandleCarTime)
	{
		return 1;
	}
	
	if(access(CARTIME_FILE,F_OK))
	{
		if(!CarTime_Create())
		{
			W_Mess(CARTIME_OPEN_ERR);
			return 0;
		}
	}
	
	if(!F_File_Open(HandleCarTime,CARTIME_FILE))
	{
		return 0;
	}
	
	char Letto;
	
	CarTime_Offset=0;
	
	do
	{
		read(HandleCarTime,&Letto,1);
		CarTime_Offset++;
		if(CarTime_Offset > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(CarTime_Offset);
			CarTime_Offset = 0;
			return 0;
		}				
	} while(Letto!=EOFVAL);

  return 1;
  
}

int CarTime_Read(struct CarTimeData &dta,int n)
{
	if(!HandleCarTime)
	{
		return 0;
	}

	int pos=CarTime_Offset+n*sizeof(struct CarTimeData);

	if(pos>filelength(HandleCarTime))
	{
		return 0;
	}

	lseek(HandleCarTime,pos,SEEK_SET);

	if(read(HandleCarTime,&dta,sizeof(struct CarTimeData))!=sizeof(struct CarTimeData))
	{
		return 0;
	}

	return 1;
}

int CarTime_Write(struct CarTimeData dta,int n)
{
	if(!HandleCarTime)
	{
		return 0;
	}

	int pos=CarTime_Offset+n*sizeof(struct CarTimeData);

	if(pos>filelength(HandleCarTime))
	{
		return 0;
	}

	int handle = FilesFunc_dup(HandleCarTime);

	lseek(HandleCarTime,pos,SEEK_SET);

	if(write(HandleCarTime,&dta,sizeof(struct CarTimeData))!=sizeof(struct CarTimeData))
	{
		return 0;
	}

	FilesFunc_close(handle);

	return 1;
}

int CarTime_NRec(void)
{
	if(!HandleCarTime)
	{
		return 0;
	}

	return((filelength(HandleCarTime)-CarTime_Offset)/sizeof(struct CarTimeData));
}

//------------------------------------------------------------------------------------

int HandleWarmUp=0;
int WarmUpParams_Offset=0;

int WarmUpParams_Read(struct WarmUpParams &data)
{
  int prev_open=1;

  if(!HandleWarmUp)
  {
    if(!WarmUpParams_Open())
    {
      return 0;
    }
    prev_open=0;
  }

  lseek(HandleWarmUp,WarmUpParams_Offset,SEEK_SET);

  read(HandleWarmUp,&data,sizeof(data));

  if(!prev_open)
  {
    WarmUpParams_Close();
  }


  return 1;
}

int WarmUpParams_Write(struct WarmUpParams data)
{
  int prev_open=1;

  if(!HandleWarmUp)
  {
    if(!WarmUpParams_Open())
    {
      return 0;
    }
    prev_open=0;
  }
  
  lseek(HandleWarmUp,WarmUpParams_Offset,SEEK_SET);

  write(HandleWarmUp,&data,sizeof(data));

  if(!prev_open)
  {
    WarmUpParams_Close();
  }

  return 1;
}

int WarmUpParams_Create(void)
{
	if(!F_File_Create(HandleWarmUp,WARMUPPARAMS_FILE))
	{
		return 0;
	}
	
	int eof=EOFVAL;
	
	WRITE_HEADER(HandleWarmUp,FILES_VERSION,WARMUPPAR_SUBVERSION); //SMOD230703-bridge
	write(HandleWarmUp,(char *)TXTWARMUPPAR,strlen(TXTWARMUPPAR));
	write(HandleWarmUp,&eof,1);
	
	WarmUpParams_Offset=lseek(HandleWarmUp,0,SEEK_CUR);
	
	struct WarmUpParams data;
	
	data.duration = DEFAULT_WARMUP_DURATION;
	data.threshold = DEFAULT_WARMUP_THRESHOLD;
	data.enable = DEFAULT_WARMUP_ENABLESTATE;
	data.acceleration = QHeader.xyMaxAcc;
	
	WarmUpParams_Write(data);
	
	WarmUpParams_Close();

	return 1;
}

void WarmUpParams_Close(void)
{
  if(HandleWarmUp)
  {
    FilesFunc_close(HandleWarmUp);
    HandleWarmUp=0;
  }
}

int WarmUpParams_Open(void)
{
	if(access(WARMUPPARAMS_FILE,F_OK))
	{
		if(!WarmUpParams_Create())
		{
			return 0;
		}
	}
	
	if(!F_File_Open(HandleWarmUp,WARMUPPARAMS_FILE))
	{
		return 0;
	}
	
	char Letto;
	
	WarmUpParams_Offset=0;
	
	do
	{
		read(HandleWarmUp,&Letto,1);
		WarmUpParams_Offset++;
		if(WarmUpParams_Offset > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleWarmUp);
			HandleWarmUp = 0;
			return 0;
		}
	} while(Letto!=EOFVAL);

	return 1;
}
//----------------------------------------------------------------------------

//Main backup info

int HandleMBkpInfo=0;

void CloseMBkpInfo(void)
{
  if(HandleMBkpInfo!=0)
  {
    FilesFunc_close(HandleMBkpInfo);
    HandleMBkpInfo=0;
  }
}

int OpenMBkpInfo(int mode,bkp::e_dest dest, const char* mnt)
{
	char buf[MAXNPATH+1];
	
	GetMBkpInfo_FileName(buf,mode,dest,mnt);
	
	if(!F_File_Open(HandleMBkpInfo,buf))
	{
		return 0;
	}
	
	int c=0;
	int NoHeader=0;
		
	while(c!=EOFVAL)
	{
		if(read(HandleMBkpInfo,&c,1)!=1)
		{
			CloseMBkpInfo();
			return 0;
		}
		
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleMBkpInfo);
			HandleMBkpInfo = 0;
			return 0;
		}
	}
	
	return 1;
}

int CreateMBkpInfo(int mode,bkp::e_dest dest, const char* mnt)
{
	char buf[MAXNPATH+1];
	
	GetMBkpInfo_FileName(buf,mode,dest,mnt);
	
	if(!F_File_Create(HandleMBkpInfo,buf))
	{
		return 0;
	}

	int eof=EOFVAL;
	
	WRITE_HEADER(HandleMBkpInfo,FILES_VERSION,MBKPINFO_SUBVERSION);
	
	if(mode==BKP_COMPLETE)
	{
		write(HandleMBkpInfo,(char *)TXTMBKPINFO1,strlen(TXTMBKPINFO1));
	}
	else
	{
		write(HandleMBkpInfo,(char *)TXTMBKPINFO2,strlen(TXTMBKPINFO2));
	}
	
	write(HandleMBkpInfo,&eof,1);
	
	return 1;
  
}

void GetMBkpInfo_FileName(char *buf,int mode,bkp::e_dest dest, const char* mnt)
{
	GetBackupBaseDir(buf,dest,mnt);

	if(mode==BKP_COMPLETE)
	{
		strcat(buf,"/" BKP_LAST_FULL);
	}
	else
	{
		strcat(buf,"/" BKP_LAST_PARTIAL);
	}
}

int ReadMBkpInfo(int &data,int mode,bkp::e_dest dest, const char* mnt)
{
  /*if(data==0)
  {
    return 0;
  }*/

	if(!OpenMBkpInfo(mode,dest,mnt))
	{
		return 0;
	}
	
	if(read(HandleMBkpInfo,&data,sizeof(data))!=sizeof(data))
	{
		CloseMBkpInfo();
		return 0;
	}
	else
	{
		CloseMBkpInfo();
		return 1;
	}
}


int WriteMBkpInfo(int data,int mode,bkp::e_dest dest, const char* mnt)
{
	char buf[MAXNPATH+1];
	
	GetMBkpInfo_FileName(buf,mode,dest,mnt);
	
	if(access(buf,F_OK))
	{
		if(!CreateMBkpInfo(mode,dest,mnt))
		{
			return 0;
		}
	}
	else
	{
		if(!OpenMBkpInfo(mode,dest,mnt))
		{
			return 0;
		}
	}

	if(write(HandleMBkpInfo,&data,sizeof(data))!=sizeof(data))
	{
		CloseMBkpInfo();
		return 0;
	}
	else
	{
		CloseMBkpInfo();
		return 1;
	}
}

int HandleBackupInfo=0;

void CloseBackupInfo(void)
{
	if(HandleBackupInfo!=0)
	{
		FilesFunc_close(HandleBackupInfo);
		HandleBackupInfo=0;
	}
}

int OpenBackupInfo(int mode,int n,bkp::e_dest dest, const char* mnt)
{
	char buf[MAXNPATH+1];
	
	GetBackupInfo_FileName(buf,mode,n,dest,mnt);
	
	if(!F_File_Open(HandleBackupInfo,buf))
	{
		return 0;
	}
	
	int c=0;
	int NoHeader=0;
		
	while(c!=EOFVAL)
	{
		if(read(HandleBackupInfo,&c,1)!=1)
		{
			CloseBackupInfo();
			return 0;
		}
		
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleBackupInfo);
			HandleBackupInfo = 0;
			return 0;
		}			
	}
	
	return 1;
}

int CreateBackupInfo(int mode,int n,bkp::e_dest dest, const char* mnt)
{
	char buf[MAXNPATH+1];
	
	GetBackupInfo_FileName(buf,mode,n,dest,mnt);
	
	if(!F_File_Create(HandleBackupInfo,buf))
	{
		W_Mess(buf);
		return 0;
	}
	
	int eof=EOFVAL;
	
	WRITE_HEADER(HandleBackupInfo,FILES_VERSION,BKPINFO_SUBVERSION);
	write(HandleBackupInfo,(char *)TXTBKPINFO,strlen(TXTBKPINFO));
	write(HandleBackupInfo,&eof,1);
	
	return 1;
  
}

void GetBackupInfo_FileName(char *buf,int mode,int nidx,bkp::e_dest dest, const char* mnt)
{
	GetBackupDir(buf,mode,nidx,dest,mnt);
	sprintf( buf+strlen(buf), "/" BKP_INFO_FILE, nidx );
}


int ReadBackupInfo(struct BackupInfoDat &data,int mode,int n,bkp::e_dest dest, const char* mnt)
{
	if(!OpenBackupInfo(mode,n,dest,mnt))
	{
		return 0;
	}
	
	if(read(HandleBackupInfo,&data,sizeof(data))!=sizeof(data))
	{
		CloseBackupInfo();
		return 0;
	}
	else
	{
		CloseBackupInfo();
		return 1;
	}
}


int WriteBackupInfo(struct BackupInfoDat data,int mode,int n,bkp::e_dest dest, const char* mnt)
{
	char buf[MAXNPATH+1];
	
	GetBackupInfo_FileName(buf,mode,n,dest,mnt);
	
	if(access(buf,F_OK))
	{
		if(!CreateBackupInfo(mode,n,dest,mnt))
		{
			return 0;
		}
	}
	else
	{
		if(!OpenBackupInfo(mode,n,dest,mnt))
		{
			return 0;
		}
	}
	
	if(write(HandleBackupInfo,&data,sizeof(data))!=sizeof(data))
	{
		CloseBackupInfo();
		return 0;
	}
	else
	{
		CloseBackupInfo();
		return 1;
	}
}

int HandleMachinesList=0;
int MachinesListOffset=0;

int OpenMachinesList(void)
{
	if(HandleMachinesList)
	{
		return 1;
	}
	
	const char* filename;
	
	if(!Get_UseQMode())
	{
		filename=MACHINES_LIST_FILE;
	}
	else
	{
		filename=MACHINES_LIST_FILE_QMODE;
	}
	
	if(access(filename,F_OK))
	{
		return(CreateMachinesList());
	}
	
	if(!F_File_Open(HandleMachinesList,filename))
	{
		return 0;
	}
	
	int c=0;
	int NoHeader=0;
		
	while(c!=EOFVAL)
	{
		if(read(HandleMachinesList,&c,1)!=1)
		{
			CloseMachinesList();
			return 0;
		}
		
		NoHeader++;
		if(NoHeader > MAX_QFILES_HEADER_SIZE)
		{
			FilesFunc_close(HandleMachinesList);
			HandleMachinesList = 0;
			return 0;
		}			
	}
	
	MachinesListOffset=lseek(HandleMachinesList,0,SEEK_CUR);
	
	return 1;
}

int CreateMachinesList(void)
{
	if(HandleMachinesList)
	{
		return 0;
	}
	
	if(Get_UseQMode())
	{
		if(!F_File_Create(HandleMachinesList,MACHINES_LIST_FILE_QMODE))
		{
			return 0;
		}
	}
	else
	{
		if(!F_File_Create(HandleMachinesList,MACHINES_LIST_FILE))
		{
			return 0;
		}
	
	}
	
	int eof=EOFVAL;
	
	WRITE_HEADER(HandleMachinesList,FILES_VERSION,MLIST_SUBVERSION);
	write(HandleMachinesList,(char *)TXTMLIST,strlen(TXTMLIST));
	write(HandleMachinesList,&eof,1);
	
	MachinesListOffset=lseek(HandleMachinesList,0,SEEK_CUR);
	
	return 1;
}

int CloseMachinesList(void)
{
  if(HandleMachinesList!=0)
  {
    FilesFunc_close(HandleMachinesList);
    HandleMachinesList=0;
    return 1;
  }
  else
  {
    return 0;
  }
}

int AddMachineName(char *name)
{
  if(!OpenMachinesList())
  {
    return 0;
  }

  lseek(HandleMachinesList,0,SEEK_END);

  char buf[50];
  strcpy(buf,name);

  if(strlen(buf)>=50)
  {
    buf[49]='\0';
  }
  else
  {  
    InsSpc(50-strlen(buf),buf+strlen(buf));
  }

  if(write(HandleMachinesList,buf,50)==-1)
  {
    CloseMachinesList();
    return 0;
  }
  else
  {
    CloseMachinesList();
    return 1;
  }
}


int GetLastMachineID(void)
{
  if(!OpenMachinesList())
  {
    return(-1);
  }

  int n=(filelength(HandleMachinesList)-MachinesListOffset)/50;

  CloseMachinesList();

  return(n);
}

int SearchMachineName(char *name)
{
  if(!OpenMachinesList())
  {
    return(-1);
  }

  lseek(HandleMachinesList,MachinesListOffset,SEEK_SET);

  DelSpcR(name);

  int n=0;

  int found=0;

  do
  {
    char buf[51];

    buf[50]='\0';

    int r=read(HandleMachinesList,buf,50);

    if(r==-1)
    {
      CloseMachinesList();
      return(-1);
    }

    if(r==0)
    {
      break;
    }

    DelSpcR(buf);

    n++;

    if(!strcasecmpQ(buf,name))
    {
      found=n;
      break;
    }
    
  } while(1);

  CloseMachinesList();

  return(found);
  
}

int GetMachineName(int n,char *name)
{
  if(!OpenMachinesList())
  {
    return(-1);
  }

  n--;

  lseek(HandleMachinesList,MachinesListOffset,SEEK_SET);

  char buf[51];

  while(n>=0)
  {
    buf[50]='\0';

    int r=read(HandleMachinesList,buf,50);

    if(r==-1)
    {
      CloseMachinesList();
      return(-1);
    }

    if(r==0)
    {
      break;
    }

    DelSpcR(buf);

    n--;

  };

  int ret=0;

  if(n==-1)
  {
    strcpy(name,buf);
    ret=1;
  }

  CloseMachinesList();

  return(ret);

}


//------------------------------------------------------------------------

static const SniperModeData SniperModeDefaultData[] = 
{
	{
		0,
		ALGO_SNIPER0_NAME_DEFAULT,
		ALGO_SNIPER0_SEARCH_SPEED_DEFAULT,
		ALGO_SNIPER0_ACCEL_SPEED_DEFAULT,
		ALGO_SNIPER0_ANGLE_LIMIT_DEFAULT,
		ALGO_SNIPER0_PREROTATION_ANGLE_DEFAULT,
		ALGO_SNIPER0_IGNORE_FINAL_FRAMES_DEFAULT,
		ALGO_SNIPER0_CODE_DEFAULT
	},
 
	{
		1,
  		ALGO_SNIPER1_NAME_DEFAULT,
		ALGO_SNIPER1_SEARCH_SPEED_DEFAULT,
		ALGO_SNIPER1_ACCEL_SPEED_DEFAULT,
		ALGO_SNIPER1_ANGLE_LIMIT_DEFAULT,
		ALGO_SNIPER1_PREROTATION_ANGLE_DEFAULT,
		ALGO_SNIPER1_IGNORE_FINAL_FRAMES_DEFAULT,
		ALGO_SNIPER1_CODE_DEFAULT
	}, 
	
	{
		2,
  		ALGO_SNIPER2_NAME_DEFAULT,
		ALGO_SNIPER2_SEARCH_SPEED_DEFAULT,
		ALGO_SNIPER2_ACCEL_SPEED_DEFAULT,
		ALGO_SNIPER2_ANGLE_LIMIT_DEFAULT,
		ALGO_SNIPER2_PREROTATION_ANGLE_DEFAULT,
		ALGO_SNIPER2_IGNORE_FINAL_FRAMES_DEFAULT,
		ALGO_SNIPER2_CODE_DEFAULT
	},  
	
	{
		3,
  		ALGO_SNIPER3_NAME_DEFAULT,
		ALGO_SNIPER3_SEARCH_SPEED_DEFAULT,
		ALGO_SNIPER3_ACCEL_SPEED_DEFAULT,
		ALGO_SNIPER3_ANGLE_LIMIT_DEFAULT,
		ALGO_SNIPER3_PREROTATION_ANGLE_DEFAULT,
		ALGO_SNIPER3_IGNORE_FINAL_FRAMES_DEFAULT,
		ALGO_SNIPER3_CODE_DEFAULT
	} 
};

SniperModes::SniperModes(void)
	: 	m_invalidated(true),
		m_handle(0),
		m_nrec(-1),
		m_start_offset(0)
{
	openFile();
}

SniperModes::~SniperModes(void)
{
	closeFile();
}

int SniperModes::openFile(void)
{
	if(m_handle)
	{
		return 1;
	}
	
	if(access(SNIPERMODES_FILE,F_OK))
	{
		return(createFile());
	}
	
	if(!F_File_Open(m_handle,SNIPERMODES_FILE))
	{
		return 0;
	}
	
	int c=0;
	
	do
	{
		// skip file header
		if(read(m_handle,&c,1)!=1)
		{
			closeFile();
			return 0;
		}
	
		m_start_offset++;
		
		if(m_start_offset > MAX_QFILES_HEADER_SIZE)
		{
			closeFile();
			return 0;
		}
	} while(c!=EOFVAL);
	
	unsigned int spare,subversion;
	
	Get_FileVersion_OLD(m_handle,spare,subversion); //SMOD230703-bridge
	
	if(subversion == 0)
	{
		int eof=EOFVAL;
		SniperModeData data;
		
		lseek(m_handle,0,SEEK_SET);
		
		WRITE_HEADER(m_handle,FILES_VERSION,SNIPERMODES_SUBVERSION);
		write(m_handle,(char *)TXTSNIPERMODES,strlen(TXTSNIPERMODES));
		write(m_handle,&eof,1);
		
		data.idx = SniperModeDefaultData[3].idx;
		strncpy(data.name,SniperModeDefaultData[3].name,sizeof(data.name) - 1);
		data.speed = SniperModeDefaultData[3].speed;
		data.acceleration = SniperModeDefaultData[3].acceleration;
		data.search_angle = SniperModeDefaultData[3].search_angle;
		data.prerotation = SniperModeDefaultData[3].prerotation;
		data.discard_nframes = SniperModeDefaultData[3].discard_nframes;
		data.algorithm = SniperModeDefaultData[3].algorithm;
		memset(data.spare1,0,sizeof(data.spare1));
		
		if(!writeFile(3,data))
		{
			return 0;
		}
	}

	return 1;
}

int SniperModes::closeFile(void)
{
	if(m_handle)
	{
		FilesFunc_close(m_handle);
		m_handle = 0;
	}
}

int SniperModes::createFile(void)
{
	SniperModeData data;

	if(!F_File_Create(m_handle,SNIPERMODES_FILE))
	{
		return 0;
	}

	int eof=EOFVAL;

	WRITE_HEADER(m_handle,FILES_VERSION,SNIPERMODES_SUBVERSION);
	write(m_handle,(char *)TXTSNIPERMODES,strlen(TXTSNIPERMODES));
	write(m_handle,&eof,1);

	m_start_offset=strlen(TXTSNIPERMODES)+1+HEADER_LEN;

	if(!writeFile(0,data))
	{
		return 0;
	}

	memset(data.name,0,sizeof(data.name));
	memset(data.spare1,0,sizeof(data.spare1));
	
	const SniperModeData* mode;
	
	for(int i = 0; i < MAX_SNIPERMODES_NREC; i++)
	{
		switch(i)
		{
			case 0:
			case 1:
			case 2:
			case 3:
				mode =  &(SniperModeDefaultData[i]);
				data.idx = mode->idx;
				break;
			default:
				mode =  &(SniperModeDefaultData[1]);
				data.idx = i;
				break;
		}
		
		//data.idx = mode->idx;
		strncpy(data.name,mode->name,sizeof(data.name) - 1);
		data.speed = mode->speed;
		data.acceleration = mode->acceleration;
		data.search_angle = mode->search_angle;
		data.prerotation = mode->prerotation;
		data.discard_nframes = mode->discard_nframes;
		data.algorithm = mode->algorithm;
		
		if(!writeFile(i,data))
		{
			return 0;
		}
	}

	return 1;
}

int SniperModes::readFile(int nrec,SniperModeData& data)
{
	if(!checkOk())
	{
		return 0;
	}

	if(nrec < 0)
	{
		return 0;
	}

	if(nrec >= MAX_SNIPERMODES_NREC)
	{
		return 0;
	}  

	int pos=m_start_offset+nrec*sizeof(SniperModeData);

	if(pos>filelength(m_handle))
	{
		return 0;
	}

	lseek(m_handle,pos,SEEK_SET);

	memset(&data,0,sizeof(data));

	if(read(m_handle,(char*)&data,sizeof(data))!=sizeof(data))
	{
		bipbip();
		W_Mess(ERR_SNIPER_MODES_FILE_RW);
		return 0;
	}

	return 1;
}

int SniperModes::writeFile(int nrec,const SniperModeData& data)
{
	if(!checkOk())
	{
		return 0;
	}

	if(nrec < 0)
	{
		return 0;
	}

	if(nrec >= MAX_SNIPERMODES_NREC)
	{
		return 0;
	}

	int pos=m_start_offset+nrec*sizeof(SniperModeData);

	int handle_dup = FilesFunc_dup(m_handle);

	lseek(m_handle,pos,SEEK_SET);            // seek for replace

	if(write(m_handle, (char *)&data, sizeof(data)) < sizeof(data))
	{
		bipbip();
		W_Mess(ERR_SNIPER_MODES_FILE_RW);
		FilesFunc_close(handle_dup);
		return 0;
	}

	FilesFunc_close(handle_dup);

	return 1;
}

int SniperModes::isOk(void)
{
	return(m_handle!=0);
}

int SniperModes::checkOk(void)
{
	if(!m_handle)
	{
		bipbip();
		W_Mess(ERR_SNIPER_MODES_FILE);
		return 0;
	}
	else
	{
		return 1;
	}

}

int SniperModes::getNRecords(void)
{
	if(m_handle)
	{
		return((int)(filelength(m_handle)-m_start_offset)/sizeof(SniperModeData));
	}
	else
	{
		return 0;
	}
}

int SniperModes::getRecord(int nrec,SniperModeData& data)
{
	if((nrec == m_nrec) && !m_invalidated)
	{
		data = m_rec;
		return 1;
	}
	else
	{
		int r = readFile(nrec,data);
		if(r)
		{
			if((nrec == m_nrec) && m_invalidated)
			{
				m_rec = data;
				m_invalidated = false;
			}
		}    
		return(r);
	}

}

int SniperModes::getRecord(SniperModeData& data)
{
	return(getRecord(m_nrec,data));
}

const SniperModeData& SniperModes::getRecord(void)
{
	return(m_rec);
}

int SniperModes::updateRecord(int nrec,SniperModeData& data)
{
	int r = writeFile(nrec,data);
	if(r && (nrec == m_nrec))
	{
		m_rec = data;
		m_invalidated = false;
	}
}

int SniperModes::updateRecord(SniperModeData& data)
{
	return(updateRecord(m_nrec,data));
}

void SniperModes::useRecord(int nrec)
{
	if(nrec >= 0)
	{
		m_nrec = nrec;
		m_invalidated = true;
		getRecord(m_rec);
	}
}

//---------------------------------------------------------------------------------

CCenteringReservedParameters::CCenteringReservedParameters(void)
:  m_handle(0),
   m_start_offset(0)
{
	if(openFile())
	{
		readData(m_data);
	}
	
}

CCenteringReservedParameters::~CCenteringReservedParameters(void)
{
	closeFile();
}

int CCenteringReservedParameters::openFile(void)
{
	if(m_handle)
	{
		return 1;
	}
	
	if(!access(OPTCENT_CFG_PAR_FILE,F_OK))
	{
		//cancella file obsoleto
		remove(OPTCENT_CFG_PAR_FILE);
	}
	
	if(access(RESERVED_CENTR_CFG_PAR_FILE,F_OK))
	{
		return(createFile());
	}
	
	if(!F_File_Open(m_handle,RESERVED_CENTR_CFG_PAR_FILE))
	{
		return 0;
	}
	
	do
	{
		char c;
		
		// skip file header
		if(read(m_handle,&c,1)!=1)
		{
			closeFile();
			return 0;
		}
	
		m_start_offset++;
		if(c == EOFVAL)
		{
			break;
		}
		
		if(m_start_offset > MAX_QFILES_HEADER_SIZE)
		{
			closeFile();
			return 0;
		}
	} while(1);
	
	unsigned int spare,subversion = 0;
	Get_FileVersion_OLD(m_handle,spare,subversion);

	return 1;
}

int CCenteringReservedParameters::closeFile(void)
{
	if(m_handle)
	{
		FilesFunc_close(m_handle);
		m_handle = 0;
	}

	return 1;
}

int CCenteringReservedParameters::createFile(void)
{
	CenteringReservedParameters data = CENTR_CFG_PAR_DEFAULT;

	if(!F_File_Create(m_handle,RESERVED_CENTR_CFG_PAR_FILE))
	{
		return 0;
	}

	int eof=EOFVAL;

	WRITE_HEADER(m_handle,FILES_VERSION,RESERVED_CENTR_CFG_PAR_SUBVERSION);
	write(m_handle,(char *)TXTRES_CENTR_CFG_PAR,strlen(TXTRES_CENTR_CFG_PAR));
	write(m_handle,&eof,1);

	m_start_offset=strlen(TXTRES_CENTR_CFG_PAR)+1+HEADER_LEN;

	if(!writeData(data))
	{
		return 0;
	}

	return 1;
}

const CenteringReservedParameters& CCenteringReservedParameters::getData(void)
{
	return m_data;
}

int CCenteringReservedParameters::readData(CenteringReservedParameters& data)
{
	if(!checkOk())
	{
		return 0;
	}

	lseek(m_handle,m_start_offset,SEEK_SET);

	memset(&data,0,sizeof(data));

	if(read(m_handle,(char*)&data,sizeof(data))!=sizeof(data))
	{
		bipbip();
		char buf[160];
		sprintf(buf,UNABLE_TO_ACCESS_TO_FILE,RESERVED_CENTR_CFG_PAR_FILE);
		W_Mess(buf);
		return 0;
	}
	
	m_data = data;

	return 1;
}

int CCenteringReservedParameters::writeData(const CenteringReservedParameters& data)
{
	if(!checkOk())
	{
		return 0;
	}

	int handle_dup = FilesFunc_dup(m_handle);

	lseek(m_handle,m_start_offset,SEEK_SET);            

	if(write(m_handle, (char *)&data, sizeof(data)) < sizeof(data))
	{
		bipbip();
		char buf[160];
		sprintf(buf,UNABLE_TO_ACCESS_TO_FILE,RESERVED_CENTR_CFG_PAR_FILE);
		W_Mess(buf);
		FilesFunc_close(handle_dup);
		return 0;
	}
	
	m_data = data;

	FilesFunc_close(handle_dup);

	return 1;
}

int CCenteringReservedParameters::isOk(void)
{
	return(m_handle!=0);
}

int CCenteringReservedParameters::checkOk(void)
{
	if(!m_handle)
	{		
		bipbip();
		char buf[160];
		sprintf(buf,UNABLE_TO_ACCESS_TO_FILE,RESERVED_CENTR_CFG_PAR_FILE);
		W_Mess(buf);		
		return 0;
	}
	else
	{
		return 1;
	}

}

//------------------------------------------------------------------------------------------------------------------

template<typename T> CQFile<T>::CQFile(void)
{
	m_handle = 0;
	m_data_start = 0;
	
	m_FileMaxNRecs = 0;
	m_FileTypeVersion = atoi(FILES_VERSION);
	m_FileTypeSubversion = 0;
	m_CreateFileIfNotExists = true;	
	
	m_ignore_case = false;
	m_lock = false;	
}

template<typename T> CQFile<T>::CQFile(const std::string& path)
{
	m_handle = 0;
	m_data_start = 0;

	m_FileMaxNRecs = 0;	
	m_FileTypeVersion = atoi(FILES_VERSION);
	m_FileTypeSubversion = 0;	
	m_CreateFileIfNotExists = true;	
	
	m_ignore_case = false;
	m_lock = false;	
	
	open(path);
}

template<typename T> CQFile<T>::CQFile(const char* path)
{
	m_handle = 0;
	m_data_start = 0;

	m_FileMaxNRecs = 0;
	m_FileTypeVersion = atoi(FILES_VERSION);
	m_FileTypeSubversion = 0;	
	m_CreateFileIfNotExists = true;	
	
	m_ignore_case = false;
	m_lock = false;
	
	open(path);	
}

template<typename T>CQFile<T>::~CQFile(void)
{
	close();
}

template<typename T> bool CQFile<T>::is_open(void)
{
	return(m_handle != 0 );
}

template<typename T> bool CQFile<T>::exists(void)
{
	if(!access(m_path.c_str(),F_OK))
	{
		return true;
	}
	else
	{
		return false;
	}
}

template<typename T> bool CQFile<T>::private_create(void)
{
	return true;
}

template<typename T> bool CQFile<T>::create(void)
{
	char eof = EOFVAL;
	
	if(m_handle)
	{
		if(!close())
		{
			return false;
		}
	}
	
	if(!F_File_Create(m_handle,m_path.c_str()))
	{
		return(false);
	}
	
	std::stringstream header;
	
	header << QUADRA_HEADER_START << m_FileTypeVersion << '.' << m_FileTypeSubversion;
	
	write(m_handle,header.str().c_str(),header.str().size());
	
	write(m_handle,(const char *)m_FileTitle.c_str(),m_FileTitle.size());
	write(m_handle,&eof,1);
	
	m_data_start = lseek(m_handle,0,SEEK_CUR);

	bool r = private_create();
	
	close();

	return(r);
}

template<typename T> bool CQFile<T>::open(void)
{
	if(m_path.size())
	{
		return open(m_path);
	}
	else
	{
		return false;
	}
	
}

template<typename T> bool CQFile<T>::open(const std::string& path)
{
	return open(path.c_str());
}

template<typename T> bool CQFile<T>::_open(const char* path)
{
	
	if ((m_handle = FilesFunc_open(path, O_RDWR , 0, m_ignore_case,m_lock)) == -1)
	{
		return false;
	}
	
	FList_AddItem(path);
	
	//SMOD230703-bridge
	if(Get_FileVersion_OLD(m_handle,m_ReadedFileTypeVersion,m_ReadedFileTypeSubversion))
	{
		if(m_ReadedFileTypeVersion != FILES_VERSION[0] - '0')
		{
			Show_FileVersionErr(path);
			close();
			return false;
		}		
	}	
	else
	{
		close();
		return false;
	}
	
	return true;

}

template<typename T> bool CQFile<T>::open(const char* path)
{
	if(m_handle)
	{
		close();
	}
	
	m_path = path;
	
	if(m_CreateFileIfNotExists)
	{
		if(!exists())
		{
			if(!create())
			{
				return false;
			}
		}
	}
			
	if(!_open(path))
	{
		m_handle = 0;
		return(false);
	}
	
	unsigned char c=0;
	m_data_start = 0;
	
	while(c != EOFVAL)
	{
		// skip file header
		read(m_handle,&c,1);
		m_data_start++;
			
		if(m_data_start > MAX_QFILES_HEADER_SIZE)
		{
			close();
			return(false);
		}			
	}
	
	if(m_ReadedFileTypeSubversion < m_FileTypeSubversion)
	{
		if(!upgrade())
		{
			close();
			return false;
		}
	}	
	
	return(true);		
}

template<typename T> bool CQFile<T>::close(void)
{
	if(m_handle)
	{
		FilesFunc_close(m_handle);
		m_handle = 0;
	}
	
	return true;
}

template<typename T> bool CQFile<T>::upgrade(void)
{
	return true;
}

template<typename T> bool CQFile<T>::readRec(T& rec,unsigned int nrec)
{
	if(!m_handle)
	{
		return false;
	}
	
	if(m_FileMaxNRecs && (nrec >= m_FileMaxNRecs))
	{
		return false;
	}
	
	unsigned int filepos = nrec * sizeof(rec) + m_data_start;
	
	lseek(m_handle,filepos,SEEK_SET);

	int n = read(m_handle,(char *)&rec, sizeof(rec));

	if(n != sizeof(rec))
	{
		return(false);
	}
	else
	{
		return(true);
	}
}

template<typename T> bool CQFile<T>::writeRec(const T& rec,unsigned int nrec)
{
	if(!m_handle)
	{
		return false;
	}
	
	if(m_FileMaxNRecs && (nrec >= m_FileMaxNRecs))
	{
		return false;
	}
	
	unsigned int filepos = nrec * sizeof(rec) + m_data_start;
	
	lseek(m_handle,filepos,SEEK_SET);

	int n = write(m_handle,(const char *)&rec, sizeof(rec));

	if(n != sizeof(rec))
	{
		return(false);
	}
	else
	{
		return(true);
	}	
}
	
template<typename T> int CQFile<T>::countRecs(void)
{
	return( ( filelength(m_handle) - m_data_start) / sizeof(T) );
}

//--------------------------------------------------------------------------------------------------------

template class CQFile<s_security_reserved_params>;

CSecurityReservedParametersFile::CSecurityReservedParametersFile(void)
{
	m_FileMaxNRecs = 1;
	m_FileTypeSubversion = 1;
	m_FileTitle = TXTRESERVED_SECURITY_PARAMETERS;	
	memset(&m_params,0,sizeof(m_params));
	m_dirty = true;
	open(SECURITY_RESERVED_PARAMETERS_FILE);
	
}

bool CSecurityReservedParametersFile::private_create(void)
{
	s_security_reserved_params init;
	memset(&init,0,sizeof(init));
	init.mov_confirm_timeout = 30;
	return writeParameters(init);
	
}

bool CSecurityReservedParametersFile::readParameters(s_security_reserved_params& params)
{	
	bool r = readRec(params,0);
	if(r)
	{
		m_params = params;
		m_dirty = false;
	}
	else
	{
		m_dirty = true;
	}
	return r;
}

bool CSecurityReservedParametersFile::writeParameters(const s_security_reserved_params& params)
{
	bool r =  writeRec(params,0);
	if(r)
	{
		m_params = params;
		m_dirty = false;
	} 
	else
	{
		m_dirty = true;
	}
	return r;
}

const s_security_reserved_params& CSecurityReservedParametersFile::get_data(void) 
{
	if(m_dirty)
	{
		if(!readParameters(m_params))
		{
			memset(&m_params,0,sizeof(m_params));
		}
	}
	return m_params;
}
