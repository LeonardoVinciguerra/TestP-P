//---------------------------------------------------------------------------
// Name:        q_carint.cpp
// Author:      
// Created:     
// Description: Gestione caricatori intelligenti.
//---------------------------------------------------------------------------
#include "q_carint.h"

#include <time.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>

#include "q_cost.h"
#include "q_inifile.h"
#include "q_carobj.h"
#include "q_wind.h"
#include "q_tabe.h"
#include "q_debug.h"
#include "msglist.h"

#include "q_files.h"
#include "q_prog.h"
#include "q_help.h"
#include "q_conf.h"
#include "q_dosat.h"
#include "q_packages.h"
#include "q_feeders.h"
#include "q_ugeobj.h"
#include "q_oper.h"
#include "q_vision.h"
#include "filefn.h"
#include "q_net.h"
#include "q_tabet.h"
#include "q_ser.h"

#include "strutils.h"
#include "fileutils.h"
#include "datetime.h"

#include "c_inputbox.h"
#include "c_waitbox.h"
#include "gui_desktop.h"
#include "gui_defs.h"
#include "keyutils.h"

#include <mss.h>


extern GUI_DeskTop* guiDeskTop;

bool feedersDatabaseRefresh = false;


int CarList_CurRow, CarList_CurRecord, CarList_StartTabe;
int CarList_nRec;

// Array elementi tabella caric. intell.
CarInt_data* CarList = 0;

//indice del primo record libero
int* FreeList = 0;
int Free_nRec = 0;
int FirstFree_idx = 0;


extern struct CfgHeader QHeader;
extern struct CfgParam  QParam;
extern struct cur_data CurData;
extern struct img_data CC_image;
extern FeederFile* CarFile;

// Se 0, significa che la rete e' stata trovata
int netOff = 1;

// Strutture d'appoggio contenenti i dati dei db magazzini
// locale e remoto
CarIntFile DBLocal(CARINT_LOCAL);

CarIntFile DBRemote;//(CARINT_REMOTE);

// Nomi dei files dei db locale e remoto
char NomeFileLocal[160],NomeFileRemote[160];

// Vettore che identifica i magazzini attualmente montati sulla macchina
int ConfDBLink[MAXMAG];
int ConfDBLink_ok=0;


#ifdef __DBNET_DEBUG
FILE * fNetDB_debug;

void DBNet_DebugOpen(void)
{
  if(access("net.dbg",F_OK))
  {
    fNetDB_debug=FilesFunc_fopen("net.dbg","wt");
  }
  else
  {
    fNetDB_debug=FilesFunc_fopen("net.dbg","at");
  }
}

void DBNet_DebugClose(void)
{
  if(fNetDB_debug!=NULL)
  {
    FilesFunc_fclose(fNetDB_debug);
  }
}
#endif



//---------------------------------------------------------------------------
//SMOD211204
//Funzioni per caricatori intelligenti (con elettronica a bordo)


//Legge il numero di serie del caricatore.
//Ritorna anche la checksum (record 0)
int CarInt_GetSerial(int caric,int &serial,unsigned int &chksum,int &type,int &swver)
{
	char buf[64];
	
	int ret,err=0;
	
	do
	{
		if((err && (QHeader.debugMode2 & DEBUG2_SMARTFEED_DBG)) || (err==CARINT_MAXERR))
		{
			char tbuf[80];
			snprintf( tbuf, sizeof(tbuf),CARINT_ERRCOMM,caric+1,buf[16]);
			W_Mess(tbuf);
			return 0;
		}
		
		snprintf( buf, sizeof(buf), "K%1X",caric);
	
		if((ret=cpu_cmd(1,buf))!='K')
		{
			err++;
			buf[16]='0';
			continue;
		}
	
		if(strlen(buf)!=17)
		{
			err++;
			continue;
		}

		//buf+16 --> status
		if(buf[16]!='K')
		{
			if(buf[16]=='T')
			{
				return 0;
			}
	
			err++;
			continue;

		}

		serial=0;

		switch(buf[0])
		{
			case 'T':
				type = CARINT_TYPE_TAPE;
				serial |= (CARINT_TAPESER_OFFSET << 24);				
				break;
			case 'A':
				type = CARINT_TYPE_AIR;
				serial |= (CARINT_AIRSER_OFFSET << 24);				
				break;
			case 'G':
				type = CARINT_TYPE_GENERIC;
				serial |= (CARINT_GENERICSER_OFFSET << 24);		
				break;
			case 'P':
				type = CARINT_TYPE_PROTCAP;
				serial |= (CARINT_PROTCAP_OFFSET << 24);
				break;		
			default:
				err++;
				continue;
				break;
		}
				
		swver=buf[1]-'0';
		
		chksum=0;
	
		for(int i=0;i<8;i++)
		{
			if(i<6)
			{
				if(!isalnum(buf[i+2]))
				{
					err++;
					continue;
		
				}
				serial+=(Hex2Dec(buf[i+2])) << (4*(5-i));
			}
		
			if(!isalnum(buf[i+8]))
			{
				err++;
				continue;
			}

			chksum+=(Hex2Dec(buf[i+8])) << (4*(7-i));
		}

		return(buf[16]);
	} while(1);

}



//Scrive su un record di un caricatore un numero a 32 bit senza segno
int CarInt_SetData(unsigned int caric,char record,unsigned int data)
{
  char buf[32],tbuf[80];

  if(strchr(CARINT_VALIDRECS_NUM,record)==NULL)
  {
    return 0;
  }

  if(caric>MAXMAG)
  {
    return 0;
  }

  int err=0;

  do
  {
    if((err && (QHeader.debugMode2 & DEBUG2_SMARTFEED_DBG)) || (err==CARINT_MAXERR))
    {
      snprintf( tbuf, sizeof(tbuf),CARINT_ERRCOMM,caric+1,buf[0]);
      W_Mess(tbuf);
      return 0;
    }

    snprintf( buf, sizeof(buf), "SK%1X%c%08X",caric,record,data);

    if(cpu_cmd(2,buf)!='K')
    {
      err++;
      buf[0]='0';
      continue;
    }

    return(buf[0]);
    
  } while(1);



}

//Scrive su un record di un caricatore una stringa di 8 caratteri al massimo
int CarInt_SetData(unsigned int caric,char record,char *data)
{
  char buf[32],tbuf[80];
  char buf2[16];

  if(strchr(CARINT_VALIDRECS_STR,record)==NULL)
  {
    return 0;
  }

  if(caric>MAXMAG)
  {
    return 0;
  }

  if(strlen(data)>8)
  {
    return 0;
  }

  int err=0;

  do
  {
    if((err && (QHeader.debugMode2 & DEBUG2_SMARTFEED_DBG)) || (err==CARINT_MAXERR))
    {
      snprintf( tbuf, sizeof(tbuf),CARINT_ERRCOMM,caric+1,buf[0]);
      W_Mess(tbuf);
      return 0;
    }

    strcpy(buf2,data);

    int len=strlen(buf2);

    for(int i=0;i<8;i++)
    {
      if(i>=len)
      {
        buf2[i]='0';
      }
      else
      {
        if(buf2[i]==' ') buf2[i]='~';
      }
    }

    buf2[8]='\0';

    snprintf( buf, sizeof(buf), "SK%1X%c%s",caric,record,buf2);

    if(cpu_cmd(2,buf)!='K')
    {
      err++;
      buf[0]='0';      
      continue;
    }

    return(buf[0]);
  } while(1);

}

//Calcola la checksum di un record
unsigned short int CarInt_CalcChecksum(struct CarInt_data rec)
{
	unsigned char *ptr=(unsigned char *)(&rec);

	unsigned short int chksum = 0;

	for(int i=0;i<int(sizeof(rec)-sizeof(rec.checksum));i++)
	{
		chksum+=*ptr;
		ptr++;
	}

	return chksum;
}


//************************************************************************
// Gestione file dati caricatori intelligenti
//************************************************************************

/*--------------------------------------------------------------------------
Costruttore banale senza apertura del file
INPUT:   -
GLOBAL:	-
NOTE:	   -
--------------------------------------------------------------------------*/
CarIntFile::CarIntFile(void)
{
	HandleCar=0;
	type=-1;
}

/*--------------------------------------------------------------------------
Costruttore con apertura del file
INPUT:   type: tipo (locale=0 o remoto=1)
GLOBAL:	-
NOTE:	   -
--------------------------------------------------------------------------*/
CarIntFile::CarIntFile(int _type)
{
  HandleCar=0;
  type=_type;

  if(type)
  {
    if(!Get_UseQMode())
    {
      strcpy(name,CARINTREMOTE);
    }
    else
    {
      strcpy(name,CARINTREMOTE_QMODE);
    }
  }
  else
  {
    strcpy(name,CARINTFILE);
  }

  lockOpen=0;

  if(type)
  {
    sprintf(err_askretry,CARINTFILE_ERR,TXTREMOTE,TXTRETRY_REMOTE);
    sprintf(err_show,CARINTFILE_ERR,TXTREMOTE,"");
  }
  else
  {
    sprintf(err_askretry,CARINTFILE_ERR,TXTLOCAL,TXTRETRY_LOCAL);
    sprintf(err_show,CARINTFILE_ERR,TXTLOCAL,"");
  }
  
  //Open(nomefile,mode);
}

CarIntFile::~CarIntFile(void)
{
  Close();
}

/*--------------------------------------------------------------------------
Controlla se il file e' stato correttamente aperto
INPUT:   -
GLOBAL:	-
RETURN:  0 se il file non e' stato aperto, 1 altrimenti
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::IsOpen(void)
{
	return HandleCar ? 1 : 0;
}

/*--------------------------------------------------------------------------
Apre file caricatori intelligenti e ne impedisce la chiusura dalle
funzioni read e write
INPUT:
         mode    : SKIPHEADER   = apre saltando header (default)
                   NOSKIPHEADER = apre file
GLOBAL:	-
RETURN:  0 se FAIL, 1 altrimenti
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::LockOpen(int errmode)
{
	int ret=Open(errmode);
	
	if(ret)
	{
		lockOpen=1;
	}
	
	return(ret);
}

/*--------------------------------------------------------------------------
Chiude il file caricatori intelligenti permettendone la apertura alle
funzioni read e write
INPUT:
         mode    : SKIPHEADER   = apre saltando header (default)
                   NOSKIPHEADER = apre file
GLOBAL:	-
RETURN:  0 se FAIL, 1 altrimenti
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::UnLockClose(void)
{
	lockOpen=0;
	return(Close());
}

/*--------------------------------------------------------------------------
Apre file caricatori intelligenti
INPUT:
         mode    : SKIPHEADER   = apre saltando header (default)
                   NOSKIPHEADER = apre file
GLOBAL:	-
RETURN:  0 se FAIL, 1 altrimenti
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::Open(int errmode/*=0*/)
{
	int NoHeader = 0;
	char c=0;
	
	if(HandleCar)
		return 1;              // File is already open
	
	if(lockOpen)
	{
		//il file e' gia aperto e ogni altro tentativo di apertura e chiusura e'
		//bloccato.
		return 1;
	}
	
	int endloop=0;
	
	int err;
	
	do
	{
		err=0;
	
		if(!err)
		{
			if(access(name,0))
			{
				if(!Create(errmode))
				{
					err=1;
				}
			}

			if(!err)
			{
				if(!F_File_Open(HandleCar,name,CHECK_HEADER, false,true))
				{
					err=1;
				}
			}
		}
	
		if(err)
		{
			if(errmode & CARINTFILE_ERR_SHOW)
			{
				if(errmode & CARINTFILE_ERR_ASKRETRY)
				{
					if(!W_Deci(1,err_askretry))
					{
						endloop=1;
					}
				}
				else
				{
					W_Mess(err_show);
					endloop=1;
				}
			}
		}
		else
		{
			endloop=1;
		}
	} while(!endloop);
	
	if(type && (err && (errmode & CARINTFILE_ERR_DISABLENET)))
	{
		W_Mess(NETDISABLED);
		DisableNet();
	}
	
	if(err)
	{
		HandleCar=0;
		return 0;
	}
	
	while(c!=26)
	{ // skip file header
		read(HandleCar,&c,1);
		NoHeader++;
	}
	
	//offset packages
	Caroffset=NoHeader;
	
	if(!CheckTypeFlagDone(errmode))
	{
		FilesFunc_close(HandleCar,true);
		HandleCar=0;
		return 0;
	}

	return 1;
}

int CarIntFile::_CheckTypeFlagDone(void)
{
  if(HandleCar==0)
  {
    return 0;
  }

  int prev_seek=lseek(HandleCar,0,SEEK_CUR);

  lseek(HandleCar,0,SEEK_SET);

  char c;

  for(int i=0;i<Caroffset;i++)
  {
    if(read(HandleCar,&c,1)!=1)
    {
      return 0;
    }

    //se letto newline
    if(c=='\n')
    {
      if(read(HandleCar,&c,1)==1)
      {
        //se prossimo carattere newline o comunque diverso da '1'
        if((c=='\n') || (c!='1'))
        {
          //init del tipo eseguire
          TypeFlagOk=0;
        }
        else
        {
          //ok: init del tipo gia' eseguito
          TypeFlagOk=1;
        }
        
        break;
      }
      else
      {
        return 0;
      }
    }
  }

  lseek(HandleCar,prev_seek,SEEK_SET);

  return 1;
}

int CarIntFile::CheckTypeFlagDone(int errmode)
{
  int err;
  
  do
  {
    err=0;

      if(!_CheckTypeFlagDone())
      {
        err=1;
      }
      else
      {
        break;
      }

    if(err)
    {
      if(errmode & CARINTFILE_ERR_SHOW)
      {
        if(errmode & CARINTFILE_ERR_ASKRETRY)
        {
          if(!W_Deci(1,err_askretry))
          {
            break;
          }
        }
        else
        {
          W_Mess(err_show);
          break;
        }
      }
    }
    else
    {
      break;
    }
  } while(1);

  if(err)
  {
    if(type && (errmode & CARINTFILE_ERR_DISABLENET))
    {
      W_Mess(NETDISABLED);
      DisableNet();
    }

    return 0;
  }
  else
  {
    return 1;
  }
}

int CarIntFile::_SetTypeFlagDone(void)
{
  if(HandleCar==0)
  {
    return 0;
  }

  int prev_seek=lseek(HandleCar,0,SEEK_CUR);

  lseek(HandleCar,0,SEEK_SET);

  char c;

  int ret=1;
  
  for(int i=0;i<Caroffset;i++)
  {
    if(read(HandleCar,&c,1)!=1)
    {
      ret=0;
      break;
    }

    //se letto newline
    if(c=='\n')
    {
      if(read(HandleCar,&c,1)!=1)
      {
        //se prossimo carattere newline o comunque diverso da '1'
        if((c=='\n') || (c!='1'))
        {
          //seek indietro sull'ultimo carattere letto
          lseek(HandleCar,-1,SEEK_CUR);

          //sovrascrive con carattere '1'
          c='1';
          
          if(write(HandleCar,&c,1)!=1)
          {
            ret=0;
            break;
          }
        }
        else
        {
          //ok: set del flag gia' eseguito
        }
      }
      else
      {
        ret=0;
        break;
      }
    }
  }

  lseek(HandleCar,prev_seek,SEEK_SET);

  return(ret);
}

int CarIntFile::SetTypeFlagDone(int errmode)
{
  int err;
  
  do
  {
    err=0;

      if(!_SetTypeFlagDone())
      {
        err=1;
      }      

    if(err)
    {
      if(errmode & CARINTFILE_ERR_SHOW)
      {
        if(errmode & CARINTFILE_ERR_ASKRETRY)
        {
          if(!W_Deci(1,err_askretry))
          {
            break;
          }
        }
        else
        {
          W_Mess(err_show);
          break;
        }
      }
    }
    else
    {
      break;
    }
  } while(1);

  if(err)
  {
    return 0;
  }
  else
  {
    TypeFlagOk=1;
    return 1;
  }
}

int CarIntFile::UpdateTypeFlag(int errmode)
{
	if(TypeFlagOk)
	{
		return 1;
	}
	
	if(!LockOpen(errmode));
	{
		return 0;
	}

	struct CarInt_data dta;
	
	for(int i=0;i<Count();i++)
	{
		if(!Read(dta,i,errmode))
		{
			UnLockClose();
			return 0;
		}
	
		dta.smart = 0;
	
		if(!Write(dta,i,errmode))
		{
			UnLockClose();
			return 0;
		}
	}

	if(!SetTypeFlagDone(errmode))
	{
		UnLockClose();
		return 0;
	}
	else
	{
		UnLockClose();
		return 1;
	}
}


/*--------------------------------------------------------------------------
Setta la struttura dati caric intell con valori di default
INPUT:   -
GLOBAL:	-
RETURN:  ritorna in car una struttura dati caricatori settata con valori di default
NOTE:	   -
--------------------------------------------------------------------------*/
void CarIntFile::SetDefault(struct CarInt_data &car)
{
	car.serial = FREE_RECORD;
	car.idx = FREE_RECORD;
	car.mounted = 0;
	
	for( int i=0; i<8; i++ )
	{
		car.packIdx[i]=-1;
		*car.pack[i]=0;
		*car.noteCar[i]=0;
		*car.elem[i]=0;
		car.num[i]=0;
		car.tipoCar[i]=0;
		car.tavanz[i]=0;
	}
	car.smart = 0;
	*car.note = 0;
}


/*--------------------------------------------------------------------------
Crea file dei caric intell
GLOBAL:	-
RETURN:  0 se FAIL, 1 altrimenti.
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::Create(int errmode)
{
   char eof = 26;
   int err=0;
   int endloop=0;

   do
   {
     err=0;
     

       if(!F_File_Create(HandleCar,name,true))
       {
         err=1;
       }

     if(err)
     {
       if(errmode & CARINTFILE_ERR_SHOW)
       {
         if(errmode & CARINTFILE_ERR_ASKRETRY)
         {
           if(!W_Deci(1,err_askretry))
           {
             endloop=1;
           }
         }
         else
         {
           W_Mess(err_show);
           endloop=1;
         }
       }
     }
     else
     {
       endloop=1;
     }
     
   } while(!endloop);

   if(type && (err && (errmode & CARINTFILE_ERR_DISABLENET)))
   {
     W_Mess(NETDISABLED);
     DisableNet();
   }

   if(err)
   {
     HandleCar=0;
     return 0;
   }
     

   WRITE_HEADER(HandleCar,FILES_VERSION,CARINT_SUBVERSION);
	write(HandleCar,(char *)TXTCARINT,strlen(TXTCARINT));
	write(HandleCar,&eof,1);

	Caroffset=strlen(TXTCARINT)+1+HEADER_LEN;

	/*
   for(int i=0;i<MAXCARINTALL;i++)
   {
     SetDefault(Car_Pack);
     Car_Pack.serial=i+1;
     Write(Car_Pack,i,0);
   }
   */

	//write(HandleCar,&eof,1);

	FilesFunc_close(HandleCar,true);

	HandleCar=0;

	return 1;
}

