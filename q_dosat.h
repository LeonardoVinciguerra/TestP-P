/*
>>>> Q_DOSAT.H

Dichiarazioni delle funzioni esterne per gestione tabella packages.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1998    ++++

*/

#if !defined(__Q_DOSAT_)
#define __Q_DOSAT_

#include "q_cost.h"
#include "q_tabe.h"
#include "c_combo.h"

#ifndef __DISP2

#define  VOLUMETRIC_NDISPENSER 1

#else

#define  VOLUMETRIC_NDISPENSER 2

#endif

#define  NODOSACONF_LOADED MsgGetString(Msg_01746)
#define  DOSACONF_CREATED  MsgGetString(Msg_01888)
#define  DOSACONF_CREATED2 MsgGetString(Msg_05124) //TOCHECK

// Stop movimenti con protezione alzata.
#define	DOSPROTOPEN	MsgGetString(Msg_00818)		// L702a

#define  NODOSAZERO  MsgGetString(Msg_01337) //SMOD170403

// Header file .DIS parametri dosatore
#define TXTDISP1     "Dispenser 1 parameter file.      \n\n\r"

#define TXTDISP2     "Dispenser 2 parameter file.      \n\n\r"
#define TXTDOSAPACK  "Package dispenser data file.          \n\n\r"

#define ERR_DISPCFGFILE MsgGetString(Msg_01192)
#define ERR_DISPPKGFILE MsgGetString(Msg_01193)

#define DOSA_DEFAULTTIME 0
#define DOSA_DEFAULTVISC 0

#define DOSAPRESS_NOGOUP      0
#define DOSAPRESS_NOUSETYPE   0
#define DOSAPRESS_ALLOWZEROM  0
#define DOSAPRESS_NOANTIDROP  0

#define DOSAPRESS_GOUP    	1  		//risalita punta al termine dell'esecuzione di un punto (default)
#define DOSAPRESS_USETYPE 	2		//considera il flag tipoDosat dalla configurazione dispenser attiva (default)
#define DOSAPRESS_NOZEROM 	4		//impedisce dosaggio su zero macchina (default)
#define DOSAPRESS_ANTIDROP	8		//esegue ciclo antisgocciolio al terminde dell'esecuzione di un punto

//operazioni di default SetPressImpulse:
#define _DOSAPRESS_DEFAULT 	DOSAPRESS_USETYPE |  \
                          	DOSAPRESS_GOUP    | \
                          	DOSAPRESS_NOZEROM   

#ifndef __DISP2
#define DOSAPRESS_DEFAULT _DOSAPRESS_DEFAULT
#else
#define DOSAPRESS_DEFAULT _DOSAPRESS_DEFAULT | DOSAPRESS_ANTIDROP
#endif

//modo test in test hardware schede
#define DOSAPRESS_TESTMODE1 DOSAPRESS_NOUSETYPE | DOSAPRESS_NOGOUP | DOSAPRESS_ALLOWZEROM | DOSAPRESS_NOANTIDROP
//modo test di dosaggio
#define DOSAPRESS_TESTMODE2 DOSAPRESS_USETYPE | DOSAPRESS_NOGOUP | DOSAPRESS_NOZEROM


#define DOSADIR_NORMAL     0
#define DOSADIR_INVERTED   1

#define DISP_DEFSPEED         		2
#define DISP_DEFWAITXY         		100
#define DISP_DEFWAITDW          	50
#define DISP_DEFWAITUP          	50
#define DISP_DEFTESTTIME        	50
#define DISP_DEFWAITEND         	50
#define DISP_DEFPREINV          	50
#define DISP_DEFINVERSION       	20
#define DISP_DEFVISCOSITY      		100
#define DISP_DEFNPOINTS          	3
#define DISP_DEFVACUOPULSE          0

#define DISP_DEFVACUOPULSE_FINAL    900

#define DISP_DEFVACUO_WAIT          100

#define DISP_DEFANTIDROP_START      100

#define DISP_DEFTYPE             0
#define DISP_DEFNAME         "disp"

#define MOTOR_STARTSTOP_TIME    10

#define DOSAINV_SWITCH_TIME     10



#define DISP_VALVE_SWITCHTIME   50

#define DISP_EMPTY_PIPE         100

#define DISP_FILL_PIPE          100


#define DOSSTATUS_DOWN      1
#define DOSSTATUS_PRESS     2
#define DOSSTATUS_MOTOR     4
#define DOSSTATUS_INVERSION 8

#define DOSNOPOINT         -999

struct DosPattern
{ float *PackPattern;   //pattern del package
  float *Pattern;       //pattern da dosare
  float ox,oy;          //coordinate origine del pattern
  int npoints;
};

#define DOSAPACK_ADDPATH      1

