//---------------------------------------------------------------------------
//
// Name:        q_packages.cpp
// Author:      Gabriel Ferri
// Created:     27/02/2012
// Description: Quadra packages manager
//
//---------------------------------------------------------------------------
#include "q_packages.h"

#include <math.h>
#include <boost/algorithm/string.hpp>
#include "c_win_imgpar_pack.h"
#include "c_pan.h"
#include "c_inputbox.h"
#include "msglist.h"
#include "q_oper.h"
#include "q_dosat.h"
#include "q_ugeobj.h"
#include "filemanager.h"
#include "fileutils.h"
#include "q_cost.h"
#include "q_tabe.h"
#include "q_prog.h"
#include "q_help.h"
#include "strutils.h"
#include "gui_functions.h"
#include "gui_defs.h"
#include "gui_desktop.h"
#include "q_grcol.h"
#include "keyutils.h"

#include <mss.h>


extern void PackVisFile_Remove( int pack_code, char* libname );


// velocita' e acc.
#define PKG_LEDTH_ZVSTARTDEF    10    //velocita start-stop asse z
#define PKG_LEDTH_ZVELDEF       80    //velocita asse z
#define PKG_LEDTH_ZACCDEF       1700  //accelerazione asse z
#define PACKOFF_MIN             -5.f
#define PACKOFF_MAX             5.f
#define PACKOFF_TMIN            -36
#define PACKOFF_TMAX            36



SPackageData currentLibPackages[MAXPACK];
SPackageOffsetData currentLibOffsetPackages[MAXPACK];

extern struct CfgHeader QHeader;
extern UgelliClass* Ugelli;
extern DosatClass* Dosatore;
extern GUI_DeskTop* guiDeskTop;


int fn_PackageAdvancedParams( CWindow* parent, int curRecord );
int fn_PackageCorrections( CWindow* parent, int curRecord );




	//---------------//
	// Gestione file //
	//---------------//

//---------------------------------------------------------------------------------
// Versioni:
//   0 - versione di partenza
//---------------------------------------------------------------------------------
#define PACKAGESLIB_VERSION       0
#define PACKAGESOFFLIB_VERSION    0


void PackagesLib_Default( SPackageData* data )
{
	data->code = 0;

	data->name[0] = '\0';
	data->notes[0] = '\0';

	data->x = 0;
	data->y = 0;
	data->z = 0;

	data->speedXY = 2;
	data->speedRot = 2;
	data->speedPick = 2;
	data->speedPlace = 2;

	strncpy( data->tools, "A", 3 );
	data->centeringMode = CenteringMode::SNIPER;
	data->orientation = 0;

	// sniper
	data->snpX = 0;
	data->snpY = 0;
	data->snpZOffset = 0;
	data->snpTolerance = 0;
	data->snpMode = 1;

	// external camera
	data->extAngle = 0;

	// advanced params
	data->centeringPID = 0;
	data->placementPID = 0;
	data->checkPick = 'X';
	data->checkVacuumThr = 1;
	data->steadyCentering = 0;
	data->placementMode = 'N';
	data->check_post_place = 0;

	// ledth placement
	data->ledth_insidedim = 0.f;
	data->ledth_securitygap = 0.f;
	data->ledth_interf = 1.f;
	data->ledth_place_vmin = PKG_LEDTH_ZVSTARTDEF;
	data->ledth_place_vel = PKG_LEDTH_ZVELDEF;
	data->ledth_place_acc = PKG_LEDTH_ZACCDEF;
	data->ledth_rising_vmin = PKG_LEDTH_ZVSTARTDEF;
	data->ledth_rising_vel = PKG_LEDTH_ZVELDEF;
	data->ledth_rising_acc = PKG_LEDTH_ZACCDEF;

	//
	memset( data->spare, 0, sizeof(data->spare) );
}

bool PackagesLib_Write( const std::string& filename, SPackageData* data )
{
	CFileManager fm( filename );
	if( fm.open( PACKAGESLIB_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( PACKAGESLIB_VERSION ) )
			return false;
	}

	for( int i = 0; i < MAXPACK; i++ )
	{
		if( !fm.writeRec( data, sizeof(SPackageData), i ) )
			return false;

		data++;
	}
	return true;
}

bool PackagesLib_Read( const std::string& filename, SPackageData* data )
{
	CFileManager fm( filename );
	int retVal = fm.open( PACKAGESLIB_VERSION, false );

	if( retVal == -4 )
	{
		// update
	}
	else if( retVal != 1 )
	{
		return false;
	}

	bool rewrite = false;
	SPackageData* startData = data;

	for( int i = 0; i < MAXPACK; i++ )
	{
		if( !fm.readRec( data, sizeof(SPackageData), i ) )
			return false;

		if( data->code != i+1 )
		{
			data->code = i+1;
			rewrite = true;
		}

		data++;
	}

	fm.close();

	if( rewrite )
	{
		printf( "lib: %s corrupted. RE-WRITE.\n", filename.c_str() );
		PackagesLib_Write( filename, startData );
	}

	return true;
}


void PackagesOffsetLib_Default( SPackageOffsetData* data )
{
	data->angle = 0.f;

	for( int j = 0; j < 4; j++ )
	{
		data->offX[j] = 0.f;
		data->offY[j] = 0.f;
		data->offRot[j] = 0.f;
	}
}

bool PackagesOffsetLib_Write( const std::string& filename, SPackageOffsetData* data )
{
	CFileManager fm( filename );
	if( fm.open( PACKAGESOFFLIB_VERSION, false ) != 1 )
	{
		// create new file
		if( !fm.create( PACKAGESOFFLIB_VERSION ) )
			return false;
	}

	for( int i = 0; i < MAXPACK; i++ )
	{
		if( !fm.writeRec( data, sizeof(SPackageOffsetData), i ) )
			return false;

		data++;
	}
	return true;
}

bool PackagesOffsetLib_Read( const std::string& filename, SPackageOffsetData* data )
{
	CFileManager fm( filename );
	int retVal = fm.open( PACKAGESOFFLIB_VERSION, false );

	if( retVal == -4 )
	{
		// update
	}
	else if( retVal != 1 )
	{
		return false;
	}

	for( int i = 0; i < MAXPACK; i++ )
	{
		if( !fm.readRec( data, sizeof(SPackageOffsetData), i ) )
			return false;

		data++;
	}
	return true;
}



	//----------------------------//
	// Interfaccia: Gestione file //
	//----------------------------//

//---------------------------------------------------------------------------
// Carica libreria package
//---------------------------------------------------------------------------
bool PackagesLib_Load( const std::string& libname )
{
	if( libname.empty() )
		return false;

	char filename[MAXNPATH];

	// open packages lib
	snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, libname.c_str(), PACKAGESLIB_EXT );
	if( !PackagesLib_Read( filename, currentLibPackages ) )
	{
		char buf[MAXNPATH];
		snprintf( buf, MAXNPATH, MsgGetString(Msg_00934), libname.c_str() );
		W_Mess( buf );
		return false;
	}

	// open/create packages offset
	snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, libname.c_str(), PACKAGESOFFLIB_EXT );
	if( !PackagesOffsetLib_Read( filename, currentLibOffsetPackages ) )
	{
		char buf[MAXNPATH];
		snprintf( buf, MAXNPATH, MsgGetString(Msg_00934), filename );
		W_Mess( buf );
		return false;
	}

	// check/create "vispack" directory
	if( !CheckDirectory( VISPACKDIR ) )
	{
		if( mkdir( VISPACKDIR, DIR_CREATION_FLAG ) )
		{
			return false;
		}
	}

	// check/create "vispack/libname" directory
	char buf[MAXNPATH];
	snprintf( buf, MAXNPATH, "%s/%s", VISPACKDIR, libname.c_str() );
	if( !CheckDirectory(buf) )
	{
		if( mkdir( buf, DIR_CREATION_FLAG ) )
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------
// Salva libreria package
//---------------------------------------------------------------------------
bool PackagesLib_Save( const std::string& libname )
{
	if( libname.empty() )
		return false;

	char filename[MAXNPATH];
	bool retVal = true;

	snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, libname.c_str(), PACKAGESLIB_EXT );
	if( !PackagesLib_Write( filename, currentLibPackages ) )
	{
		retVal = false;
	}
	snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, libname.c_str(), PACKAGESOFFLIB_EXT );
	if( PackagesOffsetLib_Write( filename, currentLibOffsetPackages ) )
	{
		retVal = false;
	}
	return retVal;
}

//---------------------------------------------------------------------------
// Crea nuova libreria package
//---------------------------------------------------------------------------
bool PackagesLib_Create( const std::string& libname )
{
	if( libname.empty() )
		return false;

	char filename[MAXNPATH];

	SPackageData libPackages[MAXPACK];

	for( int i = 0; i < MAXPACK; i++ )
	{
		PackagesLib_Default( &libPackages[i] );
		libPackages[i].code = i+1;
	}

	SPackageOffsetData libOffsetPackages[MAXPACK];
	for( int i = 0; i < MAXPACK; i++ )
	{
		PackagesOffsetLib_Default( &libOffsetPackages[i] );
	}

	bool retVal = true;

	snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, libname.c_str(), PACKAGESLIB_EXT );
	if( !PackagesLib_Write( filename, libPackages ) )
	{
		retVal = false;
	}
	snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, libname.c_str(), PACKAGESOFFLIB_EXT );
	if( PackagesOffsetLib_Write( filename, libOffsetPackages ) )
	{
		retVal = false;
	}
	return retVal;
}




#ifdef __SNIPER
//---------------------------------------------------------------------------
// Controlla se dimensioni package sono valide per centraggio sniper
//---------------------------------------------------------------------------
int CheckSniperCenteringDimensions( SPackageData* pack, int nozzle )
{
	if( nozzle != 1 && nozzle != 2 )
	{
		return PACK_ERROR;
	}

	float size = MAX( pack->snpX, pack->snpY );
	float limit = getSniperType2ComponentLimit( nozzle, CCenteringReservedParameters::inst().getData() );

	if( size > limit )
	{
		return PACK_TOOBIG;
	}

	return PACK_DIMOK;
}

//---------------------------------------------------------------------------
// Controlla se altezza package e' valida per prelievo con punta
//---------------------------------------------------------------------------
int CheckSniperCenteringHeight( SPackageData* pack, char tool, int nozzle )
{
	CfgUgelli udat;
	Ugelli->ReadRec( udat, tool-'A' );

	float pkgzpos = -udat.Z_offset[nozzle-1] - pack->z/2 - pack->snpZOffset;
	if( pkgzpos < QHeader.Min_NozHeight[nozzle-1] )
	{
		return PACK_TOOHIGH;
	}

	return PACK_DIMOK;
}
#endif


//---------------------------------------------------------------------------
// Controlla se presente almeno un ugello valido per la punta passata
// Ritorna: 1 se presente un ugello valido
//          0 altrimenti
//---------------------------------------------------------------------------
int CheckToolOnNozzle( int nozzle, char* toolsList )
{
	if( nozzle != 1 && nozzle != 2 )
	{
		return 0;
	}

	struct CfgUgelli tool;

	int value = (nozzle == 1) ? UG_P2 : UG_P1;

	int num = strlen(toolsList);
	for( int i = 0; i < num; i++ )
	{
		Ugelli->ReadRec( tool, toolsList[i]-'A' );

		if( tool.NozzleAllowed != value && tool.NozzleAllowed != 0 )
			return 1;
	}

	return 0;
}

//---------------------------------------------------------------------------
// Controlla se package puo' essere prelevato da punta
//---------------------------------------------------------------------------
int CheckPackageNozzle( SPackageData* pack, int nozzle )
{
	if( nozzle != 1 && nozzle != 2 )
	{
		return PACK_ERROR;
	}

	if( !CheckToolOnNozzle( nozzle, pack->tools ) )
	{
		return PACK_NOTOOLS;
	}

	#ifdef __SNIPER
	if( pack->centeringMode == CenteringMode::SNIPER )
	{
		int retval = CheckSniperCenteringDimensions( pack, nozzle );
		if( retval != PACK_DIMOK )
		{
			return retval;
		}

		return CheckSniperCenteringHeight( pack, pack->tools[0], nozzle );
	}
	#endif

	return PACK_DIMOK;
}


