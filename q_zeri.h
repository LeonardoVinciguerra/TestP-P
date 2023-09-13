/*
>>>> Q_ZERI.H

Dichiarazioni delle funzioni esterne per gestione tabella zeri scheda.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

*/

#if !defined(__Q_ZERI_)
#define __Q_ZERI_

#define ZER_NEWMASTER 1  //rieseguito circuito master
#define ZER_UPDATED   2  //eseguita una qualsiasi altra modifica

#define ZER_APP_ZEROREF  0
#define ZER_APP_ZERO     1
#define ZER_APP_REF      2

#define ZER_APP_MANUAL   0 //apprendimento manuale
#define ZER_APP_AUTO     1 //apprendimento automatico senza ritardo AGC

// Gestione edit tabella zeri scheda - main
void InitZer(int no_window = 0);
// Controllo codici tastiera.
extern int Z_keys(int C_comando, int C_reset);
// FIne lavoro - close del file
void ZerEnd(void);
// Autoapprend. zero scheda/ punto di riferim.

int Z_scheda(int n_scheda, int z_type = 0, int mode = 0, int tv_on= 0);
// Autoapprend. riferimenti quadrotto e composiz. circuito multiplo.
int M_scheda(void);

// Servizio: autoapprend. sequenziali
int Z_sequenza(int a_type, int mode, std::string text = "");

// Ridefinisce la posizione degli zeri scheda
int Z_redef(float &deltax,float &deltay);

int G_Zeri(void);

void ZSchMaster_App(void);  //SMOD100504

int  AskConfirm_ZRefBoardMode(void);
void Switch_ZRefSearch_Board(int show_msg=0);
int  Exist_ZRefPanelFile(void);
int  Create_ZRefPanelFile(void);
int  Z_AppMC(void);
int  AppZMasterDTheta(float zxpos,float zypos,double &angle);

#endif
