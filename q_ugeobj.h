/*
>>>> Q_UGEOBJ.H

Dichiarazione delle classi di gestione ugelli

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++    Sviluppo : Simone Navari - L.C.M. 14/03/2002        ++++
*/

#if !defined (__Q_UGEOBJ_)
#define __Q_UGEOBJ_

#include "q_tabe.h"
#include "q_cost.h"

#define NO_VLEVELSET 999    //indicatore livello vuoto (ad ugello libero) non settato

#define UGECHECK_STEP_LOSS         MsgGetString(Msg_01462) //perdita passo
#define UGECHECK_PICKUP_FAIL       MsgGetString(Msg_01463) //prelievo fallito
#define UGECHECK_PLACE_FAIL        MsgGetString(Msg_01464) //deposito fallito
#define UGECHECK_PICKUP_INS        MsgGetString(Msg_01465) //inserzione ugello sbagliata
#define UGECHECK_PICKUP_NOTFOUND   MsgGetString(Msg_01466) //ugello non trovato


#define UGECHECK_PICKUPMODE    0
#define UGECHECK_PLACEMODE     1

// Classe globale di gestione degli ugelli
extern class UgelliClass *Ugelli; //##SMOD060902

// File dedicato all'output dei dati degli ugelli
extern FILE *ufile;

// Operatore << per stampare i dati di un ugello su file
FILE* operator << (FILE *f,struct CfgUgelli uge);

// Classe UgeClass per la gestione a basso livello degli ugelli
class UgeClass
{ 
protected:
    CfgUgelli uge;				// Proprieta' dell'ugello
    struct v_vuoto val_vuoto;
    int ugepunta;				   // Punta presente sull'ugello (-1 se nessuna)
    int(*stepF)(const char *,int,int);	// Funzione di handling in modalita' passo-passo
public:
    int  GoPos(int punta);
    int  Prel(int x_punta);
    int  Depo(void);
    int  CheckUgeUp(int x_punta,int mode);

    int  AutoApp(int punta=1);

    void Set_SogliaVacuo(int val);
    void Set_VacuoLevel(int val);
    int  Set_VacuoLevel(void);
    int  Check_SogliaVacuo(void);

    void Set_StepF(int(*_stepF)(const char *,int,int));
    void GetRec(CfgUgelli *retuge);
    void SetRec(CfgUgelli inituge,int initpunta=-1);
    int  IsOnNozzle(void);
    int  Code(void);
	
    UgeClass(CfgUgelli inituge,int initpunta=-1);
    UgeClass(void);
    ~UgeClass(void) {}
    const UgeClass &operator=(const UgeClass &dat);
};


// Classe UgelliClass per la gestione ad alto livello degli ugelli
class UgelliClass
{ 
protected:
    UgeClass *cur_uge;	   // Struttura con i dati degli ugelli in uso
    int *mount_flag;			// Vettore di flag che indicano a DoOper se cambiare o no gli ugelli sulle punte
    int FileOffset;			// Offset all'interno del file degli ugelli
    int FileHandle;		   // Handler del file contenente i dati degli ugelli
    char opened;				// Flag che indica se il file dei dati degli ugelli e' aperto
    int(*stepF)(const char *,int,int);	// Funzione di handling in modalita' passo-passo
	
    int  Create(void);
public:
    void  UpdateUgeUse(void);
    int  Prel(int uge,int punta);
    int  Depo(int punta);
    int  Change(int uge,int punta);
    int  DoOper(int *n_ugello);
    void SetFlag(int p1,int p2);
    void DepoAll(void);
    void Set_StepF(int(*_stepF)(const char *,int,int));

    int  Set_VacuoLevel(int punta);
    void Set_VacuoLevel(int val,int punta);
    void Set_SogliaVacuo(int val,int punta);
    int  Check_SogliaVacuo(int punta);

    int  SaveRec(CfgUgelli H_Uge,int n);
    int  ReadRec(CfgUgelli &H_Uge,int n);
    int  GetRec(CfgUgelli &H_Uge,int punta);
    void ReloadCurUge(void);
    int  Open(void);
    int  GetInUse(int punta);
    void AutoApp_Seq(int punta=1);
	
    UgelliClass(void);
    ~UgelliClass(void);
};


#endif