/*--------------------------------------------------------------------------
Chiude il file dei caric intell
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::Close()
{
	if(HandleCar)
	{
		if(lockOpen)
		{
			return 1;
		}
	
		int ret=FilesFunc_close(HandleCar,true);
	
		HandleCar=0;
		
		return(ret);
	}
	
	return 1;
}

/*--------------------------------------------------------------------------
Lunghezza file caric intell
INPUT:   -
GLOBAL:	-
RETURN:  ritorna lunghezza del file.
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::GetLen(void)
{
   int prevopen=1;

   if(HandleCar==0)
   {
     prevopen=0;

     if(!Open())
     {
       return 0;
     }
   }

   int n=filelength(HandleCar);

   if(!prevopen)
   {
     Close();
   }

	return(n);
}


/*--------------------------------------------------------------------------
Numero di records contenuti nel file caric intell
Legge un record da file dei caric intell
INPUT:   -
GLOBAL:	-
RETURN:  ritorna il numero di records.
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::Count(void)
{
   int ret=((GetLen()-Caroffset)/sizeof(struct CarInt_data));
   return(ret);
}

/*--------------------------------------------------------------------------
Ridimensiona il file al numero di record specificato
INPUT:   nrec: numero di record da ottenere
GLOBAL:	-
RETURN:  -
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::Resize(int nrec)
{
  int prevopen=1;

  if(!HandleCar)
  {
    if(!LockOpen(CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
    {
      return 0;
    }

    prevopen=0;
  }

  if(nrec>=Count())
  {
    if(!prevopen)
    {
      UnLockClose();
    }
    return 0;
  }

  if(ftruncate(HandleCar,Caroffset+nrec*sizeof(struct CarInt_data)))
  {
    if(!prevopen)
    {
      UnLockClose();
    }

    return 0;
  }

  if(!prevopen)
  {
    UnLockClose();
  }

  return 1;
}

/*--------------------------------------------------------------------------
Legge tutta la struttura dati dei caric intell in un vettore
INPUT:   -
GLOBAL:	-
RETURN:  0 se FAIL, 1 altrimenti.
         Ritorna nel vettore Car_Tab l'intero file.
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::ReadAll(struct CarInt_data *Car_Tab,int errmode)
{
   int i;

   if(!LockOpen(errmode))
   {
     return 0;
   }

   for(i=0;i<Count();i++)
   {
      if(!Read(Car_Tab[i],i,errmode))
      {
        UnLockClose();
        return 0;
      }
   }

   UnLockClose();
   
   return(i);
}



/*--------------------------------------------------------------------------
Legge un record da file dei caric intell
INPUT:   RecNum : record da leggere
GLOBAL:	-
RETURN:  0 se FAIL, 1 altrimenti.
         Ritorna in Car_Tab il record letto.
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::Read(struct CarInt_data &Car_Tab, int RecNum,int errmode)
{
	int flag = 1;
	int rnumero = RecNum;                        // record num.
	int odim    = sizeof(Car_Tab);               // dimension of objet
	int rwpos;
   int n;

   #ifdef __DBNET_DEBUG
   char buf[40];
   #endif

   if(!Open(errmode))
   {
     #ifdef __DBNET_DEBUG
     snprintf( buf, sizeof(buf), "Error opening for reading record %d",RecNum);
     WriteLogMsg(buf);     
     #endif

     return 0;
   }

   rwpos = (rnumero*odim)+(int)Caroffset;      // r/w pointer in file

   do
   {
       lseek(HandleCar,rwpos,SEEK_SET);

       n=read(HandleCar,(char *)&Car_Tab, sizeof(Car_Tab));

       #ifdef __DBNET_DEBUG
       Check_Log(Car_Tab,RecNum,1);
       #endif

  	    if(n!=sizeof(Car_Tab))
       {
         flag=0;
       }
       else
       {
         flag=1;
       }

     if(!flag)
     {
       #ifdef __DBNET_DEBUG
       snprintf( buf, sizeof(buf), "Error reding record %d",RecNum);
       WriteLogMsg(buf);
       #endif

       if(errmode & CARINTFILE_ERR_SHOW)
       {
         if(errmode & CARINTFILE_ERR_ASKRETRY)
         {
           if(!W_Deci(1,err_askretry))
           {
             break;
           }
         }
         else
         {
           W_Mess(err_show);
           break;
         }
       }
       else
       {
         break;
       }
     }
     else
     {
       break;
     }
     
   } while(1);

   //su file gli indici sono 1 based -> una volta letti sono 0 based
   if(Car_Tab.idx>0)
   {
     Car_Tab.idx--;
   }

   //controllo validita flag modifiche pendenti
   if(Car_Tab.changed>1)
   {
     Car_Tab.changed=0;
   }

   if(type && ((flag==0) && (errmode & CARINTFILE_ERR_DISABLENET)))
   {
     #ifdef __DBNET_DEBUG
     WriteLogMsg("Disable net");
     #endif

     W_Mess(NETDISABLED);
     DisableNet();
   }

   Close();

   return(flag);
}


/*--------------------------------------------------------------------------
Inizializza e legge le liste dei dati inseriti e dei record liberi
INPUT:   list      : puntatore alla variabile puntatore alla lista dei dati
         FreeList  : puntatore a vettore lista record liberi
                     (opzionale)

GLOBAL:	-
RETURN:  numero dei record occupati
--------------------------------------------------------------------------*/
int CarIntFile::InitList(struct CarInt_data **list,int **flist,int &count,int &count2)
{
	struct CarInt_data data;

	count=0;

	if(!LockOpen(CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
	{
		count=0;
		count2=0;

		#ifdef __DBNET_DEBUG
		WriteLogMsg("Error lock open for initlist");
		#endif

		return 0;
	}

	CWaitBox waitBox( 0, 15, MsgGetString(Msg_01741), 2*Count() );
	waitBox.Show();

	#ifdef __DBNET_DEBUG
	WriteLogMsg("Initlist start");
	#endif

  for(int i=0;i<Count();i++)
  {
    if(!Read(data,i,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
    {
      UnLockClose();

      waitBox.Hide();
      
      count=0;
      count2=0;

      #ifdef __DBNET_DEBUG
      WriteLogMsg("Initlist end");
      #endif

      return 0;
    }
	
	if(data.idx!=i)
	{
		memset(&data,0,sizeof(data));
		data.idx=i;
		data.serial=FREE_RECORD;      
		Write(data,i,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET);      
	}
	
    if((data.serial & 0xFFFFFF) > 0)
    {
      count++;

      //TEST RESET
      //data.usedBy=0;
      //Write(data,i,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET);
    }

    waitBox.Increment();
  }

  //count=numero di record occupati nel file database

  if( *list != NULL)
  {
    delete [] (*list);
    *list = NULL;
  }

  *list = new struct CarInt_data [count+1];

  memset((*list)+count,(char)0,sizeof(data));
  (*list)[count].serial=NEW_RECORD;
  (*list)[count].idx=NEW_RECORD;
  (*list)[count].usedBy=MAG_USEDBY_NONE;

  if(flist!=NULL)
  {
    if(*flist!=NULL)
    {
      delete[] flist;      
    }

    *flist=new int[Count()-count+1];
  }

  int nrec=count;

  count=0;
  count2=0;

	for(int i=0;i<Count();i++)
	{
		if(!Read(data,i,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
		{
			if( *list != NULL )
			{
				delete [] (*list);
				*list = NULL;
			}

			UnLockClose();

			waitBox.Hide();


			if( *list != NULL )
			{
				delete [] (*list);
				*list = NULL;
			}

			if(*flist!=NULL)
			{
				delete[] (*flist);
				*flist=NULL;
			}

			count=0;
			count2=0;

			return 0;
		}

    if((data.serial & 0xFFFFFF)>0)
    {
      DelSpcR(data.note);

      if((data.mounted>MAXMAG) || (data.mounted<0))
      {
        data.mounted=0;
      }

      for(int k=0;k<8;k++)
      {
        DelSpcR(data.elem[k]);
        DelSpcR(data.pack[k]);
        DelSpcR(data.noteCar[k]);
      }
 
      memcpy((*list)+count,&data,sizeof(data));

      count++;
    }
    else
    {

      if(flist!=NULL)
      {
        (*flist)[count2++]=i;
      }
    }

    waitBox.Increment();
  }

  waitBox.Hide();

  //count2=numero di record liberi nel file database

  if(flist!=NULL)
  {
    (*flist)[count2++]=count;
  }

	assert2((count==nrec),print_debug("count=%d nrec=%d\n",count,nrec));

	UnLockClose();

	#ifdef __DBNET_DEBUG
	WriteLogMsg("Initlist end");
	#endif

	return 1;
}


/*--------------------------------------------------------------------------
Apre il file dei caric intell corrente
INPUT:   -
GLOBAL:	-
RETURN:  0 se FAIL, 1 altrimenti.
NOTE:	
--------------------------------------------------------------------------*/
int CarIntFile::OpenCur(void)
{
   if(HandleCar)
      return 1;              // File is already open

   return(Open());
}

/*--------------------------------------------------------------------------
Legge un record da file dei caric intell
INPUT:   RecNum: record da leggere
GLOBAL:	-
RETURN:  0 se FAIL, 1 altrimenti.
         Ritorna in Car_Tab il record letto.
NOTE:	   -
--------------------------------------------------------------------------*/
int CarIntFile::WriteAll(struct CarInt_data *Car_Tab,int errmode)
{
   int i;

   if(!LockOpen(errmode))
   {
     return 0;
   }

   for(i=0;i<Count();i++)
   {
      if(!Write(Car_Tab[i],i,FLUSHON,errmode))
      {
        UnLockClose();
        return 0;
      }
   }

   UnLockClose();
   
   return(i);
}

#ifdef __DBNET_DEBUG
/*--------------------------------------------------------------------------
Check e log su file delle operazioni sul file
INPUT:   Car_Tab : record da loggare
         RecNum  : numero record
         mode    : 0 modo scrittura
                   1 modo lettura

GLOBAL:	-
RETURN:  -
NOTE:	
--------------------------------------------------------------------------*/
void CarIntFile::Check_Log(struct CarInt_data Car_Tab,int RecNum,int mode)
{
	char buf[81];
	
	int er=0;
	
	if((Car_Tab.serial & 0xFFFFFF) > 100000)
	{
		er=1;
	}
	
	if((Car_Tab.serial & 0xFFFFFF) < 0)
	{
		er=1;
	}

	if((Car_Tab.serial & 0xFFFFFF)!=0)
	{
		switch((Car_Tab.serial >> 24) & 0xFF)
		{
			case CARINT_AIRSER_OFFSET:
			case CARINT_TAPESER_OFFSET:
			case CARINT_GENERICSER_OFFSET:				
				break;
			default:
				er=1;
				break;
		}
	}
	else
	{
		if(((Car_Tab.serial >> 24) & 0xFF)!=0)
		{
			er=1;
		}
	}

	if((type) && (mode==0))
	{
		er=1; //force log for all remote writings
	}
		
	if(er)
	{
		switch(mode)
		{
			case 0:
				snprintf( buf, sizeof(buf), "! Write serial %02X%06X (%08X) in record %d",(Car_Tab.serial >> 24) & 0xFF,Car_Tab.serial & 0xFFFFFF,Car_Tab.idx,RecNum);
				break;
			case 1:
				snprintf( buf, sizeof(buf), "! Read serial  %02X%06X (%08X) in record %d",(Car_Tab.serial >> 24) & 0xFF,Car_Tab.serial & 0xFFFFFF,Car_Tab.idx,RecNum);
				break;
		}
    
		WriteLogMsg(buf);
	}
/*
	else
	{
	switch(mode)
	{
	case 0:
	snprintf( buf, sizeof(buf), "  Write serial %02X%06X (%08X) in record %d",(Car_Tab.serial >> 24) & 0xFF,Car_Tab.serial & 0xFFFFFF,Car_Tab.idx,RecNum);
	break;
	case 1:
	snprintf( buf, sizeof(buf), "  Read serial  %02X%06X (%08X) in record %d",(Car_Tab.serial >> 24) & 0xFF,Car_Tab.serial & 0xFFFFFF,Car_Tab.idx,RecNum);
	break;
}

	WriteLogMsg(buf);  
}
*/
}

/*--------------------------------------------------------------------------
Scrive su file di log una stringa di testo
INPUT:   msg: messaggio da stampare
GLOBAL:	-
RETURN:  -
NOTE:	
--------------------------------------------------------------------------*/
void CarIntFile::WriteLogMsg(char *msg)
{
  struct time t;
  gettime(&t);

  char buf[81];

  if(type)
  {
    snprintf( buf, sizeof(buf), "%02d:%02d:%02d On remote %s",t.ti_hour,t.ti_min,t.ti_sec,msg);
  }
  else
  {
    snprintf( buf, sizeof(buf), "%02d:%02d:%02d On local  %s",t.ti_hour,t.ti_min,t.ti_sec,msg);
  }

  Pad(buf,80,' ');
  strcat(buf,"\n");

  fprintf(fNetDB_debug,"%s",buf);
}

#endif

/*--------------------------------------------------------------------------
Scrive un record in file caric intell
INPUT:   Car_Tab : record da scrivere
         RecNum: numero record
         Flush : FLUSHON = con flush dei dati
                 FLUSHOFF= senza flush dei dati
GLOBAL:	-
RETURN:  0 se FAIL, altrimenti 1.
NOTE:	
--------------------------------------------------------------------------*/
int CarIntFile::Write(struct CarInt_data Car_Tab, int RecNum, int Flush,int errmode)
{
	int flag = 1, handle;
	int rnumero = RecNum;                         // record num.
	int odim    = sizeof(Car_Tab);                // dimension of objet
	int rwpos;

   #ifdef __DBNET_DEBUG
   char buf[40];
   #endif

   if(!Open(errmode))
   {
     #ifdef __DBNET_DEBUG
     snprintf( buf, sizeof(buf), "Error opening for writing record %d",RecNum);
     WriteLogMsg(buf);
     #endif

     return 0;
   }

	rwpos = (rnumero*odim)+Caroffset;                // r/w pointer in file

	if(Flush) handle = FilesFunc_dup(HandleCar);

   Car_Tab.idx++;

   do
   {
       lseek(HandleCar,rwpos,SEEK_SET);            // seek for replace

       #ifdef __DBNET_DEBUG
       Check_Log(Car_Tab,RecNum,0);
       #endif

       if(write(HandleCar, (char *)&Car_Tab, sizeof(Car_Tab)) < sizeof(Car_Tab))
       {
         flag=0;
       }
       else
       {
         flag=1;
       }

     if(!flag)
     {
       #ifdef __DBNET_DEBUG
       snprintf( buf, sizeof(buf), "Error writing record %d",RecNum);
       WriteLogMsg(buf);
       #endif
       
       if(errmode & CARINTFILE_ERR_SHOW)
       {
         if(errmode & CARINTFILE_ERR_ASKRETRY)
         {
           if(!W_Deci(1,err_askretry))
           {
             break;
           }
         }
         else
         {
           W_Mess(err_show);
           break;
         }
       }
     }
     else
     {
       break;
     }
     
   } while(1);   

   if(Flush)
	{
     FilesFunc_close(handle);  // flush DOS buffer
   }

   Close();

   if(type && ((flag==0) && (errmode & CARINTFILE_ERR_DISABLENET)))
   {
     #ifdef __DBNET_DEBUG
     WriteLogMsg("Disable net");
     #endif

     W_Mess(NETDISABLED);
     DisableNet();
   }

   return(flag);
}




/****************************************************************

GESTIONE TABELLA CARICATORI INTELLIGENTI

****************************************************************/

//---------------------------------------------------------------------------
// finestra: Ask feeder serial
//---------------------------------------------------------------------------
class AskFeederSerialUI : public CWindowParams
{
public:
	AskFeederSerialUI( CWindow* parent, const char* title, char type, int serial ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU | WIN_STYLE_EDITMODE_ON );
		SetClientAreaPos( 0, 7 );
		SetClientAreaSize( 40, 5 );
		SetTitle( title );

		m_type = type;
		m_serial = serial;
	}

	int GetExitCode() { return m_exitCode; }
	char GetType() { return m_type; }
	int GetSerial() { return m_serial; }

	typedef enum
	{
		FEEDER_TYPE,
		FEEDER_SERIAL
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[FEEDER_TYPE]   = new C_Combo(  5, 2, MsgGetString(Msg_01283), 1, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE ); //tipo caricatore
		m_combos[FEEDER_SERIAL] = new C_Combo( 25, 2, "", 6, CELL_TYPE_UINT ); //seriale

		// set params
		m_combos[FEEDER_TYPE]->SetLegalChars( "ATG" );
		m_combos[FEEDER_SERIAL]->SetVMinMax( 1, 999999 );

		// add to combo list
		m_comboList->Add( m_combos[FEEDER_TYPE]  , 0, 0 );
		m_comboList->Add( m_combos[FEEDER_SERIAL], 0, 1 );
	}

	void onShow()
	{
		tips = new CPan( 20, 2, MsgGetString(Msg_00296), MsgGetString(Msg_00297) );
	}

	void onRefresh()
	{
		m_combos[FEEDER_TYPE]->SetTxt( m_type );
		m_combos[FEEDER_SERIAL]->SetTxt( m_serial );
	}

	void onEdit()
	{
		m_type = m_combos[FEEDER_TYPE]->GetChar();
		m_serial = m_combos[FEEDER_SERIAL]->GetInt();
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_ESC:
				m_exitCode = WIN_EXITCODE_ESC;
				break;

			case K_ENTER:
				if( m_serial == 0 )
				{
					break;
				}
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

private:
	CPan* tips;
	int m_exitCode;
	char m_type;
	int m_serial;
};




void CarInt_SearchResults(int index)
{
	CarList_CurRecord = index;
	CarList_StartTabe = CarList_CurRecord;
	CarList_CurRow = 0;

	feedersDatabaseRefresh = true;
}

int CarInt_FindSerCostr();
int CarInt_FindCar();
int CarInt_FindMag();
int CarInt_FindNote();
int CarInt_ConfImport();
int CarInt_AdvSearch();
int CarInt_UnMount(void);
int CarInt_AutoMag();
void CarInt_DelRec();
void CarIntIncRow();
void CarIntDecRow();
void CarInt_PgDN();
void CarInt_PgUP();
void CarInt_CtrlPgDN();
void CarInt_CtrlPgUP();

//---------------------------------------------------------------------------
// finestra: Feeders database
//---------------------------------------------------------------------------
FeedersDatabaseUI::FeedersDatabaseUI( CWindow* parent, bool select ) : CWindowTable( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_EDIT );
	SetClientAreaPos( 0, 6 );
	SetClientAreaSize( 71, 9 );
	SetTitle( MsgGetString(Msg_01271) );

	selectMode = select;

	assFeederWin = new CurrentFeederUI( this );

	SM_IventoryExport = new GUI_SubMenu();
	SM_IventoryExport->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &FeedersDatabaseUI::toSharedFolder, this ) ); // shared network
	SM_IventoryExport->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &FeedersDatabaseUI::toUSBDevice, this ) ); // usb pen
}

FeedersDatabaseUI::~FeedersDatabaseUI()
{
	delete SM_IventoryExport;
	delete assFeederWin;
}

void FeedersDatabaseUI::onInit()
{
	// create table
	m_table = new CTable( 3, 2, MAXCARINT, TABLE_STYLE_HIGHLIGHT_ROW, this );

	// add columns
	m_table->AddCol( MsgGetString(Msg_01623), 1, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE ); //tipo caricatore
	m_table->AddCol( MsgGetString(Msg_01274), 7, CELL_TYPE_UINT ); //seriale
	m_table->AddCol( MsgGetString(Msg_01343), 10, CELL_TYPE_UINT ); //seriale utente
	m_table->AddCol( MsgGetString(Msg_01275), 2, CELL_TYPE_UINT, CELL_STYLE_READONLY );
	m_table->AddCol( MsgGetString(Msg_00060), 41, CELL_TYPE_TEXT );

	// set params
	m_table->SetColLegalChars( 0, "ATGatg" );
	m_table->SetColMinMax( 1, 0, 999999 );
	m_table->SetColMinMax( 2, 0, 999999 );
	m_table->SetColMinMax( 3, 0, 99 );
}

void FeedersDatabaseUI::onShow()
{
	assFeederWin->Show( false, false );
	assFeederWin->DeselectCells();

	feedersDatabaseRefresh = true;
}

void FeedersDatabaseUI::onRefresh()
{
	if( feedersDatabaseRefresh )
	{
		feedersDatabaseRefresh = false;
		forceRefresh();
	}
}

void FeedersDatabaseUI::onEdit()
{
	int newserial = 0;
	int row = m_table->GetCurRow();
	int col = m_table->GetCurCol();

	//prima di inserire qualsiasi altro dato e' necessario specifiare un tipo caricatore valido e un numero di serie
	int cod = CarList[CarList_CurRecord].serial & 0xFFFFFF;
	if( ( cod == NEW_RECORD || cod == 0 ) && col > 1 )
	{
		return;
	}

	if( cod != NEW_RECORD )
	{
		if(CarInt_GetUsedState(CarList[CarList_CurRecord])==MAG_USEDBY_OTHERS)
		{
			W_Mess( ERRMAG_USEDBY_OTHERS );
			return;
		}
	}

	if( col == 0 )
	{
		// tipo caricatore
		char tmp[4];
		snprintf( tmp, 4, "%s", m_table->GetText( row, col ) );

		int tmp_cod = cod;

		switch( tmp[0] )
		{
			case 'G':
				cod |= (CARINT_GENERICSER_OFFSET << 24);
				break;

			case 'A':
				cod |= (CARINT_AIRSER_OFFSET << 24);
				break;

			case 'T':
			default:
				cod |= (CARINT_TAPESER_OFFSET << 24);
				break;
		}

		//no numero di serie / nuovo record
		if( ( tmp_cod == 0 || tmp_cod == NEW_RECORD ) && col == 0 )
		{
			//file da non aggiornare
			CarList[CarList_CurRecord].serial = cod;
			// usato da nessuno
			CarList[CarList_CurRecord].usedBy = MAG_USEDBY_NONE;
			return;
		}

		for( int i = 0; i < CarList_nRec; i++ )
		{
			if( CarList[i].serial == cod )
			{
				//numero di serie gia presente in lista
				return;
			}
		}


		if(((CarList[CarList_CurRecord].serial >> 24) & 0xFF)!=((cod >> 24) & 0xFF))
		{
			//numero di serie cambiato: reinit necessario (usa newserial come flag di attivazione)
			newserial = cod;
		}


		//se il record attuale e' un nuovo record
		if( tmp_cod == NEW_RECORD )
		{
			//Inserimento dati nel primo record disponibile
			CarList[CarList_CurRecord].idx = FreeList[FirstFree_idx++];
			//tipo default
			CarList[CarList_CurRecord].smart = 0;
			//Decremento record liberi disponibili
			Free_nRec--;
			//setta per reinit
			newserial = cod;
		}
		else
		{
			//Aggiornamento dati presenti se necessario
			if( CarList[CarList_CurRecord].serial != cod )
			{
				//il numero di serie e' cambiato: reinit
				newserial = cod;
			}
		}

		CarList[CarList_CurRecord].serial = cod;
	}
	else if( col == 1 )
	{
		if(((CarList[CarList_CurRecord].serial >> 24) & 0xFF)==0)
		{
			//tipo caricatore non valido o non settato
			return;
		}

		cod = m_table->GetInt( row, col );
		if( cod == 0 )
		{
			//numero di serie nullo
			return;
		}

		//numero di serie logico: in parte alta tipo caricatore
		cod |= (CarList[CarList_CurRecord].serial & 0xFF000000);

		//ricerca del numero di serie logico (stesso numero di serie e stesso tipo caricatore)
		for( int i = 0; i < CarList_nRec; i++ )
		{
			if( CarList[i].serial == cod )
			{
				//numero di serie gia presente in lista
				return;
			}
		}

		//Se il record corrente e' nuovo
		if((CarList[CarList_CurRecord].serial & 0xFFFFFF)==NEW_RECORD)
		{
			//Inserimento dati nel primo record disponibile
			CarList[CarList_CurRecord].idx=FreeList[FirstFree_idx++];
			//tipo default
			CarList[CarList_CurRecord].smart = 0;
			//Decrementa numero di record disponibili
			Free_nRec--;
			//flag x reinit
			newserial = cod;
		}
		else
		{
			//Aggiornamento dati presenti se necessario
			if(CarList[CarList_CurRecord].serial!=cod)
			{
				newserial = cod;
			}
		}

		CarList[CarList_CurRecord].serial = cod;
	}
	else if( col == 2 )
	{
		CarList[CarList_CurRecord].serialUser = m_table->GetInt( row, col );
	}
	else if( col == 4 )
	{
		snprintf( CarList[CarList_CurRecord].note, 43, "%s", m_table->GetText( row, col ) );
	}

	#ifdef CARINT_CHECKSUM
	CarList[CarList_CurRecord].checksum=CarInt_CalcChecksum(CarList[CarList_CurRecord]);

	if(CarList[CarList_CurRecord].mounted)
	{
		CarInt_SetData(CarList[CarList_CurRecord].mounted-1,CARINT_CHKSUMREC,CarList[CarList_CurRecord].checksum);
	}
	#endif

	CarList[CarList_CurRecord].changed = CARINT_NONET_CHANGED;
	if(IsNetEnabled())
	{
		if(DBRemote.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
		{
			CarList[CarList_CurRecord].changed=0;
		}
	}

	DBLocal.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,1);

	if( newserial )
	{
		InitCarList();

		CarList_CurRecord=0;
		CarList_CurRow=0;
		CarList_StartTabe=0;

		for(int i=0;i<CarList_nRec;i++)
		{
			if(CarList[i].serial==newserial)
			{
				CarList_CurRecord=i;
				CarList_StartTabe=i;
				break;
			}
		}

		feedersDatabaseRefresh = true;
	}
}

void FeedersDatabaseUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_00888), K_F1, 0, NULL, boost::bind( &FeedersDatabaseUI::onFeederData, this ) ); // Feeder data
	m_menu->Add( MsgGetString(Msg_00247), K_F2, 0, NULL, boost::bind( &FeedersDatabaseUI::onEditOverride, this ) ); // edit
	m_menu->Add( MsgGetString(Msg_01282), K_F4, 0, NULL, boost::bind( &FeedersDatabaseUI::onFindSerial, this ) ); // Find serial number
	m_menu->Add( MsgGetString(Msg_01782), K_F5, 0, NULL, CarInt_FindSerCostr ); // Find user's serial
	m_menu->Add( MsgGetString(Msg_01280), K_F6, 0, NULL, CarInt_FindMag ); // Find feeder pack number
	m_menu->Add( MsgGetString(Msg_01344), K_F7, 0, NULL, CarInt_FindNote ); // Find notes
	m_menu->Add( MsgGetString(Msg_01353), K_F8, 0, NULL, CarInt_FindCar ); // Find feeder
	m_menu->Add( MsgGetString(Msg_01773), K_F9, 0, NULL, CarInt_AutoMag ); // Feeder identification
	m_menu->Add( MsgGetString(Msg_01303), K_F11, 0, NULL, CarInt_ConfImport ); // Feeder data import
	m_menu->Add( MsgGetString(Msg_01705), K_SHIFT_F3, 0, NULL, CarInt_AdvSearch ); // Advanced search
	m_menu->Add( MsgGetString(Msg_01924), K_SHIFT_F5, (CarList_nRec != 0) ? 0 : SM_GRAYED, SM_IventoryExport, NULL ); // Export inventory
	m_menu->Add( MsgGetString(Msg_01329), K_SHIFT_F6, 0, NULL, CarInt_UnMount ); // Unmount feeder
}

