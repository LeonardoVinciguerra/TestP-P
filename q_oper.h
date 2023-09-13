/*
>>>> Q_OPER.H

Dichiarazione delle funzioni esterne per modulo di gestione
delle funzioni operative.

Ulteriori info in Q_OPER.CPP/Q_OPERT.H/Q_HEAD.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++    Modificato da WALTER 16.11.96

*/

#if !defined(__Q_OPER_)
#define __Q_OPER_

#include "c_window.h"

#include "tv.h"
#include "c_combo.h"
#include "q_tabe.h"

#ifdef __UBUNTU18
#define mem_pointer	unsigned long
#else
#define mem_pointer	unsigned int
#endif

//costanti lettura input bus
#define REED_UGE 1
#define INSPARE1 0

//costanti check asse z
#define    MAXZERROR       10  //massimo numero di passi di errore in z
#define    ZAXISAUTORESET     2
#define    ZAXISNOAUTORESET   0
#define    ZAXISMSGON         1  //attiva mostra messaggi
#define    ZAXISMSGOFF        0  //non mostrare alcun messaggio

#define P1_ZDIRMOD +1  //fattore di modifica direzione Z per punta 1

#ifdef __SNIPER
#define P2_ZDIRMOD +1  //fattore di modifica direzione Z per punta 2
#endif

#define SCARICACOMP_NORMAL 0
#define SCARICACOMP_DEBUG  1

#define ZCHECK_GOENDBRD    0
#define ZCHECK_NOGOENDBRD  1


//tipi movimento asse z
#define SET_POS      0
#define ABS_MOVE     1
#define REL_MOVE     2
#define RET_POS      3

#define ZLIMITON     1 //abilita limiti asse z
#define ZLIMITOFF    0 //disabilita limiti asse z


//tipi di rotazione
#define BRUSH_RESET  1
#define BRUSH_ABS    2
#define BRUSH_REL    0

#define DOSA_UP     0
#define DOSA_DOWN   1

//costante per il settaggio di acc/vel. a valori di default (funzioni PuntaR* e PuntaZ*)
#define SET_DEFAULT			1
//costante per il settaggio di acc/vel. a valori di default (funzione SetNozzleXYSpeed_Index)
#define ACC_SPEED_DEFAULT	-1

//check presenza componente
#define CHECKCOMPFAIL_LAS        0x10  //bitmask check laser fallito
#define CHECKCOMPFAIL_VACUO      0x20  //bitmask check vacuo fallito
#define CHECKCOMP_MASK           0x01  //bitmask return check flag

#define DISP2_PRESS_SWITCH_CMD  1 //CMD2

#define DISP2_VACUOGEN_CMD      2 //CMD3

#define SAFE_COMPONENT_ROTATION_LIMIT		5.0 //rotazione limite in gradi, sotto la quale il componente puo' essere ad ogni quota




#define AUTOAPP_NOFINEC            0x000001
#define AUTOAPP_COMP               0x000002
#define AUTOAPP_NOCAM              0x000004
#define AUTOAPP_NOSTART_ZSECURITY  0x000008
#define AUTOAPP_PUNTA1ON           0x000010
#define AUTOAPP_PUNTA2ON           0x000020
#define AUTOAPP_ONLY1KEY           0x000040
#define AUTOAPP_NOUPDATE           0x000080
#define AUTOAPP_VECTORIAL          0x000100
#define AUTOAPP_DOSAT              0x000200
#define AUTOAPP_MAPOFF             0x000400
#define AUTOAPP_UGELLO             0x000800
#define AUTOAPP_NOZSECURITY        0x001000
#define AUTOAPP_NOZMOVE_LONGXY     0x002000
#define AUTOAPP_PACKBOX            0x004000
#define AUTOAPP_NOEXITRESET        0x008000
#define AUTOAPP_CONTROLCAM3        0x010000
#define AUTOAPP_EXTCAM             0x020000
#define AUTOAPP_NOLIMIT            0x040000
#define AUTOAPP_XYROT              0x080000
#define AUTOAPP_CALLBACK_PACK1     0x100000
#define AUTOAPP_CALLBACK_PACK2     0x200000
#define AUTOAPP_DIAMTEACH     		0x400000
#define AUTOAPP_CONVEYOR     		0x800000


// Caricatore pronto (0)
extern int Check_Caric(void);

bool CheckSecurityInput();
#ifdef HWTEST_RELEASE
bool CheckSecurityInput_HT();
#endif
int CheckProtectionIsWorking(void);

