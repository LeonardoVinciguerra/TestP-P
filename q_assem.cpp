/*
>>>> Q_ASSEM.CPP

Gestione degli assemblaggi.

-> Comprende ottimizzazione cambio ugello.
-> Ordinamento per tipo punta/ugello automatico con ottimizzazione ON
   ( in Q_PROG ) .

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++    Modificato da TWS Simone 06.08.96
++++      Mof. Walter 02 aprile 97 (mark:  W0204)
++++      Mod. Walter ottobre 97 - mark:W09
*/
#include <boost/thread.hpp>

#include "q_assem.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <ctype.h>

#include "filefn.h"
#include "q_cost.h"
#include "q_ugeobj.h"
#include "msglist.h"
#include "stepmsg.h"

#include "q_gener.h"
#include "q_wind.h"
#include "q_help.h"
#include "q_files.h"
#include "q_oper.h"
#include "q_opert.h"
#include "q_grcol.h"
#include "q_prog.h"

#include "q_ser.h"
#include "q_progt.h"
#include "q_zeri.h"
#include "q_zerit.h"
#include "q_fox.h"

#include "q_packages.h"
#include "q_carobj.h"

#ifdef __SNIPER
#include "sniper.h"
#include "tws_sniper.h"
#include "las_scan.h"
#include "q_snprt.h"
#endif

#include "q_dosat.h"
#include "q_carint.h"
#include "q_vision.h"
#include "q_inifile.h"
#include "q_conf.h"
#include "q_net.h"

#include "keyutils.h"
#include "strutils.h"
#include "lnxdefs.h"

#include "Timer.h"
#include "c_win_select.h"
#include "c_inputbox.h"
#include "gui_functions.h"
#include "centering_thread.h"
#include "q_files_new.h"

#include "discard_log.h"
#include "working_log.h"

#ifdef __ASSEMBLY_DEBUG
#include "q_logger.h"
CLogger AssemblyVisDebug( "visdebug.txt", "Vision Log File" );

#include "motorhead.h"
CLogger AssemblyEncoder( "a_encoder.txt", "Encoder Log File" );
float reqPosX = 0;
float reqPosY = 0;
#endif


#include <mss.h>


#define CENTERING_CYCLE_TIMEOUT      5000
#define MAX_SWEEP_REPEAT             2


//GF_THREAD
extern boost::mutex centeringMutex;

extern SSpeedsTable SpeedsTable;
extern SMachineInfo MachineInfo;
extern SPackageData currentLibPackages[MAXPACK];
extern SPackageOffsetData currentLibOffsetPackages[MAXPACK];
extern struct CarInt_data* CarList;

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;



struct MountData
{
	struct Zeri ZerRecs[3];
	struct TabPrg PrgRec[2];
	struct CarDat CarRec[2];
	SPackageData Package[2];
	struct dlas_type TLas[2];
	struct CfgUgelli Uge[2];
	FeederClass *Caric[2];
	int mount_flag[2];
	float pick_angle[2];
};



int AssScaricaComp( int nozzle, int mode=SCARICACOMP_NORMAL, const char* cause = 0 );

int last_assembly_time = 0;

float maxCompHeight = PREDEPO_ZPOS;


extern struct CfgTeste MapTeste;

extern int nboard, ncomp;

int las_errmode[2]={ LASMSGOFF, LASMSGOFF };
int vacuocheck_dep[2]={ 0, 0 }; //SMOD291004
int compcheck_post_dep[2] = { 0, 0 }; //GF_07_04_2011

int startOnFile=0;

extern int global_pos[2];
extern float global_time;

extern struct TabPrg ATab[MAXRECS];   // Array di struct per display records     L2204

extern FeederFile* CarFile;
extern struct CfgDispenser CC_Dispenser;

TPrgFile* TPrgAssem=NULL;
ZerFile* ZerAssem=NULL;

TPrgFile* TPrgDosFile=NULL;
ZerFile* ZerDosa=NULL;

C_Combo* C_DosNPoint;
C_Combo* C_NBrd[2];
C_Combo* C_NComp[2];
C_Combo* C_CodComp[2];
C_Combo* C_CodCar[2];
C_Combo* C_Op[2];
C_Combo* C_TipoComp[2];


// Le strutture di dati per I/O su file sono definite in q_tabe.h.

extern struct CfgHeader QHeader;     // struct memo configuraz.
extern struct CfgParam QParam;     // struct memo configuraz.
extern struct vis_data Vision;

struct MountData DMount, *NextDMount;


/* Dichiarazione variabili public per tutte le funzioni del modulo */

int By_step = 0;                       // flag: 1 = assembl. passo-passo

//----- VARIABILI GLOBALI (MODULO) x RIPARTENZA ASS. >> S090102 ---------

unsigned int assrip_component;      //componente da cui ripartire (assemblaggio)
unsigned int dosrip_component;      //componente da cui ripartire (dosaggio)
unsigned int dosrip_point;          //punto di dosaggio da cui ripartire

int          lastpatternEnd=1;      //flag dosaggio terminato per ultimo pattern dosato

unsigned int assembling=0;          //flag assemblaggio in corso (Y/N)
unsigned int dosastate=0;           //flag dosaggio in corso (Y/N)

//SMOD110403
int nomountP2=0;           //flag non montare il comp. letto per la punta2
int lastmountP1=-1;        //n. record ultimo comp. assemblato con punta 1
int lastmountP2=-1;        //n. record ultimo comp. assemblato con punta 2
int lastmount=-1;          //n. record dell'ultimo comp. assemblato
int lastdosa=-1;           //n. record dell'ultimo comp. dosato
int lastdosapoint=-1;      //n. punto dell'ultimo pattern dosato


CDiscardLog assemblyErrorsReport;

CWorkingLog workingLog;


//SMOD260903
int* ComponentList;
unsigned int NComponent;
unsigned int PlacedNComp;


//THFEEDER2
FeederClass* last_advanced_caric = NULL;

// ----------------------------------------------------------------------

float deltaPreDown;
int prePrelQuant[2]={0,0};

int ass_carstatus[2];
CenteringResultData centeringResult[2];

int vismatch_err[2] = { 1, 1 };

// Istanza finestra immissione n. di circuiti in definiz. circuito multiplo.
CWindow* Q_assem;

// Istanza finestra visualizzazione operazioni di dosaggio
CWindow* Q_dosa;

// Puntatore a finestra visualizzazione operazioni di assemblaggio
CWindow* Q_assembla;


//------------------------------------------------------------------------------------------------------------------------
//Prototipi delle funzioni
int ShowDosa_step(const char *msg, int spare1,int spare2);
void CloseAssFiles(void);
int Ass_tot(int restart,int stepmode,int asknewAss=1,int autoref=1,int enableContAss=1);
int assembla(int re_ass = 0);
int wait_to_cont_ass(void);
int ass_las( float PCBh );

void UpdateComponentQuantP1(int& c);
void UpdateComponentQuantP2(int& c);
//------------------------------------------------------------------------------------------------------------------------


//TEST TIMINGS
#ifdef __ASSEMBLY_PROFILER
CTimeProfiling AssemblyProfiling("assembly.prof");
#endif


float GetMaxPlacedComponentHeight(void)
{
	if(Get_AssemblingFlag())
	{
		return maxCompHeight;
	}

	return 0;
}

void DoPredown(int punta,float pos)
{
	if( pos > (PREDEPO_MAX - deltaPreDown) )
	{
		pos = PREDEPO_MAX - deltaPreDown;
	}
	
	PuntaZPosMm( punta, pos, ABS_MOVE );
}

void ResetAssemblyReport(void)
{
	//TODO
	/*
	if( W_Deci( 0, MsgGetString(Msg_01489) ) )
	{
		ResetCenteringErrorsReport( centeringErrorsReport );
	}
	*/
}


//SMOD290703
#define ASSREPORT_TXT1    MsgGetString(Msg_00527)
#define ASSREPORT_TXT2    "ETol"
#define ASSREPORT_TXT3    "E64"
#define ASSREPORT_TXT4    "E98"
#define ASSREPORT_TXT5    "ECheck"
#define ASSREPORT_TXT6    "EOther"
#define ASSREPORT_TXT7    "Total"

void ShowAssemblyReport(void)
{
	//TODO
	/*
	static int alreadyShow=0;
	
	if(alreadyShow)
		return;
	
	alreadyShow=1;
	
	LoadAssemblyReport(AssemblyReport);
	
	CWindow  *Q_AssReport=new CWindow(14,5,66,11,MsgGetString(Msg_01490));
	C_Table *reportTab=new C_Table(2,7,7,2,2,Q_AssReport);

	Q_AssReport->Show();
	
	reportTab->AddCol(1,6,ASSREPORT_TXT1,6,NUM_UINT,0,0,T_ALLIGNC);
	reportTab->AddCol(2,6,ASSREPORT_TXT2,6,NUM_UINT,0,0,T_ALLIGNC);
	reportTab->AddCol(3,6,ASSREPORT_TXT3,6,NUM_UINT,0,0,T_ALLIGNC);
	reportTab->AddCol(4,6,ASSREPORT_TXT4,6,NUM_UINT,0,0,T_ALLIGNC);
	reportTab->AddCol(5,6,ASSREPORT_TXT5,6,NUM_UINT,0,0,T_ALLIGNC);
	reportTab->AddCol(6,6,ASSREPORT_TXT6,6,NUM_UINT,0,0,T_ALLIGNC);
	reportTab->AddCol(7,6,ASSREPORT_TXT7,6,NUM_UINT,0,0,T_ALLIGNC);
	
	reportTab->Activate();
	
	int refresh=1,c;

	do
	{
		if(refresh)
		{
			for(int i=0;i<2;i++)
			{
				reportTab->SetText(i+1,1,i+1);
				reportTab->SetText(i+1,2,AssemblyReport.Etol[i]);
				reportTab->SetText(i+1,3,AssemblyReport.E64[i]);
				reportTab->SetText(i+1,4,AssemblyReport.E98[i]);
				reportTab->SetText(i+1,5,AssemblyReport.ECheck[i]);
				reportTab->SetText(i+1,6,AssemblyReport.Eother[i]);
				reportTab->SetText(i+1,7,AssemblyReport.total[i]);
			}
		
			refresh=0;
		}
	
		c=Handle();
	
		if(c==CTRLF11)
		refresh=1;
		
	} while(c!=ESC);
	
	alreadyShow=0;
	
	delete reportTab;
	delete Q_AssReport;
	*/
}

//setta valore flag assemblaggio in corso
void Set_AssemblingFlag(unsigned int val)
{
	assembling=val;
}

//ritorna valore flag assemblaggio in corso
unsigned int Get_AssemblingFlag(void)
{
	return assembling;
}



// Display dei dati principali durante l'assemblaggio
// ( display altri dati nel ciclo di assemblaggio )
// Modif. by W0298
void Show_dati(int dosaggio = 0)
{
	GUI_Freeze_Locker lock;

	unsigned char punta;
	
	for(punta=0;punta<2;punta++)
	{
		if(!DMount.mount_flag[punta])
		{
			continue;
		}
		
		C_NBrd[punta]->SetTxt(DMount.PrgRec[punta].scheda);
		C_NComp[punta]->SetTxt(DMount.PrgRec[punta].Riga);
		C_CodComp[punta]->SetTxt(DMount.PrgRec[punta].CodCom);
		C_TipoComp[punta]->SetTxt(DMount.PrgRec[punta].TipCom);
	
		if(!dosaggio)
		{
			C_CodCar[punta]->SetTxt( DMount.PrgRec[punta].Caric );
		}
	}
}

// Assemblaggio passo-passo - Display messaggo msg e attesa tasto
// se variabile globale ByStep=1->modo passo-passo on.
// mode=1 mostra msg per entrabe le punte.
int Show_step(const char *msg, int punta,int mode=0)
{
	char buff[23];

	if( !By_step ) //se non passo-passo
	{
		int c = Handle( false );
		if( c != 0 )
		{
			if( c == K_ESC )
			{
				Set_Finec(OFF); // disabilita protezione tramite finecorsa
				
				if(W_Deci(1, MsgGetString(Msg_00219),MSGBOX_YLOW))
				{
					MoveComponentUpToSafePosition(1);   //se abbandono assemblaggio confermato
					MoveComponentUpToSafePosition(2);   //porta punte a posizione di sicurezza
			
					SetNozzleZSpeed_Index( 1, ACC_SPEED_DEFAULT );
					SetNozzleZSpeed_Index( 2, ACC_SPEED_DEFAULT );
					
					PuntaZPosWait(2);
					PuntaZPosWait(1);
				
					Set_Finec(ON);   // abilita protezione tramite finecorsa
					return(0); //ritorna 0->ABORT
				}
				else //abbandona=NO
				{
					Set_Finec(ON); // abilita protezione tramite finecorsa
					return 1; // ritorna 1->CONTINUE
				}
			}
			else
			{
				return 1; //tasto premuto != ESC -> ritorna CONTINUE
			}
		}

		return 1; //nessun tasto premuto -> CONTINUE
	}

	// clear keyboard buffer
	flushKeyboardBuffer();

	//MODO PASSO-PASSO
	
	strncpyQ(buff,msg,22);
	buff[22]=NULL;
	
	//mostra operazione passo-passo
	
	if(!mode)
		C_Op[punta-1]->SetTxt(buff);
	else
	{
		C_Op[0]->SetTxt(buff);
		C_Op[1]->SetTxt(buff);
	}

	while(1)
	{
		int Key_press = Handle();

		if( Key_press == K_ENTER || Key_press == K_ESC )
		{
			//svuota caselle testo operazione
			*buff=NULL;
			C_Op[0]->SetTxt(buff);
			C_Op[1]->SetTxt(buff);

			if( Key_press == K_ESC )
			{
				MoveComponentUpToSafePosition(1);       //porta punte a posizione di sicurezza
				MoveComponentUpToSafePosition(2);

				SetNozzleZSpeed_Index( 1, ACC_SPEED_DEFAULT );
				SetNozzleZSpeed_Index( 2, ACC_SPEED_DEFAULT );

				PuntaZPosWait(2);
				PuntaZPosWait(1);

				return 0; // ritorna ABORT
			}

			return 1; // ritorna 1->CONTINUE
		}
	}

	return 1; // CONTINUE
} // Show_step


//=====================================================================================
// Gestione Dosaggio - SMOD140103

// dosa la prima serie di punti per mandare a regime siringa SMOD140103
#ifndef __DISP2
int Dosa_steady(float X_colla, float Y_colla)
#else
int Dosa_steady(int ndisp,float X_colla, float Y_colla)
#endif
{
	int i;

	#ifdef __DISP2
	Dosatore->SelectCurrentDisp(ndisp);
	#endif
	
	for(i=0; i<CC_Dispenser.NPoint; i++)
	{
		//GF_05_07_2011
		Dosatore->GoPosiz(X_colla,Y_colla+i*CC_Dispenser.TestPointsOffset);   // go pos. punto colla
		Wait_PuntaXY();
	
		Dosatore->WaitAxis();                          // attesa oscillazioni siringa
		if(!ShowDosa_step(S_STEADYDOSA,0,0))
		{
			return(0);
		}
	
		if(!Dosatore->ActivateDot(DOSA_DEFAULTTIME,DOSA_DEFAULTVISC,DOSAPRESS_DEFAULT))
		{
			return(0);
		}
	}
	return(1);
}

struct TabPrg *DosaRecs=NULL;

#ifndef __DISP2
int LoadDosaPrg(void)
#else
int LoadDosaPrg(int ndisp)
#endif
{
	#ifdef __DISP2
	if((ndisp < 1) || (ndisp > 2))
	{
		ndisp = 1;
	}
	#endif
	
	struct TabPrg tmprec;
	int i,recnum=0,count=0;
	
	while(1)
	{
		if(!TPrgDosFile->Read(tmprec,recnum))
		{
			break;
		}
	
		recnum++;
		
		#ifndef __DISP2
		if((!(tmprec.status & DODOSA_MASK)) || (tmprec.status & NOMNTBRD_MASK))
		{
			continue;
		}
		#else
		if(tmprec.status & NOMNTBRD_MASK)
		{
			continue;
		}
		else
		{
			if((ndisp & 1) && !(tmprec.status & DODOSA_MASK))
			{
				continue;
			}
			
			if((ndisp & 2) && !(tmprec.status & DODOSA2_MASK))
			{
				continue;
			}			
		}		
		#endif
	
		count++;
	}
	
	DosaRecs=new struct TabPrg[recnum];
	
	struct TabPrg *tmp=DosaRecs;
	
	for(i=0;i<recnum;i++)
	{
		if(!TPrgDosFile->Read(tmprec,i))
		{
			break;
		}
		
		#ifndef __DISP2
		if((!(tmprec.status & DODOSA_MASK)) || (tmprec.status & NOMNTBRD_MASK))
		{
			continue;
		}
		#else
		if(tmprec.status & NOMNTBRD_MASK)
		{
			continue;
		}
		else
		{
			if((ndisp & 1) && !(tmprec.status & DODOSA_MASK))
			{
				continue;
			}
			
			if((ndisp & 2) && !(tmprec.status & DODOSA2_MASK))
			{
				continue;
			}			
		}		
		#endif
	
		*tmp=tmprec;
		tmp++;
	}
	
	return(count);
}


void InitDosaMask(void)
{
	Q_dosa = new CWindow( 15,6,65,18, MsgGetString(Msg_00638) );
	
	C_NBrd[0] = new C_Combo( 2, 2, MsgGetString(Msg_00190), 8, CELL_TYPE_TEXT );
	C_NComp[0] = new C_Combo( 2, 3, MsgGetString(Msg_00191), 8, CELL_TYPE_TEXT );
	C_DosNPoint = new C_Combo( 2, 4, MsgGetString(Msg_01211), 8, CELL_TYPE_TEXT );
	C_CodComp[0] = new C_Combo( 2, 5, MsgGetString(Msg_00192), 8, CELL_TYPE_TEXT );
	C_TipoComp[0] = new C_Combo( 2, 5, MsgGetString(Msg_00193), 25, CELL_TYPE_TEXT );
	C_Op[0] = new C_Combo( 2, 9, MsgGetString(Msg_00197), 25, CELL_TYPE_TEXT );

	GUI_Freeze_Locker lock;

	Q_dosa->Show();
	C_NBrd[0]->Show( Q_dosa->GetX(), Q_dosa->GetY() );
	C_NComp[0]->Show( Q_dosa->GetX(), Q_dosa->GetY() );
	C_DosNPoint->Show( Q_dosa->GetX(), Q_dosa->GetY() );
	C_CodComp[0]->Show( Q_dosa->GetX(), Q_dosa->GetY() );
	C_TipoComp[0]->Show( Q_dosa->GetX(), Q_dosa->GetY() );
	C_Op[0]->Show( Q_dosa->GetX(), Q_dosa->GetY() );

	Dosatore->SetNPointCombo(C_DosNPoint);
}

void CloseDosaMask(void)
{
	delete C_NComp[0];
	delete C_NBrd[0];
	delete C_CodComp[0];
	delete C_TipoComp[0];
	delete C_Op[0];
	delete C_DosNPoint;
	delete Q_dosa;

	Dosatore->SetXYCombo(NULL,NULL);
	Dosatore->SetNPointCombo(NULL);
}


void SetDosaMask(struct TabPrg rec)
{
	GUI_Freeze_Locker lock;

	C_NComp[0]->SetTxt(rec.Riga);
	C_NBrd[0]->SetTxt(rec.scheda);
	C_CodComp[0]->SetTxt(rec.CodCom);
	C_TipoComp[0]->SetTxt(rec.TipCom);
	C_Op[0]->SetTxt("");
}

int ShowDosa_step(const char *msg, int spare1,int spare2)
{
	int Key_press = 0;
	
	char buff[23];
	
	if(!By_step)               //se non passo-passo
	{
		Key_press = Handle( false );
		if( Key_press != 0 ) //se tasto premuto
		{
			if( Key_press == K_ESC )      //get key code...se tasto=ESC
			{
				Set_Finec(OFF);	   // disabilita protezione tramite finecorsa
		
				if(W_Deci(1,MsgGetString(Msg_00648),MSGBOX_YLOW))
				{
					ResetNozzles();
			
					SetNozzleZSpeed_Index( 1, ACC_SPEED_DEFAULT );
					SetNozzleZSpeed_Index( 2, ACC_SPEED_DEFAULT );
			
					Set_Finec(ON);   // abilita protezione tramite finecorsa
			
					return(0);                //ritorna 0->ABORT
				}
				else                //abbandona=NO
				{
					Set_Finec(ON);   // abilita protezione tramite finecorsa
					return(1);        // ritorna 1->CONTINUE
				}
			}
			else
				return(1); //tasto premuto!=ESC->ritorna CONTINUE
		}
		else
			return(1);   //nessun tasto premuto->ritorna CONTINUE
	}

	// clear keyboard buffer
	flushKeyboardBuffer();
	
	//MODO PASSO-PASSO
	
	strncpyQ(buff,msg,22);
	buff[22]=NULL;
	
	//mostra operazione passo-passo
	
	C_Op[0]->SetTxt(buff);

	while(1)
	{
		Key_press = Handle();

		if( Key_press==K_ENTER || Key_press==K_ESC )
		{
			*buff=NULL;                   //svuota caselle testo operazione
			C_Op[0]->SetTxt(buff);
		}
	
		if(Key_press==K_ESC)       //se premuto esc
		{
			if(W_Deci(1,MsgGetString(Msg_00648),MSGBOX_YLOW))
			{
				ResetNozzles();

				SetNozzleZSpeed_Index( 1, ACC_SPEED_DEFAULT );
				SetNozzleZSpeed_Index( 2, ACC_SPEED_DEFAULT );

				return(0);             //ritorna ABORT
			}
		}
		else if( Key_press == K_ENTER )      //se premuto ENTER
			return 1;           //ritorna 1->CONTINUE
	}
}

// Ritorna ultimo comp. dosato (-1=nessun componente assemblato)
int Get_LastRecDosa(void)
{
	return(lastdosa);
}

// Ritorna flag dosaggio in corso
unsigned int Get_DosaFlag(void)
{
	return(dosastate);
}

int OpenDosaFiles( int ndisp )
{
	#ifndef __DISP2
	ndisp = 1;
	#else
	if( ndisp < 1 || ndisp > 2 )
	{
		ndisp = 1;
	}
	#endif

	int files_stat=1;

	ZerDosa=NULL;
	TPrgDosFile=NULL;

	//apre il file degli zeri
	if(ZerDosa==NULL)
	{
		ZerDosa=new ZerFile(QHeader.Prg_Default);
		
		if(!ZerDosa->Open())
		{
			W_Mess(NOZSCHFILE);
			files_stat=0;
		}
	}

	//apre il file di dosaggio
	if(TPrgDosFile==NULL)
	{
		TPrgDosFile=new TPrgFile(QHeader.Prg_Default,PRG_DOSAT);
	
		if(!TPrgDosFile->Open(SKIPHEADER))              // open del file prog.
		{
			files_stat=0;
		}
	}

	//apre il file dei caricatori
	if(CarFile==NULL)
	{
		CarFile=new FeederFile(QHeader.Conf_Default);
		if(!CarFile->opened)
		{
			files_stat=0;
		}
	}

	// carica libreria packages
	if( !PackagesLib_Load( QHeader.Lib_Default ) )
	{
		files_stat = 0;
	}

	//apre dati di dosaggio per i package
	if(!Dosatore->OpenPackData(ndisp,QHeader.Lib_Default))
	{
		files_stat=0;
	}
	
	return(files_stat);
}// OpenDosaFiles

