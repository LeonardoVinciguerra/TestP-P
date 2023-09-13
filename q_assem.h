/*
>>>> Q_ASSEM.H

Dichiarazioni delle funzioni esterne per gestione assemblaggio.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

*/

#ifndef __Q_ASSEM_
#define __Q_ASSEM_

#include "q_cost.h"
#include "q_carobj.h"
#include "timeprof.h"
#include "q_tabe.h"


//#define __ASSEMBLY_PROFILER //decommentare per abilitare il profiler dettagliato dell'assemblaggio

#ifdef __ASSEMBLY_PROFILER
	#define ASSEMBLY_PROFILER_MEASURE(fmt, ...)	AssemblyProfiling.measure(fmt, ## __VA_ARGS__)
	#define ASSEMBLY_PROFILER_START(fmt,...)	AssemblyProfiling.start(fmt, ## __VA_ARGS__)
	#define ASSEMBLY_PROFILER_STOP()			AssemblyProfiling.stop()
	#define ASSEMBLY_PROFILER_CLEAR()			AssemblyProfiling.clear()
	
	extern CTimeProfiling AssemblyProfiling;
#else
	#define ASSEMBLY_PROFILER_MEASURE(fmt,...)
	#define ASSEMBLY_PROFILER_START(fmt,...)
	#define ASSEMBLY_PROFILER_STOP()
	#define ASSEMBLY_PROFILER_CLEAR()
#endif


#define DEF_DELTASECURPOS 1

#define KSECURITY  2

#define PREDEPO_ZPOS 10.5
#define PREDEPO_ZTOL 1.2
#define PREDEPO_MAX  7       //SMOD270404


typedef enum
{
	protection_state_wait_transition, 	//lo stato della protezione non e' stato ancora campionato o e' in attesa che diventi open
 	protection_state_opened,			//la protezione e' risultata aperta almeno una volta nel polling loop
 	protection_state_closed,			//la protezione e' stata aperta e chiusa almeno una volta nel polling loop
} wait_to_cont_ass_internal_state_t;

extern wait_to_cont_ass_internal_state_t wait_to_cont_ass_internal_state;


//setta valore flag assemblaggio in corso
void Set_AssemblingFlag(unsigned int val);
//ritorna valore flag assemblaggio in corso
unsigned int Get_AssemblingFlag(void);
//ritorna valore flag dosaggio in corso
unsigned int Get_DosaFlag(void);
//ritorna record ultimo componente assemblato
int Get_LastRecMount(int mode=0); //SMOD110403
//ritorna record ultimo componente dosato
int Get_LastRecDosa(void);


// Gestione assemblaggio - main
int InitAss(int restart,int stepmode,int nrestart=-1,int asknewAss=1,int autoref=1,int enableContAss=1);
// Attiva modo assemblaggio passo-passo
void Set_bystep(void);
// Reset esterno dei valori compon./scheda di ripartenza assembl.
void ri_reset(void);
// Ritorna il n. di record del caricatore selezionato
int Car_code(int ck_code);

//algoritmo ciclo di assemblaggio normale
int ass_algo1(int *mount_flag,unsigned int v_comp,unsigned int a_comp,float PCBh);

//algoritmo ciclo di assemblaggio comp. grande+piccolo
int ass_algo2(int *mount_flag,unsigned int v_comp,unsigned int a_comp,float PCBh);

//routine avanzamento caricatore x punta specificata
int ass_avcaric(int punta);

void lasercheck(float x,float y,float lx,float ly,int punta,int v_comp);

// Gestione dosaggio - main - W0298
void InitDos(void);
// Gestione ripresa dosaggio interrotto - main - W0298 - mod. W3107
void InitRiDos(int tipo=0);
// Fine lavoro dosaggio - W0298
void DosEnd(void);
// Lancio dosaggio & assemblaggio - wmp1 - W3107
void dos_ass (int tipo=0);
// Dosaggio + assemblaggio - ripartenza - W3107
void go_rip(void);

#ifndef __DISP2
// dosa la prima serie di punti per mandare a regime siringa
int Dosa_steady(float X_colla, float Y_colla);
//ciclo di dosaggio
int Dosaggio(int ripart=0,int stepmode=0,int nrestart=-1,int interruptable=-1,int asknewDosa=1,int autoref=1,int enableContDosa=1,int disableStartPoints=0);
#else
int Dosaggio(int ndisp,int ripart=0,int stepmode=0,int nrestart=-1,int interruptable=-1,int asknewDosa=1,int autoref=1,int enableContDosa=1,int disableStartPoints=0);

int Dosa_steady(int ndisp,float X_colla, float Y_colla);
#endif


void Get_PosData(struct Zeri *zeri,const struct TabPrg& tab, const struct SPackageData* pack, float deltax,float deltay,float &outx,float &outy,float &outangle,int assdos_flag=0,int map_enable=1);

float Get_PhysThetaPlacePos( float compRot, const struct SPackageData& pack );
float Get_PickThetaPosition( int caric_code, const struct SPackageData& pack );

//SMOD290703
void ResetAssemblyReport(void);
void ShowAssemblyReport(void);

//SMOD260903
int CheckAssemblyEnd(TPrgFile *file);

void DoPredown(int punta,float pos); //SMOD270404

float GetMaxPlacedComponentHeight(void);

void wait_to_cont_ass_protection_check_polling_loop(void);

#endif