// Controllo fine corsa
int CheckLimitSwitchX();
int CheckLimitSwitchY();
// Interfaccia: check fine corsa - ritorna 0 per fine corsa basso (attivo)
int FinecStatus();

// Pressione aria (0)
int Check_Press(int force=0);

//ritorna true se il generatore del vuoto relativo alla punta e' attivo, false altrimenti
bool isVacuoOn(int punta);
// Comando contropressione
void Set_Contro (int cpunta, int ccomm);
// Comando vuoto
void Set_Vacuo (int vpunta, int vcomm);
// contropressione prima del prelievo del comp.
void Prepick_Contro(int p);

#ifdef __DISP2

void SwitchDispenserPressure(int ndisp);

void SetDispVacuoGeneratorOn(int ndisp);

void SetDispVacuoGeneratorOff(void);

#endif

// Selezione caricatore/rotellina e avanzamento caricatore.
void Set_CaricMov(unsigned int Car, unsigned int Weel, int Type);

//lettura input bus
int Read_Input(int num);

// Interfaccia: da codice caric. composto ad azionamento.
void CaricMov(int Cod_car, int Type, int carType, int carAdd, int carAtt); //THFEEDER

// Controlla fattibilita' movimento assi
bool Check_XYMove( float X_mov, float Y_mov, int mode = 0 );
// Movimento assi
int NozzleXYMove( float X_mov, float Y_mov, int mode = 0 );
int NozzleXYMove_N( float x_posiz, float y_posiz, int pn, int mode = 0 );
// Attesa fine movimento assi
bool Wait_PuntaXY( int t_tempo = 0 );

//GF_TEMP
// Interfaccia: setta posizione iniziale
bool SetHome_PuntaXY();

// Interfaccia: zero macchina manuale
bool ZeroMaccManSearch();
// Interfaccia: zero macchina automatico
bool ZeroMaccAutoSearch();

// Interfaccia: autoapprendimento vari
extern int ManualTeaching( float* X_zs, float* Y_zs, const char* title, unsigned int mode = 0, int camera = CAMERA_HEAD, int nozzle = 1 );

// Interfaccia: check pressione aria
extern int PressStatus(int tipo=0);
// Interfaccia: attende il segnale di ready del caricatore
extern int CaricWait( int carType, int carAdd ); //THFEEDER
// Interfaccia: controlla stato del caricatore
extern int CaricCheck( int carType, int carAdd ); //THFEEDER
// Interfaccia: calibrazione zero macchina                      // WALTER 16.11.96
bool ZeroMaccCal(float &x_position, float &y_position);

//GF_30_05_2011 - Interfaccia: calibrazione camera esterna tramite immagine punta
int ExtCamPositionSearch();

void Init_VacuoReading(void);

// Lettura valore vuoto
extern int Get_Vacuo(int Tipo);

// Sposta la testa a fine campo di lavoro - Add W09
void HeadEndMov(void);
// Sposta la testa a inizio campo di lavoro - Add W09
void HeadHomeMov(void);
// Ritorna flag attivazione controllo finecorsa
int Is_FinecEnabled(void);
// Forza abilitazione/disabilitazione finecorsa, impedendo cambiamenti
void EnableForceFinec(int state);
// Ripristina modo standard per l'abilitazione/disablitiazione dei fiecorsa
void DisableForceFinec(void);
// Abilita protezione tramite finecorsa
void Set_Finec(int mode);

// accende uscite comandi 0..3. mode=0 OFF/1=ON
void Set_Cmd(int num,int mode);
//ritorna 1=motore in movimento/0=motore fermo
int Check_PuntaZPos(char punta);

float GetPhysZPosMm(unsigned int nozzle);

//Movimento punte
int PuntaZPosStep(int xpunta, int pos, int mode=ABS_MOVE,int limitOn=ZLIMITON);
//Movimento punte con conversione automatica da mm a passi
float PuntaZPosMm(int punta,float mm,int mode=ABS_MOVE,int limitOn=ZLIMITON);
//attesa fine movimento punta
void PuntaZPosWait( int punta );


void Switch_UgeBlock(void);
void Set_UgeBlock(int status);


//Setta start/stop asse z
void PuntaZStartStop( int nozzle, int speed );
//Setta velocita asse Z
void PuntaZSpeed( int nozzle, int speed );
//Setta accel. asse Z
void PuntaZAcc( int nozzle, int acc );

//Setta velocita rotazione (asse theta)
void PuntaRotSpeed( int nozzle, int speed );
//Setta accel. rotazione (asse theta)
void PuntaRotAcc( int nozzle, int acc );

//Setta accelerazione e velocita' assi XY
void PuntaXYAccSpeed( int acc, int speed );