//---------------------------------------------------------------------------------------
// Controlla presenza (e congruenza) files package per telecamera esterna
// Ritorna: 1 se ok
//          0 errore generale (o immagini non apprese)
//         -1 manca punta 1
//         -2 manca punta 2
//---------------------------------------------------------------------------------------
int CheckPackageVisionData( SPackageData& pack, char* libname, bool showError )
{
	char path[13][MAXNPATH];

	//dati package
	PackVisData_GetFilename( path[0], pack.code, libname );

	// NOZZLE 1
	//immagine originale
	SetImageName( path[1], PACKAGEVISION_LEFT_1, IMAGE, pack.code, libname );
	SetImageName( path[2], PACKAGEVISION_RIGHT_1, IMAGE, pack.code, libname );

	//dati immagine
	SetImageName( path[3], PACKAGEVISION_LEFT_1, DATA, pack.code, libname );
	SetImageName( path[4], PACKAGEVISION_RIGHT_1, DATA, pack.code, libname );

	//immagine elaborata
	SetImageName( path[5], PACKAGEVISION_LEFT_1, ELAB, pack.code, libname );
	SetImageName( path[6], PACKAGEVISION_RIGHT_1, ELAB, pack.code, libname );

	// NOZZLE 2
	//immagine originale
	SetImageName( path[7], PACKAGEVISION_LEFT_2, IMAGE, pack.code, libname );
	SetImageName( path[8], PACKAGEVISION_RIGHT_2, IMAGE, pack.code, libname );

	//dati immagine
	SetImageName( path[9], PACKAGEVISION_LEFT_2, DATA, pack.code, libname );
	SetImageName( path[10], PACKAGEVISION_RIGHT_2, DATA, pack.code, libname );

	//immagine elaborata
	SetImageName( path[11], PACKAGEVISION_LEFT_2, ELAB, pack.code, libname );
	SetImageName( path[12], PACKAGEVISION_RIGHT_2, ELAB, pack.code, libname );

	int err = 0;
	int errGen = 0;
	int errN1 = 0;
	int errN2 = 0;
	for( int i = 0; i < 13; i++ )
	{
		if( access(path[i],F_OK) )
		{
			if( i == 0 )
			{
				errGen++;
			}
			else if( i < 7 )
			{
				errN1++;
			}
			else
			{
				errN2++;
			}
			err++;
		}
	}

	if( !err )
	{
		//trovati tutti i file

		//controlla congruenza dei dati
		struct PackVisData pvdat;

		//leggi dati package per il sistema di visione
		PackVisData_Open( path[0], pack.name );
		PackVisData_Read( pvdat );
		PackVisData_Close();

		DelSpcR( pvdat.name );

		if( strcasecmpQ( pack.name, pvdat.name ) )
		{
			//dati non congruenti: errore
			if( showError )
			{
				char buf[160];
				sprintf( buf, MsgGetString(Msg_01578), pack.name );
				bipbip();
				W_Mess(buf);
			}

			return 0;
		}

		// nessun errore
		return 1;
	}

	// se nessun file trovato...
	if( errGen || ( errN1 && errN2 ) )
	{
		if( showError )
		{
			bipbip();
			W_Mess( MsgGetString(Msg_01577) );
		}
		return 0;
	}

	if( errN1 )
	{
		return -1;
	}

	if( errN2 )
	{
		return -2;
	}

	W_Mess( "Unknown error !" );
	return 0;
}


//---------------------------------------------------------------------------------------
// Ritorna la correzione offset del package selezionato
//---------------------------------------------------------------------------------------
void GetPackageOffsetCorrection( float angle, int packIndex, float& x, float& y, int& theta )
{
	SPackageOffsetData* poffset = &currentLibOffsetPackages[packIndex];

	while( angle >= 360 )
		angle -= 360;
	while( angle < 0 )
		angle += 360;

	if( angle >= 45 && angle < 135 )
	{
		x = poffset->offX[1];
		y = poffset->offY[1];
		theta = poffset->offRot[1];
	}
	else if( angle >= 135 && angle < 225 )
	{
		x = poffset->offX[2];
		y = poffset->offY[2];
		theta = poffset->offRot[2];
	}
	else if( angle >= 225 && angle < 315 )
	{
		x = poffset->offX[3];
		y = poffset->offY[3];
		theta = poffset->offRot[3];
	}
	else
	{
		x = poffset->offX[0];
		y = poffset->offY[0];
		theta = poffset->offRot[0];
	}
}


//---------------------------------------------------------------------------
// finestra: Package parameters
//---------------------------------------------------------------------------
extern void Pack_DimApp( int nozzle, int packIndex );
extern int Pack_ZOffManApp( int packIndex );
extern int Pack_AngLaserApp( int packIndex );
extern void G_PackImageApprend( int nozzle, int packIndex );
extern int TPack_Colla( CWindow* parent, int nozzle, int index );


PackageParamsUI::PackageParamsUI( CWindow* parent, int curRecord ) : CWindowParams( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X );
	SetClientAreaPos( 0, 5 );
	SetClientAreaSize( 73, 19 );
	SetTitle( MsgGetString(Msg_00921) );

	index = curRecord;

	SM_DimTeach = new GUI_SubMenu();
	SM_DimTeach->Add( MsgGetString(Msg_00042), K_F1, 0, NULL, boost::bind( &PackageParamsUI::onDimensionTeach1, this ) ); // punta 1
	SM_DimTeach->Add( MsgGetString(Msg_00043), K_F2, 0, NULL, boost::bind( &PackageParamsUI::onDimensionTeach2, this ) ); // punta 2

	SM_VisionTeach = new GUI_SubMenu();
	SM_VisionTeach->Add( MsgGetString(Msg_00042), K_F1, 0, NULL, boost::bind( &PackageParamsUI::onImageTeach1, this ) ); // punta 1
	SM_VisionTeach->Add( MsgGetString(Msg_00043), K_F2, 0, NULL, boost::bind( &PackageParamsUI::onImageTeach2, this ) ); // punta 2

	SM_VisionParams = new GUI_SubMenu();
	SM_VisionParams->Add( MsgGetString(Msg_01574), K_F6, 0, SM_VisionTeach, NULL );   // apprendimento immagini
	SM_VisionParams->Add( MsgGetString(Msg_00756), K_F7, 0, NULL, boost::bind( &PackageParamsUI::onImageParams, this ) ); // parametri visione

	#ifdef __DISP2
	SM_DispParams = new GUI_SubMenu();
	SM_DispParams->Add( MsgGetString(Msg_05113), K_F1, 0, NULL, boost::bind( &PackageParamsUI::onDispensingData1, this ) ); // dispenser 1
	SM_DispParams->Add( MsgGetString(Msg_05114), K_F2, 0, NULL, boost::bind( &PackageParamsUI::onDispensingData2, this ) ); // dispenser 2
	#endif
}

PackageParamsUI::~PackageParamsUI()
{
	delete SM_DimTeach;
	delete SM_VisionTeach;
	delete SM_VisionParams;
	#ifdef __DISP2
	delete SM_DispParams;
	#endif
}

