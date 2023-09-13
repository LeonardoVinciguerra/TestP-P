/*
>>>> Q_UGEOBJ.CPP

Classi di gestione ugelli

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++    Sviluppo : Simone Navari - L.C.M. 14/03/2002        ++++

  Patch - ##P##SMOD200902 - Quota di prelievo ugelli diversa
                            per le due punte.

*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "filefn.h"
#include "stepmsg.h"
#include "q_cost.h"
#include "q_graph.h"
#include "q_tabe.h"
#include "q_oper.h"
#include "q_opert.h"
#include "q_ugeobj.h"
#include "q_help.h"
#include "q_gener.h"
#include "q_fox.h"

#ifdef __SNIPER
#include "tws_sniper.h"
#endif

#include "q_init.h"
#include "lnxdefs.h"

#include "c_pan.h"

#include <mss.h>


#define TXTUG1 "Quadra Pins File v. 5.10. \n\n\r"

//TEMP //TODO: definite anche in q_conf_new.cpp
#define DELTAUGECHECK_MIN   0.1
#define DELTAUGECHECK_MAX   4.0
#define DELTAUGECHECK_DEF   0.6

extern struct CfgParam QParam;
extern struct CfgHeader QHeader;
extern struct cur_data  CurDat;

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;

// flag per visualizzare messaggio di errore in caso di prelievo
// ugello con sensore pressione aria disattivato
bool pickpressMsg = false;

// Classe globale di gestione degli ugelli
UgelliClass *Ugelli;


/******** FUNZIONI MEMBRO CLASSE UgeClass **************************************/

/*--------------------------------------------------------------------------
Costruttore con parametri della classe UgeClass
INPUT:	inituge: Struttura contenente i parametri dell'ugello
		initpunta: Punta da associare all'ugello (default = -1)
GLOBAL:	-
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
UgeClass::UgeClass(CfgUgelli inituge,int initpunta)
{ 
	uge=inituge;
	ugepunta=initpunta;
	stepF=NULL;
	memset(&val_vuoto,0,1);
}


/*--------------------------------------------------------------------------
Costruttore banale della classe UgeClass
INPUT:	-
GLOBAL:	-
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
UgeClass::UgeClass(void)
{ 
	ugepunta=-1;
	stepF=NULL;
}


/*--------------------------------------------------------------------------
Vai a posizione ugello con punta
INPUT:	punta: Punta da movimentare
RETURN:	0 se movimentazione abbandonata, 1 altrimenti
NOTE:	
--------------------------------------------------------------------------*/
int UgeClass::GoPos(int punta)
{
	int retval=1;
	
	// Punta in posizione di sicurezza
	MoveComponentUpToSafePosition(1);
	MoveComponentUpToSafePosition(2);
	PuntaZPosWait(1);
	PuntaZPosWait(2);
	
	// Check pressione
	PressStatus(1);
	
	// Settaggio parametri di movimentazione punta
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );
	
	// Vai in posizione
	if(punta==1)
	{
		if( !NozzleXYMove(uge.X_ugeP1,uge.Y_ugeP1,AUTOAPP_MAPOFF) )
		{
			return 0;
		}
	}
	else
	{
		if( !NozzleXYMove(uge.X_ugeP2,uge.Y_ugeP2,AUTOAPP_MAPOFF) )
		{
			return 0;
		}
	}
	
	// Se presente, chiama funzione handling passo-passo
	if(stepF!=NULL)
	{
		if(!stepF(S_GOUGEPOS,punta,0))
		{
			retval=0; // Ritorna 0 se abbandono
		}
	}
	
	Wait_PuntaXY();
	
	// Disabilita protezione tramite finecorsa
	//$$$	Set_Finec(OFF);
	
	return retval;
}

