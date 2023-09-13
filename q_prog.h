/*
>>>> Q_PROG.H

Dichiarazione delle funzioni di gestione della tabella di programma

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

*/

#if !defined (Q_PROG)
#define Q_PROG

#include <map>
#include "q_carobj.h"
#include "q_cost.h"
#include "q_graph.h"
#include "q_tabe.h"

#include "c_combo.h"
#include "c_win_table.h"
#include "gui_submenu.h"

#define CHECKPRG_COMPONENTS   (1 << 0)
#define CHECKPRG_PREASSEMBLY  (1 << 1)
#define CHECKPRG_FEEDERDB     (1 << 2)
#define CHECKPRG_ALIGN_TABLES (1 << 3) 
#define CHECKPRG_FULL         (CHECKPRG_COMPONENTS | CHECKPRG_ALIGN_TABLES | CHECKPRG_PREASSEMBLY | CHECKPRG_FEEDERDB)
#define CHECKPRG_DISP 		   (1 << 4)
#define CHECKPRG_DISPFULL     (CHECKPRG_COMPONENTS | CHECKPRG_ALIGN_TABLES | CHECKPRG_FEEDERDB | CHECKPRG_DISP)

extern int dosaAss;                  //flag dosaggio e assemblaggio in corso

#ifdef __DISP2
typedef enum
{
	DOSASS_IDX_NONE=0,
	DOSASS_IDX_DOSA1,
	DOSASS_IDX_ASSEM,
	DOSASS_IDX_DOSA2
} tDosAssIdx;

extern int dosa12Ass;
extern int Dosa12Ass_phase;
extern tDosAssIdx Dosa12Ass_seqvector[3];
#endif

// Main gestione programma di montaggio
extern int PrgEdit();
// Codici tastiera e selezioni programma di montaggio
extern int OnEdit(int Comando, int Reset = 0);
// Fine lavoro e off window programma di montaggio
void PrgEnd();
// Setta a on flag di edit
void ON_edit(void);
// Ritorna lo stato del flag di edit
int If_ON_edit(void);
// Lancio alla gestione assemblaggio ( in q_assem ) - main -
int GestAss(int rip_ass = 0); //LCHK

// Gestione autoapprendimento incrementi vassoi.
int auto_incr(void);
// ricerca codici caricatori del programma
// code=0 -> ricerca finita.
// code=1 -> componente non montato.
// mode=0 -> inizio programma.
int Search_car(int mode);
// Lancio alla gestione DOSAGGIO
#ifndef __DISP2

int GestDos(int rip,int stepmode,int nrip=-1,int interruptable=-1,int asknewDosa=1,int autoref=1,int enableContDosa=1,int disableStartPoints=0);
#else

int GestDos(int ndisp,int rip,int stepmode,int nrip=-1,int interruptable=-1,int asknewDosa=1,int autoref=1,int enableContDosa=1,int disableStartPoints=0);

#endif
// Setta il codice package dalla tabella packages nel record corrente - wmd0
void prg_set_pack_code(int pack_code,char *pack_txt);

// Ordinamento del database programma (bubble-sort). -- wmd1
void Sort_prg(int sort_field);

// Lancio alla gestione DOSAGGIO+ASSEMBLAGGIO
#ifndef __DISP2
int GestDosAss(int rip=0,int stepmode=0,int asknewDosa=1,int disableStartPoints=0);
#else
int GestDosAss(int ndisp,int rip=0,int stepmode=0,int asknewDosa=1,int disableStartPoints=0);
int GestDos12Ass(tDosAssIdx seq1,tDosAssIdx seq2,tDosAssIdx seq3,int rip=0,int stepmode=0,int asknewDosa=1,int disableStartPoints=0);
#endif

// Apprendimento pannello
int PrgM_Zeri(void);
// Incrementa e visualizza n. board programma in uso L230999
void inc_nboard(void);
// Incrementa e visualizza n. componenti programma in uso L230999
void inc_ncomp(void);
// Reset n. componenti programma in uso L071099
void Clear_nboard(int mode=0);

int PrgM_CreateNewFromX( char* newName, float xcoordInf, float xcoordSup, const char* title, float deltaZeroX = 0.0, float deltaZeroY = 0.0, float zerX = 0.0, float zerY = 0.0, float rifX = 0.0, float rifY = 0.0 );

void GetImageName( char* filename, int type, int num = 0, char* libname = 0 );
void AppendImageMode( char* filename, int type, int mode );
// Integr Loris
void SetImageName( char* filename, int type, int mode, int num = 0, char* libname = 0 );

#define COMPCHECK_NOERR       	0
#define COMPCHECK_NOCOMPTYPE 	-1
#define COMPCHECK_TRAYHEIGHT 	-2
#define COMPCHECK_NOCONF     	-3
#define COMPCHECK_ERRFEEDER  	-4
#define COMPCHECK_DIFF_PACKAGE 	-5

class CCheckPrg
{
public:
	CCheckPrg(CWindow* _wait_window,int _mode);
	~CCheckPrg(void);

	int Start(void);

protected:
	CWindow* wait_window;
	GUI_ProgressBar_OLD* checkprg_progbar;
	
	int mode;
	