void CloseDosaFiles(int ndisp)
{
	#ifndef __DISP2
	ndisp = 1;
	#else
	if((ndisp < 1) || (ndisp > 2))
	{
		ndisp = 1;
	}
	#endif	
	
	if(CarFile!=NULL)
		delete CarFile;
	if(TPrgDosFile!=NULL)
		delete TPrgDosFile;
	if(ZerDosa!=NULL)
		delete ZerDosa;
	
	ZerDosa=NULL;
	TPrgDosFile=NULL;
	CarFile=NULL;
	
	if(DosaRecs!=NULL)
	{
		delete[] DosaRecs;
		DosaRecs=NULL;
	}
	
	Dosatore->ClosePackData(ndisp);
	
	DosaRecs=NULL;
} // CloseDosaFiles

#ifndef __DISP2
int Dosaggio(int ripart,int stepmode,int nrestart,int interruptable,int asknewDosa,int autoref,int enableContDosa,int disableStartPoints)
#else
int Dosaggio(int ndisp,int ripart,int stepmode,int nrestart,int interruptable,int asknewDosa,int autoref,int enableContDosa,int disableStartPoints)
#endif		
{
	int files_stat=1;
	int abort_flag=0;
	int pann_on=0;
	int nrecs,dos_rec;
	
	struct TabPrg *CurRec;
	struct CarDat tmpcaric;
	struct Zeri   ZerData[2];
	
	float px,py,angle,rotaz;
	float startx,starty;
	
	#ifndef __DISP2
	int ndisp = 1;
	#endif
	
	Dosatore->ReadCurConfig();
	Dosatore->GetConfig(ndisp,CC_Dispenser);

	if( (files_stat = OpenDosaFiles(ndisp)) )
	{
		if(ZerDosa->GetNRecs()<2)
		{
			CloseDosaFiles(ndisp);
			return(0);
		}

		//print_debug("files_stat=%d\n",files_stat);
		#ifndef __DISP2
		if(!(nrecs=LoadDosaPrg()))
		#else
		if(!(nrecs=LoadDosaPrg(ndisp)))
		#endif
		{
			files_stat=0;
		}
		//print_debug("files_stat=%d\n",files_stat);
	}

	if(!files_stat)
	{
		CloseDosaFiles(ndisp);
		return(0);
	}

	//SMOD310506-FinecFix
	EnableForceFinec(ON);     // forza check finecorsa ad on impedendo cambiamenti

	#ifdef __DISP2
	Dosatore->SelectCurrentDisp(ndisp);
	#endif	

	// Ricerca riferimenti scheda in automatico
	if((QParam.Autoref && !ripart) && autoref)
	{
		if(!Exist_ZRefPanelFile())
		{
			if(Z_sequenza(0,1))
			{
				CloseDosaFiles(ndisp);
			
				//SMOD310506-FinecFix
				DisableForceFinec();
				Set_Finec(OFF);
				
				return(0);
			}

			if(Z_sequenza(1,1))
			{
				CloseDosaFiles(ndisp);
		
				//SMOD310506-FinecFix
				DisableForceFinec();
				Set_Finec(OFF);
				
				return(0);
			}
		}
		else
		{
			if(!Z_AppMC())
			{
				DisableForceFinec();
				Set_Finec(OFF);
				
				return(0);
			}
		}
	}

	//SMOD310506-FinecFix
	DisableForceFinec();
	Set_Finec(OFF);
	
	ZerDosa->Read(ZerData[0],0);
	
	if(ripart)
	{
		if(nrestart==-1)
		{
			if(!dosastate)
			{
				W_Mess( MsgGetString(Msg_00647) );
			}

			if(lastdosa!=-1)
			{
				if(lastpatternEnd)
				{
					dosrip_component=lastdosa+1;
					dosrip_point=0;
				}
				else
				{
					dosrip_component=lastdosa;
					dosrip_point=lastdosapoint+1;
				}
			}
			else
			{
				dosrip_component=0;
				dosrip_point=0;
			}

			int tmp=dosrip_component;
			
			if(!Set_RipData(dosrip_component,TPrgDosFile->Count(),RIP_DOSA_TITLE))
			{
				CloseDosaFiles(ndisp);
				return(0);
			}

			if(tmp!=dosrip_component)
				dosrip_point=0;
		}
		else
		{
			dosrip_component=nrestart;
			dosrip_point=0;
		}
	}
	else
	{
		dosrip_component=0;
		dosrip_point=0;
	}

	Dosatore->Set_StepF(ShowDosa_step);
	
	InitDosaMask();
	
	By_step=stepmode;
	
	if(interruptable==-1)
		Dosatore->SetInterruptable(stepmode);
	else
		Dosatore->SetInterruptable(interruptable);
	
	dosastate=1;
	
	#ifdef __DISP2
	int vacuopulse_done=0;
	#endif	
	
	while(1)
	{
		//SMOD310506-FinecFix
		EnableForceFinec(ON);
		
		CurRec=DosaRecs+dosrip_component;
		abort_flag=0;
		
		#ifdef __DISP2
		vacuopulse_done=0;
		#endif		
	
		if(!(enableContDosa && QParam.AS_cont))
		{
			if(!disableStartPoints)
			{
				#ifndef __DISP2
				Read_pcolla(startx,starty);
				if(!Dosa_steady(startx,starty))
				#else
				Read_pcolla(ndisp,startx,starty);
				if(!Dosa_steady(ndisp,startx,starty))
				#endif
				{
					abort_flag=1;
					break;
				}
			}
		}

		for(dos_rec=dosrip_component;dos_rec<nrecs;dos_rec++)
		{
			if(CurRec->Caric <= 0)
			{
				CurRec++;
				continue;
			}
			

			CarFile->Read(CurRec->Caric,tmpcaric); //legge dati caricatore
			if( tmpcaric.C_PackIndex > 0 && tmpcaric.C_PackIndex <= MAXPACK )
			{
				if( !Dosatore->LoadPattern( currentLibPackages[tmpcaric.C_PackIndex-1].code-1 ) )
				{
					CurRec++;
					continue;
				}
			}
			else
			{
				CurRec++;
				continue;
			}

	
			SetDosaMask(*CurRec);

			rotaz = CurRec->Rotaz;
		
			if( currentLibPackages[tmpcaric.C_PackIndex-1].centeringMode == CenteringMode::SNIPER )
			{
				rotaz += currentLibOffsetPackages[tmpcaric.C_PackIndex-1].angle;
			}
		
			ZerDosa->Read(ZerData[1],CurRec->scheda);
		
			if(!ZerData[1].Z_ass)
			{ 
				CurRec++;
				continue;
			}
	
			Get_PosData(ZerData,*CurRec,NULL,0,0,px,py,angle,1);
		

			Dosatore->CreateDosPattern(rotaz+angle);
		
			#ifdef __LINEDISP
			Dosatore->EnableMoveDown();
			#endif
		
			if(!Dosatore->DoPattern(px,py,dosrip_point))
			{
				abort_flag=1;
		
				#ifdef __LINEDISP
				Dosatore->DisableMoveDown();
				#endif
				
				break;
			}
			////

			#ifdef __LINEDISP
			Dosatore->DisableMoveDown();
			#endif
		
			dosrip_point=0;
		
			CurRec++;
		}

		Dosatore->GoUp();
		Dosatore->SetPressOff(ndisp);
	
		if(abort_flag)
		{
			lastdosapoint=Dosatore->GetLastPoint();
		
			if(!stepmode || (stepmode && (lastdosapoint==Dosatore->GetPatternNPoint())))
			{
				lastpatternEnd=1;
				lastdosa=dos_rec;
			}
			if(stepmode && (lastdosapoint<Dosatore->GetPatternNPoint()))
			{
				lastdosa=dos_rec;
				lastpatternEnd=0;
			}
		}
	
		if(!abort_flag || (dos_rec==nrecs-1 && lastpatternEnd))
		{
			dosastate=0;
			lastdosa=-1;
			lastdosapoint=-1;
			lastpatternEnd=1;
		}
	
		//SMOD310506-FinecFix
		DisableForceFinec();
	
		Set_Finec(ON);
		// go fuori scheda
		NozzleXYMove( QParam.X_endbrd, QParam.Y_endbrd );
		Wait_PuntaXY();
		Set_Finec(OFF);

		if(abort_flag)
		{
			break;
		}
		
		#ifdef __DISP2
		Dosatore->DoVacuoFinalPulse(ndisp);
		vacuopulse_done=1;
		#endif

		if(!(enableContDosa && QParam.AS_cont))
		{
			if(asknewDosa)
			{
				wait_to_cont_ass_internal_state = protection_state_wait_transition;
				int ret = W_Deci(1,MsgGetString(Msg_00649),MSGBOX_YCENT,0,0,GENERIC_ALARM);
				/*
				int ret = W_Deci(1,MsgGetString(Msg_00649),MSGBOX_YCENT,wait_to_cont_ass_protection_check_polling_loop,0,GENERIC_ALARM);
				if(ret)
				{
					if(wait_to_cont_ass_internal_state == protection_state_wait_transition)
					{
						ret = CheckProtectionIsWorking();
					}
				}
				*/
				if(!ret)
				{
					break;
				}
			}
			else
			{
				break;
			}
		}

		dosrip_component=0;
		dosrip_point=0;
	}

	#ifdef __DISP2
	if(!vacuopulse_done)
	{
		Dosatore->DoVacuoFinalPulse(ndisp);
	}
	#endif

	CloseDosaMask();	

	ResetNozzles();
	if(pann_on)
	{
		pann_on=0;
	}

	Dosatore->Set_StepF(NULL);

	CloseDosaFiles(ndisp);

	return(!abort_flag);
}

int ass_avcaric(int punta)
{
	if(DMount.Caric[punta-1]==NULL) //esce se la classe non e stata inizializzata
		return(0);
	
	//THFEEDER2
	if( last_advanced_caric != NULL )
	{
		last_advanced_caric->WaitReady();
	}
	
	if( DMount.Caric[punta-1]->Avanza() )
	{
		ass_carstatus[punta-1]=1; //setta variabile globale caric. avanzato

		C_CodCar[punta-1]->SetTxt( DMount.PrgRec[punta-1].Caric );
	
	} // Caricatore avanzato
	
	//THFEEDER2
	CarDat data = DMount.Caric[punta-1]->GetData();
	if( data.C_tipo == CARTYPE_THFEEDER )
		last_advanced_caric = DMount.Caric[punta-1];
	else
		last_advanced_caric = NULL;
	
	return(1);
}

//THFEEDER
int ass_waitcaric(int punta)
{
	if(DMount.Caric[punta-1]==NULL) //esce se la classe non e stata inizializzata
		return 0;
	
	return DMount.Caric[punta-1]->WaitReady();
}

//THFEEDER
int ass_checkcaric(int punta)
{
	if(DMount.Caric[punta-1]==NULL) //esce se la classe non e stata inizializzata
		return 0;
	
	return DMount.Caric[punta-1]->CheckReady();
}


//----------------------------------------------------------------------------

// Reset esterno dei valori compon./scheda di ripartenza assembl.
// ( per caricamento nuovo programma/tabella zeri
//===============================================================================
void ri_reset(void)
{
	assembling=0;
	assrip_component=0;

	lastmount=-1;
	lastmountP1=-1;
	lastmountP2-=1;
	
	dosaAss=0;  

	#ifdef __DISP2
	dosa12Ass=0;
	Dosa12Ass_phase=0;
	memset(Dosa12Ass_seqvector,DOSASS_IDX_NONE,sizeof(Dosa12Ass_seqvector));
	#endif	
}

// Apre tutti i file necessari al ciclo di assemblaggio
//===============================================================================
//TODO: da rivedere
int OpenAssFiles()
{
	int files_stat = 1;

	if( Get_WriteDiscardLog() )
	{
		assemblyErrorsReport.Load();
	}

	if( Get_WorkingLog() )
	{
		workingLog.Load();
	}

	//-----------------------------------------------------------------------
	
	if(ZerAssem==NULL)
	{
		ZerAssem=new ZerFile(QHeader.Prg_Default);
		
		if(!ZerAssem->Open())
		{
			W_Mess(NOZSCHFILE);
			files_stat=0;
		}
	}

	//-----------------------------------------------------------------------

	if(TPrgAssem==NULL)
	{
		TPrgAssem=new TPrgFile(QHeader.Prg_Default,PRG_ASSEMBLY);
		
		if(!TPrgAssem->Open(SKIPHEADER))
			files_stat=0;
	}

	//-----------------------------------------------------------------------
	
	if( CarFile == NULL )
	{
		CarFile = new FeederFile( QHeader.Conf_Default, false );
		DMount.Caric[0] = NULL;
		DMount.Caric[1] = NULL;
		if( !CarFile->opened )
			files_stat = 0;
	}

	//-----------------------------------------------------------------------

	if( !PackagesLib_Load( QHeader.Lib_Default ) )
	{
		files_stat = 0;
	}
	
	//-----------------------------------------------------------------------

	return files_stat;
}

// Chiude i files utilizzati dal ciclo di assemblaggio e dealloca oggetti
// caricatori
//===============================================================================
//SMOD060504
void CloseAssFiles(void)
{
	if(DMount.Caric[0]!=NULL)
	{
		delete DMount.Caric[0];
	
		if(DMount.Caric[0]==DMount.Caric[1])
		{
			DMount.Caric[1]=NULL;
		}
	
		DMount.Caric[0]=NULL;
	}
	
	if(DMount.Caric[1]!=NULL)
	{
		delete DMount.Caric[1];
		DMount.Caric[1]=NULL;
	}
	
	if(CarFile!=NULL)
	{
		delete CarFile;
		CarFile=NULL;
	}
	
	if(TPrgAssem!=NULL)
	{
		delete TPrgAssem;
		TPrgAssem=NULL;
	}
	
	if(ZerAssem!=NULL)
	{
		delete ZerAssem;
		ZerAssem=NULL;
	}
}

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
float Get_PhysThetaPlacePos( float compRot, const SPackageData& pack )
{
	float phys_theta_place_pos = compRot;

	switch( pack.centeringMode )
	{
		case CenteringMode::NONE:
		case CenteringMode::SNIPER:
			phys_theta_place_pos = compRot - pack.orientation + currentLibOffsetPackages[pack.code-1].angle;
			break;
		case CenteringMode::EXTCAM:
			phys_theta_place_pos = -90 + compRot + pack.extAngle + currentLibOffsetPackages[pack.code-1].angle;
			break;
		default:
			break;
	}

	return phys_theta_place_pos;
}

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
float Get_PickThetaPosition( int caric_code, const SPackageData& pack )
{
	float rot;
	
	switch( pack.centeringMode )
	{
		case CenteringMode::NONE:
		case CenteringMode::SNIPER:
			//120410
			//con questa rotazione dopo la prerotazione la punta si trovera' sempre a 90-prerotazione per
			//componenti con angolo sniper pari a 0 o a 90-prerotazione per i componenti con angolo sniper 90
			//NOTA: per angolo sniper si intende l'angolo memorizzato nel package (interoperabile tra macchine
			//DVC e laser; l'angolo effettivo utilizzato differisce di 90 gradi)
			//Non e' possibile fare in modo che la punta si trovi sempre a -prerotazione indipendentemente dall'angolo
			//sniper, come avviene su macchine Laser perche' altrimenti la rotazione di prelievo della punta sarebbe
			//diversa tra macchine Laser e DVC rendendo impossibile lo scambio di ugelli polarizzati tra le due
			//tipologie di macchine. Il centraggio con visione preruota la punta prima del prelievo. secondo
			//lo stesso criterio usato dal centraggio con laser; non uniformando la prerotazione di prelievo dunque
			//non sarebbe neanche possibile poter prelevare un componente con un ugello polarizzato e centrarlo
			//sia con sniper che con centratore ottico esterno.

			rot = -angleconv( caric_code, pack.orientation );
			break;
		case CenteringMode::EXTCAM:
			rot = -angleconv( caric_code, 90-pack.extAngle ); //VISANGLE
			break;
		default:
			rot = 0;
			break;
	}

	return rot;
}


//-------------------------------------------------------------------------------
// Returns the board rotation angle in Radians/Degrees
//-------------------------------------------------------------------------------
float GetBoardRotation_Rad( const Zeri& master, const Zeri& board )
{
	return board.Z_rifangle - master.Z_rifangle;
}

float GetBoardRotation_Deg( const Zeri& master, const Zeri& board )
{
	return RTOD( GetBoardRotation_Rad( master, board ) );
}


//-------------------------------------------------------------------------------
// Returns the component absolute position with/without placement mapping
//-------------------------------------------------------------------------------
void GetComponentPlacementPosition( const Zeri& master, const Zeri& board, const TabPrg& tab, const SPackageData& pack, float& outx, float& outy, bool map_enable = true )
{
	float scaledX = tab.XMon * master.Z_scalefactor;
	float scaledY = tab.YMon * master.Z_scalefactor;

	float segment = sqrt( pow(scaledX,2) + pow(scaledY,2) );
	float beta = atan2( scaledY, scaledX ) + GetBoardRotation_Rad( master, board );

	// coord. componente ruotato rispetto a master
	outx = segment * cos(beta);
	outy = segment * sin(beta);

	// coord. componente ruotato rispetto a board
	outx += board.Z_xzero;
	outy += board.Z_yzero;

	// correzione mappatura deposito
	if( map_enable )
	{
		float phys_theta_place_pos = Get_PhysThetaPlacePos( tab.Rotaz, pack );
		phys_theta_place_pos += GetBoardRotation_Deg( master, board );

		// lettura valori per mappatura di deposito
		float x_offs = 0.f, y_offs = 0.f;
		Read_offmmN( x_offs, y_offs, tab.Punta-'0', phys_theta_place_pos );

		outx -= x_offs;
		outy -= y_offs;
	}
}


//TODO: da rivedere
void Get_PosData(struct Zeri* zeri,const struct TabPrg& tab, const struct SPackageData* pack, float deltax,float deltay,float &outx,float &outy,float &board_rot,int assdos_flag,int map_enable)
{
	float cx_oo,cy_oo;
	float diff_x, diff_y;
	float x_oo, y_oo;
	float x_cc, y_cc;
	
	float x_prod, y_prod, arctan;
	float beta1, a_segment;

	float x_offs, y_offs;
	int punta=1;

	float scaledX = tab.XMon * zeri[0].Z_scalefactor;
	float scaledY = tab.YMon * zeri[0].Z_scalefactor;

	if(!assdos_flag)
	{
		punta = tab.Punta-'0';     //modo assemblaggio
	}

	diff_x = zeri[punta].Z_xzero - zeri[0].Z_xzero;
	diff_y = zeri[punta].Z_yzero - zeri[0].Z_yzero;

	x_oo=zeri[0].Z_xzero;                      // coord. zero master
	y_oo=zeri[0].Z_yzero;                      //   (assolute)

	x_cc=scaledX+deltax+x_oo;                 // coord. comp. master
	y_cc=scaledY+deltay+y_oo;                 // (assolute)

	y_prod=(double)(y_cc-y_oo);
	x_prod=(double)(x_cc-x_oo);

	// esegui ricalcolo se componente non su zero scheda
	// e se zero e fiferimento master non coincidenti o troppo vicini
	// e se zero e fiferimento scheda non coincidenti o troppo vicini
	if ((fabs(y_cc-y_oo)>0.01 || fabs(x_cc-x_oo)>0.01) &&
		(fabs(zeri[0].Z_xrif-zeri[0].Z_xzero)>MINXAPP ||
		fabs(zeri[0].Z_yrif-zeri[0].Z_yzero)>MINYAPP) &&
		(fabs(zeri[punta].Z_xrif-zeri[punta].Z_xzero)>MINXAPP ||
		fabs(zeri[punta].Z_yrif-zeri[punta].Z_yzero)>MINYAPP))
	{ //L704a

		arctan=atan2((y_cc-y_oo),(x_cc-x_oo)); // angolo comp. master
		beta1 =arctan+(zeri[punta].Z_rifangle-zeri[0].Z_rifangle);// angolo c.ruot.
		a_segment=sqrt(pow(x_prod,2)+pow(y_prod,2));

		cx_oo=(float)(a_segment*cos(beta1))+diff_x+x_oo;// posiz. reale comp.
		cy_oo=(float)(a_segment*sin(beta1))+diff_y+y_oo;// su board 1
	}
	else
	{
		cx_oo=(float)(x_cc+diff_x);   // comp. sullo zero, no rotaz.
		cy_oo=(float)(y_cc+diff_y);   // solo translaz.
	}

	board_rot = 180/PI * (zeri[punta].Z_rifangle-zeri[0].Z_rifangle);

	if(!assdos_flag)
	{
		if(tab.Punta=='1')
		{
			cx_oo+=QParam.CamPunta1Offset_X;
			cy_oo+=QParam.CamPunta1Offset_Y;
		}
		else
		{
			cx_oo+=QParam.CamPunta2Offset_X;
			cy_oo+=QParam.CamPunta2Offset_Y;
		}

		if( map_enable )
		{
			float phys_theta_place_pos;
			if( pack )
			{
				phys_theta_place_pos = Get_PhysThetaPlacePos( tab.Rotaz, *pack );
			}
			else
			{
				phys_theta_place_pos = tab.Rotaz;
			}
			
			phys_theta_place_pos += board_rot; //TODO

			// lettura valori per mappatura offset teste
			int centeringMode = 0;
			if( pack->centeringMode == CenteringMode::EXTCAM )
				centeringMode = 1;
			Read_offmmN(x_offs,y_offs,tab.Punta-'0',phys_theta_place_pos,centeringMode);
			
			//out dei dati
			outx=cx_oo-x_offs;
			outy=cy_oo-y_offs;
		}
		else
		{
			outx=cx_oo;
			outy=cy_oo;
		}
	}
	else
	{
		//CfgDispenser DosaCfg;              //dosaggio
		//Dosatore->GetConfig(DosaCfg);
		outx=cx_oo;
		outy=cy_oo;
	}
}


float MaxUsedTrayHeight(void)
{
	float max = 0;

	int nrec = TPrgAssem->Count();

	struct TabPrg rec;
	struct CarDat car;

	for(int i = 0; i < nrec; i++)
	{
		if(!(TPrgAssem->Read(rec,i)))
		{
			break;
		}

		if((!(rec.status & MOUNT_MASK)) || (rec.status & NOMNTBRD_MASK))
		{
			continue;
		}

		struct Zeri board;

		ZerAssem->Read(board,rec.scheda);

		if(!board.Z_ass)
		{
			continue;
		}

		if(rec.Caric >= FIRSTTRAY)
		{
			CarFile->Read(rec.Caric,car);

			if(max < car.C_offprel)
			{
				max = car.C_offprel;
			}
		}    
	}

	return max;
  
}

