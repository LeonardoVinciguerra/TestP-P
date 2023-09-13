/*
>>>> Q_WIND.CPP

Definizione delle classi e membri per la creazione di finestre in modo
testo, con ombreggiatura opzionale e ripristino del video ricoperto.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

*/
#include "q_wind.h"

#include <stdarg.h>
#include "q_graph.h"
#include "keyutils.h"
#include "lnxdefs.h"
#include "strutils.h"
#include "q_oper.h"
#include "gui_functions.h"

#include <mss.h>


#define MSGBOXBORDER  2
#define MSGBOXMAXW   90
#define MSGBOXMINW   46
#define MSGBOXROWW   100

/*--------------------------------------------------------------------------
Costruttore MessageBox
INPUT:	_title  : titolo della finestra
         _text   : testo da stampare nella finestra.
         _nb     : numero di bottoni
                   MSGBOX_DELAYED  (attende _delay secondi ed esce)
                   MSGBOX_ANYKEY   (attende la pressione di un qualsiasi tasto ed esce)
                   MSGBOX_GESTKEY  (gestise i tasti ed esce su ESC o CR) (default)
         _delay  : ritardo in secondi usato se _mode=MSGBOX_DELAYED (default=1s)

NOTE:	 In text sono ammessi caratteri '\n' per formattare il testo.
         Se n. bottoni=0 non e' ammesso mode=MSGBOX_GESTKEY: in questo caso
         viene usato MSGBOX_ANYKEY.
--------------------------------------------------------------------------*/
W_MsgBox::W_MsgBox(const char *_title,const char *_text,int _nb,int _mode,int _delay,int _pos,int _alarm)
{
	using namespace std;

	int pos=0;
	int txt_rows,count=0;
	int w_sx,w_dx,py;

	notitle=0;

	if(strlen(_title)==0)
	{
		notitle=1;
	}

	if(strlen(_text)==0)
	{
		txt_rows=0;
	}
	else
	{
		txt_rows=1;
	}

	title = _title;
	text = _text;

	int lastSpc=-1;
	int line_centered = -1;
	int max_row_width = 0;

	while(text[pos] != '\0')
	{
		if((count == 0) && (line_centered == -1))
		{
			line_centered = 0;
		}

		if((text[pos]=='\n') || (count==MSGBOXMAXW))
		{		
			if((count==MSGBOXMAXW) && (lastSpc!=-1))
			{				
				text[lastSpc] = '\n';
				pos = lastSpc + 1;
			}
			else
			{
				if(text[pos] == '\n')
				{
					line_centered = -1;
				}
				
				pos++;
			}
		
			lastSpc=-1;
			
			txt_rows++;
			
			if(max_row_width < count)
			{
				max_row_width = count;
			}
		
			count=0;
			continue;
		}

		if(text[pos] == ' ')
		{
			lastSpc = pos;
		}

		if( text[pos] != '\n' )
		{
			count++;
		}
		pos++;
	}

	if(max_row_width < count)
	{
		max_row_width = count;
	}	
	
	if(max_row_width <= MSGBOXMAXW)
	{
		txt_width = max_row_width;
	}
	else
	{
		txt_width = MSGBOXMAXW;
	}
	
	nb=_nb;
	w_sx=(90-txt_width)/2-MSGBOXBORDER;
	w_dx=(90+txt_width)/2+MSGBOXBORDER;
	
	if((w_dx-w_sx)<MSGBOXMINW)
	{
		w_sx=(90-MSGBOXMINW)/2;
		w_dx=(90+MSGBOXMINW)/2;
	}
    

	mode=_mode;
	time_delay=_delay;
	
	switch(_pos)
	{
		case MSGBOX_YUPPER:
			py=3;
			break;
		case MSGBOX_YLOW:
			if(notitle)
			{
				if(nb==0)
				{
					py=22-txt_rows;
				}
				else
				{
					py=19-txt_rows;
				}
			}
			else
			{
				if(nb==0)
				{
					py=21-txt_rows;
				}
				else
				{
					py=18-txt_rows;
				}
			}	
			break;
		case MSGBOX_YCENT:
		default:
			py=11;
			break;
	}


	if(nb==0)
	{
		if(notitle)
		{
			wHdl=new CWindow(w_sx,py,w_dx,py+1+txt_rows,title.c_str(),_alarm);
	//      print_debug("NOTITLE py start=%d, py end=%d, rows=%d\n",py,py+1+txt_rows,txt_rows);
		}
		else
		{
			wHdl=new CWindow(w_sx,py,w_dx,py+2+txt_rows,title.c_str(),_alarm);
	//      print_debug("TITLE   py start=%d, py end=%d, rows=%d\n",py,py+2+txt_rows,txt_rows);
		}
		
		ButtonSet=NULL;
	}
	else
	{
		if(notitle)
		{
			wHdl=new CWindow(w_sx,py,w_dx,py+4+txt_rows,title.c_str(),_alarm);
	//      print_debug("NOTITLE py start=%d, py end=%d, rows=%d\n",py,py+1+txt_rows,txt_rows);
		}
		else
		{
			wHdl=new CWindow(w_sx,py,w_dx,py+5+txt_rows,title.c_str(),_alarm);
	//      print_debug("TITLE   py start=%d, py end=%d, rows=%d\n",py,py+2+txt_rows,txt_rows);
		}
	
		ButtonSet = new GUIButtons( txt_rows+3, wHdl );
		idx_counter=0;
	}

	if(mode==MSGBOX_GESTKEY && nb==0)
	{
		mode=MSGBOX_ANYKEY;
	}
	
	loopFunction=NULL;
	auxKeyHandlerFunction=NULL;
}


