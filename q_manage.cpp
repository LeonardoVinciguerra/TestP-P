//---------------------------------------------------------------------------
//
// Name:        q_manage.cpp
// Author:      Gabriel Ferri
// Created:     31/01/2012
// Description: Quadra customer, programs,etc... manager
//
//---------------------------------------------------------------------------
#include "q_manage.h"

#include "msglist.h"
#include "c_inputbox.h"
#include "q_tabe.h"
#include "q_packages.h"
#include "q_feeders.h"
#include "fileutils.h"
#include "strutils.h"
#include "q_inifile.h"
#include "q_files_new.h"
#include "q_feederfile.h"
#include "q_carint.h"
#include "q_zip.h"
#include "q_help.h"
#include "q_net.h"
#include "q_decode.h"
#include "keyutils.h"

#include <mss.h>


extern CfgHeader QHeader;


//---------------------------------------------------------------------------
// finestra: Selezione generica
//---------------------------------------------------------------------------
std::string fn_Select( CWindow* parent, const std::string& title, const std::vector<std::string>& items )
{
	CWindowSelect win( parent, 8, 1, 10, 10 );
	win.SetStyle( win.GetStyle() | WIN_STYLE_NO_MENU );
	win.SetClientAreaPos( 0, 7 );
	win.SetClientAreaSize( 26, 13 );
	win.SetTitle( title.c_str() );

	for( unsigned int i = 0; i < items.size(); i++ )
	{
		win.AddItem( items[i].c_str() );
	}
	win.Sort();

	win.Show();
	win.Hide();

	if( win.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return "";
	}
	return win.GetSelectedItem();
}



//---------------------------------------------------------------------------
// finestra: Customers manager
//---------------------------------------------------------------------------
#define BACKUP_CUST_TAG "QL-Customer's backup"

extern void DirVerify( char* cust );
extern int CopyDir( char* dir_dest, char* dir_orig );
extern void DeleteFiles_OLD(const char *FNome,const char *filesel="*",const char *ext="*");

CustomersManagerUI::CustomersManagerUI( CWindow* parent )
: CWindowSelect( parent, 7, 1, 10, 10 )
{
	SetClientAreaPos( 0, 6 );
	SetTitle( MsgGetString(Msg_00070) );

	SM_SaveOn = new GUI_SubMenu();
	SM_SaveOn->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &CustomersManagerUI::toSharedFolder, this ) ); // shared network
	SM_SaveOn->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &CustomersManagerUI::toUSBDevice, this ) ); // usb pen

	SM_LoadFrom = new GUI_SubMenu();
	SM_LoadFrom->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &CustomersManagerUI::fromSharedFolder, this ) ); // shared network
	SM_LoadFrom->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &CustomersManagerUI::fromUSBDevice, this ) ); // usb pen

	loadCustomer();
}

CustomersManagerUI::~CustomersManagerUI()
{
	delete SM_SaveOn;
	delete SM_LoadFrom;
}

void CustomersManagerUI::onShowSomething()
{
	tips = new CPan( 22, 1, MsgGetString(Msg_00298) );
}

void CustomersManagerUI::onEnter()
{
	std::string selectedCust = GetSelectedItem();
	snprintf( QHeader.Cli_Default, sizeof(QHeader.Cli_Default), "%s", selectedCust.c_str() );
	Mod_Cfg( QHeader );

	// crea (se necessario) sottocartelle cliente
	DirVerify( QHeader.Cli_Default );
}

void CustomersManagerUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_00475), K_F3, 0, NULL, boost::bind( &CustomersManagerUI::newCustomer, this ) ); // new
	m_menu->Add( MsgGetString(Msg_00476), K_F4, 0, NULL, boost::bind( &CustomersManagerUI::duplicateCustomer, this ) ); // duplicate
	m_menu->Add( MsgGetString(Msg_00478), K_F5, 0, NULL, boost::bind( &CustomersManagerUI::renameCustomer, this ) ); // rename
	m_menu->Add( MsgGetString(Msg_00477), K_F6, 0, NULL, boost::bind( &CustomersManagerUI::deleteCustomer, this ) ); // delete
	m_menu->Add( MsgGetString(Msg_00479), K_F7, 0, SM_SaveOn, NULL ); // salva in ...
	m_menu->Add( MsgGetString(Msg_00480), K_F8, 0, SM_LoadFrom, NULL ); // carica da ...
}

bool CustomersManagerUI::onHotKey( int key )
{
	switch( key )
	{
		case K_F3:
			newCustomer();
			return true;

		case K_F4:
			duplicateCustomer();
			return true;

		case K_F5:
			renameCustomer();
			return true;

		case K_F6:
			deleteCustomer();
			return true;

		case K_F7:
			SM_SaveOn->Show();
			return true;

		case K_F8:
			SM_LoadFrom->Show();
			return true;

		default:
			break;
	}

	return false;
}

void CustomersManagerUI::loadCustomer()
{
	// cerca directory cliente
	nameList.clear();
	FindFiles( CLIDIR, 0, nameList );

	// check presenza almeno una directory cliente
	if( nameList.size() == 0 )
	{
		//TODO
		// errore: chiede di aggiungere un nuovo customer
		/*
		W_Mess( MsgGetString(Msg_00074) );
		if(!NewCust())
		{
			return(0);
		}
		*/
	}

	ClearItems();
	for( unsigned int i = 0; i < nameList.size(); i++ )
	{
		AddItem( nameList[i].c_str() );
	}
	Sort();
}

