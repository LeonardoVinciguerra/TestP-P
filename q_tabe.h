
#ifndef __Q_TABE_H
#define __Q_TABE_H


#include <sys/stat.h> //SMOD30503
#include "q_cost.h"

#include "q_gener.h"
#include "q_files.h"

#pragma pack(1)


#define CHECK_HEADER   		1
#define NOCHECK_HEADER 		0

//definizione maschere per parametri record prog. di assemblaggio
#define MOUNT_MASK    0x0001
#define SPARE1_MASK   0x0002
#define RDEL_MASK     0x0004
#define SPARE2_MASK   0x0008
#define DODOSA_MASK   0x0010
#define DODOSA2_MASK  0x0020
#define DUP_MASK      0x0080
#define SPARE3_MASK   0x0020
#define RMOV_MASK     0x0100
#define NOMNTBRD_MASK 0x0200
#define FID1_MASK     0x0400
#define FID2_MASK     0x0800
#define VERSION_MASK  0x8000

#define MODROT_MASK  0x0001
#define MODX_MASK    0x0002
#define MODY_MASK    0x0004
#define MODP_MASK    0x0008
#define MOD_MASK     0x0020

//modo di apertura files
#define SKIPHEADER   0
#define NOSKIPHEADER 1
//modo di scrittura dati su files
#define WR_APPEND  1
#define WR_REPLACE 0
//modo flush in write
#define FLUSHON  1
#define FLUSHOFF 0
//definisce record=NONE
#define NOREC   -1

//costanti di query per check se un file e' aperto
#define HANDLE_ZERI     0  //check file zeri
#define HANDLE_PACK     1  //check file packages

#define CCAL_ABSERR     0.2 //massimo errore assoluto del CCal (in mm)


//definizione maschere per bitmap modo di funzionamento
#define SCANPREL1_MASK         0x01      //laser scan test su prelievo con p1
#define SCANPREL2_MASK         0x02      //  "    "    "    "    "      "  p2
#define NOLASCENT_MASK         0x04      //disabilita centraggio con laser
#define ENABLE_XYMAP           0x10
#define ENABLE_CARINT          0x40   //abilita caricatori inteligenti

#define SCANERR1_MASK          0x80   //laser scan test su err.laser "  p1
#define SCANERR2_MASK         0x100   //  "    "    "    " err.laser "  p1
#define ASK_PACKAGE_CHANGE	  0x200   //abilita riconoscimento ottico dei caricatori
#define ONLY_SMARTFEEDERS     0x400   //utilizza esclusivamente smart feeders
#define ENABLE_ALARMSIGNAL    0x800   //abilita segnalatore di allarme esterno

//nei 4 bit piu significativi delle modalita puo' essere mappato un debug mode
#define DEBUGVAL_MASK   0xF000

// Nessun componente, check e assemblaggio impossibili.
#define	NORPROG			MsgGetString(Msg_00227)

//costanti modo debug 2
#define DEBUG2_LASERERR            1
#define DEBUG2_BIGCROSS            2   //abilita croce con quadrati
#define DEBUG2_UGECHECKOFF        16   //disabilita controllo presa ugello
#define DEBUG2_RETRYSWEEPONERR    32   //abilita ulteriore sweep di centraggio se il primo fallisce //TODO: mantenere o togliere ???
#define DEBUG2_SMARTFEED_DBG     128   //abilita debug caricatori intelligenti
#define DEBUG2_VACUOCHECK_DBG    256   //abilita modo per riportare errori di mancata presenza componenti
                                       //con controllo con vuoto ad ogni singolo tentativo
//#define DEBUG2_NO_CONFIRM_XY_MOV 512   //se attivo disattiva la richiesta di conferma prima dei movimenti xy
                                       //per evitare partenze accidentali della macchina                                    
//costanti modo debug 1
#define DEBUG1_CPUBEEPERR       1 //beep su errore CPU
#define DEBUG1_CPUSHOWERR       2 //mostra comunicazioni CPU se errore
#define DEBUG1_CPUSHOWALL       4 //mostra tutte le comunicazioni CPU
#define DEBUG1_WAITENCGR        8 //mostra grafico encoder ad ogni attesa fine rotaz.
#define DEBUG1_WAITENCGR_ONERR 16 //mostra grafico encoder dopo attese fine rotaz. in caso di errore.
#define DEBUG1_REPEATXY_TEST   64 //abilita test ripetibilita assi //SMOD080404
#define DEBUG1_XYTIMEOUT_DBG  128 //disabilita solo gli assi XY dopo un errore di timeout //SMOD24056


#define COD_FIELD      0x0001
#define TIPO_FIELD     0x0002
#define PX_FIELD       0x0004
#define PY_FIELD       0x0008
#define PUNTA_FIELD    0x0010
#define ROT_FIELD      0x0020
#define MOUNT_FIELD    0x0040
#define DOSA_FIELD     0x0080
#define NOTE_FIELD     0x0100
#define VERSION_FIELD  0x0200
#define PACK_FIELD  	0x0800
#define ADDED_REC      0x8000
#define ALL_FIELD      0xFFFF

#define FIDUCIAL1_TEXT	"FIDUCIAL 1"
#define FIDUCIAL2_TEXT	"FIDUCIAL 2"
#define FIDUCIAL1_SET	MsgGetString(Msg_05180)
#define FIDUCIAL2_SET	MsgGetString(Msg_05181)

//numero massimo di ricerca riferimenti in modo quadrotto. Superato il limite viene eseguito il ciclo normale e il contatore riportato a 0
#define MULTICIRCUIT_COUNTER_DEFAULT  4

#define FOPENLIST_MAX_SIZE  100

#define MAX_VISCENTER_DISPL		7.0

struct FOpenItem
{
	char name[MAXNPATH+1];
	struct FOpenItem* prev;
	struct FOpenItem* next;
};

struct TabPrg
{
	short int Riga;                  // num. di riga
	char CodCom[17];           // codice compon.
	char TipCom[26];           // tipo compon.
	float XMon;                // coord. x montaggio
	float YMon;                // coord. y montaggio
	float Rotaz;               // rotazione
	char Punta;                // punta
	short int Caric;                 // cod. caricatore

	short int status;

	char Uge;

	short int scheda;                // numero scheda al quale il record e' riferito

	unsigned char flags;

	char spare1;

	char pack_txt[21];         // nome package in ASCII
	char NoteC[41];            // note

	char scheda_mount;

	char spareSort[3];         // reserved ! Utilizzato da PrgSort

	char spare2[19];

	short int Changed;
};