/*--------------------------------------------------------------------------
Distruttore classe MsgBox
INPUT:   -
GLOBAL:	-
RETURN:	-
NOTE:    -
--------------------------------------------------------------------------*/
W_MsgBox::~W_MsgBox(void)
{
	delete wHdl;
	if(ButtonSet!=NULL)
	{
		delete ButtonSet;
	}
}


/*--------------------------------------------------------------------------
Ritorna puntatore al gestore finestra MsgBox
INPUT:   -
GLOBAL:	-
RETURN:	Handler della finestra
NOTE:    -
-------------------------------------------------------------------------*/
CWindow *W_MsgBox::GetWindowHandler(void)
{
	return(wHdl);
}

/*--------------------------------------------------------------------------
Aggiunge bottone
INPUT:   txt  = testo del bottone
         focus= focus SI/NO
GLOBAL:	-
RETURN:	-
NOTE:    -
--------------------------------------------------------------------------*/
void W_MsgBox::AddButton( const char* txt, int focus )
{
	if((ButtonSet==NULL) || (idx_counter==nb))
		return;
	
	ButtonSet->AddButton( txt, focus );
	idx_counter++;
}

/*--------------------------------------------------------------------------
Setta la funzione da eseguire all'interno del loop di attesa.
INPUT:   f: puntatatore alla funzione da eseguire. NULL se nessuna funzione.
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void W_MsgBox::LoopFunction(void(*f)(void))
{
	loopFunction=f;
}

/*--------------------------------------------------------------------------
Setta la funzione di gestione tasti ausiliaria
INPUT:   f: puntatatore alla funzione da eseguire. NULL se nessuna funzione.
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void W_MsgBox::AuxKeyHandlerFunction(void(*f)(int&))
{
	auxKeyHandlerFunction=f;
}

/*--------------------------------------------------------------------------
Attiva la finestra MessageBox
INPUT:   -
GLOBAL:	-
RETURN:  se mode=GESTKEY
         0 se premuto ESC,
         codice bottone se premuto ENTER.
         se mode!=GESTKEY ritorna sempre 0.
NOTE:    -
--------------------------------------------------------------------------*/
int W_MsgBox::Activate(void)
{ 
	char row[MSGBOXROWW];
	int c,fine=0,b_index=0;
	int nrow=0,count,nwrite;
	const char *ptr_start = text.c_str();
	const char *ptr_endl = NULL;
	
	GUI_Freeze();

	wHdl->Show();

	while(ptr_start!=NULL)
	{
		ptr_endl=strchr(ptr_start,'\n');
		
		if(ptr_endl!=NULL)
		{
			count=ptr_endl-ptr_start;
		}
		else
		{			
			count=strlen(ptr_start);
		}
	
		while(count!=0)
		{
			nwrite=count;
			if(nwrite>txt_width)
			{
				nwrite=txt_width;
			}
	
			strncpyQ(row,ptr_start,nwrite);
			row[nwrite]=0;

			wHdl->DrawTextCentered(nrow+1,row);
			
			nrow++;
			ptr_start+=nwrite;
			count-=nwrite;      
		}
		
		if(ptr_endl!=NULL)
		{
			ptr_start=ptr_endl+1;
		}		
		else
		{
			ptr_start=NULL;
		}
	}

	if(ButtonSet!=NULL)
	{
		ButtonSet->Activate();
	}

	GUI_Thaw();

	//TODO controllare se serve da altre parti
	GUI_Update_Force force;

	if(mode==MSGBOX_DELAYED)
	{
		delay(1000*time_delay);
	}
	else
	{
		int keypressed=0;
		
		do
		{
		
			SetConfirmRequiredBeforeNextXYMovement(true);
			
			if(loopFunction!=NULL)
			{
				loopFunction();
			}

			c = Handle( false );
			if( c != 0 )
			{
				keypressed = 1;
			}

			if(keypressed && auxKeyHandlerFunction!=NULL)
			{
				auxKeyHandlerFunction(c);
			}
		
			if(mode==MSGBOX_GESTKEY)
			{
				if(keypressed)
				{
					keypressed=0;
					b_index=ButtonSet->GestKey(c);
			
					if(((c==K_ENTER) || (c==K_ESC)) || (b_index & BUTTONCONFIRMED))
					{
						fine=1;
						if(c==K_ESC)
						{
							b_index=0;
						}
					}
				}
			}
			else
			{
				fine=keypressed;
			}
		} while(!fine);
	}

	return(b_index & ~BUTTONCONFIRMED);
}

void W_MsgBox::Reactivate(void)
{
	if(wHdl)
	{
		wHdl->Show();
	}
}

void W_MsgBox::Suspend(void)
{
	if(wHdl)
	{
		wHdl->Hide();
	}
}