int CustomersManagerUI::newCustomer()
{
	char newName[9];
	char newCust[MAXNPATH];

	CInputBox inbox( this, 8, MsgGetString(Msg_00475), MsgGetString(Msg_00073), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	snprintf( newCust, MAXNPATH, "%s/%s", CLIDIR, newName );

	if( CheckDirectory( newCust ) )
	{
		W_Mess( MsgGetString(Msg_01753) ); // customer already present
		return 0;
	}

	// crea cartella cliente
	mkdir( newCust, DIR_CREATION_FLAG );
	// crea (se necessario) sottocartelle cliente
	DirVerify( newName );

	loadCustomer();
	ShowItems();
	return 1;
}

int CustomersManagerUI::duplicateCustomer()
{
	std::string selectedCust = GetSelectedItem();

	char newName[9];
	char newCust[MAXNPATH];
	char oldCust[MAXNPATH];

	CInputBox inbox( this, 8, MsgGetString(Msg_00476), MsgGetString(Msg_00073), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	snprintf( newCust, MAXNPATH, "%s/%s", CLIDIR, newName );

	if( CheckDirectory( newCust ) )
	{
		W_Mess( MsgGetString(Msg_01753) ); // customer already present
		return 0;
	}

	snprintf( oldCust, MAXNPATH, "%s/%s", CLIDIR, selectedCust.c_str() );

	// crea cartella cliente
	mkdir( newCust, DIR_CREATION_FLAG );
	// copia sottocartelle cliente
	CopyDir( newCust, oldCust );

	loadCustomer();
	ShowItems();
	return 1;
}

int CustomersManagerUI::renameCustomer()
{
	std::string selectedCust = GetSelectedItem();

	if( strcmp( selectedCust.c_str(), QHeader.Cli_Default ) == 0 )
	{
		W_Mess( MsgGetString(Msg_01752) );
		return 0 ;
	}

	char newName[9];
	char newCust[MAXNPATH];
	char oldCust[MAXNPATH];

	CInputBox inbox( this, 8, MsgGetString(Msg_01754), MsgGetString(Msg_00073), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.SetText( selectedCust.c_str() );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	snprintf( newCust, MAXNPATH, "%s/%s", CLIDIR, newName );

	if( CheckDirectory( newCust ) )
	{
		W_Mess( MsgGetString(Msg_01753) ); // customer already present
		return 0;
	}

	snprintf( oldCust, MAXNPATH, "%s/%s", CLIDIR, selectedCust.c_str() );

	RenameDirectory( oldCust, newCust );

	loadCustomer();
	ShowItems();
	//TODO: per ciascuna funzione decidere quale voce selezionare dopo il comando
	return 1;
}

int CustomersManagerUI::deleteCustomer()
{
	std::string selectedCust = GetSelectedItem();

	if( strcmp( selectedCust.c_str(), QHeader.Cli_Default ) == 0 )
	{
		W_Mess( MsgGetString(Msg_00091) ); // Unable to delete current customer
		return 0 ;
	}

	if( !W_Deci( 0, MsgGetString(Msg_01253) ) ) // erase customer ?
	{
		return 0;
	}

	char cust[MAXNPATH];
	snprintf( cust, MAXNPATH, "%s/%s", CLIDIR, selectedCust.c_str() );

	if( !DeleteDirectory( cust ) )
	{
		W_Mess( MsgGetString(Msg_01031) );
	}

	loadCustomer();
	ShowItems();
	return 1;
}

bool CustomersManagerUI::zipFilesAndCopy( const std::string& cust, const std::string& dest )
{
	char zipFile[MAXNPATH];
	snprintf( zipFile, MAXNPATH, "%s/%s.zip", TEMP_DIR, cust.c_str() );

	ZipClass* z = new ZipClass;
	if( z->Open( zipFile, ZIP_OPEN_CREATE ) != QZIP_OK )
	{
		delete z;
		return false;
	}
	if( !z->IsOpen() )
	{
		delete z;
		return false;
	}

	z->SetZipNotes( BACKUP_CUST_TAG );

	char zipDir[MAXNPATH];
	// zip dir prog
	snprintf( zipDir, MAXNPATH, "%s/%s/%s", CLIDIR, cust.c_str(), PRGDIR );
	if( z->CompressDir( zipDir, Z_DEFAULT_COMPRESSION ) != QZIP_OK )
	{
		delete z;
		return false;
	}
	// zip dir feed
	snprintf( zipDir, MAXNPATH, "%s/%s/%s", CLIDIR, cust.c_str(), CARDIR );
	if( z->CompressDir( zipDir, Z_DEFAULT_COMPRESSION ) != QZIP_OK )
	{
		delete z;
		return false;
	}
	// zip dir vision
	snprintf( zipDir, MAXNPATH, "%s/%s/%s", CLIDIR, cust.c_str(), VISDIR );
	if( z->CompressDir( zipDir, Z_DEFAULT_COMPRESSION ) != QZIP_OK )
	{
		delete z;
		return false;
	}
	// write zip file
	if( z->WriteToZip() != QZIP_OK )
	{
		delete z;
		return false;
	}

	delete z;

	if( !CopyFile( dest.c_str(), zipFile ) )
	{
		return false;
	}

	return true;
}

bool CustomersManagerUI::unzipFiles( const std::string& cust, const std::string& src )
{
	ZipClass* z = new ZipClass;
	if( z->Open( src.c_str(), ZIP_OPEN_READ ) != QZIP_OK )
	{
		delete z;
		W_Mess( MsgGetString(Msg_01766) );
		return false;
	}

	char tag[256];
	bool notag = true;
	if( z->GetZipNotes(tag) )
	{
		if( strcmp( tag, BACKUP_CUST_TAG ) == 0 )
		{
			notag = false;
		}
	}
	if( notag )
	{
		delete z;
		W_Mess( MsgGetString(Msg_01798) );
		return false;
	}

	// check customer
	char newCust[MAXNPATH];
	snprintf( newCust, MAXNPATH, "%s/%s", CLIDIR, cust.c_str() );

	if( CheckDirectory( newCust ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_01765) ) ) // customer already present, proceed ?
		{
			delete z;
			return false;
		}
	}

	// decompress
	if( z->DeCompressAll("") )
	{
		delete z;
		W_Mess( MsgGetString(Msg_01766) );
		return false;
	}

	delete z;
	return true;
}

int CustomersManagerUI::toSharedFolder()
{
	std::string selected = GetSelectedItem();

	char sharedCust[MAXNPATH];
	snprintf( sharedCust, MAXNPATH, "%s/%s.zip", SHAREDIR, selected.c_str() );

	if( !CheckDir(SHAREDIR,CHECKDIR_CREATE) )
	{
		W_Mess( MsgGetString(Msg_01841) );
		return 0;
	}

	if( CheckFile( sharedCust ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_01799) ) )
		{
			return 0;
		}
	}

	//crea se necessario la cartella di lavoro temporanea
	if( !CheckDirectory( TEMP_DIR ) )
	{
		if( mkdir( TEMP_DIR, DIR_CREATION_FLAG ) )
		{
			W_Mess( MsgGetString(Msg_01747) );
			return 0;
		}
	}
	else
	{
		DeleteFiles_OLD( TEMP_DIR );
	}

	if( !zipFilesAndCopy( selected, sharedCust ) )
	{
		W_Mess( MsgGetString(Msg_01747) );
		return 0;
	}

	DeleteDirectory( TEMP_DIR );

	W_Mess( MsgGetString(Msg_01528) );
	return 1;
}

int CustomersManagerUI::fromSharedFolder()
{
	// choose file
	nameList.clear();
	FindFiles( SHAREDIR, "*.zip", nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_01801) );
		return 0;
	}

	std::string selected = fn_Select( this, MsgGetString(Msg_01797), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	char zipFile[MAXNPATH];
	snprintf( zipFile, MAXNPATH, "%s/%s.zip", SHAREDIR, selected.c_str() );

	if( !unzipFiles( selected, zipFile ) )
	{
		return 0;
	}

	W_Mess( MsgGetString(Msg_01796) );

	loadCustomer();
	ShowItems();
	return 1;
}

int CustomersManagerUI::toUSBDevice()
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


	std::string selected = GetSelectedItem();

	// check file
	char deviceCust[MAXNPATH];
	snprintf( deviceCust, MAXNPATH, "%s/%s.zip", selectedDevice.c_str(), selected.c_str() );

	if( CheckFile( deviceCust ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_01799) ) )
		{
			return 0;
		}
	}

	//crea se necessario la cartella di lavoro temporanea
	if( !CheckDirectory( TEMP_DIR ) )
	{
		if( mkdir( TEMP_DIR, DIR_CREATION_FLAG ) )
		{
			W_Mess( MsgGetString(Msg_01747) );
			return 0;
		}
	}
	else
	{
		DeleteFiles_OLD( TEMP_DIR );
	}

	if( !zipFilesAndCopy( selected, deviceCust ) )
	{
		W_Mess( MsgGetString(Msg_01747) );
		return 0;
	}

	DeleteDirectory( TEMP_DIR );

	W_Mess( MsgGetString(Msg_01528) );
	return 1;
}

int CustomersManagerUI::fromUSBDevice()
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

	// choose file
	nameList.clear();
	FindFiles( selectedDevice.c_str(), "*.zip", nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_01801) );
		return 0;
	}

	std::string selected = fn_Select( this, MsgGetString(Msg_01797), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	char zipFile[MAXNPATH];
	snprintf( zipFile, MAXNPATH, "%s/%s.zip", selectedDevice.c_str(), selected.c_str() );

	if( !unzipFiles( selected, zipFile ) )
	{
		return 0;
	}

	W_Mess( MsgGetString(Msg_01796) );

	loadCustomer();
	ShowItems();
	return 1;
}

