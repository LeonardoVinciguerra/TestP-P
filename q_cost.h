/*
>>>> Q_COST.H

Costanti ad uso generale pubbliche per tutti i moduli

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++  	Modificato da TWS Simone 06.08.96
++++    Modificato da WALTER 16.11.96
++++    Mod. W03 19 marzo 1997 x ver. 4.1
++++	Mod. W1806 per mappatura assi / offset teste

*/


#ifndef __Q_COST_
#define __Q_COST_

//#define HWTEST_RELEASE //decommentare per versione di test hardware a banco

//#define __LINEDISP          //decommentare per attivare il modo dispenser a linee

//livelli di allarme
//---------------------------------------------------
#define NO_ALARM         0 //nessun allarme
#define GENERIC_ALARM    1 //allarme generico

#define ALARM_DISABLED   0 //segnalatore di allarme disattivato
#define ALARM_ENABLED    1 //segnalatore di allarme attivato

#ifdef __DISP2
	#define __DISP2_REMOVABLE
#endif


//cambiamento limite della posizione di sicurezza
#define PUP_POS_LIMIT_CHANGE 0.1

//delta tra posizione di sicurezza e quota sensore di traslazione SMOD280403
#define DELTA_HEADSENS 0.7


/**********************    FILE VERSION   **********************************/
//SMOD230703-bridge
#define FILES_VERSION         "1"

#define QUADRA_HEADER_START  "QLVer"

#define QUADRA_HEADER         QUADRA_HEADER_START "X.X-"

#define QUADRA_HEADER_LEN     strlen(QUADRA_HEADER)

#define WRITE_QUADRA_HEADER(Handle,version,subversion) write(Handle,QUADRA_HEADER_START version "." subversion "-",QUADRA_HEADER_LEN)

#define WRITE_HEADER     WRITE_QUADRA_HEADER
#define HEADER_LEN       QUADRA_HEADER_LEN
#define HEADER_START     QUADRA_HEADER_START


/**********************  SOFTWARE VERSION **********************************/

/**********************  SWITCH MODO SERIALE o STANDARD ********************/
#define __SERIAL        //(comment me to switch in standard (non serial) mode)
/***************************************************************************/

/**********************  SWITCH MODO VISIONE o STANDARD ********************/
#define __VISION        //(comment me to switch in standard (non vison) mode)
/***************************************************************************/

/***********************  SWITCH DOSAGGIO COLLA  ***************************/
#define __DOSA  //(commentare per escludere il dosaggio) - W0298
/***************************************************************************/


// Max. n. di caratteri di un percorso (path) compreso nomefile.ext
#define MAXNPATH        128

#define YES            1
#define NO             2
#define ALL            3
#define EXIT           4

// Stati ON/OFF
#define ON             1
#define OFF            0


// N. max ugelli memorizzabili
#define MAXUGE         26
// Max. numero di vassoi
#define MAXTRAY        40
// Max. numero di pacchi caricatori
#define MAXMAG         15
// Max. numero di record caricatori
#define MAXNREC_FEED   MAXMAG*8
// Max. numero di caricatori (rotelline+vassoi)
#define MAXCAR         MAXMAG*8+MAXTRAY
// Numero primo vassoio
#define FIRSTTRAY      (MAXMAG+1)*10
// Numero ultimo vassoio
#define LASTTRAY       (MAXMAG+MAXTRAY)*10
// Numero ultimo caricatore
#define LASTFEED       (MAXMAG*10)+8

// Max. numero di packages utilizzabili
#define MAXPACK        500



#define MOUNT_DIR		"/media"
#define USB_MOUNT		MOUNT_DIR "/usb"
#define TEMP_DIR		"tmp"


#define ZVELMIN            5    //velocita mininma asse z
#define ZACCMIN         1700    //val. param. acc. mininmo asse z
#define RVMIN             80    //vel. rotazione min
#define RAMIN            800    //acc. rotazione min
#define XYVMIN           100    //vel. xy min
#define XYAMIN           100    //acc. xy min

// dimensione immagine finestra parametri immagine
#define SHOW_IMG_MAXX 240
#define SHOW_IMG_MAXY 240

// dimensioni min e max pattern (max minori del precedente)
#define PATTERN_MINX   20
#define PATTERN_MINY   20
#define PATTERN_MAXX   220
#define PATTERN_MAXY   220

// tipo di riconoscimento immagini
#define MATCH_CORRELATION	0
#define MATCH_VECTORIAL		1

#include "sniper_files.h"


#define DISK_DEV_PREFIX 		"/dev/sd"
#define USB_MOUNT_PREFIX 		"/media"

//Directory visione
#define VISIONDIR       "vision/"
#define VISPACKDIR      "vispack/"
#define VISPACK_EXT     ".pvd"

#define RESERVED_PARAMTERS_LOG_FILE HOME_DIR "/.qdvc.sys"