	int progress_tot_phases;
	int progress_phase;
	int progress_size;
	
	
	int tprg_assembly_toupdate;
	int tprg_normal_nrec;
	int tprg_exp_nrec;
	
	struct TabPrg* tprg_normal;
	struct TabPrg* tprg_exp;
	
	struct CarDat feeders[MAXCAR];
	

	int SearchPackage(char *name);
	
	int FeederCheck(struct CarDat& car);
	int ComponentsCheck(int& first_err);
	int AlignMasterAssemblyTable(void);
	int PreAssemblyCheck(void);
	int FeederDBCheck(void);
	int AssignToolHelper( int nrec );
};


class CaricHistogram : public C_Graph
{
private:
	struct TabPrg* AllPrg;
	struct Zeri* AllZer;

	FeederFile* Car;

	struct CarDat AllCar[MAXCAR+1];
	int CarUsed[MAXCAR+1];
	float CarDistance[MAXCAR+1];

	int MagUsed[MAXMAG];
	float MagDistance[MAXMAG];

	void* _vidbufX;
	void* _vidbufY;

	int first;

	int nCar;
	int nPrg;

	int cur_caric;
	int cur_ypos;

	int curgraph;

	int max_carused;
	int max_magused;

	int curselected;
	int selected[2][MAXCAR];

	void MoveCursorX(int incX);
	void MoveCursorY(int incY);
	void GestKey(int c);
	void ActivateMenu(void);

	int MoveCar();

	int SearchBestPosition(char *tipcomp,int *allowed,struct CarDat *_conf=NULL);
	int FindBestConfiguration(void);
	int SearchBestMagPosition(int nmag,int *allowed,struct CarDat *_conf=NULL);
	int FindBestMagConfiguration(void);

	int ShowCarGraph();
	int ShowMagGraph();

	void CalcHistogram(struct TabPrg *AllPrg,struct CarDat *AllCar);
	void SelectItem(int item,int sel);
	void ToggleSelectItem(int item);
	void ShowGraph( int graphtype );
	int LoadAllData(void);

public:
	CaricHistogram(void);
	~CaricHistogram(void);
};

#define SCALETH1_COLOR 200
#define SCALETH2_COLOR 255
#define SCALETH1_VAL   2.0
#define SCALETH2_VAL   11


#define ZEROIMG                   1
#define RIFEIMG                   2
#define ZEROMACHIMG               3
#define HEADCAM_SCALE_IMG         4
#define MAPPING_IMG               5
#define PACKAGEVISION_LEFT_1      6
#define PACKAGEVISION_RIGHT_1     7
#define PACKAGEVISION_LEFT_2      9
#define PACKAGEVISION_RIGHT_2     10
#define EXTCAM_SCALE_IMG          11
#define UGEIMG                    12
//GF_30_05_2011
#define EXTCAM_NOZ_IMG            14
//CCCP
#define CARCOMP_IMG               15
#define RECTCOMP_IMG              16
#define INKMARK_IMG               17


#define IMAGE                     1
#define DATA                      2
#define ELAB                      3
#define BOARD                     4

#define CONVEYOR_STEP1			   1
#define CONVEYOR_STEP2			   2
#define CONVEYOR_STEP3			   3

//---------------------------------------------------------------------------
// finestra: tabella di montaggio
//---------------------------------------------------------------------------
class ProgramTableUI : public CWindowTable
{
public:
	ProgramTableUI();
	~ProgramTableUI();

	typedef enum
	{
		COL_LINE = 0,
		COL_CODE,
		COL_BOARD,
		COL_COMP,
		COL_PACK,
		COL_ROT,
		COL_NOZZLE,
		COL_ASSEM,
		COL_DISP,
		COL_X,
		COL_Y,
		COL_FEEDER,
		COL_NOTES,
		COL_TOT
	} cols_labels;

	typedef enum
	{
		NUM_BOARDS,
		NUM_COMPS
	} combo_labels;

public:
	void onRefresh();

protected:
	void onInit();
	void onShow();
	void onEdit();
	void onShowMenu();
	bool onKeyPress( int key );
	void onClose();

private:
	int onProgramData();
	int onConveyorWizard();
	int onConveyorData();
	int onEditOverride();
	int onCongruenceCheck();
	int onTableSwitch();
	int onTableSwitch_Dispenser();
	int onCopyField();

	void setStyle();
	void forceRefresh();
	void showRow( int row );
	void showItems( int start_item );
	void showSelectedRowData();
	void showProgramData();
	bool vSelect( int key );

	std::map<int,C_Combo*> m_combos;
	unsigned int m_start_item;

	GUI_SubMenu* SM_TableOp; // sub menu operazioni tabella
	GUI_SubMenu *SM_MenuConveyor; // Sottomenu "conveyor"
	GUI_SubMenu *SM_ConvAss; // Sottomenu "conveyor assembling"
	GUI_SubMenu *SM_ConvDisp; // Sottomenu "conveyor dispensing"
	GUI_SubMenu *SM_ConvDispAss; // Sottomenu "conveyor dispensing & assembling"
	GUI_SubMenu *SM_ConvQL; // Sottomenu "conveyor quick load"

};

#endif
