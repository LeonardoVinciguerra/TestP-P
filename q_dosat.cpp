/*
>>>> Q_DOSAT.CPP

Gestione delle tabelle parametri dosaggio x package.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1998    ++++
++++    Mod.: W042000

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <unistd.h>

#include "filefn.h"
#include "msglist.h"
#include "q_cost.h"
#include "q_gener.h"
#include "q_wind.h"
#include "q_tabe.h"
#include "q_help.h"
#include "q_files.h"
#include "q_prog.h"
#include "q_progt.h"
#include "q_oper.h"
#include "q_dosat.h"
#include "q_fox.h"
#include "q_grcol.h"

#include "q_conf.h"
#include "q_packages.h"

#include "keyutils.h"
#include "lnxdefs.h"
#include "strutils.h"
#include "fileutils.h"

#include "gui_desktop.h"
#include "gui_functions.h"

#include <mss.h>


extern struct CfgHeader QHeader;


//--------------------------------------------------------------------------
// Costruttore della classe Dosatore
// INPUT:	foxptr: puntatore alla classe di gestione scheda Fox
//--------------------------------------------------------------------------
DosatClass::DosatClass(FoxClass *foxptr)
{
	movedown=0;
	initOk=0;
	Fox=foxptr;
	stepF=NULL;

	CurDisp=0;

	for(int i=0;i<2;i++)
	{
		Handle[i]=0;
		Offset[i]=0;
		HandlePack[i]=0;
		OffsetPack[i]=0;    
		confLoaded[i]=0;
		status[i]=0;
	}

	disabled=0;
	Interruptable=0;
	lastpoint=-1;
	PointDosaFlag=0;
	nodosapointCounter=0;
	
	PatternData.Pattern=NULL;
	PatternData.PackPattern=NULL;
	PatternData.npoints=0;
	
	C_XPos=NULL;
	C_YPos=NULL;
	C_NPoint=NULL;
	
	protezOn=0;
	
	#ifndef __DISP2
	OpenConfig(1);
	#else
	OpenConfig(1);
	OpenConfig(2);
	#endif
	
	ReadCurConfig();
	
	#ifndef __DISP2
	if(QHeader.CurDosaConfig[0]>=GetConfigNRecs(1))
	{
		QHeader.CurDosaConfig[0]=0;
		Mod_Cfg(QHeader);
	}
	#else
	for(int i=0;i<2;i++)
	{
		if(QHeader.CurDosaConfig[i]>=GetConfigNRecs(i+1))
		{
			QHeader.CurDosaConfig[i]=0;
			Mod_Cfg(QHeader);
		}
	}
	#endif
	
	#ifndef __DISP2
	curx[0]=GetLastXPos()-DosaConfig[0].CamOffset_X;
	cury[0]=GetLastYPos()-DosaConfig[0].CamOffset_Y;
	#else
	for(int i=0;i<2;i++)
	{
		curx[i]=GetLastXPos()-DosaConfig[i].CamOffset_X;
		cury[i]=GetLastYPos()-DosaConfig[i].CamOffset_Y;
	}
  
	#endif		
}

//--------------------------------------------------------------------------
// Distruttore della classe Dosatore
//--------------------------------------------------------------------------
DosatClass::~DosatClass(void)
{
	if(PatternData.Pattern!=NULL)
	{
		delete[] PatternData.Pattern;
	}

	if(PatternData.PackPattern!=NULL)
	{
		delete[] PatternData.PackPattern;
	}
 
	#ifndef __DISP2
	CloseConfig(1);
	ClosePackData(1);
	#else
	CloseConfig(1);
	CloseConfig(2);
	ClosePackData(1);
	ClosePackData(2);
	#endif  
}

//--------------------------------------------------------------------------
// Setta funzione handler passo-passo dosatore
//--------------------------------------------------------------------------
void DosatClass::Set_StepF(int(*_stepF)(const char *,int,int))
{
	stepF=_stepF;
}

//--------------------------------------------------------------------------
// Setta i puntatori alle combo box da utilizzare per mostrare la posizione
// del dosatore.
// INPUT:   Cx,Cy: puntatori per Combo asse x ed y
//--------------------------------------------------------------------------
void DosatClass::SetXYCombo( C_Combo* Cx, C_Combo* Cy )
{
	C_XPos=Cx;
	C_YPos=Cy;
}

//--------------------------------------------------------------------------
// Setta il puntatori alla combo box da utilizzare per mostrare il punto in
// corso di dosaggio
// INPUT:   CPoint: puntatore alla combo box da usare
//--------------------------------------------------------------------------
void DosatClass::SetNPointCombo( C_Combo* CPoint )
{
	C_NPoint=CPoint;
}

//--------------------------------------------------------------------------
// Disabilita operazioni dosatore
//--------------------------------------------------------------------------
void DosatClass::Disable()
{
	disabled=1;
}

//--------------------------------------------------------------------------
// Abilita operazioni dosatore
//--------------------------------------------------------------------------
void DosatClass::Enable()
{
	disabled=0;
}

//--------------------------------------------------------------------------
// Imposta la struttura dati fornita con i dati di default
// INPUT:   struttura dati da impostare
// RETURN:  nella struttura dati in ingresso i parametri di default
//--------------------------------------------------------------------------
void DosatClass::SetDefaultData(CfgDispenser& data)
{
	data.speedIndex = DISP_DEFSPEED;
	data.DosaMov_Time=DISP_DEFWAITXY;
	data.GlueOut_Time=DISP_DEFTESTTIME;
	data.GlueEnd_Time=DISP_DEFWAITEND;
	data.DosaZMovDown_Time=DISP_DEFWAITDW;
	data.Viscosity=DISP_DEFVISCOSITY;
	data.NPoint=DISP_DEFNPOINTS;
	data.TestPointsOffset=1;
	memset(&data.spare1,0,sizeof(data.spare1));
	data.tipoDosat=DISP_DEFTYPE;
	data.Inversion_Time=DISP_DEFINVERSION;
	data.DosaZMovUp_Time=DISP_DEFWAITUP;
	#ifndef __DISP2
	data.PreInversion_Time=DISP_DEFPREINV;
	#else
	data.AntiDropStart_Time=DISP_DEFANTIDROP_START;
	#endif
 
	data.VacuoPulse_Time=DISP_DEFVACUOPULSE;
	data.VacuoPulseFinal_Time=DISP_DEFVACUOPULSE_FINAL;
	data.VacuoWait_Time=DISP_DEFVACUO_WAIT;

	strcpy(data.name,DISP_DEFNAME);
	memset(data.Note,(char)0,25);
	memset(data.spare2,0,sizeof(data.spare2));
	data.CamOffset_X=0;
	data.CamOffset_Y=0;
	data.X_colla=0;
	data.Y_colla=0;
}

//--------------------------------------------------------------------------
// Apre file di configurazione
//RETURN:	FALSE se apertura fallita, TRUE altrimenti
// NOTE:	   nome file settato in precedenza o altrimenti file di default
//--------------------------------------------------------------------------
int DosatClass::OpenConfig(int ndisp)
{
	char Letto=0,ret=1;
	
	#ifdef __DISP2
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=1;
	}
	#else
	ndisp=1;
	#endif
	
	#ifndef __DISP2
	if(access(DISPNAME,0))
	#else
	if(access((ndisp==1) ? DISPNAME1 : DISPNAME2,0))
	#endif
	{
		if(!CreateConfig(ndisp))
		{
			ret=0;
			Handle[ndisp-1]=0;
		}
		else
		{
			bipbip();
			#ifndef __DISP2
			W_Mess(DOSACONF_CREATED);
			#else
			char buf[256];
			sprintf(buf,DOSACONF_CREATED2,ndisp);
			W_Mess(buf);
			#endif
		}
	}

	Offset[ndisp-1]=0;

	if(ret)
	{
		#ifndef __DISP2
		if(!F_File_Open(Handle[0],DISPNAME))
		{
			ret=0;
		}
		#else
		if(ndisp==1)
		{
			if(!F_File_Open(Handle[ndisp-1],DISPNAME1))
			{
				ret=0;
			}
		}
		else
		{
			if(!F_File_Open(Handle[ndisp-1],DISPNAME2))
			{
				ret=0;
			}
		}
		#endif
		
		if(!ret)
		{
			Handle[ndisp-1]=0;
		}
	}
	
	if(ret)
	{
		while(Letto!=EOFVAL)                // skip title
		{
			read(Handle[ndisp-1],&Letto,1);            // & init. lines
			Offset[ndisp-1]++;
		}
		
		unsigned int spare,subversion;
		Get_FileVersion_OLD(Handle[ndisp-1],spare,subversion);
		
		if( subversion == 0 )
		{
			//UPDATE
		}
		else if( subversion == 1 )
		{
			//UPDATE
		}
	}

	if(!ret)
	{
		W_Mess(ERR_DISPCFGFILE);
	}

  	return(ret);
}

/*--------------------------------------------------------------------------
Crea il file di configurazione
INPUT:   -
GLOBAL:	-
RETURN:  FALSE se creazione fallita, TRUE altrimenti
NOTE:    nome file settato in precedenza o altrimenti file di default
--------------------------------------------------------------------------*/
int DosatClass::CreateConfig(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif

	char eof = 26;
	
	struct CfgDispenser data;
	
	#ifndef __DISP2
	if(!F_File_Create(Handle[0],DISPNAME))
	{
		return 0;
	}
	#else
	if(ndisp==0)
	{
		if(!F_File_Create(Handle[0],DISPNAME1))
		{
			return 0;
		}
	}
	else
	{
		if(!F_File_Create(Handle[1],DISPNAME2))
		{
			return 0;
		}
	}
	#endif

	WRITE_HEADER(Handle[ndisp],FILES_VERSION,DOSACONFIG_SUBVERSION); //SMOD230703-bridge
	if(ndisp==0)
	{
		write(Handle[ndisp],(char *)TXTDISP1,strlen(TXTDISP1));
	}
	else
	{
		write(Handle[ndisp],(char *)TXTDISP2,strlen(TXTDISP2));
	}
	write(Handle[ndisp],&eof,1);
	
	SetDefaultData(data);

	Offset[ndisp] = lseek(Handle[ndisp],0,SEEK_CUR);
	WriteConfig(ndisp+1,data,0);
	FilesFunc_close(Handle[ndisp]);
	Handle[ndisp]=0;
	
	return 1;
}

