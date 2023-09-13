#ifndef __Q_INIT_
#define __Q_INIT_

#include "q_cost.h"

#define MSG_INIT              MsgGetString(Msg_01378)
#define MSG_INIT_QUIT         MsgGetString(Msg_01379)
#define MSG_INIT_KEY          MsgGetString(Msg_01380)
#define MSG_INIT_UART_OK      MsgGetString(Msg_01381)
#define MSG_INIT_UART_FAIL    MsgGetString(Msg_01382)
#define MSG_INIT_CPU_OK       MsgGetString(Msg_01387)
#define MSG_INIT_CPU_ERRVERS  MsgGetString(Msg_01388)
#define MSG_INIT_CPU_VERS1    MsgGetString(Msg_01389)
#define MSG_INIT_CPU_VERS2    MsgGetString(Msg_01390)
#define MSG_INIT_CPU_FAIL     MsgGetString(Msg_01391)
#define MSG_INIT_FOX_OK       MsgGetString(Msg_01392)
#define MSG_INIT_FOX_VERS     MsgGetString(Msg_01393)
#define MSG_INIT_FOX_FAIL     MsgGetString(Msg_01394)

//GF_TEMP - Motorhead messages
#define MSG_INIT_MOTORHEADX_VERS MsgGetString(Msg_06000)
#define MSG_INIT_MOTORHEADY_VERS MsgGetString(Msg_06001)
#define MSG_INIT_MOTORHEAD_FAIL  MsgGetString(Msg_06002)
#define MSG_WAIT_MOTORHEAD_RESET MsgGetString(Msg_06003)


#define MSG_INIT_EXTCAM_OK    MsgGetString(Msg_01596)
#define MSG_INIT_EXTCAM_FAIL  MsgGetString(Msg_01595)

//ricarica i parametri di rotazione senza resettare la scheda Fox
#define ROT_RELOAD 1
//resetta la scheda Fox e ricarica i parametri di rotazione
#define ROT_RESET  0

//msg errore azzeramento
#define ZZEROERR_MSG MsgGetString(Msg_00937)


#ifdef __DISP2
bool Get_SingleDispenserPar();
void Set_SingleDispenserPar( bool s );
#endif


void EnterGraphicMode(void);
void ExitGraphicMode(void);


// Init generale hardware.
bool HW_Init();
//procedura di init oggetti
int ReadMachineConfigFiles();
int WriteMachineConfigFiles();
int InitProc();
/*Inizializza azionamenti motori punte (brushless+stepper)*/
//mode=ROT_RESET      reset+ricarica parametri azionamenti
//mode=ROT_RELOAD     ricarica solo parametri azionamenti
void OpenRotation(int mode=ROT_RESET);
//disattivazione azionamenti testa
void CloseRotation(void);

#endif
