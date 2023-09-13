//---------------------------------------------------------------------------
//
// Name:        fileutils.cpp
// Author:      Simone Navari
// Created:     21/10/2008 9.43
// Description: fileutils functions implementation
//
//---------------------------------------------------------------------------
#include "fileutils.h"

#include <stdio.h>
#include "q_cost.h"
#include <sys/stat.h>
#include <fnmatch.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include <mss.h>


static struct stat _dir_stat_data;

int findfirst( const char* pathname, ffblk* fb, int attrib )
{
	char *slash_ptr = (char*)pathname;
	char *prev_slash_ptr = NULL;

	do
	{
		slash_ptr = strchr(slash_ptr,'/');
		if(slash_ptr)
		{
			prev_slash_ptr = slash_ptr;
			slash_ptr++;
		}
		else
		{
			break;
		}
	} while(1);

	if(prev_slash_ptr)
	{
		strncpy(fb->path,pathname,prev_slash_ptr - pathname);
		strcpy(fb->filespec,prev_slash_ptr + 1);
		fb->path[prev_slash_ptr - pathname ]=0;
	}
	else
	{
		strcpy(fb->path,".");
		strcpy(fb->filespec,pathname);
	}

	fb->attributes = attrib;
	fb->dirp = opendir(fb->path);

	int r = findnext(fb);
	
	return r;
}

int findnext(ffblk* fb)
{
	char fullpath[FFBLK_MAXDIR_NAME];
	
	if(!fb->dirp)
	{
		memset(fb,0,sizeof(fb));
		errno = ENOENT;
		return(-1);
	}
	
	while((fb->dir_entry = readdir(fb->dirp)) != NULL)
	{
		strcpy(fullpath,fb->path);
		strcat(fullpath,"/");
		strcat(fullpath,fb->dir_entry->d_name);
		
		if(stat(fullpath, &_dir_stat_data))
		{
			continue;
		}

		bool dot_added = false;

		char *dot = strchr(fb->dir_entry->d_name,'.');
		if(!dot)
		{
			strcat(fb->dir_entry->d_name,".");
			dot_added = true;
		}
		
		if( S_ISDIR(_dir_stat_data.st_mode) )
		{
			if( !(fb->attributes & FA_DIREC) )
			{
				continue;
			}
		}
		else
		{
			if( fb->attributes & FA_DIREC )
			{
				continue;
			}
		}

		int flags = FNM_PATHNAME;
		if(fb->attributes & FA_IGNORE_CASE)
		{
			flags|=FNM_CASEFOLD;
		}
		
		if(!fnmatch(fb->filespec, fb->dir_entry->d_name,flags))
		{
			strncpy(fb->ff_name,fb->dir_entry->d_name,FFBLK_MAXFILE);
			if(dot_added)
			{
				fb->ff_name[strlen(fb->ff_name) - 1] = '\0';
			}
			
			fb->ff_fsize = _dir_stat_data.st_size;

			struct tm *t;
			t = localtime(&_dir_stat_data.st_mtime);

			fb->ff_ftime = (t->tm_sec /2) & 0x1f;
			fb->ff_ftime |= (t->tm_min & 0x3f) << 5;
			fb->ff_ftime |= (t->tm_min & 0x1f) << 11;

			fb->ff_fdate = (t->tm_mday & 0x1f);
			fb->ff_fdate |= (t->tm_mon & 0x0f) << 5;
			fb->ff_fdate  |= ((t->tm_year - 80) & 0x7f) << 9;

			fb->ff_attrib = _dir_stat_data.st_mode;

			return(0);
		}
	}

	closedir(fb->dirp);
	fb->dirp = NULL;

	memset(fb,0,sizeof(fb));
	errno = ENOENT;
	return(-1);
}

void end_findfirst(ffblk* fb)
{
	if(fb->dirp != NULL)
	{
		closedir(fb->dirp);
		fb->dirp = NULL;
	}
}

unsigned int filelength( int fno )
{
	if(fno<0)
	{
		return(0);
	}

	int old_pos=lseek(fno,0,SEEK_CUR);
	int len=lseek(fno,0,SEEK_END);

	lseek(fno,old_pos,SEEK_SET);

	return(len);
}

unsigned int filelength( void* _f )
{
	if( !_f )
	{
		return 0;
	}

	int old_pos = fseek( (FILE*)_f, 0, SEEK_CUR );
	int len = fseek( (FILE*)_f, 0, SEEK_END );
	fseek( (FILE*)_f, old_pos, SEEK_SET );

	return len;
}