/*--------------------------------------------------------------------------
Carica configurazione
INPUT:   -
GLOBAL:	-
RETURN:  FALSE se caricamento fallito, TRUE altrimenti
NOTE:    nome file settato in precedenza o altrimenti file di default
--------------------------------------------------------------------------*/
int DosatClass::ReadConfig(int ndisp,CfgDispenser &data,int nrec)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif
	
	if(Handle[ndisp]==0)
	{
		if(!OpenConfig(ndisp+1))
		{
			return 0;
		}
	}
	
	int totrec=GetConfigNRecs(ndisp+1);
	
	if((totrec<0) || (nrec>=totrec))
	{
		return 0;
	}
	
	lseek(Handle[ndisp],Offset[ndisp]+nrec*sizeof(struct CfgDispenser),SEEK_SET);
	
	read(Handle[ndisp],(char *)&data,sizeof(struct CfgDispenser));
	
	#ifndef __DISP2
	if(data.tipoDosat>1)
	#else
	if(((data.tipoDosat>1) && (ndisp==1)) || ((data.tipoDosat!=0) && (ndisp==0)))
	#endif
	{
		data.tipoDosat=0;
		return(WriteConfig(ndisp+1,data,nrec));
	}

	#ifdef __DISP2
	if(ndisp==0)
	{
		data.tipoDosat=0;
	}
	#endif
	
	return 1;
}


/*--------------------------------------------------------------------------
Scrive configurazione
INPUT:   -
GLOBAL:	-
RETURN:  FALSE se scrittura fallita, TRUE altrimenti
NOTE:    nome file settato in precedenza o altrimenti file di default
--------------------------------------------------------------------------*/
int DosatClass::WriteConfig(int ndisp,CfgDispenser data,int nrec)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		if(ndisp==1)
		{
			data.tipoDosat=0;
		}
		ndisp--;
	}
	#endif

	if(Handle[ndisp]==0)
	{
		if(!OpenConfig(ndisp+1))
		{
			return 0;
		}
	}

	int totrec=GetConfigNRecs(ndisp+1);

	if((totrec<0) || (nrec>totrec))
	{
		return 0;
	}

	if(nrec==totrec)
	{
		lseek(Handle[ndisp],0,SEEK_END);
	}
	else
	{
		lseek(Handle[ndisp],Offset[ndisp]+nrec*sizeof(struct CfgDispenser),SEEK_SET);
	}

	write(Handle[ndisp],(char *)&data,sizeof(struct CfgDispenser));

	return 1;

}

/*--------------------------------------------------------------------------
Legge la configurazione attuale
INPUT:   -
GLOBAL:	-
RETURN:  TRUE se operazione eseguita correttamente, FALSE altrimenti
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::ReadCurConfig(void)
{
	#ifndef __DISP2
	if(!ReadConfig(1,DosaConfig[0],QHeader.CurDosaConfig[0]))
	{
		confLoaded[0]=0;
		return 0;
	}
	else
	{
		confLoaded[0]=1;
		return 1;
	}
	#else
	for(int i=0;i<2;i++)
	{
		if(!ReadConfig(i+1,DosaConfig[i],QHeader.CurDosaConfig[i]))
		{
			confLoaded[i]=0;
			return 0;
		}
		else
		{
			confLoaded[i]=1;
		}
	}

	return 1;
	#endif
}

/*--------------------------------------------------------------------------
Controlla se una configurazione valida e' stata caricata
INPUT:   -
GLOBAL:	-
RETURN:  TRUE configurazione caricata FALSE altrimenti
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::IsConfLoaded(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif
  
	return(confLoaded[ndisp]);
}

/*--------------------------------------------------------------------------
Ritorna la configurazione
INPUT:   -
GLOBAL:	-
RETURN:  Configurazione dispenser
NOTE:    non carica le informazioni da file
--------------------------------------------------------------------------*/
struct CfgDispenser DosatClass::GetConfig(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif
  
	return(DosaConfig[ndisp]);
}
/*--------------------------------------------------------------------------
Ritorna la configurazione
INPUT:   data: struttura dove ritornare i dati
GLOBAL:	-
RETURN:  -
NOTE:    non carica le informazioni da file
--------------------------------------------------------------------------*/
void DosatClass::GetConfig(int ndisp,struct CfgDispenser &data)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif
  
	memcpy((char *)&data,(char *)&DosaConfig[ndisp],sizeof(DosaConfig[0]));
}

/*--------------------------------------------------------------------------
Ritorna in name il nome della configurazione corrente
INPUT:   name: puntatore alla stringa in cui copiare il nome
--------------------------------------------------------------------------*/
void DosatClass::GetConfigName(int ndisp,char *name)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif
	  
	strncpyQ(name,DosaConfig[ndisp].name,8);
}

/*--------------------------------------------------------------------------
Ritorna il numero di record
INPUT:   -
GLOBAL:	-
RETURN:  numero di record
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::GetConfigNRecs(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif

	if(!Handle[ndisp])
	{
		return(-1);
	}

	return((filelength(Handle[ndisp])-Offset[ndisp])/sizeof(struct CfgDispenser));
}

/*--------------------------------------------------------------------------
Check se file di configurazione aperto
RETURN:  1 se file aperto, 0 altrimenti
--------------------------------------------------------------------------*/
int DosatClass::IsConfigOpen(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif
  
	
	if(!Handle[ndisp])
  	{
    	return 0;
  	}
  
  	return 1;
}

