/*
>>>> Q_PROGT.H

File include di Q_PROG.CPP.

Contiene le definizioni dei parametri e del testo solo per il modulo
di gestione della tabella di edit del programma di montaggio.

Per la definizione dei colori di sfondo, testo, titolo e cornice,
e posizione della finestra di edit vedere Q_COST.H.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++  	Modificato da TWS Simone 06.08.96

*/

// Attenzione! Rispettare spaziatura e lunghezza delle stringhe.

#if !defined Q_PROGT
#define Q_PROGT

#include "q_cost.h"


#define ZEROMACHFILE      "zeromach"
#define SETSCALEFILE      "setscale"
#define MAPPING_FILE      "mapping"
#define SETSCALEAUXFILE   "auxscale"
#define UGEREFFILE        "ugeref"
#define EXCAMNOZ_FILE     "excamnoz"
#define CARCOMP_FILE      "cccp"
#define RECTCOMP_FILE     "rectcomp"
#define INKMARK_FILE      "inkmark"


#define  DOSA_ALL_TXT   MsgGetString(Msg_05173)


// N. massimo di records visualizzabili in una pagina
#define		MAXRECS		30


// Avviso per configurazione non caricata/disponibile
#define	NOCONF			MsgGetString(Msg_00185)

// Avviso : nessun record in tabella programma, no autoappr.
#define	NORECS			MsgGetString(Msg_00186)


#define MENUCHKORI1  MsgGetString(Msg_00356), K_SHIFT_F2
#define MENUCHKORI2  MsgGetString(Msg_00357), K_SHIFT_F3
#define MENUCHKORI3  MsgGetString(Msg_00355), K_SHIFT_F4

#define MENUZTHETA1 MsgGetString(Msg_00042), K_F3
#define MENUZTHETA2 MsgGetString(Msg_00043), K_F4
#define MENUZPOS1   MsgGetString(Msg_00042), K_F3
#define MENUZPOS2   MsgGetString(Msg_00043), K_F4



// **** Definizione sub-menu zerischeda manuale automatico - VISIONE MASSIMONE 991015 ****
// Posizione Integr. Loris
// Voci
#define MENUAUT1   MsgGetString(Msg_00730), K_F3
#define MENUAUT2   MsgGetString(Msg_00731), K_F4
#define MENUAUT3   MsgGetString(Msg_00743), K_F5
#define MENUAUT4   MsgGetString(Msg_00744), K_F6

// **** Definizione menu assemblaggio ****

// Posizione
#define MENUASSEPOS 42,8
// Voci
#define MENUASSE2   MsgGetString(Msg_00386), K_F8
#define MENUASSE4   MsgGetString(Msg_00388), K_F10
#define MENUASSE5   MsgGetString(Msg_00389), K_F11
#define MENUASSE6   MsgGetString(Msg_00390), K_F12

// **** Definizione menu riordino righe programma ****

// Posizione
#define MENURIOPOS_ASSEM  42,9
#define MENURIOPOS_MASTER 42,9

#define MENUSUBRIOPOS_NORM		15,16
#define MENUSUBRIONORM_1		MsgGetString(Msg_05100), K_F11
#define MENUSUBRIONORM_2		MsgGetString(Msg_05101), K_F1
#define MENUSUBRIONORM_3		MsgGetString(Msg_05102), K_F2


// Voci
#define MENURIO0    MsgGetString(Msg_02117), K_F4
#define MENURIO1    MsgGetString(Msg_00391), K_F5
#define MENURIO2    MsgGetString(Msg_00392), K_F6
#define MENURIO3    MsgGetString(Msg_00393), K_F7
#define MENURIO4    MsgGetString(Msg_00394), K_F8
#define MENURIO6    MsgGetString(Msg_01039), K_F9
#define MENURIO7    MsgGetString(Msg_01073), K_F10
#define MENURIO5    MsgGetString(Msg_00887), K_F11
#define MENURIO8    MsgGetString(Msg_00395), K_F12

// **** Definizione submenu operazioni in tabella

// Posizione
#define MENUMOUNTPOS 64,13
// Voci
#define MENUMOUNT1 MsgGetString(Msg_00180), K_F9
#define MENUMOUNT2 MsgGetString(Msg_00181), K_F10

// Posizione
#define MENUFIDPOS 64,11
// Voci
#define MENUFID1 MsgGetString(Msg_05177), K_F3
#define MENUFID2 MsgGetString(Msg_05178), K_F4
#define MENUFID3 MsgGetString(Msg_05179), K_F5