/*--------------------------------------------------------------------------
Prelevamento ugello con la punta specificata
INPUT:	x_punta: Punta con cui prelevare l'ugello
RETURN:	0 se prelievo abbandonato, 1 altrimenti
NOTE:	
--------------------------------------------------------------------------*/
int UgeClass::Prel(int x_punta)
{
	int retval=1;

	if(!(uge.NozzleAllowed & x_punta))
	{
		return 0;
	}

	float *thoffset=&(QHeader.thoff_uge1);
	
	// Abilita protezione tramite finecorsa
	int prev_step = GetPuntaRotStep( x_punta );
	
	// Setta vel/acc. di prelievo ugello
	PuntaZSpeed(x_punta,QHeader.uge_zvel);
	PuntaZAcc(x_punta,QHeader.uge_zacc);
	PuntaZStartStop(x_punta,QHeader.uge_zmin);
	
	// ruota la punta per poter prelevare l'ugello
	PuntaRotDeg(thoffset[x_punta-1],x_punta);
	
	// Blocco ugelli = OFF
	Set_UgeBlock(0);

	// Check pressione
	PressStatus(1);
	
	// Porta la punta a posizione ugello
	GoPos(x_punta);
	
	// Attende la fine delle movimentazioni
	Wait_PuntaXY();

	while(!Check_PuntaRot(x_punta))
	{
		FoxPort->PauseLog();
	}
	FoxPort->RestartLog();

	//SMOD070703
	if(!WaitReedUge(1))
		return 0;

	// controllo pressione
	if( !QParam.AT_press && !pickpressMsg )
	{
		pickpressMsg = true;
		if( !W_Deci( 1, MsgGetString(Msg_00071) ) )
		{
			return 0;
		}
	}

	if( Check_Press(1) )
	{
		// Abbassa la punta
		if( x_punta == 1 )
			PuntaZPosMm(1,QHeader.Zero_Ugelli);
		else
			PuntaZPosMm(2,QHeader.Zero_Ugelli2);
	}
	else
	{
		if( !QParam.DemoMode )
		{
			W_Mess( MsgGetString(Msg_00072) );
			return 0;
		}
	}

	// Se presente chiama funzione handling passo-passo
	if(stepF!=NULL)
		if(!stepF(S_PDOWN,x_punta,0))
			retval=0;

	PuntaZPosWait(x_punta);

	// Se presente chiama funzione handling passo-passo
	if((stepF!=NULL) && retval)
	{
		if(!stepF(S_PUP,x_punta,0))
		{
			retval=0;
		}
	}

	if( !CheckUgeUp(x_punta,UGECHECK_PICKUPMODE) )
	{
		retval = 0;
	}

	PuntaZPosMm(x_punta,-uge.Z_offset[x_punta-1]);
	PuntaZPosWait(x_punta);
	
	// Assegna la punta attuale come in uso dall'ugello
	ugepunta = x_punta;
	
	//Porta la punta a posizione angolare precedente all'operazione
	PuntaRotStep(prev_step,x_punta);
	
	return retval;
} // UgePrel


//check perdita passo con laser e sensore di sicurezza
int UgeClass::CheckUgeUp(int x_punta,int mode)
{
	#ifdef HWTEST_RELEASE
	return 1;
	#else
	if(Get_OnFile())
	{
		return 1;
	}
	
	if(QParam.DemoMode || (QHeader.debugMode2 & DEBUG2_UGECHECKOFF))
	{
		return 1;
	}

	if((QParam.DeltaUgeCheck<DELTAUGECHECK_MIN) || (QParam.DeltaUgeCheck>DELTAUGECHECK_MAX))
	{
		QParam.DeltaUgeCheck=DELTAUGECHECK_DEF;
		Mod_Par(QParam);
	}

	PuntaZPosMm(x_punta,QHeader.DMm_HeadSens[x_punta-1]+2*DELTA_HEADSENS);
	PuntaZPosWait(x_punta);
	
	if(mode==UGECHECK_PICKUPMODE)
	{
		Set_Vacuo(x_punta,1);
	}
	
	int okStep=0;
	int okLaser=0;
	int retval=1;
	
	if(Check_PuntaTraslStatus(x_punta))
	{
		PuntaZPosMm(x_punta,QHeader.DMm_HeadSens[x_punta-1]);
		PuntaZPosWait(x_punta);
	
		if(!Check_PuntaTraslStatus(x_punta))
		{
			okStep=1;
		}
	}

	if(!okStep)
	{
		bipbip();
	
		if(!CheckZAxis(x_punta,ZAXISNOAUTORESET | ZAXISMSGOFF))
		{
			char buf[160];
			snprintf( buf, sizeof(buf), UGECHECK_STEP_LOSS,x_punta,GetZCheck_LastErr(x_punta) );

			Set_Vacuo(x_punta,0);

			bipbip();

			if( W_Deci(1,buf) )
			{
				Set_OnFile(1);
				W_Mess( MsgGetString(Msg_01317) );
				retval = 0;
			}
		}
	}
	else
	{
		#ifdef __SNIPER
		SniperModule* sniper = (x_punta == 1) ? Sniper1 : Sniper2;
	
		int measure_status;
		int measure_angle;
		float measure_position;
		float measure_shadow;

		if(mode==UGECHECK_PICKUPMODE)
		{
			PuntaZPosMm(x_punta,-uge.Z_offset[x_punta-1]+QParam.DeltaUgeCheck);
			PuntaZPosWait(x_punta);

			sniper->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow );   // Misura laser

			if( measure_status != STATUS_EMPTY ) // Read data
			{
				okLaser++;
		
				PuntaZPosMm(x_punta,-uge.Z_offset[x_punta-1]-QParam.DeltaUgeCheck);
				PuntaZPosWait(x_punta);
		
				sniper->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow );   // Misura laser
		
				if( measure_status == STATUS_EMPTY ) // Read data
				{
					okLaser++;
				}
       		}
     	}
		else
		{
			PuntaZPosMm(x_punta,-QParam.DeltaUgeCheck);
			PuntaZPosWait(x_punta);
		
			sniper->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow );   // Misura laser
		
			if( measure_status == STATUS_EMPTY )
			{
				okLaser=1;
			}
		}
    	#endif

		char buf[160];
		buf[0] = '\0';

		switch(okLaser)
		{
			case 0:
				if(mode==UGECHECK_PICKUPMODE)
				{
					snprintf( buf, sizeof(buf), UGECHECK_PICKUP_FAIL,x_punta,UGECHECK_PICKUP_NOTFOUND);
				}
				else
				{
					snprintf( buf, sizeof(buf), UGECHECK_PLACE_FAIL,x_punta);
				}
				break;
			case 1:
				if(mode==UGECHECK_PICKUPMODE)
				{
					snprintf( buf, sizeof(buf), UGECHECK_PICKUP_FAIL,x_punta,UGECHECK_PICKUP_INS);
				}
				break;
		}

		if(*buf!='\0')
		{
			Set_Vacuo(x_punta,0);
		
			bipbip();

			if( !W_Deci(0,buf) )
			{
				/*
				Set_OnFile(1);
				W_Mess(MOVDISABLED);
				*/
				retval = 0;
			}
		}
	}
	
	Set_Vacuo(x_punta,0);
	
	PuntaZPosMm(x_punta,-uge.Z_offset[x_punta-1]);
	PuntaZPosWait(x_punta);
	
	return retval;
	#endif //HWTEST_RELEASE
}