// Struttura dei campi dell'header dei files tabella di montaggio
struct FileHeader
{
	char F_nome[45];
	char F_aggi[45];
	char F_note[45];
};

// Struttura dei campi dei files zeri scheda
struct Zeri
{
	short int Z_scheda;           // Nr. scheda ( 0 = master )
	float Z_xzero;          // X,Y zero scheda
	float Z_yzero;
	float Z_rifangle;       // Angolo teta (radianti)
	char Z_note[26];        // note
	short int Z_ass;              // flag: assemblare si/no
	short int Z_serv;             // servizio: libero per usi futuri
	float Z_xrif;           // X,Y riferimento
	float Z_yrif;
	float Z_boardrot;       // Angolo teta (radianti)
	float Z_scalefactor;    // fattore di scala (usato solo per board master)
	char Z_spare[9];
};

struct BackupInfoDat
{
	short int year;
	char month;
	char day;
	
	char hour;
	char min;
	
	int type;
};


//---------------------------------------------------------------------
//Dati di default per CfgHeader
//---------------------------------------------------------------------

#define ZEROUGELLI_DEF      24.0
#define DEBUG1_DEF          DEBUG1_CPUSHOWERR | DEBUG1_CPUBEEPERR
#define DEBUG2_DEF          0
#define INTERFERNOR_DEFAULT 1.0
#define INTERFFINE_DEF      1.0
#define SPEED_AXES_DEF      1500
#define ACC_AXES_DEF        10000
#define SPEED_Z_DEF         2000
#define ACC_Z_DEF           20000
#define SPEED_ROT_DEF       100000
#define ACC_ROT_DEF         140000
#define TSCARICCOMP_DEF     25
#define TCONTROCOMP_DEF     15
#define PASSOX_DEF          0.02
#define PASSOY_DEF          0.02
#define BARISOGLIA_DEF      180
#define TREADVACUO_DEF      400
#define TIME1_DEF           30
#define TIME2_DEF           260
#define MODAL_DEF           ENABLE_XYMAP
#define ENCSTEP1_DEF        4096
#define ENCSTEP2_DEF        2048
#define KSTEP1_DEF          54.6
#define KSTEP2_DEF          54.6
#define ZPIANO_DEF          20.0
#define ZCARIC_DEF          18.0
#define ZCENTR_DEF          22.0
#define ZINK_DEF            12.0
#define LASOUT_DEF          -7.0
#define TCONTROUGE_DEF      5
#define CCAL1_DEF           16.59
#define CCAL2_DEF           27.23
#define BRUSHMAXCUR_DEF     5
#define BRUSHMAXCURTIME_DEF 2
#define THOFFUGE_DEF        90.0
#define ZTHSEARCH_DEF       13.0
#define UGEWAIT_DEF         400
#define UGEZACC_DEF         5000
#define UGEZVEL_DEF         85
#define UGEZSTART_DEF       17
#define MAXNOZH1_DEF        25.30
#define MAXNOZH2_DEF        25.30
#define MINNOZH1_DEF        -23.50
#define MINNOZH2_DEF        -13.30
#define XPASSOLIMIT_DEF     0.014
#define YPASSOLIMIT_DEF     0.02
#define PREDOWNDELTA_DEF    1.0
#define DISPVACUOGEN_DEF    200

#define MAX_SOFTPICK_DELTA  20



// Struttura dei campi del file di configurazione.
// Mod. by Walter 10/97 per nuova configurazione - W09
struct CfgHeader
{
	char Cli_Default[9];
	char Prg_Default[9];

	float Zero_Ugelli2;        // Posizione zero, prelievo ugelli (punta 2)

	unsigned char PrelVacuoDelta;

	char spare1[5];

	unsigned short int debugMode1;
	unsigned short int debugMode2;
	
	float interfNorm;          //DANY171002
	float interfFine;
	
	char Conf_Default[9];      //  "      "   car.    "
	char Lib_Default[9];       //  "      "   lib. package caricata

	unsigned short xyMaxSpeed;
	unsigned short xyMaxAcc;
	unsigned short zMaxSpeed;
	unsigned short zMaxAcc;
	int rotMaxSpeed;
	int rotMaxAcc;

	unsigned char CurDosaConfig[2];
	unsigned char brushlessNPoles[2];

	unsigned char softPickSpeedIndex;

	char spare2;
	
	short int Dis_vuoto;     // Disattivaz. vuoto
	short int TContro_Comp;  // Tempo contropressione per deposito comp.

	char spare3[6];

	float PassoX;      // Passo X default
	float PassoY;      // Passo Y default

	char spare4[18];

	short int D_vuoto;       // tempo lettura vuoto - W09

	short int Time_1;
	short int Time_2;

	float automaticPickHeightCorrection;

	short int modal;         // bitmap modalita di funzionamento

	char spare6[10];

	short int Enc_step1;     // passi encoder punta 1
	short int Enc_step2;     // passi encoder punta 2
	float Step_Trasl1; // passi traslazione punta 1
	float Step_Trasl2; // passi traslazione punta 2
	
	float Zero_Piano;  // Posizione zero, piano di lavoro
	float Zero_Caric;  // Posizione zero, caricatori
	char spare7[4];
	float Zero_Ugelli; // Posizione zero, prelievo ugelli
	
	float InkZPos;     // Quota Z inchiostro per offset teste-telecamera

	float uge_offs_x[2];
	float uge_offs_y[2];

	unsigned short DispVacuoGen_Delay;

	char spare9[2];
	
	#ifdef __SNIPER
	float Z12_Zero_delta;
	#endif

	char spare10[4];

	float SoftPickDelta;
	int  FoxPos_HeadSens[2]; //posizione fox corrispondente alla quota si sicurezza
	
	float LaserOut;    // Quota posizionamento punta fuori dal laser
	
	short int  TContro_Uge;

	char spare11[10];

	short int  zertheta_Prerot[2];

	int brushMaxCurrent;
	int brushMaxCurrTime;

	char spare12[4];
	
	float thoff_uge1;       //angolo di offset prelievo ugello con punta 1
	float thoff_uge2;       // "     "     "       "       "    "    "   2
	
	float zth_level1;       //quota z per zero theta search (p1)
	float zth_level2;       //quota z per zero theta search (p2)
	
	#ifdef __SNIPER
	float sniper1_thoffset;
	#endif
	
	int uge_wait;            //tempo di attesa in risalita cambio ugello
	int uge_zacc;            //acc. punta in risalita cambio ugello
	int uge_zvel;            //vel. punta in risalita cambio ugello
	int uge_zmin;            //vel. start/stop in risalita cambio ugello
	
	float DMm_HeadSens[2];   //delta in mm tra fine terminale e sensore traslazione

