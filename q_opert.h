/*
>>>> Q_OPERT.H

Include per Q_OPER.CPP.

Definizioni esclusive per funzioni operative e di azionamento attuatori.
Definizioni degli I/O address.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++    Modificato da TWS Simone 06.08.96
++++    Modificato da WALTER 16.11.96

*/

#if !defined(__Q_OPERT_)
#define __Q_OPERT__


// Tempo in sec. timeout segnale ready caricatori
#define	CREADY	5


// Messaggio per mancanza pressione allo start.
#define	NOPRESS		MsgGetString(Msg_00001)
// Msg. per passaggio in modalit? file.
#define	MODFILE		MsgGetString(Msg_00002)

// Msg. per zero testa automatico non possibile.
#define	ZEROMAN		MsgGetString(Msg_00004)

// Msg. per mancanza pressione durante assemblaggio
#define	NOAIR			MsgGetString(Msg_00006)

// Msg. per timeout segnale ready caricatori
#define	NOCREADY		MsgGetString(Msg_00007)



//****   Finestra zero testa/macchina manuale *****



#ifdef __DOSA
// Titolo autoappr. posizione dosatore in offset dosatore - W0398
#define	DOSATITLE	MsgGetString(Msg_00658)
#endif

// Warning: movimenti in punta down
#define	NOMOVE_PDOWN MsgGetString(Msg_00041)

// Stop movimenti con protezione alzata.
#define	PROTOPEN	MsgGetString(Msg_00547)		// WALTER 16.11.96

// Stop movimenti fuoti dei limiti.
#define	OUTLIMIT	MsgGetString(Msg_00655)		// LORIS 11.02.98/W0298

// Titolo autoapprendimento per posiz. punto colla - wmp1
#define		COLLTITLE MsgGetString(Msg_00699)

// Titolo autoapprendimento per posiz. fiduciale Integr. Loris
#define		REDEFZERI MsgGetString(Msg_00745)

#define     PACKLLEFT_TITLE MsgGetString(Msg_01572)
#define     PACKLRIGHT_TITLE MsgGetString(Msg_01573)

//brushless in protezione
#define  ERRBRUSH       MsgGetString(Msg_00956)

//errore movimento brushless (regime non raggiunto)
#define  ERRBRUSHSTEADY MsgGetString(Msg_01376)

//avviso movimenti disabilitati
#define  MOVDISABLE     MsgGetString(Msg_00957)
//nessun programma di calibrazione eseguito
#define  NOCALIBPRG     MsgGetString(Msg_00870)


#define    AUTOAPP_OK       0x0D //(CR)
#define    AUTOAPP_ABORT    0x1B //(ESC)

#define    STEP_PDOWN       MsgGetString(Msg_00203)
#define    STEP_VACUOON     MsgGetString(Msg_00202)
#define    STEP_PUP         MsgGetString(Msg_00204)
#define    STEP_COMPROTAZ   MsgGetString(Msg_00213)
#define    STEP_CONTROON    MsgGetString(Msg_00211)
#define    STEP_CONTROOFF   MsgGetString(Msg_00214)
#define    STEP_GODEPO      MsgGetString(Msg_01016)
#define    STEP_STARTSWEEP  MsgGetString(Msg_00866)
#define    STEP_GETLASER    MsgGetString(Msg_00867)  //get risultati laser

#define    STEP_PREDISCESA  MsgGetString(Msg_01207)  //prediscesa punta - SMOD220103

#define    WAITPUP_TIMEOUT  1500
#define    WAITPUP_ERROR    MsgGetString(Msg_01169)

// Presa comp. non corretta
#define	NOPRESA			MsgGetString(Msg_00223)
//messaggio result check presenza comp. con vuoto
#define  CHECKVAC       MsgGetString(Msg_00366)
//messaggio result check presenza comp. con laser
#define  CHECKLAS       MsgGetString(Msg_00369)


//costanti per ricerca zero asse z con sensore di traslazione
#define  INITPUP_LIMIT  30
#define  ERRINTPUP_MSG MsgGetString(Msg_01245)
#define  WAITRESETZ_MSG  MsgGetString(Msg_01063)


//massimo numero di campioni per grafico attesa fine rotazione con encoder
#define MAXWAITENC_ARRAY 2000                 //SMOD060503-END ROTATION TEST
#define WAITENC_GRTIT MsgGetString(Msg_01377) //SMOD060503-END ROTATION TEST

//messaggio finestra laser occupata
#define  LASZUP_ERR MsgGetString(Msg_00959)

//costanti errore movimento blocco porta ugelli
#define  WAITREEDUGE_ERR      MsgGetString(Msg_00817)
#define  WAITREEDUGE_TIMEOUT  1.0

#define  ZEROMAC_AUTO_DELTA 10.0 //SMOD250803

#define  BRUSHSTEADY_POS        5,3,76,22

//movimento in z sotto il quale viene settata una accelerazione
//piu bassa
#define CRITICAL_ZMM    5.0      //SMOD031003
//accelerazione da utilizzare per i movimenti z brevi
#define CRITICAL_ZACC   6000     //SMOD031003


#define EMPTYTRAY MsgGetString(Msg_01627)

#define ERRAXIS_XY MsgGetString(Msg_01865)

#define ASK_DOWARMUP   MsgGetString(Msg_01905)


#define AUXCAM_FIDUCIAL_TITLE  MsgGetString(Msg_01972)
#define AUXCAM_FIDSEARCH_TITLE MsgGetString(Msg_01976)
#define AUXCAMFIDUCIAL_ERR     MsgGetString(Msg_01977)
#define AUXCAM_DISABLED        MsgGetString(Msg_01978)

#define AUXCAM_FIDCHANGE_LIMIT 0.5

#define ASK_MOV_DISABLE MsgGetString(Msg_02081)

#define MENU_ZLOSSCHECK_SHORTCUT_POS  15,10
#define MENU_ZLOSSCHECK_SHORTCUT_TXT1 MsgGetString(Msg_00042)
#define MENU_ZLOSSCHECK_SHORTCUT_TXT2 MsgGetString(Msg_00043)

#define MENU_ZCHECK_SHORTCUT_POS  15,10
#define MENU_ZCHECK_SHORTCUT_TXT1 MsgGetString(Msg_00042)
#define MENU_ZCHECK_SHORTCUT_TXT2 MsgGetString(Msg_00043)

#endif