//cerca a partire dal record rcount, due componenti che possono essere
//montati contemporaneamente. Se non vengono trovati, cerca il primo da montare
//ed esce.
//Ritorna in rcount il prossimo record da cui iniziare a leggere
//===============================================================================
int ass_readcomp(struct MountData *data,int &rcount)
{
	struct TabPrg tmpprg;
	struct CarDat tmpcaric;
	int punta,uge_index,prevpunta,prevuge,prevcentr;

	int car[2]={-1,-1}; //SMOD060504

	//ALLOCAZIONE OGGETTI CARICATORI
	//-----------------------------------------------------------
	//se oggetti caricatori sono allocati->dealloca oggetti
	if(data->Caric[0]!=NULL) //###MSS SMOD060504
	{
		delete data->Caric[0];

		if(data->Caric[0]==data->Caric[1])
		{
			data->Caric[1]=NULL;
		}
	}

	if(data->Caric[1]!=NULL)
	delete data->Caric[1];

	//alloca oggetti caricatori (uno per punta)
	data->Caric[0]=new FeederClass(CarFile);
	data->Caric[1]=new FeederClass(CarFile);

	//-----------------------------------------------------------
	//LETTURA DA FILE DEI RECORD
	//-----------------------------------------------------------
	data->mount_flag[0]=0;
	data->mount_flag[1]=0;

	prevpunta=-1;
	prevuge=-1;
	prevcentr = -1;
	while(1)
	{
		if(!(TPrgAssem->Read(tmpprg,rcount)))     //legge un record
		{
			break; //se fine file esce dal ciclo
		}

		if((!(tmpprg.status & MOUNT_MASK)) || (tmpprg.status & NOMNTBRD_MASK))
		{
			rcount++;
			continue;
		}
		
		punta=tmpprg.Punta-'1';               //estrae n.punta
		if(punta==prevpunta || prevuge==tmpprg.Uge)
		{
			break;
		}

		rcount++;
	
		ZerAssem->Read(data->ZerRecs[punta+1],tmpprg.scheda);
	
		if((tmpprg.status & MOUNT_MASK) && data->ZerRecs[punta+1].Z_ass)  //se record letto=da montare
		{
			CarFile->Read(tmpprg.Caric,tmpcaric); //legge dati caricatore
	
			//se il caricatore ha un package associato
			if(tmpcaric.C_PackIndex>0 && tmpcaric.C_PackIndex<=MAXPACK)
			{
				data->mount_flag[punta]=1;                                    //setta mount flag per la punta
				memcpy((data->PrgRec)+punta,&tmpprg,sizeof(struct TabPrg));     //carica dati prg x punta
				memcpy((data->CarRec)+punta,&tmpcaric,sizeof(struct CarDat)); //carica dati prg x punta
	
				//carica dati package
				data->Package[punta] = currentLibPackages[data->CarRec[punta].C_PackIndex-1];

				if( prevcentr != -1 )
				{
					int single_nozzle = 0;
					
					switch( data->Package[punta].centeringMode )
					{
						case CenteringMode::NONE:
						case CenteringMode::SNIPER:
							//centraggio laser/sniper o nessuno
							if( prevcentr != CenteringMode::SNIPER && prevcentr != CenteringMode::NONE )
							{
								//se per l'altra punta il centraggio non e' laser/sniper o nessuno
								//assembla la punta precedente a monopunta
								single_nozzle = 1;
							}
							break;
						case CenteringMode::EXTCAM:
							#ifdef __SNIPER
							if( prevcentr != CenteringMode::EXTCAM )
							{
								//per la macchina sniper e' permesso l'assemblaggio in coppia di due
								//componenti con centratore ottico: tutte le altre combinazioni sono
								//permesse solo a monopunta
								single_nozzle = 1;
							}
							#endif
							break;
					}
					
					if(single_nozzle)
					{
						//non e' possibile assemblare i componenti insieme: monopunta
						//imposta come da non assemblare
						data->mount_flag[punta]=0;
						//riporta indietro di 1 il puntatore nel file del programma di assemblaggio
						rcount--;
						//termina ciclo
						break;
					}
				}

				//carica dati ugelli
				uge_index=punta;
				if(strlen(data->Package[punta].tools)<=uge_index)
				{
					uge_index=strlen(data->Package[punta].tools)-1;
				}
				
				if(uge_index!=-1)
				{
					Ugelli->ReadRec(data->Uge[punta],data->Package[punta].tools[uge_index]-'A');
				}
				else
				{
					Ugelli->ReadRec(data->Uge[punta],'A');
				}

				//associa numero caricatore ad oggetto caricatore
				data->Caric[punta]->SetCode(tmpprg.Caric);
		
				car[punta]=tmpprg.Caric; //SMOD060504
		
				prevpunta=punta;
				prevuge=tmpprg.Uge;
				prevcentr = data->Package[punta].centeringMode;
				
				data->pick_angle[punta] = Get_PickThetaPosition( data->PrgRec[punta].Caric, data->Package[punta] );
			
				punta++;

				if(punta==2) //se punta=2
				{
					break;     //fine ciclo
				}
			}
		}
	}

	//se nessun componente da montare trovato
	if(!(data->mount_flag[0] || data->mount_flag[1]))
	{
		return -1; //ritorna EOF condition
	}

	if(data->mount_flag[0] && data->mount_flag[1])
	{
		//se i caricatori per le due punte sono uguali
		if((car[0]==car[1]) && (car[0]!=-1))
		{
			//non e' necessario mantenere due oggetti caricatori separati
			//si dealloca una delle istanze
			delete data->Caric[1];
	
			data->Caric[1]=data->Caric[0];
		}
	}

	return 1;
}

int AssScaricaComp( int nozzle, int mode/*=SCARICACOMP_NORMAL*/, const char* cause/*=NULL*/ )
{
	if( nozzle != 1 &&  nozzle != 2 )
	{
		nozzle = 1;
	}

	// serve per evitare di inviare un comando di movimento con la testa gia' in moto
	Wait_PuntaXY();

	SPackageData pack;
	if( DMount.PrgRec[nozzle-1].Caric >= FIRSTTRAY && GetPackageOnNozzle( nozzle, pack ) )
	{
		//scarica componente su vassoio
		if( !ScaricaCompOnTray(DMount.Caric[nozzle-1],nozzle,prePrelQuant[nozzle-1],mode,cause) )
		{
			return 0;
		}
	}
	else
	{
		//scarica componente
		if( !ScaricaComp(nozzle,mode,cause) )
		{
			return 0;
		}
	}

	return 1;
}


// Funzione che, dato un angolo in ingresso (espresso in gradi...), ritorna la correzione
// da dare (in passi) in base alla tabella di mappatura rotazione
int correct_rotation( float angle, unsigned int nozzle, int device )
{
	int angle_corr = 0;
	
	while( angle > 360 )
		angle -= 360;
	while( angle < 0 )
		angle += 360;

	if((angle>=45) && (angle<135))
	{
		angle_corr = (nozzle == 1) ? MapTeste.T1_90[device] : MapTeste.T2_90[device];
	}
	else if((angle>=135) && (angle<225))
	{
		angle_corr = (nozzle == 1) ? MapTeste.T1_180[device] : MapTeste.T2_180[device];
	}
	else if((angle>=225) && (angle<315))
	{
		angle_corr = (nozzle == 1) ? MapTeste.T1_270[device] : MapTeste.T2_270[device];
	}
	else
	{
		angle_corr = (nozzle == 1) ? MapTeste.T1_360[device] : MapTeste.T2_360[device];
	}
	
	return angle_corr;
}



//Assembla il record letto con telecamera esterna
//===============================================================================
#define VISCENT_MAX_THETA_CORRECTION_1     10.0
#define VISCENT_MAX_THETA_CORRECTION        4.0
#define VISCENT_MAX_THETA_CORRECTION_LAST   0.5

int ass_vis( float PCBh )
{
	int perso_flag,NoPreso,retval=1;

	float xmatch[2];
	float ymatch[2];
	float dx[4],dy[4];
	float board_rotation[2];
	float place_xcoord[2];
	float place_ycoord[2];

	for(int nc = 0; nc < 2 ; nc++)
	{
		if(!DMount.mount_flag[nc])
		{
			continue;
		}

		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

		//get coordinate reali di assemblaggio
		Get_PosData( DMount.ZerRecs,DMount.PrgRec[nc], &DMount.Package[nc], 0,0,place_xcoord[nc],place_ycoord[nc],board_rotation[nc],0,1);
 
		//vai a posiz. di prelievo
		if( !DMount.Caric[nc]->GoPos( nc+1 ) )
			return 0;
		
		ASSEMBLY_PROFILER_MEASURE("Goto pick %d position",nc+1);
	
		if( !Show_step(S_GOPREL,nc+1) )
			return(0);
	
		//attiva contropressione prima di prelevare il comp.
		Prepick_Contro(nc+1);
	
		struct PrelStruct preldata=PRELSTRUCT_DEFAULT;
	
		if(DMount.PrgRec[nc].Caric < FIRSTTRAY)          //se non vassoi
		{
			preldata.zprel=GetZCaricPos(nc+1) - DMount.Caric[nc]->GetDataConstRef().C_offprel;  // n. corsa prelievo
		}
		else
		{
			preldata.zprel=GetPianoZPos(nc+1) - DMount.Caric[nc]->GetDataConstRef().C_offprel;  // n. corsa prelievo
			//attiva vuoto
			Set_Vacuo(nc+1,ON);
			if(!Show_step(S_VACUON,nc+1)) return(0);
		}

		preldata.punta = nc+1;
		
		//se il componente ha il controllo presenza componente attivo e diverso da solo vuoto, forza a controllo 
		//con il solo vuoto
		if( DMount.Package[nc].checkPick != 'X' && DMount.Package[nc].checkPick != 'V' )
		{
			DMount.Package[nc].checkPick = 'V';
		}
		
		#ifdef __SNIPER
		if((nc == 1) || !DMount.mount_flag[(nc==0) ? 1 : 0])
		{
			//se singola punta o la punta attuale e' la seconda,
			
			if( -angleconv(DMount.Caric[nc]->GetCode(), 90-DMount.Package[nc].extAngle ) == -90 ) //VISANGLE
			{
				//dopo il prelievo il componente non deve ruotare : porta alla quota di sicurezza
				preldata.zup = GetComponentSafeUpPosition( DMount.Package[nc] );
			}
			else
			{
				//porta alla punta alla quota di rotazione
				preldata.zup = GetZRotationPointForPackage( DMount.Package[nc] );
			}
		}
		else
		{
			//altrimenti porta il componente in posizione sicura ma dove la rotazione potrebbe non essere possibile:
			//verra' eseguita successivamente
			preldata.zup = GetComponentSafeUpPosition( DMount.Package[nc] );
		}
		#endif
		
		preldata.stepF = Show_step;
		preldata.package = &DMount.Package[nc];
		preldata.caric = DMount.Caric[nc];

		SetNozzleRotSpeed_Index( nc, DMount.Package[nc].speedRot );

		while(1)
		{
			//inzializza flag componente perso
			perso_flag=0;
			
			//inizia ciclo tentativi prelievo componente
			do
			{
				//attende punta ferma in theta
				while(!Check_PuntaRot(nc+1))
				{
					delay( 1 );
				}
				
				ASSEMBLY_PROFILER_MEASURE("Wait pre-pick %d theta stop",nc+1);
			
				Wait_PuntaXY();
				
				ASSEMBLY_PROFILER_MEASURE("Wait pick xy stop %d",nc+1);
			
				//THFEEDER
				if( !ass_waitcaric(nc+1) )
				{
					return(0);
				}
		
				ASSEMBLY_PROFILER_MEASURE("Wait pick feeder ready %d",nc+1);
				prePrelQuant[nc]=DMount.Caric[nc]->GetQuantity();
	
				//preleva componente
				if(!PrelComp(preldata))
				{
					return(0);
				}

				//decrementa numero di componenti presenti sul caricatore
				DMount.Caric[nc]->DecNComp();
			
				//check componente preso
				if(!Show_step(S_CHKPRESO,nc+1)) return(0);

				NoPreso = Check_CompIsOnNozzle( nc+1, DMount.Package[nc].checkPick );
				ASSEMBLY_PROFILER_MEASURE("Check if component %d has been picked",nc+1);

				//se comp. preso
				if(!NoPreso)
				{
					//disattiva flag com. perso
					perso_flag=0;
					
					//FEEDER_ADVANCE_FIX : componente preso->setta caricatore da avanzare
					ass_carstatus[nc]=0;
					
					if(nc == 0)
					{
						if(DMount.Caric[1] != NULL)
						{
							if(DMount.Caric[0]->GetDataConstRef().C_codice == DMount.Caric[1]->GetDataConstRef().C_codice)
							{
								ass_avcaric(2);           //avanza caricatore
								ASSEMBLY_PROFILER_MEASURE("feeder 2 advance");
					
								if(!Show_step(S_CARAVANZ,2)) return(0);
								ass_carstatus[0]=1;
								ass_carstatus[1]=1;
								
								//rilegge dati caricatore per punta 2
								DMount.Caric[1]->ReloadData();
							}
						}
					}
				}
				else
				{
					//reload feeder data
					DMount.Caric[nc]->ReloadData();

					perso_flag++;  //incrementa contatore perdite

					//raggiunto il limite massimo di tentativi o modo di richiesta per nuovo
					//tentativo ad ogni presa fallita
					if((perso_flag==QParam.N_try) || ((QHeader.debugMode2 & DEBUG2_VACUOCHECK_DBG) && (NoPreso & CHECKCOMPFAIL_VACUO)))
					{
						//chiede all'utente se vuole riprovare
						if(nc == 0)
						{
							if(!NotifyPrelError(nc+1,NoPreso,DMount.Package[nc],DMount.PrgRec[nc].CodCom,C_TipoComp[nc],UpdateComponentQuantP1))
							{
								return(0);
							}
						}
						else
						{
							if(!NotifyPrelError(nc+1,NoPreso,DMount.Package[nc],DMount.PrgRec[nc].CodCom,C_TipoComp[nc],UpdateComponentQuantP2))
							{
								return(0);
							}
						}
					}

					//SMOD010405
					//porta la punta a theta di prelievo
					PuntaRotDeg(DMount.pick_angle[nc],nc+1);
			
					if(!AssScaricaComp(nc+1,SCARICACOMP_DEBUG,MsgGetString(Msg_01457)))
					{
						return(0);
					}

					if( !DMount.Caric[nc]->GoPos(nc+1) )
						return 0;
	
					//attiva contropressione prima di prelevare il comp.
					Prepick_Contro(nc+1);
					//attendi fine movimento caricatore
					ass_waitcaric(nc+1);
			
					//avanza caricatore
					if(!ass_avcaric(nc+1))
					{
						return(0);
					}

					if(!Show_step(S_CARAVANZ,nc+1)) return(0);
				}
	
			} while((perso_flag!=0) && (perso_flag!=QParam.N_try));
	
			//SMOD250903
			while(!Check_PuntaRot(1))
			{
				delay( 1 );
			}
			
			ASSEMBLY_PROFILER_MEASURE("Component pick %d loop ends",nc+1);
	
			if(perso_flag!=0) //se componente non preso
			{
				//ritorna a posizione di prelievo
				if( !DMount.Caric[nc]->GoPos( nc+1 ) )
					return 0;
				//attiva contropressione prima di prelevare il comp.
				Prepick_Contro(nc+1);
			}
			else //altrimenti se componente preso:esci dal ciclo tentativo presa componente
			{
				break;
			}
		}

		// Setta velocita'
		if( nc == 0 )
		{
			SetNozzleXYSpeed_Index( DMount.Package[0].speedXY );
		}
		else
		{
			if( DMount.mount_flag[0] )
			{
				SetMinNozzleXYSpeed_Index( DMount.Package[0].speedXY, DMount.Package[1].speedXY );
			}
			else
			{
				SetNozzleXYSpeed_Index( DMount.Package[1].speedXY );
			}
		}
	}

	set_currentcam( CAMERA_EXT );

	float post_centering_rot[2];

	for(int nc = 0 ; nc < 2 ; nc++)
	{
		if( !DMount.mount_flag[nc] )
		{
			continue;
		}

		post_centering_rot[nc] = DMount.PrgRec[nc].Rotaz + DMount.Package[nc].extAngle + currentLibOffsetPackages[DMount.Package[nc].code-1].angle + board_rotation[nc]; //VISANGLE

		struct PackVisData pvdat;

		char NameFile[MAXNPATH+1];
		PackVisData_GetFilename( NameFile, DMount.Package[nc].code, QHeader.Lib_Default );
		PackVisData_Open( NameFile, DMount.Package[nc].name );
		PackVisData_Read( pvdat );
		PackVisData_Close();
		
		SetExtCam_Light( pvdat.light[nc] );
		SetExtCam_Shutter( pvdat.shutter[nc] );
		SetExtCam_Gain(DEFAULT_EXT_CAM_GAIN);
	
		ASSEMBLY_PROFILER_MEASURE("Set external camera parameters %d",nc+1);

		//VISANGLE
		//PuntaRotDeg(go_dtheta+DMount.Pack[0].visangle,1);
		//per come vengono prelevati componenti a zero gradi il componente
		//avranno sempre il lato lungo orientato lungo y. Si porta la punta a
		//-90 per allineare il componente all'assse X
		if(!Show_step(S_PREROT,nc+1))
		{
			return(0);
		}
		PuntaRotDeg_component_safe( nc+1, -90 );
		
		if(!ass_carstatus[nc])
		{
			//attendi fine movimento caricatore
			DMount.Caric[nc]->WaitReady();
			ASSEMBLY_PROFILER_MEASURE("Wait post-pick %d feeder ready",nc+1);
			
			//avanza caricatore
			if(!ass_avcaric(nc+1))
			{
				return(0);
			}
			ASSEMBLY_PROFILER_MEASURE("Feeder %d advance",nc+1);

			if(!Show_step(S_CARAVANZ,nc+1))
			{
				return(0);
			}
		}

		Wait_EncStop(nc+1);

		ASSEMBLY_PROFILER_MEASURE("Centering rotation %d and wait for stop",nc+1);

		xmatch[0] = QParam.AuxCam_X[nc] - pvdat.xp[nc][0];
		ymatch[0] = QParam.AuxCam_Y[nc] - pvdat.yp[nc][0];
		xmatch[1] = QParam.AuxCam_X[nc] - pvdat.xp[nc][1];
		ymatch[1] = QParam.AuxCam_Y[nc] - pvdat.yp[nc][1];

		if(!Show_step(S_EXTCAM1,nc+1))
		{
			return(0);
		}

		if( !NozzleXYMove( xmatch[0], ymatch[0] ) )
		{
			return 0;
		}
		ASSEMBLY_PROFILER_MEASURE("Goto centering position 1 component %d",nc+1);

		//SMOD010704
		if( pvdat.zoff == 0 )
		{
			pvdat.zoff = -DMount.Package[nc].z;

			char NameFile[MAXNPATH+1];
			PackVisData_GetFilename( NameFile, DMount.Package[nc].code, QHeader.Lib_Default );
			PackVisData_Open( NameFile, DMount.Package[nc].name );
			PackVisData_Write(pvdat);
			PackVisData_Close();
		}

		Wait_PuntaXY();
		ASSEMBLY_PROFILER_MEASURE("Wait centering position 1 component %d",nc+1);

		// Muove punta z
		PuntaZPosMm( nc+1, pvdat.zoff + QParam.AuxCam_Z[nc] );
		PuntaZPosWait( nc+1 );
		ASSEMBLY_PROFILER_MEASURE("move to matching z and wait stop");

		// setta velocita' di confronto
		SetNozzleXYSpeed_Index( Vision.matchSpeedIndex );

		for( int ii = 0; ii < pvdat.niteraz; ii++ )
		{
			vismatch_err[nc] = 1;
			if(ii!=0)
			{
				if(!Show_step(S_EXTCAM1,nc+1))
				{
					return(0);
				}

				if( !NozzleXYMove( xmatch[0], ymatch[0], AUTOAPP_NOZSECURITY ) )
				{
					return 0;
				}
				Wait_PuntaXY();
				ASSEMBLY_PROFILER_MEASURE("Goto centering position 1 component %d (iteration number %d)",nc+1,ii);
			}

			int checkMaxError = (ii == pvdat.niteraz - 1) ? 1 : 0;
			if( !Image_match_package(nc+1,&xmatch[0],&ymatch[0],DMount.Package[nc].code,pvdat,QHeader.Lib_Default, (nc == 0) ? PACKAGEVISION_RIGHT_1 : PACKAGEVISION_RIGHT_2, checkMaxError) )
			{
				break;
			}

			dx[0+2*nc] = xmatch[0] - (QParam.AuxCam_X[nc]-pvdat.xp[nc][0]);
			dy[0+2*nc] = ymatch[0] - (QParam.AuxCam_Y[nc]-pvdat.yp[nc][0]);

			if(!Show_step(S_EXTCAM2,nc+1))
			{
				return(0);
			}

			if( !NozzleXYMove( xmatch[1], ymatch[1], AUTOAPP_NOZSECURITY ) )
			{
				return 0;
			}
			Wait_PuntaXY();
			ASSEMBLY_PROFILER_MEASURE("Goto centering position 2 component %d (iteration number %d)",nc+1, ii);

			if( !Image_match_package(nc+1,&xmatch[1],&ymatch[1],DMount.Package[nc].code,pvdat,QHeader.Lib_Default, (nc == 0) ? PACKAGEVISION_LEFT_1 : PACKAGEVISION_LEFT_2, checkMaxError) )
			{
				break;
			}

			dx[1+2*nc] = xmatch[1]-(QParam.AuxCam_X[nc] - pvdat.xp[nc][1]);
			dy[1+2*nc] = ymatch[1]-(QParam.AuxCam_Y[nc] - pvdat.yp[nc][1]);

			// controlla dimensione componente in base ad angolo di centraggio
			float pack_x = DMount.Package[nc].x;
			float pack_y = DMount.Package[nc].y;

			// se angolo centraggio e' 90 o 270 invertiamo dimensioni
			if( DMount.Package[nc].orientation == 90.f || DMount.Package[nc].orientation == 270.f )
			{
				// swap values
				float temp = pack_x;
				pack_x = pack_y;
				pack_y = temp;
			}

			// se angolo centraggio e' 90 o 270 invertiamo dimensioni
			if( DMount.Package[nc].extAngle == 90.f || DMount.Package[nc].extAngle == 270.f )
			{
				// swap values
				float temp = pack_x;
				pack_x = pack_y;
				pack_y = temp;
			}

			double dtx = pack_x + (pvdat.xp[nc][0]+dx[0+2*nc]) - (pvdat.xp[nc][1]+dx[1+2*nc]);
			double dty = -(dy[0+2*nc]-dy[1+2*nc]);
			double angle = -atan2(dty,dtx)*180/PI;

			// controllo prima iterazione
			if( ii == 0 && abs(angle) > VISCENT_MAX_THETA_CORRECTION_1 )
			{
				vismatch_err[nc] = 1;
				#ifdef __ASSEMBLY_DEBUG
				AssemblyVisDebug.Log( "[n: %d]   ERROR itera[%d]  angle: %.3f", nc+1, ii, angle );
				#endif
				break;
			}
			// controllo ultima iterazione
			else if( ii == pvdat.niteraz - 1 && abs(angle) > VISCENT_MAX_THETA_CORRECTION_LAST )
			{
				vismatch_err[nc] = 1;
				#ifdef __ASSEMBLY_DEBUG
				AssemblyVisDebug.Log( "[n: %d]   ERROR itera[%d]  angle: %.3f", nc+1, ii, angle );
				#endif
				break;
			}
			// controllo iterazioni intermedie
			else if( ii != 0 && abs(angle) > VISCENT_MAX_THETA_CORRECTION )
			{
				vismatch_err[nc] = 1;
				#ifdef __ASSEMBLY_DEBUG
				AssemblyVisDebug.Log( "[n: %d]   ERROR itera[%d]  angle: %.3f", nc+1, ii, angle );
				#endif
				break;
			}


			#ifdef __ASSEMBLY_DEBUG
			double _dx = dx[0+2*nc]-dx[1+2*nc];
			double _dy = dy[0+2*nc]-dy[1+2*nc];
			AssemblyVisDebug.Log( "[n: %d]   dx: %.3f   dy: %.3f   angle: %.3f", nc+1, _dx, _dy, angle );
			#endif

			if(!Show_step(S_EXTCAM_ROT,nc+1))
			{
				return(0);
			}

			PuntaRotDeg( angle, nc+1, BRUSH_REL );
			Wait_EncStop( nc+1 );
			ASSEMBLY_PROFILER_MEASURE("Apply theta correction component %d (iteration number %d)",nc+1,ii);

			vismatch_err[nc] = 0;
		}

		if( vismatch_err[nc] )
		{
			MoveComponentUpToSafePosition(nc+1);
			PuntaZPosWait(nc+1);
			continue;
		}


		if(!Show_step(S_SECZPOS,nc+1))
		{
			return(0);
		}
		
		if(fabs(post_centering_rot[nc]) <= SAFE_COMPONENT_ROTATION_LIMIT)
		{
			//porta il componente in posizione di sicurezza dove potrebbe non essere possibile la rotazione
			//se la posizione corrente fosse gia' sopra quella sicura forza comunque il movimento
			MoveComponentUpToSafePosition(nc+1,false);
		}
		else
		{
			float d = sqrt(DMount.Package[nc].x * DMount.Package[nc].x + DMount.Package[nc].y * DMount.Package[nc].y);
			
			if(d <= CCenteringReservedParameters::inst().getData().opt_cent_d1)
			{
				PuntaZPosMm( nc+1, GetZRotationPointForPackage( DMount.Package[nc] ) );
			}
			else
			{
				if(d <= CCenteringReservedParameters::inst().getData().opt_cent_d2)
				{
					PuntaZPosMm( nc+1, GetZRotationPointForPackage( DMount.Package[nc] ) );
				}
				else
				{
					//i componenti piu grandi eseguiranno la rotazione successivamente al movimento assi xy
					MoveComponentUpToSafePosition(nc+1,false);
				}
			}
			
			PuntaZPosWait(nc+1);
		}

		ASSEMBLY_PROFILER_MEASURE("Move nozzle %d in security position and wait for stop",nc);

		// Setta velocita'
		if( DMount.mount_flag[0] && DMount.mount_flag[1] )
		{
			SetMinNozzleXYSpeed_Index( DMount.Package[0].speedXY, DMount.Package[1].speedXY );
		}
		else if( DMount.mount_flag[0] )
		{
			SetNozzleXYSpeed_Index( DMount.Package[0].speedXY );
		}
		else
		{
			SetNozzleXYSpeed_Index( DMount.Package[1].speedXY );
		}
	}

	//GF_06_06_2011
	// aggiunto controllo perche' in alcuni casi la testa cerca di eseguire un movimento con punta bassa
	for(int nc = 0 ; nc < 2 ; nc++)
	{
		if(vismatch_err[nc])
		{
			MoveComponentUpToSafePosition(nc+1);
			PuntaZPosWait(nc+1);
		}
	}

	for(int nc = 0 ; nc < 2 ; nc++)
	{
		if(!DMount.mount_flag[nc] || vismatch_err[nc])
		{
			continue;
		}

		//set parametri di deposito
		struct DepoStruct depdata=DEPOSTRUCT_DEFAULT;

		depdata.stepF=Show_step;
		depdata.punta=nc+1;
		depdata.zpos = GetPianoZPos(nc+1) - PCBh - DMount.Package[nc].z;
		depdata.package = &DMount.Package[nc];
		depdata.xpos = place_xcoord[nc];
		depdata.ypos = place_ycoord[nc];
		
		float physical_place_theta_pos = Get_PhysThetaPlacePos( DMount.PrgRec[nc].Rotaz, DMount.Package[nc] ) + board_rotation[nc];

		//(nota: la rotazione e' relativa)
		depdata.rot_step = Deg2Step(post_centering_rot[nc],nc + 1);

		// Correzione tramite tabella rotazione per montaggio con telecamera
		depdata.rot_step += correct_rotation( physical_place_theta_pos, nc + 1, TABLE_CORRECT_EXTCAM );

		//DB270117
		struct PackVisData pvdat;

		char NameFile[MAXNPATH+1];
		PackVisData_GetFilename( NameFile, DMount.Package[nc].code, QHeader.Lib_Default );
		PackVisData_Open( NameFile, DMount.Package[nc].name );
		PackVisData_Read( pvdat );
		PackVisData_Close();

		if( (fabs(pvdat.centerX[nc]) > MAX_VISCENTER_DISPL) || (fabs(pvdat.centerY[nc]) > MAX_VISCENTER_DISPL) )
		{
			pvdat.centerX[nc] = 0.0;
			pvdat.centerY[nc] = 0.0;
		}
		dx[0+2*nc] -= pvdat.centerX[nc];
		dy[0+2*nc] -= pvdat.centerY[nc];
		dx[1+2*nc] -= pvdat.centerX[nc];
		dy[1+2*nc] -= pvdat.centerY[nc];

		double beta = (post_centering_rot[nc]*PI)/180;
		depdata.xpos += dx[0+2*nc]*cos(beta) - dy[0+2*nc]*sin(beta);
		depdata.ypos += dx[0+2*nc]*sin(beta) + dy[0+2*nc]*cos(beta);


		// Correzioni package per l'angolo selezionato
		float pkgoffs_x, pkgoffs_y;
		int pkgoffs_t;

		GetPackageOffsetCorrection( DMount.PrgRec[nc].Rotaz, DMount.Package[nc].code-1, pkgoffs_x, pkgoffs_y, pkgoffs_t );

		depdata.xpos += pkgoffs_x;
		depdata.ypos += pkgoffs_y;
		depdata.rot_step += pkgoffs_t;

		//SMOD270404-SMOD150305
		if(QParam.ZPreDownMode)
		{
			depdata.predepo_zdelta = maxCompHeight + DMount.Package[nc].z * PREDEPO_ZTOL;
		}

		//deposita componente
		if( !DepoComp( &depdata ) ) //se procedura abbandonata
		{
			if(!depdata.vacuocheck_dep)
			{
				retval=1; //procedura non abbandonata: controllo vuoto predeposito fallito
				vacuocheck_dep[nc]=0;
			}
			else
			{
				retval=0;
				break;
			}
		}
		else
		{
			retval=1;
			vacuocheck_dep[nc]=1;
		}

		if(depdata.mounted)
		{
			//SMOD270404
			if(QParam.ZPreDownMode)
			{
				//determina nuova quota di sicurezza per discesa
				if(DMount.Package[nc].z * PREDEPO_ZTOL + PCBh + deltaPreDown > maxCompHeight)
				{
					maxCompHeight = DMount.Package[nc].z * PREDEPO_ZTOL + PCBh  + deltaPreDown;
				}
			}
		
			ComponentList[(DMount.PrgRec[nc].Riga)-1]++;
			PlacedNComp++;
		}
	
		if(((depdata.mounted)  && ((startOnFile==0) && (startOnFile==Get_OnFile()))) || startOnFile==1)
		{
			if(nc == 0)
			{
				lastmountP1=DMount.PrgRec[0].Riga-1;
				if(nomountP2==1)
				{
					nomountP2=0;
					lastmount=lastmountP1+1;
				}
				else
				{
					lastmount=lastmountP1;
				}
				assrip_component=lastmount+1;
			}
			else
			{
				if(DMount.mount_flag[0])
				{
					lastmountP2=DMount.PrgRec[1].Riga-1;
				}

				lastmount=DMount.PrgRec[1].Riga-1;
				assrip_component=lastmount+1;
			}
			
			ncomp++;
		}
	}
	
	ASSEMBLY_PROFILER_MEASURE("End");
	
	return(retval);
}


