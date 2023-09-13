//---------------------------------------------------------------------------
//
// Name:        q_opt2.h
// Author:      Gabriel Ferri
// Created:     
// Description: 
//
//---------------------------------------------------------------------------

#ifndef __Q_OPT2_H
#define __Q_OPT2_H

#include <stdio.h>
#include "q_cost.h"
#include "q_tabe.h"
#include "c_window.h"
#include "gui_progressbar.h"


struct VertexPrgAux
{
	char CodCom[17];
	unsigned short int scheda;
	int mount;
};


struct OptStruct2
{
	OptStruct2()
	{
		idx = 0;
		n = -1;
		list = 0;
		ncollision = 0;
		nlowcollide = 0;
		height = 0.0f;
		MountMode = 0;
	};
	
	short int idx;
	short int n;
	short int* list;
	short int ncollision;
	short int nlowcollide;
	float height;
	int MountMode;
};


#define MM_SP_W		1.000f
#define MM_SF_W		0.800f
#define MM_DF_W		0.700f
#define VV_SP_W		0.700f
//#define SP_W			0.580f
#define VV_SF_W		0.600f
#define VV_DF_W		0.550f
//#define SF_W			0.500f
//#define DF_W			0.480f
#define M_W			0.400f
#define V_W			0.400f

#ifndef MIN
	#define MIN(a, b)	(((a)>(b))?(b):(a))
#endif

#ifndef MAX
	#define MAX(a, b)	(((a)>(b))?(a):(b))
#endif

struct NozzlesPair
{
	void Update()
	{
		MM_SP = MIN( MMcounter, SPcounter );
		MM_SF = MIN( MMcounter, SFcounter );
		MM_DF = MIN( MMcounter, SScounter - SFcounter );
		M = Mcounter;
		VV_SP = MIN( VVcounter, SPcounter );
		VV_SF = MIN( VVcounter, SFcounter );
		VV_DF = MIN( VVcounter, SScounter - SFcounter );
		V = Scounter - Mcounter;
	};
	
	void UpdateScore()
	{
		score = 0.0f;
		score += ( MM_SP_W * MM_SP_W ) * MM_SP;
		score += ( MM_SF_W * MM_SF_W ) * MM_SF;
		score += ( MM_DF_W * MM_DF_W ) * MM_DF;
		score += ( VV_SP_W * VV_SP_W ) * VV_SP;
		score += ( M_W * M_W ) * M;
		//score += ( SP_W * SP_W ) * SPcounter;
		score += ( VV_SF_W * VV_SF_W ) * VV_SF;
		score += ( VV_DF_W * VV_DF_W ) * VV_DF;
		//score += ( SF_W * SF_W ) * SFcounter;
		//score += ( DF_W * DF_W ) * SScounter - SFcounter;
		score += ( V_W * V_W ) * V;
	};
	
	char nozzles[2];
	bool canSwap;

	int Cycles;			// number of cycles
	
	int SPcounter;		// simultaneous pick
	int SFcounter;		// same feeder bank pick
	//int DFcounter;	// different feeder bank pick
	int MMcounter;		// two sniper alignment
	int Mcounter;		// only one component with sniper alignment
	int VVcounter;		// two vision alignment
	//int Vcounter;		// only one component with vision alignment
	int SScounter;		// two components sub-tour
	int Scounter;		// one component sub-tour
	
	int MM_SP;
	int MM_SF;
	int MM_DF;
	int M;
	int VV_SP;
	int VV_SF;
	int VV_DF;
	int V;
	
	float score;
};


struct MachineStatus
{
	MachineStatus()
	{
		headX = 0.0f;
		headY = 0.0f;
		nozzlesPair = 0;
		currentNozzles[0] = 0;
		currentNozzles[1] = 0;
		lastUsedNozzles[0] = 0;
		lastUsedNozzles[1] = 0;
		lastNozzlesChangeOptimized = true;
	};

	float headX;
	float headY;
	
	NozzlesPair* nozzlesPair;
	
	char currentNozzles[2];
	char lastUsedNozzles[2];
	bool lastNozzlesChangeOptimized;
};


struct STDistancesStruct
{
	STDistancesStruct()
	{
		dist_pick1_pick2 = -1.0f;
		dist_pick2_place1 = -1.0f;
		dist_place1_place2 = -1.0f;
	};
	
