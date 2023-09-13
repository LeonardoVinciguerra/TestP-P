#ifndef __FILEFN_
#define __FILEFN_

#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>

#include "q_cost.h"

#define __FILESFUNC                    //attiva funzionalita avanzate per la gestione file


#define FILESFUNC_NENTRY 128           //numero massimo di file aperti contemporaneamente.

#ifdef  __FILESFUNC

//#define LOGFILES_STDERR
#define FILESFUNC_DUP    "dup()   "
#define FILESFUNC_OPEN   "open()  "
#define FILESFUNC_CLOSE  "close() "
#define FILESFUNC_FOPEN  "fopen() "
#define FILESFUNC_FCLOSE "fclose()"

#define FILESFUNC_SEARCH_OPEN      0
#define FILESFUNC_SEARCH_DUP       1
#define FILESFUNC_SEARCH_FOPEN     2

struct FilesFuncEntry
{
  char filename[MAXNPATH+1];
  int Handle;
  int counterOpen;
  
  FILE *filePtr;
  int counterFopen;
};

void FilesFunc_Msg(char *msg);
int FilesFunc_searchfilename(const char *filename,int mode);
int FilesFunc_searchhandle(int Handle,char *filefound);
int FilesFunc_searchhandle(int Handle);
void FilesFunc_insertentry(const char *type,const char *filename,int Handle,FILE *filePtr=NULL);
void FilesFunc_delentry(const char *type,int entry);
void FilesFunc_dupentry(const char *type,int entry,int newhandle);

#endif


int FilesFunc_dup(int Handle);
int FilesFunc_open(const char *filename,int mode,int perm=0,bool ignore_case = false, bool lock = false);
int FilesFunc_close(int Handle,bool unlock = false);
FILE *FilesFunc_fopen(const char *file, const char *mode,bool ignore_case = false);
int FilesFunc_fclose(FILE *f);

void EnableFileFunc(void);
void DisableFileFunc(void);
int IsFileFuncEnabled(void);
#endif