/*--------------------------------------------------------------------------
Gestione deposito ugello
INPUT:	-
RETURN:	0 se deposito abbandonato, 1 altrimenti
NOTE:	Ritorna 1 anche se l'ugello non appartiene a nessuna punta!
--------------------------------------------------------------------------*/
int UgeClass::Depo(void)
{
	if( Get_OnFile() )
		return 1;
	
	int retval=1,tmp_punta,prev_step;
	float *thoffset=&(QHeader.thoff_uge1);
	float zeropos;

	if( ugepunta == -1 )
	{
		return(1);
	}

	if(!(uge.NozzleAllowed & ugepunta))
	{
		return(1);
	}
	
	prev_step = GetPuntaRotStep( ugepunta );

	// ruota la punta per poter depositare l'ugello
	PuntaRotDeg(thoffset[ugepunta-1],ugepunta);

	//##P##SMOD200902
	if(ugepunta==1)
	{
		zeropos=QHeader.Zero_Ugelli;
	}
	else
	{
		zeropos=QHeader.Zero_Ugelli2;
	}

	// Settaggio velocita' e accelerazione punta
	PuntaZSpeed(ugepunta,QHeader.uge_zvel);
	PuntaZAcc(ugepunta,QHeader.uge_zacc);
	PuntaZStartStop(ugepunta,QHeader.uge_zmin);

	// Porta la punta a posizione ugello
	GoPos(ugepunta);

	// Blocco ugelli = OFF
	Set_UgeBlock(0);


	if(!WaitReedUge(1))
	{
		return(1);
	}

	FoxPort->flag=1;
	
	// Attesa termine amovimentazioni
	Wait_PuntaXY();

	//SMOD250903
	while(!Check_PuntaRot(ugepunta))
	{
		FoxPort->PauseLog();
	}
	FoxPort->RestartLog();


	// controllo pressione
	if( !QParam.AT_press && !pickpressMsg )
	{
		pickpressMsg = true;
		if( !W_Deci( 1, MsgGetString(Msg_00071) ) )
		{
			return 0;
		}
	}

	if( Check_Press(1) )
	{
		// Abbassa la punta
		PuntaZPosMm( ugepunta, zeropos+uge.Z_offset[ugepunta-1] );
	}
	else
	{
		if( !QParam.DemoMode )
		{
			W_Mess( MsgGetString(Msg_00072) );
			return 0;
		}
	}


	// Se presente chiama funzione handling passo-passo
	if(stepF!=NULL)
	{
		if(!stepF(S_PDOWN,ugepunta,0))
		{
			//Porta la punta a posizione angolare precedente all'operazione
			PuntaRotStep(prev_step,ugepunta);
			return(0); // Ritorna 0 se abbandono
		}
	}

	PuntaZPosWait(ugepunta);
	
	// Attiva blocco ugelli

	Set_UgeBlock(1);
	
	if(stepF!=NULL)
	{
		if(!stepF(S_UGEBLK_ON,ugepunta,0)) // Se abbandono richiesto
		{
			//Porta la punta a posizione angolare precedente all'operazione
			PuntaRotStep(prev_step,ugepunta);

			Set_UgeBlock(0);                // Disattiva blocco ugelli
			retval=0;
		}
	}
		
	// ###  t->Reset();
	// ###  while(!t->Status());


	if(!WaitReedUge(0))
	{
		return(1);
	}

	// Attesa cambio ugelli
	delay(QHeader.uge_wait);
	
	FoxPort->flag=0;
	
	// Se abbandono richiesto...
	if(!retval)
	{
	   // Riporta ugello su zero laser
		PuntaZPosMm(ugepunta,0);   
		// Attende fine movimentazione
		PuntaZPosWait(ugepunta);

		//Porta la punta a posizione angolare precedente all'operazione
		PuntaRotStep(prev_step,ugepunta);

		return(0);
	}

	//##SMOD200902
	
	tmp_punta=ugepunta;
	
	// Setta ugello non piu su punta
	ugepunta=-1;

	bool checkTool = true;
	if( !CheckUgeUp(tmp_punta,UGECHECK_PLACEMODE) )
	{
		checkTool = false;
	}

	//porta punta su
	PuntaZSecurityPos(tmp_punta);
	PuntaZPosWait(tmp_punta);


	if( !checkTool )
	{
		//Porta la punta a posizione angolare precedente all'operazione
		PuntaRotStep(prev_step,tmp_punta);
		return 0; // Ritorna 0 se abbandono
	}

	// Se presente chiama funzione handling passo-passo
	if( stepF )
	{
		if( !stepF(S_PUP,tmp_punta,0) )
		{
			//Porta la punta a posizione angolare precedente all'operazione
			PuntaRotStep(prev_step,tmp_punta);
			return 0; // Ritorna 0 se abbandono
		}
	}

	// Porta la punta a zero theta
	PuntaRotDeg(0,tmp_punta);

	// Disattiva blocco ugelli
	Set_UgeBlock(0);

	// Se presente funzione handling passo passo
	if( stepF )
	{
		if(!stepF(S_UGEBLK_OFF,tmp_punta,0))
		{
			//Porta la punta a posizione angolare precedente all'operazione
			PuntaRotStep(prev_step,tmp_punta);
			return 0; // Ritorna 0 se abbandono
		}
	}

	//Porta la punta a posizione angolare precedente all'operazione
	PuntaRotStep(prev_step,tmp_punta);

	// Setta vel./accel. asse z a max
	SetNozzleZSpeed_Index( tmp_punta, ACC_SPEED_DEFAULT );
	
	return 1;
} // UgeDepo

