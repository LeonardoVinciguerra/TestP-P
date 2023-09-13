#include "filefn.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "q_help.h"
#include "q_gener.h"
#include "strutils.h"

#include <mss.h>

//--------------------------------------------------------------------------
//SMOD300503 - LOG FILES HANDLE

#ifdef  __FILESFUNC

int FilesFunc_nFopen=0;
int FilesFunc_nFclose=0;
int FilesFunc_nOpenDup=0;
int FilesFunc_nClose=0;
int FilesFunc_nMax=0;

FilesFuncEntry FilesFuncTable[FILESFUNC_NENTRY];

int FilesFuncCounter=0;

int useFileFunc=1;

int IsFileFuncEnabled(void)
{
  return(useFileFunc);
}

void EnableFileFunc(void)
{
  useFileFunc=1;
}

void DisableFileFunc(void)
{
  useFileFunc=0;
}

void FilesFunc_Msg(char *msg)
{
#ifdef LOGFILES_STDERR
	print_debug("%s\n",msg);
#endif
}

//--------------------------------------------------------------------------
void FilesFunc_checkmax(void)
{
  if(FilesFuncCounter==FILESFUNC_NENTRY)
  {
    fprintf(stdout,"WARNING !       : too many files opened.\n");
    exit(0);
  }
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
int FilesFunc_searchfilename(const char *filename,int mode)
{
  char buf[MAXNPATH+1];

  strncpy(buf,filename,MAXNPATH);
  DelAllSpc(buf);
  //strlwr(buf);


  for(int i=0;i<FilesFuncCounter;i++)
  {
    if(!strcmp(FilesFuncTable[i].filename,buf))
    {
      if((mode==FILESFUNC_SEARCH_DUP) && (FilesFuncTable[i].counterOpen==-1))
      {
        return(i);
      }

      if((mode==FILESFUNC_SEARCH_OPEN) && (FilesFuncTable[i].counterOpen!=-1))
      {
        return(i);
      }

      if((mode==FILESFUNC_SEARCH_FOPEN) && (FilesFuncTable[i].counterFopen!=-1))
      {
        return(i);
      }
    }
  }

  return(-1);
}
//--------------------------------------------------------------------------


int FilesFunc_searchhandle(int Handle,char *filefound)
{
  int i;

  if((i=FilesFunc_searchhandle(Handle))!=-1)
  {
    strcpy(filefound,FilesFuncTable[i].filename);
    return(1);
  }
  else
  {
    return(0);
  }
}

//--------------------------------------------------------------------------
int FilesFunc_searchhandle(int Handle)
{
  for(int i=0;i<FilesFuncCounter;i++)
  {
    if(Handle==FilesFuncTable[i].Handle)
      return(i);
  }

  return(-1);
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
void FilesFunc_insertentry(const char *type,const char *filename,int Handle,FILE *filePtr/*=NULL*/)
{
  char buf[256];

  snprintf( buf, sizeof(buf),"          %s : file %s handle %d",type,filename,Handle);
  FilesFunc_Msg(buf);

  strncpy(FilesFuncTable[FilesFuncCounter].filename,filename,MAXNPATH);
  DelAllSpc(FilesFuncTable[FilesFuncCounter].filename);
  //strlwr(FilesFuncTable[FilesFuncCounter].filename);

  FilesFuncTable[FilesFuncCounter].Handle=Handle;
  FilesFuncTable[FilesFuncCounter].filePtr=filePtr;

  if(filePtr==NULL)
    FilesFuncTable[FilesFuncCounter].counterOpen=0;
  else
    FilesFuncTable[FilesFuncCounter].counterFopen=0;
    
  FilesFuncCounter++;

  if(FilesFuncCounter>FilesFunc_nMax)
    FilesFunc_nMax=FilesFuncCounter;

  FilesFunc_checkmax();
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
void FilesFunc_delentry(const char *type,int entry)
{
  char buf[256];

  if(entry<FilesFuncCounter)
  {
    snprintf( buf, sizeof(buf),"          %s : file %s handle %d.",type,FilesFuncTable[entry].filename,FilesFuncTable[entry].Handle);

    FilesFunc_Msg(buf);
/* V.x.14 chenged memcpy to memmove for buffer overlap error
    memcpy(FilesFuncTable+entry,FilesFuncTable+(entry+1),sizeof(FilesFuncEntry)*(FilesFuncCounter-(entry+1)));
*/
    memmove(FilesFuncTable+entry,FilesFuncTable+(entry+1),sizeof(FilesFuncEntry)*(FilesFuncCounter-(entry+1)));

    FilesFuncCounter--;
  }
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
void FilesFunc_dupentry(const char *type,int entry,int newhandle)
{
/* V.x.14 chenged memcpy to memmove for buffer overlap error
  memcpy(FilesFuncTable+FilesFuncCounter,FilesFuncTable+entry,sizeof(FilesFuncTable[0]));
*/
  memmove(FilesFuncTable+FilesFuncCounter,FilesFuncTable+entry,sizeof(FilesFuncTable[0]));
  FilesFuncTable[FilesFuncCounter].Handle=newhandle;
  FilesFuncTable[FilesFuncCounter].counterOpen=-1;

  char buf[256];
  snprintf( buf, sizeof(buf),"          %s : file %s handle %d duplicated in %d.",type,FilesFuncTable[entry].filename,FilesFuncTable[entry].Handle,newhandle);
  FilesFunc_Msg(buf);

  FilesFuncCounter++;

  FilesFunc_checkmax();
}
//--------------------------------------------------------------------------
#endif

//--------------------------------------------------------------------------
int FilesFunc_dup(int Handle)
{

#ifndef __FILESFUNC
  return(dup(Handle));
#else

  if(!useFileFunc)
  {
    return(dup(Handle));
  }

  int entry=FilesFunc_searchhandle(Handle);

  char buf[256];

  if(entry==-1)
  {
    snprintf( buf, sizeof(buf),"WARNING ! %s : invalid handle %d.",FILESFUNC_DUP,Handle);
    FilesFunc_Msg(buf);

    return(-1);
  }
  else
  {
    int newhandle=dup(Handle);

    if(newhandle!=-1)
    {
      FilesFunc_nOpenDup++;

      FilesFunc_dupentry(FILESFUNC_DUP,entry,newhandle);

      FilesFuncTable[FilesFuncCounter-1].filePtr=NULL;

      FilesFunc_checkmax();
    }

    return(newhandle);
  }
#endif
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
int _FilesFunc_open(const char *filename,int mode,int perm,bool lock)
{
	char fbuf[MAXNPATH];
	int handle = -1;
	strncpy(fbuf,filename,MAXNPATH);
	//strlwr(fbuf);
		
	#ifndef __FILESFUNC
	handle = open(fbuf,mode,perm);
	
	//LOCK
	if((handle != -1) && lock)
	{	
		struct flock test_file_lock = { F_WRLCK, SEEK_SET, 0, 0, 0 }; // l_type, l_whence, l_start, l_len, l_pid
		test_file_lock.l_pid = getpid();
			
		if( fcntl( handle, F_SETLKW, &test_file_lock ) == -1 )
		{
			close(handle);
			return -1;
		}
	}	
		
	return handle;
		
	retrurn handle;
	#else
	
	if(!useFileFunc)
	{
		handle = open(fbuf,mode,perm);
		
		//LOCK
		if((handle != -1) && lock)
		{
			struct flock test_file_lock = { F_WRLCK, SEEK_SET, 0, 0, 0 }; // l_type, l_whence, l_start, l_len, l_pid
			test_file_lock.l_pid = getpid();
		
			if( fcntl( handle, F_SETLKW, &test_file_lock ) == -1 )
			{
				close(handle);
				return -1;
			}			
		}
		
		return handle;
	}
	
	
	int entry=FilesFunc_searchfilename(filename,FILESFUNC_SEARCH_OPEN);
	
	char buf[256];
	
	if(entry!=-1)
	{
		snprintf( buf, sizeof(buf),"WARNING ! %s : file %s already open with handle %d.",FILESFUNC_OPEN,filename,FilesFuncTable[entry].Handle);
		FilesFunc_Msg(buf);
		
		FilesFuncTable[entry].counterOpen++;
		FilesFunc_nOpenDup++;
	
		lseek(FilesFuncTable[entry].Handle,0,SEEK_SET);
		
		return(FilesFuncTable[entry].Handle);
	}
	
	//  print_debug("%s %X\n",filename,mode);
	
	//  delay(100);
	
	handle=open(fbuf,mode,perm);
	
	if(handle!=-1)
	{
		FilesFunc_nOpenDup++;
		FilesFunc_insertentry(FILESFUNC_OPEN,filename,handle);
	
		FilesFuncTable[FilesFuncCounter-1].filePtr=NULL;
	
		//LOCK
		if(lock)
		{		
			struct flock test_file_lock = { F_WRLCK, SEEK_SET, 0, 0, 0 }; // l_type, l_whence, l_start, l_len, l_pid
			test_file_lock.l_pid = getpid();
			
			if( fcntl( handle, F_SETLKW, &test_file_lock ) == -1 )
			{
				fprintf(stdout,"err %d !\n",errno);
				close(handle);
				return -1;
			}
		}
	}
	
	return(handle);
	
	#endif
}

int FilesFunc_open(const char *filename,int mode,int perm, bool ignore_case,bool lock)
{
	if(!ignore_case)
	{
		return(_FilesFunc_open(filename,mode,perm,lock));
	}
	else
	{
		char buf[MAXNPATH+1];
		strncpyQ(buf,filename,MAXNPATH);
		char *ptr = strchr(buf,'.');
		if((ptr == NULL) || !ignore_case)
		{
			return(_FilesFunc_open(buf,mode,perm,lock));
		}
		else
		{
			int h = _FilesFunc_open(buf,mode,perm,lock);
			if(h == -1)
			{
				ptr++;
				strupr(ptr);
				h = _FilesFunc_open(buf,mode,perm,lock);
			}
			
			return(h);
		}
	}
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
int FilesFunc_close(int Handle,bool unlock)
{
	//LOCK
	if(unlock)
	{
		struct flock test_file_unlock = { F_WRLCK, SEEK_SET, 0, 0, 0 }; // l_type, l_whence, l_start, l_len, l_pid
		test_file_unlock.l_pid = getpid();	
		
		test_file_unlock.l_type = F_UNLCK;
		fcntl( Handle, F_SETLK, &test_file_unlock );
	}

	//printf("File close: %d\n", Handle); //DB190712

	#ifndef __FILESFUNC
	return(close(Handle));
	#else

	if(!useFileFunc)
	{
		return(close(Handle));
	}
	
	if(FilesFuncCounter==0)
	{
		char buf[256];
		snprintf( buf, sizeof(buf),"WARNING ! %s : unable to close handle %d. No files opened.",FILESFUNC_CLOSE,Handle);
		FilesFunc_Msg(buf);
		return(-1);
	}
	
	int entry=FilesFunc_searchhandle(Handle);
	
	if(entry==-1)
	{
		char buf[256];
		snprintf( buf, sizeof(buf),"WARNING ! %s : unable to close handle %d. No files associated to handle.",FILESFUNC_CLOSE,Handle);
		FilesFunc_Msg(buf);
		return(-1);
	}
	else
	{
		int ret=-1;
		
		if(FilesFuncTable[entry].counterOpen<=0)
		{
			ret=close(FilesFuncTable[entry].Handle);
			FilesFunc_delentry(FILESFUNC_CLOSE,entry);
		}
		else
		{
			FilesFuncTable[entry].counterOpen--;
			ret=0;
		}
		
		if(!ret)
		{
			FilesFunc_nClose++;
		}
		
		return(ret);
	}

	#endif
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
FILE *_FilesFunc_fopen(const char *file, const char *mode)
{
	char fbuf[MAXNPATH];
	
	strncpy(fbuf,file,MAXNPATH);
	//strlwr(fbuf);

	#ifndef __FILESFUNC
	return(fopen(fbuf,mode));
	#else

	if(!useFileFunc)
	{
		return(fopen(fbuf,mode));
	}

	int entry=FilesFunc_searchfilename(file,FILESFUNC_SEARCH_FOPEN);

	char buf[256];

	if(entry!=-1)
	{
		if(FilesFuncTable[entry].filePtr)
		{
			snprintf( buf, sizeof(buf),"WARNING ! %s : file %s already open with handle %d.",FILESFUNC_FOPEN,file,FilesFuncTable[entry].filePtr->_fileno);
		}
		else
		{
			snprintf( buf, sizeof(buf),"WARNING ! %s : file %s already.",FILESFUNC_FOPEN,file);
		}

		FilesFunc_Msg(buf);

		FilesFunc_nFopen++;

		FilesFuncTable[entry].counterFopen++;

		if(FilesFuncTable[entry].filePtr)
		{
			fseek(FilesFuncTable[entry].filePtr,0,SEEK_SET);
		}
		else
		{
			lseek(FilesFuncTable[entry].Handle,0,SEEK_SET);
		}

		return(FilesFuncTable[entry].filePtr);
	}

	FILE *f;
	f=fopen(fbuf,mode);
	FilesFunc_nFopen++;

	if(f!=NULL)
	{
		FilesFunc_insertentry(FILESFUNC_FOPEN,file,f->_fileno,f);
	}

	return(f);

	#endif
}

FILE *FilesFunc_fopen(const char *file, const char *mode, bool ignore_case)
{
	if(!ignore_case)
	{
		return(_FilesFunc_fopen(file,mode));
	}
	else
	{
		char buf[MAXNPATH+1];
		strncpyQ(buf,file,MAXNPATH);
		char *ptr = strchr(buf,'.');
		if((ptr == NULL) || !ignore_case)
		{
			return(_FilesFunc_fopen(buf,mode));
		}
		else
		{
			FILE *f = _FilesFunc_fopen(buf,mode);
			if(f == NULL)
			{
				ptr++;
				strupr(ptr);
				f = _FilesFunc_fopen(buf,mode);
			}
			
			return(f);
		}
	}
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
int FilesFunc_fclose(FILE *f)
{
	#ifndef __FILESFUNC
  return(fclose(f));
	#else

  if(!useFileFunc)
  {
    return(fclose(f));
  }
  
  if(FilesFuncCounter==0)
  {
    char buf[256];
    snprintf( buf, sizeof(buf),"WARNING ! %s : unable to close handle %d. No files opened.",FILESFUNC_FCLOSE,f->_fileno);
    FilesFunc_Msg(buf);
    return(EOFVAL);
  }

  int entry=FilesFunc_searchhandle(f->_fileno);

  if(entry==-1)
  {
    char buf[256];
    snprintf( buf, sizeof(buf),"WARNING ! %s : unable to close handle %d. No files associated to handle.",FILESFUNC_FCLOSE,f->_fileno);
    FilesFunc_Msg(buf);
    return(-1);
  }
  else
  {
    int ret=EOFVAL;

    if(FilesFuncTable[entry].counterFopen<=0)
    {
      ret=fclose(FilesFuncTable[entry].filePtr);
      FilesFunc_delentry(FILESFUNC_FCLOSE,entry);
    }
    else
    {
      FilesFuncTable[entry].counterFopen--;
      ret=0;
    }

    if(!ret)
		{
      FilesFunc_nClose++;
		}

    return(ret);
  }
	#endif

}