void ForceSpeedsSetting();
void SetNozzleXYSpeed_Index( int index );
void SetMinNozzleXYSpeed_Index( int index1, int index2 );
void SetNozzleZSpeed_Index( int nozzle, int index );
void SetNozzleRotSpeed_Index( int nozzle, int index );

void SaveNozzleXYSpeed();
void RestoreNozzleXYSpeed();
void SaveNozzleRotSpeed( int nozzle );
void RestoreNozzleRotSpeed( int nozzle );



//ritorna coordinate ultimo movimento
extern float GetLastXPos(void);
extern float GetLastYPos(void);



//preleva ugello
void UgePrel(int x_uge, int x_punta, struct CfgUgelli *CK_Pins);

int ScaricaCompOnTray(class FeederClass *feeder,int nozzle,int prelQuant=-1,int mode=SCARICACOMP_NORMAL,const char *cause=NULL);
int ScaricaComp(int xx_punta,int mode=SCARICACOMP_NORMAL,const char *cause=NULL);

void Set_OnFile( int val );
int Get_OnFile();

//deposita componente utilizzando i parametri contenuti in struttura deposito
//ritorna depdat aggiornata
int DepoComp( struct DepoStruct* depdat );
//preleva componente, scendendo a mmprel. Risalendo porta il componente a mmup. definisce vel/acc. di pickup
int PrelComp(struct PrelStruct preldat);
//Ritorna stato sebsire di traslazione per punta.
int Check_PuntaTraslStatus(int punta);
//Test iniziale punte up. Ritorna passi tra fine punta e z sensore
float InitPuntaUP(int punta);
//attesa punta in posizione di sicurezza per movimento assi
void WaitPUP(int punta);
// Reset punte/contropress./vuoto
void ResetNozzles(void);
//check rotazione : 0 in movimento, 1 motore fermo
int Check_PuntaRot( int punta );

//check rotazione effettivamente terminata (con encoder)
int Wait_EncStop(int punta,int time,int timeMax,unsigned int delta); //SMOD061003
int Wait_EncStop(int punta); //SMOD061003

// ritorna posizione assoluta
int GetPuntaRotStep( int nozzle );
int GetPuntaRotDeg( int nozzle );

//ruota punta di passi specificati.
int PuntaRotStep( int passi, int punta, int mode = BRUSH_ABS );
//rotazione con angolo espresso in gradi di punta.
//mode=(vedi void Set_PuntaRotStep(...))
float PuntaRotDeg(float degree_angle,int punta,int mode=BRUSH_ABS);

//check presenza componente sulla punta
int Check_CompIsOnNozzle( int punta, char checkMode );

//notifica errore di prelievo componente
int NotifyPrelError(int punta,int flag,const struct SPackageData& pack,const char *comp,C_Combo* combo=NULL,void(*fUpdateComp)(int&)=NULL,void(*fLoop)(void)=NULL);


int VacuoFindZPosition( int punta, int step, int& pos, int posLim );

// Accende/spegne illuminazione telecamera sulla testa
void Set_HeadCameraLight( int status );


void PuntaZSecurityPos(int punta,float offset=0);


// Mappatura offset teste W2604
int M_offset(void);
//funzioni autoapprendimento componenti
int Prg_Autoall(float *X_all, float *Y_all, int mode);
int Prg_Autocomp(int mode, int rec,TPrgFile *TPrg=NULL,struct TabPrg *ptrRec=NULL);

//check perdita passo in asse z
//TODO - obsolete
int CheckZAxis(int punta,int errmode=ZAXISMSGON | ZAXISNOAUTORESET);
int GetZCheck_LastErr(int punta);

// Controlla perdita passo sulle punte e riazzera se necessario
int CheckNozzlesLossSteps();


//struttura dati per prelievo componente
struct PrelStruct
{
	int punta;  //punta da utilizzare
	
	float zprel; //quota di prelievo
	float zup;   //quota alla quale risalire
	
	int waitup;
	int* downpos;
	
	int(*stepF)(const char *,int,int);    //puntatore a funzione gestione passo-passo

	struct SPackageData* package; //dati del package

	class FeederClass* caric;
};


//struttura dati per deposito componente
struct DepoStruct
{
	int punta;  //punta da utilizzare

	float xpos; //coordinate di deposito
	float ypos;
	float zpos; //quota di deposito

	int   rot_step;                   //passi di rotazione richiesti;
	unsigned char rot_mode;           //modalita di rotazione BRUSH_ABS/BRUSH_REL (default)

	int(*stepF)(const char *,int,int);    //puntatore a funzione gestione passo-passo