/*--------------------------------------------------------------------------
Set del livello di vuoto ad ugello aperto
INPUT:	-
RETURN:  valore misurato e settato
NOTE:    -
--------------------------------------------------------------------------*/
int UgeClass::Set_VacuoLevel(void)
{  if(uge.MinVLevel==NO_VLEVELSET)
   { Set_Vacuo(ugepunta,1);
   delay(100);
     uge.MinVLevel=Get_Vacuo(ugepunta);
     Set_Vacuo(ugepunta,0);
     return(uge.MinVLevel);
   }
   else
     return(-1);
}

/*--------------------------------------------------------------------------
Set del livello di vuoto a valore indicato
INPUT:	val : valore da impostare
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void UgeClass::Set_VacuoLevel(int val)
{
  uge.MinVLevel=val;
}

/*--------------------------------------------------------------------------
Set soglia vuoto
INPUT:	val : soglia da impostare
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void UgeClass::Set_SogliaVacuo(int val)
{
   short int *max=&(QParam.vuoto_1);

   if(val<1 || val>9)  //se soglia out of range
     val=5;            //soglia=mid-range

   val_vuoto.max_vuoto=max[ugepunta-1];
   val_vuoto.min_vuoto=uge.MinVLevel;
   val_vuoto.sog_vuoto=val_vuoto.min_vuoto+(val_vuoto.max_vuoto-val_vuoto.min_vuoto)*val/10;
}

/*--------------------------------------------------------------------------
Check soglia vuoto
INPUT:   -
RETURN:  0 se la soglia non e' superata (ugello libero). 1 altrimenti
NOTE:    -
--------------------------------------------------------------------------*/
int UgeClass::Check_SogliaVacuo(void)
{
  if(QParam.DemoMode)
    return(1);

  int retry=0;

  do
  {
	val_vuoto.liv_vuoto = Get_Vacuo(ugepunta);

    if(val_vuoto.liv_vuoto >= val_vuoto.sog_vuoto)
    {
      return(1);
    }

    retry++;
    
  } while(retry<10);

  return(0);
    
}


/*--------------------------------------------------------------------------
Autoapprende posizione ugello.
INPUT:	-
GLOBAL:	QParam
RETURN:	1 se autoapprendimento andato a buon fine, 0 altrimenti
NOTE:	Non aggiorna su file !!
--------------------------------------------------------------------------*/
//DANY181202
int UgeClass::AutoApp(int punta)
{
	float c_ax, c_ay;
	int ritorno;
	
	// Punte in pos di sicurezza
	PuntaZSecurityPos(1);
	PuntaZSecurityPos(2);
	PuntaZPosWait(2);
	PuntaZPosWait(1);
	
	if((punta==1) && (uge.NozzleAllowed==UG_P2))
	{
		return(1);
	}
	
	if((punta==2) && (uge.NozzleAllowed==UG_P1))
	{
		return(1);
	}


	if(punta==2)
	{
		// ruota la punta per poter depositare l'ugello
		PuntaRotDeg(QHeader.thoff_uge2,punta);
	
		c_ax=uge.X_ugeP2;
		c_ay=uge.Y_ugeP2;
	}
	else
	{
		// ruota la punta per poter depositare l'ugello
		PuntaRotDeg(QHeader.thoff_uge1,punta);
	
		c_ax=uge.X_ugeP1;
		c_ay=uge.Y_ugeP1;
	}

	// Visualizza il pannello di informazioni dei dati dell'ugello
	char buf[80];
	snprintf( buf, 80, MsgGetString(Msg_01044), punta == 1 ? '1' : '2', uge.U_code );
	CPan* tips = new CPan( 22, 1, buf );

	// Attiva il blocco ugelli
	Set_UgeBlock(1);

	//SMOD250903
	while(!Check_PuntaRot(punta))
	{
		FoxPort->PauseLog();
	}
	FoxPort->RestartLog();

	int mode = AUTOAPP_NOCAM | AUTOAPP_UGELLO;
	mode |= (punta == 1) ? AUTOAPP_PUNTA1ON : AUTOAPP_PUNTA2ON;

	// Autoappr. posizione
	if( ManualTeaching(&c_ax,&c_ay, MsgGetString(Msg_00031), mode) )
	{ 
		// Se OK, aggiorna parametri ugello
		if(punta==2)
		{
			uge.X_ugeP2=c_ax;
			uge.Y_ugeP2=c_ay;
		}
		else
		{
			uge.X_ugeP1=c_ax;
			uge.Y_ugeP1=c_ay;
		}

		ritorno=1;
	}
	else
	{ 
		ritorno=0;
	}
	
	// Nasconde il pannello di informazioni dei dati dell'ugello
	delete tips;

	// Disattiva il blocco ugelli
	Set_UgeBlock(0);
	
	return(ritorno);
}