/*--------------------------------------------------------------------------
Chiude file di configurazione se aperto
RETURN:  0 se chiusura fallita, 1 altrimenti
--------------------------------------------------------------------------*/
int DosatClass::CloseConfig(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif
  
	if(!Handle[ndisp])
	{
		return 0;
	}
	FilesFunc_close(Handle[ndisp]);
	Handle[ndisp]=0;
	return 1;
}

#ifdef __DISP2
/*--------------------------------------------------------------------------
Seleziona dispenser in uso
INPUT:   ndisp : dispenser 1/2
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void DosatClass::SelectCurrentDisp(int ndisp)
{
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=1;
	}

	CurDisp=ndisp-1;
}

int DosatClass::GetCurrentDisp(void)
{
	return(CurDisp+1);
}

#endif

/*--------------------------------------------------------------------------
Attesa assi fermi
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void DosatClass::WaitAxis(void)
{
	if(disabled)
	{
		return;
	}
  
	#ifndef __DISP2
  	CurDisp = 0;
	#endif

 	delay(DosaConfig[CurDisp].DosaMov_Time);
}

/*--------------------------------------------------------------------------
Esegue il check di protezione aperta
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::CheckProtez(void)
{
	if(protezOn)
	{
		int prev_stat[2];

		prev_stat[0]=status[0];

		#ifdef __DISP2
		prev_stat[1]=status[1];
		#endif
	
		while( CheckSecurityInput() )
		{
			SetPressOff(1);
			#ifdef __DISP2
			SetPressOff(2);
			#endif
			W_Mess(DOSPROTOPEN);
		}
	
    	//riattiva la pressione se necessario
		if((prev_stat[0] & DOSSTATUS_PRESS) && !(status[0] & DOSSTATUS_PRESS))
		{
			SetPressOn(1);
		}

		#ifdef __DISP2
		if((prev_stat[1] & DOSSTATUS_PRESS) && !(status[1] & DOSSTATUS_PRESS))
		{
			SetPressOn(2);
		}
		#endif
	
		return 0;
	}
	return(1);

}

/*--------------------------------------------------------------------------
Abilita o disabilita il check di protezione aperta
INPUT:   0 disattiva check
         1 attiva check
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void DosatClass::Set_CheckProtezFlag(int val)
{
  protezOn=val;
}

/*--------------------------------------------------------------------------
Esegue intero ciclo di dosaggio
INPUT:   time:      durata attivazione attivata in ms
                    (default:tempo di default settato nei parametri)
         viscosity: fattore correttivo di time dovuto
                    (default:fattore default settato nei parametri)
         ctrl_flags: vedi fuzione SetPressImpulse
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::ActivateDot(int time,int viscosity,int ctrl_flags)
{
	if(disabled)
	{
		return 1;
	}
	
	PointDosaFlag=0;
	
	if(!GoDown())
	{
		return(0);
	}
	
	PointDosaFlag=1;
	
	if(!SetPressImpulse(time,viscosity,ctrl_flags))
	{
		return(0);
	}
	
	return(1);
}

#ifdef __LINEDISP
/*--------------------------------------------------------------------------
Esegue intero ciclo di dosaggio linee colla
INPUT:   start      punto di inizio linea
         time:      durata attivazione attivata in ms
                    (default:tempo di default settato nei parametri)
         end:       variabile di ritorno del punto di arrivo
         viscosity: fattore correttivo di time dovuto
                    (default:fattore default settato nei parametri)
         ctrl_flags: vedi fuzione SetPressImpulse
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::ActivateLine(int start,int &end,float x,float y,int time,int viscosity,int ctrl_flags)
{
	if(disabled)
	{
		return(1);
	}
	
	end=start+1;
	
	int ok=1;
	
	float xstart,ystart;
	float xend,yend;
	
	do
	{
		xstart=PatternData.Pattern[start*2];
		ystart=PatternData.Pattern[start*2+1];
	
		do
		{
		xend=PatternData.Pattern[end*2];
		yend=PatternData.Pattern[end*2+1];
	
		if((xend==DOSNOPOINT) || (yend==DOSNOPOINT))
		{
			end++;
		}
		else
		{
			break;
		}

		} while(end<PatternData.npoints);
	
		if((xstart!=xend) && (ystart!=yend))
		{
			start=end;
			end++;
		}
		else
		{
		break;
		}
		
	} while(end<PatternData.npoints);

	if(end>=PatternData.npoints)
	{
		return(1);
	}

	PointDosaFlag=0;
	
	Wait_PuntaXY();
	
	if(!GoPosiz(x+xstart,y+ystart))
	{
		return(0);
	}

	Wait_PuntaXY();
	
	if(!GoDown())
	{
		return(0);
	}

	PointDosaFlag=1;
	
	SwitchDispenserPressure(CurDisp+1);
	SetPressOn();
	
	//volumetrico
	if((CurDisp==VOLUMETRIC_NDISPENSER-1) && (DosaConfig.tipoDosat && (ctrl_flags & DOSAPRESS_USETYPE)))
	{
		SetInversion(DOSADIR_NORMAL);
		SetMotorOn();
	}
	
	delay(DosaConfig.GlueOut_Time); //attesa uscita colla
	
	unsigned int tmp=DosaConfig.Vel_Max;

	int retval=1;
	
	//corregge la velocita di movimento assi
	DosaConfig.Vel_Max=(DosaConfig.Vel_Max*DosaConfig.GlueEnd_Time)/100;
	
	//il dispenser si trova gia' nel punto di partenza
	
	if(!GoPosiz(x+xend,y+yend))
	{
		retval=0;
	}
	
	DosaConfig.Vel_Max=tmp;

	//a prescindere dalla durata del movimento, mantiene attiva la pressione
	//fino ad un massimo di 20ms
	
	if(time>20)
	{
		delay(time);
		SetPressOff();
	}
	
	Wait_PuntaXY();
	
	if(time<=20)
	{
		SetPressOff();
	}
	
	if(DosaConfig.tipoDosat && (ctrl_flags & DOSAPRESS_USETYPE))
	{
		delay(DosaConfig.AntiDropStart_Time);
	
		SetInversion(DOSADIR_INVERTED);
		SetMotorOn();
		delay(DosaConfig.Inversion_Time);
		SetMotorOff();
		SetInversion(DOSADIR_NORMAL);
	}
	
	PointDosaFlag=0;

	if(!GoUp())
	{
		return(0);
	}
	
	delay(DosaConfig.DosaZMovUp_Time);
	
	return(retval);
}
#endif


/*--------------------------------------------------------------------------
Ciclo di attivazione pressione
INPUT:   time:      durata attivazione attivata in ms
                    (default:tempo di default settato nei parametri)
         viscosity: fattore correttivo di time dovuto
                    (default:fattore default settato nei parametri)
         ctrl_flags:bit 0 (DOSAPRESS_GOUP)
                          attivazione risalita al termine del dosaggio
                    bit 1 (DOSAPRESS_USETYPE)
                          uso del flag tipodosat per determinare il tipo
                    bit 2 (DOSAPRESS_NOZEROM)
                          impedisce dosaggio su zero macchina

GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::SetPressImpulse(int time,int viscosity,int ctrl_flags)
{
	int quant;
	
	if(disabled)
	{
		return 1;
	}
	
	#ifndef __DISP2
	CurDisp=0;
	#endif  

	curx[CurDisp]=GetLastXPos()-DosaConfig[CurDisp].CamOffset_X;
	cury[CurDisp]=GetLastYPos()-DosaConfig[CurDisp].CamOffset_Y;	
		
	#ifndef HWTEST_RELEASE
	if(ctrl_flags & DOSAPRESS_NOZEROM)
	{
		if(curx==0 && cury==0)
		{
			bipbip();
			W_Mess(NODOSAZERO);
			return(0);
		}
	}
	#endif    
	
	if(!time)
	{
		quant=DosaConfig[CurDisp].GlueOut_Time;
	}
	else
	{
		quant=time;
	}
	
	if(!viscosity)
	{
		quant=(quant*DosaConfig[CurDisp].Viscosity/100);
	}
	else
	{
		quant=(quant*viscosity/100);
	}
	
	if(stepF!=NULL)
	{
		if(!stepF( MsgGetString(Msg_01203),1,0))
		{
			return 0;
		}
	}
	
	#ifdef __DISP2
	SwitchDispenserPressure(CurDisp+1);
	#endif
	SetPressOn(CurDisp+1);
	
	//volumetrico
	if((CurDisp==(VOLUMETRIC_NDISPENSER-1)) && (DosaConfig[CurDisp].tipoDosat && (ctrl_flags & DOSAPRESS_USETYPE)))
	{
		SetInversion(DOSADIR_NORMAL);
		SetMotorOn();
	}

	delay(quant);  

	SetPressOff();

	if((CurDisp==(VOLUMETRIC_NDISPENSER-1)) && (DosaConfig[CurDisp].tipoDosat && (ctrl_flags & DOSAPRESS_USETYPE)))
	{
		SetMotorOff();
	}  
	
	delay(DosaConfig[CurDisp].GlueEnd_Time);
	
	if(ctrl_flags & DOSAPRESS_GOUP)
	{
		if(!GoUp())
		{
			return(0);
		}
	}

	#ifndef __DISP2
	if((CurDisp==(VOLUMETRIC_NDISPENSER-1)) && (DosaConfig[CurDisp].tipoDosat && (ctrl_flags & DOSAPRESS_USETYPE)))
	{
		delay(DosaConfig[CurDisp].PreInversion_Time);

		SetInversion(DOSADIR_INVERTED);
		SetMotorOn();
		delay(DosaConfig[CurDisp].Inversion_Time);
		SetMotorOff();
		SetInversion(DOSADIR_NORMAL);
	}
	#else	
	if(ctrl_flags & DOSAPRESS_ANTIDROP) 
	{
		if((DosaConfig[CurDisp].tipoDosat==0) || (CurDisp!=(VOLUMETRIC_NDISPENSER-1)))
		{
			if(DosaConfig[CurDisp].VacuoPulse_Time!=0)
			{
				delay(DosaConfig[CurDisp].AntiDropStart_Time);
			}
		}
		else
		{
			if(DosaConfig[1].Inversion_Time)
			{
				delay(DosaConfig[1].AntiDropStart_Time);
			}
		}

		DoVacuoPulse();    
	}    	
	#endif
	

	delay(DosaConfig[CurDisp].DosaZMovUp_Time);
	
	return(1);
}

#ifdef __DISP2
void DosatClass::DoVacuoPulse(int ndisp)
{
	if(disabled)
	{
		return;
	}

	if(ndisp==0)
	{
		ndisp=CurDisp+1;
	}
	else
	{
		if((ndisp<1) || (ndisp>2))
		{
			ndisp=1;
		}
	}

	if((DosaConfig[ndisp-1].tipoDosat==0) || (ndisp!=VOLUMETRIC_NDISPENSER)) //tempo pressione
	{
		if(DosaConfig[ndisp-1].VacuoPulse_Time!=0)
		{
			SetVacuoOn(ndisp);
			delay(DosaConfig[ndisp-1].VacuoPulse_Time);
			SetVacuoOff(ndisp);
		}
	}
	else
	{
		if(DosaConfig[1].Inversion_Time)
		{
			if(DosaConfig[1].VacuoPulse_Time)
			{
				SetVacuoOn(2);

				SetInversion(DOSADIR_INVERTED);
				SetMotorOn();

				short tmpDelay;

				if(DosaConfig[1].Inversion_Time>=DosaConfig[1].VacuoPulse_Time)
				{
					delay(DosaConfig[1].VacuoPulse_Time); //l'inversione dura piu dell'impulso di vuoto
					SetVacuoOff(2);
					tmpDelay = DosaConfig[1].Inversion_Time-DosaConfig[1].VacuoPulse_Time;
					delay(tmpDelay);
					SetMotorOff();
					SetInversion(DOSADIR_NORMAL);
          
				}
				else
				{
					delay(DosaConfig[1].Inversion_Time); //l'inversione dura meno dell'impulso di vuoto :
															 //il vuoto rimane attivo solo durante l'inversione
					SetMotorOff();
					SetInversion(DOSADIR_NORMAL);
					tmpDelay = DosaConfig[1].VacuoPulse_Time-DosaConfig[1].Inversion_Time;
					delay(tmpDelay);
					SetVacuoOff(2);
				}
			}
			else
			{
				SetInversion(DOSADIR_INVERTED);
				SetMotorOn();
				delay(DosaConfig[1].Inversion_Time);
				SetMotorOff();
				SetInversion(DOSADIR_NORMAL);      
			}      
		}
	}
}

void DosatClass::DoVacuoFinalPulse(int ndisp)
{
	if(disabled)
	{
		return;
	}

	if(ndisp==0)
	{
		ndisp=CurDisp+1;
	}
	else
	{
		if((ndisp<1) || (ndisp>2))
		{
			ndisp=1;
		}
	}
  

	if((DosaConfig[ndisp-1].tipoDosat==0) || (ndisp!=VOLUMETRIC_NDISPENSER)) //tempo-pressione o primo dispenser
	{
		SetVacuoOn(ndisp,1);
		delay(DosaConfig[ndisp-1].VacuoPulseFinal_Time);
		SetVacuoOff(ndisp);
	}
	else
	{
		SetInversion(DOSADIR_INVERTED);
		SetMotorOn();
		SetVacuoOn(2,1);
		delay(DosaConfig[ndisp-1].VacuoPulseFinal_Time);
		SetVacuoOff(2);
		SetMotorOff();
		SetInversion(DOSADIR_NORMAL);
	}
}
#endif

/*--------------------------------------------------------------------------
Abbassa dosatore
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::GoDown(void)
{
	#ifndef __DISP2
	CurDisp=0;
	#endif

	if(disabled)
	{
		return 1;
	}

	if(status[CurDisp] & DOSSTATUS_DOWN)
	{
		return 1;
	}

	if(stepF!=NULL)
	{
		if(!stepF( MsgGetString(Msg_01205),1,0))
		{
			return 0;
		}
	}

	#ifndef __DISP2
	Fox->SetOutput(FOX,DISPMOVE);
	#else
	Fox->SetOutput(FOX,(CurDisp==0) ? DISPMOVE1 : DISPMOVE2);
	#endif

	delay(DosaConfig[CurDisp].DosaZMovDown_Time);

	status[CurDisp]|=DOSSTATUS_DOWN;
	return 1;

}

/*--------------------------------------------------------------------------
Alza dosatore
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::GoUp(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if(ndisp==0)
	{
		ndisp=CurDisp;
	}
	else
	{
		if((ndisp<1) || (ndisp>2))
		{
			ndisp=0;
		}
		else
		{
			ndisp--;
		}
	}	
	#endif

	if(disabled)
	{
		return 1;
	}

	if(!(status[ndisp] & DOSSTATUS_DOWN))
	{
		return 1;
	}

	if(stepF!=NULL)
	{
		if(!stepF( MsgGetString(Msg_01204),1,0))
		{
			return 0;
		}
	}

	#ifndef __DISP2
	Fox->ClearOutput(FOX,DISPMOVE);
	#else
	Fox->ClearOutput(FOX,(ndisp==0) ? DISPMOVE1 : DISPMOVE2);
	#endif

	status[ndisp]&=~DOSSTATUS_DOWN;
	return 1;



}

/*--------------------------------------------------------------------------
Attiva pressione siringa
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::SetPressOn(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if(ndisp==0)
	{
		ndisp=CurDisp;
	}
	else
	{
		if((ndisp<1) || (ndisp>2))
		{
			ndisp=0;
		}
		else
		{
			ndisp--;
		}
	}
	#endif
  
	if(disabled)
	{
		return 1;
	}

	Set_CheckProtezFlag(1);
	CheckProtez();

	status[ndisp]|=DOSSTATUS_PRESS;

	#ifndef __DISP2
	Fox->SetOutput(FOX,DISPPRESS);
	#else
	Fox->SetOutput(FOX,(ndisp==0) ? DISPPRESS1 : DISPPRESS2);
	#endif

	return 1;
}

/*--------------------------------------------------------------------------
Disattiva pressione siringa
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::SetPressOff(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if(ndisp==0)
	{
		ndisp=CurDisp;
	}
	else
	{
		if((ndisp<1) || (ndisp>2))
		{
			ndisp=0;
		}
		else
		{
			ndisp--;
		}
	}
	#endif

	if(disabled)
	{
		return 1;
	}

	status[ndisp]&=~DOSSTATUS_PRESS;
	#ifndef __DISP2
	Fox->ClearOutput(FOX,DISPPRESS);
	#else
	Fox->ClearOutput(FOX,(ndisp==0) ? DISPPRESS1 : DISPPRESS2);
	#endif

	return 1;
}

#ifdef __DISP2
/*--------------------------------------------------------------------------
Attiva vuoto per antisgocciolio
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void DosatClass::SetVacuoOn(int ndisp,int wait_long)
{
	if(disabled)
	{
		return;
	}

	if(ndisp==0)
	{
		ndisp=CurDisp+1;
	}
	else
	{
		if((ndisp<1) || (ndisp>2))
		{
			ndisp=1;
		}
	}

	SetDispVacuoGeneratorOn(ndisp);

	if(wait_long)
	{
		delay(QHeader.DispVacuoGen_Delay);
	}
	else
	{
		delay(DosaConfig[ndisp-1].VacuoWait_Time);
	}
  
	SetPressOn(ndisp);
}

/*--------------------------------------------------------------------------
Disattiva vuoto per antisgocciolio
INPUT:
GLOBAL:	-
RETURN:  -
NOTE:
--------------------------------------------------------------------------*/
void DosatClass::SetVacuoOff(int ndisp)
{
	if(disabled)
	{
		return;
	}


	if(ndisp==0)
	{
		ndisp=CurDisp+1;
	}
	else
	{
		if((ndisp<1) || (ndisp>2))
		{
			ndisp=1;
		}
	}

	SetPressOff(ndisp);
	delay(DISP_VALVE_SWITCHTIME);
	SetDispVacuoGeneratorOff();

}
#endif