	unsigned char mounted;        //flag componente montato SI/NO->
								//0 se procedura abbandonata prima di deposito componente
								//1 "     "            "     dopo       "         "

	struct SPackageData* package; //dati del package

	int waitup;

	float predepo_zdelta;

	int xymode;

	int goposFlag;
	int dorotFlag;

	int vacuocheck_dep;
};

#define DEPOSTRUCT_DEFAULT  {1,0,0,0,0,BRUSH_REL,NULL,0,NULL,1,-1,0,1,1,0}

#ifdef __SNIPER
#define PRELSTRUCT_DEFAULT  {1,0,0,1,NULL,NULL,NULL,NULL}
#endif

void ResetZAxis(int punta);

void doZCheck(int ZCheck_punta,int mode=ZCHECK_GOENDBRD);


#define APPRENDZPOS_VACUO        1
#define APPRENDZPOS_NOVACUO      0
#define APPRENDZPOS_DELTAMODE    2 //SMOD300503
#define APPRENDZPOS_NOXYZERORET  4

int AutoAppZPosMm( const char* title, int nozzle, float x, float y, float &z, int mode=APPRENDZPOS_NOVACUO, float zmin=0, float zmax=0, struct CarDat* carRec = 0 );
int AutoAppZPosStep(const char* title,char* tip, int nozzle,float x,float y,int &z,int mode=APPRENDZPOS_NOVACUO );

int GetPackageOnNozzle( int nozzle, struct SPackageData& pack );

void SetPackageOnNozzle( int nozzle, const struct SPackageData& pack );
void RemovePackageFromNozzle( int nozzle );
void RemovePackageFromNozzle_SNIPER( int nozzle ); // **** SOLO SNIPER ****

int CheckNozzlesUp(void);
int WaitReedUge(int status);

//Controlla perdita passo sulle punte e riazzera se necessario
int CheckNozzleOk(int punta=0);

//Controlla la posizione p con la posizione di sicurezza della punta
//specificata, notifica se necessario differenza se superiore a limite
//e aggiorna se richiesto la posizione di sicurezza
void CheckHeadSensPosChanged(float p,int nozzle);

float GetXYApp_LastZPos(int punta);

void ResetXYStatus(void);
void ResetXYVariables(void); //resetta variabili di controllo movimenti x/y

void SetAlarmMode(int mode);
int  GetAlarmMode(void);
void StartAlarm(int alarm);
void StopAlarm(int alarm);

int DoWarmUpCycle( CWindow* parent );
void CheckWarmUp(void);

float GetZCaricPos(int nozzle);

float GetPianoZPos(int nozzle);

void DoZCheckStepLossShortCut(void);
void DoZCheckShortCut(void);

float GetCCal(int punta,float z);
float GetCCal(int punta);

float GetZRotationPointForPackage( const struct SPackageData& pack );
float GetComponentSafeUpPosition( const struct SPackageData& pack, unsigned int nozzle = 0 );
float GetComponentSafeUpPosition(unsigned int nozzle);
bool isComponentOnNozzleTooBig(unsigned int nozzle);
void PuntaRotDeg_component_safe(unsigned int nozzle,float r,int mode = BRUSH_ABS,bool gosafe = true);
void PuntaRotStep_component_safe(unsigned int nozzle,int r,int mode = BRUSH_ABS,bool gosafe = true);
void MoveComponentUpToSafePosition(unsigned int nozzle,bool force_movement = false);
float GetComponentSafeUpPosition(unsigned int nozzle);
bool isCurrentZPosiziontOkForRotation(unsigned int nozzle);
float GetComponentOnNozzleRotation(unsigned int nozzle);

int RotationTest(int punta,float pos,float idle_time = 400); //TEST_THETA_LOG

float getSniperType1ComponentLimit(unsigned int nsniper,const CenteringReservedParameters& par);
float getSniperType2ComponentLimit(unsigned int nsniper,const CenteringReservedParameters& par);

void SetConfirmRequiredBeforeNextXYMovement(bool r);

bool IsConfirmRequiredBeforeNextXYMovement(void);

void EnableTimerConfirmRequiredBeforeNextXYMovement(bool e);

void ResetTimerConfirmRequiredBeforeNextXYMovement(void);

bool isTimerConfirmRequestedBeforeNextXYMovementElapsed(void);

static inline bool UseSaferProtection(void)
{
	return (!CSecurityReservedParametersFile::inst().get_data().flags.bits.open_protection_mov_enabled);
}

// Gestione autoapprend. posizione scarico componenti. - W09
int Sca_auto(void);

#endif
