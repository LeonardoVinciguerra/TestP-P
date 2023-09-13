/*
>>>> Q_GENER.CPP

Funzioni di utilita' generale pubbliche

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++ 	Modificato da TWS Simone  
++++  Integrazioni LCM Simone >>S260701
*/

#include <vector>
#include <string>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h> //SMOD230703-bridge

#include <sys/stat.h>  //GNU
#include <sys/vfs.h>

#include <mntent.h>

#include "q_carint.h"
#include "msglist.h"
#include "q_cost.h"
#include "q_graph.h"
#include "q_gener.h"
#include "q_tabe.h"
#include "q_slide.h"  // L704

#ifdef __SNIPER
#include "sniper.h"
#include "las_scan.h"
#endif

#include "q_oper.h"
#include "motorhead.h"
#include "filefn.h"
#include "q_files.h"
#include "q_fox.h"
#include "q_assem.h"
#include "q_help.h"
#include "q_net.h"
#include "q_inifile.h"
#include "keyutils.h"
#include "datetime.h"
#include "lnxdefs.h"
#include "sound.h"
#include "strutils.h"
#include "fileutils.h"
#include "q_conf.h"
#include "q_dosat.h"

#include "gui_submenu.h"
#include "gui_functions.h"
#include "gui_defs.h"

#include "c_win_par.h"
#include "c_inputbox.h"
#include "c_pan.h"

#include <mss.h>

#ifdef __LOG_ERROR
#include "q_inifile.h"
#include "q_logger.h"
extern CLogger BackupLogger;
#endif

extern struct CfgHeader QHeader;

char MonthTable[]={31,28,31,30,31,30,31,31,30,31,30,31};


/*--------------------------------------------------------------------------
Confronta una stringa con una maschera di riferimento che puo contenere
caratteri jolly ? e *
INPUT:   str : stringa da confrontare
         maks: stringa della maschera di riferimento
RETURN:  0 se c'e corrispondenza, 1 altrimenti
NOTE:
--------------------------------------------------------------------------*/
int strcmp_wildcard(char *str,char *mask)
{
  char *start_mask=mask;
  char *start_str=str;

  int matchmode=1;
  int end=0;

  char *ptr1;
  char *ptr2;
  char *ptr;

  do
  {
    ptr1=strchr(start_mask,'?');
    ptr2=strchr(start_mask,'*');

    if((ptr1!=NULL) || (ptr2!=NULL))
    {
      if(ptr1<ptr2)
      {
        if(ptr1!=NULL)
        {
          ptr=ptr1;
        }
        else
        {
          ptr=ptr2;
        }
      }
      else
      {
        if(ptr2!=NULL)
        {
          ptr=ptr2;
        }
        else
        {
          ptr=ptr1;
        }
      }
    }
    else
    {
      ptr=start_mask+strlen(start_mask);
      end=1;
    }

    char wildcard=*ptr;

    if(ptr!=start_mask)
    {
      //wildcard trovata non ad inizio sottostringa mask corrente

      //terina sottostringa mask corrente
      *ptr='\0';

      if(matchmode)
      {
        //? (corrispondenza di posizione e valore)
        if(strncmp(start_str,start_mask,ptr-start_mask))
        {
          return(1);
        }

        start_str+=ptr-start_mask+1;

        if((wildcard!='\0') && (start_str>=(str+strlen(str))))
        {
          return(1);
        }
        
        start_mask=++ptr;
      }
      else
      {
        //* (corrispondenza solo del valore)
        start_str=strstr(start_str,start_mask);

        if(start_str==NULL)
        {
          return(1);
        }

        start_str+=strlen(start_mask);
        start_mask=++ptr;
      }

      if(wildcard=='?')
      {
        matchmode=1;
      }
      else
      {
        matchmode=0;
      }
    }
    else
    {
      if(wildcard=='?')
      {
        start_mask=++ptr;
        start_str++;
        if(start_str>=(str+strlen(str)))
        {
          return(1);
        }
        matchmode=1;
      }
      else
      {
        start_mask=++ptr;
        matchmode=0;
      }
    }
  } while(!end);

  return(0);

}

/*--------------------------------------------------------------------------
Confronta due date e fornisce un idice di ordine tra le due
INPUT:   date1,date2: date da confrontare
RETURN:  0 se d1 e' coincidente o precedenta a d2
         1 se d2 e' precedente a d1
NOTE:
--------------------------------------------------------------------------*/
int FirstDate(struct date d1,struct date d2)
{
  if(d1.da_year>d2.da_year)
  {
    return(1);
  }

  if(d1.da_year<d2.da_year)
  {
    return(0);
  }

  if(d1.da_mon>d2.da_mon)
  {
    return(1);
  }

  if(d1.da_mon<d2.da_mon)
  {
    return(0);
  }

  if(d1.da_day>d2.da_day)
  {
    return(1);
  }

  return(0);
}


/*--------------------------------------------------------------------------
Calcola il numero di giorni trascorsi tra due date
INPUT:   date1,date2: date tra cui calcolare il numero dei giorni

GLOBAL:	-
RETURN:	numero dei giorni trascorsi
NOTE:	
--------------------------------------------------------------------------*/
int DiffDate(struct date d1,struct date d2)
{
	int count=0;
	
	int dir=FirstDate(d1,d2);
	//dir=0 d1-->d2
	//dir=1 d1<--d2
	
	if((d1.da_day < 1) || (d1.da_day > 31))
	{
		return -1;
	}
	
	if((d2.da_day < 1) || (d2.da_day > 31))
	{
		return -1;
	}
	
	if((d1.da_mon < 1) || (d1.da_mon > 12))
	{
		return -1;
	}
	
	if((d2.da_mon < 1) || (d2.da_mon > 12))
	{
		return -1;
	}
	
	if((d1.da_year < 1980) || (d1.da_year > 2100))
	{
		return -1;
	}
		
	if((d2.da_year < 1980) || (d2.da_year > 2100))
	{
		return -1;
	}

	while((d2.da_year!=d1.da_year) || (d2.da_mon!=d1.da_mon))
	{
		if(!dir)
		{
			count+=MonthTable[d1.da_mon-1]-d1.da_day+1;
			d1.da_mon++;
			d1.da_day=1;
			if(d1.da_mon==13)
			{
			d1.da_year++;
			d1.da_mon=1;
			}
		}
		else
		{
			count-=MonthTable[d2.da_mon-1]-d2.da_day+1;
			d2.da_mon++;
			d2.da_day=1;
			if(d2.da_mon==13)
			{
				d2.da_year++;
				d2.da_mon=1;
			}
		}
	}

	if(!dir)
	{
		count+=(d2.da_day-d1.da_day+1);
	}
	else
	{
		count-=(d1.da_day-d2.da_day+1);
	}

	return(count);
}