void UpdateComponentQuant( int key, FeederClass* feeder )
{
	if( key == K_F5 )
	{
		char buf[80], c_type[26];
		
		MoveComponentUpToSafePosition(1);
		MoveComponentUpToSafePosition(2);
		PuntaZPosWait(1);
		PuntaZPosWait(2);

		NozzleXYMove( QParam.X_endbrd, QParam.Y_endbrd );
		Wait_PuntaXY();

		const struct CarDat& feeder_data = feeder->GetDataConstRef();	

		strncpyQ( c_type, feeder_data.C_comp, sizeof(feeder_data.C_comp)-1 );
		DelSpcR( c_type );

		int max;
		int ncomp = 0;
		int feeder_code = feeder->GetCode();

		if( feeder_code <= LASTFEED )
		{
			snprintf( buf, sizeof(buf), "%s n. %d (%s) : ", MsgGetString(Msg_01930), feeder_code, c_type );
			ncomp = feeder_data.C_quant;
			max = 999999;
		}
		else
		{
			snprintf( buf, sizeof(buf), "%s n. %d (%s) : ", MsgGetString(Msg_01929), feeder_code, c_type );
			ncomp = feeder_data.C_nx*feeder_data.C_ny;
			max = ncomp;
		}

		CInputBox inbox( 0, 6, MsgGetString(Msg_01932), buf, 6, CELL_TYPE_UINT );
		inbox.SetText( ncomp );
		inbox.SetVMinMax( 0, max );
		inbox.Show();

		if( inbox.GetExitCode() == WIN_EXITCODE_ESC )
		{
			return;
		}

		ncomp = inbox.GetInt();

		feeder->SetQuantity( ncomp );

		if( feeder_code <= LASTFEED && (QHeader.modal & ENABLE_CARINT) )
		{
			//estrae la posizione magazzino dal codice caricatore
			int nmag = (feeder_code/10);
			
			//se il caricatore in esame e' collegato al database
			int idx = GetConfDBLink(nmag+1);
			if( idx != -1 )
			{
				//NOTA031106
				//In questa fase il magazzino DEVE appartendere alla macchina (...non puo' essere altrimenti !!!)

				int f=DBToMemList_Idx(idx);
				if(f!=-1)
				{
					//DBLocal.Read(dbrec,idx);
				
					int n=(feeder_code-(nmag*10))-1;
				
					CarList[f].num[n] = feeder->GetQuantity();
				
					CarList[f].changed=CARINT_NONET_CHANGED;
				
					if(IsNetEnabled())
					{
						if(DBRemote.Write(CarList[f],idx,FLUSHON,CARINTFILE_ERR_ASKRETRY | CARINTFILE_ERR_ASKDISABLENET))
						{
							CarList[f].changed=0;
						}
					}

					DBLocal.Write(CarList[f],idx,FLUSHON);
				}
			}
		}
	}
}

void UpdateComponentQuantP1(int& c)
{
	UpdateComponentQuant(c,DMount.Caric[0]);
}

void UpdateComponentQuantP2(int& c)
{
	UpdateComponentQuant(c,DMount.Caric[1]);
}

//Esce dal ciclo di assemblaggio
//===============================================================================
void ass_quit(void)
{
	if(!ass_carstatus[0])
	{
		ass_avcaric(1);
	}
	if(!ass_carstatus[1])
	{
		ass_avcaric(2);
	}
	
	AssScaricaComp(1,SCARICACOMP_NORMAL,NULL);
	AssScaricaComp(2,SCARICACOMP_NORMAL,NULL);
	
	//SMOD010405
	PuntaRotDeg(0,1);
	PuntaRotDeg(0,2);
	while(!Check_PuntaRot(1));
	while(!Check_PuntaRot(2));	
}