//---------------------------------------------------------------------------
// Ritorna true se il file e' presente
//---------------------------------------------------------------------------
bool CheckFile( const char* filename )
{
	return access( filename, F_OK ) == 0 ? true : false;
}

//---------------------------------------------------------------------------
// Rinomina un file
//---------------------------------------------------------------------------
bool RenameFile( const char* oldname, const char* newname )
{
	return rename( oldname, newname ) == 0 ? true : false;
}

//---------------------------------------------------------------------------
// Copia un file
//---------------------------------------------------------------------------
bool CopyFile( const char* dest, const char* src )
{
	if( !CheckFile( src ) )
		return false;

	FILE* fin = fopen( src, "r" );
	if( fin == NULL )
		return false;

	FILE* fout = fopen( dest, "w" );
	if( fout == NULL )
	{
		fclose( fin );
		return false;
	}

	char buf[1024*1024]; // 1Mb
	bool eof = false;

	while( !eof )
	{
		size_t nr = fread( buf, 1, sizeof(buf), fin );

		if( nr != sizeof(buf) )
		{
			if( feof( fin ) )
			{
				eof = true;
			}
			else
			{
				eof = false;
				break;
			}
		}

		size_t nw = fwrite( buf, 1, nr, fout );

		if( nw != nr )
		{
			eof = false;
			break;
		}
	}

	fclose( fin );
	fclose( fout );

	return eof;
}

//---------------------------------------------------------------------------
// Ritorna true se la directory e' presente
//---------------------------------------------------------------------------
bool CheckDirectory( const char* dir )
{
	struct stat st;
	if( stat( dir, &st ) == 0 )
	{
		if( S_ISDIR( st.st_mode ) )
		{
			return true;
		}
	}
	else
	{
		perror( "stat" );
	}

	return false;
}

//---------------------------------------------------------------------------
// Rinomina una directory
//---------------------------------------------------------------------------
bool RenameDirectory( const char* oldname, const char* newname )
{
	return rename( oldname, newname ) == 0 ? true : false;
}

//---------------------------------------------------------------------------
// Elimina una directory (con file e sottocartelle)
//---------------------------------------------------------------------------
bool DeleteDirectory( const char* dir )
{
	bool err = false;
	std::vector<std::string> itemlist;

	// enter sub-directories
	FindFiles( dir, 0, itemlist, false );

	for( unsigned int i = 0; i < itemlist.size(); i++ )
	{
		std::string subdir = dir;
		subdir.append( "/" );
		subdir.append( itemlist[i] );

		if( !DeleteDirectory( subdir.c_str() ) )
		{
			err = true;
		}
	}

	itemlist.clear();

	// remove files
	FindFiles( dir, "*", itemlist, false );

	for( unsigned int i = 0; i < itemlist.size(); i++ )
	{
		std::string filename = dir;
		filename.append( "/" );
		filename.append( itemlist[i] );

		if( remove( filename.c_str() ) != 0 )
		{
			err = true;
		}
	}

	// remove directory
	if( rmdir( dir ) != 0 )
	{
		err = true;
	}

	return !err;
}

//---------------------------------------------------------------------------
// Cerca file o cartelle (se type = 0 cerca cartelle)
//---------------------------------------------------------------------------
bool FindFiles( const char* path, const char* type, std::vector<std::string>& list, bool removeExt )
{
	char filesName[MAXNPATH];

	int flags = FA_IGNORE_CASE;
	if( type == 0 )
	{
		flags |= FA_DIREC;
		snprintf( filesName, MAXNPATH, "%s/*", path );
	}
	else
	{
		snprintf( filesName, MAXNPATH, "%s/%s", path, type );
	}

	struct ffblk ffblk;
	int done = findfirst( filesName, &ffblk, flags );

	while( !done )
	{
		if( removeExt )
		{
			char* p = strchr( ffblk.ff_name, '.' );
			if( p != NULL && p != ffblk.ff_name )
			{
				*p = '\0';
			}
		}

		if( flags & FA_DIREC )
		{
			if( ffblk.ff_name[0] != '.' )
			{
				list.push_back( ffblk.ff_name );
			}
		}
		else
		{
			list.push_back( ffblk.ff_name );
		}

		done = findnext( &ffblk );
	}

	return true;
}

