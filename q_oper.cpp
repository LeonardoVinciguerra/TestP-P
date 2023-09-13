/*
>>>> Q_OPER.CPP

Funzioni operative e di azionamento attuatori.
-- Per le rotazioni delle teste, vedere anche il modulo Q_HEAD.

Per i dati relativi ad I/O address, ecc. vedere Q_OPERT.H

Classificazione delle funzioni nel modulo per nome:

Input   :  I_NomeFunz
Output  :  O_NomeFunz
Gener.  :  X_NomeFunz
Interf. :  F_NomeFunz

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

+++   Settaggio I/O modificato con hold stato porte in array
+++   by Moretti/Festa

++++  Modificato da TWS Simone 06.08.96
++++  Modificato da WALTER 16.11.96 / W03 / W0204 / W042000

*/

#include <boost/thread.hpp>

#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "filefn.h"
#include "bitmap.h"
#include "q_init.h"
#include "q_inifile.h"
#include "q_dosat.h"
#include "msglist.h"
#include "q_cost.h"
#include "q_debug.h"
#include "q_wind.h"
#include "motorhead.h"
#include "q_tabe.h"
#include "q_ugeobj.h"
#include "tv.h"
#include "q_help.h"
#include "q_files.h"
#include "q_files_new.h"
#include "q_opert.h"
#include "q_oper.h"
#include "q_assem.h"
#include "q_param.h"
#include "q_vision.h"
#include "q_mapping.h"
#include "q_conf.h"
#include "q_prog.h"
#include "q_progt.h"
#include "q_packages.h"
#include "q_conveyor.h"

#include "q_ser.h"
#include "q_testh.h"
#include "commclass.h"

#include "q_carint.h"

#include "q_camera.h"

#include "gui_functions.h"
#include "q_grcol.h"
#include <unistd.h>
#include "q_fox.h"

#ifdef __SNIPER
#include "sniper.h"
#include "tws_sniper.h"
#include "las_scan.h"
#include "q_snprt.h"
#endif

#include "stepper_modules.h"

#include "q_carobj.h"
#include "q_zerit.h"

#include "stepmsg.h"

#include "keyutils.h"
#include "lnxdefs.h"
#include "strutils.h"

#include "Timer.h"

#include "gui_defs.h"
#include "c_inputbox.h"
#include "c_pan.h"

#ifdef __LOG_ERROR
#include "q_logger.h"
extern CLogger QuadraLogger;
#endif

#include <mss.h>


//GF_THREAD
extern boost::mutex centeringMutex;

extern SSpeedsTable SpeedsTable;
extern struct CfgTeste MapTeste;
extern struct cur_data  CurDat;

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;

int currentZAcc[2] = { 0, 0 };
int criticalZAcc[2] = { 0, 0 };


void UpdateComponentQuant( int key, FeederClass* feeder );


// Testo autoapprendimento
char auto_text1[50] = "";
char auto_text2[50] = "";
char auto_text3[50] = "";


float AutoApprendXYRot=0; //SMOD180208


#ifdef HWTEST_RELEASE
extern int autozero;
#endif

extern struct CfgDispenser CC_Dispenser; // L1905


// public nel modulo: variabili e strutture.
extern struct CfgParam  QParam;
extern struct CfgHeader QHeader;
extern struct vis_data Vision;
extern FeederFile* CarFile;
extern SPackageData currentLibPackages[MAXPACK];
extern GUI_SubMenu* Q_ShortCutsMenu;

float XYApprend_Zpos[2]={0,0};
SPackageData PackageOnNozzle[2];

int PackageOnNozzleFlag[2];
float PackageOnNozzlePickedRotation[2];

//SMOD260506-FinecFix
//Flag forza stato finecorsa impedendo cambiamenti finche il flag e' abilitato
int FinecForce_enabled=0;


int finec_en = 0;   // Flag attivazione controllo finecorsa
int _onFile = 0;    // Flag uscita su file

// valore soglia presa componente
int SogliaVacuo[2];

static int global_passi[2] = { 0, 0 };

bool use_orthoXY_correction = true;
bool use_encScale_correction = true;
bool use_mapping_correction = true;


struct _xyStatus
{
	_xyStatus()
	{
		xPosLogical = yPosLogical = 0.f;
		xPosPhysical = yPosPhysical = 0.f;
		xyAcc = xySpeed = 0;
		savedXYAcc = savedXYSpeed = 0;

		for( int i = 0; i < 2; i++ )
		{
			rotAcc[i] = rotSpeed[i] = 0;
			savedRotAcc[i] = savedRotSpeed[i] = 0;
		}
	}

	float xPosLogical, yPosLogical;
	float xPosPhysical, yPosPhysical;

	int xyAcc, xySpeed;
	int savedXYAcc, savedXYSpeed;
	int rotAcc[2], rotSpeed[2];
	int savedRotAcc[2], savedRotSpeed[2];
};

_xyStatus CurXYStat;


// valori passo da aggiungere in autoapprendimento movimento di un passo
float K_PassoX;		// L2109
float K_PassoY;		// L2109
int press_count;	// L2109
int time_out;

static char vac_status[2]={0,0};
static char contro_status[2]={0,0};

int lastZCheckErr[2]={0,0};

int alarm_enabled=0;

int autoapp_extcam_rotation=0;

int vacuoOffset[2] = {0,0};

int autoapp_running = 0;

struct confirm_required_before_next_xy_movement_s
{
	bool active;
	bool timer_active;
	bool timer_first;
	clock_t tstart;
} confirm_required_before_next_xy_movement =
{
	true,
	false,
	false,
	0
};

void SetConfirmRequiredBeforeNextXYMovement(bool r)
{
	confirm_required_before_next_xy_movement.active = r;
}

void EnableTimerConfirmRequiredBeforeNextXYMovement(bool e)
{
	if(e)
	{
		if(!confirm_required_before_next_xy_movement.timer_active)
		{
			confirm_required_before_next_xy_movement.timer_first = true;
		}
	}
	else
	{
		confirm_required_before_next_xy_movement.timer_first = false;
	}
	confirm_required_before_next_xy_movement.timer_active = e;
}

void ResetTimerConfirmRequiredBeforeNextXYMovement(void)
{
	confirm_required_before_next_xy_movement.tstart = clock();
}

bool isTimerConfirmRequestedBeforeNextXYMovementElapsed(void)
{
	float dt = float(clock() - confirm_required_before_next_xy_movement.tstart) / CLOCKS_PER_SEC;
	
	if(dt > CSecurityReservedParametersFile::inst().get_data().mov_confirm_timeout)
	{
		return true;
	}
	else
	{
		return false;
	}	
}

bool IsConfirmRequiredBeforeNextXYMovement(void)
{
	if( !Get_OnFile() )
	{
		//quando i movimenti sono disattivati non chiedere mai la conferma
		return false;
	}

	if( CSecurityReservedParametersFile::inst().get_data().flags.bits.mov_confirm_disabled ||
		!confirm_required_before_next_xy_movement.active )
	{
		return false;
	}

	if( confirm_required_before_next_xy_movement.timer_active )
	{
		if( !confirm_required_before_next_xy_movement.timer_first )
		{
			return isTimerConfirmRequestedBeforeNextXYMovementElapsed();
		}
		else
		{
			confirm_required_before_next_xy_movement.timer_first = false;
		}
	}

	return true;
}


void ResetXYStatus(void)
{
	memset(&CurXYStat,(char)0,sizeof(CurXYStat));
}

//---------------------------------------------------------------------------------
// Resetta i parametri di controllo per la movimentazione degli assi x/y
//----------------------------------------------------------------------------------
void ResetXYVariables(void)
{
	CurXYStat.xPosLogical=0;
	CurXYStat.yPosLogical=0;
	
	CurXYStat.xPosPhysical=0;
	CurXYStat.yPosPhysical=0;
}

//----------------------------------------------------------------------------------
// Legge offset vacuostati
//----------------------------------------------------------------------------------
void Init_VacuoReading(void)
{
	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	short int cha,chb;
	if(FoxHead->ReadADC(cha,chb))
	{
		vacuoOffset[0]=(cha-102)/4;
		vacuoOffset[1]=(chb-102)/4;
	}

	centeringMutex.unlock();
}

/*---------------------------------------------------------------------------------
Funzione di lettura dei vacuostati.
Parametri di ingresso:
   Tipo: vacuostato sul quale effettuare la lettura
             1: vacuostato punta 1
             2: vacuostato punta 2
             altro:vacuostato 1
Valori di ritorno:
   Ritorna il valore letto dal vacuostato, dove i valori hanno la seguente scala:
      100: identifica il vuoto assoluto (-1 Atm)
      0: identifica la condizione di pressione atmosferica (1 Atm) 
----------------------------------------------------------------------------------*/
int Get_Vacuo(int Tipo)
{
	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	int vuoto;
	short int cha,chb;
	
	FoxHead->ReadADC(cha,chb);
	
	if(Tipo==2)
	{
		vuoto=((chb-102)/4) - vacuoOffset[1];
	}
	else
	{
		vuoto=((cha-102)/4) - vacuoOffset[0];
	}

	if(vuoto>99) 
		vuoto=99;

	centeringMutex.unlock();
	return vuoto;
}


/*---------------------------------------------------------------------------------
Check di caricatore pronto.
Parametri di ingresso:
   nessuno
Valori di ritorno:
   Ritorna 0 se il caricatore e' pronto, 1 altrimenti.
----------------------------------------------------------------------------------*/
int Check_Caric(void)
{
	if( Get_OnFile() )
		return 0;
	
	if((cpu_cmd(1,(char*)"c"))!='K')
		return(1);  // not ready
	else
		return 0;  // ready
}


//---------------------------------------------------------------------------------
// Controlla ingresso di sicurezza
// Ritorna: 1 se la proteziona e' aperta
//          0 altrimenti
//---------------------------------------------------------------------------------
bool CheckSecurityInput()
{
	#ifdef HWTEST_RELEASE
	return false;
	#endif

	if( Get_OnFile() )
	{
		return false;
	}

	if( cpu_cmd( 2, (char*)"EP" ) == 'O' )
	{
		return true;
	}
	return false;
}

#ifdef HWTEST_RELEASE
bool CheckSecurityInput_HT()
{
	if( Get_OnFile() )
	{
		return false;
	}

	if( cpu_cmd( 2, (char*)"EP" ) == 'O' )
	{
		return true;
	}
	return false;
}
#endif


void Request_SHIFT2_ToContinue()
{
	int cam = pauseLiveVideo();

	CPan* tips = new CPan( -1, 1, MsgGetString(Msg_05134) );
	bipbip();

	while( 1 )
	{
		int key = keyRead();
		if( key == (K_LSHIFT | K_RSHIFT) )
		{
			break;
		}
	}

	delete tips;

	ResetTimerConfirmRequiredBeforeNextXYMovement();
	playLiveVideo( cam );
}

void Request_F5_ToContinue()
{
	int cam = pauseLiveVideo();

	CPan* tips = new CPan( -1, 1, MsgGetString(Msg_05133) );
	bipbip();

	while( 1 )
	{
		int key = keyRead();
		if( key == K_F5 )
		{
			break;
		}

		if( isTimerConfirmRequestedBeforeNextXYMovementElapsed() )
		{
			delete tips;
			tips = 0;
			Request_SHIFT2_ToContinue();
			break;
		}
	}

	if( tips )
		delete tips;

	ResetTimerConfirmRequiredBeforeNextXYMovement();
	playLiveVideo( cam );
}

void Request_Confirm_ToContinue()
{
	if( !isTimerConfirmRequestedBeforeNextXYMovementElapsed() )
	{
		ResetTimerConfirmRequiredBeforeNextXYMovement();

		Request_F5_ToContinue();
	}
	else
	{
		Request_SHIFT2_ToContinue();
	}
}

//TODO: controllare
int CheckProtectionIsWorking(void)
{
	int r = 1;
	
	int cam = pauseLiveVideo();
	
	CPan* pan = new CPan( -1, 1, MsgGetString(Msg_05135) );

	enum
	{
		wait_open,
		opened,
		closed,
	} state = wait_open;
	
	while(1)
	{
		switch(state)
		{
			case wait_open:
				if( CheckSecurityInput() )
				{
					state = opened;
				}
				break;
			case opened:
				if( !CheckSecurityInput() )
				{
					state = closed;
				}
				break;

			default:
				break;
		}
		
		if(state == closed)
		{
			ResetTimerConfirmRequiredBeforeNextXYMovement();
			break;
		}

		if( keyRead() == K_ESC )
		{
			if(W_Deci(0,ASK_MOV_DISABLE))
			{
				r=0;
				ResetNozzles();
				Set_OnFile(1);
				W_Mess( MsgGetString(Msg_01317) );
				break;
			}
		}
	}

	delete pan;
	
	if(r)
	{
		Request_Confirm_ToContinue();
	}

	playLiveVideo( cam );
	
	return(r);
}

//---------------------------------------------------------------------------------
// Se protezione aperta mostra messaggio di avviso e attende fino a chiusura
//---------------------------------------------------------------------------------
int CheckProtection_WaitClose()
{
	#ifdef HWTEST_RELEASE
	return 1;
	#endif

	if( Get_OnFile() )
	{
		return 1;
	}

	while( 1 )
	{
		// se protezione aperta
		if( CheckSecurityInput() )
		{
			bipbip();

			// se movimenti con protezione aperta non permessi (default)
			if( UseSaferProtection() )
			{
				// mostra messaggio a video e attendi chiusura protezione
				int cam = pauseLiveVideo();

				CPan* tips = new CPan( -1, 3, MsgGetString(Msg_02079), "", MsgGetString(Msg_02080) );

				while( 1 )
				{
					// se protezione chiusa
					if( !CheckSecurityInput() )
					{
						ResetTimerConfirmRequiredBeforeNextXYMovement();
						break;
					}
				}

				delete tips;

				// richiedi conferma per ripartire con il movimento
				Request_Confirm_ToContinue();

				playLiveVideo( cam );
			}
		}
		else
		{
			break;
		}
	}

	return 1;
}



//----------------------------------------------------------------------------------
// Controllo ingresso fine corsa motore
// Ritorna 1 se il fine corsa e' stato raggiunto, 0 altrimenti
//----------------------------------------------------------------------------------
int CheckLimitSwitchX()
{
	int state = 0;
	if( !Motorhead->GetLimitSwitchInput( HEADX_ID, state ) )
	{
		return 0;
	}
	return state;
}
int CheckLimitSwitchY()
{
	int state = 0;
	if( !Motorhead->GetLimitSwitchInput( HEADY_ID, state ) )
	{
		return 0;
	}
	return state;
}

//----------------------------------------------------------------------------------
// Controllo fine corsa asse X
// Ritorna 1 se il fine corsa e' stato raggiunto, 0 altrimenti
//----------------------------------------------------------------------------------
int CheckLimitsX()
{
	if( !Get_UseFinec() )
		return 0;

	int status;
	if( !Motorhead->GetStatus( HEADX_ID, status ) )
	{
		return 0;
	}

	return status & MOTOR_OVERRUN ? 1 : 0;
}

//----------------------------------------------------------------------------------
// Controllo fine corsa asse Y
// Ritorna 1 se il fine corsa e' stato raggiunto, 0 altrimenti
//----------------------------------------------------------------------------------
int CheckLimitsY()
{
	if( !Get_UseFinec() )
		return 0;

	int status;
	if( !Motorhead->GetStatus( HEADY_ID, status ) )
	{
		return 0;
	}

	return status & MOTOR_OVERRUN ? 1 : 0;
}

//----------------------------------------------------------------------------------
// Controllo fine corsa asse motore
// Ritorna 1 se il fine corsa e' stato raggiunto, 0 altrimenti
//----------------------------------------------------------------------------------
int CheckAxisOverrun( int module )
{
	if( !Get_UseFinec() )
		return 0;

	int status;
	if( !Motorhead->GetStatus( module, status ) )
	{
		return 0;
	}

	return status & MOTOR_OVERRUN ? 1 : 0;
}

//---------------------------------------------------------------------------------
// Check dei finecorsa meccanici
// Ritorna 0 se un finecorsa e' basso (aperto), 1 altrimenti
//---------------------------------------------------------------------------------
int FinecStatus()
{
	if( Get_OnFile() )
	{
		return 1;
	}

	if( CheckAxisOverrun( HEADX_ID ) || CheckAxisOverrun( HEADY_ID ) )
	{
		//W_Mess( MsgGetString(Msg_00005),MSGBOX_YCENT,0,GENERIC_ALARM );

		Motorhead->DisableMotors();
		Set_OnFile( 1 );

		return 0;
	}

	return 1;
}


/*---------------------------------------------------------------------------------
Check del microswitch di protezione premuto (non utilizzati attualmente).
Parametri di ingresso:
   Tipo: identifica il microswitch da testare
             0: check asse x
             1: check asse y
             altro:check asse y
Valori di ritorno:
   Ritorna 0 se lo switch e' attivo.
----------------------------------------------------------------------------------*/
int Check_ProtSwitch(int Tipo)
{
	char buf[6];
	
	buf[0]='E';                 // comando
	
	if(!Tipo)
		buf[1]='X';
	else
		buf[1]='Y';
	
	buf[2]=0;                   // terminatore
	
	if(cpu_cmd(2,buf)=='C') 
		return 0;    // go
	else
		return(1);
}


/*---------------------------------------------------------------------------------
Funzione a basso livello per il check della presenza della pressione dell'aria.
Parametri di ingresso:
   force: se=1 forza il controllo di pressione anche se e' disattivato
Valori di ritorno:
   Ritorna 1 se la pressione dell'aria e' presente, 0 altrimenti.
----------------------------------------------------------------------------------*/
int Check_Press(int force/*=0*/)
{
	#ifdef HWTEST_RELEASE
	return 1;
	#endif

	// se sensore pressione disattivato ritorna 1
	if( !QParam.AT_press )
	{
		return 1;
	}

	if( !force && (Get_OnFile() || QParam.DemoMode || !Get_UseCpu()) )
	{
		return 1;
	}

	int status = cpu_cmd(2,(char*)"EA");
	if( status == 'C' || status=='X' || status=='Y' )
	{
		return 1;
	}
	return 0;
}


/*---------------------------------------------------------------------------------
Ritorna stato del motore dell'asse z.
Parametri di ingresso:
   punta:  punta della quale attendere l'arresto
               1: punta 1
               2: punta 2
               altro:punta 1
Valori di ritorno:
   Ritorna 1 se il motore e' in movimento, 0 se e' fermo.
----------------------------------------------------------------------------------*/
int Check_PuntaZPos(char punta)
{
	if( punta > 2 || punta < 1 )
		punta=1;
	punta--;

	int ret = 0;

	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	if(FoxPort->enabled)
	{
		int readstat=FoxHead->ReadStatus(punta*2+1);
		
		if( readstat & CODABUSY_MASK )
		{
			ret = 1;
		}
	}

	centeringMutex.unlock();

	return ret;
}


/*---------------------------------------------------------------------------------
Ciclo di attesa per fine movimento lungo l'asse z per la punta specificata.
Parametri di ingresso:
   punta: punta della quale attendere l'arresto
               1: punta 1
               2: punta 2
----------------------------------------------------------------------------------*/
void PuntaZPosWait( int punta )
{
	if( Get_OnFile() )
	  return;

	int moving = 1;

	while( moving )
	{
		moving = Check_PuntaZPos( punta );
	}
}


/*---------------------------------------------------------------------------------
Attesa punta in posizione di sicurezza per movimento assi
Parametri di ingresso:
   punta:  punta della quale attendere l'arresto
               1: punta 1
               2: punta 2
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void WaitPUP( int punta )
{
	if( Get_OnFile() )
		return;

	if(punta<1 || punta>2)
	{
		punta=1;
	}

	FoxPort->PauseLog(); //SMOD250903
	
	Timer timeoutTimer;
	timeoutTimer.start();
	
	int pos;
	
	do
	{
		if(!Check_PuntaZPos(punta))
		{
			//esci se il motore e' fermo
			break;
		}

		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		if(punta==1)
		{
			pos=FoxHead->ReadPos(STEP1)*P1_ZDIRMOD;
		}
		else
		{
			pos=FoxHead->ReadPos(STEP2)*P2_ZDIRMOD;
		}

		if( timeoutTimer.getElapsedTimeInMilliSec() > WAITPUP_TIMEOUT )
		{
			char buf[80];
			snprintf( buf, sizeof(buf),WAITPUP_ERROR,punta);
			W_Mess(buf,MSGBOX_YLOW,0,GENERIC_ALARM);
		
			Set_OnFile(1);
			break;
		}

		centeringMutex.unlock();
	
	} while(pos>QHeader.FoxPos_HeadSens[punta-1]); //SMOD240907
	
	FoxPort->RestartLog(); //SMOD250903
}


float GetLogicalZPosMm(unsigned int nozzle,float z)
{
	CfgUgelli dat;
	if(Ugelli->GetRec(dat,nozzle))
	{ 
		z += dat.Z_offset[nozzle-1];
	}	
	return z;	
}

float GetPhysZPosMm(unsigned int nozzle,float z)
{
	CfgUgelli dat;
	if(Ugelli->GetRec(dat,nozzle))
	{ 
		z -= dat.Z_offset[nozzle-1];
	}	
	return z;
}

float GetPhysZPosMm(unsigned int nozzle)
{
	float z = PuntaZPosMm(nozzle,0,RET_POS);
		
	CfgUgelli dat;
	if(Ugelli->GetRec(dat,nozzle))
	{ 
		z -= dat.Z_offset[nozzle-1];
	}	
	return z;
}

/*---------------------------------------------------------------------------------
Movimento punte del numero di passi fornito come parametro.
Uguale a PuntaMov ma le posizioni sono espresse sempre in passi.
Parametri di ingresso:
   xpunta:  punta da muovere
                 1: punta 1
                 2: punta 2
                 altro:punta 1
   pos: movimento in passi da effettuare
   mode: modalita' di movimentazione
              ABS_MOVE: movimento assoluto della quantita' specificata
              REL_MOVE: movimento della quantita' specificata
              RET_POS: non effettua il movimento, ritorna esclusivamente la posizione attuale (in passi)
              SET_POS: non effettua il movimento, setta la posizione attuale pari al valore pos specificato
   limitOn:   attiva o disattiva check limiti movimentazione
              ZLIMITON : attiva limiti in movimentazione     (default)
              ZLIMITOFF: disabilita limiti in movimentazione
Valori di ritorno:
   Ritorna la posizione attuale in passi.
----------------------------------------------------------------------------------*/
//##SMOD300802 - DANY200103
int PuntaZPosStep(int xpunta, int pos, int mode,int limitOn)
{
	static int curr_pos[2]={0,0};

	int mod;
	float *step_const=&(QHeader.Step_Trasl1);
	int delta=0;
	float k;

	if( Get_OnFile() )
	{
		return 0;
	}

	if((xpunta>2) || (xpunta<1)) 
	{
		xpunta=1;
	}
	
	PuntaZPosWait(xpunta);
	
	xpunta--;
	
	if(mode==ABS_MOVE || mode==RET_POS)
	{
		CfgUgelli dat;
		
		if(Ugelli->GetRec(dat,xpunta+1))
		{
			delta=int(dat.Z_offset[xpunta]*step_const[xpunta]);
			if(mode==ABS_MOVE)
			{
				pos-=delta;
			}
		}
	}
	
	if(xpunta)
	{
		mod=P2_ZDIRMOD;
		k=QHeader.Step_Trasl2;
	}
	else
	{
		mod=P1_ZDIRMOD;
		k=QHeader.Step_Trasl1;
	}
	
	pos*=mod;


	if(limitOn==ZLIMITON)
	{
		//Se supero i limiti previsti, taglio il valore di pos!
		if( mode==REL_MOVE )
		{
			if((curr_pos[xpunta]+pos)*mod/k>=QHeader.Max_NozHeight[xpunta])
			{
				pos=int(QHeader.Max_NozHeight[xpunta]*k*mod)-curr_pos[xpunta];
			}
	
			if((curr_pos[xpunta]+pos)*mod/k<=QHeader.Min_NozHeight[xpunta])
			{
				pos=int(QHeader.Min_NozHeight[xpunta]*k*mod)-curr_pos[xpunta];
			}
		}
		else
		{
			if(pos*mod/k>=QHeader.Max_NozHeight[xpunta])
			{
				pos=int(QHeader.Max_NozHeight[xpunta]*k*mod);
			}
				
			if(pos*mod/k<=QHeader.Min_NozHeight[xpunta])
			{
				pos=int(QHeader.Min_NozHeight[xpunta]*k*mod);
			}
		}
	}

	switch (mode) 
	{
		case SET_POS:
			//print_debug("SET POS \n prev=%d prevfox=%d next=%d\n",curr_pos[xpunta],fpos,pos);
			curr_pos[xpunta]=pos;
			break;
			
		case REL_MOVE:
			//print_debug("REL MOVE\n prev=%d prevfox=%d step=%d next=%d\n",curr_pos[xpunta],fpos,pos,curr_pos[xpunta]+pos);
	
			//SMOD031003 START
			if(abs(pos)<(int(CRITICAL_ZMM*step_const[xpunta])))
			{
				if( currentZAcc[xpunta] > CRITICAL_ZACC )
				{
					int tmp_acc=currentZAcc[xpunta];
					PuntaZAcc(xpunta+1,CRITICAL_ZACC);
					criticalZAcc[xpunta]=tmp_acc;
				}
			}
			else
			{
				if(criticalZAcc[xpunta]!=0)
				{
					PuntaZAcc(xpunta+1,criticalZAcc[xpunta]);
					criticalZAcc[xpunta]=0;
				}
			}
			//SMOD031003 END
	
			curr_pos[xpunta]+=pos;

			while( !centeringMutex.try_lock() )
			{
				delay( 1 );
			}

			FoxHead->MoveAbs(xpunta*2+1,curr_pos[xpunta]);

			centeringMutex.unlock();
			break;
			
		case ABS_MOVE:
			//print_debug("ABS MOVE\n prev=%d prevfox=%d step=%d\n",curr_pos[xpunta],fpos,pos);
	
			//SMOD031003 START
			if(abs(curr_pos[xpunta]-pos)<(int(CRITICAL_ZMM*step_const[xpunta])))
			{
				if( currentZAcc[xpunta] > CRITICAL_ZACC )
				{
					int tmp_acc = currentZAcc[xpunta];
					PuntaZAcc(xpunta+1,CRITICAL_ZACC);
					criticalZAcc[xpunta]=tmp_acc;
				}
			}
			else
			{
				if(criticalZAcc[xpunta]!=0)
				{
					PuntaZAcc(xpunta+1,criticalZAcc[xpunta]);
					criticalZAcc[xpunta]=0;
				}
			}
			//SMOD031003 END
			
			curr_pos[xpunta]=pos;

			while( !centeringMutex.try_lock() )
			{
				delay( 1 );
			}

			FoxHead->MoveAbs(xpunta*2+1,curr_pos[xpunta]);

			centeringMutex.unlock();
			break;
			
		case RET_POS:
			if(xpunta)
			{
				return(curr_pos[1]*P2_ZDIRMOD+delta);
			}
			else
			{	
				return(curr_pos[0]*P1_ZDIRMOD+delta);
			}
			break;
	}
	
	return curr_pos[xpunta];
} // PuntaZPosStep



/*---------------------------------------------------------------------------------
Movimento punte con conversione automatica da mm a passi.
Parametri di ingresso:
   punta:  punta da muovere
               1: punta 1
               2: punta 2
   mm: movimento in mm da effettuare
   mode: modalita' di movimentazione
              ABS_MOVE: movimento assoluto della quantita' specificata
              REL_MOVE: movimento della quantita' specificata
              RET_POS: non effettua il movimento, ritorna esclusivamente la posizione attuale (in mm)
              SET_POS: non effettua il movimento, setta la posizione attuale pari al valore mm specificato
   limitOn:   attiva o disattiva check limiti movimentazione
              ZLIMITON : attiva limiti in movimentazione     (default)
              ZLIMITOFF: disabilita limiti in movimentazione
Valori di ritorno:
   Ritorna la posizione attuale in mm.
----------------------------------------------------------------------------------*/
float PuntaZPosMm(int punta,float mm,int mode,int limitOn)
{
	float k = ( punta == 1 ) ? QHeader.Step_Trasl1 : QHeader.Step_Trasl2;
	return(PuntaZPosStep(punta,int(mm*k),mode,limitOn)/k);
}


/*---------------------------------------------------------------------------------
Attiva o disattiva la contropressione sulla punta specificata.
Se sulla punta indicata ? attivato il vuoto e viene richiesto di attivare la contropressione, 
viene prima disattivato il vuoto.
Parametri di ingresso:
   vcomm: flag che identifica l'attivazione o la disattivazione della contropressione
                 1: attiva
                 0: disattiva
                 altro: disattiva
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
//##SMOD300802
void Set_Contro(int xpunta, int Comm)
{
	int tmp;
	
	if( Get_OnFile() )
		return;
	
	if(!PressStatus(1))
		return;

	if((xpunta>2) || (xpunta<1))
		xpunta=1;
	
	if((Comm<0) || (Comm>1))
		Comm=0;
	
	if(((vac_status[xpunta-1]) && (Comm==1)) && (!contro_status[xpunta-1]))
	{
		Set_Vacuo(xpunta,0);
	}
	
	if(contro_status[xpunta-1]!=Comm)
	{
		contro_status[xpunta-1]=Comm;
		
		if(xpunta==1)
		{
			tmp=CPRS1;
		}
		else
		{
			tmp=CPRS2;
		}
		
		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		if(Comm)
		{
			FoxHead->SetOutput(FOX,tmp);
		}
		else
		{
			FoxHead->ClearOutput(FOX,tmp);
		}

		centeringMutex.unlock();
	}
}

/*---------------------------------------------------------------------------------
Rtitorna lo stato di attivazione del generatore del vuoto per la punta indicata
Parametri di ingresso:
   punta: numero della punta (1 o 2)
Valori di ritorno:
   true se il generatore del vuoto e' attivo, false altrimenti
----------------------------------------------------------------------------------*/
bool isVacuoOn(int punta)
{
	if(punta == 1)
	{
		return (vac_status[0] != 0);
	}
	else
	{
		return (vac_status[1] != 0);
	}
}

/*---------------------------------------------------------------------------------
Attiva o disattiva il vuoto sulla punta specificata.
Se sulla punta indicata ? attivata la contropressione e viene richiesto di attivare 
il vuoto, viene prima disattivata la contropressione.
Parametri di ingresso:
   vcomm: flag che identifica l'attivazione o la disattivazione del vuoto
                 1: attiva
                 0: disattiva
                 altro:disattiva
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
//##SMOD300802
void Set_Vacuo(int vpunta, int vcomm)
{
	int tmp;
	
	if( Get_OnFile() )
	{
		return;
	}
	
	if(!PressStatus(1))
	{
		return;
	}
	
	if((vpunta>2) || (vpunta<1))
	{
		vpunta=1;
	}
	
	if((vcomm<0) || (vcomm>1))
	{
		vcomm=0;
	}
	
	if(((contro_status[vpunta-1]) && (vcomm == 1)) && (!vac_status[vpunta-1]))
	{
		Set_Contro(vpunta,0);
	}
	
	if(vac_status[vpunta-1] != vcomm)
	{
		vac_status[vpunta-1] = vcomm;
		
		if(vpunta==1)
		{
			tmp=VACUO1;
		}
		else
		{
			tmp=VACUO2;
		}
		
		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		if(vcomm)
		{
			FoxHead->SetOutput(FOX,tmp);
		}
		else
		{
			FoxHead->ClearOutput(FOX,tmp);
		}

		centeringMutex.unlock();
	}
}

/*---------------------------------------------------------------------------------
Attivazione contropressione prima del prelievo di un componente
Parametri di ingresso:
   nessuno
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void Prepick_Contro(int p)
{
	if(QHeader.TContro_Uge!=0)
	{
		Set_Contro(p,1);
		delay(QHeader.TContro_Uge);
		Set_Contro(p,0);
	}
}

#ifdef __DISP2

int DispenserPressureSwitchStat=0;

/*---------------------------------------------------------------------------------
Switch pressione dispenser
Parametri di ingresso:
   ndisp: 1 switch pressione su dispenser 1
          2 switch pressione su dispenser 2
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void SwitchDispenserPressure(int ndisp)
{
	if(QParam.Dispenser)
	{
		if((ndisp!=1) && (ndisp!=2))
		{
			ndisp=1;
		}

		if(DispenserPressureSwitchStat==ndisp)
		{
			return;
		}

		DispenserPressureSwitchStat=ndisp;

    	//scarica il tubo dalla pressione relativa al dispenser precedente
		Set_Cmd(DISP2_VACUOGEN_CMD,1);
		delay(DISP_VALVE_SWITCHTIME+DISP_EMPTY_PIPE);
		Set_Cmd(DISP2_VACUOGEN_CMD,0);

   	 	//switch pressione per dispenser ndisp
		Set_Cmd(DISP2_PRESS_SWITCH_CMD,(ndisp==1) ? 0 : 1);
		delay(DISP_VALVE_SWITCHTIME+DISP_FILL_PIPE);
    
	}
}

/*---------------------------------------------------------------------------------
Attiva generatore vuoto per antisgocciolio dispenser
Parametri di ingresso:
   nessuno
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void SetDispVacuoGeneratorOn(int ndisp)
{
	if(QParam.Dispenser)
	{
		SwitchDispenserPressure(ndisp);
		Set_Cmd(DISP2_VACUOGEN_CMD,1);
	}
}

/*---------------------------------------------------------------------------------
Disattiva generatore vuoto per antisgocciolio dispenser
Parametri di ingresso:
   nessuno
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void SetDispVacuoGeneratorOff(void)
{
	if(QParam.Dispenser)
	{
		Set_Cmd(DISP2_VACUOGEN_CMD,0);
		//Set_Cmd(DISP2_PRESS_SWITCH_CMD,0);
	}
}
#endif


/*---------------------------------------------------------------------------------
Interfaccia a basso livello di avanzamento caricatore.
Parametri di ingresso:
   Car: pacco caricatore (1..15)
   Weel: ruota caricarore (0..7)
   Type: modalita' di avanzamento del caricatore
             CARAVA_NORMAL : avanzamento corto
             CARAVA_MEDIUM : avanzamanto medio
             CARAVA_LONG   : avanzamento lungo
             CARAVA_DOUBLE : avanzamento doppio
----------------------------------------------------------------------------------*/
//##SMOD300802
void Set_CaricMov(unsigned int Car, unsigned int Weel, int Type) 
{
	int mode=Type;
	
	if( Get_OnFile() )
		return;
	
	if((Weel>7 || Car>15) || Type<0)
	{
		return;
	}
		
	// per comp 0402 avanzamento corto
	/*if(Type==CARAVA_DOUBLE)
	{
		mode=CARAVA_NORMAL;
	}*/
		
	char buf[6];
	
	if((mode==CARAVA_LONG) || (mode==CARAVA_LONG1))
	{
		snprintf( buf, sizeof(buf),"C%1X%1X%c",Car,Weel,mode+0x60);
	}
	else
	{
		snprintf( buf, sizeof(buf),"C%1X%1X%1X",Car,Weel,mode);
	}
	
	cpu_cmd(1,buf);
}


