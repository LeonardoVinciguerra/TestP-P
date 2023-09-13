/*
>>>> Q_CONF.H

Dichiarazione delle funzioni esterne per modulo di gestione
configurazione comandi 2.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++	Modificato da WALTER 16.11.96
++++    >>VUO

*/

#if !defined (__Q_CONF_)
#define __Q_CONF_

#include "q_cost.h"


#define ZANGLE_CALIBRATION_UGEREF 'E'
#define ZANGLE_UGEREF_SIZE        7.3

#define PCOLLA_TESTPOINT          1     //punto colla di test
#define PCOLLA_INITIALPOINT       0     //punto colla iniziale prima del dosaggio


// Gestione Z calibration
void G_ZCalManual( int nozzle );

//Controllo database tipi ugelli
int CheckUgeDim(void);


void G_WarmUp(void);

int CheckAndCreateUSBMachinesFolder(const char* name,const char* mnt);


//GF_30_05_2011
int AuxCam_App( int punta );
int ExtCam_Scale( int nozzle );


#define HEADCAM_MMPIX_DEF    0.020
#define HEADCAM_MMPIX_MIN    (HEADCAM_MMPIX_DEF * 0.70)
#define HEADCAM_MMPIX_MAX    (HEADCAM_MMPIX_DEF * 1.80)

#define EXTCAM_MMPIX_DEF     0.015
#define EXTCAM_MMPIX_MIN     (EXTCAM_MMPIX_DEF * 0.70)
#define EXTCAM_MMPIX_MAX     (EXTCAM_MMPIX_DEF * 1.30)

#endif