// Posizione
#define MENUSPLITPOS 64,15
// Voci
#define MENUSPLIT1 MsgGetString(Msg_05183), K_F3
#define MENUSPLIT2 MsgGetString(Msg_05184), K_F4

// Posizione
#define MENUCOORDPOS 64,12
// Voci
#define MENUCOORD1 MsgGetString(Msg_05190), K_F3
#define MENUCOORD2 MsgGetString(Msg_05191), K_F4

// Posizione
#define MENUDISPPOS 64,14
// Voci
#ifndef __DISP2
#define MENUDISP1 MsgGetString(Msg_00180), K_F9
#define MENUDISP2 MsgGetString(Msg_00181), K_F10
#else
#define MENUDISP1 MsgGetString(Msg_05171), K_F9
#define MENUDISP2 MsgGetString(Msg_05172), K_F10
#define MENUDISP3 MsgGetString(Msg_05173), K_F11
#define MENUDISP4 MsgGetString(Msg_00181), K_F12
#endif


// **** Definizione sub-menu dosaggio & assemblaggio - W3107 ****

// Posizione
#ifndef __DISP2
#define MENUDASPOS 42,17
#else
#define MENUDASPOS 42,20
#define MENUDIST_SELN1 MsgGetString(Msg_05113),999 //TOCHECK
#define MENUDIST_SELN2 MsgGetString(Msg_05114),999 //TOCHECK

#define MENUDASPOS1       10,17
#define MENUDASPOS2       10,18
#define MENUDASPOS_D1D2AS 10,19
#define MENUDASPOS_D1ASD2 10,19
#endif

// Voci
#define MENUDAS1   MsgGetString(Msg_00386), K_F8
#define MENUDAS2   MsgGetString(Msg_00388), K_F10
#define MENUDAS3   MsgGetString(Msg_00389), K_F11
#define MENUDAS4   MsgGetString(Msg_00390), K_F12

// **** Definizione menu dosaggio - W0298 ****

// Posizione
#ifndef __DISP2
#define MENUDISTPOS 42,17
#else
#define MENUDISTPOS 42,10
#define MENUDAS_D1ASS    MsgGetString(Msg_05125),999 //TOCHECK
#define MENUDAS_D2ASS    MsgGetString(Msg_05126),999 //TOCHECK
#define MENUDAS_D1D2ASS  MsgGetString(Msg_05131),999 //TOCHECK
#define MENUDAS_D1ASSD2  MsgGetString(Msg_05127),999 //TOCHECK

#define MENUDISTPOS1 10,10
#define MENUDISTPOS2 10,11
#endif

// Voci
#define MENUDIST1   MsgGetString(Msg_00386), K_F8
#define MENUDIST2   MsgGetString(Msg_00388), K_F10
#define MENUDIST3   MsgGetString(Msg_00389), K_F11
#define MENUDIST4   MsgGetString(Msg_00390), K_F12

#define ERRDUPCOMP  MsgGetString(Msg_01789)

#define SETPCBHPOS   20,5,70,9
#define SETPCBHTIT   MsgGetString(Msg_00990)
#define SETPCBHTIT1  MsgGetString(Msg_00991)


//costanti funzione PrgM_CreateNewFromX
#define SPLIT_INF    		0
#define SPLIT_SUP    		1

//costanti funzione Prg_Sort
#define SORT_TIPOCOM    0
#define SORT_CODCOM     1
#define SORT_CARIC      2
#define SORT_PUNTA      3
#define SORT_XMON       4
#define SORT_PUNTAUGE   5
#define SORT_SPARE      6
#define SORT_NUMSCHEDA  7
#define SORT_NOTE       8


//costanti funzione autoapprendimento componente
#define AUTO_COMP           0 //autoapprendimento componente
#define AUTO_IC             1 //autoapprendimento I.C.
#define AUTO_COMPREC        2 //autoapprendimento normale con passaggio del numero di record
#define AUTO_ICREC          3 //autoapprendimento I.C. con passaggio del numero di record
#define AUTO_IC_MAPOFFSET   8

//impossibile creare progr. espanso
#define ERRCREATE_ASSF MsgGetString(Msg_00463)
//codice componente duplicato
#define ERRDUPCODE     MsgGetString(Msg_01085)
//messaggio zeri modificati
#define ZERCHANGED     MsgGetString(Msg_00258)