/*--------------------------------------------------------------------------
Conta i file all'interno di una directory con recursione
INPUT:   directory da scandire
RETURN:  numero dei file
--------------------------------------------------------------------------*/
int DirCount(const char *dir)
{
	char buf[MAXNPATH];
	strcpy(buf,dir);
	strcat(buf,"/*");

	struct ffblk f;

	int done = -1;
	int count = 0;

	// Conta i files all'interno della dir corrente
	done = findfirst( buf, &f, 0 );
	while( !done )
	{
		count++;

		done = findnext( &f );
	}

	// Trova le eveltuali sottodirectory e ritorna il loro num di files
	done = findfirst( buf, &f, FA_DIREC );

	while( !done )
	{
		if( S_ISDIR( f.ff_attrib ) )
		{
			if( *f.ff_name != '.' )
			{
				strcpy( buf, dir );
				strcat( buf, "/" );
				strcat( buf, f.ff_name );
				count += DirCount( buf );
			}
		}

		done = findnext( &f );
	}

	return( count );
}

/*--------------------------------------------------------------------------
Copia i file all'interno di una directory con recursione
INPUT:   dest: directory di destinazione
         orig: directory di origine

         progbar: puntatore a oggetto barra di progressione (opzionale)
GLOBAL:	-
RETURN:  FALSE se la copia e' fallita TRUE se terminata correttamente
--------------------------------------------------------------------------*/
int DirCopy(const char *dest,const char *orig,GUI_ProgressBar_OLD *progbar)
{
	char buf[MAXNPATH];
	strcpy(buf,orig);
	strcat(buf,"/*");
	char tmp_o[MAXNPATH];
	char tmp_d[MAXNPATH];

	struct ffblk f;

	int done = -1;

	// Copia i files all'interno della dir corrente
	done = findfirst( buf, &f, 0 );
	while( !done )
	{
		strcpy( tmp_o, orig );
		strcat( tmp_o, "/" );
		strcat( tmp_o, f.ff_name );

		strcpy( tmp_d, dest );
		strcat( tmp_d, "/" );
		strcat( tmp_d, f.ff_name );

		if( !CopyFileOLD( tmp_d, tmp_o ) )
		{
			#ifdef __LOG_ERROR
				BackupLogger.Log( "%s", tmp_o );
			#endif
			//return 0;
		}

		if( progbar != NULL )
		{
			progbar->Increment(1);
		}

		done = findnext( &f );
	}

	// Trova le eveltuali sottodirectory e copia i loro files
	done = findfirst( buf, &f, FA_DIREC );

	while( !done )
	{
		strcpy( tmp_o, orig );
		strcat( tmp_o, "/" );
		strcat( tmp_o, f.ff_name );

		strcpy( tmp_d, dest );
		strcat( tmp_d, "/" );
		strcat( tmp_d, f.ff_name );

		if( S_ISDIR( f.ff_attrib ) )
		{
			if( *f.ff_name != '.' )
			{
				if( !DirCopy( tmp_d, tmp_o, progbar ) )
				{
					return 0;
				}
			}
		}

		done = findnext( &f );
	}

	return 1;
}


/*--------------------------------------------------------------------------
Controlla che il mount sia presente
INPUT:	mnt : pathname del mount
GLOBAL:	-
RETURN:	true se presente, false altrimenti
NOTE:	
--------------------------------------------------------------------------*/
bool is_mount_ready(const char* mnt)
{
	FILE* f=setmntent("/etc/mtab","r");
	
	if(f == NULL)
	{
		return false;
	}
	
	struct mntent* m;
	bool ret = false;

	while( (m = getmntent(f)) )
	{
		struct stat st;
		if(stat(m->mnt_dir,&st) == 0)
		{
			if(!strcmp(m->mnt_dir,mnt))
			{
				ret = true;
				break;					
			}
		}
	}
	
	endmntent(f);
	
	return(ret);
}


int getAllUsbMountPoints(vector<string>& m)
{
	m.clear();
	
	FILE* f=setmntent("/etc/mtab","r");	
	
	if(f==NULL)
	{
		return(0);
	}
	
	struct mntent *mnt;
	
	while( (mnt = getmntent(f)) )
	{
		struct stat st;
		if(stat(mnt->mnt_dir,&st) == 0)
		{
			if(!strncmp(mnt->mnt_fsname,DISK_DEV_PREFIX,strlen(DISK_DEV_PREFIX)) && !strncmp(mnt->mnt_dir,USB_MOUNT_PREFIX,strlen(USB_MOUNT_PREFIX)))
			{
				char buf[MAXNPATH];
				strncpyQ(buf,mnt->mnt_fsname,strlen(DISK_DEV_PREFIX)+1);
				if(isDeviceRemovable(buf))
				{								
					m.push_back(mnt->mnt_dir);				
				}
			}
		}
	}
	
	endmntent(f);
	
	return(m.size());
}

const char* getMountPointName(const char* mnt)
{
	const char* ptr = strstr(mnt,USB_MOUNT_PREFIX "/");
	if((ptr!=NULL) && (strlen(ptr+1)))
	{
		return(ptr+strlen(USB_MOUNT_PREFIX)+1);
	}
	else
	{
		return(mnt);
	}
}


//---------------------------------------------------------------------------
// Controlla se la cartella condivisa di rete e' montata oppure no
//---------------------------------------------------------------------------
bool IsSharedDirMounted()
{
#ifndef __NET_TEST
	FILE* f = setmntent("/etc/mtab","r");
	if( !f )
	{
		return false;
	}

	bool ret = false;

	struct mntent* mnt;
	while( (mnt = getmntent(f)) )
	{
		if( strcmp( mnt->mnt_dir, REMOTEDISK ) == 0 )
		{
			ret = true;
			break;
		}
	}

	endmntent(f);

	return ret;
#else
	return true;
#endif
}