	char spare13[20];

	//DANY200103
	//Massima e minima altezza a cui far scendere e salire le punte a partire
	//dallo zero in z (punta su zero laser)
	float Max_NozHeight[2];
	float Min_NozHeight[2];
	
	int DStep_HeadSens[2];   //delta in step tra fine terminale e sensore traslazione
	
	char spare14[8];

	float predownDelta;

	unsigned char BrushSteady_Step[2];
	unsigned char ComponentDwellTime;

	char spare15[13];

	float UgeCamRef_XPos;
	float UgeCamRef_YPos;
	
	char spare16[4];
	
	#ifdef __SNIPER
	float sniper2_thoffset;
	float sniper_kpix_um[2];
	#endif

	char spare17[80];
};

// Struttura dei campi del file di memorizzazione parametri
struct CfgParam
{
	unsigned short xySpeedFactor; // Fattore di riduzione velocita' assi XY

	char spare1[4];

	float CamPunta1Offset_X;     // Offset X telecamera-punta 1
	float CamPunta1Offset_Y;     // Offset Y telecamera-punta 1

	float CamPunta2Offset_X;     // Offset X telecamera-punta 2
	float CamPunta2Offset_Y;     // Offset Y telecamera-punta 2

	char spare1_a[8];

	char  AutoOptimize;
	float DeltaUgeCheck;
	char  LogMode;

	char  spare2[2];

	short int US_file;                 // Uscita su file

	char  spare2_a[2];

	short int AS_cont;                 // Assemb. continuato
	
	char spare3[2];
	
	short int AT_vuoto;                // Attiv. sens. vuoto
	short int AT_press;                // Attiv. sens. pressione aria
	short int AZ_tassi;                // Azzer. autom. testa su assi
	
	char spare4[2];
	
	short int AS_ottim;                // Attivazione ottimizzazione
	
	char spare5[2];
	
	float LX_maxcl;              // Limite campo lavoro X  max
	float LY_maxcl;              // Limite campo lavoro Y  max
	float LX_mincl;              // Limite campo lavoro X  min
	float LY_mincl;              // Limite campo lavoro Y  min
	float PX_scaric;             // Posizione di scarico componente
	float PY_scaric;

	float OFFX_mark;             // posizione punto di mark
	float OFFY_mark;

	float X_endbrd;              // Pos. fine scheda
	float Y_endbrd;
	
	char spare6[16];
	
	short int   vuoto_1;               // vuoto max punta 1
	short int   vuoto_2;               // vuoto max punta 2
	float X_zmacc;               // Pos. zero macchina
	float Y_zmacc;

	float X_colla;               // Pos. punto colla X - wmp1
	float Y_colla;               // Pos. punto colla Y - wmp1
	short int Dispenser;               // Dispenser si/no        L251099
	short int N_try;                   // N. tentativi per deposito componente //SMOD240907

	short int Autoref;                 // Ricerca automatica riferimenti scheda Integr. Loris
	
	char spare7[4];
	
	float OFFX_telec1[2];         // offset x punta 1 -telecamera 1/2
	float OFFY_telec1[2];         // offset y punta 1 -telecamera 1/2
	float OFFX_telec2[2];         // offset x punta 2 -telecamera 1/2
	float OFFY_telec2[2];         // offset y punta 2 -telecamera 1/2
	float OFFX_ink;               // posizione contenitore inchiostro
	float OFFY_ink;
	
	int  velp1;                   // velocita   asse z punta 1
	int  accp1;                   // acceleraz.   "  "  "    1
	int  velp2;                   // velocita   asse z punta 2
	int  accp2;                   // acceleraz.   "  "  "    2
	
	int prot1_acc;               // Accelerazione rotazione punta 1
	int prot2_acc;               // Accelerazione rotazione punta 2
	int prot1_vel;               // Velocita rotazione punta 1
	int prot2_vel;               // Velocita rotazione punta 2
	
	short int Trasl_VelMin1;            //velocita start/stop asse z asse 1
	short int Trasl_VelMin2;            //velocita start/stop asse z asse 2
	
	float EncScale_Movement_x;    // movimento X calibrazione scala encoder
	float EncScale_Movement_y;    // movimento Y calibrazione scala encoder

	char spare8[2];
	
	short int DemoMode;                 //modalita demo
	short int ZPreDownMode;             //modalita' prediscesa in prelievo/deposito componenti
	
	float ZCheckPos[2];           //posizione della bussola per check asse z
	
	float OrthoXY_Movement_x;     // movimento X calibrazione ortogonalita' assi
	float OrthoXY_Movement_y;     // movimento Y calibrazione ortogonalita' assi
	float OrthoXY_Error;          // fattore di corrrezione ortogonalita' assi
	
	float AuxCam_X[2];            // posizione X telecamera aux
	float AuxCam_Y[2];            // posizione Y telecamera aux

	char spare9[4];

	int Disable_LaserCompCheck;   // disabilita tutti i check di presenza comp.

	float AuxCam_Scale_x[2];
	float AuxCam_Scale_y[2];

	float AuxCam_Z[2];            // quota z telecamera aux

	float EncScale_x;             // scala encoder X
	float EncScale_y;             // scala encoder Y
};


#define NORMAL_DISPENSER      0
#define VOLUMETRIC_DISPENSER  1

// Struttura dei campi del file di configurazione dosatore.

struct CfgDispenser
{
	unsigned short VacuoPulse_Time;
	unsigned short VacuoPulseFinal_Time;

	unsigned char speedIndex;	// Vel. max

	char spare0[7];

	short int DosaMov_Time;		// Tempo attesa assi fermi
	short int GlueOut_Time;		// Tempo attesa uscita colla
	short int GlueEnd_Time;		// Tempo attesa fine dosaggio punto prima di risalita
	short int DosaZMovDown_Time;	// Tempo movimento z dosatore
	
	short int Viscosity;			// viscosita'
	char NPoint;			// num. punti dosaggio iniziale
	char spare1;				// flag record valido (occpuato)
	
	unsigned char tipoDosat;               //tipo dosatore:0=normale 1=volumetrico
	unsigned short int Inversion_Time;     //Durata inversione motore volumetrico
	unsigned short int DosaZMovUp_Time;    //Tempo attesa salita siringa
	#ifndef __DISP2
	unsigned short int PreInversion_Time;
	#else
	unsigned short int AntiDropStart_Time;
	#endif
	
	char name[8];             //Nome del set di dosaggio (senza fine stringa !! 8 caratteri effettivi !!!)
	char Note[25];            //note dosatore
	
	float CamOffset_X;
	float CamOffset_Y;
	