/*--------------------------------------------------------------------------
Attiva/Disattiva pressione siringa
INPUT:   val: 0 disattiva
              1 attiva
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
/*int DosatClass::SetPress(int val)
{
  if(disabled)
    return 1;

  if(val)
  {
    status|=DOSSTATUS_PRESS;
    Fox->SetOutput(FOX,DISPPRESS);

  }
  else
  {
    status&=~DOSSTATUS_PRESS;
    Fox->ClearOutput(FOX,DISPPRESS);
  }

  return 1;
}*/

/*--------------------------------------------------------------------------
Attiva motore
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::SetMotorOn(void)
{
	#ifndef __DISP2
	CurDisp=0;
	#else
	if(CurDisp==0) //se dispenser 1 non fare nulla (CurDisp is zero based)
	{
		return(1);
	}
	#endif

	if(status[CurDisp] & DOSSTATUS_MOTOR)
	{
		return(1);
	}

	#ifdef __DISP2
	/*
	float time_elapsed=(clock()-InversionStatChanged_Timer)*1000.0/CLOCKS_PER_SEC;

	if((time_elapsed>=0) && (time_elapsed<DOSAINV_SWITCH_TIME))
	{
		delay(DOSAINV_SWITCH_TIME-int(time_elapsed));
	}
	*/
	delay(DOSAINV_SWITCH_TIME);
	#endif

	status[CurDisp]|=DOSSTATUS_MOTOR;
	Fox->SetOutput(FOX,DISPMOTOR);

	#ifdef __DISP2
	MotorStatChanged_Timer=clock();  
	#endif	
}