/*--------------------------------------------------------------------------
Ritorna il nomefile completo di path ed estensione per programmi di montaggio
GLOBAL:	QHeader
RETURN:	Ritorna in D_File_Name il nome del file
NOTE:	D_File_Name deve essere gia' allocata
		Aggiunto tipo per file dati legati al programma L0709
--------------------------------------------------------------------------*/
void PrgPath(char *D_File_Name, const char *S_File_Name,int tipo)
{
	char* buffer = D_File_Name;

	strcpy(buffer,CLIDIR);
	strcat(buffer,"/");
	strcat(buffer,QHeader.Cli_Default);
	strcat(buffer,"/");
	strcat(buffer,PRGDIR);
	strcat(buffer,"/");
	strcat(buffer,S_File_Name);
	
	switch (tipo)
	{
		case PRG_NORMAL:
			strcat(buffer,PRGEXT);
			break;
		case PRG_DATA:
			strcat(buffer,DTAEXT);
			break;
		case PRG_ASSEMBLY:
			strcat(buffer,ASSEXT);
			break;
		case PRG_DOSAT:
			strcat(buffer,DOSATEXT);
			break;
		case PRG_ZER:
			strcat(buffer,ZEREXT);
			break;
		case PRG_ZERDATA:
			strcat(buffer,ZERDAT_EXT);
			break;
		case PRG_ZER_MC:
			strcat(buffer,ZERMC_EXT);
			break;
		case PRG_LASTOPT:
			strcat(buffer,LASTOPT_EXT);
			break;
	}
} // PrgPath


/*--------------------------------------------------------------------------
Ritorna il nomefile completo di path ed estensione per files caricatori
INPUT:	???
GLOBAL:	-
RETURN:	Ritorna in D_File_Name il nome del file
NOTE:	D_File_Name deve essere gia' allocata
--------------------------------------------------------------------------*/
void CarPath(char *D_File_Name, const char *S_File_Name)
{
	char *buffer=D_File_Name;
	
	if(!Get_UseCommonFeederDir()) //SMOD180208
	{
		strcpy(buffer,CLIDIR);
		strcat(buffer,"/");
		strcat(buffer,QHeader.Cli_Default);
		strcat(buffer,"/");
	}
	else
	{
		*buffer='\0';
	}

	strcat(buffer,CARDIR);
	strcat(buffer,"/");
	strcat(buffer,S_File_Name);
	strcat(buffer,CAREXT);
} // CarPath


/*--------------------------------------------------------------------------
Ritorna il nomefile completo di path ed estensione per files packages (dati dosatore)
INPUT:	???
GLOBAL:	-
RETURN:	Ritorna in D_File_Name il nome del file
NOTE:	D_File_Name deve essere gia' allocata
--------------------------------------------------------------------------*/
#ifndef __DISP2
void PackDosPath(char *D_File_Name,const char *S_File_Name)
#else
void PackDosPath(int ndisp,char *D_File_Name,const char *S_File_Name)
#endif
{
	char buf[MAXNPATH];
	int i;
	
	strcpy(D_File_Name,FPACKDIR);
	strcat(D_File_Name,"/");
	
	for(i=0;i<8;i++)
	{
		if(isalnum(S_File_Name[i]) || (S_File_Name[i]=='_'))
		{
			buf[i]=S_File_Name[i];
		}
		else
		{
			break;
		}
	}
	buf[i]=0;
	
	strcat(D_File_Name,buf);
	#ifndef __DISP2
	strcat(D_File_Name,FPACKDOSEXT);
	#else
	strcat(D_File_Name,(ndisp==1) ? FPACKDOSEXT1 : FPACKDOSEXT);
	#endif
}


//--------------------------------------------------------------------------
// Emette un beep singolo a 2KHz
//--------------------------------------------------------------------------
void bip( int type )
{
	switch( type )
	{
	case BEEPLONG:
		sound( 2000, 100 );
		break;
	case BEEPSHORT:
		sound( 2000, 10 );
		break;
	}
};

//--------------------------------------------------------------------------
// Emette un suono bitonale di avvertimento
//--------------------------------------------------------------------------
void bipbip()
{
	sound( 500, 50 );
	sound( 2000, 50 );
}

//--------------------------------------------------------------------------
// Emette uno squillo di avviso
//--------------------------------------------------------------------------
void beep()
{
	for( int i = 0; i < 5; i++ )
	{
		sound( 500, 60 );
		sound( 1000, 60 );
	}
}


//--------------------------------------------------------------------------
// Attende pressione tasto e converte in codice esteso
//--------------------------------------------------------------------------
int Handle( bool wait )
{
	SetConfirmRequiredBeforeNextXYMovement(true);

	if( Is_FinecEnabled() )
	{
		//FinecStatus();
	}

	int key = 0;
	while( 1 )
	{
		int _key = keyRead();

		if( !wait && _key == 0 )
		{
			return 0;
		}

		if( _key )
		{
			key = _key;
			break;
		}
	}

	switch( key )
	{
		case K_CTRL_F1:
			#ifdef __SNIPER
			GetSniperImageShortCut();
			#endif
			break;

		case K_CTRL_F2:
			#ifdef __SNIPER
			GetSniperFramesShortCut();
			#endif
			break;

		case K_CTRL_F3:
			#ifdef __SNIPER
			DoSniperScanTestShortCut();
			#endif
			break;

		case K_CTRL_F4:
			DoZCheckStepLossShortCut();
			break;

		case K_CTRL_F5:
			DoZCheckShortCut();
			break;

		case K_CTRL_F6:
			//Snipers_Enable();
			#ifdef __LOG_ERROR
			ShowLogger();
			#endif
			break;

		#ifdef __USE_MOTORHEAD
		case K_CTRL_F7:
			Motorhead_ShowLog( HEADX_ID );
			break;
		case K_CTRL_F8:
			Motorhead_ShowLog( HEADY_ID );
			break;
		#endif

		//GF_TEMP
		#ifdef __USE_MOTORHEAD
		case K_CTRL_F9:
			Motorhead_ShowStatus();
			//Motorhead_ReadPIDs();
			break;
		case K_CTRL_F10:
			//Motorhead_ActualPosition();
			//Motorhead_ShowPIDStatus();
			if( 0 )
			{
				Motorhead->ReloadParameters();
				W_Mess( "Motors params reloaded !" );
			}
			else
			{
				Motorhead->ReloadPIDParams();
				W_Mess( "PID params reloaded !" );
			}
			break;
		case K_CTRL_F11:
			Motorhead_ActualEncoder();
			break;
		#endif

		case K_CTRL_F12:
			make_slide();
			break;

		#ifdef __SNIPER
		case K_ALT_F1:
			GetSniperImageDetailedShortCut();
			break;
		#endif
		
		case K_ALT_F8:
			ResetAssemblyReport();
			break;

		case K_ALT_F9:
			ShowAssemblyReport();
			break;
			
		#ifdef __DISP2
		case K_ALT_F5:
			if(QParam.Dispenser)
			{
				Dosatore->DoVacuoFinalPulse(1);
			}
			break;
		case K_ALT_F6:
			if(QParam.Dispenser)
			{
				Dosatore->DoVacuoFinalPulse(2);
			}
			break;
		#else
		#endif
	}

	return key;
}