int fn_CustomersManager()
{
	CustomersManagerUI win( 0 );
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Programs manager
//---------------------------------------------------------------------------
extern int* ComponentList;
extern unsigned int NComponent;
extern unsigned int PlacedNComp;
extern void ri_reset(void);

struct FileHeader header;

ProgramsManagerUI::ProgramsManagerUI( CWindow* parent )
: CWindowSelect( parent, 4, 1, 10, 10 )
{
	SetClientAreaPos( 0, 6 );
	SetClientAreaSize( 54, 13 );

	char buf[80];
	snprintf( buf, 80, "%s - %s", MsgGetString(Msg_00229), QHeader.Cli_Default );
	SetTitle( buf );

	SM_SaveOn = new GUI_SubMenu();
	SM_SaveOn->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &ProgramsManagerUI::toSharedFolder, this ) );
	SM_SaveOn->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &ProgramsManagerUI::toUSBDevice, this ) );

	SM_LoadFrom = new GUI_SubMenu();
	SM_LoadFrom->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &ProgramsManagerUI::fromSharedFolder, this ) );
	SM_LoadFrom->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &ProgramsManagerUI::fromUSBDevice, this ) );

	SM_ImportFrom = new GUI_SubMenu();
	SM_ImportFrom->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &ProgramsManagerUI::asciiQFromSharedFolder, this ) );
	SM_ImportFrom->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &ProgramsManagerUI::asciiQFromUSBDevice, this ) );

	SM_ExportTo = new GUI_SubMenu();
	SM_ExportTo->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &ProgramsManagerUI::asciiQToSharedFolder, this ) );
	SM_ExportTo->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &ProgramsManagerUI::asciiQToUSBDevice, this ) );

	// create combos
	combos[PRG_MODDATE] = new C_Combo( 20, 1, MsgGetString(Msg_00121), 12, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
	combos[PRG_NOTE1]   = new C_Combo( 27, 9, MsgGetString(Msg_06025), 15, CELL_TYPE_TEXT );
	combos[PRG_NOTE2]   = new C_Combo( 34, 10, "", 15, CELL_TYPE_TEXT );

	// add to combo list
	comboList = new CComboList( this );
	comboList->Add( combos[PRG_MODDATE], 0, 0 );
	comboList->Add( combos[PRG_NOTE1],   1, 0 );
	comboList->Add( combos[PRG_NOTE2],   2, 0 );

	loadPrograms();
}

ProgramsManagerUI::~ProgramsManagerUI()
{
	delete SM_SaveOn;
	delete SM_LoadFrom;
	delete SM_ImportFrom;
	delete SM_ExportTo;

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
}

void ProgramsManagerUI::onInitSomething()
{
	m_table->SetOnSelectCellCallback( boost::bind( &ProgramsManagerUI::onSelectionChange, this, _1, _2 ) );
}

void ProgramsManagerUI::onShowSomething()
{
	// show combos
	comboList->Show();
	// deselect first combo
	combos[PRG_NOTE1]->Deselect();

	tips = new CPan( 22, 1, MsgGetString(Msg_00299) );
}

void ProgramsManagerUI::onEnter()
{
	std::string selected = GetSelectedItem();

	snprintf( QHeader.Prg_Default, sizeof(QHeader.Prg_Default), "%s", selected.c_str() );
	Mod_Cfg( QHeader );

	//TODO: ???
	if( strncmp( QHeader.Prg_Default, selected.c_str(), 8 ) )
	{
		if(ComponentList!=NULL)
		{
			delete[] ComponentList;
			ComponentList=NULL;
		}
		NComponent=0;
		PlacedNComp=0;

		//reset valori ripartenza in q_assem.cpp
		ri_reset();
	}

	// check file zeri
	ZerFile* fz = new ZerFile( (char*)selected.c_str() );
	if( !fz->IsOnDisk() )
	{
		fz->Create();
	}
	delete fz;
}

void ProgramsManagerUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_00475), K_F3, 0, NULL, boost::bind( &ProgramsManagerUI::newProgram, this ) ); // new
	m_menu->Add( MsgGetString(Msg_00476), K_F4, 0, NULL, boost::bind( &ProgramsManagerUI::duplicateProgram, this ) ); // duplicate
	m_menu->Add( MsgGetString(Msg_00478), K_F5, 0, NULL, boost::bind( &ProgramsManagerUI::renameProgram, this ) ); // rename
	m_menu->Add( MsgGetString(Msg_00477), K_F6, 0, NULL, boost::bind( &ProgramsManagerUI::deleteProgram, this ) ); // delete
	m_menu->Add( MsgGetString(Msg_00479), K_F7, 0, SM_SaveOn, NULL );   // salva in ...
	m_menu->Add( MsgGetString(Msg_00480), K_F8, 0, SM_LoadFrom, NULL ); // carica da ...
	m_menu->Add( MsgGetString(Msg_00079), K_F9, 0, NULL, boost::bind( &ProgramsManagerUI::insertProgramNotes, this ) ); // program notes
	m_menu->Add( MsgGetString(Msg_00036), K_F11, 0, SM_ImportFrom, NULL ); // import ASCII-Q
	m_menu->Add( MsgGetString(Msg_00037), K_F12, 0, SM_ExportTo, NULL );   // export ASCII-Q
}

bool ProgramsManagerUI::onHotKey( int key )
{
	switch( key )
	{
		case K_F3:
			newProgram();
			return true;

		case K_F4:
			duplicateProgram();
			return true;

		case K_F5:
			renameProgram();
			return true;

		case K_F6:
			deleteProgram();
			return true;

		case K_F7:
			SM_SaveOn->Show();
			return true;

		case K_F8:
			SM_LoadFrom->Show();
			return true;

		case K_F9:
			insertProgramNotes();
			return true;

		default:
			break;
	}

	return false;
}

void ProgramsManagerUI::loadPrograms()
{
	char path[MAXNPATH];
	snprintf( path, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, PRGDIR );

	nameList.clear();
	FindFiles( path, "*" PRGEXT, nameList );

	ClearItems();
	for( unsigned int i = 0; i < nameList.size(); i++ )
	{
		AddItem( nameList[i].c_str() );
	}

	Sort();
}

int ProgramsManagerUI::onSelectionChange( unsigned int row, unsigned int col )
{
	if( GetSelectedItem().empty() )
	{
		return 0;
	}

	TPrgFile* fp = new TPrgFile( GetSelectedItem().c_str(), PRG_NORMAL );
	if( !fp->Open( NOSKIPHEADER ) )
	{
		combos[PRG_MODDATE]->SetTxt( "" );
		combos[PRG_NOTE1]->SetTxt( "" );
		combos[PRG_NOTE2]->SetTxt( "" );
	}
	else
	{
		fp->ReadHeader( header );

		combos[PRG_MODDATE]->SetTxt( header.F_aggi );
		combos[PRG_NOTE1]->SetTxt( header.F_note );
		combos[PRG_NOTE2]->SetTxt( &header.F_note[15] );
	}

	delete fp;

	return 1;
}