	float X_colla; //SMOD090403
	float Y_colla; //SMOD090403
	
	unsigned short VacuoWait_Time;

	//GF_05_07_2011
	int TestPointsOffset; // distanza punti dosaggio iniziale lungo asse Y

	char spare2[60];
};


#define UG_P1   1
#define UG_P2   2
#define UG_ALL  3

// Struttura dei campi della tabella degli ugelli.
struct CfgUgelli
{
	char U_code;                  // Cod. ugello (A-Z)
	char U_tipo[25];              // Tipo ugello
	float X_ugeP1;                // Coord. X punta 1
	float Y_ugeP1;                // Coord. Y punta 1
	unsigned short int MinVLevel;
	float Z_offset[2];            // Z-offset ugello
	char NozzleAllowed;           // punte a cui e' consentito prelevare l'ugello
	char utype;                   // tipo ugello SMOD141003
	char U_note[21];              // Note
	
	float X_ugeP2;                // Coord. X punta 2
	float Y_ugeP2;                // Coord. Y punta 2
	
	char spare1[12];
};


//SMOD141003
struct CfgUgeDim
{
	int index;
	char name[25];
	float a;           //diametro foro
	float b;           //diametro terminale 1
	float c;           //altezza terminale 1
	float d;           //diametro terminale 2
	float e;           //altezza terminale 2
	float f;           //diametro terminale 3
	float g;           //altezza terminale 3
	float spare1[10];
};

//database tipi ugelli di default
#define DEF_NUGEDIM 6
const struct CfgUgeDim defUgeDim[DEF_NUGEDIM]=
{
	{0,"A",9,10,9,10,9,10,0},
	{1,"B",3,6.5,9,10,9,10,0},
	{2,"C/D",2.4,2.45,9,10,9,10,0},
	{3,"E/I",1.6,1.3,4,2.5,10,7,0},
	{4,"F/J",1.6,0.9,4,2.5,10,7,0},
	{5,"G/H/K",1.6,0.7,4,2.5,10,7,0}
};

//Tempi di avanzamento caricatori
struct CarTimeData
{
	char name[3];
	unsigned short int start_ele;
	unsigned short int start_motor;
	unsigned short int start_inv;
	unsigned short int end_ele;
	unsigned short int end_motor;
	unsigned short int end_inv;
	unsigned short int wait;
	int spare1[8];
};


// Indici delle tabelle di correzione angoli per deposito con sniper e telecamera
#define TABLE_CORRECT_SNIPER      0
#define TABLE_CORRECT_EXTCAM      1

struct CfgTeste
{
	CfgTeste()
	{
		for( int i = 0; i < 2; i++ )
		{
			ccal_z_cal_m[i] = 0.f;
			ccal_z_cal_q[i] = 0.f;

			T1_90[i] = 0;
			T1_180[i] = 0;
			T1_270[i] = 0;
			T1_360[i] = 0;

			T2_90[i] = 0;
			T2_180[i] = 0;
			T2_270[i] = 0;
			T2_360[i] = 0;
		}
	}

	float ccal_z_cal_m[2];
	float ccal_z_cal_q[2];
	
	short int T1_90[2];         // passi per angoli testa 1
	short int T1_180[2];
	short int T1_270[2];
	short int T1_360[2];
	
	short int T2_90[2];         // passi per angoli testa 1
	short int T2_180[2];
	short int T2_270[2];
	short int T2_360[2];
};

// Struttura dei parametri di ottimizzazione - DANY101002 - SMOD180303
struct CfgOpt
{
	float HorizDim;
	float VertDim;
	float CarrOff;
	float PCBOff;
	float PesoUge;
	int   PesoCaric;
};

// Struttura dei parametri del test hw centraggio  - DANY191102
struct CentTestParam
{
	float xcam;
	float ycam;
	float zcam;
	float xcomp;
	float ycomp;
	float zcomp;
	float rcomp;
	int carcode;
	float cam_dx;
	float cam_dy;
};

struct LasParam
{
	float xcam;
	float ycam;
	float zcam;
	float xcomp;
	float ycomp;
	float zcomp;
	float rcomp;
};


struct CfgBrush
{
	int p[2];
	int i[2];
	int d[2];
	int clip[2];
};

// Struttura dei parametri di ricerca zero macchina automatico.
struct zer_auto
{
	short int zer_vel;     // velocita' assi
	short int zer_acc;     // accelerazione assi
	short int zer_ram;     // freq. start/stop assi
	float zer_stp;   // lunghezza steps in mm (scontro mswitch)
	float zer_dis;   // lunghezza steps in mm (distacco)
	short int zer_del;     // ritardo tra i passi
};

// Struttura della tabella di mappatura offset teste.  W2604
struct map_off
{
	short int x;   // delta asse X
	short int y;   // delta asse Y
};


// memo parametri vuoto in ram
struct v_vuoto
{
	short int min_vuoto;
	short int liv_vuoto;
	short int sog_vuoto;
	short int max_vuoto;
};

#define PKVIS_LIGHT_DEF       7
#define PKVIS_SHUTTER_DEF     3
#define PKVIS_MATCH_THR_DEF   0.65f
#define PKVIS_ITERAZ_DEF      2

struct PackVisData
{
	char name[21];

	float xp[2][4];
	float yp[2][4];

	float zoff;

	float match_thr;
	unsigned char niteraz;

	unsigned char light[2];
	unsigned char shutter[2];

	//DB270117
	float centerX[2];
	float centerY[2];

	char spare[84];
};


struct PackDosData
{
	short int code;

	//dati fisici package
	char nameX[21];
	
	float oy_b;  //origine riga bassa
	float ox_b;
	float oy_a;  //origine riga alta
	float ox_a;
	float oy_d;  //origine riga destra
	float ox_d;
	float oy_s;  //origine riga sinistra
	float ox_s;
	
	//distanza tra i punti
	float dist_b; //riga bassa
	float dist_a; //riga alta
	float dist_d; //riga destra
	float dist_s; //riga sinistra
	
	//numero di punti
	int npoints_b; //riga bassa
	int npoints_a; //riga alta
	int npoints_d; //riga destra
	int npoints_s; //riga sinistra
	
	//intervalli di esclusione
	short int escl_start_a; //riga alta
	short int escl_end_a;
	short int escl_start_b; //riga bassa
	short int escl_end_b;
	short int escl_start_d; //riga destra
	short int escl_end_d;
	short int escl_start_s; //riga sinistra
	short int escl_end_s;
	
	short int quant;        //quantita colla
	
	char note[25];
	
	float displacement;
	
	char spare1[12];
};