// Assemblaggio
// re_ass: 0 (default) assemblaggio / 1 = ripresa assemblaggio
//===============================================================================
int assembla( int re_ass )
{
	int prg_loop =0;                  // counter loop progr.
	int nextloop=-1;
	int prg_end_loop;
	int resp;                         // flag: function response

	int n_ugello[2];                  // ugello A/B/C/...  (0,1,2,...)
	
	int ret_val;                      // valore ritornato da routine algoritmo di assemblaggio
	
	int firstComp=1;
	
	#define LASRET_P1		1
	#define LASRET_P2		2
	#define VCENTRET1		8
	#define VCENTRET2		16
	#define VACUOCHECK_DEP1	32
	#define VACUOCHECK_DEP2	64
	
	#define RETRY_MASK1		(LASRET_P1 | VCENTRET1 | VACUOCHECK_DEP1)
	#define RETRY_MASK2		(LASRET_P2 | VCENTRET2 | VACUOCHECK_DEP2)

	unsigned char ass_retry_flag = 0;
	unsigned char ass_retry_count[2] = { 0, 0 };

	struct TabPrg tmpPrg[2];

	ASSEMBLY_PROFILER_CLEAR();
	
	if(QHeader.predownDelta==0)
		deltaPreDown=DEF_DELTASECURPOS;
	else
		deltaPreDown=QHeader.predownDelta;


	las_errmode[0] = LASMSGOFF;
	las_errmode[1] = LASMSGOFF;
	

	if(re_ass)                                    //se restart
	{
		prg_loop=assrip_component;                     //recupera indice file programma di assem.
	}
	else
	{
		assrip_component=0;
	
		assembling=1;
	
		maxCompHeight = PREDEPO_ZPOS;
		
		float maxTray = MaxUsedTrayHeight() + 0.5; //margine di sicurezza

		if(maxCompHeight < maxTray)
		{
			maxCompHeight = maxTray;
		}
	
	}
	
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );              // set acc. vel. assi
	
	for( int i = 1; i <= 2; i++ )
	{
		//set z speed di default (max)
		SetNozzleZSpeed_Index( i, ACC_SPEED_DEFAULT );
		//set theta speed & accel di default (max)
		SetNozzleRotSpeed_Index( i, ACC_SPEED_DEFAULT );

		PuntaRotDeg( 0, i );
	}

	Wait_EncStop(1);
	Sniper1->Zero_Cmd();
	Wait_EncStop(2);
	Sniper2->Zero_Cmd();

	float PCBh;
	Read_PCBh( PCBh ); //lettura altezza PCB programma attuale
	
	DMount.Caric[0] = NULL;
	DMount.Caric[1] = NULL;


	//            CICLO LETTURA COMPONENTI
	//-------------------------------------------------------------------------
	prg_end_loop=0;

	if(!re_ass)          // se modo!=modo restart
	{
		prg_loop=0;        // parte da primo componente in tabella
		nomountP2=0;       // SMOD110403
	}

	startOnFile = Get_OnFile();

	while(!prg_end_loop) // loop lettura prog.
	{
		//se laser retry non richiesto
		if(!(ass_retry_flag & (LASRET_P1 | LASRET_P2 | VCENTRET1 | VCENTRET2 | VACUOCHECK_DEP1 | VACUOCHECK_DEP2)))
		{
			//legge coppia di componenti
			resp=ass_readcomp(&DMount,prg_loop);

			if(resp==-1)      //se fine file raggiunta
			{
				prg_end_loop=1;      //fine ciclo
				break;
			}

			ass_retry_count[0]=0;
			ass_retry_count[1]=0;
			las_errmode[0] = LASMSGOFF;
			las_errmode[1] = LASMSGOFF;
			
			//legge coppia componenti successiva, da montare dopo l'attuale
			nextloop=prg_loop;
			resp=ass_readcomp(NextDMount,nextloop);

			if(resp==-1)
			{
				//SMOD060504 START
				if(NextDMount->Caric[0]!=NULL)
				{
					delete NextDMount->Caric[0];
					if(NextDMount->Caric[0]==NextDMount->Caric[1])
					{
						NextDMount->Caric[1]=NULL;
					}
				}
			
				if(NextDMount->Caric[1]!=NULL)
				{
					delete NextDMount->Caric[1];
				}
				//SMOD060504 END
		
				delete NextDMount;
				NextDMount=NULL;
		
				prg_end_loop=1;
			}
	
			//SMOD110403
			if(re_ass && nomountP2)
			{
				DMount.mount_flag[1]=0;
			}
			
			//memo dei dati letti in un buffer temporaneo
			memcpy(tmpPrg,DMount.PrgRec,2*sizeof(struct TabPrg));
		}
		else //laser retry richiesto
		{
			//recupera ultimi dati letti
			memcpy(DMount.PrgRec,tmpPrg,2*sizeof(struct TabPrg));
	
			//reload feeder data
			if(DMount.mount_flag[0])
			{
				DMount.Caric[0]->ReloadData();
			}

			if(DMount.mount_flag[1])
			{
				DMount.Caric[1]->ReloadData();
			}
	
			int onlyP1=0;

			if(DMount.mount_flag[0] && (!DMount.mount_flag[1]))
			{
				onlyP1=1;
			}

			//reset mount_flag
			DMount.mount_flag[0]=0;
			DMount.mount_flag[1]=0;
	
			if(ass_retry_flag & LASRET_P1) //se richiesto retry su P1
			{
				DMount.mount_flag[0]=1;             //mount P1=TRUE
			}
	
			if(ass_retry_flag & LASRET_P2) //se richiesto retry su P2
			{
				DMount.mount_flag[1]=1;             //mount P2=TRUE
			}

			if(ass_retry_flag & VCENTRET1)   //se richiesto retry con telecamera su P1
			{
				DMount.mount_flag[0]=1;             //mount P1=TRUE
			}
				
			if(ass_retry_flag & VCENTRET2)   //se richiesto retry con telecamera su P2
			{
				DMount.mount_flag[1]=1;             //mount P2=TRUE
			}
			
			//SMOD291004
			if(ass_retry_flag & VACUOCHECK_DEP1)
			{
				DMount.mount_flag[0]=1;
			}
	
			if(ass_retry_flag & VACUOCHECK_DEP2)
			{
				DMount.mount_flag[1]=1;
			}
	
			bool doRetry1 =  (ass_retry_flag & LASRET_P1) || (ass_retry_flag & VCENTRET1) || (ass_retry_flag & VACUOCHECK_DEP1);
			bool doRetry2 =  (ass_retry_flag & LASRET_P2) || (ass_retry_flag & VCENTRET2) || (ass_retry_flag & VACUOCHECK_DEP2);

			if(doRetry1 && !doRetry2)
			{
				if(!onlyP1)
				{
					nomountP2=1;
				}
			}
		}

		//SMOD240907
		for( int p = 0; p < 2; p++ )
		{
			//se ultimo tentativo mostra dettaglio errore di centraggio a seguito di ulteriore fallimento
			las_errmode[p] = ( ass_retry_count[p] >= QParam.N_try-1 ) ? LASMSGON : LASMSGOFF;
		}
	
		re_ass = 0; //reset flag di restart (non piu' necessario)

		for( int i = 0; i < 2; i++ )
		{
			if(!DMount.mount_flag[i])
			{
				C_NComp[i]->SetTxt("");
				C_CodComp[i]->SetTxt("");
				C_CodCar[i]->SetTxt("");
				C_Op[i]->SetTxt("");
				C_TipoComp[i]->SetTxt("");
			}
		}

		Show_dati();                              // display dati
	
		//            CARICA DATI CARICATORE E UGELLO
		//-------------------------------------------------------------------------
		
		//SMOD270404
		for( int i = 0; i < 2; i++ )
		{
			if(!DMount.mount_flag[i])
			{
				continue;
			}
	
			//set dati prelievo ugelli
			n_ugello[i]=DMount.PrgRec[i].Uge;       //codice ugello
		}


		//            CAMBIA UGELLI SE NECESSARIO
		//-------------------------------------------------------------------------

		//setta parametri classe Ugelli
		Ugelli->SetFlag(DMount.mount_flag[0],DMount.mount_flag[1]);
	
		//cambia ugelli se necessario
		if(!Ugelli->DoOper(n_ugello))
		{
			//abbandono programma di assemblaggio
			PuntaZSecurityPos(1); //porta punte a posizione di sicurezza
			PuntaZSecurityPos(2);
		
			PuntaZPosWait(2);
			PuntaZPosWait(1);
			
			Set_UgeBlock(0);
		
			return(0);
		}
	
		//setta soglie vuoto
		Ugelli->Set_SogliaVacuo( DMount.Package[0].checkVacuumThr, 1 );
		Ugelli->Set_SogliaVacuo( DMount.Package[1].checkVacuumThr, 2 );


		//            ASSEMBLAGGIO COMPONENTI
		//-------------------------------------------------------------------------

		ass_carstatus[0]=0; //setta stato caricatori = non avanzati
		ass_carstatus[1]=0;

		//reset flag errori laser
		centeringResult[0].Result = STATUS_OK;
		centeringResult[1].Result = STATUS_OK;

		vismatch_err[0]=0;	
		vismatch_err[1]=0;

		//GF_07_04_2011
		compcheck_post_dep[0] = 1;
		compcheck_post_dep[1] = 1;

		ret_val=0;

		//SMOD051104
		for( int i = 0; i < 2; i++ )
		{
			if(!DMount.mount_flag[i])
			{
				continue;
			}
			
			if(firstComp)
			{
				PuntaRotDeg(DMount.pick_angle[i],i+1);
			}
		}

		firstComp = 0;

		//se assemblaggio monopunta (1)
		if((DMount.mount_flag[0]) && (!DMount.mount_flag[1]))
		{
			ASSEMBLY_PROFILER_START("%d %d %s %s",DMount.PrgRec[0].Riga,DMount.PrgRec[0].scheda,DMount.PrgRec[0].CodCom,DMount.PrgRec[0].TipCom);

			switch( DMount.Package[0].centeringMode )
			{
				case CenteringMode::EXTCAM:
					if(Get_UseCam() && Get_UseExtCam())
					{
						//assemblaggio con centratore ottico
						#ifdef __ASSEMBLY_DEBUG
						AssemblyVisDebug.Log("%d %d %s %s",DMount.PrgRec[0].Riga,DMount.PrgRec[0].scheda,DMount.PrgRec[0].CodCom,DMount.PrgRec[0].TipCom);
						#endif
						ret_val = ass_vis(PCBh);
					}
					else
					{
						//SE TUTTO FUNZIONA NON DOVREBBE MAI ACCADERE !!
						W_Mess( MsgGetString(Msg_05039) );
						return(0);
					}
					break;

				case CenteringMode::NONE:
				case CenteringMode::SNIPER:
					//assemblaggio con centraggio su testa
					ret_val = ass_las(PCBh);
					break;
			}

			ASSEMBLY_PROFILER_STOP();
		}

		//se assemblaggio monopunta (2)
		if((DMount.mount_flag[1]) && (!DMount.mount_flag[0]))
		{
			ASSEMBLY_PROFILER_START("%d %d %s %s",DMount.PrgRec[1].Riga,DMount.PrgRec[1].scheda,DMount.PrgRec[1].CodCom,DMount.PrgRec[1].TipCom);

			switch( DMount.Package[1].centeringMode )
			{
				case CenteringMode::EXTCAM:
					if( Get_UseCam() && Get_UseExtCam() )
					{
						//assemblaggio con centratore ottico
						#ifdef __ASSEMBLY_DEBUG
						AssemblyVisDebug.Log("%d %d %s %s",DMount.PrgRec[1].Riga,DMount.PrgRec[1].scheda,DMount.PrgRec[1].CodCom,DMount.PrgRec[1].TipCom);
						#endif
						ret_val = ass_vis(PCBh);
					}
					else
					{
						//SE TUTTO FUNZIONA NON DOVREBBE MAI ACCADERE !!
						W_Mess( MsgGetString(Msg_05039) );
						return(0);
					}
					break;

				case CenteringMode::NONE:
				case CenteringMode::SNIPER:
					//assemblaggio con centraggio su testa
					ret_val= ass_las(PCBh);
					break;
			}

			ASSEMBLY_PROFILER_STOP();
		}
	
		//se assemblaggio a doppia punta
		if(DMount.mount_flag[0] && DMount.mount_flag[1])
		{
			ASSEMBLY_PROFILER_START("%d %d %s %s , %d %d %s %s",DMount.PrgRec[0].Riga,DMount.PrgRec[0].scheda,DMount.PrgRec[0].CodCom,DMount.PrgRec[0].TipCom , DMount.PrgRec[1].Riga,DMount.PrgRec[1].scheda,DMount.PrgRec[1].CodCom,DMount.PrgRec[1].TipCom );

			if( (DMount.Package[0].centeringMode == CenteringMode::SNIPER || DMount.Package[0].centeringMode == CenteringMode::NONE) &&
				(DMount.Package[1].centeringMode == CenteringMode::SNIPER || DMount.Package[1].centeringMode == CenteringMode::NONE) )
			{
				ret_val = ass_las(PCBh);
			}
			else
			{
				if( DMount.Package[0].centeringMode == CenteringMode::EXTCAM && DMount.Package[1].centeringMode == CenteringMode::EXTCAM && Get_UseCam() && Get_UseExtCam() )
				{
					#ifdef __ASSEMBLY_DEBUG
					AssemblyVisDebug.Log("%d %d %s %s , %d %d %s %s",DMount.PrgRec[0].Riga,DMount.PrgRec[0].scheda,DMount.PrgRec[0].CodCom,DMount.PrgRec[0].TipCom , DMount.PrgRec[1].Riga,DMount.PrgRec[1].scheda,DMount.PrgRec[1].CodCom,DMount.PrgRec[1].TipCom );
					#endif
					ret_val = ass_vis(PCBh);
				}
				else
				{
					//SE TUTTO FUNZIONA NON DOVREBBE MAI ACCADERE !!
					W_Mess( MsgGetString(Msg_05039) );
					return(0);
				}
			}
			
			ASSEMBLY_PROFILER_STOP();
		}
	
		if(!((startOnFile==0 && startOnFile==Get_OnFile()) || startOnFile==1))
		{
			//riscontrata condizione di errore grave: termina ciclo
			return(0);
		}

	
		if( !ret_val ) //se ass. abbandonato
		{
			if(!ass_carstatus[0]) //se caricatore da avanzare
			{
				DMount.Caric[0]->WaitReady(); //attendi caricatore libero
				ass_avcaric(1); //avanza caricatore
			}
			
			//scarica componenti
			AssScaricaComp(1,SCARICACOMP_NORMAL,NULL);
			AssScaricaComp(2,SCARICACOMP_NORMAL,NULL);
	
			return(0);
		}

		// esegue log se richiesto
		if( Get_WriteDiscardLog() )//&& !QParam.DemoMode )
		{
			if( DMount.mount_flag[0] )
				assemblyErrorsReport.Log( 1, centeringResult[0].Result, DMount.CarRec[0].C_PackIndex-1 );

			if( DMount.mount_flag[1] )
				assemblyErrorsReport.Log( 2, centeringResult[1].Result, DMount.CarRec[1].C_PackIndex-1 );
		}


		//se errore laser su primo componente (p1)
		if( centeringResult[0].Result != STATUS_OK && DMount.Package[0].centeringMode == CenteringMode::SNIPER && DMount.mount_flag[0] )
		{
			ass_retry_count[0]++;
	
			char laserrbuf[160];
	
			if(ass_retry_count[0]>=QParam.N_try)
			{
				//superato il numero di retry
				C_TipoComp[0]->SetNorColor(GUI_color(GR_WHITE),GUI_color(GR_LIGHTRED));
				C_TipoComp[0]->Refresh();

				ass_retry_count[0]=0;
				las_errmode[0]=LASMSGOFF;
	
				snprintf(laserrbuf, sizeof(laserrbuf),"%s%s",MsgGetString(Msg_00877),MsgGetString(Msg_01933));
	
				if(!W_Deci(1,laserrbuf,MSGBOX_YLOW,0,UpdateComponentQuantP1,GENERIC_ALARM))
				{
					//richiesto di non riprovare
				
					//scarica componente
					#ifdef __SNIPER
					snprintf(laserrbuf, sizeof(laserrbuf),MsgGetString(Msg_01456),0);//TODO Sniper1->GetErr(1));
					AssScaricaComp(1,SCARICACOMP_DEBUG,laserrbuf);
					#endif
	
					if( centeringResult[1].Result != STATUS_OK && DMount.Package[1].centeringMode == CenteringMode::SNIPER && DMount.mount_flag[1] )
					{
						//centraggio fallito per il componente su punta 2 : scarica componente
						#ifdef __SNIPER
						snprintf(laserrbuf, sizeof(laserrbuf),MsgGetString(Msg_01456),0);//TODO Sniper1->GetErr(2));
						AssScaricaComp(2,SCARICACOMP_DEBUG,laserrbuf);
						#endif
					}
				
					//termina ciclo
					ass_quit();

					C_TipoComp[0]->SetNorColor();
					C_TipoComp[0]->Refresh();

					return(0);
				}
			}

			//se si arriva qui si deve eseguire un retry per il componente su punta 1
			C_TipoComp[0]->SetNorColor();
			C_TipoComp[0]->Refresh();

			ass_retry_flag=ass_retry_flag | LASRET_P1;
	
			//scarica componente/i
			#ifdef __SNIPER
			snprintf(laserrbuf, sizeof(laserrbuf),MsgGetString(Msg_01456),0);//TODO Sniper1->GetErr(1));
			AssScaricaComp(1,SCARICACOMP_DEBUG,laserrbuf);
			#endif
	
			prg_end_loop = 0;
		}
		else
		{
			//nessun errore di centraggio laser su punta 1
			ass_retry_flag=ass_retry_flag & ~LASRET_P1;
		}

		//se errore laser su secondo componente (p2)
		if( centeringResult[1].Result != STATUS_OK && DMount.Package[1].centeringMode == CenteringMode::SNIPER && DMount.mount_flag[1] )
		{
			ass_retry_count[1]++;
	
			char laserrbuf[160];
	
			if(ass_retry_count[1]>=QParam.N_try)
			{
				C_TipoComp[1]->SetNorColor(GUI_color(GR_WHITE),GUI_color(GR_LIGHTRED));
				C_TipoComp[1]->Refresh();

				ass_retry_count[1]=0;
				las_errmode[1]=LASMSGOFF;
	
				snprintf(laserrbuf, sizeof(laserrbuf),"%s%s",MsgGetString(Msg_00878),MsgGetString(Msg_01933));

				if(!W_Deci(1,laserrbuf,MSGBOX_YLOW,0,UpdateComponentQuantP2,GENERIC_ALARM)) //SMOD140305
				{
					if( centeringResult[0].Result != STATUS_OK && DMount.Package[0].centeringMode == CenteringMode::SNIPER && DMount.mount_flag[0] )
					{
						#ifdef __SNIPER
						snprintf(laserrbuf, sizeof(laserrbuf),MsgGetString(Msg_01456),0);//TODO Sniper1->GetErr(1));
						AssScaricaComp(1,SCARICACOMP_DEBUG,laserrbuf);
						#endif
					}

					#ifdef __SNIPER
					snprintf(laserrbuf, sizeof(laserrbuf),MsgGetString(Msg_01456),0);//TODO Sniper1->GetErr(2));
					AssScaricaComp(2,SCARICACOMP_DEBUG,laserrbuf);
					#endif

					ass_quit();

					C_TipoComp[1]->SetNorColor();
					C_TipoComp[1]->Refresh();

					return(0);
            	}
          	}

			C_TipoComp[1]->SetNorColor();
			C_TipoComp[1]->Refresh();

			ass_retry_flag=ass_retry_flag | LASRET_P2;

			#ifdef __SNIPER
			snprintf(laserrbuf, sizeof(laserrbuf),MsgGetString(Msg_01456),0);//TODO Sniper1->GetErr(2));
			AssScaricaComp(2,SCARICACOMP_DEBUG,laserrbuf);
			#endif

			prg_end_loop = 0;
		}
		else
		{
			ass_retry_flag=ass_retry_flag & ~LASRET_P2;
		}

		//------------------------------------------------------------------------------
		ass_retry_flag &= ~VCENTRET1;

		//se nessun retry richiesto per la punta 1 (no errori laser)
		if(!(ass_retry_flag & RETRY_MASK1))
		{
			if( DMount.mount_flag[0] && DMount.Package[0].centeringMode == CenteringMode::EXTCAM && vismatch_err[0] )
			{
				ass_retry_count[0]++;
				ass_retry_flag|=VCENTRET1;
				if(ass_retry_count[0] >= QParam.N_try)
				{
					C_TipoComp[0]->SetNorColor(GUI_color(GR_WHITE),GUI_color(GR_LIGHTRED));
					C_TipoComp[0]->Refresh();

					ass_retry_count[0]=0;

					char buf[160];
					snprintf(buf,sizeof(buf),"%s%s",MsgGetString(Msg_00877),MsgGetString(Msg_01933));
					AssScaricaComp(1,SCARICACOMP_DEBUG,MsgGetString(Msg_00764));
					if(!W_Deci(0,buf,MSGBOX_YLOW,0,UpdateComponentQuantP1,GENERIC_ALARM)) //SMOD140305
					{
						ass_retry_flag&= ~VCENTRET1;
						C_TipoComp[0]->SetNorColor();
						C_TipoComp[0]->Refresh();

						ass_quit();
						return(0);
					}

					C_TipoComp[0]->SetNorColor();
					C_TipoComp[0]->Refresh();
				}
				else
				{
					AssScaricaComp(1,SCARICACOMP_DEBUG,MsgGetString(Msg_00764));
				}
			}
		}
		
		//------------------------------------------------------------------------------
		
		ass_retry_flag&=~VCENTRET2;
		
		//se nessn retry richiesto per la punta 2 (no errori laser)
		if(!(ass_retry_flag & RETRY_MASK2))
		{
			if( DMount.mount_flag[1] && DMount.Package[1].centeringMode == CenteringMode::EXTCAM && vismatch_err[1] )
			{
				ass_retry_count[1]++;
				ass_retry_flag|=VCENTRET2;
				if(ass_retry_count[1] >= QParam.N_try)
				{
					C_TipoComp[1]->SetNorColor(GUI_color(GR_WHITE),GUI_color(GR_LIGHTRED));
					C_TipoComp[1]->Refresh();

					ass_retry_count[1]=0;

					char buf[160];
					snprintf(buf,sizeof(buf),"%s%s",MsgGetString(Msg_00878),MsgGetString(Msg_01933));
					ass_retry_flag=ass_retry_flag | VCENTRET2;
					AssScaricaComp(2,SCARICACOMP_DEBUG,MsgGetString(Msg_00764));
					if(!W_Deci(0,buf,MSGBOX_YLOW,0,UpdateComponentQuantP2,GENERIC_ALARM)) //SMOD140305
					{
						C_TipoComp[1]->SetNorColor();
						C_TipoComp[1]->Refresh();

						ass_quit();
						return(0);
					}

					C_TipoComp[1]->SetNorColor();
					C_TipoComp[1]->Refresh();
				}
				else
				{
					AssScaricaComp(2,SCARICACOMP_DEBUG,MsgGetString(Msg_00764));
				}
			}
		}

		//------------------------------------------------------------------------------
		
		ass_retry_flag &= ~VACUOCHECK_DEP1;
		
		//se nessn retry richiesto per la punta 1 (no errori di centraggio)
		if( !(ass_retry_flag & RETRY_MASK1) )
		{
			//se il controllo di presenza componente pre-deposito e' fallito
			if(!vacuocheck_dep[0] && DMount.mount_flag[0])
			{
				//retry
				ass_retry_count[0]++;
	
				if(ass_retry_count[0]>=QParam.N_try)
				{
					ass_retry_count[0]=0;

					C_TipoComp[0]->SetNorColor(GUI_color(GR_WHITE),GUI_color(GR_LIGHTRED));
					C_TipoComp[0]->Refresh();

					char buf[160];
					snprintf(buf,sizeof(buf),NOPRESA,DMount.PrgRec[0].CodCom,1);
					strncat(buf,MsgGetString(Msg_01933), sizeof(buf));
	
					if(!W_Deci(1,buf,MSGBOX_YLOW,0,UpdateComponentQuantP1,GENERIC_ALARM)) //SMOD140305
					{
						AssScaricaComp(1,SCARICACOMP_DEBUG,MsgGetString(Msg_01714));
		
						if(!vacuocheck_dep[1] && DMount.mount_flag[1])
						{
							AssScaricaComp(2,SCARICACOMP_DEBUG,MsgGetString(Msg_01714));
						}
				
						ass_quit();

						C_TipoComp[0]->SetNorColor();
						C_TipoComp[0]->Refresh();

						return(0);
					}
				}

				C_TipoComp[0]->SetNorColor();
				C_TipoComp[0]->Refresh();

				ass_retry_flag=ass_retry_flag | VACUOCHECK_DEP1;
	
				AssScaricaComp(1,SCARICACOMP_DEBUG,MsgGetString(Msg_01714));
	
				prg_end_loop = 0;
			}
			else
			{
				ass_retry_flag=ass_retry_flag & ~VACUOCHECK_DEP1;
			}
			
			//GF_07_04_2011
			// controlla presenza componente dopo deposito - punta 1
			if( !compcheck_post_dep[0] && DMount.mount_flag[0] )
			{
				char buf[160];
				snprintf( buf, sizeof(buf), MsgGetString(Msg_05144), 1 );

				AssScaricaComp( 1, SCARICACOMP_DEBUG, buf );

				// porta la punta a quota di centraggio
				PuntaZPosMm( 1, -(DMount.Package[0].z/2+DMount.Package[0].snpZOffset) );
				PuntaZPosWait( 1 );

				// attivo vuoto
				Set_Vacuo( 1, 1 );
				delay( 300 );

				// controlla presenza componente
				int ret_val = Sniper_CheckComp( 1 );

				// disattivo vuoto
				Set_Vacuo( 1, 0 );

				//retry
				ass_retry_count[0]++;

				// se componente non scaricato o raggiunto retry max
				if( ret_val || ass_retry_count[0] >= QParam.N_try )
				{
					ass_retry_count[0] = 0;

					C_TipoComp[0]->SetNorColor(GUI_color(GR_WHITE),GUI_color(GR_LIGHTRED));
					C_TipoComp[0]->Refresh();

					W_Mess( buf,MSGBOX_YCENT,0,GENERIC_ALARM );

					// se necessario scarica punta 2
					if( !compcheck_post_dep[1] && DMount.mount_flag[1] )
					{
						snprintf( buf, sizeof(buf), MsgGetString(Msg_05144), 2 );

						AssScaricaComp( 2, SCARICACOMP_DEBUG, buf );

						W_Mess( buf,MSGBOX_YCENT,0,GENERIC_ALARM );
					}

					ass_quit();

					C_TipoComp[0]->SetNorColor();
					C_TipoComp[0]->Refresh();

					return 0;
				}

				C_TipoComp[0]->SetNorColor();
				C_TipoComp[0]->Refresh();

				ass_retry_flag = ass_retry_flag | VACUOCHECK_DEP1;
		
				prg_end_loop = 0;
			}
		}

		//------------------------------------------------------------------------------
		
		ass_retry_flag&=~VACUOCHECK_DEP2;
		
		//se nessn retry richiesto per la punta 2 (no errori di centraggio)
		if(!(ass_retry_flag & RETRY_MASK2))
		{
			if(!vacuocheck_dep[1] && DMount.mount_flag[1])
			{
				ass_retry_count[1]++;
	
				if(ass_retry_count[1]>=QParam.N_try)
				{
					C_TipoComp[1]->SetNorColor(GUI_color(GR_WHITE),GUI_color(GR_LIGHTRED));
					C_TipoComp[1]->Refresh();

					ass_retry_count[1]=0;
		
					char buf[160];
					snprintf(buf,sizeof(buf),NOPRESA,DMount.PrgRec[1].CodCom,2);
					strncat(buf,MsgGetString(Msg_01933),sizeof(buf));
	
					if(!W_Deci(1,buf,MSGBOX_YLOW,0,UpdateComponentQuantP2,GENERIC_ALARM)) //SMOD140305
					{
						if(!vacuocheck_dep[0] && DMount.mount_flag[0])
						{
							AssScaricaComp(1,SCARICACOMP_DEBUG,MsgGetString(Msg_01714));
						}
		
						AssScaricaComp(2,SCARICACOMP_DEBUG,MsgGetString(Msg_01714));
		
						ass_quit();

						C_TipoComp[1]->SetNorColor();
						C_TipoComp[1]->Refresh();

						return(0);
					}
				}

				C_TipoComp[1]->SetNorColor();
				C_TipoComp[1]->Refresh();

				ass_retry_flag=ass_retry_flag | VACUOCHECK_DEP2;
	
				AssScaricaComp(2,SCARICACOMP_DEBUG,MsgGetString(Msg_01714));
	
				prg_end_loop = 0;
			}
			else
			{
				ass_retry_flag=ass_retry_flag & ~VACUOCHECK_DEP2;
			}
			
			//GF_07_04_2011
			// controlla presenza componente dopo deposito - punta 2
			if( !compcheck_post_dep[1] && DMount.mount_flag[1] )
			{
				char buf[160];
				snprintf(buf,sizeof(buf), MsgGetString(Msg_05144), 2 );

				AssScaricaComp( 2, SCARICACOMP_DEBUG, buf );

				// porta la punta a quota di centraggio
				PuntaZPosMm( 2, -(DMount.Package[1].z/2+DMount.Package[1].snpZOffset) );
				PuntaZPosWait( 2 );

				// attivo vuoto
				Set_Vacuo( 2, 1 );
				delay( 300 );

				// controlla presenza componente
				int ret_val = Sniper_CheckComp( 2 );

				// disattivo vuoto
				Set_Vacuo( 2, 0 );

				//retry
				ass_retry_count[1]++;

				// se componente non scaricato o raggiunto retry max
				if( ret_val || ass_retry_count[1] >= QParam.N_try )
				{
					ass_retry_count[1] = 0;

					C_TipoComp[1]->SetNorColor(GUI_color(GR_WHITE),GUI_color(GR_LIGHTRED));
					C_TipoComp[1]->Refresh();

					W_Mess( buf,MSGBOX_YCENT,0,GENERIC_ALARM );
					
					ass_quit();

					C_TipoComp[1]->SetNorColor();
					C_TipoComp[1]->Refresh();

					return 0;
				}

				C_TipoComp[1]->SetNorColor();
				C_TipoComp[1]->Refresh();

				ass_retry_flag = ass_retry_flag | VACUOCHECK_DEP2;
		
				prg_end_loop = 0;
			}
		}

		//------------------------------------------------------------------------------


		if(!(ass_retry_flag & (RETRY_MASK1 | RETRY_MASK2)))
		{
			lastmountP1=-1;
			lastmountP2=-1;
		}
		else
		{
			//SMOD020404
			//Riprova centraggio:
			//anche se raggiunta fine programma, continua ad eseguire
			//il ciclo.
			prg_end_loop=0;
		}
	
		if(ass_retry_flag & (LASRET_P1 | VCENTRET1 | VACUOCHECK_DEP1))
		{
			//Ritentativo per punta 1: riporta la punta alla posizione angolare
			//di prelievo per il componente da prelevare
			if(DMount.mount_flag[0])
			{
				PuntaRotDeg(DMount.pick_angle[0],1);

				if(!Show_step(S_ZERTHETA,1))
				{
					ass_quit();
					return(0);
				}
			}
		}
		else
		{
			//No ritentativo per punta 1: ruota la punta per il prossimo
			//componente da prelevare
			if(NextDMount!=NULL)
			{
				if(NextDMount->mount_flag[0])
				{
					PuntaRotDeg(NextDMount->pick_angle[0],1);
		
					if(!Show_step(S_ZERTHETA,1))
					{
						ass_quit();
						return(0);
					}
				}
			}
		}
	
		if(ass_retry_flag & (LASRET_P2 | VCENTRET2 | VACUOCHECK_DEP2))
		{
			//Ritentativo per punta 2: riporta la punta alla posizione angolare
			//di prelievo per il componente da prelevare
			if(DMount.mount_flag[1])
			{
				PuntaRotDeg(DMount.pick_angle[1],2);
	
				if(!Show_step(S_ZERTHETA,2))
				{
					ass_quit();
					return(0);
				}
			}
		}
		else
		{
			//No ritentativo per punta 2: ruota la punta per il prossimo
			//componente da prelevare
			if(NextDMount!=NULL)
			{
				if(NextDMount->mount_flag[1])
				{
					PuntaRotDeg(NextDMount->pick_angle[1],2);
	
					if(!Show_step(S_ZERTHETA,2))
					{
						ass_quit();
						return(0);
					}
				}
			}
		}
	}
	//fine ciclo componenti
	
	ResetNozzles(); //resetta posizione

	return 1;
} // assembla

extern wait_to_cont_ass_internal_state_t wait_to_cont_ass_internal_state;

void wait_to_cont_ass_protection_check_polling_loop(void)
{
	if(CSecurityReservedParametersFile::inst().get_data().flags.bits.end_assembly_protection_check_disabled)
	{
		wait_to_cont_ass_internal_state = protection_state_closed;
		return;
	}
	
	//sequenza stati : unknown --> opened --> closed
	switch(wait_to_cont_ass_internal_state)
	{
		case protection_state_wait_transition:
			if( CheckSecurityInput() )
			{
				//protezione aperta : passa a stato opened
				wait_to_cont_ass_internal_state = protection_state_opened;				
			}
			break;

		case protection_state_opened:
			if( !CheckSecurityInput() )
			{
				//protezione aperta : passa a stato closed
				wait_to_cont_ass_internal_state = protection_state_closed;	
				//resetta il timer per la richiesta di conferma movimenti xy: se l'utente
				//dara' il consenso ad un nuovo assemblaggio prima che il timer scada, 
				//l'assemblaggio partira' immediatamente a meno che al momento dell'avvio
				//del nuovo assemblaggio la protezione e' aperta: in questo caso l'utente
				//dovra' abbassare la protezione e confermare con F5 o doppio shift in
				//base a quanto tempo e' trascorso da quando la protezione era stata chiusa
				ResetTimerConfirmRequiredBeforeNextXYMovement();			
			}			
			break;

		case protection_state_closed:
			if( CheckSecurityInput() )
			{
				//la protezione e' stata riaperta
				wait_to_cont_ass_internal_state = protection_state_opened;	
			}
			break;
	}

}

wait_to_cont_ass_internal_state_t wait_to_cont_ass_internal_state;

// aspetta per continuare assemblaggio
//===============================================================================
int wait_to_cont_ass(void)
{  
	wait_to_cont_ass_internal_state = protection_state_wait_transition;
	
	char buf[160];
	buf[0]='\0';

	if( Get_ShowAssemblyTime() )
	{
		int min = last_assembly_time / 60;
		int sec = last_assembly_time % 60;

		snprintf( buf, sizeof(buf), MsgGetString(Msg_01545), min, sec );
		strncat( buf, "\n\n", sizeof(buf) );
	}

	strncat( buf, MsgGetString(Msg_00218), sizeof(buf) );
	
	int ret = W_Deci(1,buf,MSGBOX_YCENT,0,0,GENERIC_ALARM);
	/*
	int ret = W_Deci(1,buf,MSGBOX_YCENT,wait_to_cont_ass_protection_check_polling_loop,0,GENERIC_ALARM);
	if(ret)
	{
		if(wait_to_cont_ass_internal_state == protection_state_wait_transition)
		{
			//la protezione non e' stata aperta e successivamente chiusa mentre veniva mostrata all'utente
			//la richiesta per un nuovo assemblaggio: l'operazione e' obbligatoria in quanto permette di verificare
			//il corretto funzionamento del sensore protezione aperta/chiusa; chiede quindi all'utente di verificarne
			//il funzionamento (dovra' aprire e chiudere la protezione) quindi il software si comporta come se l'utente
			//avesse aperto e richiusto la protezione prima di confermare un nuovo assemblaggio; l'utente puo' anche
			//annullare l'operazione premendo ESC ma questo comportera' la disattivazione dei movimenti
			ret = CheckProtectionIsWorking();		
		}
	}
	*/
	return ret;
}