/*--------------------------------------------------------------------------
Verifica la legalita' di un carattere all'interno di un set di caratteri legali
INPUT:	legal: Set di caratteri legali
		c: Carattere da testare
GLOBAL:	-
RETURN:	Ritorna 1 se il carattere appartiene all'insieme di caratteri legali,
		0 altrimenti
NOTE:	
--------------------------------------------------------------------------*/
//TODO: VERIFICARE
int ischar_legal(const char *legal,int c)
{
	if( legal[0] != 0 && (strchr(legal, c) == NULL) )
		return(0);
	else if((c>=' ') && (c<='~'))
		return(1);
	else
		return(0);
}


/*--------------------------------------------------------------------------
Centra la stringa nella lunghezza passata
INPUT:	s: Stringa da centrare
		len: Lunghezza su cui centrare la stringa
GLOBAL:	-
RETURN:	Ritorna la stringa modificata
NOTE:	L140999
--------------------------------------------------------------------------*/
void Centre_string(char* s, int len)
{
	if( strlen(s) < len )
	{
		int p1 = (len-strlen(s))/2;
		int p2 = p1+strlen(s);

		for(int i=p2; i>=p1; i--)
			s[i]=s[i-p1];

		for(int i=0; i<p1; i++)
			s[i]=' ';

		for(int i=p2; i<len; i++)
			s[i]=' ';
	}

	s[len] = 0;
}


//--------------------------------------------------------------------------
// Testa la pressione del tasto ESC
//--------------------------------------------------------------------------
bool Esc_Press()
{
	return (Handle( false ) == K_ESC) ? true : false;
}


/*--------------------------------------------------------------------------
Inserisce spazi fino a riempire la stringa di lunghezza len
INPUT:	len: Lunghezza della stringa
		txt: Stringa da riempire di spazi
GLOBAL:	-
RETURN:	Ritorna la stringa modificata
NOTE:	Integr. Simone >>S260701
--------------------------------------------------------------------------*/
//TODO: DA RIVEDERE
void InsSpc(int len,char *txt)
{
	int i;
	
	for(i=0;i<len;i++)
	{ 
		*txt=' ';
		txt++;
	}
	
	*txt='\0';
}

//TODO: DA RIVEDERE
void InsSpc(int len,std::string& txt,int pos)
{
	std::string tmp = "";
	
	for(int i=0;i<len;i++)
	{ 	
		tmp+=' ';
	}	
			
	if((pos!=-1) && (pos <= txt.length()))
	{
		if(pos < txt.length())
		{
			txt = txt.substr(0,pos) + tmp;
		}
		else
		{
			txt+=tmp;
		}
	}	
	else
	{
		txt = tmp;
	}
}


/*--------------------------------------------------------------------------
Pad di una stringa con carattere di riempimento specificato
INPUT:	str: stringa su cui eseguire il pad
         n  : numero massimo di caratteri
         ch : carattere di riempimento
GLOBAL:	-
RETURN:	Ritorna la stringa modificata
NOTE:	   Integr. Simone >>S260603
--------------------------------------------------------------------------*/
//TODO: DA RIVEDERE
void Pad(char *str,int n,int ch)
{
  if(strlen(str)<n)
  {
    for(int i=strlen(str);i<n;i++)
      str[i]=' ';
  }
  str[n]='\0';
}




/*--------------------------------------------------------------------------
Allinea la stringa di lunghezza len a destra
INPUT:	len: Lunghezza della stringa
		txt: Stringa da allineare
GLOBAL:	-
RETURN:	Ritorna la stringa modificata
NOTE:	Integr. Simone >>S260701
--------------------------------------------------------------------------*/
//TODO: DA RIVEDERE
void AllignR(int len,char *txt)
{
	int i;
	char *tmpTxt=new char[len+1];
	char *startTxt=tmpTxt;
	int l=strlen(txt);
	
	for(i=0;i<len-l;i++)
	{
		*tmpTxt=' ';
		tmpTxt++;
	}
	
	strcpy(tmpTxt,txt);
	strcpy(txt,startTxt);
	
	delete [] startTxt;
}


/*--------------------------------------------------------------------------
Ritorna il numero di record del caricatore selezionato
INPUT:	ck_code: Codice del caricatore (>11)
GLOBAL:	-
RETURN:	Ritorna -1 se codice caricatore non valido, numero del record altrimenti
NOTE:	
--------------------------------------------------------------------------*/
int GetCarRec(int ck_code) 
{
	int decina=(ck_code/10)*10;
	int unita =ck_code-decina;
	int n_record = ((decina/10-1)*8)+(unita-1);    // calcolo n. record
	
	if(ck_code<11)
		return(-1);
	
	if (ck_code>158) 
		n_record=120+(decina/10-16);  // cod. vassoi
	
	return(n_record);
}// GetCarRec