bool FeedersDatabaseUI::onKeyPress( int key )
{
	switch( key )
	{
		case K_F1:
			return onFeederData();

		case K_F2:
			return onEditOverride();

		case K_F4:
			onFindSerial();
			return true;

		case K_F6:
			CarInt_FindMag();
			return true;

		case K_F7:
			CarInt_FindNote();
			return true;

		case K_F8:
			CarInt_FindCar();
			return true;

		case K_F9:
			CarInt_AutoMag();
			return true;

		case K_F11:
			CarInt_ConfImport();
			return true;

		case K_SHIFT_F3:
			CarInt_AdvSearch();
			return true;

		case K_SHIFT_F5:
			SM_IventoryExport->Show();
			return true;

		case K_SHIFT_F6:
			CarInt_UnMount();
			return true;

		case K_DEL:
			if( guiDeskTop->GetEditMode() )
			{
				CarInt_DelRec();
				return true;
			}
			break;

		case K_DOWN:
		case K_UP:
		case K_PAGEDOWN:
		case K_PAGEUP:
		case K_CTRL_PAGEUP:
		case K_CTRL_PAGEDOWN:
			return vSelect( key );

		case K_ESC:
			m_exitCode = WIN_EXITCODE_ESC;
			break;

		case K_ENTER:
			if( selectMode )
			{
				if(CarInt_GetUsedState(CarList[CarList_CurRecord])==MAG_USEDBY_OTHERS)
				{
					W_Mess(ERRMAG_USEDBY_OTHERS);
				}
				else
				{
					forceExit();
					m_exitCode = WIN_EXITCODE_ENTER;
					return true;
				}
			}
			break;

		default:
			break;
	}

	return false;
}

void FeedersDatabaseUI::onClose()
{
	assFeederWin->Hide();
}

int FeedersDatabaseUI::onEditOverride()
{
	guiDeskTop->SetEditMode( true );
	m_table->SetEdit( true );
	feedersDatabaseRefresh = true;
	return 1;
}

int FeedersDatabaseUI::onFeederData()
{
	GUI_Freeze();
	m_table->SetRowStyle( m_table->GetCurRow(), TABLEROW_STYLE_SELECTED_GREEN );
	m_table->Deselect();
	assFeederWin->SelectFirstCell();
	GUI_Thaw();

	assFeederWin->SetFocus();

	GUI_Freeze();
	assFeederWin->DeselectCells();
	m_table->SetRowStyle( m_table->GetCurRow(), TABLEROW_STYLE_DEFAULT );
	m_table->Select( m_table->GetCurRow(), m_table->GetCurCol() );

	if( !(GetStyle() & WIN_STYLE_NO_MENU) && !(GetStyle() & WIN_STYLE_NO_EDIT) )
	{
		m_table->SetEdit( guiDeskTop->GetEditMode() );
	}
	GUI_Thaw();
	return 1;
}

//---------------------------------------------------------------------------------
// Trova il numero seriale specificato
//---------------------------------------------------------------------------------
int FeedersDatabaseUI::onFindSerial()
{
	AskFeederSerialUI inputBox( this, MsgGetString(Msg_01282), 'T', 0 );
	inputBox.Show();
	inputBox.Hide();
	if( inputBox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	char type = inputBox.GetType();
	int ser = inputBox.GetSerial() & 0xFFFFFF;

	switch(type)
	{
		case 'A':
			ser |= (CARINT_AIRSER_OFFSET << 24);
			break;
		case 'G':
			ser |= (CARINT_GENERICSER_OFFSET << 24);
			break;
		case 'T':
		default:
			ser |= (CARINT_TAPESER_OFFSET << 24);
			break;
	}

	for(int index=0; index<CarList_nRec; index++)
	{
		if(CarList[index].serial == ser)
		{
			CarInt_SearchResults(index);
			return 1;
		}
	}

	W_Mess(FINDSER_ERR);
	return 0;
}

//---------------------------------------------------------------------------------
// Esporta inventario in cartella condivisa
//---------------------------------------------------------------------------------
int FeedersDatabaseUI::toSharedFolder()
{
	// input file name
	CInputBox inbox( 0, 6, MsgGetString(Msg_01924), MsgGetString(Msg_01925), 40, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	// check file
	char sharedFile[MAXNPATH];
	snprintf( sharedFile, MAXNPATH, "%s/%s.txt", SHAREDIR, inbox.GetText() );

	if( !CheckDir(SHAREDIR,CHECKDIR_CREATE) )
	{
		W_Mess( MsgGetString(Msg_01841) );
		return 0;
	}

	if( CheckFile( sharedFile ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_01926) ) )
		{
			return 0;
		}
	}

	// export
	if( !exportInventory( sharedFile ) )
	{
		W_Mess( MsgGetString(Msg_00195) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00196) );
	return 1;
}

//---------------------------------------------------------------------------------
// Esporta inventario in dispositivo usb
//---------------------------------------------------------------------------------
extern std::string fn_Select( CWindow* parent, const std::string& title, const std::vector<std::string>& items );

int FeedersDatabaseUI::toUSBDevice()
{
	// choose device
	std::vector<std::string> usb_mountpoints;
	if( !getAllUsbMountPoints( usb_mountpoints ) )
	{
		W_Mess( MsgGetString(Msg_01757) );
		return 0;
	}

	std::string selectedDevice = fn_Select( this, MsgGetString(Msg_02105), usb_mountpoints );
	if( selectedDevice.empty() )
	{
		return 0;
	}

	// input file name
	CInputBox inbox( 0, 6, MsgGetString(Msg_01924), MsgGetString(Msg_01925), 40, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	// check file
	char deviceFile[MAXNPATH];
	snprintf( deviceFile, MAXNPATH, "%s/%s.txt", selectedDevice.c_str(), inbox.GetText() );

	if( CheckFile( deviceFile ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_01799) ) )
		{
			return 0;
		}
	}

	// export
	if( !exportInventory( deviceFile ) )
	{
		W_Mess( MsgGetString(Msg_00195) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00196) );
	return 1;
}

//---------------------------------------------------------------------------------
// Esporta inventario
//---------------------------------------------------------------------------------
bool FeedersDatabaseUI::exportInventory( char* filename )
{
	int* done = new int[CarList_nRec*8];
	for( int i = 0; i < CarList_nRec*8; i++ )
	{
		done[i] = 0;
	}

	char buf1[26];
	char buf2[26];

	std::vector<_inventoryItem> inventory;

	for( int i = 0; i < CarList_nRec; i++ )
	{
		for( int j = 0; j < 8; j++ )
		{
			if( done[i*8+j] )
			{
				continue;
			}
			done[i*8+j] = 1;

			strncpyQ( buf1, CarList[i].elem[j], sizeof(buf1) );
			DelSpcR( buf1 );

			if( *buf1 == '\0' )
			{
				continue;
			}

			_inventoryItem item;
			strncpyQ( item.name, buf1, sizeof(item.name) );
			strncpyQ( item.pack, CarList[i].pack[j], sizeof(item.pack) );
			item.quant = CarList[i].num[j];

			for( int k = i+1; k < CarList_nRec; k++ )
			{
				for( int y = 0; y < 8; y++ )
				{
					if( done[k*8+y] )
					{
						continue;
					}

					strncpyQ( buf2, CarList[k].elem[y], sizeof(buf2) );
					DelSpcR( buf2 );

					if( *buf2 == '\0' )
					{
						done[k*8+y] = 1;
						continue;
					}

					if( !strncasecmpQ( buf1, buf2, sizeof(buf1) ) )
					{
						done[k*8+y] = 1;
						item.quant += CarList[k].num[y];
					}
				}
			}

			inventory.push_back( item );
		}
	}

	delete [] done;

	if( inventory.empty() )
	{
		return true;
	}

	CPan pan( -1, 1, MsgGetString(Msg_01927) );

	FILE* f = FilesFunc_fopen( filename, "wt" );
	if( f == NULL )
	{
		return false;
	}
	else
	{
		for( unsigned int i = 0; i < inventory.size(); i++ )
		{
			fprintf( f, "%s;%s;%d\n", inventory[i].name, inventory[i].pack, inventory[i].quant );
		}
		FilesFunc_fclose( f );
	}

	return true;
}


void FeedersDatabaseUI::forceRefresh()
{
	GUI_Freeze_Locker lock;

	for( unsigned int i = 0; i < m_table->GetRows(); i++ )
	{
		int row = i + CarList_StartTabe;
		if( row > CarList_nRec )
		{
			// clear
			m_table->SetText( i, 0, "" );
			m_table->SetText( i, 1, "" );
			m_table->SetText( i, 2, "" );
			m_table->SetText( i, 3, "" );
			m_table->SetText( i, 4, "" );
		}
		else
		{
			if( CarList[row].usedBy == 0 || !IsNetEnabled() )
			{
				m_table->SetRowStyle( i, TABLEROW_STYLE_DEFAULT );
			}
			else
			{
				if( CarList[row].usedBy == nwpar.NetID_Idx )
				{
					m_table->SetRowStyle( i, TABLEROW_STYLE_HIGHLIGHT_GREEN );
				}
				else
				{
					m_table->SetRowStyle( i, TABLEROW_STYLE_HIGHLIGHT_RED );
				}
			}

			if( (CarList[row].serial & 0xFFFFFF) != NEW_RECORD )
			{
				switch((CarList[row].serial >> 24) & 0xFF)
				{
					case CARINT_GENERICSER_OFFSET:
						m_table->SetText( i, 0, "G" );
						break;
					case CARINT_AIRSER_OFFSET:
						m_table->SetText( i, 0, "A" );
						break;
					case CARINT_TAPESER_OFFSET:
						m_table->SetText( i, 0, "T" );
						break;
					default:
						m_table->SetText( i, 0, "X" );
						break;
				}

				m_table->SetText( i, 1, (CarList[row].serial & 0xFFFFFF) );
			}
			else
			{
				m_table->SetText( i, 0, "" );
				m_table->SetText( i, 1, "" );
			}

			if( CarList[row].serialUser )
			{
				m_table->SetText( 2, 1, CarList[row].serialUser );
			}
			else
			{
				m_table->SetText( i, 2, "" );
			}

			if( (CarList[row].mounted==0) || (IsNetEnabled() && (CarList[row].usedBy!=nwpar.NetID_Idx)) )
			{
				m_table->SetText( i, 3, "" );
			}
			else
			{
				m_table->SetText( i, 3, CarList[row].mounted );
			}

			m_table->SetText( i, 4, CarList[row].note );
		}
	}

	m_table->Select( CarList_CurRow, m_table->GetCurCol() );
	assFeederWin->UpdateFeeder();
	showUsedBy();
}

//--------------------------------------------------------------------------
// Sposta la selezione verticalmente
//--------------------------------------------------------------------------
bool FeedersDatabaseUI::vSelect( int key )
{
	if( m_table->GetCurRow() < 0 )
		return false;

	if( key == K_DOWN )
	{
		CarIntIncRow();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_UP )
	{
		CarIntDecRow();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_PAGEDOWN )
	{
		CarInt_PgDN();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_PAGEUP )
	{
		CarInt_PgUP();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_CTRL_PAGEDOWN )
	{
		CarInt_CtrlPgDN();
		// aggiorno la tabella
		return true;
	}
	else if( key == K_CTRL_PAGEUP )
	{
		CarInt_CtrlPgUP();
		// aggiorno la tabella
		return true;
	}

	return false;
}

void FeedersDatabaseUI::showUsedBy()
{
	GUI_Freeze_Locker lock;

	char buf[52];
	InsSpc( 50, buf );
	DrawText( 3, 8, buf );

	if( !IsNetEnabled() )
	{
		return;
	}

	if( CarInt_GetUsedState(CarList[CarList_CurRecord]) == MAG_USEDBY_OTHERS )
	{
		char name[51];
		if(GetMachineName(CarList[CarList_CurRecord].usedBy,name))
		{
			name[30]='\0';

			snprintf( buf, 50, MsgGetString(Msg_02008), name );
			DrawText( 3, 8, buf );
		}
	}
}


//---------------------------------------------------------------------------
// finestra: Caricatore corrente (associato al magazzino)
//---------------------------------------------------------------------------
CurrentFeederUI::CurrentFeederUI( CWindow* parent ) : CWindowTable( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X );
	SetClientAreaPos( 0, 18 );
	SetClientAreaSize( 87, 11 );
	SetTitle( MsgGetString(Msg_00888) );
}

void CurrentFeederUI::SelectFirstCell()
{
	if( m_table )
	{
		m_table->Select( 0, 1 );
	}
}

void CurrentFeederUI::DeselectCells()
{
	if( m_table )
	{
		m_table->Deselect();
	}
}

void CurrentFeederUI::UpdateFeeder()
{
	onRefresh();
}

void CurrentFeederUI::onInit()
{
	// create table
	m_table = new CTable( 3, 2, 8, TABLE_STYLE_DEFAULT, this );

	// add columns
	m_table->AddCol( MsgGetString(Msg_01289), 1, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
	m_table->AddCol( MsgGetString(Msg_01290), 25, CELL_TYPE_TEXT );
	m_table->AddCol( MsgGetString(Msg_00678), 21, CELL_TYPE_TEXT, CELL_STYLE_READONLY );
	m_table->AddCol( MsgGetString(Msg_01034), 20, CELL_TYPE_TEXT );
	m_table->AddCol( MsgGetString(Msg_01306), 5, CELL_TYPE_UINT );
	m_table->AddCol( MsgGetString(Msg_01622), 2, CELL_TYPE_TEXT );
	m_table->AddCol( MsgGetString(Msg_01623), 1, CELL_TYPE_TEXT );

	// set params
	m_table->SetColMinMax( 4, 0, 99999 );
	m_table->SetColLegalStrings( 5, 8, (char**)CarAvType_StrVect );
	#ifndef __DOME_FEEDER
	m_table->SetColLegalStrings( 6, 3, (char**)CarType_StrVect );
	#else
	m_table->SetColLegalStrings( 6, 4, (char**)CarType_StrVect );
	#endif

	// set data
	for( int i = 0; i < 8; i++ )
	{
		m_table->SetText( i, 0, i + 1 );
	}
}

void CurrentFeederUI::onRefresh()
{
	GUI_Freeze_Locker lock;

	for( unsigned int i = 0; i < m_table->GetRows(); i++ )
	{
		m_table->SetText( i, 1, CarList[CarList_CurRecord].elem[i] );
		m_table->SetText( i, 2, CarList[CarList_CurRecord].pack[i] );
		m_table->SetText( i, 3, CarList[CarList_CurRecord].noteCar[i] );
		m_table->SetText( i, 4, CarList[CarList_CurRecord].num[i] );
		m_table->SetStrings_Pos( i, 5, CarList[CarList_CurRecord].tavanz[i] );
		m_table->SetStrings_Pos( i, 6, CarList[CarList_CurRecord].tipoCar[i] );
	}
}

void CurrentFeederUI::onEdit()
{
	if( IsNetEnabled() )
	{
		if( CarList[CarList_CurRecord].usedBy != 0 && CarList[CarList_CurRecord].usedBy != nwpar.NetID_Idx )
		{
			W_Mess(ERRMAG_USEDBY_OTHERS);
			return;
		}
	}

	int r = m_table->GetCurRow();
	snprintf( CarList[CarList_CurRecord].elem[r], 26, "%s", m_table->GetText( r, 1 ) );
	snprintf( CarList[CarList_CurRecord].pack[r], 21, "%s", m_table->GetText( r, 2 ) );
	snprintf( CarList[CarList_CurRecord].noteCar[r], 21, "%s", m_table->GetText( r, 3 ) );
	CarList[CarList_CurRecord].num[r] = m_table->GetInt( r, 4 );
	CarList[CarList_CurRecord].tavanz[r] = m_table->GetStrings_Pos( r, 5 );
	CarList[CarList_CurRecord].tipoCar[r] = m_table->GetStrings_Pos( r, 6 );

	CarList[CarList_CurRecord].changed = CARINT_NONET_CHANGED;

	if( IsNetEnabled() )
	{
		if(DBRemote.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
		{
			CarList[CarList_CurRecord].changed = 0;
		}
	}

	DBLocal.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,1);
}

void CurrentFeederUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_01142), K_F11, 0, NULL, boost::bind( &CurrentFeederUI::OnPackagesLib, this ) ); // Tabella packages
}

bool CurrentFeederUI::onKeyPress( int key )
{
	switch( key )
	{
		case K_F11:
			OnPackagesLib();
			return true;

		case K_DEL:
			OnDelete();
			return true;

		default:
			break;
	}

	return false;
}

int CurrentFeederUI::OnDelete()
{
	if( !m_table->GetEdit() )
	{
		return 0;
	}

	if(IsNetEnabled())
	{
		if( CarList[CarList_CurRecord].usedBy != 0 && CarList[CarList_CurRecord].usedBy != nwpar.NetID_Idx )
		{
			W_Mess(ERRMAG_USEDBY_OTHERS);
			return 0;
		}
	}

	if( !W_Deci(0,ASK_CONFIRMDELETE2) )
	{
		return 0;
	}

	int row = m_table->GetCurRow();
	CarList[CarList_CurRecord].packIdx[row]=-1;
	*CarList[CarList_CurRecord].pack[row]=0;
	*CarList[CarList_CurRecord].noteCar[row]=0;
	*CarList[CarList_CurRecord].elem[row]=0;
	CarList[CarList_CurRecord].num[row]=0;
	CarList[CarList_CurRecord].tipoCar[row]=0;
	CarList[CarList_CurRecord].tavanz[row]=0;

	if((CarList[CarList_CurRecord].serial & 0xFFFFFF)<NEW_RECORD)
	{
		CarList[CarList_CurRecord].changed=CARINT_NONET_CHANGED;

		if(IsNetEnabled())
		{
			if(DBRemote.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
			{
				CarList[CarList_CurRecord].changed = 0;
			}
		}

		DBLocal.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,1);
	}
	return 1;
}

int CurrentFeederUI::OnPackagesLib()
{
	int packIdx;
	std::string packName;

	if( IsNetEnabled() )
	{
		if( CarList[CarList_CurRecord].usedBy != 0 && CarList[CarList_CurRecord].usedBy != nwpar.NetID_Idx )
		{
			W_Mess(ERRMAG_USEDBY_OTHERS);
			return 0;
		}
	}

	// Gestione tabella packages
	int row = m_table->GetCurRow();
	if( CarList[CarList_CurRecord].elem[row][0] != '\0' )
	{
		if( fn_PackagesTableSelect( this, CarList[CarList_CurRecord].pack[row], packIdx, packName ) )
		{
			strcpy(CarList[CarList_CurRecord].pack[row],packName.c_str());
			CarList[CarList_CurRecord].packIdx[row] = packIdx;
			CarList[CarList_CurRecord].changed = CARINT_NONET_CHANGED;

			if( IsNetEnabled() )
			{
				if(DBRemote.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
				{
					CarList[CarList_CurRecord].changed=0;
				}
			}

			DBLocal.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,1);
		}
	}
	return 1;
}