int ProgramsManagerUI::newProgram()
{
	char newName[9];
	char newProg[MAXNPATH];

	CInputBox inbox( this, 8, MsgGetString(Msg_00475), MsgGetString(Msg_00983), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	PrgPath( newProg, newName );

	if( CheckFile( newProg ) )
	{
		W_Mess( MsgGetString(Msg_00093) ); // already present
		return 0;
	}

	// crea programma
	TPrgFile* fp = new TPrgFile( newName, PRG_NORMAL );
	int ret = fp->Create();
	delete fp;
	if( !ret )
	{
		W_Mess( MsgGetString(Msg_05187) );
		return 0;
	}

	loadPrograms();
	ShowItems();
	return 1;
}

int ProgramsManagerUI::duplicateProgram()
{
	char newName[9];

	CInputBox inbox( this, 8, MsgGetString(Msg_00476), MsgGetString(Msg_00983), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	char destPrg[MAXNPATH];
	PrgPath( destPrg, newName );
	if( CheckFile( destPrg ) )
	{
		W_Mess( MsgGetString(Msg_00093) ); // already present
		return 0;
	}

	char dir[MAXNPATH];
	snprintf( dir, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, PRGDIR );

	if( !DuplicateFiles( dir, GetSelectedItem().c_str(), newName ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	loadPrograms();
	ShowItems();
	return 1;
}

int ProgramsManagerUI::renameProgram()
{
	std::string selected = GetSelectedItem();

	if( strcmp( selected.c_str(), QHeader.Prg_Default ) == 0 )
	{
		W_Mess( MsgGetString(Msg_00094) );
		return 0 ;
	}

	char newName[9];

	CInputBox inbox( this, 8, MsgGetString(Msg_00478), MsgGetString(Msg_00983), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.SetText( selected.c_str() );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	// check file
	char destPrg[MAXNPATH];
	PrgPath( destPrg, newName );
	if( CheckFile( destPrg ) )
	{
		W_Mess( MsgGetString(Msg_00093) ); // already present
		return 0;
	}

	char dir[MAXNPATH];
	snprintf( dir, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, PRGDIR );

	if( !RenameFiles( dir, selected.c_str(), newName ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	loadPrograms();
	ShowItems();
	return 1;
}

int ProgramsManagerUI::deleteProgram()
{
	std::string selected = GetSelectedItem();

	if( strcmp( selected.c_str(), QHeader.Prg_Default ) == 0 )
	{
		W_Mess( MsgGetString(Msg_00094) );
		return 0 ;
	}

	if( !W_Deci( 0, MsgGetString(Msg_00082) ) ) // erase ?
	{
		return 0;
	}

	bool err = false;
	char dir[MAXNPATH];

	// delete program data
	snprintf( dir, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, PRGDIR );
	if( !DeleteFiles( dir, selected.c_str() ) )
	{
		err = true;
	}

	// delete vision data
	snprintf( dir, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, VISIONDIR );
	if( !DeleteFiles( dir, selected.c_str() ) )
	{
		err = true;
	}

	if( err )
	{
		W_Mess( MsgGetString(Msg_00199) );
	}

	loadPrograms();
	ShowItems();
	return 1;
}

int ProgramsManagerUI::toSharedFolder()
{
	std::string selected = GetSelectedItem();

	// check file
	char sharedProg[MAXNPATH];
	snprintf( sharedProg, MAXNPATH, "%s/%s%s", SHAREDIR, selected.c_str(), PRGEXT );

	if( !CheckDir(SHAREDIR,CHECKDIR_CREATE) )
	{
		W_Mess( MsgGetString(Msg_01841) );
		return 0;
	}

	if( CheckFile( sharedProg ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00088) ) )
		{
			return 0;
		}
	}

	// copy files
	char sDir[MAXNPATH];
	snprintf( sDir, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, PRGDIR );

	if( !CopyFiles( SHAREDIR, sDir, selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_01843) );
	return 1;
}

int ProgramsManagerUI::fromSharedFolder()
{
	// load files
	nameList.clear();
	FindFiles( SHAREDIR, "*" PRGEXT, nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00101) );
		return 0;
	}

	// select file
	std::string selected = fn_Select( this, MsgGetString(Msg_05053), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	//TODO controllare che non sia quello attuale
	//TODO se non possibile copiare chiedere utente nuovo nome (FARE PER TUTTI)

	// check file
	char destPrg[MAXNPATH];
	PrgPath( destPrg, selected.c_str() );
	if( CheckFile( destPrg ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00088) ) )
		{
			return 0;
		}
	}

	// copy files
	char dDir[MAXNPATH];
	snprintf( dDir, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, PRGDIR );

	if( !CopyFiles( dDir, SHAREDIR, selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	if( !ModifyFilesExt( dDir, selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_01843) );

	loadPrograms();
	ShowItems();
	return 1;
}

int ProgramsManagerUI::toUSBDevice()
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

	std::string selected = GetSelectedItem();

	// check file
	char deviceProg[MAXNPATH];
	snprintf( deviceProg, MAXNPATH, "%s/%s%s", selectedDevice.c_str(), selected.c_str(), PRGEXT );

	if( CheckFile( deviceProg ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00088) ) )
		{
			return 0;
		}
	}

	// copy files
	char sDir[MAXNPATH];
	snprintf( sDir, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, PRGDIR );

	if( !CopyFiles( selectedDevice.c_str(), sDir, selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_01843) );
	return 1;
}

int ProgramsManagerUI::fromUSBDevice()
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

	// load files
	nameList.clear();
	FindFiles( (char*)selectedDevice.c_str(), "*" PRGEXT, nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00101) );
		return 0;
	}

	// select file
	std::string selected = fn_Select( this, MsgGetString(Msg_02105), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	// check file
	char destPrg[MAXNPATH];
	PrgPath( destPrg, selected.c_str() );
	if( CheckFile( destPrg ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00088) ) )
		{
			return 0;
		}
	}

	// copy files
	char dDir[MAXNPATH];
	snprintf( dDir, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, PRGDIR );

	if( !CopyFiles( dDir, selectedDevice.c_str(), selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_01843) );

	loadPrograms();
	ShowItems();
	return 1;
}

#define PRG_NOTES_LEN     30
int ProgramsManagerUI::insertProgramNotes()
{
	CInputBox inbox( this, 8, MsgGetString(Msg_00079), "", PRG_NOTES_LEN, CELL_TYPE_TEXT );
	inbox.SetText( header.F_note );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	TPrgFile* fp = new TPrgFile( GetSelectedItem().c_str(), PRG_NORMAL );
	if( fp->Open( NOSKIPHEADER ) )
	{
		snprintf( header.F_note, PRG_NOTES_LEN, "%s", inbox.GetText() );

		fp->WriteHeader( header );

		combos[PRG_NOTE1]->SetTxt( header.F_note );
		combos[PRG_NOTE2]->SetTxt( &header.F_note[15] );
	}
	delete fp;

	return 1;
}

int ProgramsManagerUI::asciiQFromSharedFolder()
{
	// load files
	nameList.clear();
	FindFiles( SHAREDIR, "*" ASCIIQ_EXT, nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00101) );
		return 0;
	}

	// select file
	std::string selected = fn_Select( this, MsgGetString(Msg_05053), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	// check file
	char destPrg[MAXNPATH];
	PrgPath( destPrg, selected.c_str() );
	if( CheckFile( destPrg ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00088) ) )
		{
			return 0;
		}
	}

	// import file
	char asqFile[MAXNPATH];
	snprintf( asqFile, MAXNPATH, "%s/%s%s", SHAREDIR, selected.c_str(), ASCIIQ_EXT );

	if( !Program_ImportAsciiQ( selected.c_str(), asqFile ) )
	{
		W_Mess( MsgGetString(Msg_00066) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00050) );

	loadPrograms();
	ShowItems();
	return 1;
}

int ProgramsManagerUI::asciiQToSharedFolder()
{
	std::string selected = GetSelectedItem();

	// check file
	char asqFile[MAXNPATH];
	if( Get_AsciiqIdMode() )
		snprintf( asqFile, MAXNPATH, "%s/%s_%s%s", SHAREDIR, selected.c_str(), nwpar.NetID, ASCIIQ_EXT );
	else
		snprintf( asqFile, MAXNPATH, "%s/%s%s", SHAREDIR, selected.c_str(), ASCIIQ_EXT );

	if( !CheckDir(SHAREDIR,CHECKDIR_CREATE) )
	{
		W_Mess( MsgGetString(Msg_01841) );
		return 0;
	}

	if( CheckFile( asqFile ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00088) ) )
		{
			return 0;
		}
	}

	// export file
	if( !Program_ExportAsciiQ( selected.c_str(), asqFile ) )
	{
		W_Mess( MsgGetString(Msg_00067) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00065) );
	return 1;
}

int ProgramsManagerUI::asciiQFromUSBDevice()
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

	// load files
	nameList.clear();
	FindFiles( (char*)selectedDevice.c_str(), "*" ASCIIQ_EXT, nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00101) );
		return 0;
	}

	// select file
	std::string selected = fn_Select( this, MsgGetString(Msg_02105), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	// check file
	char destPrg[MAXNPATH];
	PrgPath( destPrg, selected.c_str() );
	if( CheckFile( destPrg ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00088) ) )
		{
			return 0;
		}
	}

	// import file
	char asqFile[MAXNPATH];
	snprintf( asqFile, MAXNPATH, "%s/%s%s", selectedDevice.c_str(), selected.c_str(), ASCIIQ_EXT );

	if( !Program_ImportAsciiQ( selected.c_str(), asqFile ) )
	{
		W_Mess( MsgGetString(Msg_00066) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00050) );

	loadPrograms();
	ShowItems();
	return 1;
}

