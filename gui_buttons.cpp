//---------------------------------------------------------------------------
//
// Name:        gui_buttons.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUIButtons class implementation
//
//---------------------------------------------------------------------------
#include "gui_buttons.h"

#include "q_oper.h" //SetConfirmRequiredBeforeNextXYMovement

#include "gui_defs.h"
#include "gui_functions.h"
#include "keyutils.h"

#include <mss.h>


//--------------------------------------------------------------------------
// Costruttore classe
//--------------------------------------------------------------------------
GUIButtons::GUIButtons( int y, GUIWindow* _parent )
{
	Y = y;
	parent = _parent;
	
	activated = 0;
	cursel = -1;
	maxTxtLen = 0;
}

//--------------------------------------------------------------------------
// Distruttore classe
//--------------------------------------------------------------------------
GUIButtons::~GUIButtons()
{
	for( unsigned int i = 0; i < buttons.size(); i++ )
	{
		if( buttons[i] )
			delete buttons[i];
	}
	buttons.clear();
}

//--------------------------------------------------------------------------
// Aggiunge un bottone
//--------------------------------------------------------------------------
void GUIButtons::AddButton( const char* label, char focus )
{
	int lung = strlen( label );
	// carattere hotkey nella label
	if( strchr( label, '~' ) )
	{
		lung--;
	}
	
	if( maxTxtLen < lung )
	{
		maxTxtLen = lung;
	}
	
	GUIMenuEntry* button = new GUIMenuEntry( label );
	button->SetMode( focus );
	
	buttons.push_back( button );
}

//--------------------------------------------------------------------------
// Setta il valore di un bottone con il valore passato come parametro
//--------------------------------------------------------------------------
bool GUIButtons::SetFocus( int index, int focus )
{
	if( index < 0 || index >= buttons.size() )
		return false;
	
	buttons[index]->SetMode( focus );
	
	// Se la message box e' attiva, fa il refresh dei bottoni
	if( activated )
	{
		refresh();
	}

	return true;
}

//--------------------------------------------------------------------------
// Ridisegna i bottoni sullo schermo
//--------------------------------------------------------------------------
void GUIButtons::refresh()
{
	if( !activated )
		return;

	GUI_Freeze_Locker lock;

	RectI rect_in, rect_out;
	
	// calc button width
	int bwText = maxTxtLen + 2*BT_TXT_BORDER;
	if( bwText < BT_TXT_MIN_W )
		bwText = BT_TXT_MIN_W;
	
	rect_in.X1 = 0;
	rect_in.X2 = rect_in.X1 + bwText;
	GUI_TextToGraph( rect_in, rect_out );

	rect_out.X1 -= GUI_TextOffsetX();
	rect_out.Y1 -= GUI_TextOffsetY();
	rect_out.X2 -= 1;
	rect_out.Y2 -= 1;
	
	int bw = rect_out.X2 - rect_out.X1 + 1;
	
	// window width
	int ww = parent->GetW();
	
	// calc button position
	int rx = ww - bw * buttons.size();
	int step = rx / (buttons.size()+1);

	// calc text Y position
	int textY_oofset = (BT_HEIGHT - GUI_CharH()) / 2;

	int x = parent->GetX() + step;
	int y1 = parent->GetY() + TextToGraphY( Y );
	int y2 = BT_HEIGHT;

	for( int i = 0; i < buttons.size(); i++ )
	{
		int len = strlen(buttons[i]->GetLabel());
		if( strchr( buttons[i]->GetLabel(), '~' ) )
			len--;
		
		int x1 = x;
		int x2 = bw-1;
		
		GUI_color _color = buttons[i]->GetMode() ? GUI_color( BT_COL_FOCUS ) : GUI_color( BT_COL_NORMAL );
		GUI_FillRect( RectI(x1, y1, x2, y2), _color );
		GUI_Rect( RectI(x1-1, y1-1, x2+1, y2+1), GUI_color( BT_COL_BORDER ) );
		
		// calc text Y position
		int textX = x1 + (x2 - len*GUI_CharW()) / 2;
		
		// display text
		GUI_DrawText_HotKey( textX, y1+textY_oofset, buttons[i]->GetLabel(), GUI_DefaultFont, _color, GUI_color( BT_COL_TXT ), GUI_color( WIN_COL_HOTKEY ) );
		
		x += bw + step;
	}
}

//--------------------------------------------------------------------------
// Attiva l'insieme dei bottoni
//--------------------------------------------------------------------------
void GUIButtons::Activate()
{
	if( !buttons.size() )
		return;
	
	activated = 1;
	cursel = 0;
	
	for( int i = 0; i < buttons.size(); i++ )
	{
		if( buttons[i]->GetMode() )
		{
			cursel = i;
			break;
		}
	}
	
	refresh();
}

//--------------------------------------------------------------------------
// Cerca il bottone con Hokey indicato
//--------------------------------------------------------------------------
int GUIButtons::search( int key )
{
	if( key >= 'a' && key <= 'z' )  // convers. min. in maiuscolo
		key = key + 'A' - 'a';
	
	for( unsigned int i = 0; i < buttons.size(); i++ )
	{
		if( key == buttons[i]->GetHotKey() )
		{
			return i;
		}
	}

	return -1;
}

//--------------------------------------------------------------------------
// Gestore tasti per insieme bottoni
//--------------------------------------------------------------------------
int GUIButtons::GestKey( int key )
{
	if( !activated )
		return 0;
	
	int ret = 0;
	int pos;
	bool isChanged = false;

	SetConfirmRequiredBeforeNextXYMovement(true);

	switch( key )
	{
		case K_RIGHT:
			buttons[cursel]->SetMode( 0 );
			
			cursel++;
			if( cursel > buttons.size()-1 )
				cursel = 0;
			
			buttons[cursel]->SetMode( 1 );
			isChanged = true;
			break;

		case K_LEFT:
			buttons[cursel]->SetMode( 0 );
			
			cursel--;
			if( cursel < 0 )
				cursel = buttons.size()-1;
			
			buttons[cursel]->SetMode( 1 );
			isChanged = true;
			break;

		default:
			pos = search( key );
			if( pos != -1 )
			{
				buttons[cursel]->SetMode( 0 );
				
				cursel = pos;
				
				buttons[cursel]->SetMode( 1 );
				isChanged = true;
				ret = ret | BUTTONCONFIRMED;
			}
			break;
	}
	
	if( isChanged )
	{
		refresh();
	}
	
	return ret + cursel + 1;
}
