//---------------------------------------------------------------------------
//
// Name:        q_feedersel.h
// Author:      
// Created:     23/05/2012
// Description: Finestra selezione caricatore
//
//---------------------------------------------------------------------------

#include "q_feedersel.h"

#include "q_tabe.h"
#include "msglist.h"
#include "q_help.h"
#include "gui_defs.h"
#include "keyutils.h"

#include <mss.h>


extern struct CfgHeader QHeader;


//---------------------------------------------------------------------------------
// Costruttore classe FeederSelect
// Parametri di ingresso:
//   pack : se indicato, mostra solo i caricatori con tale package
//---------------------------------------------------------------------------------

//TODO:
//  1. aggiungere pan con tasti da usare per scorrere i caricatori
//  2. aggiungere costruttore con codice caricatore di partenza (senza selezione package)
//  2. possibilita' di modificare titolo finestra

FeederSelect::FeederSelect( CWindow* parent, char* pack )
{
	//apre file caricatore
	file = new FeederFile( QHeader.Conf_Default );
	
	if(!file->opened)           //check se aperto
		err=1;                    //file non aperto
	else
	{
		err=0;                    //file aperto
	
		if(pack!=NULL)            //se package indicato
		{
			strcpy(packtxt,pack);
			nrec=0;
			if(file->SearchPack(pack,car,0)!=CAR_SEARCHFAIL) //cerca il primo caricatore con package indicato
			{
				firstrec=GetCarRec(car.C_codice);              //codice del caricatore trovato
				nrec++;
			}
			if(nrec==0)         //se nessun caricatore ha il package indicato
			{
				bipbip();         //notifica errore
				W_Mess(NOCARPACK);
				err=1;
			}
			else
				//ricerca e conta tutti gli altri record con package indicato
				while(file->FindNextPack(car)!=CAR_SEARCHFAIL)
					nrec++;
		}
		else  //packahe non indicato
		{
			nrec=CAR_SEARCHFAIL;
			firstrec=0;          //primo caricatore=primo record nel file
		}


		Window = new CWindow( parent );
		Window->SetStyle( WIN_STYLE_CENTERED_X );
		Window->SetClientAreaPos( 0, 7 );
		Window->SetClientAreaSize( 60, 12 );
		Window->SetTitle( MsgGetString(Msg_00464) );

		ComboCar  = new C_Combo( 13, 1, MsgGetString(Msg_00975), 3, CELL_TYPE_UINT );
		ComboTipo = new C_Combo( 13, 2, MsgGetString(Msg_00981), 23, CELL_TYPE_TEXT );
		ComboPack = new C_Combo( 13, 3, MsgGetString(Msg_00980), 23, CELL_TYPE_TEXT );
		
		ComboSet = new CComboList( Window );

		ComboSet->Add( ComboCar , 0, 0 );
		ComboSet->Add( ComboTipo, 1, 0 );
		ComboSet->Add( ComboPack, 2, 0 );

		maxrec=MAXCAR;
		rec=0;
	}
}

//---------------------------------------------------------------------------------
// Disstruttore classe FeederSelect
//---------------------------------------------------------------------------------
FeederSelect::~FeederSelect()
{
	delete file;
	delete Window;
	delete ComboCar;
	delete ComboPack;
	delete ComboTipo;
	delete ComboSet;
}

//---------------------------------------------------------------------------------
// Mostra un record
// Parametri di ingresso:
//   rec    : record da mostrare
//---------------------------------------------------------------------------------
void FeederSelect::Read_Show(int rec)
{
	char buf[5];

	file->ReadRec(rec,car);  //legge record indicato

	//mostra record
	sprintf(buf,"%d",car.C_codice);

	ComboCar->SetTxt(buf);

	if(car.C_PackIndex!=0)
		ComboPack->SetTxt(car.C_Package);
	else
		ComboPack->SetTxt("");

	ComboTipo->SetTxt(car.C_comp);
}