/*--------------------------------------------------------------------------
Check se un ugello e' su una punta 
INPUT:	-
GLOBAL:	-
RETURN:	1 se e' presente, 0 altrimenti
NOTE:	
--------------------------------------------------------------------------*/
int UgeClass::IsOnNozzle(void)
{
	if(ugepunta!=-1)
		return 1;
	else
		return 0;
}


/*--------------------------------------------------------------------------
Ritorna codice ugello
INPUT:	-
GLOBAL:	-
RETURN:	Codice dell'ugello.
NOTE:	
--------------------------------------------------------------------------*/
int UgeClass::Code(void)
{  return uge.U_code;
}


/*--------------------------------------------------------------------------
Setta struttura dati ugello e, se indicato, forza valore della punta su cui si trova
INPUT:	inituge: Valore della struttura dell'ugello
		initpunta: Punta da associare all'ugello (default = -1)
GLOBAL:	-
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
void UgeClass::SetRec(CfgUgelli inituge,int initpunta)
{ 
	uge=inituge;
	
	if(initpunta!=-1)
		ugepunta=initpunta;
}


/*--------------------------------------------------------------------------
Ritorna struttura dati dell'ugello
INPUT:	-
GLOBAL:	-
RETURN:	Struttura dati dell'ugello in retuge
NOTE:	
--------------------------------------------------------------------------*/
void UgeClass::GetRec(CfgUgelli *retuge)
{ 
	memcpy(retuge,&uge,sizeof(CfgUgelli));
}


/*--------------------------------------------------------------------------
Setta handler funzione passo-passo
INPUT:	_stepF: Funzione di handling della movimentazione passo-passo
GLOBAL:	-
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
void UgeClass::Set_StepF(int(*_stepF)(const char *,int,int))
{ 
	stepF=_stepF;
}


/*--------------------------------------------------------------------------
Overloading dell'operatore di assegnamento(=) di due classi UgeClass
INPUT:	dat: Classe da copiare
GLOBAL:	-
RETURN:	Classe da copiare
NOTE:	
--------------------------------------------------------------------------*/
const UgeClass& UgeClass::operator=(const UgeClass &dat)
{ 
	memcpy(&uge,&dat.uge,sizeof(uge));

	return(dat);
}



/******** FUNZIONI MEMBRO CLASSE UgelliClass **************************************/

/*--------------------------------------------------------------------------
Costruttore della classe UgelliClass
INPUT:	-
GLOBAL:	ufile
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
UgelliClass::UgelliClass(void)
{ 
	cur_uge=new UgeClass[2]();
	opened=0;
	FileOffset=0;
	mount_flag=new int[2];
	mount_flag[0]=1;
	mount_flag[1]=1;
	FileHandle=0;
	//ufile=Fopen("uge.log","wr");
	stepF=NULL;
}

/*--------------------------------------------------------------------------
Distruttore della classe UgelliClass
INPUT:	-
GLOBAL:	ufile
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
UgelliClass::~UgelliClass(void)
{
   CPan *wait=new CPan( -1,1,MsgGetString(Msg_00330));

   DepoAll();

   delete wait;

	delete[] mount_flag;
	delete[] cur_uge;
	
	if(opened)
	  FilesFunc_close(FileHandle);
	
	//Fclose(ufile);
}


/*--------------------------------------------------------------------------
Aggiorna la struttura contenente gli id degli ugelli in uso
--------------------------------------------------------------------------*/
void UgelliClass::UpdateUgeUse()
{
	// Guarda se c'e' un ugello montato sulla punta 1
	if(cur_uge[0].IsOnNozzle())
		CurDat.U_p1=cur_uge[0].Code();
	else
		CurDat.U_p1=-1;
	
	// Guarda se c'e' un ugello montato sulla punta 2
	if(cur_uge[1].IsOnNozzle())
		CurDat.U_p2=cur_uge[1].Code();
	else
		CurDat.U_p2=-1;
	
	// Modifica i dati nel file di memo ugelli in uso
   CurData_Write(CurDat);
}