/*---------------------------------------------------------------------------------
Attende che i caricatori siano pronti. Non e' necessario specificare l'identificativo
del caricatore poiche' e' prevista un'unica movimentazione alla volta.
Parametri di ingresso:
   nessuno
Valori di ritorno:
   Ritorna 1 se i caricatori sono pronti, 0 altrimenti.
----------------------------------------------------------------------------------*/
//THFEEDER2
int CaricWait( int carType, int carAdd )
{
	time_t tmp_one, tmp_two;
	
	if( Get_OnFile() || QParam.DemoMode )
		return(1);
	
	if( carType == CARTYPE_THFEEDER )
		ThFeeder->ChangeAddress( carAdd );
	
	tmp_one = tmp_two = time(NULL); // reset timer time-out
	
	while(1)
	{
		int ret_val = MOTOR_OK;
	
		if( carType == CARTYPE_THFEEDER )
		{
			// controlla thFeeder
			ret_val = ThFeeder->FeederReady();
			if( ret_val == 1 )
				break;
		}
		else
		{
			if( !Check_Caric() )
				break;
		}
		
		tmp_two = time(NULL);

		if( difftime( tmp_two, tmp_one ) > CREADY || ret_val == MOTOR_ERROR )
		{
			W_Mess(NOCREADY,MSGBOX_YCENT,0,GENERIC_ALARM);
			return 0;
		}
	}

	return 1;
}

/*---------------------------------------------------------------------------------
Avanza caricatore indicato come parametro.
Parametri di ingresso:
   Cod_car: codice del caricatore sul quale effettuare l'avanzamento (11..158)
   Type: modalita' di avanzamento del caricatore
             0: avanzamento corto
             1: avanzamanto medio
             2: avanzamento lungo
----------------------------------------------------------------------------------*/
//THFEEDER
void CaricMov( int Cod_car, int Type, int carType, int carAdd, int carAtt )
{
	int unit=Cod_car/10;
	int feeder=(Cod_car-(unit*10))-1;
	
	if( Get_OnFile() )
		return;
	
	if((Cod_car>158) || (Cod_car<11))
		return;    // esclusione vassoi
	
	if( carType == CARTYPE_THFEEDER )
	{
		ThFeeder->ChangeAddress( carAdd );
		
		int carSteps = 0;
		switch( carAtt )
		{
		case CARAVA_MEDIUM:
		case CARAVA_MEDIUM1:
			carSteps = FEEDER_STEPS_MEDIUM;
			break;
			
		case CARAVA_LONG:
		case CARAVA_LONG1:
			carSteps = FEEDER_STEPS_LONG;
			break;
			
		case CARAVA_DOUBLE:
		case CARAVA_DOUBLE1:
			carSteps = FEEDER_STEPS_DOUBLE;
			break;
			
		case CARAVA_NORMAL:
		case CARAVA_NORMAL1:
		default:
			carSteps = FEEDER_STEPS_NORMAL;
			break;
		}
		
		// avanza thFeeder
		ThFeeder->FeederAdvance( carSteps );
	}
	else
	{
		unit=unit-1;
		Set_CaricMov(unit,feeder,Type);
	}
}

/*---------------------------------------------------------------------------------
Controlla se i caricatori siano pronti.
Parametri di ingresso:
   nessuno
Valori di ritorno:
   Ritorna 1 se ii caricatore è pronto, 0 se non pronto, -1 se errore.
----------------------------------------------------------------------------------*/
//THFEEDER2
int CaricCheck( int carType, int carAdd )
{
	int ret_val;
	
	if( carType == CARTYPE_THFEEDER )
	{
		ThFeeder->ChangeAddress( carAdd );
		ret_val = ThFeeder->FeederReady();
		if( ret_val == MOTOR_ERROR )
			ret_val = -1;
	}
	else
		ret_val = Check_Caric() ? 0 : 1;
	
	return ret_val;
}


//---------------------------------------------------------------------------------
// Ritorna coordinata Y ultimo movimento
//---------------------------------------------------------------------------------
float GetLastYPos()
{
	return CurXYStat.yPosLogical;
}

//---------------------------------------------------------------------------------
// Ritorna coordinata X ultimo movimento
//---------------------------------------------------------------------------------
float GetLastXPos()
{
	return CurXYStat.xPosLogical;
}

//---------------------------------------------------------------------------------
// Controlla fattibilita' movimento assi
// Ritorna 1 se il movimento e' possibile, 0 altrimenti.
//---------------------------------------------------------------------------------
bool Check_XYMove( float X_mov, float Y_mov, int mode )
{
	if( !(mode & AUTOAPP_MAPOFF) )
	{
		// correzione ortogonalita'
		if( use_orthoXY_correction && QParam.OrthoXY_Error != 0.f )
		{
			float newPosX = X_mov / cos( QParam.OrthoXY_Error );
			float newPosY = Y_mov - X_mov * sin( QParam.OrthoXY_Error );
		
			X_mov = newPosX;
			Y_mov = newPosY;
		}
		
		// correzione scala encoder
		if( use_encScale_correction )
		{
			X_mov *= QParam.EncScale_x;
			Y_mov *= QParam.EncScale_y;
		}
		
		// correzione mappatura piano
		if( use_mapping_correction )
		{
			Errormap_Correct( X_mov, Y_mov );
		}
	}

	if( X_mov > QParam.LX_maxcl || X_mov < QParam.LX_mincl ||
		Y_mov > QParam.LY_maxcl || Y_mov < QParam.LY_mincl )
	{
		return false;
	}

	return true;
}

/*---------------------------------------------------------------------------------
Movimentazione in base ai parametri passati come ingresso.
Parametri di ingresso:
   X-mov, Y-mov: coordinate alle quali effettuare la movimentazione espresse in mm
   F_mov: velocita' di spostamento espressa in Hz. Il range del basso livello e' di 16-300000.
          Tale campo di valori viene limitato nell'alto livello a seconda del tipo di
          azionamento utilizzato (attualmente il limite massimo e di 50000)
   mode: modalita' di movimentazione
              AUTOAPP_NOFINEC: disabilita le protezioni dei finecorsa meccanici
              AUTOAPP_PUNTA1ON: disabilta la protezione sul movimento assi con punta 1 abbassata
              AUTOAPP_PUNTA2ON: disabilta la protezione sul movimento assi con punta 2 abbassata
              MAPASSI_OFF     : disabilita mappatura
Valori di ritorno:
   Ritorna 0 se il movimento e' fallito, 1 altrimenti.
----------------------------------------------------------------------------------*/
int NozzleXYMove( float X_mov, float Y_mov, int mode )
{
	if( CurXYStat.xPosLogical == X_mov && CurXYStat.yPosLogical == Y_mov )
	{
		return 1;
	}

	CurXYStat.xPosLogical = X_mov;
	CurXYStat.yPosLogical = Y_mov;

	if( !(mode & AUTOAPP_MAPOFF) )
	{
		// correzione ortogonalita'
		if( use_orthoXY_correction && QParam.OrthoXY_Error != 0.f )
		{
			float newPosX = X_mov / cos( QParam.OrthoXY_Error );
			float newPosY = Y_mov - X_mov * sin( QParam.OrthoXY_Error );
		
			X_mov = newPosX;
			Y_mov = newPosY;
		}

		// correzione scala encoder
		if( use_encScale_correction )
		{
			X_mov *= QParam.EncScale_x;
			Y_mov *= QParam.EncScale_y;
		}

		// correzione mappatura piano
		if( use_mapping_correction )
		{
			Errormap_Correct( X_mov, Y_mov );
		}
	}

	if( Get_OnFile() )
	{
		return 1;
	}

	#ifndef HWTEST_RELEASE
	if( (Check_PuntaTraslStatus(1) || Check_PuntaTraslStatus(2) || !Dosatore->CanXYMove()) && (!(mode & AUTOAPP_NOZSECURITY)) )
	{
		// check movimento con punta down
		W_Mess(NOMOVE_PDOWN,MSGBOX_YCENT,0,GENERIC_ALARM);
	
		ResetNozzles(); // up punte, off controp./vuoto
		Set_OnFile(1);
		return 0;
	}

	mode = mode & 1;

	if( !mode )
	{
		if( X_mov > QParam.LX_maxcl || X_mov < QParam.LX_mincl ||
			Y_mov > QParam.LY_maxcl || Y_mov < QParam.LY_mincl )
		{
			// movimentio fuori dai limiti
			W_Mess(OUTLIMIT,MSGBOX_YCENT,0,GENERIC_ALARM);

			ResetNozzles();
			return 0;
		}
	}
	#endif

	if((CurXYStat.xPosPhysical != X_mov) || (CurXYStat.yPosPhysical != Y_mov))
	{
		CheckProtection_WaitClose();

		if( IsConfirmRequiredBeforeNextXYMovement() )
		{
			Request_SHIFT2_ToContinue();
		}
	}

	ResetTimerConfirmRequiredBeforeNextXYMovement();
	SetConfirmRequiredBeforeNextXYMovement(false);

	CurXYStat.xPosPhysical = X_mov;
	CurXYStat.yPosPhysical = Y_mov;

	#ifdef __USE_MOTORHEAD
	float xNorm = (X_mov - QParam.LX_mincl) / (QParam.LX_maxcl - QParam.LX_mincl);
	xNorm = MID( 0.f, xNorm, 1.f );
	//TEMP //TODO: nel ciclo di assemblaggio il movimento non deve esser controllato ci pensa la wait
	if( !Motorhead->Move( X_mov, Y_mov, xNorm ) )
	{
		return 0;
	}
	#endif

	return 1;
}

/*---------------------------------------------------------------------------------
Posizionamento della punta passata come parametro.
Parametri di ingresso:
   pn:  punta da posizionare
              1: punta 1
              2: punta 2
              altro:punta=1
   x_posiz, y_posiz: coordinate alle quali posizionare la punta. Sono espresse in mm e
                     riferite allo zero  macchina.
   v_punta: velocita' della punta espressa in Hz. Il range del basso livello e' di 16-300000.
            Tale campo di valori viene limitato nell'alto livello a seconda del tipo di
            azionamento utilizzato (attualmente il limite massimo e di 50000)
   mode: modalita' di posizionamento
              AUTOAPP_NOFINEC: disabilita la protezione dei finecorsa
              AUTOAPP_PUNTA1ON: disabilta protezione sul movimento assi con punta 1 abbassata
              AUTOAPP_PUNTA2ON: disabilta protezione sul movimento assi con punta 2 abbassata
----------------------------------------------------------------------------------*/
int NozzleXYMove_N( float x_posiz, float y_posiz, int pn, int mode )
{
	float cx_oo, cy_oo;

	if( pn != 2 && pn != 1 )
		pn = 1;

	if( pn == 1 )
	{
		cx_oo = x_posiz + QParam.CamPunta1Offset_X;
		cy_oo = y_posiz + QParam.CamPunta1Offset_Y;
	}
	else
	{
		cx_oo = x_posiz + QParam.CamPunta2Offset_X;
		cy_oo = y_posiz + QParam.CamPunta2Offset_Y;
	}

	return NozzleXYMove( cx_oo, cy_oo, mode ); // vai in posizione
}

//---------------------------------------------------------------------------------
// Attende il completamento di un comando di posizionamento
//---------------------------------------------------------------------------------
bool Wait_PuntaXY( int t_tempo )
{
	#ifdef __USE_MOTORHEAD
	while( 1 )
	{
		if( Motorhead->Wait() == false )
		{
			if( Motorhead->GetError() & ERR_SECURITYSTOP )
			{
				//TODO: controllare valore ritorno delle funzioni motorhead

				// mostra messaggio e attende chiusura protezione
				CheckProtection_WaitClose();

				Motorhead->RestartAfterImmediateStop();

				#ifdef __USE_MOTORHEAD
				float xNorm = (CurXYStat.xPosPhysical - QParam.LX_mincl) / (QParam.LX_maxcl - QParam.LX_mincl);
				xNorm = MID( 0.f, xNorm, 1.f );
				Motorhead->Move( CurXYStat.xPosPhysical, CurXYStat.yPosPhysical, xNorm, true );
				#endif

				continue;
			}

			int errId = Motorhead->GetErrorID();
			int err = Motorhead->GetError();
			int status, encStatus;
			Motorhead->GetStatus( errId, status );
			Motorhead->GetEncoderStatus( errId, encStatus );

			#ifdef __LOG_ERROR
			if( Get_WriteErrorLog() )
			{
				QuadraLogger.Log( "Motorhead Move [axis: %c  ID: %d  status: %d  enc: %d]", (errId == HEADY_ID) ? 'Y' : 'X', err, status, encStatus );
			}
			#endif

			// Svuota il buffer della tastiera
			flushKeyboardBuffer();

			char buf[256];
			char buf_out[1024];
			buf_out[0] = '\0';

			snprintf( buf, sizeof(buf), MsgGetString(Msg_06026), (errId == HEADY_ID) ? 'Y' : 'X' );
			strcat( buf_out, buf );
			strcat( buf_out, MsgGetString(Msg_06027) );
			if( err & ERR_DRIVER )
				strcat( buf_out, "Comunication " );
			if( err & ERR_OVERCURRENT )
				strcat( buf_out, "Overcurrent " );
			if( err & ERR_MOVETIMEOUT )
				strcat( buf_out, "Timeout " );
			if( err & ERR_OVERSPEED )
				strcat( buf_out, "Overspeed " );
			if( err & ERR_NOENC )
			{
				strcat( buf_out, "Encoder (" );
				if( encStatus & ENC_MF_ERROR )
					strcat( buf_out, "MagField" );
				if( encStatus & ENC_OVERRUN_ERROR )
					strcat( buf_out, "Overrun" );
				if( encStatus & ENC_FRAME_ERROR )
					strcat( buf_out, "Frame" );
				if( encStatus & ENC_NORECEPTIONS )
					strcat( buf_out, "NoReceptions" );
				strcat( buf_out, ")" );
			}
			if( err & ERR_LIMITSWITCH )
				strcat( buf_out, "Switch " );
			if( err & ERR_SECURITYSTOP )
				strcat( buf_out, "Security " );
			if( err & ERR_STEADYPOSITION )
				strcat( buf_out, "SteadyPos " );
			strcat( buf_out, "\n" );

			W_Mess( buf_out );

			Motorhead->DisableMotors();
			Set_OnFile( 1 );
			return false;
		}
		else
		{
			break;
		}
	}
	#endif

	return true;
}

//---------------------------------------------------------------------------------
// Setta la posizione corrente come origine
//---------------------------------------------------------------------------------
bool SetHome_PuntaXY()
{
	#ifdef __USE_MOTORHEAD
	bool ret = Motorhead->SetHome();
	#endif

	ResetXYVariables();

	if( !ret )
	{
		flushKeyboardBuffer();
		W_Mess( "Unable to set XY home !" );
	}
	return ret;
}


/*---------------------------------------------------------------------------------
Check della pressione dell'aria.
Parametri di ingresso:
   tipo: tipo di test effettuato
           0: test istantaneo sulla pressione
           1: test all'interno di un ciclo durante il quale viene fatta la
              richiesta di accensione dell'aria
Valori di ritorno:
   Ritorna 0 se il check e' fallito, 1 altrimenti.
----------------------------------------------------------------------------------*/
int PressStatus( int tipo )
{
	switch( tipo )
	{
		case 0:
			if( !Check_Press() )
			{
				W_Mess(NOAIR,MSGBOX_YCENT,0,GENERIC_ALARM);
				Set_OnFile(1);             // flag modal. file on
				return 0;
			}
			break;

		case 1:
			while( !Check_Press() )
			{
				if(!W_Deci(1,NOPRESS,MSGBOX_YCENT,0,0,GENERIC_ALARM) )
				{
					W_Mess(MODFILE);
					Set_OnFile(1);             // flag modal. file on
					return 0;
				}
			}
			break;

		default:
			return 0;
	}

	return 1;
}


#define MAXSTEPX 1.00
#define MAXSTEPY 1.00
#define MAXPRESS 60
#define STARTINC 25

/*---------------------------------------------------------------------------------
Incrementa le quantita` di spostamento a seconda del tempo del tasto premuto
aggiornando le variabili globali K_PassoX e  K_PassoY
----------------------------------------------------------------------------------*/
//GF_TEMP - andrebbe rifatta con moltiplicatore del movimento per tutte le lunghezze
void Read_Keyboard(void)
{
	// L2109
	static float stepX = (MAXSTEPX-QHeader.PassoX)/(MAXPRESS-STARTINC);
	static float stepY = (MAXSTEPY-QHeader.PassoY)/(MAXPRESS-STARTINC);
	
	if( press_count > 0 )
	{
		if( press_count < MAXPRESS )
		{
			if( press_count > STARTINC )
			{
				K_PassoX+=stepX;
				K_PassoY+=stepY;
				
				K_PassoX=((float)ftoi(K_PassoX/QHeader.PassoX))*QHeader.PassoX;
				K_PassoY=((float)ftoi(K_PassoY/QHeader.PassoY))*QHeader.PassoY;
			}
			press_count++;
		}
	}
	else
	{
		K_PassoX=QHeader.PassoX;
		K_PassoY=QHeader.PassoY;
		press_count++;
	}
}


bool AutoApprendCheckMoveLimit( float& xpos, float& ypos, float deltax, float deltay, unsigned int mode )
{
	if( !(mode & AUTOAPP_NOLIMIT) )
	{
		if( !Check_XYMove( xpos + deltax, ypos + deltay, AUTOAPP_MAPOFF ) )
		{
			return false;
		}
	}

	xpos += deltax;
	ypos += deltay;

	return true;
}

int(*fnTeachingKeyCallback)(int,float&,float&)=NULL;

/*---------------------------------------------------------------------------------
Imposta callback per pressione tasto in apprendimento
La routine di callback riceve in ingresso il tasto premuto e le coordinate
di apprendimento attuali.
---------------------------------------------------------------------------------*/
void SetTeachingKeyCallback(int(*f)(int,float&,float&))
{
	fnTeachingKeyCallback = f;
}

int AutoAppExtCamDirCallback(int key,float& x,float& y)
{
	switch(key)
	{
		case K_RIGHT:
			key=K_LEFT;
			break;
	
		case K_ALT_RIGHT:
			key=K_ALT_LEFT;
			break;
	
		case K_CTRL_RIGHT:
			key=K_CTRL_LEFT;
			break;
		
		case K_SHIFT_RIGHT:
			key=K_SHIFT_LEFT;
			break;

		case K_LEFT:
			key=K_RIGHT;
			break;
	
		case K_ALT_LEFT:
			key=K_ALT_RIGHT;
			break;
	
		case K_CTRL_LEFT:
			key=K_CTRL_RIGHT;
			break;

		case K_SHIFT_LEFT:
			key=K_SHIFT_RIGHT;
			break;

		case K_UP:
			key=K_DOWN;
			break;

		case K_ALT_UP:
			key=K_ALT_DOWN;
			break;

		case K_CTRL_UP:
			key=K_CTRL_DOWN;
			break;

		case K_SHIFT_UP:
			key=K_SHIFT_DOWN;
			break;
		
		case K_DOWN:
			key=K_UP;
			break;
	
		case K_ALT_DOWN:
			key=K_ALT_UP;
			break;

		case K_CTRL_DOWN:
			key=K_CTRL_UP;
			break;

		case K_SHIFT_DOWN:
			key=K_SHIFT_UP;
			break;
	}

	return key;
}

int AutoAppExtCamCtrlCallback(int key,float& x,float& y)
{
	int light = GetExtCam_Light();
	int shutter = GetExtCam_Shutter();
	
	int prevlight=light;
	int prevshutter=shutter;
	
	switch(key)
	{
		case K_SHIFT_F5:
			switch(light)
			{
				case EXTCAM_FULL_LIGHT:
					light=EXTCAM_SIDE_LIGHT;
					break;
				case EXTCAM_SIDE_LIGHT:
					light=EXTCAM_CENTRAL_LIGHT;
					break;
				case EXTCAM_CENTRAL_LIGHT:
					light=EXTCAM_FULL_LIGHT;
					break;
			}
			break;
	
		case K_SHIFT_F6:
			switch(light)
			{
				case EXTCAM_FULL_LIGHT:
					light=EXTCAM_CENTRAL_LIGHT;
					break;
				case EXTCAM_SIDE_LIGHT:
					light=EXTCAM_FULL_LIGHT;
					break;
				case EXTCAM_CENTRAL_LIGHT:
					light=EXTCAM_SIDE_LIGHT;
					break;
			}
			break;

		case K_SHIFT_F7:
			shutter--;
			if(shutter < EXTCAM_MIN_SHUTTER)
			{
				shutter = EXTCAM_MAX_SHUTTER;
			}
			break;

		case K_SHIFT_F8:
			shutter++;
			if(shutter > EXTCAM_MAX_SHUTTER)
			{
				shutter = EXTCAM_MIN_SHUTTER;
			}
			break;
	}
	
	if(light!=prevlight)
	{
		SetExtCam_Light(light);
	}
	
	if(shutter!=prevshutter)
	{
		SetExtCam_Shutter(shutter);
	}
	
	return(key);
}

int AutoAppExtCamPosCallback(int key,float& x,float& y)
{
	key = AutoAppExtCamCtrlCallback(key,x,y);
	key = AutoAppExtCamDirCallback(key,x,y);
	
	float prev_rot=autoapp_extcam_rotation;
	
	switch(key)
	{
		case K_F3:
			setCross( CROSS_NEXT_MODE, CROSSBOX_EXTCAM );
			key=0;
			break;

		case K_ENTER:
			if(autoapp_extcam_rotation!=0)
			{
				//non si puo' apprendere se non si e' a zero gradi
				//si ritorna quindi a zero gradi e si rifiuta la richiesta
				//di conferma
				bipbip();
				autoapp_extcam_rotation=0;
				key=0;
			}
			break;

		case K_ESC:
			if(autoapp_extcam_rotation!=0)
			{
				autoapp_extcam_rotation=0;
			}
			break;
	
		case 'r':
		case 'R':
			switch(autoapp_extcam_rotation)
			{
				case 0:
				autoapp_extcam_rotation=90;
				break;
				case 90:
				autoapp_extcam_rotation=180;
				break;
				case 180:
				autoapp_extcam_rotation=270;
				break;
				case 270:
				autoapp_extcam_rotation=0;
				break;
			}
			break;

		case '1':
			autoapp_extcam_rotation=0;
			break;
		case '2':
			autoapp_extcam_rotation=90;
			break;
		case '3':
			autoapp_extcam_rotation=180;
			break;
		case '4':
			autoapp_extcam_rotation=270;
			break;
	}
	
	if(prev_rot!=autoapp_extcam_rotation)
	{
		Wait_EncStop(1);
		PuntaRotDeg(autoapp_extcam_rotation,1);
	
		char buf[80];
		snprintf( buf, sizeof(buf),"%s%3d", MsgGetString(Msg_01358), autoapp_extcam_rotation );

		Set_Tv_Title( buf );
	}
	
	return(key);
}


int AutoAppPackCallback(int key,float& x,float& y)
{
	key = AutoAppExtCamCtrlCallback(key,x,y);
	key = AutoAppExtCamDirCallback(key,x,y);
	
	return key;
}

int AutoAppPack2Callback(int key,float& x,float& y)
{
	switch( key )
	{
		case K_UP:
		case K_ALT_UP:
		case K_CTRL_UP:
		case K_SHIFT_UP:
		case K_DOWN:
		case K_ALT_DOWN:
		case K_CTRL_DOWN:
		case K_SHIFT_DOWN:
			return 0;
	}

	key = AutoAppExtCamCtrlCallback(key,x,y);
	key = AutoAppExtCamDirCallback(key,x,y);

	return key;
}

int dosa_presspulse_done=0;

#define VIDEO_CALLBACK_Y      29
#define VECTORIAL_TIPS_Y      24

//---------------------------------------------------------------------------------
// displayConveyorInfoTips
// Mostra help per movimento convogliatore in apprendimento con telecamera
//---------------------------------------------------------------------------------
void displayConveyorInfoTips( void* parentWin )
{
	GUI_Freeze_Locker lock;

	CWindow* parent = (CWindow*)parentWin;

	parent->DrawText( 2, VIDEO_CALLBACK_Y + 2, MsgGetString(Msg_00940) );
	parent->DrawText( 2, VIDEO_CALLBACK_Y + 3, "          [arrows] -> " );
	parent->DrawText( 24, VIDEO_CALLBACK_Y + 3, MsgGetString(Msg_00022) );
	parent->DrawText( 2, VIDEO_CALLBACK_Y + 4, "[ Shift ]+[arrows] -> " );
	parent->DrawText( 24, VIDEO_CALLBACK_Y + 4, MsgGetString(Msg_00078) );
	parent->DrawText( 2, VIDEO_CALLBACK_Y + 5, "[  Ctrl ]+[arrows] -> " );
	parent->DrawText( 24, VIDEO_CALLBACK_Y + 5, MsgGetString(Msg_00023) );
	parent->DrawText( 2, VIDEO_CALLBACK_Y + 6, "[  Alt  ]+[arrows] -> " );
	parent->DrawText( 24, VIDEO_CALLBACK_Y + 6, MsgGetString(Msg_00024) );

	parent->DrawText( 41, VIDEO_CALLBACK_Y + 2, MsgGetString(Msg_00945) );
	parent->DrawText( 41, VIDEO_CALLBACK_Y + 3, "          [PgUP/PgDN] -> " );
	parent->DrawText( 66, VIDEO_CALLBACK_Y + 3, MsgGetString(Msg_00023) );
	parent->DrawText( 41, VIDEO_CALLBACK_Y + 4, "[  Ctrl ]+[PgUP/PgDN] -> " );
	parent->DrawText( 66, VIDEO_CALLBACK_Y + 4, MsgGetString(Msg_00024) );
	parent->DrawText( 41, VIDEO_CALLBACK_Y + 5, "[  Alt  ]+[PgUP/PgDN] -> " );
	parent->DrawText( 66, VIDEO_CALLBACK_Y + 5, MsgGetString(Msg_00939) );
}

//---------------------------------------------------------------------------------
// displayInfoTips
// Mostra help generico in apprendimento con telecamera
//---------------------------------------------------------------------------------
void displayInfoTips( void* parentWin )
{
	GUI_Freeze_Locker lock;

	CWindow* parent = (CWindow*)parentWin;

	parent->DrawText( 21, VIDEO_CALLBACK_Y + 2, "          [arrows] -> " );
	parent->DrawText( 43, VIDEO_CALLBACK_Y + 2, MsgGetString(Msg_00022) );

	parent->DrawText( 21, VIDEO_CALLBACK_Y + 3, "[ Shift ]+[arrows] -> " );
	parent->DrawText( 43, VIDEO_CALLBACK_Y + 3, MsgGetString(Msg_00078) );

	parent->DrawText( 21, VIDEO_CALLBACK_Y + 4, "[  Ctrl ]+[arrows] -> " );
	parent->DrawText( 43, VIDEO_CALLBACK_Y + 4, MsgGetString(Msg_00023) );

	parent->DrawText( 21, VIDEO_CALLBACK_Y + 5, "[  Alt  ]+[arrows] -> " );
	parent->DrawText( 43, VIDEO_CALLBACK_Y + 5, MsgGetString(Msg_00024) );
}

//---------------------------------------------------------------------------------
// displayVectorialTips
// Mostra help con parametri riconoscimento vettoriale in apprendimento con telecamera
//---------------------------------------------------------------------------------
void displayVectorialTips( void* parentWin )
{
	GUI_Freeze_Locker lock;

	CWindow* parent = (CWindow*)parentWin;

	parent->DrawText( 21, VIDEO_CALLBACK_Y + 2, "          [arrows] -> " );
	parent->DrawText( 43, VIDEO_CALLBACK_Y + 2, MsgGetString(Msg_00022) );

	parent->DrawText( 21, VIDEO_CALLBACK_Y + 3, "[ Shift ]+[arrows] -> " );
	parent->DrawText( 43, VIDEO_CALLBACK_Y + 3, MsgGetString(Msg_00078) );

	parent->DrawText( 21, VIDEO_CALLBACK_Y + 4, "[  Ctrl ]+[arrows] -> " );
	parent->DrawText( 43, VIDEO_CALLBACK_Y + 4, MsgGetString(Msg_00023) );

	parent->DrawText( 21, VIDEO_CALLBACK_Y + 5, "[  Alt  ]+[arrows] -> " );
	parent->DrawText( 43, VIDEO_CALLBACK_Y + 5, MsgGetString(Msg_00024) );

	parent->DrawText( 85, VECTORIAL_TIPS_Y + 1, "            [ F9 ] -> " );
	parent->DrawText( 107, VECTORIAL_TIPS_Y + 1, MsgGetString(Msg_07005) );
	parent->DrawText( 85, VECTORIAL_TIPS_Y + 2, "     [ PgUp/PgDn ] -> " );
	parent->DrawText( 107, VECTORIAL_TIPS_Y + 2, MsgGetString(Msg_07010) );
	parent->DrawText( 85, VECTORIAL_TIPS_Y + 3, "[ Ctrl/Alt ]+[ 1 ] -> " );
	parent->DrawText( 107, VECTORIAL_TIPS_Y + 3, MsgGetString(Msg_07006) );
	parent->DrawText( 85, VECTORIAL_TIPS_Y + 4, "[ Ctrl/Alt ]+[ 2 ] -> " );
	parent->DrawText( 107, VECTORIAL_TIPS_Y + 4, MsgGetString(Msg_07007) );
	parent->DrawText( 85, VECTORIAL_TIPS_Y + 5, "[ Ctrl/Alt ]+[ 3 ] -> " );
	parent->DrawText( 107, VECTORIAL_TIPS_Y + 5, MsgGetString(Msg_07008) );
	parent->DrawText( 85, VECTORIAL_TIPS_Y + 6, "[ Ctrl/Alt ]+[ 4 ] -> " );
	parent->DrawText( 107, VECTORIAL_TIPS_Y + 6, MsgGetString(Msg_07009) );
	parent->DrawText( 85, VECTORIAL_TIPS_Y + 7, "[ Ctrl/Alt ]+[ X ] -> " );
	parent->DrawText( 107, VECTORIAL_TIPS_Y + 7, MsgGetString(Msg_07031) );
	parent->DrawText( 85, VECTORIAL_TIPS_Y + 8, "[ Ctrl/Alt ]+[ Y ] -> " );
	parent->DrawText( 107, VECTORIAL_TIPS_Y + 8, MsgGetString(Msg_07032) );
	parent->DrawText( 85, VECTORIAL_TIPS_Y + 9, "    [ Ctrl ]+[ T ] -> " );
	parent->DrawText( 107, VECTORIAL_TIPS_Y + 9, MsgGetString(Msg_07011) );
}