// dati correnti
struct cur_data
{
	unsigned short int HeadBright;
	unsigned short int HeadContrast;
	short int Cross;
	short int U_p1;  // ugello in punta 1
	short int U_p2;  // ugello in punta 2
	unsigned char extcam_light;
	unsigned char extcam_gain;
	unsigned char extcam_shutter;

	char spare1[13];
};

#define DEFAULT_WARMUP_THRESHOLD    20
#define DEFAULT_WARMUP_ENABLESTATE   0
#define DEFAULT_WARMUP_DURATION      1

struct WarmUpParams
{
	int enable;
	int duration;
	int threshold;
	int acceleration;
};

//Struct tabella degli algoritmi laser
#define MAXTLASREC 20
struct dlas_type
{
	short int code;
	float holdoff1;
	float holdoff2;
	float holdoff3;
	float holdoff4;
	short int algo1;
	short int algo2;
	short int algo3;
	short int algo4;
	float alimit1;
	float alimit2;
	float alimit3;
	float alimit4;
};

// Struttura dei dati dei file .dta
struct dta_data
{
	float dosa_x;
	float dosa_y;
	int  n_board;
	int  n_comp;
	char  lastlib[9];
	char  lastconf[9];
	float PCBh;
	char  zersearch_mode;
	char  matching_mode;
	char  spare1[15];
	#ifdef __DISP2
	float dosa_x2;
	float dosa_y2;
	#else
	float spare2[2];
	#endif
};

#define CONV_MOVE_DEF				0.0
#define CONV_SPEED_DEF				600
#define CONV_ACC_DEF				6000
#define CONV_STEPMM_DEF				35.6228
#define CONV_ZEROIN_DEF				1
#define CONV_LIMITIN_DEF			2
#define CONV_MINCURR_DEF			400
#define CONV_MAXCURR_DEF			1600
#define CONV_MINPOS_DEF				0.0
#define CONV_MAXPOS_DEF				698.0
#define CONV_SLOWSEARCHSPEED_DEF	80
#define CONV_FASTSEARCHSPEED_DEF	300

// Struttura dei dati dei parametri convogliatore
struct conv_data
{
	char  enabled;
	float zeroPos; //n.u.
	float refPos;  //n.u.
	char  step1enabled;
	char  cust1[9];
	char  prog1[9];
	char  conf1[9];
	char  lib1[9];
	float move1;
	char  step2enabled;
	char  cust2[9];
	char  prog2[9];
	char  conf2[9];
	char  lib2[9];
	float move2;
	char  step3enabled;
	char  cust3[9];
	char  prog3[9];
	char  conf3[9];
	char  lib3[9];
	float move3;
	int  speed;
	int  accDec;
	float stepsMm;
	char  zero;
	char  limit;
	int   minCurr;
	int   maxCurr;
	float minPos;
	float maxPos;
	char  spare1[50];
};


// Struttura dei dati parametri immagine
struct img_data
{
	short int image_type;
	short int pattern_x;
	short int pattern_y;
	short int atlante_x;
	short int atlante_y;
	unsigned char vect_smooth;
	unsigned char vect_edge;
	short int match_iter;
	short int vect_atlante_x;
	short int vect_atlante_y;
	float match_thr;
	char spare1[8];
	short int filter_type;
	short int filter_p1;
	short int filter_p2;
	short int filter_p3;
	float vect_diameter;
	float vect_tolerance;
	float vect_accumulator;
	unsigned short int contrast;
	unsigned short int bright;
};

// Struttura dei dati parametri visione
struct vis_data
{
	// parametri riconoscimento pattern mappatura
	unsigned short circleDiameter;		// diametro cerchi espresso in centesimi di millimetro
	unsigned short circleTolerance;   // tolleranza sul diametro dei cerchi espressa in centesimi di millimetro
	unsigned short circleFSmoothDim;
	unsigned short circleFEdgeThr;
	float circleFAccum;

	float match_err;           // errore max match in millimetri
	float scale_off;           // setscale offset X e Y

	unsigned char matchSpeedIndex;
	unsigned char visionSpeedIndex;

	char spare0[8];

	unsigned short wait_time;    // tempo attesa assi fermi
	unsigned short image_time;   // tempo acquisizione intera immagine (2 frame)
	unsigned short debug;        // debug visione
	float pos_x;               // posizione punto inizio calibrazioni
	float pos_y;
	float mapoff_x;            // offset punti mappatura
	float mapoff_y;
	unsigned short mapnum_x;  // n. punti asse x mappatura
	unsigned short mapnum_y;  // n. punti asse y mappatura

	char spare1[8];

	float mmpix_x;          	// costante millimetri pixel cam head
	float mmpix_y;

	unsigned short rectX;				// dimensione X espressa in centesimi di millimetro
	unsigned short rectY;  	 		// dimensione Y espressa in centesimi di millimetro
	unsigned short rectTolerance;		// tolleranza su dimensione espressa in centesimi di millimetro
	unsigned short rectFSmoothDim;
	unsigned short rectFBinThrMin;
	unsigned short rectFBinThrMax;
	unsigned short rectFApprox;		// valore espresso in centesimi

	char spare2[2];

	float scalaXCoord;
	float scalaYCoord;
};

#ifdef __GNUC__
  #pragma pack()
#endif


// Copia il file gia' aperto (CP_File_Handle) in CP_Name.
extern int CopyFileOLD(const char *CP_Name, int CP_File_Handle);
//Copia file fornendo nome originee destinazione
extern int CopyFileOLD(const char *fdest,const char *forig);
// Create new files - Funz. di supporto alle create programma/caricatori.
// Ritorna FALSE per creazione fallita.
/*  con gestione errore hardware per disco protetto **W0204 */
int F_File_Create ( int &N_Handle, const char *NomeFile , bool lock = false);
// Open files - Funz. di supporto alle open programma/caricatori.
// Ritorna FALSE per open fallito.
/* Bug fixed in error if not file found (O_CREAT) - W09 */
int F_File_Open ( int &N_Handle, const char *NomeFile,int mode=CHECK_HEADER, bool ignore_case = false, bool lock = false);

//ritrorna versione e subversion di un file Quadra Laser
int Get_FileVersion_OLD(int Handle,unsigned int &version,unsigned int &subversion);


//***********************************************************************

extern CfgUgelli DatUgelli[MAXUGE];

int Uge_SaveRec(CfgUgelli &H_Uge,int n);
int Uge_ReadRec(CfgUgelli H_Uge,int n);
void Uge_ReadAll(CfgUgelli *uge);
void Uge_SaveAll(CfgUgelli *uge);
int Uge_Open(void);
void Uge_Close(int save=0);