/*--------------------------------------------------------------------------
Disattiva motore
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::SetMotorOff(void)
{
	#ifndef __DISP2
	CurDisp=0;
	#else
	if(CurDisp==0) //se dispenser 1 non fare nulla (CurDisp is zero based)
	{
		return(1);
	}
	#endif

	if(!(status[CurDisp] & DOSSTATUS_MOTOR))
	{
		return(1);
	}

	#ifdef __DISP2
	/*
	float time_elapsed=(clock()-InversionStatChanged_Timer)*1000.0/CLOCKS_PER_SEC;

	if((time_elapsed>=0) && (time_elapsed<DOSAINV_SWITCH_TIME))
	{
		delay(DOSAINV_SWITCH_TIME-int(time_elapsed));
	}
	*/
	delay(DOSAINV_SWITCH_TIME);
	#endif

	status[CurDisp]&=~DOSSTATUS_MOTOR;
	Fox->ClearOutput(FOX,DISPMOTOR);

	#ifdef __DISP2
	MotorStatChanged_Timer=clock();
	#endif
}

/*--------------------------------------------------------------------------
Setta direzione movimento motore
INPUT:   DOSADIR_NORMAL  : normale
         DOSADIR_INVERTED: movimento invertito
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::SetInversion(int dir)
{
	#ifndef __DISP2
	CurDisp=0;
	#else
	if(CurDisp==0) //se dispenser 1 non fare nulla (CurDisp is zero based)
	{
		return(1);
	}
	#endif

	if(dir==DOSADIR_INVERTED)
	{
		if(status[CurDisp] & DOSSTATUS_INVERSION)
		{
			return(1);
		}
	}
	else
	{
		if(!(status[CurDisp] & DOSSTATUS_INVERSION))
		{
			return(1);
		}
  
	}

	#ifdef __DISP2
	/*
	float time_elapsed=(clock()-MotorStatChanged_Timer)*1000.0/CLOCKS_PER_SEC;

	if((time_elapsed>=0) && (time_elapsed<MOTOR_STARTSTOP_TIME))
	{
		delay(MOTOR_STARTSTOP_TIME-int(time_elapsed));
	}
	*/
	delay(MOTOR_STARTSTOP_TIME);
	int prev_stat = status[CurDisp];
	#endif

	if(dir==DOSADIR_INVERTED)
	{
		status[CurDisp]|=DOSSTATUS_INVERSION;
		Fox->SetOutput(FOX,DISPINV);
	}
	else
	{
		status[CurDisp]&=~DOSSTATUS_INVERSION;
		Fox->ClearOutput(FOX,DISPINV);
	}

	#ifdef __DISP2
	if( (prev_stat ^ status[CurDisp]) & DOSSTATUS_INVERSION )
	{
		InversionStatChanged_Timer=clock();
	}
	#endif
}



/*--------------------------------------------------------------------------
Ritorna stato dispenser
INPUT:   -
GLOBAL:	-
RETURN:  bitmap: bit 0: (0) dosatore alto
                        (1) dosatore abbassato
                 bit 1: (0) pressione disattivata
                        (1) pressione attivata
                 bit 2: (0) motore disattivato
                        (1) motore attivato
                 bit 3: (0) inversione disattivata
                        (1) inversione attivata
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::GetStatus(int ndisp)
{
	return(status[ndisp]);
}

/*--------------------------------------------------------------------------
Controlla che il dosatore possa muoversi in XY
RETURN:  TRUE se movimento ok, FALSE altrimenti
--------------------------------------------------------------------------*/
int DosatClass::CanXYMove(void)
{
	#ifndef __DISP2
	return((!(status[0] & DOSSTATUS_DOWN)) || movedown);
	#else
	if(movedown)
	{
		return(1);
	}

	if((status[0] & DOSSTATUS_DOWN) || (status[1] & DOSSTATUS_DOWN))
	{
		return(0);
	}
	else
	{
		return(1);
	}
	#endif
}

/*--------------------------------------------------------------------------
Abilita i movimenti a dispenser abbassato
--------------------------------------------------------------------------*/
void DosatClass::EnableMoveDown(void)
{
	movedown = 1;
}

