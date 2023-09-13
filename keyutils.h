//---------------------------------------------------------------------------
//
// Name:        keyutils.h
// Author:      Gabriel Ferri
// Created:     15/10/2008 9.43.20
// Description: keyutils functions declaration
//
//---------------------------------------------------------------------------

#ifndef __KEYUTILS_H
#define __KEYUTILS_H

#include <SDL/SDL.h>


// Codici di scansione tasti

#define K_LSHIFT			0x0400
#define K_RSHIFT			0x0800
#define K_LCTRL				0x1000
#define K_RCTRL				0x2000
#define K_LALT				0x4000
#define K_RALT				0x8000


#define K_BACKSPACE			SDLK_BACKSPACE
#define K_TAB				SDLK_TAB
#define K_ENTER				SDLK_RETURN
#define K_ESC				SDLK_ESCAPE
#define K_SPACE				SDLK_SPACE


#define K_DEL				SDLK_DELETE
#define K_INSERT			SDLK_INSERT
#define K_HOME				SDLK_HOME
#define K_END				SDLK_END
#define K_PAGEUP			SDLK_PAGEUP
#define K_PAGEDOWN			SDLK_PAGEDOWN
#define K_UP				SDLK_UP
#define K_DOWN				SDLK_DOWN
#define K_RIGHT				SDLK_RIGHT
#define K_LEFT				SDLK_LEFT

#define K_SHIFT_DEL			(K_DEL | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_INSERT		(K_INSERT | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_HOME		(K_HOME | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_END			(K_END | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_PAGEUP		(K_PAGEUP | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_PAGEDOWN	(K_PAGEDOWN | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_UP			(K_UP | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_DOWN		(K_DOWN | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_RIGHT		(K_RIGHT | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_LEFT		(K_LEFT | K_LSHIFT | K_RSHIFT)

#define K_CTRL_DEL			(K_DEL | K_LCTRL | K_RCTRL)
#define K_CTRL_INSERT		(K_INSERT | K_LCTRL | K_RCTRL)
#define K_CTRL_HOME			(K_HOME | K_LCTRL | K_RCTRL)
#define K_CTRL_END			(K_END | K_LCTRL | K_RCTRL)
#define K_CTRL_PAGEUP		(K_PAGEUP | K_LCTRL | K_RCTRL)
#define K_CTRL_PAGEDOWN		(K_PAGEDOWN | K_LCTRL | K_RCTRL)
#define K_CTRL_UP			(K_UP | K_LCTRL | K_RCTRL)
#define K_CTRL_DOWN			(K_DOWN | K_LCTRL | K_RCTRL)
#define K_CTRL_RIGHT		(K_RIGHT | K_LCTRL | K_RCTRL)
#define K_CTRL_LEFT			(K_LEFT | K_LCTRL | K_RCTRL)

#define K_ALT_DEL			(K_DEL | K_LALT | K_RALT)
#define K_ALT_INSERT		(K_INSERT | K_LALT | K_RALT)
#define K_ALT_HOME			(K_HOME | K_LALT | K_RALT)
#define K_ALT_END			(K_END | K_LALT | K_RALT)
#define K_ALT_PAGEUP		(K_PAGEUP | K_LALT | K_RALT)
#define K_ALT_PAGEDOWN		(K_PAGEDOWN | K_LALT | K_RALT)
#define K_ALT_UP			(K_UP | K_LALT | K_RALT)
#define K_ALT_DOWN			(K_DOWN | K_LALT | K_RALT)
#define K_ALT_RIGHT			(K_RIGHT | K_LALT | K_RALT)
#define K_ALT_LEFT			(K_LEFT | K_LALT | K_RALT)


#define K_F1				SDLK_F1
#define K_F2				SDLK_F2
#define K_F3				SDLK_F3
#define K_F4				SDLK_F4
#define K_F5				SDLK_F5
#define K_F6				SDLK_F6
#define K_F7				SDLK_F7
#define K_F8				SDLK_F8
#define K_F9				SDLK_F9
#define K_F10				SDLK_F10
#define K_F11				SDLK_F11
#define K_F12				SDLK_F12
#define K_F13				SDLK_F13
#define K_F14				SDLK_F14
#define K_F15				SDLK_F15

#define K_SHIFT_F1			(K_F1 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F2			(K_F2 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F3			(K_F3 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F4			(K_F4 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F5			(K_F5 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F6			(K_F6 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F7			(K_F7 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F8			(K_F8 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F9			(K_F9 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F10			(K_F10 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F11			(K_F11 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F12			(K_F12 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F13			(K_F13 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F14			(K_F14 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_F15			(K_F15 | K_LSHIFT | K_RSHIFT)

