/*
>>> Q_WIND.H

Include di q_wind.cpp. Dichiarazione delle classi per la costruzione
di finestre.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++
++++  Modificato LCM Simone >> S180701


#18/07/2001 (Simone) Coordinate X,Y spostate dalla sezione protected a
quella public.
#18/07/2001 (Simone) Corretto baco in destructor. WVBuffer viene deallocato
da Hide ed il destructor chiama Hide.

*/
#if !defined (Q_WIND)
#define Q_WIND

#include <string>

#include "c_window.h"
#include "gui_buttons.h"


#define MSGBOX_DELAYED  1
#define MSGBOX_ANYKEY   2
#define MSGBOX_GESTKEY  3
#define MSGBOX_CENTER   5

#define MSGBOX_YCENT    0
#define MSGBOX_YLOW     1
#define MSGBOX_YUPPER   2

class W_MsgBox
{
protected:
	CWindow *wHdl;
	int notitle;
	int nb;
	int mode;
	int time_delay;
	std::string title;
	std::string text;
	int txt_width;
	int idx_counter;
	GUIButtons* ButtonSet;
	void(*loopFunction)(void);
	void(*auxKeyHandlerFunction)(int&);
public:
	W_MsgBox(const char *_title,const char *_text,int _nb=0,int _mode=MSGBOX_ANYKEY,int _delay=1,int _pos=MSGBOX_YCENT,int _alarm=NO_ALARM);
	~W_MsgBox(void);
	CWindow *GetWindowHandler(void);
	void AddButton(const char *txt,int focus=0);
	void LoopFunction(void(*f)(void));
	void AuxKeyHandlerFunction(void(*f)(int&));
	int Activate(void);
	void Reactivate(void);
	void Suspend(void);
};


#define WAITTEXT     MsgGetString(Msg_00291)
#define WAITELAB     MsgGetString(Msg_00292)

#endif

