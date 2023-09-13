/*
>>>> Q_TABET.H

Definizioni dei testi specifici per Q_TABE.CPP.

ATTENZIONE: In caso di modifica dei testi, lasciare invariata la
		  loro lunghezza, o verificare prima attentamente il
		  codice in Q_TABE.CPP.
		  Non omettere eventuali CR/LF.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++    Modificato da WALTER 16.11.96
++++    Modificato da WALTER W1296 per mapp. assi -> MP
++++    Modificato da WALTER per mapp. offset teste W2604
++++	>>>UGE

*/

#if !defined(__Q_TABET_)
#define __Q_TABET_

// Intestazione di default dei campi dei files.

#if defined __SNIPER

#define TXTSNIPERMODES "Sniper modes file.\n\r"
#define TXTRES_CENTR_CFG_PAR "Reserved centering parameters\n\n\r"

#endif


#define TXTHD1      "Quadra Program File.\n\n\r"
#define TXTHD2      "Customer    :"
#define TXTHD2_QVIS "Title       :"
#define TXTHD3      "Last update :"
#define TXTHD4      "Note        :"

#define TXTNETPAR "Quadra network parameters file  \n\n\r"

#define TXTCF1 "Quadra Configuration File.\n\n\r"

#define TXTOPT "Quadra Optimization File.\n\n\r"   //DANY101002

#define TXTCENTRTEST "Quadra Centering Test File.\n\n\r"   //DANY191102 - SMOD150903

#define TXTCAR "Quadra Feeders File.\n\n\r"

#define TXTPAR "Quadra Parameters File. \n\n\r"

#define TXTUG1 "Quadra Pins File. \n\n\r"

#define TXTMAP "Quadra Heads Map File. \n\n\r"

#define TXTVIS "Quadra Vision data File. \n\n\r\x1A"

#define TXTZER "Quadra Board Ref. File. \n\n\r"

#define TXTUS	"Quadra Used Pins File. \n\n\r"

#define TXTCARINT "Quadra feeders pack database file.        \n1\r"

#define TXTRIFFEED  "Quadra feeders package reference pos.\n\n\r"

#define TXTIMG "Quadra Vision data File. \n\n\r\x1A"

#define	TXTCEN "Quadra centering tool parameters file.\n\n\r"

#define TXTTLAS "Quadra Laser Algorithm Types \n\n\r"

#define TXTBRUSH "Quadra Brushless motor configuration file \n\n\r"

#define TXTPACK    "Quadra Packages Library File ver 1.0\n\n\r"
#define TXTCONF    "Quadra Configuration File.\nPackages & Feeder\n\n\r"

#define TXTRESERVED_SECURITY_PARAMETERS    "Quadra Reserved Parameters\n\n\r"

#define TXTASSPRG "Quadra assembly file \n\n\r"

#define OPTHEADER_TXT "Quadra last optim.  \n\n\r"

// Header file .XAP di mappatura assi - W1296 MP
#define TXTAXS "Quadra Work Area Map File. \n\n\r"

// Header file .OAP di mappatura offset teste - W2604
#define TXTOFF "Quadra Offset Map File. \n\n\r"

// Header file .DEF settaggi default
#define TXTDEF "Quadra Default Parameter File. \n\n\r"

// Header dei file dati visione per package
#define TXTPACKVIS "Quadra Package Vision Data \n\n\r"

// Header file report assemblaggio
#define TXTASSREP "Quadra assembly report. \n\n\r" //SMOD30703

// Header file report errori di sweep
#define TXTSWEEPREP "Quadra laser sweep reports. \n\n\r" //SMOD050903

//Header file dimensioni ugelli
#define TXTUGEDIM "Quadra tools mechanical descriptions. \n\n\r" //SMOD141003

// Definizioni a seguire per desc. header tabelle packages. - wmd0
#define	TXTDS	"Quadra Packages Code File \n\n\r"
#define	TXTDSD	"Quadra Packages Data File \n\n\r"

// Definizione per dati aggiuntivi file programma  L0709
#define	TXTDTA	"Quadra Extra Program Data File \n\n\r"

#define  TXTDLAS	"Quadra Laser Data File. \n\n\r"

#define  TXTCARTIME "Quadra Feeder timings data file \n\n\r"

#define  TXTWARMUPPAR "Quadra Warm-Up settings file\n\n\r"

#define  TXTENCXPARAM "Quadra encoder X parameters file\n\n\r"
#define  TXTENCYPARAM "Quadra encoder Y parameters file\n\n\r"

#define  TXTMBKPINFO1 "Quadra last complete backup information\n\n\r"
#define  TXTMBKPINFO2 "Quadra last partial backup information\n\n\r"

#define  TXTBKPINFO  "Quadra net backup informations\n\n\r"

#define  TXTMLIST    "Quadra networked machines's name table\n\n\r"

#define TXTCONV "Quadra Conveyor Data File. \n\n\r"

// Messaggio per file parametri di rotazione non trovato
#define	NOROTNAME	MsgGetString(Msg_00578)

// N� max di classi di rotazione da leggere
// classe 0 per rotaz. a velocit� max
// classe 1 per rotaz. step by step
// classi 2-6 per rotazioni ugello
#define 	MAXCLROT	7

// Avviso per configurazione non caricata/disponibile
#define		NOCONF			MsgGetString(Msg_00185)

// Avviso per libreria package non caricata/disponibile
#define		NOPACK			MsgGetString(Msg_00284)

// Messaggio per file configurazione comandi trovato L2905
#define		NOCFGNAME	MsgGetString(Msg_00671)