//confronta un magazzino nella configurazione caricatori con
//uno del database (configurazione intereamente caricata in memoria)
int CompareConfDB(int mag,struct CarDat *car,CarInt_data db,int mode=0,int *mod_fields=NULL)
{
	int found=0;
	
	mag--;
	
	int modf=0;
	
	for(int n=0;n<8;n++)
	{
		if(!strcasecmpQ(db.elem[n],car[mag*8+n].C_comp))
		{
			if(!strcasecmpQ(db.pack[n],car[mag*8+n].C_Package))
			{
				if(db.tavanz[n]==car[mag*8+n].C_att)
				{
					if( db.tipoCar[n] == car[mag*8+n].C_tipo )
					{
						//print_debug("+");
						if(!mode)
						{
							found++;
						}
						else
						{
							if(db.num[n]==car[mag*8+n].C_quant)
							{
								if(!strcasecmpQ(db.noteCar[n],car[mag*8+n].C_note))
								{
									found++;
								}
								else
								{
									modf|=CARINT_FIELD_NOTE;
								}
							}
							else
							{
								modf|=CARINT_FIELD_QUANT;
							}
						}
					}
					else
					{
						modf|=CARINT_FIELD_TYPE;
					}
				}
				else
				{
					modf|=CARINT_FIELD_AVTYPE;
				}
			}
			else
			{
				modf|=CARINT_FIELD_PACKAGE;
			}
		}
		else
		{
			modf|=CARINT_FIELD_TIPCOM;
		}
	}

	if(mod_fields!=NULL)
	{
		*mod_fields=modf;
	}
	
	return(found==8);
}

//confronta un magazzino nella configurazione caricatori con
//uno del database (legge elementi della configurazione da file)
int CompareConfDB(int mag,FeederFile *file,CarInt_data db,int mode=0,int *mod_fields=NULL)
{
  int found=0;

  mag--;

  int modf=0;

  struct CarDat car;

  for(int n=0;n<8;n++)
  {
    file->ReadRec(mag*8+n,car);

    if(!strcasecmpQ(db.elem[n],car.C_comp))
    {
      if(!strcasecmpQ(db.pack[n],car.C_Package))
      {
        if(db.tavanz[n]==car.C_att)
        {
          if( db.tipoCar[n] == car.C_tipo )
          {
            if(!mode)
            {
              found++;
            }
            else
            {
              if(db.num[n]==car.C_quant)
              {
                if(!strcasecmpQ(db.noteCar[n],car.C_note))
                {
                  found++;
                }
                else
                {
                  modf|=CARINT_FIELD_NOTE;
                }
              }
              else
              {
                modf|=CARINT_FIELD_QUANT;
              }
            }
          }
          else
          {
            modf|=CARINT_FIELD_TYPE;
          }
        }
        else
        {
          modf|=CARINT_FIELD_AVTYPE;
        }
      }
      else
      {
        modf|=CARINT_FIELD_PACKAGE;
      }
    }
    else
    {
      modf|=CARINT_FIELD_TIPCOM;
    }
  }

  if(mod_fields!=NULL)
  {
    *mod_fields=modf;
  }

  return(found==8);
}




//--------------------------------------------------------------------------

//Dealloca strutture dati
void DeAllocCarInt(void)
{
  if(CarList!=NULL)
  {
    delete [] CarList;
    CarList=NULL;
  }

  if(FreeList!=NULL)
  {
    delete[] FreeList;
    FreeList=NULL;
  }
}

//Alloca e inizalizza strutture dati
int InitCarList(void)
{
	if( CarList )
	{
		delete [] CarList;
		CarList = NULL;
	}

	if(FreeList)
	{
		delete [] FreeList;
		FreeList = NULL;
	}

	DBLocal.InitList(&CarList,&FreeList,CarList_nRec,Free_nRec);

	FirstFree_idx=0;

	//print_debug("Local nRec=%d FreeRec=%d Total=%d\n",CarList_nRec,Free_nRec,CarList_nRec+Free_nRec);

	SortData((void *)CarList,SORTFIELDTYPE_INT32,CarList_nRec,((mem_pointer)&(CarList[0].serial))-((mem_pointer)CarList),sizeof(struct CarInt_data),0);

	return 1;
}


//Cancella una riga
void CarInt_DelRec(void)
{
	//controlla che il record attuale sia valido: no nuovo record
	if((CarList[CarList_CurRecord].serial & 0xFFFFFF)>=NEW_RECORD)
	{
		bipbip();
		return;
	}

	if(IsNetEnabled())
	{
		if((CarList[CarList_CurRecord].usedBy!=0) && (CarList[CarList_CurRecord].usedBy!=nwpar.NetID_Idx))
		{
			W_Mess(ERRMAG_USEDBY_OTHERS);
			return;
		}
	}

	char buf[80];

	switch((CarList[CarList_CurRecord].serial >> 24) & 0xFF)
	{
		case CARINT_TAPESER_OFFSET:
			snprintf( buf, sizeof(buf), ASK_CONFIRMDELETE,'T',CarList[CarList_CurRecord].serial & 0xFFFFFF);
			break;
		case CARINT_AIRSER_OFFSET:
			snprintf( buf, sizeof(buf), ASK_CONFIRMDELETE,'A',CarList[CarList_CurRecord].serial & 0xFFFFFF);
			break;
		case CARINT_GENERICSER_OFFSET:
			snprintf( buf, sizeof(buf), ASK_CONFIRMDELETE,'G',CarList[CarList_CurRecord].serial & 0xFFFFFF);
			break;
		default:
			snprintf( buf, sizeof(buf), ASK_CONFIRMDELETE,'X',CarList[CarList_CurRecord].serial & 0xFFFFFF);
			break;
	}


	if(!W_Deci(0,buf))
	{
		return;
	}

	int prev_nrec=CarList_nRec;
	int prev_rec=CarList_CurRecord;

	CarList[CarList_CurRecord].serial=FREE_RECORD;

	CarList[CarList_CurRecord].changed=CARINT_NONET_CHANGED;

	if(IsNetEnabled())
	{
		if(DBRemote.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
		{
			CarList[CarList_CurRecord].changed=0;
		}
	}

	DBLocal.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,1);

	InitCarList();

	if(CarList_nRec==0)
	{
		CarList_CurRecord=0;
		CarList_CurRow=0;
	}
	else
	{
		if(prev_rec==prev_nrec)
		{
			CarList_CurRecord--;

			if(CarList_CurRow!=0)
			{
				CarList_CurRow--;
			}
			else
			{
				CarList_StartTabe--;
			}
		}
	}

	feedersDatabaseRefresh = true;
}

// Gestore cursore DOWN
void CarIntIncRow(void)
{
	if((CarList[CarList_CurRecord].serial & 0xFFFFFF)==NEW_RECORD)
	{
		return;
	}

	CarList_CurRow++;
	CarList_CurRecord++;

	if(CarList_CurRow==MAXCARINT)
	{
		CarList_CurRow=MAXCARINT-1;
		CarList_StartTabe++;
	}

	feedersDatabaseRefresh = true;
}

// Gestore cursore UP
void CarIntDecRow(void)
{
	if(CarList_CurRecord==0)
	{
		return;
	}

	CarList_CurRecord--;
	CarList_CurRow--;

	if(CarList_CurRow<0)
	{
		CarList_CurRow=0;
		CarList_StartTabe--;
	}

	feedersDatabaseRefresh = true;
}




// Trova il magazzino specificato
// SMOD261104
void CarIntFindMag()
{
	CInputBox inbox( 0, 6, MsgGetString(Msg_01280), MsgGetString(Msg_01286), 4, CELL_TYPE_UINT );
	inbox.SetVMinMax( 1, MAXMAG );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return;
	}

	int value = inbox.GetInt();
	int index;

	for( index = 0; index < CarList_nRec; index++ )
	{
		if( CarList[index].mounted == value )
		{
			break;
		}
	}

	if(index < CarList_nRec)
	{
		CarInt_SearchResults(index);
	}
	else
	{
		W_Mess(FINDMAG_ERR);
	}
}

void CarIntFindSerCostr(void)
{
	CInputBox inbox( 0, 6, MsgGetString(Msg_01782), MsgGetString(Msg_01781), 6, CELL_TYPE_UINT );
	inbox.SetVMinMax( 1, 999999 );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return;
	}

	int value = inbox.GetInt();
	int index;

	while( 1 )
	{
		for(index=0; index<CarList_nRec; index++)
		{
			if( CarList[index].serialUser == value )
			{
				break;
			}
		}

		if(index < CarList_nRec)
		{
			CarInt_SearchResults(index);

			if(!W_Deci(1,MORE_NOTES))
			{
				break;
			}
		}
		else
		{
			W_Mess(FINDMAG_ERR);
			break;
		}
	}
}



// Trova il testo specificato nel campo note
// SMOD261104
void CarIntFindNote()
{
	CInputBox inbox( 0, 6, MsgGetString(Msg_01344), MsgGetString(Msg_01345), 40, CELL_TYPE_TEXT );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return;
	}

	char searchtxt[41];
	snprintf( searchtxt, 41, "%s", inbox.GetText() );

	int index, startVal=0;

	while( 1 )
	{
		for(index=startVal; index<CarList_nRec; index++)
		{
			if(strstr(CarList[index].note, searchtxt))
			{
				break;
			}
		}

		if(index<CarList_nRec)
		{
			startVal=index+1;

			CarInt_SearchResults(index);

			if(!W_Deci(1,MORE_NOTES))
			{
				break;
			}
		}
		else
		{
			W_Mess(FINDNOTE_ERR);
			break;
		}
	}
}

// Trova il caricatore con i dati specificati
// SMOD261104
void CarIntFindCar()
{
	CInputBox inbox( 0, 6, MsgGetString(Msg_01353), MsgGetString(Msg_01354), 25, CELL_TYPE_TEXT );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return;
	}

	char searchtxt[41];
	snprintf( searchtxt, 41, "%s", inbox.GetText() );


	int index, indexCar, startVal=0;

	while( 1 )
	{
		for(index=startVal; index<CarList_nRec; index++)
		{
			for(indexCar=0; indexCar<8; indexCar++)
			{
				if(strstr(CarList[index].elem[indexCar], searchtxt))
					break;
			}

			if(indexCar<8)
			{
				break;
			}
		}

		if( index < CarList_nRec )
		{
			startVal=index+1;

			CarInt_SearchResults(index);

			feedersDatabaseRefresh = true;

			if(!W_Deci(1,MORE_NOTES,MSGBOX_YUPPER))
			{
				break;
			}
		}
		else
		{
			W_Mess(FINDCAR_ERR);
			break;
		}
	}
}



void CarIntAuto_ShowResults( int* tab )
{
	CWindow* wind = new CWindow( 0 );
	wind->SetStyle( WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
	wind->SetClientAreaSize( 60, 25 );
	wind->SetTitle( MsgGetString(Msg_01773) );

	CComboList* CLSer = new CComboList( wind );
	C_Combo* CSer[2][MAXMAG];

	for(int i=0;i<MAXMAG;i++)
	{
		char buf[64];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01738), i+1 );
	
		CSer[0][i] = new C_Combo( 5, 3+i, buf, 1, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE );
		CSer[0][i]->SetLegalChars( "ATG" );

		CSer[1][i] = new C_Combo( 25, 3+i, "", 8, CELL_TYPE_UINT );
		CSer[1][i]->SetVMinMax( 1, 999999 );
	
		CLSer->Add( CSer[0][i], i, 0 );
		CLSer->Add( CSer[1][i], i, 1 );
	}

	GUI_Freeze();

	wind->Show();
	CLSer->Show();
	wind->DrawTextCentered( 1, MsgGetString(Msg_01563) );

	for(int i=0;i<MAXMAG;i++)
	{
		int t = (tab[i] >> 24) & 0xFF;
		if(t && (t != CARINT_PROTCAP_OFFSET))
		{
			switch(t)
			{
				case CARINT_TAPESER_OFFSET:
					CSer[0][i]->SetTxt("T");
					break;
				case CARINT_AIRSER_OFFSET:
					CSer[0][i]->SetTxt("A");
					break;
				case CARINT_GENERICSER_OFFSET:
					CSer[0][i]->SetTxt("G");
					break;
				default:
					CSer[0][i]->SetTxt("-");
					CSer[1][i]->SetTxt("------");
					break;
			}
	
			if(t != 0xff)
			{
				CSer[1][i]->SetTxt(tab[i] & 0xFFFFFF);
			}
		}
		else
		{
			CSer[0][i]->SetTxt("-");
			CSer[1][i]->SetTxt("------");
		}
	}

	GUI_Thaw();

	int c;
	do
	{
		c=Handle();

		if( c == K_DEL )
		{
			CSer[0][CLSer->GetCurRow()]->SetTxt("-");
			CSer[1][CLSer->GetCurRow()]->SetTxt("------");
		}
		else
		{
			CLSer->ManageKey( c );
		}
	} while( c != K_ESC );

	for(int i=0;i<MAXMAG;i++)
	{
		char buf[64];
		snprintf( buf, sizeof(buf), "%s", CSer[1][i]->GetTxt() );
		DelSpcL(buf);
		DelSpcR(buf);

		if( *buf == '-' )
		{
			tab[i] = CARINT_NOSERIAL;
		}
		else
		{
			tab[i] = atoi(buf);
		}

		switch( CSer[0][i]->GetChar() )
		{
			case 'T':
				tab[i] |= (CARINT_TAPESER_OFFSET << 24);
				break;
			case 'A':
				tab[i] |= (CARINT_AIRSER_OFFSET << 24);
				break;
			case 'G':
				tab[i] |= (CARINT_GENERICSER_OFFSET << 24);
				break;
			default:
				tab[i]=CARINT_NOSERIAL;
				break;
		}

		if(CSer[0][i]!=NULL)
		{
			delete CSer[0][i];
		}

		if(CSer[1][i]!=NULL)
		{
			delete CSer[1][i];
		}
	}

	delete CLSer;
	delete wind;
}

// Riconoscimento automatico dei magazzini caricati (smart feeders)
void CarIntAutoMag(int mode,int mask,struct CarIntAuto_res &results)
{
	int count=0;

	for(int nloop=0;nloop<MAXMAG;nloop++)
	{
		results.ser[nloop]=CARINT_NOSERIAL;

		if(!(mask & (1<< nloop)))
		{
			continue;
		}

		count++;
	}

	if( !(QHeader.modal & ENABLE_CARINT) )
	{
		return;
	}

	CWaitBox* waitBox = 0;

	if( mode == CARINT_SEARCH_NORMAL )
	{
		waitBox = new CWaitBox( 0, 15, MsgGetString(Msg_01769), count );
		waitBox->Show();
	}

	for(int i=0;i<2;i++)
	{
		for(int nloop=0;nloop<MAXMAG;nloop++)
		{
			if(!(mask & (1 << nloop)))
			{
				continue;
			}

			// Esegue riconoscimento smart
			if( results.ser[nloop] == CARINT_NOSERIAL )
			{
				CarInt_GetSerial(nloop,results.ser[nloop],results.chksum[nloop],results.type[nloop],results.swver[nloop]);
			}
		
			if(mode == CARINT_SEARCH_NORMAL)
			{
				if( (results.ser[nloop] != CARINT_NOSERIAL) || (i == 1) )
				{
					waitBox->Increment();
				}
			}
		}

	}

	if( mode == CARINT_SEARCH_NORMAL )
	{
		waitBox->Hide();
		delete waitBox;
	}
}

void MagUnmountAll( CWaitBox* waitBox )
{
	if(!(QHeader.modal & ENABLE_CARINT))
	{
		return;
	}

	ConfDBLink_ok=0;

	for(int i=0;i<CarList_nRec;i++)
	{
		if(CarList[i].mounted!=0)
		{
			ConfDBLink[CarList[i].mounted-1]=-1;
			CarInt_SetAsUnused(CarList[i]);
			CarList[i].mounted=0;
			DBLocal.Write(CarList[i],CarList[i].idx,FLUSHON);
		}

		if( waitBox )
		{
			waitBox->Increment();
		}
	}
}


int* ConfImport_valid;
CarDat* CarIntConf;

int ConfImport_CheckConfSerial(int mag)
{
	mag--;
	
	//se nella configurazione e' indicato un numero di serie
	
	if(CarIntConf[mag*8].C_sermag & 0xFFFFFF)
	{
		int t=(CarIntConf[mag*8].C_sermag >> 24) & 0xFF;

		switch(t)
		{
			case CARINT_TAPESER_OFFSET:
			case CARINT_AIRSER_OFFSET:
			case CARINT_GENERICSER_OFFSET:
			{
				int found=0;
      				//si cerca il numero di serie nel database
				for(int i=0;i<CarList_nRec;i++)
				{
					if(CarList[i].serial==CarIntConf[mag*8].C_sermag)
					{
						found=i+1;
						break;
					}
				}

				if(found && (CarList[found-1].mounted==0))
				{
						//se il numero di serie e' stato trovato nel database
						//e non risulta montato

        				//confronta gli elementi della configurazione e del database
					if(CompareConfDB(mag+1,CarIntConf,CarList[found-1]))
					{
         					 //se uguali ritorna indice corrispondente del database
						return(found-1);
					}
					break;

				}
			}
		}
	}

  //il magazzino della configurazione non ha un numero di serie valido associato
	return(-1);
}