// estensioni files quadra
// programma di montaggio
#define PRGEXT      ".qpf"
// dati aggiuntivi programma di montaggio
#define DTAEXT      ".dta"
// file assemblaggio completo
#define ASSEXT      ".qaf"
// file dosaggio
#define DOSATEXT    ".qdf"
// file ultima ottimizzazione
#define LASTOPT_EXT ".qlo"
// file ASCII-Q
#define ASCIIQ_EXT  ".asq"
// file CSV
#define CSV_EXT  ".csv"
// file caricatori
#define CAREXT      ".qff"


#define         PKG_STDLIB "stdlib"

#define         FPACKDIR    "pack"
// Subdirectory visione
#define         VISDIR  "vision"
// Subdirectory della corrente per files caricatori
#define         CARDIR  "feed"
// Subdirectory della corrente per files programma.
#define         PRGDIR  "prog"
// Subdirectory della directory clienti
#define         CLIDIR  "cust"
// Subdirectory visione packages
#define         VISPACK  "vispack"

// subdir. per il backup dei file aggiornati
#define         BACKDIR "back" //SMOD230703-bridge

// Estensione files zeri scheda
#define         ZEREXT     ".qfz"
// Estensione files data zeri
#define         ZERDAT_EXT ".qdz"
//Estension file dati circuito multiplo
#define         ZERMC_EXT  ".qmc"

// ok
#define CHARSET_TEXT                " 0123456789ABCDEFGHILMNOPQRSTUVZXYWKJabcdefghilmnopqrstuvzxywkj~!#$%&*()=+:-_/.,'[]{}<>?;/\\"
#define CHARSET_TEXT_NO_SPACE       "0123456789ABCDEFGHILMNOPQRSTUVZXYWKJabcdefghilmnopqrstuvzxywkj~!#$%&*()=+:-_/.,'[]{}<>?;/\\"
#define CHARSET_FILENAME            "0123456789ABCDEFGHILMNOPQRSTUVZXYWKJabcdefghilmnopqrstuvzxywkj_/\\"
#define CHARSET_NUMUINT             "0123456789"
#define CHARSET_NUMSINT             "-0123456789"
#define CHARSET_NUMSDEC             ".-0123456789"
#define CHARSET_NUMUDEC             ".0123456789"
#define CHARSET_TOOL                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"


//Costanti misura tensioni / correnti / temperatura macchina
#define 	COSTV1	0.216
#define 	COSTV2	0.157
#define 	COSTV3	0.1118
#define 	COSTI1	0.00834
#define 	COSTT1	0.46
#define 	COSTT2	-2

#define  COSTV4   0.154

#define	VINMIN	28.0
#define	VINMAX	32.0
#define	VC24MIN	25.5
#define	VC24MAX	27.5
#define	IC0MIN	0.0
#define	IC0MAX	0.1
#define	IH0MIN	0.0
#define	IH0MAX	0.1
#define	VH24MIN	23.0
#define	VH24MAX	25.0
#define	VP12MIN	12.0
#define	VP12MAX	14.0
#define	TPMIN	5
#define	TPMAX	60
#define	IROTMIN	0.5
#define	IROTMAX	1.2
#define	IPNZMIN	0.5
#define	IPNZMAX	1.2
#define	IMOTMIN	0.5
#define	IMOTMAX	1.5
#define	IINVMIN	0.0
#define	IINVMAX	0.1
#define	V30MIN	28.0
#define	V30MAX	32.0
#define	V24MIN	23.0
#define	V24MAX	25.0
#define	V12MIN	12.0
#define	V12MAX	14.0


#define  EOFVAL         26

//SMOD280503
#define PCB_H_ABS_MIN   0.1 //valore assoluto minimo per altezza pcb
#define PCB_H_MIN      -5.f // limiti combo altezza pcb
#define PCB_H_MAX      10.f

//distanza tra il fascio laser e la fine del corpo del sensore
#ifdef __SNIPER
#define SNIPER_BODY_DELTA		5.57
#define SNIPER_MAXCOMP_DELTA 	0.43
#endif


//------------------------------------------------------------------------------------------------
//SMOD230703-bridge

//Files current subversion
//(DEFINIRLI COME STRINGHE !!)

#define DOSACONFIG_SUBVERSION    "2"  //GF_05_07_2011
#define DOSAPACK_SUBVERSION      "0"
#define MAPDATA_SUBVERSION       "0"
#define LASTOPT_SUBVERSION       "0"
#define UGEFILE_SUBVERSION       "0"
#define CARFILE_SUBVERSION       "1"
#define PRG_SUBVERSION           "0"
#define PACKVIS_SUBVERSION       "0"
#define TLAS_SUBVERSION          "0"
#define ZERFILE_SUBVERSION       "1"  //dalla versione 1.07
#define PRGDTA_SUBVERSION        "0"
#define VISPAR_SUBVERSION        "0"
#define IMPGPAR_SUBVERSION       "0"
#define PARAM_SUBVERSION         "0"
#define CONFIG_SUBVERSION        "4"
#define MAPROT_SUBVERSION        "2"
#define CURDATA_SUBVERSION       "0"
#define OPTPARAM_SUBVERSION      "0"
#define BRUSHDATA_SUBVERSION     "0"
#define TESTHWCENTR_SUBVERSION   "0"
#define CARINT_SUBVERSION        "0"
#define CARINTPOSREF_SUBVERSION  "0"
#define ASSREPORT_SUBVERSION     "0"
#define SWEEPERR_SUBVERSION      "0"
#define UGEDIM_SUBVERSION        "0"
#define UGEREFS_SUBVERSION       "1"
#define NETPAR_SUBVERSION        "0"
#define CARTIME_SUBVERSION       "0"
#define WARMUPPAR_SUBVERSION     "0"
#define ENCXYPARAM_SUBVERSION    "0"
#define MBKPINFO_SUBVERSION      "0"
#define BKPINFO_SUBVERSION       "0"
#define MLIST_SUBVERSION         "0"
#define SNIPERMODES_SUBVERSION   "1"
#define RESERVED_CENTR_CFG_PAR_SUBVERSION "0"
#define CONV_SUBVERSION          "0"
//Files previus subversion
#define CARFILE_PRESUBVERSION    "0"
#define CONFIG_PRESUBVERSION     "3"
#define MAPROT_PRESUBVERSION	 "1"