#define K_CTRL_F1			(K_F1 | K_LCTRL | K_RCTRL)
#define K_CTRL_F2			(K_F2 | K_LCTRL | K_RCTRL)
#define K_CTRL_F3			(K_F3 | K_LCTRL | K_RCTRL)
#define K_CTRL_F4			(K_F4 | K_LCTRL | K_RCTRL)
#define K_CTRL_F5			(K_F5 | K_LCTRL | K_RCTRL)
#define K_CTRL_F6			(K_F6 | K_LCTRL | K_RCTRL)
#define K_CTRL_F7			(K_F7 | K_LCTRL | K_RCTRL)
#define K_CTRL_F8			(K_F8 | K_LCTRL | K_RCTRL)
#define K_CTRL_F9			(K_F9 | K_LCTRL | K_RCTRL)
#define K_CTRL_F10			(K_F10 | K_LCTRL | K_RCTRL)
#define K_CTRL_F11			(K_F11 | K_LCTRL | K_RCTRL)
#define K_CTRL_F12			(K_F12 | K_LCTRL | K_RCTRL)
#define K_CTRL_F13			(K_F13 | K_LCTRL | K_RCTRL)
#define K_CTRL_F14			(K_F14 | K_LCTRL | K_RCTRL)
#define K_CTRL_F15			(K_F15 | K_LCTRL | K_RCTRL)

#define K_ALT_F1			(K_F1 | K_LALT | K_RALT)
#define K_ALT_F2			(K_F2 | K_LALT | K_RALT)
#define K_ALT_F3			(K_F3 | K_LALT | K_RALT)
#define K_ALT_F4			(K_F4 | K_LALT | K_RALT)
#define K_ALT_F5			(K_F5 | K_LALT | K_RALT)
#define K_ALT_F6			(K_F6 | K_LALT | K_RALT)
#define K_ALT_F7			(K_F7 | K_LALT | K_RALT)
#define K_ALT_F8			(K_F8 | K_LALT | K_RALT)
#define K_ALT_F9			(K_F9 | K_LALT | K_RALT)
#define K_ALT_F10			(K_F10 | K_LALT | K_RALT)
#define K_ALT_F11			(K_F11 | K_LALT | K_RALT)
#define K_ALT_F12			(K_F12 | K_LALT | K_RALT)
#define K_ALT_F13			(K_F13 | K_LALT | K_RALT)
#define K_ALT_F14			(K_F14 | K_LALT | K_RALT)
#define K_ALT_F15			(K_F15 | K_LALT | K_RALT)


#define K_NUMPAD_0			SDLK_KP0
#define K_NUMPAD_1			SDLK_KP1
#define K_NUMPAD_2			SDLK_KP2
#define K_NUMPAD_3			SDLK_KP3
#define K_NUMPAD_4			SDLK_KP4
#define K_NUMPAD_5			SDLK_KP5
#define K_NUMPAD_6			SDLK_KP6
#define K_NUMPAD_7			SDLK_KP7
#define K_NUMPAD_8			SDLK_KP8
#define K_NUMPAD_9			SDLK_KP9

#define K_SHIFT_NUMPAD_0	(K_NUMPAD_0 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_NUMPAD_1	(K_NUMPAD_1 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_NUMPAD_2	(K_NUMPAD_2 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_NUMPAD_3	(K_NUMPAD_3 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_NUMPAD_4	(K_NUMPAD_4 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_NUMPAD_5	(K_NUMPAD_5 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_NUMPAD_6	(K_NUMPAD_6 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_NUMPAD_7	(K_NUMPAD_7 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_NUMPAD_8	(K_NUMPAD_8 | K_LSHIFT | K_RSHIFT)
#define K_SHIFT_NUMPAD_9	(K_NUMPAD_9 | K_LSHIFT | K_RSHIFT)

#define K_CTRL_1			(SDLK_1 | K_LCTRL | K_RCTRL)
#define K_CTRL_2			(SDLK_2 | K_LCTRL | K_RCTRL)
#define K_CTRL_3			(SDLK_3 | K_LCTRL | K_RCTRL)
#define K_CTRL_4			(SDLK_4 | K_LCTRL | K_RCTRL)
#define K_CTRL_C			(SDLK_c | K_LCTRL | K_RCTRL)
#define K_CTRL_V			(SDLK_v | K_LCTRL | K_RCTRL)
#define K_CTRL_T			(SDLK_t | K_LCTRL | K_RCTRL)
#define K_CTRL_X			(SDLK_x | K_LCTRL | K_RCTRL)
#define K_CTRL_Y			(SDLK_y | K_LCTRL | K_RCTRL)

#define K_ALT_1				(SDLK_1 | K_LALT | K_RALT)
#define K_ALT_2				(SDLK_2 | K_LALT | K_RALT)
#define K_ALT_3		 		(SDLK_3 | K_LALT | K_RALT)
#define K_ALT_4				(SDLK_4 | K_LALT | K_RALT)
#define K_ALT_M				(SDLK_m | K_LALT | K_RALT)
#define K_ALT_V				(SDLK_v | K_LALT | K_RALT)
#define K_ALT_X				(SDLK_x | K_LALT | K_RALT)
#define K_ALT_Y 			(SDLK_y | K_LALT | K_RALT)



int keyRead();
void flushKeyboardBuffer();
void waitKeyPress();

#endif