//************************************************************************
// Gestione file tabella programma
//************************************************************************

#define PRG_CODSEARCH  1
#define PRG_TIPSEARCH  2
#define PRG_PACKSEARCH 4
#define PRG_ALLSEARCH  PRG_CODSEARCH | PRG_TIPSEARCH | PRG_PACKSEARCH

#define PRG_ADDPATH     0x00 //aggiunge pathname
#define PRG_NOADDPATH   0x80 //non aggiunge pathname

#define PRG_NORMAL     0
#define PRG_ASSEMBLY   1
#define PRG_DATA       2
#define PRG_DOSAT      3
#define PRG_ZER        4
#define PRG_ZERDATA    5
#define PRG_LASTOPT    6
#define PRG_ZER_MC     7

class TPrgFile
{
private:
	int Prgoffset;
	int HandlePrgTab;
	char NomeFile[MAXNPATH];
	
	struct TabPrg *SearchBuf;
	
	struct FileHeader PrgHdr;
	int hdrUpdateOnWrite;
	

public:
	int FLen(void);
	int Open(int OpenType=SKIPHEADER,bool ignore_case = false);
	int Close(void);
	int Count(void);
	int Read(TabPrg &ITab, int RecNum);
	void InitSearchBuf(void);
	void DestroySearchBuf(void);
	int Search(int start,struct TabPrg tab,int mode=PRG_ALLSEARCH,int scheda=0);
	int Create(void);
	int WriteLast(short int Ultimo);
	int FindLast(void);
	int Replace(TabPrg &ITab, int RecNum, int Flush,int Mode);
	int Write(TabPrg tab,int RecNum,int flush=FLUSHON);
	int GetHandle(void) { return(HandlePrgTab); }
	int IsOnDisk(bool ignore_case = false);
	void DupRec(int dup_rec);
	void InsRec(int ins_rec,struct TabPrg tab);
	int  DelSel(void);
	void DelRec(int del_rec);
	void Change(int ch_src, int ch_dest);
	void MoveRec(int nrec,int newpos);
	void Reduce(int n=1);
	void WriteHeader(FileHeader F_Header);
	void ReadHeader(FileHeader &F_Header);
	void GetHeader(FileHeader &F_Header);
	void UpdateHeader_OnWrite(int flag);
	
	TPrgFile(const char *NomeFile,int tipo);
	~TPrgFile(void);
};

//***********************************************************************


//************************************************************************
// Dichiarazioni delle funzioni per zeri scheda
//************************************************************************

#define ZER_NOADDPATH 0
#define ZER_ADDPATH   1

class ZerFile
{ protected:
    int Offset1,Offset2;
    int Handle;
    char pathname[MAXNPATH];
	bool create_if_not_exixts;
  public:
	ZerFile(char *NomeFile,int mode=ZER_ADDPATH,bool _create_if_not_exixts=false);
    ~ZerFile(void);
    int  Create(void);
    int  Open(int OpenType=SKIPHEADER,bool ignore_case = false);
    void Close(void);
    int  IsOnDisk(void);
    int  GetLen(void);
    int  GetHandle(void);
    int  GetNRecs(void);
    int  GetNAssBoard(void);    
    int  Reduce(int Size);
    int  DtaWrite(Zeri dat,int Flush);
    int  DtaRead(Zeri &dat);
    int  Write( Zeri& ZTab, int RecNum );
    int  Read(Zeri &ZTab, int RecNum);
    int  ReadAll(struct Zeri *zerList,int n);
    void GetVersion(unsigned int &version, unsigned int &subversion);
    void Update(unsigned int actVer, unsigned int actSubver);
};


//************************************************************************
// Dichiarazioni delle funzioni per file .DAT
//************************************************************************


void Save_Dta(struct dta_data val,char *nomefile=NULL);
void Read_Dta(struct dta_data *val,char *nomefile=NULL);

// legge altezza pcb
void Read_PCBh(float &PCBh,char *nomefile=NULL);
#ifndef __DISP2
// salva posizione punto colla L0709
void Save_pcolla(float X_colla, float Y_colla,char *nomefile=NULL);
// legge posizione punto colla L0709
void Read_pcolla(float &X_colla, float &Y_colla,char *nomefile=NULL);
#else
// salva posizione punto colla
void Save_pcolla(int ndisp,float X_colla, float Y_colla,char *nomefile=NULL);
// legge posizione punto colla
void Read_pcolla(int ndisp,float &X_colla, float &Y_colla,char *nomefile=NULL);
#endif
// salva n. board e comp del programma in uso
void Save_nboard(int nboard,int ncomp,char *nomefile=NULL);
// legge n. board e comp del programma in uso
void Read_nboard(int &nboard,int &ncomp,char *nomefile=NULL);
// salva modalita' di riconoscimento fiduciali del programma in uso
void Save_matchingMode(char mode,char *nomefile=NULL);
// legge modalita' di riconoscimento fiduciali del programma in uso
void Read_matchingMode(char &mode,char *nomefile=NULL);
//salva nomi file config. x programma
void Save_PrgCFile(const char *lib,const char *conf,const char *nomefile=NULL);
//legge nomi file config. x programma
void Read_PrgCFile(char *lib,char *conf,char *nomefile=NULL);

//************************************************************************
// Dichiarazioni delle funzioni per mappatura offset teste
//************************************************************************
// Legge/crea il file di mappatura offset teste. W2604
int Init_off(void);
// Modifica il valore di un elemento di mappatura offset teste W2604
void Mod_off(int posizione, int valore_x, int valore_y);

// Ritorna il valore dell'elemento di mappatura in mm
int Read_offmmN(float &valore_x, float &valore_y, int nozzle,float rot,int centering=0);

// Ritorna il valore dell'elemento di mappatura in passi relativo all'indice
//della tabella di mappatura.
int Read_off(int &valore_x, int &valore_y, int posizione);

// Aggiornamento del file mappatura offset teste. W2604
void Update_off(void);

// Reset della tabella mappatura offset teste. W2604
void Reset_off(void);

//************************************************************************
// Dichiarazioni delle funzioni per file visione
//************************************************************************
// Caricamento file dati parametri visione Integr. Loris
bool VisDataLoad(struct vis_data &Vision);
// Salvataggio file dati parametri visione Integr. Loris
bool VisDataSave(struct vis_data Vision);


//************************************************************************
// Dichiarazioni delle funzioni per file parametri immagini
//************************************************************************

// Caricamento file dati parametri immagine
int ImgDataLoad( char* filename, img_data* data );
// Salvataggio file dati parametri immagine
int ImgDataSave( char* filename, img_data* data );
// Elimina file dati parametri immagine
bool ImgDataDelete( char* filename );