int DoConfImport(int mag,int *mags_found,int nmags_found)
{
	int nvalid=0;
	int firstvalid=-1;
	
	char buf[256];
	
	for(int k=0;k<nmags_found;k++)
	{
		if(CarList[mags_found[k]].mounted)
		{
			continue;
		}
	
		int t=(CarList[mags_found[k]].serial >> 24) & 0xFF;
	
		switch(t)
		{
			case CARINT_TAPESER_OFFSET:
			case CARINT_AIRSER_OFFSET:
			case CARINT_GENERICSER_OFFSET:
				break;
			default:
				continue;
		}

		if((CarList[mags_found[k]].serial & 0xFFFFFF)==NEW_RECORD)
		{
			continue;
		}
	
		nvalid++;
	
		if(firstvalid==-1)
		{
			firstvalid=k;
		}
	}

	if( nvalid )
	{
		if( nvalid == 1 )
		{
			CarList[mags_found[firstvalid]].mounted=0;
		
			//CarList[mags_found[firstvalid]].type=CARINT_NORMAL;
		
			if(CarInt_GetUsedState(CarList[mags_found[firstvalid]])!=MAG_USEDBY_OTHERS)
			{
				CarInt_SetAsUsed(CarList[mags_found[firstvalid]]);
			}
			else
			{
				ConfDBLink[mag-1] = -1;
				DBLocal.Write(CarList[mags_found[firstvalid]],CarList[mags_found[firstvalid]].idx,FLUSHON);
				return 1;
			}


			CarList[mags_found[firstvalid]].mounted=mag;

			DBLocal.Write(CarList[mags_found[firstvalid]],CarList[mags_found[firstvalid]].idx,FLUSHON);
		
			ConfDBLink[mag-1] = CarList[mags_found[firstvalid]].idx;
		
			CarIntConf[(mag-1)*8].C_sermag=CarList[mags_found[firstvalid]].serial;

			for(int j=0;j<8;j++)
			{
				//aggiorna le quantita nella configurazione
				CarIntConf[(mag-1)*8+j].C_quant=CarList[mags_found[firstvalid]].num[j];

				//SMOD040706 Aggiorna anche le note
				strncpyQ(CarIntConf[(mag-1)*8+j].C_note,CarList[mags_found[firstvalid]].noteCar[j],20);

				CarFile->SaveRecX((mag-1)*8+j,CarIntConf[(mag-1)*8+j]);
			}
			CarFile->SaveFile();

			return 1;
		}
		else
		{
			//sono presenti piu magazzini che possono essere associati
			//al magazzino in esame

			int idx=ConfImport_CheckConfSerial(mag);

			if( idx != -1 )
			{
				//il numero di serie nella configurazione e' valido: si esegue
				//l'associazione con quel numero di serie

				CarList[idx].mounted=0;

				//CarList[idx].type=CARINT_NORMAL;

				if(CarInt_GetUsedState(CarList[idx])!=MAG_USEDBY_OTHERS)
				{
					CarInt_SetAsUsed(CarList[idx]);
				}
				else
				{
					ConfDBLink[mag-1] = -1;
					DBLocal.Write(CarList[idx],CarList[idx].idx,FLUSHON);
					return 1;
				}

				CarList[idx].mounted=mag;

				DBLocal.Write(CarList[idx],CarList[idx].idx,FLUSHON);

				for(int j=0;j<8;j++)
				{
					//aggiorna le quantita nella configurazione
					CarIntConf[(mag-1)*8+j].C_quant=CarList[idx].num[j];
			
					//SMOD040706 Aggiorna anche le note
					strncpyQ(CarIntConf[(mag-1)*8+j].C_note,CarList[idx].noteCar[j],20);

					CarFile->SaveRecX((mag-1)*8+j,CarIntConf[(mag-1)*8+j]);
				}
				CarFile->SaveFile();

				ConfDBLink[mag-1] = CarList[idx].idx;
		
				return 1;

			}
			else
			{
				//il numero di serie nella configurazione non e' valido: viene chiesto di associare manualmente
				snprintf( buf, sizeof(buf), MsgGetString(Msg_01809), mag );
				W_Mess(buf);

				ConfImport_valid=new int[nvalid];
		
				int count = 0;

				FeederImportUI importWin( 0, mag );

				for(int k=0;k<nmags_found;k++)
				{
					if(CarList[mags_found[k]].mounted)
					{
						continue;
					}
			
					int t=(CarList[mags_found[k]].serial >> 24) & 0xFF;
			
					switch(t)
					{
						case CARINT_TAPESER_OFFSET:
						case CARINT_AIRSER_OFFSET:
						case CARINT_GENERICSER_OFFSET:
							break;
						default:
							continue;
					}

					if((CarList[mags_found[k]].serial & 0xFFFFFF)==NEW_RECORD)
					{
						continue;
					}

					if(t==CARINT_AIRSER_OFFSET)
					{
						snprintf( buf, sizeof(buf), "A%06d",CarList[mags_found[k]].serial & 0xFFFFFF);
					}
					else
					{
						snprintf( buf, sizeof(buf), "T%06d",CarList[mags_found[k]].serial & 0xFFFFFF);
					}

					importWin.AddItem( buf );

					ConfImport_valid[count++] = mags_found[k];
				}

				int sel = -1;

				importWin.Show( true, false );

				while( 1 )
				{
					importWin.SetFocus();

					if( importWin.GetExitCode() == WIN_EXITCODE_ESC )
					{
						sel = -1;
					}
					else
					{
						sel = importWin.GetSelectedItemIndex();
					}

					if(sel!=-1)
					{
						CarList[ConfImport_valid[sel]].mounted = 0;

						if(CarInt_GetUsedState(CarList[ConfImport_valid[sel]])!=MAG_USEDBY_OTHERS)
						{
							CarInt_SetAsUsed(CarList[ConfImport_valid[sel]]);

							CarList[ConfImport_valid[sel]].mounted=mag;

							DBLocal.Write(CarList[ConfImport_valid[sel]],CarList[ConfImport_valid[sel]].idx,FLUSHON);

							ConfDBLink[mag-1] = CarList[ConfImport_valid[sel]].idx;

							CarIntConf[(mag-1)*8].C_sermag=CarList[ConfImport_valid[sel]].serial;

							for(int j=0;j<8;j++)
							{
								//aggiorna le quantita nella configurazione
								CarIntConf[(mag-1)*8+j].C_quant=CarList[ConfImport_valid[sel]].num[j];

								//SMOD040706 Aggiorna anche le note
								strncpyQ(CarIntConf[(mag-1)*8+j].C_note,CarList[ConfImport_valid[sel]].noteCar[j],20);

								CarFile->SaveRecX((mag-1)*8+j,CarIntConf[(mag-1)*8+j]);
							}
							CarFile->SaveFile();

							break;
						}
						else
						{
							//repeat
						}
					}
					else
					{
						ConfDBLink[mag-1] = -1;
						DBLocal.Write(CarList[ConfImport_valid[sel]],CarList[ConfImport_valid[sel]].idx,FLUSHON);
						//abort
						break;
					}

				}

				importWin.Hide();

				delete [] ConfImport_valid;

				return(sel!=-1);
			}
		}
	}
	else
	{
		//nessuna corrispondenza valida trovata
		//chiede se aggiungere elemento nel database
		char buf[256];
		snprintf( buf, 256, MsgGetString(Msg_01806), mag );
		if(!W_Deci(1,buf))
		{
			return 0;
		}

		//Init input box
		AskFeederSerialUI inputBox( 0, MsgGetString(Msg_01281), 'T', 0 );
		inputBox.Show( true, false );

		int new_ser;
		int exists = 0;

		while(1)
		{
			inputBox.SetFocus();

			if( inputBox.GetExitCode() == WIN_EXITCODE_ESC )
			{
				inputBox.Hide();
				return 0;
			}

			char type = inputBox.GetType();
			new_ser = inputBox.GetSerial() & 0xFFFFFF;

			switch( type )
			{
				case 'A':
					new_ser |= (CARINT_AIRSER_OFFSET << 24);
					break;
				case 'G':
					new_ser |= (CARINT_GENERICSER_OFFSET << 24);
					break;
				default:
				case 'T':
					new_ser |= (CARINT_TAPESER_OFFSET << 24);
					break;
			}

			//search "new" serial in database
			for( int k = 0; k < CarList_nRec; k++ )
			{
				if( CarList[k].serial == new_ser )
				{
					if(CarList[k].mounted)
					{
						new_ser = 0;
						break;
					}

					snprintf( buf, 256, MsgGetString(Msg_01814), type, new_ser & 0xFFFFFF );
					if( !W_Deci(0,buf) )
					{
						return 0;
					}

					if( CarInt_GetUsedState(CarList[k]) == MAG_USEDBY_OTHERS )
					{
						snprintf( buf, 256, MsgGetString(Msg_02009), type, new_ser & 0xFFFFFF );
						W_Mess(buf);
						new_ser = 0;
						break;
					}

					exists = k+1;
					break;
				}
			}

			if( new_ser == 0 )
			{
				continue;
			}

			break;
		}

		inputBox.Hide();


		struct CarInt_data new_rec;
		memset(&new_rec,(char)0,sizeof(new_rec));
		new_rec.serial = new_ser;

		if(!exists)
		{
			if( Free_nRec )
			{
				new_rec.idx=FreeList[FirstFree_idx++];
				Free_nRec--;

				//L'ultimo record segnato come libero in realta non e' su disco
				//si tratta di un record temporaneo, in memoria pronto per essere
				//riempito e salvato su disco

				if(Free_nRec==0)
				{
					//il record viene aggiunto fisicamente al db remoto
					CarList_nRec++;

					//a questo punto la struttura in memoria non contiene
					//altri elementi liberi: ulteriori record vengono aggiunti
					//alla fine del file
				}
			}
			else
			{
				//il record viene aggiunto in fondo al database remoto
				new_rec.idx=CarList_nRec++;
			}
		}
		else
		{
			new_rec.idx=CarList[exists-1].idx;
			strncpyQ(new_rec.note,CarList[exists-1].note,42);
		}

		for(int k=0;k<8;k++)
		{
			strncpyQ(new_rec.elem[k],CarIntConf[(mag-1)*8+k].C_comp,25);
			strncpyQ(new_rec.pack[k],CarIntConf[(mag-1)*8+k].C_Package,20);
			strncpyQ(new_rec.noteCar[k],CarIntConf[(mag-1)*8+k].C_note,24);

			new_rec.packIdx[k]=CarIntConf[(mag-1)*8+k].C_PackIndex;
			new_rec.num[k]=CarIntConf[(mag-1)*8+k].C_quant;

			new_rec.tavanz[k]=CarIntConf[(mag-1)*8+k].C_att;
			new_rec.tipoCar[k]=CarIntConf[(mag-1)*8+k].C_tipo;
		}

		new_rec.smart = 0;
		new_rec.changed=CARINT_NONET_CHANGED;

		if(IsNetEnabled())
		{
			if(DBRemote.Write(new_rec,new_rec.idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
			{
				new_rec.changed=0;
			}
		}

		new_rec.mounted=mag;

		DBLocal.Write(new_rec,new_rec.idx,1);

		InitCarList();

		CarIntConf[(mag-1)*8].C_sermag=new_rec.serial;
		CarFile->SaveRecX((mag-1)*8,CarIntConf[(mag-1)*8]);
		CarFile->SaveFile();

		ConfDBLink[mag-1] = new_rec.idx;

		CarInt_SetAsUsed(new_rec.idx);

		return 1;
	}

	return 0;
}


int ConfImport( int mode )
{
   int prevopen_Lib=1;
   int previnit_List=1;

   if(!(QHeader.modal & ENABLE_CARINT))
   {
     return 1;
   }

   int dummy,err=0;

   if(CarList==NULL)
   {
     DBLocal.InitList(&CarList,NULL,CarList_nRec,dummy);

     previnit_List=0;
   }

   int *mags_found=new int[CarList_nRec];

   ConfDBLink_ok=0;

   for(int i=0;i<CarList_nRec;i++)
   {
     if(CarList[i].mounted!=0)
     {
       ConfDBLink[CarList[i].mounted-1]=-1;
       CarInt_SetAsUnused(CarList[i]);
       CarList[i].mounted=0;
       DBLocal.Write(CarList[i],CarList[i].idx,FLUSHON);
     }
   }

   if(CarFile==NULL)
   {
     CarFile=new FeederFile(QHeader.Conf_Default);
     prevopen_Lib=0;

     if(CarFile->opened==0)
     {
       if(mags_found!=NULL)
       {
         delete[] mags_found;
       }

       delete CarFile;
       CarFile = NULL;

       return 0;
     }
   }

   CarIntConf=new CarDat[MAXNREC_FEED];

   for(int i=0;i<MAXNREC_FEED;i++)
   {
     CarFile->ReadRec(i,CarIntConf[i]);

     DelSpcR(CarIntConf[i].C_comp);
     DelSpcR(CarIntConf[i].C_Package);
   }

   for(int i=0;i<MAXMAG;i++)
   {
     if(mags_found!=NULL)
     {
       delete[] mags_found;
     }

     mags_found=new int[CarList_nRec];

     int nmags_found=0;

     int isBlank=1;

     for(int n=0;n<8;n++)
     {
       if(CarIntConf[i*8+n].C_comp[0]!='\0')
       {
         isBlank=0;
         break;
       }
     }

     if(isBlank)
     {
       continue;
     }

     for(int j=0;j<CarList_nRec;j++)
     {
        if(CompareConfDB(i+1,CarIntConf,CarList[j]))
        {
          //trovata una corrispondenza tra database e configurazione caricatori
          mags_found[nmags_found++]=j;
        }
     }

     int ret=DoConfImport(i+1,mags_found,nmags_found);

     if(ret==0)
     {
       err=1;
       break;
     }
   }

   if(err)
   {
     if(mode)
     {
       W_Mess(CONFIMPORT_ABORT);
     }

     ConfDBLink_ok=0;

     for(int i=0;i<CarList_nRec;i++)
     {
       if(CarList[i].mounted!=0)
       {
         ConfDBLink[CarList[i].mounted-1]=-1;
         CarInt_SetAsUnused(CarList[i]);
         CarList[i].mounted=0;
         DBLocal.Write(CarList[i],CarList[i].idx,FLUSHON);
       }
     }

     W_Mess(CARINT_RESET_WARN);
   }
   else
   {
     if(mode)
     {
       W_Mess(CONFIMPORT_OK);
     }

     ConfDBLink_ok=1;
   }

   delete[] mags_found;

   delete[] CarIntConf;
   
   if(!prevopen_Lib)
   {
     delete CarFile;
     CarFile=NULL;
   }

   if(!previnit_List)
   {
     delete CarList;
     CarList=NULL;
   }

   return(!err);
}

int CarInt_FindSerCostr()
{
	CarIntFindSerCostr();
	return 1;
}

int CarInt_FindCar()
{
	CarIntFindCar();
	return 1;
}

int CarInt_FindMag()
{
	CarIntFindMag();
	return 1;
}

int CarInt_FindNote()
{
	CarIntFindNote();
	return 1;
}

int CarInt_ConfImport()
{
	//TODO: deselect parent
	//Q_CarInt->Deselect();
	ConfImport();
	//Q_CarInt->Select();

	feedersDatabaseRefresh = true;
	return 1;
}

int CarInt_AdvSearch()
{
	CInputBox inbox( 0, 6, MsgGetString(Msg_00398), MsgGetString(Msg_00103), 40, CELL_TYPE_TEXT );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	char searchtxt[41];
	snprintf( searchtxt, sizeof(searchtxt), "%s", inbox.GetText() );

	if( UpdateDBData(CARINT_UPDATE_FULL) )
	{
		FeederAdvancedSearchUI win( 0 );

		if( win.Search( searchtxt ) )
		{
			win.Show();
			win.Hide();
		}
	}

	InitCarList();

	CarList_CurRecord=0;
	CarList_CurRow=0;
	CarList_StartTabe=0;

	feedersDatabaseRefresh = true;
	return 1;
}


int CarInt_UnMount(void)
{
	if((CarList[CarList_CurRecord].serial & 0xFFFFFF)==NEW_RECORD)
	{
		return 0;
	}
	
	int  forced=0;
	
	char type;
	int  ser;
	char buf[160];

	switch((CarList[CarList_CurRecord].serial >> 24) & 0xFF)
	{
		case CARINT_AIRSER_OFFSET:
			type='A';
			break;
		case CARINT_TAPESER_OFFSET:
			type='T';      
			break;
		case CARINT_GENERICSER_OFFSET:
			type='G';      		
			break;
		default:
			type=' ';
			break;
	}

	ser = CarList[CarList_CurRecord].serial & 0xFFFFFF;

	if(CarInt_GetUsedState(CarList[CarList_CurRecord])==MAG_USEDBY_OTHERS)
	{
		char name[51];
	
		if(!GetMachineName(CarList[CarList_CurRecord].usedBy,name))
		{
			name[0]='\0';
		}
		else
		{
			name[30]='\0';
		}

		char buf[160];
		snprintf( buf, sizeof(buf), ERRMAG_USEDBY_OTHERS_CONFIRM1, name );
			if(!W_Deci(0,buf))
		{
			return 0;
		}

		if(!W_Deci(0,ERRMAG_USEDBY_OTHERS_CONFIRM2))
		{
			return 0;
		}
	
		forced=1;
	}
	else
	{
		if(CarList[CarList_CurRecord].mounted!=0)
		{
			snprintf( buf, sizeof(buf), ASK_CONFIRM_UNMOUNT,type,ser,CarList[CarList_CurRecord].mounted);
		
			if(!W_Deci(0,buf))
			{
				return 0;
			}
		}
		
	}

	int prevopen_Conf = 1;
	
	if(CarFile==NULL)
	{
		CarFile=new FeederFile(QHeader.Conf_Default);
		prevopen_Conf=0;
	}
		
	
	if(CarList[CarList_CurRecord].mounted != 0)
	{
		CarDat car;
		SFeederDefault cardef[MAXCAR];
		FeedersDefault_Read( cardef );

		for(int i=1;i<=8;i++)
		{
			int mag = CarList[CarList_CurRecord].mounted;
			
			memset(&car,(char)0,sizeof(car));
			
			car.C_codice = (mag*10) + i;
			car.C_xcar = cardef[((mag-1)*8)+i-1].x;
			car.C_ycar = cardef[((mag-1)*8)+i-1].y;
		
			CarFile->SaveX((mag*10) + i,car);
		}
		CarFile->SaveFile();
	}


	if(!prevopen_Conf)
	{
		delete CarFile;
		CarFile=NULL;
	}
	
	ConfDBLink[CarList[CarList_CurRecord].mounted-1]=-1;
	
	CarInt_SetAsUnused(CarList[CarList_CurRecord],forced);
	
	CarList[CarList_CurRecord].smart = 0;

	if(IsNetEnabled())
	{
		if(DBRemote.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
		{
			CarList[CarList_CurRecord].changed=0;
		}
	}
	
	CarList[CarList_CurRecord].mounted=0;
	
	DBLocal.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,FLUSHON);
	
	feedersDatabaseRefresh = true;
	return 1;
}



//n=numero del record nel file
//loc_update (opzionale) puntatore a struttura dati magazzino che conterra'
//il record locale aggiornato (se necessario)

int CarInt_SetAsUsed(int n,CarInt_data *loc_update)
{
  if(!IsNetEnabled())
  {
    //funzionalita di rete disattivate: nessuna operazione
    //necessaria
    return 1;
  }

  struct CarInt_data  rdata;
  struct CarInt_data  ldata;

  if(!DBRemote.Read(rdata,n,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
  {
    //errore di lettura dalla rete
    return 0;
  }

  if(loc_update==NULL)
  {
    int f=DBToMemList_Idx(n);

    if(f!=-1)
    {
      loc_update=&CarList[f];
    }
    else
    {
      return 0;
    }

    //DBLocal.Read(ldata,n);
  }

  ldata=*loc_update;

  //se il magazzini risulta essere gia' utilizzato da questa macchina
  if(rdata.usedBy==nwpar.NetID_Idx)
  {
    //aggiorna locale se non allineato al remoto per il campo usedBy
    //non dovrebbe accadere visto che i db dovrebbero essere allineati

    //!!! Non dovrebbe mai accadere !!!!
    
    if(ldata.usedBy!=rdata.usedBy)
    {
      ldata.usedBy=nwpar.NetID_Idx;
      DBLocal.Write(ldata,n,FLUSHON);
    }

    //nothing else to do
    return 1;
  }

  if(rdata.usedBy==0)
  {
    //ok: magazzino libero puo' essere assegnato alla macchina
    rdata.usedBy=nwpar.NetID_Idx;
    if(!DBRemote.Write(rdata,n,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
    {
      //errore di scrittura sulla rete: il magazzino non viene
      //segnato come usato neanche sul locale
      return 0;
    }
    else
    {
      //ok: aggiorna db locale (solo campo usedBy)
      ldata.usedBy=rdata.usedBy;
      DBLocal.Write(ldata,n,FLUSHON);

      if(loc_update!=NULL)
      {
        *loc_update=ldata;
      }
      return 1;
    }
  }
  else
  {
    //il magazzino risulta essere gia' montato da un'altra macchina
    //se db locale e remoto differiscono per il campo usedby aggiorna
    //ma solo questo campo !
    if(ldata.usedBy!=rdata.usedBy)
    {
      ldata.usedBy=rdata.usedBy;
      DBLocal.Write(ldata,n,FLUSHON);

      if(loc_update!=NULL)
      {
        *loc_update=ldata;
      }
    }
    return 1;
  }
}

int CarInt_SetAsUsed(CarInt_data &mag)
{
  return CarInt_SetAsUsed(mag.idx,&mag);
}


int CarInt_SetAsUnused(CarInt_data &mag,int forced)
{
  if(!IsNetEnabled())
  {
    return 1;
  }

  int idx=mag.idx;

  struct CarInt_data rdata;  

  if(!DBRemote.Read(rdata,idx,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
  {
    //errore di lettura dalla rete
    return 0;
  }

  if(rdata.usedBy==0)
  {
    //il magazzino e' gia' libero

    if(mag.usedBy!=rdata.usedBy)
    {
      //aggiorna stato usedby locale se non corrisponde a remoto
      mag.usedBy=rdata.usedBy;
      DBLocal.Write(mag,idx,FLUSHON);
    }
    
    return 1;
  }

  if((rdata.usedBy==nwpar.NetID_Idx) || forced)
  {
    //il magazzino e' in uso alla macchina oppure e' stato richiesto
    //di smontarlo forzatamente: ok smonta !

    rdata.usedBy=0;

    if(!DBRemote.Write(rdata,idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
    {
      return 0;
    }

    mag.usedBy=0;    
    
    DBLocal.Write(mag,idx,FLUSHON);

    return 1;
  }
  else
  {
    //non smontare ma aggiorna stato usedby locale se non corrisponde a remoto
    if(mag.usedBy!=rdata.usedBy)
    {
      mag.usedBy=rdata.usedBy;
      DBLocal.Write(mag,idx,FLUSHON);
    }
    
    return 1;
  }
    
}

int CarInt_SetAllAsUnused(void)
{
  for(int i=0;i<CarList_nRec;i++)
  {
    if(CarInt_GetUsedState(CarList[i])==MAG_USEDBY_ME)
    {
      CarInt_SetAsUnused(CarList[i]);
    }
  }

  return 1;
}

int CarInt_GetUsedState(CarInt_data &mag,int remote_idx/*=-99*/)
{
  if(!IsNetEnabled())
  {
    //figura come se fosse usato da questa macchina in modo da permettere
    //all'utente di eseguire modifiche che verranno comunque confinate
    //al db locale
    return(MAG_USEDBY_ME);
  }

  int idxl=mag.idx;
  int idxr;
  
  if(remote_idx==-99)
  {
    idxr=mag.idx;
  }
  else
  {
    idxr=remote_idx;
  }

  struct CarInt_data rdata;  

  if(!DBRemote.Read(rdata,idxr))
  {
    //errore di lettura dalla rete
    return(-1);
  }

  if(mag.usedBy!=rdata.usedBy)
  {
    //aggiorna stato usedby locale de non corrisponde a remoto
    mag.usedBy=rdata.usedBy;
    DBLocal.Write(mag,idxl,FLUSHON);
  }

  if(rdata.usedBy==0)
  {
    return(MAG_USEDBY_NONE);
  }
  else
  {
    if(rdata.usedBy==nwpar.NetID_Idx)
    {
      return(MAG_USEDBY_ME);
    }
    else
    {
      return(MAG_USEDBY_OTHERS);
    }
  }

}


int CarInt_AutoMag()
{
	struct CarIntAuto_res results;
	
	CarIntAutoMag(CARINT_SEARCH_NORMAL,CARINT_AUTOALL_MASK,results);

	CarIntAuto_ShowResults( results.ser );
	
	if(!CarIntAuto_ElabResults(results,CARINT_AUTOALL_MASK))
	{
		ConfDBLink_ok=0;
		
		for(int i=0;i<CarList_nRec;i++)
		{
			if(CarList[i].mounted!=0)
			{
				ConfDBLink[CarList[i].mounted-1]=-1;
				CarInt_SetAsUnused(CarList[i]);
				CarList[i].mounted=0;
				DBLocal.Write(CarList[i],CarList[i].idx,FLUSHON);
			}
		}

		W_Mess(CARINT_RESET_WARN);
	}
	
	feedersDatabaseRefresh = true;
	return 1;
}


// Gestore PGDOWN
void CarInt_PgDN()
{
	if((CarList[CarList_CurRecord].serial & 0xFFFFFF)==NEW_RECORD)
	{
		return;
	}

	if((CarList_StartTabe+MAXCARINT)>CarList_nRec)
	{
		//la pagina correntemente visualizzata e' l'ultima
		CarList_CurRecord=CarList_nRec-1;
	}
	else
	{
		CarList_StartTabe+=MAXCARINT-1;

		if((CarList_CurRecord+MAXCARINT)>CarList_nRec)
		{
			CarList_CurRecord=CarList_nRec-1;
		}
		else
		{
			CarList_CurRecord+=MAXCARINT-1;
		}
	}

	CarList_CurRow=CarList_CurRecord-CarList_StartTabe;

	feedersDatabaseRefresh = true;
}

// Gestore PGUP
void CarInt_PgUP()
{
	CarList_StartTabe-=MAXCARINT-1;
	CarList_CurRecord-=MAXCARINT-1;

	if(CarList_StartTabe<0)
	{
		CarList_StartTabe=0;
		CarList_CurRow=0;
		CarList_CurRecord=0;
	}

	feedersDatabaseRefresh = true;
}



void CarInt_CtrlPgDN()
{
	if((CarList[CarList_CurRecord].serial & 0xFFFFFF)==NEW_RECORD)
	{
		return;
	}

	CarList_CurRow=MAXCARINT-1;

	CarList_CurRecord=CarList_nRec-1;

	CarList_StartTabe=CarList_nRec-MAXCARINT;
	if(CarList_StartTabe<0)
	{
		CarList_StartTabe=0;
	}

	feedersDatabaseRefresh = true;
}

void CarInt_CtrlPgUP()
{
	CarList_CurRow=0;
	CarList_CurRecord=0;

	CarList_StartTabe=0;

	feedersDatabaseRefresh = true;
}



int GetConfDBLink(int mag)
{
  return(ConfDBLink[--mag]);
}

//dato l'indice del record nel file db ritorna l'indice corrispondente
//per la struttura dati in memoria CarList
int DBToMemList_Idx(int idx)
{  
	for(int i=0;i<CarList_nRec;i++)
	{
		if(CarList[i].idx==idx)
		{
			return(i);
		}
	}

	return(-1);
}


int IsConfDBLinkOk(void)
{
  return(ConfDBLink_ok);
}


// Chiede se si vuole fare l'update dei dati in tab caricatori
int Car_AskUpdate(int code)
{
   char buf[160];
   snprintf( buf, sizeof(buf), ASK_UPDATECAR, code );

	int retval = W_DeciYNA(NO,buf);
	if( retval==YES || retval==ALL )
	{
		if(W_Deci(0,ASK_UPDATECAR2))
		{
			return(retval);
		}
	}

   return(NO);
}

// Esegue l'update del file di libreria dei caric a seguito
// di modifiche fatte nei magazzini
// ritorna 1 se ok, 0 altrimenti
int UpdateCaricLib()
{
   struct CarDat car;
   int cloop, ploop, index;
   int carupdateAskFlag = 1;
   
   int prevopen_Lib=1;

   if(!(QHeader.modal & ENABLE_CARINT))
   {
     return 1;
   }

   int dummy;

   DBLocal.InitList(&CarList,NULL,CarList_nRec,dummy);

   if(CarFile==NULL)
   {
     CarFile=new FeederFile(QHeader.Conf_Default);
     prevopen_Lib=0;
   }

   for(ploop=1;ploop<=MAXMAG;ploop++)
   {
      // Verifico se e' montato un magazzino con codice ploop
      for(index=0; index<CarList_nRec; index++)
      {
         if( CarList[index].mounted == ploop )
            break;
      }

      int blank=1;

      //si controlla se il magazzino in posizione ploop nella configurzione
      //e' completamente vuoto
      for(cloop=1;cloop<=8;cloop++)
      {
        CarFile->Read((ploop*10)+cloop,car);
        if(car.C_comp[0]!='\0')
        {
          blank=0;
          break;
        }
      }

      if(index<CarList_nRec) // C'e'... Aggiorno i dati dei caricatori associati
      {
         //se il magazzino in posizione ploop e' completamente vuoto copia
         //direttamente da database a configurazione
         if(blank)
         {
           for(cloop=1;cloop<=8;cloop++)
           {
             CarFile->Read((ploop*10)+cloop,car);

             car.C_att=CarList[index].tavanz[cloop-1];
             car.C_tipo=CarList[index].tipoCar[cloop-1];
             car.C_Ncomp=CarList[index].num[cloop-1];
             car.C_PackIndex=CarList[index].packIdx[cloop-1];
             strcpy(car.C_Package,CarList[index].pack[cloop-1]);
             strcpy(car.C_note,CarList[index].noteCar[cloop-1]);
             strcpy(car.C_comp,CarList[index].elem[cloop-1]);

             CarFile->SaveX((ploop*10)+cloop,car);
           }
           CarFile->SaveFile();
         }
         else
         {
           //Controllo uguaglianza tra database e configurazione

           // Se i dati nel db dei magazzini sono diversi da quelli della
           // lib caricatori, si chiede conferma per sostituirli

           int modified_fields;

           if(!CompareConfDB(ploop,CarFile,CarList[index],1,&modified_fields))
           {
             //la domanda viene eseguita una sola volta: accettando per
             //un singolo elemento si accetta per tutti

             //SMOD030706 Se solo il campo quantita' e/o note risulta modificato
             //aggiorna senza fare domande.

             int no_ask=0;

             if(((modified_fields==CARINT_FIELD_QUANT) || (modified_fields==CARINT_FIELD_NOTE)) || (modified_fields==(CARINT_FIELD_QUANT | CARINT_FIELD_NOTE)))
             {
               no_ask=1;
             }

             if(carupdateAskFlag && !no_ask)
             {
               int retask=Car_AskUpdate(ploop);

               if(retask==NO)
               {
                 //rispondendo no l'operazione viene interrotta

                 if(!prevopen_Lib)
                 {
                   delete CarFile;
                   CarFile=NULL;
                 }
                      
                 return 0;
               }
               else
               {
                 if(retask==ALL)
                 {
                   carupdateAskFlag=0;
                 }
               }
             }

             //Aggiornamento configurazione caricatori
             for(int cloop=1;cloop<=8;cloop++)
             {
               CarFile->Read((ploop*10)+cloop,car);

               car.C_att=CarList[index].tavanz[cloop-1];
               car.C_tipo=CarList[index].tipoCar[cloop-1];
               car.C_PackIndex=CarList[index].packIdx[cloop-1];
               car.C_quant=CarList[index].num[cloop-1];
               strcpy(car.C_Package,CarList[index].pack[cloop-1]);
               strcpy(car.C_note,CarList[index].noteCar[cloop-1]);
               strcpy(car.C_comp,CarList[index].elem[cloop-1]);

               CarFile->SaveX((ploop*10)+cloop,car);
             }
             CarFile->SaveFile();
           }
         }
      }
	}

   if(!prevopen_Lib)
   {
     delete CarFile;
     CarFile=NULL;
   }

   return 1;
}


//associa il magazzino attualmente selzionato nel database alla configurazione
//caricatori
//nmag : magazzino da associare in tabella caricatori
void CarIntSetMounted(int nmag)
{
	int i;
	
	int prevopen_CarFile=1;
	
	if(CarFile==NULL)
	{
		CarFile=new FeederFile(QHeader.Conf_Default);
		prevopen_CarFile=0;
	}
	
	struct CarDat car;
	
	int blank=1;
	
	for(i=1;i<=8;i++)
	{
		CarFile->Read((nmag*10)+i,car);
	
		DelSpcR(car.C_comp);
		DelSpcR(car.C_Package);
	
		if(car.C_comp[0]!='\0')
		{
			blank=0;
		}
	}
	
	if(!blank)
	{
		char buf[80];
		snprintf( buf, sizeof(buf), ASK_UPDATECAR3,nmag);
	
		if(!W_Deci(0,buf))
		{
			if(!prevopen_CarFile)
			{
				delete CarFile;
				CarFile=NULL;
			}
		
			return;
		}
	}
	
	memset(&car,(char)0,sizeof(car));
	
	for(i=1;i<=8;i++)
	{
		SFeederDefault cardef[MAXCAR];
		FeedersDefault_Read( cardef );
	
		car.C_codice=(nmag*10)+i;
	
		car.C_att=CarList[CarList_CurRecord].tavanz[i-1];
		car.C_tipo=CarList[CarList_CurRecord].tipoCar[i-1];
		car.C_PackIndex=CarList[CarList_CurRecord].packIdx[i-1];
		car.C_quant=CarList[CarList_CurRecord].num[i-1];
	
		SetFeederNCompDef(car);
	
		strcpy(car.C_Package,CarList[CarList_CurRecord].pack[i-1]);
		strcpy(car.C_note,CarList[CarList_CurRecord].noteCar[i-1]);
		strcpy(car.C_comp,CarList[CarList_CurRecord].elem[i-1]);
	
		car.C_xcar = cardef[((nmag-1)*8)+i-1].x;
		car.C_ycar = cardef[((nmag-1)*8)+i-1].y;
		
		car.C_nx     =1;
		car.C_ny     =1;
		car.C_incx   =0;
		car.C_incy   =0;
		car.C_offprel=0;
		car.C_avan   =0;
	
		CarFile->SaveX((nmag*10)+i,car);
	}
	CarFile->SaveFile();
	
	if(!prevopen_CarFile)
	{
		delete CarFile;
		CarFile=NULL;
	}
	
	for(i=0;i<CarList_nRec;i++)
	{
		if(CarList[i].mounted==nmag)
		{
			break;
		}
	}
	
	if(i!=CarList_nRec)
	{
		//posizione occupata !
		CarList[i].mounted=0;
	
		CarInt_SetAsUnused(CarList[i]);
		
		DBLocal.Write(CarList[i],CarList[i].idx,1);
	}
	
	CarList[CarList_CurRecord].smart = 0;
	CarList[CarList_CurRecord].changed = CARINT_NONET_CHANGED;
	
	if(IsNetEnabled())
	{
		if(DBRemote.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
		{
			CarList[CarList_CurRecord].changed=0;
		}
	}
	
	CarList[CarList_CurRecord].mounted=nmag;
	ConfDBLink[nmag-1]=CarList[CarList_CurRecord].idx;
	
	CarInt_SetAsUsed(CarList[CarList_CurRecord]);
	
	DBLocal.Write(CarList[CarList_CurRecord],CarList[CarList_CurRecord].idx,1);

}


// Attiva tabella magazzini
// mode = 0       modo normale
// mode = n (!=0) associazione config. caricatori-database
//                (n: numero del magazzino da associare)
int CarInt( int mode/*=0*/, int initsel_mag/*=-1*/ )
{
	if(!(QHeader.modal & ENABLE_CARINT))
	{
		return 0;
	}

	CarList_CurRow=0;
	CarList_CurRecord=0;
	CarList_StartTabe=0;

	if(!UpdateDBData(CARINT_UPDATE_FULL))
	{
		return 0;
	}

	if(!InitCarList())
	{
		return 0;
	}

	CarList_CurRow=0;

	if((initsel_mag!=-1) && (initsel_mag<=(MAXMAG*10)+8))
	{
		int c;
		for(c=0; c<CarList_nRec; c++)
		{
			if(CarList[c].mounted==(initsel_mag/10))
			{
				break;
			}
		}

		if(c!=CarList_nRec)
		{
			CarList_CurRecord=c;
			CarList_StartTabe=CarList_CurRecord;
		}
	}


	FeedersDatabaseUI feederDatabaseWin( 0, (mode != 0) ? true : false );
	feederDatabaseWin.Show();
	feederDatabaseWin.Hide();

	if( mode != 0 && feederDatabaseWin.GetExitCode() == WIN_EXITCODE_ENTER )
	{
		CarIntSetMounted(mode);
	}

	// Update del file della lib caricatori
	if(!UpdateCaricLib())
	{
		return 0;
	}

	return 1;
}



//**************************************************************************
// Gestione file posizione riferimenti caricatori intelligenti
//**************************************************************************

int HandleCdfRif=0;    // File handle per riferimenti carcatori.

// Crea un nuovo file riferimenti caricatori.
int CdfRifCreate(void)
{
	char eof = 26;

	if(!F_File_Create(HandleCdfRif,CARRIFNAME))
   {
		return 0;
	}

   WRITE_HEADER(HandleCdfRif,FILES_VERSION,CARINTPOSREF_SUBVERSION);
	write(HandleCdfRif,(char *)TXTRIFFEED,strlen(TXTRIFFEED));
	write(HandleCdfRif,&eof,1);
	FilesFunc_close(HandleCdfRif);
	return 1;
}


// Modifica i dati nel file default caricat.
int Mod_Rif_Car(CfgCaricInt &H_Car, int AlreadyOpen, int Last)
{
	char eof=26;
	char Letto=0;

	if(!AlreadyOpen)
   {
		if(access(CARRIFNAME,0))
      {
			if(!CdfRifCreate()) return 0;
		}
		if(!F_File_Open(HandleCdfRif,CARRIFNAME)) return 0;

		while(Letto!=eof) {                    // skip title PAR
			read(HandleCdfRif,&Letto,1);         // & init. lines
		}
		Letto=NULL;
	}
	if(!Last) {
		write(HandleCdfRif,(char *)&H_Car,sizeof(H_Car));   // scrive dati
	}
	if(Last) FilesFunc_close(HandleCdfRif);
	return 1;
}

// Legge i valori dal file default caricatori.
int Read_Rif_Car(CfgCaricInt &H_Car, int AlreadyOpen, int Last)
{
	char eof=26;
	char Letto=0;
	if(!AlreadyOpen)
	{
		if(access(CARRIFNAME,0))
		{
			return 0;
		}
		if(!F_File_Open(HandleCdfRif,CARRIFNAME)) return 0;
		while(Letto!=eof)
		{                     // skip title QPF
			read(HandleCdfRif,&Letto,1);          // & init. lines
		}
		Letto=NULL;
	}

	if(!Last)
	{
		read(HandleCdfRif,(char *)&H_Car,sizeof(H_Car));
	}
	if(Last) FilesFunc_close(HandleCdfRif);
	return 1;
}

// Legge tutti i valori nel file di default caricatori
int Read_Rif_CarAll(CfgCaricInt *car)
{
	int nloop;
	
	Read_Rif_Car(car[0],0,0);
	for(nloop=1;nloop<MAXCROCI;nloop++)
	{
		Read_Rif_Car(car[nloop],1,0);
	}
	Read_Rif_Car(car[0],1,1);

	return 1;
}



/****************************************************************

GESTIONE RETE QUADRA LASER

****************************************************************/

// Check della presenza dei files dei db e della rete
int CheckDB()
{
	// funzione per vedere se la rete e' attiva o meno
	if( IsNetEnabled() )
	{
		if( !IsSharedDirMounted() )
		{
			ErrNetMsg();
			DisableNet();
			return 0;
		}
	}

	int NoFile = access(CARINTFILE,F_OK);
	if( NoFile )
	{
		W_Mess(NOLOCALCARMSG);
		if(!DBLocal.Create())
		{
			return 0;
		}
	}
	
	if( IsNetEnabled() )
	{
		if( Get_UseQMode() )
		{
			NoFile = access(CARINTREMOTE_QMODE,F_OK);
		}
		else
		{
			NoFile = access(CARINTREMOTE,F_OK);
		}

		if(NoFile)
		{
			W_Mess(NOREMOTECARMSG);

			return DBRemote.Create(CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET);
		}
	}
	
	return 1;
}


int DBModified_AskUpdate(int serial)
{
	char buf[256];
	int c;
	
	switch((serial >> 24) & 0xFF)
	{
		case CARINT_TAPESER_OFFSET:
			snprintf( buf, sizeof(buf), ASK_UPDATEMODLOCAL,'T',serial & 0xFFFFFF);
			break;
		case CARINT_AIRSER_OFFSET:
			snprintf( buf, sizeof(buf), ASK_UPDATEMODLOCAL,'A',serial & 0xFFFFFF);
			break;
		case CARINT_GENERICSER_OFFSET:
			snprintf( buf, sizeof(buf), ASK_UPDATEMODLOCAL,'G',serial & 0xFFFFFF);
			break;
		default:
			snprintf( buf, sizeof(buf), ASK_UPDATEMODLOCAL,'X',serial & 0xFFFFFF);
			break;
	}   
	
	
	W_MsgBox *box=new W_MsgBox(MsgGetString(Msg_00289),buf,2,MSGBOX_GESTKEY);
	
	box->AddButton(FAST_YES,1);
	box->AddButton(FAST_NO,0);
	
	c=box->Activate();
	
	delete box;
	
	return(c);
}


//Aggiorna le quantita nel database magazzini con quelle nella configurazione corrente.
//void UpdateMagaComp(int *used_feeder_vect)
void UpdateMagaComp(void)
{
  //if((!(QHeader.modal & ENABLE_CARINT)) || (used_feeder_vect==NULL))
  if(!(QHeader.modal & ENABLE_CARINT))
  {
    return;
  }

  int prevopen=1;

  if(CarFile==NULL)
  {
    CarFile=new FeederFile(QHeader.Conf_Default);
    prevopen=0;
  }

  struct CarDat car;
  //struct CarInt_data dbrec;
  
  for(int i=0;i<MAXMAG;i++)
  {
    /*
    if(!used_feeder_vect[i])
    {
      continue;
    }
    */

    if(ConfDBLink[i] != -1)
    {
      int f=DBToMemList_Idx(ConfDBLink[i]);

      if(f!=-1)
      {
        //DBLocal.Read(dbrec,ConfDBLink[i]);

        for(int j=0;j<8;j++)
        {
          CarFile->Read(((i+1)*10)+j+1,car);
          CarList[f].num[j]=car.C_quant;
        }

        CarList[f].changed=CARINT_NONET_CHANGED;

        if(IsNetEnabled())
        {
          if(DBRemote.Write(CarList[f],ConfDBLink[i],FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
          {
            CarList[f].changed=0;
          }
        }

        DBLocal.Write(CarList[f],ConfDBLink[i],FLUSHON);
      }
    }
  }

  if(!prevopen)
  {
    delete CarFile;
    CarFile=NULL;
  }
}

// Decrementa il numero di componenti del caricatore passato se il
// suo magazzino risulta montato
void DecMagaComp(int code)
{
   if(!(QHeader.modal & ENABLE_CARINT))
   {
     return;
   }

   //CarInt_data car;
   int maga = code/10;
   int caric = code%10;

   if( code >= 160 )
   {
     return;
   }

   if( ConfDBLink[maga-1] != -1 )
   {
      int f=DBToMemList_Idx(ConfDBLink[maga-1]);

      //if( DBLocal.Read(car,ConfDBLink[maga-1]) )

      if(f!=-1)
      {
        if( CarList[f].num[caric-1] > 0 )
        {
          CarList[f].num[caric-1]--;

          CarList[f].changed=CARINT_NONET_CHANGED;

          if(IsNetEnabled())
          {
            if(DBRemote.Write(CarList[f],ConfDBLink[maga-1],FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
            {
              CarList[f].changed=0;
            }
          }

          DBLocal.Write(CarList[f],ConfDBLink[maga-1],FLUSHON);
        }
        else
        {
          //snprintf( buf, sizeof(buf), NOMAGACOMP,ConfDBLink[maga-1]+1,code);
          //W_Mess(buf);
        }
      }
   }
   
}

// Esegue l'allineamento tra il campo note del db locale e quello remoto
// SMOD151004 : aggiunto anche init di ConfDBLink
// mode bit 0=0     : (default) solo check e update dei magazzini
//            1     : check, update e initi di ConfDBLink

// mode bit 1=0     : (default) init delle liste dati Locale e Remoto
//            1     : non esegue l'init delle lista dati (160107: non utilizzato)
//
// mode bit 2=0     : (default) esegue il controllo solo sui magazzini
//                    attualmente montati
//           =1     : esegue il controllo su tutti i magazzini

int UpdateDBData(int mode/*=0*/)
{
   if(!(QHeader.modal & ENABLE_CARINT))
   {
     return 1;
   }

   if(!(mode & 1))
   {
     //se solo check la rete deve essere abilitata
     if(!IsNetEnabled())
     {
       return 1;
     }
   }

   int ret=1;
   int endloop=0;

   if(mode & 1)
   {
     for(int i=0;i<MAXMAG;i++)
     {
       ConfDBLink[i] = -1;
     }

     ConfDBLink_ok=0;
   }

   CarInt_data *remoteCar=NULL;
   CarInt_data *localCar=NULL;

   int remote_nRec=0;
   int local_nRec=0;

   int remote_nFree=0;
   int *remote_FreeList=NULL;
   int remote_ptrFree=0;

   int local_nFree=0;
   int *local_FreeList=NULL;

   //------------------------------------------------------------------------
   // Controlla esistenza e accesibilita dei database
   //------------------------------------------------------------------------   

   if(!CheckDB())   
   {
     return 0;
   }   

   //------------------------------------------------------------------------
   // Legge database remoto
   //------------------------------------------------------------------------   

   int remote_nFileRec;

   if(IsNetEnabled())
   {
     if(!DBRemote.LockOpen(CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
     {
       return(-1);
     }

     DBRemote.UpdateTypeFlag(CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET);

     remote_nFileRec=DBRemote.Count();

     DBRemote.UnLockClose();

     /*
     SMOD170107 Spostato al termine dell'allineamento tra database remoto
     e locale
     if(mode & 1)
     {
       ConfImport(0);
     }
     */

     if(!DBRemote.InitList(&remoteCar,&remote_FreeList,remote_nRec,remote_nFree))
     {
       return(-1);
     }

     if(remote_nRec==-1)
     {
       if(remoteCar!=NULL)
       {
         delete[] remoteCar;
       }

       if(remote_FreeList!=NULL)
       {
         delete[] remote_FreeList;
       }

       char buf[80];
       snprintf( buf, sizeof(buf), CARINTFILE_ERR,TXTREMOTE,"");
       W_Mess( buf );
       return 0;
     }

   }   

   //------------------------------------------------------------------------
   // Legge database locale
   //------------------------------------------------------------------------   

   //160107 Attualmente il bit1 di mode non viene mai posto ad 1
   if(!(mode & 2))
   {
     DBLocal.UpdateTypeFlag();

     DBLocal.InitList(&localCar,&local_FreeList,local_nRec,local_nFree);

     if(local_nRec==-1)
     {
       if(localCar!=NULL)
       {
         delete[] localCar;
       }

       if(local_FreeList!=NULL)
       {
         delete[] local_FreeList;
       }

       if(remoteCar!=NULL)
       {
         delete[] remoteCar;
       }

       if(remote_FreeList!=NULL)
       {
         delete[] remote_FreeList;
       }

       char buf[80];
       snprintf( buf, sizeof(buf),CARINTFILE_ERR,TXTLOCAL,"");
       W_Mess( buf );

       return 0;
     }
   }
   else
   {
     localCar=CarList;
     local_nRec=CarList_nRec;
     local_FreeList=FreeList;
     local_nFree=Free_nRec;
   }


	CWaitBox waitBox( 0, 15, MsgGetString(Msg_01312), local_nRec );
	waitBox.Show();


   short int *found=NULL;

   if(IsNetEnabled())
   {
     found=new short int[local_nRec];
   }

   for(int i=0;i<local_nRec;i++)
   {
     //se bit mode2=0 esegue il controllo solo sui magazzini segnati
     //come montati
     
     if(!(mode & 4))
     {
       if(localCar[i].mounted==0)
       {
         continue;
       }
     }

     //se rete attivata cerca elemento anche in database remoto
     if(IsNetEnabled())
     {
       found[i]=-1;

       for(int j=0;j<remote_nRec;j++)
       {
         //elemento trovato
         if(remoteCar[j].serial==localCar[i].serial)
         {

           found[i]=j;
           break;
         }
       }
     }
   }

   #ifdef __DBNET_DEBUG
   DBLocal.WriteLogMsg("Searching new items in local that have to be inserted in remote");
   #endif
   
   for(int i=0;i<local_nRec;i++)
   {
     //se found non allocato (rete non abilitata): interrompi
     if(!IsNetEnabled())
     {
       break;
     }

     //se bit mode2=0 esegue il controllo solo sui magazzini segnati
     //come montati
     
     if(!(mode & 4))
     {
       if(localCar[i].mounted==0)
       {
			waitBox.Increment();
			continue;
       }
     }

     //se elemento nel database locale modificato a rete
     //disattivata e presente anche in remoto passa a prossimo
     //(verra' gestito successivamente)
     if(localCar[i].changed && (found[i]!=-1))
     {
    	 waitBox.Increment();
       continue;
     }

     //se elemento i-esimo nel db locale non trovato nel remoto
     //e in locale risulta cambiato: e' un record aggiunto in locale
     //a rete disattivata e quindi deve essere aggiunto
     if((found[i]==-1) && (localCar[i].changed))
     {
       struct CarInt_data data;
       memcpy(&data,&localCar[i],sizeof(data));
       data.mounted=0;

       if(remote_nFree)
       {
         //ci sono elementi liberi all'interno del database remoto
         //si inserisce il nuovo elemento nel primo record libero
         data.idx=remote_FreeList[remote_ptrFree++];
         remote_nFree--;

         //L'ultimo record segnato come libero in realta non e' su disco
         //si tratta di un record temporaneo, in memoria pronto per essere
         //riempito e salvato su disco
         if(remote_nFree==0)
         {
           //il record viene aggiunto fisicamente al db remoto
           remote_nFileRec++;
           //a questo punto la struttura in memoria non contiene
           //altri elementi liberi: ulteriori record vengono aggiunti
           //alla fine del file
         }
       }
       else
       {
         //il record viene aggiunto in fondo al database remoto
         data.idx=remote_nFileRec++;
       }

       if(!DBRemote.Write(data,data.idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
       {
         ret=0;
         break;
       }
     }
     //il caso magazzino presente in locale ma non risulta modificato e
     //non trovato in remoto in teoria non dovrebbe mai accadere eccetto che
     //nei casi in cui una macchina elimina un record dal proprio locale
     //e dal remoto; un'altra macchina quando cerchera' di allineare il proprio
     //locale al remoto trovera' un record non modificato in locale ed assente
     //in remoto. In questi casi non si esegue alcuna operazione: la successiva
     //copia del remoto sul locale eliminera' il record anche dalle macchine
     //estranee alla cancellazione del record

     waitBox.Increment();
   }

   #ifdef __DBNET_DEBUG
   DBLocal.WriteLogMsg("Search end. Now checking local/remote congruence");
   #endif

   for(int i=0;i<local_nRec;i++)
   {
     if(!IsNetEnabled())
     {
       break;
     }

     //se bit mode2=0 esegue il controllo solo sui magazzini segnati
     //come montati
     
     if(!(mode & 4))
     {
       if(localCar[i].mounted==0)
       {
         continue;
       }
     }

     //se elemento nel database locale non modificato a rete
     //disattivata e non presente in quello remoto: passa a prossimo elemento

     if(!localCar[i].changed)
     {
       continue;
     }

     //se elemento i-esimo nel db locale trovato nel remoto
     if(found[i]!=-1)
     {
       //chiede se portare le modifiche da locale su remoto

       int j=found[i];
       int tmpidx=remoteCar[j].idx;

       //NOTA: Per lo stesso numero di serie,in questo punto, il record locale
       //e quello remoto potrebbero avere indici diversi !!!

       if(CarInt_GetUsedState(localCar[i],remoteCar[j].idx)!=MAG_USEDBY_OTHERS)
       {
         switch(DBModified_AskUpdate(localCar[i].serial))
         {
           case DBMOD_UPDATE:

             memcpy(&remoteCar[j],&localCar[i],sizeof(localCar[i]));
             remoteCar[j].idx=tmpidx;
             remoteCar[j].mounted=0;

             if(!DBRemote.Write(remoteCar[j],remoteCar[j].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
             {
               ret=0;
               endloop=1;
             }
             else
             {
               localCar[i].changed=0;
               DBLocal.Write(localCar[i],localCar[i].idx,FLUSHON);
             }
                 
             break;
           case DBMOD_ABORT:
             ret=0;
             endloop=1;
             break;
         }
       }

       if(endloop)
       {
         break;
       }

       waitBox.Increment();
     }
   }

   #ifdef __DBNET_DEBUG
   DBLocal.WriteLogMsg("Checking complete. Now copy remote on local");
   #endif   

   if(found)
   {
     delete[] found;
   }

   if(remoteCar!=NULL)
   {
     delete[] remoteCar;
   }

   if(remote_FreeList!=NULL)
   {
     delete[] remote_FreeList;
   }

   //Se rete abilitata sovrascrive database locale con remoto
   if(IsNetEnabled())
   {
     if(DBRemote.LockOpen(CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
     {
       remote_nRec=DBRemote.Count();

       waitBox.SetValue( 0 );
       waitBox.SetVMax( remote_nRec );

       //crea file nuovo database locale
       DBLocal.Create();

       CarInt_data data;

       for(int i=0;i<remote_nRec;i++)
       {
         //legge l'elemento i-esimo del db remoto
         if(!DBRemote.Read(data,i))
         {
           if(IsNetEnabled())
           {
             continue;
           }

           W_Mess(ERR_NETDBUPDATE);

           ret=0;
           break;
         }

         //reset flag magazzino montato
         data.mounted=0;

         //Si cerca nel vecchio db locale il numero di serie
         for(int j=0;j<local_nRec;j++)
         {
           if(((data.serial & 0xFFFFFF)>0) && (localCar[j].serial==data.serial))
           {
             //se risulta montato lo mantiene montato anche nel nuovo
             //db locale
             data.mounted=localCar[j].mounted;
             break;
           }           
         }

         //reset flag modificato
         data.changed=0;

         //SMOD021106 - Commentato. Se c'� stato un errore di lettura dalla
         //rete e' impossibile arrivare in questo punto: il ciclo viene
         //interrotto prima

         //l'indice corrente puo' essere diverso da i a causa di eventuali
         //errori di lettura dalla rete
         //data.idx=count;

         //Scrive l'elemento nel database locale
         //SMOD021106 - Count diventa quindi inutile e si puo' usare idx
         DBLocal.Write(data,data.idx,FLUSHON);

         waitBox.Increment();
       }

       //SMOD040706 START - Reload del database locale in memoria

       if(CarList)
       {
         delete [] CarList;
         CarList=NULL;
       }

       if(FreeList)
       {
         delete[] FreeList;
         FreeList=NULL;
       }

       DBLocal.InitList(&CarList,&FreeList,CarList_nRec,Free_nRec);

       //SMOD040706 END
     }

     DBRemote.UnLockClose();
   }

   #ifdef __DBNET_DEBUG
   DBLocal.WriteLogMsg("Copy completed");
   #endif

   //SMOD170107 Spostato qui in modo da evitare che ConfImport acceda
   //ai database remoto e locale prima che siano allineati
   if(mode & 1)
   {
     ConfImport(0);
   }

   //160107 Attualmente il bit1 di mode non viene mai posto ad 1
   //la porzione di codice successiva viene sempre eseguita
   if(!(mode & 2))
   {
     if(localCar!=NULL)
     {
       delete[] localCar;
     }

     if(local_FreeList!=NULL)
     {
       delete[] local_FreeList;
     }
   }

   waitBox.Hide();

   return(ret);

}

//copia database locale su database remoto
void CopyLocalDB(void)
{
	if(!IsNetEnabled() || !(QHeader.modal & ENABLE_CARINT))
	{
		return;
	}

	if(!W_Deci(0,ASK_COPYDBLOCAL2))
	{
		return;
	}

	if(!W_Deci(0,ASK_COPYDBLOCAL))
	{
		return;
	}

	if(!DBRemote.LockOpen(CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
	{
		return;
	}


	CWaitBox waitBox( 0, 15, MsgGetString(Msg_01758), MAXCARINTALL );
	waitBox.Show();


	CarInt_data data;

	for(int i=0; i<DBLocal.Count(); i++)
	{
		DBLocal.Read(data,i);

		int tmp=data.mounted;

		data.mounted=0;

		if(!DBRemote.Write(data,i,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
		{
			waitBox.Hide();
			DBRemote.UnLockClose();
			return;
		}

		data.mounted=tmp;

		waitBox.Increment();
	}

	waitBox.Hide();

	DBRemote.UnLockClose();
}

//===========================================================================



//---------------------------------------------------------------------------
// finestra: Caricatore corrente (associato al magazzino)
//---------------------------------------------------------------------------
#define CINTSEARCH_ROWS 12

FeederAdvancedSearchUI::FeederAdvancedSearchUI( CWindow* parent ) : CWindowTable( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU );
	SetClientAreaPos( 0, 7 );
	SetClientAreaSize( 115, CINTSEARCH_ROWS + 6 );
	SetTitle( MsgGetString(Msg_01699) );

	m_Results = 0;
	m_ResultsEnd = 0;
	m_curSel = 0;
	m_TabStart = 0;
	m_db = 0;
}

FeederAdvancedSearchUI::~FeederAdvancedSearchUI()
{
	if( m_db )
	{
		delete [] m_db;
	}

	while( m_Results )
	{
		struct _searchNode* ptr = m_Results->next;
		delete m_Results;
		m_Results = ptr;
	}
}

void FeederAdvancedSearchUI::onInit()
{
	// create table
	m_table = new CTable( 3, 2, CINTSEARCH_ROWS, TABLE_STYLE_DEFAULT, this );

	// add columns
	m_table->AddCol( MsgGetString(Msg_01274),  7, CELL_TYPE_UINT, CELL_STYLE_READONLY ); //numero di serie
	m_table->AddCol( "N",  1, CELL_TYPE_UINT, CELL_STYLE_READONLY ); //ruota
	m_table->AddCol( MsgGetString(Msg_01290), 25, CELL_TYPE_TEXT, CELL_STYLE_READONLY ); //tipo comp
	m_table->AddCol( MsgGetString(Msg_01306),  5, CELL_TYPE_UINT, CELL_STYLE_READONLY ); //quantita'
	m_table->AddCol( MsgGetString(Msg_01701), 33, CELL_TYPE_TEXT, CELL_STYLE_READONLY ); //note car
	m_table->AddCol( MsgGetString(Msg_01700), 33, CELL_TYPE_TEXT, CELL_STYLE_READONLY ); //note mag
}

void FeederAdvancedSearchUI::onShow()
{
	if( !m_Results )
	{
		return;
	}
	m_curSel = m_Results;

	char buf[80];
	snprintf( buf, sizeof(buf), MsgGetString(Msg_01704), m_totQuant );
	DrawTextCentered( CINTSEARCH_ROWS + 3, buf );
}

void FeederAdvancedSearchUI::onRefresh()
{
	if( !m_Results )
	{
		return;
	}

	GUI_Freeze_Locker lock;

	struct _searchNode* ptr = m_TabStart;

	for( unsigned int i = 0; i < m_table->GetRows(); i++ )
	{
		if( ptr->data->usedBy == 0 || !IsNetEnabled() )
		{
			m_table->SetRowStyle( i, TABLEROW_STYLE_DEFAULT );
		}
		else
		{
			if( ptr->data->usedBy == nwpar.NetID_Idx )
			{
				m_table->SetRowStyle( i, TABLEROW_STYLE_HIGHLIGHT_GREEN );
			}
			else
			{
				m_table->SetRowStyle( i, TABLEROW_STYLE_HIGHLIGHT_RED );
			}
		}

		char buf[12];
		switch((ptr->data->serial >> 24) & 0xFF)
		{
			case CARINT_TAPESER_OFFSET:
				snprintf(buf,sizeof(buf),"T%6d",ptr->data->serial & 0xFFFFFF);
				break;
			case CARINT_AIRSER_OFFSET:
				snprintf(buf,sizeof(buf),"A%6d",ptr->data->serial & 0xFFFFFF);
				break;
			case CARINT_GENERICSER_OFFSET:
				snprintf(buf,sizeof(buf),"G%6d",ptr->data->serial & 0xFFFFFF);
				break;
			default:
				snprintf(buf,sizeof(buf),"X%6d",ptr->data->serial & 0xFFFFFF);
				break;
		}

		m_table->SetText( i, 0, buf );
		m_table->SetText( i, 1, ptr->nruota+1 );
		m_table->SetText( i, 2, ptr->data->elem[ptr->nruota] );
		m_table->SetText( i, 3, ptr->data->num[ptr->nruota] );
		m_table->SetText( i, 4, ptr->data->note );
		m_table->SetText( i, 5, ptr->data->noteCar[ptr->nruota] );

		ptr = ptr->next;

		if( ptr == NULL )
		{
			break;
		}
	}

	showUsedBy();
}

bool FeederAdvancedSearchUI::onKeyPress( int key )
{
	switch( key )
	{
		case K_DOWN:
			return onKeyDown();

		case K_UP:
			return onKeyUp();

		default:
			break;
	}

	return false;
}

bool FeederAdvancedSearchUI::onKeyDown()
{
	if( m_table->GetCurRow() == CINTSEARCH_ROWS-1 )
	{
		// ultima riga

		if( m_curSel == m_ResultsEnd )
		{
			//gestito: niente altro da visualizzare
		}
		else
		{
			// visualizzo nuovo record
			m_TabStart = m_TabStart->next;
			m_curSel = m_curSel->next;
		}
		return true;
	}

	// niente altro da visualizzare
	if( m_curSel == m_ResultsEnd )
	{
		return true;
	}

	m_curSel = m_curSel->next;
	showUsedBy();
	return false;
}

bool FeederAdvancedSearchUI::onKeyUp()
{
	if( m_table->GetCurRow() == 0 )
	{
		//prima riga

		if( m_curSel == m_Results )
		{
			//gestito: niente altro da visualizzare
		}
		else
		{
			// visualizzo nuovo record
			m_TabStart = m_TabStart->prev;
			m_curSel = m_TabStart;
		}
		return true;
	}

	m_curSel = m_curSel->prev;
	showUsedBy();
	return false;
}

bool FeederAdvancedSearchUI::Search( char* mask )
{
	m_Results = NULL;
	m_ResultsEnd = NULL;
	m_curSel = NULL;

	if( m_db )
	{
		delete [] m_db;
	}

	int dummy;
	int nrec;
	DBLocal.InitList( &m_db, NULL, nrec, dummy );

	m_totQuant = 0;

	DelSpcR( mask );
	for( int i = 0; i < nrec; i++ )
	{
		for( int j = 0; j < 8; j++ )
		{
			DelSpcR( m_db[i].elem[j] );

			if( !strcmp_wildcard(m_db[i].elem[j],mask) )
			{
				if( m_ResultsEnd != NULL )
				{
					m_ResultsEnd->next = new _searchNode;
					m_ResultsEnd->next->prev = m_ResultsEnd;
					m_ResultsEnd = m_ResultsEnd->next;
				}
				else
				{
					m_Results = new _searchNode;
					m_Results->prev = NULL;
					m_ResultsEnd = m_Results;
				}

				m_ResultsEnd->nruota = j;
				m_ResultsEnd->data = &m_db[i];

				m_totQuant += m_db[i].num[j];

				m_ResultsEnd->next=NULL;
			}
		}
	}

	m_TabStart = m_Results;

	if( !m_Results )
	{
		W_Mess( MsgGetString(Msg_01355) ); // not found
		return false;
	}

	return true;
}

void FeederAdvancedSearchUI::showUsedBy()
{
	GUI_Freeze_Locker lock;

	char buf[52];
	InsSpc( 50, buf );
	DrawText( 3, 17, buf );

	if( !IsNetEnabled() )
	{
		return;
	}

	if( CarInt_GetUsedState(*m_curSel->data) == MAG_USEDBY_OTHERS )
	{
		char name[51];
		if( GetMachineName(m_curSel->data->usedBy,name) )
		{
			name[30]='\0';

			snprintf( buf, 50, MsgGetString(Msg_02008), name );
			DrawText( 3, 17, buf );
		}
	}
}



int CarIntAuto_ElabResults(struct CarIntAuto_res results,int elabmask,int mode)
{
	char testo[1000];
	int oneFound=0;
	int nloop;

	//Controlla che non vi siano due o piu numeri di serie uguali
	
	for(nloop=0;nloop<MAXMAG;nloop++)
	{
		if(results.ser[nloop]==CARINT_NOSERIAL)
		{
			continue;
		}
		
		if(results.type[nloop] == CARINT_TYPE_PROTCAP)
		{
			continue;
		}
			
		oneFound=1;
	
		int k;
	
		for(k=0;k<MAXMAG;k++)
		{
			if(k==nloop)
			{
				continue;
			}
	
			if(results.ser[k] == CARINT_NOSERIAL)
			{
				continue;
			}
			
			if(results.type[k] == CARINT_TYPE_PROTCAP)
			{
				continue;
			}
				
			if(results.ser[nloop] == results.ser[k])
			{
				break;
			}
		}

		if(k<MAXMAG)
		{
			//trovato magazzino duplicato
	
			switch(results.type[k])
			{
				case CARINT_TYPE_TAPE:
					snprintf( testo, sizeof(testo),CARINT_AUTODUP,'T',results.ser[k] & 0xFFFFFF);
					break;
				case CARINT_TYPE_AIR:
					snprintf( testo, sizeof(testo),CARINT_AUTODUP,'A',results.ser[k] & 0xFFFFFF);
					break;
				case CARINT_TYPE_GENERIC:
					snprintf( testo, sizeof(testo),CARINT_AUTODUP,'G',results.ser[k] & 0xFFFFFF);
					break;
				default:
					snprintf( testo, sizeof(testo),CARINT_AUTODUP,'X',results.ser[k] & 0xFFFFFF);
					break;
			}
			
			W_Mess(testo);
	
			//interrompi operazione
			return 0;
		}      
	}

	//Se nessun caricatore trovato
	if(mode && (!oneFound))
	{
		W_Mess(CARINT_NOFOUND_ERR);

		//termina operazione
		return 1;
	}   

	CWindow *wait=new CWindow(WAITAUTOMAG_POS2,"");
	wait->Show();
	wait->DrawTextCentered( 0, WAITAUTOMAG_TXT2 );
	
	int prevopen_CarFile=1;
	
	if(CarFile==NULL)
	{
		CarFile=new FeederFile(QHeader.Conf_Default);
		prevopen_CarFile=0;
	}
	
	int status[MAXMAG];
	
	//puntatori ai record del database corrispondenti ai magazzini trovati
	int found_idx[MAXMAG];
	
	//puntatori ai record del database a cui fa riferimento la configurazione
	int conf_idx[MAXMAG];
	
	//Prepara conf_idx: indice del record nel database corrispondente
	//al magazzino indicato dalla configurazione in una determinata posizione
	for(nloop=0;nloop<MAXMAG;nloop++)
	{
		conf_idx[nloop]=-1;
	
		if(ConfDBLink[nloop]==-1)
		{
			continue;
		}

		for(int k=0;k<CarList_nRec;k++)
		{
			if(CarList[k].idx==ConfDBLink[nloop])
			{
				conf_idx[nloop]=k;
				break;
			}       
		}
	}

	//verifica correttezza dei numeri di serie trovati
	for(nloop=0;nloop<MAXMAG;nloop++)
	{
		found_idx[nloop]=-1;
	
		//Se in posizione nloop e' stato trovato un magazzino e non e' un tappo di protezione
		
		if((results.ser[nloop]!=CARINT_NOSERIAL) && (results.type[nloop]!=CARINT_TYPE_PROTCAP))
		{
			//Cerca il numero di serie riconosciuto, nel database.
		
			for(int i=0;i<CarList_nRec;i++)
			{
				if(CarList[i].serial==results.ser[nloop])
				{
					found_idx[nloop]=i;
					break;
				}
			}

			//il numero di serie trovato non e' presente nel database
			if(found_idx[nloop]==-1)
			{
				wait->Hide();
				
				switch((results.ser[nloop] >> 24) & 0xFF)
				{
					case CARINT_TAPESER_OFFSET:
						snprintf( testo, sizeof(testo),AUTOMAG_SINGLE_NOTINDB,'T',results.ser[nloop] & 0xFFFFFF);
						break;
					case CARINT_AIRSER_OFFSET:
						snprintf( testo, sizeof(testo),AUTOMAG_SINGLE_NOTINDB,'A',results.ser[nloop] & 0xFFFFFF);
						break;
					case CARINT_GENERICSER_OFFSET:
						snprintf( testo, sizeof(testo),AUTOMAG_SINGLE_NOTINDB,'G',results.ser[nloop] & 0xFFFFFF);
						break;
					default:
						snprintf( testo, sizeof(testo),AUTOMAG_SINGLE_NOTINDB,'X',results.ser[nloop] & 0xFFFFFF);
						break;
				}
		
				//segnala errore e termina operazione
				W_Mess(testo);
		
				if(!prevopen_CarFile)
				{
					delete CarFile;
					CarFile=NULL;
				}
		
				delete wait;
			
				return 0;
			}
			else
			{
				//il magazzino trovato e' presente nel database
				int used_by =CarInt_GetUsedState(CarList[found_idx[nloop]]);
	
				//se il modo usa solo caricatori intelligenti non e' attivo
				if(!(QHeader.modal & ONLY_SMARTFEEDERS))
				{
					if(used_by==MAG_USEDBY_OTHERS)
					{
						//mostra errore se il magazzino riulta utilizzato da un'altra macchina
						switch((results.ser[nloop] >> 24) & 0xFF)
						{
							case CARINT_TAPESER_OFFSET:
								snprintf( testo, sizeof(testo),ERRMAG_USEDBY_OTHERS2,'T',results.ser[nloop] & 0xFFFFFF);
								break;
							case CARINT_AIRSER_OFFSET:
								snprintf( testo, sizeof(testo),ERRMAG_USEDBY_OTHERS2,'A',results.ser[nloop] & 0xFFFFFF);
								break;
							case CARINT_GENERICSER_OFFSET:
								snprintf( testo, sizeof(testo),ERRMAG_USEDBY_OTHERS2,'G',results.ser[nloop] & 0xFFFFFF);
								break;
							default:
								snprintf( testo, sizeof(testo),ERRMAG_USEDBY_OTHERS2,'X',results.ser[nloop] & 0xFFFFFF);
								break;
						}					

						W_Mess(testo);
	
						if(!prevopen_CarFile)
						{
							delete CarFile;
							CarFile=NULL;
						}
	
						delete wait;
			
						return 0;
					}           
				}
				else
				{
					//modo usa solo caricatori intelligenti attivo
					if(used_by==MAG_USEDBY_OTHERS)
					{
						//se il magazzino risulta in uso da un'altra macchina
						//forza il magazzino come non piu in uso da quest'ultima
						CarInt_SetAsUnused(CarList[found_idx[nloop]],1);
						//e prende possesso del magazzino
						CarInt_SetAsUsed(CarList[found_idx[nloop]]);
					}
				}

				CarList[found_idx[nloop]].smart = 1;
		
				CarList[found_idx[nloop]].changed=CARINT_NONET_CHANGED;
		
				if(IsNetEnabled())
				{
					if(DBRemote.Write(CarList[found_idx[nloop]],CarList[found_idx[nloop]].idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
					{
						CarList[found_idx[nloop]].changed=0;
					}
				}

				DBLocal.Write(CarList[found_idx[nloop]],CarList[found_idx[nloop]].idx,FLUSHON);           
			}
		}
	}

	//conf_ok[i] vale 1 se il magazzino in posizione i indicato dalla configurazione
	//e' stato trovato anche se in una posizione diversa da quanto previsto, oppure
	//la configurazione non indicava alcun magazzino ed effettivamente non e' stato
	//trovato alcuno.
	int conf_ok[MAXMAG];
	
	for(int i=0;i<MAXMAG;i++)
	{
		conf_ok[i]=0;
	}
	
	for(int i=0;i<MAXMAG;i++)
	{
		//se in posizione i nessun magazzino e' stato trovato
		if(found_idx[i]==-1)
		{
			//e la configurazione non ne indicava
			if(conf_idx[i]==-1)
			{
				//ok
				status[i]=CARINT_AUTO_OK;
				conf_ok[i]=1;
			}
			else
			{
				//altrimenti segnala la mancanza del magazzino: non e' detto
				//che rappresenti un errore in quanto il magazzino richiesto
				//potra' essere trovato in un'altra posizione
				status[i]=CARINT_AUTO_NOFOUND;
			}
		
			continue;
		}

		//Cerca il magazzino trovato in posizione i tra quelli indicati dalla
		//configurazione
		int conf_pos=-1;
	
		for(int k=0;k<MAXMAG;k++)
		{
			if(conf_idx[k]!=-1)
			{
				if(found_idx[i]==conf_idx[k])
				{
					conf_pos=k;
					break;
				}
			}
		}

		if(conf_pos==i)
		{
			//se il magazzino trovato corrisponde a quanto indicato dalla
			//configurazione: ok
			status[i]=CARINT_AUTO_OK;
			conf_ok[i]=1;
		}
		else
		{
			if(conf_pos!=-1)
			{
		
				//il magazzino trovato e' indicato dalla configurazione
				//in un'altra posizione (conf_ok)
				status[i]=CARINT_AUTO_WRONGPOS;
				conf_ok[conf_pos]=1;
			}
			else
			{
				//nessuna corrispondenza tra il magazzino trovato e quelli nella
				//configurazione
				status[i]=CARINT_AUTO_NOCONF;
			}
		}
	}

	for(int i=0;i<MAXMAG;i++)
	{
		if(status[i]==CARINT_AUTO_NOFOUND)
		{
			//se in posizione nloop la configurazione indicava un magazzino
			//non smart che non e' stato trovato ne in nloop ne in un'altra
			//posizione...
		
			if(!(QHeader.modal & ONLY_SMARTFEEDERS))
			{
				if((conf_idx[i]!=-1) && (conf_ok[i]==0))
				{
					if(!CarList[conf_idx[i]].smart)
					{
						//fa finta che  sia stato trovato il magazzino indicato dalla
						//configurazione
						found_idx[i]=conf_idx[i];
			
						//corrispondenza ok ma con caricatore no smart
						status[i]=CARINT_AUTO_OKMANUAL;
					}
				}
			}     
		}   
	}

	struct CarDat *allcar=new struct CarDat[8*MAXMAG];
	
	//Legge tutti i caricatori della configurazione
	for(int k=0;k<8*MAXMAG;k++)
	{
		CarFile->ReadRec(k,allcar[k]);
	}
	
	int showRep=0;
	int doOper=1;
	
	
	//Determina se mostrare o meno il report
	for(nloop=0;nloop<MAXMAG;nloop++)
	{
		switch(status[nloop])
		{
			case CARINT_AUTO_NOFOUND:
				break;
		
			case CARINT_AUTO_OKMANUAL:
				break;
				
			//se il magazzino trovato in nloop non e' indicato dalla configurazione
			//mostra sempre il report
			case CARINT_AUTO_NOCONF:
				showRep=1;
				break;
		}

		if(showRep)
		{
			break;
		}
	}

	if(showRep)
	{
		CWindow *wreport=new CWindow(CARINTAUTO_REPPOS, MsgGetString(Msg_01773) );
		wreport->Show();
	
		wreport->DrawTextCentered( 0, CARINT_AUTOREP_MSG );
	
		char confT,dbT;
		unsigned int confSer,dbSer;
	
		for(nloop=0;nloop<MAXMAG;nloop++)
		{
			if(conf_idx[nloop]!=-1)
			{
				confSer=CarList[conf_idx[nloop]].serial & 0xFFFFFF;
		
				switch((CarList[conf_idx[nloop]].serial >> 24) & 0xFF)
				{
					case CARINT_TAPESER_OFFSET:
						confT='T';
						break;
					case CARINT_AIRSER_OFFSET:
						confT='A';
						break;
					case CARINT_GENERICSER_OFFSET:
						confT='G';
						break;
					default:
						confT='X';
						break;
				}								
			}

			if(found_idx[nloop]!=-1)
			{
				dbSer=CarList[found_idx[nloop]].serial & 0xFFFFFF;
		
				switch((CarList[found_idx[nloop]].serial >> 24) & 0xFF)
				{
					case CARINT_TAPESER_OFFSET:
						dbT='T';
						break;
					case CARINT_AIRSER_OFFSET:
						dbT='A';
						break;
					case CARINT_GENERICSER_OFFSET:
						dbT='G';
						break;
					default:
						dbT='X';
						break;
				}					
				
			}

			int msg_type;
		
			if((conf_idx[nloop]!=-1) && (found_idx[nloop]!=-1))
			{
				msg_type=0;
			}
			else
			{
				if((conf_idx[nloop]==-1) && (found_idx[nloop]!=-1))
				{
					msg_type=1;
				}
				else
				{
					if((conf_idx[nloop]!=-1) && (found_idx[nloop]==-1))
					{
						msg_type=2;
					}
					else
					{
						msg_type=3;
					}
				}
			}

			char status_txt[64];
		
			switch(status[nloop])
			{
				case CARINT_AUTO_OK:
					strcpy( status_txt, MsgGetString(Msg_01827) );
					break;
				case CARINT_AUTO_OKMANUAL:
					strcpy( status_txt, CARINT_OK_MANUAL_TXT );
					break;
				case CARINT_AUTO_WRONGPOS:
					strcpy( status_txt, CARINT_ERR_WRONGPOS_TXT );
					break;
				case CARINT_AUTO_NOCONF:
					strcpy( status_txt, CARINT_ERR_NOCONF_TXT );
					break;
				case CARINT_AUTO_NOFOUND:
					strcpy( status_txt, CARINT_ERR_NOFOUND_TXT );
					break;
			}
       
			switch(msg_type)
			{
				case 0:
					snprintf( testo, sizeof(testo),CARINT_AUTOSTAT_MSG1,nloop+1,confT,confSer,dbT,dbSer,status_txt);
					break;
				case 1:
					snprintf( testo, sizeof(testo),CARINT_AUTOSTAT_MSG2,nloop+1,dbT,dbSer,status_txt);
					break;
				case 2:
					snprintf( testo, sizeof(testo),CARINT_AUTOSTAT_MSG3,nloop+1,confT,confSer,status_txt);
					break;
				case 3:
					snprintf( testo, sizeof(testo),CARINT_AUTOSTAT_MSG4,nloop+1,status_txt);
					break;
			}
	
			wreport->DrawText( 2, 2+nloop, testo );
		}

		wreport->DrawTextCentered( 17, CARINT_AUTOREP_ASK );
	
		GUIButtons* ButtonSet = new GUIButtons( 20, wreport );
		ButtonSet->AddButton( FAST_NO , 1 );
		ButtonSet->AddButton( FAST_YES , 0 );
		ButtonSet->Activate();
	
		int c,bidx;
	
		do
		{
			c=Handle();
			bidx=ButtonSet->GestKey(c);
	
		} while(((c!=K_ESC) && (c!=K_ENTER)) && ((bidx & BUTTONCONFIRMED)==0));

		bidx=bidx & ~BUTTONCONFIRMED;
	
		if((c==K_ESC) || (bidx==1))
		{
			//Operazione annullata: non eseguire alcuna operazione sul database
			doOper=0;
		}
	
		delete ButtonSet;
		delete wreport;
	}
	
	if(doOper)
	{
		//Esegue se necessario le operazioni sui magazzini
		for(nloop=0;nloop<MAXMAG;nloop++)
		{
			switch(status[nloop])
			{
				case CARINT_AUTO_OKMANUAL:
					//nop: il caricatore era montato e rimane tale
					break;
				case CARINT_AUTO_NOFOUND:	
					if(conf_idx[nloop]!=-1)
					{
						CarList[conf_idx[nloop]].mounted=0;
						CarInt_SetAsUnused(CarList[conf_idx[nloop]]);
			
						DBLocal.Write(CarList[conf_idx[nloop]],CarList[conf_idx[nloop]].idx,FLUSHON);
						conf_idx[nloop]=-1;
						ConfDBLink[nloop]=-1;
			
						for(int r=0;r<8;r++)
						{
							allcar[nloop*8+r].C_att=0;
							allcar[nloop*8+r].C_tipo=0;
							allcar[nloop*8+r].C_quant=0;
							allcar[nloop*8+r].C_PackIndex=0;
							allcar[nloop*8+r].C_Ncomp=0;
							allcar[nloop*8+r].C_Package[0]='\0';
							allcar[nloop*8+r].C_note[0]='\0';
							allcar[nloop*8+r].C_comp[0]='\0';
							allcar[nloop*8+r].C_sermag=0;
	
							CarFile->SaveRecX(nloop*8+r,allcar[nloop*8+r]);
						}
					}
					break;

				case CARINT_AUTO_NOCONF:
				case CARINT_AUTO_WRONGPOS:
		
					//smonta il magazzino precedentemente indicato dalla configurazione
					//(se c'era)
			
					if(conf_idx[nloop]!=-1)
					{
						CarList[conf_idx[nloop]].mounted=0;
						CarInt_SetAsUnused(CarList[conf_idx[nloop]]);
						
						DBLocal.Write(CarList[conf_idx[nloop]],CarList[conf_idx[nloop]].idx,FLUSHON);
						conf_idx[nloop]=-1;
						ConfDBLink[nloop]=-1;
					}
					
					//monta il nuovo magazzino
					ConfDBLink[nloop]=CarList[found_idx[nloop]].idx;

					CarList[found_idx[nloop]].mounted=nloop+1;
					CarInt_SetAsUsed(CarList[found_idx[nloop]]);
			
					DBLocal.Write(CarList[found_idx[nloop]],CarList[found_idx[nloop]].idx,FLUSHON);

					//aggiorna la configurazione
					for(int r=0;r<8;r++)
					{
						allcar[nloop*8+r].C_att=CarList[found_idx[nloop]].tavanz[r];
						allcar[nloop*8+r].C_tipo=CarList[found_idx[nloop]].tipoCar[r];
						allcar[nloop*8+r].C_PackIndex=CarList[found_idx[nloop]].packIdx[r];
						allcar[nloop*8+r].C_quant=CarList[found_idx[nloop]].num[r];
						
						allcar[nloop*8+r].C_sermag=CarList[found_idx[nloop]].serial;
						strcpy(allcar[nloop*8+r].C_Package,CarList[found_idx[nloop]].pack[r]);
						strcpy(allcar[nloop*8+r].C_note,CarList[found_idx[nloop]].noteCar[r]);
						strcpy(allcar[nloop*8+r].C_comp,CarList[found_idx[nloop]].elem[r]);
			
						SetFeederNCompDef(allcar[nloop*8+r]);
						
						CarFile->SaveRecX(nloop*8+r,allcar[nloop*8+r]);
					}
					break;
			}
		}
		CarFile->SaveFile();
	}
	else
	{
		//operazione annullata: nessuna operazione eseguita
	
		if(!prevopen_CarFile)
		{
			delete CarFile;
			CarFile=NULL;
		}
	
		delete wait;
		delete allcar;
	
		return 0;
	}

	if(!prevopen_CarFile)
	{
		delete CarFile;
		CarFile=NULL;
	}

	delete wait;
	delete[] allcar;

	return 1;
}



int CarList_GetNRec(void)
{
  return(CarList_nRec);
}

void DisableDB(void)
{
	CWaitBox waitBox( 0, 15, MsgGetString(Msg_02013), CarList_nRec );
	waitBox.Show();

	MagUnmountAll( &waitBox );

	waitBox.Hide();

	if(IsNetEnabled())
	{
		DBRemote.Close();
	}

	//dealloca struttura dati magazzini
	DeAllocCarInt();

	//chiude il database locale
	DBLocal.Close();
}


void EnableFeederDB()
{
	//apre il database locale
	DBLocal.Open();
	
	if( IsNetEnabled() )
	{
		//se la rete e' attivata apre file database remoto
		if(DBRemote.Open(CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
		{
			//aggiorna db locale con remoto; comprende anche link tra db e configurazione.
			UpdateDBData(CARINT_UPDATE_FULL | CARINT_UPDATE_INITMNT);
		}
	}
	else
	{
		//altrimenti esegui solo link tra configurazione db locale
		ConfImport(0);
	}
	
	//carica struttura dati db locale
	InitCarList();
}

void DisableFeederDB()
{
	//esegui force su abilitazione rete
	RemoveForceNetEnabled();
	
	//..lo stesso per il flag di abilitazione db
	QHeader.modal |= ENABLE_CARINT;
	
	//esegui disabilitazione completa del db
	DisableDB();
	
	//elimina force su abilitazione rete
	RemoveForceNetEnabled();
	
	//e disattiva il db come richiesto dall'utente
	QHeader.modal &= ~ENABLE_CARINT;
}



//---------------------------------------------------------------------------
// finestra: Feeder import
//---------------------------------------------------------------------------
FeederImportUI::FeederImportUI( CWindow* parent, int num )
: CWindowSelect( parent, 8, 1, 8, 10 )
{
	SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU );
	SetClientAreaPos( 0, 7 );
	SetClientAreaSize( 74, 24 );
	SetTitle( MsgGetString(Msg_00464) );

	feederPackNumber = num;

	// create combos
	combos[FEEDER_PACK]  = new C_Combo( 3, 11, MsgGetString(Msg_01810),  2, CELL_TYPE_UINT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
	combos[FEEDER_NOTES] = new C_Combo( 3, 12, MsgGetString(Msg_01035), 42, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );

	// add to combo list
	comboList = new CComboList( this );
	comboList->Add( combos[FEEDER_PACK] , 0, 0 );
	comboList->Add( combos[FEEDER_NOTES], 1, 0 );

	// create table
	infoTable = new CTable( 3, 15, 8, TABLE_STYLE_DEFAULT, this );
	// add columns
	infoTable->AddCol( MsgGetString(Msg_01290), 25, CELL_TYPE_TEXT, CELL_STYLE_READONLY ); //tipo comp
	infoTable->AddCol( MsgGetString(Msg_00678), 21, CELL_TYPE_TEXT, CELL_STYLE_READONLY ); //package
	infoTable->AddCol( MsgGetString(Msg_01034), 20, CELL_TYPE_TEXT, CELL_STYLE_READONLY ); //note
}

FeederImportUI::~FeederImportUI()
{
	for( unsigned int i = 0; i < combos.size(); i++ )
	{
		if( combos[i] )
		{
			delete combos[i];
		}
	}

	if( comboList )
	{
		delete comboList;
	}

	if( infoTable )
	{
		delete infoTable;
	}
}

void FeederImportUI::onInitSomething()
{
	m_table->SetOnSelectCellCallback( boost::bind( &FeederImportUI::onSelectionChange, this, _1, _2 ) );
}

void FeederImportUI::onShowSomething()
{
	// show combos
	comboList->Show();
	// show table
	infoTable->Show();
	infoTable->Deselect();

	DrawTextCentered( 3, MsgGetString(Msg_01811) );
	DrawTextCentered( 4, MsgGetString(Msg_01812) );
	DrawTextCentered( 5, MsgGetString(Msg_01813) );
}

int FeederImportUI::onSelectionChange( unsigned int row, unsigned int col )
{
	for( int i = 0; i < 8; i++ )
	{
		infoTable->SetText( i, 0, CarList[ConfImport_valid[col]].elem[i] );
		infoTable->SetText( i, 1, CarList[ConfImport_valid[col]].pack[i] );
		infoTable->SetText( i, 2, CarList[ConfImport_valid[col]].noteCar[i] );
	}

	combos[FEEDER_PACK]->SetTxt( feederPackNumber );
	combos[FEEDER_NOTES]->SetTxt( CarList[ConfImport_valid[col]].note );

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Feeders test
//---------------------------------------------------------------------------

int FeedersTest_X()
{
	struct CarIntAuto_res results;
	CarIntAutoMag( CARINT_SEARCH_NORMAL, CARINT_AUTOALL_MASK, results );
	CarIntAuto_ShowResults( results.ser );
	return 1;
}

int FeedersTest()
{
	CWindow* wind = new CWindow( 0 );
	wind->SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU );
	wind->SetClientAreaPos( 0, 5 );
	wind->SetClientAreaSize( 40, 20 );
	wind->SetTitle( MsgGetString(Msg_01773) );

	CComboList* CLSer = new CComboList( wind );
	C_Combo* CSer[2][MAXMAG];

	for(int i=0;i<MAXMAG;i++)
	{
		char buf[64];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01738), i+1 );

		CSer[0][i] = new C_Combo( 5, 3+i, buf, 1, CELL_TYPE_TEXT, CELL_STYLE_READONLY );
		CSer[0][i]->SetLegalChars( "ATG" );

		CSer[1][i] = new C_Combo( 25, 3+i, "", 8, CELL_TYPE_UINT, CELL_STYLE_READONLY );
		CSer[1][i]->SetVMinMax( 1, 999999 );

		CLSer->Add( CSer[0][i], i, 0 );
		CLSer->Add( CSer[1][i], i, 1 );
	}


	GUI_Freeze();

	wind->Show();
	CLSer->Show();
	wind->DrawTextCentered( 1, MsgGetString(Msg_01563) );

	GUI_Thaw();

	int serial;
	unsigned int chksum;
	int type;
	int swver;

	for( int i = 0; i < MAXMAG; i++ )
	{
		if( !CarInt_GetSerial( i, serial, chksum, type, swver ) )
		{
			serial = 0;
		}

		int t = (serial >> 24) & 0xFF;
		if(t && (t != CARINT_PROTCAP_OFFSET))
		{
			switch(t)
			{
				case CARINT_TAPESER_OFFSET:
					CSer[0][i]->SetTxt("T");
					break;
				case CARINT_AIRSER_OFFSET:
					CSer[0][i]->SetTxt("A");
					break;
				case CARINT_GENERICSER_OFFSET:
					CSer[0][i]->SetTxt("G");
					break;
				default:
					CSer[0][i]->SetTxt("-");
					CSer[1][i]->SetTxt("------");
					break;
			}

			if(t != 0xff)
			{
				CSer[1][i]->SetTxt(serial & 0xFFFFFF);
			}
		}
		else
		{
			CSer[0][i]->SetTxt("-");
			CSer[1][i]->SetTxt("------");
		}
	}

	int c;
	do
	{
		c=Handle();
		CLSer->ManageKey( c );
	} while( c != K_ESC );

	// cleanup
	for( int i = 0; i < MAXMAG; i++ )
	{
		delete CSer[0][i];
		delete CSer[1][i];
	}
	delete CLSer;
	delete wind;

	return 1;
}
