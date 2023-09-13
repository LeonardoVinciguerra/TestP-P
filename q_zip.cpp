#include "q_zipt.h"
#include "q_zip.h"

#include "strutils.h"
#include "fileutils.h"
#include "datetime.h"

#include <mss.h>


//#define QZIP_DEBUG stderr

//-----------------------------------------------------------------------
//Costruttore
//Inizializza le variabili senza aprire il file

//Variabili di ingresso: nessuna
//-----------------------------------------------------------------------
ZipClass::ZipClass(void)
{
  hFile=0;
  zs_init=0;

  zs_FlushDest=0;

  zs_buffer=NULL;

  zCDirEnd=NULL;
  zFileName=NULL;
  zPromptDisk=NULL;

  for(int i=0;i<MAX_PROGFUNC_TYPE;i++)
  {
    fProgress[i]=NULL;
  }
}


//-----------------------------------------------------------------------
//Distruttore
//Dealloca le strutture dati

//Variabili di ingresso: nessuna
//-----------------------------------------------------------------------
ZipClass::~ZipClass(void)
{
  Close();

  if(zs_buffer)
  {
    delete[] zs_buffer;
  }

}

//-----------------------------------------------------------------------
//Open
//Apre un file specificato
//
//Variabili di ingresso:
//  fname: nome del file completo di pathname
//  mode :
//         ZIP_OPEN_CREATE crea un nuovo file .zip
//         ZIP_OPEN_READ   apre in lettura un file .zip esistente
//         ZIP_OPEN_SPAN   (solo per create) abilita multidisk spanning
//
//Valore ritornato: QZIP_OK(0) se nessun errore
//                  codice di errore altrimenti.
//-----------------------------------------------------------------------
int ZipClass::Open(const char *fname,int mode)
{
  if(hFile)
  {
    return(QZIP_ISOPEN);
  }

  zs_FlushDest=0;
  zWriting=0;
  
  ClearTempBlocks();
  ClearLocalDataList();
  ClearCDirDataList();  

  if(zCDirEnd!=NULL)
  {  
    FreeCDirEnd(zCDirEnd);
    zCDirEnd=NULL;
  }

  if(zFileName!=NULL)
  {
    delete[] zFileName;
    zFileName=NULL;
  }

  zFileName=new char[strlen(fname)+5];
  strncpyQ(zFileName,fname,strlen(fname));
  zFileName[strlen(fname)]='\0';  

  char *ptr=strchr(zFileName,'.');

  if(ptr==NULL)
  {
    //il file non ha estensione: viene forzata a zip
    strcat(zFileName,".zip");
  }  

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nOpen file=%s\n",zFileName);
  #endif

  if(!(mode & ZIP_OPEN_CREATE))
  {
    if(access(zFileName,F_OK))
    {
      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"File not found\n");
      #endif
      return(QZIP_NOTFOUND);
    }
  }

  int flag;

  zOpenMode=mode;

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Open mode=%d\n",zOpenMode);
  #endif

  if(mode & ZIP_OPEN_CREATE)
  {
    flag=O_RDWR | O_CREAT | O_TRUNC;
  }

  if(mode & ZIP_OPEN_READ)
  {
    flag=O_RDWR;
  }

  if(!flag)
  {
    flag=O_RDWR;  
  }

  if((hFile=FilesFunc_open(zFileName,flag,S_IREAD | S_IWRITE))<0)
  {
    return(QZIP_OPEN_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Open ok\n");
  #endif

  zCDirEnd=AllocCDirEnd();

  if(mode & ZIP_OPEN_CREATE)
  {
    memset(zCDirEnd,(char)0,sizeof(zCDirEnd));

    if(mode & ZIP_OPEN_SPAN)
    {
      zSpan=DEFAULT_SPANSIZE;
      zDisk=-1;
    }
    else
    {
      zSpan=0;
      zDisk=0;
    }    
  }
  else
  {
    zDisk=0;

    int err=GetCentralDirEnd(zCDirEnd);

    if(err!=QZIP_OK)
    {
      return(err);
    }

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Get central directory end ok\n");
    #endif    

    zSpan=(zCDirEnd->hdr.disk>0);

    if(zCDirEnd->hdr.disk<zCDirEnd->hdr.disk_startcd)
    {
      //disk with start of central diretory is greater than last disk:
      //impossible !!
      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"End disk number error\n");
      #endif
      
      return(QZIP_FORMAT_ERR);
    }

    if(!zSpan && (zCDirEnd->hdr.nentry_cdir!=zCDirEnd->hdr.nentry_cdir_tot))
    {
      //no multidisk spanning: total central directory number of items must
      //be the same that are on that disk.
      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"Entry number error\n");
      #endif
      
      return(QZIP_FORMAT_ERR);
    }

    if(zCDirEnd->hdr.disk!=zCDirEnd->hdr.disk_startcd)
    {
      //central directory is on another disk
      err=PromptForDisk(zCDirEnd->hdr.disk_startcd);

      if(err!=QZIP_OK)
      {
        return(err);
      }
    }

    zDisk=zCDirEnd->hdr.disk_startcd;

    if(lseek(hFile,zCDirEnd->hdr.cdir_offset,SEEK_SET)!=zCDirEnd->hdr.cdir_offset)
    {
      return(QZIP_SEEK_ERR);
    }

    err=GetAllCentralDirData();

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Get all central directory headers\n");
    #endif

    if(err!=QZIP_OK)
    {
      return(err);
    }    
  }

  return(QZIP_OK);
  
}

int ZipClass::GetZipNotes(char *notes)
{
  if(zCDirEnd->hdr_var.zipcomment==NULL)
  {
    return(0);
  }

  if(zCDirEnd->hdr.zipcomment_len)
  {
    strcpy(notes,zCDirEnd->hdr_var.zipcomment);

    return(1);
  }

  return(0);
}


int ZipClass::SetZipNotes(const char *notes)
{
  zCDirEnd->hdr.zipcomment_len=strlen(notes);
  zCDirEnd->hdr_var.zipcomment=new char[strlen(notes)+1];
  strcpy(zCDirEnd->hdr_var.zipcomment,notes);
}

int ZipClass::PromptForDisk(int disk)
{
  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nGet disk %d\n",disk);
  #endif

  if(zPromptDisk)
  {
    int r=zPromptDisk(disk,zFileName);

    if(r!=QZIP_OK)
    {
      return(r);
    }
  }
  else
  {
    char* ptr = strchr(zFileName,'.');

    if(ptr==NULL)
    {
      return(QZIP_INTERNAL_ERR);
    }

    if(disk>0)
    {
      sprintf(ptr,".z%02d",disk);
    }
    else
    {
      strcpy(ptr,".zip");
    }
  }

  if(hFile)
  {
    FilesFunc_close(hFile);
    hFile=0;
  }

  int flag=O_RDWR;

  if(zOpenMode & ZIP_OPEN_CREATE)
  {
    flag=O_RDWR | O_CREAT | O_TRUNC;
  }

  if((hFile=FilesFunc_open(zFileName,flag,S_IREAD | S_IWRITE))<0)
  {
    return(QZIP_OPEN_ERR);    
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Disk %d (%s) opened\n",disk,zFileName);
  #endif

  if(zOpenMode & (ZIP_OPEN_CREATE | ZIP_OPEN_SPAN))
  {
    zSpanAvail=zSpan;

    if(disk==0)
    {
      int sign=SIGN_SPANNING_MARKER;

      if(write(hFile,&sign,SIGNATURE_SIZE)!=SIGNATURE_SIZE)
      {
        return(QZIP_WRITE_ERR);
      }

      zSpanAvail-=SIGNATURE_SIZE;

      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"Spanning signature writed\n");
      #endif
    }
  }

  zDisk=disk;

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"New disk is %d\n",zDisk);
  #endif

  return(QZIP_OK);
  
}

//-----------------------------------------------------------------------
//Close
//Chiude il file
//
//Variabili di ingresso: nessuna
//Valore ritornato     : nessuno
//-----------------------------------------------------------------------
void ZipClass::Close(void)
{
  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nClosing\n");
  #endif

  if(zWriting)
  {
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Some data are present in write buffer: flushing\n");
    #endif
    WriteToZipEnd();
  }

  if(hFile)
  {
    FilesFunc_close(hFile);
    hFile=0;
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"File %s closed\n",zFileName);
    #endif
  }

  if(zs_FlushDest)
  {
    FilesFunc_close(zs_FlushDest);
    zs_FlushDest=0;
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Output file closed\n");
    #endif
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Clearing data buffers\n");
  #endif

  ClearTempBlocks();
  ClearLocalDataList();
  ClearCDirDataList();

  if(zCDirEnd!=NULL)
  {  
    FreeCDirEnd(zCDirEnd);
    zCDirEnd=NULL;
  }

  if(zFileName!=NULL)
  {
    delete[] zFileName;
    zFileName=NULL;
  }  

}

//-----------------------------------------------------------------------
//GetTotalDiskNumber
//Ritorna il numero totale dei dischi
//
//Variabili di ingresso: nessuna
//Valore ritornato     : numero dei dischi se disponibile
//                       0 altrimenti
//-----------------------------------------------------------------------
int ZipClass::GetTotalDiskNumber(void)
{
  if(zCDirEnd==NULL)
  {
    return(0);
  }

  return(zCDirEnd->hdr.disk);
}

//-----------------------------------------------------------------------
//GetTotalDiskNumber
//Ritorna il numero totale di file presenti
//
//Variabili di ingresso: nessuna
//Valore ritornato     : numero dei file se disponibile
//                       0 altrimenti
//-----------------------------------------------------------------------
int ZipClass::GetTotalFilesNumber(void)
{
  if(zCDirEnd==NULL)
  {
    return(0);
  }

  return(zCDirEnd->hdr.nentry_cdir_tot);
}

//-----------------------------------------------------------------------
//IsOpen
//Controlla corretta apertura del file
//
//Variabili di ingresso: nessuna
//Valore ritornato     : 1 se il file e' aperto
//                       0 altrimenti
//-----------------------------------------------------------------------
int ZipClass::IsOpen(void)
{
  return(hFile!=0);
}

