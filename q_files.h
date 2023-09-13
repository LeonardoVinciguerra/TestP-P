/*
>>>> Q_FILES.H

Dichiarazione delle funzioni esterne per modulo di gestione files programma.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

*/
#include <list>

#if !defined (__Q_FILES_)
#define __Q_FILES_


namespace bkp
{
	typedef enum
	{
		USB,
  		Net
	} e_dest;
}


extern int CountDirFiles(char *);

//Esegue backup macchina su floppy
extern int DataBackup(const char* mnt);

//Verifica la presenza delle dir per il cliente specificato - DANY210103
void DirVerify(char *cust);

extern GUI_ProgressBar_OLD *DelDir_Progbar;
extern CWindow        *DelDir_wait;

//controlla se il backup e' stato eseguito almeno una volta
//ed esegui backup completo se necessario e richiesto
#define CHKBKP_FIRST        1
//controlla i giorni trascorsi dall'ultimo backup completo
//se superiori a 30 chiede se eseguire backup con possibilita
//di scelta tra completo e parziale
#define CHKBKP_DATE         2
//esegue il backup parziale se i controlli precedenti sono falliti
#define CHKBKP_PARTIAL      4

#define CHKBKP_NORMAL CHKBKP_FIRST | CHKBKP_DATE | CHKBKP_PARTIAL

#define BKP_COMPLETE        1
#define BKP_PARTIAL         2

int CheckBkp(int mode,bkp::e_dest dest, const char* mnt = NULL);
void GetBackupBaseDir(char* base,bkp::e_dest dest, const char* mnt = NULL);
void GetBackupDir(char *bkp_base,int mode,int bkp_num,bkp::e_dest dest,const char* mnt = NULL);
int DoBkp(int mode,bkp::e_dest dest, const char* mnt = NULL);
int DoBkpRestore(bkp::e_dest dest, const char* mnt = NULL);


#endif
