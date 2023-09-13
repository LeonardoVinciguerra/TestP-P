/*
>>>> Q_HELP.H

Dichiarazione delle classi e funzioni di gestione dell'help.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++    Modificato da WALTER 16.11.96

*/

#if !defined(Q_HELP)
#define Q_HELP

#include "q_wind.h"

// Dichiarazione delle funzioni di help

// Funzione messaggio operatore. L704b
extern void W_Mess( const char* text, int pos = MSGBOX_YCENT, const char* title = 0,int Alarm=NO_ALARM );
// Funzioni per richiesta di decisione si/no/all
extern int W_DeciYNA(int Default, const char* text,int pos=MSGBOX_YCENT,void(*f1)(void)=NULL,void(*f2)(int&)=NULL,int Alarm=NO_ALARM);
// Funzioni per richiesta di decisione si/no
extern int W_Deci(int Default, const char* text,int pos=MSGBOX_YCENT,void(*f1)(void)=NULL,void(*f2)(int&)=NULL,int Alarm=NO_ALARM);
// Funzioni per richiesta di decisione si/no/exit
extern int W_DeciYNE(int Default, const char* text,int pos=MSGBOX_YCENT,void(*f1)(void)=NULL,void(*f2)(int&)=NULL,int Alarm=NO_ALARM);

int W_DeciManAuto( const char* text, int pos = MSGBOX_YCENT, int def_btn = 1,int Alarm=NO_ALARM );

// Display di pannello info per test in sequenza/attesa fine test gener.
void Pan_WaitTest(int Stato, int Tipo=0);

#endif