//prima versione del file parametri di configurazione con supporto usb
#define CFG_FIRST_VERSION_MULTIPOLE_BRUSHLESS 	"4"

#define CFG_FIRST_VERSION_DUAL_DISPENSER		"3"

#define DISP_FIRST_VERSION_DUAL_DISPENSER		"1"

//==============================================================================

//Dati PID brushless
#define BRUSHFNAME        STDFILENAME".bsh"

// Nome del file tempi avanzamenti caricatori
#define CARTIME_FILE      STDFILENAME".ftf"

// Nome del file di configurazione
#define CFGNAME           STDFILENAME".cfg"

// Nome del file dei parametri del test hw centraggio - DANY191102 SMOD150903
#define CENTTESTNAME      STDFILENAME".cht"

//Nome file dati correnti
#define CURFNAME          STDFILENAME".cur"

// Nome file caric intell (copia locale)
#define  CARINTFILE       STDFILENAME".dbm"


// Nome file caric intell (copia remota)
#define  CARINTREMOTE     	REMOTEDISK "/" STDFILENAME ".dbm"
#define  CARINTREMOTE_QMODE REMOTEDISK "/QLASER.DBM"

// Nome file posiz riferimenti caric intell
#define  CARRIFNAME       STDFILENAME".dbr"

// Nome del file parametri dosatore - L1905
#ifndef __DISP2
#define DISPNAME	        STDFILENAME".dis"
#else
#define DISPNAME1         STDFILENAME".di1"
#define DISPNAME2         STDFILENAME".di2"
#endif

//nome del file report hardware test
#define	HARDWNAME        STDFILENAME".hdw"

//nome del file dei flag di init
#define INIFILE           STDFILENAME".ini"

//Nome file selezione lingua
#define DEFFILE_LANGUAGE  STDFILENAME".lan"

// Nome del file di mappatura teste.
#define MAPNAME           STDFILENAME".map"

//Nome del file parametri rete
#define NETPARNAME        STDFILENAME".nwp"

// Nome del file mappatura offset teste - W2604
#define OFFMAP	           STDFILENAME".oap"

// Nome del file parametri di lavoro
#define PARNAME           STDFILENAME".par"

// Nome file dimensioni ugelli
#define UGEDIM_FILE       STDFILENAME".udm" //SMOD141003

// Nome del file di memorizzazione ugelli.
#define UGENAME           STDFILENAME".uge"

//Nome file parametri di warm-up
#define WARMUPPARAMS_FILE STDFILENAME".wrm"

//Nome file modi sniper
#define SNIPERMODES_FILE  STDFILENAME".SMO"

// Nome del file di configurazione convogliatore
#define CONVNAME          STDFILENAME".cnv"


#define SECURITY_RESERVED_PARAMETERS_FILE STDFILENAME".rvd"

#define OPTCENT_CFG_PAR_FILE STDFILENAME ".ocp"
#define RESERVED_CENTR_CFG_PAR_FILE STDFILENAME ".rcp"

//Nome file dati mappatura assi
#define MAPDATAFILEXY     "mapdataxy.dat"

// Nome file dati visione
#define VISDATAFILE       "visdata.dat"

//nome programma split
#define ZIP_PRG           "zip"			//TEMP verificare
#define UNZIP_PRG         "unzip"		//TEMP verificare

#define USB_MACHINE_ID_FOLDER "machines"
#define USB_BACKUP_FOLDER     "backup"
#define USB_INSTALL_FOLDER    "install"

#define BKP_COMPLETE_TAG      "bkc"
#define BKP_PARTIAL_TAG       "bkp"

#define BKP_LAST_FULL         STDFILENAME ".lcb"
#define BKP_LAST_PARTIAL      STDFILENAME ".lpb"
#define BKP_INFO_FILE         STDFILENAME ".bkp"

#define BKP_LIMIT_FULL        3
#define BKP_LIMIT_PARTIAL     100
#define BACKUPDAYS            30

#define BACKUP_ROOTDIR        "root"


// Password
#define	 CONFIG_PSW				"CONFIG"

#endif