//init della maschera dati assemblaggio
//===============================================================================
//DANY201102
void InitAssMask( CWindow* parent )
{
	Q_assembla = new CWindow( parent );
	Q_assembla->SetStyle( WIN_STYLE_CENTERED_X );
	Q_assembla->SetClientAreaPos( 0, 6 );
	Q_assembla->SetClientAreaSize( 80, 13 );
	Q_assembla->SetTitle( MsgGetString(Msg_00189) );

	C_NBrd[0] = new C_Combo( 2, 3, MsgGetString(Msg_00190), 5, CELL_TYPE_TEXT );
	C_NBrd[1] = new C_Combo( 52, 3, "", 5, CELL_TYPE_TEXT );

	C_NComp[0] = new C_Combo( 2, 4, MsgGetString(Msg_00191), 5, CELL_TYPE_TEXT );
	C_NComp[1] = new C_Combo( 52, 4, "", 5, CELL_TYPE_TEXT );

	C_CodComp[0] = new C_Combo( 2, 5, MsgGetString(Msg_00192), 8, CELL_TYPE_TEXT );
	C_CodComp[1] = new C_Combo( 52, 5, "", 8, CELL_TYPE_TEXT );

	C_TipoComp[0] = new C_Combo( 2, 6, MsgGetString(Msg_00193), 25, CELL_TYPE_TEXT );
	C_TipoComp[1] = new C_Combo( 52, 6, "", 25, CELL_TYPE_TEXT );

	C_CodCar[0] = new C_Combo( 2, 7, MsgGetString(Msg_00194), 4, CELL_TYPE_TEXT );
	C_CodCar[1] = new C_Combo( 52, 7, "", 4, CELL_TYPE_TEXT );

	C_Op[0] = new C_Combo( 2, 10, MsgGetString(Msg_00197), 25, CELL_TYPE_TEXT );
	C_Op[1] = new C_Combo( 52, 10, "", 25, CELL_TYPE_TEXT );

	GUI_Freeze_Locker lock;

	Q_assembla->Show();
	C_NBrd[0]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_NBrd[1]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_NComp[0]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_NComp[1]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_CodComp[0]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_CodComp[1]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_TipoComp[0]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_TipoComp[1]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_CodCar[0]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_CodCar[1]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_Op[0]->Show( Q_assembla->GetX(), Q_assembla->GetY() );
	C_Op[1]->Show( Q_assembla->GetX(), Q_assembla->GetY() );

	Q_assembla->DrawText( 21, 1, MsgGetString(Msg_00042) );
	Q_assembla->DrawText( 52, 1, MsgGetString(Msg_00043) );
}

//dealloca mascheda dati assemblaggio
//===============================================================================
void CloseAssMask()
{
	Q_assembla->Hide();
	delete Q_assembla;

	for( int i = 0; i < 2; i++ )
	{
		delete C_NBrd[i];
		delete C_NComp[i];
		delete C_TipoComp[i];
		delete C_CodComp[i];
		delete C_CodCar[i];
		delete C_Op[i];
	}
}

//SMOD260903
int CheckAssemblyEnd(TPrgFile *file)
{
	if(ComponentList==NULL)
	{
		return 0;
	}

	int assncomp=file->Count();
	struct TabPrg tab;
	int NCompAss_InFile=0;

	for(int i=0;i<assncomp;i++)
	{
		file->Read(tab,i);
		if((tab.status & MOUNT_MASK) && (!(tab.status & NOMNTBRD_MASK)))
			NCompAss_InFile++;
		else
			ComponentList[i]=-1;
	}

	unsigned int *no_placed=NULL;
	unsigned int *multiple_placed=NULL;
	int N_noplaced=0;
	int N_multiple=0;

	for(unsigned int i=0;i<NComponent;i++)
	{
		if(ComponentList[i]==0)
		{
		  N_noplaced++;
		}
		else
		{
		  if((ComponentList[i]!=-1) && (ComponentList[i]!=1))
		  {
			N_multiple++;
		  }
		}
	}

	if(N_noplaced)
	{
		no_placed=new unsigned int[N_noplaced];
	}

	if(N_multiple)
	{
		multiple_placed=new unsigned int[N_multiple];
	}

	if(PlacedNComp!=NCompAss_InFile)
	{
		if(!(N_noplaced || N_multiple))
		{
		   char buf[240];
		   snprintf( buf, sizeof(buf),MsgGetString(Msg_01518),PlacedNComp,NCompAss_InFile,N_noplaced,N_multiple);
	       W_Mess(buf,MSGBOX_YCENT,0,GENERIC_ALARM);
		}
	}

	int j=0;
	int k=0;

	if(N_noplaced || N_multiple)
	{
		for(unsigned int i=0;i<NComponent;i++)
		{
		  if(ComponentList[i]==0)
		  {
			no_placed[j++]=i+1;
		  }
		  else
		  {
			if((ComponentList[i]!=1) && (ComponentList[i]!=-1))
			{
			  multiple_placed[k++]=i+1;
			}
		  }
		}

		char buf[20];

		if( N_noplaced )
		{
			CWindowSelect win( 0, 3, 1, 15, strlen(MsgGetString(Msg_01519)) );
			win.SetStyle( win.GetStyle() | WIN_STYLE_NO_MENU );
			win.SetClientAreaPos( 0, 7 );
			win.SetTitle( MsgGetString(Msg_01519) );

			for( int i = 0; i < N_noplaced; i++ )
			{
				snprintf( buf, 20, "%d", no_placed[i] );
				win.AddItem( buf );
			}

			win.Show();
			win.Hide();

			delete [] no_placed;
		}

		if( N_multiple )
		{
			CWindowSelect win( 0, 3, 1, 15, strlen(MsgGetString(Msg_01520)) );
			win.SetStyle( win.GetStyle() | WIN_STYLE_NO_MENU );
			win.SetClientAreaPos( 0, 7 );
			win.SetTitle( MsgGetString(Msg_01520) );

			for( int i = 0; i < N_multiple; i++ )
			{
				snprintf( buf, 20, "%d", multiple_placed[i] );
				win.AddItem( buf );
			}

			win.Show();
			win.Hide();

			delete [] multiple_placed;
		}

		return 0;
	}

	return 1;
}

//Funzione start del ciclo di assemblaggio
//restart  : 0 modo normale
//           1 modo restart
//stepmopde: 0 modo ass. passo-passo di default
//           1 forza asse. passo-passo
//asknewAss: 0 non chiedere nuovo assemblaggio
//           1 chiede nuovo assemblaggio        (default)
//===============================================================================
int Ass_tot(int restart,int stepmode,int asknewAss,int autoref,int enableContAss)
{
	int end_ass;
	int pann_on=0;
	int rit_ass=1;
	
	if(!restart) // se no modo restart
	{
		assrip_component=0;
	}

	By_step = stepmode ? 1 : 0;

	int numerozeri=ZerAssem->GetNRecs();
	
	Read_nboard(nboard,ncomp);

	if(numerozeri<2)                                 //no recs zeri presenti
	{
		W_Mess(NOZSCHEDA);
		rit_ass=0;                                     //ritorna FALSE
	}
	else
	{
		//loop assemblaggi
		while(1)
		{
			workingLog.LogProduction( PRODUCTION_START, QHeader.Prg_Default, nboard );

			EnableForceFinec(ON);      // abilita protezione tramite finecorsa
		
			// Ricerca riferimenti scheda in automatico
			if((QParam.Autoref && !restart) && autoref)
			{
				if(!Exist_ZRefPanelFile())
				{
					if(Z_sequenza(0,1))
					{
						//SMOD310506-FinecFix
						DisableForceFinec();
						Set_Finec(OFF);
						return(0);
					}
			
					if(Z_sequenza(1,1))
					{
						//SMOD310506-FinecFix
						DisableForceFinec();
						Set_Finec(OFF);
						return(0);
					}
				}
				else
				{
					if(!Z_AppMC())
					{
						DisableForceFinec();
						Set_Finec(OFF);
						return(0);
					}
				}
			}

			ZerAssem->Read(DMount.ZerRecs[0],0); // read master
		
			if(By_step && !pann_on)
			{
				pann_on=1;
			}

			NextDMount=new(struct MountData);
			NextDMount->Caric[0]=NULL;
			NextDMount->Caric[1]=NULL;
		
			Ugelli->Set_StepF(Show_step); //setta funzione passo passo per gestore ugelli

			for( unsigned int i = 0; i < NComponent; i++ )
			{
				if( ComponentList[i] == -1 || !restart )
				{
					ComponentList[i]=0;
					PlacedNComp=0;
				}
			}

			//--- Assembly start ---

			Timer assembly_timer;
			assembly_timer.start();

			//GF_THREAD
			StartCenteringThread();

			end_ass = assembla(restart); // assemblaggio

			//GF_THREAD
			StopCenteringThread();

			assembly_timer.stop();
			last_assembly_time = assembly_timer.getElapsedTimeInSec();
			
			// measure machine WorkTime
			MachineInfo.WorkTime += assembly_timer.getElapsedTimeInSec();
			MachineInfo_Write( MachineInfo );

			SetExtCam_Light(0);
			SetExtCam_Shutter(DEFAULT_EXT_CAM_SHUTTER);
			SetExtCam_Gain(DEFAULT_EXT_CAM_GAIN);

			if( Get_WriteDiscardLog() )
			{
				assemblyErrorsReport.Save();
				assemblyErrorsReport.SaveCVS();
			}

			//SMOD310506-FinecFix
			DisableForceFinec();       // torna a modo normale per il settaggio del controllo finecorsa
			Set_Finec(OFF);            // disabilita controllo finecorsa
			
			//SMOD060504
			if(DMount.Caric[0]!=NULL)
			{
				delete DMount.Caric[0];
		
				if(DMount.Caric[0]==DMount.Caric[1])
				{
					DMount.Caric[1]=NULL;
				}

				DMount.Caric[0]=NULL;
			}

			if(DMount.Caric[1]!=NULL)
			{
				delete DMount.Caric[1];
				DMount.Caric[1]=NULL;
			}
			
			// ---  Assembly end  ---

			//SMOD031003
			//controlla perdita passo sulle punte ed eventualmente riazzera
			#ifndef HWTEST_RELEASE
			PuntaZSecurityPos(1);
			PuntaZSecurityPos(2);
			PuntaZPosWait(2);
			PuntaZPosWait(1);
			HeadEndMov();
			//TODO - provare nuova funzione
			//CheckNozzlesLossSteps();
			CheckNozzleOk();
			#endif

			//SMOD260903
			if(end_ass)
			{
				CheckAssemblyEnd(TPrgAssem);
			}
		
			restart=0; //SMOD110403
		
			// assemblaggio terminato
			if(end_ass)
			{
				assembling=0;

				//SMOD110403
				lastmountP1=-1;
				lastmountP2=-1;
		
				if(((startOnFile==0) && (startOnFile==Get_OnFile())) || startOnFile==1)
				{
					nboard+=ZerAssem->GetNAssBoard();
				}
				rit_ass=1;
			}
			else
			{
				assembling=1;
			}

			workingLog.LogStatus( STATUS_IDLE );
			workingLog.LogProduction( PRODUCTION_END, QHeader.Prg_Default, nboard );

			Save_nboard(nboard,ncomp);
		
			PuntaRotDeg(0,1);
			PuntaRotDeg(0,2);
		
			if(NextDMount!=NULL)
			{
				//SMOD060504 START
				if(NextDMount->Caric[0]!=NULL) //###MSS
				{
					delete NextDMount->Caric[0];
					if(NextDMount->Caric[0]==NextDMount->Caric[1])
					{
						NextDMount->Caric[1]=NULL;
					}
				}
					
				if(NextDMount->Caric[1]!=NULL)
				{
					delete NextDMount->Caric[1];
				}
				//SMOD060504 END
		
				delete NextDMount;
				NextDMount=NULL;
			}
			
			Ugelli->Set_StepF(NULL);           //elimina gestore passo passo per gestore ugelli
		
			//reset acc e vel XY a valori di default
			SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
		
			//se assemblaggio abbandonato o no assemblaggio continuato
			if(!end_ass || (!(QParam.AS_cont && enableContAss)))
			{
				ResetNozzles(); //resetta stato delle punte

				if(pann_on)
				{
					pann_on=0;
				}

				if(!end_ass) //se assemblaggio abbandonato
				{
					rit_ass=0; //setta flag di ritorno a FALSE
					UpdateMagaComp();
					break; //fine ciclo assemblaggi
				}

				//SMOD31056 - FinecFix
				Set_Finec(ON);
				// go fuori scheda
				NozzleXYMove( QParam.X_endbrd, QParam.Y_endbrd );
				//SMOD31056 - FinecFix
				Wait_PuntaXY();
				Set_Finec(OFF);

				UpdateMagaComp();

				if(asknewAss)
				{
					if(!wait_to_cont_ass()) //se no nuovo assemblaggio
					{
						ResetNozzles(); //resetta stato delle punte
			
						assrip_component=0;
						if(pann_on)
						{
							pann_on=0;
						}
						break; //esci da ciclo: no altro assemblaggio
					}
					else
					{
						restart=0;
					}
				}
				else
				{
					if(pann_on)
					{
						pann_on=0;
					}
					break;
				}
			}
		}
	}
	
	//setta vel di default
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
	
	return(rit_ass);
} // Ass_tot

// Gestione assemblaggio
//===============================================================================
int InitAss(int restart,int stepmode,int nrestart,int asknewAss,int autoref,int enableContAss)
{
	float PCBh;
	int ret;
	
	Read_PCBh(PCBh);
	if( fabs(PCBh) < PCB_H_ABS_MIN )
	{
		int ris=W_Deci( 1, MsgGetString(Msg_01139) );
		if(!ris)
		{
			return(0);
		}
	}
	if( PCBh < PCB_H_MIN || PCBh > PCB_H_MAX )
	{
		W_Mess( MsgGetString(Msg_00096) );
		return 0;
	}
	
	if(!OpenAssFiles())
	{
		CloseAssFiles();
		return 0;
	}
	
	if(restart)
	{
		if(nrestart==-1)
		{
			if(!assembling)
			{
				W_Mess( MsgGetString(Msg_00220) );
			}
		
			assrip_component=Get_LastRecMount(1)+1; //SMOD110403
		
			unsigned int prev_assrip=assrip_component;
			
			if(!Set_RipData(assrip_component,TPrgAssem->Count(),RIP_ASS_TITLE))
			{
				CloseAssFiles();
				return(0);
			}
		
			if(assrip_component!=prev_assrip)
			{
				nomountP2=0; //SMOD251104
				lastmountP1=-1;
				lastmountP2=-1;
			}
		}
		else
		{
			assrip_component=nrestart;
			if(assrip_component>TPrgAssem->Count())
				assrip_component=TPrgAssem->Count();
		}
	}
	else
		lastmount=-1;
	
	InitAssMask( 0 );

	//THFEEDER2
	last_advanced_caric = NULL;
	
	workingLog.LogStatus( STATUS_WORKING );

	ret=Ass_tot(restart,stepmode,asknewAss,autoref,enableContAss);
	
	workingLog.LogStatus( STATUS_IDLE );

	CloseAssMask();
	
	CloseAssFiles();   //chiude i file aperti
	
	return(ret);
}

// Ritorna ultimo comp. assemblato (-1=nessun componente assemblato)
// mode=1 per ciclo di assemblaggio
//     =0 per tabella di programma
//===============================================================================
//SMOD110403 - SMOD280404
int Get_LastRecMount(int mode)
{
	if(mode)
	{
		//non puo accadere !? eliminare ??
		if(lastmount<lastmountP2)
		{
			lastmount=lastmountP2;
		
			lastmountP1=-1;
			lastmountP2=-1;
		}
		else
		{
			//assemblaggio terminato su errore punta 1 e componente punta 2
			//depositato correttamente
			if((lastmountP1==-1) && (lastmountP2!=-1))
			{
				//si deve ripetere il ciclo per la sola punta 1:
				//virtualmente si setta come ultimo componente assemblato
				//il precedente a quello sulla punta 1 (ancora da assemblare)
				lastmount=lastmountP2-2;
				//il ciclo deve essere eseguito a singola punta
				nomountP2=1;
		
				//reset dei flag
				lastmountP1=-1;
				lastmountP2=-1;
			}
		}
	}
	return(lastmount);
}

void AssScanTest(int punta,int *pflag)
{
	PuntaZPosWait(punta);

	ScanTest( punta, LASSCAN_ZEROCUR | LASSCAN_BIGWIN, 0, DMount.Package[punta-1].z, 0 );
}


//---------------------------------------------------------------------------
// Notifica a video errore di centraggio ed aggiorna report degli errori
//---------------------------------------------------------------------------
void NotifyCenteringError( int nozzle, int errorCode )
{
	//TODO: rivedere messaggi di errore
	if( las_errmode[nozzle-1] == LASMSGON )
	{
		const char* msg;

		switch( errorCode )
		{
		case CEN_ERR_EMPTY:
			msg = MsgGetString(Msg_01331); // componente non trovato
			break;

		case CEN_ERR_L_BLOCKED:
		case CEN_ERR_R_BLOCKED:
		case CEN_ERR_B_BLOCKED:
			msg = MsgGetString(Msg_01330);
			break;

		case CEN_ERR_NOMIN:
			msg = MsgGetString(Msg_01326);
			break;

		case CEN_ERR_DIMX:
			msg = MsgGetString(Msg_01327);
			break;

		case CEN_ERR_DIMY:
			msg = MsgGetString(Msg_01328);
			break;

		case CEN_ERR_BUF_FULL:
		case CEN_ERR_ENCODER:
		case CEN_ERR_TOO_BIG:
		default:
			msg = MsgGetString(Msg_01503);
		}

		char buf[256];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01325), nozzle );
		strncat( buf, "\n", sizeof(buf) );
		strncat( buf, msg, sizeof(buf) );

		bipbip();
		W_Mess( buf,MSGBOX_YCENT,0,GENERIC_ALARM );
	}
}

//TODO: inserire ri-tentativo di centraggio se fallisce il primo come parametro generale o per package ???
/*
int Ass_GetLaserResults(int punta,int errmode)
{
	int ret;
	int count=0;
	
	do
	{
		//Wait_EncStop(punta); //BOOST
		//get laser result
		
		int sweep_retry_on = 0;
		
		//abilita retry immediato del centraggio laser in caso di errori
		//se l'opportuno bit in debugMode2 e' attivato o se il caricatore
		//da cui il componente e' stato prelevato e' 0402
		if(QHeader.debugMode2 & DEBUG2_RETRYSWEEPONERR)
		{
			sweep_retry_on = 1;
		}
		else
		{
			if(DMount.Caric[punta-1] != NULL) 
			{
				if((DMount.Caric[punta-1]->GetDataConstRef().C_att==CARAVA_DOUBLE) || (DMount.Caric[punta-1]->GetDataConstRef().C_att==CARAVA_DOUBLE1))
				{
					sweep_retry_on = 1;
				}
			}
		}

		#ifdef __SNIPER
		if(sweep_retry_on && (count<MAX_SWEEP_REPEAT-1))
		{
			ret = ( punta == 1 ? Sniper1->GetResult(LASMSGOFF) : Sniper2->GetResult(LASMSGOFF) );
		}
		else
		{
			ret = ( punta == 1 ? Sniper1->GetResult(errmode) : Sniper2->GetResult(errmode) );
		}
		laser_result[punta-1] = ret;
		#endif
		
		if(ret==1)
		{
			if((count==0) && assembling)
			{
				#ifdef __SNIPER
				switch( punta == 1 ? Sniper1->GetErr(punta) : Sniper2->GetErr(punta) )
				#endif
				{
					case LAS_ERRCODE_NOCOMP:
						AssemblyReport.E64[punta-1]++;
						break;
					case LAS_ERRCODE_OUTANGLELIMIT:
						AssemblyReport.E98[punta-1]++;
						break;
					case LAS_ERRCODE_DIMX:
					case LAS_ERRCODE_DIMY:
						AssemblyReport.Etol[punta-1]++;
						break;
					default:
						AssemblyReport.Eother[punta-1]++;
						break;
				}

				if(QHeader.debugMode1 & DEBUG1_SWEEPREPORT)
				{
					struct SweepErrorReport Report;
			
					#ifdef __SNIPER
					if(punta == 1)
					{
						Report.errcode[0] = Sniper1->GetErr(0);
						Report.errcode[1] = Sniper1->GetErr(1);
					}
					else
					{
						Report.errcode[0] = Sniper2->GetErr(0);
						Report.errcode[1] = Sniper2->GetErr(1);
					}
					#endif
			
					strncpyQ(Report.progname,QHeader.Prg_Default,8);
					strncpyQ(Report.cust,QHeader.Cli_Default,8);
					strncpyQ(Report.conf,QHeader.Conf_Default,8);
					strncpyQ(Report.lib,QHeader.Lib_Default,8);
			
					Report.Riga=DMount.PrgRec[punta-1].Riga;
					strncpyQ(Report.CodCom,DMount.PrgRec[punta-1].CodCom,16);
					strncpyQ(Report.TipCom,DMount.PrgRec[punta-1].TipCom,25);
					Report.nozzle=punta;
			
					struct date d;
					getdate(&d);
					struct time t;
					gettime(&t);
			
					Report.day=d.da_day;
					Report.year=d.da_year;
					Report.month=d.da_mon;
			
					Report.hour=t.ti_hour;
					Report.min=t.ti_min;
					Report.sec=t.ti_sec;

					#ifdef __SNIPER
					punta == 1 ? Sniper1->GetFrames() : Sniper2->GetFrames();
					Report.nframes= ( punta == 1 ? Sniper1->ReadFrames(Report.buf_w,Report.buf_a) : Sniper2->ReadFrames(Report.buf_w,Report.buf_a) );
					#endif
			
					WriteSweepErrReport(Report);
				}

				WriteAssemblyReport(AssemblyReport);                             //SMOD300703
		
				#ifdef __SNIPER
				Assembly_WriteLogFull( punta,ASSLOG_TYPE_ERROR,(punta == 1 ? Sniper1->GetErr(punta) : Sniper2->GetErr(punta)) ); //SMOD300703
				#endif
			}

			if(sweep_retry_on)
			{
				print_debug("Sweep Retry %d on nozzle %d\n",count,punta);
		
				#ifdef __SNIPER
				PuntaRotDeg(-SniperModes::inst().getRecord().search_angle,punta,BRUSH_REL);
		
				Wait_EncStop(punta);
		
				count++;
		
				if(count!=MAX_SWEEP_REPEAT)
				{
					ResetSniperCycle( punta );
					while( !isSniperCycleCompleted(punta) )
					{
						NextSniperPhase(punta,0);
					}
				}
				else
				{
					break;
				}
				#endif
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	} while(count!=MAX_SWEEP_REPEAT);
	
	return(ret);
}
*/


