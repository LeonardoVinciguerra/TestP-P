/*
>>>> Q_MENU.H

Costanti, nomi e messaggi dei menu a barra


>> Il flag FAST istruisce le classi di composizione del menu a verificare
   la presenza della 'tilde' nelle stringhe delle barre e ad utilizzare
   il carattere evidenziato come tasto di scelta rapida. Questa opzione
   rallenta l'esecuzione dei menu. Utilizzare FAST sempre se si inserisce
   la ~ nelle voci del menu corrispondente. Nella lunghezza delle barre
   la tilde non viene considerata.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++	Modificato da TWS Simone 06.08.96
++++	Modificato da WALTER 16.11.96
++++ >>MAP >>OFF

*/

#if !defined (__Q_MENU_)
#define __Q_MENU_

#include "q_cost.h"

extern int VTeste_Edit(void);
extern int Limit_Edit(void);
extern int Endb_Edit(void);
extern int Vuoto_Edit(void);
extern int CalRot_Edit(void);
extern void TIM_Edit(void);
extern void TTest_Edit(void);
extern void LConf_Edit(void);
extern void Cen_Edit(void);
extern void TLas_Edit(void);
extern void Pack_Edit(void);

int Call_Exit(int code = YES);



//--------------- S140102 Def. Menu gestione programmi ------------

#define MSG_NO_REMOVABLE_STORAGE_DEVICE MsgGetString(Msg_01757)

#define MENUSPO_TXT1  MsgGetString(Msg_00871)
#define MENUSPO_TXT2  MsgGetString(Msg_00872)
#define MENUSPO_TXT3  MsgGetString(Msg_00806)

#define MENUAXCAL1    MsgGetString(Msg_01224)
#define MENUAXCAL2    MsgGetString(Msg_01225)
#define MENUAXCAL3    MsgGetString(Msg_01226)

#define MENU_PARAM_BACKUP         12,7
#define MENU_MACHINE_BACKUP_TO    62,8
#define MENU_MACHINE_RESTORE_FROM 62,9
#define MENU_MACHINE_BACKUP       40,7
#define MENU_BACKUP               14,6

#define MENU_MACHINE_BKP_TO_USB	  		55,10
#define MENU_MACHINE_RESTORE_FROM_USB	55,11

#define MENU_BACKUP1 MsgGetString(Msg_02108),999
#define MENU_BACKUP2 MsgGetString(Msg_02109),999

#define MENU_PARAM_BACKUP1 MsgGetString(Msg_02104),999
#define MENU_PARAM_BACKUP2 MsgGetString(Msg_02105),999

#endif