//---------------------------------------------------------------------------------
// Converte da gradi a step (per la punta specificata)
//---------------------------------------------------------------------------------
int Deg2Step( float deg, int nozzle )
{
	int enc_const;

	if( nozzle == 2 )
		enc_const = QHeader.Enc_step2;
	else
		enc_const = QHeader.Enc_step1;

	return int(deg * enc_const / 360);
}


//---------------------------------------------------------------------------------
// Converte da gradi a step (per la punta specificata)
//---------------------------------------------------------------------------------
float Step2Deg( int step, int nozzle )
{
	int enc_const;

	if( nozzle == 2 )
		enc_const = QHeader.Enc_step2;
	else
		enc_const = QHeader.Enc_step1;

	return step * 360.f / enc_const;
}


/*---------------------------------------------------------------------------------
Converte un angolo in base alla convenzione caricatori
Parametri di ingresso:
   ncaric: codice del caricatore
   angle : angolo da convertire
Valori di ritorno:
   angolo convertito
---------------------------------------------------------------------------------*/
float angleconv(int ncaric,float angle)
{
	int dtheta = 0;
	int nrec = GetCarRec(ncaric);

	if( nrec < 56 && nrec > 23 )
		dtheta = 90;
	if( nrec < 88 && nrec > 55 )
		dtheta = 180;
	if( nrec < 120 && nrec > 87 )
		dtheta = -90;

	angle += dtheta;

	while( angle >= 360 )
		angle -= 360;

	return angle;
}

/*--------------------------------------------------------------------------
Controlla la validita del codice caricatore
INPUT:   code: codice da controllare
RETURN:	Ritorna 1 se ok, 0 altrimenti
--------------------------------------------------------------------------*/
int IsValidCarCode(int code)
{
	if( GetCarRec(code) < MAXCAR && GetCarRec(code) >= 0 )
	{
		return 1;
	}

	return 0;
}

/*--------------------------------------------------------------------------
Ritorna il codice del caricatore a partire dal numero del record
INPUT:	rec_num: Numero del record
RETURN:	Ritorna -1 se il numero del record e' <0, codice del caricatore altrimenti
--------------------------------------------------------------------------*/
int GetCarCode( int rec_num )
{
	if( rec_num < 0 )
	{
		return -1;
	}

	int car_code = 0;
	if( rec_num >= MAXNREC_FEED )
	{
		car_code = FIRSTTRAY+(rec_num-MAXNREC_FEED)*10; // cod. vassoi
	}
	else
	{
		car_code = 10*((rec_num >> 3)+1)+((rec_num % 8)+1);
	}
	return car_code;
}


/*--------------------------------------------------------------------------
Converte un unsigned int in un esadecimale
INPUT:d_num    : Numero da convertire
		n_bytes  : Numero di bytes che compongono l'esadecimale (1..4)
      endstring: 1=inserisce carattere di finestringa in fondo alla stringa
                   contenente il numero convertito
                 0=non inserire il carattere di fine stringa
GLOBAL:	-
RETURN:	Ritorna in ex_num il unmero convertito
NOTE:	
--------------------------------------------------------------------------*/
//##SMOD060902
void Val2Hex(unsigned int d_num, int n_bytes, char *ex_num,int endstring)
{
  char *stringa=ex_num;
  unsigned int val;
  
  n_bytes=n_bytes*2;
  while(n_bytes>0)
  { val=(d_num >> (n_bytes-1)*4);
    val=val & 0x0F;

    if((val>=0) && (val<=9))
      *stringa=val+'0';
    else
      if((val>=10) && (val<=15))
        *stringa=val-10+'A';

    n_bytes--;
    stringa++;
  }

  if(endstring)
    *stringa=0;
}


/*--------------------------------------------------------------------------
Converte una stringa esadecimale in intero
INPUT:	num: stringa di testo contenente il numero esadecimale da convertire
GLOBAL:	-
RETURN:	Ritorna il numero convertito se la cifra di ingresso e' legale, 0
		altrimenti
NOTE:	
--------------------------------------------------------------------------*/
unsigned int Hex2Val( char* hexStr )
{
	unsigned int num = 0;

	for( int i = strlen(hexStr); i--; )
	{
		if( !isxdigit(*hexStr) )
		{
			return 0;
		}

		num += Hex2Dec(*hexStr);

		hexStr++;
	}

	return num;
}


/*--------------------------------------------------------------------------
Elevazione a potenza di interi
INPUT:	x: Base
		y: Esponente
GLOBAL:	-
RETURN:	Ritorna il numero x^y
NOTE:	
--------------------------------------------------------------------------*/
int Potenza(int x,int y)
{
	int i;
	int tmp=1;
	
	for(i=0;i<y;i++)
		tmp=tmp*x;
	
	return(tmp);
}


/*--------------------------------------------------------------------------
Ordina un vettore
INPUT:   recset  : puntatore al vettore
         type    : tipo
         nrec    : numero di record presenti nel vettore
         offset  : offset da inizio record del campo che si vuole ordinare
         dim     : dimensione di un record nel vettore
         showwait: 1 (default) mostra maschera di attesa
                   0           non mostrare maschera di attesa
GLOBAL:	-
RETURN:	-
NOTE:	   -
--------------------------------------------------------------------------*/
void SortData( void *recset,int type,int nrec,int offset,int dim,int showwait/*=1*/ )
{
  int swap=1,doswap=0,i,k;
  char *tmp=(char *)malloc(dim);

  CPan* wait = 0;

  if(showwait)
  {
    wait=new CPan(-1,1,MsgGetString(Msg_00332));
  }

  for(i=0;swap && i<nrec-1;i++)
  {
    swap=0;

    char *ptr=(char *)recset;
    char *ptr_1;
    char *ptr_2;
    
    for(k=0;k<nrec-i-1;k++)
    {
      ptr_1=ptr+k*dim+offset;
      ptr_2=ptr+(k+1)*dim+offset;

      switch(type)
      {
        case SORTFIELDTYPE_INT16:
          if(type & SORTFIELDTYPE_UNSIGNED)
          {
            if(*(unsigned short int *)ptr_2<*(unsigned short int *)ptr_1)
              doswap=1;
          }
          else
          {
            if(*(short int *)ptr_2<*(short int *)ptr_1)
              doswap=1;
          }
          break;

        case SORTFIELDTYPE_INT32:
          if(type & SORTFIELDTYPE_UNSIGNED)
          {
            if(*(unsigned int *)ptr_2<*(unsigned int *)ptr_1)
              doswap=1;
          }
          else
          {
            if(*(int *)ptr_2<*(int *)ptr_1)
              doswap=1;
          }
          break;

        case SORTFIELDTYPE_CHAR:
          if(type & SORTFIELDTYPE_UNSIGNED)
          {
            if(*(unsigned char *)ptr_2<*(unsigned char *)ptr_1)
              doswap=1;
          }
          else
          {
            if(*ptr_2<*ptr_1)
              doswap=1;
          }
          break;                    

        case SORTFIELDTYPE_FLOAT32:
          if(*(float *)ptr_2<*(float *)ptr_1)
              doswap=1;
          break;

        case SORTFIELDTYPE_STRING:
          if(strcmp(ptr_1,ptr_2)>0)
            doswap=1;
          break;
      }

      if(doswap)
      {
        swap=1;
        doswap=0;

        memcpy(tmp,ptr+k*dim,dim);
        memcpy(ptr+k*dim,ptr+(k+1)*dim,dim);
        memcpy(ptr+(k+1)*dim,tmp,dim);
      }
    }
  }

  if(showwait)
  {
    delete wait;
  }
    
  free(tmp);
}

