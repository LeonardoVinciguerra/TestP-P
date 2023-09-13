//---------------------------------------------------------------------------
//
// Name:        filemanager.cpp
// Author:      Gabriel Ferri
// Created:     17/01/2011
// Description: CFileManager class implementation
//
//---------------------------------------------------------------------------

#include "filemanager.h"

#include "msglist.h"
#include "q_help.h"
#include "fileutils.h"



#define Q_HEADER_START       "QH<"
#define Q_HEADER_END         ">"

#define Q_HEADER_EXAMPLE     Q_HEADER_START "V.SV" Q_HEADER_END
#define Q_HEADER_LEN         strlen(Q_HEADER_EXAMPLE)

#define Q_FILE_VERSION       1



//---------------------------------------------------------------------------
// Costruttore / Distruttore
//---------------------------------------------------------------------------
CFileManager::CFileManager( const std::string& filename )
{
	_f = 0;
	_filename = filename;
	_fileversion = -1;

	_dataptr = 0;
}

CFileManager::~CFileManager()
{
	if( _f )
		close();
}

//---------------------------------------------------------------------------
// Open file
// Returns: 1 successfully open
//         -1 warning: file doesn't exist
//         -2 error: unable to open
//         -4 warning: bad file version
//---------------------------------------------------------------------------
int CFileManager::open( int fileVersion, bool showError )
{
	// if open close it
	if( _f )
		close();

	if( !exists() )
	{
		// file doesn't exist
		if( showError )
		{
			char buf[160];
			snprintf( buf, 160, MsgGetString(Msg_07028), _filename.c_str() );
			W_Mess( buf );
		}

		return -1;
	}

	// open
	_f = fopen( _filename.c_str(), "r+" );
	if( _f == NULL )
	{
		if( showError )
		{
			char buf[160];
			snprintf( buf, 160, MsgGetString(Msg_07029), _filename.c_str() );
			W_Mess( buf );
		}

		return -2;
	}

	_readHeader( _fileversion );

	_dataptr = ftell( _f );

	if( !checkVersion( fileVersion ) )
		return -4;

	return 1;
}

//---------------------------------------------------------------------------
// Close file
//---------------------------------------------------------------------------
void CFileManager::close()
{
	if( _f )
	{
		fclose( _f );
		_f = 0;
	}
}

//---------------------------------------------------------------------------
// Check file existence
//---------------------------------------------------------------------------
bool CFileManager::exists()
{
	return access( _filename.c_str(), F_OK ) == 0 ? true : false;
}

//---------------------------------------------------------------------------
// Create file
// If a file with the same name already exists its content is erased and
// the file is treated as a new empty file.
//---------------------------------------------------------------------------
bool CFileManager::create( int fileversion )
{
	// if open close it
	if( _f )
		close();

	// create new
	_f = fopen( _filename.c_str(), "w" );
	if( _f == NULL )
	{
		char buf[160];
		snprintf( buf, 160, MsgGetString(Msg_07030), _filename.c_str() );
		W_Mess( buf );

		return false;
	}

	_writeHeader( fileversion );

	_dataptr = ftell( _f );

	return true;
}

//---------------------------------------------------------------------------
// Read a record
//---------------------------------------------------------------------------
bool CFileManager::readRec( void* rec, unsigned int recSize, int recPos )
{
	if( !_f || recPos < 0 )
		return false;

	unsigned int filepos = _dataptr + recPos * recSize;
	if( fseek( _f, filepos, SEEK_SET ) != 0 )
	{
		return false;
	}

	int n = fread( rec, recSize, 1, _f );

	return ( n != 1 ) ? false : true;
}

//---------------------------------------------------------------------------
// Write a record
//---------------------------------------------------------------------------
bool CFileManager::writeRec( const void* rec, unsigned int recSize, int recPos )
{
	if( !_f || recPos < 0 )
		return false;

	unsigned int filepos = _dataptr + recPos * recSize;
	if( fseek( _f, filepos, SEEK_SET ) != 0 )
	{
		return false;
	}

	int n = fwrite( rec, recSize, 1, _f );

	return ( n != 1 ) ? false : true;
}

//---------------------------------------------------------------------------
// Read data
//---------------------------------------------------------------------------
bool CFileManager::readBytes( void* data, unsigned int dataSize )
{
	if( !_f )
		return false;

	int n = fread( data, dataSize, 1, _f );

	return ( n != 1 ) ? false : true;
}

//---------------------------------------------------------------------------
// Write data
//---------------------------------------------------------------------------
bool CFileManager::writeBytes( const void* data, unsigned int dataSize )
{
	if( !_f )
		return false;

	int n = fwrite( data, dataSize, 1, _f );

	return ( n != 1 ) ? false : true;
}

//---------------------------------------------------------------------------
// Count records
//---------------------------------------------------------------------------
int CFileManager::countRecs( unsigned int recSize )
{
	return ( filelength( _f ) - _dataptr ) / sizeof(recSize);
}

//---------------------------------------------------------------------------
// Write file header
//---------------------------------------------------------------------------
void CFileManager::_writeHeader( int fileVersion )
{
	fseek( _f, 0, SEEK_SET );

	char buf[Q_HEADER_LEN+1];
	snprintf( buf, sizeof(buf), "%s%d.%02d%s", Q_HEADER_START, Q_FILE_VERSION, fileVersion, Q_HEADER_END );

	fwrite( buf, Q_HEADER_LEN, 1, _f );
}

//---------------------------------------------------------------------------
// Read file header
// Returns: 1 successfully read
//         -1 read error
//         -2 bad header
//---------------------------------------------------------------------------
int CFileManager::_readHeader( int& fileVersion )
{
	fseek( _f, 0, SEEK_SET );

	char buf[Q_HEADER_LEN+1];

	int n = fread( buf, Q_HEADER_LEN, 1, _f );
	if( n != 1 )
	{
		// read error
		return -1;
	}
	buf[Q_HEADER_LEN] = 0;

	char* p = buf;
	p += strlen(Q_HEADER_START);
	int header_version = atoi( p );

	if( header_version != Q_FILE_VERSION )
	{
		// bad header
		return -2;
	}

	p += 2;
	fileVersion = atoi( p );

	return 1;
}