/*---------------------------------------------------------------------------------
Funzione di gestione dei tasti durante le modalita' di autoapprendimento tramite 
l'utilizzo delle variabili globali K_PassoX e  K_PassoY aggiornate dalla routine set_press_step().
Parametri di ingresso:
   x_coord, y_coord: coordinate da cui partire per l'autoapprendimento
   mode: modalita' con cui eseguire l'autoapprendimento
              AUTOAPP_NOFINEC: disabilita la protezione dei finecorsa durante l'autoapprendimento
              AUTOAPP_COMP: autoapprendimento di un componente
              AUTOAPP_PUNTA1ON: abilita la possibilita di muovere la punta 1 con CTRL+PGUP/PGDN
              AUTOAPP_PUNTA2ON: abilita la possibilita di muovere la punta 2 con CTRL+PGUP/PGDN
              AUTOAPP_ONLY1KEY: esce dopo la pressione di un tasto tra Cursore/ENTER/ESC	
              AUTOAPP_NOUPDATE: non aggiorna X_Auto e Y_Auto prima di uscire	
              AUTOAPP_NOCAM: non gestisce i tasti dedicati alla telecamera
              AUTOAPP_UGELLO  : abilita solo i movimenti di un singolo passo con la punta giu
Valori di ritorno:
   Ritorna il codice dell'ultimo tasto premuto.
   Ritorna x_coord e y_coord le coordinate aggiornate.
----------------------------------------------------------------------------------*/
//DANY211102 - DANY270103
int AutoApprendGestKey( float& x_coord, float& y_coord, unsigned int mode )
{
	int coord_update=0,exit=0;
	int inkey;
	int punta=0;

	bool move = false;
	bool checkZPos = false;
	float moveX, moveY;
	
	/*
	if(mode & AUTOAPP_DOSAT)
	{
		Dosatore->GetConfig(CC_Dispenser);
	}
	*/

	if(mode & AUTOAPP_PUNTA1ON)
	{
		punta = 1;
	}
	else
	{
		if(mode & AUTOAPP_PUNTA2ON)
		{
			punta = 2;
		}
	}
	
	while(1)
	{
		flushKeyboardBuffer();

		inkey = Handle();

		if( fnTeachingKeyCallback )
		{
			inkey = fnTeachingKeyCallback(inkey,x_coord,y_coord);
		}

		//GF_TEMP - modificato valore: da 6 a 3
		if( clock() > (time_out+int(CLOCKS_PER_SEC/3)) )
		{
			press_count=0;
		}

		time_out=clock(); // reset timer counter

		switch(inkey)
		{
			case 'b':
			case 'B':
				Switch_UgeBlock();
				break;

			case K_SHIFT_NUMPAD_1:
				x_coord=QParam.LX_mincl+((QParam.LX_maxcl-QParam.LX_mincl)/6);
				y_coord=QParam.LY_mincl+((QParam.LY_maxcl-QParam.LY_mincl)/6);
				coord_update = 1;
				break;
			case K_SHIFT_NUMPAD_2:
				x_coord=QParam.LX_mincl+3*((QParam.LX_maxcl-QParam.LX_mincl)/6);
				y_coord=QParam.LY_mincl+((QParam.LY_maxcl-QParam.LY_mincl)/6);
				coord_update = 1;
				break;
			case K_SHIFT_NUMPAD_3:
				x_coord=QParam.LX_mincl+5*((QParam.LX_maxcl-QParam.LX_mincl)/6);
				y_coord=QParam.LY_mincl+((QParam.LY_maxcl-QParam.LY_mincl)/6);
				coord_update = 1;
				break;
			case K_SHIFT_NUMPAD_4:
				x_coord=QParam.LX_mincl+((QParam.LX_maxcl-QParam.LX_mincl)/6);
				y_coord=QParam.LY_mincl+3*((QParam.LY_maxcl-QParam.LY_mincl)/6);
				coord_update=1;
				break;
			case K_SHIFT_NUMPAD_5:
				x_coord=QParam.LX_mincl+3*((QParam.LX_maxcl-QParam.LX_mincl)/6);
				y_coord=QParam.LY_mincl+3*((QParam.LY_maxcl-QParam.LY_mincl)/6);
				coord_update = 1;
				break;
			case K_SHIFT_NUMPAD_6:
				x_coord=QParam.LX_mincl+5*((QParam.LX_maxcl-QParam.LX_mincl)/6);
				y_coord=QParam.LY_mincl+3*((QParam.LY_maxcl-QParam.LY_mincl)/6);
				coord_update = 1;
				break;
			case K_SHIFT_NUMPAD_7:
				x_coord=QParam.LX_mincl+((QParam.LX_maxcl-QParam.LX_mincl)/6);
				y_coord=QParam.LY_mincl+5*((QParam.LY_maxcl-QParam.LY_mincl)/6);
				coord_update = 1;
				break;
			case K_SHIFT_NUMPAD_8:
				x_coord=QParam.LX_mincl+3*((QParam.LX_maxcl-QParam.LX_mincl)/6);
				y_coord=QParam.LY_mincl+5*((QParam.LY_maxcl-QParam.LY_mincl)/6);
				coord_update = 1;
				break;
			case K_SHIFT_NUMPAD_9:
				x_coord=QParam.LX_mincl+5*((QParam.LX_maxcl-QParam.LX_mincl)/6);
				y_coord=QParam.LY_mincl+5*((QParam.LY_maxcl-QParam.LY_mincl)/6);
				coord_update = 1;
				break;

#ifdef __DEBUG
			case K_F12:
				set_zoom( !get_zoom() );
				break;
#endif

			case K_RIGHT:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 1, 0 );
				}
				else
				{
					Read_Keyboard();
					moveX = K_PassoX;
					moveY = 0.f;
					checkZPos = false;
					move = true;
				}
				break;

			case K_ALT_RIGHT:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 30, 0 );
				}
				else
				{
					moveX = 10.f;
					moveY = 0.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_CTRL_RIGHT:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 10, 0 );
				}
				else
				{
					moveX = 1.f;
					moveY = 0.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_SHIFT_RIGHT:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 5, 0 );
				}
				else
				{
					moveX = 0.1f;
					moveY = 0.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_LEFT:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( -1, 0 );
				}
				else
				{
					Read_Keyboard();
					moveX = -K_PassoX;
					moveY = 0.f;
					checkZPos = false;
					move = true;
				}
				break;
	
			case K_ALT_LEFT:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( -30, 0 );
				}
				else
				{
					moveX = -10.f;
					moveY = 0.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_CTRL_LEFT:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( -10, 0 );
				}
				else
				{
					moveX = -1.f;
					moveY = 0.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_SHIFT_LEFT:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( -5, 0 );
				}
				else
				{
					moveX = -0.1f;
					moveY = 0.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_UP:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 0, -1 );
				}
				else
				{
					Read_Keyboard();
					moveX = 0.f;
					moveY = K_PassoY;
					checkZPos = false;
					move = true;
				}
				break;
	
			case K_ALT_UP:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 0, -30 );
				}
				else
				{
					moveX = 0.f;
					moveY = 10.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_CTRL_UP:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 0, -10 );
				}
				else
				{
					moveX = 0.f;
					moveY = 1.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_SHIFT_UP:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 0, -5 );
				}
				else
				{
					moveX = 0.f;
					moveY = 0.1f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_DOWN:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 0, 1 );
				}
				else
				{
					Read_Keyboard();
					moveX = 0.f;
					moveY = -K_PassoY;
					checkZPos = false;
					move = true;
				}
				break;
	
			case K_ALT_DOWN:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 0, 30 );
				}
				else
				{
					moveX = 0.f;
					moveY = -10.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_CTRL_DOWN:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 0, 10 );
				}
				else
				{
					moveX = 0.f;
					moveY = -1.f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_SHIFT_DOWN:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					circle_pointer_movexy( 0, 5 );
				}
				else
				{
					moveX = 0.f;
					moveY = -0.1f;
					checkZPos = true;
					move = true;
					press_count = 0;
				}
				break;

			case K_ALT_PAGEUP:
				if(mode & AUTOAPP_PUNTA1ON)
				{
					PuntaZPosWait(1);
					PuntaZPosMm(1,-1,REL_MOVE);
				}
				if(mode & AUTOAPP_PUNTA2ON)
				{
					PuntaZPosWait(2);
					PuntaZPosMm(2,-1,REL_MOVE);
				}
				if(mode & AUTOAPP_DOSAT)
				{
					Dosatore->SetPressImpulse(DOSA_DEFAULTTIME,DOSA_DEFAULTVISC,DOSAPRESS_TESTMODE2);
				}
				if(mode & AUTOAPP_CONVEYOR)
				{
					if( Get_UseConveyor() )
					{
						MoveConveyorAndWait( 100, REL_MOVE );
					}
				}
				break;
	
			case K_ALT_PAGEDOWN:
				if(mode & AUTOAPP_PUNTA1ON)
				{
					if(!(mode & AUTOAPP_UGELLO) || (PuntaZPosMm(1,0,RET_POS)+1 <= QHeader.Zero_Ugelli))
					{
						PuntaZPosWait(1);
						PuntaZPosMm(1,1,REL_MOVE);
					}
					else
					{
						bipbip();
					}
				}
				if(mode & AUTOAPP_PUNTA2ON)
				{
					if(!(mode & AUTOAPP_UGELLO) || (PuntaZPosMm(2,0,RET_POS)+1 <= QHeader.Zero_Ugelli2))
					{
						PuntaZPosWait(2);
						PuntaZPosMm(2,1,REL_MOVE);
					}
					else
					{
						bipbip();
					}
				}
				if(mode & AUTOAPP_CONVEYOR)
				{
					if( Get_UseConveyor() )
					{
						MoveConveyorAndWait( -100, REL_MOVE );
					}
				}
				break;
	
			case K_CTRL_PAGEUP:
				if(mode & AUTOAPP_PUNTA1ON)
				{
					PuntaZPosWait(1);
					PuntaZPosMm(1,-0.5,REL_MOVE);
				}
				if(mode & AUTOAPP_PUNTA2ON)
				{
					PuntaZPosWait(2);
					PuntaZPosMm(2,-0.5,REL_MOVE);
				}
				if(mode & AUTOAPP_DOSAT)
				{
					Dosatore->SetPressImpulse(DOSA_DEFAULTTIME,DOSA_DEFAULTVISC,DOSAPRESS_TESTMODE2);
					dosa_presspulse_done=1;
				}
				if(mode & AUTOAPP_CONVEYOR)
				{
					if( Get_UseConveyor() )
					{
						MoveConveyorAndWait( 10, REL_MOVE );
					}
				}
				break;
	
			case K_CTRL_PAGEDOWN:
				if(mode & AUTOAPP_PUNTA1ON)
				{
					if(!(mode & AUTOAPP_UGELLO) || (PuntaZPosMm(1,0,RET_POS)+0.5 <= QHeader.Zero_Ugelli))
					{
						PuntaZPosWait(1);
						PuntaZPosMm(1,0.5,REL_MOVE);
					}
					else
					{
						bipbip();
					}
				}
				if(mode & AUTOAPP_PUNTA2ON)
				{
					if(!(mode & AUTOAPP_UGELLO) || (PuntaZPosMm(2,0,RET_POS)+0.5 <= QHeader.Zero_Ugelli2))
					{
						PuntaZPosWait(2);
						PuntaZPosMm(2,0.5,REL_MOVE);
					}
					else
					{
						bipbip();
					}
				}
				if(mode & AUTOAPP_DOSAT)
				{
					Dosatore->ActivateDot();
					dosa_presspulse_done=1;
				}
				if(mode & AUTOAPP_CONVEYOR)
				{
					if( Get_UseConveyor() )
					{
						MoveConveyorAndWait( -10, REL_MOVE );
					}
				}
				break;
	
			case K_PAGEUP:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					set_diameter( 0.05 );
				}
				else
				{
					if(mode & AUTOAPP_PUNTA1ON)
					{
						PuntaZPosWait(1);
						PuntaZPosStep(1,-1,REL_MOVE);
					}
					if(mode & AUTOAPP_PUNTA2ON)
					{
						PuntaZPosWait(2);
						PuntaZPosStep(2,-1,REL_MOVE);
					}

					if(mode & AUTOAPP_DOSAT)
					{
						Dosatore->GoUp();
					}
				}
				if(mode & AUTOAPP_CONVEYOR)
				{
					if( Get_UseConveyor() )
					{
						MoveConveyorAndWait( 1, REL_MOVE );
					}
				}
				break;

			case K_PAGEDOWN:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					set_diameter( -0.05 );
				}
				else
				{
					if(mode & AUTOAPP_PUNTA1ON)
					{
						if(!(mode & AUTOAPP_UGELLO) || (PuntaZPosMm(1,0,RET_POS) <= QHeader.Zero_Ugelli))
						{
							PuntaZPosWait(1);
							PuntaZPosStep(1,1,REL_MOVE);
						}
						else
						{
							bipbip();
						}
					}
					if(mode & AUTOAPP_PUNTA2ON)
					{
						if(!(mode & AUTOAPP_UGELLO) || (PuntaZPosMm(2,0,RET_POS) <= QHeader.Zero_Ugelli2))
						{
							PuntaZPosWait(2);
							PuntaZPosStep(2,1,REL_MOVE);
						}
						else
						{
							bipbip();
						}
					}

					if(mode & AUTOAPP_DOSAT)
					{
						Dosatore->GoDown();
					}
				}
				if(mode & AUTOAPP_CONVEYOR)
				{
					if( Get_UseConveyor() )
					{
						MoveConveyorAndWait( -1, REL_MOVE );
					}
				}
				break;

			case K_ENTER:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					mode &= ~AUTOAPP_DIAMTEACH;
					set_pointer( POINTER_CROSS );
				}
				else
				{
					if(punta)
					{
						XYApprend_Zpos[punta-1]=PuntaZPosMm(punta,0,RET_POS);
					}
					exit=1;
				}
				break;
				
			case K_ESC:
				if( mode & AUTOAPP_DIAMTEACH )
				{
					mode &= ~AUTOAPP_DIAMTEACH;
					set_pointer( POINTER_CROSS );
				}
				exit=1;
				break;
	
			case K_SHIFT_F4:
				if(!(mode & AUTOAPP_NOCAM))
					exit=1;
				break;

			case K_F3:
				if(!(mode & AUTOAPP_NOCAM))
				{
					press_count=0;
					if(mode & AUTOAPP_PACKBOX)
					{
						setCross(CROSS_NEXT_MODE,CROSSBOX_PACK);
					}
					else
					{
						setCross(CROSS_NEXT_MODE);
					}
				}
				break;

			case K_F4:
				if(!(mode & AUTOAPP_NOCAM))
				{
					press_count=0;
					set_bright(0);    //default
					set_contrast(0);  //default
				}
				break;
	
			case K_F5:
				if(!(mode & AUTOAPP_NOCAM))
				{
					press_count=0;
					set_bright(-512);
				}
				break;

			case K_F6:
				if(!(mode & AUTOAPP_NOCAM))
				{
					press_count=0;
					set_bright(512);
				}
				break;

			case K_F7:
				if(!(mode & AUTOAPP_NOCAM))
				{
					press_count=0;             // gnu
					set_contrast(-512);
				}
				break;

			case K_F8:
				if(!(mode & AUTOAPP_NOCAM))
				{
					press_count=0;             // gnu
					set_contrast(512);
				}
				break;

			case K_F9:
				if( mode & AUTOAPP_VECTORIAL )
				{
					if( mode & AUTOAPP_DIAMTEACH )
					{
						mode &= ~AUTOAPP_DIAMTEACH;
						set_pointer( POINTER_CROSS );
					}
					else
					{
						mode |= AUTOAPP_DIAMTEACH;
						set_pointer( POINTER_CIRCLE );
					}
				}
				break;

			case K_CTRL_T:
				if( !(mode & AUTOAPP_DIAMTEACH) )
				{
					float mmpix = (Vision.mmpix_x + Vision.mmpix_y) / 2.f;
					float posX, posY;
					unsigned short diameter = ( (get_diameter()-get_tolerance()/2.0) / mmpix );
					bool res;

					int cam = pauseLiveVideo();

					res = findCircleFiducial( posX, posY, diameter );

					Handle();

					playLiveVideo( cam );
				}
				break;

			case K_CTRL_1:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_tolerance( 0.05 );
				}
				break;

			case K_ALT_1:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_tolerance( -0.05 );
				}
				break;

			case K_CTRL_2:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_smooth( 1 );
				}
				break;

			case K_ALT_2:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_smooth( -1 );
				}
				break;

			case K_CTRL_3:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_edge( 1 );
				}
				break;

			case K_ALT_3:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_edge( -1 );
				}
				break;

			case K_CTRL_4:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_accumulator( 0.05 );
				}
				break;

			case K_ALT_4:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_accumulator( -0.05 );
				}
				break;

			case K_CTRL_X:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_area_x( 10 );
				}
				break;

			case K_ALT_X:
			case K_ALT_X-32:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_area_x( -10 );
				}
				break;

			case K_CTRL_Y:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_area_y( 10 );
				}
				break;

			case K_ALT_Y:
			case K_ALT_Y-32:
				if( mode & AUTOAPP_VECTORIAL )
				{
					set_area_y( -10 );
				}
				break;

			default:
				press_count=0;
				break;
		}

		if( move )
		{
			move = false;

			if( checkZPos && (mode & AUTOAPP_UGELLO) )
			{
				PuntaZSecurityPos(1);
				PuntaZSecurityPos(2);
		
				PuntaZPosWait(1);
				PuntaZPosWait(2);
			}

			coord_update = 1;

			if( !AutoApprendCheckMoveLimit( x_coord, y_coord, moveX, moveY, mode ) )
			{
				bipbip();
			}
			else
			{
				ResetTimerConfirmRequiredBeforeNextXYMovement();
			}
		}

		/*
		if(!exit)
		{
			ResetTimerConfirmRequiredBeforeNextXYMovement();
		}
		*/

		if(coord_update || exit)
			break;
	}

   return(inkey);
}

