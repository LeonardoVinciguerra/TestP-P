/*
>>>> Q_TEST2.H

Dichiarazioni delle funzioni esterne per test hardware 2.
(CARICATORI, CAMBIO UGELLO).

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

*/

#if !defined(__Q_TEST2_)
#define __Q_TEST2_

#include "q_carobj.h"
#include "q_tabe.h"

// Gestione cursore - Test caricatori.
int KCAR_Keys(int comando, int reset);
// Gestione cursore - Test movimento assi.
int MOV_Keys(int comando, int reset);
// Sequenza caricatori
int KCAR_seq(void);
// Sequenza movimento assi
int MOV_seq(void);
// Gestione test zero macchina (solo testa !!).
void K_zeromacc(void);

float ZAutoApprend(int punta,float avvpos,float xpos,float ypos,float coarse_step,float fine_step);

int fn_FeederTest();
int fn_ToolChangeTest();
int fn_AxesMoveTest();
int fn_CenteringTest();
int fn_MotorheadLogParams();
int fn_ConveyorTest();

#endif
