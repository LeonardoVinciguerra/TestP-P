//---------------------------------------------------------------------------
//
// Name:        gui_widgets.cpp
// Author:      Gabriel Ferri
// Created:     24/04/2012
// Description: GUI widgets declarations
//
//---------------------------------------------------------------------------
#include "gui_widgets.h"

#include "gui_functions.h"
#include "gui_defs.h"
#include "q_grcol.h"

#include <mss.h>


//--------------------------------------------------------------------------
// Indicatore analogico a barra
// Ingressi:
//		x, y: Posizione
//		value: Valore attuale della barra
//		min, max: valori minimo/massimo
//		txt: Testo da stampare
//--------------------------------------------------------------------------
void GUI_Showbar( int x, int y, int value, int min, int max, const char* txt )
{
#define XBARRA  100
#define YBARRA  12
#define GBARRA  3
#define XBARRAI (XBARRA-2*GBARRA)
#define YBARRAF (YBARRA-GBARRA)
#define XBARRAF (XBARRA-GBARRA)

	GUI_Freeze();

	GUI_color blackColor( 0,0,0 );

	// bar frame
	// central line
	GUI_VLine( x+XBARRA/2, y, y+1, blackColor );
	GUI_VLine( x+XBARRA/2, y+YBARRA-1, y+YBARRA, blackColor );
	// left line
	GUI_VLine( x+XBARRA/4, y, y+1, blackColor );
	GUI_VLine( x+XBARRA/4, y+YBARRA-1, y+YBARRA, blackColor) ;
	// right line
	GUI_VLine( x+3*XBARRA/4, y, y+1, blackColor );
	GUI_VLine( x+3*XBARRA/4, y+YBARRA-1, y+YBARRA, blackColor );
	// box
	GUI_Rect( RectI( x+GBARRA-2, y+GBARRA-2, XBARRAF+2, YBARRAF+2 ), blackColor );

	// bar
	int xvalue = (value-min)*(XBARRAI+2)/(max-min)-2;
	GUI_FillRect( RectI( x+GBARRA-1, y+GBARRA-1, XBARRAF, YBARRAF ), GUI_color(GR_DARKGRAY) );
	GUI_FillRect( RectI( x+GBARRA-1, y+GBARRA-1, GBARRA+xvalue, YBARRAF ), GUI_color(GR_RED) );

	// signs
	int y_signs = y + (YBARRA-GUI_CharH(GUI_SmallFont))/2;
	GUI_DrawText( x-(GUI_CharW(GUI_SmallFont)+4), y_signs, "-", GUI_SmallFont, GUI_color( WIN_COL_TXT ) );
	GUI_DrawText( x+XBARRA+5, y_signs, "+", GUI_SmallFont, GUI_color( WIN_COL_TXT ) );

	// text
	int l_txt = strlen(txt);
	if( l_txt )
	{
		GUI_DrawText( x+(XBARRA-(l_txt*GUI_CharW(GUI_SmallFont)))/2, y-(GUI_CharH(GUI_SmallFont)+3), txt, GUI_SmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_TXT ) );
	}

	GUI_Thaw();
}