/*---------------------------------------------------------------------------------
Autoapprendimento senza telecamera.
Parametri di ingresso:
   X_Auto, Y_Auto: coordinate da cui partire per l'autoapprendimento
   title: titolo dell'autoapprendimento
   mode: modalita' con cui eseguire l'autoapprendimento
              AUTOAPP_NOFINEC: disabilita la protezione dei finecorsa durante l'autoapprendimento
              AUTOAPP_COMP: autoapprendimento di un componente
              AUTOAPP_PUNTA1ON: abilita la possibilita di muovere la punta 1 con CTRL+PGUP/PGDN
              AUTOAPP_PUNTA2ON: abilita la possibilita di muovere la punta 2 con CTRL+PGUP/PGDN
              AUTOAPP_ONLY1KEY: esce dopo la pressione di un tasto tra Cursore/ENTER/ESC	
              AUTOAPP_NOUPDATE: non aggiorna X_Auto e Y_Auto prima di uscire
              AUTOAPP_DOSAT   : attiva dosatore durante l'autoapprendimento
Valori di ritorno:
   Ritorna il codice dell'ultimo tasto premuto.
   Se l'autoapprendimento e' andato a buon fine, X_Auto e Y_Auto sono significativi e 
   contengono le coordinate autoapprese.
----------------------------------------------------------------------------------*/
int AutoApprendNoCam( float& X_Auto, float& Y_Auto, const char* title, unsigned int mode )
{
	float x_coord=X_Auto;
	float y_coord=Y_Auto;
	int ret_val;
	float zcur1,zcur2; //SMOD090404
	
	CWindow* Q_autonocam = new CWindow( 0 );
	Q_autonocam->SetStyle( WIN_STYLE_CENTERED_X );
	Q_autonocam->SetClientAreaSize( 50, (mode & (AUTOAPP_PUNTA1ON | AUTOAPP_PUNTA2ON | AUTOAPP_UGELLO)) ? 11 : 10 );
	Q_autonocam->SetClientAreaPos( 0, 7 );
	Q_autonocam->SetTitle( title );

	Q_autonocam->Show();

	//box di help
	if( mode & (AUTOAPP_PUNTA1ON | AUTOAPP_PUNTA2ON) )
	{
		Q_autonocam->DrawPanel( RectI( 2, Q_autonocam->GetH()/GUI_CharH() - 7, Q_autonocam->GetW()/GUI_CharW() - 4, 6) );
	}
	else
	{
		Q_autonocam->DrawPanel( RectI( 2, Q_autonocam->GetH()/GUI_CharH() - 6, Q_autonocam->GetW()/GUI_CharW() - 4, 5) );
	}

	Q_autonocam->DrawText( 5, 5, "          [arrows] -> ", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	Q_autonocam->DrawText( 5, 6, "[  Ctrl ]+[arrows] -> ", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	Q_autonocam->DrawText( 5, 7, "[  Alt  ]+[arrows] -> ", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	Q_autonocam->DrawText( 27, 5, MsgGetString(Msg_00022), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	Q_autonocam->DrawText( 27, 6, MsgGetString(Msg_00023), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	Q_autonocam->DrawText( 27, 7, MsgGetString(Msg_00024), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );

	if( mode & (AUTOAPP_PUNTA1ON | AUTOAPP_PUNTA2ON) )
	{
		Q_autonocam->DrawText( 5, 8, "[Ctrl]+[PgUp/PgDn] -> ", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
		Q_autonocam->DrawText( 27, 8, MsgGetString(Msg_01246), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
	}

	if(!(mode & AUTOAPP_ONLY1KEY)) //se modal!=un solo keystroke
	{
		//SMOD080404
		//il primo movimento deve avvenire in posizione di sicureazza
		zcur1=PuntaZPosMm(1,0,RET_POS);
		zcur2=PuntaZPosMm(2,0,RET_POS);
	
		PuntaZSecurityPos(1);
		PuntaZSecurityPos(2);
		PuntaZPosWait(1);
		PuntaZPosWait(2);
	
		if(!(mode & AUTOAPP_NOFINEC))
		{
			Set_Finec(ON);
		}

		//posiziona testa a coordinate forzando il controllo del sensore di sicurezza
		NozzleXYMove( x_coord, y_coord, mode & ~AUTOAPP_NOZSECURITY );
		Wait_PuntaXY();
		
		if(!(mode & AUTOAPP_NOFINEC))
		{
			Set_Finec(OFF);
		}

		PuntaZPosMm(1,zcur1);
		PuntaZPosMm(2,zcur2);
		PuntaZPosWait(1);
		PuntaZPosWait(2);
	}

	autoapp_running = 1;


	C_Combo xCombo(  6, 2, MsgGetString(Msg_00020), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	C_Combo yCombo( 27, 2, MsgGetString(Msg_00021), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );

	GUI_Freeze();
	xCombo.Show( Q_autonocam->GetX(), Q_autonocam->GetY() );
	yCombo.Show( Q_autonocam->GetX(), Q_autonocam->GetY() );
	GUI_Thaw();

	while(1)
	{
		// init valori in combo boxes
		GUI_Freeze();
		xCombo.SetTxt( x_coord );
		yCombo.SetTxt( y_coord );
		GUI_Thaw();


		float prevx=x_coord;
		float prevy=y_coord;
		int   gomode=mode;
		int   zMovedToSecurity=0;

		//gestione tasti per autoapprendimento
		ret_val = AutoApprendGestKey( x_coord, y_coord, mode | AUTOAPP_NOCAM );
		
		//SMOD080404 START
		if(mode & (AUTOAPP_PUNTA1ON | AUTOAPP_PUNTA2ON | AUTOAPP_UGELLO))
        {
			gomode|=AUTOAPP_NOZSECURITY;
			
			if((fabs(x_coord-prevx)>2*K_PassoX) || (fabs(y_coord-prevy)>2*K_PassoY))
			{
				//movimenti z abilitati e movimento xy richiesto e' superiore ai 2 passi
				if(!(mode & AUTOAPP_NOZMOVE_LONGXY))
				{
					zcur1=PuntaZPosMm(1,0,RET_POS);
					zcur2=PuntaZPosMm(2,0,RET_POS);
		
					//a meno che non sia esplicitamente richiesto porta la punta
					//in sicurezza prima di eseguire il movimento e controlla lo
					//stato del sensore di sicurezza
					
					//SMOD090404
					PuntaZSecurityPos(1);
					PuntaZSecurityPos(2);
					PuntaZPosWait(1);
					PuntaZPosWait(2);
					gomode=mode & ~AUTOAPP_NOZSECURITY;
					zMovedToSecurity=1;
				}
			}
		}
		//SMOD080404 END
	
		
		if(ret_val!=K_ESC)  //se non ritornato per abbandono
		{
			//se mode=un solo keystroke, aggiorna cordinate ed esci
			if(mode & AUTOAPP_ONLY1KEY)
			{
				if(!(mode & AUTOAPP_NOUPDATE))
				{
					X_Auto=x_coord;
					Y_Auto=y_coord;
				}
				break;
			}
			
			//se non ritornato per termine operazione
			if(ret_val!=K_ENTER)
			{
				//aggiorna dati in combo boxes
				GUI_Freeze();
				xCombo.SetTxt( x_coord );
				yCombo.SetTxt( y_coord );
				GUI_Thaw();

				//sposta testa
				if(!(mode & AUTOAPP_NOFINEC))
				{
					Set_Finec(ON);
				}

				NozzleXYMove( x_coord, y_coord, gomode );
			
				x_coord = GetLastXPos();
				y_coord = GetLastYPos();
				
				Wait_PuntaXY();

				if(!(mode & AUTOAPP_NOFINEC))
				{
					Set_Finec(OFF);
				}

				//SMOD090404
				if(zMovedToSecurity)
				{
					PuntaZPosMm(1,zcur1);
					PuntaZPosMm(2,zcur2);
					PuntaZPosWait(1);
					PuntaZPosWait(2);
				}
			}
			else //termine operazione return coordinate
			{
				if(!(mode & AUTOAPP_NOUPDATE))
				{
					X_Auto=x_coord;
					Y_Auto=y_coord;
				}
				break; //esci dal ciclo
			}
		}
		else
		{
			break;   //premuto ESC->esci dal ciclo
		}
	}
	
	autoapp_running = 0;
	
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
	
	ResetNozzles(); // reset stato testa

	delete Q_autonocam;
	
	return ret_val;
}



//---------------------------------------------------------------------------------
// videoCallback
// Funzioni/variabili usate per stampare a video i valori di posizione/rotazione
// delle punte
//---------------------------------------------------------------------------------
char _video_cb_X[12];
char _video_cb_Y[12];
char _video_cb_R[12];

void videoCallback_XY( void* parentWin )
{
	GUI_Freeze_Locker lock;

	C_Combo cX( 22, VIDEO_CALLBACK_Y, MsgGetString(Msg_00020), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	cX.SetTxt( _video_cb_X );

	C_Combo cY( 44, VIDEO_CALLBACK_Y, MsgGetString(Msg_00021), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	cY.SetTxt( _video_cb_Y );

	CWindow* parent = (CWindow*)parentWin;
	cX.Show( parent->GetX(), parent->GetY() );
	cY.Show( parent->GetX(), parent->GetY() );
}

void videoCallback_XYR( void* parentWin )
{
	GUI_Freeze_Locker lock;

	C_Combo cX( 18, VIDEO_CALLBACK_Y, "X:", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	cX.SetTxt( _video_cb_X );

	C_Combo cR( 33, VIDEO_CALLBACK_Y, MsgGetString(Msg_02100), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	cR.SetTxt( _video_cb_R );

	C_Combo cY( 50, VIDEO_CALLBACK_Y, "Y:", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	cY.SetTxt( _video_cb_Y );

	CWindow* parent = (CWindow*)parentWin;
	cX.Show( parent->GetX(), parent->GetY() );
	cR.Show( parent->GetX(), parent->GetY() );
	cY.Show( parent->GetX(), parent->GetY() );
}

void videoCallback_LS( void* parentWin )
{
	GUI_Freeze_Locker lock;

	char buf[80];
	CWindow* parent = (CWindow*)parentWin;

	snprintf( buf, 80, MsgGetString(Msg_01597), GetExtCam_Light() );
	parent->DrawText( 32, VIDEO_CALLBACK_Y, buf );

	if( Get_ShutterEnable() )
	{
		snprintf( buf, 80, MsgGetString(Msg_01599), GetExtCam_Shutter() );
		parent->DrawText( 32, VIDEO_CALLBACK_Y + 1, buf );
	}
}

void videoCallback_LSG( void* parentWin )
{
	GUI_Freeze_Locker lock;

	char buf[80];
	CWindow* parent = (CWindow*)parentWin;

	snprintf( buf, 80, MsgGetString(Msg_01598), GetExtCam_Gain() );
	parent->DrawText( 32, VIDEO_CALLBACK_Y, buf );

	snprintf( buf, 80, MsgGetString(Msg_01597), GetExtCam_Light() );
	parent->DrawText( 32, VIDEO_CALLBACK_Y + 1, buf );

	if( Get_ShutterEnable() )
	{
		snprintf( buf, 80, MsgGetString(Msg_01599), GetExtCam_Shutter() );
		parent->DrawText( 32, VIDEO_CALLBACK_Y + 2, buf );
	}
}

void videoCallback_Vect( void* parentWin )
{
	GUI_Freeze_Locker lock;

	char buf[80];
	double tmpD;

	C_Combo cX( 22, VIDEO_CALLBACK_Y, MsgGetString(Msg_00020), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	cX.SetTxt( _video_cb_X );

	C_Combo cY( 44, VIDEO_CALLBACK_Y, MsgGetString(Msg_00021), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	cY.SetTxt( _video_cb_Y );

	C_Combo c1( 85, VECTORIAL_TIPS_Y-4, MsgGetString(Msg_07000), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	tmpD = get_diameter();
	sprintf( buf, "%.2f", tmpD );
	c1.SetTxt( buf );

	C_Combo c2( 110, VECTORIAL_TIPS_Y-4, MsgGetString(Msg_07001), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	tmpD = get_tolerance();
	sprintf( buf, "%.2f", tmpD );
	c2.SetTxt( buf );

	C_Combo c3( 85, VECTORIAL_TIPS_Y-3, MsgGetString(Msg_07002), 7, CELL_TYPE_UINT );
	c3.SetTxt(  get_smooth() );

	C_Combo c4( 110, VECTORIAL_TIPS_Y-3, MsgGetString(Msg_07003), 7, CELL_TYPE_UINT );
	c4.SetTxt( get_edge() );

	C_Combo c5( 95, VECTORIAL_TIPS_Y-2, MsgGetString(Msg_07004), 7, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	tmpD = get_accumulator();
	sprintf( buf, "%.2f", tmpD );
	c5.SetTxt( buf );

	CWindow* parent = (CWindow*)parentWin;
	cX.Show( parent->GetX(), parent->GetY() );
	cY.Show( parent->GetX(), parent->GetY() );

	c1.Show( parent->GetX(), parent->GetY() );
	c2.Show( parent->GetX(), parent->GetY() );
	c3.Show( parent->GetX(), parent->GetY() );
	c4.Show( parent->GetX(), parent->GetY() );
	c5.Show( parent->GetX(), parent->GetY() );
}


//--------------------------------------------------------------------------
// Divide un testo al carattere ':'
//--------------------------------------------------------------------------
void G_makecombodata( const char* _src, char* _tit, char* _data )
{
	char* dots;

	*_tit = 0;
	*_data = 0;

	if( !(dots=strchr( (char*)_src,':')) )
	{
		strcpy( _tit, _src );
		return;
	}

	strncpyQ( _tit, _src, dots-_src+1 );
	strcpy( _data, dots+1 );
}


//---------------------------------------------------------------------------------
// displayInfoComp
// Mostra informazioni sul componente
//---------------------------------------------------------------------------------
void displayInfoComp( void* parentWin )
{
	GUI_Freeze_Locker lock;

	char titolo[80];
	char dati[80];

	CWindow* parent = (CWindow*)parentWin;

	if( auto_text1[0] )
	{
		G_makecombodata( auto_text1, titolo, dati );
		C_Combo c1( 21, VIDEO_CALLBACK_Y + 2, titolo, 18, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		c1.SetTxt( dati );
		c1.Show( parent->GetX(), parent->GetY() );
	}

	if( auto_text2[0] )
	{
		G_makecombodata( auto_text2, titolo, dati );
		C_Combo c2( 21, VIDEO_CALLBACK_Y + 3, titolo, 18, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		c2.SetTxt( dati );
		c2.Show( parent->GetX(), parent->GetY() );
	}

	if( auto_text3[0] )
	{
		G_makecombodata( auto_text3, titolo, dati );
		C_Combo c3( 21, VIDEO_CALLBACK_Y + 4, titolo, 18, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
		c3.SetTxt( dati );
		c3.Show( parent->GetX(), parent->GetY() );
	}
}


/*---------------------------------------------------------------------------------
Autoapprendimento tramite telecamera.
Parametri di ingresso:
   X_Auto, Y_Auto: coordinate da cui partire per l'autoapprendimento
   title: titolo dell'autoapprendimento
   mode: modalita' con cui eseguire l'autoapprendimento
   camera: identifica la telecamera da usare (default=1)
                1: telecamera 1
                2: telecamera 2

Valori di ritorno:
   Ritorna il codice dell'ultimo tasto premuto.
   Se l'apprendimento e' andato a buon fine, X_Auto e Y_Auto sono significativi e contengono
   le coordinate apprese
----------------------------------------------------------------------------------*/
int AutoApprendCam( float& X_Auto, float& Y_Auto, const char* title, int camera, unsigned int mode )
{
	float x_coord = X_Auto;
	float y_coord = Y_Auto;
	int ret_val;
	float zcur1, zcur2;
	int punta = 0;

	if(mode & (AUTOAPP_PUNTA1ON | AUTOAPP_PUNTA2ON | AUTOAPP_UGELLO))
	{
		if(mode & AUTOAPP_UGELLO)
		{
			//apprendimento ugello implica l'abilitazione dei movimenti per la singola punta 1
			mode |= AUTOAPP_PUNTA1ON;
			mode &= ~AUTOAPP_PUNTA2ON;
		}

		if(mode & AUTOAPP_PUNTA1ON)
		{
			punta = 1;
		}

		if(mode & AUTOAPP_PUNTA2ON)
		{
			punta = 2;
		}
	}

	if(punta)
	{
		XYApprend_Zpos[punta-1]=PuntaZPosMm(punta,0,RET_POS);
	}

	// setta funzione di callback
	if( !(mode & AUTOAPP_CONTROLCAM3) )
	{
		sprintf( _video_cb_X, "%7.2f", x_coord );
		sprintf( _video_cb_Y, "%7.2f", y_coord );
		
		if( mode & AUTOAPP_XYROT )
		{
			sprintf( _video_cb_R, "%6.2f", AutoApprendXYRot );
			setLiveVideoCallback( videoCallback_XYR );
		}
		else if( mode & AUTOAPP_VECTORIAL )
		{
			setLiveVideoCallback( videoCallback_Vect );
		}
		else
		{
			setLiveVideoCallback( videoCallback_XY );
		}
	}
	else
	{
		setLiveVideoCallback( videoCallback_LS );
	}

	// setta funzione per visualizzare informazioni nel riquadro di help
	void(*fnDisplayInfo)( void* parentWin ); // puntatore a funzione
	fnDisplayInfo = 0;

	if( mode & AUTOAPP_COMP )
	{
		fnDisplayInfo = displayInfoComp;
	}
	else if( mode & AUTOAPP_VECTORIAL )
	{
		fnDisplayInfo = displayVectorialTips;
	}
	else if( mode & AUTOAPP_CONVEYOR )
	{
		fnDisplayInfo = displayConveyorInfoTips;
	}
	else
	{
		fnDisplayInfo = displayInfoTips;
	}

	// setta titolo finestra video
	Set_Tv_Title( title );

	// setta tipo croce
	int cross_mode2 = 0;

	if( !(mode & AUTOAPP_COMP) )
	{
		if( mode & AUTOAPP_PACKBOX )
		{
			cross_mode2 = CROSSBOX_PACK;
		}
		else if( mode & AUTOAPP_EXTCAM )
		{
			cross_mode2 = CROSSBOX_EXTCAM;
		}
	}
	setCross( CROSS_CURR_MODE, cross_mode2 );

	// apre finestra video
	Set_Tv( 1, camera, fnDisplayInfo );

	if( !(mode & AUTOAPP_ONLY1KEY) ) //se mode != un solo keystroke
	{
		if( !(mode & AUTOAPP_NOSTART_ZSECURITY) )
		{
			//il primo movimento deve avvenire in posizione di sicureazza
			zcur1=PuntaZPosMm(1,0,RET_POS);
			zcur2=PuntaZPosMm(2,0,RET_POS);
		
			MoveComponentUpToSafePosition(1);
			MoveComponentUpToSafePosition(2);
			PuntaZPosWait(1);
			PuntaZPosWait(2);
		
			if(!(mode & AUTOAPP_NOFINEC))
			{
				Set_Finec(ON);
			}

			//posiziona testa a coordinate forzando il controllo del sensore di sicurezza
			NozzleXYMove( x_coord, y_coord,  mode & ~AUTOAPP_NOZSECURITY );
			Wait_PuntaXY();
	
			if(!(mode & AUTOAPP_NOFINEC))
			{
				Set_Finec(OFF);
			}

			PuntaZPosMm(1,zcur1);
			PuntaZPosMm(2,zcur2);
			PuntaZPosWait(1);
			PuntaZPosWait(2);
		}
		else
		{
			if(!(mode & AUTOAPP_NOFINEC))
			{
				Set_Finec(ON);
			}

			NozzleXYMove( x_coord, y_coord, mode | AUTOAPP_NOZSECURITY );
			Wait_PuntaXY();

			if(!(mode & AUTOAPP_NOFINEC))
			{
				Set_Finec(OFF);
			}
		}
	}


	int punta_vacuo = 0;

	if(mode & AUTOAPP_EXTCAM)
	{
		if(mode & AUTOAPP_PUNTA1ON)
		{
			if(Ugelli->GetInUse(1)!=-1)
			{
				punta_vacuo = 1;
			}
		}
		else
		{
			if(Ugelli->GetInUse(2)!=-1)
			{
				punta_vacuo = 2;
			}
		}
	}

	if(punta_vacuo)
	{
		Set_Vacuo(punta_vacuo,1);
	}

	autoapp_running = 1;

	while(1)
	{
		//SMOD080404 START
		float prevx=x_coord;
		float prevy=y_coord;
		int   gomode=mode;
		int   zMovedToSecurity=0;
		//SMOD080404 END
	
		//gestione tasti per autoapprendimento
		ret_val=AutoApprendGestKey( x_coord, y_coord, mode );
	
		//SMOD080404 START
		if(mode & (AUTOAPP_PUNTA1ON | AUTOAPP_PUNTA2ON | AUTOAPP_UGELLO))
		{
			gomode|=AUTOAPP_NOZSECURITY;
			
			if((fabs(x_coord-prevx)>2*K_PassoX) || (fabs(y_coord-prevy)>2*K_PassoY))
			{
				//movimenti z abilitati e movimento xy richiesto e' superiore ai 2 passi
				if(!(mode & AUTOAPP_NOZMOVE_LONGXY))
				{
					zcur1=PuntaZPosMm(1,0,RET_POS);
					zcur2=PuntaZPosMm(2,0,RET_POS);
		
					//a meno che non sia esplicitamente richiesto porta la punta
					//in sicurezza prima di eseguire il movimento e controlla lo
					//stato del sensore di sicurezza
					
					//SMOD090404
					PuntaZSecurityPos(1);
					PuntaZSecurityPos(2);
					PuntaZPosWait(1);
					PuntaZPosWait(2);
					gomode=mode & ~AUTOAPP_NOZSECURITY;
					zMovedToSecurity=1;
				}
			}
		}
		//SMOD080404 END

		//se non ritornato per abbandono
		if(ret_val!=K_ESC)
		{
			//se non ritornato per termine operazione
			if(ret_val!=K_ENTER)
			{
				//se mode=un solo keystroke, aggiorna cordinate ed esci
				if(mode & AUTOAPP_ONLY1KEY)
				{
					if(!(mode & AUTOAPP_NOUPDATE))
					{
						X_Auto = x_coord;
						Y_Auto = y_coord;
					}
					break;
				}

				//sposta testa
				if(!(mode & AUTOAPP_NOFINEC))
				{
					Set_Finec(ON);
				}
				
				NozzleXYMove( x_coord, y_coord, gomode );
				x_coord = GetLastXPos();
				y_coord = GetLastYPos();

				if(!(mode & AUTOAPP_CONTROLCAM3))
				{
					sprintf( _video_cb_X, "%7.2f", x_coord );
					sprintf( _video_cb_Y, "%7.2f", y_coord );
				}

				Wait_PuntaXY();

				if(!(mode & AUTOAPP_NOFINEC))
				{
					Set_Finec(OFF);
				}

				//SMOD090404
				if(zMovedToSecurity)
				{
					PuntaZPosMm(1,zcur1);
					PuntaZPosMm(2,zcur2);
					PuntaZPosWait(1);
					PuntaZPosWait(2);
				}
			}
			else //termine operazione return coordinate
			{
				if(!(mode & AUTOAPP_NOUPDATE))
				{
					X_Auto=x_coord;
					Y_Auto=y_coord;
				}
				break; //esci dal ciclo
			}
		}
		else
		{
			break;   //premuto ESC->esci dal ciclo
		}
	}
	
	autoapp_running = 0;
	
	Set_Tv(0);

	if(!(mode & AUTOAPP_NOEXITRESET))
	{
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
	
		PuntaZSecurityPos(1); //porta entrambe le punte in posizione di sicurezza
		PuntaZSecurityPos(2);
		PuntaRotStep(0,1);    // porta punte a zero theta
		PuntaRotStep(0,2);
		PuntaZPosWait(2);
		PuntaZPosWait(1);
	
		//SMOD250903 START
		while (!Check_PuntaRot(2)) //attendi fine rotazione testa
		{
			FoxPort->PauseLog();
		}
		FoxPort->RestartLog();
	
		while (!Check_PuntaRot(1))   //attendi fine rotazione testa
		{
			FoxPort->PauseLog();
		}
		FoxPort->RestartLog();
		//SMOD250903 END
	
		if(QParam.Dispenser)
		{
			Dosatore->GoUp();
			Dosatore->SetPressOff();
		}
	}
	
	if(punta_vacuo)
	{
		Set_Vacuo(punta_vacuo,0);
	}
	
	setLiveVideoCallback( 0 );
	
	return(ret_val);
}

//---------------------------------------------------------------------------------
// Ritorna la posizione della punta al termine dell'ultimo apprendimento in XY
// con movimento in z abilitato
//---------------------------------------------------------------------------------
float GetXYApp_LastZPos(int punta)
{
  return(XYApprend_Zpos[punta-1]);
}

/*---------------------------------------------------------------------------------
Forza il settaggio dell'abilitazione finecorsa, impedendo ad ulteriori chiamate
di Set_Finec di intervenire su esso. Per ristabilire il modo normale utilizzare
la funzione DisableForceFinc.
Parametri di ingresso:
   state: ON  forza abilitazione finecorsa
          OFf forza disabilitazione finecorsa
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void EnableForceFinec( int state )
{
	if( FinecForce_enabled )
	{
		return;
	}

	Set_Finec( state );
	FinecForce_enabled = 1;
}

//---------------------------------------------------------------------------------
// Riabilita possibilita di cambiare il settaggio dell'abilitazione finecorsa
//---------------------------------------------------------------------------------
void DisableForceFinec()
{
	FinecForce_enabled = 0;
}

//---------------------------------------------------------------------------------
// Abilita/disabilita la protezione tramite finecorsa meccanici
// Parametri di ingresso:
//   mode: stato della protezione tramite finecorsa
//              0: disabilita la protezione
//              1: abilita la protezione
//---------------------------------------------------------------------------------
void Set_Finec( int mode )
{
	if( FinecForce_enabled || !Get_UseFinec() || Get_OnFile() )
	{
		return;
	}

	if( mode )
	{
		#ifdef __USE_MOTORHEAD
		Motorhead->EnableLimits( true );
		#endif
		finec_en = 1;
	}
	else
	{
		#ifdef __USE_MOTORHEAD
		Motorhead->EnableLimits( false );
		#endif
		finec_en = 0;
	}
}

//---------------------------------------------------------------------------------
// Ritorna flag controllo finecorsa on/off
//---------------------------------------------------------------------------------
int Is_FinecEnabled()
{
	return finec_en;
}

//---------------------------------------------------------------------------------
// Ricerca dello zero macchina manuale
//---------------------------------------------------------------------------------
bool ZeroMaccManSearch(void)
{
	NozzleXYMove( 0, 0 );
	Wait_PuntaXY();

	Set_Finec(OFF);   // disabilita protezion tramite finecorsa

	float zx_coord = 0;
	float zy_coord = 0;

	if( AutoApprendCam( zx_coord, zy_coord, MsgGetString(Msg_00019), CAMERA_HEAD, AUTOAPP_MAPOFF | AUTOAPP_NOFINEC | AUTOAPP_NOLIMIT ) == AUTOAPP_OK )
	{
		return SetHome_PuntaXY();
	}
	return false;
}

//---------------------------------------------------------------------------------
// Ricerca dello zero macchina automatico
//---------------------------------------------------------------------------------
bool ZeroMaccAutoSearch()
{
	Set_Finec(OFF);

	// check zero image
	char NameFile[MAXNPATH];
	SetImageName(NameFile,ZEROMACHIMG,ELAB);
	if( access(NameFile,0) != 0 )
	{
		return false;
	}

	CPan wait( -1, 2, WAITTEXT, WAITELAB );

	SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

	float x_pos = 0, y_pos = 0;
	if( Image_match( &x_pos, &y_pos, ZEROMACHIMG ) )
	{
		return SetHome_PuntaXY();
	}

	//TODO procedura di zero: possibilita' di break da parte dell'utente
	if( Motorhead->ZeroSearch() )
	{
		NozzleXYMove( QParam.X_zmacc, QParam.Y_zmacc, AUTOAPP_NOFINEC );
		Wait_PuntaXY();

		if( !SetHome_PuntaXY() )
		{
			return false;
		}

		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

		if( Image_match( &x_pos, &y_pos, ZEROMACHIMG ) )
		{
			return SetHome_PuntaXY();
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Calibrazione dello zero macchina
// Ritorna le coordinate dello zero macchina riferite ai finecorsa meccanici
//---------------------------------------------------------------------------------
bool ZeroMaccCal( float& x_position, float& y_position )
{
	Set_Finec(OFF);

#ifdef __DISP2
	#ifndef __DOME_FEEDER
		if( Get_SingleDispenserPar() )
	#else
		if(1)
	#endif
#else
	if(1)
#endif
	{
		CPan wait( -1, 2, WAITTEXT, WAITELAB );

		// cerco limiti in automatico
		if( !Motorhead->ZeroSearch() )
		{
			return false;
		}

		// porto testa in posizione di zero memorizzata
		NozzleXYMove( QParam.X_zmacc, QParam.Y_zmacc, AUTOAPP_NOFINEC );
		Wait_PuntaXY();

		if( !SetHome_PuntaXY() )
		{
			return false;
		}
	}

	// avvio ricerca manuale
	float x_coord = 0.f;
	float y_coord = 0.f;

	Set_Tv(2); // predispone per non richiudere immagine su video

	bool ret = false;
	if( AutoApprendCam( x_coord, y_coord, MsgGetString(Msg_00019), CAMERA_HEAD, AUTOAPP_MAPOFF | AUTOAPP_NOFINEC | AUTOAPP_NOLIMIT ) == AUTOAPP_OK )
	{
		ret = true;

	#ifdef __DISP2
		#ifndef __DOME_FEEDER
			if( Get_SingleDispenserPar() )
		#else
			if(1)
		#endif
	#else
		if(1)
	#endif
		{
			// aggiorno posizione nuovo zero
			x_position = QParam.X_zmacc + x_coord;
			y_position = QParam.Y_zmacc + y_coord;

			// salvo immagine di riferimento
			ImageCaptureSave( ZEROMACHIMG, YESCHOOSE, CAMERA_HEAD );
		}
	}

	Set_Tv(3); // chiude immagine su video

	return ret;
}


//---------------------------------------------------------------------------------
// Procedura ricerca posizione telecamera esterna tramite riconoscimento immagine
// Ritorna: -1 se immagine mancante, 0 se la ricerca e' fallita, 1 altrimenti.
//----------------------------------------------------------------------------------
int ExtCamPositionSearch()
{
	char NameFile[MAXNPATH];
	SetImageName( NameFile, EXTCAM_NOZ_IMG, ELAB );
	if( access( NameFile, 0 ) != 0 )
	{
		// immagine mancante
		return -1;
	}

	CPan wait = CPan( -1, 2, WAITTEXT, WAITELAB );

	// porta punta in posizione XY
	float x_pos = QParam.AuxCam_X[0];
	float y_pos = QParam.AuxCam_Y[0];

	Set_Finec( ON );
	NozzleXYMove( x_pos, y_pos );

	set_currentcam( CAMERA_EXT );

	Wait_PuntaXY();

	// porta punta 1 in posizione Z
	PuntaZPosMm( 1, QParam.AuxCam_Z[0] );
	PuntaZPosWait( 1 );

	int found = 0;
	if( ImageMatch_ExtCam( &x_pos, &y_pos, EXTCAM_NOZ_IMG, 1 ) )
	{
		found = 1;

		// Aggiorna posizione punte
		QParam.AuxCam_X[0] = x_pos;
		QParam.AuxCam_Y[0] = y_pos;

		Mod_Par(QParam);
	}
	else
	{
		// ricerca manuale ???
		if( W_Deci( 1, MsgGetString(Msg_05159) ) )
		{
			// manuale
			if( AuxCam_App( 1 ) )
			{
				found = 1;

				Mod_Par(QParam);
			}
		}
	}

	// riporta punta in posizione di sicurezza
	PuntaZSecurityPos( 1 );
	PuntaZPosWait( 1 );

	// porta punta 2 in posizione XY
	x_pos = QParam.AuxCam_X[1];
	y_pos = QParam.AuxCam_Y[1];

	NozzleXYMove( x_pos, y_pos );
	Wait_PuntaXY();

	// porta punta 2 in posizione Z
	PuntaZPosMm( 2, QParam.AuxCam_Z[1] );
	PuntaZPosWait( 2 );

	if( ImageMatch_ExtCam( &x_pos, &y_pos, EXTCAM_NOZ_IMG, 2 ) )
	{
		found = 1;

		// Aggiorna posizione punte
		QParam.AuxCam_X[1] = x_pos;
		QParam.AuxCam_Y[1] = y_pos;

		Mod_Par(QParam);
	}
	else
	{
		// ricerca manuale ???
		if( W_Deci( 1, MsgGetString(Msg_05159) ) )
		{
			// manuale
			if( AuxCam_App( 2 ) )
			{
				found = 1;

				Mod_Par(QParam);
			}
		}
	}

	// riporta punta in posizione di sicurezza
	PuntaZSecurityPos( 2 );
	PuntaZPosWait( 2 );

	Set_Finec(OFF);

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	return found;
}


/*---------------------------------------------------------------------------------
Funzione di richiamo degli apprendimenti manuali
Parametri di ingresso:
   X_zs, Y_zs: coordinate da cui partire nell'autoapprendimento
   A_type: indice tipo autoapprendimento
   camera: identificativo della telecamera avente valore di default 1
Valori di ritorno:
   Ritorna: 1 se autoapprendimento confermato (premuto ENTER)
            0 se autoapprendimento annullato (premuto ESC)
            codice carattere di spostamento (premuto tasti cursore in modalita' ad un solo keystroke)
----------------------------------------------------------------------------------*/
int ManualTeaching( float* X_zs, float* Y_zs, const char* title, unsigned int mode, int camera, int nozzle )
{
	float x_coord = *X_zs;
	float y_coord = *Y_zs;
	int ritorno = 0;

	dosa_presspulse_done = 0;
	autoapp_extcam_rotation = 0;

	// set callback
	if( mode & AUTOAPP_EXTCAM )
	{
		SetTeachingKeyCallback( AutoAppExtCamPosCallback );
	}
	if( mode & AUTOAPP_CALLBACK_PACK1 )
	{
		SetTeachingKeyCallback( AutoAppPackCallback );
	}
	if( mode & AUTOAPP_CALLBACK_PACK2 )
	{
		SetTeachingKeyCallback( AutoAppPack2Callback );
	}

	// teaching
	if( mode & AUTOAPP_NOCAM )
	{
		ritorno = AutoApprendNoCam( x_coord, y_coord, title, mode );
	}
	else
	{
		ritorno = AutoApprendCam( x_coord, y_coord, title, camera, mode );
	}

	// reset callback
	SetTeachingKeyCallback(NULL);

	if( ritorno == AUTOAPP_OK )
	{
		ritorno = 1;
	}
	else if( ritorno == AUTOAPP_ABORT )
	{
		ritorno = 0;
	}

	// update coordinates
	if( ritorno != 0 )
	{
		*X_zs = x_coord;
		*Y_zs = y_coord;
	}
	
	#ifdef __DISP2
	if( dosa_presspulse_done )
	{
		Dosatore->DoVacuoFinalPulse(); //senza parametri: esegue per il dispenser selezionato
	}
	#endif
	
	return ritorno;
}


/*---------------------------------------------------------------------------------
Porta la testa in posizione di fine campo di lavoro le cui coordinate ed i cui parametri 
di accelerazione e velocita' sono memorizzati all'interno della struttura Qparam di tipo 
struct CfgParam i cui campi sono letti da file.
Parametri di ingresso:
   nessuno
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void HeadEndMov()
{
	Set_Finec(ON);		// abilita protezione tramite finecorsa

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
	NozzleXYMove( QParam.X_endbrd, QParam.Y_endbrd );
	Wait_PuntaXY();

	Set_Finec(OFF);		// disabilita protezione tramite finecorsa
}

//---------------------------------------------------------------------------------
// Porta la testa in posizione di zero macchina
//---------------------------------------------------------------------------------
void HeadHomeMov()
{
	Set_Finec(ON);		// abilita protezione tramite finecorsa

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
	NozzleXYMove( 0, 0 );
	Wait_PuntaXY();

	Set_Finec(OFF);		// disabilita protezione tramite finecorsa
}


/*---------------------------------------------------------------------------------
Abilitazione dell'uscita comandi identificata dal parametro di ingresso. 
Nella Quadra Laser l'unica uscita utilizzata e' quella costituita dal blocco degli ugelli 
identificata dal codice 1.
Parametri di ingresso:
   num: identificatore dell'uscita (0..3)
      : altro=uscita 0
   mode: stato dell'uscita passata come parametro
              0    :spegne l'uscita
              1    :accende l'uscita
              altro:spegne uscita.
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
//##SMOD300802
void Set_Cmd(int num,int mode)
{
	char buf[4];
	
	if((num<0 || num>3) && num!=9) //##SMOD030902
	{
		num=0;
	}

	if(mode<0)
	{
		mode=0;
	}
	else
	{
		if(mode)
		{
			mode=1;
		}
		else
		{
			mode=0;
		}
	}

	snprintf( buf, sizeof(buf),"U%1d%1d",num,mode);
	cpu_cmd(1,buf);
}

/*---------------------------------------------------------------------------------
Lettura input bus.
Parametri di ingresso:
   num: REED_UGE reed rele blocco porta ugelli
        INSPARE1 input ausiliario CTB03A
Valori di ritorno:
   1 per input disattivo
   0 altrimenti
----------------------------------------------------------------------------------*/
int Read_Input(int num)
{
	if(num<0)
		num=0;

	if(num>1)
		num=1;

	switch(num)
	{
	case REED_UGE:
		return(cpu_cmd(2,(char*)"ER1")=='C');
	case INSPARE1:
		return(cpu_cmd(2,(char*)"ER0")=='C');
	}

	return 1;
}

int WaitReedUge(int status)
{
#ifdef HWTEST_RELEASE
 	return(1);
#else
	if(!Get_UseReed())
	{
		return(1);
	}

	clock_t start=clock();
	
	status=!status;
	
	while(Read_Input(REED_UGE)==status)
	{
		if(((clock()-start)/CLOCKS_PER_SEC)>WAITREEDUGE_TIMEOUT)
		{
			if(W_Deci(1,WAITREEDUGE_ERR,MSGBOX_YLOW,NULL,0,GENERIC_ALARM)) //SMOD140305
			{
				start=clock();
			}
			else
			{
				Set_OnFile(1);
				return 0;
			}
		}
	}
	
	return(1);
#endif
}


int _ugeblock_status=0;
//---------------------------------------------------------------------------------
// Inverte lo stato del blocco porta ugelli
//---------------------------------------------------------------------------------
void Switch_UgeBlock(void)
{ 
	_ugeblock_status=_ugeblock_status ^ 1;

	Set_UgeBlock(_ugeblock_status);
}

//---------------------------------------------------------------------------------
// Apre o chiude il blocco porta ugelli
//    status: 1: chiuso, altro: aperto
//---------------------------------------------------------------------------------
void Set_UgeBlock(int status)
{
	if( Get_OnFile() )
		return;
	
	if( status != 1 )
		status = 0;

	//check pressione aria
	if(PressStatus(1))
	{
		_ugeblock_status = status;

		Set_Cmd(9,status);
	}
}


//---------------------------------------------------------------------------------
// Accende/spegne l'illuminazione per la telecamera della testa
// Parametri di ingresso:
//   status: stato dell'illuminatore
//             -1: forza spegnimento
//              0: spengi
//              altro: accendi
//----------------------------------------------------------------------------------
void Set_HeadCameraLight( int status )
{
	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	static int camera_light_status = 0; //BOOST

	if( status == 0 )
	{
		if( camera_light_status > 0 )
		{
			camera_light_status--;

			if( camera_light_status == 0 )
			{
				// turn camera led off
				FoxHead->ClearOutput( FOX, LIGHT1 );
			}
		}
	}
	else if( status == -1 )
	{
		// force turn off
		camera_light_status = 0;
		// turn camera led off
		FoxHead->ClearOutput( FOX, LIGHT1 );
	}
	else // status == 1
	{
		camera_light_status++;

		if( camera_light_status == 1 )
		{
			// turn camera led on
			FoxHead->SetOutput( FOX, LIGHT1 );
		}
	}

	centeringMutex.unlock();
}

//---------------------------------------------------------------------------------
// Setta accelerazione e velocita' degli assi XY
//
//    acc   : valore dell'accelerazione espresso in mm/sec^2
//    speed : valore dell'accelerazione espresso in mm/sec
//
// Se acc o speed uguale a zero setta valore di default cosi' come definito nella
// struttura Qparam di tipo struct CfgParam i cui campi sono letti da file
//---------------------------------------------------------------------------------
void PuntaXYAccSpeed( int acc, int speed )
{
	if( Get_OnFile() )
		return;

	if( acc == 0 )
		acc = QHeader.xyMaxAcc;

	if( speed == 0 )
		speed = QHeader.xyMaxSpeed;

	if( CurXYStat.xyAcc == acc && CurXYStat.xySpeed == speed )
		return;

	CurXYStat.xyAcc = acc;
	CurXYStat.xySpeed = speed;

	// limiti
	acc = MIN( acc, QHeader.xyMaxAcc );
	speed = MIN( speed, QHeader.xyMaxSpeed );

	// speed factor
	acc = acc * QParam.xySpeedFactor / 100.f;
	speed = speed * QParam.xySpeedFactor / 100.f;

	#ifdef __USE_MOTORHEAD
	Motorhead->SetSpeedAcc( speed / 1000.f, acc / 1000.f ); // si porta vel. in m/s e acc. in m/s^2
	#endif
}


//---------------------------------------------------------------------------
// Resetta velocita' correnti. Serve per forzare il set delle velocita'
//---------------------------------------------------------------------------
void ForceSpeedsSetting()
{
	CurXYStat.xyAcc = 0;
	CurXYStat.xySpeed = 0;
	for( int i = 0; i < 2; i++ )
	{
		CurXYStat.rotAcc[i] = 0;
		CurXYStat.rotSpeed[i] = 0;
	}
}

//---------------------------------------------------------------------------
// Setta accelerazione e velocita' assi XY dalla tabella
//    index: -1 setta velocita' massima
//---------------------------------------------------------------------------
void SetNozzleXYSpeed_Index( int index )
{
	if( index == -1 )
	{
		PuntaXYAccSpeed( QHeader.xyMaxAcc, QHeader.xyMaxSpeed );
		return;
	}

	index = MID( 0, index, 2 ) + SPEED_XY_L;
	PuntaXYAccSpeed( SpeedsTable.entries[index].a, SpeedsTable.entries[index].v );
}

//---------------------------------------------------------------------------
// Setta accelerazione e velocita' MINIME assi XY dalla tabella
//---------------------------------------------------------------------------
void SetMinNozzleXYSpeed_Index( int index1, int index2 )
{
	index1 = MID( 0, index1, 2 ) + SPEED_XY_L;
	index2 = MID( 0, index2, 2 ) + SPEED_XY_L;

	int acc = MIN( SpeedsTable.entries[index1].a, SpeedsTable.entries[index2].a );
	int speed = MIN( SpeedsTable.entries[index1].v, SpeedsTable.entries[index2].v );
	PuntaXYAccSpeed( acc, speed );
}

//---------------------------------------------------------------------------
// Setta accelerazione e velocita' di rotazione dalla tabella
//---------------------------------------------------------------------------
void SetNozzleZSpeed_Index( int nozzle, int index )
{
	if( index == -1 )
	{
		PuntaZStartStop( nozzle, SET_DEFAULT );
		PuntaZSpeed( nozzle, SET_DEFAULT );
		PuntaZAcc( nozzle, SET_DEFAULT );
		return;
	}

	index = MID( 0, index, 2 ) + SPEED_Z_L;

	PuntaZStartStop( nozzle, SpeedsTable.entries[index].s );
	PuntaZAcc( nozzle, SpeedsTable.entries[index].a );
	PuntaZSpeed( nozzle, SpeedsTable.entries[index].v );
}

//---------------------------------------------------------------------------
// Setta accelerazione e velocita' di rotazione dalla tabella
//---------------------------------------------------------------------------
void SetNozzleRotSpeed_Index( int nozzle, int index )
{
	if( index == -1 )
	{
		PuntaRotAcc( nozzle, SET_DEFAULT );
		PuntaRotSpeed( nozzle, SET_DEFAULT );
		return;
	}

	index = MID( 0, index, 2 ) + SPEED_R_L;

	PuntaRotAcc( nozzle, SpeedsTable.entries[index].a );
	PuntaRotSpeed( nozzle, SpeedsTable.entries[index].v );
}

//---------------------------------------------------------------------------
// Salva accelerazione e velocita' correnti
//---------------------------------------------------------------------------
void SaveNozzleXYSpeed()
{
	CurXYStat.savedXYAcc = CurXYStat.xyAcc;
	CurXYStat.savedXYSpeed = CurXYStat.xySpeed;
}

//---------------------------------------------------------------------------
// Ripristina accelerazione e velocita' salvate in precedenza
//---------------------------------------------------------------------------
void RestoreNozzleXYSpeed()
{
	PuntaXYAccSpeed( CurXYStat.savedXYAcc, CurXYStat.savedXYSpeed );
}

//---------------------------------------------------------------------------
// Salva velocita' rotazione corrente
//---------------------------------------------------------------------------
void SaveNozzleRotSpeed( int nozzle )
{
	CurXYStat.savedRotAcc[nozzle-1] = CurXYStat.rotAcc[nozzle-1];
	CurXYStat.savedRotSpeed[nozzle-1] = CurXYStat.rotSpeed[nozzle-1];
}

//---------------------------------------------------------------------------
// Ripristina velocita' rotazione salvata in precedenza
//---------------------------------------------------------------------------
void RestoreNozzleRotSpeed( int nozzle )
{
	PuntaRotAcc( nozzle, CurXYStat.savedRotAcc[nozzle-1] );
	PuntaRotSpeed( nozzle, CurXYStat.savedRotSpeed[nozzle-1] );
}



//---------------------------------------------------------------------------
// Setta la velocita' (minima) dell'asse verticale (Z)
//---------------------------------------------------------------------------
void PuntaZStartStop( int nozzle, int speed )
{
	static int zoldss[2] = { 0, 0 };

	if( Get_OnFile() )
		return;

	if( nozzle != 1 && nozzle != 2 )
		nozzle = 1;

	if( speed == SET_DEFAULT )
	{
		if( nozzle == 1 )
		{
			speed = QParam.Trasl_VelMin1;
		}
		else
		{
			speed = QParam.Trasl_VelMin2;
		}
	}

	if( speed != zoldss[nozzle-1] )
	{
		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		// limiti
		int speedLim = MIN( speed, QHeader.zMaxSpeed );

		if( nozzle == 1 )
		{
			FoxHead->SetVMin( STEP1, int((float)speedLim*QHeader.Step_Trasl1) );
		}
		else
		{
			FoxHead->SetVMin( STEP2, int((float)speedLim*QHeader.Step_Trasl2) );
		}

		zoldss[nozzle-1] = speed;

		centeringMutex.unlock();
	}
}

//---------------------------------------------------------------------------
// Setta la velocita' dell'asse verticale (Z)
//---------------------------------------------------------------------------
void PuntaZSpeed( int nozzle, int speed )
{
	static int zoldspeed[2] = { 0, 0 };

	if( Get_OnFile() )
		return;

	if( nozzle != 1 && nozzle != 2 )
		nozzle = 1;

	if( speed == SET_DEFAULT )
	{
		if( nozzle == 1 )
		{
			speed = QParam.velp1;
		}
		else
		{
			speed = QParam.velp2;
		}
	}

	if( speed != zoldspeed[nozzle-1] )
	{
		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		// limiti
		int speedLim = MIN( speed, QHeader.zMaxSpeed );

		if( nozzle == 1 )
		{
			FoxHead->SetVMax( STEP1, int((float)speedLim*QHeader.Step_Trasl1) );
		}
		else
		{
			FoxHead->SetVMax( STEP2, int((float)speedLim*QHeader.Step_Trasl2) );
		}

		zoldspeed[nozzle-1] = speed;

		centeringMutex.unlock();
	}
}

//---------------------------------------------------------------------------
// Setta l'accelerazione dell'asse verticale (Z)
//---------------------------------------------------------------------------
void PuntaZAcc( int nozzle, int acc )
{
	if( Get_OnFile() )
		return;

	if( nozzle != 1 && nozzle != 2 )
		nozzle = 1;

	if( acc == SET_DEFAULT )
	{
		if( nozzle == 1 )
		{
			acc = QParam.accp1;
		}
		else
		{
			acc = QParam.accp2;
		}
	}

	if( acc != currentZAcc[nozzle-1] )
	{
		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		if( criticalZAcc[nozzle-1] != 0 )
		{
			// aggiorna l'accelerazione precedente all'impostazione dell'accelerazione
			// limitata per garantire il corretto funzionamento del prossimo movimento "lungo"
			criticalZAcc[nozzle-1] = acc;
		}

		// limiti
		int accLim = MIN( acc, QHeader.zMaxAcc );

		if( nozzle == 1 )
		{
			FoxHead->SetAcc( STEP1, int((float)accLim*QHeader.Step_Trasl1) );
		}
		else
		{
			FoxHead->SetAcc( STEP2, int((float)accLim*QHeader.Step_Trasl2) );
		}

		currentZAcc[nozzle-1] = acc;

		centeringMutex.unlock();
	}
}


//---------------------------------------------------------------------------
// Setta la velocita' (in gradi/s) di rotazione della punta
//---------------------------------------------------------------------------
void PuntaRotSpeed( int nozzle, int speed )
{
	if( Get_OnFile() )
		return;

	if( nozzle != 1 && nozzle != 2 )
		nozzle = 1;

	if( speed == SET_DEFAULT )
	{
		if( nozzle == 1 )
		{
			speed = QParam.prot1_vel;
		}
		else
		{
			speed = QParam.prot2_vel;
		}
	}

	if( speed != CurXYStat.rotSpeed[nozzle-1] )
	{
		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		// limiti
		int speedLim = MIN( speed, QHeader.rotMaxSpeed );

		if( nozzle == 1 )
		{
			FoxHead->SetVMax( BRUSH1, int((float)speedLim*QHeader.Enc_step1/360) );
		}
		else
		{
			FoxHead->SetVMax( BRUSH2, int((float)speedLim*QHeader.Enc_step2/360) );
		}

		CurXYStat.rotSpeed[nozzle-1] = speed;

		centeringMutex.unlock();
	}
}


//---------------------------------------------------------------------------
// Setta l'accelerazione (in gradi/s^2) di rotazione della punta
//---------------------------------------------------------------------------
void PuntaRotAcc( int nozzle, int acc )
{
	if( Get_OnFile() )
		return;

	if( nozzle != 1 && nozzle != 2 )
		nozzle = 1;

	if( acc == SET_DEFAULT )
	{
		if( nozzle == 1 )
		{
			acc = QParam.prot1_acc;
		}
		else
		{
			acc = QParam.prot2_acc;
		}
	}

	if( acc != CurXYStat.rotAcc[nozzle-1] )
	{
		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		// limiti
		int accLim = MIN( acc, QHeader.rotMaxAcc );

		if( nozzle == 1 )
		{
			FoxHead->SetAcc( BRUSH1, int((float)accLim*QHeader.Enc_step1/360) );
		}
		else
		{
			FoxHead->SetAcc( BRUSH2, int((float)accLim*QHeader.Enc_step2/360) );
		}

		CurXYStat.rotAcc[nozzle-1] = acc;

		centeringMutex.unlock();
	}
}


/*---------------------------------------------------------------------------------
Ritorna lo stato del sensore di traslazione per punta  specificata come parametro.
Parametri di ingresso:
   punta:  punta sulla quale viene effettuato il test del sensore di traslazione
               1: punta 1
               2: punta 2
               altro: punta 1
Valori di ritorno:
   Ritorna 1 se il sensore e' coperto, 0 se e' scoperto.
----------------------------------------------------------------------------------*/
//##SMOD300802-SMOD040203
int Check_PuntaTraslStatus(int punta)
{
	if( !FoxHead->IsEnabled(FOX) )
	{
		return 0;
	}

	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	int ret;
	if(punta==2)
	{
		ret = (FoxHead->ReadInput(FOX) & TRASL2_STAT) >> 4;
	}
	else
	{
		ret = (FoxHead->ReadInput(FOX) & TRASL1_STAT) >> 3;
	}

	centeringMutex.unlock();

	return ret;
}

#define PUPTEST_STEP 20
/*---------------------------------------------------------------------------------
Alza le punte per azzeramento iniziale scoprendo il sensore di traslazione.
Parametri di ingresso:
   punta:  punta che viene portata in posizione iniziale
               1: punta 1
               2: punta 2
Valori di ritorno:
   Ritorna la distanza in mm tra la posizione attuale della punta punta e la quota del sensore
----------------------------------------------------------------------------------*/
//SMOD040203
float InitPuntaUP(int punta)
{
   if( Get_OnFile() )
     return 0;

   float delta;

   if(punta==1)
   {
     delta=1/QHeader.Step_Trasl1;
   }
   else
   {
     delta=1/QHeader.Step_Trasl2;
   }

   char buf[80];
   
   snprintf( buf, sizeof(buf),ERRINTPUP_MSG,punta);

   float pos=0;

   if(!Get_OnFile() && FoxHead->IsEnabled(FOX))
	{
		while(Check_PuntaTraslStatus(punta))
		{ 
			PuntaZPosStep(punta,-PUPTEST_STEP,REL_MOVE,ZLIMITOFF);
			PuntaZPosWait(punta);

         if( Get_OnFile() )
         {
           return 0;
         }
		}
		
		while(!Check_PuntaTraslStatus(punta))
		{
			PuntaZPosStep(punta,1,REL_MOVE,ZLIMITOFF);
			PuntaZPosWait(punta);

         pos+=delta;

         if(pos>=INITPUP_LIMIT)
         {
           //print_debug("%d\n",pos);
           W_Mess(buf,MSGBOX_YCENT,0,GENERIC_ALARM);
           
           Set_OnFile(1);
           return 0;
         }

         if( Get_OnFile() )
         {
            return 0;
         }

		}

		while(Check_PuntaTraslStatus(punta))
		{
			PuntaZPosStep(punta,-1,REL_MOVE,ZLIMITOFF);
			PuntaZPosWait(punta);

         pos-=delta;

         if(pos<=-INITPUP_LIMIT)
         {
           //print_debug("%d\n",pos);
           W_Mess(buf,MSGBOX_YCENT,0,GENERIC_ALARM);
           
           Set_OnFile(1);
           return 0;
         }

         if( Get_OnFile() )
         {
           return 0;
         }
		}
		
      return(PuntaZPosMm(punta,0,RET_POS)); //ritorna posizione attuale in mm
	}

   return 0;
}

/*---------------------------------------------------------------------------------
Scarica il componente eventualmente presente sulla punta specificata come parametro 
nella posizione identificata nella struttura Qparam di tipo struct CfgParam letta da file.
Parametri di ingresso:
   xx_punta :  punta dalla quale viene scaricato il componente
                     1: punta 1
                     2: punta 2
                     altro:punta 1
   mode     : SCARICACOMP_DEBUG  mostra pannello di debug prima di scaricare il comp.
              SCARICACOMP_NORMAL scarica il componente direttamente
   zpos     : valido solo in caso di modo debug: quota a cui portare la punta/ugello.
   cause    : stringa di testo (opzionale) che indica il motivo dello scarico. Parametro
              valido solo nel caso di modo debug.
   zlimit   : posizione z limite indicante il comoponente piu' alto presente sul piano
              di lavoro
Valori di ritorno:
   1 quando la funzione e' terminata.

//INTEGR.V1.2k+++
   
----------------------------------------------------------------------------------*/
//##SMOD300802
int ScaricaComp(int xx_punta,int mode/*=SCARICACOMP_NORMAL*/,const char *cause/*=NULL*/)
{
	if( Get_OnFile() )
		return 1;

	if(xx_punta!=2 && xx_punta!=1) //##SMOD230902
	{
		xx_punta=1;
	}

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	MoveComponentUpToSafePosition(1);
	MoveComponentUpToSafePosition(2);
	PuntaZPosWait(1);
	PuntaZPosWait(2);

	Set_Finec(ON);
	
	NozzleXYMove_N( QParam.PX_scaric, QParam.PY_scaric, xx_punta );

	Wait_PuntaXY();                                   // assi fermi

	RemovePackageFromNozzle(xx_punta);
	
	Set_Contro(xx_punta,1);
	
	delay(QHeader.Dis_vuoto);                // attesa disatt. vuoto
	
	Set_Contro(xx_punta,0);
	
	Set_Finec(OFF);
	
	return(1);
} // ScaricaComp

int ScaricaCompOnTray(FeederClass *feeder,int nozzle,int prelQuant/*-1*/,int mode/*=SCARICACOMP_NORMAL*/,const char *cause/*=NULL*/)
{
	if( Get_OnFile() )
		return 1;

	if( nozzle != 1 &&  nozzle != 2 )
	{
		nozzle = 1;
	}

	//TODO: perche' ??? a cosa serve ???
	if( !PackageOnNozzleFlag[nozzle-1] )
	{
		return 1;
	}

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
	
	MoveComponentUpToSafePosition(1);
	MoveComponentUpToSafePosition(2);
	
	bool rot_started = true;
	
	//anche se il movimento e' in corso la posizione di arrivo e' stata comunque gia' bufferizzata
	if(isCurrentZPosiziontOkForRotation(nozzle))
	{
		//esegue la rotazione se la posizione attuale lo permette
		PuntaRotDeg(PackageOnNozzlePickedRotation[nozzle-1],nozzle);
	}
	else
	{
		//altrimenti se la posizione attuale non permette la rotazione
		float current_pos = GetPuntaRotDeg( nozzle );
		float move_rel_required = fabs(current_pos - PackageOnNozzlePickedRotation[nozzle-1]);
		
		if(move_rel_required < SAFE_COMPONENT_ROTATION_LIMIT)
		{
			//il componente non deve essere ruotato per essere depositato nel vassoio o la rotazione e' piccola
			//la posizione di safe e' adatta alla rotazione che viene quindi eseguita
			PuntaRotDeg(PackageOnNozzlePickedRotation[nozzle-1],nozzle);
		}
		else
		{
			//altrimenti la rotazione verra' eseguita dopo il posizionamento xy della punta sul vassoio
			rot_started = false;		
		}
	}

	PuntaZPosWait(1);
	PuntaZPosWait(2);
	
	Set_Finec(ON);

	int tmpQuant = feeder->GetQuantity();
	
	if(prelQuant>0)
	{
		//per far si che il metodo GoPos dell'oggetto caricatore
		//vada sulla posizione da cui e' stato appena prelevato il componente
		feeder->SetQuantity(prelQuant);
	}
	
	feeder->GoPos(nozzle);

	if(prelQuant>0)
	{
		//reimposta quantita' in modo da prelevare, la prossima volta, il successivo
		feeder->SetQuantity(tmpQuant);
	}
	
	Wait_PuntaXY(); 
	
	if(!rot_started)
	{
		PuntaRotDeg_component_safe(nozzle,PackageOnNozzlePickedRotation[nozzle-1],BRUSH_ABS,false);
	}
	
	//attendi fine rotazione testa
	while( !Check_PuntaRot(nozzle) )
	{
		FoxPort->PauseLog();
	}
	FoxPort->RestartLog();	

	PuntaZPosMm(nozzle,GetPianoZPos(nozzle)-feeder->GetData().C_offprel);
	PuntaZPosWait(nozzle);
	
	Set_Contro(nozzle,1);
	
	RemovePackageFromNozzle(nozzle);
	
	delay(QHeader.Dis_vuoto);                // attesa disatt. vuoto
	
	Set_Contro(nozzle,0);

	PuntaZSecurityPos(1);
	PuntaZSecurityPos(2);
	PuntaZPosWait(1);
	PuntaZPosWait(2);

	Set_Finec(OFF);

	return(1);
}

/*---------------------------------------------------------------------------------
Setta la variabile OnFile che disabilita tutti i movimenti e le azioni meccaniche 
della macchina al valore desiderato.
Parametri di ingresso:
   val: valore da associare alla variabile OnFile
          1: disabilita la meccanica della macchina
          0: abilita la meccanica della macchina
----------------------------------------------------------------------------------*/
void Set_OnFile( int val )
{
	if(Dosatore!=NULL)
	{
		if(val)
		{
			Dosatore->Disable();
		}
		else
		{
			Dosatore->Enable();
		}
	}

	#ifdef __USE_MOTORHEAD
	if(val)
	{
		Motorhead->Disable();
	}
	else
	{
		if( Get_UseMotorhead() )
			Motorhead->Enable();
	}
	#endif

	#ifdef __SNIPER
	if(val)
	{
		if( Sniper1 != NULL )
		{
			Sniper1->Disable();
		}
		
		if( Sniper2 != NULL )
		{
			Sniper2->Disable();
		}
	}
	else
	{
		if( Get_UseSniper(1) && Sniper1 != NULL )
		{
			Sniper1->Enable();
		}
		
		if( Get_UseSniper(2) && Sniper2 != NULL )
		{
			Sniper2->Enable();
		}
	}
	#endif

	_onFile = val;
}

//---------------------------------------------------------------------------------
// Ritorna il valore del flag uscita su file
//---------------------------------------------------------------------------------
int Get_OnFile()
{
	return _onFile;
}


/*---------------------------------------------------------------------------------
Resetta tutti i parametri delle punte, ed in particolare:
- porta le punte in posizione di sicurezza
- porta le punte ad un angolo theta pari a 0
- disattiva contropressione e/o vuoto
----------------------------------------------------------------------------------*/
void ResetNozzles()
{
	PuntaZSecurityPos(1); //porta entrambe le punte in posizione di sicurezza
	PuntaZSecurityPos(2);

	Set_Vacuo(1,0);             // vuoto off, punte 1/2
	Set_Vacuo(2,0);
	Set_Contro(1,0);            // contropress. off, punte 1/2
	Set_Contro(2,0);

	PuntaRotStep(0,1);            // porta punte a zero theta
	PuntaRotStep(0,2);
	PuntaZPosWait(2);
	PuntaZPosWait(1);

	// attendi fine rotazione testa
	FoxPort->PauseLog();
	while(!Check_PuntaRot(2))
	{
		delay( 1 );
	}
	while(!Check_PuntaRot(1))
	{
		delay( 1 );
	}
	FoxPort->RestartLog();

	if(QParam.Dispenser)
	{
		#ifndef __DISP2
		Dosatore->GoUp();
		Dosatore->SetPressOff();
		#else
		Dosatore->SetPressOff(1);
		Dosatore->SetPressOff(2);
		Dosatore->GoUp(1);
		Dosatore->GoUp(2);
		#endif
	}
}


/*---------------------------------------------------------------------------------
Porta la punta ad una posizione di sicurezza. Tale posizione e' specificata dal valore 
presente nella struttura QHeader di tipo struct CfgHeader letta da file a cui viene 
sommato un ulteriore offset passato come parametro e specificato in mm.
Parametri di ingresso:
   punta:  punta che viene portata in posizione di sicurezza
               1: punta 1
               2: punta 2
               altro: punta 1
   offset: valore in mm da sommare alla posizione di sicurezza definita in Qheader. 
		   Il suo valore di default e' 0.
----------------------------------------------------------------------------------*/
void PuntaZSecurityPos(int punta,float offset)
{
	if(punta!=1 && punta!=2)
	{
		punta=1;
	}

	if(Ugelli->GetInUse(punta)==-1)
	{
		//se la punta si trova sopra la posizione di sicurezza
		if(PuntaZPosMm(punta,0,RET_POS)<=(QHeader.DMm_HeadSens[punta-1]+offset))
		{
			/*
			//SMOD010807 commentato: il sensore potrebbe non funzionare correttamente...
			//e sensore scoperto (...non dovrebbe essere altrimenti !)
			if(!Check_PuntaTraslStatus(punta))
			{
			//non fare niente
			return;
			}
			*/
		}

		//porta la punta alla posizione di sicureza
		PuntaZPosMm(punta,QHeader.DMm_HeadSens[punta-1]+offset);
	}
	else
	{
		/*
		Ugelli->GetRec(dat,punta);
		PuntaZPosMm(punta,QHeader.DMm_HeadSens[punta-1]+dat.Z_offset[punta-1]+offset);
		*/
		PuntaZPosMm(punta,-QParam.DeltaUgeCheck+offset);
	}
}


/*---------------------------------------------------------------------------------
Preleva un componente sfruttando i dati passati come parametri di ingresso.
Parametri di ingresso:
   preldat: struttura di tipo struct PrelStruct contenente i dati necessari al prelievo 
			del componente
Valori di ritorno:
   Ritorna 0 se e' richiesto l'abbandono, 1 altrimenti.
----------------------------------------------------------------------------------*/

FeederClass* prel_comp_feeder;

void PrelCompUpdateComponentQuant(int& c)
{
	UpdateComponentQuant(c,prel_comp_feeder);
}

int PrelComp( struct PrelStruct preldat )
{
	ASSEMBLY_PROFILER_MEASURE("Pick start");
	
	int punta=preldat.punta;
	
	prel_comp_feeder = preldat.caric;
	
	int(*stepF)(const char *,int,int);
	struct CfgBrush brushPar; //DANY151102
	
	stepF=preldat.stepF;
	
	if(stepF!=NULL)
	{
		if(!stepF(STEP_PDOWN,punta,0))
		{
			return 0;
		}
	}
	
	//SMOD210703 start
	float VacuoDelta,deltauge=0;
	int VacuoDelta_steps;
	float curPos[2];
	float kStep;
	
	int velZmin;
	int velZmax;
	int accZ;
	
	char dirmod;
	
	curPos[0]=PuntaZPosMm(1,0,RET_POS);
	curPos[1]=PuntaZPosMm(2,0,RET_POS);

	//nel caso di componenti prelevati da vassoio il vuoto puo' gia' essere attivo al momento della chiamata
	bool vacuo_active_on_enter = isVacuoOn(punta);

	if(!QParam.DemoMode)
	{
		if(preldat.caric!=NULL)
		{
			if((preldat.caric->GetCode()>=FIRSTTRAY) && (preldat.caric->GetQuantity()==0))
			{
				char buf[120];

				snprintf(buf, sizeof(buf), EMPTYTRAY,preldat.caric->GetCode());
				strncat(buf,"\n", sizeof(buf) );
				strncat(buf,MsgGetString(Msg_01933), sizeof(buf) );

				Set_Vacuo(punta,0);

				_xyStatus prevXYStat = CurXYStat;
				SaveNozzleXYSpeed();

				if(!W_Deci(0,buf,MSGBOX_YLOW,0,PrelCompUpdateComponentQuant,GENERIC_ALARM))
				{
					return 0;
				}

				if((prevXYStat.xPosLogical != GetLastXPos()) || (prevXYStat.yPosLogical != GetLastYPos()))
				{
					RestoreNozzleXYSpeed();
					Set_Vacuo(punta,1);

					if( !NozzleXYMove( prevXYStat.xPosLogical, prevXYStat.yPosLogical ) )
					{
						return 0;
					}
					Wait_PuntaXY();
				}
				else
				{
					Set_Vacuo(punta,1);
					delay(300);
				}
			}
		}
	}

	if(punta==1)
	{
		velZmin=QParam.Trasl_VelMin1;
		velZmax=QParam.velp1;
		accZ=QParam.accp1;
		kStep=QHeader.Step_Trasl1;
		dirmod=P1_ZDIRMOD;
	}
	else
	{
		velZmin=QParam.Trasl_VelMin2;
		velZmax=QParam.velp2;
		accZ=QParam.accp2;
		kStep=QHeader.Step_Trasl2;
		dirmod=P2_ZDIRMOD;
	}

	int double_0402_pick=0;
	
	if((preldat.caric->GetDataConstRef().C_att==CARAVA_DOUBLE) || (preldat.caric->GetDataConstRef().C_att==CARAVA_DOUBLE1))
	{
		//velocita' ridotta nella fase finale di discesa per tutti i componenti su caricatore tipo 0402
		double_0402_pick = 1;
	}

	//calcola tempo totale di movimento
	if(QHeader.PrelVacuoDelta!=0)
	{
		if(double_0402_pick && (QHeader.SoftPickDelta!=0))
		{
			float dz1=(preldat.zprel-QHeader.SoftPickDelta)-curPos[punta-1];
			float dz2=QHeader.SoftPickDelta;
		
			//tempo richiesto per muoversi dalla posizione z corrente al limite
			//a velocita' alta
			float timeMove1=CalcMoveTime(dz1,velZmin,velZmax,accZ);
			//tempo necessario per eseguire il tratto finale a velocita' ridotta
			float timeMove2 = CalcMoveTime( dz2, velZmin, SpeedsTable.entries[QHeader.softPickSpeedIndex].s, SpeedsTable.entries[QHeader.softPickSpeedIndex].a );
		
			print_debug("%5.3f %5.3f %5.3f %d\n",timeMove1,timeMove2,timeMove1+timeMove2,QHeader.PrelVacuoDelta);
		
			if(!vacuo_active_on_enter && (QHeader.PrelVacuoDelta < timeMove2))
			{
				//l'anticipo del vuoto e' tale per cui il vuoto verra' attivato
				//nella fase finale a velocita' ridotta
		
				//calcola spazio da percorrere dall'inizio della fase a velocita' ridotta
				//fino all'attivazione del vuoto
				VacuoDelta=CalcSpaceInTime(timeMove2-QHeader.PrelVacuoDelta/1000.0,dz2,velZmin,velZmax,accZ);
		
				//calcola posizione assoluta a cui attivare il vuoto
				VacuoDelta+=curPos[punta-1]+(preldat.zprel-QHeader.SoftPickDelta);
			}
			else
			{
				if(!vacuo_active_on_enter && (QHeader.PrelVacuoDelta < (timeMove1 + timeMove2)))
				{
					//l'anticipo del vuoto e' tale per cui il vuoto verra' attivato
					//nella fase iniziale a velocita' di default
			
					//calcola spazio da percorrere dall'inizio del movimento e fino all'attivazione del vuoto
					VacuoDelta=CalcSpaceInTime(timeMove1+timeMove2-QHeader.PrelVacuoDelta/1000.0,dz1,velZmin,velZmax,accZ);
			
					//calcola posizione assoluta a cui attivare il vuoto
					VacuoDelta+=curPos[punta-1];
				}
				else
				{
					//l'anticipo di attivazione del vuoto e' superiore alla durata
					//del movimento: attiva immediatamente il vuoto
					//viene anche abbassata la punta e messo in coda il movimento z
					//a velocita' ridotta
					
					if(!vacuo_active_on_enter)
					{
						if(stepF!=NULL)
						{
							if(!stepF(STEP_VACUOON,punta,0))
							{
								return 0;
							}
						}
				
						Set_Vacuo(punta,1);
						
						ASSEMBLY_PROFILER_MEASURE("Vacuo on (preactivation time > time to move down, soft pick)");
					}
			
					PuntaZPosMm(punta,preldat.zprel-QHeader.SoftPickDelta);
			
					SetNozzleZSpeed_Index( punta, QHeader.softPickSpeedIndex );
			
					PuntaZPosMm(punta,preldat.zprel);
				}
			}
		}
		else
		{
			//prelievo non 0402 o tratto a velocita' ridotta disattivato
		
			//calcola tempo necessario all'esecuzione dell'intero movimento
			float timeMove=CalcMoveTime(preldat.zprel-curPos[punta-1],velZmin,velZmax,accZ);
		
			if(!vacuo_active_on_enter && (QHeader.PrelVacuoDelta < timeMove))
			{
				//calcola posizione assoluta corrispondente all'istante temporale
				//in cui attivare il vuoto
				VacuoDelta=curPos[punta-1]+CalcSpaceInTime(timeMove-QHeader.PrelVacuoDelta/1000.0,(preldat.zprel-curPos[punta-1]),velZmin,velZmax,accZ);
			}
			else
			{
				//l'anticipo e' superiore alla durata di tutto il movimento
		
				//attiva subito il vuto
				if(!vacuo_active_on_enter)
				{
					if(stepF!=NULL)
					{
						if(!stepF(STEP_VACUOON,punta,0))
						{
							return 0;
						}
					}
					
					Set_Vacuo(punta,1);
					
					ASSEMBLY_PROFILER_MEASURE("Vacuo on (preactivation time > time to move down)");
				}
		
				//abbassa la punta
				PuntaZPosMm(punta,preldat.zprel);
			}
		}

		if(!isVacuoOn(punta))
		{
			//vuoto non ancora attivato (vero sempre eccetto che in caso di
			//anticipo attivo e di durata superiore a quella del movimento)
		
			//la punta non e' ancora scesa
			
			CfgUgelli dat;
			if(Ugelli->GetRec(dat,punta))
			{
				deltauge=dat.Z_offset[punta-1];
			}
		
			//converte in passi la posizione trovata
			VacuoDelta_steps=dirmod*int((VacuoDelta-deltauge)*kStep);
		
			//scende a quota mmprel
			if(double_0402_pick && (QHeader.SoftPickDelta!=0))
			{
				PuntaZPosMm(punta,preldat.zprel-QHeader.SoftPickDelta);
		
				SetNozzleZSpeed_Index( punta, QHeader.softPickSpeedIndex );
				
				PuntaZPosMm(punta,preldat.zprel);
			}
			else
			{
				PuntaZPosMm(punta,preldat.zprel);
			}

			if(VacuoDelta_steps>int((preldat.zprel-deltauge)*kStep))
			{
				VacuoDelta_steps=int((preldat.zprel-deltauge)*kStep);
			}

			ASSEMBLY_PROFILER_MEASURE("Wait vacuo preactivation positon");

			bool stop = false;
			while( !stop )
			{
				if( Get_OnFile() )
				{
					break;
				}

				while( !centeringMutex.try_lock() )
				{
					delay( 1 );
				}

				if( punta == 1 )
				{
					if( FoxHead->ReadPos(STEP1) >= VacuoDelta_steps )
					{
						stop = true;
					}
				}

				if( punta == 2 )
				{
					if( FoxHead->ReadPos(STEP2) >= VacuoDelta_steps )
					{
						stop = true;
					}
				}

				centeringMutex.unlock();
			}
			
			if(stepF!=NULL)
			{
				if(!stepF(STEP_VACUOON,punta,0))
				{
					//dato che a questo punto e' possibile che il componente sia gia' stato "risucchiato" da vuoto
					//ce ne si assicura aspettando che la punta raggiunga il componente che si considera quindi prelevato
					PuntaZPosWait(punta);
					SetPackageOnNozzle(punta,*(preldat.package));
					return 0;
				}
			}

			//attiva vuoto
			Set_Vacuo(punta,1);
			ASSEMBLY_PROFILER_MEASURE("vacuo on (preactivation)");
		}
	}
	else
	{
		//anticipo accensione vuoto non attivo
	
		//scende a quota mmprel
		if(double_0402_pick)
		{
			PuntaZPosMm(punta,preldat.zprel-QHeader.SoftPickDelta);
		
			SetNozzleZSpeed_Index( punta, QHeader.softPickSpeedIndex );
		
			PuntaZPosMm(punta,preldat.zprel);
		}
		else
		{
			PuntaZPosMm(punta,preldat.zprel);
		}
		
		ASSEMBLY_PROFILER_MEASURE("pick move down");
	}

	PuntaZPosWait(punta);
	ASSEMBLY_PROFILER_MEASURE("wait pick move down");
	
	SetPackageOnNozzle(punta,*(preldat.package));
	
	if(!isVacuoOn(punta))
	{
		if(stepF!=NULL)
		{
			if(!stepF(STEP_VACUOON,punta,0))
			{
				return 0;
			}
		}
		
		Set_Vacuo(punta,1);
		ASSEMBLY_PROFILER_MEASURE("vauco on");
	}
	
	delay(QHeader.ComponentDwellTime); //SMOD080807
	ASSEMBLY_PROFILER_MEASURE("touch time");
	
	
	//THFEEDER
	if( preldat.caric->GetDataConstRef().C_tipo == CARTYPE_THFEEDER )
	{
		ThFeeder->ChangeAddress( preldat.caric->GetDataConstRef().C_thFeederAdd );
		
		// sblocco thFeeder
		ThFeeder->FeederUnlock();
		// attendo fine azione
		if( !CaricWait( CARTYPE_THFEEDER, preldat.caric->GetDataConstRef().C_thFeederAdd ) )
		{
			//TEMP - segnalare errore !!!
			return 0;
		}
	}
	
	
	if(preldat.downpos!=NULL)
	{
		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		*(preldat.downpos)=FoxHead->ReadPos((punta-1)*2+1);
		if(punta==1)
		{
			*(preldat.downpos)*=P1_ZDIRMOD;
		}
		else
		{
			*(preldat.downpos)*=P2_ZDIRMOD;
		}

		centeringMutex.unlock();
	}
	//SMOD210703 end

	if(stepF!=NULL)
	{
		if(!stepF(STEP_PUP,punta,0))
		{
			return 0;
		}
	}


	//set pickup speed
	SetNozzleZSpeed_Index( punta, preldat.package->speedPick );
	
	//porta la punta a quota mmup
	PuntaZPosMm(punta,preldat.zup);

	ASSEMBLY_PROFILER_MEASURE("move up and set pickup z speed and acceleration");
	
	if(preldat.waitup)
	{
		PuntaZPosWait(punta);
	
		//reset a default z speed
		SetNozzleZSpeed_Index( punta, ACC_SPEED_DEFAULT );

		ASSEMBLY_PROFILER_MEASURE("wait for rise movement stop");
	}

	//DANY151102
	BrushDataRead( brushPar, preldat.package->centeringPID );

	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	if(punta == 1)
	{
		FoxHead->SetPID(BRUSH1,brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0]);
	}
	else
	{
		FoxHead->SetPID(BRUSH2,brushPar.p[1],brushPar.i[1],brushPar.d[1],brushPar.clip[1]);
	}
	
	centeringMutex.unlock();

	ASSEMBLY_PROFILER_MEASURE("set centering pid (pick ends)");
	
	return(1);
}

#define DELTAZ_FINEPITCH  1.1;

/*---------------------------------------------------------------------------------
Deposita un componente utilizzando i dati contenuti nella struttura di deposito puntata in depdat.
Parametri di ingresso:
   depdat: struttura di tipo struct DepoStruct contenente i dati necessari al deposito del componente
Valori di ritorno:
   Restituisce 0 se e'stato richiesto l'abbandono della procedura, 1 altrimenti.
   Se il deposito e' andato a buon fine, restituisce anche la struttura depdat aggiornata.
----------------------------------------------------------------------------------*/
int DepoComp( struct DepoStruct* depdat )
{
	int(*stepF)(const char *,int,int); //puntatore a funzione gestione passo-passo
	float pzmm = depdat->zpos;
	
	int punta=depdat->punta;
	stepF=depdat->stepF;
	
	struct CfgBrush brushPar; //DANY171002
	
	depdat->mounted=0;
	
	bool rot_started = false;

	if(depdat->dorotFlag)
	{
		if( depdat->package->centeringMode == CenteringMode::EXTCAM || depdat->package->centeringMode == CenteringMode::NONE )
		{
			PuntaZPosWait(punta);
			if(isCurrentZPosiziontOkForRotation(punta))
			{
				PuntaRotStep_component_safe(punta,depdat->rot_step,depdat->rot_mode);
				rot_started = true;
			}
		}
		else
		{
			PuntaRotStep(depdat->rot_step,punta,depdat->rot_mode);  //rotazione componente
			rot_started = true;
		}
		ASSEMBLY_PROFILER_MEASURE("place rotation");
	
		//richiama handler passo-passo se presente
		if(stepF!=NULL)
		{
			if(!stepF(STEP_COMPROTAZ,punta,0))
			{
				return 0;
			}
		}
	}
	
	if(depdat->goposFlag)
	{
		//porta la punta a coordinate di deposito
		if( !NozzleXYMove( depdat->xpos, depdat->ypos, depdat->xymode ) )
		{
			return 0;
		}
		
		ASSEMBLY_PROFILER_MEASURE("goto place xy position");
	
		if(stepF!=NULL)
		{
			if(!stepF(STEP_GODEPO,punta,0))
			{
				return 0;
			}
		}
	}

	//SMOD220103
	if(depdat->predepo_zdelta > 0)
	{
		if(stepF!=NULL)
		{
			if(!stepF(STEP_PREDISCESA,punta,0))
			{
				return 0;
			}
		}

		DoPredown(punta,GetPianoZPos(punta)-depdat->predepo_zdelta); //SMOD270404
		ASSEMBLY_PROFILER_MEASURE("Do nozzle pre-down");
	}

	//DANY171002
	//Setta i parametri del PID basandosi sui dai del package
	BrushDataRead( brushPar, depdat->package->placementPID );

	//attende fine movimento assi
	Wait_PuntaXY();
	ASSEMBLY_PROFILER_MEASURE("wait place xy position");

	if(depdat->dorotFlag && !rot_started)
	{
		if( depdat->package->centeringMode == CenteringMode::EXTCAM || depdat->package->centeringMode == CenteringMode::NONE )
		{
			PuntaZPosWait(punta);
			PuntaRotStep_component_safe(punta,depdat->rot_step,depdat->rot_mode,false);			
			rot_started = true;
		}
		else
		{
			PuntaRotStep(depdat->rot_step,punta,depdat->rot_mode);  //rotazione componente
			rot_started = true;
		}
		ASSEMBLY_PROFILER_MEASURE("place rotation after xy movement");
	
		//richiama handler passo-passo se presente
		if(stepF!=NULL)
		{
			if(!stepF(STEP_COMPROTAZ,punta,0))
			{
				return 0;
			}
		}
	}
	
	//SMOD291004
	depdat->vacuocheck_dep=1;	
	
	if( depdat->package->checkPick == 'V' || depdat->package->checkPick == 'B' )
	{
		if(stepF!=NULL)
		{
			if(!stepF(S_CHKPRESO,punta,0))
			{
				return 0;
			}
		}

		if( Check_CompIsOnNozzle( punta, depdat->package->checkPick ) )
		{
			depdat->vacuocheck_dep=0;
			return 0;
		}
		
		ASSEMBLY_PROFILER_MEASURE("Check component presence before place");
   }

	//set place speed
	SetNozzleZSpeed_Index( punta, depdat->package->speedPlace );

	ASSEMBLY_PROFILER_MEASURE("set place z speed and acceleration");
	
	//attendi fine rotazione
	Wait_EncStop(punta);
	ASSEMBLY_PROFILER_MEASURE("wait place rotation stop");

	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	if(punta == 1)
	{
		FoxHead->SetPID(BRUSH1,brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0]);
	}
	else
	{
		FoxHead->SetPID(BRUSH2,brushPar.p[1],brushPar.i[1],brushPar.d[1],brushPar.clip[1]);
	}
	
	centeringMutex.unlock();

	ASSEMBLY_PROFILER_MEASURE("set place pid");


	// modo di deposito
	//--------------------------------------------------------------------------
	if( depdat->package->placementMode == 'N' )
	{
		pzmm += QHeader.interfNorm;
	}
	else if(depdat->package->placementMode == 'F')
	{
		pzmm += QHeader.interfFine;
	}


	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	FoxHead->LogOn((punta-1)*2);

	centeringMutex.unlock();


	PuntaZPosMm( punta, pzmm );
	ASSEMBLY_PROFILER_MEASURE("place down");
	
	if( stepF )
	{
		if( !stepF(STEP_PDOWN,punta,0) )
		{
			return 0;
		}
	}
	PuntaZPosWait(punta);
	ASSEMBLY_PROFILER_MEASURE("wait place down");
	
	
	BrushDataRead(brushPar,0);

	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	if(punta == 1)
	{
		FoxHead->SetPID(BRUSH1,brushPar.p[0],brushPar.i[0],brushPar.d[0],brushPar.clip[0]);
	}
	else
	{
		FoxHead->SetPID(BRUSH2,brushPar.p[1],brushPar.i[1],brushPar.d[1],brushPar.clip[1]);
	}

	centeringMutex.unlock();

	ASSEMBLY_PROFILER_MEASURE("reset default pid");
	
	if(depdat->package->placementMode == 'F')
	{
		delay(200);
		ASSEMBLY_PROFILER_MEASURE("fine pitch place delay");
	
		//SMOD101104 - introdotto attivazione contropressione anche
		//             per deposito di tipo F
		
		//porta contropressione a punta
		//Set_Vacuo(punta,0);
	}
	//else
	{
		Set_Contro(punta,1);
		ASSEMBLY_PROFILER_MEASURE("contropressure on");
	}

	RemovePackageFromNozzle(punta);
	depdat->mounted=1; //setta componente montato=1
	
	if(stepF!=NULL)
	{
		if(!stepF(STEP_CONTROON,punta,0))
		{
			return 0;
		}
	}
	
	// attesa contropress.
	delay(QHeader.TContro_Comp);
	
	//disattiva contropressione
	Set_Contro(punta,0);
	ASSEMBLY_PROFILER_MEASURE("contropressure off");
	
	if(stepF!=NULL)
	{
		if(!stepF(STEP_CONTROOFF,punta,0))
		{
			return 0;
		}
	}

	// porta punta a zero
	PuntaZSecurityPos(punta);
	ASSEMBLY_PROFILER_MEASURE("place rise to security");
		
	if(stepF!=NULL)
	{
		if(!stepF(STEP_PUP,punta,0))
		{
			return 0;
		}
	}

	if(depdat->waitup)
	{
		//attendi fine movimento
		PuntaZPosWait(punta);
		ASSEMBLY_PROFILER_MEASURE("wait place rise to security (place ends)");
	}
	else
	{
		ASSEMBLY_PROFILER_MEASURE("place ends");
	}
	
	return(1);
}


/*---------------------------------------------------------------------------------
Funzione che verifica se il motore di rotazione della punta specificata e' in movimento.
Parametri di ingresso:
   punta: identifica la punta sulla quale effettuare il test del motore
              1: punta 1
              2: punta 2
   comboenc: (opzionale) combobox dove scrivere il valore dell'encoder mentre
             attende.
Valori di ritorno:
   Ritorna 0 se il motore e' in movimento, 1 se e' fermo.
----------------------------------------------------------------------------------*/
int Check_PuntaRot( int punta )
{
	int readstat;
	
	if( Get_OnFile() || !(FoxPort->enabled) )
		return 1;
	
	if(punta==1 && !FoxHead->IsEnabled(BRUSH1))
	{
		return 1;
	}
	
	if(punta==2 && !FoxHead->IsEnabled(BRUSH2))
	{
		return 1;
	}

	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	readstat=FoxHead->ReadStatus((punta-1)*2);
	
	centeringMutex.unlock();

	return (readstat & CODABUSY_MASK) ? 0 : 1;
}


//---------------------------------------------------------------------------
// Ritorna posizione assoluta in passi
//---------------------------------------------------------------------------
int GetPuntaRotStep( int nozzle )
{
	short int k_encoder = (nozzle == 1 ) ? QHeader.Enc_step1 : QHeader.Enc_step2;

	if( k_encoder == 2048 )
	{
		return -global_passi[nozzle-1]/2;
	}
	else
	{
		return -global_passi[nozzle-1];
	}
}

//---------------------------------------------------------------------------
// Ritorna posizione assoluta in gradi
//---------------------------------------------------------------------------
int GetPuntaRotDeg( int nozzle )
{
	return Step2Deg( GetPuntaRotStep( nozzle ), nozzle );
}

/*---------------------------------------------------------------------------------
Effettua la rotazione della punta del numero di passi specificato.
Parametri di ingresso:
   punta: identifica la punta da ruotare
              1: punta 1
              2: punta 2
   passi: numero di passi di cui effettuare la rotazione
   mode: modalita' con cui effettuare la rotazione (valore di default pari a BRUSH_ABS)
              BRUSH_ABS    : movimento assoluto
              BRUSH_REL    : movimento relativo
              BRUSH_RESET  : setta la posizione attuale come posizione di zero

Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
int PuntaRotStep( int passi, int punta, int mode )
{
	int  doPassi;

	if(punta!=1 && punta!=2) punta=1;
	
	short int *k_encoder = &QHeader.Enc_step1;

	if(punta==1 && !FoxHead->IsEnabled(BRUSH1))
	{
		return 0;
	}
	
	if(punta==2 && !FoxHead->IsEnabled(BRUSH2))
	{
		return 0;
	}

	//se tutti movimenti o fox disabilitati o se richiesto solo ritorno
	//della posizione logica corrente o se richiesto movimento relativo
	//di zero passi
	if( (mode == BRUSH_REL && passi == 0) || Get_OnFile() || !FoxPort->enabled )
	{
		//esce ritornando la posizone
		if(k_encoder[punta-1]==2048)
		{
			return -global_passi[punta-1]/2;
		}
		else
		{
			return -global_passi[punta-1];
		}
	}

	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	//se motore in protezione
	if((FoxHead->ReadStatus((punta-1)*2) & 0x10))
	{
		Set_OnFile(1);                       //disattiva tutti i movimenti

		char buf[80];
		snprintf( buf, sizeof(buf),"%s %d %s",ERRBRUSH,punta,MOVDISABLE);  //notifica errore
		W_Mess(buf,MSGBOX_YCENT,0,GENERIC_ALARM);
	}
	

	passi=-passi;
	
	//i passi sono espressi sempre in base 4096 anche se l'encoder e a 2048
	if(k_encoder[punta-1]==2048)
	{
		passi=passi*2;
	}

	switch(mode)
	{
	case BRUSH_ABS:
		global_passi[punta-1]=passi;
		break;
		
	case BRUSH_REL:
		global_passi[punta-1]+=passi;
		break;
	}

	doPassi=global_passi[punta-1];

	switch(mode)
	{
	case BRUSH_RESET:
		global_passi[punta-1]=0;
		FoxHead->SetZero((punta-1)*2);
		break;

	case BRUSH_ABS:
	case BRUSH_REL:
		FoxHead->MoveAbs((punta-1)*2,doPassi);
		break;
	}

	centeringMutex.unlock();

	if(k_encoder[punta-1]==2048)
	{
		return(-global_passi[punta-1]/2);
	}
	else
	{
		return(-global_passi[punta-1]);
	}
}


/*---------------------------------------------------------------------------------
Effettua la rotazione della punta dei gradi passati come parametro.
Parametri di ingresso:
   punta: identifica la punta da ruotare
              1: punta 1
              2: punta 2
              altro:punta 1
   degree: angolo di rotazione espresso in gradi
   mode: modalita' con cui effettuare la rotazione
              BRUSH_ABS: movimento assoluto (default)
              BRUSH_REL: movimento relativo
              BRUSH_RESET: setta la posizione attuale come posizione di zero
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
//##SMOD300802
float PuntaRotDeg(float degree_angle,int punta,int mode)
{
	int step_angle,step_const;
	
	if( punta == 2 )
	{
		step_const=QHeader.Enc_step2;
	}
	else
	{
		punta = 1;
		step_const=QHeader.Enc_step1;
	}
	
	step_angle=ftoi((step_const*degree_angle)/360);
	
	return (PuntaRotStep(step_angle,punta,mode)*360.0/step_const);
}

/*---------------------------------------------------------------------------------
Controlla se il componente e' sulla punta.
Parametri di ingresso:
   punta    : 1= punta 1
              2= punta 2
              altro= punta 1
   checkMode: modalita' di controllo

Valori di ritorno:
   Ritorna 0 se check passato, bitmask altrimenti.
   Bitmask ritornata in caso di errore:
     CHECKCOMPFAIL_VACUO: fallito check con vuoto
     CHECKCOMPFAIL_LAS  : fallito check con laser
----------------------------------------------------------------------------------*/
int Check_CompIsOnNozzle( int punta, char checkMode )
{
	int okPreso = 0;

	#ifdef HWTEST_RELEASE
	return 0;
	#endif

	if(Get_OnFile() || QParam.DemoMode)
		return 0;

	if( punta > 2 || punta < 1 )
		punta=1;

	switch( checkMode )
	{
	case 'X':
		okPreso=0;
		break;
	case 'L':
		if( QParam.Disable_LaserCompCheck )
			break;

		PuntaZPosWait(punta);

		#ifdef __SNIPER
		if( Sniper_CheckComp( punta ) == 0 )
			okPreso|=CHECKCOMPFAIL_LAS;
		#endif
		break;
	case 'V':
		if(QParam.AT_vuoto)
			if((Ugelli->Check_SogliaVacuo(punta))==0)
				okPreso|=CHECKCOMPFAIL_VACUO;
		break;
	case 'B':
		if(QParam.AT_vuoto && ((Ugelli->Check_SogliaVacuo(punta))==0))
			okPreso|=CHECKCOMPFAIL_VACUO;
		else
		{
			if(QParam.Disable_LaserCompCheck)
				break;

			PuntaZPosWait(punta);

			#ifdef __SNIPER
			if( Sniper_CheckComp( punta ) == 0 )
				okPreso|=CHECKCOMPFAIL_LAS;
			#endif
		}
		break;
	}

	return okPreso;
}

/*---------------------------------------------------------------------------------
Notifica errore di prelievo componente
Parametri di ingresso:
   punta    : 1= punta 1
              2= punta 2
              altro:punta 1
   flag     : flag errore riscontrato (ritornato da Check_CompIsOnNozzle)
   pack     : package associato al componente da controllare
Valori di ritorno:
   ritorna 1 se l'utente ha chiesto di riprovare,0 altrimenti
----------------------------------------------------------------------------------*/
int NotifyPrelError( int punta, int flag, const SPackageData& pack, const char *comp,C_Combo* combo/*=NULL*/,void(*fUpdateComp)(int&)/*=NULL*/,void(*fLoop)(void)/*=NULL*/)
{
	char buf[160],buf2[160];

	NozzleXYMove( QParam.X_endbrd, QParam.Y_endbrd );
	Wait_PuntaXY();

	snprintf( buf2, sizeof(buf2), NOPRESA, comp, punta );

	punta--;

	*buf=0;
	if( (pack.checkPick == 'V' || pack.checkPick=='B') && QParam.AT_vuoto )
	{
		strcpy(buf,CHECKVAC);
		if(flag & CHECKCOMPFAIL_VACUO)
		{
			strcat( buf, MsgGetString(Msg_01136) );
		}
		else
		{
			strcat( buf, MsgGetString(Msg_01135) );
		}
	}

	if( pack.checkPick=='L' || pack.checkPick=='B' )
	{
		if(*buf==0)
		{
			strncpy( buf, CHECKLAS, sizeof(buf) );
		}
		else
		{
			strncat( buf, "\n", sizeof(buf) );
			strncat( buf, CHECKLAS, sizeof(buf) );
		}

		if((pack.checkPick=='B') && (flag & CHECKCOMPFAIL_VACUO))
		{
			strcat(buf,".........");
		}
		else
		{
			if(flag & CHECKCOMPFAIL_LAS)
			{
				strncat(buf, MsgGetString(Msg_01136), sizeof(buf) );
			}
			else
			{
				strncat( buf, MsgGetString(Msg_01135), sizeof(buf) );
			}
		}
	}

	strncat( buf2, "\n", sizeof(buf2) );
	strncat( buf2, buf, sizeof(buf2) );

	if(fUpdateComp!=NULL)
	{
		snprintf( buf, sizeof(buf), "\n%s", MsgGetString(Msg_01933) );
		strncat( buf2, buf, sizeof(buf2) );
	}

	int ret=1;

	if( combo )
	{
		combo->SetNorColor( GUI_color(GR_WHITE), GUI_color(GR_LIGHTRED) );
		combo->Refresh();
	}

	if(!W_Deci(1,buf2,MSGBOX_YLOW,fLoop,fUpdateComp,GENERIC_ALARM)) //ask riprova-SMOD140305
	{
		ret=0;
	}

	if( combo )
	{
		combo->SetNorColor();
		combo->Refresh();
	}

	return ret;
}



void GetSchedaPos(struct Zeri master,struct Zeri zero,float x_posiz,float y_posiz,float &cx_oo,float &cy_oo)
{    float diff_x,diff_y;
     float x_oo,y_oo,x_cc,y_cc;
     float arctan,beta1,a_segment;

     double x_prod,y_prod;
     
     diff_x=zero.Z_xzero-master.Z_xzero;
     diff_y=zero.Z_yzero-master.Z_yzero;

     x_oo=master.Z_xzero;                      // coord. zero master
     y_oo=master.Z_yzero;                      //   (assolute)

     x_cc=x_posiz*master.Z_scalefactor+x_oo;                 // coord. comp. master
     y_cc=y_posiz*master.Z_scalefactor+y_oo;                 // (assolute)

     y_prod=(double)(y_cc-y_oo);
     x_prod=(double)(x_cc-x_oo);
     // esegui ricalcolo se componente non su zero scheda
     // e se zero e fiferimento master non coincidenti o troppo vicini
     // e se zero e fiferimento scheda non coincidenti o troppo vicini
	  if ((fabs(y_cc-y_oo)>0.01 || fabs(x_cc-x_oo)>0.01) &&
         (fabs(master.Z_xrif-master.Z_xzero)>MINXAPP ||
          fabs(master.Z_yrif-master.Z_yzero)>MINYAPP) &&
         (fabs(zero.Z_xrif-zero.Z_xzero)>MINXAPP ||
          fabs(zero.Z_yrif-zero.Z_yzero)>MINYAPP)) { //L704a

         arctan=atan2((y_cc-y_oo),(x_cc-x_oo)); // angolo comp. master
         beta1 =arctan+(zero.Z_rifangle-master.Z_rifangle);// angolo c.ruot.

         a_segment=sqrt(pow(x_prod,2)+pow(y_prod,2));

         cx_oo=(float)(a_segment*cos(beta1))+diff_x+x_oo;// posiz. reale comp.
         cy_oo=(float)(a_segment*sin(beta1))+diff_y+y_oo;// su board 1

     }
     else
     {
          cx_oo=(float)(x_cc+diff_x);   // comp. sullo zero, no rotaz.
          cy_oo=(float)(y_cc+diff_y);   // solo translaz.
     }
}


// Servizio: autoapprendimento componente / piazzole integrati.
int Prg_Autoall(float *X_all, float *Y_all, int mode)
{

	float ax_all, ay_all;
	float cx_all, cy_all;
	int ritorno=0;

	ax_all=*X_all;
	ay_all=*Y_all;

	switch(mode)
	{
		case 0: //normal
			if( ManualTeaching(&ax_all,&ay_all, MsgGetString(Msg_00027), AUTOAPP_COMP | AUTOAPP_XYROT) )
			{
				*X_all=ax_all;
				*Y_all=ay_all;
				ritorno=1;
			}
			else
			{
				ritorno=0;
			}
			break;
		case 1:	 //IC
			if(ManualTeaching(&ax_all,&ay_all, MsgGetString(Msg_00032), AUTOAPP_COMP | AUTOAPP_XYROT))
			{
				ritorno=1;
			}
			else
			{
				ritorno=0;
			}
		
			if(ritorno)
			{
				cx_all=ax_all;
				cy_all=ay_all;
				if(ManualTeaching(&cx_all,&cy_all, MsgGetString(Msg_00033), AUTOAPP_COMP | AUTOAPP_XYROT))
				{
					*X_all=(cx_all-ax_all)/2+ax_all;
					*Y_all=(cy_all-ay_all)/2+ay_all;
					ritorno=1;
				}
				else
				{
					ritorno=0;
				}
			}
			break;
		case 4: //ic offset ping
			if(ManualTeaching(&ax_all,&ay_all, MsgGetString(Msg_00032), AUTOAPP_COMP))
			{
				ritorno=1;
			}
			else
			{
				ritorno=0;
			}
		
			if(ritorno)
			{
				cx_all=ax_all;
				cy_all=ay_all;
				if(ManualTeaching(&cx_all,&cy_all, MsgGetString(Msg_00033), AUTOAPP_COMP))
				{
					*X_all=(cx_all-ax_all)/2+ax_all;
					*Y_all=(cy_all-ay_all)/2+ay_all;
					ritorno=1;
				}
				else
				{
					ritorno=0;
				}
			}
			break;
	}

	return(ritorno);
} // Prg_Autoall

// Apprendimento posizione componenti
// integ: 0 = autoappr. componente
//        1 = autoappr. C.I.
//        2 = autoappr. comp. con passaggio record
//        3 = autoappr. C.I.  con passaggio record      L2204
// Eliminato righe in commento - W042000
int Prg_Autocomp(int mode, int rec,TPrgFile *TPrg,struct TabPrg *ptrRec)
{
	struct Zeri Zeri[2];
	struct TabPrg TabRec,tmprec;

	float cx_oo, cy_oo;
	float diff_x,diff_y;
	float x_oo, y_oo;
	float x_cc,y_cc;
	
	int open=0;
	
	double a_segment;
	double arctan, beta1;
	double x_ris, y_ris;
	float x_int, y_int;
	float x_supp, y_supp;
	int numerorecs;
	int tipo_auto;
	int valid;
	static float x_cca, y_cca;  // memo x,y ultimo componente autoappreso

	char X_NomeFile[MAXNPATH];
	
	if(TPrg==NULL)
	{
		PrgPath(X_NomeFile,QHeader.Prg_Default);
		TPrg=new TPrgFile(X_NomeFile,PRG_NOADDPATH);
		open=1;
	}

	if(TPrg->Count()<1)   // no recs in tabella progr.
	{
		W_Mess(NORECS);
		if(open)
		{
			delete TPrg;
		}
		return 0;
	}

	TPrg->Read(TabRec,rec);
	memcpy(&tmprec,&TabRec,sizeof(struct TabPrg));
	
	//   if((!(TabRec.status & MOUNT_MASK)) || (strcmp(TabRec.TipCom,"              ") == 0))
	if(strcmp(TabRec.TipCom,"              ") == 0)
	{
		if(open)
		{
			delete TPrg;
		}
		return 0;
	}

	switch(mode)
	{
		case AUTO_IC:
		case AUTO_ICREC:
			tipo_auto=1;
			break;
		case AUTO_IC_MAPOFFSET:
			tipo_auto=4;
			break;
		default:
			tipo_auto=0;
			break;
	}
	
	ZerFile *zer=new ZerFile(QHeader.Prg_Default);
	if(!zer->Open())
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		if(open)
		{
			delete TPrg;
		}
		return 0;
	}
	
	numerorecs=zer->GetNRecs();          // n. recs in tab. zeri

	if(numerorecs<2)                     // no recs zeri presenti
	{
		delete zer;
		if(open)
		{
			delete TPrg;
		}
		W_Mess(NOZSCHEDA);
		return 0;
	}

	if((mode!=AUTO_ICREC) && (mode!=AUTO_IC_MAPOFFSET))
	{
		snprintf( auto_text1, sizeof(auto_text1), "   %s %d",MsgGetString(Msg_00308),TabRec.Riga);
		snprintf( auto_text2, sizeof(auto_text2), "   %s %s",MsgGetString(Msg_00309),TabRec.CodCom);
		snprintf( auto_text3, sizeof(auto_text3), "   %s %s",MsgGetString(Msg_00310),TabRec.TipCom);
	}

	zer->Read(Zeri[0],0);                            // read master
	zer->Read(Zeri[1],TabRec.scheda);                  // read board
	
	diff_x=Zeri[1].Z_xzero-Zeri[0].Z_xzero;
	diff_y=Zeri[1].Z_yzero-Zeri[0].Z_yzero;
	
	x_oo=Zeri[0].Z_xzero;                      // coord. zero master
	y_oo=Zeri[0].Z_yzero;                      //   (assolute)
	
	GetSchedaPos(Zeri[0],Zeri[1],TabRec.XMon,TabRec.YMon,cx_oo,cy_oo);

	x_supp=cx_oo;        // check: nessuna variazione, nessun calcolo
	y_supp=cy_oo;
	
	if(rec>0)
	{
		if((TabRec.XMon==0)&&(TabRec.YMon==0))
		{
			if(x_cca!=0)
			cx_oo=x_cca;
			if(y_cca!=0)
			cy_oo=y_cca;
		}
	}

	AutoApprendXYRot=TabRec.Rotaz; //SMOD180208

	if(Prg_Autoall(&cx_oo,&cy_oo,tipo_auto))      // autoapp. compon. / C.I.
	{
		x_cc=cx_oo-diff_x;
		y_cc=cy_oo-diff_y;
	
		// esegui ricalcolo se componente non su zero scheda
		// e se zero e fiferimento master non coincidenti o troppo vicini
		// e se zero e fiferimento scheda non coincidenti o troppo vicini
		if ((fabs(y_cc-y_oo)>0.01 || fabs(x_cc-x_oo)>0.01) &&
			(fabs(Zeri[0].Z_xrif-Zeri[0].Z_xzero)>MINXAPP ||
			fabs(Zeri[0].Z_yrif-Zeri[0].Z_yzero)>MINYAPP) &&
			(fabs(Zeri[1].Z_xrif-Zeri[1].Z_xzero)>MINXAPP ||
			fabs(Zeri[1].Z_yrif-Zeri[1].Z_yzero)>MINYAPP))
		{
			arctan=atan2((y_cc-y_oo),(x_cc-x_oo));   // angolo comp. ruot.
			beta1 =arctan-(Zeri[1].Z_rifangle-Zeri[0].Z_rifangle);
	
			a_segment=sqrt(pow((y_cc-y_oo),2)+pow((x_cc-x_oo),2));
	
			x_ris=a_segment*cos(beta1);
			y_ris=a_segment*sin(beta1);
	
		}
		else
		{
			x_ris=x_cc-x_oo;   // comp. sullo zero, nessuna rotaz.
			y_ris=y_cc-y_oo;   //L704
		}

		x_int=x_ris; // L2109
		y_int=y_ris; // L2109

		if(x_supp!=cx_oo)
		{
			TabRec.XMon=x_int/Zeri[0].Z_scalefactor;
		}
		if(y_supp!=cy_oo)
		{
			TabRec.YMon=y_int/Zeri[0].Z_scalefactor;
		}

	   valid=1;
	}
   	else
	{
		valid=0;
	}

	if(valid)
	{
		x_cca=cx_oo;
		y_cca=cy_oo;
	
		if(memcmp(&tmprec,&TabRec,sizeof(struct TabPrg))!=0)
		{
			TabRec.Changed|=PX_FIELD | PY_FIELD;
		}
	
		if(ptrRec!=NULL)
		{
			memcpy(ptrRec,&TabRec,sizeof(struct TabPrg));
		}
	
		if((mode!=AUTO_ICREC) && (mode!=AUTO_IC_MAPOFFSET))
		{
			TPrg->Write(TabRec,rec);
		}
		
	}

	if(open)
	{
		delete TPrg;
	}

	delete zer;

	return(valid);
} // Prg_Autocomp


// Massimo spostamento in millimetri per mappatura offset   // L2204
#define MAX_MAPOFFSIZE 2                                    // L2204
// Massima altezza per poter effettuare la mappatura - DANY261102
#define MAX_ZOFF 6


int M_offset(void)
{
	int loop_compo=0;
	int tot_passi_x, tot_passi_y;
	int x_passi, y_passi;
	int auto_ic;
	int x_pss, y_pss;
	// mod. W042000
	int max_step;
	int p_index;
	int ret_val,fine;

	loop_compo=Get_LastRecMount();
	
	if(loop_compo==-1)
	{
		W_Mess(NOCALIBPRG);
		return 0;
	}
	
	struct Zeri AA_Zeri[2];
	struct TabPrg AA_Tab,tmprec;
	struct CarDat AA_Caric;
	SPackageData pack;

	char buf[80];
	
	float px,py;

	TPrgFile* TPrg;

	tot_passi_x=tot_passi_y=0;
	x_passi=y_passi=0;

	// chiede se eseguire la mappatura sul programma attualmente selezionato
	snprintf( buf, sizeof(buf), MsgGetString(Msg_00561), QHeader.Prg_Default );
	if( !W_Deci(0,buf) )
	{
		return 0;
	}
	
	
	CarFile=new FeederFile(QHeader.Conf_Default);
	if(!CarFile->opened)
	{
		delete CarFile;
		CarFile=NULL;
		return 0;
	}

	TPrg=new TPrgFile(QHeader.Prg_Default,PRG_ASSEMBLY);
	if(!TPrg->Open(SKIPHEADER))
	{
		delete TPrg;
		delete CarFile;
		CarFile=NULL;
		return 0;
	}

	ZerFile *zer=new ZerFile(QHeader.Prg_Default);
	if(!zer->Open())                   // open del file zeri
	{
		W_Mess(NOZSCHFILE);
		delete zer;
		delete TPrg;
		delete CarFile;
		CarFile=NULL;
		return 0;
	}

	if( !PackagesLib_Load( QHeader.Lib_Default ) )
	{
		delete zer;
		delete TPrg;
		delete CarFile;
		CarFile=NULL;
		return 0;
	}

	// W042000 - assegnazioni spostate
	max_step=int(MAX_MAPOFFSIZE/QHeader.PassoX);  // L2204
	
	if(zer->GetNRecs()<2)                  // no recs zeri presenti
	{	
		W_Mess(NOZSCHEDA);              // ...no mappatura
		delete zer;
		delete TPrg;
		delete CarFile;
		CarFile=NULL;
		return 0;
	}

	zer->Read(AA_Zeri[0],0);                 // read board 0

	if(TPrg->Count()<1)      // no recs in tabella progr...
	{
		W_Mess(NORPROG);                // ...no mappatura
		delete zer;
		delete TPrg;
		delete CarFile;
		CarFile=NULL;
		return 0;
	}

	// vettore per memo tipo di mappatura gia' eseguita
	int c_fatti[16];
	for( int i = 0; i < 16; i++ )
		c_fatti[i] = 0;

	// setta parametri telecamera
	SetImgBrightCont( CurDat.HeadBright, CurDat.HeadContrast );

	Set_Tv(2);         //attiva visualizzazione senza interruzioni

	Set_Finec(ON);

	while(loop_compo>=0)                   // loop componenti
	{
		if(!TPrg->Read(AA_Tab,loop_compo))   // legge il componente
		{
			break;
		}
	
		if(!(AA_Tab.status & MOUNT_MASK))          // skip se flag non montare
		{
			loop_compo--;
			continue;
		}

  	  	zer->Read(AA_Zeri[1],AA_Tab.scheda);     // read board associata al comp.
		if(!AA_Zeri[1].Z_ass)                    // skip se board non da montare
		{
			loop_compo--;
			continue;
		}

		CarFile->Read(AA_Tab.Caric,AA_Caric);    // legge dati caricatore
		
		pack = currentLibPackages[AA_Caric.C_PackIndex-1]; // legge dati package associato
		if( pack.z >= MAX_ZOFF ) // skip se componente troppo alto
		{
			loop_compo--;
			continue;
		}

		float phys_theta_place_pos = Get_PhysThetaPlacePos( AA_Tab.Rotaz, pack );

		while( phys_theta_place_pos >= 360 )
		{
			phys_theta_place_pos -= 360;
		}
		while( phys_theta_place_pos < 0 )
		{
			phys_theta_place_pos += 360;
		}

		// CALCOLO INDICE VETTORE DI MAPPATURA OFFSET TESTE
		if( phys_theta_place_pos >= 45 && phys_theta_place_pos < 135 )
		{
			p_index = 1;
		}
		else if( phys_theta_place_pos >= 135 && phys_theta_place_pos < 225 )
		{
			p_index = 2;
		}
		else if( phys_theta_place_pos >= 225 && phys_theta_place_pos < 315 )
		{
			p_index = 3;
		}
		else
		{
			p_index = 0;
		}

		if( AA_Tab.Punta == '2')
		{
			p_index += 4;
		}

		if( pack.centeringMode == CenteringMode::EXTCAM )
		{
			p_index += 8;
		}

		// se tipo gia' mappato continua
		if( c_fatti[p_index] )
		{
			loop_compo--;
			continue;
		}

		c_fatti[p_index] = 1;

		auto_ic=0;
		fine=0;

		// legge i passi di correzione dal vettore di mappatura
		Read_off( x_pss, y_pss, p_index );

		// app. punti di mappatura
		do
		{
			/* sposta la telecamera sul componente senza mappatura */
			/* telecamera sul componente (no punta) */
			SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
		
			GetSchedaPos(AA_Zeri[0],AA_Zeri[1],AA_Tab.XMon,AA_Tab.YMon,px,py);
		
			px+=tot_passi_x*QHeader.PassoX;
			py+=tot_passi_y*QHeader.PassoY;

			NozzleXYMove( px, py );

			sprintf(auto_text1,"%s %3.2f",MsgGetString(Msg_00565),phys_theta_place_pos);
			sprintf(auto_text2,"%s %3d",MsgGetString(Msg_00566),AA_Tab.Punta-'0');
			sprintf(auto_text3,"%s %3d / %3d",MsgGetString(Msg_00567),x_pss,y_pss);

			ret_val = ManualTeaching(&px,&py, MsgGetString(Msg_00872), AUTOAPP_COMP | AUTOAPP_ONLY1KEY | AUTOAPP_NOUPDATE); //one shot

			switch(ret_val)
			{
				case K_RIGHT:
					if(tot_passi_x<max_step)
					{
						x_passi++;
						tot_passi_x++;
					}
					break;
				case K_LEFT:
					if(tot_passi_x>-max_step)
					{
						x_passi--;
						tot_passi_x--;
					}
					break;
				case K_UP:
					if(tot_passi_y<max_step)
					{
						y_passi++;
						tot_passi_y++;
					}
					break;
				case K_DOWN:
					if(tot_passi_y>-max_step)
					{
						y_passi--;
						tot_passi_y--;
					}
					break;
				case K_SHIFT_F4:
					if(!auto_ic)
					{
						if(Prg_Autocomp(AUTO_IC_MAPOFFSET,loop_compo,TPrg,&tmprec))  // autoapp. compon. / C.I.
						{
							x_passi=ftoi((tmprec.XMon-AA_Tab.XMon)/QHeader.PassoX);
							y_passi=ftoi((tmprec.YMon-AA_Tab.YMon)/QHeader.PassoY);
							tot_passi_x=x_passi;
							tot_passi_y=y_passi;
							auto_ic=1;
						}
					}
					else
					{
						bipbip();
					}
					break;

				case 1:
				case 0:
					fine=1;
					break;
		 	}

			if((x_passi || y_passi) && !fine) //SMOD090503
			{
				x_pss+=x_passi;
				y_pss+=y_passi;
			}

		 	x_passi=y_passi=0;
		} while(!fine);

		tot_passi_x=tot_passi_y=0;
	
		if(ret_val) //SMOD090503
		{
			Mod_off( p_index, x_pss+x_passi, y_pss+y_passi );
			Update_off();            //update su file dei dati mappatura
		}
		else
		{
			break;                   // stop autoappr., ESC premuto
		}
		loop_compo--;
	} // loop componenti
	
	Set_Finec(OFF);
	
	Set_Tv(3);         //disattiva visualizzazione senza interruzioni
	
	delete CarFile;
	delete TPrg;
	delete zer;
	
	CarFile=NULL;

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	return 1;
}

/*---------------------------------------------------------------------------------
Ritorna passi di errore rilevati all'ultimo check perdita passo asse z
Parametri di ingresso:
   punta    : 1= punta 1
              2= punta 2
Valori di ritorno:
   numero di passi persi.
----------------------------------------------------------------------------------*/
int GetZCheck_LastErr(int punta)
{
	return(lastZCheckErr[punta-1]);
}


/*---------------------------------------------------------------------------------
Check perdita passo su asse z
Parametri di ingresso:
   punta    : 1= punta 1
              2= punta 2
              altro:punta 1
   mode     : (bimap)
              Bit0 ZAXISMSGON           notifica errore       (default)
                   ZAXISMSGOFF          non notificare errore
              Bit1 ZAXISAUTORESET       riazzera automaticamente
                   ZAXISNOAUTORESET     solo check: non riazzerare (default)
Valori di ritorno:
   Ritorna 0 se check fallito, 1 altrimenti.
----------------------------------------------------------------------------------*/
int CheckZAxis(int punta,int mode)
{

	#ifdef HWTEST_RELEASE
	if(!autozero)
	{
		return(1);
	}
	#endif

	char buf[80];

	if(punta>2 || punta<1) punta=1;
	
	CPan *msg;
	
	float *step_const=&(QHeader.Step_Trasl1);
	
	int sensPos=QHeader.DStep_HeadSens[punta-1]+int(DELTA_HEADSENS*step_const[punta-1]); //SMOD280403
	
	int curPos=PuntaZPosStep(punta,0,RET_POS);
	int startPos=curPos;
	int delta=0;
	int okFlag;
	
	CfgUgelli dat;
	if(Ugelli->GetRec(dat,punta))
	{
		delta=int(dat.Z_offset[punta-1]*step_const[punta-1]);
	}

	if(mode & ZAXISMSGON)
	{
		msg=new CPan( -1,1,MsgGetString(Msg_00498));
	}
		
	//se posizione corrente sopra sensore e sensore scoperto o
	//se posizione corrente sotto sensore e sensore coperto:
	//porta a quota sensore
	if(((curPos-delta)<sensPos && !Check_PuntaTraslStatus(punta)) || ((curPos-delta)>=sensPos && Check_PuntaTraslStatus(punta)))
	{
		PuntaZPosStep(punta,sensPos+delta,ABS_MOVE);
		PuntaZPosWait(punta);
	}
	
	//cerca quota sensore
	InitPuntaUP(punta);
	curPos=PuntaZPosStep(punta,0,RET_POS);
	
	if(mode & ZAXISMSGON)
	{
		delete msg;
	}

	lastZCheckErr[punta-1]=curPos-delta-sensPos;
	
	//se rilevato errore
	if(abs(curPos-delta-sensPos)>MAXZERROR)
	{
		if(mode & ZAXISMSGON)
		{
			snprintf( buf, sizeof(buf), MsgGetString(Msg_01215), punta, curPos-delta-sensPos );
		
			//chiede riazzeramento automatico
			if(W_Deci(1,buf,MSGBOX_YCENT,0,0,GENERIC_ALARM)) //SMOD140305
			{
				//riazzera in automatico la punta
				CheckNozzleOk();
				//azzera flag di azzeramento punta su sensore: gia eseguito
				//dalla funzione CheckNozzleOk
				mode=mode & (~ZAXISAUTORESET);
			}
		}

		okFlag=0;
	}
	else
	{
		if(mode & ZAXISMSGON)
		{
			snprintf( buf, sizeof(buf),MsgGetString(Msg_00499),punta,curPos-delta-sensPos);
			W_Mess(buf);      
		}
		
		okFlag=1;
	}

	if((!(mode & ZAXISAUTORESET)) || okFlag)
	{
		PuntaZPosStep(punta,startPos,ABS_MOVE);
		PuntaZPosWait(punta);
	}
	else
	{
		PuntaZPosStep(punta,-sensPos,REL_MOVE);
		PuntaZPosWait(punta);
		PuntaZPosStep(punta,0,SET_POS);


		while( !centeringMutex.try_lock() )
		{
			delay( 1 );
		}

		if(punta==1)
		{
			FoxHead->SetZero(STEP1);
		}
		else
		{
			FoxHead->SetZero(STEP2);
		}

		centeringMutex.unlock();

		PuntaZPosStep(punta,0,ABS_MOVE);
		PuntaZPosWait(punta);
	}

  	return(okFlag);

}


//---------------------------------------------------------------------------------
// Determina una posizione z tramite la misurazione del valore del vuoto.
// Parametri di ingresso:
//    punta    : 1 = punta 1,  2 = punta 2
//    step     : passi da compiere ad ogni ciclo
//    pos      : ritorna la posizione finale
//    posLim   : limite posizione
// Valore di ritorno:
//    0 se ricerca fallita (superati limiti)
//   -1 interrotto dall'utente
//    1 se Ok
//----------------------------------------------------------------------------------
#define VZF_START_SAMPLES   15
#define VZF_DELTA_STEP      4
#define VZF_AVG_DELTA       5
#define VZF_THR             7

int VacuoFindZPosition( int punta, int step, int& pos, int posLim )
{
	PuntaZPosStep( punta, pos );
	PuntaZPosWait( punta );
	
	// attiva vuoto
	Set_Vacuo( punta, 1 );
	// Wait vacuo
	delay(QHeader.D_vuoto);
	
	
	// calc start vacuo value
	int startvacuo = 0;
	for( int i = 0; i < VZF_START_SAMPLES; i++ )
	{
		startvacuo += Get_Vacuo( punta );
		delay(25);
	}
	startvacuo /= VZF_START_SAMPLES;
	
	
	// vacuo vector
	int v_vacuo[VZF_DELTA_STEP];
	memset( v_vacuo, 0, sizeof(int) * VZF_DELTA_STEP );
	int i_vacuo = -1;
	
	// delta vector
	int v_delta[VZF_AVG_DELTA];
	memset( v_delta, 0, sizeof(int) * VZF_AVG_DELTA );
	int i_delta = -1;
	
	// sum delta
	int sum_delta = 0;
	int sum_delta_prev = sum_delta;
	
	int state = 0;
	int c = 0;

	while( 1 )
	{
		SetConfirmRequiredBeforeNextXYMovement(true);

		if( keyRead() == K_ESC )
		{
			break;
		}

		delay(25);
		
		// read vacuo
		i_vacuo++;
		if( i_vacuo == VZF_DELTA_STEP )
			i_vacuo = 0;
		
		v_vacuo[i_vacuo] = Get_Vacuo( punta ) - startvacuo;
		
		
		// calc delta and update sum_delta
		i_delta++;
		if( i_delta == VZF_AVG_DELTA )
			i_delta = 0;
		
		int i_vacuo_d4 = i_vacuo + 1;
		if( i_vacuo_d4 == VZF_DELTA_STEP )
			i_vacuo_d4 = 0;
		
		sum_delta -= v_delta[i_delta];
		v_delta[i_delta] = v_vacuo[i_vacuo] - v_vacuo[i_vacuo_d4];
		sum_delta += v_delta[i_delta];
		
		
		if( state == 0 )
		{
			if( v_vacuo[i_vacuo] > VZF_THR )
			{
				state = 1;
			}
		}
		else if( state == 1 )
		{
			if( sum_delta < sum_delta_prev )
			{
				break;
			}
		}

		sum_delta_prev = sum_delta;

		pos += step;

		if( pos > posLim )
		{
			// ricerca fallita (superati limiti)
			// disattiva vuoto
			Set_Vacuo( punta, 0 );
			return 0;
		}

		PuntaZPosStep( punta, step, REL_MOVE );
		PuntaZPosWait( punta );
	}

	// disattiva vuoto
	Set_Vacuo( punta, 0 );

	if( c == K_ESC )
	{
		// interrotto dall'utente
		return -1;
	}

	return 1;
}


/*---------------------------------------------------------------------------------
Riazzera la punta in z utilizzando il sensore di traslazione
Parametri di ingresso:
   punta    : 1= punta 1
              2= punta 2
----------------------------------------------------------------------------------*/
void ResetZAxis( int punta )
{
	CPan* wait = new CPan( -1, 1, WAITRESETZ_MSG );

	CheckZAxis( punta, ZAXISMSGOFF | ZAXISAUTORESET );

	PuntaZSecurityPos( punta );
	PuntaZPosWait( punta );

	delete wait;
}

/*---------------------------------------------------------------------------------
Esegue check bussola
Parametri di ingresso:
   punta    : 1= punta 1
              2= punta 2
   mode     : ZCHECK_GOENDBRD   va a posizione di fine scheda prima di eseguire il test.
              ZCHECK_NOGOENDBRD mantiene la posizione attuale.
----------------------------------------------------------------------------------*/
void doZCheck(int ZCheck_punta,int mode/*=ZCHECK_GOENDBRD*/) //0.41a
{
	CfgUgelli dat;
	float duge=0;
	
	float oldpos1=PuntaZPosMm(1,0,RET_POS);
	float oldpos2=PuntaZPosMm(2,0,RET_POS);
	
	if(Ugelli->GetRec(dat,ZCheck_punta))
	{
		duge=dat.Z_offset[ZCheck_punta-1];
	}

	float curX;
	float curY;

	if(mode==ZCHECK_GOENDBRD)
	{
		curX=GetLastXPos();
		curY=GetLastYPos();

		PuntaZSecurityPos(1);
		PuntaZSecurityPos(2);
		PuntaZPosWait(1);
		PuntaZPosWait(2);
		
		Set_Finec(ON);
		NozzleXYMove( QParam.X_endbrd, QParam.Y_endbrd );
		Wait_PuntaXY();
		Set_Finec(OFF);
	}

	PuntaZPosMm(ZCheck_punta,QParam.ZCheckPos[ZCheck_punta-1]+duge);
	PuntaZPosWait(ZCheck_punta);
	
	PuntaZPosMm(ZCheck_punta^3,QHeader.LaserOut);
	PuntaZPosWait(ZCheck_punta^3);
	
	#ifdef __SNIPER
	float prev_theta_pos = GetPuntaRotDeg( ZCheck_punta );
	PuntaRotDeg(90,ZCheck_punta);
	while(!Check_PuntaRot(ZCheck_punta))
	{
		FoxPort->PauseLog();
	}
	#endif

	float delta=0;
	
	if(ScanTest(ZCheck_punta,LASSCAN_BIGWIN,LASSCAN_DEFZRANGE,0,0,&delta))
	{
		if(W_Deci(0,MsgGetString(Msg_01421))) // ask to save
		{
			QParam.ZCheckPos[ZCheck_punta-1]-=delta;
			Mod_Par(QParam);
		}
	}


	if(mode==ZCHECK_GOENDBRD)
	{
		PuntaZSecurityPos(1);
		PuntaZSecurityPos(2);
		PuntaZPosWait(1);
		PuntaZPosWait(2);
	
		Set_Finec(ON);
		NozzleXYMove( curX, curY );
		Wait_PuntaXY();
		Set_Finec(OFF);
	}

	PuntaZPosMm(1,oldpos1);
	PuntaZPosMm(2,oldpos2);
	PuntaZPosWait(1);
	PuntaZPosWait(2);

	#ifdef __SNIPER
	PuntaRotDeg(prev_theta_pos,ZCheck_punta);
	
	while(!Check_PuntaRot(ZCheck_punta))
	{
		FoxPort->PauseLog();
	}
	#endif
}



/*---------------------------------------------------------------------------------
Autoapprendimento quota z
Parametri di ingresso:
  title: titolo della finestra
  nozzle: punta da utilizzare
  x,y  : coordinate assi a cui eseguire l'operazione (mm)
  z    : quota di partenza (mm)

Valore ritornato:
  in z la quota autoappresa
  1 se Ok, 0 se procedura abbandonata
----------------------------------------------------------------------------------*/
int AutoAppZPosMm( const char* title, int nozzle, float x, float y, float &z, int mode, float zmin, float zmax, struct CarDat* carRec )
{
	int c = 0;
	int move=0;
	float step;

	if(zmin==0)
	{
		zmin=QHeader.Min_NozHeight[nozzle-1];
	}
	if(zmax==0)
	{
		zmax=QHeader.Max_NozHeight[nozzle-1];
	}
	
	if(nozzle==1)
	{
		step=1/QHeader.Step_Trasl1;
	}
	else
	{
		step=1/QHeader.Step_Trasl2;
	}

	CWindow* Q_AppZ = new CWindow( 0 );
	Q_AppZ->SetStyle( WIN_STYLE_CENTERED_X );
	Q_AppZ->SetClientAreaPos( 0, 8 );
	Q_AppZ->SetClientAreaSize( 56, 14 );
	Q_AppZ->SetTitle( title );

	GUI_Freeze();
	Q_AppZ->Show();

	Q_AppZ->DrawPanel( RectI( 2, 6, Q_AppZ->GetW()/GUI_CharW() - 4, 7 ) );
	Q_AppZ->DrawText( 5,  7, MsgGetString(Msg_01366), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Q_AppZ->DrawText( 5,  8, MsgGetString(Msg_01367), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Q_AppZ->DrawText( 5,  9, MsgGetString(Msg_01368), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Q_AppZ->DrawText( 5, 10, MsgGetString(Msg_01369), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Q_AppZ->DrawText( 5, 11, MsgGetString(Msg_01370), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );


	CComboList* zPosCList = new CComboList( Q_AppZ );

	C_Combo* cPos = new C_Combo( 4, 1, MsgGetString(Msg_01372), 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	cPos->SetVMinMax( zmin, zmax );
	zPosCList->Add( cPos, 0, 0 );

	C_Combo* cVacuo = 0;
	if( mode & APPRENDZPOS_VACUO )
	{
		cVacuo = new C_Combo( 4, 2, MsgGetString(Msg_01426), 8, CELL_TYPE_SINT, CELL_STYLE_NOSEL | CELL_STYLE_READONLY );
		zPosCList->Add( cVacuo, 1, 0 );
	}

	C_Combo* cCarCode = 0;
	C_Combo* cCarComp = 0;
	if( carRec )
	{
		cCarCode = new C_Combo( 4, 3, MsgGetString(Msg_00311),  8, CELL_TYPE_UINT, CELL_STYLE_NOSEL | CELL_STYLE_READONLY );
		cCarComp = new C_Combo( 4, 4, MsgGetString(Msg_00310), 20, CELL_TYPE_UINT, CELL_STYLE_NOSEL | CELL_STYLE_READONLY );

		cCarCode->SetTxt( carRec->C_codice );
		cCarComp->SetTxt( carRec->C_comp );

		zPosCList->Add( cCarCode, 2, 0 );
		zPosCList->Add( cCarComp, 3, 0 );
	}

	zPosCList->Show();
	zPosCList->SetEdit( 1 );
	GUI_Thaw();


	float zpos = z;
	float zstart = z;
	if(mode & APPRENDZPOS_DELTAMODE)
	{
		cPos->SetTxt(zpos-zstart);
	}
	else
	{
		cPos->SetTxt(zpos);
	}

	// porta punta in posizione
	Set_Finec(ON);
	NozzleXYMove_N( x, y, nozzle );

	// attende assi XY fermi
	Wait_PuntaXY();
	Set_Finec(OFF);
	
	if(mode & APPRENDZPOS_VACUO)
	{
		Set_Vacuo(nozzle,1);
		// Wait vacuo
		delay(QHeader.D_vuoto);
	}

	PuntaZPosMm(nozzle,zpos);
	PuntaZPosWait(nozzle);

	do
	{
		if(mode & APPRENDZPOS_VACUO)
		{
			cVacuo->SetTxt(Get_Vacuo(nozzle));
		}
		
		SetConfirmRequiredBeforeNextXYMovement(true);

		c = Handle( false );
		if( c != 0 )
		{
			switch(c)
			{
				case K_ALT_UP:
					if((zpos-10)>zmin)
					{
						move=1;
						zpos-=10;
					}
					else
					{
						bipbip();
					}
					break;
				case K_ALT_DOWN:
					if((zpos+10)<zmax)
					{
						move=1;
						zpos+=10;
					}
					else
					{
						bipbip();
					}
          			break;
				case K_CTRL_UP:
					if((zpos-1)>zmin)
					{
						move=1;
						zpos-=1;
					}
					else
					{
						bipbip();
					}
					break;
				case K_CTRL_DOWN:
					if((zpos+1)<zmax)
					{
						move=1;
						zpos+=1;
					}
					else
					{
						bipbip();
					}
					break;
				case K_UP:
					if((zpos-step)>zmin)
					{
						move=1;
						zpos-=step;
					}
					else
					{
						bipbip();
					}
					break;
				case K_DOWN:
					if((zpos+step)<zmax)
					{
						move=1;
						zpos+=step;
					}
					else
					{
						bipbip();
					}
					break;
				default:
					zPosCList->ManageKey( c );
			
					if( zpos != cPos->GetFloat() )
					{
						zpos = cPos->GetFloat();
						move = 1;
					}

					if(mode & APPRENDZPOS_DELTAMODE)
					{
						zpos+=zstart;
					}
					break;
			}

			if( move )
			{
				if(mode & APPRENDZPOS_DELTAMODE)
				{
					cPos->SetTxt(zpos-zstart);
				}
				else
				{
					cPos->SetTxt(zpos);
				}
		
				PuntaZPosMm(nozzle,zpos);
				PuntaZPosWait(nozzle);
				move=0;
			}
		}
	} while((c!=K_ESC) && (c!=K_ENTER));

	if(mode & APPRENDZPOS_VACUO)
	{
		Set_Vacuo(nozzle,0);
	}
	
	PuntaZSecurityPos(nozzle);
	PuntaZPosWait(nozzle);
	
	if((!(mode & APPRENDZPOS_NOXYZERORET)) || (c == K_ESC))
	{
		Set_Finec(ON);
		NozzleXYMove(0,0);
		Wait_PuntaXY();
		Set_Finec(OFF);
	}

	delete Q_AppZ;
	delete cPos;
	if( cVacuo )
	{
		delete cVacuo;
	}
	if( carRec )
	{
		delete cCarCode;
		delete cCarComp;
	}
	delete zPosCList;

	if(c==K_ENTER)
	{
		z=zpos;
		return(1);
	}
	else
	{
		return 0;
	}
}

//TODO: non serve -> e' una copia di AutoAppZPosMm
int AutoAppZPosStep(const char* title,char* tip,int punta,float x,float y,int &z,int mode)
{
	int c=0;
	int move=0;
	float step;
	int zStep=z;
	
	if(punta==1)
	{
		step=QHeader.Step_Trasl1;
	}
	else
	{
		step=QHeader.Step_Trasl2;
	}


	CWindow* Q_AppZ = new CWindow( 0 );
	Q_AppZ->SetStyle( WIN_STYLE_CENTERED_X );
	Q_AppZ->SetClientAreaPos( 0, 8 );
	Q_AppZ->SetClientAreaSize( 56, 14 );
	Q_AppZ->SetTitle( title );

	GUI_Freeze();
	Q_AppZ->Show();

	Q_AppZ->DrawPanel( RectI( 2, 5, Q_AppZ->GetW()/GUI_CharW() - 4, 8 ) );
	Q_AppZ->DrawText( 5,  6, tip, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Q_AppZ->DrawText( 5,  7, MsgGetString(Msg_01366), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Q_AppZ->DrawText( 5,  8, MsgGetString(Msg_01367), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Q_AppZ->DrawText( 5,  9, MsgGetString(Msg_01368), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Q_AppZ->DrawText( 5, 10, MsgGetString(Msg_01369), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	Q_AppZ->DrawText( 5, 11, MsgGetString(Msg_01370), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );


	CComboList* zPosCList = new CComboList( Q_AppZ );

	C_Combo* cPos = new C_Combo( 4, 1, MsgGetString(Msg_01372), 8, CELL_TYPE_SINT, CELL_STYLE_DEFAULT, 2 );
	cPos->SetVMinMax( -3000, 3000 );
	cPos->SetTxt(zStep);
	zPosCList->Add( cPos, 0, 0 );

	C_Combo* cVacuo = 0;
	if( mode & APPRENDZPOS_VACUO )
	{
		cVacuo = new C_Combo( 4, 2, MsgGetString(Msg_01426), 8, CELL_TYPE_SINT, CELL_STYLE_NOSEL | CELL_STYLE_READONLY );
		zPosCList->Add( cVacuo, 1, 0 );
	}

	zPosCList->Show();
	zPosCList->SetEdit( 1 );
	GUI_Thaw();


	Set_Finec(ON);
	NozzleXYMove( x, y, punta );
	Wait_PuntaXY();
	Set_Finec(OFF);
	
	if(mode & APPRENDZPOS_VACUO) //SMOD300503
	{
		Set_Vacuo(punta,1);
		// Wait vacuo
		delay(QHeader.D_vuoto);
	}
	
	PuntaZPosStep(punta,zStep);
	PuntaZPosWait(punta);

	do
	{
		if(mode & APPRENDZPOS_VACUO)
		{
			//SMOD300503
			cVacuo->SetTxt(Get_Vacuo(punta));
		}
		
		SetConfirmRequiredBeforeNextXYMovement(true);

		c = Handle( false );
		if( c != 0 )
		{
			switch(c)
			{
				case K_ALT_UP:
					move=1;
					zStep-=int(step*10);
					break;
				case K_ALT_DOWN:
					move=1;
					zStep+=int(step*10);
					break;
				case K_CTRL_UP:
					move=1;
					zStep-=int(step);
					break;
				case K_CTRL_DOWN:
					move=1;
					zStep+=int(step);
					break;
				case K_UP:
					move=1;
					zStep--;
					break;
				case K_DOWN:
					move=1;
					zStep++;
					break;
				default:
					zPosCList->ManageKey( c );
			
					if( zStep != cPos->GetFloat() )
					{
						zStep = cPos->GetFloat();
						move=1;
					}
					break;
			}

			if(move)
			{
				cPos->SetTxt(zStep);

				PuntaZPosStep(punta,zStep);
				PuntaZPosWait(punta);
				move=0;
			}
    	}
  	} while((c!=K_ESC) && (c!=K_ENTER));

	if(mode & APPRENDZPOS_VACUO)
	{
		Set_Vacuo(punta,0);
	}

	PuntaZSecurityPos(punta);
	PuntaZPosWait(punta);
	
	Set_Finec(ON);
	NozzleXYMove(0,0);
	Wait_PuntaXY();
	Set_Finec(OFF);

	delete Q_AppZ;
	if( cVacuo )
	{
		delete cVacuo;
	}
	delete cPos;
	delete zPosCList;

	if(c == K_ENTER)
	{
		z=zStep;
		return(1);
	}
	else
	{
		return 0;
	}
}

void SetPackageOnNozzle( int nozzle, const SPackageData& pack )
{
	PackageOnNozzle[nozzle-1] = pack;
	PackageOnNozzleFlag[nozzle-1] = 1;
	PackageOnNozzlePickedRotation[nozzle-1] = GetPuntaRotDeg( nozzle );
}

void RemovePackageFromNozzle( int nozzle )
{
	PackageOnNozzleFlag[nozzle-1]=0;
	
	SetNozzleZSpeed_Index( nozzle, ACC_SPEED_DEFAULT );
	SetNozzleRotSpeed_Index( nozzle, ACC_SPEED_DEFAULT );
}

//---------------------------------------------------------------------------------
// Rimuove il package dalla punta. **** SOLO PER SNIPER ****
//---------------------------------------------------------------------------------
void RemovePackageFromNozzle_SNIPER( int nozzle )
{
	PackageOnNozzleFlag[nozzle-1] = 0;
}

int GetPackageOnNozzle( int nozzle, SPackageData& pack )
{
	if( !PackageOnNozzleFlag[nozzle-1] )
	{
		return 0;
	}

	pack = PackageOnNozzle[nozzle-1];
	return 1;
}


/*---------------------------------------------------------------------------------

INTEGR.V1.2k

Funzione che verifica se il motore di rotazione della punta specificata e' realmente
fermo.
   punta  : identifica la punta sulla quale effettuare il test del motore
              1: punta 1
              2: punta 2
   delta  : delta minimo per cui si suppone fermo il motore
   time   : tempo (in ms) minimo per cui si devono trovare spostamenti
            inferiori a delta
   timeMax: tempo limite (in ms) oltre cui il motore deve essere certamente
            fermo   
Valori di ritorno:
       1 se ok
       0 se attesa fallita
----------------------------------------------------------------------------------*/

int Wait_EncStop(int punta)
{
	return(Wait_EncStop(punta,QHeader.Time_1,QHeader.Time_2,QHeader.BrushSteady_Step[punta-1]));
}

int Wait_EncStop(int punta,int time,int timeMax,unsigned int delta)
{
	int motor,state=0;
	char buf[160];
	int curclock,curclock0;
	int deltatime,deltatime0;
	int fine=0,err=0;
	
	int count=0;
	int datax[MAXWAITENC_ARRAY];
	int datay[MAXWAITENC_ARRAY];
	
	int miny=9999999;
	int maxy=-99999999;
	
	int stepMotor;

	if(punta==1)
	{
		stepMotor=QHeader.Enc_step1;
		motor=BRUSH1;
	}
	else
	{
		stepMotor=QHeader.Enc_step2;
		motor=BRUSH2;
	}
	
	if(stepMotor==2048)
		delta*=2;
	
	//SMOD250903
	while(!Check_PuntaRot(punta))
	{
		FoxPort->PauseLog();
		delay(1);
	}
	FoxPort->RestartLog();


	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	int Enc;
	int endEnc = FoxHead->GetLastRotPos(motor);
	int start,start0;
	
	Timer t;
	t.start();
	
	start0 = t.getElapsedTimeInMilliSec();
	
	do
	{
		curclock0 = t.getElapsedTimeInMilliSec();
		deltatime0 = curclock0 - start0;
	
		Enc=FoxHead->ReadBrushPosition(motor);
		FoxPort->PauseLog(); //SMOD250903

		if(((QHeader.debugMode1 & DEBUG1_WAITENCGR) || (QHeader.debugMode1 & DEBUG1_WAITENCGR_ONERR)) && (count<MAXWAITENC_ARRAY))
		{
			if(Enc<miny)
			{
				miny=Enc;
			}
			if(Enc>maxy)
			{
				maxy=Enc;
			}
	
			datay[count]=Enc;
			datax[count]=deltatime0;
			count++;
		}

		if(deltatime0>timeMax)
		{
			//SMOD061003
			if(punta==1)
			{
				snprintf( buf, sizeof(buf),ERRBRUSHSTEADY,punta,Enc-endEnc);
			}
			else
			{
				snprintf( buf, sizeof(buf),ERRBRUSHSTEADY,punta,(Enc-endEnc)/2);
			}
	
			/*
			print_debug("-----------------------------------------------\n");

			print_debug("End Pos %d\n",endEnc);

			for(int j=0;j<count;j++)
			{
				print_debug("Time = %5.3f   Enc=%5.3f\n",datax[j],datay[j]);
			}

			print_debug("-----------------------------------------------\n");
			*/
			
			W_Mess(buf,MSGBOX_YLOW,0,GENERIC_ALARM);
		
			err=1;
			break;
		}

		if(abs(Enc-endEnc)<=delta)
		{
			if(!state)
			{
				start = t.getElapsedTimeInMilliSec();
				state = 1;
			}
			else
			{
				curclock = t.getElapsedTimeInMilliSec();
				deltatime= curclock - start;
				
				if( deltatime > time )
				{
					fine=1;
				}
			}
		}
		else
		{
			state=0;
		}
	
	} while(!err && !fine);
	
	FoxPort->RestartLog(); //SMOD250903
	
	if((QHeader.debugMode1 & DEBUG1_WAITENCGR) || ((QHeader.debugMode1 & DEBUG1_WAITENCGR_ONERR) && err))
	{
		char buf[80];
		snprintf( buf, sizeof(buf),WAITENC_GRTIT,punta,endEnc);
	
		C_Graph *graph=new C_Graph(BRUSHSTEADY_POS,buf,GRAPH_NUMTYPEY_INT | GRAPH_NUMTYPEX_INT | GRAPH_AXISTYPE_XY | GRAPH_DRAWTYPE_LINE | GRAPH_SHOWPOSTXT,1);
	
		graph->SetVMinY(miny);
		graph->SetVMaxY(maxy);
		graph->SetVMinX(0);
		graph->SetVMaxX(curclock0-start0);
		graph->SetNData(count,0);
	
		graph->SetDataY(datay,0);
		graph->SetDataX(datax,0);
	
		graph->Show();
	
		int c;

		do
		{
			c=Handle();
			if(c!=K_ESC)
			{
				graph->GestKey(c);
			}
		} while(c!=K_ESC);
	
		delete graph;
	}
	
	centeringMutex.unlock();

	return(!err);
}


//controlla che le punte salendo a laser out scoprano il laser
int CheckNozzlesUp()
{
	if( Get_OnFile() )
		return(1);

	int fine=0;

	do
	{
		PuntaZPosMm(1,QHeader.LaserOut);
		PuntaZPosMm(2,QHeader.LaserOut);
		PuntaZPosWait(1);
		PuntaZPosWait(2);

		#ifdef __SNIPER
		int measure_status1, measure_status2;
		int measure_angle;
		float measure_position;
		float measure_shadow;

		Sniper1->MeasureOnce( measure_status1, measure_angle, measure_position, measure_shadow );
		Sniper1->MeasureOnce( measure_status2, measure_angle, measure_position, measure_shadow );

		if( measure_status1 == STATUS_OK && measure_status2 == STATUS_OK )
		#endif
		{
			char buf[80];
			buf[0]='\1';
			strncpy( buf+1, LASZUP_ERR, sizeof(buf)  );
			
			W_MsgBox* msgbox=new W_MsgBox("",buf,2,MSGBOX_GESTKEY);
		
			msgbox->AddButton(FAST_YES,1);
			msgbox->AddButton(FAST_NO,0);
		
			int msgRet=msgbox->Activate();
		
			delete msgbox;

			if(msgRet==2)
			{
				Set_OnFile(1);
				W_Mess( MsgGetString(Msg_01317) );
				fine=1;
				return 0;
			}
		}
		else
		{
			fine=1;
		}

		PuntaZSecurityPos(1);
		PuntaZSecurityPos(2);
		PuntaZPosWait(1);
		PuntaZPosWait(2);
	} while(!fine);

	return(1);
}

void CheckHeadSensPosChanged(float p,int nozzle)
{
  if( Get_OnFile() )
  {
    return;
  }

  if((nozzle<1) || (nozzle>2))
  {
    return;
  }

  nozzle--;
  
  if((fabs(p-QHeader.DMm_HeadSens[nozzle]))>PUP_POS_LIMIT_CHANGE)
  {
    char buf[160];
    snprintf( buf, sizeof(buf), MsgGetString(Msg_01986), nozzle+1, -QHeader.DMm_HeadSens[nozzle], -p );
    if( W_Deci( 1, buf ) )
    {
      QHeader.DMm_HeadSens[nozzle]=p;
      Mod_Cfg(QHeader);      
    }
  }

  if(nozzle==0)
  {
    QHeader.DStep_HeadSens[0]=int(QHeader.DMm_HeadSens[0]*QHeader.Step_Trasl1);
  }
  else
  {
    QHeader.DStep_HeadSens[1]=int(QHeader.DMm_HeadSens[1]*QHeader.Step_Trasl2);
  }

  Mod_Cfg(QHeader);
  
}

//Controlla perdita passo sulle punte e riazzera se necessario
int CheckNozzleOk(int punta)
{
	int okz[2]={1,1};

	if((punta==0) || (punta==1))
	{
		okz[0]=CheckZAxis(1,ZAXISMSGOFF);
	}
	
	if((punta==0) || (punta==2))
	{
		okz[1]=CheckZAxis(2,ZAXISMSGOFF);
	}

	int resetnozzle=0;

	if(!okz[0] && okz[1]) //punta 2 ok, punta 1 fail
	{
		resetnozzle=1;      //riazzera solo punta 1
	}
	
	if(okz[0] && !okz[1]) //punta 1 ok, punta 2 fail
	{
		resetnozzle=2;      //riazzera solo punta 2
	}

	if(resetnozzle!=0)
	{
		//solo una punta da riazzerare
		
		//punta da non riazzerare fuori da quota laser
		PuntaZPosMm(resetnozzle^3,QHeader.LaserOut,ABS_MOVE);
		PuntaZPosWait(resetnozzle^3);
	
		//riazzeramento punta con sensore
		//CPan *wait=new CPan( -1,1,WAITRESETZ_MSG);
		CheckZAxis(resetnozzle,ZAXISMSGOFF | ZAXISAUTORESET);
	
		int prevuge=Ugelli->GetInUse(resetnozzle);
	
		//deposita ugello
		Ugelli->Depo(resetnozzle);

		InitPuntaUP(resetnozzle);

		//riazzeramento punta con laser
		#ifdef __SNIPER
		Sniper_FindNozzleHeight( resetnozzle );
		#endif
	
		float pos=InitPuntaUP(resetnozzle)-DELTA_HEADSENS;
		CheckHeadSensPosChanged(pos,resetnozzle);
		
		Mod_Cfg(QHeader);
	
		PuntaZSecurityPos(1);
		PuntaZSecurityPos(2);
	
		PuntaZPosWait(1);
		PuntaZPosWait(2);

		//riprende ugello
		if(prevuge!=-1)
		{
			Ugelli->Prel(prevuge,resetnozzle);
		}
	}
	else
	{
		if(!okz[0] && !okz[1])
		{
			//entrambe le punte da riazzerare
		
			//CPan *wait=new CPan( -1,1,WAITRESETZ_MSG);
		
			//riazzera le punte con sensore
			CheckZAxis(1,ZAXISMSGOFF | ZAXISAUTORESET);
			CheckZAxis(2,ZAXISMSGOFF | ZAXISAUTORESET);
		
			int prevuge1=Ugelli->GetInUse(1);
			int prevuge2=Ugelli->GetInUse(2);
		
		
			//deposita ugelli
			Ugelli->DepoAll();
		
			InitPuntaUP(1);
			InitPuntaUP(2);

			//riazzera le punte con laser
			#ifdef __SNIPER
			Sniper_FindNozzleHeight( 1 );
			#endif
			
			float pos=InitPuntaUP(1)-DELTA_HEADSENS;
			CheckHeadSensPosChanged(pos,1);
		
			#ifdef __SNIPER
			Sniper_FindNozzleHeight( 2 );
			#endif

			pos=InitPuntaUP(2)-DELTA_HEADSENS;
			CheckHeadSensPosChanged(pos,2);
		
			PuntaZSecurityPos(1);
			PuntaZSecurityPos(2);
		
			PuntaZPosWait(1);
			PuntaZPosWait(2);
		
			if(prevuge1!=-1)
			{
				Ugelli->Prel(prevuge1,1);
			}
		
			if(prevuge2!=-1)
			{
				Ugelli->Prel(prevuge2,2);
			}
		}
	}

	return 1;
}



//TODO - finire

//----------------------------------------------------------------------------------
// Controlla perdita passo sulla punta
// Ritorna:   1 nessuna perdita di passo  - punta in posizione iniziale
//            0 perdita di passo - punta in quota sensore
//           -1 perdita di passo (riazzerato con sensore) - punta in quota sensore
//           -2 perdita di passo (scelto di non azzerare) - punta in posizione iniziale
//----------------------------------------------------------------------------------
int CheckZAxis_XXX( int nozzle, int& error, int resetOnError )
{
	int startPos = PuntaZPosStep( nozzle, 0, RET_POS );

	float step_const = (nozzle == 1) ? QHeader.Step_Trasl1 : QHeader.Step_Trasl2;
	int sensPos = QHeader.DStep_HeadSens[nozzle-1] + int(DELTA_HEADSENS*step_const);

	CfgUgelli dat;
	if( Ugelli->GetRec( dat, nozzle ) )
	{
		sensPos += int(dat.Z_offset[nozzle-1]*step_const);
	}

	//se posizione corrente sopra sensore e sensore scoperto o
	//se posizione corrente sotto sensore e sensore coperto:
	//porta a quota sensore
	if((startPos<sensPos && !Check_PuntaTraslStatus(nozzle)) || (startPos>=sensPos && Check_PuntaTraslStatus(nozzle)))
	{
		PuntaZPosStep( nozzle, sensPos, ABS_MOVE );
		PuntaZPosWait( nozzle );
	}

	//cerca quota sensore
	InitPuntaUP( nozzle );
	int curPos = PuntaZPosStep( nozzle, 0, RET_POS );

	error = curPos - sensPos;

	if( abs(error) > MAXZERROR )
	{
		if( resetOnError )
		{
			// chiede riazzeramento automatico
			char buf[120];
			snprintf( buf, sizeof(buf), MsgGetString(Msg_01215), nozzle, error );
			if( W_Deci(1, buf) )
			{
				PuntaZPosStep( nozzle, -sensPos, REL_MOVE );
				PuntaZPosWait( nozzle );
				PuntaZPosStep( nozzle, 0, SET_POS );

				FoxHead->SetZero( (nozzle == 1) ? STEP1 : STEP2 );

				PuntaZPosStep( nozzle, 0, ABS_MOVE );
				PuntaZPosWait( nozzle );

				// la punta e' stata azzerata con il sensore
				return -1;
			}

			// riporta punta in posizione iniziale
			PuntaZPosStep( nozzle, startPos, ABS_MOVE );
			PuntaZPosWait( nozzle );
			return -2;
		}

		return 0;
	}

	// riporta punta in posizione iniziale
	PuntaZPosStep( nozzle, startPos, ABS_MOVE );
	PuntaZPosWait( nozzle );
	return 1;
}

//----------------------------------------------------------------------------------
// Azzera la punta con il sensore
//----------------------------------------------------------------------------------
bool ResetNozzleWithSensor( int nozzle )
{
	float step_const = (nozzle == 1) ? QHeader.Step_Trasl1 : QHeader.Step_Trasl2;
	int sensPos = QHeader.DStep_HeadSens[nozzle-1] + int(DELTA_HEADSENS*step_const);

	//riazzeramento punta con sensore
	InitPuntaUP( nozzle );

	PuntaZPosStep( nozzle, -sensPos, REL_MOVE );
	PuntaZPosWait( nozzle );
	PuntaZPosStep( nozzle, 0, SET_POS );

	FoxHead->SetZero( (nozzle == 1) ? STEP1 : STEP2 );

	PuntaZPosStep( nozzle, 0, ABS_MOVE );
	PuntaZPosWait( nozzle );

	return true;
}

//----------------------------------------------------------------------------------
// Controlla perdita passo sulle punte e riazzera se necessario
//----------------------------------------------------------------------------------
int CheckNozzlesLossSteps()
{
	int retval[2];
	int errors[2];
	int prevTools[2];

	prevTools[0] = Ugelli->GetInUse( 1 );
	prevTools[1] = Ugelli->GetInUse( 2 );

	// controlla perdita di passo
	retval[0] = CheckZAxis_XXX( 1, errors[0], true );
	retval[1] = CheckZAxis_XXX( 2, errors[1], true );

	if( (retval[0] == 1 || retval[0] == -2) && (retval[1] == 1 || retval[1] == -2) )
	{
		return 1;
	}

	// in caso di errore il valore viene riportato a 0/1 per comodita'
	retval[0] = ( retval[0] == 1 || retval[0] == -2 ) ? 1 : 0;
	retval[1] = ( retval[1] == 1 || retval[1] == -2 ) ? 1 : 0;

	if( !retval[0] )
	{
		// deposita ugello
		Ugelli->Depo( 1 );

		// alza la punta per azzeramento iniziale
		InitPuntaUP( 1 );
		// riazzeramento punta con sensore di visione
		#ifdef __SNIPER
		Sniper_FindNozzleHeight( 1 );
		#endif

		float pos = InitPuntaUP( 1 ) - DELTA_HEADSENS;
		CheckHeadSensPosChanged( pos, 1 );

		PuntaZSecurityPos( 1 );
		PuntaZPosWait( 1 );
	}
	if( !retval[1] )
	{
		// deposita ugello
		Ugelli->Depo( 2 );

		// alza la punta per azzeramento iniziale
		InitPuntaUP( 2 );
		// riazzeramento punta con sensore di visione
		#ifdef __SNIPER
		Sniper_FindNozzleHeight( 2 );
		#endif

		float pos = InitPuntaUP( 2 ) - DELTA_HEADSENS;
		CheckHeadSensPosChanged( pos, 2 );

		PuntaZSecurityPos( 2 );
		PuntaZPosWait( 2 );
	}

	// preleva ugelli
	if( !retval[0] && prevTools[0] != -1 )
	{
		Ugelli->Prel( prevTools[0], 1 );
	}
	if( !retval[1] && prevTools[1] != -1 )
	{
		Ugelli->Prel( prevTools[1], 2 );
	}

	return 1;
}

/*---------------------------------------------------------------------------------
  Attiva o disattiva l'allarme
  Parametri di ingresso:
    mode : ALARM_ENABLED  attiva il controllo di allarme
           ALARM_DISABLED disattiva il controllo di allarme

-----------------------------------------------------------------------------------*/
void SetAlarmMode(int mode)
{
  if((Get_OnFile() || QParam.DemoMode) || !(QHeader.modal & ENABLE_ALARMSIGNAL))
  {
    return;
  }

  alarm_enabled=mode;

  if(!mode)
  {
    StopAlarm(GENERIC_ALARM);
  }
}

/*---------------------------------------------------------------------------------
  Ritorna il flag di abilitazione controllo di allarme
  Parametri di ingresso:
    nessuno

-----------------------------------------------------------------------------------*/
int GetAlarmMode(void)
{
  return(alarm_enabled);
}


/*---------------------------------------------------------------------------------

  Aziona allarme

-----------------------------------------------------------------------------------*/
void StartAlarm(int alarm)
{
  if(alarm_enabled)
  {
    if(alarm==GENERIC_ALARM)
    {
      Set_Cmd(3,1);
    }
  }
}

/*---------------------------------------------------------------------------------

  Ferma allarme

-----------------------------------------------------------------------------------*/
void StopAlarm(int alarm)
{
	if( alarm_enabled )
	{
		if( alarm == GENERIC_ALARM )
		{
			Set_Cmd(3,0);
		}
	}
}


int DoWarmUpCycle( CWindow* parent )
{
	WarmUpParams data;
	WarmUpParams_Read(data);

	Set_Finec(ON);

	PuntaXYAccSpeed( data.acceleration, 0 );

	CWindow* Q_WarmingUp = new CWindow( parent );
	Q_WarmingUp->SetStyle( WIN_STYLE_CENTERED );
	Q_WarmingUp->SetClientAreaSize( 49, 10 );
	Q_WarmingUp->SetTitle( MsgGetString(Msg_01903) );
	Q_WarmingUp->Show();

	GUI_Freeze();

	//box
	Q_WarmingUp->DrawPanel( RectI( 3, 4, 42, 5 ) );
	Q_WarmingUp->DrawTextCentered( 6, MsgGetString(Msg_00297), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );

	C_Combo tempCombo( 7, 1, MsgGetString(Msg_01897), 6, CELL_TYPE_TEXT );
	C_Combo timeCombo( 7, 2, MsgGetString(Msg_01904), 6, CELL_TYPE_TEXT );

	tempCombo.Show( Q_WarmingUp->GetX(), Q_WarmingUp->GetY() );
	timeCombo.Show( Q_WarmingUp->GetX(), Q_WarmingUp->GetY() );

	int sec_left = data.duration * 60;
	char buf[12];
	snprintf( buf, sizeof(buf), "%02d:%02d", sec_left/60, sec_left%60 );
	timeCombo.SetTxt( buf );

	GUI_Thaw();

	time_t start, end;
	time( &start );

	int _sec = 0;

	while( 1 )
	{
		float temp;
		GetMachineTemperature( temp );

		Wait_PuntaXY();
		NozzleXYMove( 0, 0 );
		Wait_PuntaXY();
		NozzleXYMove( QParam.X_endbrd, QParam.Y_endbrd );

		time( &end );

		sec_left = int(data.duration*60-difftime(end,start));

		if( sec_left != _sec )
		{
			// aggiorna combo boxes
			GUI_Freeze();
			tempCombo.SetTxt( int(temp) );

			snprintf( buf, sizeof(buf), "%02d:%02d", sec_left/60, sec_left%60 );
			timeCombo.SetTxt( buf );
			GUI_Thaw();

			_sec = sec_left;
		}

		if( difftime(end,start) > (data.duration*60) )
		{
			break;
		}

		if( keyRead() == K_ESC )
		{
			break;
		}

		//TODO: sfruttare waitXY
		/*
		if(!Check_Finec(FINEC_BOTH))
		{
			break;
		}
		*/
	}

	delete Q_WarmingUp;

	Wait_PuntaXY();
	NozzleXYMove( 0, 0 );
	Wait_PuntaXY();
	
	Set_Finec(OFF);
	
	return 1;
}

void CheckWarmUp(void)
{
	float temp;
	GetMachineTemperature( temp );

	WarmUpParams data;
	WarmUpParams_Read(data);

	if(!data.enable)
	{
		return;
	}

	if(temp<data.threshold)
	{
		if(W_Deci(1,ASK_DOWARMUP))
		{
			DoWarmUpCycle( 0 );
		}
	}
}

float GetZCaricPos(int nozzle)
{
	float f=QHeader.Zero_Caric;
	#ifdef __SNIPER
	if(nozzle==2)
	{
		f+=QHeader.Z12_Zero_delta;
	}
	#endif
	return(f);
}

float GetPianoZPos(int nozzle)
{
	float p=QHeader.Zero_Piano;
	#ifdef __SNIPER
	if(nozzle==2)
	{
		p+=QHeader.Z12_Zero_delta;
	}
	#endif
	return(p);
}

int DoZStepLossCheck1(void)
{
	if(!isComponentOnNozzleTooBig(1))
	{
		PuntaZPosWait(1);
		CheckZAxis(1);
	}
	else
	{
		W_Mess( MsgGetString(Msg_05074) );
	}
	return 1;
}

int DoZStepLossCheck2(void)
{
	if(!isComponentOnNozzleTooBig(2))
	{
		PuntaZPosWait(2);
		CheckZAxis(2);
	}
	else
	{
		W_Mess( MsgGetString(Msg_05074) );
	}
	return 1;
}

void DoZCheckStepLossShortCut(void)
{
	if(autoapp_running)
	{
		bipbip();
		return;
	}

	if(Q_ShortCutsMenu!=NULL)
	{
		bipbip();
		return;
	}

	Q_ShortCutsMenu = new GUI_SubMenu(MENU_ZLOSSCHECK_SHORTCUT_POS);
	Q_ShortCutsMenu->Add(MENU_ZLOSSCHECK_SHORTCUT_TXT1,999,9,NULL,DoZStepLossCheck1);
	Q_ShortCutsMenu->Add(MENU_ZLOSSCHECK_SHORTCUT_TXT2,999,9,NULL,DoZStepLossCheck2);

	Q_ShortCutsMenu->Show();

	delete Q_ShortCutsMenu;
	Q_ShortCutsMenu = NULL;
}

int DoZCheck1(void)
{
	if(!isComponentOnNozzleTooBig(1))
	{
		doZCheck(1);
	}
	else
	{
		W_Mess( MsgGetString(Msg_05074) );
	}
	return 1;
}

int DoZCheck2(void)
{
	if(!isComponentOnNozzleTooBig(2))
	{
		doZCheck(2);	
	}
	else
	{
		W_Mess( MsgGetString(Msg_05074) );
	}
	return 1;
}

void DoZCheckShortCut(void)
{
	if(autoapp_running)
	{
		bipbip();
		return;
	}
	
	if(Q_ShortCutsMenu!=NULL)
	{
		bipbip();
		return;
	}

	Q_ShortCutsMenu = new GUI_SubMenu(MENU_ZCHECK_SHORTCUT_POS);
	Q_ShortCutsMenu->Add(MENU_ZCHECK_SHORTCUT_TXT1,999,9,NULL,DoZCheck1);
	Q_ShortCutsMenu->Add(MENU_ZCHECK_SHORTCUT_TXT2,999,9,NULL,DoZCheck2);

	Q_ShortCutsMenu->Show();

	delete Q_ShortCutsMenu;
	Q_ShortCutsMenu = NULL;
}


float GetCCal(int punta,float z)
{
	float ccal = MapTeste.ccal_z_cal_m[punta-1] * z + MapTeste.ccal_z_cal_q[punta-1];

	return ccal;
}

float GetCCal(int punta)
{
	return GetCCal(punta,GetPhysZPosMm(punta));
}


#ifdef __SNIPER
float GetZRotationPointForPackage( const SPackageData& pack )
{
	float package_diagonal = sqrt(pack.x * pack.x + pack.y * pack.y);
	
	if(package_diagonal <= CCenteringReservedParameters::inst().getData().opt_cent_d1) 
	{
		return CCenteringReservedParameters::inst().getData().opt_cent_z1 - pack.z;
	}
	else
	{
		if(package_diagonal <= CCenteringReservedParameters::inst().getData().opt_cent_d2)
		{
			return CCenteringReservedParameters::inst().getData().opt_cent_z2;
		}
		else
		{
			return CCenteringReservedParameters::inst().getData().opt_cent_zrot;
		}
	}
}
#endif


bool isComponentOnNozzleTooBig( unsigned int nozzle )
{
	if( PackageOnNozzleFlag[nozzle-1] )
	{
		float package_diagonal = sqrt(PackageOnNozzle[nozzle].x * PackageOnNozzle[nozzle].x + PackageOnNozzle[nozzle].y * PackageOnNozzle[nozzle].y);
		if(package_diagonal <= CCenteringReservedParameters::inst().getData().opt_cent_d1)		
		{
			return false;
		}
		else
		{
			return true;
		}
	}	
	else
	{
		return false;
	}
}

bool isCurrentZPosiziontOkForRotation(unsigned int nozzle)
{
	bool is_current_zposition_ok_for_rotation = true;
	
	if(PackageOnNozzleFlag[nozzle-1])
	{
		const SPackageData& pack = PackageOnNozzle[nozzle-1];
			
		float package_diagonal = sqrt(pack.x * pack.x + pack.y * pack.y);
		float zcurrent = PuntaZPosMm(nozzle,0,RET_POS);
					
		if( package_diagonal <= CCenteringReservedParameters::inst().getData().opt_cent_d1 )
		{		
			/*if(zcurrent < (PACKAGE_ROTATION_ZPOS1 - pack.zdim))
			{
				is_current_zposition_ok_for_rotation =  false;
			}*/
			//per componenti "piccoli" ogni quota e' buona per ruotare il componente
			
			is_current_zposition_ok_for_rotation = true;
		}
		else
		{
			if(package_diagonal <= CCenteringReservedParameters::inst().getData().opt_cent_d2)
			{
				// Nota: il delta di 0.05 e' cautelativo perche' nell'ottenere la posizione attuale ci possono essere arrotondamenti al passo
				if(zcurrent < CCenteringReservedParameters::inst().getData().opt_cent_z2 - 0.05) 
				{
					is_current_zposition_ok_for_rotation =  false;
				}
			}
			else
			{
				if(zcurrent < CCenteringReservedParameters::inst().getData().opt_cent_zrot - 0.05)
				{
					is_current_zposition_ok_for_rotation = false;
				}
			}
		}		
	}
	else
	{
		is_current_zposition_ok_for_rotation = true;
	}
	
	return is_current_zposition_ok_for_rotation;
	
}

void PuntaRotDeg_component_safe(unsigned int nozzle,float r,int mode,bool gosafe)
{
	#ifdef __SNIPER
	if(PackageOnNozzleFlag[nozzle-1])
	{
		float rel_move;
		float abs_move;
		
		if(mode == BRUSH_REL)
		{
			rel_move = r;
			abs_move = r + GetPuntaRotDeg( nozzle );
		}
		else if(mode == BRUSH_ABS)
		{
			rel_move = r - GetPuntaRotDeg( nozzle );
			abs_move = r;
		}
		else
		{
			return;
		}
		
		if(rel_move == 0)
		{
			return;
		}
		
		if( fabs(rel_move) > SAFE_COMPONENT_ROTATION_LIMIT )
		{
			if(!isCurrentZPosiziontOkForRotation(nozzle))
			{
				PuntaZPosMm(nozzle,GetZRotationPointForPackage(PackageOnNozzle[nozzle-1]));
				PuntaZPosWait(nozzle);
				PuntaRotDeg(r,nozzle,mode);
			}
			else
			{
				//il componente puo' ruotare liberamente nella posizione in cui si trova
				PuntaRotDeg(r,nozzle,mode);
			}
			
			if(gosafe)
			{
				float zsafe = GetComponentSafeUpPosition(nozzle);
				//float zcurrent = PuntaZPosMm(nozzle,0,RET_POS);
				
				//if(zcurrent > zsafe)
				{
					float required_component_rotation = abs_move - PackageOnNozzlePickedRotation[nozzle-1];
					float temp = fmod(fabs(required_component_rotation),90);
					
					if((temp <= SAFE_COMPONENT_ROTATION_LIMIT) || (temp >= (90 - SAFE_COMPONENT_ROTATION_LIMIT)))
					{
						//la rotazione finale del componente e' tale che il componente puo' essere portato in posizione
						//"safe" al termine della rotazione in modo da minimizzare il rischio di collisione con i componenti
						//gia' assemblati
						while (!Check_PuntaRot(nozzle))
						{
							FoxPort->PauseLog();
						}
						FoxPort->RestartLog();
						PuntaZPosMm(nozzle,zsafe);
						PuntaZPosWait(nozzle);
					}
				}
			}
		}
		else
		{
			//la rotazione richiesta e' abbastanza piccola da poter venire eseguita ovunque
			PuntaRotDeg(r,nozzle,mode);
		}
	}
	else
	{
		//nessun componente sulla punta : nessun vincolo
		PuntaRotDeg(r,nozzle,mode);
	}
	#endif
}


void PuntaRotStep_component_safe(unsigned int nozzle,int r,int mode,bool gosafe)
{
	#ifdef __SNIPER
	if(PackageOnNozzleFlag[nozzle-1])
	{
		int rel_move;
		int abs_move;
		
		if(mode == BRUSH_REL)
		{
			rel_move = r;
			abs_move = r + GetPuntaRotStep( nozzle );
		}
		else
		{
			if(mode == BRUSH_ABS)
			{
				rel_move = r - GetPuntaRotStep( nozzle );
				abs_move = r;
			}
			else
			{
				return;
			}
		}
		
		if(rel_move == 0)
		{
			return;
		}
		
		
		float rel_move_deg = (rel_move * 360.0) / ((nozzle == 1) ? QHeader.Enc_step1 : QHeader.Enc_step2);
		
		if(( rel_move_deg > SAFE_COMPONENT_ROTATION_LIMIT) || (rel_move_deg < -SAFE_COMPONENT_ROTATION_LIMIT))
		{
			if(!isCurrentZPosiziontOkForRotation(nozzle))
			{
				PuntaZPosMm(nozzle,GetZRotationPointForPackage(PackageOnNozzle[nozzle-1]));
				PuntaZPosWait(nozzle);
				PuntaRotStep(r,nozzle,mode);
			}
			else
			{
				//il componente puo' ruotare liberamente nella posizione in cui si trova
				PuntaRotStep(r,nozzle,mode);
			}
			
			if(gosafe)
			{
				float zsafe = GetComponentSafeUpPosition(nozzle);
				//float zcurrent = PuntaZPosMm(nozzle,0,RET_POS);
				
				//if(zcurrent > zsafe)
				{
					float abs_move_deg = (abs_move * 360.0) / ((nozzle == 1) ? QHeader.Enc_step1 : QHeader.Enc_step2);
					float required_component_rotation = (abs_move_deg - PackageOnNozzlePickedRotation[nozzle-1]);
					float temp = fmod(fabs(required_component_rotation),90);
					
					if((temp <= SAFE_COMPONENT_ROTATION_LIMIT) || (temp >= (90 - SAFE_COMPONENT_ROTATION_LIMIT)))
					{
						//la rotazione finale del componente e' tale che il componente puo' essere portato in posizione
						//"safe" al termine della rotazione in modo da minimizzare il rischio di collisione con i componenti
						//gia' assemblati
						while (!Check_PuntaRot(nozzle))
						{
							FoxPort->PauseLog();
						}
						FoxPort->RestartLog();
						PuntaZPosMm(nozzle,zsafe);
						PuntaZPosWait(nozzle);
					}
				}
			}
		}
		else
		{
			PuntaRotStep(r,nozzle,mode);
		}
	}
	#endif
}

void MoveComponentUpToSafePosition(unsigned int nozzle,bool force_movement)
{
	//float zcurrent = PuntaZPosMm(nozzle,0,RET_POS);
	float zsafe = GetComponentSafeUpPosition(nozzle);
	if(force_movement || (PuntaZPosMm(nozzle,0,RET_POS) > zsafe))
	{
		PuntaZPosMm(nozzle,zsafe);
	}
}

float GetComponentSafeUpPosition( const SPackageData& pack, unsigned int nozzle )
{
	float d;
	bool use_diagonal = false;
	
	if(nozzle && (PackageOnNozzleFlag[nozzle-1]))
	{
		float component_rotation = GetPuntaRotDeg( nozzle ) - PackageOnNozzlePickedRotation[nozzle-1];
		float temp = fmod(fabs(component_rotation),90);	
		
		if((temp > SAFE_COMPONENT_ROTATION_LIMIT) && (temp < (90 - SAFE_COMPONENT_ROTATION_LIMIT)))
		{
			//il componente e' ruotato : valuta la posizione z considerando la diagonale del componente
			use_diagonal = true;
		}
	}
	
	if(use_diagonal)
	{
		d = sqrt(pack.x * pack.x + pack.y * pack.y);
	}
	else
	{
		d = MAX( pack.x, pack.y );
	}
	
	float zpos;
	
	if(d <= CCenteringReservedParameters::inst().getData().opt_cent_d1)
	{
		zpos = CCenteringReservedParameters::inst().getData().opt_cent_z1 - pack.z;
	}
	else
	{
		if(d <= CCenteringReservedParameters::inst().getData().opt_cent_d2)
		{
			zpos = CCenteringReservedParameters::inst().getData().opt_cent_z2;
		}
		else
		{
			zpos = CCenteringReservedParameters::inst().getData().opt_cent_z3; 
		}
	}
	

	//DATA LA STRUTTURA DELLA MACCHINA QUESTE CONDIZIONI NON SI VERIFICANO MAI : NEL CASO IN CUI DOVESSERO PRESENTARSI
	//SI PREFERISCE RENDERE EVIDENTE IL PROBLEMA (SEGNALAZIONE DI MOVIMENTO CON PUNTA ABBASSATA O COLLISIONE CON
	//COMPONENTI GIA' ASSEMBLATI
	/*float zpos_phys = GetPhysZPosMm(nozzle,zpos); //converte in posizione fisica (terminale punta)
	
	if(zpos_phys > QHeader.DMm_HeadSens[nozzle-1])
	{
		//la quota richiesta e' piu in basso di quella di sicurezza: non e' possibile scendere sotto questa quota
		//non dovrebbe mai accadere....
		zpos_phys = QHeader.DMm_HeadSens[nozzle-1];
		zpos = GetLogicalZPosMm(nozzle,zpos_phys);
	}
	
	float z_component_bottom = zpos + pack.zdim;
	
	if(z_component_bottom >= zlimit_no_collision)
	{
		//il componente oltrepassa la linea ideale oltre al quale i componenti gia' assemblati possono essere
		//colpiti da quello sulla punta ma per i componenti piu grandi di PACKAGE_SIZE_LIMIT_ZUP1 non ci sono alternative
		//e comunque vi e' una tolleranza per cui si ignora il vincolo di anticollisione quando le dimensioni
		//sono piu grandi di limit dim 1
		
		if(package_max_size <= CCenteringReservedParameters::inst().getData().d1)
		{
			zpos = zlimit_no_collision - pack.zdim;
		}
	}
*/
	
	return zpos;
}


//alza il componente in modo da :
// - essere in quota si sicurezza
// - non far collidere il componente con le strutture meccaniche della testa
// - non far collidere il componente con quelli gia' assemblati
float GetComponentSafeUpPosition(unsigned int nozzle)
{
	float zpos;	
	float zlimit_no_collision = GetPianoZPos(nozzle) - GetMaxPlacedComponentHeight();	
	
	if(!PackageOnNozzleFlag[nozzle-1])
	{	
		zpos = GetLogicalZPosMm(nozzle,QHeader.DMm_HeadSens[nozzle-1]); //posizione ugello corrispondente alla posizione di sicurezza
		if( zpos > zlimit_no_collision)
		{
			//in posizione di sicurezza l'ugello colliderebbe con i componenti gia' assemblati:
			//ritorna la posizione limte per evitare la collisione
			zpos = zlimit_no_collision;
		}		
	}
	else
	{	   
		zpos = GetComponentSafeUpPosition( PackageOnNozzle[nozzle-1], nozzle );
	}
	
	return zpos;		
}

float GetComponentOnNozzleRotation(unsigned int nozzle)
{
	if(PackageOnNozzleFlag[nozzle-1])
	{
		return GetPuntaRotDeg( nozzle ) - PackageOnNozzlePickedRotation[nozzle-1];
	}
	return 0;
}


#define  THETA_LOG_GRAPH_POS 5,3,76,22
#define  MAX_THETA_LOG 2000
#define  THETA_LOG_TITLE ">> Theta log graph <<"

int RotationTest(int punta,float pos,float idle_time)
{
	std::vector<int> datax;
	std::vector<int> datay;
	
	datax.reserve(MAX_THETA_LOG);
	datay.reserve(MAX_THETA_LOG);
	
	int miny;
	int maxy;
	int minx;
	int maxx;
	
	int motor;
		
	if(punta==1)
	{
		motor=BRUSH1;
	}
	else
	{
		motor=BRUSH2;
	}
	
	Wait_EncStop(punta);
	
	FoxPort->PauseLog();
	
	unsigned int prev_time;
	unsigned int start_time;
	unsigned int phase2_start_time;
	unsigned int phase2_index;
	bool first = true;
	bool end = false;
	int state = 0;

	Timer t;
	t.start();
	
	start_time = prev_time = t.getElapsedTimeInMilliSec();

	PuntaRotDeg(pos,punta);
	
	int target_encoder_position = FoxHead->GetLastRotPos(motor);
	
	do
	{
		unsigned int current_time = t.getElapsedTimeInMilliSec();
		
		if((current_time - prev_time) || first)
		{
			datax.push_back(current_time - start_time);
			datay.push_back(FoxHead->ReadBrushPosition(motor));
			prev_time = current_time;
			first = false;
		}
		
		if(state == 0)
		{
			if(Check_PuntaRot(punta))
			{
				state = 1;
				phase2_start_time = current_time - start_time;
				phase2_index= datax.size() - 1;
			}
		}
		else
		{
			if((current_time - start_time - phase2_start_time) > idle_time)
			{
				end = true;
			}
		}
		
	} while(!end);
				
	FoxPort->RestartLog(); //SMOD250903
	//-----------------------------------------------------------------------------------
	for(int i = 0 ; i < datax.size(); i++)
	{
		if(i == 0)
		{
			maxy = miny = datay[0];
		}
		else
		{		
			if(maxy < datay[i])
			{
				maxy = datay[i];
			}
			
			if(miny > datay[i])
			{
				miny = datay[i];
			}
		}
	}
	
	minx = 0;
	maxx = datax[datax.size() - 1];

	C_Graph *graph=new C_Graph(THETA_LOG_GRAPH_POS,THETA_LOG_TITLE,GRAPH_NUMTYPEY_INT | GRAPH_NUMTYPEX_INT | GRAPH_AXISTYPE_XY | GRAPH_DRAWTYPE_LINE | GRAPH_SHOWPOSTXT,1);

	graph->SetVMinY(miny);
	graph->SetVMaxY(maxy);
	graph->SetVMinX(minx);
	graph->SetVMaxX(maxx);
	graph->SetNData(datax.size(),0);

	graph->SetDataY(&datay[0],0);
	graph->SetDataX(&datax[0],0);

	graph->Show();
	
	graph->DrawVerticalLine(phase2_start_time,GUI_color(GR_YELLOW));
	graph->DrawHorizontalLine(target_encoder_position,GUI_color(GR_CYAN));

	int c;

	do
	{
		c=Handle();
		if(c!=K_ESC)
		{
			graph->GestKey(c);
		}
	} while(c!=K_ESC);

	delete graph;
	//-----------------------------------------------------------------------------------
	
	for(int i = phase2_index ; i < datax.size(); i++)
	{
		if(i == phase2_index)
		{
			maxy = miny = datay[i];
		}
		else
		{		
			if(maxy < datay[i])
			{
				maxy = datay[i];
			}
			
			if(miny > datay[i])
			{
				miny = datay[i];
			}
		}
	}
	
	minx = datax[phase2_index];
	maxx = datax[datax.size() - 1];

	graph=new C_Graph(THETA_LOG_GRAPH_POS,THETA_LOG_TITLE,GRAPH_NUMTYPEY_INT | GRAPH_NUMTYPEX_INT | GRAPH_AXISTYPE_XY | GRAPH_DRAWTYPE_LINE | GRAPH_SHOWPOSTXT,1);

	graph->SetVMinY(miny);
	graph->SetVMaxY(maxy);
	graph->SetVMinX(minx);
	graph->SetVMaxX(maxx);
	graph->SetNData(datax.size() - phase2_index,0);

	graph->SetDataY(&datay[phase2_index],0);
	graph->SetDataX(&datax[phase2_index],0);

	graph->Show();
	
	//graph->DrawVerticalLine(phase2_start_time,GUI_color(GR_YELLOW));
	graph->DrawHorizontalLine(target_encoder_position,GUI_color(GR_CYAN));


	do
	{
		c=Handle();
		if(c!=K_ESC)
		{
			graph->GestKey(c);
		}
	} while(c!=K_ESC);

	delete graph;	
	
	
	return(1);
}


float getSniperType1ComponentLimit(unsigned int nsniper,const CenteringReservedParameters& par)
{
	float limit = 2 * (MapTeste.ccal_z_cal_q[nsniper - 1] - par.sniper_type1_components_max_pick_error - par.sniper_type1_safety_margin);
	return limit;
}

float getSniperType2ComponentLimit(unsigned int nsniper,const CenteringReservedParameters& par)
{
	float window_size = QHeader.sniper_kpix_um[nsniper - 1] * SNIPER_RIGHT_USABLE / 1000.0;
	float limit = 2 * (window_size - MapTeste.ccal_z_cal_q[nsniper - 1] - par.sniper_type2_components_max_pick_error - par.sniper_type2_safety_margin);
	return limit;
}


// Gestione autoapprend. posizione scarico componenti. - W09
// Add by Walter 10/97 - W09

#define SCARERR1  MsgGetString(Msg_01720)
#define SCARERR2  MsgGetString(Msg_01721)

int Sca_auto(void)
{
	float c_ax, c_ay;

	int limitX=0;
	int limitY=0;

	c_ax=QParam.PX_scaric;
	c_ay=QParam.PY_scaric;

	if( ManualTeaching(&c_ax,&c_ay,MsgGetString(Msg_00235)) )
	{
		if(c_ax>(QParam.LX_maxcl-QParam.CamPunta2Offset_X))
		{
			limitX=1;
			c_ax=QParam.LX_maxcl-QParam.CamPunta2Offset_X;
		}
		else
		{
			if(c_ax<(QParam.LX_mincl-QParam.CamPunta1Offset_X))
			{
				limitX=1;
				c_ax=QParam.LX_mincl-QParam.CamPunta1Offset_X;
			}
		}

		if(c_ay>(QParam.LY_maxcl-QParam.CamPunta2Offset_Y))
		{
			limitY=1;
			c_ay=QParam.LY_maxcl-QParam.CamPunta2Offset_Y;
		}
		else
		{
			if(c_ay<(QParam.LY_mincl-QParam.CamPunta1Offset_Y))
			{
				limitY=1;
				c_ay=QParam.LY_mincl-QParam.CamPunta1Offset_Y;
			}
		}

		if((limitX==1) && (limitY==1))
		{
			W_Mess(SCARERR2);
		}
		else
		{
			char buf[80];

			if(limitX==1)
			{
				snprintf( buf, sizeof(buf),SCARERR1,'X');
				W_Mess(buf);
			}

			if(limitY==1)
			{
				snprintf( buf, sizeof(buf),SCARERR1,'Y');
				W_Mess(buf);
			}
		}

		QParam.PX_scaric=c_ax;
		QParam.PY_scaric=c_ay;
	}

	return 1;
} // Sca_auto