int ProgramsManagerUI::asciiQToUSBDevice()
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

	std::string selected = GetSelectedItem();

	// check file
	char asqFile[MAXNPATH];
	if( Get_AsciiqIdMode() )
		snprintf( asqFile, MAXNPATH, "%s/%s_%s%s", selectedDevice.c_str(), selected.c_str(), nwpar.NetID, ASCIIQ_EXT );
	else
		snprintf( asqFile, MAXNPATH, "%s/%s%s", selectedDevice.c_str(), selected.c_str(), ASCIIQ_EXT );

	if( CheckFile( asqFile ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00088) ) )
		{
			return 0;
		}
	}

	// export file
	if( !Program_ExportAsciiQ( selected.c_str(), asqFile ) )
	{
		W_Mess( MsgGetString(Msg_00067) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00065) );
	return 1;
}


int fn_ProgramsManager()
{
	if( *QHeader.Cli_Default == 0 )
	{
		W_Mess( MsgGetString(Msg_00935) );
		return 0;
	}

	char tmp[9];
	strcpy( tmp, QHeader.Conf_Default );

	ProgramsManagerUI win( 0 );
	win.Show();
	win.Hide();

	//TODO: ???
	if((QHeader.modal & ENABLE_CARINT) && (strcasecmpQ(tmp,QHeader.Conf_Default)))
	{
		if(UpdateDBData(CARINT_UPDATE_FULL))
		{
			ConfImport(0);
		}
	}

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Feeder configuration manager
//---------------------------------------------------------------------------
FeederConfigManagerUI::FeederConfigManagerUI( CWindow* parent )
: CWindowSelect( parent, 7, 1, 10, 10 )
{
	SetClientAreaPos( 0, 6 );
	SetTitle( MsgGetString(Msg_00958) );

	SM_SaveOn = new GUI_SubMenu();
	SM_SaveOn->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &FeederConfigManagerUI::toSharedFolder, this ) ); // shared network
	SM_SaveOn->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &FeederConfigManagerUI::toUSBDevice, this ) ); // usb device

	SM_LoadFrom = new GUI_SubMenu();
	SM_LoadFrom->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &FeederConfigManagerUI::fromSharedFolder, this ) ); // shared network
	SM_LoadFrom->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &FeederConfigManagerUI::fromUSBDevice, this ) ); // usb pen

	SM_ExportTo = new GUI_SubMenu();
	SM_ExportTo->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &FeederConfigManagerUI::csvToSharedFolder, this ) );
	SM_ExportTo->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &FeederConfigManagerUI::csvToUSBDevice, this ) );

	loadFeederConfigs();
}

FeederConfigManagerUI::~FeederConfigManagerUI()
{
	delete SM_SaveOn;
	delete SM_LoadFrom;
}

void FeederConfigManagerUI::onShowSomething()
{
	tips = new CPan( 22, 1, MsgGetString(Msg_00300) );
}

void FeederConfigManagerUI::onEnter()
{
	std::string selected = GetSelectedItem();
	snprintf( QHeader.Conf_Default, sizeof(QHeader.Conf_Default), "%s", selected.c_str() );
	Mod_Cfg( QHeader );

	if( *QHeader.Prg_Default != 0 )
	{
		Save_PrgCFile( QHeader.Lib_Default, QHeader.Conf_Default );
	}
}

void FeederConfigManagerUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_00475), K_F3, 0, NULL, boost::bind( &FeederConfigManagerUI::newFeederConfig, this ) ); // new
	m_menu->Add( MsgGetString(Msg_00476), K_F4, 0, NULL, boost::bind( &FeederConfigManagerUI::duplicateFeederConfig, this ) ); // duplicate
	m_menu->Add( MsgGetString(Msg_00478), K_F5, 0, NULL, boost::bind( &FeederConfigManagerUI::renameFeederConfig, this ) ); // rename
	m_menu->Add( MsgGetString(Msg_00477), K_F6, 0, NULL, boost::bind( &FeederConfigManagerUI::deleteFeederConfig, this ) ); // delete
	m_menu->Add( MsgGetString(Msg_00479), K_F7, 0, SM_SaveOn, NULL ); // salva in ...
	m_menu->Add( MsgGetString(Msg_00480), K_F8, 0, SM_LoadFrom, NULL ); // carica da ...
	m_menu->Add( MsgGetString(Msg_01041), K_F9, 0, NULL, boost::bind( &FeederConfigManagerUI::importFromCustomer, this ) ); // importa da cliente
	m_menu->Add( MsgGetString(Msg_00009), K_F12, 0, SM_ExportTo, NULL );
}

bool FeederConfigManagerUI::onHotKey( int key )
{
	switch( key )
	{
		case K_F3:
			newFeederConfig();
			return true;

		case K_F4:
			duplicateFeederConfig();
			return true;

		case K_F5:
			renameFeederConfig();
			return true;

		case K_F6:
			deleteFeederConfig();
			return true;

		case K_F7:
			SM_SaveOn->Show();
			return true;

		case K_F8:
			SM_LoadFrom->Show();
			return true;

		case K_F9:
			importFromCustomer();
			return true;

		case K_F12:
			SM_ExportTo->Show();
			return true;

		default:
			break;
	}

	return false;
}

void FeederConfigManagerUI::loadFeederConfigs()
{
	char path[MAXNPATH];

	if( !Get_UseCommonFeederDir() )
	{
		snprintf( path, MAXNPATH, "%s/%s/%s", CLIDIR, QHeader.Cli_Default, CARDIR );
	}
	else
	{
		snprintf( path, MAXNPATH, "%s", CARDIR );
	}

	nameList.clear();
	FindFiles( path, "*" CAREXT, nameList );

	// check presenza almeno una voce
	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00924) );
	}

	ClearItems();
	for( unsigned int i = 0; i < nameList.size(); i++ )
	{
		AddItem( nameList[i].c_str() );
	}
	Sort();
}

int FeederConfigManagerUI::newFeederConfig()
{
	char newName[9];
	char newConf[MAXNPATH];

	CInputBox inbox( this, 8, MsgGetString(Msg_00475), MsgGetString(Msg_00170), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	CarPath( newConf, newName );

	if( CheckFile( newConf ) )
	{
		W_Mess( MsgGetString(Msg_00092) ); // already present
		return 0;
	}

	// crea configurazione caricatori
	FeedersConfig_Create( newName );

	loadFeederConfigs();
	ShowItems();
	return 1;
}

int FeederConfigManagerUI::duplicateFeederConfig()
{
	std::string selected = GetSelectedItem();

	char newName[9];
	char newConf[MAXNPATH];
	char oldConf[MAXNPATH];

	CInputBox inbox( this, 8, MsgGetString(Msg_00476), MsgGetString(Msg_00170), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	CarPath( newConf, newName );

	if( CheckFile( newConf ) )
	{
		W_Mess( MsgGetString(Msg_00092) ); // already present
		return 0;
	}

	CarPath( oldConf, selected.c_str() );
	if( !CopyFile( newConf, oldConf ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	loadFeederConfigs();
	ShowItems();
	return 1;
}

int FeederConfigManagerUI::renameFeederConfig()
{
	std::string selected = GetSelectedItem();

	if( strcmp( selected.c_str(), QHeader.Conf_Default ) == 0 )
	{
		W_Mess( MsgGetString(Msg_01483) );
		return 0 ;
	}

	char newName[9];
	char newConf[MAXNPATH];
	char oldConf[MAXNPATH];


	CInputBox inbox( this, 8, MsgGetString(Msg_00478), MsgGetString(Msg_00170), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.SetText( selected.c_str() );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	CarPath( newConf, newName );

	if( CheckFile( newConf ) )
	{
		W_Mess( MsgGetString(Msg_00092) ); // already present
		return 0;
	}

	CarPath( oldConf, selected.c_str() );
	rename( oldConf, newConf );

	loadFeederConfigs();
	ShowItems();
	return 1;
}

int FeederConfigManagerUI::deleteFeederConfig()
{
	std::string selected = GetSelectedItem();

	if( strcmp( selected.c_str(), QHeader.Conf_Default ) == 0 )
	{
		W_Mess( MsgGetString(Msg_01483) );
		return 0 ;
	}

	if( !W_Deci( 0, MsgGetString(Msg_00087) ) ) // erase library ?
	{
		return 0;
	}

	char conf[MAXNPATH];
	CarPath( conf, selected.c_str() );

	remove( conf );

	loadFeederConfigs();
	ShowItems();
	return 1;
}

int FeederConfigManagerUI::toSharedFolder()
{
	std::string selected = GetSelectedItem();

	char conf[MAXNPATH];
	CarPath( conf, selected.c_str() );

	char sharedConf[MAXNPATH];
	snprintf( sharedConf, MAXNPATH, "%s/%s%s", SHAREDIR, selected.c_str(), CAREXT );

	if( !CheckDir(SHAREDIR,CHECKDIR_CREATE) )
	{
		W_Mess( MsgGetString(Msg_01841) );
		return 0;
	}

	if( CheckFile( sharedConf ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00086) ) )
		{
			return 0;
		}
	}

	if( !CopyFile( sharedConf, conf ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_01845) );
	return 1;
}

int FeederConfigManagerUI::fromSharedFolder()
{
	nameList.clear();
	FindFiles( SHAREDIR, "*" CAREXT, nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00924) );
		return 0;
	}

	std::string selected = fn_Select( this, MsgGetString(Msg_05053), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	char conf[MAXNPATH];
	CarPath( conf, selected.c_str() );

	if( CheckFile( conf ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00086) ) )
		{
			return 0;
		}
	}

	char sharedConf[MAXNPATH];
	snprintf( sharedConf, MAXNPATH, "%s/%s%s", SHAREDIR, selected.c_str(), CAREXT );

	if( !CopyFile( conf, sharedConf ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	//TODO - estensione minuscola
	/*
	if( !ModifyFilesExt( dDir, selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}
	*/

	loadFeederConfigs();
	ShowItems();
	return 1;
}

int FeederConfigManagerUI::toUSBDevice()
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

	std::string selected = GetSelectedItem();

	// check file
	char deviceConf[MAXNPATH];
	snprintf( deviceConf, MAXNPATH, "%s/%s%s", selectedDevice.c_str(), selected.c_str(), CAREXT );

	if( CheckFile( deviceConf ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00086) ) )
		{
			return 0;
		}
	}

	// copy file
	char conf[MAXNPATH];
	CarPath( conf, selected.c_str() );

	if( !CopyFile( deviceConf, conf ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_01845) );
	return 1;
}

