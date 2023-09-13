/*
>>>> Q_FILES.CPP

Gestione delle operazioni sui programmi.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++  	Modificato da TWS Simone 06.08.96 
++++      Modificato da Walter ottobre 1997 - mark: W09
++++      Fissato bug allocazione arrays dei nomi - 27.06.98 - W98
++++      Modificato per interf. grafica avanzata - W042000

*/

#include <string>
#include <algorithm>

using namespace std;

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "c_win_select.h"
#include "c_pan.h"

#include "filefn.h"
#include "fileutils.h"
#include "msglist.h"
#include "q_cost.h"
#include "q_gener.h"
#include "q_help.h"
#include "q_tabe.h"
#include "q_wind.h"
#include "q_assem.h"
#include "q_files.h"
#include "q_filest.h"
#include "q_prog.h"
#include "q_conf.h"

#include <unistd.h>  //GNU
#include <sys/stat.h> //GNU

#include "q_carobj.h"
#include "q_tabe.h"
#include "q_dosat.h"

#include "q_zip.h"
#include "q_net.h"

#include "q_init.h"

#include "strutils.h"
#include "fileutils.h"

#include <mss.h>

#ifdef __LOG_ERROR
#include "q_inifile.h"
#include "q_logger.h"
extern CLogger BackupLogger;
#endif

extern struct CfgParam  QParam;     // struct memo configuraz.
extern FeederFile* CarFile;
extern struct CfgHeader QHeader;      // Struct per memo configurazione


/*--------------------------------------------------------------------------
Cancella i file presenti in una cartella
INPUT:   FNome  : nome cartella
         filesel: nome del file da eliminare (senza estensione). Sono
                  permessi i caratteri jolly.
         ext    : estensione dei file da eliminare. Sono permessi i caratteri
                  jolly.
GLOBAL:	-
RETURN:  -
NOTE:	   -
--------------------------------------------------------------------------*/
void DeleteFiles_OLD(const char *FNome,const char *filesel="*",const char *ext="*") {
   char Buff[MAXNPATH], Nome[MAXNPATH];
   struct ffblk ffblk;
   int done;

   strcpy(Buff,FNome);
   strcat(Buff,"/");
   strcat(Buff,filesel);
   DelSpcR(Buff);
   strcat(Buff,".");
   strcat(Buff,ext);

   done = findfirst(Buff,&ffblk,0);
   while (!done)
   { strcpy(Nome,FNome);
     strtok(Nome," ");
     strcat(Nome,"/");
     strcat(Nome, ffblk.ff_name);
     remove(Nome);
     done = findnext(&ffblk);
   }

}// DeleteFile

GUI_ProgressBar_OLD *CopyDir_Progbar=NULL;
CWindow        *CopyDir_wait=NULL;

/*--------------------------------------------------------------------------
Copia una cartella in un'altra (comprese le sotto directory)
INPUT:   dir_dest: cartella di destinazione
         dir_orig: cartella di origine
GLOBAL:	-
RETURN:  0 se procedura fallita 1 altrimenti
NOTE:	   -
--------------------------------------------------------------------------*/
int CopyDir(char *dir_dest,char *dir_orig)
{
  char Buff_o[MAXNPATH],Buff_d[MAXNPATH];
  struct ffblk ffblk;
  int done;

  int ImFirst=0;

  if(!CheckDirectory(dir_orig))
  {
    return(0);
  }

  if(!CheckDirectory(dir_dest))
  {
    if(mkdir(dir_dest,DIR_CREATION_FLAG))
    {
      return(0);
    }
  }

  strcpy(Buff_o,dir_orig);

  if(CopyDir_Progbar==NULL)
  {
    int nfiles=CountDirFiles(Buff_o);

    CopyDir_wait=new CWindow(20,10,60,15,"");
	CopyDir_wait->Show();
	CopyDir_wait->DrawTextCentered( 0, WAIT_COPYDIR );
	CopyDir_Progbar=new GUI_ProgressBar_OLD(CopyDir_wait,WAITCOPYDIR_POS,nfiles);

    ImFirst=1;
  }

  strcat(Buff_o,"/*.*");

  done = findfirst(Buff_o,&ffblk,FA_DIREC);

  char tmp_o[MAXNPATH];
  char tmp_d[MAXNPATH];
  
  int ret=0;

  while (!done)
  {
    if(S_ISDIR(ffblk.ff_attrib))
    {
      if(ffblk.ff_name[0]!='.')
      {
        strcpy(tmp_o,dir_orig);
        strcat(tmp_o,"/");
        strcat(tmp_o,ffblk.ff_name);
        
        strcpy(tmp_d,dir_dest);
        strcat(tmp_d,"/");
        strcat(tmp_d,ffblk.ff_name);

        if(!CopyDir(tmp_d,tmp_o))
        {
          done=0;
          ret=0;
        }
      }
    }
    else
    {
      strcpy(Buff_d,dir_dest);
      strcat(Buff_d,"/");
      strcat(Buff_d,ffblk.ff_name);

      strcpy(Buff_o,dir_orig);
      strcat(Buff_o,"/");
      strcat(Buff_o,ffblk.ff_name);

      if(!CopyFileOLD(Buff_d,Buff_o))
      {
        done=1;
        ret=0;
      }

      CopyDir_Progbar->Increment(1);
    }    

    done = findnext(&ffblk);
  }

  if(ImFirst)
  {
    delete CopyDir_Progbar;
    delete CopyDir_wait;
    CopyDir_Progbar=NULL;
    CopyDir_wait=NULL;
  }

  return(ret);
}


/*--------------------------------------------------------------------------
Conta i file in una cartella (comprese le sotto directory)
INPUT:   dir: nome cartella
GLOBAL:	-
RETURN:  ritorna il numero dei file
NOTE:	   -
--------------------------------------------------------------------------*/
int CountDirFiles(char *dir)
{
  char buf[MAXNPATH];

  int count=0;

  strcpy(buf,dir);
  strcat(buf,"/*.*");

  struct ffblk ffblk;
  
  int done = findfirst(buf,&ffblk,FA_DIREC);

  while(!done)
  {
	if(S_ISDIR(ffblk.ff_attrib))   //se e' una directory
    {
      if(ffblk.ff_name[0]!='.')  //se e' una directory effettiva (non . o ..)
      {
        strcpy(buf,dir);
        strcat(buf,"/");
        strcat(buf,ffblk.ff_name);
        count+=CountDirFiles(buf);
      }
    }
    else
    {
      count++;
    }

    done=findnext(&ffblk);
  }

  return(count);

}


