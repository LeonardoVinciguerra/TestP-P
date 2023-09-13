//---------------------------------------------------------------------------
//
// Name:        gui_submenu.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_SubMenu class implementation
//
//---------------------------------------------------------------------------
#include "gui_submenu.h"

#include "q_oper.h" //SetConfirmRequiredBeforeNextXYMovement

#include "gui_defs.h"
#include "gui_functions.h"
#include "keyutils.h"

#include <mss.h>


// Lista hotkey esterni validi per il menu
int HotkeyMap[] = { K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, K_SHIFT_F1, K_SHIFT_F2, K_SHIFT_F3, K_SHIFT_F4, K_SHIFT_F5, K_SHIFT_F6, K_SHIFT_F7, K_SHIFT_F8, K_SHIFT_F9, K_SHIFT_F10, K_SHIFT_F11, K_SHIFT_F12 };

const char* HotkeyStr[] = { "  F1", "  F2", "  F3", "  F4", "  F5", "  F6", "  F7", "  F8", "  F9", " F10", " F11", " F12", " SF1", " SF2", " SF3", " SF4", " SF5", " SF6", " SF7", " SF8", " SF9", "SF10", "SF11", "SF12" };


//---------------------------------------------------------------------------
// Costruttore della classe GUI_SubMenu
//---------------------------------------------------------------------------
GUI_SubMenu::GUI_SubMenu( int x, int y, GUI_SubMenu* parent )
: GUIWindow()
{
	SetStyle( WIN_STYLE_NO_CAPTION );

	client_area_text.X = x;
	client_area_text.Y = y;
	menu_parent = parent;

	maxTxtLen = 0;
	curPos = 0;
};

//--------------------------------------------------------------------------
// Distruttore della classe
//--------------------------------------------------------------------------
GUI_SubMenu::~GUI_SubMenu()
{
	Hide();

	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		if( entries[i] )
			delete entries[i];
	}
	entries.clear();
}

//--------------------------------------------------------------------------
// Funzione che aggiunge una voce al menu
//--------------------------------------------------------------------------
void GUI_SubMenu::Add( const char* label, int hotkey, int mode, GUI_SubMenu* subMenu, GUIMenuCallback callback, GUIMenuCallback exitcallback )
{
	int lung = strlen( label );
	// carattere hotkey nella label
	if( strchr( label, '~' ) )
	{
		lung--;
	}
	// spazio per freccia sottomenu
	if( subMenu )
	{
		lung++;
	}
	// spazio per hotkey
	int hk_index = checkHotkey( hotkey );
	if( hk_index != -1 )
	{
		lung += SM_HOTKEY_LEN;
	}

	if( maxTxtLen < lung )
	{
		maxTxtLen = lung;
	}

	GUIMenuEntry* entry = new GUIMenuEntry( label );
	entry->SetSubMenu( subMenu );
	entry->SetCallback( callback );
	entry->SetExitCallback( exitcallback );
	entry->SetMode( mode );
	if( hk_index != -1 )
	{
		entry->SetHotKey( hotkey );
	}

	entries.push_back( entry );
}

//--------------------------------------------------------------------------
// Ritorna le dimensioni del sotto menu
//--------------------------------------------------------------------------
int GUI_SubMenu::GetWidth()
{
	return maxTxtLen + 2*SM_TXT_BORDER;
}

int GUI_SubMenu::GetHeight()
{
	return entries.size() - 1;
}

