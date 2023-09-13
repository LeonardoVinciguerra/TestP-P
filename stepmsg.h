#include  "msglist.h"

#define S_CARAVANZ    MsgGetString(Msg_00440)  //avanz. caricatore
#define S_CHKPRESO    MsgGetString(Msg_00208)  //check componente preso
#define S_VACUON      MsgGetString(Msg_00202)  //aziona vuoto
#define S_PDOWN       MsgGetString(Msg_00203)  //punta giu
#define S_PUP         MsgGetString(Msg_00204)  //punta su
#define S_CONTROPON   MsgGetString(Msg_00211)  //attiva contropressione
#define S_CONTROPOFF  MsgGetString(Msg_00214)  //disattiva contropressione
#define S_LASERSWEEP  MsgGetString(Msg_00866)  //inizia centraggio laser
#define S_GETLASER    MsgGetString(Msg_00867)  //get risultati laser
#define S_COMPROT     MsgGetString(Msg_00213)  //postrotazione di correzione
#define S_PREROT      MsgGetString(Msg_00205)  //prerotazione laser
#define S_GODEPO      MsgGetString(Msg_01016)  //vai a pos. di deposito
#define S_GOUGEPOS    MsgGetString(Msg_00882)  //vai a posizione ugello
#define S_UGEBLK_ON   MsgGetString(Msg_00883)  //attiva blocco porta ugelli
#define S_UGEBLK_OFF  MsgGetString(Msg_00884)  //attiva blocco porta ugelli
#define S_GOPREL      MsgGetString(Msg_00572)  //vai a coordinate di prelievo
#define S_ZERTHETA    MsgGetString(Msg_00573)  //ritorna a zero theta
#define S_PREDISCESA  MsgGetString(Msg_01207)  //prediscesa punta - DANY220103
#define S_STEADYDOSA  MsgGetString(Msg_00540)  //dosaggio punti colla iniziali
#define S_ZFOCUS	  MsgGetString(Msg_05075)  //abbassa a quota di fuoco telecamera esterna
#define S_EXTCAM1	  MsgGetString(Msg_05076)  //centraggio con camera esterna : prima foto
#define S_EXTCAM2	  MsgGetString(Msg_05077)  //centraggio con camera esterna : seconda foto
#define S_EXTCAM_ROT  MsgGetString(Msg_05078)  //centraggio con camera esterna : allineamento
#define S_SECZPOS	  MsgGetString(Msg_05079)  //risalita in sicurezza