//-----------------------------------------------------------------------
//AllocLocalFileData/CDirData/CDirEnd
//Alloca una struttura dati per localheader/central directory header/
//central directory end
//
//Variabili di ingresso: nessuna
//Valore ritornato     : puntatore alla struttura dati
//-----------------------------------------------------------------------
sLocalData *ZipClass::AllocLocalFileData(void)
{
  sLocalData *d=new sLocalData;
  memset(d,(char)0,sizeof(sLocalData));

  return(d);
}

sCDirData *ZipClass::AllocCDirData(void)
{
  sCDirData *d=new sCDirData;
  memset(d,(char)0,sizeof(sCDirData));
  return(d);

}

sCDirEnd *ZipClass::AllocCDirEnd(void)
{
  sCDirEnd *d=new sCDirEnd;
  memset(d,(char)0,sizeof(sCDirEnd));
  return(d);
}

//-----------------------------------------------------------------------
//FreeLocalFileData/CDirData/CDirEnd
//Dealloca una struttura dati per localheader/central directory header/
//central directory end
//
//Variabili di ingresso:
//  d: puntatore alla struttura dati
//-----------------------------------------------------------------------
void ZipClass::FreeLocalFileData(sLocalData *d)
{
  if(d->data.buf!=NULL)
  {
    delete[] d->data.buf;
  }

  if(d->hdr_var.filename!=NULL)
  {
    delete[] d->hdr_var.filename;
  }

  if(d->hdr_var.extrafield!=NULL)
  {
    delete[] d->hdr_var.extrafield;
  }

  delete d;
}


void ZipClass::FreeCDirData(sCDirData *d)
{
  if(d->hdr_var.filename!=NULL)
  {
    delete[] d->hdr_var.filename;
  }

  if(d->hdr_var.extrafield!=NULL)
  {
    delete[] d->hdr_var.extrafield;
  }

  if(d->hdr_var.filecomment!=NULL)
  {
    delete[] d->hdr_var.filecomment;
  }

  delete d;
}

void ZipClass::FreeCDirEnd(sCDirEnd *d)
{
  if(d->hdr_var.zipcomment!=NULL)
  {
    delete[] d->hdr_var.zipcomment;
  }

  delete d;
}

//-----------------------------------------------------------------------
//AddToZipBlock
//Aggiunge alla lista dei blocchi compressi un altro buffer
//compresso
//
//Variabili di ingresso:
//  data: buffer compresso
//  len : lunghezza del buffer
//-----------------------------------------------------------------------
void ZipClass::AddToTempBlocks(unsigned char *data,int len)
{
  sDataBlock dblock;

  dblock.data=new unsigned char[len];
  memcpy(dblock.data,data,len);
  dblock.len=len;
  
  zTempList.push_back(dblock);

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"%d bytes added to temporanely blocks\n",len);
  #endif
}

//-----------------------------------------------------------------------
//ClearTempBlocks
//Azzera la lista dei blocchi compressi
//
//Variabili di ingresso: nessuna
//-----------------------------------------------------------------------
void ZipClass::ClearTempBlocks(void)
{
  if(zTempList.empty())
  {
    return;
  }

  list<sDataBlock>::const_iterator iter;

  for(iter=zTempList.begin();iter!=zTempList.end();iter++)
  {
    if(iter->data!=NULL)
    {
      delete[] iter->data;
    }
  }

  zTempList.clear();

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Temporanely blocks cleared\n");
  #endif
}

//-----------------------------------------------------------------------
//FlushTempBlocks
//Svuota la lista dei blocchi compressi in un unico buffer
//
//Variabili di ingresso:
//  len: variabile di ritorno della lunghezza del nuovo buffer
//
//Valore ritornato: puntatore al buffer se ok, NULL altrimenti
//-----------------------------------------------------------------------
unsigned char *ZipClass::FlushTempBlocks(unsigned int &len)
{
  if(zTempList.empty())
  {
    return(NULL);
  }

  unsigned char *ptr;

  len=0;

  list<sDataBlock>::const_iterator iter;

  for(iter=zTempList.begin();iter!=zTempList.end();iter++)
  {
    if((iter->data!=NULL) && (iter->len!=0))    
    {
      len+=iter->len;
    }
  }

  if(!len)
  {
    return(NULL);
  }

  ptr=new unsigned char[len];

  int count=0;

  for(iter=zTempList.begin();iter!=zTempList.end();iter++)
  {
    if((iter->data!=NULL) && (iter->len!=0))    
    {
      memcpy(ptr+count,iter->data,iter->len);
      count+=iter->len;
    }
  }

  ClearTempBlocks();

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"%d bytes flushed from temporanely blocks in a new buffer\n",count);
  #endif

  return(ptr);
  
}

//-----------------------------------------------------------------------
//ClearLocalDataList
//Azzera la lista dei local header
//
//Variabili di ingresso: nessuna
//-----------------------------------------------------------------------
void ZipClass::ClearLocalDataList(void)
{
  if(zLocalDataList.empty())
  {
    return;
  }

  list<sLocalData *>::iterator iter;

  for(iter=zLocalDataList.begin();iter!=zLocalDataList.end();iter++)
  {
    if((*iter)->hdr_var.filename!=NULL)
    {
      delete[] (*iter)->hdr_var.filename;
    }

    if((*iter)->hdr_var.extrafield!=NULL)
    {
      delete[] (*iter)->hdr_var.extrafield;
    }

    if((*iter)->data.buf!=NULL)
    {
      delete[] (*iter)->data.buf;
    }

    delete (*iter);
  }

  zLocalDataList.clear();

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Local headers list cleared\n");
  #endif
}

//-----------------------------------------------------------------------
//ClearCDirDataList
//Azzera la lista delle central directory
//
//Variabili di ingresso: nessuna
//-----------------------------------------------------------------------
void ZipClass::ClearCDirDataList(void)
{
  if(zCDirDataList.empty())
  {
    return;
  }

  list<sCDirData *>::iterator iter;

  for(iter=zCDirDataList.begin();iter!=zCDirDataList.end();iter++)
  {
    if((*iter)->hdr_var.filename!=NULL)
    {
      delete[] (*iter)->hdr_var.filename;
    }

    if((*iter)->hdr_var.extrafield!=NULL)
    {
      delete[] (*iter)->hdr_var.extrafield;
    }

    if((*iter)->hdr_var.filecomment!=NULL)
    {
      delete[] (*iter)->hdr_var.filecomment;
    }

    delete (*iter);
  }

  zCDirDataList.clear();

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Central directory headers list cleared\n");
  #endif
}

//-----------------------------------------------------------------------
//GetLocalDataSize
//Ritorna dimensioni totali di un local header
//
//Variabili di ingresso:
//  d: puntatore alla struttura dati
//Valore ritornato:
//  dimensione totale, in byte, del local header
//-----------------------------------------------------------------------
int ZipClass::GetLocalDataSize(sLocalData *d)
{
  return(LOCHDR_SIZE+d->hdr.filename_len+d->hdr.extrafield_len+d->data.len);
}

//-----------------------------------------------------------------------
//GetCDirDataSize
//Ritorna dimensioni totali di un elemento central directory
//
//Variabili di ingresso:
//  d: puntatore alla struttura dati
//Valore ritornato:
//  dimensione totale, in byte, della struttura dati
//-----------------------------------------------------------------------
int ZipClass::GetCDirDataSize(sCDirData *d)
{
  return(CDIRHDR_SIZE+d->hdr.filename_len+d->hdr.extrafield_len+d->hdr.filecomment_len);
}

//-----------------------------------------------------------------------
//SetSpanSize
//Setta dimensione disco per multidisk spanning
//
//Variabili di ingresso:
//  size : dimensione disco
//-----------------------------------------------------------------------
void ZipClass::SetSpanSize(unsigned int size)
{
  if(zOpenMode & ZIP_OPEN_SPAN)
  {
    if(size>0)
    {
      zSpan=size;
    }
  }
}

//-----------------------------------------------------------------------
//IsSpan
//Ritorna modo spanning
//
//Variabili di ingresso: nessuna
//Valore ritornato: 1 se il file e' aperto in modo spanning
//                  0 altrimenti
//-----------------------------------------------------------------------
int ZipClass::IsSpan(void)
{
  return(zSpan!=0);
}