//--------------------------------------------------------------------------
// Visualizza il sotto menu
//--------------------------------------------------------------------------
int GUI_SubMenu::Show()
{
	GUI_Freeze();

	if( client_area_text.X == 0 && client_area_text.Y == 0 )
	{
		client_area_text.X = 5;
		client_area_text.Y = 5;
	}

	client_area_text.W = GetWidth();
	client_area_text.H = GetHeight() + 1;

	RectI client_area_pix;
	GUI_TextToGraph( client_area_text, client_area_pix );

	GUIWindow::SetClientArea( client_area_pix );
	GUIWindow::Show();

	// draw entries
	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		drawEntry( i, false );
	}
	// draw selected entry
	drawEntry( curPos, true );

	GUI_Thaw();

	while( 1 )
	{
		SetConfirmRequiredBeforeNextXYMovement(true);

		int key = Handle();
		switch( key )
		{
			case K_UP:
				drawEntry( curPos, false );

				if( curPos > 0 )
					curPos--;
				else
					curPos = entries.size() - 1;
				break;

			case K_DOWN:
				drawEntry( curPos, false );

				if( curPos < entries.size() - 1 )
					curPos++;
				else
					curPos = 0;
				break;

			case K_PAGEUP:
				drawEntry( curPos, false );
				curPos = 0;
				break;

			case K_PAGEDOWN:
				drawEntry( curPos, false );
				curPos = entries.size() - 1;
				break;

			case K_LEFT:
			case K_ESC:
				Hide();
				return K_ESC;
				break;

			case K_ENTER:
				if( entries[curPos]->GetMode() == MENU_ENTRY_STYLE_GRAYED )
				{
					break;
				}

				onSelect();

				// se presente funzione di uscita la chiama
				if( entries[curPos]->GetExitCallback() )
				{
					entries[curPos]->GetExitCallback()();
				}

				//se il menu e' stato cancellato dal sottomenu chiamato, termina
				if( !IsShown() )
				{
					return K_ENTER;
				}

				if( entries[curPos]->GetCallback() )
				{
					return K_ENTER;
				}
				break;

			default:
				int pos = search( key );

				if( pos != -1 )
				{
					drawEntry( curPos, false );
					curPos = pos;
					drawEntry( curPos, true );
					onSelect();

					//se il menu e' stato cancellato dal sottomenu chiamato, termina
					if( !IsShown() )
					{
						return K_ENTER;
					}

					if( entries[curPos]->GetCallback() )
					{
						return K_ENTER;
					}
				}
				break;
		}

		drawEntry( curPos, true );
	}
}

//--------------------------------------------------------------------------
// Nasconde tutti i menu chiamanti il menu corrente
//--------------------------------------------------------------------------
void GUI_SubMenu::HideAll()
{
	GUI_SubMenu* pMenu = this;

	while( pMenu != NULL )
	{
		GUI_SubMenu* pParent = pMenu->menu_parent;
		pMenu->Hide();
		pMenu->menu_parent = NULL;
		pMenu = pParent;
	}
}

//--------------------------------------------------------------------------
// Gestione tasto premuto
//--------------------------------------------------------------------------
bool GUI_SubMenu::ManageKey( int key )
{
	int pos = search( key );

	if( pos != -1 )
	{
		entries[curPos]->GetCallback();
		return true;
	}

	return false;
}