	float TotalDistance()
	{
		return dist_pick1_pick2 + dist_pick2_place1 + dist_place1_place2;
	};
	
	float dist_pick1_pick2;
	float dist_pick2_place1;
	float dist_place1_place2;
};


#define OPT_NOZZLE1            1
#define OPT_NOZZLE2            2
#define OPT_NOZZLE_ALL         3

class PrgOptimize2
{
public:
	PrgOptimize2( char* prog, int nozzle_mode = OPT_NOZZLE_ALL );
	~PrgOptimize2(void);
	
	int InitOptimize();
	
	int DoOptimize_NN( FILE* out = NULL );
	
	void PrintOptimize( FILE* out );
	bool WriteOptimize( void );
	
	// debug functions
	void PrintCollisionList( FILE* out );
	void PrintNozzlesList( FILE* out );
	void PrintSetsAB( FILE* out );
	void PrintSetsA1A2( FILE* out );
	void PrintSetsA1A2Dist( FILE* out );

private:
	GUI_ProgressBar_OLD* progbar;
	CWindow* wait;

	struct TabPrg* AllPrg;
	struct TabPrg* OptTab;
	struct Zeri* AllZer;
	struct CarDat* AllCar;
	struct VertexType* VertexList;
	struct VertexType* OptTabVertex;
	struct VertexPrgAux* AuxVertexList;
	struct OptStruct2* CollisionList;
	int OptTab_idx;

	int UgeUsed[MAXUGE];
	CfgUgelli AllUge[MAXUGE];

	TPrgFile* Prg;

	char prgName[9];

	int nPrg;
	int nCar;
	int nZer;
	int nPack;

	int optNozzles;

	int okFlag;
	
	// collision list functions
	struct VertexType Create2DVertex(float cx,float cy,float width,float height,float z);
	void Enlarge2DVertex(struct VertexType &v,float val);
	void Rotate2DVertex(struct VertexType &v,float rot);
	int IsPointInside2DVertex(struct VertexType v,float px,float py);
	int CheckVertexIntersect(struct VertexType v1,struct VertexType v2);
	int Compare2DVertex(struct VertexType v1,struct VertexType v2);
	void CalcComp2DVertex(struct VertexType &v,int n);
	void CalcCompUge2DVertex(struct VertexType &ugev,struct VertexType compv,float deltauge);
	
	void UpdateCollisionList( int n, bool restore = false );
	void RestoreCollisionList( int start_line );
	
	// ottimizzazione
	int nCompToAssembly;
	
	int* setA;			// insieme degli elementi assemblabili
	int setA_size;
	int* setB;			// insieme degli elementi bloccati da altri elementi
	int setB_size;
	NozzlesPair setD[MAXUGE*MAXUGE+MAXUGE];	// insieme delle coppie di ugelli possibili
	int setD_size;
	
	unsigned int* setA1;
	int setA1_size;
	unsigned int* setA2;
	int setA2_size;
	STDistancesStruct* distA1A2;
	int distA1A2_size;
	
	void InitAB();
	bool InitA1A2();
	void InitA1A2Dist();
	
	int OptimizeA_SubTour2_NN( FILE* out = 0 );
	int OptimizeA_SubTour1_NN( FILE* out = 0 );
	
	enum SubTourType
	{
		STT_SP,
		STT_SF,
		STT_DF
	};
	int OptimizeA_SubTour2_NN_STT( bool MM_flag, SubTourType STType );
	
	void InitNozzlesList();
	void ElabNozzlesList();
	void RankNozzlesList();
	int RankNozzlesList_R0( int a, int b );
	int RankNozzlesList_R1( int a, int b );
	
	bool CheckNozzlesPair( char* tools, char* nozzles, char* flag = NULL );
	void OptimizeNozzlesChange( int d_index );
	
	void GetPlacePosition( struct TabPrg tab, float &outx, float &outy );
	void GetPickPosition( struct TabPrg tab, float &outx, float &outy );
	void GetToolPosition( char tool, char punta, float &outx, float &outy );
	
	// simula la testa durante l'assemblaggio
	MachineStatus currentStatus;
	
	// sequenza ugelli durante assemblaggio
	struct OptSequenceStruct
	{
		char nozzles[2];
		bool optimized;
		int opt_line;
	};
	OptSequenceStruct* OptSequence;
	int OptSequence_size;
};

#endif // __Q_OPT2_H
