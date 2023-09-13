//---------------------------------------------------------------------------
// Name:        q_functions.cpp
// Author:      Gabriel Ferri
// Created:     15/09/2011
// Description: Quadra miscellaneous functions
//---------------------------------------------------------------------------
#include "q_functions.h"

#include "c_window.h"
#include "msglist.h"
#include "keyutils.h"
#include "q_gener.h"

#include <mss.h>


//--------------------------------------------------------------------------
// Richiesta immissione password
//--------------------------------------------------------------------------
bool Ask_Password( std::string password )
{
	if( password.size() == 0 )
	{
		return true;
	}

	char buf[128];
	strncpy( buf, ">> ", sizeof(buf) );
	for( unsigned int i = 0; i < password.size(); i++ )
	{
		strncat( buf, ". ", sizeof(buf) );
	}
	strncat( buf, "<<", sizeof(buf) );

	CWindow* win = new CWindow( 0 );
	win->SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_CAPTION | WIN_STYLE_NO_MENU );
	win->SetClientAreaPos( 0, 10 );
	win->SetClientAreaSize( 60, 7 );

	GUI_Freeze();

	win->Show();
	win->DrawTextCentered( 1, MsgGetString(Msg_00345) );
	win->DrawTextCentered( 4, buf );

	GUI_Thaw();

	// clear keyboard buffer
	flushKeyboardBuffer();

	std::string input;

	int key = 0;
	while( key != K_ESC && key != K_ENTER )
	{
		key = Handle();

		if( key == K_BACKSPACE )
		{
			if( input.size() > 0 )
			{
				input.erase( input.size()-1 );
				buf[3+input.size()*2] = '.';

				win->DrawTextCentered( 4, buf );
			}
		}
		else if( ischar_legal( CHARSET_FILENAME, key ) )
		{
			if( input.size() < password.size() )
			{
				buf[3+input.size()*2] = '*';
				input.push_back( toupper(key) );

				win->DrawTextCentered( 4, buf );
			}
		}
	}

	win->Hide();
	delete win;

	if( key == K_ESC )
	{
		return false;
	}

	if( password.compare( input ) != 0 )
	{
		return false;
	}

	return true;
}