int FeederConfigManagerUI::fromUSBDevice()
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

	// choose file
	nameList.clear();
	FindFiles( (char*)selectedDevice.c_str(), "*" CAREXT, nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00924) );
		return 0;
	}

	std::string selected = fn_Select( this, MsgGetString(Msg_02105), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	// check file
	char conf[MAXNPATH];
	CarPath( conf, selected.c_str() );

	if( CheckFile( conf ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00086) ) )
		{
			return 0;
		}
	}

	// copy file
	char deviceConf[MAXNPATH];
	snprintf( deviceConf, MAXNPATH, "%s/%s%s", selectedDevice.c_str(), selected.c_str(), CAREXT );

	if( !CopyFile( conf, deviceConf ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	loadFeederConfigs();
	ShowItems();
	return 1;
}

int FeederConfigManagerUI::importFromCustomer()
{
	// choose customer
	nameList.clear();
	FindFiles( CLIDIR, 0, nameList );

	for( unsigned int i = 0; i < nameList.size(); i++ )
	{
		if( nameList[i].compare( QHeader.Cli_Default ) == 0 )
		{
			nameList.erase( nameList.begin() + i );
		}
	}

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00295) );
		return 0;
	}

	std::string selectedCust = fn_Select( this, MsgGetString(Msg_00070), nameList );
	if( selectedCust.empty() )
	{
		return 0;
	}

	// choose file
	char path[MAXNPATH];
	snprintf( path, MAXNPATH, "%s/%s/%s", CLIDIR, selectedCust.c_str(), CARDIR );

	nameList.clear();
	FindFiles( path, "*" CAREXT, nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00924) );
		return 0;
	}

	std::string selectedConf = fn_Select( this, MsgGetString(Msg_00958), nameList );
	if( selectedConf.empty() )
	{
		return 0;
	}

	// input new name
	char newName[9];

	CInputBox inbox( this, 8, MsgGetString(Msg_01041), MsgGetString(Msg_00170), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	// check file
	char newConf[MAXNPATH];
	CarPath( newConf, newName );

	if( CheckFile( newConf ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00086) ) )
		{
			return 0;
		}
	}

	// copy file
	snprintf( path, MAXNPATH, "%s/%s/%s/%s%s", CLIDIR, selectedCust.c_str(), CARDIR, selectedConf.c_str(), CAREXT );

	if( !CopyFile( newConf, path ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	loadFeederConfigs();
	ShowItems();
	return 1;
}

int FeederConfigManagerUI::csvToSharedFolder()
{
	std::string selected = GetSelectedItem();

	// check file
	char csvFile[MAXNPATH];
	snprintf( csvFile, MAXNPATH, "%s/%s%s", SHAREDIR, selected.c_str(), CSV_EXT );

	if( !CheckDir(SHAREDIR,CHECKDIR_CREATE) )
	{
		W_Mess( MsgGetString(Msg_01841) );
		return 0;
	}

	if( CheckFile( csvFile ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00086) ) )
		{
			return 0;
		}
	}

	// export file
	if( !FeederConfig_ExportCSV( selected.c_str(), csvFile ) )
	{
		W_Mess( MsgGetString(Msg_00067) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00222) );
	return 1;
}

int FeederConfigManagerUI::csvToUSBDevice()
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

	std::string selected = GetSelectedItem();

	// check file
	char csvFile[MAXNPATH];
	snprintf( csvFile, MAXNPATH, "%s/%s%s", selectedDevice.c_str(), selected.c_str(), CSV_EXT );

	if( CheckFile( csvFile ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00086) ) )
		{
			return 0;
		}
	}

	// export file
	if( !FeederConfig_ExportCSV( selected.c_str(), csvFile ) )
	{
		W_Mess( MsgGetString(Msg_00067) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_00222) );
	return 1;
}

int fn_FeederConfigManager()
{
	if( *QHeader.Cli_Default == 0 )
	{
		W_Mess( MsgGetString(Msg_00935) );
		return 0;
	}

	char tmp[9];
	strcpy( tmp, QHeader.Conf_Default );

	FeederConfigManagerUI win( 0 );
	win.Show();
	win.Hide();


	//TODO: ???
	if((QHeader.modal & ENABLE_CARINT) && (strcasecmpQ(tmp,QHeader.Conf_Default)))
	{
		if(UpdateDBData(CARINT_UPDATE_FULL))
		{
			ConfImport(0);
		}
	}

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Packages library manager
//---------------------------------------------------------------------------
PackagesLibManagerUI::PackagesLibManagerUI( CWindow* parent )
: CWindowSelect( parent, 7, 1, 10, 10 )
{
	SetClientAreaPos( 0, 6 );
	SetTitle( MsgGetString(Msg_00231) );

	SM_SaveOn = new GUI_SubMenu();
	SM_SaveOn->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &PackagesLibManagerUI::toSharedFolder, this ) ); // shared network
	SM_SaveOn->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &PackagesLibManagerUI::toUSBDevice, this ) ); // usb device

	SM_LoadFrom = new GUI_SubMenu();
	SM_LoadFrom->Add( MsgGetString(Msg_05053), K_F1, IsNetEnabled() ? 0 : SM_GRAYED, NULL, boost::bind( &PackagesLibManagerUI::fromSharedFolder, this ) ); // shared network
	SM_LoadFrom->Add( MsgGetString(Msg_02105), K_F2, 0, NULL, boost::bind( &PackagesLibManagerUI::fromUSBDevice, this ) ); // usb pen

	loadPackagesLibs();
}

PackagesLibManagerUI::~PackagesLibManagerUI()
{
	delete SM_SaveOn;
	delete SM_LoadFrom;
}

void PackagesLibManagerUI::onShowSomething()
{
	tips = new CPan( 22, 1, MsgGetString(Msg_00301) );
}