//-----------------------------------------------------------------------
//WriteLocalFileData
//Scrive su file un local file header
//
//Variabili di ingresso:
//  locdata   : puntatore alla struttura dati da scrivere
//  locoffset : se specificato puntatore a variabile di ritorno
//              per offset local header
//Valore ritornato: QZIP_Ok se ok
//                  codice di errore altrimenti
//-----------------------------------------------------------------------
int ZipClass::WriteLocalFileData(sLocalData *locdata,unsigned int *locoffset)
{
  if(hFile==0)
  {
    return(QZIP_NOTOPEN);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nWriting local file header=%s\n",locdata->hdr_var.filename);

  fprintf(QZIP_DEBUG,"Version needed to extract=%d\n",locdata->hdr.ver_to_extract);
  fprintf(QZIP_DEBUG,"Method=%d\n",locdata->hdr.method);
  fprintf(QZIP_DEBUG,"Compressed size=%d\n",locdata->hdr.compressed_size);
  fprintf(QZIP_DEBUG,"Uncompressed size=%d\n",locdata->hdr.uncompressed_size);
  fprintf(QZIP_DEBUG,"Filename=%s\n",locdata->hdr_var.filename);
  #endif
  
  if(zSpan)
  {
    int size=SIGNATURE_SIZE+LOCHDR_SIZE+locdata->hdr.filename_len+locdata->hdr.extrafield_len;

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Disk avaiable space      =%d\n",zSpanAvail);
    fprintf(QZIP_DEBUG,"Space required for header=%d\n",size);
    #endif

    if(size>zSpanAvail)
    {
      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"New disk required\n");
      #endif

      //this local header cannot be writed to disk: set another disk

      zDisk++;
      int r=PromptForDisk(zDisk);

      if(r!=QZIP_OK)
      {
        return(r);
      }

      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"New disk is %d\n",zDisk);
      #endif
    }
  }

  if(locoffset!=NULL)
  {  
    *locoffset=lseek(hFile,0,SEEK_CUR);
  }

  //The local header must fit in the disk !

  unsigned int sign=SIGN_LOCAL_HEADER;

  if(write(hFile,&sign,SIGNATURE_SIZE)!=SIGNATURE_SIZE)
  {
    return(QZIP_WRITE_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Signature writed\n");
  #endif

  zSpanAvail-=SIGNATURE_SIZE;

  if(write(hFile,&(locdata->hdr),LOCHDR_SIZE)!=LOCHDR_SIZE)
  {
    return(QZIP_WRITE_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Header writed\n");
  #endif

  zSpanAvail-=LOCHDR_SIZE;

  if(locdata->hdr.filename_len)
  {
    if(write(hFile,locdata->hdr_var.filename,locdata->hdr.filename_len)!=locdata->hdr.filename_len)
    {
      return(QZIP_WRITE_ERR);
    }

    zSpanAvail-=locdata->hdr.filename_len;

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Filename writed\n");
    #endif
  }

  if(locdata->hdr.extrafield_len)
  {
    if(write(hFile,locdata->hdr_var.extrafield,locdata->hdr.extrafield_len)!=locdata->hdr.extrafield_len)
    {
      return(QZIP_WRITE_ERR);
    }

    zSpanAvail-=locdata->hdr.extrafield_len;

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Extrafield writed\n");
    #endif

  }

  //--------------------------------------------------------------------------

  //Local file data can be splitted in multidisk

  if(locdata->data.len)
  {
    int write_remain=locdata->data.len;
    int nwrite=write_remain;
    int pos=0;

    do
    {
      if(zSpan)
      {
        if(zSpanAvail==0)
        {
          #ifdef QZIP_DEBUG
          fprintf(QZIP_DEBUG,"No more space avaiable on disk %d\n",zDisk);
          #endif

          //No more space avaiable --> new disk
          
          zDisk++;
          int r=PromptForDisk(zDisk);

          if(r!=QZIP_OK)
          {
            return(r);
          }

          #ifdef QZIP_DEBUG
          fprintf(QZIP_DEBUG,"New disk is %d\n",zDisk);
          #endif
        }

        nwrite=zSpanAvail;

        if(nwrite>write_remain)
        {
          nwrite=write_remain;
        }

      }

      if(write(hFile,locdata->data.buf+pos,nwrite)!=nwrite)
      {
        return(QZIP_WRITE_ERR);
      }

      write_remain-=nwrite;

      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"Writed %d bytes remaining %d\n",nwrite,write_remain);
      #endif

      pos+=nwrite;

      if(zSpan)
      {
        zSpanAvail-=nwrite;

        #ifdef QZIP_DEBUG
        fprintf(QZIP_DEBUG,"%d bytes avaiable on disk %d\n",zSpanAvail,zDisk);
        #endif

      }

    } while(write_remain>0);

    //Flush compressed buffer: not needed anymore !
    delete[] locdata->data.buf;
    locdata->data.buf=NULL;
    locdata->data.len=0;

  }

  return(QZIP_OK);
}

//-----------------------------------------------------------------------
//GetLocalFileData
//Cerca il prossimo localheader e i dati ad esso associati, nel file.
//
//Variabili di ingresso:
//  locdata: puntatore alla struttura dati da riempire
//
//Valore ritornato: QZIP_OK se ok
//                  codice di errore altrimenti
//-----------------------------------------------------------------------
int ZipClass::GetLocalFileData(sLocalData *locdata)
{
  if(hFile==0)
  {
    return(QZIP_NOTOPEN);
  }

  unsigned int sign;

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nGet local file header\n");
  #endif

  if(read(hFile,&sign,SIGNATURE_SIZE)!=SIGNATURE_SIZE)
  {
    return(QZIP_READ_ERR);
  }

  if(sign!=SIGN_LOCAL_HEADER)
  {
    return(QZIP_NOLOCALHDR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Signature found\n");
  #endif

  lseek(hFile,0,SEEK_CUR);

  if(read(hFile,&(locdata->hdr),LOCHDR_SIZE)!=LOCHDR_SIZE)
  {
    return(QZIP_READ_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Header loaded\n");
  #endif

  if(locdata->hdr_var.filename!=NULL)
  {
    delete[] locdata->hdr_var.filename;
  }

  if(locdata->hdr_var.extrafield!=NULL)
  {
    delete[] locdata->hdr_var.extrafield;
  }  
  
  locdata->hdr_var.filename=NULL;
  locdata->hdr_var.extrafield=NULL;

  if(locdata->hdr.filename_len)
  {  
    locdata->hdr_var.filename=new char[locdata->hdr.filename_len+1];

    if(read(hFile,locdata->hdr_var.filename,locdata->hdr.filename_len)!=locdata->hdr.filename_len)
    {
      return(QZIP_READ_ERR);
    }

    locdata->hdr_var.filename[locdata->hdr.filename_len]='\0';

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Filename=%s\n",locdata->hdr_var.filename);
    #endif

  }

  if(locdata->hdr.extrafield_len)
  {
    locdata->hdr_var.extrafield=new char[locdata->hdr.extrafield_len];

    if(read(hFile,locdata->hdr_var.extrafield,locdata->hdr.extrafield_len)!=locdata->hdr.extrafield_len)
    {
      return(QZIP_READ_ERR);
    }
  }

  locdata->pos=lseek(hFile,0,SEEK_CUR);

  if(locdata->data.buf!=NULL)
  {
    delete[] locdata->data.buf;
  }

  locdata->data.buf=new unsigned char[locdata->hdr.compressed_size];
  locdata->data.len=0;

  int err=ReadDataBlock(locdata);

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"First datablock readed: %d of %d bytes\n",locdata->data.len,locdata->hdr.compressed_size);
  #endif

  return(err);
  
}
  
//-----------------------------------------------------------------------
//ReadDataBlock
//legge tutto o una parte dei dati associati ad un localheader
//
//Variabili di ingresso:
//  locdata   : puntatore alla struttura localheader a cui si fa riferimento
//
//Valore ritornato: QZIP_OK se ok
//                  codice di errore altrimenti
//-----------------------------------------------------------------------
int ZipClass::ReadDataBlock(sLocalData *locdata)
{
  if(hFile==0)
  {
    return(QZIP_NOTOPEN);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nRead next datablock\n");
  #endif

  int nread=locdata->hdr.compressed_size-locdata->data.len;

  if(locdata->data.len==locdata->hdr.compressed_size)
  {
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"No more data blocks\n");
    #endif
    
    return(QZIP_OK);
  }

  int pos=lseek(hFile,0,SEEK_CUR);

  //if current position is equal to file size: data block continues on
  //next disk

  if(pos==filelength(hFile))
  {
    zDisk++;
    int err=PromptForDisk(zDisk);

    if(err!=QZIP_OK)
    {
      return(err);
    }

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"End of file reached: get new disk %d\n",zDisk);
    #endif

    pos=0;
  }

  //can't read all remaining data: read maximum for this disk
  if((pos+nread)>=filelength(hFile))
  {
    nread=filelength(hFile)-pos;
  }

  pos+=nread;  

  if(read(hFile,locdata->data.buf+locdata->data.len,nread)!=nread)
  {
    return(QZIP_READ_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"%d bytes readed\n",nread);
  #endif

  locdata->data.len+=nread;

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Total bytes %d readed of %d\n",locdata->data.len,locdata->hdr.compressed_size);
  #endif

  return(QZIP_OK);
}

//-----------------------------------------------------------------------
//WriteCDirData
//Scrive su file un elemento central directory
//
//Variabili di ingresso:
//  cdir_data: puntatore alla struttura dati da scrivere
//
//Valore ritornato: numero di byte scritti se ok (>0)
//                  codice di errore altrimenti
//-----------------------------------------------------------------------
int ZipClass::WriteCDirData(sCDirData *cdir_data)
{
  if(hFile==0)
  {
    return(QZIP_NOTOPEN);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nWrite central directory data\n");
  fprintf(QZIP_DEBUG,"Compressed size=%d\n",cdir_data->hdr.compressed_size);
  fprintf(QZIP_DEBUG,"Uncompressed size=%d\n",cdir_data->hdr.uncompressed_size);
  fprintf(QZIP_DEBUG,"Disk start=%d\n",cdir_data->hdr.diskstart);
  fprintf(QZIP_DEBUG,"Local header offset=%d\n",cdir_data->hdr.localhdr_offset);
  fprintf(QZIP_DEBUG,"Central directory crc=%X\n",cdir_data->hdr.crc32);
  #endif
  
  if(zSpan)
  {
    int size=SIGNATURE_SIZE+CDIRHDR_SIZE+cdir_data->hdr.filename_len+cdir_data->hdr.extrafield_len+cdir_data->hdr.filecomment_len;

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Space required for central directory header=%d\n",size);
    fprintf(QZIP_DEBUG,"Space avaiable on disk=%d\n",zSpanAvail);
    #endif

    if(size>zSpanAvail)
    {
      //this local header cannot be writed to disk: set another disk

      zDisk++;
      int r=PromptForDisk(zDisk);

      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"Currenti disk is %d\n",zDisk);
      #endif

      if(r!=QZIP_OK)
      {
        return(r);
      }

      zCDirEnd->hdr.nentry_cdir=0;
    }
  }

  if((short)zCDirEnd->hdr.disk_startcd==-1)
  {
    zCDirEnd->hdr.disk_startcd=zDisk;
  }

  if((short)zCDirEnd->hdr.cdir_offset==-1)
  {
    zCDirEnd->hdr.cdir_offset=lseek(hFile,0,SEEK_CUR);
  }

  //The local header must fit in the disk !

  unsigned int sign=SIGN_CENTRAL_DIRECTORY;

  if(write(hFile,&sign,SIGNATURE_SIZE)!=SIGNATURE_SIZE)
  {
    return(QZIP_WRITE_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Signature writed\n");
  #endif

  zSpanAvail-=SIGNATURE_SIZE;

  if(write(hFile,&(cdir_data->hdr),CDIRHDR_SIZE)!=CDIRHDR_SIZE)
  {
    return(QZIP_WRITE_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Central directory header writed\n");
  #endif

  zSpanAvail-=CDIRHDR_SIZE;

  if(cdir_data->hdr.filename_len)
  {
    if(write(hFile,cdir_data->hdr_var.filename,cdir_data->hdr.filename_len)!=cdir_data->hdr.filename_len)
    {
      return(QZIP_WRITE_ERR);
    }

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Filename=%s\n",cdir_data->hdr_var.filename);
    #endif

    zSpanAvail-=cdir_data->hdr.filename_len;
  }

  if(cdir_data->hdr.extrafield_len)
  {
    if(write(hFile,cdir_data->hdr_var.extrafield,cdir_data->hdr.extrafield_len)!=cdir_data->hdr.extrafield_len)
    {
      return(QZIP_WRITE_ERR);
    }

    zSpanAvail-=cdir_data->hdr.extrafield_len;
  }

  if(cdir_data->hdr.filecomment_len)
  {
    if(write(hFile,cdir_data->hdr_var.filecomment,cdir_data->hdr.filecomment_len)!=cdir_data->hdr.filecomment_len)
    {
      return(QZIP_WRITE_ERR);
    }

    zSpanAvail-=cdir_data->hdr.filecomment_len;
  }

  zCDirEnd->hdr.nentry_cdir++;

  return(QZIP_OK);
}

//-----------------------------------------------------------------------
//GetAllCentralDirData
//Legge tutte le strutture dati central directory
//
//Variabili di ingresso: nessuna
//
//Valore ritornato: QZIP_OK se ok
//                  codice di errore altrimenti
//-----------------------------------------------------------------------
int ZipClass::GetAllCentralDirData(void)
{
  if(hFile==0)
  {
    return(QZIP_NOTOPEN);
  }

  if(zCDirEnd==NULL)
  {
    return(QZIP_INTERNAL_ERR);
  }

  sCDirData *cdir;

  int count=0;

  do
  {
    cdir=AllocCDirData();

    int stat=GetCentralDirData(cdir);

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Get central directory %d status=%d\n",count,stat);
    #endif

    if(stat==QZIP_NOCDIRHDR)
    {
      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"No more central directories found on current disk\n");
      #endif
      
      if(zDisk==zCDirEnd->hdr.disk)
      {
        #ifdef QZIP_DEBUG
        fprintf(QZIP_DEBUG,"Last disk reached:end\n");
        #endif
        break;
      }
      
      zDisk++;

      stat=PromptForDisk(zDisk);

      if(stat!=QZIP_OK)
      {
        return(stat);
      }

      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"Pass to disk %d\n",zDisk);
      #endif
    }
    else
    {
      count++;
      zCDirDataList.push_back(cdir);

      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"Central directory header found\n");
      #endif

      if(zCDirEnd->hdr.nentry_cdir_tot==count)
      {
        #ifdef QZIP_DEBUG
        fprintf(QZIP_DEBUG,"All central directories readed\n");
        #endif
        break;
      }
    }
    
  } while(1);

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Central directories %d of %d readed\n",count,zCDirEnd->hdr.nentry_cdir_tot);
  #endif

  if(zCDirEnd->hdr.nentry_cdir_tot!=count)
  {
    return(QZIP_FORMAT_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Reading central directory headers ok\n",count);
  #endif

  return(QZIP_OK);
}

//-----------------------------------------------------------------------
//GetCentralDirData
//Cerca il prossimo central directory header e i dati ad esso associati,
//nel file.
//
//Variabili di ingresso:
//  cdir_data: puntatore alla struttura dati da riempire
//
//Valore ritornato: QZIP_OK se ok
//                  codice di errore altrimenti
//-----------------------------------------------------------------------
int ZipClass::GetCentralDirData(sCDirData *cdir_data)
{
  if(hFile==0)
  {
    return(QZIP_NOTOPEN);
  }

  int curpos=lseek(hFile,0,SEEK_CUR);


  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nGet central directory at %d of disk %d\n",curpos,zDisk);
  #endif

  if((curpos+SIGNATURE_SIZE)>filelength(hFile))
  {
    //End of file reached

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"End of file reached\n");
    #endif
    
    return(QZIP_NOCDIRHDR);
  }

  unsigned int sign;

  if(read(hFile,&sign,SIGNATURE_SIZE)!=SIGNATURE_SIZE)
  {
    return(QZIP_READ_ERR);
  }

  if(sign!=SIGN_CENTRAL_DIRECTORY)
  {
    return(QZIP_NOCDIRHDR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Signature ok\n");
  #endif

  cdir_data->pos=lseek(hFile,0,SEEK_CUR);
  
  if(read(hFile,&(cdir_data->hdr),CDIRHDR_SIZE)!=CDIRHDR_SIZE)
  {
    return(QZIP_READ_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Central directory header readed\n");
  #endif

  if(cdir_data->hdr_var.filename!=NULL)
  {
    delete[] cdir_data->hdr_var.filename;
  }

  if(cdir_data->hdr_var.extrafield!=NULL)
  {
    delete[] cdir_data->hdr_var.extrafield;
  }

  if(cdir_data->hdr_var.filecomment!=NULL)
  {
    delete[] cdir_data->hdr_var.filecomment;
  }

  if(cdir_data->hdr.filename_len)
  {  
    cdir_data->hdr_var.filename=new char[cdir_data->hdr.filename_len+1];

    if(read(hFile,cdir_data->hdr_var.filename,cdir_data->hdr.filename_len)!=cdir_data->hdr.filename_len)
    {
      return(QZIP_READ_ERR);
    }

    cdir_data->hdr_var.filename[cdir_data->hdr.filename_len]='\0';

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Filename=%s\n",cdir_data->hdr_var.filename);
    #endif
  }

  if(cdir_data->hdr.extrafield_len)
  {  
    cdir_data->hdr_var.extrafield=new char[cdir_data->hdr.extrafield_len];

    if(read(hFile,cdir_data->hdr_var.extrafield,cdir_data->hdr.extrafield_len)!=cdir_data->hdr.extrafield_len)
    {
      return(QZIP_READ_ERR);
    }
  }

  if(cdir_data->hdr.filecomment_len)
  {  
    cdir_data->hdr_var.filecomment=new char[cdir_data->hdr.filecomment_len+1];

    if(read(hFile,cdir_data->hdr_var.filecomment,cdir_data->hdr.filecomment_len)!=cdir_data->hdr.filecomment_len)
    {
      return(QZIP_READ_ERR);
    }

    cdir_data->hdr_var.filecomment[cdir_data->hdr.filecomment_len]='\0';
  }

  return(QZIP_OK);
}

//-----------------------------------------------------------------------
//WriteCentralDirEnd
//Scrive su file central directory end
//
//Variabili di ingresso:
//  cdir_end: puntatore alla struttura dati da scrivere
//
//Valore ritornato: numero di byte scritti se ok (>0)
//                  codice di errore altrimenti
//-----------------------------------------------------------------------
int ZipClass::WriteCentralDirEnd(sCDirEnd *cdir_end)
{
  if(hFile==0)
  {
    return(QZIP_NOTOPEN);
  }

  if(zSpan)
  {
    int size=SIGNATURE_SIZE+CDIREND_SIZE+cdir_end->hdr.zipcomment_len;

    if(size>zSpanAvail)
    {
      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"New disk required\n");
      #endif

      //this local header cannot be writed to disk: set another disk

      zDisk++;
      int r=PromptForDisk(zDisk);

      if(r!=QZIP_OK)
      {
        return(r);
      }
    }
  }

  cdir_end->hdr.disk=zDisk;

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nWrite central directory end\n");
  fprintf(QZIP_DEBUG,"this disk=%d\n",cdir_end->hdr.disk);
  fprintf(QZIP_DEBUG,"start central directory disk=%d\n",cdir_end->hdr.disk_startcd);
  fprintf(QZIP_DEBUG,"Central directory entry on this disk=%d\n",cdir_end->hdr.nentry_cdir);
  fprintf(QZIP_DEBUG,"Total Central directory=%d\n",cdir_end->hdr.nentry_cdir_tot);
  fprintf(QZIP_DEBUG,"Central directory size=%d\n",cdir_end->hdr.cdir_size);
  fprintf(QZIP_DEBUG,"Central directory offset=%d\n",cdir_end->hdr.cdir_offset);
  #endif  

  unsigned int sign=SIGN_END_CENTRAL_DIRECTORY;

  if(write(hFile,&sign,SIGNATURE_SIZE)!=SIGNATURE_SIZE)
  {
    return(QZIP_WRITE_ERR);
  }

  int writed=SIGNATURE_SIZE;  

  if(write(hFile,&(cdir_end->hdr),CDIREND_SIZE)!=CDIREND_SIZE)
  {
    return(QZIP_WRITE_ERR);
  }

  writed+=CDIREND_SIZE;

  if(cdir_end->hdr.zipcomment_len)
  {
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Zip comment=%s\n",cdir_end->hdr_var.zipcomment);
    #endif

    if(write(hFile,cdir_end->hdr_var.zipcomment,cdir_end->hdr.zipcomment_len)!=cdir_end->hdr.zipcomment_len)
    {
      return(QZIP_WRITE_ERR);
    }

    writed+=cdir_end->hdr.zipcomment_len;
  }

  return(writed);
}


//-----------------------------------------------------------------------
//GetCentralDirData
//Cerca il central directory end nel file
//
//Variabili di ingresso:
//  cdir_end: puntatore alla struttura dati da riempire
//
//Valore ritornato: QZIP_OK se ok
//                  codice di errore altrimenti
//-----------------------------------------------------------------------
int ZipClass::GetCentralDirEnd(sCDirEnd *cdir_end)
{
  if(hFile==0)
  {
    return(QZIP_NOTOPEN);
  }

  unsigned char data;
  int found=0,count=0;

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nSearch central directory end\n");
  #endif

  int p=1;

  while(1)
  {
    lseek(hFile,-p,SEEK_END);

    unsigned char search_for=((SIGN_END_CENTRAL_DIRECTORY >> ((3-count)*8))) & 0xFF;

    int r;

    if((r=read(hFile,&data,1))!=1)
    {
      break;
    }

    if(data==search_for)
    {
      count++;

      if(count==SIGNATURE_SIZE)
      {
        found=1;
        break;
      }
    }
    else
    {
      count=0;
    }

    p++;
  }

  if(!found)
  {
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"No central directory end found on disk %d\n",zDisk);
    #endif
    
    return(QZIP_NOCDIREND);
  }

  lseek(hFile,SIGNATURE_SIZE-1,SEEK_CUR);

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Central directory end found on disk %d\n",zDisk);
  #endif

  cdir_end->pos=lseek(hFile,0,SEEK_CUR);
  
  if(read(hFile,&(cdir_end->hdr),CDIREND_SIZE)!=CDIREND_SIZE)
  {
    return(QZIP_READ_ERR);
  }

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Central directory header loaded\n");
  #endif

  if(cdir_end->hdr_var.zipcomment!=NULL)
  {
    delete[] cdir_end->hdr_var.zipcomment;
  }

  if(cdir_end->hdr.zipcomment_len)
  {  
    cdir_end->hdr_var.zipcomment=new char[cdir_end->hdr.zipcomment_len+1];

    if(read(hFile,cdir_end->hdr_var.zipcomment,cdir_end->hdr.zipcomment_len)!=cdir_end->hdr.zipcomment_len)
    {
      return(QZIP_READ_ERR);
    }

    cdir_end->hdr_var.zipcomment[cdir_end->hdr.zipcomment_len]='\0';
  }

  return(QZIP_OK);  
}


//-----------------------------------------------------------------------
//ZipStreamFlush
//Sposta i dati compressi dal buffer di compressione alla lista dei blocchi
//di dati temporanei o su file se specificato con zs_FlushDest
//
//Variabili di ingresso: nessuna
//Valore ritornato     : QZIP_OK se ok
//                       errorcode altrimenti
//-----------------------------------------------------------------------
int ZipClass::ZipStreamFlush(void)
{
  if(!zs_init)
  {
    return(QZIP_NOINIT_ZIP);
  }

  if(zs.avail_out==Z_BUFSIZE)
  {
    return(QZIP_OK);
  }
  
  if(!zs_FlushDest)
  {
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Flushing to temporaly blocks %d bytes\n",Z_BUFSIZE-zs.avail_out);
    #endif
    
    AddToTempBlocks(zs_buffer,Z_BUFSIZE-zs.avail_out);
  }
  else  
  {
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Flushing to file %d bytes\n",Z_BUFSIZE-zs.avail_out);
    #endif

    if(write(zs_FlushDest,zs_buffer,Z_BUFSIZE-zs.avail_out)!=Z_BUFSIZE-zs.avail_out)
    {
      return(QZIP_WRITE_ERR);
    }
  }

  zs.avail_out = (uInt)Z_BUFSIZE;
  zs.next_out = zs_buffer;

  return(QZIP_OK);
}


//-----------------------------------------------------------------------
//CompressInit
//Inizializza le strutture dati per la compressione di un elemento
//
//Variabili di ingresso: nessuna
//
//-----------------------------------------------------------------------
int ZipClass::CompressInit(int level,int method)
{
  if(zs_buffer)
  {
    delete[] zs_buffer;
  }
  
  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nInitializing compression routine\n");
  #endif

  zs_buffer=new unsigned char[Z_BUFSIZE];

  zs.avail_in = (uInt)0;
  zs.avail_out = (uInt)Z_BUFSIZE;
  zs.next_out = zs_buffer;
  zs.total_in = 0;
  zs.total_out = 0;

  zs.zalloc = (alloc_func)0;
  zs.zfree = (free_func)0;
  zs.opaque = (voidpf)0;

  if(!zTempList.empty())
  {
    zTempList.clear();
  }

  int err = deflateInit2(&zs,level,method,-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);

  if (err==Z_OK)
  {
    zs_init = 1;
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Initialization ok\n");
    #endif
  }
  else
  {
    zs_init=0;
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Initialization error\n");
    #endif
  }

  return(err);
}

//-----------------------------------------------------------------------
//Compress
//Comprime i dati presenti in un buffer
//
//Variabili di ingresso:
//  inbuf: puntatore a buffer da comprimere
//  len  : lunghezza del buffer da comprimere
//
//-----------------------------------------------------------------------
int ZipClass::CompressBuffer(unsigned char *inbuf,int len)
{
  if(!zs_init)
  {
    return(QZIP_NOINIT_ZIP);
  }

  zs.next_in=inbuf;
  zs.avail_in=len;

  zCrc32=crc32(zCrc32,inbuf,len);

  int err=Z_OK;

  zs.avail_out = (uInt)Z_BUFSIZE;
  zs.next_out = zs_buffer;

  int is_outbuf_full=0;

  do
  {

    err=deflate(&zs,Z_NO_FLUSH);

    //se avail_out=0 puo' significare che:

    // - non tutti i dati sono stati elaborati per l'esaurimento
    //   dell'output buffer

    // - tutti i dati sono stati elaborati arrivando all'esaurimento
    //   del buffer di uscita

    // nel primo caso e' necessario rieseguire deflate fino a che tutti i dati
    // siano stati elaborati (nessun cambiamento di avail_out tra prima e dopo
    // il deflate

    // nel secondo caso si puo' uscire subito dal ciclo; per comodita' il
    // ciclo viene comunque ripetuto ma avendo deflate terminato l'elaborazione
    // il buffer di uscita non verra' consumato.

    if(err==Z_OK)
    {
      //prima di un'eventuale svuotamento del buffer di uscita e' necessario
      //memorizzare l'informazione buffer pieno che verrebbe persa con
      //un flushing

      if(zs.avail_out==0)
      {
        is_outbuf_full=1;
      }
      else
      {
        is_outbuf_full=0;
      }

      //procede quindi a svuotare il buffer di uscita (se non e' vuoto...)
      ZipStreamFlush();      
    }
    else
    {
      break;
    }

    //ripete se il buffer era stato completamente riempito dalla deflate:
    //potrebbero esserci altri dati pronti nel buffer interno alla libreria
    //che attendono di essere inseriti nel buffer di uscita

  } while(is_outbuf_full);

  return err;

}

//-----------------------------------------------------------------------
//CompressEnd
//Termina le operazione di compressione
//
//Variabili di ingresso: nessuna
//-----------------------------------------------------------------------
int ZipClass::CompressEnd(void)
{
  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nClosing compression routines\n");
  #endif

  /*
  if(zs.avail_out==0)
  {
    ZipStreamFlush();
  }
  */

  zs.avail_in=0;

  int err=Z_OK;

  do
  {
    zs.avail_out = (uInt)Z_BUFSIZE;
    zs.next_out = zs_buffer;

    err=deflate(&zs,Z_FINISH);

    if((err==Z_STREAM_END) || (err==Z_OK))
    {
      ZipStreamFlush();
    }
    else
    {
      //error
      break;
    }

    //termina quando non ci sono ulteriori dati da elaborare da parte
    //della deflate o in caso di errore

  } while(err!=Z_STREAM_END);

  err=deflateEnd(&zs);
  zs_init = 0;

  delete[] zs_buffer;
  zs_buffer=NULL;

  return(err);
}

//-----------------------------------------------------------------------
//CompressFile
//Comprime un file aggiornando le liste localheade e central directory
//
//Variabili di ingresso:
//  file  : nome del file completo di pathname
//  lelvel: livello di compressione
//  remove_pathname : memorizza il file senza il pathname completo (default = false)
//
//Valore ritornato     : QZIP_OK se nessun errore
//                       error code altrimenti
//-----------------------------------------------------------------------

#define DOSTIME_MINIMUM	((unsigned int)0x00210000) // 01/01/1980 00:00:00

/* Convert the date y/n/d and time h:m:s to a four byte DOS date and
   time (date in high two bytes, time in low two bytes allowing magnitude
   comparison). */
unsigned int dostime(int y, int n, int d, int h, int m, int s)
{
  return y < 1980 ? DOSTIME_MINIMUM /* dostime(1980, 1, 1, 0, 0, 0) */ :
		  (((unsigned int)y - 1980) << 25) | ((unsigned int)n << 21) | ((unsigned int)d << 16) |
		  ((unsigned int)h << 11) | ((unsigned int)m << 5) | ((unsigned int)s >> 1);
}

/* Return the Unix time t in DOS format, rounded up to the next two
   second boundary. */
unsigned int unix2dostime(time_t *t)
{
	time_t t_even;
	struct tm *s;         /* result of localtime() */

	t_even = (time_t)(((unsigned int)(*t) + 1) & (~1));
	/* Round up to even seconds. */
	s = localtime(&t_even);       /* Use local time since MSDOS does. */
	if (s == (struct tm *)NULL) {
      /* time conversion error; use current time as emergency value
		(assuming that localtime() does at least accept this value!) */
		t_even = (time_t)(((unsigned int)time(NULL) + 1) & (~1));
		s = localtime(&t_even);
	}
	return dostime(s->tm_year + 1900, s->tm_mon + 1, s->tm_mday,
				   s->tm_hour, s->tm_min, s->tm_sec);
}



int ZipClass::CompressFile(const char *file,int level,bool remove_pathname)
{
	#ifdef QZIP_DEBUG
	fprintf(QZIP_DEBUG,"\nCompress file=%s\n",file);
	#endif
	
	if((access(file,F_OK)) && !CheckDirectory(file))
	{
		return(QZIP_NOTFOUND);
	}
	
	#ifdef QZIP_DEBUG
	fprintf(QZIP_DEBUG,"Compression of %s started\n",file);
	#endif
	
	int store_dir=0;
	
	if(CheckDirectory(file))
	{
		#ifdef QZIP_DEBUG
		fprintf(QZIP_DEBUG,"%s is a directory: will be stored\n",file);
		#endif
		
		store_dir=1;
	
	}

	int hf = -1;
	
	if(!store_dir)
	{
		if((hf=FilesFunc_open(file,O_RDONLY,S_IREAD))<0)
		{
			return(QZIP_OPEN_ERR);
		}
		
		#ifdef QZIP_DEBUG
		fprintf(QZIP_DEBUG,"%s opened\n",file);
		#endif
	}

	sLocalData *zLocData=AllocLocalFileData();
	
	zLocData->hdr.ver_to_extract=VERSION_NEEDED;
	
	zLocData->hdr.gp_flag=COMPRESSION_NORMAL;
	
	if((level==8) || (level==9))
	{
		zLocData->hdr.gp_flag=COMPRESSION_MAXIMUM;
	}
	
	if(level==2)
	{
		zLocData->hdr.gp_flag=COMPRESSION_FAST;
	}
	
	if(level==1)
	{
		zLocData->hdr.gp_flag=COMPRESSION_SUPERFAST;
	}

	zCrc32=crc32(0L,NULL,0);   
	
	struct stat st;    
	stat(file,&st);
		
	unsigned int dostime=unix2dostime(&st.st_mtime);
	
	zLocData->hdr.last_mod_time=dostime & 0xffff;
	zLocData->hdr.last_mod_date=(dostime >> 16);
	zLocData->hdr.crc32=zCrc32;
	
	if(!store_dir)
	{
		zLocData->hdr.uncompressed_size=filelength(hf);
	}
	else
	{
		zLocData->hdr.uncompressed_size=0;
	}
    
	//------------------------------------------------------------------------------------------------------
	//Linux extra fields
	
	zLocData->hdr.extrafield_len=sizeof(sLocalExtraField_extendedTS) + sizeof(sLocalExtraField_unix);
	zLocData->hdr_var.extrafield=new char[zLocData->hdr.extrafield_len];
	char* ptrLocalExtraField = zLocData->hdr_var.extrafield;
	
	sLocalExtraField_extendedTS extTS;
	memcpy(extTS.tag,EXTRAFIELDTAG_EXTTS,2);
	extTS.size = LOC_EXTRAFIELDSIZE_EXTTS;
	extTS.flags = EXTRAFIELD_EXTTS_FLAGS;
	
	extTS.modtime = st.st_mtime;
	extTS.lastacc = st.st_atime;
	
	memcpy(ptrLocalExtraField, &extTS, sizeof(extTS));
	ptrLocalExtraField+=sizeof(extTS);
	
	sLocalExtraField_unix extUnix;
	memcpy(extUnix.tag,EXTRAFIELDTAG_UNIX,2);
	extUnix.size = LOC_EXTRAFIELDSIZE_UNIX;  
	extUnix.uid = st.st_uid & 0xffff;
	extUnix.gid = st.st_gid & 0xffff;
	
	memcpy(ptrLocalExtraField, &extUnix, sizeof(extUnix));
   
  	//------------------------------------------------------------------------------------------------------        
  
	zLocData->hdr.filename_len=strlen(file);
	
	if(store_dir)
	{
		zLocData->hdr.filename_len++;
	}

	
	zLocData->hdr_var.filename=new char[zLocData->hdr.filename_len+1];
	
	if(store_dir)
	{
		strncpyQ(zLocData->hdr_var.filename,file,zLocData->hdr.filename_len-1);
		zLocData->hdr_var.filename[zLocData->hdr.filename_len-1]='\0';
		strcat(zLocData->hdr_var.filename,"/");
	}
	else
	{		
		strncpyQ(zLocData->hdr_var.filename,file,zLocData->hdr.filename_len);
		zLocData->hdr_var.filename[zLocData->hdr.filename_len]='\0';					
		
		if(remove_pathname)
		{
			char* ptr = zLocData->hdr_var.filename;
			char* last_slash = NULL;
			while(*ptr != '\0') 
			{
				if(*ptr == '/')
				{
					last_slash = ptr;
				}
				ptr++;
			}
			if(last_slash != NULL)
			{
				last_slash++;
				strcpy(zLocData->hdr_var.filename,last_slash);
				zLocData->hdr.filename_len = strlen(zLocData->hdr_var.filename);				
			}
		}			
	}
	
	zLocData->hdr.compressed_size=0;
	
	if(!store_dir)
	{
		zLocData->hdr.method=Z_DEFLATED;
	
		if(CompressInit(level,zLocData->hdr.method)!=Z_OK)
		{
			FreeLocalFileData(zLocData);
			FilesFunc_close(hf);
			return(QZIP_INTERNAL_ERR);
		}

		int nread=Z_BUFSIZE;
		int size=zLocData->hdr.uncompressed_size;
	
		unsigned char buffer[Z_BUFSIZE];
	
		do
		{
			if(nread>size)
			{
				nread=size;
			}
		
			int readed = read(hf,buffer,nread);
			if(readed!=nread)
			{
				FreeLocalFileData(zLocData);
				FilesFunc_close(hf);
				return(QZIP_READ_ERR);
			}

			CompressBuffer(buffer,nread);
		
			zLocData->hdr.crc32=zCrc32;
		
			size-=nread;
		
			#ifdef QZIP_DEBUG
			fprintf(QZIP_DEBUG,"%d bytes compressed. %d remaining\n",nread,size);
			#endif
    
    	} while(size>0);

		CompressEnd();
	
		zLocData->hdr.compressed_size=zs.total_out;
	
		zLocData->data.buf=FlushTempBlocks(zLocData->data.len);
	}
	else
	{
		zLocData->hdr.compressed_size=0;
		zLocData->data.buf=NULL;
		zLocData->data.len=0;
		zLocData->hdr.method=Z_NO_COMPRESSION;
	
		#ifdef QZIP_DEBUG
		fprintf(QZIP_DEBUG,"Stored\n");
		#endif
  	}	

	zLocalDataList.push_back(zLocData);
	
	sCDirData *zCDirData=AllocCDirData();
	
	zCDirData->hdr.version_madeby=VERSIONMADEBY;
	zCDirData->hdr.version_needed=VERSION_NEEDED;
	
	zCDirData->hdr.gp_flag=zLocData->hdr.gp_flag;
	zCDirData->hdr.method=zLocData->hdr.method;
	zCDirData->hdr.last_mod_time=zLocData->hdr.last_mod_time;
	zCDirData->hdr.last_mod_date=zLocData->hdr.last_mod_date;
	zCDirData->hdr.crc32=zLocData->hdr.crc32;
	zCDirData->hdr.compressed_size=zLocData->hdr.compressed_size;
	zCDirData->hdr.uncompressed_size=zLocData->hdr.uncompressed_size;
	zCDirData->hdr.filename_len=zLocData->hdr.filename_len;
	zCDirData->hdr.filecomment_len=0;

	//nei bit piu significativi gli attributi linux del file
	zCDirData->hdr.external_attrib = (st.st_mode << 16);
	//nei bit meno significativi gli attributi in formato dos
	zCDirData->hdr.external_attrib|= !(st.st_mode & S_IWUSR); //bit 0 = sola lettura
	if(S_ISDIR(st.st_mode))
	{
		zCDirData->hdr.external_attrib|= MSDOS_ATTRIB_DIR; //bit 8 = directory
	}
	zCDirData->hdr.internal_attrib=0;
	
	//Set those before write to file
	zCDirData->hdr.localhdr_offset=0;
	zCDirData->hdr.diskstart=0;

	zCDirData->hdr_var.filename=new char[zCDirData->hdr.filename_len+1];
	
	//------------------------------------------------------------------------------------------------------
	//Linux extra fields  
	zCDirData->hdr.extrafield_len=sizeof(sCDataExtraField_extendedTS) + sizeof(sCDataExtraField_unix);  
	zCDirData->hdr_var.extrafield=new char[zCDirData->hdr.extrafield_len];
	
	char* ptrCDExtraField = zCDirData->hdr_var.extrafield;
	
	sCDataExtraField_extendedTS extCDTS;
	memcpy(extCDTS.tag,EXTRAFIELDTAG_EXTTS,2);
	extCDTS.size = CDATA_EXTRAFIELDSIZE_EXTTS;
	extCDTS.flags = EXTRAFIELD_EXTTS_FLAGS;  
	extCDTS.modtime = st.st_mtime;  
	memcpy(ptrCDExtraField, &extCDTS, sizeof(extCDTS));
	
	ptrCDExtraField+=sizeof(extCDTS);
	
	sCDataExtraField_unix extCDUnix;
	memcpy(extUnix.tag,EXTRAFIELDTAG_UNIX,2);
	extCDUnix.size = CDATA_EXTRAFIELDSIZE_UNIX;  
	
	memcpy(ptrCDExtraField, &extCDUnix, sizeof(extCDUnix));  
  
	//------------------------------------------------------------------------------------------------------  
	
	zCDirData->hdr_var.filecomment=NULL;
	
	strncpyQ(zCDirData->hdr_var.filename,zLocData->hdr_var.filename,zCDirData->hdr.filename_len);
	zCDirData->hdr_var.filename[zCDirData->hdr.filename_len]='\0';
	
	zCDirDataList.push_back(zCDirData);
	
	if(!store_dir)
	{
		FilesFunc_close(hf);
	
		#ifdef QZIP_DEBUG
		fprintf(QZIP_DEBUG,"File %s closed\n",file);
		#endif
	}
	
	return(QZIP_OK);

}

//-----------------------------------------------------------------------
//CompressDir
//Comprime una intera directory
//
//Variabili di ingresso:
//  dir  : directory da comprimere
//  level: livello di compressione
//
//Valore ritornato     : QZIP_OK se nessun errore
//                       error code altrimenti
//-----------------------------------------------------------------------
int ZipClass::CompressDir(const char *dir,int level)
{
  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nCompress directory %s\n",dir);
  #endif

  if(!CheckDirectory(dir))
  {
    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Directory not found\n");
    #endif

    return(QZIP_NOTFOUND);
  }

  int err=CompressFile(dir,level);

  if(err!=QZIP_OK)
  {
    return(err);
  }

  struct ffblk ffblk;

  char buffer[MAXNPATH];

  strcpy(buffer,dir);
  strcat(buffer,"/*.*");

  int done=findfirst(buffer,&ffblk,FA_DIREC);

  while(!done)
  {
    err=QZIP_OK;

    snprintf(buffer,sizeof(buffer),"%s/%s",dir,ffblk.ff_name);

	if(S_ISDIR(ffblk.ff_attrib) && (ffblk.ff_name[0]!='.'))
    {
      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"Compressing dir %s\n",buffer);
      #endif
      
      err=CompressDir(buffer,level);
    }

	if(!S_ISDIR(ffblk.ff_attrib))
    {
      #ifdef QZIP_DEBUG
      fprintf(QZIP_DEBUG,"Compressing file %s\n",buffer);
      #endif

      err=CompressFile(buffer,level);

      if(fProgress[QZIP_DIRCOMP])
      {
        fProgress[QZIP_DIRCOMP](NULL);
      }

    }

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Compress result=%d\n",err);
    #endif

    if(err!=QZIP_OK)
    {
		end_findfirst(&ffblk);
		return(err);
    }

    done=findnext(&ffblk);

  }

  return(QZIP_OK);

}

//-----------------------------------------------------------------------
//WriteToZip
//Scrive su file i dati compressi
//
//Variabili di ingresso:
//  file  : nome del file completo di pathname
//  lelvel: livello di compressione
//
//Valore ritornato     : QZIP_OK se nessun errore
//                       error code altrimenti
//-----------------------------------------------------------------------
int ZipClass::WriteToZip(void)
{
  int nentry=0;

  int size_loc=0;
  int size_cd=0;

  zWriting=1;

  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nWriting buffered zip struture to file\n");
  #endif

  if(!zSpan)
  {
    if(zDisk>0)
    {
      return(QZIP_INTERNAL_ERR);
    }
  }

  if(zDisk==-1)
  {
    lseek(hFile,0,SEEK_SET);
    zSpanAvail=0;
  }

  list<sLocalData *>::iterator iter1;
  list<sCDirData *>::iterator iter2;
  
  //check lists and calc size
  for(iter1=zLocalDataList.begin(),iter2=zCDirDataList.begin();iter1!=zLocalDataList.end() && iter2!=zCDirDataList.end();iter1++,iter2++)
  {
    size_loc+=GetLocalDataSize(*iter1)+SIGNATURE_SIZE;

    size_cd+=GetCDirDataSize(*iter2)+SIGNATURE_SIZE;

    nentry++;
  }

  zCDirEnd->hdr.cdir_size=size_cd;

  if((nentry!=zLocalDataList.size()) || (nentry!=zCDirDataList.size()))
  {
     return(QZIP_INTERNAL_ERR);
  }

  for(iter1=zLocalDataList.begin(),iter2=zCDirDataList.begin();iter1!=zLocalDataList.end();iter1++,iter2++)
  {
    if(zDisk!=-1)
    {
      (*iter2)->hdr.diskstart=zDisk;
    }
    else
    {
      (*iter2)->hdr.diskstart=0;
    }

    int err=WriteLocalFileData(*iter1,&((*iter2)->hdr.localhdr_offset));

    if(err!=QZIP_OK)
    {
      return(err);
    }

    if(zSpan)
    {
      if(zSpanAvail<0)
      {
        return(QZIP_INTERNAL_ERR);
      }
    }
  }

  return(QZIP_OK);
}

//-----------------------------------------------------------------------
//WriteToZipEnd
//Scrive su file i dati compressi
//
//Variabili di ingresso:
//  file  : nome del file completo di pathname
//  lelvel: livello di compressione
//
//Valore ritornato     : QZIP_OK se nessun errore
//                       error code altrimenti
//-----------------------------------------------------------------------
int ZipClass::WriteToZipEnd(void)
{
  list<sCDirData *>::iterator iter;

  zCDirEnd->hdr.disk_startcd=(short)-1;
  zCDirEnd->hdr.cdir_offset=(short)-1;
  zCDirEnd->hdr.nentry_cdir=0;
  
  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"\nWrite central directory headers/end to file\n");
  #endif

  for(iter=zCDirDataList.begin();iter!=zCDirDataList.end();iter++)
  {
    int nbyte=WriteCDirData(*iter);

    if(nbyte<0)
    {
      return(nbyte);
    }
  }

  zCDirEnd->hdr.disk=zDisk;
  zCDirEnd->hdr.nentry_cdir_tot=zCDirDataList.size();

  int nbyte=WriteCentralDirEnd(zCDirEnd);

  if(nbyte<0)
  {
    return(nbyte);
  }
  else
  {
    zWriting=0;
    return(QZIP_OK);
  }
}

//-----------------------------------------------------------------------
//DeCompressAll
//Decomprime il file aperto nella directory specificata
//
//Variabili di ingresso:
//  dest  : nome della cartella di destinazione
//
//Valore ritornato     : QZIP_OK se nessun errore
//                       error code altrimenti
//-----------------------------------------------------------------------
int ZipClass::DeCompressAll(const char *dest)
{
	if(hFile==0)
	{
		return(QZIP_NOTOPEN);
	}
	
	if(zCDirEnd==NULL)
	{
		return(QZIP_INTERNAL_ERR);
	}

	char *outfile=new char[strlen(dest)+13];
	
	list<sCDirData *>::iterator iter;
	
	#ifdef QZIP_DEBUG
	fprintf(QZIP_DEBUG,"\nDecompression start\n");
	#endif

	for(iter=zCDirDataList.begin();iter!=zCDirDataList.end();iter++)
	{
		if(zDisk!=((*iter)->hdr.diskstart))
		{
			#ifdef QZIP_DEBUG
			fprintf(QZIP_DEBUG,"Need disk %d\n",(*iter)->hdr.diskstart);
			#endif
	
			int err=PromptForDisk((*iter)->hdr.diskstart);
		
			if(err!=QZIP_OK)
			{
				delete[] outfile;
				return(err);
			}
		}

		if(lseek(hFile,(*iter)->hdr.localhdr_offset,SEEK_SET)!=(*iter)->hdr.localhdr_offset)
		{
			delete[] outfile;
			return(QZIP_SEEK_ERR);
		}

		zDisk=((*iter)->hdr.diskstart);
	
		sLocalData *locdata=AllocLocalFileData();
	
		int err=GetLocalFileData(locdata);

		#ifdef QZIP_DEBUG
		fprintf(QZIP_DEBUG,"Get local file data status=%d\n",err);
		#endif

		if(err!=QZIP_OK)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(err);
		}

		if(locdata->hdr.filename_len==0)
		{
			#ifdef QZIP_DEBUG
			fprintf(QZIP_DEBUG,"Filename lenght=0\n");
			#endif
		
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}    

    	while(locdata->data.len!=locdata->hdr.compressed_size)
	    {
			if(!zSpan)
			{
				#ifdef QZIP_DEBUG
				fprintf(QZIP_DEBUG,"Data block size is different to compressed size\n");
				#endif
		
				FreeLocalFileData(locdata);
				delete[] outfile;
				return(QZIP_FORMAT_ERR);
			}
      
			err=ReadDataBlock(locdata);
		
			#ifdef QZIP_DEBUG
			fprintf(QZIP_DEBUG,"Read data block status=%d\n",err);
			#endif
		
			if(err!=QZIP_OK)
			{
				FreeLocalFileData(locdata);
				delete[] outfile;
				return(err);
			}
		}	

		#ifdef QZIP_DEBUG
		fprintf(QZIP_DEBUG,"Block size=%d\n",locdata->data.len);
		#endif

		for(int i=0;i<locdata->data.len;i++)
		{
			#ifdef QZIP_DEBUG
			fprintf(QZIP_DEBUG,"%02X ",locdata->data.buf[i]);
			#endif
	
			if(((i % 16)==0) && (i!=0))
			{
				#ifdef QZIP_DEBUG
				fprintf(QZIP_DEBUG,"\n");
				#endif
			}
		}

		#ifdef QZIP_DEBUG
		fprintf(QZIP_DEBUG,"\n");
		#endif
	
		if((*iter)->hdr.version_needed!=locdata->hdr.ver_to_extract)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
    
		if((*iter)->hdr.gp_flag!=locdata->hdr.gp_flag)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
    
		if((*iter)->hdr.method!=locdata->hdr.method)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
    
		if((*iter)->hdr.last_mod_time!=locdata->hdr.last_mod_time)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
    
		if((*iter)->hdr.last_mod_date!=locdata->hdr.last_mod_date)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
    
		if((*iter)->hdr.crc32!=locdata->hdr.crc32)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
    
		if((*iter)->hdr.compressed_size!=locdata->hdr.compressed_size)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
    
		if((*iter)->hdr.uncompressed_size!=locdata->hdr.uncompressed_size)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
    
		if((*iter)->hdr.filename_len!=locdata->hdr.filename_len)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
	
		//non e'vero che sono sempre ugali !!!		
		/* 
		if((*iter)->hdr.extrafield_len!=locdata->hdr.extrafield_len)
		{
			FreeLocalFileData(locdata);
			delete[] outfile;
			return(QZIP_FORMAT_ERR);
		}
		*/

	    strcpy(outfile,dest);
    	strcat(outfile,locdata->hdr_var.filename);
		
		unsigned int linux_attr = (*iter)->hdr.external_attrib >> 16;						
		if((*iter)->hdr.version_madeby == ZIP_MADEBY_MSDOS)
		{
			unsigned int tmp = 0;			
			if(!((*iter)->hdr.external_attrib & MSDOS_ATTR_RDONLY))			
			{
				tmp |= (1 << 1); 	//writeable
			}
			
			if((*iter)->hdr.external_attrib & MSDOS_ATTR_DIR)
			{
				tmp |= (1 << 0); 	//executable
			}
			
			if((linux_attr & S_IRWXU) != (S_IRUSR | (tmp << 6)))
			{
				//da InfoZip Unzip 5.52
				//gli attributi linux non sono readable + gli attributi writeable ed executable (subdirectory) specificati
				//dal dos : gli attributi linux non sono attendibili assegna quindi i permessi ottenuti da dos per tutti gli
				//utenti + readable per tutti gli utenti
				mode_t current_umask = umask(0);
				umask(current_umask);
				//non consentire i permessi negati all�'utente corrente				
				linux_attr = tmp & ~current_umask;				
			}						
		}
		
		//si concedono sempre i diritti di lettura e scrittura per l'utente 
		linux_attr|=(S_IWUSR | S_IRUSR);
		
		//i nomi dei file che terminano per slash sono delle directory
		if(outfile[strlen(outfile) - 1] == '/')
		{
			char *ptr_start=outfile;
			
			//create full path directory
			char tmpbuf[256]="";
      
			do
			{
				if(*ptr_start=='\0')
				{
					break;
				}
		
				//cerca la prossima occorrenza del carattere separatore di directory (slash)
				char *ptr=strchr(ptr_start,'/');
		
				if(ptr!=NULL)
				{
					strncpyQ(tmpbuf,outfile,ptr-outfile);
					tmpbuf[ptr-outfile]='\0';
					ptr_start=++ptr;
				}

				if(!CheckDirectory(tmpbuf))
				{
					if(mkdir(tmpbuf,linux_attr))
					{
						FreeLocalFileData(locdata);
						delete[] outfile;
						return(QZIP_OPEN_ERR);
					}
			
					#ifdef QZIP_DEBUG
					fprintf(QZIP_DEBUG,"%s created\n",tmpbuf);
					#endif
				}
        
      		} while(1);
    	}
    	else
    	{
			//file normale : crea
      		if((zs_FlushDest=FilesFunc_open(outfile,O_RDWR | O_CREAT | O_TRUNC,S_IREAD | S_IWRITE))<0)
      		{
				FreeLocalFileData(locdata);
				delete[] outfile;
				return(QZIP_OPEN_ERR);
      		}

			#ifdef QZIP_DEBUG
			fprintf(QZIP_DEBUG,"Output file created\n");
			#endif
    	}

		if(locdata->hdr.method==Z_DEFLATED)
		{
			if(DeCompressInit()!=Z_OK)
			{
				FreeLocalFileData(locdata);
				delete[] outfile;
				return(QZIP_INTERNAL_ERR);
			}

			#ifdef QZIP_DEBUG
			fprintf(QZIP_DEBUG,"Decompression system initialized\n");
			#endif
		
			int retcode=DeCompressBuffer(locdata->data.buf,locdata->data.len);
		
			if(retcode<Z_OK)
			{
				FreeLocalFileData(locdata);
				delete[] outfile;
				return(QZIP_INTERNAL_ERR);
			}
		
			//NOTA: lo stream compresso non viene fornito a blocchi
			//alla DeCompressBuffer ma interamente, quindi, il codice di ritorno
			//deve essere obbligatoriamente Z_STREAM_END se lo stream e'
			//completo / non danneggiato.
		
			if(retcode!=Z_STREAM_END)
			{
				#ifdef QZIP_DEBUG
				fprintf(QZIP_DEBUG,"Compressed buffer seems to be incomplete or damaged\n");
				#endif
				
				FreeLocalFileData(locdata);
				delete[] outfile;
				return(QZIP_FORMAT_ERR);
			}

			#ifdef QZIP_DEBUG
			fprintf(QZIP_DEBUG,"Decompressed %d bytes\n",locdata->data.len);
			#endif
		
			DeCompressEnd();
		
			#ifdef QZIP_DEBUG
			fprintf(QZIP_DEBUG,"Decompression end\n");
			#endif
		}
		else
		{
			if(locdata->hdr.method==Z_NO_COMPRESSION)
			{
				if(locdata->data.len>0)
				{
					if(write(zs_FlushDest,locdata->data.buf,locdata->data.len)!=locdata->data.len)
					{
						FreeLocalFileData(locdata);
						delete[] outfile;
						return(QZIP_WRITE_ERR);
					}
				}
			}
			else
			{
				//Invalid or unsupported method
				FreeLocalFileData(locdata);
				delete[] outfile;
				return(QZIP_FORMAT_ERR);
			}
		}

		if(fProgress[QZIP_DECOMP])
		{
			fProgress[QZIP_DECOMP](NULL);
		}
	
		FreeLocalFileData(locdata);
		
		FilesFunc_close(zs_FlushDest);
		zs_FlushDest=0;
 	 }

	delete[] outfile;
	return(QZIP_OK);

}

//-----------------------------------------------------------------------
//DeCompressInit
//Inizializza le strutture dati per la decompressione di un elemento
//
//Variabili di ingresso: nessuna
//
//-----------------------------------------------------------------------
int ZipClass::DeCompressInit(void)
{
  if(zs_buffer)
  {
    delete[] zs_buffer;
  }
  
  zs_buffer=new unsigned char[Z_BUFSIZE];

  zs.avail_in  = (uInt)0;
  zs.avail_out = (uInt)Z_BUFSIZE;
  zs.next_out  = zs_buffer;
  zs.total_in  = 0;
  zs.total_out = 0;

  zs.zalloc    = (alloc_func)0;
  zs.zfree     = (free_func)0;
  zs.opaque    = (voidpf)0;

  if(!zTempList.empty())
  {
    zTempList.clear();
  }

  int err=inflateInit2(&zs,-MAX_WBITS);

  if (err == Z_OK)
  {
    zs_init=1;
  }
  else
  {
    zs_init=0;
  }

  return(err);
}

//-----------------------------------------------------------------------
//DeCompress
//Scompattae i dati presenti in un buffer
//
//Variabili di ingresso:
//  inbuf: puntatore a buffer da comprimere
//  len  : lunghezza del buffer da comprimere
//
//-----------------------------------------------------------------------
int ZipClass::DeCompressBuffer(unsigned char *inbuf,int len)
{
  if(!zs_init)
  {
    return(QZIP_NOINIT_ZIP);
  }

  zs.next_in=inbuf;
  zs.avail_in=len;

  zs.avail_out = (uInt)Z_BUFSIZE;
  zs.next_out  = zs_buffer;

  int err;
  int is_outbuf_full=0;

  do
  {
    err=inflate(&zs,Z_NO_FLUSH);

    #ifdef QZIP_DEBUG
    fprintf(QZIP_DEBUG,"Decompression buffer status=%d\n",err);
    #endif

    if((err==Z_OK) || (err==Z_STREAM_END))
    {
      //se nessun errore: flush dei dati eventualmente disponibili
      //nel buffer di uscita

      //prima pero valuta se il buffer di uscita e' pieno: il buffer
      //verra' svuotato e quindi l'informazione verrebbe persa se non
      //la si memorizzasse in questo punto
      
      if(zs.avail_out==0)
      {
        is_outbuf_full=1;
      }
      else
      {
        is_outbuf_full=0;
      }

      //procede allo svuotamento del buffer di uscita
      ZipStreamFlush();
    }
    else
    {
      //se errore termina ciclo
      break;
    }

    //se il buffer di uscita era pieno e' necessario ripetere
    //il ciclo di inflate (potrebbero esserci altri dati da processare
    //che la inflate non e' riuscita ad inserire nel buffer)

    //nel caso in cui il codice di ritorno della inflate sia Z_STREAM_END
    //si e' raggiunta la fine dello stream compresso: termina operazione

  } while(is_outbuf_full && (err!=Z_STREAM_END));

  //Nota: se il buffer fornito in ingresso a questa funzione e' un intero
  //file compresso, il codice di ritorno deve essere sempre Z_STREAM_END

  return err;
}

//-----------------------------------------------------------------------
//DeCompressEnd
//Termina le operazione di decompressione
//
//Variabili di ingresso: nessuna
//Valore di ritorno    : nessuno
//-----------------------------------------------------------------------
void ZipClass::DeCompressEnd(void)
{
  #ifdef QZIP_DEBUG
  fprintf(QZIP_DEBUG,"Closing decompression routines\n");
  #endif

  ZipStreamFlush();

  inflateEnd(&zs);

  zs_init = 0;

  delete[] zs_buffer;
  zs_buffer=NULL;
}


//-----------------------------------------------------------------------
//GetNumberOfDisks
//Ritorna il numero di dischi totali (disk spanning)
//
//Variabili di ingresso: nessuna
//-----------------------------------------------------------------------
int ZipClass::GetNumberOfDisks(void)
{
  if(zCDirEnd!=NULL)
  {
    return(zCDirEnd->hdr.disk+1);
  }
  else
  {
    return(0);
  }
}

//-----------------------------------------------------------------------
//GetNumberOfFiles
//Ritorna il numero di file compressi presenti nel file zip
//
//Variabili di ingresso: nessuna
//-----------------------------------------------------------------------
int ZipClass::GetNumberOfFiles(void)
{
  if(zCDirEnd!=NULL)
  {
    return(zCDirEnd->hdr.nentry_cdir_tot);
  }
  else
  {
    return(0);
  }
}

//-----------------------------------------------------------------------
//SetProgressFunction
//Imposta funzione di callback per avanzamento compressione/decompressione
//
//Variabili di ingresso:
//   f   : puntatore a funzione
//   mode: QZIP_DECOMP   imposta per decompressione file
//         QZIP_DIRCOMP  imposta per compressione intera cartella
//         QZIP_COMPRESS imposta per compressione file
//-----------------------------------------------------------------------
void ZipClass::SetProgressFunction(int(*f)(void *),int mode)
{
  if((mode<0) || (mode>QZIP_COMPRESS))
  {
    return;
  }

  fProgress[mode]=f;
}

//-----------------------------------------------------------------------
//UnSetProgressFunction
//Disabilita funzione di callback per avanzamento compressione/decompressione
//
//Variabili di ingresso:
//   f   : puntatore a funzione
//   mode: QZIP_DECOMP   imposta per decompressione file
//         QZIP_DIRCOMP  imposta per compressione intera cartella
//         QZIP_COMPRESS imposta per compressione file
//-----------------------------------------------------------------------
void ZipClass::UnSetProgressFunction(int(*f)(void *),int mode)
{
  if((mode<0) || (mode>MAX_PROGFUNC_TYPE))
  {
    return;
  }

  fProgress[mode]=NULL;
}

//-----------------------------------------------------------------------
//SetPromptForDiskFunction
//Imposta funzione di callback per prompt prossimo disco
//
//Variabili di ingresso:
//   f   : puntatore a funzione
//-----------------------------------------------------------------------
void ZipClass::SetPromptForDiskFunction(int(*f)(int,char *))
{
  zPromptDisk=f;
}