//---------------------------------------------------------------------------------
// Gestore tasto PGUP
//---------------------------------------------------------------------------------
void FeederSelect::FSel_PGUP(void)
{ 
	int tmp;

	if(nrec==CAR_SEARCHFAIL) //se nessuna ricerca(package non indicato)
	{
		if(rec==0)
		{
			rec=maxrec-1;
		}
		else
		{
			rec--;                 //decrementa record attuale
		}
	}
	else
	{
		tmp=file->FindPrevPack(car); //cerca il record precedente con pkg specificato
	
		if(tmp==CAR_SEARCHFAIL)      //se nessun elemento trovato
		{
			bipbip();
			return;                    //esci
		}
		else                         //altrimenti
		{
			rec=tmp;                   //setta record attuale=record trovato
		}
	}
  	Read_Show(rec);                //mostra record attuale
}

//---------------------------------------------------------------------------------
// Gestore tasto PGDOWN
//---------------------------------------------------------------------------------
void FeederSelect::FSel_PGDN(void)
{
  int tmp;
	
	if(nrec==CAR_SEARCHFAIL) //se nessuna ricerca (package non indicato)
	{
		if(rec==(maxrec-1))  //se ultimo record su file
		{
			rec=0;
		}
		else
		{
			rec++;                 //incrementa posizione record attuale
		}
	}
	else
	{
		tmp=file->FindNextPack(car); //cerca il record successivo, con pkg specificato
		if(tmp==CAR_SEARCHFAIL)      //se nessun elemento trovato
		{ 
			bipbip();
			return;                    //esci
		}
		else
		{
			//altrimenti
			rec=tmp;                   //record attuale=record trovato
		}
	}
	
	Read_Show(rec);    //mostra record attuale
}

//---------------------------------------------------------------------------------
// Gestore tasto Ctrl-PGUP
//---------------------------------------------------------------------------------
void FeederSelect::FSel_CtrlPGUP(void)
{
  if(nrec==CAR_SEARCHFAIL) //se nessuna ricerca (package non indicato)
  {
    if(rec<FIRSTTRAY)
    {
      rec-=8;
    }
    else
    {
      rec--;                 //incrementa posizione record attuale
    }

    if(rec<0)
    {
      rec=(maxrec-1);
    }

    Read_Show(rec);    //mostra record attuale
  }
  else
   bipbip();
}

//---------------------------------------------------------------------------------
// Gestore tasto Ctrl-PGDOWN.
//---------------------------------------------------------------------------------
void FeederSelect::FSel_CtrlPGDN(void)
{
  if(nrec==CAR_SEARCHFAIL) //se nessuna ricerca (package non indicato)
  {
    if(rec<FIRSTTRAY)
    {
      rec+=8;
    }
    else
    {
      rec++;
    }

    if(rec>=maxrec) rec=0;

    Read_Show(rec);    //mostra record attuale
  }
  else
   bipbip();
}


//---------------------------------------------------------------------------------
// Attiva la visualizzazione
// Ritorna: 0  se operazione annullata
//          codice caricatore selezionato altrimenti
//---------------------------------------------------------------------------------
int FeederSelect::Activate(void)
{
	int c;

	if( err )      //se err: file non aperto o nessun record trovato
		return 0;  //esci con FAIL

	GUI_Freeze();

	Window->Show();
	ComboSet->Show();

	Window->DrawPanel( RectI( 2, 5, Window->GetW()/GUI_CharW() - 4, 6 ) );
	Window->DrawText( 5, 6, MsgGetString(Msg_00813), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Window->DrawText( 5, 7, MsgGetString(Msg_00816), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Window->DrawText( 5, 8, MsgGetString(Msg_00814), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Window->DrawText( 5, 9, MsgGetString(Msg_00815), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );

	GUI_Thaw();

	if(nrec!=CAR_SEARCHFAIL) //se ricerca per package attivata
	{
		file->SearchPack(packtxt,car,0); //cerca prima occorrenza del package
		rec=GetCarRec(car.C_codice);
	}

	Read_Show(firstrec);  //mostra primo record

	do
	{
		c=Handle();
		switch(c)
		{
		case K_PAGEUP:
			FSel_PGUP();
			break;
		case K_PAGEDOWN:
			FSel_PGDN();
			break;
		case K_CTRL_PAGEUP:
			FSel_CtrlPGUP();
			break;
		case K_CTRL_PAGEDOWN:
			FSel_CtrlPGDN();
			break;
		}
	} while (c!=K_ESC && c!=K_ENTER);
	
	if(c==K_ESC)
		return(0);    //exit
	else
		return(GetCarCode(rec)); //premuto ENTER: ritorna codice caricatore selezionato
}