//---------------------------------------------------------------------------
// Copia tutti i files filename.* dalla cartella src alla cartella dest
//---------------------------------------------------------------------------
bool CopyFiles( const char* destDir, const char* srcDir, const char* filename )
{
	char dFile[MAXNPATH];
	char sFile[MAXNPATH];
	std::vector<std::string> fileslist;

	std::string filemask = filename;
	filemask.append( ".*" );

	FindFiles( srcDir, filemask.c_str(), fileslist, false );

	bool err = false;

	for( unsigned int i = 0; i < fileslist.size(); i++ )
	{
		snprintf( dFile, MAXNPATH, "%s/%s", destDir, fileslist[i].c_str() );
		snprintf( sFile, MAXNPATH, "%s/%s", srcDir, fileslist[i].c_str() );

		if( !CopyFile( dFile, sFile ) )
		{
			err = true;
		}
	}

	return !err;
}

//---------------------------------------------------------------------------
// Duplica tutti i files oldName.* in newName.*
//---------------------------------------------------------------------------
bool DuplicateFiles( const char* dir, const char* oldName, const char* newName )
{
	char dFile[MAXNPATH];
	char sFile[MAXNPATH];
	std::vector<std::string> fileslist;

	std::string filemask = oldName;
	filemask.append( ".*" );

	FindFiles( dir, filemask.c_str(), fileslist, false );

	bool err = false;

	for( unsigned int i = 0; i < fileslist.size(); i++ )
	{
		snprintf( sFile, MAXNPATH, "%s/%s", dir, fileslist[i].c_str() );

		size_t found = fileslist[i].find_last_of( "." );
		if( found == std::string::npos )
			continue;

		snprintf( dFile, MAXNPATH, "%s/%s%s", dir, newName, fileslist[i].substr(found).c_str() );

		if( !CopyFile( dFile, sFile ) )
		{
			err = true;
		}
	}

	return !err;
}

//---------------------------------------------------------------------------
// Rinomina tutti i files oldName.* in newName.*
//---------------------------------------------------------------------------
bool RenameFiles( const char* dir, const char* oldName, const char* newName )
{
	char dFile[MAXNPATH];
	char sFile[MAXNPATH];
	std::vector<std::string> fileslist;

	std::string filemask = oldName;
	filemask.append( ".*" );

	FindFiles( dir, filemask.c_str(), fileslist, false );

	bool err = false;

	for( unsigned int i = 0; i < fileslist.size(); i++ )
	{
		size_t found = fileslist[i].find_last_of( "." );
		if( found == std::string::npos )
			continue;

		snprintf( sFile, MAXNPATH, "%s/%s", dir, fileslist[i].c_str() );
		snprintf( dFile, MAXNPATH, "%s/%s%s", dir, newName, fileslist[i].substr(found).c_str() );

		if( rename( sFile, dFile ) != 0 )
		{
			err = true;
		}
	}

	return !err;
}

//---------------------------------------------------------------------------
// Elimina tutti i files filename.* dalla cartella dir
//---------------------------------------------------------------------------
bool DeleteFiles( const char* dir, const char* filename )
{
	char sFile[MAXNPATH];
	std::vector<std::string> fileslist;

	std::string filemask = filename;
	filemask.append( ".*" );

	FindFiles( dir, filemask.c_str(), fileslist, false );

	bool err = false;

	for( unsigned int i = 0; i < fileslist.size(); i++ )
	{
		snprintf( sFile, MAXNPATH, "%s/%s", dir, fileslist[i].c_str() );

		if( remove( sFile ) != 0 )
		{
			err = true;
		}
	}

	return !err;
}

//---------------------------------------------------------------------------
// Modifica l'estensione di tutti i files filename.* in minuscolo
//---------------------------------------------------------------------------
bool ModifyFilesExt( const char* dir, const char* filename )
{
	char dFile[MAXNPATH];
	char sFile[MAXNPATH];
	std::vector<std::string> fileslist;

	std::string filemask = filename;
	filemask.append( ".*" );

	FindFiles( dir, filemask.c_str(), fileslist, false );

	bool err = false;

	for( unsigned int i = 0; i < fileslist.size(); i++ )
	{
		size_t found = fileslist[i].find_last_of( "." );
		if( found == std::string::npos )
			continue;

		snprintf( sFile, MAXNPATH, "%s/%s", dir, fileslist[i].c_str() );

		for( unsigned int c = found+1; c < fileslist[i].size(); c++ )
		{
			fileslist[i][c] = tolower( fileslist[i][c] );
		}
		snprintf( dFile, MAXNPATH, "%s/%s", dir, fileslist[i].c_str() );

		rename( sFile, dFile );
	}

	return !err;
}