/*--------------------------------------------------------------------------
Disabilita i movimenti a dispenser abbassato
--------------------------------------------------------------------------*/
void DosatClass::DisableMoveDown(void)
{
	movedown = 0;
}

/*--------------------------------------------------------------------------
Crea file dati dosaggio per i packages
INPUT:   filename: file da creare completo di pathname
GLOBAL:	-
RETURN:  FALSE se creazione fallita, TRUE altrimenti
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::CreatePackData(int ndisp,char *filename,int addpath)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif	
	
	char eof = 26;
	int i;
	struct PackDosData data;
	
	char path[MAXNPATH];
	
	#ifndef __DISP2
	if(addpath==DOSAPACK_ADDPATH)
	{
		PackDosPath(path,filename);  // nome con path
	}
	else
	{
		strcpy(path,filename);
	}
	#else
	if(addpath==DOSAPACK_ADDPATH)
	{
		PackDosPath(ndisp+1,path,filename);  // nome con path
	}
	else
	{
		strcpy(path,filename);
	}   
	#endif		
	
	if(!F_File_Create(HandlePack[ndisp],path))
	{
		return 0;
	}
	
	WRITE_HEADER(HandlePack[ndisp],FILES_VERSION,DOSAPACK_SUBVERSION); //SMOD230703-bridge
	write(HandlePack[ndisp],(char *)TXTDOSAPACK,strlen(TXTDOSAPACK));
	write(HandlePack[ndisp],&eof,1);

	OffsetPack[ndisp]=strlen(TXTDOSAPACK)+1+HEADER_LEN;
	
	//init dati packages
	for(i=0;i<MAXPACK;i++)
	{
		data.code=i+1;
		data.nameX[0]=0;
		data.ox_b=0;
		data.oy_b=0;
		data.ox_a=0;
		data.oy_a=0;
		data.ox_s=0;
		data.oy_s=0;
		data.ox_d=0;
		data.oy_d=0;
		
		data.dist_b=0;
		data.dist_a=0;
		data.dist_s=0;
		data.dist_d=0;
	
		data.escl_start_b=0;
		data.escl_end_b=0;
		data.escl_start_a=0;
		data.escl_end_a=0;
		data.escl_start_s=0;
		data.escl_end_s=0;
		data.escl_start_d=0;
		data.escl_end_d=0;
	
		data.npoints_a=0;
		data.npoints_b=0;
		data.npoints_s=0;
		data.npoints_d=0;
	
		data.quant=100;
		data.note[0]=0;
		
		WritePackData(ndisp+1,i,data);
	}
	
	FilesFunc_close(HandlePack[ndisp]);
	HandlePack[ndisp]=0;
	
	return 1;
}

/*--------------------------------------------------------------------------
Apre file dati dosaggio per i package
INPUT:   filename: file da aprire completo di pathname
GLOBAL:	-
RETURN:  FALSE se apertura fallita, TRUE altrimenti
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::OpenPackData(int ndisp,char *filename,int addpath)
{
	char Letto=0;
	
	#ifndef __DISP2
	ndisp=1;
	if(addpath==DOSAPACK_ADDPATH)
	{
		PackDosPath(packfile,filename);  // nome con path
	}
	else
	{
    	strcpy(packfile,filename);
	}
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=1;
	}
	if(addpath==DOSAPACK_ADDPATH)
	{
		PackDosPath(ndisp,packfile,filename);  // nome con path
	}
	else
	{
		strcpy(packfile,filename);
	}
	#endif
	
	if(access(packfile,0))
	{
		if(!CreatePackData(ndisp,packfile,DOSAPACK_NOADDPATH))
		{
			return 0;
		}
	}
	
	if(!F_File_Open(HandlePack[ndisp-1],packfile))
	{
		return 0;
	}
	
	OffsetPack[ndisp-1]=0;
	
	while(Letto!=EOFVAL)                // skip title
	{ 
		read(HandlePack[ndisp-1],&Letto,1);            // & init. lines
		OffsetPack[ndisp-1]++;
	}
	
	return 1;
}  
  
/*--------------------------------------------------------------------------
Carica un record dal file dati di dosaggio per i package
INPUT:   rec: numero record
GLOBAL:	-
RETURN:  FALSE se caricamento fallito, TRUE altrimenti
NOTE:    carica i dati internamente alla classe
--------------------------------------------------------------------------*/
int DosatClass::LoadPackData( int ndisp, int rec )
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if( ndisp < 1 || ndisp > 2 )
	{
		ndisp = 0;
	}
	else
	{
		ndisp--;
	}
	#endif

	if(!HandlePack[ndisp])
	{
		return 0;
	}

	lseek(HandlePack[ndisp],OffsetPack[ndisp]+rec*sizeof(PackDosData),SEEK_SET);

	if(read(HandlePack[ndisp],(char *)&PackData[ndisp],sizeof(PackDosData))==-1)
	{
		return 0;
	}
	else
	{
		if(PackData[ndisp].displacement<PKDISPLACEMENT_MIN)
		{
			PackData[ndisp].displacement=PKDISPLACEMENT_MIN;

			WritePackData(ndisp+1,rec,PackData[ndisp]);
		}
		else
		{
			if(PackData[ndisp].displacement>PKDISPLACEMENT_MAX)
			{
				PackData[ndisp].displacement=PKDISPLACEMENT_MAX;

				WritePackData(ndisp+1,rec,PackData[ndisp]);
			}
		}

		return 1;
	}
}

/*--------------------------------------------------------------------------
Scrive   un record dal file dati di dosaggio per i package
INPUT:   rec : numero record
         data: dati da scrivere
GLOBAL:	-
RETURN:  FALSE se scrittura fallita, TRUE altrimenti
NOTE:    salva e setta i dati internamente alla classe
--------------------------------------------------------------------------*/
int DosatClass::WritePackData(int ndisp,int rec,struct PackDosData data)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif

	if(!HandlePack[ndisp])
	{
		return 0;
	}

	lseek(HandlePack[ndisp],OffsetPack[ndisp]+rec*sizeof(PackDosData),SEEK_SET);

	memcpy((char *)&PackData[ndisp],(char *)&data,sizeof(PackDosData));

	if(write(HandlePack[ndisp],(char *)&PackData[ndisp],sizeof(PackDosData))==-1)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/*--------------------------------------------------------------------------
Check se file dati dosaggio per i package e' aperto
INPUT:   -
GLOBAL:	-
RETURN:  FALSE se file chiuso, TRUE altrimenti
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::IsPackDataOpen(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif

	if(!HandlePack[ndisp])
	{
		return 0;
	}
  
	return 1;
}

/*--------------------------------------------------------------------------
Chiude il file dati dosaggio per i package se aperto
INPUT:   -
GLOBAL:	-
RETURN:  FALSE se chiusura fallita, TRUE altrimenti
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::ClosePackData(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif

	if(!HandlePack[ndisp])
	{
		return 0;
	}
  
	FilesFunc_close(HandlePack[ndisp]);
	HandlePack[ndisp]=0;
	return 1;
}

/*--------------------------------------------------------------------------
Ritorna i dati di dosaggio per il package
INPUT:   -
GLOBAL:	-
RETURN:  Configurazione di dosaggio per il package
NOTE:    non carica le informazioni da file
--------------------------------------------------------------------------*/
PackDosData DosatClass::GetPackData(int ndisp)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif

	return(PackData[ndisp]);
}

/*--------------------------------------------------------------------------
Ritorna i dati di dosaggio per il package
INPUT:   data: strttura dove ritornare i dati
GLOBAL:	-
RETURN:  -
NOTE:    non carica le informazioni da file
--------------------------------------------------------------------------*/
void DosatClass::GetPackData(int ndisp,PackDosData &data)
{
	#ifndef __DISP2
	ndisp=0;
	#else
	if((ndisp<1) || (ndisp>2))
	{
		ndisp=0;
	}
	else
	{
		ndisp--;
	}
	#endif
  
	memcpy((char *)&data,(char *)&PackData[ndisp],sizeof(PackData[ndisp]));
}

