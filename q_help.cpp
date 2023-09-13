/*
>>>> Q_HELP.CPP

Gestione di:   Help on-line.
			Messaggi operatore.
			Scelta logica.
			Messaggio di attesa.
			Pannelli info.
			Altri pannelli.

			  *******
Attenzione! Il file di help deve rispettare la formattazione :

#nnnnnn#\nTesto di help relativo al codice...

dove:nnnnnn   e' il numero di codice dell'help
	 \n       opzionale, forza il new line
			  *******

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++  	Modificato da TWS Simone 06.08.96
++++    Modificato da WALTER 16.11.96

*/
#include <iostream>
#include <fstream>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "q_wind.h"
#include <ctype.h>

#include "msglist.h"
#include "q_cost.h"
#include "q_graph.h"
#include "q_gener.h"
#include "q_help.h"

#include "q_tabe.h"
#include "q_oper.h"

#include "tv.h"

#include "keyutils.h"
#include "strutils.h"
#include "lnxdefs.h"
#include "q_oper.h"

#include "c_pan.h"

#include <mss.h>
#include "q_inifile.h"
#include "working_log.h"

using namespace std;

extern CWorkingLog workingLog;

//---------------------------------------------------------------------------
// Display e gestione di finestra con decisione Si/No/tutti, tipo BASSO (Low).
// Ritorna YES/NO/ALL
//---------------------------------------------------------------------------
int W_DeciYNA( int Default, const char* Messaggio, int pos, void(*f1)(void), void(*f2)(int&), int Alarm )
{
	int cam = pauseLiveVideo();

	W_MsgBox* box = new W_MsgBox(MsgGetString(Msg_00289),Messaggio,3,MSGBOX_GESTKEY,0,pos,Alarm);

	box->AddButton(FAST_YES,(Default==YES));
	box->AddButton(FAST_NO,(Default==NO));
	box->AddButton(FAST_ALL,(Default==ALL));

	box->LoopFunction(f1);
	box->AuxKeyHandlerFunction(f2);

	int comando = box->Activate();

	delete box;

	playLiveVideo( cam );

	return comando == 0 ? NO : comando;
}

//---------------------------------------------------------------------------
// Display e gestione di finestra con decisione Si/No/Exit
// Ritorna YES/NO/ALL
//---------------------------------------------------------------------------
int W_DeciYNE( int Default, const char* Messaggio, int pos, void(*f1)(void), void(*f2)(int&), int Alarm )
{
	int cam = pauseLiveVideo();

	W_MsgBox* box = new W_MsgBox(MsgGetString(Msg_00289),Messaggio,3,MSGBOX_GESTKEY,0,pos,Alarm);

	box->AddButton(FAST_YES,(Default==YES));
	box->AddButton(FAST_NO,(Default==NO));
	box->AddButton(FAST_EXIT,(Default==EXIT));

	box->LoopFunction(f1);
	box->AuxKeyHandlerFunction(f2);

	int comando = box->Activate();

	delete box;

	playLiveVideo( cam );

	return comando == 0 ? NO : comando;
}


//---------------------------------------------------------------------------
// Display e gestione di finestra con decisione Si/No
// Default: 0 (NO), altro (SI)
// Ritorna 1 per Si, 0 per il resto.
//---------------------------------------------------------------------------
int W_Deci( int Default, const char* Messaggio, int pos, void(*f1)(void), void(*f2)(int&), int Alarm )
{
	int cam = pauseLiveVideo();

	W_MsgBox *box=new W_MsgBox(MsgGetString(Msg_00289),Messaggio,2,MSGBOX_GESTKEY,0,pos,Alarm);
	box->AddButton( MsgGetString(Msg_00206), Default );
	box->AddButton( MsgGetString(Msg_00207), !Default );

	box->LoopFunction(f1);
	box->AuxKeyHandlerFunction(f2);
	int comando = box->Activate();
	delete box;

	playLiveVideo( cam );

	return comando == 1 ? 1 : 0;
}


//---------------------------------------------------------------------------
// Finestra con decisione Man/Auto
// Default: 0 (Manual), altro (Automatic)
// Ritorna 1 per Auto, 0 per Man.
//---------------------------------------------------------------------------
int W_DeciManAuto( const char* text, int pos, int def_btn, int Alarm )
{
	int cam = pauseLiveVideo();

	W_MsgBox* box = new W_MsgBox( MsgGetString(Msg_00289), text, 2, MSGBOX_GESTKEY, 0, pos, Alarm );

	std::string man_str = MsgGetString(Msg_00730);
	man_str.append( " [ESC]" );

	box->AddButton( man_str.c_str(), !def_btn );
	box->AddButton( MsgGetString(Msg_00731), def_btn );

	int comando = box->Activate();

	delete box;

	playLiveVideo( cam );

	return comando == 2 ? 1 : 0;
}


/*--------------------------------------------------------------------------
Display di una finestra di messaggio all'operatore
--------------------------------------------------------------------------*/
void W_Mess( const char* text, int pos, const char* title, int Alarm )
{
	char* buf;

	if( Get_WorkingLog() )
	{
		if(Alarm==GENERIC_ALARM)
		{
			workingLog.LogStatus( STATUS_ERROR );
		}
	}

	if( title != NULL )
	{
		buf = new char[strlen(MsgGetString(Msg_00289))+strlen(title)+2];
		strcpy( buf, MsgGetString(Msg_00289) );
		strcat( buf, ": " );
		strcat( buf, title );
	}
	else
	{
		buf = new char[strlen(MsgGetString(Msg_00289))+1];
		strcpy( buf, MsgGetString(Msg_00289) );
	}
	
	int cam = pauseLiveVideo();

	W_MsgBox* Q_Mess = new W_MsgBox(buf,text,0,MSGBOX_ANYKEY,0,pos,Alarm);
	Q_Mess->Activate();
	
	delete [] buf;
	delete Q_Mess;

	workingLog.LogStatus( STATUS_WORKING );

	playLiveVideo( cam );
}



// Display di pannello info per test in sequenza/attesa generica fine test.
// ( chiamata in Q_TEST2/Q_OPER ).
// Tipo: 0 = test sequenza (default) / 1 = attesa fine test.
void Pan_WaitTest(int Stato, int Tipo)
{ static CPan *panwaittest;

  if(!Stato)
  { if(panwaittest!=NULL)
      delete panwaittest;
    panwaittest=NULL;
    return;
  }

  if(panwaittest==NULL)
  {
    if(!Tipo)
    {
      panwaittest=new CPan( 22,2,MsgGetString(Msg_00338),MsgGetString(Msg_00339));
    }
    else
    {
      panwaittest=new CPan( 22,2,MsgGetString(Msg_00338),MsgGetString(Msg_00291));
    }
  }
}