// Input dei dati di ripartenza - DANY141102
//===============================================================================
int Set_RipData( unsigned int& ripcomp, int numerorecs, const char* title )
{
	if( numerorecs < 0 )
	{
		W_Mess( MsgGetString(Msg_00227) );
		return 0;
	}

	if( ripcomp >= numerorecs )
		ripcomp = 0;

	CInputBox inbox( 0, 6, title, MsgGetString(Msg_00226), 5, CELL_TYPE_UINT );
	inbox.SetVMinMax( 1, numerorecs );
	inbox.SetText( int(ripcomp+1) );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	ripcomp = inbox.GetInt() - 1;
	return 1;
}

//rimuove tutti i riferimenti a directory da un pathname indicato
//(ritorna solo il nomefile contenuto nel pathname)
//SMOD280403
char *RemovePathString(char *path)
{
  char *ptr;
  char *ptrprev=NULL;

  ptr=path;

  ptr=strchr(ptr,':');
  if(ptr!=NULL)
  {
    ptr++;
  }

  if(ptr==NULL)
  {
    ptr=path;
  }

  while(ptr!=NULL)
  {
    ptrprev=ptr;
    ptr=strchr(ptr,'/');
    if(ptr!=NULL)
      ptr++;
  }

  //trovato almeno uno slash
  if(ptrprev!=NULL)
  {
    return(ptrprev); //ritorna sottostringa dopo ultimo slash trovato
  }
  else
  {
    return(path);      //il path contiene solo il nome file
  }
}


//Controlla esistenza di una directory con possibilita di crearla nel caso non esista
int CheckDir( const char* dir, int mode )
{
	if( !CheckDirectory(dir) )
	{
		if( mode == CHECKDIR_CREATE )
		{
			return(!mkdir(dir,DIR_CREATION_FLAG));
		}
		else
		{
			return 0;
		}
	}

	return 1;
}


//SMOD230703-bridge
void CheckBackupDir( char* cust )
{
	char buf[MAXNPATH];

	if( cust != NULL )
	{
		strcpy(buf,CLIDIR);
		strcat(buf,"/");
		strcat(buf,cust);
		strcat(buf,"/");
		strcat(buf,BACKDIR);
	}
	else
	{
		strcpy(buf,BACKDIR);
	}

	CheckDir( buf, CHECKDIR_CREATE );
}

//dati tre punti ritorna 1 se sono posizionati in senso antiorario
//-1 se in senso orario
int PointCCW(float x1,float y1,float x2,float y2,float x3,float y3)
{
  float dx1,dx2;
  float dy1,dy2;

  dx1=x2-x1;
  dx2=x3-x1;

  dy1=y2-y1;
  dy2=y3-y1;

  if((dx1==0) && (dy2==0))
  {
    return((dx2*dy1<0) ? 1 : -1);
  }

  if((dx2==0) && (dy1==0))
  {
    return((dx1*dy2>0) ? 1 : -1);
  }

  return((dx1*dy2>dy1*dx2) ? 1 : -1);
}

//dati due segmenti ritorna 1 se si intersecano, 0 altrimenti
int SegmentIntersect(float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4)
{

  //i segmenti sono allineati in y
  if((y1==y2) && (y2==y3))
  {
    float x0_min,x0_max,x1_min,x1_max;

    if(x1<x2)
    {
      x0_min=x1;
      x0_max=x2;
    }
    else
    {
      x0_min=x2;
      x0_max=x1;
    }

    if(x3<x4)
    {
      x1_min=x3;
      x1_max=x4;
    }
    else
    {
      x1_min=x4;
      x1_max=x3;
    }

    if(x0_max==x1_max)
      return(1);

    if(((x0_max>=x1_min) && (x1_max>x0_max)) || ((x1_max>=x0_min) && (x1_max<x0_max)))
      return(1);

  }

  //i segmenti sono allineati in x
  if((x1==x2) && (x2==x3))
  {
    float y0_min,y0_max,y1_min,y1_max;

    if(y1<y2)
    {
      y0_min=y1;
      y0_max=y2;
    }
    else
    {
      y0_min=y2;
      y0_max=y1;
    }

    if(y3<y4)
    {
      y1_min=y3;
      y1_max=y4;
    }
    else
    {
      y1_min=y4;
      y1_max=y3;
    }


    if(y0_max==y1_max)
      return(1);

    if(((y0_max>=y1_min) && (y1_max>y0_max)) || ((y1_max>=y0_min) && (y1_max<y0_max)))
      return(1);
  }

  return ((PointCCW(x1,y1,x2,y2,x3,y3)*PointCCW(x1,y1,x2,y2,x4,y4)<=0) &&
         (PointCCW(x3,y3,x4,y4,x1,y1)*PointCCW(x3,y3,x4,y4,x2,y2)<=0));
}


