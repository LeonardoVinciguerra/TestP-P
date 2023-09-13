/*
>>>> Q_FILEST.H

Include per Q_FILES.CPP.
Definizioni dei testi esclusivi per gestione dei files di programma.

ATTENZIONE: In caso di modifiche ai testi,
		   rispettare la lunghezza attuale delle stringhe !!!

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++  	Modificato da TWS Simone 06.08.96 

*/

#if !defined (__Q_FILEST_)
#define __Q_FILEST_

#include "q_cost.h"


#define UNABLE_TO_ACCESS_TO_FOLDER MsgGetString(Msg_02115)
#define UNABLE_TO_ACCESS_TO_FILE   MsgGetString(Msg_02116)

#define BACKUP_ERR_TEMPDIR    MsgGetString(Msg_01876)
#define BACKUP_ERR_CREATEZIP  MsgGetString(Msg_01877)
#define BACKUP_ERR_COMPRESSF  MsgGetString(Msg_01880)
#define BACKUP_ERR_COMPRESSD  MsgGetString(Msg_01881)
#define BACKUP_ERR_WRITEZIP   MsgGetString(Msg_01882)
#define BACKUP_ERR_COPYFILE   MsgGetString(Msg_01884)

// Messaggio per USB drive not ready.
#define	 USB_NOT_READY   MsgGetString(Msg_02106)


#define BKPCUST_COMPLETED MsgGetString(Msg_01528)
#define WAIT_BKPCUST2     MsgGetString(Msg_01762)
#define WAITBKPCUST_POS   2,2,49,1


#define WAIT_COPYDIR    MsgGetString(Msg_01758)
#define WAITCOPYDIR_POS 2,3,36,1

#define BKPWAIT        MsgGetString(Msg_01643)

#define BKPPOS         10,8,70,13

#define NOBKP          MsgGetString(Msg_01642)
#define NETRESTORE_MSG MsgGetString(Msg_01645)
#define RESTOREWAIT    MsgGetString(Msg_01644)

#define ASKRESTORE     MsgGetString(Msg_01871)

#define BKPTYPE_TXT      MsgGetString(Msg_02003)
#define BKPCOMPLETE_TXT  MsgGetString(Msg_02004)
#define BKPPARTIAL_TXT   MsgGetString(Msg_02005)

#define RESTOREPOS     10,8,70,13

#define FIRSTBKP       MsgGetString(Msg_01648)
#define LASTBKP        MsgGetString(Msg_01649)

#define BKP_PARTBUTTON MsgGetString(Msg_01650)
#define BKP_FULLBUTTON MsgGetString(Msg_01651)

#define ASK_BKP        MsgGetString(Msg_01652)

#define NO_MACHINE_ID        MsgGetString(Msg_01640)

#define BKPFILESERR    MsgGetString(Msg_01906)

#endif