/*--------------------------------------------------------------------------
Interfaccia prelievo ugello
INPUT:	uge: Ugello da prelevare
		punta: Punta con cui prelevare l'ugello
RETURN:	0 se prelievo abbandonato, 1 altrimenti
--------------------------------------------------------------------------*/
int UgelliClass::Prel(int uge,int punta)
{
	// Legge il record relativo all'ugello passato
	CfgUgelli rec;
	ReadRec(rec,uge-'A');

	// Assegna i dati alla classe di gestione dell'ugello
	cur_uge[punta-1].SetRec(rec);

	// Attiva il prelievo
	int retval = cur_uge[punta-1].Prel(punta);	// DANY050902 Aggiunto il ritorno a retval

	if(cur_uge[punta-1].IsOnNozzle())      //se ugello preso
		Set_VacuoLevel(punta);               //setta livello di vuoto ad uge. libero (se non settato)

	return(retval);
}


/*--------------------------------------------------------------------------
Interfaccia deposito ugello
INPUT:	punta: Punta da cui depositare l'ugello
RETURN:	0 se deposito abbandonato, 1 altrimenti
--------------------------------------------------------------------------*/
int UgelliClass::Depo(int punta)
{
	if( cur_uge[punta-1].IsOnNozzle() )
	{
		return cur_uge[punta-1].Depo(); //deposita
	}
	return 1;
}


/*--------------------------------------------------------------------------
Gestione cambio ugello
INPUT:	uge: Ugello da prelevare
		punta: Punta su cui cambiare l'ugello
RETURN:	1 se cambio avvenuto, 0 altrimenti
--------------------------------------------------------------------------*/
int UgelliClass::Change(int uge,int punta)
{
	int retval = 0;
	int other_punta = punta ^ 3;
	
	// Deposita l'ugello eventualmente presente sulla punta
	if( (retval = Depo(punta)) )
	{
		// Se l'ugello da prelevare si trova sull'altra punta, depositalo
		if(cur_uge[other_punta-1].Code()==uge)
		  retval=Depo(other_punta);
		
		// Preleva il nuovo ugello
		if(retval)
		  retval=Prel(uge,punta);
	}
	
	// Setta vel/acc. punta a max
	SetNozzleZSpeed_Index( punta+1, ACC_SPEED_DEFAULT );

	// Aggiorna la struttura degli ugelli in uso
	UpdateUgeUse();
	return retval;
}


/*--------------------------------------------------------------------------
Deposita entrambi gli ugelli (se presenti sulle due punte)
--------------------------------------------------------------------------*/
void UgelliClass::DepoAll()
{ 
   if( Get_OnFile() )
   {
	   return;
   }

	cur_uge[0].Depo();
	cur_uge[1].Depo();
}


/*--------------------------------------------------------------------------
Gestione completa ugelli.
INPUT:	n_ugello: Ugelli richiesti per le due punte
         s_vacuo : livelli di soglia per i due ugelli
RETURN:	1 se operazione avvenuta correttamente, 0 altrimenti
--------------------------------------------------------------------------*/
int UgelliClass::DoOper(int *n_ugello)
{
	int code_cur;
	int retval = 1;
	int *o_ugello=new int[2];
	int *tmp_ptr,*tmp_mflag;
	
	tmp_ptr=o_ugello;
	tmp_mflag=mount_flag;

	o_ugello[0]=CurDat.U_p1;
	o_ugello[1]=CurDat.U_p2;

	// Per ogni punta...
	for( int i = 0; i < 2; i++ )
	{
		// Se non si devono svolgere operazioni sulla punta attuale...
		if(!(*mount_flag))
		{ 
			o_ugello++;
			n_ugello++;
			mount_flag++;
			continue;
		}

		if( cur_uge[i].IsOnNozzle() )
			code_cur=cur_uge[i].Code();
		else
			code_cur=-1;

		// Se l'ugello non e' sulla punta...
		if( *n_ugello != code_cur )
		{
			retval = Change(*n_ugello,i+1); //esegui cambio/prelievo ugello
			if( !retval )
			{
				char old_tool = *o_ugello;
				char new_tool = *n_ugello;

				if( old_tool < 'A' || old_tool > 'Z' )
					old_tool = '*';
				if( new_tool < 'A' || new_tool > 'Z' )
					new_tool = '*';

				char _msg[128];
				snprintf( _msg, sizeof(_msg), MsgGetString(Msg_00117), i+1, old_tool, new_tool );
				W_Mess( _msg );
				break;
			}
		}
		
		o_ugello++;
		n_ugello++;
		mount_flag++;
	}

	// Setta vel/acc. punta a max per le due punte
	SetNozzleZSpeed_Index( 1, ACC_SPEED_DEFAULT );
	SetNozzleZSpeed_Index( 2, ACC_SPEED_DEFAULT );

	UpdateUgeUse();

	mount_flag = tmp_mflag;

	delete [] tmp_ptr;

	return retval;
}