void PackagesLibManagerUI::onEnter()
{
	// select packages lib
	std::string selectedLib = GetSelectedItem();
	snprintf( QHeader.Lib_Default, sizeof(QHeader.Lib_Default), "%s", selectedLib.c_str() );
	Mod_Cfg( QHeader );

	if( *QHeader.Prg_Default != 0 )
	{
		Save_PrgCFile( QHeader.Lib_Default, QHeader.Conf_Default );
	}
}

void PackagesLibManagerUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_00475), K_F3, 0, NULL, boost::bind( &PackagesLibManagerUI::newPackagesLib, this ) ); // new
	m_menu->Add( MsgGetString(Msg_00476), K_F4, 0, NULL, boost::bind( &PackagesLibManagerUI::duplicatePackagesLib, this ) ); // duplicate
	m_menu->Add( MsgGetString(Msg_00478), K_F5, 0, NULL, boost::bind( &PackagesLibManagerUI::renamePackagesLib, this ) ); // rename
	m_menu->Add( MsgGetString(Msg_00477), K_F6, 0, NULL, boost::bind( &PackagesLibManagerUI::deletePackagesLib, this ) ); // delete
	m_menu->Add( MsgGetString(Msg_00479), K_F7, 0, SM_SaveOn, NULL ); // salva in ...
	m_menu->Add( MsgGetString(Msg_00480), K_F8, 0, SM_LoadFrom, NULL ); // carica da ...
}

bool PackagesLibManagerUI::onHotKey( int key )
{
	switch( key )
	{
		case K_F3:
			newPackagesLib();
			return true;

		case K_F4:
			duplicatePackagesLib();
			return true;

		case K_F5:
			renamePackagesLib();
			return true;

		case K_F6:
			deletePackagesLib();
			return true;

		case K_F7:
			SM_SaveOn->Show();
			return true;

		case K_F8:
			SM_LoadFrom->Show();
			return true;

		default:
			break;
	}

	return false;
}

void PackagesLibManagerUI::loadPackagesLibs()
{
	nameList.clear();
	FindFiles( FPACKDIR, "*" PACKAGESLIB_EXT, nameList );

	// check presenza almeno una directory cliente
	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00943) );
	}

	ClearItems();
	for( unsigned int i = 0; i < nameList.size(); i++ )
	{
		AddItem( nameList[i].c_str() );
	}
	Sort();
}

int PackagesLibManagerUI::newPackagesLib()
{
	char newName[9];
	CInputBox inbox( this, 8, MsgGetString(Msg_00475), MsgGetString(Msg_00171), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	char newLib[MAXNPATH];
	snprintf( newLib, MAXNPATH, "%s/%s%s", FPACKDIR, newName, PACKAGESLIB_EXT );

	if( CheckFile( newLib ) )
	{
		W_Mess( MsgGetString(Msg_00444) ); // lib already present
		return 0;
	}

	// crea libreria packages
	PackagesLib_Create( newName );

	loadPackagesLibs();
	ShowItems();
	return 1;
}

int PackagesLibManagerUI::duplicatePackagesLib()
{
	char newName[9];
	CInputBox inbox( this, 8, MsgGetString(Msg_00476), MsgGetString(Msg_00171), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	// check file
	char destLib[MAXNPATH];
	snprintf( destLib, MAXNPATH, "%s/%s%s", FPACKDIR, newName, PACKAGESLIB_EXT );

	if( CheckFile( destLib ) )
	{
		W_Mess( MsgGetString(Msg_00444) ); // already present
		return 0;
	}

	// duplicate files
	if( !DuplicateFiles( FPACKDIR, GetSelectedItem().c_str(), newName ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	loadPackagesLibs();
	ShowItems();
	return 1;
}

int PackagesLibManagerUI::renamePackagesLib()
{
	std::string selectedLib = GetSelectedItem();

	if( strcmp( selectedLib.c_str(), QHeader.Lib_Default ) == 0 )
	{
		W_Mess( MsgGetString(Msg_00944) );
		return 0 ;
	}

	char newName[9];
	CInputBox inbox( this, 8, MsgGetString(Msg_00478), MsgGetString(Msg_00171), 8, CELL_TYPE_TEXT );
	inbox.SetLegalChars( CHARSET_FILENAME );
	inbox.SetText( selectedLib.c_str() );
	inbox.Show();

	if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		return 0;
	}

	snprintf( newName, 9, "%s", inbox.GetText() );
	strlwr( newName );

	// check file
	char destLib[MAXNPATH];
	snprintf( destLib, MAXNPATH, "%s/%s%s", FPACKDIR, newName, PACKAGESLIB_EXT );

	if( CheckFile( destLib ) )
	{
		W_Mess( MsgGetString(Msg_00444) ); // already present
		return 0;
	}

	// rename files
	if( !RenameFiles( FPACKDIR, selectedLib.c_str(), newName ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	loadPackagesLibs();
	ShowItems();
	return 1;
}

int PackagesLibManagerUI::deletePackagesLib()
{
	std::string selectedLib = GetSelectedItem();

	if( strcmp( selectedLib.c_str(), QHeader.Lib_Default ) == 0 )
	{
		W_Mess( MsgGetString(Msg_00942) );
		return 0 ;
	}

	if( !W_Deci( 0, MsgGetString(Msg_00941) ) ) // erase library ?
	{
		return 0;
	}

	// delete files
	if( !DeleteFiles( FPACKDIR, selectedLib.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_00199) );
		return 0;
	}

	loadPackagesLibs();
	ShowItems();
	return 1;
}

int PackagesLibManagerUI::toSharedFolder()
{
	std::string selected = GetSelectedItem();

	// check file
	char sharedLib[MAXNPATH];
	snprintf( sharedLib, MAXNPATH, "%s/%s%s", SHAREDIR, selected.c_str(), PACKAGESLIB_EXT );

	if( !CheckDir(SHAREDIR,CHECKDIR_CREATE) )
	{
		W_Mess( MsgGetString(Msg_01841) );
		return 0;
	}

	if( CheckFile( sharedLib ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00095) ) )
		{
			return 0;
		}
	}

	// copy files
	if( !CopyFiles( SHAREDIR, FPACKDIR, selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_01848) );
	return 1;
}

int PackagesLibManagerUI::fromSharedFolder()
{
	// select file
	nameList.clear();
	FindFiles( SHAREDIR, "*" PACKAGESLIB_EXT, nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00943) );
		return 0;
	}

	std::string selected = fn_Select( this, MsgGetString(Msg_05053), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	// check file
	char lib[MAXNPATH];
	snprintf( lib, MAXNPATH, "%s/%s%s", FPACKDIR, selected.c_str(), PACKAGESLIB_EXT );

	if( CheckFile( lib ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00095) ) )
		{
			return 0;
		}
	}

	// copy files
	if( !CopyFiles( FPACKDIR, SHAREDIR, selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	loadPackagesLibs();
	ShowItems();
	return 1;
}

int PackagesLibManagerUI::toUSBDevice()
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

	std::string selected = GetSelectedItem();

	// check file
	char deviceLib[MAXNPATH];
	snprintf( deviceLib, MAXNPATH, "%s/%s%s", selectedDevice.c_str(), selected.c_str(), PACKAGESLIB_EXT );
	if( CheckFile( deviceLib ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00095) ) )
		{
			return 0;
		}
	}

	// copy files
	if( !CopyFiles( selectedDevice.c_str(), FPACKDIR, selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	W_Mess( MsgGetString(Msg_01848) );
	return 1;
}

int PackagesLibManagerUI::fromUSBDevice()
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

	// choose file
	nameList.clear();
	FindFiles( (char*)selectedDevice.c_str(), "*" PACKAGESLIB_EXT, nameList );

	if( nameList.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_00943) );
		return 0;
	}

	std::string selected = fn_Select( this, MsgGetString(Msg_02105), nameList );
	if( selected.empty() )
	{
		return 0;
	}

	// check file
	char lib[MAXNPATH];
	snprintf( lib, MAXNPATH, "%s/%s%s", FPACKDIR, selected.c_str(), PACKAGESLIB_EXT );

	if( CheckFile( lib ) )
	{
		if( !W_Deci( 0, MsgGetString(Msg_00095) ) )
		{
			return 0;
		}
	}

	// copy files
	if( !CopyFiles( FPACKDIR, selectedDevice.c_str(), selected.c_str() ) )
	{
		W_Mess( MsgGetString(Msg_01872) );
		return 0;
	}

	loadPackagesLibs();
	ShowItems();
	return 1;
}