//************************************************************************
// Dichiarazioni delle funzioni per file di configurazione
//************************************************************************

// Scrive i dati nel file di configurazione / crea se inesistente
extern int Mod_Cfg(CfgHeader &H_Cfg);
// Legge i dati dal file di configurazione.
extern int Read_Cfg(CfgHeader &H_Cfg);

//************************************************************************
// Dichiarazioni delle funzioni per file parametri di lavoro
//************************************************************************

// Scrive i dati nel file memo parametri / crea se inesistente
extern int Mod_Par(CfgParam &H_Par);
// Legge i dati dal file parametri.
extern int Read_Par(CfgParam &H_Par);

//***************************************************************************
// Dichiarazioni delle funzioni per parametri test hw centraggio- DANY191102
//***************************************************************************

// Modifica i dati nel file dei parametri del test hw laser
extern int Mod_CentTest(const CentTestParam &param);
// Crea il file dei parametri del test hw laser
extern int Create_CentTest(void);
// Legge i valori dal file dei parametri del test hw laser
extern int Read_CentTest(CentTestParam& param);

//************************************************************************
// Dichiarazioni delle funzioni per file parametri convogliatore
//************************************************************************

// Modifica i dati  / crea se inesistente.
extern int Mod_Conv( struct conv_data convVal );
// Legge i valori dal file
extern int Read_Conv( struct conv_data* convVal );

//************************************************************************
// Dichiarazioni delle funzioni per file mappatura rotazione testa
//************************************************************************

// Modifica i dati nel mappatura teste. / crea se inesistente.
extern int Mod_Map(CfgTeste &H_Map);
// Legge i valori dal file mappatura teste.
extern int Read_Map(CfgTeste &H_Map);

int Read_ThetaMap( CfgTeste& H_Map );

//************************************************************************
// Dichiarazioni delle funzioni per file dati correnti
//************************************************************************

//crea file dati correnti
extern int CurData_Crea(void);
//apre file dati correnti (crea se non esiste)
extern int CurData_Open(void);
//chiude il file dei dati correnti
extern int CurData_Close(void);
//legge struttura dati correnti
extern int CurData_Read( cur_data& Dat );
//scrive struttura dati correnti
extern int CurData_Write( cur_data Dat );

//************************************************************************
// Dichiarazioni delle funzioni per file assemblaggio SMOD081002
//************************************************************************
extern int AssPrg_Write(TabPrg &ITab, int Flush);
extern void AssPrg_Close(void);
extern int AssPrg_Create(char *NomeFile);
extern int AssPrg_Open(char *NomeFile, int OpenType);
extern int AssPrg_Read(TabPrg &ITab, int RecNum);


//**************************************************************************
// Gestione file configurazione Brushless
//**************************************************************************

void BrushDataSave(struct CfgBrush data,int n);
void BrushDataRead(struct CfgBrush &data,int n);
void BrushDataSave(struct CfgBrush *data);
void BrushDataRead(struct CfgBrush *data);


//SMOD141003
//**************************************************************************
// Gestione file dimensioni ugelli
//**************************************************************************
#define MAXUGEDIM 50

int UgeDimCreate(void);
int UgeDimOpen(void);
int UgeDimClose(void);
int UgeDimWrite(CfgUgeDim data,int n);
int UgeDimRead(CfgUgeDim &data,int n);
int UgeDimWriteAll(CfgUgeDim *data);
int UgeDimReadAll(CfgUgeDim *data);

//**************************************************************************
// Gestione file dati package
//**************************************************************************
void PackVisData_GetFilename( char* filename, int pack_code, char* libname );
int PackVisData_Open(char *path,char *name=NULL);
int PackVisData_Remove( int pack_code, char* libname );
void PackVisData_Close(void);
void PackVisData_Write(struct PackVisData data);
void PackVisData_Read(struct PackVisData &data);


//**************************************************************************
// Gestione file parametri rete
//**************************************************************************

int LoadNetPar(struct NetPar &nwpar);
int WriteNetPar(struct NetPar nwpar);
int CreateNetPar(void);

void FList_AddItem(const char *fname);
void FList_RemoveItem(const char *fname);
int FList_Count(void);
struct FOpenItem *FList_GetHead(void);
void FList_Flush(void);
void FList_Read(void);
void FList_Save(void);


//**************************************************************************
// Gestione file tempi di avanzamento caricatori
//**************************************************************************

int CarTime_WriteDefault(void);

void CarTime_Close(void);
int  CarTime_Create(void);
int  CarTime_Open(void);
int  CarTime_Read(struct CarTimeData &dta,int n);
int  CarTime_Write(struct CarTimeData dta,int n);
int  CarTime_NRec(void);

//**************************************************************************
// Gestione file parametri di warmup
//**************************************************************************

int  WarmUpParams_Read(struct WarmUpParams &data);
int  WarmUpParams_Write(struct WarmUpParams data);
int  WarmUpParams_Create(void);
int  WarmUpParams_Open(void);
void WarmUpParams_Close(void);


//**************************************************************************
// File gestione backup su rete
//**************************************************************************

int ReadMBkpInfo(int &data,int mode,bkp::e_dest dest, const char* mnt = NULL);
int  WriteMBkpInfo(int data,int mode,bkp::e_dest dest, const char* mnt  = NULL);

void GetMBkpInfo_FileName(char *buf,int mode,bkp::e_dest dest, const char* mnt);

int ReadBackupInfo(struct BackupInfoDat &data,int mode,int n,bkp::e_dest dest, const char* mnt = NULL);
int WriteBackupInfo(struct BackupInfoDat data,int mode,int n,bkp::e_dest dest, const char* mnt = NULL);

void GetBackupInfo_FileName(char *buf,int mode,int nidx,bkp::e_dest dest, const char* mnt);

int OpenMachinesList(void);
int CreateMachinesList(void);
int CloseMachinesList(void);
int GetLastMachineID(void);
int AddMachineName(char *name);
int SearchMachineName(char *name);
int GetMachineName(int n,char *name);


//**************************************************************************
// Modi di funzionamento sniper
//**************************************************************************

#define MAX_SNIPERMODES_NREC                       64

#pragma pack(1)
typedef struct
{
	int idx;
	char name[16];
	int speed;
	int acceleration;
	int search_angle;
	int prerotation;
	int discard_nframes;
	int algorithm;
	int spare1[16];
} SniperModeData;
#pragma pack()



class SniperModes
{
	protected:
		int m_nrec;
		SniperModeData m_rec;
		int m_invalidated;