/*--------------------------------------------------------------------------
Ritorna il codice dell'ugello in uso per la punta indicata
INPUT:	punta: Punta da cui ottenere il codice ugello
GLOBAL:	-
RETURN:	Codice ugello in uso sulla punta indicata. -1 se nessuno
NOTE:	
--------------------------------------------------------------------------*/
int UgelliClass::GetInUse(int punta)
{ 
	if(cur_uge[punta-1].IsOnNozzle())
	{ 
		return(cur_uge[punta-1].Code());
	}
	return -1;
}

/*--------------------------------------------------------------------------
Set soglia vuoto
INPUT:	val : soglia da impostare
RETURN:  -
NOTE:    chiamare solo dopo aver settato il liv. di vuoto ad uge. libero
--------------------------------------------------------------------------*/
void UgelliClass::Set_SogliaVacuo(int val,int punta)
{
	if(cur_uge[punta-1].IsOnNozzle())
	{
		cur_uge[punta-1].Set_SogliaVacuo(val);
	}
}

/*--------------------------------------------------------------------------
Check soglia vuoto
INPUT:   -
RETURN:  0 se la soglia non e' superata (ugello libero). 1 altrimenti
NOTE:    -
--------------------------------------------------------------------------*/
int UgelliClass::Check_SogliaVacuo(int punta)
{
	if(cur_uge[punta-1].IsOnNozzle())
	{
		return(cur_uge[punta-1].Check_SogliaVacuo());
	}
	return(0);
}

/*--------------------------------------------------------------------------
Set del livello di vuoto ad ugello aperto
INPUT:	-
RETURN:  valore misurato e settato
NOTE:    -
--------------------------------------------------------------------------*/
int UgelliClass::Set_VacuoLevel(int punta)
{
   int ret,code;
   CfgUgelli tmp;

   if((code=cur_uge[punta-1].Code())!=-1)
   { ret=cur_uge[punta-1].Set_VacuoLevel();
     GetRec(tmp,punta);
     SaveRec(tmp,code-'A');
     return(ret);
   }
   return(-1);
}

/*--------------------------------------------------------------------------
Set del livello di vuoto a valore indicato
INPUT:	val : valore da impostare
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void UgelliClass::Set_VacuoLevel(int val,int punta)
{
   CfgUgelli tmp;
   int code;

   if((code=cur_uge[punta-1].Code())!=-1)
   { cur_uge[punta-1].Set_VacuoLevel(val);
     GetRec(tmp,punta);
     SaveRec(tmp,code-'A');
   }
}


/*--------------------------------------------------------------------------
Setta flag di abilitazione/disabilitazione per DoOper
INPUT:	p1: Abilitazione o meno della punta 1
		p2: Abilitazione o meno della punta 2
GLOBAL:	-
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
void UgelliClass::SetFlag(int p1,int p2)
{ 
	mount_flag[0]=p1;
	mount_flag[1]=p2;
}


/*--------------------------------------------------------------------------
Estrae la struttura dati dell'ugello sulla punta specificata
INPUT:   H_Uge = struttura dove estrarre i dati
         punta = punta 1-2
GLOBAL:	-
RETURN:	TRUE se sulla punta specificata e' presente un ugello (struttura estratta),
         FALSE altrimenti.
NOTE:	   ##SMOD041002
--------------------------------------------------------------------------*/
int UgelliClass::GetRec(CfgUgelli &H_Uge,int punta)
{ if(punta>2 && punta <1)
    punta=1;
  if(cur_uge[punta-1].IsOnNozzle())
  { cur_uge[punta-1].GetRec(&H_Uge);
    return(1);
  }
  else
    return(0);
}

/*--------------------------------------------------------------------------
Legge un record dal file ugelli
INPUT:	n: Indice del record da leggere
GLOBAL:	-
RETURN:	TRUE se lettura andata a buon fine, FALSE altrimenti
		Ritorna in H_Uge il record letto
NOTE:	
--------------------------------------------------------------------------*/
int UgelliClass::ReadRec(CfgUgelli &H_Uge,int n)
{  
	int odim = sizeof(CfgUgelli);
	int rwpos;
	
	// R/W pointer in file
	rwpos = (n*odim)+(int)FileOffset;            
	
	lseek(FileHandle,rwpos,SEEK_SET);
	
	if(read(FileHandle,(char *)&H_Uge,sizeof(H_Uge))==sizeof(H_Uge))
   {
     //SMOD020204 START
     int zoff_correct=0;

     if(H_Uge.Z_offset[0]==0)
     {
       H_Uge.Z_offset[0]=-QHeader.DMm_HeadSens[0];
       zoff_correct=1;
     }

     if(H_Uge.Z_offset[1]==0)
     {
       H_Uge.Z_offset[1]=-QHeader.DMm_HeadSens[1];
       zoff_correct=1;
     }

     if(zoff_correct)
     {
       lseek(FileHandle,rwpos,SEEK_SET);
       write(FileHandle,(char *)&H_Uge,sizeof(H_Uge));
     }
     //SMOD020204 END

     return 1;
   }
	else
   {
		return 0;
   }
}