//==========================================================================
// Gestione Clienti

int ndir;

/*--------------------------------------------------------------------------
Verifica la presenza delle tre directory associate ad un cliente
PROG, FEED e VISION
INPUT:   cust: cliente su cui verificare le directory
GLOBAL:	-
RETURN:  -
NOTE:	   Se non esistono una o piu' dir, le crea - DANY210103
--------------------------------------------------------------------------*/
void DirVerify(char *cust)
{
	if(strlen(cust)==0)
	{
		return;
	}
	
	char buf1[MAXNPATH];	
    
    strcpy(buf1,CLIDIR);
    strcat(buf1,"/");
    strcat(buf1,cust);

	if( !CheckDirectory(buf1) )
    {
      return;
    }
    
    strcat(buf1,"/");
    strcat(buf1,PRGDIR);
	if(!CheckDirectory(buf1))
    {
      mkdir(buf1, DIR_CREATION_FLAG); //GNU
    }
    
    strcpy(buf1,CLIDIR);
    strcat(buf1,"/");
    strcat(buf1,cust);
    strcat(buf1,"/");
    strcat(buf1,CARDIR);
	if(!CheckDirectory(buf1))
    {
      mkdir(buf1, DIR_CREATION_FLAG); //GNU
    }
    
    strcpy(buf1,CLIDIR);
    strcat(buf1,"/");
    strcat(buf1,cust);
    strcat(buf1,"/");
    strcat(buf1,VISDIR);
	if(!CheckDirectory(buf1))
    {
      mkdir(buf1, DIR_CREATION_FLAG); //GNU
    }
}

//==========================================================================

struct sBackupFilesList
{
	const char* name;	
	const char* dest;
	bool isDir;
	bool compress;
	bool multi;
	bool compress_store_pathname;
} backupFilesList[] =

{
	{  HOME_DIR "/" INSTALL_SCRIPT_FILE , INSTALL_SCRIPT_FILE  , false , false ,false , false},
	{  HOME_DIR "/" UPDATE_SCRIPT_FILE , UPDATE_SCRIPT_FILE , false , false , false , false},
	{  HOME_DIR "/" BACKUP_SCRIPT_FILE , BACKUP_SCRIPT_FILE , false , false , false, false},
	{  HOME_DIR "/" RESTORE_SCRIPT_FILE , RESTORE_SCRIPT_FILE , false , false , false, false},
	
	{ INSTALL_HELPER_FILES_DIR INSTALL_HELPER_SCRIPT_FILE, INSTALL_HELPER_SCRIPT_FILE, false, false , false, false},
	{ INSTALL_HELPER_FILES_DIR UPDATE_HELPER_SCRIPT_FILE, UPDATE_HELPER_SCRIPT_FILE, false, false , false, false},
	{ INSTALL_HELPER_FILES_DIR VERSION_DEF_FILE, VERSION_DEF_FILE, false, false , false, false},
	{ EXE_FILE, ZIP_EXE_FILE, false, true , false,true},
	
	{ FONT_DIR, ZIP_DATA_FILE,true, true , false, true},
	{ VISIONDIR MAPDATAFILEXY, ZIP_DATA_FILE, false, true , false, true},
	{ VISIONDIR VISDATAFILE, ZIP_DATA_FILE, false, true , false, true},
	{ STDFILENAME ".*" , ZIP_DATA_FILE, false, true , true, true},
	{ MAPFILENAME "*.*" , ZIP_DATA_FILE, false, true , true, true},
	{ MOTORHEADFILENAME "*.*" , ZIP_DATA_FILE, false, true , true, true},
	{ CLIDIR "/" STD_CUSTOMER "/" PRGDIR "/" STD_PROGRAM ".*", ZIP_DATA_FILE, false, true , true, true},
	{ CLIDIR "/" STD_CUSTOMER "/" CARDIR "/" STD_FEEDCONF CAREXT, ZIP_DATA_FILE, false, true , true, true},
	{ FPACKDIR "/" PKG_STDLIB ".*" , ZIP_DATA_FILE, false, true , true, true},

#ifdef __DISP2_REMOVABLE
	{ DISP1_DATA_DIR, ZIP_DATA_FILE, true, true, false, true },
	{ DISP2_DATA_DIR, ZIP_DATA_FILE, true, true, false, true },
	
	{ HOME_DIR "/" START_DISP1_SCRIPT_FILE, ZIP_HOME_FILE, false, true , false, false},
	{ HOME_DIR "/" START_DISP2_SCRIPT_FILE, ZIP_HOME_FILE, false, true , false, false},
	{ HOME_DIR "/" COPY_DISP1_DATA_SCRIPT_FILE, ZIP_HOME_FILE, false, true , false, false},
	{ HOME_DIR "/" COPY_DISP2_DATA_SCRIPT_FILE, ZIP_HOME_FILE, false, true , false, false}, 
#else
	{ HOME_DIR "/" START_SCRIPT_FILE, ZIP_HOME_FILE, false, true , false, false},
#endif

	{ HOME_DIR "/" OFF_SCRIPT_FILE, ZIP_HOME_FILE, false, true , false, false},
	{ HOME_DIR "/" RESET_SCRIPT_FILE, ZIP_HOME_FILE, false, true , false, false},
	{ HOME_DIR "/" SET_LANGUAGE_SCRIPT_FILE, ZIP_HOME_FILE, false, true , false, false},
};

typedef enum
{
	tmp_dir_creation_error,
	zip_create_error,
 	zip_compression_error,
  	zip_compression_dir_error,
    zip_write_error,
	copy_error,
} eDataBackupError;

typedef struct
{
	eDataBackupError ecode;
	const char* name;
} DataBackupError;

