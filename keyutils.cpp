//---------------------------------------------------------------------------
//
// Name:        keyutils.cpp
// Author:      Gabriel Ferri
// Created:     15/10/2008 9.43.20
// Description: keyutils functions implementation
//
//---------------------------------------------------------------------------
#include "keyutils.h"

#include <mss.h>


//---------------------------------------------------------------------------
// Ritorna il codice del tasto premuto, oppure 0 se nessun tasto premuto
//---------------------------------------------------------------------------
int keyRead()
{
	int keyCode = 0;
	SDL_Event event;

	if( SDL_PollEvent( &event ) )
	{
		if( event.type == SDL_KEYDOWN )
		{
			if( event.key.keysym.sym >= SDLK_F1 && event.key.keysym.sym <= SDLK_F15 )
			{
				keyCode = event.key.keysym.sym;
			}
			else if( event.key.keysym.sym >= SDLK_NUMLOCK && event.key.keysym.sym <= SDLK_UNDO )
			{
				keyCode = 0;
			}
			else if( (char)event.key.keysym.unicode != 0 )
			{
				// questo controllo serve per prendere le combinazioni CTRL+c e CTRL+v
				if( (event.key.keysym.mod & KMOD_LCTRL) || (event.key.keysym.mod & KMOD_RCTRL) )
				{
					keyCode = event.key.keysym.sym;
				}
				else
				{
					keyCode = (char)event.key.keysym.unicode;
				}
			}
			else
			{
				keyCode = event.key.keysym.sym;
			}

			// key modifiers
			if( (event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT) )
			{
				keyCode |= (K_LSHIFT | K_RSHIFT );
			}
			if( (event.key.keysym.mod & KMOD_LCTRL) || (event.key.keysym.mod & KMOD_RCTRL) )
			{
				keyCode |= (K_LCTRL | K_RCTRL );
			}
			if( (event.key.keysym.mod & KMOD_LALT) || (event.key.keysym.mod & KMOD_RALT) )
			{
				keyCode |= (K_LALT | K_RALT );
			}
		}
	}

	return keyCode;
}


//---------------------------------------------------------------------------
// Svuota il buffer della tastiera
//---------------------------------------------------------------------------
void flushKeyboardBuffer()
{
	while( keyRead() );
}


//---------------------------------------------------------------------------
// Attende pressione di un tasto
//---------------------------------------------------------------------------
void waitKeyPress()
{
	while( !keyRead() );
}