/*--------------------------------------------------------------------------
Scrive un record sul file ugelli
INPUT:	H_Uge: Record da scrivere
		n: Indice del record da scrivere
RETURN:	TRUE se scrittura andata a buon fine, FALSE altrimenti
--------------------------------------------------------------------------*/
int UgelliClass::SaveRec(CfgUgelli H_Uge,int n)
{
	int odim = sizeof(CfgUgelli); 
	int rwpos;
	
	// R/W pointer in file
	rwpos = (n*odim)+(int)FileOffset;            
	
	lseek(FileHandle,rwpos,SEEK_SET);

	if( write(FileHandle,(char *)&H_Uge,sizeof(H_Uge))==sizeof(H_Uge) )
		return 1;

	return 0;
}


/*--------------------------------------------------------------------------
Crea nuovo file ugelli
INPUT:	-
GLOBAL:	-
RETURN:	TRUE se creazione andata a buon fine, FALSE altrimenti
NOTE:	Funzione di tipo protetto
--------------------------------------------------------------------------*/
int UgelliClass::Create(void)
{
	int nloop;
	CfgUgelli uge;
	char eof = 26;
	
	if(!F_File_Create(FileHandle,UGENAME))
		return 0;
	else
	{ 
      WRITE_HEADER(FileHandle,FILES_VERSION,UGEFILE_SUBVERSION); //SMOD230703-bridge
		write(FileHandle,(char *)TXTUG1,strlen(TXTUG1));
		write(FileHandle,&eof,1);
		
		for(nloop=0;nloop<MAXUGE;nloop++)
		{
			uge.U_code=nloop+65;
			uge.U_note[0]=0;
			uge.U_tipo[0]=0;
			uge.X_ugeP1=0;
			uge.Y_ugeP1=0;
			uge.X_ugeP2=0;
			uge.Y_ugeP2=0;

         uge.NozzleAllowed=UG_ALL;

			if(write(FileHandle,(char *)&uge,sizeof(CfgUgelli))!=sizeof(CfgUgelli))
			{ 
				FilesFunc_close(FileHandle);
				return 0;
			}
		}
	}
	
	write(FileHandle,&eof,1);
	
	FilesFunc_close(FileHandle);
	
	return 1;
}


/*--------------------------------------------------------------------------
Apre il file ugelli
INPUT:	-
GLOBAL:	-
RETURN:	TRUE se apertura andata a buon fine, FALSE altrimenti
NOTE:	
--------------------------------------------------------------------------*/
int UgelliClass::Open(void)
{
	char eof=26;
	char Letto=0;
	int  i;
	CfgUgelli tmp;
	
	FileOffset=0;
	
	if(access(UGENAME,0))
	{ 
		if(!Create())
			return 0;
	}
	
	if(!F_File_Open(FileHandle,UGENAME))
		return 0;
	
	while(Letto!=eof)                    // skip title QPF
	{ 
		read(FileHandle,&Letto,1);          // & init. lines
		FileOffset++;
	}
	
	opened=1;

   for(i=0;i<MAXUGE;i++)  //azzera livello vuoto per tutti gli ugelli
   {
     ReadRec(tmp,i);
     tmp.MinVLevel=NO_VLEVELSET;
     SaveRec(tmp,i);
   }
   
	return 1;	// DANY050902	Aggiunto return(TRUE)
} //Open


/*--------------------------------------------------------------------------
Rilegge i dati degli ugelli sulle due punte
INPUT:	-
GLOBAL:	-
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
void UgelliClass::ReloadCurUge(void)
{
	CfgUgelli rec;
	int code;
	
	code=cur_uge[0].Code();
	if(code!=-1)
	{
		ReadRec(rec,code-'A');
		cur_uge[0].SetRec(rec);
	}
	
	code=cur_uge[1].Code();
	if(code!=-1)
	{
		ReadRec(rec,code-'A');
		cur_uge[1].SetRec(rec);
	}
}


/*--------------------------------------------------------------------------
Autoapprendimento sequenziale ugelli
INPUT:	-
GLOBAL:	-
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
void UgelliClass::AutoApp_Seq(int punta)
{
	int nloop;
	int changed=0;
	CfgUgelli rec;
	
	UgeClass *uge=new UgeClass(rec);
	
	for(nloop=0;nloop<MAXUGE;nloop++)
	{
		ReadRec(rec,nloop);

      if(!(rec.NozzleAllowed & punta))
      {
        continue;
      }
      
		uge->SetRec(rec);
		
		if(!uge->AutoApp(punta))
      {
        break;
      }
		else
		{
			uge->GetRec(&rec);
			SaveRec(rec,nloop);
			changed=1;
		}
	}
	
	if(changed)
   {
     ReloadCurUge();
   }
	
	delete uge;
}


/*--------------------------------------------------------------------------
Setta handler funzione passo-passo
INPUT:	-
GLOBAL:	-
RETURN:	-
NOTE:	
--------------------------------------------------------------------------*/
void UgelliClass::Set_StepF(int(*_stepF)(const char *,int,int))
{ 
	stepF=_stepF;
	
	cur_uge[0].Set_StepF(stepF);
	cur_uge[1].Set_StepF(stepF);
}