int DataBackup(const char* mnt)
{
	unsigned int nbackup_items = sizeof(backupFilesList) / sizeof(backupFilesList[0]);
	
	int count = 0;
	
	DataBackupError backup_error;
	
	if(strlen(nwpar.NetID)==0)
	{
		bipbip();
		W_Mess(NO_MACHINE_ID);
		return(0);
	}	
	
	for(int i = 0; i < nbackup_items; i++)
	{
		//fprintf(stderr,"i=%d\n",i);
		if(!backupFilesList[i].isDir)
		{
			if(!backupFilesList[i].multi)
			{				
				if(!access(backupFilesList[i].name,F_OK))
				{
					//fprintf(stderr,"%s found\n",backupFilesList[i].name);
					count++;
				}
				else
				{
					//fprintf(stderr,"%s not found\n",backupFilesList[i].name);
				}
			}
			else
			{
				struct ffblk ffblk;
				int done = findfirst(backupFilesList[i].name,&ffblk,0);
				
				//fprintf(stderr,"searching %s\n",backupFilesList[i].name);
				
				while(!done)
				{
					//fprintf(stderr,"  found %s\n",ffblk.ff_name);
					count++;				
					//cerca prossimo elemento
					done=findnext(&ffblk);
				}				
			}
		}
		else
		{
			if(CheckDirectory(backupFilesList[i].name))
			{
				//fprintf(stderr,"%s found\n",backupFilesList[i].name);
				count++;
			}			
			else
			{
				//fprintf(stderr,"%s not found\n",backupFilesList[i].name);
			}
		}
	}
	
	if(!count)
	{
		//IMPOSSIBILE !!!
		return 0;
	}
	
	ZipClass *z=NULL;
	CWindow   *DataBackup_Wait=NULL;
	
	GUI_ProgressBar_OLD *DataBackup_ProgBar=NULL;
		
	if(!CheckDirectory("tmp"))
	{
		if(mkdir("tmp",DIR_CREATION_FLAG))
		{
			backup_error.ecode = tmp_dir_creation_error;
			throw backup_error;		
		}
	}
	else
	{
		DeleteFiles_OLD("tmp");
	}	
	
	string dest_path;
	
	dest_path = mnt;
	dest_path += "/"USB_MACHINE_ID_FOLDER"/";
	dest_path += nwpar.NetID;
	dest_path += "/";
	
	//fprintf(stderr,"Dest path = %s\n",dest_path.c_str());
	
	DataBackup_Wait = new CWindow(13,10,67,14,"");
	DataBackup_Wait->Show();
	DataBackup_Wait->DrawTextCentered( 0, WAIT_BKPCUST2 );
	DataBackup_ProgBar = new GUI_ProgressBar_OLD(DataBackup_Wait,WAITBKPCUST_POS,count);
				
	DisableFileFunc();	
	
	string zfile_name;
	string zfile_fullpath_name;
	
	try
	{
		int err;
		
		for(int i = 0; i <= nbackup_items; i++)
		{		
			if(z && ((i == nbackup_items) || !backupFilesList[i].compress || (backupFilesList[i].compress && (zfile_name!=backupFilesList[i].dest))))
			{		
				err = z->WriteToZip();
				delete z;
				z = NULL;
							
				if(err != QZIP_OK)
				{
					backup_error.ecode = zip_write_error;
					backup_error.name = backupFilesList[i-1].dest;
					throw backup_error;								
				}										
				
				//fprintf(stderr,"Writing data to %s\n",zfile_fullpath_name.c_str());
				
				string dest = dest_path + backupFilesList[i-1].dest; 
				
				if(!CopyFileOLD(dest.c_str(),zfile_fullpath_name.c_str()))
				{
					backup_error.ecode = copy_error;
					backup_error.name = backupFilesList[i-1].dest;
					throw backup_error;																
				}
				
				//fprintf(stderr,"Copy %s to %s\n",zfile_fullpath_name.c_str(),dest.c_str());
			}
					
			if(i == nbackup_items)
			{
				break;
			}
			
			if(!backupFilesList[i].isDir)
			{
				if(!backupFilesList[i].multi)
				{
					if(!access(backupFilesList[i].name,F_OK))
					{				
						if(backupFilesList[i].compress)
						{
							if(z == NULL)
							{
								z = new ZipClass;
								
								zfile_fullpath_name = "tmp/";
								zfile_name = backupFilesList[i].dest;
								zfile_fullpath_name = zfile_fullpath_name + zfile_name;
								
								err=z->Open(zfile_fullpath_name.c_str(),ZIP_OPEN_CREATE);
								if(err != QZIP_OK)
								{
									backup_error.ecode = zip_create_error;
									backup_error.name = backupFilesList[i].dest;
									throw backup_error;							
								}
							}
							
							//fprintf(stderr,"Create %s\n",zfile_fullpath_name.c_str());
								
							if(backupFilesList[i].compress_store_pathname)
							{
								err=z->CompressFile(backupFilesList[i].name,Z_BEST_COMPRESSION);
							}
							else
							{
								err=z->CompressFile(backupFilesList[i].name,Z_BEST_COMPRESSION,true);
							}
							//fprintf(stderr,"%s compressed\n",backupFilesList[i].name);							
							DataBackup_ProgBar->Increment(1);
										
							if(err != QZIP_OK)
							{
								backup_error.ecode = zip_compression_error;
								backup_error.name = backupFilesList[i].name;
								throw backup_error;								
							}												
						}
						else
						{				
							string dest;
							string src(backupFilesList[i].name);
							
							if(backupFilesList[i].dest != NULL)
							{
								dest = dest_path + backupFilesList[i].dest;
							}
							else
							{
								dest = dest_path + backupFilesList[i].name;
							}
							
							//fprintf(stderr,"Copy %s to %s\n",src.c_str(),dest.c_str());
																											
							if(!CopyFileOLD(dest.c_str(),src.c_str()))
							{
								backup_error.ecode = copy_error;
								backup_error.name = backupFilesList[i].name;
								throw backup_error;																
							}	
							DataBackup_ProgBar->Increment(1);				
						}
					}
				}
				else
				{
					struct ffblk ffblk;
					
					int done = findfirst(backupFilesList[i].name,&ffblk,0);	
					
					//fprintf(stderr,"search %s\n",backupFilesList[i].name);
								
					while(!done)
					{												
						string file = backupFilesList[i].name;
						size_t last_slash_pos = file.rfind('/');
						if(last_slash_pos != string::npos)
						{
							file = file.substr(0,last_slash_pos) + "/" + ffblk.ff_name;
						}
						else
						{
							file = ffblk.ff_name;
						}					
						
						//fprintf(stderr,"found %s\n",file.c_str());
						
						if(backupFilesList[i].compress)
						{
							if(z == NULL)
							{		
								z = new ZipClass;
								
								zfile_fullpath_name = "tmp/";
								zfile_name = backupFilesList[i].dest;
								zfile_fullpath_name = zfile_fullpath_name + zfile_name;							
													
								err=z->Open(zfile_fullpath_name.c_str(),ZIP_OPEN_CREATE);
								if(err != QZIP_OK)
								{
									backup_error.ecode = zip_create_error;
									backup_error.name = backupFilesList[i].dest;
									throw backup_error;							
								}		
																	
								//fprintf(stderr,"Create %s\n",zfile_fullpath_name.c_str());
							}												
																							
							err = z->CompressFile(file.c_str(),Z_BEST_COMPRESSION);	
							if(err != QZIP_OK)
							{
								backup_error.ecode = zip_compression_error;
								backup_error.name = file.c_str();
								throw backup_error;								
							}
							//fprintf(stderr,"%s compressed\n",file.c_str());							
							DataBackup_ProgBar->Increment(1);						
						}
						else
						{
							string dest;
							string src(file);
						
							dest = dest_path + file;
							
							if(!CopyFileOLD(dest.c_str(),src.c_str()))
							{
								backup_error.ecode = copy_error;
								backup_error.name = file.c_str();
								throw backup_error;																
							}	
							DataBackup_ProgBar->Increment(1);						
						}
						
						done = findnext(&ffblk);
						
					}				
				}
			}
			else
			{
				if(z == NULL)
				{		
					z = new ZipClass;
								
					zfile_fullpath_name = "tmp/";
					zfile_name = backupFilesList[i].dest;
					zfile_fullpath_name = zfile_fullpath_name + zfile_name;							
													
					err=z->Open(zfile_fullpath_name.c_str(),ZIP_OPEN_CREATE);
					if(err != QZIP_OK)
					{
						backup_error.ecode = zip_create_error;
						backup_error.name = backupFilesList[i].dest;
						throw backup_error;							
					}											
					
					//fprintf(stderr,"Create %s\n",zfile_fullpath_name.c_str());
				}				
				
				if(CheckDirectory(backupFilesList[i].name))
				{								
					err=z->CompressDir(backupFilesList[i].name,Z_DEFAULT_COMPRESSION);
					DataBackup_ProgBar->Increment(1);					
	
					if(err!=QZIP_OK)
					{
						backup_error.ecode = zip_compression_dir_error;
						backup_error.name = backupFilesList[i].name;
						throw backup_error;	
					}				
					
					//fprintf(stderr,"directory %s compressed\n",backupFilesList[i].name);	
				}	
				else
				{
					//fprintf(stderr,"directory %s not found\n",backupFilesList[i].name);	
				}		
			}
		}	
		
		delete DataBackup_ProgBar;
		delete DataBackup_Wait;

		DeleteDirectory( "tmp" );

		EnableFileFunc();
	}
	catch(DataBackupError& err)
	{
		bipbip();
		char buf[80];
				
		switch(err.ecode)
		{
			case tmp_dir_creation_error:
				W_Mess(BACKUP_ERR_TEMPDIR);
				break;
			case zip_create_error:
				snprintf( buf, sizeof(buf),BACKUP_ERR_CREATEZIP,err.name);
				W_Mess(buf);				
				break;
			case zip_compression_error:
				snprintf( buf, sizeof(buf),BACKUP_ERR_COMPRESSF,err.name);
				W_Mess(buf);				
				break;
			case zip_write_error:
				snprintf( buf, sizeof(buf),BACKUP_ERR_WRITEZIP,err.name);
				W_Mess(buf);				
				break;
			case copy_error:
				snprintf( buf, sizeof(buf),BACKUP_ERR_COPYFILE,err.name);
				W_Mess(buf);				
				break;
			case zip_compression_dir_error:
				snprintf( buf, sizeof(buf),BACKUP_ERR_COMPRESSD,err.name);
				W_Mess(buf);								
				break;
		}
		
		if(z != NULL)
		{
			delete z;
			z = NULL;
		}
		
		delete DataBackup_ProgBar;
		delete DataBackup_Wait;
		
		DeleteDirectory( "tmp" );
		
		EnableFileFunc();		
		
		return 0;
	}
	
	bipbip();
	W_Mess(BKPCUST_COMPLETED);
	
	return 1;
}

