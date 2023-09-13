//---------------------------------------------------------------------------
//
// Name:        gui_desktop.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_DeskTop class implementation
//
//---------------------------------------------------------------------------
#include "gui_desktop.h"

#include "q_graph.h"
#include "msglist.h"
#include "strutils.h"
#include "q_gener.h"
#include "q_init.h"
#include "q_inifile.h"
#include "gui_defs.h"
#include "gui_functions.h"

#include <mss.h>


// top
#define SB_MENU       0
#define SB_CUSTOMER   1
#define SB_PROGRAM    2
#define SB_FEED_CONF  3
#define SB_PACK_LIB   4
// bottom
#define SB_DOSA_CONF  1
#ifndef __DISP2
#define SB_EDIT       2
#else
#define SB_DOSA2_CONF 2
#define SB_EDIT       3
#endif



//---------------------------------------------------------------------------
// Costruttore della classe
//---------------------------------------------------------------------------
GUI_DeskTop::GUI_DeskTop()
{
	background = 0;
	foreground = 0;
	
	sbCounter = 0;
	editMode = false;
}

//---------------------------------------------------------------------------
// Distruttore della classe
//---------------------------------------------------------------------------
GUI_DeskTop::~GUI_DeskTop()
{
	Quit();
}

//---------------------------------------------------------------------------
// Crea e visualizza il desktop
//---------------------------------------------------------------------------
void GUI_DeskTop::Show()
{
	if( !GUI_Init( Get_WindowedMode() ) )
	{
		printf( "No graphics driver found!\n" );
		exit(1);
	}

	// create status bars
	createStatusBars();

	// draw background
	GUI_Freeze_Locker lock;

	bool ret = false;
	if( GUI_ScreenW() == 1440 )
	{
		ret = displayImage( "bg1440.png", 0, 0 );
		if( !ret )
		{
			ret = displayImage( "bg1440.jpg", 0, 0 );
		}
	}
	else if( GUI_ScreenW() == 1600 )
	{
		ret = displayImage( "bg1600.png", 0, 0 );
		if( !ret )
		{
			ret = displayImage( "bg1600.jpg", 0, 0 );
		}
	}
	else if( GUI_ScreenW() == 1920 )
	{
		ret = displayImage( "bg1920.png", 0, 0 );
		if( !ret )
		{
			ret = displayImage( "bg1920.jpg", 0, 0 );
		}
	}

	if( !ret )
	{
		GUI_FillRect( RectI(0, 0, GUI_ScreenW(), GUI_ScreenH()), GUI_color(BG_COL_BRIGHT) );

		GUI_color color_dark( BG_COL_DARK );
		for( int i = -GUI_ScreenH(); i < GUI_ScreenW(); i += BG_STRIPES_GAP )
		{
			GUI_Line( i+GUI_ScreenH(), 0, i, GUI_ScreenH(), color_dark );
		}
	}

	// save desktop background
	background = GUI_SaveScreen( RectI( 0, 0, GUI_ScreenW(), GUI_ScreenH()) );
}