/*--------------------------------------------------------------------------
Porta il dosatore alle coordinate impostate
INPUT:   x_posiz, y_posiz: coordinate a cui portare il dosatore
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::GoPosiz(float x_posiz,float y_posiz)
{
	float cx_oo,  cy_oo;
	char buf[40];
	
	#ifndef __DISP2
	CurDisp=0;
	#endif
	
	
	curx[CurDisp]=x_posiz;  //SMOD020703
	cury[CurDisp]=y_posiz;  //SMOD020703
	
	cx_oo=x_posiz+DosaConfig[CurDisp].CamOffset_X;
	cy_oo=y_posiz+DosaConfig[CurDisp].CamOffset_Y;
	
	if(C_XPos!=NULL)
	{
		sprintf(buf,"%5.2f",cx_oo);
		C_XPos->SetTxt(buf);
	}
	
	if(C_YPos!=NULL)
	{
		sprintf(buf,"%5.2f",cy_oo);
		C_YPos->SetTxt(buf);
	}

	SetNozzleXYSpeed_Index( DosaConfig[CurDisp].speedIndex );

	if(stepF!=NULL)
	{
		if(!stepF( MsgGetString(Msg_01202),1,0))
		{
			return(0);
		}
	}

	return NozzleXYMove( cx_oo, cy_oo );
}

/*--------------------------------------------------------------------------
Ruota un punto di dosaggio
INPUT:   px,py: coordinate del punto rispetto al centro di dosaggio
         rotaz: rotazione in gradi
GLOBAL:	-
RETURN:  ritorna in px,py le coordinate trasformate
NOTE:    -
--------------------------------------------------------------------------*/
int DosatClass::PointRot(float *px,float *py,float rotaz)
{
	float x_ass, y_ass;
	float arctan, segment;
	float beta;

	//nessuna rotazione
   //SMOD110403
	if((rotaz==0) || (PatternData.Pattern==NULL) || (*px==0 && *py==0))
     return(1);


	x_ass=*px;
	y_ass=*py;

	arctan=atan2(y_ass,x_ass);
	segment=sqrt(pow(x_ass,2)+pow(y_ass,2));

   beta=arctan+(rotaz*(PI/180));

   *px=(float)(segment*cos(beta));
	*py=(float)(segment*sin(beta));

	return(1);

}

/*--------------------------------------------------------------------------
Ruota il pattern di dosaggio
INPUT:   rotaz: rotazione in gradi
GLOBAL:	-
RETURN:  -
NOTE:    ricalcolo del pattern di dosaggio (il pattern precedente viene perso)
--------------------------------------------------------------------------*/
void DosatClass::PatternRotate(float rotaz)
{
   float *ptrX,*ptrY;
   int i;

   if(rotaz==0 || PatternData.Pattern==NULL)
     return;

   memcpy((char *)PatternData.Pattern,(char *)PatternData.PackPattern,PatternData.npoints*8);

   ptrX=PatternData.Pattern;
   ptrY=PatternData.Pattern+1;

   for(i=0;i<PatternData.npoints;i++)
   {
     PointRot(ptrX,ptrY,rotaz);
     ptrX+=2;
     ptrY+=2;
   }
}

/*--------------------------------------------------------------------------
Ordina il Pattern di dosaggio con ordine cresecente in x
INPUT:   -
GLOBAL:	-
RETURN:  -
NOTE:    -
--------------------------------------------------------------------------*/
void DosatClass::PatternSort(void)
{
	int i=0,j,k,swap=1;
	float tmp;

   float *ptrX,*ptrY,*ptr2X,*ptr2Y;

   if(PatternData.Pattern==NULL)
     return;

   nodosapointCounter=0;

   ptrX=PatternData.Pattern;
   ptrY=PatternData.Pattern+1;

   for(i=0;swap && i<PatternData.npoints-1;i++)
   {
     swap=0;
     for(k=0;k<PatternData.npoints-i-1;k++)
     {
       if(PatternData.Pattern[(k+1)*2]<PatternData.Pattern[k*2])
       {
         swap=1;

         tmp=PatternData.Pattern[(k+1)*2];
         PatternData.Pattern[(k+1)*2]=PatternData.Pattern[k*2];
         PatternData.Pattern[k*2]=tmp;

         tmp=PatternData.Pattern[(k+1)*2+1];
         PatternData.Pattern[(k+1)*2+1]=PatternData.Pattern[k*2+1];
         PatternData.Pattern[k*2+1]=tmp;
       }
     }
   }


   ptrX=PatternData.Pattern;
   ptrY=PatternData.Pattern+1;

	// check punti coincidenti
   for(i=0;i<PatternData.npoints-1;i++)
   {
     if(*ptrX==DOSNOPOINT || *ptrY==DOSNOPOINT)
     {
       ptrX+=2;
       ptrY+=2;
       continue;
     }

     ptr2X=ptrX+2;
     ptr2Y=ptrY+2;

     for(j=0;j<(PatternData.npoints-i-1);j++)
     {
       if(fabs(*ptr2X-*ptrX)<0.05 && fabs(*ptr2Y-*ptrY)<0.05)
       {
         *ptr2X=DOSNOPOINT;
         *ptr2Y=DOSNOPOINT;
         nodosapointCounter++;
       }
       
       ptr2X+=2;
       ptr2Y+=2;
       
     }

     ptrX+=2;
     ptrY+=2;

//     MSS_CHECK_POINTER_VALIDITY(ptrX);
//     MSS_CHECK_POINTER_VALIDITY(ptrY);
     
   }

}

/*--------------------------------------------------------------------------
Crea il pattern di dosaggio a partire dai dati package gi� inseriti
INPUT:   rotaz: angolo di cui ruotare il pattern (opzionale)
--------------------------------------------------------------------------*/
void DosatClass::CreateDosPattern(float rotaz)
{
  if(PatternData.PackPattern==NULL)
    return;

  if(PatternData.Pattern==NULL)
  {
    PatternData.Pattern=new float[PatternData.npoints*2];
  }

  nodosapointCounter=0;

  memcpy((char *)PatternData.Pattern,(char *)PatternData.PackPattern,PatternData.npoints*8);


  /*
  print_debug("\nCreate\n");
  for(int i=0;i<PatternData.npoints;i++)
    print_debug("%f %f\n",PatternData.Pattern[i*2],PatternData.Pattern[i*2+1]);
  print_debug("\n");
  */


#ifndef __LINEDISP
  PatternRotate(rotaz);
#endif


/*
  print_debug("\nRotate %f\n",rotaz);
  for(int i=0;i<PatternData.npoints;i++)
    print_debug("%f %f\n",PatternData.Pattern[i*2],PatternData.Pattern[i*2+1]);
  print_debug("\n");
*/

#ifndef __LINEDISP
  PatternSort();
#else
  float *ptrX,*ptrY,*ptr2X,*ptr2Y;

  ptrX=PatternData.Pattern;
  ptrY=PatternData.Pattern+1;

  // check punti coincidenti
  for(int i=0;i<PatternData.npoints-1;i++)
  {
    if(*ptrX==DOSNOPOINT || *ptrY==DOSNOPOINT)
    {
      ptrX+=2;
      ptrY+=2;
      continue;
    }

    ptr2X=ptrX+2;
    ptr2Y=ptrY+2;

    if(fabs(*ptr2X-*ptrX)<0.05 && fabs(*ptr2Y-*ptrY)<0.05)
    {
      *ptr2X=DOSNOPOINT;
      *ptr2Y=DOSNOPOINT;
      nodosapointCounter++;
    }

    ptrX+=2;
    ptrY+=2;
  }

#endif


  /*
  print_debug("\nSort\n");
  for(int i=0;i<PatternData.npoints;i++)
    print_debug("%f %f\n",PatternData.Pattern[i*2],PatternData.Pattern[i*2+1]);
  print_debug("\n");
  */
}