typedef struct
{
  int num_idx;
  
  string name;
  
  struct BackupInfoDat info;
    
} BackupDirData;

int operator< (const BackupDirData &x,const BackupDirData &y)
{
  if(x.info.year<y.info.year)
  {
    return(1);
  }

  if(x.info.year>y.info.year)
  {
    return(0);
  }

  int d1=x.info.month*100+x.info.day;
  int d2=y.info.month*100+y.info.day;

  if(d1<d2)
  {
    return(1);
  }

  int t1=x.info.hour*100+x.info.min;
  int t2=y.info.hour*100+y.info.min;

  if(t1<t2)
  {
    return(1);
  }
  else
  {
    return(0);
  }
  
}

void GetBackupBaseDir(char* base,bkp::e_dest dest, const char* mnt)
{
	*base = '\0';
	switch(dest)
	{
		case bkp::USB:
			if(mnt!=NULL)
			{
				sprintf(base,"%s/" USB_MACHINE_ID_FOLDER "/%s/" USB_BACKUP_FOLDER,mnt,nwpar.NetID);
			}
			break;
		case bkp::Net:
			strcpy(base,REMOTEDISK "/" REMOTEBKP_DIR "/");
			strcat(base,nwpar.NetID);    
			break;
	}
			
}


void GetBackupDir(char *bkp_base,int mode,int bkp_num,bkp::e_dest dest,const char* mnt)
{
	GetBackupBaseDir(bkp_base,dest,mnt);
	
	if(mode==BKP_COMPLETE)
	{
		sprintf(bkp_base+strlen(bkp_base),"/" BKP_COMPLETE_TAG "%05X",bkp_num);
	}
	else
	{
		sprintf(bkp_base+strlen(bkp_base),"/" BKP_PARTIAL_TAG "%05X",bkp_num);
	}    
}

int ExtractBackupIdxFromName(char *name,int &mode,int &n)
{
	//search last "/"
	char *prev=NULL;
	
	char *p=strchr(name,'/');
	
	while(p!=NULL)
	{
		prev=p;
		p=strchr(++p,'/');
	}

	if(prev!=NULL)
	{
		p=++prev;
	}
	else
	{
		p=name;
	}
	
	if(strlen(p)!=8)
	{
		return(0);
	}
  
	prev=p+3;
	
	for(int i=0;i<5;i++)
	{
		if(!isxdigit(*prev))
		{
		return(0);
		}
	
		prev++;
	}

	if(!strncasecmpQ(p,BKP_COMPLETE_TAG,3))
	{
		mode=BKP_COMPLETE;
	}
	else
	{
		if(!strncasecmpQ(p,BKP_PARTIAL_TAG,3))
		{
		mode=BKP_PARTIAL;
		}
	}
	
	n=Hex2Val(p+3);
	
	return(1);  
}