//---------------------------------------------------------------------------
// Chiude il desktop
//---------------------------------------------------------------------------
void GUI_DeskTop::Quit()
{
	GUI_Quit();

	if( background )
	{
		GUI_FreeSurface( &background );
		background = 0;
	}

	if( foreground )
	{
		GUI_FreeSurface( &foreground );
		foreground = 0;
	}

	if( sb_top )
	{
		delete sb_top;
		sb_top = 0;
	}

	if( sb_bottom )
	{
		delete sb_bottom;
		sb_bottom = 0;
	}
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void GUI_DeskTop::ShowBanner()
{
	// header and footer
	GUI_color color1( HEADER_COL_BRIGHT );
	color1.a = 220;
	GUI_FillRectA( RectI(0, 0, GUI_ScreenW(), SB_HEIGHT), color1 );
	GUI_FillRectA( RectI(0, GUI_ScreenH()-SB_HEIGHT, GUI_ScreenW(), SB_HEIGHT), color1 );

	// sw version
	char sw_ver[256];
	strcpy( sw_ver, SOFT_VER );
	#ifdef __DISP2
		#ifdef __DISP2_CAM
		strcat( sw_ver, Get_SingleDispenserPar() ? "  D2s" : "  D2c" );
		#else
		strcat( sw_ver, Get_SingleDispenserPar() ? "  D2s" : "  D2" );
		#endif
	#endif
	#ifdef __DOME_FEEDER
	strcat( sw_ver, "  DOMES" );
	#endif
	#ifdef __DEBUG
	strcat( sw_ver, "  (DEBUG)" );
	#endif
	strcat( sw_ver, "  " );
	strcat( sw_ver, __DATE__ );

	int len = strlen(sw_ver);
	GUI_DrawText( GUI_ScreenW() - GUI_CharW() * len - 10, GUI_ScreenH()-(SB_HEIGHT+GUI_CharH())/2, sw_ver, GUI_DefaultFont, GUI_color( 255, 255, 255 ) );
}

//---------------------------------------------------------------------------
// Mostra le barre di stato
//---------------------------------------------------------------------------
void GUI_DeskTop::ShowStatusBars( bool showMenuItem )
{
	ShowMenuItem( showMenuItem );

	if( sbCounter == 0)
	{
		// save desktop
		foreground = GUI_SaveScreen( RectI( 0, 0, GUI_ScreenW(), GUI_ScreenH()) );

		// show background
		GUI_DrawSurface( PointI(0, 0), background );

		sb_top->Show();
		sb_bottom->Show();
	}

	sbCounter++;
}

//---------------------------------------------------------------------------
// Nasconde le barre di stato
//---------------------------------------------------------------------------
void GUI_DeskTop::HideStatusBars()
{
	sbCounter--;

	if( sbCounter == 0)
	{
		sb_top->Hide();
		sb_bottom->Hide();

		// show desktop
		GUI_DrawSurface( PointI(0, 0), foreground );
		// free surface
		GUI_FreeSurface( &foreground );
		foreground = 0;
	}
}

//---------------------------------------------------------------------------
// Aggiorna le barre di stato
//---------------------------------------------------------------------------
void GUI_DeskTop::SetSB_Top( const char* cust, const char* prog, const char* conf, const char* lib )
{
	char buf[32];
	
	// customer
	snprintf( buf, 32, "%s: %s", MsgGetString(Msg_00045), cust );
	sb_top->SetLabel( SB_CUSTOMER, buf, (sbCounter > 0) ? true : false );
	
	// program
	snprintf( buf, 32, "%s: %s", MsgGetString(Msg_00046), prog );
	sb_top->SetLabel( SB_PROGRAM, buf, (sbCounter > 0) ? true : false );
	
	// feeders conf
	snprintf( buf, 32, "%s: %s", MsgGetString(Msg_00047), conf );
	sb_top->SetLabel( SB_FEED_CONF, buf, (sbCounter > 0) ? true : false );
	
	// pack lib
	snprintf( buf, 32, "%s: %s", MsgGetString(Msg_00955), lib );
	sb_top->SetLabel( SB_PACK_LIB, buf, (sbCounter > 0) ? true : false );
}

void GUI_DeskTop::SetSB_Bottom( int ndisp, char* dosaconf )
{
	char buf[32];
	
	#ifndef __DISP2
	snprintf( buf, 32, "%s%s", MsgGetString(Msg_01743), dosaconf );
	sb_bottom->SetLabel( SB_DOSA_CONF, buf, (sbCounter > 0) ? true : false );
	#else
	ndisp--;
	if( ndisp < 0 || ndisp > 1 )
		ndisp = 0;

	if( ndisp == 0 )
	{
		snprintf( buf, 32, "%s%s", MsgGetString(Msg_05119), dosaconf );
		sb_bottom->SetLabel( SB_DOSA_CONF, buf, (sbCounter > 0) ? true : false );
	}
	else
	{
		snprintf( buf, 32, "%s%s", MsgGetString(Msg_05120), dosaconf );
		sb_bottom->SetLabel( SB_DOSA2_CONF, buf, (sbCounter > 0) ? true : false );
	}
	#endif
}

//---------------------------------------------------------------------------
// Stampa la scritta Edit ON/OFF sulla riga di stato bassa
//---------------------------------------------------------------------------
void GUI_DeskTop::SetEditMode( bool mode )
{
	if( editMode == mode )
		return;

	editMode = mode;

	if( mode )
		sb_bottom->SetLabel( SB_EDIT, "EDIT ON", (sbCounter > 0) ? true : false );
	else
		sb_bottom->SetLabel( SB_EDIT, "EDIT OFF", (sbCounter > 0) ? true : false );
}

//---------------------------------------------------------------------------
// Abilita/Disabilita messaggio per l'hotkey del sottomenu
//---------------------------------------------------------------------------
void GUI_DeskTop::ShowMenuItem( bool show )
{
	sb_top->SetStatus( SB_MENU, show );
}

//---------------------------------------------------------------------------
// Abilita/Disabilita messaggio per l'hotkey del sottomenu
//---------------------------------------------------------------------------
void GUI_DeskTop::MenuItemActivated( bool activated )
{
	sb_top->SetType( SB_MENU, activated ? SB_GRAPH_2 : SB_GRAPH_1 );
}

//---------------------------------------------------------------------------
// Crea le barre di stato
//---------------------------------------------------------------------------
void GUI_DeskTop::createStatusBars()
{
	sb_top = new GUI_StatusBar( 0, SB_HEIGHT );
	sb_bottom = new GUI_StatusBar( GUI_ScreenH()-SB_HEIGHT, SB_HEIGHT );

	sb_top->Add( 21, SB_GRAPH_1, MsgGetString(Msg_00048) );
	sb_top->Add( 28, SB_TEXT, "" );
	sb_top->Add( 28, SB_TEXT, "" );
	sb_top->Add( 28, SB_TEXT, "" );
	sb_top->Add( 28, SB_TEXT, "" );

	#ifndef __DISP2
	sb_bottom->Add( 111, SB_EMPTY, "" );
	sb_bottom->Add( 14, SB_TEXT, "" );
	sb_bottom->Add( 12, SB_TEXT, "" );
	#else
	sb_bottom->Add( 95, SB_EMPTY, "" );
	sb_bottom->Add( 15, SB_TEXT, "" );
	sb_bottom->Add( 15, SB_TEXT, "" );
	sb_bottom->Add( 12, SB_TEXT, "" );
	#endif
}

//---------------------------------------------------------------------------
// Visualizza immagine
//---------------------------------------------------------------------------
bool GUI_DeskTop::displayImage( const char* filename, int x, int y )
{
	void* loadedImage = GUI_LoadImage( filename );

	if( !loadedImage )
	{
		return false;
	}

	GUI_DrawSurface( PointI( x, y ), loadedImage );
	GUI_FreeSurface( &loadedImage );
	return true;
}