/*--------------------------------------------------------------------------
Carica un pattern dal file dati dosaggio per i package
INPUT:   rec: record del package da caricare
GLOBAL:	-
RETURN:  -
NOTE:    dealloca il pattern di dosaggio se presente
--------------------------------------------------------------------------*/
int DosatClass::LoadPattern(int rec)
{
	struct PackDosData data;
	int i;
	
	if(!LoadPackData(CurDisp+1,rec))
	{
		return(0);
	}
		
	GetPackData(CurDisp+1,data);
	
	int np_s=data.npoints_s;
	int np_d=data.npoints_d;
	int np_a=data.npoints_a;
	int np_b=data.npoints_b;
	
	
	if(data.escl_end_s!=0 && data.escl_start_s!=0)
		np_s-=(data.escl_end_s-data.escl_start_s+1);
	
	if(data.escl_end_d!=0 && data.escl_start_d!=0)
		np_d-=(data.escl_end_d-data.escl_start_d+1);
	
	if(data.escl_end_a!=0 && data.escl_start_a!=0)
		np_a-=(data.escl_end_a-data.escl_start_a+1);
	
	if(data.escl_end_b!=0 && data.escl_start_b!=0)
		np_b-=(data.escl_end_b-data.escl_start_b+1);
	
	PatternData.npoints=np_s+np_d+np_a+np_b;
	
	if( PatternData.PackPattern != NULL )
	{
		delete [] PatternData.PackPattern;
	}
	
	PatternData.PackPattern = new float[PatternData.npoints*2];
	
	if( PatternData.Pattern != NULL )
	{
		delete[] PatternData.Pattern;
		PatternData.Pattern=NULL;
	}

	float* ptr = PatternData.PackPattern;

	#ifndef __LINEDISP
	for(i=0;i<data.npoints_s;i++)
	{
		if(i>=data.escl_start_s-1 && i<=data.escl_end_s-1)
		{
		continue;
		}
	
		if((i & 1)==0)
		{
			*(ptr++)=data.ox_s-data.displacement*0.5;
		}
		else
		{
			*(ptr++)=data.ox_s+data.displacement*0.5;
		}
		
		*(ptr++)=data.oy_s+i*data.dist_s;
	}
	
	for(i=0;i<data.npoints_d;i++)
	{
		if(i>=data.escl_start_d-1 && i<=data.escl_end_d-1)
		{
			continue;
		}
	
		if((i & 1)==0)
		{
			*(ptr++)=data.ox_d+data.displacement*0.5;
		}
		else
		{
			*(ptr++)=data.ox_d-data.displacement*0.5;
		}
	
		*(ptr++)=data.oy_d+i*data.dist_d;
	}
	
	
	for(i=0;i<data.npoints_b;i++)
	{
		if(i>=data.escl_start_b-1 && i<=data.escl_end_b-1)
		{
			continue;
		}
	
		*(ptr++)=data.ox_b+i*data.dist_b;
	
		if((i & 1)==0)
		{
			*(ptr++)=data.oy_b-data.displacement*0.5;
		}
		else
		{
			*(ptr++)=data.oy_b+data.displacement*0.5;
		}
	}
	
	for(i=0;i<data.npoints_a;i++)
	{
		if(i>=data.escl_start_a-1 && i<=data.escl_end_a-1)
		{
			continue;
		}
	
		*(ptr++)=data.ox_a+i*data.dist_a;
	
		if((i & 1)==0)
		{
			*(ptr++)=data.oy_a+data.displacement*0.5;
		}
		else
		{
			*(ptr++)=data.oy_a-data.displacement*0.5;
		}
	}
	#else
	for(i=0;i<data.npoints_s;i++)
	{
		if(i>=data.escl_start_s-1 && i<=data.escl_end_s-1)
		{
			continue;
		}
	
		*(ptr++)=data.ox_s;
		*(ptr++)=data.oy_s+i*data.dist_s;
	}
	
	for(i=0;i<data.npoints_a;i++)
	{
		if(i>=data.escl_start_a-1 && i<=data.escl_end_a-1)
		{
			continue;
		}
	
		*(ptr++)=data.ox_a+i*data.dist_a;
		*(ptr++)=data.oy_a;
	}
	
	for(i=(data.npoints_d-1);i>=0;i--)
	{
		if(i>=data.escl_start_d-1 && i<=data.escl_end_d-1)
		{
			continue;
		}
	
		*(ptr++)=data.ox_d;
		*(ptr++)=data.oy_d+i*data.dist_d;
	}
	
	for(i=(data.npoints_b-1);i>=0;i--)
	{
		if(i>=data.escl_start_b-1 && i<=data.escl_end_b-1)
		{
			continue;
		}
	
		*(ptr++)=data.ox_b+i*data.dist_b;
		*(ptr++)=data.oy_b;
	}
	
	#endif

  	return(1);

}

void DosatClass::SetInterruptable(int val)
{
  Interruptable=val;
}

/*--------------------------------------------------------------------------
Esegue il pattern di dosaggio alle coordinate indicate
INPUT:   x,y: coordinate centro del pattern
GLOBAL:	-
RETURN:  -
NOTE:    dealloca il pattern di dosaggio se presente
--------------------------------------------------------------------------*/
int DosatClass::DoPattern(float x,float y,int point_restart)
{
	int i,retval=1,n=1;
	char buf[20];
	PatternData.ox=x;
	PatternData.ox=y;

	if(PatternData.Pattern==NULL)
		return(1);

	/*
	print_debug("pointer = %p\n\n",PatternData.Pattern);

	print_debug("Pattern\n");
	for(i=0;i<PatternData.npoints;i++)
	print_debug("%f %f\n",PatternData.Pattern[i*2],PatternData.Pattern[i*2+1]);
	print_debug("\n");
	*/

	lastpoint = point_restart-1;

	//calcolo del numero di punti effettivi fino a point_restart
	for(i=0;i<point_restart;i++)
	{
		if((PatternData.Pattern[i*2]==DOSNOPOINT) || (PatternData.Pattern[i*2+1]==DOSNOPOINT))
			continue;
		n++;
	}

  //dosaggio punti
#ifdef __LINEDISP
  for(i=point_restart;i<PatternData.npoints-1;)
#else
  for(i=point_restart;i<PatternData.npoints;i++)
#endif
  {
    if((PatternData.Pattern[i*2]==DOSNOPOINT) || (PatternData.Pattern[i*2+1]==DOSNOPOINT))
    {
      continue;
    }
    else
    {
      if(C_NPoint!=NULL)
      {
        sprintf(buf," %3d/%3d",n,PatternData.npoints-nodosapointCounter);
        C_NPoint->SetTxt(buf);
        n++;
      }
    }

    if(!PressStatus())
    {
      return(0);
    }

    if(!GoPosiz(x+PatternData.Pattern[i*2],y+PatternData.Pattern[i*2+1]))
    {
      if(Interruptable)
      {
        return(0);
      }
      else
      {
        retval=0;
      }
    }
    
    Wait_PuntaXY();
    WaitAxis();

    #ifndef __LINEDISP
    if(!ActivateDot(PackData[CurDisp].quant))
    {
      if(PointDosaFlag)
      {
        lastpoint=i;
      }

      if(Interruptable)
      {
        return(0);
      }
      else
      {
        retval=0;
      }
    }
    else
    {
      if(PointDosaFlag)
      {
        lastpoint=i;
      }    
    }
    #else
    int end_point;

    if(!ActivateLine(i,end_point,x,y,PackData.quant))
    {
      if(PointDosaFlag)
      {
        lastpoint=end_point;
      }

      if(Interruptable)
      {
        return(0);
      }
      else
      {
        retval=0;
      }
    }
    else
    {
      if(PointDosaFlag)
      {
        lastpoint=end_point;
      }    
    }

    i=end_point;
    #endif
  }



  lastpoint=-1;

  return(retval);

}

int DosatClass::GetLastPoint(void)
{
  return(lastpoint);
}

int DosatClass::GetPatternNPoint(void)
{
  return(PatternData.npoints);
}

DosatClass *Dosatore;
