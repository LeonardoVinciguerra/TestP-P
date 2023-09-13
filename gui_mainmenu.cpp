//---------------------------------------------------------------------------
//
// Name:        gui_mainmenu.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_MainMenu class implementation
//
//---------------------------------------------------------------------------
#include "gui_mainmenu.h"

#include "gui_defs.h"
#include "gui_functions.h"
#include "gui_submenu.h"
#include "keyutils.h"
#include "q_oper.h"

#include <mss.h>


//---------------------------------------------------------------------------
// Costruttore della classe
//---------------------------------------------------------------------------
GUI_MainMenu::GUI_MainMenu()
{
	curPos = -1;
}

//---------------------------------------------------------------------------
// Distruttore della classe
//---------------------------------------------------------------------------
GUI_MainMenu::~GUI_MainMenu()
{
	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		if( entries[i] )
			delete entries[i];
	}
	entries.clear();
}

//--------------------------------------------------------------------------
// Associa ad una voce della barra del menu' il menu a tendina passato
//--------------------------------------------------------------------------
void GUI_MainMenu::Add( GUIMenuEntry* entry )
{
	entries.push_back( entry );
}

//---------------------------------------------------------------------------
// Cerca il codice del tasto premuto all'interno delle voci del PopUp
// INPUT:   Key: Codice del tasto premuto
// RETURN:  Indice della voce corrispondente se trovata, -1 altrimenti
//---------------------------------------------------------------------------
int GUI_MainMenu::search( int key )
{
	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		int altKey = (SDLK_a + (entries[i]->GetHotKey() - 65) | K_LALT | K_RALT);
		if( key == altKey )
			return i;
	}
	
	return -1;
}

//---------------------------------------------------------------------------
// Crea e visualizza la barra del menu
//---------------------------------------------------------------------------
void GUI_MainMenu::Show()
{
	GUI_Freeze_Locker lock;

	// background
	GUI_color color2( MM_COL_BG );
	color2.a = MM_COL_BG_ALPHA;
	GUI_FillRectA( RectI(0, SB_HEIGHT, MM_MENU_WIDTH, GUI_ScreenH()-2*SB_HEIGHT), color2 );

	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		drawEntry( i, false );
	}

	curPos = -1;
	Activate( 0 );
}

//---------------------------------------------------------------------------
// Visualizza voce selezionata
// INPUT:	pos: Indice della voce da selezionare
//---------------------------------------------------------------------------
void GUI_MainMenu::Activate( int pos )
{
	GUI_Freeze_Locker lock;

	if( pos < 0 || pos >= entries.size() )
		return;

	if( pos != curPos )
	{
		for( unsigned int i = 0; i < entries.size(); i++ )
		{
			if( i == curPos )
			{
				drawEntry( i, false );
			}
			else if( i == pos )
			{
				drawEntry( i, true );
			}
		}
	}

	curPos = pos;
}

//---------------------------------------------------------------------------
// Seleziona voce precedente
//---------------------------------------------------------------------------
void GUI_MainMenu::SelPrev(void)
{
	if( curPos > 0 )
	{
		if( entries[curPos]->GetSubMenu() )
			((GUI_SubMenu*)entries[curPos]->GetSubMenu())->Hide();
		Activate( curPos-1 );
	}
}

//---------------------------------------------------------------------------
// Seleziona voce successiva
//---------------------------------------------------------------------------
void GUI_MainMenu::SelNext(void)
{
	if( curPos < entries.size()-1 )
	{
		if( entries[curPos]->GetSubMenu() )
			((GUI_SubMenu*)entries[curPos]->GetSubMenu())->Hide();
		Activate( curPos+1 );
	}
}

//---------------------------------------------------------------------------
// Gestione dei tasti premuti
// INPUT:	c: Codice del tasto premuto
//---------------------------------------------------------------------------
void GUI_MainMenu::GestKey( int c )
{
	SetConfirmRequiredBeforeNextXYMovement(true);

	switch(c)
	{
		case K_UP:
			SelPrev();
			break;

		case K_DOWN:
			SelNext();
			break;

		case K_ENTER:
			onSelect();
			break;

		default:
			int pos = search( c );

			if( pos != -1 )
			{
				if( entries[curPos]->GetSubMenu() )
					((GUI_SubMenu*)entries[curPos]->GetSubMenu())->Hide();
				Activate( pos );
				onSelect();
			}
			break;
	}
}

//--------------------------------------------------------------------------
// Apre sottomenu/Esegue funzione collegata alla entry
//--------------------------------------------------------------------------
bool GUI_MainMenu::onSelect()
{
	if( entries[curPos]->GetSubMenu() )
	{
		((GUI_SubMenu*)entries[curPos]->GetSubMenu())->SetPos( (entries[curPos]->GetPosX() + MM_MENU_ENTRY_WIDTH)/GUI_CharW(), entries[curPos]->GetPosY()/GUI_CharH() );
		((GUI_SubMenu*)entries[curPos]->GetSubMenu())->Show();
		return true;
	}
	else if( entries[curPos]->GetCallback() )
	{
		entries[curPos]->GetCallback()();
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
// Visializza voce della barra del menu
//--------------------------------------------------------------------------
void GUI_MainMenu::drawEntry( int i, bool highlighted )
{
	GUI_Freeze_Locker lock;

	RectI r( entries[i]->GetPosX() + MM_MENU_HEIGHT/2, entries[i]->GetPosY()-5, MM_MENU_ENTRY_WIDTH - MM_MENU_HEIGHT, MM_MENU_HEIGHT+1 );
	int x2 = entries[i]->GetPosX() + MM_MENU_ENTRY_WIDTH - MM_MENU_HEIGHT/2;

	GUI_color _color = highlighted ? GUI_color( MM_COL_HIGHLIGHT ) : GUI_color( MM_COL_NORMAL );

	GUI_FillRect( r, _color );
	GUI_FillCircle( PointI( r.X-1, r.Y+MM_MENU_HEIGHT/2), MM_MENU_HEIGHT/2, _color );
	GUI_FillCircle( PointI(r.X+r.W+1, r.Y+MM_MENU_HEIGHT/2), MM_MENU_HEIGHT/2, _color );

	// calc text Y position
	int textY = r.Y + (MM_MENU_HEIGHT - GUI_CharH()) / 2 + 2;
	GUI_DrawText_HotKey( r.X, textY, entries[i]->GetLabel(), GUI_DefaultFont, _color, GUI_color( 0, 0, 0 ), GUI_color( WIN_COL_HOTKEY ) );

	if( entries[i]->GetMode() == MM_MODE_SQUARE )
	{
		int box_x = x2 - SB_BOX_DIM/2;
		int box_y = r.Y + (MM_MENU_HEIGHT - SB_BOX_DIM) / 2;

		GUI_FillRect( RectI( box_x, box_y, SB_BOX_DIM, SB_BOX_DIM ), GUI_color( SB_COL_ITEM_BORDER_1 ) );
		GUI_FillRect( RectI( box_x+2, box_y+2, SB_BOX_DIM-4, SB_BOX_DIM-4 ), GUI_color( SB_BOX_COL ) );
	}
	else if( entries[i]->GetSubMenu() )
	{
		int xa = entries[i]->GetPosX() + MM_MENU_ENTRY_WIDTH - MM_MENU_ARROW_DIM - 5;
		int ya = r.Y + MM_MENU_HEIGHT/2;

		GUI_FillTrigon( xa, ya-MM_MENU_ARROW_DIM, xa, ya+MM_MENU_ARROW_DIM, xa+MM_MENU_ARROW_DIM, ya, GUI_color( 0, 0, 0 ) );
	}
}