int SetLastUsedBkpSlot(int mode,int n,char *bkp_base,bkp::e_dest dest,const char* mnt = NULL)
{
	if(!WriteMBkpInfo(n,mode,dest,mnt))
	{
		return(0);
	}
	
	GetBackupDir(bkp_base,mode,n,dest,mnt);

	if(!CheckDirectory(bkp_base))
	{
		if(mkdir(bkp_base,DIR_CREATION_FLAG))
		{
			return(0);
		}
	}
	else
	{
		CPan panwait( -1, 1, MsgGetString(Msg_02039) );

		if( !DeleteDirectory(bkp_base) )
		{
			return 0;
		}

		if(mkdir(bkp_base,DIR_CREATION_FLAG))
		{
			return 0;
		}
  	}

	struct BackupInfoDat data;
	
	struct time t;
	struct date day;
	
	gettime(&t);
	getdate(&day);
	
	data.day=day.da_day;
	data.month=day.da_mon;
	data.year=day.da_year;
	data.hour=t.ti_hour;
	data.min=t.ti_min;
	data.type=mode;
	
	if(!WriteBackupInfo(data,mode,n,dest,mnt))
	{
		return(0);
	}

	if(mode==BKP_COMPLETE)
	{
		if(dest == bkp::Net)
		{
			char buf[MAXNPATH+1];
		
			strcpy(buf,RESTORE_INFO_FILE);
		
			FILE *f=FilesFunc_fopen(buf,"w");
		
			if(f==NULL)
			{
				return(0);
			}
			
			fprintf(f,bkp_base);

      		FilesFunc_fclose(f);
			
			chmod(buf,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    	}    
  	}
  	return(1);

}

int GetLastBkpSlot(int mode,bkp::e_dest dest, const char* mnt = NULL)
{
	int n;
	
	char buf[MAXNPATH+1];
	
	GetMBkpInfo_FileName(buf,mode,dest,mnt);
	
	if(access(buf,F_OK))
	{
		return(0);
	}
	else
	{
		if(!ReadMBkpInfo(n,mode,dest,mnt))
		{
			bipbip();
			switch(dest)
			{
				case bkp::Net:
					W_Mess(NET_BKPACCESS_ERR);
					break;
				case bkp::USB:
					W_Mess(USB_NOT_READY);
					break;
			}
	
			return(-1);
		}
  	}

  	return(n);
}

#define BACKUP_COPY_CHECK(r)    if(!r)\
                                {\
                                  delete prgbar;\
                                  delete Q_Bkp;\
                                  bipbip();\
                                  switch(dest)\
                                  {\
                                    case bkp::Net:\
									  W_Mess(NET_BKPACCESS_ERR);\
                                      break;\
                                    case bkp::USB:\
                                      /*fprintf(stderr,"%s %s %d\n",__FILE__,__PRETTY_FUNCTION__,__LINE__);*/\
                                      W_Mess(USB_NOT_READY);\
                                      break;\
                                  }\
                                  EnableFileFunc();\
                                  return(0);\
                                }


#define BACKUP_COPY_DIR(dir,must_exists) \
								if(!CheckDirectory(dir))\
                                {\
                                  char tmpbuf[256];\
                                  sprintf(tmpbuf,UNABLE_TO_ACCESS_TO_FOLDER,dir);\
                                  if(must_exists)\
                                  {\
                                    bipbip();\
                                    W_Mess(tmpbuf);\
                                    EnableFileFunc();\
                                    return(0);\
                                  }\
                                }\
                                else\
                                {\
								  strcpy(buf,bkp_base);\
                                  strcat(buf,dir);\
                                  BACKUP_COPY_CHECK(DirCopy(buf,dir,prgbar));\
                                }


#define BACKUP_COPY_FILE_NO_PREFIX(file,must_exists)\
                                if(access(file,F_OK))\
                                {\
                                  char tmpbuf[256];\
                                  sprintf(tmpbuf,UNABLE_TO_ACCESS_TO_FILE,file);\
                                  if(must_exists)\
                                  {\
                                    bipbip();\
                                    W_Mess(tmpbuf);\
                                    EnableFileFunc();\
                                    return(0);\
                                  }\
                                  /*fprintf(stderr,"%s not found\n",file);*/\
                                }\
                                else\
                                {\
                                  strcpy(buf2,bkp_base);\
                                  strcat(buf2,"/");\
                                  strcat(buf2,file);\
                                  /*fprintf(stderr,"copy %s in %s\n",file,buf2);*/\
                                  BACKUP_COPY_CHECK(CopyFileOLD(buf2,file));\
                                }

#define BACKUP_COPY_FILE(file,src_prefix,must_exists) \
                                if(access(src_prefix file,F_OK))\
                                {\
                                  char tmpbuf[256];\
                                  sprintf(tmpbuf,UNABLE_TO_ACCESS_TO_FILE,src_prefix file);\
                                  if(must_exists)\
                                  {\
                                    bipbip();\
                                    W_Mess(tmpbuf);\
                                    EnableFileFunc();\
                                    return(0);\
                                  }\
                                  /*fprintf(stderr,src_prefix file " not found\n")*/;\
                                }\
                                else\
                                {\
                                  strcpy(buf2,bkp_base);\
                                  strcat(buf2,"/" file);\
                                  BACKUP_COPY_CHECK(CopyFileOLD(buf2,src_prefix file));\
                                }



int DoBkp(int mode,bkp::e_dest dest,const char* mnt)
{
	if(strlen(nwpar.NetID)==0)
	{
		bipbip();
		W_Mess(NO_MACHINE_ID);
		return(0);
	}

	char buf[MAXNPATH];
	char bkp_base[MAXNPATH+1];
	
	switch(dest)
	{
		case bkp::Net:
			strcpy(buf,REMOTEDISK "/" REMOTEBKP_DIR);
			break;
		
		case bkp::USB:
			CheckAndCreateUSBMachinesFolder(nwpar.NetID,mnt);
			snprintf( buf, sizeof(buf),"%s/" USB_MACHINE_ID_FOLDER,mnt);
			break;      
	}

	if(!CheckDirectory(buf))
	{
		if(mkdir(buf,DIR_CREATION_FLAG))
		{
			bipbip();
			switch(dest)
			{
				case bkp::Net:
					W_Mess(NET_BKPACCESS_ERR);
					break;
				case bkp::USB:
					W_Mess(USB_NOT_READY);
					break;
			}
			return(0);
		}
	}

	strcat(buf,"/");
	strcat(buf,nwpar.NetID);  

	if(!CheckDirectory(buf))
	{
		if(mkdir(buf,DIR_CREATION_FLAG))
		{
			bipbip();
			switch(dest)
			{
				case bkp::Net:
					W_Mess(NET_BKPACCESS_ERR);
					break;
				case bkp::USB:
					W_Mess(USB_NOT_READY);
					break;
			}
			return(0);
		}
	}

	if(dest == bkp::USB)
	{
		strcat(buf,"/" USB_BACKUP_FOLDER);
		if(!CheckDirectory(buf))
		{
			if(mkdir(buf,DIR_CREATION_FLAG))
			{
				bipbip();
				W_Mess(USB_NOT_READY);
				return(0);
			}
		}
	}

  	DisableFileFunc();

	//cerca prossimo slot libero di backup per il modo selezionato
	int n=GetLastBkpSlot(mode,dest,mnt);
	
	if(n==-1)
	{
		EnableFileFunc();
		return(0);
	}

	n++;
		
	if(mode==BKP_COMPLETE)
	{
		if(n==BKP_LIMIT_FULL)
		{
		n=0;
		}
	}
	else
	{
		if(n==BKP_LIMIT_PARTIAL)
		{
		n=0;
		}
	}

	if(!SetLastUsedBkpSlot(mode,n,bkp_base,dest,mnt))
	{
		bipbip();
		switch(dest)
		{
			case bkp::Net:
				W_Mess(NET_BKPACCESS_ERR);
				break;
			case bkp::USB:
				W_Mess(USB_NOT_READY);
				break;
		}
		EnableFileFunc();
		return(0);
	}

  	strcat(bkp_base,"/");

	CWindow *Q_Bkp=new CWindow(BKPPOS,"");
	Q_Bkp->Show();
	
	Q_Bkp->DrawTextCentered( 0, BKPWAIT );
	
	struct ffblk f;
	
	int count=0;
	int done;
	
	if(mode==BKP_COMPLETE)
	{
		done = findfirst(STDFILENAME".*", &f, 0);
		while (!done)
		{
			DelSpcR(f.ff_name);

			if(strcasecmpQ(RemovePathString(f.ff_name),BKP_INFO_FILE))
			{
				count++;
			}

			done = findnext(&f);
		}
		
		done = findfirst(MAPFILENAME"*.*", &f, 0);
		while (!done)
		{
			DelSpcR(f.ff_name);

			count++;

			done = findnext(&f);
		}
		
		done = findfirst(MOTORHEADFILENAME"*.*", &f, 0);
		while (!done)
		{
			DelSpcR(f.ff_name);

			count++;

			done = findnext(&f);
		}
	
		count+=DirCount(CLIDIR);
		count+=DirCount(VISDIR);
		count+=DirCount(FPACKDIR);
		count+=DirCount(FONT_DIR);
		count+=DirCount(CARDIR);
		count+=DirCount(VISPACK);
		count+=DirCount(INSTALL_STORAGE_DIR);
	
		count+=13;
	}
	else
	{
		count+=FList_Count();
	}

	GUI_ProgressBar_OLD *prgbar=new GUI_ProgressBar_OLD(Q_Bkp,3,3,54,1,count,0);

	char buf2[MAXNPATH];
	
	if(mode==BKP_COMPLETE)
	{
		done = findfirst(STDFILENAME".*", &f, 0);
		while(!done)
		{
			DelSpcR(f.ff_name);
		
			if(!strcasecmpQ(RemovePathString(f.ff_name),BKP_INFO_FILE))
			{
				prgbar->Increment(1);
				done=findnext(&f);
				continue;
			}

			BACKUP_COPY_FILE_NO_PREFIX(f.ff_name,true);
		
			prgbar->Increment(1);
		
			done=findnext(&f);
		}
		
		done = findfirst(MAPFILENAME"*.*", &f, 0);
		while(!done)
		{
			DelSpcR(f.ff_name);

			BACKUP_COPY_FILE_NO_PREFIX(f.ff_name,true);

			prgbar->Increment(1);

			done=findnext(&f);
		}

		done = findfirst(MOTORHEADFILENAME"*.*", &f, 0);
		while(!done)
		{
			DelSpcR(f.ff_name);

			BACKUP_COPY_FILE_NO_PREFIX(f.ff_name,true);

			prgbar->Increment(1);

			done=findnext(&f);
		}
		snprintf( buf2, sizeof(buf2),"%s -q %s.zip %s",ZIP_PRG,STDFILENAME,EXE_FILE);
		system(buf2);
	
		if(!access(STDFILENAME".zip",F_OK))
		{
			BACKUP_COPY_FILE(STDFILENAME".zip","",true);
			remove(STDFILENAME".zip");
		}

		prgbar->Increment(1);

		BACKUP_COPY_DIR(CLIDIR,false);
		BACKUP_COPY_DIR(VISDIR,false);
		BACKUP_COPY_DIR(FPACKDIR,false);
		BACKUP_COPY_DIR(FONT_DIR,false);
		BACKUP_COPY_DIR(CARDIR,false);
		BACKUP_COPY_DIR(VISPACK,false);
		BACKUP_COPY_DIR(INSTALL_STORAGE_DIR,false);

		strcat(bkp_base,BACKUP_ROOTDIR);
		if(!CheckDirectory(bkp_base))
		{
			BACKUP_COPY_CHECK(!mkdir(bkp_base,DIR_CREATION_FLAG));
		}
	
		BACKUP_COPY_FILE(INSTALL_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);
	
		#ifndef __DISP2_REMOVABLE
		BACKUP_COPY_FILE(START_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);
		#else
		BACKUP_COPY_FILE(START_DISP1_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);
		BACKUP_COPY_FILE(START_DISP2_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);
		BACKUP_COPY_FILE(COPY_DISP1_DATA_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);
		BACKUP_COPY_FILE(COPY_DISP2_DATA_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);
		#endif
		
		
		BACKUP_COPY_FILE(OFF_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);
		
		BACKUP_COPY_FILE(RESET_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);

		BACKUP_COPY_FILE(UPDATE_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);

		BACKUP_COPY_FILE(BACKUP_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);

		BACKUP_COPY_FILE(RESTORE_SCRIPT_FILE,HOME_DIR "/",false);
		prgbar->Increment(1);

		nwpar.executed=1;
		getdate(&(nwpar.last));
		WriteNetPar(nwpar);

#ifdef __LOG_ERROR
		if( !BackupLogger.IsEmpty() )
			W_Mess( BKPFILESERR );
#endif
	}
	else
	{
		struct FOpenItem *ptr=FList_GetHead();
		
		while(ptr!=NULL)
		{
			BACKUP_COPY_FILE_NO_PREFIX(ptr->name,false);
			prgbar->Increment(1);
			ptr=ptr->next;
		}
	}
	
	delete prgbar;
	delete Q_Bkp;
	
	FList_Flush();
	FList_Save();
	
	EnableFileFunc();
}


//TODO: non si distingue se c'e' un errore di accesso oppure se non ci sono backup
int GetBackupList(list<BackupDirData> &l,bkp::e_dest dest, const char* mnt)
{
	char buf[MAXNPATH+1];
	
	GetBackupBaseDir(buf,dest,mnt);
	strcat(buf,"/*.*");
	
	struct ffblk f;
	
	int done=findfirst(buf,&f,FA_DIREC);
	
	while(!done)
	{
		if(S_ISDIR(f.ff_attrib))   //se e' una directory
		{
			if(f.ff_name[0]!='.')  //se e' una directory effettiva (non . o ..)
			{
				//se e' un nome directory valido
				int mode;
		
				BackupDirData bkp_data;
		
				if(ExtractBackupIdxFromName(f.ff_name,mode,bkp_data.num_idx))
				{
					bkp_data.name=f.ff_name;
		
					if(ReadBackupInfo(bkp_data.info,mode,bkp_data.num_idx,dest,mnt))
					{
						if(bkp_data.info.type==mode)
						{
							l.push_back(bkp_data);
						}
					}
				}
			}
		}
    
    	done=findnext(&f);
  	}

	l.sort();
	
	return(l.size());
}

//---------------------------------------------------------------------------
// Selezione del backup da ripristinare
//---------------------------------------------------------------------------
bool SelectRestore( BackupDirData &dir_data, bkp::e_dest dest, const char* mnt )
{
	std::list<BackupDirData> SelBackup_DirList;
	if( !GetBackupList(SelBackup_DirList,dest,mnt) )
	{
		// errore di accesso
		switch( dest )
		{
			case bkp::Net:
				W_Mess( NET_BKPACCESS_ERR );
				break;
			case bkp::USB:
				W_Mess( USB_NOT_READY );
				break;
		}
		return false;
	}

	if( SelBackup_DirList.size() == 0 )
	{
		W_Mess( NOBKP );
		return false;
	}

	// selection window
	CWindowSelect win( 0, 3, 1, 10, 40 );
	win.SetStyle( win.GetStyle() | WIN_STYLE_NO_MENU );
	win.SetClientAreaPos( 0, 7 );
	win.SetTitle( MsgGetString(Msg_02002) );

	for( list<BackupDirData>::const_iterator it = SelBackup_DirList.begin(); it != SelBackup_DirList.end(); it++ )
	{
		char buf[40];
		if( it->info.type == BKP_COMPLETE )
		{
			strcpy( buf, BKPCOMPLETE_TXT );
		}
		else
		{
			strcpy( buf, BKPPARTIAL_TXT );
		}

		snprintf( buf+strlen(buf), sizeof(buf), "  %02d/%02d/%04d  %02d:%02d", it->info.day, it->info.month, it->info.year, it->info.hour, it->info.min );

		win.AddItem( buf );
	}

	win.Show();
	win.Hide();

	if( win.GetExitCode() == WIN_EXITCODE_ESC )
	{
		// user aborted
		return false;
	}

	int selectedItem = win.GetSelectedItemIndex();
	int count = 0;

	for( list<BackupDirData>::const_iterator it = SelBackup_DirList.begin(); it != SelBackup_DirList.end(); it++ )
	{
		if( count == selectedItem )
		{
			dir_data = *it;
			break;
		}

		count++;
	}

	return true;
}


#define RESTORE_COPY_CHECK(r)   if(!r)\
                                {\
                                  delete prgbar;\
                                  delete Q_Restore;\
                                  bipbip();\
                                  switch(dest)\
                                  {\
                                    case bkp::Net:\
                                      W_Mess(NET_BKPACCESS_ERR);\
                                      break;\
                                    case bkp::USB:\
                                      W_Mess(USB_NOT_READY);\
                                      break;\
                                  }\
                                  EnableFileFunc();\
                                  return(0);\
                                }



int DoBkpRestore(bkp::e_dest dest, const char* mnt)
{
	char buf[256];

	if(strlen(nwpar.NetID)==0)
	{
		bipbip();
		W_Mess(NO_MACHINE_ID);
		return(0);
	}
	
	if(dest == bkp::USB)
	{
		CheckAndCreateUSBMachinesFolder(nwpar.NetID,mnt);		
	}
	
	BackupDirData bkpData;
	
	DisableFileFunc();
	
	if( !SelectRestore(bkpData,dest,mnt) )
	{
		EnableFileFunc();
		return(0);
  	}

  	char tmp[16];

	if(bkpData.info.type==BKP_COMPLETE)
	{
		strcpy(tmp,BKPCOMPLETE_TXT);
	}
	else
	{
		strcpy(tmp,BKPPARTIAL_TXT);
	}
	
	strlwr(tmp);
	
	snprintf( buf, sizeof(buf),NETRESTORE_MSG,tmp,bkpData.info.day,bkpData.info.month,bkpData.info.year,bkpData.info.hour,bkpData.info.min);

	if(!W_Deci(0,buf))
	{
		EnableFileFunc();
		return(0);
	}
	
	CWindow *Q_Restore=new CWindow(RESTOREPOS,"");
	Q_Restore->Show();
	
	Q_Restore->DrawTextCentered( 0, RESTOREWAIT );
	
	struct ffblk f;
	
	char bkp_base[MAXNPATH];
	char bkp_path[MAXNPATH];

	GetBackupDir(bkp_base,bkpData.info.type,bkpData.num_idx,dest,mnt);
  	strcat(bkp_base,"/");

  	strcpy(bkp_path,bkp_base);
  	strcat(bkp_path,"*.*");

	int done=findfirst(bkp_path, &f, 0);
	
	int count=0;
	
	while (!done)
	{
		if(strlen(f.ff_name)<=3)
		{
			continue;
		}
		
		strncpyQ(buf,(f.ff_name+strlen(f.ff_name)-3),3);
	
		//se estensione diversa da bkp e zip aggiungi al conteggio
		if(strcasecmpQ(buf,"zip") && strcasecmpQ(buf,"bkp"))
		{
			count++;
		}

		done = findnext(&f);
	}
	
	int net_err=0;
	
	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,CLIDIR);
	count+=DirCount(bkp_path);
	
	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,VISDIR);
	count+=DirCount(bkp_path);
	
	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,INSTALL_STORAGE_DIR);
	count+=DirCount(bkp_path);
	
	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,FPACKDIR);
	count+=DirCount(bkp_path);

	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,FONT_DIR);
	count+=DirCount(bkp_path);

	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,CARDIR);
	count+=DirCount(bkp_path);

	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,VISPACK);
	count+=DirCount(bkp_path);

	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,BACKUP_ROOTDIR);
	
	if(!net_err)
	{
		count += DirCount(bkp_path);
	}
  
	GUI_ProgressBar_OLD *prgbar=new GUI_ProgressBar_OLD(Q_Restore,3,3,54,1,count,0);
	
	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,FPACKDIR);
	if(CheckDirectory(bkp_path))
	{
		RESTORE_COPY_CHECK(DirCopy(FPACKDIR,bkp_path,prgbar));
	}
	
	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,VISDIR);
	if(CheckDirectory(bkp_path))
	{
		RESTORE_COPY_CHECK(DirCopy(VISDIR,bkp_path,prgbar));
	}

	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,CLIDIR);
	if(CheckDirectory(bkp_path))
	{
		RESTORE_COPY_CHECK(DirCopy(CLIDIR,bkp_path,prgbar));
	}
	
	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,INSTALL_STORAGE_DIR);
	if(CheckDirectory(bkp_path))
	{
		RESTORE_COPY_CHECK(DirCopy(INSTALL_STORAGE_DIR,bkp_path,prgbar));
	}
	
	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,FONT_DIR);
	if(CheckDirectory(bkp_path))
	{
		RESTORE_COPY_CHECK(DirCopy(FONT_DIR,bkp_path,prgbar));
	}

	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,CARDIR);
	if(CheckDirectory(bkp_path))
	{
		RESTORE_COPY_CHECK(DirCopy(CARDIR,bkp_path,prgbar));
	}

	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,VISPACK);
	if(CheckDirectory(bkp_path))
	{
		RESTORE_COPY_CHECK(DirCopy(VISPACK,bkp_path,prgbar));
	}

	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,BACKUP_ROOTDIR);
	
	if( !net_err )
	{
		if(CheckDirectory(bkp_path))
		{
			RESTORE_COPY_CHECK(DirCopy(HOME_DIR "/",bkp_path,prgbar));
		}
	}
	else
	{
		net_err=1;
	}

	strcpy(bkp_path,bkp_base);
	strcat(bkp_path,"*.*");
	done = findfirst(bkp_path, &f, 0);

	while (!done)
	{
		strncpyQ(buf,(f.ff_name+strlen(f.ff_name)-3),3);
	
		if((strcasecmpQ(buf,"zip")) && strcasecmpQ(buf,"bkp"))
		{
			strcpy(bkp_path,bkp_base);
			strcat(bkp_path,f.ff_name);
			RESTORE_COPY_CHECK(CopyFileOLD(f.ff_name,bkp_path));
		}
	
		done = findnext(&f);
	}

	EnableFileFunc();
	
	delete prgbar;
	
	delete Q_Restore;
}