int ass_las(float PCBh)
{
	char prel_perso[2] = { 0, 0 };
	bool prel_ok[2] = { false, false };
	bool caric_p1_da_avanzare = true;
	bool caric_p2_da_avanzare = true;
	bool caric_p1_uguale_p2 = false;

	struct CfgBrush brushPar;
	
	float corsa_p; // corsa z prelievo (mm)
	char NoPreso;
	int pos,downpos;
	
	struct PrelStruct preldata=PRELSTRUCT_DEFAULT;
	
	CarDat tmpCar;
	short int tmpCarCode;

	//SMOD010606 - PackOffset
	float pkgoffs_x[2],pkgoffs_y[2];
	int pkgoffs_t[2];

	// setta velocita ed accelerazioni
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
	
	for( int i = 1; i <= 2; i++ )
	{
		SetNozzleRotSpeed_Index( i, ACC_SPEED_DEFAULT );
	}

	float* KZmm_passi = &QHeader.Step_Trasl1;


	//GF_THREAD
	int placeAngle[2] = { 0, 0 };

	for( int i = 0; i < 2; i++ )
	{
		if( DMount.mount_flag[i] )
		{
			float board_rotation = GetBoardRotation_Deg( DMount.ZerRecs[0], DMount.ZerRecs[i+1] );

			// 1.
			// Calcolo correzione tabella di mappatura rotazione
			float a1 = DMount.PrgRec[i].Rotaz + board_rotation + currentLibOffsetPackages[DMount.Package[i].code-1].angle - DMount.Package[i].orientation;
			placeAngle[i] += correct_rotation( a1, i+1, TABLE_CORRECT_SNIPER );
			// 2.
			// Calcolo correzione parametri package
			GetPackageOffsetCorrection( DMount.PrgRec[i].Rotaz, DMount.Package[i].code-1, pkgoffs_x[i], pkgoffs_y[i], pkgoffs_t[i] );
			placeAngle[i] += pkgoffs_t[i];
			// 3.
			//
			float a3 = DMount.PrgRec[i].Rotaz + board_rotation + currentLibOffsetPackages[DMount.Package[i].code-1].angle - LASANGLE(DMount.Package[i].orientation);
			a3 += ( i == 0 ) ? QHeader.sniper1_thoffset : QHeader.sniper2_thoffset;
			placeAngle[i] += Deg2Step( a3, i+1 );
		}
	}




	//            INIZIO PRELIEVO COMPONENTE PUNTA 1
	//-------------------------------------------------------------------------
	
	ASSEMBLY_PROFILER_MEASURE("START pick 1");
	
	if( DMount.mount_flag[0] )
	{
		while( !prel_ok[0] )
		{
			// punta 1 in posizione di prelievo
			//--------------------------------------------------------------------------
			if(!Show_step(S_GOPREL,1))
				return(0);
			
			if( !DMount.Caric[0]->GoPos( 1 ) )
				return 0;
			ASSEMBLY_PROFILER_MEASURE("move xy pick position p1");

			// avvicina punta 1 a quota di prelievo
			//--------------------------------------------------------------------------
			if( QParam.ZPreDownMode )
			{
				if(!Show_step(S_PREDISCESA,1))
					return(0);
				
				DoPredown( 1, GetPianoZPos(1)-maxCompHeight );
				ASSEMBLY_PROFILER_MEASURE("z predown p1");
			}

			// attiva contropressione prima di prelevare il comp.
			//--------------------------------------------------------------------------
			Prepick_Contro(1);
			ASSEMBLY_PROFILER_MEASURE("prepick back pressure p1");

			// calcola
			//--------------------------------------------------------------------------
			if( DMount.PrgRec[0].Caric < FIRSTTRAY )
			{
				corsa_p = GetZCaricPos(1) - DMount.Caric[0]->GetDataConstRef().C_offprel;
			}
			else
			{
				if(!Show_step(S_VACUON,1))
					return(0);

				//vassoio
				corsa_p = GetPianoZPos(1) - DMount.Caric[0]->GetDataConstRef().C_offprel;
				//attiva vuoto
				Set_Vacuo(1,ON);
			}

			// init della struttura dati di prelievo
			//--------------------------------------------------------------------------
			preldata.punta = 1;
			preldata.zprel = corsa_p;                        //corsa in mm
			preldata.zup = -(DMount.Package[0].z/2+DMount.Package[0].snpZOffset); //quota a cui andare dopo il prelievo
			preldata.stepF = Show_step;                      //puntatore ala funzione handler passo-passo
			preldata.package = &DMount.Package[0];           //struttura package
			preldata.waitup = 0;                             //non attendere punta up prima di uscire
			preldata.downpos = &downpos;
			preldata.caric = DMount.Caric[0];

			// attesa punta ferma
			//--------------------------------------------------------------------------
			while(!Check_PuntaRot(1))
			{
				delay( 1 );
			}
			FoxPort->RestartLog();
			ASSEMBLY_PROFILER_MEASURE("wait theta p1");

			// attesa assi fermi
			//--------------------------------------------------------------------------
			Wait_PuntaXY();
			ASSEMBLY_PROFILER_MEASURE("wait xy p1");

			// attesa caricatore pronto
			//--------------------------------------------------------------------------
			//THFEEDER
			if( !ass_waitcaric(1) )
			{
				return 0;
			}
			ASSEMBLY_PROFILER_MEASURE("wait feeder ready p1");

			// preleva componente
			//--------------------------------------------------------------------------
			prePrelQuant[0] = DMount.Caric[0]->GetDataConstRef().C_quant;

			if( !PrelComp(preldata) )
			{
				return 0;
			}

			//decrementa numero di componenti presenti sul caricatore
			DMount.Caric[0]->DecNComp();

			ASSEMBLY_PROFILER_MEASURE("pick component p1");


			// se centraggio laser
			if( DMount.Package[0].centeringMode == CenteringMode::SNIPER )
			{
				//Se la quota di centraggio e' piu bassa di quella di prerotazione attende fine movimento
				//NOTA: Non dovrebbe mai accadere ! E' una patch per evitare che errori sulla quota di prelievo blocchino il sw.
				if( preldata.zup > (preldata.zprel-DMount.Package[0].z-KSECURITY) )
				{
					PuntaZPosWait(1);
				}
				else
				{
					//attende che la punta sia a quota di prerotazione
					do
					{
						if( QParam.DemoMode || Get_OnFile() || !FoxHead->IsEnabled() )
							break;

						while( !centeringMutex.try_lock() )
						{
							delay( 1 );
						}

						pos = FoxHead->ReadPos(STEP1)*P1_ZDIRMOD;

						centeringMutex.unlock();
					} while(pos>downpos-int((DMount.Package[0].z+KSECURITY)*KZmm_passi[0]));
				}
				ASSEMBLY_PROFILER_MEASURE("Wait z prerotation p1");
			}
			
			// segnala caricatore da avanzare
			//--------------------------------------------------------------------------
			ass_carstatus[0] = 0;

			if( DMount.mount_flag[1] )
			{
				// Se comp sulla punta 2 e' da prelevare dallo stesso caric della punta 1
				// avanza caricatore
				//--------------------------------------------------------------------------
				tmpCar = DMount.Caric[0]->GetData();
				tmpCarCode = tmpCar.C_codice;
				tmpCar = DMount.Caric[1]->GetData();
			
				if( tmpCarCode == tmpCar.C_codice )
				{
					if(!Show_step(S_CARAVANZ,2))
						return(0);
					
					if( !ass_avcaric(2) )
					{
						return(0);
					}
					ASSEMBLY_PROFILER_MEASURE("feeder advance 1->2");
					caric_p2_da_avanzare = false;
					caric_p1_uguale_p2 = true;
				}
			}

			//SMOD260907
			if(QHeader.modal & SCANPREL1_MASK)
			{
				AssScanTest(1,DMount.mount_flag);
			}
	
			// check componente preso
			//--------------------------------------------------------------------------
			if( DMount.Package[0].checkPick != 'V' )
			{
				if(!Show_step(S_CHKPRESO,1))
					return(0);
		
				NoPreso = Check_CompIsOnNozzle( 1, DMount.Package[0].checkPick );
				ASSEMBLY_PROFILER_MEASURE("check component on nozzle p1");
			}
			else
			{
				NoPreso = 0;
			}

			if( !NoPreso )
			{
				// componente preso
				//--------------------------------------------------------------------------
				prel_ok[0] = true;
			}
			else
			{
				// componente NON preso
				//--------------------------------------------------------------------------
		
				//reload feeder data
				DMount.Caric[0]->ReloadData();
				
				//riporta la punta alla posizione angolare precedente la prerotazione
				//--------------------------------------------------------------------------
				PuntaRotDeg(DMount.pick_angle[0],1);
				
				//attende punta in posizione di sicurezza
				//--------------------------------------------------------------------------
				WaitPUP(1);
				ASSEMBLY_PROFILER_MEASURE("wait z secured p1");
		
				//incrementa contatore perdite
				prel_perso[0]++;

				//raggiunto il limite massimo di tentativi o modo di richiesta per nuovo tentativo ad ogni presa fallita
				//--------------------------------------------------------------------------
				if( (prel_perso[0]==QParam.N_try) || ((QHeader.debugMode2 & DEBUG2_VACUOCHECK_DBG) && (NoPreso & CHECKCOMPFAIL_VACUO)) )
				{
					//chiede all'utente se vuole riprovare
					if(!NotifyPrelError( 1, NoPreso, DMount.Package[0], DMount.PrgRec[0].CodCom,C_TipoComp[0],UpdateComponentQuantP1))
					{
						return(0);
					}
					
					if( prel_perso[0] == QParam.N_try )
						prel_perso[0] = 0;
				}
		
				// scarica componente su punta
				//--------------------------------------------------------------------------
				if(!AssScaricaComp(1,SCARICACOMP_DEBUG,MsgGetString(Msg_01457)))
				{
					return(0);
				}
				
				// avanza caricatore
				//--------------------------------------------------------------------------
				//NOTA: se (caric_p1_uguale_p2 = false) allora il componente sulla punta 2 e' in
				//      un caricatore diverso e quindi avanza altrimenti e' gia' avanzato
				if( !caric_p1_uguale_p2 )
				{
					if(!ass_avcaric(1))
					{
						return(0);
					}
					ASSEMBLY_PROFILER_MEASURE("feeder advance p1");
				}
			
				//--------------------------------------------------------------------------
			}
		}
	
		//attendi che la punta sia in posizione di sicurezza
		//--------------------------------------------------------------------------
		WaitPUP(1);
		ASSEMBLY_PROFILER_MEASURE("wait z secured p1");
		
		// setta acc/vel del componente sulla punta 1
		//--------------------------------------------------------------------------
		SetNozzleXYSpeed_Index( DMount.Package[0].speedXY );
	} // if( DMount.mount_flag[0] )

	//GF_THREAD
	//TODO: e' questa la posizione piu' vantaggiosa dove iniziare il centraggio ???
	if( DMount.mount_flag[0] )
	{
		StartCentering( 1, placeAngle[0], &DMount.Package[0] );
	}

	// controlla se necessario centraggio ad assi fermi
	//--------------------------------------------------------------------------
	if( DMount.Package[0].steadyCentering )
	{
		Timer timeoutTimer;
		timeoutTimer.start();

		while( !IsCenteringCompleted( 1 ) )
		{
			delay( 1 );

			if( timeoutTimer.getElapsedTimeInMilliSec() > CENTERING_CYCLE_TIMEOUT )
			{
				W_Mess( "Error - Sniper 1 Centering Timeout !");
				return 0;
			}
		}
	}

	//            FINE PRELIEVO COMPONENTE PUNTA 1
	//-------------------------------------------------------------------------
	
	
	//            INIZIO PRELIEVO COMPONENTE PUNTA 2
	//-------------------------------------------------------------------------

	ASSEMBLY_PROFILER_MEASURE("START pick 2");

	// se punta 2 ha un componente da assemblare
	if( DMount.mount_flag[1] )
	{
		while( !prel_ok[1] )
		{
			// punta 2 in posizione di prelievo
			//--------------------------------------------------------------------------
			if( !Show_step(S_GOPREL,2) )
				return(0);
			
			if( !DMount.Caric[1]->GoPos( 2 ) )
				return 0;
			ASSEMBLY_PROFILER_MEASURE("move xy pick position p2");

			// avvicina la punta alla quota di prelievo
			//--------------------------------------------------------------------------
			if( QParam.ZPreDownMode )
			{
				if(!Show_step(S_PREDISCESA,2))
					return(0);
			
				DoPredown( 2, GetPianoZPos(2)-maxCompHeight );
				ASSEMBLY_PROFILER_MEASURE("z predown p2");
			}

			// attiva contropressione prima di prelevare il comp.
			//--------------------------------------------------------------------------
			Prepick_Contro(2);
			ASSEMBLY_PROFILER_MEASURE("prepick back pressure p2");
			
			// calcola
			//--------------------------------------------------------------------------
			if( DMount.PrgRec[1].Caric < FIRSTTRAY )
			{
				corsa_p = GetZCaricPos(2) - DMount.Caric[1]->GetDataConstRef().C_offprel;
			}
			else
			{
				if(!Show_step(S_VACUON,2))
					return(0);
				
				//vassoio
				corsa_p = GetPianoZPos(2) - DMount.Caric[1]->GetDataConstRef().C_offprel;
				//attiva vuoto
				Set_Vacuo(2,ON);
			}
		
			// init della struttura dati di prelievo
			//---------------------------------------------------------
			preldata.punta=2;
			preldata.zprel=corsa_p;                        //corsa in mm
			preldata.zup=-(DMount.Package[1].z/2+DMount.Package[1].snpZOffset); //quota a cui andare dopo il prelievo
			preldata.stepF=Show_step;                      //puntatore ala funzione handler passo-passo
			preldata.package = &DMount.Package[1];         //struttura package
			preldata.waitup=0;                             //non attendere punta up prima di uscire
			preldata.downpos=&downpos;
			preldata.caric = DMount.Caric[1];

			// attesa punta ferma
			//--------------------------------------------------------------------------
			while( !Check_PuntaRot(2) )
			{
				delay( 1 );
			}
			ASSEMBLY_PROFILER_MEASURE("wait theta p2");
			
			// attesa assi fermi
			//--------------------------------------------------------------------------
			Wait_PuntaXY();
			ASSEMBLY_PROFILER_MEASURE("wait xy p2");

			// attesa caricatore pronto
			//--------------------------------------------------------------------------
			Timer timerFeeder;
			timerFeeder.start();
			
			while( 1 )
			{
				if( QParam.DemoMode )
					break;
				
				//THFEEDER2
				int ret_val = ass_checkcaric( 2 );
				if( ret_val == 1 )
					break;
		
				if( timerFeeder.getElapsedTimeInSec() > CREADY || ret_val == -1 )
				{
					W_Mess(NOCREADY,MSGBOX_YCENT,0,GENERIC_ALARM);
					return 0;
				}
			}
			ASSEMBLY_PROFILER_MEASURE("wait feeder ready p2");
	
			prePrelQuant[1] = DMount.Caric[1]->GetQuantity();
			
			// Preleva componente
			//--------------------------------------------------------------------------
			if( !PrelComp(preldata) )
			{
				return(0);
			}
			
			// decrementa numero di componenti presenti sul caricatore
			DMount.Caric[1]->DecNComp();
			
			ASSEMBLY_PROFILER_MEASURE("pick component p1");
			
			// se centraggio laser
			if( DMount.Package[1].centeringMode == CenteringMode::SNIPER )
			{
				//Se la quota di centraggio e' piu bassa di quella di prerotazione
				//attende fine movimento
				//NOTA: Non dovrebbe mai accadere ! E' una patch per evitare che
				//errori sulla quota di prelievo blocchino il sw.
				if(preldata.zup>(preldata.zprel-DMount.Package[1].z-KSECURITY))
				{
					PuntaZPosWait(2);
				}
				else
				{
					//attende che la punta sia a quota di prerotazione
					do
					{
						if( QParam.DemoMode || Get_OnFile() || !FoxHead->IsEnabled() )
							break;

						while( !centeringMutex.try_lock() )
						{
							delay( 1 );
						}

						pos=FoxHead->ReadPos(STEP2)*P2_ZDIRMOD;

						centeringMutex.unlock();
					} while(pos>downpos-int((DMount.Package[1].z+KSECURITY)*KZmm_passi[1]));
				}
				ASSEMBLY_PROFILER_MEASURE("Wait z prerotation p2");
			}
			
			// segnala caricatore da avanzare
			//--------------------------------------------------------------------------
			ass_carstatus[1] = 0;

			//SMOD260907
			if(QHeader.modal & SCANPREL2_MASK)
			{
				AssScanTest(2,DMount.mount_flag);
			}

			// check componente preso
			//--------------------------------------------------------------------------
			if( DMount.Package[1].checkPick != 'V' )
			{
				if(!Show_step(S_CHKPRESO,2))
					return(0);
				
				NoPreso = Check_CompIsOnNozzle( 2, DMount.Package[1].checkPick );
				ASSEMBLY_PROFILER_MEASURE("check component on nozzle p2");
			}
			else
			{
				NoPreso = 0;
			}
			
			if( !NoPreso )
			{
				// componente preso
				//--------------------------------------------------------------------------
				prel_ok[1] = true;
			}
			else
			{
				//componente NON preso
				//--------------------------------------------------------------------------

				//reload feeder data
				DMount.Caric[1]->ReloadData();

				// riporta la punta alla posizione angolare precedente la prerotazione
				//--------------------------------------------------------------------------
				//120410 vedi assembla() : caso firstComp
				PuntaRotDeg(DMount.pick_angle[1],2);

				//attende punta in posizione di sicurezza
				//--------------------------------------------------------------------------
				WaitPUP(2);
				ASSEMBLY_PROFILER_MEASURE("wait z secured p2");
	
				//incrementa contatore perdite
				prel_perso[1]++;

				//raggiunto il limite massimo di tentativi o modo di richiesta per nuovo tentativo ad ogni presa fallita
				//--------------------------------------------------------------------------
				if( (prel_perso[1]==QParam.N_try) || ((QHeader.debugMode2 & DEBUG2_VACUOCHECK_DBG) && (NoPreso & CHECKCOMPFAIL_VACUO)) )
				{
					//chiede all'utente se vuole riprovare
					if(!NotifyPrelError( 2, NoPreso, DMount.Package[1], DMount.PrgRec[1].CodCom,C_TipoComp[1],UpdateComponentQuantP2))
					{
						return(0);
					}
					
					if( prel_perso[1] == QParam.N_try )
						prel_perso[1] = 0;
				}
				
				// scarica componente su punta
				//--------------------------------------------------------------------------
				if(!AssScaricaComp(2,SCARICACOMP_DEBUG,MsgGetString(Msg_01457)))
				{
					return(0);
				}
				
				// avanza caricatore
				//--------------------------------------------------------------------------
				if(!ass_avcaric(2))
				{
					return(0);
				}
				ASSEMBLY_PROFILER_MEASURE("feeder advance p2");
			}
		}
		
		//attende punta in posizione di sicurezza
		//--------------------------------------------------------------------------
		WaitPUP(2);
		ASSEMBLY_PROFILER_MEASURE("wait z secured p2");
		
		// setto acc/vel minima dei componenti sulle punte
		if( DMount.mount_flag[0] )
			SetMinNozzleXYSpeed_Index( DMount.Package[0].speedXY, DMount.Package[1].speedXY );
		else
			SetNozzleXYSpeed_Index( DMount.Package[1].speedXY );
	} // if( DMount.mount_flag[1] )

	//GF_THREAD
	//TODO: e' questa la posizione piu' vantaggiosa dove iniziare il centraggio ???
	if( DMount.mount_flag[1] )
	{
		StartCentering( 2, placeAngle[1], &DMount.Package[1] );
	}

	// controlla se necessario centraggio ad assi fermi
	//--------------------------------------------------------------------------
	if( DMount.Package[1].steadyCentering )
	{
		Timer timeoutTimer;
		timeoutTimer.start();

		while( !IsCenteringCompleted( 2 ) )
		{
			delay( 1 );

			if( timeoutTimer.getElapsedTimeInMilliSec() > CENTERING_CYCLE_TIMEOUT )
			{
				W_Mess( "Error - Sniper 2 Centering Timeout !");
				return 0;
			}
		}
	}

	//            FINE PRELIEVO COMPONENTE PUNTA 2
	//-------------------------------------------------------------------------
	
	
	//            INIZIO DEPOSITO COMPONENTE PUNTA 1
	//-------------------------------------------------------------------------
	
	ASSEMBLY_PROFILER_MEASURE("START place 1");

	// se punta 1 ha un componente da assemblare
	if( DMount.mount_flag[0] )
	{
		DepoStruct depdata = DEPOSTRUCT_DEFAULT;

		// Coordinate di deposito teoriche
		GetComponentPlacementPosition( DMount.ZerRecs[0], DMount.ZerRecs[1], DMount.PrgRec[0], DMount.Package[0], depdata.xpos, depdata.ypos );
		// Somma correzione package
		depdata.xpos += pkgoffs_x[0];
		depdata.ypos += pkgoffs_y[0];

		if( !IsCenteringCompleted( 1 ) )
		{
			// componente da assemblare e ciclo centraggio iniziato ma non terminato

			// porta la punta 1 alle coordinate di deposito teoriche
			//--------------------------------------------------------------------------
			if( !NozzleXYMove_N( depdata.xpos, depdata.ypos, 1 ) )
			{
				return 0;
			}
			ASSEMBLY_PROFILER_MEASURE("move near place position p1");

			// avanza caricatore
			//--------------------------------------------------------------------------
			if( caric_p1_da_avanzare )
			{
				if( caric_p1_uguale_p2 )
				{
					//THFEEDER
					if( !ass_waitcaric(1) )
					{
						return(0);
					}
					ASSEMBLY_PROFILER_MEASURE("wait feeder p1");
				}

				if(!Show_step(S_CARAVANZ,1))
					return(0);

				if( !ass_avcaric(1) )
				{
					return(0);
				}
				ASSEMBLY_PROFILER_MEASURE("feeder advance p1");
				caric_p1_da_avanzare = false;
			}

			// avanza lo stato di centraggio sniper fino a quando non e' completato
			//--------------------------------------------------------------------------
			Timer timeoutTimer;
			timeoutTimer.start();

			while( !IsCenteringCompleted( 1 ) )
			{
				delay( 1 );

				if( timeoutTimer.getElapsedTimeInMilliSec() > CENTERING_CYCLE_TIMEOUT )
				{
					W_Mess( "Error - Sniper 1 Centering Timeout !");
					return 0;
				}
			}
		}


		// centraggio punta 1 sicuramente terminato
		//--------------------------------------------------------------------------
		GetCenteringResult( 1, centeringResult[0] );

		if( centeringResult[0].Result == STATUS_OK )
		{
			// calcolo del punto di deposito effettivo
			//--------------------------------------------------------------------------
			depdata.xpos += centeringResult[0].Position1;
			depdata.ypos += centeringResult[0].Position2;

			// init della struttura dati di deposito
			//--------------------------------------------------------------------------
			depdata.zpos = GetPianoZPos(1) - PCBh-DMount.Package[0].z;
			depdata.package = &DMount.Package[0];
			depdata.stepF = Show_step;
			depdata.punta = 1;
			depdata.waitup = 0;
			depdata.dorotFlag = 0;
			depdata.goposFlag = 0;
			depdata.xymode = AUTOAPP_NOZSECURITY; //SMOD220103
			depdata.mounted = 0;

			// porta la punta a coordinate di deposito
			//--------------------------------------------------------------------------
			if(!Show_step(S_GODEPO,1))
			{
				return 0;
			}

			if( !NozzleXYMove_N( depdata.xpos, depdata.ypos, 1 ) )
			{
				return 0;
			}
			ASSEMBLY_PROFILER_MEASURE( "move xy place position p1" );

			// avanza caricatore
			//--------------------------------------------------------------------------
			if( caric_p1_da_avanzare )
			{
				if( caric_p1_uguale_p2 )
				{
					//THFEEDER
					if( !ass_waitcaric(1) )
					{
						return(0);
					}
					ASSEMBLY_PROFILER_MEASURE("wait feeder p1");
				}

				if(!Show_step(S_CARAVANZ,1))
					return(0);

				if( !ass_avcaric(1) )
				{
					return(0);
				}
				ASSEMBLY_PROFILER_MEASURE("feeder advance p1");
				caric_p1_da_avanzare = false;
			}

			// esegue prediscesa
			//--------------------------------------------------------------------------
			if(QParam.ZPreDownMode)
			{
				if(!Show_step(S_PREDISCESA,1))
					return(0);

				DoPredown(1,GetPianoZPos(1)-DMount.Package[0].z*PREDEPO_ZTOL-maxCompHeight);
				ASSEMBLY_PROFILER_MEASURE("move z predown p1");
			}

			// attesa assi fermi
			//--------------------------------------------------------------------------
			Wait_PuntaXY();
			ASSEMBLY_PROFILER_MEASURE("wait xy p1");


			if( DMount.Package[0].checkPick == 'V' )
			{
				if(!Show_step(S_CHKPRESO,1))
					return(0);
		
				if( Check_CompIsOnNozzle( 1, DMount.Package[0].checkPick ) )
				{
					vacuocheck_dep[0] = 0;
					//porta punta a zero
					return(1);
				}
				else
				{
					vacuocheck_dep[0] = 1;
				}
				ASSEMBLY_PROFILER_MEASURE("check component on nozzle (pre place V) p1");
			}
			else
			{
				vacuocheck_dep[0] = 1;
			}
	
			// setta speed di deposito
			//--------------------------------------------------------------------------
			SetNozzleZSpeed_Index( 1, DMount.Package[0].speedPlace );

			// Setta i parametri del PID (deposito)
			//--------------------------------------------------------------------------
			BrushDataRead( brushPar, DMount.Package[0].placementPID );

			while( !centeringMutex.try_lock() )
			{
				delay( 1 );
			}

			FoxHead->SetPID(BRUSH1,brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0]);

			centeringMutex.unlock();

			//GF_07_04_2011
			// modo di deposito
			//--------------------------------------------------------------------------
			if( DMount.Package[0].placementMode == 'L' )
			{
				depdata.zpos -= DMount.Package[0].ledth_securitygap;
			}
			else if( DMount.Package[0].placementMode == 'N' )
			{
				depdata.zpos += QHeader.interfNorm;
			}
			else if( DMount.Package[0].placementMode == 'F' )
			{
				depdata.zpos += QHeader.interfFine;
			}

			if(!Show_step(S_PDOWN,1))
				return(0);
			
			PuntaZPosMm( 1, depdata.zpos );
			ASSEMBLY_PROFILER_MEASURE("move z down p1");
			
			PuntaZPosWait(1);
			ASSEMBLY_PROFILER_MEASURE("wait z p1");
			
			//GF_07_04_2011
			if( DMount.Package[0].placementMode == 'L' )
			{
				if(!Show_step(S_PDOWN,1))
					return(0);
				
				// set vel e acc ridotte
				PuntaZSpeed( 1, DMount.Package[0].ledth_place_vel );
				PuntaZAcc( 1, DMount.Package[0].ledth_place_acc );
				PuntaZStartStop( 1, DMount.Package[0].ledth_place_vmin );

				depdata.zpos += DMount.Package[0].ledth_securitygap + DMount.Package[0].ledth_interf;

				PuntaZPosMm( 1, depdata.zpos );
				ASSEMBLY_PROFILER_MEASURE("depo L: move z down p1");
				PuntaZPosWait( 1 );
				ASSEMBLY_PROFILER_MEASURE("depo L: wait z p1");
			}
			
			// Setta i parametri del PID (default)
			//--------------------------------------------------------------------------
			BrushDataRead(brushPar,0);

			while( !centeringMutex.try_lock() )
			{
				delay( 1 );
			}

			FoxHead->SetPID(BRUSH1,brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0]);

			centeringMutex.unlock();

			if( DMount.Package[0].placementMode == 'F' )
			{
				delay(200);
				ASSEMBLY_PROFILER_MEASURE("depo F: place delay p1");
			}

			// determina nuova quota di sicurezza per discesa
			//--------------------------------------------------------------------------
			//SMOD270404
			if(QParam.ZPreDownMode)
			{
				if(DMount.Package[0].z * PREDEPO_ZTOL + PCBh + deltaPreDown > maxCompHeight)
				{
					maxCompHeight = DMount.Package[0].z * PREDEPO_ZTOL + PCBh + deltaPreDown;
				}
			}
			
			ComponentList[(DMount.PrgRec[0].Riga)-1]++;
			PlacedNComp++;
			
			// setta speed/acc di default
			//--------------------------------------------------------------------------
			//GF_07_04_2011
			if( DMount.Package[0].placementMode == 'L' )
			{
				// set vel acc per salita lenta
				PuntaZSpeed( 1, DMount.Package[0].ledth_rising_vel );
				PuntaZAcc( 1, DMount.Package[0].ledth_rising_acc );
				PuntaZStartStop( 1, DMount.Package[0].ledth_rising_vmin );
			}
			else
			{
				SetNozzleZSpeed_Index( 1, ACC_SPEED_DEFAULT );
			}

			SetNozzleRotSpeed_Index( 1, ACC_SPEED_DEFAULT );
			
			RemovePackageFromNozzle_SNIPER( 1 );
			depdata.mounted=1; //setta componente montato=1
		
		
			if(((startOnFile==0) && (startOnFile==Get_OnFile())) || startOnFile==1)
			{
				//SMOD110403
				lastmountP1=DMount.PrgRec[0].Riga-1;
				if(nomountP2==1)
				{
					nomountP2=0;
					lastmount=lastmountP1+1;
				}
				else
				{
					lastmount=lastmountP1;
				}
				assrip_component=lastmount+1;
	
				ncomp++;
			}

			// contropressione deposito
			//--------------------------------------------------------------------------
			//SMOD101104 - introdotto attivazione contropressione anche per deposito di tipo F
			Set_Contro(1,1);

			if(!Show_step(S_CONTROPON,1))
				return(0);

			// attesa contropress.
			delay(QHeader.TContro_Comp);

			Set_Contro(1,0);
			ASSEMBLY_PROFILER_MEASURE("place back pressure p1");
	
			// salita punta
			//--------------------------------------------------------------------------
			if( DMount.Package[0].placementMode == 'L' )
			{
				if(!Show_step(S_PUP,1))
					return(0);

				// porta la punta fuori dal componente
				depdata.zpos -= DMount.Package[0].ledth_interf + DMount.Package[0].ledth_insidedim;
				
				PuntaZPosMm( 1, depdata.zpos );
				ASSEMBLY_PROFILER_MEASURE("depo L: move z up p1");
				PuntaZPosWait( 1 );
				ASSEMBLY_PROFILER_MEASURE("depo L: wait z p1");
				
				// riporta speed/acc di default
				SetNozzleZSpeed_Index( 1, ACC_SPEED_DEFAULT );
			}
			
			if(!Show_step(S_PUP,1))
				return(0);

			// se attivo check componente depositato
			//--------------------------------------------------------------------------
			if( DMount.Package[0].check_post_place )
			{
				// porta la punta a quota di centraggio
				PuntaZPosMm( 1, -(DMount.Package[0].z/2+DMount.Package[0].snpZOffset) );
			}
			else
			{
				PuntaZSecurityPos( 1 );
			}
		}
		else
		{
			// errore in centraggio componente
			//--------------------------------------------------------------------------

			// avanza caricatore
			//--------------------------------------------------------------------------
			if( caric_p1_da_avanzare )
			{
				if( caric_p1_uguale_p2 )
				{
					//THFEEDER
					if( !ass_waitcaric(1) )
					{
						return(0);
					}
					ASSEMBLY_PROFILER_MEASURE("wait feeder p1");
				}
				
				if( !ass_avcaric(1) )
				{
					return(0);
				}
				ASSEMBLY_PROFILER_MEASURE("feeder advance p1");
				caric_p1_da_avanzare = false;
			}
			
			// porta punta a zero
			MoveComponentUpToSafePosition( 1 );

			// notifica errore
			//--------------------------------------------------------------------------
			NotifyCenteringError( 1, centeringResult[0].Result );

			// attesa assi fermi
			//--------------------------------------------------------------------------
			Wait_PuntaXY();
		} // if( !laser_result[0] )
		
		// attende punta in posizione di sicurezza
		//--------------------------------------------------------------------------
		WaitPUP(1);
		ASSEMBLY_PROFILER_MEASURE("wait z secured p1");
	} // if( DMount.mount_flag[0] )

	//            FINE DEPOSITO COMPONENTE PUNTA 1
	//-------------------------------------------------------------------------
	
	
	//            INIZIO DEPOSITO COMPONENTE PUNTA 2
	//-------------------------------------------------------------------------
	
	ASSEMBLY_PROFILER_MEASURE("START place 2");

	if( DMount.mount_flag[1] )
	{
		DepoStruct depdata = DEPOSTRUCT_DEFAULT;

		// Coordinate di deposito teoriche
		GetComponentPlacementPosition( DMount.ZerRecs[0], DMount.ZerRecs[2], DMount.PrgRec[1], DMount.Package[1], depdata.xpos, depdata.ypos );
		// Somma correzione package
		depdata.xpos += pkgoffs_x[1];
		depdata.ypos += pkgoffs_y[1];

		// setto acc/vel del componente sulla punta 2
		//--------------------------------------------------------------------------
		SetNozzleXYSpeed_Index( DMount.Package[1].speedXY );

		if( !IsCenteringCompleted( 2 ) )
		{
			// componente da assemblare e ciclo centraggio iniziato ma non terminato

			// porta la punta 2 alle coordinate di deposito teoriche
			//--------------------------------------------------------------------------
			if( !NozzleXYMove_N( depdata.xpos, depdata.ypos, 2 ) )
			{
				return 0;
			}
			ASSEMBLY_PROFILER_MEASURE("move near place position p2");

			// avanza caricatore
			//--------------------------------------------------------------------------
			if( caric_p2_da_avanzare )
			{
				//THFEEDER
				if( !ass_waitcaric(2) )
				{
					return(0);
				}
				
				if(!Show_step(S_CARAVANZ,2))
					return(0);

				if( !ass_avcaric(2) )
				{
					return(0);
				}
				ASSEMBLY_PROFILER_MEASURE("feeder advance p2");
				caric_p2_da_avanzare = false;
			}

			// avanza lo stato di centraggio sniper fino a quando il ciclo non e' completato
			//--------------------------------------------------------------------------
			Timer timeoutTimer;
			timeoutTimer.start();

			while( !IsCenteringCompleted( 2 ) )
			{
				delay( 1 );

				if( timeoutTimer.getElapsedTimeInMilliSec() > CENTERING_CYCLE_TIMEOUT )
				{
					W_Mess( "Error - Sniper 2 Centering Timeout !");
					return 0;
				}
			}
		}


		// centraggio punta 2 sicuramente terminato
		//--------------------------------------------------------------------------
		GetCenteringResult( 2, centeringResult[1] );

		if( centeringResult[1].Result == STATUS_OK )
		{
			// calcolo del punto di deposito effettivo
			//--------------------------------------------------------------------------
			depdata.xpos += centeringResult[1].Position1;
			depdata.ypos += centeringResult[1].Position2;

			// init della struttura dati di deposito
			//---------------------------------------------------------
			depdata.zpos = GetPianoZPos(2) - PCBh-DMount.Package[1].z;
			depdata.package = &DMount.Package[1];
			depdata.stepF=Show_step;
			depdata.punta=2;
			depdata.waitup=0;
			depdata.dorotFlag=0;
			depdata.goposFlag=0;
			depdata.xymode=AUTOAPP_PUNTA2ON; //SMOD220103
			depdata.mounted=0;

			// porta la punta a coordinate di deposito
			//--------------------------------------------------------------------------
			if(!Show_step(S_GODEPO,2))
				return(0);

			if( !NozzleXYMove_N( depdata.xpos, depdata.ypos, 2, AUTOAPP_PUNTA2ON ) )
			{
				return 0;
			}
			ASSEMBLY_PROFILER_MEASURE("move xy place position p2");

			// avanza caricatore
			//--------------------------------------------------------------------------
			if( caric_p2_da_avanzare )
			{
				//THFEEDER
				if( !ass_waitcaric(2) )
				{
					return(0);
				}

				if(!Show_step(S_CARAVANZ,2))
					return(0);

				if( !ass_avcaric(2) )
				{
					return(0);
				}
				ASSEMBLY_PROFILER_MEASURE("feeder advance p2");
				caric_p2_da_avanzare = false;
			}

			if(QParam.ZPreDownMode)
			{
				if(!Show_step(S_PREDISCESA,2))
					return(0);

				DoPredown(2,GetPianoZPos(2)-DMount.Package[1].z*PREDEPO_ZTOL-maxCompHeight); //SMOD270404
				ASSEMBLY_PROFILER_MEASURE("move z predown p2");
			}

			//attende fine movimento assi
			Wait_PuntaXY();
			ASSEMBLY_PROFILER_MEASURE("wait xy p2");

			#ifdef __ASSEMBLY_DEBUG
			//TEMP
			float encPosX, encPosY;
			Motorhead_GetEncoderMM( encPosX, encPosY );
			AssemblyEncoder.Log( "[2] req: %.2f , %.2f   enc: %.2f , %.2f   (%d %d %s)", reqPosX, reqPosY, encPosX, encPosY,DMount.PrgRec[1].Riga,DMount.PrgRec[1].scheda,DMount.PrgRec[1].CodCom );
			#endif

			if( DMount.Package[1].checkPick == 'V' )
			{
				if(!Show_step(S_CHKPRESO,2))
					return(0);
				
				if( Check_CompIsOnNozzle( 2, DMount.Package[1].checkPick ) )
				{
					vacuocheck_dep[1] = 0;
					//porta punta a zero
					return(1);
				}
				else
				{
					vacuocheck_dep[1] = 1;
				}
				ASSEMBLY_PROFILER_MEASURE("check component on nozzle (pre place V) p2");
			}
			else
			{
				vacuocheck_dep[1] = 1;
			}
		
			// setta speed di deposito
			//--------------------------------------------------------------------------
			SetNozzleZSpeed_Index( 2, DMount.Package[1].speedPlace );

			// Setta i parametri del PID (deposito)
			//--------------------------------------------------------------------------
			BrushDataRead( brushPar, DMount.Package[1].placementPID );

			while( !centeringMutex.try_lock() )
			{
				delay( 1 );
			}

			FoxHead->SetPID(BRUSH2,brushPar.p[1],brushPar.i[1],brushPar.d[1],brushPar.clip[1]);

			centeringMutex.unlock();
	
			//GF_07_04_2011
			// modo di deposito
			//--------------------------------------------------------------------------
			if( DMount.Package[1].placementMode == 'L' )
			{
				depdata.zpos -= DMount.Package[1].ledth_securitygap;
			}
			else if( DMount.Package[1].placementMode == 'N' )
			{
				depdata.zpos += QHeader.interfNorm;
			}
			else if( DMount.Package[1].placementMode == 'F' )
			{
				depdata.zpos += QHeader.interfFine;
			}
		
			if(!Show_step(S_PDOWN,2))
				return(0);
			
			PuntaZPosMm( 2, depdata.zpos );
			ASSEMBLY_PROFILER_MEASURE("move z down p2");
			
			PuntaZPosWait(2);
			ASSEMBLY_PROFILER_MEASURE("wait z p2");
			
			//GF_07_04_2011
			if( DMount.Package[1].placementMode == 'L' )
			{
				if(!Show_step(S_PDOWN,2))
					return(0);
				
				// settare vel e acc ridotte
				PuntaZSpeed( 2, DMount.Package[1].ledth_place_vel );
				PuntaZAcc( 2, DMount.Package[1].ledth_place_acc );
				PuntaZStartStop( 2, DMount.Package[1].ledth_place_vmin );
				
				depdata.zpos += DMount.Package[1].ledth_securitygap + DMount.Package[1].ledth_interf;
				
				PuntaZPosMm( 2, depdata.zpos );
				ASSEMBLY_PROFILER_MEASURE("depo L: move z down p2");
				PuntaZPosWait( 2 );
				ASSEMBLY_PROFILER_MEASURE("depo L: wait z p2");
			}
		
			// Setta i parametri del PID (default)
			//--------------------------------------------------------------------------
			BrushDataRead(brushPar,0);

			while( !centeringMutex.try_lock() )
			{
				delay( 1 );
			}

			FoxHead->SetPID(BRUSH2,brushPar.p[1],brushPar.i[1],brushPar.d[1],brushPar.clip[1]);

			centeringMutex.unlock();

			if( DMount.Package[1].placementMode == 'F' )
			{
				delay(200);
				ASSEMBLY_PROFILER_MEASURE("depo F: place delay p2");
			}
	
			// determina nuova quota di sicurezza per discesa
			//--------------------------------------------------------------------------
			//SMOD270404
			if(QParam.ZPreDownMode)
			{
				if(DMount.Package[1].z * PREDEPO_ZTOL + PCBh + deltaPreDown > maxCompHeight)
				{
					maxCompHeight = DMount.Package[1].z * PREDEPO_ZTOL + PCBh + deltaPreDown;
				}
			}
	
			ComponentList[(DMount.PrgRec[1].Riga)-1]++;
			PlacedNComp++;
		
			// setta speed/acc di default
			//--------------------------------------------------------------------------
			//GF_07_04_2011
			if( DMount.Package[1].placementMode == 'L' )
			{
				// set vel acc per salita lenta
				PuntaZSpeed( 2, DMount.Package[1].ledth_rising_vel );
				PuntaZAcc( 2, DMount.Package[1].ledth_rising_acc );
				PuntaZStartStop( 2, DMount.Package[1].ledth_rising_vmin );
			}
			else
			{
				SetNozzleZSpeed_Index( 2, ACC_SPEED_DEFAULT );
			}

			SetNozzleRotSpeed_Index( 2, ACC_SPEED_DEFAULT );
			
			RemovePackageFromNozzle_SNIPER( 2 );
			depdata.mounted=1; //setta componente montato=1
	

			if(((startOnFile==0) && (startOnFile==Get_OnFile())) || startOnFile==1)
			{
				//SMOD110403
				if(DMount.mount_flag[0])
				{
					lastmountP2=DMount.PrgRec[1].Riga-1;
				}
				
				lastmount=DMount.PrgRec[1].Riga-1;
				
				assrip_component=lastmount+1;
		
				ncomp++;
			}
	
			// contropressione deposito
			//--------------------------------------------------------------------------
			//SMOD101104 - introdotto attivazione contropressione anche per deposito di tipo F
			Set_Contro(2,1);

			if(!Show_step(S_CONTROPON,2))
				return(0);

			// attesa contropress.
			delay(QHeader.TContro_Comp);

			Set_Contro(2,0);
			ASSEMBLY_PROFILER_MEASURE("place back pressure p2");

			// salita punta
			//--------------------------------------------------------------------------
			//GF_07_04_2011
			// se attiva salita lenta
			if( DMount.Package[1].placementMode == 'L' )
			{
				if(!Show_step(S_PUP,2))
					return(0);
				
				// porta la punta fuori dal componente
				depdata.zpos -= DMount.Package[1].ledth_interf + DMount.Package[1].ledth_insidedim;
				
				PuntaZPosMm( 2, depdata.zpos );
				ASSEMBLY_PROFILER_MEASURE("depo L: move z up p2");
				PuntaZPosWait( 2 );
				ASSEMBLY_PROFILER_MEASURE("depo L: wait z p2");
				
				// riporta speed/acc di default
				SetNozzleZSpeed_Index( 2, ACC_SPEED_DEFAULT );
			}
			
			if(!Show_step(S_PUP,2))
				return(0);

			// se attivo check componente depositato
			//--------------------------------------------------------------------------
			if( DMount.Package[1].check_post_place )
			{
				// porta la punta a quota di centraggio
				PuntaZPosMm( 2, -(DMount.Package[1].z/2+DMount.Package[1].snpZOffset) );
			}
			else
			{
				PuntaZSecurityPos( 2 );
			}
		}
		else
		{
			// errore in centraggio componente
			//--------------------------------------------------------------------------

			// avanza caricatore
			//--------------------------------------------------------------------------
			if( caric_p2_da_avanzare )
			{
				//THFEEDER
				if( !ass_waitcaric(2) )
				{
					return(0);
				}
				
				if(!Show_step(S_CARAVANZ,2))
					return(0);
				
				if( !ass_avcaric(2) )
				{
					return(0);
				}
				ASSEMBLY_PROFILER_MEASURE("feeder advance p2");
				caric_p2_da_avanzare = false;
			}

			// porta punta a zero
			MoveComponentUpToSafePosition(2);

			// notifica errore
			//--------------------------------------------------------------------------
			NotifyCenteringError( 2, centeringResult[1].Result );

			// attesa assi fermi
			//--------------------------------------------------------------------------
			Wait_PuntaXY();
		}

		// attende punta in posizione di sicurezza
		//--------------------------------------------------------------------------
		WaitPUP(2);
		ASSEMBLY_PROFILER_MEASURE("wait z secured p2");
	} // if( DMount.mount_flag[1] )


	//            CONTROLLO PRESENZA COMPONENTE PUNTA 1 e 2
	//-------------------------------------------------------------------------
	
	// se componente su punta 1 e nessun errore
	//--------------------------------------------------------------------------
	if( DMount.mount_flag[0] && centeringResult[0].Result == STATUS_OK )
	{
		// check componente depositato
		//--------------------------------------------------------------------------
		if( DMount.Package[0].check_post_place )
		{
			if( !Show_step( "Post place check", 1 ) )
				return(0);

			PuntaZPosWait( 1 );

			Set_Vacuo( 1, 1 );
			delay( 300 );

			if( Sniper_CheckComp( 1 ) )
			{
				// errore: componente non depositato
				compcheck_post_dep[0] = 0;
			}

			// disattivo vuoto
			Set_Vacuo( 1, 0 );

			ASSEMBLY_PROFILER_MEASURE("post place component presence check p1");
		}
	}
	
	// se componente su punta 2 e nessun errore
	//--------------------------------------------------------------------------
	if( DMount.mount_flag[1] && centeringResult[1].Result == STATUS_OK )
	{
		// check componente depositato
		//--------------------------------------------------------------------------
		if( DMount.Package[1].check_post_place )
		{
			if( !Show_step( "Post place check", 2 ) )
				return(0);

			PuntaZPosWait( 2 );

			Set_Vacuo( 2, 1 );
			delay( 300 );

			if( Sniper_CheckComp( 2 ) )
			{
				// errore: componente non depositato
				compcheck_post_dep[1] = 0;
			}

			// disattivo vuoto
			Set_Vacuo( 2, 0 );

			ASSEMBLY_PROFILER_MEASURE("post place component presence check p2");
		}
	}

	return 1;
}