//---------------------------------------------------------------------------
// Funzionalita' copia/incolla
//---------------------------------------------------------------------------
string __clipText;
int __clipType = -1;

void copyTextToClipboard( const char* txt, int type )
{
	__clipText = txt;
	__clipType = type;
	printf( "[%d] %s\n", __clipType, __clipText.c_str() );
}

const char* pasteTextFromClipboard()
{
	return __clipText.c_str();
}

int getClipboardTextType()
{
	return __clipType;
}






float CalcMoveTime(float s,float vmin,float vmax,float acc)
{
	float slimit=(vmax*vmax-vmin*vmin)/acc;
	
	if(s>slimit)
	{
		float t=(2*(vmax-vmin)/acc); //ramp-up & ramp-down
		t+=(s-slimit)/vmax;          //constant speed
		
		return(t);
	}
	else
	{
		vmax=sqrt(s*acc+vmin*vmin);
		
		return(2*(vmax-vmin)/acc);
	}
}

float CalcSpaceInTime(float t,float smax,float vmin,float vmax,float acc)
{
  float tmax=CalcMoveTime(smax,vmin,vmax,acc);

  if(t>=tmax)
  {
    return(smax);
  }

  float slimit=(vmax*vmax-vmin*vmin)/acc;

  if(smax<slimit)
  {
    if(t<tmax/2)
    {
      return((vmin+acc*t/2)*t);
    }
    else
    {
      float v1=vmin+acc*(tmax-t);
      float v2=vmin+(acc*tmax/2);

      float s=(vmin+acc*tmax/4)*tmax/2;

      return(s+((v1+v2)*(t-tmax/2)/2));
    }
  }
  else
  {
    float t_ramp=(vmax-vmin)/acc;

    if(t<=t_ramp)
    {
      return((vmin+acc*t/2)*t);
    }
    else
    {
      if(t<=(tmax-t_ramp))
      {
        float s=(vmin+acc*t_ramp/2)*t_ramp;
        s+=(t-t_ramp)*vmax;
        return(s);
      }
      else
      {
        float s=(vmin+acc*t_ramp/2)*t_ramp;
        s+=(tmax-2*t_ramp)*vmax;

        float v=vmin+acc*(tmax-t);
        return(s+((v+vmax)*(t-(tmax-t_ramp))/2));

      }
    }
  }

  return(smax);
}


//flag per /sys/block/xxx/capability
#define GENHD_FL_CD			0x08

bool isDeviceRemovable(char* dev)
{
	char path[MAXNPATH];
	char buf[8];
	int data;
	FILE* f;
	
	char* d = strstr(dev,"/dev/");
	if(d == NULL)
	{
		return false;
	}
	
	strcpy(path,"/sys/block/");
	strcat(path,d+strlen("/dev/"));
	strcat(path,"/removable");
	f = fopen(path,"r");
	if(f == NULL)
	{
		return false;
	}
	fgets(buf,2,f);
	sscanf(buf,"%d",&data);
	if(data == 0)
	{
		fclose(f);
		return false;
	}
	else
	{
		fclose(f);
	}
	
	strcpy(path,"/sys/block/");
	strcat(path,dev);
	strcat(path,"/capability");
	f = fopen(path,"r");
	if(f == NULL)
	{
		return true;
	}
	fgets(buf,2,f);
	sscanf(buf,"%x",&data);
	if(data & GENHD_FL_CD)
	{
		//e'un CD !!
		fclose(f);
		return false;
	}
	else
	{
		fclose(f);
		return true;
	}
}

int accessQ(const char* path, int type,bool ignore_case)
{
	if(!ignore_case)
	{
		return(access(path,type));
	}
	else
	{
		struct ffblk ffblk;
		if(!findfirst(path,&ffblk,FA_IGNORE_CASE))
		{
			end_findfirst(&ffblk);
			return(0);
		}
		else
		{
			end_findfirst(&ffblk);
			return 1;
		}
		
		/*
		char buf[MAXNPATH+1];
		strncpyQ(buf,path,MAXNPATH);
		char *ptr = strchr(buf,'.');
		if(ptr == NULL)
		{
			return(access(path,type));
		}
		else
		{
			if(!access(buf,type))
			{
				return 0;
			}
			else
			{
				ptr++;
				strupr(ptr);
				return(access(buf,type));
			}
		}
		*/
	}
}



//---------------------------------------------------------------------------
// finestra: Tool selection
//---------------------------------------------------------------------------
#include "q_ugeobj.h"
extern UgelliClass* Ugelli;

#include "gui_desktop.h"
extern GUI_DeskTop* guiDeskTop;

class ToolSelectionUI : public CWindow
{
public:
	ToolSelectionUI( CWindow* parent, char tool, int nozzle, bool askSwitchNozzle = false ) : CWindow( parent )
	{
		usedNozzle = nozzle;
		selectedTool = tool;
		enableSwitchNozzle = askSwitchNozzle;

		m_comboList = new CComboList( this );

		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 45, 5 );

		char buf[160];
		snprintf( buf, 160, "%s - %s", MsgGetString(Msg_01428), usedNozzle ? MsgGetString(Msg_00042) : MsgGetString(Msg_00043) );
		SetTitle( buf );
	}

	~ToolSelectionUI()
	{
		for( unsigned int i = 0; i < m_combos.size(); i++ )
		{
			if( m_combos[i] )
			{
				delete m_combos[i];
			}
		}

		if( m_comboList )
		{
			delete m_comboList;
		}
	}

	int Show()
	{
		// init classe
		onInit();

		// visualizza le barre di stato e "pulisce il desktop"
		guiDeskTop->ShowStatusBars( false );
		guiDeskTop->SetEditMode( true );
		m_comboList->SetEdit( 1 );

		// visualizza finestra
		CWindow::Show( true );

		// show classe
		onShow();

		// visualizza eventuali controlli
		m_comboList->Show();

		// refresh classe
		onRefresh();

		int retVal = WorkingCycle();

		// close classe
		onClose();

		CWindow::Hide();

		// nasconde le barre di stato
		guiDeskTop->HideStatusBars();

		return retVal;
	}

	char GetSelectedTool()
	{
		return selectedTool;
	}

	char GetSelectedNozzle()
	{
		return usedNozzle;
	}

	typedef enum
	{
		TOOL_CODE,
		TOOL_NOZZ
	} combo_labels;