//warning creazione assembly file con assemblaggio in corso
#define WARN_ASSEMBLING1  MsgGetString(Msg_01137)
//messaggio richiesta conferma update riga in programma di assemblaggio
#define ASK_UPDATEASSF    MsgGetString(Msg_01138)

//Check  punta/tipo centraggio fallito
#define  ERRCENTR_NOZ   MsgGetString(Msg_01715)
// check prog./package fallito (ask corregi)
#define  ERR_PACKCHECK  MsgGetString(Msg_00515)
//errore altezza vassoio nulla
#define  ERR_TRAYZEROHEIGHT MsgGetString(Msg_00714)
//errore numero componenti tra master ed assemblaggio - DANY161302
#define  ERR_NUMCOMP     MsgGetString(Msg_01170)
//errore incongruenza tra tabella master e di assemblaggio
#define  ERR_MOUNT_MASTER   MsgGetString(Msg_02090)
//errore componente mancante tra tabella master e di assmblaggio
#define  ERR_COMP_MISSING   MsgGetString(Msg_02091)

#define COMPCHECK_ASK_PACKAGE_CHANGE MsgGetString(Msg_05103)


//DANY271102
#define INVALID_CODE    MsgGetString(Msg_01151)

//DANY131202
#define INVALID_DUP     MsgGetString(Msg_01168)

#define FINDCOMP_ERR  MsgGetString(Msg_01162)

#define SELECTVERS_NOFOUND MsgGetString(Msg_02033)
#define SELECTVERS_DONE    MsgGetString(Msg_02034)
#define SELECTVERS_NOVERS  MsgGetString(Msg_02095)

#define SELECTVERS_SPECIALCHAR            '\\'

#define MSG_REDEFZERI MsgGetString(Msg_01173)


#define BOARDS_TXT   MsgGetString(Msg_01234)
#define COMP_TXT     MsgGetString(Msg_01235)

#define NO_DASRESTART MsgGetString(Msg_00560)
#define DAS_END       MsgGetString(Msg_00618)

//soglia di intervento ottimizzazione automatica (mm)
#define SOGLIA_AUTOOPT 3.0

#define ERR_NOMOUNTEDCAR   MsgGetString(Msg_01825)

#define CREATEASS_REFRESH   1
#define CREATEASS_NOREFRESH 0

#define DUPROW_ALREADYPRESENT MsgGetString(Msg_01469)
#define DUPROW_TITLE          MsgGetString(Msg_01468)
#define DUPROW_TXT            MsgGetString(Msg_01467)
#define DUPROW_POS            15,12,65,15

#define RIP_DOSA_TITLE        MsgGetString(Msg_01471)
#define RIP_ASS_TITLE         MsgGetString(Msg_01470)
#define RIP_DOSAASS_TITLE     MsgGetString(Msg_01472)

#define ERRNORECS             MsgGetString(Msg_01473)

#define ERRCHECK_UGEPACK      MsgGetString(Msg_01479) //SMOD090703-CHECK PREOPT

#define CARIC_HISTOGRAMPOS  5,6,76,25
#define CARIC_HISTOGRAMTIT  MsgGetString(Msg_01539)
#define CARIC_HISTOGRAM_TXT1 MsgGetString(Msg_01535)
#define CARIC_HISTOGRAM_TXT2 MsgGetString(Msg_01536)
#define CARIC_HISTOGRAM_TXT3 MsgGetString(Msg_01537)

#define MAG_HISTOGRAMTIT     MsgGetString(Msg_01552)
#define MAG_HISTOGRAM_TXT1   MsgGetString(Msg_01551)

#define NOZEROFILE MsgGetString(Msg_01538)

#define OPTFEEDER_OVERWRITE MsgGetString(Msg_01541)
#define OPTFEEDER_ASKLOAD   MsgGetString(Msg_01544)

#define OPTFEEDER_MENU1     MsgGetString(Msg_01546), K_F1
#define OPTFEEDER_MENU2     MsgGetString(Msg_01547), K_F2
#define OPTFEEDER_MENU3     MsgGetString(Msg_01548), K_F3
#define OPTFEEDER_MENU4     MsgGetString(Msg_01549), K_F4
#define OPTFEEDER_MENU5     MsgGetString(Msg_01550), K_F5
#define OPTFEEDER_MENU6_1   MsgGetString(Msg_01554), K_F6
#define OPTFEEDER_MENU6_2   MsgGetString(Msg_01555), K_F6

#endif