		int m_handle;
		int m_start_offset;

		SniperModes(void);
		~SniperModes(void);

		int openFile(void);
		int closeFile(void);
		int createFile(void);
		int readFile(int nrec,SniperModeData& data);
		int writeFile(int nrec,const SniperModeData& data);

	public:
		static SniperModes& inst() { static SniperModes me; return me;}

		int isOk(void);
		int checkOk(void);
		int getNRecords(void);
		int getRecord(int nrec,SniperModeData& data);
		int updateRecord(int nrec,SniperModeData& data);
		int getRecord(SniperModeData& data);
		const SniperModeData& getRecord(void);
		int updateRecord(SniperModeData& data);
		void useRecord(int nrec);
};

//**************************************************************************
// Parametri riservati centratore ottico
//**************************************************************************

#pragma pack(1)
typedef struct
{
	float opt_cent_d1;
	float opt_cent_d2;
	float opt_cent_z1;
	float opt_cent_z2;
	float opt_cent_z3;
	float opt_cent_zrot;
	
	float sniper_type1_components_max_pick_error;
	float sniper_type2_components_max_pick_error;
	float sniper_type1_safety_margin;
	float sniper_type2_safety_margin;
	
} CenteringReservedParameters;

#pragma pack()

#define CENTR_CFG_PAR_DIM1							28.0f
#define CENTR_CFG_PAR_DIM2							40.0f
#define CENTR_CFG_PAR_Z1							4.5f
#define CENTR_CFG_PAR_Z2							3.0f
#define CENTR_CFG_PAR_Z3							7.0f
#define CENTR_CFG_PAR_BIG_COMP_ZROT					8.0f
#define CENTR_CFG_PAR_MIN_DIM_P1					13.0f
#define CENTR_CFG_PAR_MIN_DIM_P2					13.0f
#define CENTR_CFG_PAR_SNIPER_TYPE1_PICK_ERR			0.75f
#define CENTR_CFG_PAR_SNIPER_TYPE2_PICK_ERR			0.75f
#define CENTR_CFG_PAR_SNIPER_TYPE1_SAFETY_MARGIN	0.25f
#define CENTR_CFG_PAR_SNIPER_TYPE2_SAFETY_MARGIN	0.25f														

#define CENTR_CFG_PAR_DEFAULT 	{	CENTR_CFG_PAR_DIM1,\
									CENTR_CFG_PAR_DIM2,\
									CENTR_CFG_PAR_Z1,\
									CENTR_CFG_PAR_Z2,\
									CENTR_CFG_PAR_Z3,\
									CENTR_CFG_PAR_BIG_COMP_ZROT,\
									CENTR_CFG_PAR_SNIPER_TYPE1_PICK_ERR,\
									CENTR_CFG_PAR_SNIPER_TYPE2_PICK_ERR,\
									CENTR_CFG_PAR_SNIPER_TYPE1_SAFETY_MARGIN,\
									CENTR_CFG_PAR_SNIPER_TYPE2_SAFETY_MARGIN,\
								}


class CCenteringReservedParameters
{
	protected:

		int m_handle;
		int m_start_offset;
		CenteringReservedParameters m_data;

		CCenteringReservedParameters(void);
		~CCenteringReservedParameters(void);

		int openFile(void);
		int closeFile(void);
		int createFile(void);		
		
	public:
		static CCenteringReservedParameters& inst() { static CCenteringReservedParameters me; return me;}

		int readData(CenteringReservedParameters& data);
		int writeData(const CenteringReservedParameters& data);		
		const CenteringReservedParameters& getData(void);
		
		int isOk(void);
		int checkOk(void);

};

extern struct CfgParam  QParam;

//-----------------------------------------------------------------------------------------------

template<typename T>
class CQFile
{
	protected:
		unsigned int m_FileTypeVersion;
		unsigned int m_FileTypeSubversion;
		
		unsigned int m_ReadedFileTypeVersion;
		unsigned int m_ReadedFileTypeSubversion;		
		
		unsigned int m_FileMaxNRecs;
		std::string m_FileTitle;
		bool m_CreateFileIfNotExists;
		bool m_ignore_case;
		bool m_lock;
		
		std::string m_path;
		
		int m_handle;
		unsigned int m_data_start;
		
		//unsigned int m_version;
		//unsigned int m_subversion;	
					
		bool _open(const char* path);
		
		virtual bool private_create(void);
		virtual bool upgrade(void);
				
	public:
		CQFile(void);
		CQFile(const std::string& path);
		CQFile(const char* path);
		~CQFile(void);
						
		bool exists(void);
		bool is_open(void);
		bool create(void);
		bool open(void);
		bool open(const std::string& path);
		bool open(const char* path);
		bool close(void);
		
		bool readRec(T& rec,unsigned int nrec);
		bool writeRec(const T& rec,unsigned int nrec);	
			
		int countRecs(void);
};

//-----------------------------------------------------------------------------------------------

#pragma pack(1)
typedef struct
{
	union
	{
		struct
		{
			unsigned int open_protection_mov_enabled : 1;
			unsigned int emergency_stop_disabled : 1;
			unsigned int end_assembly_protection_check_disabled : 1;
			unsigned int mov_confirm_disabled : 1;
			unsigned int spare : 28;
		} bits;
		
		unsigned int mask;
	} flags;
	
	unsigned short mov_confirm_timeout;
} s_security_reserved_params;
#pragma pack()

class CSecurityReservedParametersFile : public CQFile<s_security_reserved_params>
{
	protected:
		bool m_dirty;
		s_security_reserved_params m_params;
		
		bool private_create(void);
		CSecurityReservedParametersFile(void);
		
	public:
		
		static CSecurityReservedParametersFile& inst(void) { static CSecurityReservedParametersFile srpf_inst; return srpf_inst; }
		bool readParameters(s_security_reserved_params& params);
		bool writeParameters(const s_security_reserved_params& params);
		const s_security_reserved_params& get_data(void);
};

//-----------------------------------------------------------------------------------------------


//GF_TEMP
//TODO: spostare tutte le funzioni operative in apposito file
struct RotCenterCalibStruct
{
	RotCenterCalibStruct()
	{
		nozzle = 1;

		for( int i = 0; i < 2; i++ )
		{
			pos_z[i][0] = 0.f;
			pos_z[i][1] = 0.f;
			rot_center[i][0] = 0.f;
			rot_center[i][1] = 0.f;
			delta_pos[i][0] = 0.f;
			delta_pos[i][1] = 0.f;
		}
	}

	int nozzle;
	float pos_z[2][2];
	float rot_center[2][2];
	float delta_pos[2][2];
};


#endif