protected:
	std::map<int,C_Combo*> m_combos;
	CComboList* m_comboList;

	void onInit()
	{
		// create combos
		m_combos[TOOL_CODE] = new C_Combo( 10, 1, MsgGetString(Msg_00261), 1, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE );
		m_combos[TOOL_NOZZ] = new C_Combo( 10, 3, MsgGetString(Msg_01006), 1, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_UPPERCASE );

		// set params
		m_combos[TOOL_CODE]->SetLegalChars( CHARSET_TOOL );
		m_combos[TOOL_NOZZ]->SetLegalChars( "12B-" );

		// add to combo list
		m_comboList->Add( m_combos[TOOL_CODE], 0, 0 );
		m_comboList->Add( m_combos[TOOL_NOZZ], 1, 0 );
	}

	void onShow()
	{
	}

	void onRefresh()
	{
		m_combos[TOOL_CODE]->SetTxt( selectedTool );

		Ugelli->ReadRec( toolData, selectedTool-'A' ); // lettura del record

		switch( toolData.NozzleAllowed )
		{
		case UG_P1:
			m_combos[TOOL_NOZZ]->SetTxt( '1' );
			break;
		case UG_P2:
			m_combos[TOOL_NOZZ]->SetTxt( '2' );
			break;
		case UG_ALL:
			m_combos[TOOL_NOZZ]->SetTxt( 'B' );
			break;
		default:
			m_combos[TOOL_NOZZ]->SetTxt( '-' );
			break;
		}
	}

	void onEdit()
	{
		selectedTool = m_combos[TOOL_CODE]->GetChar();
	}

	void onClose()
	{
	}

private:
	int WorkingCycle()
	{
		int c = 0;
		while( c != K_ESC )
		{
			SetConfirmRequiredBeforeNextXYMovement(true);

			c = Handle( false );
			if( c != 0 )
			{
				if( c == K_ENTER )
				{
					switch( toolData.NozzleAllowed )
					{
					case UG_P1:
						if( usedNozzle == 1 )
							return 1;
						else if( enableSwitchNozzle )
						{
							char buf[160];
							snprintf( buf, 160, MsgGetString(Msg_02122), 2 ); // ugello non consentito per punta 1
							strcat( buf, "\n" );
							char buf2[64];
							snprintf( buf2, 64, MsgGetString(Msg_02119), 1 );
							strcat( buf, buf2 );

							if( W_Deci( 1, buf ) )
							{
								usedNozzle = 1;
								return 1;
							}
						}
						break;

					case UG_P2:
						if( usedNozzle == 2 )
							return 1;
						else if( enableSwitchNozzle )
						{
							char buf[160];
							snprintf( buf, 160, MsgGetString(Msg_02122), 1 ); // ugello non consentito per punta 2
							strcat( buf, "\n" );
							char buf2[64];
							snprintf( buf2, 64, MsgGetString(Msg_02119), 2 );
							strcat( buf, buf2 );

							if( W_Deci( 1, buf ) )
							{
								usedNozzle = 2;
								return 1;
							}
						}
						break;

					case UG_ALL:
						return 1;

					default:
						// no nozzle allowed
						break;
					}
				}

				if( m_comboList->ManageKey( c ) )
				{
					// edit classi derivate
					onEdit();
					// refresh classi derivate
					onRefresh();
				}
			}
		}

		return 0;
	}

	CfgUgelli toolData;
	char selectedTool;
	int usedNozzle;
	bool enableSwitchNozzle;
};


//---------------------------------------------------------------------------
// Finestra selezione ugello
// Ritorna: 1 se ugello selezionato, 0 altrimenti
//---------------------------------------------------------------------------
int ToolSelectionWin( CWindow* parent, char& tool, int nozzle )
{
	ToolSelectionUI win( parent, tool, nozzle, false );
	int retVal = win.Show();

	if( retVal == 1 )
	{
		tool = win.GetSelectedTool();
	}

	return retVal;
}

//---------------------------------------------------------------------------
// Finestra selezione ugello con richiesta di cambio punta se ugello scelto
//   non supporta la punta iniziale
// Ritorna: 1 se ugello selezionato, 0 altrimenti
//---------------------------------------------------------------------------
int ToolNozzleSelectionWin( CWindow* parent, char& tool, int& nozzle )
{
	ToolSelectionUI win( parent, tool, nozzle, true );
	int retVal = win.Show();

	if( retVal == 1 )
	{
		tool = win.GetSelectedTool();
		nozzle = win.GetSelectedNozzle();
	}

	return retVal;
}


//---------------------------------------------------------------------------
// Stampa dati sulle barre di stato
//---------------------------------------------------------------------------
void ShowCurrentData()
{
	guiDeskTop->SetSB_Top( QHeader.Cli_Default, QHeader.Prg_Default, QHeader.Conf_Default, QHeader.Lib_Default );

	if( QParam.Dispenser )
	{
		char buf[9];

		#ifndef __DISP2
		if(Dosatore->IsConfLoaded(1))
		{
			Dosatore->GetConfigName(1,buf);
		}
		else
		{
			*buf='\0';
		}

		guiDeskTop->SetSB_Bottom( 1, buf );
		#else
		for(int i=1;i<=2;i++)
		{
			if(Dosatore->IsConfLoaded(i))
			{
				Dosatore->GetConfigName(i,buf);
			}
			else
			{
				*buf='\0';
			}

			guiDeskTop->SetSB_Bottom( i, buf );
		}
		#endif
	}
	else
	{
		guiDeskTop->SetSB_Bottom( 1, "" );
		#ifdef __DISP2
		guiDeskTop->SetSB_Bottom( 2, "" );
		#endif
	}
}

const char* getMotorheadComPort()
{
	if( Get_MotorheadOnUart() )
		return UARTPCI_COM4;
	else
		return USB_COM0;
}

const char* getStepperAuxComPort()
{
	if( Get_MotorheadOnUart() )
		return USB_COM0;
	else
		return UARTPCI_COM4;
}

char* trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}
