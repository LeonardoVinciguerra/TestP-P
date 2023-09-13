/*
>>>> Q_ZERIT

Include per Q_ZERI.CPP.
Definizioni dei testi esclusivi per gestione tabella ZERI SCH.


ATTENZIONE: In caso di modifiche ai testi,
		   rispettare la lunghezza attuale delle stringhe !!!

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++  	Modificato da TWS Simone 06.08.96

*/

#if !defined(__Q_ZERIT_)
#define __Q_ZERIT_


// Max. n. di record in display
#define	MAXRZER	8
/*
// Titoli campi fissi in tabella
#define	ZERTITFISSI		MsgGetString(Msg_00489)
#define  ZERTITFISSI_1  MsgGetString(Msg_00534)
#define  ZERTITFISSI_2  MsgGetString(Msg_00535)
#define  ZERTITFISSI_3  MsgGetString(Msg_00536)
#define  ZERTITFISSI_4  MsgGetString(Msg_00537)
#define  ZERTITFISSI_5  MsgGetString(Msg_00538)
#define  ZERTITFISSI_6  MsgGetString(Msg_00539)

#define	ZERTITFISSI1	MsgGetString(Msg_00060)
*/


// Messaggio per nuovo autoapprend. master
#define	RIMASTER			MsgGetString(Msg_00054)


// Testi
#define	NEWP1				MsgGetString(Msg_00056)
#define	NEWP2				MsgGetString(Msg_00057)

// Codice help
#define	NEWHELP   7020

// Msg. per zero/riferim. uguali in autoappr. scheda
#define	COINCID			MsgGetString(Msg_00058)

// Distanza minima x/y per autoapprendimento zero/riferimento
#define	MINXAPP	10
#define	MINYAPP	10
// Messaggio per autoappr. zero/riferim. inferiore a distanza minima
#define TOONEAR			MsgGetString(Msg_00059)

// Messaggio memorizzazione immagine di confronto
#define	MEMIMAGEQ	MsgGetString(Msg_00729)	// VISIONE MASSIMONE 991015


// Posizione
#define	MENUZERPOS 1,2

#define ZREF_SEARCHMODE_SWITCHBRD1 MsgGetString(Msg_00847)
#define ZREF_SEARCHMODE_SWITCHBRD2 MsgGetString(Msg_01739)
#define ZREF_SEARCHMODE_ASKPANEL   MsgGetString(Msg_00727)

#define ASKNEW_MULTIPLE MsgGetString(Msg_01254)

#define NOREFAPP_MSG  MsgGetString(Msg_01320)

#define ZSCHMASTER_APP_DELTA      MsgGetString(Msg_01605) //SMOD100504
#define ZSCHMASTER_APP_POINTNEAR  MsgGetString(Msg_01606) //SMOD100504
#define ZSCHMASTER_APP_NOALLIGNED MsgGetString(Msg_01607) //SMOD100504

#define ZMULTI_APP_DELTA          MsgGetString(Msg_02101)

#endif