//--------------------------------------------------------------------------
// Visializza voce della barra del menu
//--------------------------------------------------------------------------
void GUI_SubMenu::drawEntry( int i, bool highlight )
{
	GUI_Freeze_Locker lock;

	RectI rect_in, rect_out;
	rect_in.X = client_area_text.X;
	rect_in.Y = client_area_text.Y + i;
	rect_in.W = GetWidth();
	rect_in.H = 1;
	GUI_TextToGraph( rect_in, rect_out );

	GUI_color bgColor;
	GUI_color textColor;

	if( highlight )
	{
		bgColor = GUI_color( SM_COL_HIGHLIGHT );
		textColor = (entries[i]->GetMode() == MENU_ENTRY_STYLE_GRAYED) ? GUI_color( SM_COL_TXT_GRAYED ) : GUI_color( SM_COL_TXT_HIGHLIGHT );
	}
	else
	{
		bgColor = GUI_color( SM_COL_BACKGROUND );
		textColor = (entries[i]->GetMode() == MENU_ENTRY_STYLE_GRAYED) ? GUI_color( SM_COL_TXT_GRAYED ) : GUI_color( SM_COL_TXT_NORMAL );
	}

	int Y1 = rect_out.Y;
	int X2 = rect_out.X + rect_out.W - 1;
	GUI_FillRect( rect_out, bgColor );

	// check hot key
	int hk_index = checkHotkey( entries[i]->GetHotKey() );

	// draw text
	if( hk_index != -1 )
	{
		GUI_DrawText( rect_out.X + GUI_CharW(), Y1, entries[i]->GetLabel(), GUI_DefaultFont, bgColor, textColor );
	}
	else
	{
		GUI_DrawText_HotKey( rect_out.X + GUI_CharW(), Y1, entries[i]->GetLabel(), GUI_DefaultFont, bgColor, textColor, GUI_color( WIN_COL_HOTKEY ) );
	}

	// draw hotkey
	if( hk_index != -1 )
	{
		GUI_DrawText( X2 - SM_HOTKEY_LEN * GUI_CharW(), Y1, HotkeyStr[hk_index], GUI_DefaultFont, textColor );
	}

	// draw arrow
	if( entries[i]->GetSubMenu() )
	{
		int xa = X2 - SM_MENU_ARROW_DIM - 1;
		int ya = Y1 + SM_MENU_ARROW_DIM + 4;

		GUI_FillTrigon( xa, ya-SM_MENU_ARROW_DIM, xa, ya+SM_MENU_ARROW_DIM, xa+SM_MENU_ARROW_DIM, ya, GUI_color( 0, 0, 0 ) );
	}
}

//--------------------------------------------------------------------------
// Cerca il cod. tasto premuto nelle entries
//--------------------------------------------------------------------------
int GUI_SubMenu::search( int key )
{
	if( key >= 'a' && key <= 'z' )  // convers. min. in maiuscolo
		key = key + 'A' - 'a';

	for( unsigned int i = 0; i < entries.size(); i++ )
	{
		if( key == entries[i]->GetHotKey() )
		{
			if( entries[i]->GetMode() != MENU_ENTRY_STYLE_GRAYED )
			{
				return i;
			}
		}
	}

	return -1;
}

//--------------------------------------------------------------------------
// Apre sottomenu/Esegue funzione collegata alla entry
//--------------------------------------------------------------------------
bool GUI_SubMenu::onSelect()
{
	// se presente apri sottomenu
	if( entries[curPos]->GetSubMenu() )
	{
		((GUI_SubMenu*)entries[curPos]->GetSubMenu())->SetParent( this );

		int x, y;
		calGUI_SubMenuPosition( (GUI_SubMenu*)entries[curPos]->GetSubMenu(), x, y );
		((GUI_SubMenu*)entries[curPos]->GetSubMenu())->SetPos( x, y );

		((GUI_SubMenu*)entries[curPos]->GetSubMenu())->Show();
	}
	// se presente chiama funzione e chiude tutti i menu
	else if( entries[curPos]->GetCallback() )
	{
		HideAll();
		return entries[curPos]->GetCallback()();
	}

	return false;
}

//--------------------------------------------------------------------------
// Calcola coordinate sottomenu
//--------------------------------------------------------------------------
void GUI_SubMenu::calGUI_SubMenuPosition( GUI_SubMenu* sm, int& x, int& y )
{
	x = client_area_text.X + GetWidth();
	y = client_area_text.Y + curPos;

	if( x + sm->GetWidth() > GUI_ScreenCols() )
	{
		x = MAX( 0, client_area_text.X - sm->GetWidth() );
	}

	if( y + sm->GetHeight() > GUI_ScreenRows() )
	{
		y = MAX( 0, GUI_ScreenRows() - sm->GetWidth() );
	}
}

//--------------------------------------------------------------------------
// Controlla hotkey
//--------------------------------------------------------------------------
int GUI_SubMenu::checkHotkey( int hotkey )
{
	for( unsigned int i = 0; i < sizeof(HotkeyMap)/sizeof(int); i++ )
	{
		if( hotkey == HotkeyMap[ i ] )
			return i;
	}

	return -1;
}