void PackageParamsUI::onInit()
{
	// create combos
	m_combos[NAME]        = new C_Combo( 11,  1, MsgGetString(Msg_00890), 21, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
	m_combos[NOTES]       = new C_Combo( 12,  2, MsgGetString(Msg_06025), 23, CELL_TYPE_TEXT );

	m_combos[DIM_X]       = new C_Combo( 12,  5, "X :", 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
	m_combos[DIM_Y]       = new C_Combo( 12,  6, "Y :", 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
	m_combos[DIM_Z]       = new C_Combo( 12,  7, "Z :", 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );

	m_combos[SPEED_XY]    = new C_Combo( 11, 10, "XY :", 8, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
	m_combos[SPEED_ROT]   = new C_Combo(  4, 11, MsgGetString(Msg_00083), 8, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
	m_combos[SPEED_PICK]  = new C_Combo(  4, 12, MsgGetString(Msg_00084), 8, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
	m_combos[SPEED_PLACE] = new C_Combo(  4, 13, MsgGetString(Msg_00085), 8, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );

	m_combos[TOOLS]       = new C_Combo( 40,  5, MsgGetString(Msg_00918), 4, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE );
	m_combos[CENTERING]   = new C_Combo( 32,  6, MsgGetString(Msg_00985), 12, CELL_TYPE_TEXT, CELL_STYLE_CENTERED );
	m_combos[ORIENTATION] = new C_Combo( 32,  7, MsgGetString(Msg_00014), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

	m_combos[SNIPER_X]    = new C_Combo( 48, 10, "X :", 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
	m_combos[SNIPER_Y]    = new C_Combo( 48, 11, "Y :", 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
	m_combos[SNIPER_Z]    = new C_Combo( 39, 12, MsgGetString(Msg_00904), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	m_combos[SNIPER_TOL]  = new C_Combo( 39, 13, MsgGetString(Msg_00913), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
	m_combos[SNIPER_MODE] = new C_Combo( 39, 14, MsgGetString(Msg_00919), 3, CELL_TYPE_UINT );

	m_combos[EXT_ANGLE]   = new C_Combo( 39, 17, MsgGetString(Msg_00008), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );

	// set params
	m_combos[SPEED_XY]->SetLegalStrings( 3, (char**)Speeds_StrVect );
	m_combos[SPEED_ROT]->SetLegalStrings( 3, (char**)Speeds_StrVect );
	m_combos[SPEED_PICK]->SetLegalStrings( 3, (char**)Speeds_StrVect );
	m_combos[SPEED_PLACE]->SetLegalStrings( 3, (char**)Speeds_StrVect );
	m_combos[TOOLS]->SetLegalChars( CHARSET_TOOL );
	#ifdef __SNIPER
	m_combos[CENTERING]->SetLegalStrings( 3, (char**)Centering_StrVect );
	#endif
	m_combos[ORIENTATION]->SetVMinMax( 0.f, 360.f );
	m_combos[SNIPER_X]->SetVMinMax( 0.f, 999.f );
	m_combos[SNIPER_Y]->SetVMinMax( 0.f, 999.f );
	m_combos[SNIPER_Z]->SetVMinMax( -50.f, 50.f );
	m_combos[SNIPER_TOL]->SetVMinMax( 0.f, 10.f );
	m_combos[SNIPER_MODE]->SetVMinMax( 1, 20 );
	m_combos[EXT_ANGLE]->SetVMinMax( 0.f, 360.f );

	m_combos[SNIPER_X]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[SNIPER_Y]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[SNIPER_Z]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[SNIPER_TOL]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[SNIPER_MODE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[EXT_ANGLE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );

	// add to combo list
	m_comboList->Add( m_combos[NAME]       , 0, 0 );
	m_comboList->Add( m_combos[NOTES]      , 1, 0 );
	m_comboList->Add( m_combos[DIM_X]      , 2, 0 );
	m_comboList->Add( m_combos[DIM_Y]      , 3, 0 );
	m_comboList->Add( m_combos[DIM_Z]      , 4, 0 );
	m_comboList->Add( m_combos[SPEED_XY]   , 5, 0 );
	m_comboList->Add( m_combos[SPEED_ROT]  , 6, 0 );
	m_comboList->Add( m_combos[SPEED_PICK] , 7, 0 );
	m_comboList->Add( m_combos[SPEED_PLACE], 8, 0 );
	m_comboList->Add( m_combos[TOOLS]      , 2, 1 );
	m_comboList->Add( m_combos[CENTERING]  , 3, 1 );
	m_comboList->Add( m_combos[ORIENTATION], 4, 1 );
	m_comboList->Add( m_combos[SNIPER_X]   , 5, 1 );
	m_comboList->Add( m_combos[SNIPER_Y]   , 6, 1 );
	m_comboList->Add( m_combos[SNIPER_Z]   , 7, 1 );
	m_comboList->Add( m_combos[SNIPER_TOL] , 8, 1 );
	m_comboList->Add( m_combos[SNIPER_MODE], 9, 1 );
	m_comboList->Add( m_combos[EXT_ANGLE]  ,10, 1 );
}

void PackageParamsUI::onShow()
{
	DrawGroup( RectI(  2,  4, 29, 5 ), MsgGetString(Msg_00129) );
	DrawGroup( RectI(  2,  9, 29, 6 ), MsgGetString(Msg_00130) );
	DrawGroup( RectI( 33,  9, 38, 7 ), MsgGetString(Msg_00131) );
	DrawGroup( RectI( 33, 16, 38, 3 ), MsgGetString(Msg_00132) );
}

void PackageParamsUI::onRefresh()
{
	m_combos[NAME]->SetTxt( currentLibPackages[index].name );
	m_combos[NOTES]->SetTxt( currentLibPackages[index].notes );

	m_combos[DIM_X]->SetTxt( currentLibPackages[index].x );
	m_combos[DIM_Y]->SetTxt( currentLibPackages[index].y );
	m_combos[DIM_Z]->SetTxt( currentLibPackages[index].z );

	m_combos[SPEED_XY]->SetStrings_Pos( currentLibPackages[index].speedXY );
	m_combos[SPEED_ROT]->SetStrings_Pos( currentLibPackages[index].speedRot );
	m_combos[SPEED_PICK]->SetStrings_Pos( currentLibPackages[index].speedPick );
	m_combos[SPEED_PLACE]->SetStrings_Pos( currentLibPackages[index].speedPlace );

	m_combos[TOOLS]->SetTxt( currentLibPackages[index].tools );
	m_combos[CENTERING]->SetStrings_Pos( currentLibPackages[index].centeringMode );
	m_combos[ORIENTATION]->SetTxt( currentLibPackages[index].orientation );

	if( currentLibPackages[index].centeringMode == CenteringMode::SNIPER )
	{
		m_combos[SNIPER_X]->SetStyle( m_combos[SNIPER_X]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[SNIPER_Y]->SetStyle( m_combos[SNIPER_Y]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[SNIPER_Z]->SetStyle( m_combos[SNIPER_Z]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[SNIPER_TOL]->SetStyle( m_combos[SNIPER_TOL]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[SNIPER_MODE]->SetStyle( m_combos[SNIPER_MODE]->GetStyle() & ~CELL_STYLE_READONLY );

		m_combos[SNIPER_X]->SetTxt( currentLibPackages[index].snpX );
		m_combos[SNIPER_Y]->SetTxt( currentLibPackages[index].snpY );
		m_combos[SNIPER_Z]->SetTxt( currentLibPackages[index].snpZOffset );
		m_combos[SNIPER_TOL]->SetTxt( currentLibPackages[index].snpTolerance );
		m_combos[SNIPER_MODE]->SetTxt( currentLibPackages[index].snpMode );
	}
	else
	{
		m_combos[SNIPER_X]->SetStyle( m_combos[SNIPER_X]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[SNIPER_Y]->SetStyle( m_combos[SNIPER_Y]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[SNIPER_Z]->SetStyle( m_combos[SNIPER_Z]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[SNIPER_TOL]->SetStyle( m_combos[SNIPER_TOL]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[SNIPER_MODE]->SetStyle( m_combos[SNIPER_MODE]->GetStyle() | CELL_STYLE_READONLY );

		m_combos[SNIPER_X]->SetTxt( "- " );
		m_combos[SNIPER_Y]->SetTxt( "- " );
		m_combos[SNIPER_Z]->SetTxt( "- " );
		m_combos[SNIPER_TOL]->SetTxt( "- " );
		m_combos[SNIPER_MODE]->SetTxt( "- " );
	}

	if( currentLibPackages[index].centeringMode == CenteringMode::EXTCAM )
	{
		m_combos[EXT_ANGLE]->SetStyle( m_combos[EXT_ANGLE]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[EXT_ANGLE]->SetTxt( currentLibPackages[index].extAngle );
	}
	else
	{
		m_combos[EXT_ANGLE]->SetStyle( m_combos[EXT_ANGLE]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[EXT_ANGLE]->SetTxt( "- " );
	}
}

void PackageParamsUI::onEdit()
{
	snprintf( currentLibPackages[index].notes, 24, "%s", m_combos[NOTES]->GetTxt() );

	currentLibPackages[index].x = m_combos[DIM_X]->GetFloat();
	currentLibPackages[index].y = m_combos[DIM_Y]->GetFloat();
	currentLibPackages[index].z = m_combos[DIM_Z]->GetFloat();

	currentLibPackages[index].speedXY = m_combos[SPEED_XY]->GetStrings_Pos();
	currentLibPackages[index].speedRot = m_combos[SPEED_ROT]->GetStrings_Pos();
	currentLibPackages[index].speedPick = m_combos[SPEED_PICK]->GetStrings_Pos();
	currentLibPackages[index].speedPlace = m_combos[SPEED_PLACE]->GetStrings_Pos();

	snprintf( currentLibPackages[index].tools, 8, "%s", m_combos[TOOLS]->GetTxt() );

	int old_centeringMode = currentLibPackages[index].centeringMode;
	currentLibPackages[index].centeringMode = m_combos[CENTERING]->GetStrings_Pos();

	currentLibPackages[index].orientation = m_combos[ORIENTATION]->GetFloat();

	if( old_centeringMode == 0 )
	{
		currentLibPackages[index].snpX = m_combos[SNIPER_X]->GetFloat();
		currentLibPackages[index].snpY = m_combos[SNIPER_Y]->GetFloat();
		currentLibPackages[index].snpZOffset = m_combos[SNIPER_Z]->GetFloat();
		currentLibPackages[index].snpTolerance = m_combos[SNIPER_TOL]->GetFloat();
		currentLibPackages[index].snpMode = m_combos[SNIPER_MODE]->GetInt();
	}
	else if( old_centeringMode == 1 )
	{
		currentLibPackages[index].extAngle = m_combos[EXT_ANGLE]->GetFloat();
	}

	adjustPackageData();
}

void PackageParamsUI::onShowMenu()
{
	int status = (currentLibPackages[index].centeringMode == CenteringMode::SNIPER) ? 0 : SM_GRAYED;
	m_menu->Add( MsgGetString(Msg_00979), K_F3, status, NULL, boost::bind( &PackageParamsUI::onPackageZOffset, this ) );
	m_menu->Add( MsgGetString(Msg_00210), K_F4, status, SM_DimTeach, NULL );
	m_menu->Add( MsgGetString(Msg_01494), K_F5, status, NULL, boost::bind( &PackageParamsUI::onPackageAngleTeach, this ) );
	status = (currentLibPackages[index].centeringMode == CenteringMode::EXTCAM) ? 0 : SM_GRAYED;
	m_menu->Add( MsgGetString(Msg_00756), K_F6, status, SM_VisionParams, NULL );
	m_menu->Add( MsgGetString(Msg_00055), K_F7, 0, NULL, boost::bind( &PackageParamsUI::onAdvancedParameters, this ) );
	m_menu->Add( MsgGetString(Msg_01965), K_F8, 0, NULL, boost::bind( &PackageParamsUI::onPackageOffset, this ) );

	#ifndef __DISP2
	m_menu->Add( MsgGetString(Msg_00126), K_F9, !QParam.Dispenser, NULL, boost::bind( &PackageParamsUI::onDispensingData, this ) );
	#else
	m_menu->Add( MsgGetString(Msg_00126), K_F9, !QParam.Dispenser, SM_DispParams, NULL );
	#endif
}

bool PackageParamsUI::onKeyPress( int key )
{
	switch( key )
	{
		case K_F3:
			if( currentLibPackages[index].centeringMode == CenteringMode::SNIPER )
			{
				onPackageZOffset();
			}
			return true;

		case K_F4:
			if( currentLibPackages[index].centeringMode == CenteringMode::SNIPER )
			{
				SM_DimTeach->Show();
			}
			return true;

		case K_F5:
			if( currentLibPackages[index].centeringMode == CenteringMode::SNIPER )
			{
				onPackageAngleTeach();
			}
			return true;

		case K_F6:
			if( currentLibPackages[index].centeringMode == CenteringMode::EXTCAM )
			{
				SM_VisionParams->Show();
			}
			return true;

		case K_F7:
			onAdvancedParameters();
			return true;

		case K_F8:
			onPackageOffset();
			return true;

		case K_F9:
			if( QParam.Dispenser )
			{
				#ifndef __DISP2
				onDispensingData();
				#else
				SM_DispParams->Show();
				#endif
			}
			break;

		case K_ESC:
			if( !checkPackageData() )
			{
				// controllo fallito: rimani nella finestra
				return true;
			}
			break;

		default:
			break;
	}

	return false;
}

void PackageParamsUI::onClose()
{
	PackagesLib_Save( QHeader.Lib_Default );
}

int PackageParamsUI::onPackageZOffset()
{
	return Pack_ZOffManApp( index );
}

int PackageParamsUI::onPackageAngleTeach()
{
	return Pack_AngLaserApp( index );
}

int PackageParamsUI::onAdvancedParameters()
{
	return fn_PackageAdvancedParams( this, index );
}

int PackageParamsUI::onImageParams()
{
	if( CheckPackageVisionData( currentLibPackages[index], QHeader.Lib_Default, true ) == 0 )
		return 0;

	return ShowPackImgParams( this, currentLibPackages[index].code, QHeader.Lib_Default );
}

int PackageParamsUI::onDimensionTeach1()
{
	Pack_DimApp( 1, index );
	return 1;
}

int PackageParamsUI::onDimensionTeach2()
{
	Pack_DimApp( 2, index );
	return 1;
}

int PackageParamsUI::onImageTeach1()
{
	G_PackImageApprend( 1, index );
	return 1;
}

int PackageParamsUI::onImageTeach2()
{
	G_PackImageApprend( 2, index );
	return 1;
}

int PackageParamsUI::onPackageOffset()
{
	return fn_PackageCorrections( this, index );
}

#ifndef __DISP2
int PackageParamsUI::onDispensingData()
{
	return TPack_Colla( this, 1, index );
}
#else
int PackageParamsUI::onDispensingData1()
{
	return TPack_Colla( this, 1, index );
}

int PackageParamsUI::onDispensingData2()
{
	return TPack_Colla( this, 2, index );
}
#endif

bool PackageParamsUI::adjustPackageData()
{
	int val = ftoi( currentLibPackages[index].orientation / 90.f );
	val = MID( 0, val, 3 );
	currentLibPackages[index].orientation = 90.f * val;

	val = ftoi( currentLibPackages[index].extAngle / 90.f );
	val = MID( 0, val, 3 );
	currentLibPackages[index].extAngle = 90.f * val;
	return true;
}

bool PackageParamsUI::checkPackageData()
{
	std::string checkMsg;

	// check X/Y dimensions (X < Y)
	if( currentLibPackages[index].x > currentLibPackages[index].y )
	{
		checkMsg.append( MsgGetString(Msg_01146) );
		checkMsg.append( "\n" );
	}

	// check tools withdrawable
	if( CheckToolOnNozzle( 1, currentLibPackages[index].tools ) == 0 &&
		CheckToolOnNozzle( 2, currentLibPackages[index].tools ) == 0 )
	{
		checkMsg.append( MsgGetString(Msg_00115) );
		checkMsg.append( "\n" );
	}

	if( currentLibPackages[index].centeringMode == CenteringMode::SNIPER )
	{
		// check dimensions
		if( currentLibPackages[index].snpX > currentLibPackages[index].snpY )
		{
			checkMsg.append( MsgGetString(Msg_01147) );
			checkMsg.append( "\n" );
		}

		if( currentLibPackages[index].snpX == 0.f || currentLibPackages[index].snpY == 0.f )
		{
			checkMsg.append( MsgGetString(Msg_01149) );
			checkMsg.append( "\n" );
		}
		else if( CheckSniperCenteringDimensions( &currentLibPackages[index], 1 ) != PACK_DIMOK &&
				  CheckSniperCenteringDimensions( &currentLibPackages[index], 2 ) != PACK_DIMOK )
		{
			checkMsg.append( MsgGetString(Msg_01152) );
			checkMsg.append( "\n" );
		}
		// check Z offset
		if( fabs(currentLibPackages[index].snpZOffset) >= currentLibPackages[index].z/2.f )
		{
			checkMsg.append( MsgGetString(Msg_00221) );
			checkMsg.append( "\n" );
		}
		// check tolerance
		if( currentLibPackages[index].snpTolerance == 0.f )
		{
			checkMsg.append( MsgGetString(Msg_01150) );
			checkMsg.append( "\n" );
		}
	}

	if( checkMsg.size() )
	{
		checkMsg.append( " \n" );
		checkMsg.append( MsgGetString(Msg_00100) );

		Deselect();
		int ret = W_Deci( 0, checkMsg.c_str() );
		Select();
		if( ret == 0 )
		{
			return false;
		}
	}

	return true;
}

int fn_PackageParams( CWindow* parent, int curRecord )
{
	PackageParamsUI win( parent, curRecord );
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Package advanced parameters
//---------------------------------------------------------------------------
PackageAdvancedParamsUI::PackageAdvancedParamsUI( CWindow* parent, int curRecord ) : CWindowParams( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X );
	SetClientAreaPos( 0, 6 );
	SetClientAreaSize( 70, 20 );
	SetTitle( MsgGetString(Msg_00055) );

	index = curRecord;
}

void PackageAdvancedParamsUI::onInit()
{
	// create combos
	m_combos[CHECK_PICK]    = new C_Combo(  3,  1, MsgGetString(Msg_00954), 1, CELL_TYPE_TEXT , CELL_STYLE_UPPERCASE );
	m_combos[CHECK_VTHR]    = new C_Combo(  3,  2, MsgGetString(Msg_01030), 1, CELL_TYPE_UINT );
	m_combos[PID_CENT]      = new C_Combo( 45,  1, MsgGetString(Msg_01141), 1, CELL_TYPE_UINT );
	m_combos[PID_PLACE]     = new C_Combo( 45,  2, MsgGetString(Msg_01119), 1, CELL_TYPE_UINT );
	m_combos[STEADY_CENT]   = new C_Combo(  3,  4, MsgGetString(Msg_05046), 4, CELL_TYPE_YN );
	m_combos[PLACE_MODE]    = new C_Combo(  3,  6, MsgGetString(Msg_01118), 1, CELL_TYPE_TEXT, CELL_STYLE_UPPERCASE );

	m_combos[INSIDE_DIM]    = new C_Combo(  3,  9, MsgGetString(Msg_05147), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
	m_combos[SECURITY_GAP]  = new C_Combo(  3, 10, MsgGetString(Msg_05148), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
	m_combos[INTERFERENCE]  = new C_Combo(  3, 11, MsgGetString(Msg_05149), 8, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
	m_combos[VELSTART_DOWN] = new C_Combo(  3, 14, MsgGetString(Msg_00993), 8, CELL_TYPE_UINT );
	m_combos[VELSTART_UP]   = new C_Combo( 45, 14, "", 8, CELL_TYPE_UINT );
	m_combos[VEL_DOWN]      = new C_Combo(  3, 15, MsgGetString(Msg_00241), 8, CELL_TYPE_UINT );
	m_combos[VEL_UP]        = new C_Combo( 45, 15, "", 8, CELL_TYPE_UINT );
	m_combos[ACC_DOWN]      = new C_Combo(  3, 16, MsgGetString(Msg_00242), 8, CELL_TYPE_UINT );
	m_combos[ACC_UP]        = new C_Combo( 45, 16, "", 8, CELL_TYPE_UINT );
	m_combos[CHECK_PLACE]   = new C_Combo(  3, 18, MsgGetString(Msg_05153), 4, CELL_TYPE_YN );

	// set params
	m_combos[CHECK_PICK]->SetLegalChars( "SVBX" );
	m_combos[CHECK_VTHR]->SetVMinMax( 1, 9 );
	m_combos[PID_CENT]->SetVMinMax( 0, 9 );
	m_combos[PID_PLACE]->SetVMinMax( 0, 9 );
	m_combos[PLACE_MODE]->SetLegalChars( "NFL" );
	m_combos[INSIDE_DIM]->SetVMinMax( 0.f, 99.f );
	m_combos[SECURITY_GAP]->SetVMinMax( 0.f, 3.f );
	m_combos[INTERFERENCE]->SetVMinMax( 0.f, 3.f );
	m_combos[VELSTART_DOWN]->SetVMinMax( ZVELMIN, 100 );
	m_combos[VELSTART_UP]->SetVMinMax( ZVELMIN, 100 );
	m_combos[VEL_DOWN]->SetVMinMax( ZVELMIN, QHeader.zMaxSpeed );
	m_combos[VEL_UP]->SetVMinMax( ZVELMIN, QHeader.zMaxSpeed );
	m_combos[ACC_DOWN]->SetVMinMax( ZACCMIN, QHeader.zMaxAcc );
	m_combos[ACC_UP]->SetVMinMax( ZACCMIN, QHeader.zMaxAcc );

	m_combos[CHECK_VTHR]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[INSIDE_DIM]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[SECURITY_GAP]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[INTERFERENCE]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[VELSTART_DOWN]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[VELSTART_UP]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[VEL_DOWN]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[VEL_UP]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[ACC_DOWN]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );
	m_combos[ACC_UP]->SetReadOnlyBgColor( GUI_color( WIN_COL_CLIENTAREA ) );

	// add to combo list
	m_comboList->Add( m_combos[CHECK_PICK]   ,  0, 0 );
	m_comboList->Add( m_combos[CHECK_VTHR]   ,  1, 0 );
	m_comboList->Add( m_combos[PID_CENT]     ,  0, 3 );
	m_comboList->Add( m_combos[PID_PLACE]    ,  1, 3 );
	m_comboList->Add( m_combos[STEADY_CENT]  ,  2, 0 );
	m_comboList->Add( m_combos[PLACE_MODE]   ,  3, 0 );
	m_comboList->Add( m_combos[INSIDE_DIM]   ,  4, 0 );
	m_comboList->Add( m_combos[SECURITY_GAP] ,  5, 0 );
	m_comboList->Add( m_combos[INTERFERENCE] ,  6, 0 );
	m_comboList->Add( m_combos[VELSTART_DOWN],  7, 0 );
	m_comboList->Add( m_combos[VELSTART_UP]  ,  7, 1 );
	m_comboList->Add( m_combos[VEL_DOWN]     ,  8, 0 );
	m_comboList->Add( m_combos[VEL_UP]       ,  8, 1 );
	m_comboList->Add( m_combos[ACC_DOWN]     ,  9, 0 );
	m_comboList->Add( m_combos[ACC_UP]       ,  9, 1 );
	m_comboList->Add( m_combos[CHECK_PLACE]  , 10, 0 );
}

void PackageAdvancedParamsUI::onShow()
{
	DrawSubTitle( 8, MsgGetString(Msg_05146) );

	DrawText( 35, 13, MsgGetString(Msg_05154) );
	DrawText( 46, 13, MsgGetString(Msg_05155) );
}

void PackageAdvancedParamsUI::onRefresh()
{
	m_combos[CHECK_PICK]->SetTxt( currentLibPackages[index].checkPick );

	if( currentLibPackages[index].checkPick == 'V' || currentLibPackages[index].checkPick == 'B' )
	{
		m_combos[CHECK_VTHR]->SetStyle( m_combos[CHECK_VTHR]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[CHECK_VTHR]->SetTxt( currentLibPackages[index].checkVacuumThr );
	}
	else
	{
		m_combos[CHECK_VTHR]->SetStyle( m_combos[CHECK_VTHR]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[CHECK_VTHR]->SetTxt( "-" );
	}

	m_combos[PID_CENT]->SetTxt( currentLibPackages[index].centeringPID );
	m_combos[PID_PLACE]->SetTxt( currentLibPackages[index].placementPID );
	m_combos[STEADY_CENT]->SetTxtYN( currentLibPackages[index].steadyCentering );
	m_combos[PLACE_MODE]->SetTxt( currentLibPackages[index].placementMode );
	m_combos[CHECK_PLACE]->SetTxtYN( currentLibPackages[index].check_post_place );

	if( currentLibPackages[index].placementMode == 'L' )
	{
		m_combos[INSIDE_DIM]->SetStyle( m_combos[INSIDE_DIM]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[SECURITY_GAP]->SetStyle( m_combos[SECURITY_GAP]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[INTERFERENCE]->SetStyle( m_combos[INTERFERENCE]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[VELSTART_DOWN]->SetStyle( m_combos[VELSTART_DOWN]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[VELSTART_UP]->SetStyle( m_combos[VELSTART_UP]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[VEL_DOWN]->SetStyle( m_combos[VEL_DOWN]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[VEL_UP]->SetStyle( m_combos[VEL_UP]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[ACC_DOWN]->SetStyle( m_combos[ACC_DOWN]->GetStyle() & ~CELL_STYLE_READONLY );
		m_combos[ACC_UP]->SetStyle( m_combos[ACC_UP]->GetStyle() & ~CELL_STYLE_READONLY );

		m_combos[INSIDE_DIM]->SetTxt( currentLibPackages[index].ledth_insidedim );
		m_combos[SECURITY_GAP]->SetTxt( currentLibPackages[index].ledth_securitygap );
		m_combos[INTERFERENCE]->SetTxt( currentLibPackages[index].ledth_interf );
		m_combos[VELSTART_DOWN]->SetTxt( currentLibPackages[index].ledth_place_vmin );
		m_combos[VELSTART_UP]->SetTxt( currentLibPackages[index].ledth_rising_vmin );
		m_combos[VEL_DOWN]->SetTxt( currentLibPackages[index].ledth_place_vel );
		m_combos[VEL_UP]->SetTxt( currentLibPackages[index].ledth_rising_vel );
		m_combos[ACC_DOWN]->SetTxt( currentLibPackages[index].ledth_place_acc );
		m_combos[ACC_UP]->SetTxt( currentLibPackages[index].ledth_rising_acc );
	}
	else
	{
		m_combos[INSIDE_DIM]->SetStyle( m_combos[INSIDE_DIM]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[SECURITY_GAP]->SetStyle( m_combos[SECURITY_GAP]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[INTERFERENCE]->SetStyle( m_combos[INTERFERENCE]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[VELSTART_DOWN]->SetStyle( m_combos[VELSTART_DOWN]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[VELSTART_UP]->SetStyle( m_combos[VELSTART_UP]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[VEL_DOWN]->SetStyle( m_combos[VEL_DOWN]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[VEL_UP]->SetStyle( m_combos[VEL_UP]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[ACC_DOWN]->SetStyle( m_combos[ACC_DOWN]->GetStyle() | CELL_STYLE_READONLY );
		m_combos[ACC_UP]->SetStyle( m_combos[ACC_UP]->GetStyle() | CELL_STYLE_READONLY );

		m_combos[INSIDE_DIM]->SetTxt( "- " );
		m_combos[SECURITY_GAP]->SetTxt( "- " );
		m_combos[INTERFERENCE]->SetTxt( "- " );
		m_combos[VELSTART_DOWN]->SetTxt( "- " );
		m_combos[VELSTART_UP]->SetTxt( "- " );
		m_combos[VEL_DOWN]->SetTxt( "- " );
		m_combos[VEL_UP]->SetTxt( "- " );
		m_combos[ACC_DOWN]->SetTxt( "- " );
		m_combos[ACC_UP]->SetTxt( "- " );
	}
}

void PackageAdvancedParamsUI::onEdit()
{
	char old_checkPick = currentLibPackages[index].checkPick;
	currentLibPackages[index].checkPick = m_combos[CHECK_PICK]->GetChar();

	if( old_checkPick == 'V' || old_checkPick == 'B' )
	{
		currentLibPackages[index].checkVacuumThr = m_combos[CHECK_VTHR]->GetInt();
	}

	currentLibPackages[index].centeringPID = m_combos[PID_CENT]->GetInt();
	currentLibPackages[index].placementPID = m_combos[PID_PLACE]->GetInt();
	currentLibPackages[index].steadyCentering = m_combos[STEADY_CENT]->GetYN();
	currentLibPackages[index].check_post_place = m_combos[CHECK_PLACE]->GetYN();

	char old_placementMode = currentLibPackages[index].placementMode;
	currentLibPackages[index].placementMode = m_combos[PLACE_MODE]->GetChar();

	if( old_placementMode == 'L' )
	{
		currentLibPackages[index].ledth_insidedim = m_combos[INSIDE_DIM]->GetFloat();
		currentLibPackages[index].ledth_securitygap = m_combos[SECURITY_GAP]->GetFloat();
		currentLibPackages[index].ledth_interf = m_combos[INTERFERENCE]->GetFloat();
		currentLibPackages[index].ledth_place_vmin = m_combos[VELSTART_DOWN]->GetInt();
		currentLibPackages[index].ledth_rising_vmin = m_combos[VELSTART_UP]->GetInt();
		currentLibPackages[index].ledth_place_vel = m_combos[VEL_DOWN]->GetInt();
		currentLibPackages[index].ledth_rising_vel = m_combos[VEL_UP]->GetInt();
		currentLibPackages[index].ledth_place_acc = m_combos[ACC_DOWN]->GetInt();
		currentLibPackages[index].ledth_rising_acc = m_combos[ACC_UP]->GetInt();
	}
}

void PackageAdvancedParamsUI::onClose()
{
}

int fn_PackageAdvancedParams( CWindow* parent, int curRecord )
{
	PackageAdvancedParamsUI win( parent, curRecord );
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Package corrections
//---------------------------------------------------------------------------
extern int G_MapPackOffsetXY( int packIndex );

PackageCorrectionsUI::PackageCorrectionsUI( CWindow* parent, int curRecord ) : CWindowTable( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X );
	SetClientAreaPos( 0, 6 );
	SetClientAreaSize( 50, 12 );

	SetTitle( MsgGetString(Msg_01965) );

	index = curRecord;
	selectedObject = 0;
	prevTableCol = 1;
	firstTime = true;

	// create combos
	combos[GLOBAL_ANGLE] = new C_Combo( 2, 1, MsgGetString(Msg_00008), 10, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	// set params
	combos[GLOBAL_ANGLE]->SetVMinMax( -360.f, 360.f );
	// add to combo list
	comboList = new CComboList( this );
	comboList->Add( combos[GLOBAL_ANGLE], 0, 0 );
}

PackageCorrectionsUI::~PackageCorrectionsUI()
{
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

void PackageCorrectionsUI::onInit()
{
	// create table
	m_table = new CTable( 2, 4, 4, TABLE_STYLE_DEFAULT, this );

	// add columns
	m_table->AddCol( MsgGetString(Msg_00532),  8, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL | CELL_STYLE_CENTERED );
	m_table->AddCol( MsgGetString(Msg_01958), 10, CELL_TYPE_SDEC, CELL_STYLE_CENTERED );
	m_table->AddCol( MsgGetString(Msg_01959), 10, CELL_TYPE_SDEC, CELL_STYLE_CENTERED );
	m_table->AddCol( MsgGetString(Msg_01960), 15, CELL_TYPE_SINT, CELL_STYLE_CENTERED );

	// set params
	m_table->SetColMinMax( 1, PACKOFF_MIN, PACKOFF_MAX );
    m_table->SetColMinMax( 2, PACKOFF_MIN, PACKOFF_MAX );
	m_table->SetColMinMax( 3, PACKOFF_TMIN, PACKOFF_TMAX );

	m_table->SetText( 0, 0, "  0" );
	m_table->SetText( 1, 0, " 90" );
	m_table->SetText( 2, 0, "180" );
	m_table->SetText( 3, 0, "270" );

	// set callback function
	m_table->SetOnSelectCellCallback( boost::bind( &PackageCorrectionsUI::onSelectionChange, this, _1, _2 ) );
}

void PackageCorrectionsUI::onShow()
{
	// show combos
	comboList->Show();
}

void PackageCorrectionsUI::onRefresh()
{
	if( firstTime )
	{
		firstTime = false;
		m_table->Deselect();

		selectedObject = 0;
	}

	combos[GLOBAL_ANGLE]->SetTxt( currentLibOffsetPackages[index].angle );

	for( int i = 0; i < 4; i++ )
	{
		m_table->SetText( i, 1, currentLibOffsetPackages[index].offX[i] );
		m_table->SetText( i, 2, currentLibOffsetPackages[index].offY[i] );
		m_table->SetText( i, 3, int(currentLibOffsetPackages[index].offRot[i]) );
	}
}

void PackageCorrectionsUI::onEdit()
{
	currentLibOffsetPackages[index].angle = combos[GLOBAL_ANGLE]->GetFloat();

	for( int i = 0; i < 4; i++ )
	{
		currentLibOffsetPackages[index].offX[i] = m_table->GetFloat( i, 1 );
		currentLibOffsetPackages[index].offY[i] = m_table->GetFloat( i, 2 );
		currentLibOffsetPackages[index].offRot[i] = m_table->GetInt( i, 3 );
	}
}

void PackageCorrectionsUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_01968), K_F3, 0, NULL, boost::bind( &PackageCorrectionsUI::onOffsetTeaching, this ) );
}

bool PackageCorrectionsUI::onKeyPress( int key )
{
	if( key == K_ESC || key == K_TAB || key == K_ALT_M )
	{
		return false;
	}

	if( key == K_F2 )
	{
		comboList->SetEdit( true );
		return false;
	}
	if( key == K_F3 )
	{
		onOffsetTeaching();
		return true;
	}

	if( selectedObject == 0 )
	{
		switch( key )
		{
			case K_DOWN:
				selectedObject = 1;
				comboList->CurDeselect();
				m_table->Select( 0, prevTableCol );
				break;

			default:
				comboList->ManageKey( key );
				onEdit();
				break;
		}
		return true;
	}

	switch( key )
	{
		case K_UP:
			if( m_table->GetCurRow() == 0 )
			{
				selectedObject = 0;
				prevTableCol = m_table->GetCurCol();
				comboList->CurSelect();
				m_table->Deselect();
				return true;
			}
			break;

		default:
			break;
	}

	return false;
}

int PackageCorrectionsUI::onSelectionChange( unsigned int row, unsigned int col )
{
	DrawPie( PointI( GetX()+GetW()/2, GetY()+10*GUI_CharH() ), 18, (row+1)*90 );
	return 1;
}

int PackageCorrectionsUI::onOffsetTeaching()
{
	return G_MapPackOffsetXY( index );
}


int fn_PackageCorrections( CWindow* parent, int curRecord )
{
	PackageCorrectionsUI win( parent, curRecord );
	win.Show();
	win.Hide();

	return 1;
}



//---------------------------------------------------------------------------
// finestra: Package dispensing
//---------------------------------------------------------------------------
PackDosData dispPack;


// stampa maschera punti
void Print_pack( CWindow* parent )
{
	float max_x, max_y, min_x, min_y;
	float endy_s, endy_d, endx_a, endx_b;
	float pix_x, pix_y, scala=0.25;  // il valore max della scala e' 0.5
	float min_dist;
	int x, y, i, raggio=5;
	int max_punti;

	CWindow* Q_Fds = new CWindow( parent );
	Q_Fds->SetStyle( WIN_STYLE_CENTERED );
	Q_Fds->SetClientAreaSize( 80, 25 );
	Q_Fds->SetTitle( MsgGetString(Msg_00703) );

	GUI_Freeze();

	Q_Fds->Show();

	// draw panel
	RectI _panel( 1, 1, Q_Fds->GetW()/GUI_CharW() - 2, Q_Fds->GetH()/GUI_CharH() - 2 );
	Q_Fds->DrawPanel( _panel );

	int size_x = _panel.W*GUI_CharW() - 1;
	int size_y = _panel.H*GUI_CharH() - 1;
	int centre_x = Q_Fds->GetX() + _panel.X*GUI_CharW() + size_x/2;
	int centre_y = Q_Fds->GetY() + _panel.Y*GUI_CharH() + size_y/2;

	// draw cross
	GUI_VLine( centre_x, centre_y-size_y/2+1, centre_y+size_y/2-1, GUI_color(GR_WHITE) );
	GUI_HLine( centre_x-size_x/2+1, centre_x+size_x/2, centre_y, GUI_color(GR_WHITE) );

	// cerca n. max punti
	max_punti=1;

	if(dispPack.npoints_s>max_punti)
	{
		max_punti=dispPack.npoints_s;
	}

	if(dispPack.npoints_d>max_punti)
	{
		max_punti=dispPack.npoints_d;
	}

	if(dispPack.npoints_a>max_punti)
	{
		max_punti=dispPack.npoints_a;
	}

	if(dispPack.npoints_b>max_punti)
	{
		max_punti=dispPack.npoints_b;
	}

	// cerca min distanza
	min_dist=1000;

	if((dispPack.npoints_s>1) && (dispPack.dist_s<min_dist))
	{
		min_dist=dispPack.dist_s;
	}

	if((dispPack.npoints_d>1) && (dispPack.dist_d<min_dist))
	{
		min_dist=dispPack.dist_d;
	}

	if((dispPack.npoints_a>1) && (dispPack.dist_a<min_dist))
	{
	  	min_dist=dispPack.dist_a;
   	}

	if((dispPack.npoints_b>1) && (dispPack.dist_b<min_dist))
	{
	 	 min_dist=dispPack.dist_b;
   	}

	// cerca coordinata fine ogni riga punti
	endy_s=dispPack.npoints_s*dispPack.dist_s+dispPack.oy_s;
	endy_d=dispPack.npoints_d*dispPack.dist_d+dispPack.oy_d;
	endx_a=dispPack.npoints_a*dispPack.dist_a+dispPack.ox_a;
	endx_b=dispPack.npoints_b*dispPack.dist_b+dispPack.ox_b;

   //------------------------------------------------------------

	min_x=-0.05; // per evitare divisione per zero

	if(dispPack.ox_s-(dispPack.displacement*0.5)<min_x)
	{
		min_x=dispPack.ox_s-(dispPack.displacement*0.5);
	}

	if(dispPack.ox_d-(dispPack.displacement*0.5)<min_x)
	{
		min_x=dispPack.ox_d-(dispPack.displacement*0.5);
	}

	if(dispPack.ox_a<min_x)
	{
		min_x=dispPack.ox_a;
	}

	if(dispPack.ox_b<min_x)
	{
		min_x=dispPack.ox_b;
	}

   //------------------------------------------------------------

	min_y=-0.05; // per evitare divisione per zero

	if(dispPack.oy_s<min_y)
	{
		min_y=dispPack.oy_s;
	}

	if(dispPack.oy_d<min_y)
	{
		min_y=dispPack.oy_d;
	}

	if(dispPack.oy_a-(dispPack.displacement*0.5)<min_y)
	{
		min_y=dispPack.oy_a-(dispPack.displacement*0.5);
	}

	if(dispPack.oy_b-(dispPack.displacement*0.5)<min_y)
	{
		min_y=dispPack.oy_b-(dispPack.displacement*0.5);
	}

   //------------------------------------------------------------

	max_x=1; // per evitare divisione per zero

	if(endx_a>max_x)
	{
		max_x=endx_a;
	}

	if(endx_b>max_x)
	{
		max_x=endx_b;
	}

	if(dispPack.ox_s+(dispPack.displacement*0.5)>max_x)
	{
		max_x=dispPack.ox_s+(dispPack.displacement*0.5);
	}

	if(dispPack.ox_d+(dispPack.displacement*0.5)>max_x)
	{
		max_x=dispPack.ox_d+(dispPack.displacement*0.5);
	}

   //------------------------------------------------------------

	max_y=1; // per evitare divisione per zero

	if(endy_s>max_y)
	{
		max_y=endy_s;
	}

	if(endy_d>max_y)
	{
		max_y=endy_d;
	}

	if(dispPack.oy_a+(dispPack.displacement*0.5)>max_y)
	{
		max_y=dispPack.oy_a+(dispPack.displacement*0.5);
	}

	if(dispPack.oy_b+(dispPack.displacement*0.5)>max_y)
	{
		max_y=dispPack.oy_b+(dispPack.displacement*0.5);
	}

	if(max_y > -min_y)
	{
		pix_y=size_y/fabs(max_y);
	}
	else
	{
		pix_y=size_y/fabs(min_y);
	}

	if(max_x > -min_x)
	{
		pix_x=size_x/fabs(max_x);
	}
	else
	{
		pix_x=size_x/fabs(min_x);
	}

	if(pix_x>pix_y)
	{
		pix_x=pix_y;
	}
	else
	{
		pix_y=pix_x;
	}

	if((min_dist*pix_x*scala)<2.5*float(raggio))
	{
		scala=0.35;
		raggio=int((min_dist*pix_x*scala)/3.);
	}

	pix_x=pix_x*scala;
	pix_y=pix_y*scala;

	for(i=0;i<dispPack.npoints_s;i++)
	{
		if( (dispPack.escl_start_s==0) ||
			(dispPack.escl_start_s>0 && i<dispPack.escl_start_s-1) ||
			(dispPack.escl_end_s>=dispPack.escl_start_s &&
			i>dispPack.escl_end_s-1))
		{
			if((i & 1)==0)
			{
				x=int(centre_x+pix_x*(dispPack.ox_s-dispPack.displacement*0.5));
			}
			else
			{
				x=int(centre_x+pix_x*(dispPack.ox_s+dispPack.displacement*0.5));
			}

			y=int(centre_y-pix_y*(dispPack.dist_s*i+dispPack.oy_s));
			GUI_FillCircle( PointI( x, y ), raggio, GUI_color(GR_LIGHTGREEN) );
			GUI_Circle( PointI( x, y ), raggio, GUI_color(GR_WHITE) );
		}
	}

	for(i=0;i<dispPack.npoints_d;i++)
	{
		if( (dispPack.escl_start_d==0) ||
			(dispPack.escl_start_d>0 && i<dispPack.escl_start_d-1) ||
			(dispPack.escl_end_d>=dispPack.escl_start_d &&
			i>dispPack.escl_end_d-1))
		{

			if((i & 1)==0)
			{
				x=int(centre_x+pix_x*(dispPack.ox_d+dispPack.displacement*0.5));
			}
			else
			{
				x=int(centre_x+pix_x*(dispPack.ox_d-dispPack.displacement*0.5));
			}

			y=int(centre_y-pix_y*(dispPack.dist_d*i+dispPack.oy_d));
			GUI_FillCircle( PointI( x, y ), raggio, GUI_color(GR_LIGHTGREEN) );
			GUI_Circle( PointI( x, y ), raggio, GUI_color(GR_WHITE) );
		}
	}

	for(i=0;i<dispPack.npoints_a;i++)
	{
		if( (dispPack.escl_start_a==0) ||
			(dispPack.escl_start_a>0 && i<dispPack.escl_start_a-1) ||
			(dispPack.escl_end_a>=dispPack.escl_start_a &&
			i>dispPack.escl_end_a-1))
		{

			x=int(centre_x+pix_x*(dispPack.dist_a*i+dispPack.ox_a));

			if((i & 1)==0)
			{
				y=int(centre_y-pix_y*(dispPack.oy_a+dispPack.displacement*0.5));
			}
			else
			{
				y=int(centre_y-pix_y*(dispPack.oy_a-dispPack.displacement*0.5));
			}

			GUI_FillCircle( PointI( x, y ), raggio, GUI_color(GR_LIGHTGREEN) );
			GUI_Circle( PointI( x, y ), raggio, GUI_color(GR_WHITE) );
		}
	}

	for(i=0;i<dispPack.npoints_b;i++)
	{
		if( (dispPack.escl_start_b==0) ||
			(dispPack.escl_start_b>0 && i<dispPack.escl_start_b-1) ||
			(dispPack.escl_end_b>=dispPack.escl_start_b &&
			i>dispPack.escl_end_b-1))
		{

			x=int(centre_x+pix_x*(dispPack.dist_b*i+dispPack.ox_b));

			if((i & 1)==0)
			{
				y=int(centre_y-pix_y*(dispPack.oy_b-dispPack.displacement*0.5));
			}
			else
			{
				y=int(centre_y-pix_y*(dispPack.oy_b+dispPack.displacement*0.5));
			}

			GUI_FillCircle( PointI( x, y ), raggio, GUI_color(GR_LIGHTGREEN) );
			GUI_Circle( PointI( x, y ), raggio, GUI_color(GR_WHITE) );
		}
	}

	GUI_Thaw();

	Handle();

	Q_Fds->Hide();
	delete Q_Fds;
}

PackageDispensingUI::PackageDispensingUI( CWindow* parent, int curRecord, int dispenser ) : CWindowTable( parent )
{
	SetStyle( WIN_STYLE_CENTERED_X );
	SetClientAreaPos( 0, 8 );
	//SetClientAreaSize( 59, 12 );
	SetClientAreaSize( 59, 25 );

	#ifndef __DISP2
	SetTitle( MsgGetString(Msg_00126) );
	#else
	char buf[80];
	snprintf( buf, 80, "%s - %d", MsgGetString(Msg_00126), dispenser+1 );
	SetTitle( buf );
	#endif

	index = curRecord;
	dispNumber = dispenser;
	selectedObject = 0;
	prevTableCol = 1;
	firstTime = true;

	// create combos
	combos[QUANT] = new C_Combo( 2, 2, MsgGetString(Msg_00692), 7, CELL_TYPE_UINT );
	combos[DISPL] = new C_Combo( 2, 3, MsgGetString(Msg_01870), 7, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
	// set params
	combos[QUANT]->SetVMinMax( 10, 999 );
	combos[DISPL]->SetVMinMax( PKDISPLACEMENT_MIN, PKDISPLACEMENT_MAX );
	// add to combo list
	comboList = new CComboList( this );
	comboList->Add( combos[QUANT], 0, 0 );
	comboList->Add( combos[DISPL], 1, 0 );

	//
	Dosatore->LoadPackData( dispNumber+1, currentLibPackages[index].code-1 );
	Dosatore->GetPackData( dispNumber+1, dispPack );
}

PackageDispensingUI::~PackageDispensingUI()
{
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

void PackageDispensingUI::onInit()
{
	// create table
	m_table = new CTable( 2, 7, 4, TABLE_STYLE_DEFAULT, this );

	// add columns
	m_table->AddCol( "", 14, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
	m_table->AddCol( MsgGetString(Msg_00536),  6, CELL_TYPE_SDEC, CELL_STYLE_CENTERED );
	m_table->AddCol( MsgGetString(Msg_00537),  6, CELL_TYPE_SDEC, CELL_STYLE_CENTERED );
	m_table->AddCol( MsgGetString(Msg_00687),  2, CELL_TYPE_UINT, CELL_STYLE_CENTERED );
	m_table->AddCol( MsgGetString(Msg_00688),  5, CELL_TYPE_UDEC, CELL_STYLE_CENTERED );
	m_table->AddCol( MsgGetString(Msg_00690), 12, CELL_TYPE_UINT, CELL_STYLE_CENTERED );
	m_table->AddCol( MsgGetString(Msg_00691),  4, CELL_TYPE_UINT, CELL_STYLE_CENTERED );

	// set params
	m_table->SetColMinMax( 1, -99.99f, 99.99f );
    m_table->SetColMinMax( 2, -99.99f, 99.99f );
	m_table->SetColMinMax( 3, 0, 999 );
	m_table->SetColMinMax( 4, 0.f, 99.99f );
	m_table->SetColMinMax( 5, 0, 999 );
	m_table->SetColMinMax( 6, 0, 999 );

	m_table->SetText( 0, 0, MsgGetString(Msg_00683) );
	m_table->SetText( 1, 0, MsgGetString(Msg_00684) );
	m_table->SetText( 2, 0, MsgGetString(Msg_00685) );
	m_table->SetText( 3, 0, MsgGetString(Msg_00686) );

	// set callback function
	m_table->SetOnSelectCellCallback( boost::bind( &PackageDispensingUI::onSelectionChange, this, _1, _2 ) );
}

void PackageDispensingUI::onShow()
{
	// show combos
	comboList->Show();

	// show hints
	printPackHints();
}

void PackageDispensingUI::onRefresh()
{
	if( firstTime )
	{
		firstTime = false;
		m_table->Deselect();

		selectedObject = 0;
	}

	combos[QUANT]->SetTxt( dispPack.quant );
	combos[DISPL]->SetTxt( dispPack.displacement );

	m_table->SetText( 0, 1, dispPack.ox_s );
	m_table->SetText( 0, 2, dispPack.oy_s );
	m_table->SetText( 0, 3, dispPack.npoints_s );
	m_table->SetText( 0, 4, dispPack.dist_s );
	m_table->SetText( 0, 5, dispPack.escl_start_s );
	m_table->SetText( 0, 6, dispPack.escl_end_s );

	m_table->SetText( 1, 1, dispPack.ox_a );
	m_table->SetText( 1, 2, dispPack.oy_a );
	m_table->SetText( 1, 3, dispPack.npoints_a );
	m_table->SetText( 1, 4, dispPack.dist_a );
	m_table->SetText( 1, 5, dispPack.escl_start_a );
	m_table->SetText( 1, 6, dispPack.escl_end_a );

	m_table->SetText( 2, 1, dispPack.ox_d );
	m_table->SetText( 2, 2, dispPack.oy_d );
	m_table->SetText( 2, 3, dispPack.npoints_d );
	m_table->SetText( 2, 4, dispPack.dist_d );
	m_table->SetText( 2, 5, dispPack.escl_start_d );
	m_table->SetText( 2, 6, dispPack.escl_end_d );

	m_table->SetText( 3, 1, dispPack.ox_b );
	m_table->SetText( 3, 2, dispPack.oy_b );
	m_table->SetText( 3, 3, dispPack.npoints_b );
	m_table->SetText( 3, 4, dispPack.dist_b );
	m_table->SetText( 3, 5, dispPack.escl_start_b );
	m_table->SetText( 3, 6, dispPack.escl_end_b );
}

void PackageDispensingUI::onEdit()
{
	dispPack.quant = combos[QUANT]->GetInt();
	dispPack.displacement = combos[DISPL]->GetFloat();

	dispPack.ox_s = m_table->GetFloat( 0, 1 );
	dispPack.oy_s = m_table->GetFloat( 0, 2 );
	dispPack.npoints_s = m_table->GetInt( 0, 3 );
	dispPack.dist_s = m_table->GetFloat( 0, 4 );

	int start = m_table->GetInt( 0, 5 );
	int end = m_table->GetInt( 0, 6 );

	if( start <= dispPack.npoints_s )
	{
		dispPack.escl_start_s = start;
	}

	if( dispPack.escl_start_s == 0 )
	{
		dispPack.escl_end_s = 0;
	}
	else
	{
		if( dispPack.escl_end_s == 0 )
		{
			dispPack.escl_end_s = dispPack.escl_start_s;
		}
		else
		{
			if( end > dispPack.npoints_s || end < dispPack.escl_start_s )
			{
			}
			else
			{
				dispPack.escl_end_s = end;
			}
		}
	}


	dispPack.ox_a = m_table->GetFloat( 1, 1 );
	dispPack.oy_a = m_table->GetFloat( 1, 2 );
	dispPack.npoints_a = m_table->GetInt( 1, 3 );
	dispPack.dist_a = m_table->GetFloat( 1, 4 );

	start = m_table->GetInt( 1, 5 );
	end = m_table->GetInt( 1, 6 );

	if( start <= dispPack.npoints_a )
	{
		dispPack.escl_start_a = start;
	}

	if( dispPack.escl_start_a == 0 )
	{
		dispPack.escl_end_a = 0;
	}
	else
	{
		if( dispPack.escl_end_a == 0 )
		{
			dispPack.escl_end_a = dispPack.escl_start_a;
		}
		else
		{
			if( end > dispPack.npoints_a || end < dispPack.escl_start_a )
			{
			}
			else
			{
				dispPack.escl_end_a = end;
			}
		}
	}


	dispPack.ox_d = m_table->GetFloat( 2, 1 );
	dispPack.oy_d = m_table->GetFloat( 2, 2 );
	dispPack.npoints_d = m_table->GetInt( 2, 3 );
	dispPack.dist_d = m_table->GetFloat( 2, 4 );

	start = m_table->GetInt( 2, 5 );
	end = m_table->GetInt( 2, 6 );

	if( start <= dispPack.npoints_d )
	{
		dispPack.escl_start_d = start;
	}

	if( dispPack.escl_start_d == 0 )
	{
		dispPack.escl_end_d = 0;
	}
	else
	{
		if( dispPack.escl_end_d == 0 )
		{
			dispPack.escl_end_d = dispPack.escl_start_d;
		}
		else
		{
			if( end > dispPack.npoints_d || end < dispPack.escl_start_d )
			{
			}
			else
			{
				dispPack.escl_end_d = end;
			}
		}
	}


	dispPack.ox_b = m_table->GetFloat( 3, 1 );
	dispPack.oy_b = m_table->GetFloat( 3, 2 );
	dispPack.npoints_b = m_table->GetInt( 3, 3 );
	dispPack.dist_b = m_table->GetFloat( 3, 4 );

	start = m_table->GetInt( 3, 5 );
	end = m_table->GetInt( 3, 6 );

	if( start <= dispPack.npoints_b )
	{
		dispPack.escl_start_b = start;
	}

	if( dispPack.escl_start_b == 0 )
	{
		dispPack.escl_end_b = 0;
	}
	else
	{
		if( dispPack.escl_end_b == 0 )
		{
			dispPack.escl_end_b = dispPack.escl_start_b;
		}
		else
		{
			if( end > dispPack.npoints_b || end < dispPack.escl_start_b )
			{
			}
			else
			{
				dispPack.escl_end_b = end;
			}
		}
	}
}

void PackageDispensingUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_00703), K_F3, 0, NULL, boost::bind( &PackageDispensingUI::onPackPreview, this ) );
}

bool PackageDispensingUI::onKeyPress( int key )
{
	if( key == K_ESC || key == K_TAB || key == K_ALT_M )
	{
		return false;
	}

	if( key == K_F2 )
	{
		comboList->SetEdit( true );
		return false;
	}
	if( key == K_F3 )
	{
		onPackPreview();
		return true;
	}

	if( selectedObject < 2 )
	{
		if( key == K_DOWN )
		{
			selectedObject++;
		}
		if( key == K_UP )
		{
			selectedObject = MAX( 0 , selectedObject-1);
		}

		if( selectedObject != 2 )
		{
			comboList->ManageKey( key );
			onEdit();
			return true;
		}

		// passo alla tabella
		comboList->CurDeselect();
		m_table->Select( 0, prevTableCol );
		return true;
	}
	else
	{
		if( key == K_UP )
		{
			if( m_table->GetCurRow() == 0 )
			{
				selectedObject--;
				prevTableCol = m_table->GetCurCol();
				comboList->CurSelect();
				m_table->Deselect();
				return true;
			}
		}
	}

	return false;
}

void PackageDispensingUI::onClose()
{
	Dosatore->WritePackData( dispNumber+1, currentLibPackages[index].code-1, dispPack );
}

int PackageDispensingUI::onSelectionChange( unsigned int row, unsigned int col )
{
	return 1;
}

int PackageDispensingUI::onPackPreview()
{
	Print_pack( this );
	return 1;
}

void PackageDispensingUI::printPackHints()
{
	int size = 200;
	int pos_x = 475;
	int pos_y = 460;
	int cross_half = 5;
	int radius = 5;
	int dist_l = 20;
	int dist_h = 40;
	int displ = 45;

	// HORIZONTAL
	char* text = (char*)MsgGetString(Msg_01583);

	// draw contour
	GUI_FillRect( RectI( pos_x, pos_y, size, size ), GUI_color(GR_DARKGRAY) );
	GUI_Rect( RectI( pos_x, pos_y, size, size ), GUI_color(GR_BLACK) );

	// draw center cross
	GUI_VLine( pos_x+size/2, pos_y+size/2-cross_half, pos_y+size/2+cross_half, GUI_color(GR_BLACK) );
	GUI_HLine( pos_x+size/2-cross_half, pos_x+size/2+cross_half, pos_y+size/2, GUI_color(GR_BLACK) );

	// draw dots
	GUI_FillCircle( PointI( pos_x+dist_l, pos_y+dist_h ), radius, GUI_color(GR_LIGHTGREEN) );
	GUI_Circle( PointI( pos_x+dist_l, pos_y+dist_h ), radius, GUI_color(GR_WHITE) );
	GUI_FillCircle( PointI( pos_x+dist_l+displ, pos_y+dist_h ), radius, GUI_color(GR_LIGHTGREEN) );
	GUI_Circle( PointI( pos_x+dist_l+displ, pos_y+dist_h ), radius, GUI_color(GR_WHITE) );
	GUI_FillCircle( PointI( pos_x+dist_l+displ*2, pos_y+dist_h ), radius, GUI_color(GR_LIGHTGREEN) );
	GUI_Circle( PointI( pos_x+dist_l+displ*2, pos_y+dist_h ), radius, GUI_color(GR_WHITE) );

	// draw title
	GUI_DrawText( pos_x+size/2-strlen(text)*GUI_CharW(GUI_SmallFont)/2, pos_y-GUI_CharH(GUI_SmallFont)-2, text, GUI_SmallFont, GUI_color(GR_BLACK) );

	// draw X rif
	GUI_VLine( pos_x+dist_l, pos_y+dist_h+radius*2+2, pos_y+size-10, GUI_color(GR_WHITE) );
	GUI_VLine( pos_x+size/2, pos_y+size/2+cross_half+6, pos_y+size-10, GUI_color(GR_WHITE) );
	text = (char*)MsgGetString(Msg_00536);
	GUI_DrawText( pos_x+dist_l+(size/2-dist_l)/2-strlen(text)*GUI_CharW(GUI_XSmallFont)/2, pos_y+size-10-GUI_CharH(GUI_XSmallFont), text, GUI_XSmallFont, GUI_color(GR_WHITE) );

	// draw Y rif
	GUI_HLine( pos_x+dist_l+displ*2+radius+6, pos_x+size-10, pos_y+dist_h, GUI_color(GR_WHITE) );
	GUI_HLine( pos_x+size/2+cross_half+6, pos_x+size-10, pos_y+size/2, GUI_color(GR_WHITE) );
	text = (char*)MsgGetString(Msg_00537);
	GUI_DrawText( pos_x+size-strlen(text)*GUI_CharW(GUI_XSmallFont)-10, pos_y+dist_h+(size/2-dist_h)/2-GUI_CharH(GUI_XSmallFont)/2, text, GUI_XSmallFont, GUI_color(GR_WHITE) );

	// draw Dist
	GUI_VLine( pos_x+dist_l, pos_y+10, pos_y+dist_h-radius*2-2, GUI_color(GR_WHITE) );
	GUI_VLine( pos_x+dist_l+displ, pos_y+10, pos_y+dist_h-radius*2-2, GUI_color(GR_WHITE) );
	text = (char*)MsgGetString(Msg_00688);
	GUI_DrawText( pos_x+dist_l+displ/2-strlen(text)*GUI_CharW(GUI_XSmallFont)/2, pos_y+10, text, GUI_XSmallFont, GUI_color(GR_WHITE) );

	// VERTICAL
	pos_x = 765;

	text = (char*)MsgGetString(Msg_01584);

	// draw contour
	GUI_FillRect( RectI( pos_x, pos_y, size, size ), GUI_color(GR_DARKGRAY) );
	GUI_Rect( RectI( pos_x, pos_y, size, size ), GUI_color(GR_BLACK) );

	// draw center cross
	GUI_VLine( pos_x+size/2, pos_y+size/2-cross_half, pos_y+size/2+cross_half, GUI_color(GR_BLACK) );
	GUI_HLine( pos_x+size/2-cross_half, pos_x+size/2+cross_half, pos_y+size/2, GUI_color(GR_BLACK) );

	// draw dots
	GUI_FillCircle( PointI( pos_x+dist_h, pos_y+size-dist_l ), radius, GUI_color(GR_LIGHTGREEN) );
	GUI_Circle( PointI( pos_x+dist_h, pos_y+size-dist_l ), radius, GUI_color(GR_WHITE) );
	GUI_FillCircle( PointI( pos_x+dist_h, pos_y+size-dist_l-displ ), radius, GUI_color(GR_LIGHTGREEN) );
	GUI_Circle( PointI( pos_x+dist_h, pos_y+size-dist_l-displ ), radius, GUI_color(GR_WHITE) );
	GUI_FillCircle( PointI( pos_x+dist_h, pos_y+size-dist_l-displ*2 ), radius, GUI_color(GR_LIGHTGREEN) );
	GUI_Circle( PointI( pos_x+dist_h, pos_y+size-dist_l-displ*2 ), radius, GUI_color(GR_WHITE) );

	// draw title
	GUI_DrawText( pos_x+size/2-strlen(text)*GUI_CharW(GUI_SmallFont)/2, pos_y-GUI_CharH(GUI_SmallFont)-2, text, GUI_SmallFont, GUI_color(GR_BLACK) );

	// draw X rif
	GUI_VLine( pos_x+dist_h, pos_y+10, pos_y+size-dist_l-displ*2-radius-6, GUI_color(GR_WHITE) );
	GUI_VLine( pos_x+size/2, pos_y+10, pos_y+size/2-cross_half-6, GUI_color(GR_WHITE) );
	text = (char*)MsgGetString(Msg_00536);
	GUI_DrawText( pos_x+dist_h+(size/2-dist_h)/2-strlen(text)*GUI_CharW(GUI_XSmallFont)/2, pos_y+10, text, GUI_XSmallFont, GUI_color(GR_WHITE) );

	// draw Y rif
	GUI_HLine( pos_x+size/2+cross_half+6, pos_x+size-10, pos_y+size/2, GUI_color(GR_WHITE) );
	GUI_HLine( pos_x+dist_h+radius+6, pos_x+size-10, pos_y+size-dist_l, GUI_color(GR_WHITE) );
	text = (char*)MsgGetString(Msg_00537);
	GUI_DrawText( pos_x+size-strlen(text)*GUI_CharW(GUI_XSmallFont)-10, pos_y+size-dist_l-(size/2-dist_l)/2-GUI_CharH(GUI_XSmallFont)/2, text, GUI_XSmallFont, GUI_color(GR_WHITE) );

	// draw Dist
	GUI_HLine( pos_x+10, pos_x+dist_h-radius-6, pos_y+size-dist_l, GUI_color(GR_WHITE) );
	GUI_HLine( pos_x+10, pos_x+dist_h-radius-6, pos_y+size-dist_l-displ, GUI_color(GR_WHITE) );
	text = (char*)MsgGetString(Msg_00688);
	GUI_DrawText( pos_x+10, pos_y+size-dist_l-displ/2-GUI_CharH(GUI_XSmallFont)/2, text, GUI_XSmallFont, GUI_color(GR_WHITE) );
}



//---------------------------------------------------------------------------
// finestra: Packages selection
//---------------------------------------------------------------------------
#define PACKSEL_NAME_LEN         21
#define PACKSEL_NOTES_LEN        23

//---------------------------------------------------------------------------
// Costruttore/Distruttore
//---------------------------------------------------------------------------
PackagesSelectUI::PackagesSelectUI( CWindow* parent, const std::string& name, int selectEnable )
: CWindowTable( parent )
{
	m_x = 2;
	m_y = 2;
	m_numRows = 20;
	m_selectEnable = selectEnable;

	SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_EDIT );
	SetClientAreaPos( 0, 4 );

	int width = PACKSEL_NAME_LEN + PACKSEL_NOTES_LEN + 5;
	SetClientAreaSize( width, m_numRows + 3 );

	SetTitle( MsgGetString(Msg_00680) );

	tips = 0;
	m_exitCode = WIN_EXITCODE_ESC;

	loadPackages();

	m_start_item = searchName( name );
}

PackagesSelectUI::~PackagesSelectUI()
{
}

//---------------------------------------------------------------------------
// Visualizza elementi della lista (partendo dal primo)
//---------------------------------------------------------------------------
void PackagesSelectUI::ShowItems()
{
	m_start_item = -1;
	showItems( 0 );
}

//---------------------------------------------------------------------------
// Ritorna il nome del package selezionato
//---------------------------------------------------------------------------
std::string PackagesSelectUI::GetPackageName()
{
	int selectedRow = getSelectedRow();
	if( selectedRow == -1 )
		return 0;

	std::list<SRow>::iterator it = m_items.begin();
	for( int i = 0; i < selectedRow; i++)
		++it;

	return currentLibPackages[(*it).index].name;
}

//---------------------------------------------------------------------------
// Ritorna il codice del package selezionato
//---------------------------------------------------------------------------
int PackagesSelectUI::GetPackageCode()
{
	int selectedRow = getSelectedRow();
	if( selectedRow == -1 )
		return -1;

	std::list<SRow>::iterator it = m_items.begin();
	for( int i = 0; i < selectedRow; i++)
		++it;

	return currentLibPackages[(*it).index].code;
}


void PackagesSelectUI::onInit()
{
	// create table
	m_table = new CTable( m_x, m_y, m_numRows, TABLE_STYLE_DEFAULT, this );

	// add column
	m_table->AddCol( MsgGetString(Msg_00061), PACKSEL_NAME_LEN, CELL_TYPE_TEXT, CELL_STYLE_READONLY );
	m_table->AddCol( MsgGetString(Msg_00060), PACKSEL_NOTES_LEN, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
}

void PackagesSelectUI::onShow()
{
	// serve per assicurare il refresh della tabella
	int start = m_start_item;
	m_start_item = -1;
	showItems( start );

	if( m_selectEnable )
	{
		tips = new CPan( 22, 1, MsgGetString(Msg_01439) );
	}
}

void PackagesSelectUI::onShowMenu()
{
	m_menu->Add( MsgGetString(Msg_00921), K_F2, 0, NULL, boost::bind( &PackagesSelectUI::showPackage, this ) );      //show package
	m_menu->Add( MsgGetString(Msg_00475), K_F3, 0, NULL, boost::bind( &PackagesSelectUI::newPackage, this ) );       // new
	m_menu->Add( MsgGetString(Msg_00476), K_F4, 0, NULL, boost::bind( &PackagesSelectUI::duplicatePackage, this ) ); // duplicate
	m_menu->Add( MsgGetString(Msg_00478), K_F5, 0, NULL, boost::bind( &PackagesSelectUI::renamePackage, this ) );    // rename
	m_menu->Add( MsgGetString(Msg_00477), K_F6, 0, NULL, boost::bind( &PackagesSelectUI::deletePackage, this ) );    // delete
}

bool PackagesSelectUI::onKeyPress( int key )
{
	switch( key )
	{
		case K_F2:
			showPackage();
			return true;

		case K_F3:
			newPackage();
			return true;

		case K_F4:
			duplicatePackage();
			return true;

		case K_F5:
			renamePackage();
			return true;

		case K_F6:
			deletePackage();
			return true;

		case K_DOWN:
		case K_UP:
		case K_PAGEDOWN:
		case K_PAGEUP:
			if( !m_search.empty() )
			{
				m_search.clear();
				showSearch();
			}
			return vSelect( key );

		case K_BACKSPACE:
			if( !m_search.empty() )
			{
				m_search.erase( m_search.end()-1, m_search.end() );
				searchItem();
			}
			break;

		case K_ESC:
			if( !m_search.empty() )
			{
				m_search.clear();
				showSearch();
				return true;
			}
			m_exitCode = WIN_EXITCODE_ESC;
			break;

		case K_ENTER:
			if( !m_search.empty() )
			{
				m_search.clear();
				showSearch();
			}

			if( m_selectEnable )
			{
				// ENTER classi derivate
				onEnter();
				forceExit();
				m_exitCode = WIN_EXITCODE_ENTER;
				return true;
			}
			break;

		default:
			if( onHotKey( key ) )
			{
				if( !m_search.empty() )
				{
					m_search.clear();
					showSearch();
				}
				return true;
			}
			else if( key < 256 )
			{
				if( strchr( CHARSET_TEXT, key ) != NULL && m_search.size() < PACKSEL_NAME_LEN )
				{
					m_search.push_back( key );
					searchItem();
				}
			}
			break;
	}

	return false;
}

void PackagesSelectUI::onClose()
{
	if( tips )
	{
		delete tips;
		tips = 0;
	}
}

//---------------------------------------------------------------------------
// Carica nella lista i packages
//---------------------------------------------------------------------------
void PackagesSelectUI::loadPackages()
{
	m_items.clear();

	for( int i = 0; i < MAXPACK; i++ )
	{
		SRow row;
		if( currentLibPackages[i].name[0] != '\0' )
		{
			row.index = i;
			row.name = currentLibPackages[i].name;
			row.notes = currentLibPackages[i].notes;
			m_items.push_back( row );
		}
	}

	// Ordina la lista in base al campo name
	m_items.sort( boost::bind( &PackagesSelectUI::compareNoCase, this, _1, _2 ) );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void PackagesSelectUI::showItems( int start_item )
{
	GUI_Freeze_Locker lock;

	start_item = MAX( 0, start_item );
	if( start_item == m_start_item || start_item >= m_items.size() )
		return;

	std::list<SRow>::iterator it = m_items.begin();
	for( int i = 0; i < start_item; i++)
		++it;

	unsigned int i = 0;
	while( i + start_item < m_items.size() && i < m_table->GetRows() )
	{
		m_table->SetText( i, 0, (*it).name.c_str() );
		m_table->SetText( i, 1, (*it).notes.c_str() );
		++i;
		++it;
	}
	while( i < m_table->GetRows() )
	{
		m_table->SetText( i, 0, "" );
		++i;
	}

	m_start_item = start_item;
}

//--------------------------------------------------------------------------
// Sposta la selezione verticalmente
//--------------------------------------------------------------------------
bool PackagesSelectUI::vSelect( int key )
{
	int curR = m_table->GetCurRow();
	if( curR < 0 )
		return false;

	if( key == K_DOWN )
	{
		if( curR + m_start_item >= m_items.size() - 1 )
		{
			// niente altro da selezionare
			return true;
		}

		if( curR < m_table->GetRows() - 1 )
		{
			// lascio eseguire la selezione al parent
			return false;
		}
		else // sono nell'ultima riga
		{
			// aggiorno la tabella
			showItems( m_start_item + 1 );
			return true;
		}
	}
	else if( key == K_UP )
	{
		if( curR + m_start_item == 0 )
		{
			// niente altro da selezionare
			return true;
		}

		if( curR > 0 )
		{
			// lascio eseguire la selezione al parent
			return false;
		}
		else // sono nella prima riga
		{
			// aggiorno la tabella
			showItems( m_start_item - 1 );
			return true;
		}
	}
	else if( key == K_PAGEDOWN )
	{
		if( curR + m_start_item >= m_items.size() - 1 )
		{
			// niente altro da selezionare
			return true;
		}

		// aggiorno la tabella
		showItems( m_start_item + m_table->GetRows() );
		m_table->Select( 0, 0 );
		return true;
	}
	else if( key == K_PAGEUP )
	{
		if( curR + m_start_item == 0 )
		{
			// niente altro da selezionare
			return true;
		}

		// aggiorno la tabella
		showItems( m_start_item - m_table->GetRows() );
		m_table->Select( 0, 0 );
		return true;
	}

	return false;
}

//--------------------------------------------------------------------------
// Ricerca elemento nella lista
//--------------------------------------------------------------------------
#define SEARCH_MATCH_OK       0
#define SEARCH_MATCH_NEXT     1
#define SEARCH_MATCH_END      2

void PackagesSelectUI::searchItem()
{
	unsigned int c_match = 0;
	unsigned int match_result = SEARCH_MATCH_OK;

	unsigned int pos = -1;
	unsigned int pos_match = -1;
	for( std::list<SRow>::iterator it = m_items.begin(); it != m_items.end(); ++it )
	{
		unsigned int c = 0;
		while( c < m_search.size() && c < (*it).name.size() )
		{
			if( tolower(m_search[c]) < tolower((*it).name[c]) )
			{
				match_result = SEARCH_MATCH_END;
				break;
			}
			else if( tolower(m_search[c]) > tolower((*it).name[c]) )
			{
				match_result = SEARCH_MATCH_NEXT;
				break;
			}

			match_result = SEARCH_MATCH_OK;
			++c;
		}

		if( c > c_match )
		{
			c_match = c;
			pos_match = pos;
		}
		else if( match_result == SEARCH_MATCH_END || c == m_search.size() )
		{
			break;
		}

		++pos;
	}

	if( c_match )
	{
		showItems( pos_match+1 );
		m_table->Select( 0, 0 );
	}

	showSearch( c_match < m_search.size() ? true : false );
}

//---------------------------------------------------------------------------
// Visualizza/Nasconde box di ricerca
//---------------------------------------------------------------------------
void PackagesSelectUI::showSearch( bool error )
{
	GUI_Freeze_Locker lock;

	RectI r;
	r.X = GetX() + TextToGraphX( m_x ) - 2;
	r.Y = GetY() + TextToGraphY( m_y + m_numRows ) - 2;
	r.W = TextToGraphX( PACKSEL_NAME_LEN ) + 4;
	r.H = GUI_CharH() + 2;

	if( m_search.empty() )
	{
		GUI_FillRect( r, GUI_color( WIN_COL_CLIENTAREA ) );
		GUI_HLine( r.X, r.X+r.W, r.Y, GUI_color( GRID_COL_IN_BORDER ) );
		GUI_HLine( r.X, r.X+r.W, r.Y+2, GUI_color( GRID_COL_OUT_BORDER ) );
	}
	else
	{
		GUI_Rect( r, GUI_color( WIN_COL_BORDER ) );
		GUI_FillRect( RectI( r.X+1, r.Y+1, r.W-2, r.H-2 ), error ? GUI_color( GRID_COL_BG_ERROR ) : GUI_color( 255,255,255 ) );

		int X = GetX() + TextToGraphX( m_x );
		int Y = GetY() + TextToGraphY( m_y + m_numRows ) - 1;
		GUI_DrawText( X, Y, m_search.c_str(), GUI_SmallFont, error ? GUI_color( GRID_COL_BG_ERROR ) : GUI_color( 255,255,255 ), error ? GUI_color( GRID_COL_FG_ERROR ) : GUI_color( 0, 0, 0 ) );
	}
}

//---------------------------------------------------------------------------
// Compara due stringhe (usata per il sort)
//---------------------------------------------------------------------------
bool PackagesSelectUI::compareNoCase( SRow row1, SRow row2 )
{
	return boost::ilexicographical_compare( row1.name, row2.name );
}

//---------------------------------------------------------------------------
// Restituisce il numero della riga corrente
//---------------------------------------------------------------------------
int PackagesSelectUI::getSelectedRow()
{
	int selectedRow = m_start_item + m_table->GetCurRow();
	if( selectedRow < 0 || selectedRow >= m_items.size() )
		return -1;
	return selectedRow;
}

//---------------------------------------------------------------------------
// Ritorna l'indice del primo slot libero nella libreria (-1 se non trovato)
//---------------------------------------------------------------------------
int PackagesSelectUI::searchFirstFreeSlot()
{
	for( int i = 0; i < MAXPACK; i++ )
	{
		if( currentLibPackages[i].name[0] == '\0' )
		{
			return i;
		}
	}

	return -1;
}

//---------------------------------------------------------------------------
// Cerca un package nella libreria (-1 se non trovato)
//---------------------------------------------------------------------------
int PackagesSelectUI::searchName( const std::string& name )
{
	if( name.empty() )
	{
		return -1;
	}

	int i = 0;
	for( std::list<SRow>::iterator it = m_items.begin(); it != m_items.end(); ++it )
	{
		if( boost::iequals( (*it).name, name.c_str() ) )
		{
			return i;
		}

		i++;
	}
/*
	for( int i = 0; i < MAXPACK; i++ )
	{
		if( boost::iequals( currentLibPackages[i].name, name.c_str() ) )
		{
			return i;
		}
	}
*/
	return -1;
}

//---------------------------------------------------------------------------
// Visualizza parametri package
//---------------------------------------------------------------------------
int PackagesSelectUI::showPackage()
{
	int selectedRow = getSelectedRow();
	if( selectedRow == -1 )
		return 0;

	std::list<SRow>::iterator it = m_items.begin();
	for( int i = 0; i < selectedRow; i++)
		++it;

	fn_PackageParams( this, (*it).index );

	// reload notes
	(*it).notes = currentLibPackages[(*it).index].notes;
	// serve per assicurare il refresh della tabella
	int start = m_start_item;
	m_start_item = -1;
	showItems( start );

	return 1;
}

int PackagesSelectUI::newPackage()
{
	// get first free package slot
	int freeSlot = searchFirstFreeSlot();
	if( freeSlot == -1 )
	{
		W_Mess( MsgGetString(Msg_01579) );
		return 0;
	}

	bool create = false;
	char newName[22];

	while( !create )
	{
		CInputBox inbox( this, 8, MsgGetString(Msg_00475), MsgGetString(Msg_01429), 20, CELL_TYPE_TEXT );
		inbox.SetLegalChars( CHARSET_TEXT );
		inbox.Show();

		if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
		{
			return 0;
		}

		snprintf( newName, 22, "%s", inbox.GetText() );
		strupr( newName );

		if( searchName( newName ) != -1 )
		{
			W_Mess( MsgGetString(Msg_00933) );
		}
		else
		{
			create = true;
		}
	}

	if( create )
	{
		char filename[MAXNPATH];

		// new pakage
		PackagesLib_Default( &currentLibPackages[freeSlot] );
		currentLibPackages[freeSlot].code = freeSlot+1;
		snprintf( currentLibPackages[freeSlot].name, 22, "%s", newName );

		snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, QHeader.Lib_Default, PACKAGESLIB_EXT );
		PackagesLib_Write( filename, currentLibPackages );

		// reset corrections
		PackagesOffsetLib_Default( &currentLibOffsetPackages[freeSlot] );

		snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, QHeader.Lib_Default, PACKAGESOFFLIB_EXT );
		PackagesOffsetLib_Write( filename, currentLibOffsetPackages );

		// remove vis data
		PackVisFile_Remove( currentLibPackages[freeSlot].code, QHeader.Lib_Default );

		loadPackages();
		ShowItems();
	}

	return 1;
}

int PackagesSelectUI::duplicatePackage()
{
	int selectedRow = getSelectedRow();
	if( selectedRow == -1 )
		return 0;

	std::list<SRow>::iterator it = m_items.begin();
	for( int i = 0; i < selectedRow; i++)
		++it;

	// get first free package slot
	int freeSlot = searchFirstFreeSlot();
	if( freeSlot == -1 )
	{
		W_Mess( MsgGetString(Msg_01579) );
		return 0;
	}

	bool duplicate = false;
	char newName[22];

	while( !duplicate )
	{
		CInputBox inbox( this, 8, MsgGetString(Msg_00476), MsgGetString(Msg_01429), 20, CELL_TYPE_TEXT );
		inbox.SetLegalChars( CHARSET_TEXT );
		inbox.Show();

		if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
		{
			return 0;
		}

		snprintf( newName, 22, "%s", inbox.GetText() );
		strupr( newName );

		if( searchName( newName ) != -1 )
		{
			W_Mess( MsgGetString(Msg_00933) );
		}
		else
		{
			duplicate = true;
		}
	}

	if( duplicate )
	{
		char filename[MAXNPATH];

		currentLibPackages[freeSlot] = currentLibPackages[(*it).index];
		currentLibPackages[freeSlot].code = freeSlot+1;
		snprintf( currentLibPackages[freeSlot].name, 22, "%s", newName );

		snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, QHeader.Lib_Default, PACKAGESLIB_EXT );
		PackagesLib_Write( filename, currentLibPackages );

		//TODO: duplicare anche dati visione

		loadPackages();
		ShowItems();
	}

	return 1;
}

int PackagesSelectUI::renamePackage()
{
	int selectedRow = getSelectedRow();
	if( selectedRow == -1 )
		return 0;

	std::list<SRow>::iterator it = m_items.begin();
	for( int i = 0; i < selectedRow; i++)
		++it;

	bool rename = false;
	char newName[22];

	while( !rename )
	{
		CInputBox inbox( this, 8, MsgGetString(Msg_00478), MsgGetString(Msg_01429), 20, CELL_TYPE_TEXT );
		inbox.SetLegalChars( CHARSET_TEXT );
		inbox.SetText( currentLibPackages[(*it).index].name );
		inbox.Show();

		if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
		{
			return 0;
		}

		snprintf( newName, 22, "%s", inbox.GetText() );
		strupr( newName );

		if( searchName( newName ) != -1 )
		{
			W_Mess( MsgGetString(Msg_00933) );
		}
		else
		{
			rename = true;
		}
	}

	if( rename )
	{
		char filename[MAXNPATH];

		snprintf( currentLibPackages[(*it).index].name, 22, "%s", newName );

		snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, QHeader.Lib_Default, PACKAGESLIB_EXT );
		PackagesLib_Write( filename, currentLibPackages );

		loadPackages();
		ShowItems();
	}

	return 1;
}

int PackagesSelectUI::deletePackage()
{
	int selectedRow = getSelectedRow();
	if( selectedRow == -1 )
		return 0;

	std::list<SRow>::iterator it = m_items.begin();
	for( int i = 0; i < selectedRow; i++)
		++it;

	char buf[MAXNPATH];
	snprintf( buf, MAXNPATH, MsgGetString(Msg_01177), (*it).name.c_str() );

	if( !W_Deci( 0, buf ) )
	{
		return 0;
	}

	char filename[MAXNPATH];

	// remove package
	currentLibPackages[(*it).index].name[0] = '\0';

	snprintf( filename, MAXNPATH, "%s/%s%s", FPACKDIR, QHeader.Lib_Default, PACKAGESLIB_EXT );
	PackagesLib_Write( filename, currentLibPackages );

	// remove visdata
	PackVisFile_Remove( currentLibPackages[(*it).index].code, QHeader.Lib_Default );

	loadPackages();
	ShowItems();
	return 1;
}



int fn_PackagesTable( CWindow* parent, const std::string& name )
{
	PackagesSelectUI win( parent, name );
	win.Show();
	win.Hide();

	return 1;
}

int fn_PackagesTableSelect( CWindow* parent, const std::string& name, int& packCode, std::string& packName )
{
	PackagesSelectUI win( parent, name, true );
	win.Show();
	win.Hide();

	if( win.GetExitCode() == WIN_EXITCODE_ENTER )
	{
		packCode = win.GetPackageCode();
		packName = win.GetPackageName();
		return 1;
	}

	return 0;
}