#define DOSAPACK_NOADDPATH    0

class DosatClass
{
	private:
		int(*stepF)(const char *,int,int);	// Funzione di handling in modalita' passo-passo
		int Handle[2];
		int Offset[2];
		int HandlePack[2];
		int OffsetPack[2];
		int status[2];
		int movedown;
		int disabled;
		struct DosPattern PatternData;
		int initOk;
		char packfile[MAXNPATH];
		int Interruptable;
		int lastpoint;
		int PointDosaFlag;
		int nodosapointCounter;
		
		float curx[2];
		float cury[2];
		
		int CurDisp;
			
		int confLoaded[2];
		
		int protezOn;
	
		class C_Combo *C_XPos;
		class C_Combo *C_YPos;
		class C_Combo *C_NPoint;
	
		class FoxClass  *Fox;
	
		CfgDispenser DosaConfig[2];
		PackDosData  PackData[2];
		
		#ifdef __DISP2
		clock_t InversionStatChanged_Timer;
		clock_t MotorStatChanged_Timer;
		#endif


	public:
		DosatClass(FoxClass *foxptr);
		~DosatClass(void);
	
		void Enable(void);
		void Disable(void);
	
		void Set_StepF(int(*_stepF)(const char *,int,int));
	
		#ifdef __DISP2
		void SelectCurrentDisp(int ndisp);

		int  GetCurrentDisp(void);
		#endif


		void SetDefaultData(CfgDispenser& data);
		
		int OpenConfig(int ndisp);

		int CreateConfig(int ndisp);

		int CloseConfig(int ndisp);


		int ReadConfig(int ndisp,CfgDispenser &data,int nrec);

		int WriteConfig(int ndisp,CfgDispenser data,int nrec);

		int GetConfigNRecs(int ndisp);
		
		int IsConfigOpen(int ndisp);
		int IsConfLoaded(int ndisp);

		void GetConfigName(int ndisp,char *name);

		void GetConfig(int ndisp,CfgDispenser &data);

		CfgDispenser GetConfig(int ndisp);

		int ReadCurConfig(void);

		int CreatePackData(int ndisp,char *filename,int addpath=DOSAPACK_ADDPATH);

		int OpenPackData(int ndisp,char *filename,int addpath=DOSAPACK_ADDPATH);



		int LoadPackData(int ndisp,int rec);

		int WritePackData(int ndisp,int rec,struct PackDosData data);



		void GetPackData(int ndisp,PackDosData &data);

		PackDosData GetPackData(int ndisp);



		int IsPackDataOpen(int ndisp);

		int ClosePackData(int ndisp);
	
		int  CheckProtez(void);
		void Set_CheckProtezFlag(int val);
	
		int ActivateDot(int time=DOSA_DEFAULTTIME,int viscosity=DOSA_DEFAULTVISC,int ctrl_flags=DOSAPRESS_DEFAULT);
		#ifdef __LINEDISP
		int ActivateLine(int start,int &end,float x,float y,int time=DOSA_DEFAULTTIME,int viscosity=DOSA_DEFAULTVISC,int ctrl_flags=DOSAPRESS_DEFAULT);
		#endif
	
		int GoDown(void);
		int GoUp(int ndisp = 0);

		int SetInversion(int dir);
		int SetMotorOn(void);
		int SetMotorOff(void);
	
		int SetPressOn(int ndisp=0);
		int SetPressOff(int ndisp=0);
		//int SetPress(int ndisp,int val); //not used
		int SetPressImpulse(int time=DOSA_DEFAULTTIME,int viscosity=DOSA_DEFAULTVISC,int ctrl_flags=DOSAPRESS_DEFAULT);
	
		#ifdef __DISP2
		void DoVacuoFinalPulse(int ndisp=0);

		void DoVacuoPulse(int ndisp=0);

		void SetVacuoOn(int ndisp=0,int wait_long=0);

		void SetVacuoOff(int ndisp=0);
		#endif
		
		int PointRot(float *px,float *py,float rotaz);
	
		int  LoadPattern(int rec);
		void CreateDosPattern(float rotaz=0);
		void PatternRotate(float rotaz);
		void PatternSort(void);
		int  DoPattern(float x,float y,int point_restart=0);
		int  GoPosiz(float x_posiz,float y_posiz);
		void WaitAxis(void);
		int  GetLastPoint(void);
		int  GetPatternNPoint(void);
	
		void SetXYCombo(class C_Combo *Cx,class C_Combo *Cy);
		void SetNPointCombo(C_Combo *CPoint);
	
		int GetStatus(int ndisp);
		int CanXYMove(void);
		void EnableMoveDown(void);
		void DisableMoveDown(void);
	
		void SetInterruptable(int val);

};


extern DosatClass *Dosatore;
#endif