int fn_PackagesLibManager()
{
	PackagesLibManagerUI win( 0 );
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Quick load programs list
//---------------------------------------------------------------------------
extern SQuickStart QuickStartList;

class QuickLoadProgramsUI : public CWindowTable
{
public:
	QuickLoadProgramsUI() : CWindowTable( 0 )
	{
		SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU );
		SetClientAreaPos( 0, 8 );
		SetClientAreaSize( 65, QUICKSTARTLIST_NUM + 3 );

		SetTitle( MsgGetString(Msg_00661) );
	}

protected:
	void onInit()
	{
		// create table
		m_table = new CTable( 3, 2, QUICKSTARTLIST_NUM, TABLE_STYLE_DEFAULT, this );

		// add columns
		m_table->AddCol( "", 3, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_CENTERED );
		m_table->AddCol( MsgGetString(Msg_00045), 13, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
		m_table->AddCol( MsgGetString(Msg_00046), 13, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
		m_table->AddCol( MsgGetString(Msg_01066), 13, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
		m_table->AddCol( MsgGetString(Msg_01067), 13, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );

		// set params
		char Fn[] = "Fn";
		for( int i = 0; i < QUICKSTARTLIST_NUM; i++ )
		{
			Fn[1] = '1' + i;
			m_table->SetText( i, 0, Fn );
		}
	}

	void onShow()
	{
		//tips = new CPan( 22, 1, MsgGetString(Msg_00318) );
	}

	void onRefresh()
	{
		for( int i = 0; i < QUICKSTARTLIST_NUM; i++ )
		{
			m_table->SetText( i, 1, QuickStartList.list[i*4] );
			m_table->SetText( i, 2, QuickStartList.list[i*4+1] );
			m_table->SetText( i, 3, QuickStartList.list[i*4+2] );
			m_table->SetText( i, 4, QuickStartList.list[i*4+3] );
		}
	}

	void onEdit()
	{
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_ENTER:
				onEnter();
				return true;

			case K_DEL:
				onDelete();
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		QuickStart_Write( QuickStartList );
		//delete tips;
	}

	int onEnter()
	{
		std::vector<std::string> nameList;
		char path[MAXNPATH];

		// selezione cliente
		FindFiles( CLIDIR, 0, nameList );

		if( nameList.size() == 0 )
		{
			//TODO: messaggio di errore
		}

		std::string selectedCustomer = fn_Select( this, MsgGetString(Msg_00070), nameList );
		if( selectedCustomer.empty() )
		{
			return 0;
		}


		// selezione programma
		snprintf( path, MAXNPATH, "%s/%s/%s", CLIDIR, selectedCustomer.c_str(), PRGDIR );

		nameList.clear();
		FindFiles( path, "*" PRGEXT, nameList );

		if( nameList.size() == 0 )
		{
			W_Mess( MsgGetString(Msg_00090) );
		}

		std::string selectedProgram = fn_Select( this, MsgGetString(Msg_00229), nameList );
		if( selectedProgram.empty() )
		{
			return 0;
		}


		// selezione configurazione
		if( !Get_UseCommonFeederDir() )
		{
			snprintf( path, MAXNPATH, "%s/%s/%s", CLIDIR, selectedCustomer.c_str(), CARDIR );
		}
		else
		{
			snprintf( path, MAXNPATH, "%s", CARDIR );
		}

		nameList.clear();
		FindFiles( path, "*" CAREXT, nameList );

		// check presenza almeno una voce
		if( nameList.size() == 0 )
		{
			W_Mess( MsgGetString(Msg_00924) );
		}

		std::string selectedConf = fn_Select( this, MsgGetString(Msg_00958), nameList );
		if( selectedConf.empty() )
		{
			return 0;
		}


		// selezione libreria packages
		nameList.clear();
		FindFiles( FPACKDIR, "*" PACKAGESLIB_EXT, nameList );

		// check presenza almeno una directory cliente
		if( nameList.size() == 0 )
		{
			W_Mess( MsgGetString(Msg_00943) );
		}

		std::string selectedLib = fn_Select( this, MsgGetString(Msg_00231), nameList );
		if( selectedLib.empty() )
		{
			return 0;
		}

		int row = m_table->GetCurRow();
		m_table->SetText( row, 1, selectedCustomer.c_str() );
		m_table->SetText( row, 2, selectedProgram.c_str() );
		m_table->SetText( row, 3, selectedConf.c_str() );
		m_table->SetText( row, 4, selectedLib.c_str() );

		snprintf( QuickStartList.list[row*4], QUICKSTARTTXT_LEN, "%s", selectedCustomer.c_str() );
		snprintf( QuickStartList.list[row*4+1], QUICKSTARTTXT_LEN, "%s", selectedProgram.c_str() );
		snprintf( QuickStartList.list[row*4+2], QUICKSTARTTXT_LEN, "%s", selectedConf.c_str() );
		snprintf( QuickStartList.list[row*4+3], QUICKSTARTTXT_LEN, "%s", selectedLib.c_str() );

		//TODO: sistemare
		std::string cli_old = QHeader.Cli_Default;
		std::string prg_old = QHeader.Prg_Default;

		snprintf( QHeader.Cli_Default, sizeof(QHeader.Cli_Default), "%s", selectedCustomer.c_str() );
		snprintf( QHeader.Prg_Default, sizeof(QHeader.Prg_Default), "%s", selectedProgram.c_str() );

		Save_PrgCFile( selectedLib.c_str(), selectedConf.c_str() );

		snprintf( QHeader.Cli_Default, sizeof(QHeader.Cli_Default), "%s", cli_old.c_str() );
		snprintf( QHeader.Prg_Default, sizeof(QHeader.Prg_Default), "%s", prg_old.c_str() );

		return 1;
	}

	int onDelete()
	{
		int row = m_table->GetCurRow();
		m_table->SetText( row, 1, "" );
		m_table->SetText( row, 2, "" );
		m_table->SetText( row, 3, "" );
		m_table->SetText( row, 4, "" );

		QuickStartList.list[row*4][0] = '\0';
		QuickStartList.list[row*4+1][0] = '\0';
		QuickStartList.list[row*4+2][0] = '\0';
		QuickStartList.list[row*4+3][0] = '\0';

		return 1;
	}

	//CPan* tips;
};

int fn_QuickLoadPrograms()
{
	QuickLoadProgramsUI win;
	win.Show();
	win.Hide();

	return 1;
}


int fn_LoadProgramFromQuickList( unsigned int row )
{
	if( row >= QUICKSTARTLIST_NUM )
	{
		return 0;
	}

	if( QuickStartList.list[4*row][0] == 0 || QuickStartList.list[4*row+1][0] == 0 ||
		QuickStartList.list[4*row+2][0] == 0 || QuickStartList.list[4*row+3][0] == 0 )
	{
		return 0;
	}

	if( strncmp( QHeader.Prg_Default, QuickStartList.list[4*row+1], 8 ) )
	{
		if( ComponentList != NULL )
		{
			delete [] ComponentList;
			ComponentList = NULL;
		}
		NComponent = 0;
		PlacedNComp = 0;

		ri_reset();
	}

	snprintf( QHeader.Cli_Default, sizeof(QHeader.Cli_Default), "%s", QuickStartList.list[4*row] );
	snprintf( QHeader.Prg_Default, sizeof(QHeader.Prg_Default), "%s", QuickStartList.list[4*row+1] );
	snprintf( QHeader.Conf_Default, sizeof(QHeader.Conf_Default), "%s", QuickStartList.list[4*row+2] );
	snprintf( QHeader.Lib_Default, sizeof(QHeader.Lib_Default), "%s", QuickStartList.list[4*row+3] );

	Mod_Cfg(QHeader);
	return 1;
}