// Messaggio per creazione file vuoto L2905
#define		ASKCFGCREATE MsgGetString(Msg_00672)

// Avviso per programma corrotto
#define 	PRGCORRUPTED	MsgGetString(Msg_00820) //L704

#define 	ERR_SNIPER_MODES_FILE    MsgGetString(Msg_05024)

#define 	ERR_SNIPER_MODES_FILE_RW MsgGetString(Msg_05025)



#define CARTIMEDEF0_NAME         "C"
#define CARTIMEDEF0_STRT_ELE     80
#define CARTIMEDEF0_STRT_MOT     20
#define CARTIMEDEF0_STRT_INV     0
#define CARTIMEDEF0_END_ELE      250
#define CARTIMEDEF0_END_MOT      550
#define CARTIMEDEF0_END_INV      100
#define CARTIMEDEF0_WAIT         10

#define CARTIMEDEF1_NAME         "M"
#define CARTIMEDEF1_STRT_ELE     80
#define CARTIMEDEF1_STRT_MOT     20
#define CARTIMEDEF1_STRT_INV     0
#define CARTIMEDEF1_END_ELE      250
#define CARTIMEDEF1_END_MOT      800
#define CARTIMEDEF1_END_INV      100
#define CARTIMEDEF1_WAIT         10

#define CARTIMEDEF2_NAME         "L"
#define CARTIMEDEF2_STRT_ELE     80
#define CARTIMEDEF2_STRT_MOT     20
#define CARTIMEDEF2_STRT_INV     0
#define CARTIMEDEF2_END_ELE      250
#define CARTIMEDEF2_END_MOT      800
#define CARTIMEDEF2_END_INV      100
#define CARTIMEDEF2_WAIT         10

#define CARTIMEDEF3_NAME         "D"
#define CARTIMEDEF3_STRT_ELE     80
#define CARTIMEDEF3_STRT_MOT     20
#define CARTIMEDEF3_STRT_INV     0
#define CARTIMEDEF3_END_ELE      250
#define CARTIMEDEF3_END_MOT      550
#define CARTIMEDEF3_END_INV      100
#define CARTIMEDEF3_WAIT         10

#define CARTIMEDEF4_NAME         "C1"
#define CARTIMEDEF4_STRT_ELE     80
#define CARTIMEDEF4_STRT_MOT     20
#define CARTIMEDEF4_STRT_INV     0
#define CARTIMEDEF4_END_ELE      250
#define CARTIMEDEF4_END_MOT      550
#define CARTIMEDEF4_END_INV      100
#define CARTIMEDEF4_WAIT         10

#define CARTIMEDEF5_NAME         "M1"
#define CARTIMEDEF5_STRT_ELE     80
#define CARTIMEDEF5_STRT_MOT     20
#define CARTIMEDEF5_STRT_INV     0
#define CARTIMEDEF5_END_ELE      250
#define CARTIMEDEF5_END_MOT      999
#define CARTIMEDEF5_END_INV      100
#define CARTIMEDEF5_WAIT         10

#define CARTIMEDEF6_NAME         "L1"
#define CARTIMEDEF6_STRT_ELE     80
#define CARTIMEDEF6_STRT_MOT     20
#define CARTIMEDEF6_STRT_INV     0
#define CARTIMEDEF6_END_ELE      250
#define CARTIMEDEF6_END_MOT      999
#define CARTIMEDEF6_END_INV      100
#define CARTIMEDEF6_WAIT         10

#define CARTIMEDEF7_NAME         "D1"
#define CARTIMEDEF7_STRT_ELE     80
#define CARTIMEDEF7_STRT_MOT     20
#define CARTIMEDEF7_STRT_INV     0
#define CARTIMEDEF7_END_ELE      250
#define CARTIMEDEF7_END_MOT      550
#define CARTIMEDEF7_END_INV      100
#define CARTIMEDEF7_WAIT         10

#ifdef __DOME_FEEDER  
#define CARTIMEDEF_DOME_NAME         	"DM"
#define CARTIMEDEF_DOME_STRT_ELE     	50
#define CARTIMEDEF_DOME_STRT_MOT     	20
#define CARTIMEDEF_DOME_STRT_INV     	0
#define CARTIMEDEF_DOME_END_ELE      	200
#define CARTIMEDEF_DOME_END_MOT      	300
#define CARTIMEDEF_DOME_END_INV      	2
#define CARTIMEDEF_DOME_WAIT         	10

#define CARTIMEDEF_DOME_UP_NAME        "DU"
#define CARTIMEDEF_DOME_UP_STRT_ELE  	15
#define CARTIMEDEF_DOME_UP_STRT_MOT     10
#define CARTIMEDEF_DOME_UP_STRT_INV     15
#define CARTIMEDEF_DOME_UP_END_ELE      900
#define CARTIMEDEF_DOME_UP_END_MOT      800
#define CARTIMEDEF_DOME_UP_END_INV      900
#define CARTIMEDEF_DOME_UP_WAIT         10

#define CARTIMEDEF_DOME_DW_NAME         "DD"
#define CARTIMEDEF_DOME_DW_STRT_ELE  	0
#define CARTIMEDEF_DOME_DW_STRT_MOT     10
#define CARTIMEDEF_DOME_DW_STRT_INV     15
#define CARTIMEDEF_DOME_DW_END_ELE      2
#define CARTIMEDEF_DOME_DW_END_MOT      800
#define CARTIMEDEF_DOME_DW_END_INV      900
#define CARTIMEDEF_DOME_DW_WAIT         10
#endif


#define CARTIME_OPEN_ERR  MsgGetString(Msg_00586)

#endif
