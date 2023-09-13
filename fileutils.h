
//---------------------------------------------------------------------------
//
// Name:        fileutils.h
// Author:      Simone Navari
// Created:     21/10/2008 9.43
// Description: fileutils functions declaration
//
//---------------------------------------------------------------------------

#ifndef __FILEUTILS_H
#define __FILEUTILS_H

#include <sys/types.h>
#include <dirent.h>
#include <string>
#include <vector>

#define FA_DIREC				1
#define FA_IGNORE_CASE			(1 << 8)

#define FFBLK_MAXFILE 			255
#define FFBLK_MAXDIR_NAME		768

#define DIR_CREATION_FLAG		S_IRWXG | S_IRWXU | S_IROTH | S_IXOTH

#define chsize(handle,size)		ftruncate(handle,size)

struct ffblk
{
	char lfn_magic[6];			/* LFN: the magic "LFN32" signature */
	short lfn_handle;			/* LFN: the handle used by findfirst/findnext */
	unsigned short lfn_ctime;	/* LFN: file creation time */
	unsigned short lfn_cdate;	/* LFN: file creation date */
	unsigned short lfn_atime;	/* LFN: file last access time (usually 0) */
	unsigned short lfn_adate;	/* LFN: file last access date */
	char ff_reserved[5];		/* used to hold the state of the search */
	mode_t ff_attrib;	/* actual attributes of the file found */
	unsigned short ff_ftime;	/* hours:5, minutes:6, (seconds/2):5 */
	unsigned short ff_fdate;	/* (year-1980):7, month:4, day:5 */
	unsigned int ff_fsize;		/* size of file */
	char ff_name[FFBLK_MAXFILE+1];	/* name of file as ASCIIZ string */

	//only for linux 
	DIR*			dirp;
	struct dirent*	dir_entry;
	unsigned int	attributes;
	char			filespec[FFBLK_MAXFILE+1];
	char			path[FFBLK_MAXDIR_NAME+1];
};

int findnext(struct ffblk *ffblk);
int findfirst(const char *pathname, struct ffblk *ffblk, int attrib);
void end_findfirst(ffblk* fb);

unsigned int filelength( int fno );
unsigned int filelength( void* _f );



bool CopyFile( const char* dest, const char* src );
bool CheckFile( const char* filename );
bool RenameFile( const char* oldname, const char* newname );

bool CheckDirectory( const char* dir );
bool RenameDirectory( const char* oldname, const char* newname );
bool DeleteDirectory( const char* dir );

bool FindFiles( const char* path, const char* type, std::vector<std::string>& list, bool removeExt = true );

bool CopyFiles( const char* destDir, const char* srcDir, const char* filename );
bool DuplicateFiles( const char* dir, const char* oldName, const char* newName );
bool RenameFiles( const char* dir, const char* oldName, const char* newName );
bool DeleteFiles( const char* dir, const char* filename );
bool ModifyFilesExt( const char* dir, const char* filename );

#endif