int CheckBkp(int mode,bkp::e_dest dest, const char* mnt)
{
	if(dest==bkp::Net)
	{
		if(!IsNetEnabled())
		{
			return(0);
		}
	}

	if(mode & CHKBKP_FIRST)
	{
		char buf[MAXNPATH+1];
		
		GetMBkpInfo_FileName(buf,BKP_COMPLETE,dest,mnt);
	
		if(access(buf,F_OK))
		{
			if(W_Deci(1,FIRSTBKP))
			{
				DoBkp(BKP_COMPLETE,dest,mnt);
				return(1);
			}
			else
			{
				return(0);
			}
		}
	}

	if(mode & CHKBKP_DATE)
	{
		struct date today;
	
		getdate(&today);
	
		int n=GetLastBkpSlot(BKP_COMPLETE,dest,mnt);
	
		if(n!=-1)
		{
			struct BackupInfoDat bdata;
		
			if(ReadBackupInfo(bdata,BKP_COMPLETE,n,dest,mnt))
			{
				int days_since_last_backup = DiffDate(nwpar.last,today);
							
				//in caso di errore nella data si forza la richiesta di un nuovo backup
				//come se fosse trascorso il numero di giorni limite
				if((days_since_last_backup >= BACKUPDAYS) || (days_since_last_backup == -1))
				{
					char buf[256];
			
					snprintf( buf, sizeof(buf),LASTBKP,nwpar.last.da_day,nwpar.last.da_mon,nwpar.last.da_year);
			
					W_MsgBox *box=new W_MsgBox(MsgGetString(Msg_00289),buf,3,MSGBOX_GESTKEY);
			
					box->AddButton(FAST_NO,0);
					box->AddButton(BKP_FULLBUTTON,1);
					box->AddButton(BKP_PARTBUTTON,0);
			
					//r==1 NO, r==2 FULL, r==3 PATIAL
					int r=box->Activate();
			
					delete box;
			
					r--;
	
					if(r)
					{
						DoBkp(r,dest,mnt);
						return(1);
					}
					else
					{
						return(0);
					}
        		}
      		}
    	}
    	else
    	{
      		return(0);
 	   	}	
  	}
  
	if(mode & CHKBKP_PARTIAL)
	{
		if(FList_Count())
		{
			if(!W_Deci(1,ASK_BKP))
			{
				return(0);
			}
		}
	
		DoBkp(BKP_PARTIAL,dest,mnt);
	}

  return(1);

}
