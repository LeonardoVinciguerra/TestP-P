//---------------------------------------------------------------------------
//
// Name:        filemanager.h
// Author:      Gabriel Ferri
// Created:     17/01/2011
// Description: CFileManager class definition
//
//---------------------------------------------------------------------------

#ifndef __FILEMANAGER_H
#define __FILEMANAGER_H

#include <string>
#include <stdio.h>


class CFileManager
{
public:
	CFileManager( const std::string& filename );
	~CFileManager();

	int open( int fileVersion, bool showError );
	void close();

	bool exists();
	bool is_open() { return _f == 0 ? false : true; }
	bool create( int fileversion );

	bool readRec( void* rec, unsigned int recSize, int recPos );
	bool writeRec( const void* rec, unsigned int recSize, int recPos );
	int countRecs( unsigned int recSize );

	bool readBytes( void* data, unsigned int dataSize );
	bool writeBytes( const void* data, unsigned int dataSize );

	bool checkVersion( int fileversion ) { return fileversion == _fileversion ? true : false; }

	FILE* GetFD() { return _f; }

protected:
	FILE* _f;
	std::string _filename;
	int _fileversion;

	unsigned int _dataptr;

	void _writeHeader( int fileVersion );
	int _readHeader( int& fileVersion );
};


#endif
