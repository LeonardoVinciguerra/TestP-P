//---------------------------------------------------------------------------
//
// Name:        q_opt2.cpp
// Author:      Gabriel Ferri
// Created:     
// Description: 
//
//---------------------------------------------------------------------------
#include "q_opt2.h"

//---------------------------------------------------------------------------
// NOTE: si assume che prima di eseguire l'ottimizzazione venga eseguito il
//       check di congruenza
//---------------------------------------------------------------------------

#include <assert.h>
#include <ctype.h>

#include <string.h>
#include <math.h>
#include <unistd.h>

#include "filefn.h"
#include "msglist.h"
#include "q_debug.h"
#include "q_wind.h"
#include "q_tabet.h"
#include "q_help.h"
#include "q_oper.h"
#include "q_assem.h"

#include "q_carobj.h"
#include "q_packages.h"
#include "q_ugeobj.h"
#include "q_progt.h"

#include "sniper.h"

#include "strutils.h"
#include "datetime.h"
#include "lnxdefs.h"

#include <mss.h>


#define WAITOPT_TXT1 MsgGetString(Msg_01800)
#define WAITOPT_TXT2 MsgGetString(Msg_01540)
#define WAITOPT_POS  2,3,36,1


#define MOUNT_P1               1
#define MOUNT_P2               2

#define ERRPREL_DELTA          1.2

#define SP_DIST_RANGE          150


extern struct CfgParam QParam;
extern SPackageData currentLibPackages[MAXPACK];


/*
-------------------------------------------------------------------------
PrgOptimize2
  Costruttore e distruttore
Parametri di ingresso:
  prog			: nome del progama da ottimizzare
  nozzle_mode	: modalità punte
  					1 -> mono punta 1
  					2 -> mono punta 2
  					3 -> doppia punta
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
PrgOptimize2::PrgOptimize2( char *prog, int nozzle_mode )
{
	okFlag = 0;
	
	optNozzles = nozzle_mode;
	
	strncpyQ( prgName, prog, 8 );
	
	// init delle strutture dati
	//----------------------------------------------------------------------
	ZerFile *Zer = NULL;
	FeederFile *Car = NULL;
	Prg = NULL;
	
	progbar = NULL;
	wait = NULL;
	
	AllPrg = NULL;
	AllCar = NULL;
	AllZer = NULL;

	OptTab = NULL;
	CollisionList = NULL;
	
	OptTabVertex = NULL;
	VertexList = NULL;
	AuxVertexList = NULL;
	
	char pathPrgAssem[MAXNPATH];
	char pathPrgMaster[MAXNPATH];
	char pathDta[MAXNPATH];
	char pathZer[MAXNPATH];
	
	// apertura files
	//----------------------------------------------------------------------
	PrgPath(pathPrgMaster,prog);
	PrgPath(pathPrgAssem,prog,PRG_ASSEMBLY);
	PrgPath(pathDta,prog,PRG_DATA);
	PrgPath(pathZer,prog,PRG_ZER);

	if(access(pathPrgMaster,F_OK))
	{
		W_Mess( MsgGetString(Msg_00184) );
		return;
	}

	if(access(pathPrgAssem,F_OK))
	{
		W_Mess( MsgGetString(Msg_01209) );
		return;
	}

	if(access(pathDta,F_OK))
	{
		W_Mess( MsgGetString(Msg_01787) );
		return;
	}

	if(access(pathZer,F_OK))
	{
		W_Mess(NOZEROFILE);
		return;
	}
	
	char lib[9];
	char conf[9];
	
	Read_PrgCFile(lib,conf);
	
	char pathCar[MAXNPATH];
	
	CarPath(pathCar,conf);

	if(access(pathCar,F_OK))
	{
		bipbip();
		W_Mess(NOCONF);
		return;
	}

	Prg=new TPrgFile(pathPrgAssem,PRG_NOADDPATH);
	Prg->Open(SKIPHEADER);
	
	Car=new FeederFile(conf);
	
	Zer=new ZerFile(pathZer,ZER_NOADDPATH);
	Zer->Open();


	nPrg = Prg->Count();
	nCar = MAXCAR;
	nZer = Zer->GetNRecs();
	nPack = MAXNPATH;
	
	OptTabVertex = new struct VertexType [nPrg*4];
	VertexList = new struct VertexType [nPrg*4];
	AuxVertexList = new struct VertexPrgAux [nPrg];
	
	// carica i dati da file in memoria
	//----------------------------------------------------------------------
	AllPrg = new struct TabPrg[nPrg];
	AllZer = new struct Zeri[nZer];
	AllCar = new struct CarDat[nCar];
	
	// init statistiche caricatori utilizzati nel programma
	//----------------------------------------------------------------------
	for(int i=0;i<nCar;i++)
	{
		Car->ReadRec(i,AllCar[i]);
	}

	for(int i=0;i<nZer;i++)
	{
		Zer->Read(AllZer[i],i);
	}
	
	for(int i=0;i<nPrg;i++)
	{
		Prg->Read(AllPrg[i],i);
	}
	
	// init statistiche ugelli utilizzabili dal programma
	//----------------------------------------------------------------------
	for( int i = 0; i < MAXUGE; i++ )
	{
		Ugelli->ReadRec(AllUge[i],i);
		UgeUsed[i] = 0;
	}

	delete Car;
	delete Zer;


	setA = 0;
	setA_size = 0;
	setB = 0;
	setB_size = 0;
	setD_size = 0;
	setA1 = 0;
	setA1_size = 0;
	setA2 = 0;
	setA2_size = 0;
	distA1A2 = 0;
	distA1A2_size = 0;
	
	nCompToAssembly = 0;
	
	OptSequence = NULL;
	OptSequence_size = 0;
	
	okFlag = 1;
}

PrgOptimize2::~PrgOptimize2(void)
{
	if(AllPrg!=NULL)
		delete[] AllPrg;
	if(AllCar!=NULL)
		delete[] AllCar;
	if(AllZer!=NULL)
		delete[] AllZer;
	if(OptTab!=NULL)
		delete[] OptTab;
	if(VertexList!=NULL)
		delete[] VertexList;
	if(OptTabVertex!=NULL)
		delete[] OptTabVertex;
	if(AuxVertexList!=NULL)
		delete[] AuxVertexList;
	if(wait!=NULL)
		delete wait;
	if(progbar!=NULL)
		delete progbar;
	
	if(CollisionList!=NULL)
	{
		for(int i=0;i<nPrg;i++)
		{
			if(CollisionList[i].list!=NULL)
				delete[] CollisionList[i].list;
		}
		delete[] CollisionList;
	}
	
	if(Prg!=NULL)
		delete Prg;
	
	if( setA )
		delete [] setA;
	if( setB )
		delete [] setB;
	if( setA1 )
		delete [] setA1;
	if( setA2 )
		delete [] setA2;
	if( distA1A2 )
		delete [] distA1A2;
	
	if( OptSequence )
		delete [] OptSequence;
}


/*
-------------------------------------------------------------------------
CalcDistance
  Calcola la distanza tra due punti (movimento cartesiano con assi separati)
Parametri di ingresso:
  p1_x,p1_y		: package da controllare
  p1_x,p1_y		: package da controllare
Valori ritornati:
  distanza
-------------------------------------------------------------------------
*/
#ifndef ABS
	#define ABS(a)				(((a) < 0) ? -(a) : (a))
#endif
float CalcDistance( float p1_x, float p1_y, float p2_x, float p2_y )
{
	return MAX( ABS(p2_x - p1_x), ABS(p2_y - p1_y) );
}

/*
-------------------------------------------------------------------------
InitAB
  Suddivide gli elementi negli insiemi A e B
Parametri di ingresso:
   nessuno
Valori ritornati:
   nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::InitAB()
{
	if( setA ) delete setA;
	if( setB ) delete setB;
	setA = new int[nPrg]; // insieme degli elementi assemblabili
	setB = new int[nPrg]; // insieme degli elementi bloccati da altri elementi
	setA_size = 0;
	setB_size = 0;
	
	for( int i = 0; i < nPrg; i++ )
	{
		// controlla se il componente e' da assemblare
		if( CollisionList[i].n == -1 )
			continue;
		
		// controlla che non sia bloccato da altri
		if( CollisionList[i].nlowcollide != 0 )
		{
			setB[setB_size++] = i;
		}
		else
		{
			setA[setA_size++] = i;
		}
	}
}

/*
-------------------------------------------------------------------------
InitA1A2
  Suddivide gli elementi negli insiemi A1 e A2
  A1: componenti assemblabili con l'ugello sulla punta 1
  A2: componenti assemblabili con l'ugello sulla punta 2
Parametri di ingresso:
   nessuno
Valori ritornati:
   true: se A1 e A2 hanno almeno un elemento
   false: altrimenti
-------------------------------------------------------------------------
*/
bool PrgOptimize2::InitA1A2()
{
	if( setA1 ) delete [] setA1;
	if( setA2 ) delete [] setA2;
	setA1 = new unsigned int[setA_size];
	setA2 = new unsigned int[setA_size];
	setA1_size = 0;
	setA2_size = 0;
	
	for( int i = 0; i < setA_size; i++ )
	{
		// check if component scheduled
		if( CollisionList[setA[i]].n == -1 )
			continue;
		
		SPackageData* pack = &currentLibPackages[AllCar[GetCarRec(AllPrg[CollisionList[setA[i]].n].Caric)].C_PackIndex-1];
		char pack_flag;
		if( CheckNozzlesPair( pack->tools, currentStatus.currentNozzles, &pack_flag ) == true )
		{
			if( optNozzles & OPT_NOZZLE1 )
			{
				// if component can use nozzle 1...
				if( pack_flag == 1 || pack_flag == 3 )
					setA1[setA1_size++] = i;
			}
			
			if( optNozzles & OPT_NOZZLE2 )
			{
				// if component can use nozzle 2...
				if( pack_flag == 2 || pack_flag == 3 )
					setA2[setA2_size++] = i;
			}
		}
	}
	
	return (setA1_size && setA2_size) ? true : false;
}

/*
-------------------------------------------------------------------------
InitA1A2Dist
  Calcola la distanza tra gli elementi degli insiemi A1 e A2 e la memorizza
  nel vettore distA1A2
Parametri di ingresso:
   nessuno
Valori ritornati:
   nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::InitA1A2Dist()
{
	// calculate distances between A1 and A2
	if( distA1A2 ) delete [] distA1A2;
	distA1A2 = new STDistancesStruct[setA1_size*setA2_size];
	distA1A2_size = 0;
	
	for( unsigned int a1 = 0; a1 < setA1_size; a1++ )
	{
		struct TabPrg comp1 = AllPrg[CollisionList[setA[setA1[a1]]].n];
		comp1.Punta = '1';

		SPackageData* pack1 = &currentLibPackages[AllCar[GetCarRec(comp1.Caric)].C_PackIndex-1];
		
		float comp1_pick_x, comp1_pick_y;
		GetPickPosition( comp1, comp1_pick_x, comp1_pick_y );
		float comp1_place_x, comp1_place_y;
		GetPlacePosition( comp1, comp1_place_x, comp1_place_y );
		
		for( unsigned int a2 = 0; a2 < setA2_size; a2++ )
		{
			if( setA1[a1] == setA2[a2] )
			{
				distA1A2_size++;
				continue;
			}
			
			struct TabPrg comp2 = AllPrg[CollisionList[setA[setA2[a2]]].n];
			comp2.Punta = '2';
			
			SPackageData* pack2 = &currentLibPackages[AllCar[GetCarRec(comp2.Caric)].C_PackIndex-1];
			
			// ciclo misto non supportato
			if( ( pack1->centeringMode != CenteringMode::EXTCAM && pack2->centeringMode == CenteringMode::EXTCAM ) ||
				( pack1->centeringMode == CenteringMode::EXTCAM && pack2->centeringMode != CenteringMode::EXTCAM ) )
			{
				distA1A2_size++;
				continue;
			}
			
			float comp2_pick_x, comp2_pick_y;
			GetPickPosition( comp2, comp2_pick_x, comp2_pick_y );
			float comp2_place_x, comp2_place_y;
			GetPlacePosition( comp2, comp2_place_x, comp2_place_y );
			
			STDistancesStruct distances;
			
			distances.dist_pick1_pick2 = CalcDistance( comp1_pick_x, comp1_pick_y, comp2_pick_x, comp2_pick_y );
			distances.dist_pick2_place1 = CalcDistance( comp2_pick_x, comp2_pick_y, comp1_place_x, comp1_place_y );
			distances.dist_place1_place2 = CalcDistance( comp1_place_x, comp1_place_y, comp2_place_x, comp2_place_y );
			
			// se centraggio su telecamera esterna aggiunge la distanza necessaria
			if( pack1->centeringMode == CenteringMode::EXTCAM ) // di conseguenza anche pack2 e' CenteringMode::EXTCAM per il precedente controllo
			{
				distances.dist_pick2_place1 = CalcDistance( comp2_pick_x, comp2_pick_y, QParam.AuxCam_X[0], QParam.AuxCam_Y[0] );
				distances.dist_pick2_place1 += CalcDistance( QParam.AuxCam_X[0], QParam.AuxCam_Y[0], comp1_place_x, comp1_place_y );
			}
			
			distA1A2[distA1A2_size++] = distances;
		}
	}
}

/*
-------------------------------------------------------------------------
CheckNozzlesPair
  Controlla se un dato package può utilizzare una data coppia di ugelli
Parametri di ingresso:
  tools		    : ugelli da controllare
  nozzles		: ugelli sulle punte
  flag			: (opzionale) ritorna quali ugelli può utilizzare
					1. ugello in nozzle[0]
					2. ugello in nozzle[1]
					3. entrambe gli ugelli
					0. nessun ugello
Valori ritornati:
  true			: il package può usare la coppia di ugelli
  false			: altrimenti
-------------------------------------------------------------------------
*/
bool PrgOptimize2::CheckNozzlesPair( char* tools, char* nozzles, char* flag )
{
	char _flag = 0;

	for( unsigned int i = 0; i < strlen(tools); i++ )
	{
		if( tools[i] == nozzles[0] )
			_flag |= 0x01;
		
		if( tools[i] == nozzles[1] )
			_flag |= 0x02;
	}

	if( flag )
		*flag = _flag;

	return _flag ? true : false;
}

/*
-------------------------------------------------------------------------
InitNozzlesList
  Inizializza la liste delle possibili coppie di ugelli
Parametri di ingresso:
  nessuno
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::InitNozzlesList()
{
	setD_size = 0;
	
	if( ( optNozzles & OPT_NOZZLE1 ) && ( optNozzles & OPT_NOZZLE2 ) )
	{
		for( int i = 0; i < MAXUGE; i++ )
		{
			if( !UgeUsed[i] )
				continue;
			
			// if uge[i] cannot use nozzle1 or nozzle2...
			if( !(AllUge[i].NozzleAllowed & 0x03) )
				continue;
			
			for( int j = i+1; j < MAXUGE; j++ )
			{
				if( !UgeUsed[j] )
					continue;
				
				// if uge[j] cannot use nozzle1 or nozzle2...
				if( !(AllUge[j].NozzleAllowed & 0x03) )
					continue;
				
				
				bool i1j2 = false;
				bool i2j1 = false;
				
				// if uge[i] can use nozzle1 and uge[j] can use nozzle2...
				if( (AllUge[i].NozzleAllowed & 0x01) && (AllUge[j].NozzleAllowed & 0x02) )
					i1j2 = true;
				
				// if uge[i] can use nozzle2 and uge[j] can use nozzle1...
				if( (AllUge[i].NozzleAllowed & 0x02) && (AllUge[j].NozzleAllowed & 0x01) )
					i2j1 = true;
				
				if( i1j2 && i2j1 )
				{
					// ok: si possono anche scambiare
					memset( &setD[setD_size], 0, sizeof(NozzlesPair) );
					setD[setD_size].nozzles[0] = i+'A';
					setD[setD_size].nozzles[1] = j+'A';
					setD[setD_size].canSwap = true;
				}
				else if( i1j2 )
				{
					// ok: i j
					memset( &setD[setD_size], 0, sizeof(NozzlesPair) );
					setD[setD_size].nozzles[0] = i+'A';
					setD[setD_size].nozzles[1] = j+'A';
					setD[setD_size].canSwap = false;
				}
				else if( i2j1 )
				{
					// ok: j i
					memset( &setD[setD_size], 0, sizeof(NozzlesPair) );
					setD[setD_size].nozzles[0] = j+'A';
					setD[setD_size].nozzles[1] = i+'A';
					setD[setD_size].canSwap = false;
				}
				else
					continue;
				
				setD_size++;
			}
		}
	}
	
	// single nozzle
	for( int i = 0; i < MAXUGE; i++ )
	{
		if( !UgeUsed[i] )
			continue;
		
		// if uge[i] can use nozzle1
		if( optNozzles & OPT_NOZZLE1 )
		{
			if( AllUge[i].NozzleAllowed & 0x01 )
			{
				memset( &setD[setD_size], 0, sizeof(NozzlesPair) );
				setD[setD_size].nozzles[0] = i+'A';
				setD[setD_size].nozzles[1] = '^';
				
				// if uge[i] can use nozzle2
				if( ( optNozzles & OPT_NOZZLE2 ) && ( AllUge[i].NozzleAllowed & 0x02 ) )
					setD[setD_size].canSwap = true;
				else
					setD[setD_size].canSwap = false;
				
				setD_size++;
				continue;
			}
		}
		
		// if uge[i] can use nozzle2
		if( optNozzles & OPT_NOZZLE2 )
		{
			if( AllUge[i].NozzleAllowed & 0x02 )
			{
				memset( &setD[setD_size], 0, sizeof(NozzlesPair) );
				setD[setD_size].nozzles[0] = '^';
				setD[setD_size].nozzles[1] = i+'A';
				setD[setD_size].canSwap = false;
				
				setD_size++;
				continue;
			}
		}
	}
}

/*
-------------------------------------------------------------------------
ElabNozzlesList
  Elabora la liste delle possibili coppie di ugelli
Parametri di ingresso:
  nessuno
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
#define MM_COUNTER_FLAG			0x01
#define VV_COUNTER_FLAG			0x02
#define SP_COUNTER_FLAG			0x04
#define SF_COUNTER_FLAG			0x08

void PrgOptimize2::ElabNozzlesList()
{
	char pack1_flag, pack2_flag;
	
	// reset nozzles pair's counters
	for( int pair = 0; pair < setD_size; pair++ )
	{
		//setD[pair].Cycles = 0;
		setD[pair].SPcounter = 0;
		setD[pair].SFcounter = 0;
		setD[pair].MMcounter = 0;
		setD[pair].Mcounter = 0;
		setD[pair].VVcounter = 0;
		setD[pair].SScounter = 0;
		setD[pair].Scounter = 0;
	}
	
	// for each component...
	for( int i = 0; i < setA_size; i++ )
	{
		// component i
		SPackageData* pack1 = &currentLibPackages[AllCar[GetCarRec(AllPrg[CollisionList[setA[i]].n].Caric)].C_PackIndex-1];

		// for each nozzles pair...
		for( int pair = 0; pair < setD_size; pair++ )
		{
			// if component can use this nozzle pair...
			if( CheckNozzlesPair( pack1->tools, setD[pair].nozzles, &pack1_flag ) == true )
			{
				// S sub-tour
				setD[pair].Scounter++;
				
				// look for M sub-tour
				char pack1_M_flag = false;
				if( pack1->centeringMode == CenteringMode::SNIPER || pack1->centeringMode == CenteringMode::NONE )
				{
					setD[pair].Mcounter++;
					pack1_M_flag = true;
				}
				
				
				// look for other components...
				char subtour_flag = ( pack1_M_flag ? MM_COUNTER_FLAG : VV_COUNTER_FLAG ) | SP_COUNTER_FLAG | SF_COUNTER_FLAG;
				
				for( int j = 0; j < setA_size && subtour_flag; j++ )
				{
					if( i == j )
						continue;
					
					// component j
					SPackageData* pack2 = &currentLibPackages[AllCar[GetCarRec(AllPrg[CollisionList[setA[j]].n].Caric)].C_PackIndex-1];
					
					// if component can use this nozzle pair...
					if( CheckNozzlesPair( pack2->tools, setD[pair].nozzles, &pack2_flag ) == true )
					{
						// if both component use the same nozzle continue
						if( pack1_flag != 3 && pack1_flag == pack2_flag )
							continue;
					}
					else
					{
						continue;
					}
					
					
					if( pack1_M_flag && ( pack2->centeringMode == CenteringMode::SNIPER || pack2->centeringMode == CenteringMode::NONE ) )
					{
						// look for MM sub-tour
						if( subtour_flag & MM_COUNTER_FLAG )
						{
							setD[pair].SScounter++;
							setD[pair].MMcounter++;
							subtour_flag -= MM_COUNTER_FLAG;
						}
					}
					else if( !pack1_M_flag && ( pack2->centeringMode == CenteringMode::EXTCAM ) )
					{
						// look for VV sub-tour
						if( subtour_flag & VV_COUNTER_FLAG )
						{
							setD[pair].SScounter++;
							setD[pair].VVcounter++;
							subtour_flag -= VV_COUNTER_FLAG;
						}
					}
					else // non sono ammessi cicli misti M e V
						continue;
					
					
					TabPrg comp1 = AllPrg[CollisionList[setA[i]].n];
					TabPrg comp2 = AllPrg[CollisionList[setA[j]].n];
					
					// look for SP sub-tour
					if( subtour_flag & SP_COUNTER_FLAG )
					{
						// avoid SF sub-tour
						if( comp1.Caric != comp2.Caric )
						{
							float comp1_pick_x, comp1_pick_y;
							GetPickPosition( comp1, comp1_pick_x, comp1_pick_y );
							float comp2_pick_x, comp2_pick_y;
							GetPickPosition( comp2, comp2_pick_x, comp2_pick_y );
							
							float pick_dist = CalcDistance( comp1_pick_x, comp1_pick_y, comp2_pick_x, comp2_pick_y );
							if( pick_dist < SP_DIST_RANGE )
							{
								setD[pair].SPcounter++;
								subtour_flag -= SP_COUNTER_FLAG;
							}
						}
					}
					
					// look for SF sub-tour
					if( subtour_flag & SF_COUNTER_FLAG )
					{
						if( comp1.Caric == comp2.Caric )
						{
							setD[pair].SFcounter++;
							subtour_flag -= SF_COUNTER_FLAG;
						}
					}
				}
			}
		}
	}
	
	// for each nozzle pair...
	for( int pair = 0; pair < setD_size; pair++ )
	{
		setD[pair].Update();
		setD[pair].UpdateScore();
	}
}

/*
-------------------------------------------------------------------------
RankNozzlesList
  Esegue procedura di ranking delle coppie di ugelli
Parametri di ingresso:
  nessuno
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::RankNozzlesList()
{
	// ShellSort
	int flag = 1;
	int d = setD_size;
	NozzlesPair temp;
	
	while( flag || (d > 1) )
	{
		flag = 0; // reset flag to check for future swaps
		d = (d+1) / 2;
		
		for( int i = 0; i < (setD_size - d); i++ )
		{
			if( RankNozzlesList_R1( i+d, i ) > 0 )
			{
				// swap positions i+d and i
				temp = setD[i + d];
				setD[i + d] = setD[i];
				setD[i] = temp;
				flag = 1;
			}
		}
	}
	
	// reset cycles counter to avoid to re-use old nozzles pair with low score
	for( int i = 0; i < setD_size; i++ )
		setD[i].Cycles = 0;
}

/*
-------------------------------------------------------------------------
RankNozzlesList_Rx
  Compara due coppie di ugelli
Parametri di ingresso:
  a		: coppia di ugelli da controllare
  b		: coppia di ugelli da controllare
Valori ritornati:
  0: a = b,  1: a > b,  -1: a < b
-------------------------------------------------------------------------
*/
int PrgOptimize2::RankNozzlesList_R0( int a, int b )
{
	// order by number of sub-tours scheduled
	if( setD[a].Cycles > setD[b].Cycles )
		return 1;
	else if( setD[a].Cycles < setD[b].Cycles )
		return -1;
	
	// order by MM sub-tour
	if( setD[a].MMcounter > setD[b].MMcounter )
		return 1;
	else if( setD[a].MMcounter < setD[b].MMcounter )
		return -1;
	
	// order by SP sub-tour
	if( setD[a].SPcounter > setD[b].SPcounter )
		return 1;
	else if( setD[a].SPcounter < setD[b].SPcounter )
		return -1;
	
	// order by M sub-tour
	if( setD[a].Mcounter > setD[b].Mcounter )
		return 1;
	else if( setD[a].Mcounter < setD[b].Mcounter )
		return -1;
	
	// order by VV sub-tour
	if( setD[a].VVcounter > setD[b].VVcounter )
		return 1;
	else if( setD[a].VVcounter < setD[b].VVcounter )
		return -1;
	
	// order by SF sub-tour
	if( setD[a].SFcounter > setD[b].SFcounter )
		return 1;
	else if( setD[a].SFcounter < setD[b].SFcounter )
		return -1;

	// order by SS sub-tour
	if( setD[a].SScounter > setD[b].SScounter )
		return 1;
	else if( setD[a].SScounter < setD[b].SScounter )
		return -1;
	
	// order by S sub-tour
	if( setD[a].Scounter > setD[b].Scounter )
		return 1;
	else if( setD[a].Scounter < setD[b].Scounter )
		return -1;
	
	// items are equal, order by nozzle[0]
	// ATTENTION !!! we consider nozzle 'A' > 'Z'
	if( setD[a].nozzles[0] > setD[b].nozzles[0] )
		return -1;
	else if( setD[a].nozzles[0] < setD[b].nozzles[0] )
		return 1;
	
	// items are equal, order by nozzle[1]
	// ATTENTION !!! we consider nozzle 'A' > 'Z'
	if( setD[a].nozzles[1] > setD[b].nozzles[1] )
		return -1;
	else if( setD[a].nozzles[1] < setD[b].nozzles[1] )
		return 1;
	
	return 0;
}

int PrgOptimize2::RankNozzlesList_R1( int a, int b )
{
	// order by number of sub-tours scheduled
	if( setD[a].Cycles > setD[b].Cycles )
		return 1;
	else if( setD[a].Cycles < setD[b].Cycles )
		return -1;
	
	// order by score
	if( setD[a].score > setD[b].score )
		return 1;
	else if( setD[a].score < setD[b].score )
		return -1;
	
	// items are equal, order by nozzle[0]
	// ATTENTION !!! we consider nozzle 'A' > 'Z'
	if( setD[a].nozzles[0] > setD[b].nozzles[0] )
		return -1;
	else if( setD[a].nozzles[0] < setD[b].nozzles[0] )
		return 1;
	
	// items are equal, order by nozzle[1]
	// ATTENTION !!! we consider nozzle 'Z' > 'A'
	if( setD[a].nozzles[1] < setD[b].nozzles[1] )
		return -1;
	else if( setD[a].nozzles[1] > setD[b].nozzles[1] )
		return 1;
	
	return 0;
}

//-------------------------------------------------------------------------
// Calcola ed elabora le liste di collisione tra i componenti
//-------------------------------------------------------------------------
int PrgOptimize2::InitOptimize()
{
	if( !okFlag )
	{
		return 0;
	}

	//---------------//
	//    PARTE 1    //
	//---------------//
	
	nCompToAssembly = 0;
	
	CollisionList = new struct OptStruct2[nPrg];
	
	short int *tmp = new short int[nPrg-1];
	int tmpcount;
	
	struct SPackageData* pdat1;
	struct SPackageData* pdat2;
	
	struct VertexType v1,v2,u1,u2,u3;
	
	struct CfgUgeDim UgeDim[MAXUGEDIM];
	
	UgeDimOpen();
	UgeDimReadAll(UgeDim);
	UgeDimClose();
	
	wait = new CWindow(20,10,60,15,"");
	wait->Show();
	wait->DrawTextCentered( 0, WAITOPT_TXT1 );
	progbar = new GUI_ProgressBar_OLD( wait, WAITOPT_POS, 2*nPrg );
	
	for( int i = 0; i < nPrg; i++ )
	{
		// ciclo di controllo componente di riferimento
		
		strncpyQ(AuxVertexList[i].CodCom,AllPrg[i].CodCom,16);
		AuxVertexList[i].scheda=AllPrg[i].scheda;
	
		if((AllPrg[i].status & NOMNTBRD_MASK) || (!(AllPrg[i].status & MOUNT_MASK)))
		{
			VertexList[i].enable = 0;
			AuxVertexList[i].mount = 0;
			
			progbar->Increment(1);
			continue;
		}
		
		// componente da assemblare
		nCompToAssembly++;
		for(int j=0;j<strlen(currentLibPackages[AllCar[GetCarRec(AllPrg[i].Caric)].C_PackIndex-1].tools);j++)
		{
			int nuge = currentLibPackages[AllCar[GetCarRec(AllPrg[i].Caric)].C_PackIndex-1].tools[j]-'A';
			UgeUsed[nuge]++;
		}
		
		VertexList[i].enable = 1;
		AuxVertexList[i].mount = 1;
		tmpcount = 0;

		pdat1 = &currentLibPackages[AllCar[GetCarRec(AllPrg[i].Caric)].C_PackIndex-1];
	
		int ugedim_idx = AllUge[pdat1->tools[0]-'A'].utype;
	
		//calcola vertex del componente
		CalcComp2DVertex(v1,i);
	
		//calcola i vertex per le varie sezioni dell'ugello
		CalcCompUge2DVertex(u1,v1,UgeDim[ugedim_idx].b);
		Rotate2DVertex(u1,AllPrg[i].Rotaz);
		
		CalcCompUge2DVertex(u2,v1,UgeDim[ugedim_idx].d);
		Rotate2DVertex(u2,AllPrg[i].Rotaz);
		
		CalcCompUge2DVertex(u3,v1,UgeDim[ugedim_idx].f);
		Rotate2DVertex(u3,AllPrg[i].Rotaz);
	
		VertexList[i*4]=v1;
		Rotate2DVertex(VertexList[i*4],AllPrg[i].Rotaz);
		
		VertexList[i*4+1]=u1;
		VertexList[i*4+2]=u2;
		VertexList[i*4+3]=u3;
	
		//controlla se il componente di riferimento collide con altri
		for(int j=0;j<nPrg;j++)
		{
			if(j==i)
			{
				continue;
			}
		
			if((AllPrg[j].status & NOMNTBRD_MASK) || (!(AllPrg[j].status & MOUNT_MASK)))
			{
				continue;
			}
			
			pdat2 = &currentLibPackages[AllCar[GetCarRec(AllPrg[j].Caric)].C_PackIndex-1];
		
			if( pdat2->z <= pdat1->z )
				continue;

			CalcComp2DVertex(v2,j);
			Rotate2DVertex(v2,AllPrg[j].Rotaz);
		
			//se il componente in esame e' piu basso di quello di riferimento+
			//altezza della prima sezione dell'ugello
		
			if( pdat2->z < pdat1->z + UgeDim[ugedim_idx].a )
			{
				//controlla se il componente in esame collide con il rettangolo
				//della prima sezione dell'ugello
				if(CheckVertexIntersect(v2,u1))
				{
					//se si memorizza la collisione
					tmp[tmpcount++]=j;
				}
			}
			else
			{
				//se il componente in esame e' piu basso di quello di riferimento+
				//altezza della seconda sezione dell'ugello
		
				if( pdat2->z < pdat1->z + UgeDim[ugedim_idx].c )
				{
					//controlla se il componente in esame collide con il rettangolo
					//della seconda sezione dell'ugello
					if(CheckVertexIntersect(v2,u2))
					{
						//si: memorizza la collisione
						tmp[tmpcount++]=j;
					}
				}
				else
				{
					//il componente in esame e' alto oltre l'altezza del componente
					//di riferimento+seconda sezione ugello
					//controlla collisione tra componente in esame e rettangolo della
					//terza sezione ugello
					if(CheckVertexIntersect(v2,u3))
					{
						//si: memorizza la collisione
						tmp[tmpcount++]=j;
					}
				}
			}
		}

		if(tmpcount!=0)
		{
			//trovati uno o piu componenti che collidono con quello di riferimento
			CollisionList[i].ncollision=tmpcount;
			
			CollisionList[i].list=new short int[tmpcount];
			//memorizza la lista di collisione per il componente di riferimento
			memcpy(CollisionList[i].list,tmp,sizeof(short int)*tmpcount);
			//il componente di riferimento deve essere assemblato prima di quelli
			//presenti nella sua lista di collisione
		
			//per ogni componente nella lista di collisione incrementa il numero
			//di componenti che devono essere assemblati prima
		
			for(int j=0;j<tmpcount;j++)
			{
				CollisionList[CollisionList[i].list[j]].nlowcollide++;
			}
		}
		else
		{
			//nessuna collisione trovata
			CollisionList[i].ncollision=0;
			CollisionList[i].list=NULL;
		}

		CollisionList[i].n=i;
		CollisionList[i].height = pdat1->z;

		progbar->Increment(1);
	}
	
	delete[] tmp;
	
	
	//---------------//
	//    PARTE 2    //
	//---------------//
	
	// questo passaggio serve per verificare che i componenti siano prelevabili dalle punte
	// selezionate
	// ad esempio un componente con ugello solo punta 1 non puo' essere assemblato con
	// ottimizzazione solo punta 2

	for( int i = 0; i < nPrg; i++ )
	{
		//controlla se l'elemento nella lista indica un componente da assemblare
		if( CollisionList[i].n == -1 )
		{
			progbar->Increment(1);
			continue;
		}

		SPackageData* pdat = &currentLibPackages[AllCar[GetCarRec(AllPrg[i].Caric)].C_PackIndex-1];

		if(optNozzles & OPT_NOZZLE1)
		{
			//controlla che il componente possa essere prelevato dalla punta 1
			if( CheckPackageNozzle( pdat, 1 ) == PACK_DIMOK )
			{
				CollisionList[i].MountMode|=MOUNT_P1;
			}
			else
			{
				CollisionList[i].MountMode&=~MOUNT_P1;
			}
		}

		if(optNozzles & OPT_NOZZLE2)
		{
			//controlla che il componente possa essere prelevato dalla punta 2
			if( CheckPackageNozzle( pdat, 2 ) == PACK_DIMOK )
			{
				CollisionList[i].MountMode|=MOUNT_P2;
			}
			else
			{
				CollisionList[i].MountMode&=~MOUNT_P2;
			}
		}

		if(!(CollisionList[i].MountMode & (MOUNT_P1 | MOUNT_P2)))
		{
			//impossibile eseguire l'ottimizzazione in quanto il componente non
			//puo essere prelevato/centrato ne dalla punta 1 ne dalla 2
			// !!! Questa condizione non dovrebbe mai verificarsi !!!!
			char buf[80];
			snprintf( buf, sizeof(buf), MsgGetString(Msg_01889), pdat->name );
			W_Mess( buf );
		
			delete progbar;
			delete wait;
			progbar=NULL;
			wait=NULL;
			
			return 0;
		}

		progbar->Increment(1);
	}
	
	delete progbar;
	delete wait;
	progbar=NULL;
	wait=NULL;
	
	return 1;
}

/*
-------------------------------------------------------------------------
OptimizeNozzlesChange
  Ottimizza il cambio ugelli
Parametri di ingresso:
  nessuno
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::OptimizeNozzlesChange( int d_index )
{
	int i = d_index;
	
	if( setD[d_index].score == 0.0f )
		return;
	
	while( setD[i].score == setD[d_index].score )
	{
		bool doSwap = false;
		
		currentStatus.nozzlesPair = &setD[i];
		
		if( currentStatus.nozzlesPair->nozzles[0] != currentStatus.lastUsedNozzles[0] &&
			currentStatus.nozzlesPair->nozzles[1] != currentStatus.lastUsedNozzles[1] )
		{
			// è avvenuto un doppio cambio ugelli...
			
			if( currentStatus.nozzlesPair->nozzles[0] == currentStatus.lastUsedNozzles[1] ||
				currentStatus.nozzlesPair->nozzles[1] == currentStatus.lastUsedNozzles[0] )
			{
				doSwap = true;
			}
		}
		
		if( doSwap && currentStatus.nozzlesPair->canSwap )
		{
			currentStatus.lastNozzlesChangeOptimized = true;
			currentStatus.currentNozzles[0] = currentStatus.nozzlesPair->nozzles[1];
			currentStatus.currentNozzles[1] = currentStatus.nozzlesPair->nozzles[0];
			return;
		}
		else
		{
			if( i == d_index )
			{
				currentStatus.lastNozzlesChangeOptimized = doSwap ? false : true;
				currentStatus.currentNozzles[0] = currentStatus.nozzlesPair->nozzles[0];
				currentStatus.currentNozzles[1] = currentStatus.nozzlesPair->nozzles[1];
				
				if( currentStatus.lastNozzlesChangeOptimized )
					return;
			}
			
			if( i < setD_size-1 )
				i++;
			else
				return;
		}
	}
}

/*
-------------------------------------------------------------------------
OptimizeA_SubTour2
  Esegue ciclo di ottimizzazione su tutto l'insieme A considerando solo
  sub-tour con 2 componenti
Parametri di ingresso:
  nessuno
Valori ritornati:
  numero di elementi inseriti nell'ottimizzazione
-------------------------------------------------------------------------
*/
int PrgOptimize2::OptimizeA_SubTour2_NN( FILE* out )
{
	int ret_val;
	
	if( currentStatus.currentNozzles[0] != '^' && currentStatus.currentNozzles[1] != '^' )
	{
		// init nozzle1 and nozzle2 lists
		//-----------------------------------------------------------------------------
		if( InitA1A2() )
		{
			// calc nozzle1 and nozzle2 distances
			//-----------------------------------------------------------------------------
			InitA1A2Dist();
			
			// PRINT RESULTS
			//-----------------------------------------------------------------------------
			if( out )
			{
				PrintSetsA1A2( out );
				//PrintSetsA1A2Dist( out );
			}
			
			// MM_SP
			//-----------------------------------------------------------------------------
			ret_val = OptimizeA_SubTour2_NN_STT( true, STT_SP );
			if( ret_val > 0 )
			{
				return ret_val;
			}
			
			// MM_SF
			//-----------------------------------------------------------------------------
			ret_val = OptimizeA_SubTour2_NN_STT( true, STT_SF );
			if( ret_val > 0 )
			{
				return ret_val;
			}
			
			// MM_DF
			//-----------------------------------------------------------------------------
			ret_val = OptimizeA_SubTour2_NN_STT( true, STT_DF );
			if( ret_val > 0 )
			{
				return ret_val;
			}
			
			// VV_SP
			//-----------------------------------------------------------------------------
			ret_val = OptimizeA_SubTour2_NN_STT( false, STT_SP );
			if( ret_val > 0 )
			{
				return ret_val;
			}
			
			// VV_SF
			//-----------------------------------------------------------------------------
			ret_val = OptimizeA_SubTour2_NN_STT( false, STT_SF );
			if( ret_val > 0 )
			{
				return ret_val;
			}
			
			// VV_DF
			//-----------------------------------------------------------------------------
			ret_val = OptimizeA_SubTour2_NN_STT( false, STT_DF );
			if( ret_val > 0 )
			{
				return ret_val;
			}
		}
	}
	
	return 0;
}

/*
-------------------------------------------------------------------------
OptimizeA_SubTour2_NN_STT
  
Parametri di ingresso:
  nessuno
Valori ritornati:
  numero di elementi inseriti nell'ottimizzazione
-------------------------------------------------------------------------
*/
int PrgOptimize2::OptimizeA_SubTour2_NN_STT( bool MM_flag, SubTourType STType )
{
	bool inserted;
	float minDist;
	int min_a1, min_a2;
	int q_count = 0;
	
	while( 1 )
	{
		inserted = false;
		minDist = 1000000.0f;
		
		// get current head position
		//-----------------------------------------------------------------------------
		if( currentStatus.currentNozzles[0] == currentStatus.lastUsedNozzles[0] &&
			currentStatus.currentNozzles[1] == currentStatus.lastUsedNozzles[1] )
		{
			// la posizione è quella dell'ultimo componente assemblato
			struct TabPrg last_comp = OptTab[OptTab_idx-1];
			GetPlacePosition( last_comp, currentStatus.headX, currentStatus.headY );
		}
		else
		{
			// la posizione è quella dell'ultimo ugello cambiato
			if( currentStatus.currentNozzles[1] != currentStatus.lastUsedNozzles[1] )
				GetToolPosition( currentStatus.currentNozzles[1], '2', currentStatus.headX, currentStatus.headY );
			else
				GetToolPosition( currentStatus.currentNozzles[0], '1', currentStatus.headX, currentStatus.headY );
		}
		
		// look for component on nozzle 1
		//-----------------------------------------------------------------------------
		for( unsigned int a1 = 0; a1 < setA1_size; a1++ )
		{
			// check if component scheduled
			if( CollisionList[setA[setA1[a1]]].n == -1 )
				continue;
			
			// check component centering
			SPackageData* pack1 = &currentLibPackages[AllCar[GetCarRec(AllPrg[CollisionList[setA[setA1[a1]]].n].Caric)].C_PackIndex-1];
			if( MM_flag )
			{
				if( pack1->centeringMode != CenteringMode::SNIPER && pack1->centeringMode != CenteringMode::NONE )
					continue;
			}
			else
			{
				if( pack1->centeringMode != CenteringMode::EXTCAM )
					continue;
			}
			
			struct TabPrg comp1 = AllPrg[CollisionList[setA[setA1[a1]]].n];
			comp1.Punta = '1';
			// calc pick distance
			float comp1_pick_x, comp1_pick_y;
			GetPickPosition( comp1, comp1_pick_x, comp1_pick_y );
			float dist_empty = CalcDistance( currentStatus.headX, currentStatus.headY, comp1_pick_x, comp1_pick_y );
			// calc distA1A2 table displacement
			int a1_displacement = a1 * setA2_size;
			
			// look for component on nozzle 2
			//-----------------------------------------------------------------------------
			for( int a2 = 0; a2 < setA2_size; a2++ )
			{
				// check if component scheduled
				if( CollisionList[setA[setA2[a2]]].n == -1  || setA1[a1] == setA2[a2] )
					continue;
				
				// check component centering
				SPackageData* pack2 = &currentLibPackages[AllCar[GetCarRec(AllPrg[CollisionList[setA[setA2[a2]]].n].Caric)].C_PackIndex-1];
				if( MM_flag )
				{
					if( pack2->centeringMode != CenteringMode::SNIPER && pack2->centeringMode != CenteringMode::NONE )
						continue;
				}
				else
				{
					if( pack2->centeringMode != CenteringMode::EXTCAM )
						continue;
				}
				
				if( STType == STT_SP )
				{
					if( distA1A2[a1_displacement+a2].dist_pick1_pick2 >= SP_DIST_RANGE )
						continue;
					if( comp1.Caric == AllPrg[CollisionList[setA[setA2[a2]]].n].Caric )
						continue;
				}
				else if( STType == STT_SF )
				{
					if( comp1.Caric != AllPrg[CollisionList[setA[setA2[a2]]].n].Caric )
						continue;
				}
				//else ...va bene qualsiasi combinazione
				
				// calc components pick-place distance
				float dist = dist_empty + distA1A2[a1_displacement+a2].TotalDistance();
				if( dist < minDist )
				{
					minDist = dist;
					min_a1 = setA1[a1];
					min_a2 = setA2[a2];
					inserted = true;
				}
			}
		}
		
		if( !inserted )
			break;
		else
		{
			// inserisce componente per la punta 1 nella lista di ottimizzazione
			//-----------------------------------------------------------------------------
			OptTab[OptTab_idx] = AllPrg[CollisionList[setA[min_a1]].n];
			OptTab[OptTab_idx].Punta = '1';
			OptTab[OptTab_idx].Riga = OptTab_idx + 1;
			OptTab[OptTab_idx].Uge = currentStatus.currentNozzles[0];
			
			for( int k = 0; k < 4; k++ )
			{
				OptTabVertex[OptTab_idx*4+k] = VertexList[CollisionList[setA[min_a1]].n*4+k];
			}
			
			// elimina i riferimenti a questo componente nella lista di collisione
			UpdateCollisionList( setA[min_a1] );
			CollisionList[setA[min_a1]].n = -1;
			
			// incrementa numero di componenti assegnati
			OptTab_idx++;
			progbar->Increment(1);
			
			// inserisce componente per la punta 2 nella lista di ottimizzazione
			//-----------------------------------------------------------------------------
			OptTab[OptTab_idx] = AllPrg[CollisionList[setA[min_a2]].n];
			OptTab[OptTab_idx].Punta = '2';
			OptTab[OptTab_idx].Riga = OptTab_idx+1;
			OptTab[OptTab_idx].Uge = currentStatus.currentNozzles[1];
			
			for( int k = 0; k < 4; k++ )
			{
				OptTabVertex[OptTab_idx*4+k] = VertexList[CollisionList[setA[min_a2]].n*4+k];
			}
			
			// elimina i riferimenti a questo componente nella lista di collisione
			UpdateCollisionList( setA[min_a2] );
			CollisionList[setA[min_a2]].n = -1;
			
			// incrementa numero di componenti assegnati
			OptTab_idx++;
			progbar->Increment(1);
			
			currentStatus.nozzlesPair->Cycles++;
			
			// aggiorna la sequenza di ugelli utilizzata
			if( currentStatus.lastUsedNozzles[0] != currentStatus.currentNozzles[0] ||
				currentStatus.lastUsedNozzles[1] != currentStatus.currentNozzles[1] )
			{
				OptSequence[OptSequence_size].nozzles[0] = currentStatus.currentNozzles[0];
				OptSequence[OptSequence_size].nozzles[1] = currentStatus.currentNozzles[1];
				OptSequence[OptSequence_size].opt_line = OptTab_idx-2;
				OptSequence[OptSequence_size].optimized = currentStatus.lastNozzlesChangeOptimized;
				OptSequence_size++;
			}
			
			// save last nozzles pair used
			currentStatus.lastUsedNozzles[0] = currentStatus.currentNozzles[0];
			currentStatus.lastUsedNozzles[1] = currentStatus.currentNozzles[1];
			
			q_count += 2;
		}
	}
	
	return q_count;
}

/*
-------------------------------------------------------------------------
OptimizeA_SubTour1
  Esegue ciclo di ottimizzazione su tutto l'insieme A considerando solo
  sub-tour con 1 componente
Parametri di ingresso:
  nessuno
Valori ritornati:
  numero di elementi inseriti nell'ottimizzazione
-------------------------------------------------------------------------
*/
int PrgOptimize2::OptimizeA_SubTour1_NN( FILE* out )
{
	bool inserted;
	float minDist;
	int min_a1, min_a2;
	int q_count = 0;
	
	
	// init nozzle1 and nozzle2 lists
	//-----------------------------------------------------------------------------
	InitA1A2();

	if( setA1_size || setA2_size )
	{
		// PRINT RESULTS
		//-----------------------------------------------------------------------------
		if( out )
		{
			PrintSetsA1A2( out );
		}
		
		//
		//-----------------------------------------------------------------------------
		while( setA1_size || setA2_size )
		{
			inserted = false;
			minDist = 1000000.0f;

			// get current head position
			//-----------------------------------------------------------------------------
			if( currentStatus.currentNozzles[0] == currentStatus.lastUsedNozzles[0] &&
				currentStatus.currentNozzles[1] == currentStatus.lastUsedNozzles[1] )
			{
				// la posizione è quella dell'ultimo componente assemblato
				struct TabPrg last_comp = OptTab[OptTab_idx-1];
				GetPlacePosition( last_comp, currentStatus.headX, currentStatus.headY );
			}
			else
			{
				// la posizione è quella dell'ultimo ugello cambiato
				if( currentStatus.currentNozzles[1] != '^' && currentStatus.currentNozzles[1] != currentStatus.lastUsedNozzles[1] )
					GetToolPosition( currentStatus.currentNozzles[1], '2', currentStatus.headX, currentStatus.headY );
				else
					GetToolPosition( currentStatus.currentNozzles[0], '1', currentStatus.headX, currentStatus.headY );
			}
			
			// look for component on nozzle 1
			//-----------------------------------------------------------------------------
			for( int a1 = 0; a1 < setA1_size; a1++ )
			{
				// check if component scheduled
				if( CollisionList[setA[setA1[a1]]].n == -1 )
					continue;
				
				struct TabPrg comp = AllPrg[CollisionList[setA[setA1[a1]]].n];
				comp.Punta = '1';
				// calc pick-place distance
				float comp_pick_x, comp_pick_y;
				GetPickPosition( comp, comp_pick_x, comp_pick_y );
				float comp_place_x, comp_place_y;
				GetPlacePosition( comp, comp_place_x, comp_place_y );
				
				float dist_empty = CalcDistance( currentStatus.headX, currentStatus.headY, comp_pick_x, comp_pick_y );
				float dist_pick_place = CalcDistance( comp_pick_x, comp_pick_y, comp_place_x, comp_place_y );
				float dist = dist_empty + dist_pick_place;
				
				if( dist < minDist )
				{
					minDist = dist;
					min_a1 = setA1[a1];
					min_a2 = -1;
					inserted = true;
				}
			}
			
			// look for component on nozzle 2
			//-----------------------------------------------------------------------------
			for( int a2 = 0; a2 < setA2_size; a2++ )
			{
				// check if component scheduled
				if( CollisionList[setA[setA2[a2]]].n == -1 )
					continue;
				
				struct TabPrg comp = AllPrg[CollisionList[setA[setA2[a2]]].n];
				comp.Punta = '2';
				// calc pick-place distance
				float comp_pick_x, comp_pick_y;
				GetPickPosition( comp, comp_pick_x, comp_pick_y );
				float comp_place_x, comp_place_y;
				GetPlacePosition( comp, comp_place_x, comp_place_y );
				
				float dist_empty = CalcDistance( currentStatus.headX, currentStatus.headY, comp_pick_x, comp_pick_y );
				float dist_pick_place = CalcDistance( comp_pick_x, comp_pick_y, comp_place_x, comp_place_y );
				float dist = dist_empty + dist_pick_place;
				
				if( dist < minDist )
				{
					minDist = dist;
					min_a1 = -1;
					min_a2 = setA2[a2];
					inserted = true;
				}
			}
			
			if( !inserted )
				break;
			else
			{
				// inserisce componente nella lista di ottimizzazione
				//-----------------------------------------------------------------------------
				if( min_a1 != -1 )
				{
					OptTab[OptTab_idx] = AllPrg[CollisionList[setA[min_a1]].n];
					OptTab[OptTab_idx].Punta = '1';
					OptTab[OptTab_idx].Uge = currentStatus.currentNozzles[0];
					
					for( int k = 0; k < 4; k++ )
					{
						OptTabVertex[OptTab_idx*4+k] = VertexList[CollisionList[setA[min_a1]].n*4+k];
					}
				}
				else
				{
					OptTab[OptTab_idx] = AllPrg[CollisionList[setA[min_a2]].n];
					OptTab[OptTab_idx].Punta = '2';
					OptTab[OptTab_idx].Uge = currentStatus.currentNozzles[1];
					
					for( int k = 0; k < 4; k++ )
					{
						OptTabVertex[OptTab_idx*4+k] = VertexList[CollisionList[setA[min_a2]].n*4+k];
					}
				}
				
				OptTab[OptTab_idx].Riga = OptTab_idx + 1;
				
				// elimina i riferimenti a questo componente nella lista di collisione
				if( min_a1 != -1 )
				{
					UpdateCollisionList( setA[min_a1] );
					CollisionList[setA[min_a1]].n = -1;
				}
				else
				{
					UpdateCollisionList( setA[min_a2] );
					CollisionList[setA[min_a2]].n = -1;
				}
			
				// incrementa numero di componenti assegnati
				OptTab_idx++;
				progbar->Increment(1);
				
				currentStatus.nozzlesPair->Cycles++;
				
				
				if( min_a1 != -1 )
				{
					// aggiorna la sequenza di ugelli utilizzata
					if( currentStatus.lastUsedNozzles[0] != currentStatus.currentNozzles[0] )
					{
						OptSequence[OptSequence_size].nozzles[0] = currentStatus.currentNozzles[0];
						OptSequence[OptSequence_size].nozzles[1] = (OptSequence_size > 0) ? OptSequence[OptSequence_size-1].nozzles[1] : '^';
						OptSequence[OptSequence_size].opt_line = OptTab_idx-1;
						OptSequence[OptSequence_size].optimized = currentStatus.lastNozzlesChangeOptimized;
						OptSequence_size++;
					}
				}
				else
				{
					// aggiorna la sequenza di ugelli utilizzata
					if( currentStatus.lastUsedNozzles[1] != currentStatus.currentNozzles[1] )
					{
						OptSequence[OptSequence_size].nozzles[0] = (OptSequence_size > 0) ? OptSequence[OptSequence_size-1].nozzles[0] : '^';
						OptSequence[OptSequence_size].nozzles[1] = currentStatus.currentNozzles[1];
						OptSequence[OptSequence_size].opt_line = OptTab_idx-1;
						OptSequence[OptSequence_size].optimized = currentStatus.lastNozzlesChangeOptimized;
						OptSequence_size++;
					}
				}

				// save last nozzles pair used
				currentStatus.lastUsedNozzles[0] = currentStatus.currentNozzles[0];
				currentStatus.lastUsedNozzles[1] = currentStatus.currentNozzles[1];

				q_count++;
			}
		}
	}
	
	return q_count;
}

//-----------------------------------------------------------------------------------------
// Esegue ciclo di ottimizzazione
//-----------------------------------------------------------------------------------------
int PrgOptimize2::DoOptimize_NN( FILE* out )
{
	if( nCompToAssembly == 0 )
	{
		return 0;
	}

	int retval = 1;
	
	if( OptTab ) delete [] OptTab;
	OptTab = new struct TabPrg[nPrg];
	OptTab_idx = 0;
	
	if( OptSequence ) delete [] OptSequence;
	OptSequence = new OptSequenceStruct[nPrg];
	OptSequence_size = 0;
	
	if( wait == NULL )
	{
		wait = new CWindow(20,10,60,15,"");
		wait->Show();
		wait->DrawTextCentered( 0, WAITOPT_TXT2 );
		progbar = new GUI_ProgressBar_OLD( wait, WAITOPT_POS, nCompToAssembly+nPrg );
	}
	
	// inizializzazione
	//-----------------------------------------------------------------------------
	InitNozzlesList();
	
	currentStatus.headX = 0.0f;
	currentStatus.headY = 0.0f;
	currentStatus.nozzlesPair = 0;
	currentStatus.lastUsedNozzles[0] = 0;
	currentStatus.lastUsedNozzles[1] = 0;
	
	
	//-----------------------------------------------------------------------------
	int ret_val;
	int pair;
	int q = nCompToAssembly;
	while( q > 0 )
	{
		// suddivide i componenti nei due gruppi A e B
		//-----------------------------------------------------------------------------
		InitAB();
		
		// crea la lista delle possibili coppie di ugelli e la ordina
		//-----------------------------------------------------------------------------
		ElabNozzlesList();
		RankNozzlesList();
		
		// PRINT RESULTS
		//-----------------------------------------------------------------------------
		if( out )
		{
			PrintSetsAB( out );
			PrintNozzlesList( out );
		}


		bool inserted = false;

		// PARTE I - sub-tour con 2 comp
		//-----------------------------------------------------------------------------
		pair = 0;
		while( pair < setD_size )
		{
			OptimizeNozzlesChange( pair );
			
			ret_val = OptimizeA_SubTour2_NN( out );
			if( ret_val > 0 )
			{
				q -= ret_val;
				inserted = true;
				break;
			}
			
			pair++;
		}
		if( inserted )
			continue;


		// PARTE II - sub-tour con 1 comp
		//-----------------------------------------------------------------------------
		pair = 0;
		while( pair < setD_size )
		{
			OptimizeNozzlesChange( pair );
			
			ret_val = OptimizeA_SubTour1_NN( out );
			if( ret_val > 0 )
			{
				q -= ret_val;
				inserted = true;
				break;
			}
			
			pair++;
		}
		if( inserted )
			continue;
		
		// ERRORE - non deve mai arrivare qui
		W_Mess( "Unknown optimization ERROR !");
		retval = 0;
		break;
	}


	// CLEAN
	//-----------------------------------------------------------------------------
	if( setA1 )
	{
		delete [] setA1;
		setA1 = 0;
	}
	if( setA2 )
	{
		delete [] setA2;
		setA2 = 0;
	}
	if( distA1A2 )
	{
		delete [] distA1A2;
		distA1A2 = 0;
	}

	return retval;
}

/*
-------------------------------------------------------------------------
GetPlacePosition
  Ritorna la posizione di "place" di un componente (considerando la punta)
Parametri di ingresso:
  nessuno
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::GetPlacePosition( struct TabPrg tab, float &outx, float &outy )
{
	struct Zeri zero = AllZer[ tab.scheda ];
	
	outx = zero.Z_xzero + tab.XMon;
	outy = zero.Z_yzero + tab.YMon;
	
	if( tab.Punta == '1' )
	{
		outx += QParam.CamPunta1Offset_X;
		outy += QParam.CamPunta1Offset_Y;
	}
	else
	{
		outx += QParam.CamPunta2Offset_X;
		outy += QParam.CamPunta2Offset_Y;
	}
}

/*
-------------------------------------------------------------------------
GetPickPosition
Ritorna la posizione di "pick" di un componente (considerando la punta)
Parametri di ingresso:
  nessuno
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::GetPickPosition( struct TabPrg tab, float &outx, float &outy )
{
	struct CarDat car = AllCar[GetCarRec(tab.Caric)];
	
	outx = car.C_xcar;
	outy = car.C_ycar;
	
	if( tab.Punta == '1' )
	{
		outx += QParam.CamPunta1Offset_X;
		outy += QParam.CamPunta1Offset_Y;
	}
	else
	{
		outx += QParam.CamPunta2Offset_X;
		outy += QParam.CamPunta2Offset_Y;
	}
}

/*
-------------------------------------------------------------------------
GetToolPosition
Ritorna la posizione di "pick" di un ugello (considerando la punta)
Parametri di ingresso:
  nessuno
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::GetToolPosition( char tool, char punta, float &outx, float &outy )
{
	struct CfgUgelli uge = AllUge[tool-'A'];
	
	if( punta == '1' )
	{
		outx = uge.X_ugeP1;
		outy = uge.Y_ugeP1;
	}
	else
	{
		outx = uge.X_ugeP2;
		outy = uge.Y_ugeP2;
	}
}

/*
-------------------------------------------------------------------------
PrintCollisionList
  stampa su file le liste di collisione
Parametri di ingresso:
  nessuno
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::PrintCollisionList(FILE *fOutput)
{
	fprintf(fOutput,"\nCollisionList dump");
	
	for(int i=0;i<nPrg;i++)
	{
		int nc;
	
		if((nc=CollisionList[i].n)==-1)
			continue;
	
		fprintf(fOutput,"\n----------------------------------------------------------\n");
		fprintf(fOutput,"[i=%4d / PrgRow=%4d] %s (%s)\n",i,AllPrg[nc].Riga,AllPrg[nc].CodCom,AllPrg[nc].TipCom);
	
		fprintf(fOutput,"N of components that must be mounted after this   :%4d\n",CollisionList[i].ncollision);
		fprintf(fOutput,"N of lower comp. that must be mounted before this :%4d\n",CollisionList[i].nlowcollide);
		fprintf(fOutput,"Height                                            :%5.3f\n",CollisionList[i].height);
		fprintf(fOutput,"Mount mode (1/2=Only Noz1/2 3=Both nozzles        :%4d\n",CollisionList[i].MountMode);
	
		if(CollisionList[i].ncollision)
		{
			fprintf(fOutput,"\nComponents that must be mounted after this:\n");
			fprintf(fOutput,"  Row  Comp.Code\n");
		
			for(int j=0;j<CollisionList[i].ncollision;j++)
			{
				int nc=CollisionList[CollisionList[i].list[j]].n;
				fprintf(fOutput,"  %4d %20s\n",AllPrg[nc].Riga,AllPrg[nc].CodCom);
			}
		}
	}
}

//-------------------------------------------------------------------------
//  Salva il programma ottimizzato
//-------------------------------------------------------------------------
bool PrgOptimize2::WriteOptimize()
{
	int count=0;
	
	Prg->Open(SKIPHEADER);
	
	char lastOptPath[MAXNPATH];
	PrgPath(lastOptPath,prgName,PRG_LASTOPT);
	TPrgFile *lastOptFile=new TPrgFile(lastOptPath,PRG_NOADDPATH);
	lastOptFile->Create();
	delete lastOptFile;
	
	lastOptFile=new TPrgFile(lastOptPath,PRG_NOADDPATH);
	lastOptFile->Open();
	lastOptFile->UpdateHeader_OnWrite(0);
	
	//scrive intestazione file lastOpt
	int lastOptFile_tmppos=lseek(lastOptFile->GetHandle(),0,SEEK_CUR);
	lseek(lastOptFile->GetHandle(),0,SEEK_SET);
	WRITE_HEADER(lastOptFile->GetHandle(),FILES_VERSION,LASTOPT_SUBVERSION);
	write(lastOptFile->GetHandle(),(char *)OPTHEADER_TXT,strlen(OPTHEADER_TXT));
	lseek(lastOptFile->GetHandle(),lastOptFile_tmppos,SEEK_SET);
	
	struct VertexType* newV=new struct VertexType[nPrg*4];
	
	//all'inizio del programma i componenti da non assemblare
	for( int i = 0; i < nPrg; i++ )
	{
		if((AllPrg[i].status & NOMNTBRD_MASK) || (!(AllPrg[i].status & MOUNT_MASK)))
		{
			AllPrg[i].Riga=count+1;
			Prg->Write(AllPrg[i],count,FLUSHON);
			lastOptFile->Write(AllPrg[i],count,FLUSHON);
			progbar->Increment(1);
		
			for(int k=0;k<4;k++)
			{
				newV[count*4+k].enable=0;
			}
		
			count++;
		}
	}
	
	//successivamente i componenti da assemblare
	for(int i=0;i<OptTab_idx;i++)
	{
		assert(count<nPrg);
	
		OptTab[i].Riga=count+1;
		Prg->Write(OptTab[i],count,FLUSHON);
	
		lastOptFile->Write(OptTab[i],count,FLUSHON);
	
		for(int k=0;k<4;k++)
		{
			newV[count*4+k]=OptTabVertex[i*4+k];
		}
		
		progbar->Increment(1);
		count++;
	}

	delete [] VertexList;
	
	VertexList=newV;
	
	delete lastOptFile;
	
	delete progbar;
	delete wait;
	progbar = NULL;
	wait = NULL;

	return (count == nPrg) ? true : false;
}

/*
-------------------------------------------------------------------------
UpdateCollisionList
  Aggiorna le liste di collisione dopo che un componente e' stato
  inserito nel programma ottimizzato
Parametri di ingresso:
  n       : elemento nella lista di collisione da aggiornare
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::UpdateCollisionList( int n, bool restore /*= false*/ )
{
	// cerca tutti i componenti che devono essere depositati dopo il componente in esame
	for( int i = 0; i < CollisionList[n].ncollision; i++ )
	{
		if( !restore )
		{
			assert2(CollisionList[CollisionList[n].list[i]].nlowcollide!=0,
					{
						print_debug("%s placed.",AllPrg[CollisionList[n].n].CodCom);
						print_debug("error in %s (%d)\n",AllPrg[CollisionList[CollisionList[n].list[i]].n].CodCom,CollisionList[CollisionList[n].list[i]].n);
					}
				);
			
			if( CollisionList[CollisionList[n].list[i]].nlowcollide != 0 )
			{
		// decrementa il numero di componenti che devono essere assemblati prima di questo
				CollisionList[CollisionList[n].list[i]].nlowcollide--;
			}
		}
		else
		{
			// incrementa il numero di componenti che devono essere assemblati prima di questo
			CollisionList[CollisionList[n].list[i]].nlowcollide++;
		}
	}
	
	// decrementa/incrementa numero di utilizzi ugelli per componente in esame
	for( int j = 0; j < strlen(currentLibPackages[AllCar[GetCarRec(AllPrg[CollisionList[n].n].Caric)].C_PackIndex-1].tools); j++ )
	{
		int nuge = currentLibPackages[AllCar[GetCarRec(AllPrg[CollisionList[n].n].Caric)].C_PackIndex-1].tools[j]-'A';
		
		if( !restore )
			UgeUsed[nuge]--;
		else
			UgeUsed[nuge]++;
	}
}

/*
-------------------------------------------------------------------------
RestoreCollisionList
  Ripristina le liste di collisione
Parametri di ingresso:
  start_line: 
Valori ritornati:
  nessuno
-------------------------------------------------------------------------
*/
void PrgOptimize2::RestoreCollisionList( int start_line )
{
	// cerca tutti i componenti che devono essere depositati dopo il componente in esame
	for( int line = start_line; line < OptTab_idx; line++ )
	{
		int comp = -1;
		for( int i = 0; i < nPrg; i++ )
		{
			if( (strcmp( OptTab[line].CodCom, AllPrg[i].CodCom ) == 0 ) &&
				( OptTab[line].scheda == AllPrg[i].scheda ) )
			{
				comp = i;
				break;
			}
		}
		
		
		if( comp != -1 )
		{
			//
			CollisionList[comp].n = comp;
			UpdateCollisionList( comp, true );
		}
	}
}


/*
-------------------------------------------------------------------------
 DEBUG FUNCTIONS
-------------------------------------------------------------------------
*/

void PrgOptimize2::PrintOptimize( FILE *fOutput )
{
	fprintf(fOutput,"\nFinal results\n");
	fprintf(fOutput,"--------------------------------------------------------\n");
	
	for(int i=0;i<OptTab_idx;i++)
	{
		if(OptTab[i].Punta=='1')
		{
			fprintf(fOutput,"\n");
		}
		else
		{
			if( i > 1 )
			{
				if( OptTab[i].Punta == OptTab[i-1].Punta )
					fprintf(fOutput,"\n");
			}
		}
		
		TabPrg* pTabPrg = &OptTab[i];
		fprintf( fOutput, "[%4d-%4d] ", pTabPrg->scheda, pTabPrg->Riga );
		fprintf( fOutput, "%20s %20s ", pTabPrg->CodCom, pTabPrg->TipCom );
		fprintf( fOutput, "P%c%c %4d ", pTabPrg->Punta, pTabPrg->Uge, pTabPrg->Caric );
		fprintf( fOutput, "%4s ", currentLibPackages[AllCar[GetCarRec(pTabPrg->Caric)].C_PackIndex-1].tools );
		fprintf( fOutput, "%7.3f ", currentLibPackages[AllCar[GetCarRec(pTabPrg->Caric)].C_PackIndex-1].z );
		fprintf( fOutput, "%d\n", pTabPrg->flags );
	}
}

void PrgOptimize2::PrintNozzlesList( FILE* out )
{
	fprintf( out, "\nNOZZLES USED\n" );
	fprintf( out, "--------------------------------------------------------\n" );
	for( int i = 0; i < MAXUGE; i++ )
	{
		if( UgeUsed[i] )
			fprintf( out, "[%c]", i+'A' );
	}
	fprintf( out, "\n");
	fprintf( out, "--------------------------------------------------------\n" );
	for( int i = 0; i < setD_size; i++ )
	{
		fprintf( out, "[%c][%c] ", setD[i].nozzles[0], setD[i].nozzles[1] );
		fprintf( out, "score: %6.3f  ", setD[i].score );
		fprintf( out, "Cycles: %4d  ", setD[i].Cycles );
		fprintf( out, "MM_SP: %4d  ", setD[i].MM_SP );
		fprintf( out, "MM_SF: %4d  ", setD[i].MM_SF );
		fprintf( out, "MM_DF: %4d  ", setD[i].MM_DF );
		fprintf( out, "VV_SP: %4d  ", setD[i].VV_SP );
		fprintf( out, "VV_SF: %4d  ", setD[i].VV_SF );
		fprintf( out, "VV_DF: %4d\n", setD[i].VV_DF );
	}
	fprintf( out, "\n" );
}

void PrgOptimize2::PrintSetsAB( FILE* out )
{
	fprintf( out, "\nSET A\n" );
	fprintf( out, "--------------------------------------------------------\n" );
	for( int i = 0; i < setA_size; i++ )
	{
		TabPrg* pTabPrg = &AllPrg[CollisionList[setA[i]].n];
		fprintf( out, "[%4d-%4d] ", pTabPrg->scheda, pTabPrg->Riga );
		fprintf( out, "%20s %20s ", pTabPrg->CodCom, pTabPrg->TipCom );
		fprintf( out, "P%c%c %4d ", pTabPrg->Punta, pTabPrg->Uge, pTabPrg->Caric );
		fprintf( out, "%4s ", currentLibPackages[AllCar[GetCarRec(pTabPrg->Caric)].C_PackIndex-1].tools );
		fprintf( out, "%5.3f ", currentLibPackages[AllCar[GetCarRec(pTabPrg->Caric)].C_PackIndex-1].z );
		fprintf( out, "%d\n", pTabPrg->flags );
	}
	
	fprintf( out, "\nSET B\n" );
	fprintf( out, "--------------------------------------------------------\n" );
	for( int i = 0; i < setB_size; i++ )
	{
		TabPrg* pTabPrg = &AllPrg[CollisionList[setB[i]].n];
		fprintf( out, "[%4d-%4d] ", pTabPrg->scheda, pTabPrg->Riga );
		fprintf( out, "%20s %20s ", pTabPrg->CodCom, pTabPrg->TipCom );
		fprintf( out, "P%c%c %4d ", pTabPrg->Punta, pTabPrg->Uge, pTabPrg->Caric );
		fprintf( out, "%4s ", currentLibPackages[AllCar[GetCarRec(pTabPrg->Caric)].C_PackIndex-1].tools );
		fprintf( out, "%5.3f ", currentLibPackages[AllCar[GetCarRec(pTabPrg->Caric)].C_PackIndex-1].z );
		fprintf( out, "%d\n", pTabPrg->flags );
	}
	fprintf( out, "\n" );
}

void PrgOptimize2::PrintSetsA1A2( FILE* out )
{
	fprintf( out, "\nSUB-SETS: A1[%c] A2[%c]\n", currentStatus.currentNozzles[0], currentStatus.currentNozzles[1] );
	fprintf( out, "--------------------------------------------------------\n" );
	for( int i = 0; i < MAX( setA1_size, setA2_size ); i++ )
	{
		if( i < setA1_size )
			fprintf( out, " %4d , ", setA1[i] );
		else
			fprintf( out, "    - , " );
				
		if( i < setA2_size )
			fprintf( out, "%4d\n", setA2[i] );
		else
			fprintf( out, "   -\n" );
	}
	fprintf( out, "\n" );
}

void PrgOptimize2::PrintSetsA1A2Dist( FILE* out )
{
	fprintf( out, "\nA1 -> A2 DISTANCES\n" );
	fprintf( out, "--------------------------------------------------------\n" );
	for( unsigned int a1 = 0; a1 < setA1_size; a1++ )
	{
		for( unsigned int a2 = 0; a2 < setA2_size; a2++ )
		{
			fprintf( out, " %4d -> %4d : %6.3f  %6.3f  %6.3f\n", setA1[a1], setA2[a2], distA1A2[a1*setA2_size+a2].dist_pick1_pick2, distA1A2[a1*setA2_size+a2].dist_pick2_place1, distA1A2[a1*setA2_size+a2].dist_place1_place2 );
		}
	}
	fprintf( out, "\n" );
}


/*
-------------------------------------------------------------------------
 OLD FUNCTIONS
-------------------------------------------------------------------------
*/

/*
-------------------------------------------------------------------------
Create2DVertex:
Crea e definisce i 4 vertici di un rettangolo
Parametri di ingresso:
  cx,cy   :  coordinate del centro del rettangolo
  width   :  larghezza del rettangolo  
  height  :  altezza del rettangolo
Valori ritornati
  Struttura dati Vertex del rettangolo creato
-------------------------------------------------------------------------
*/
struct VertexType PrgOptimize2::Create2DVertex(float cx,float cy,float width,float height,float z)
{
	float xdim=width/2;
	float ydim=height/2;

	struct VertexType v;

	v.x[0]=-xdim+cx;
	v.x[1]=xdim+cx;
	v.x[2]=v.x[1];
	v.x[3]=v.x[0];

	v.y[0]=ydim+cy;
	v.y[2]=-ydim+cy;

	v.y[1]=v.y[0];
	v.y[3]=v.y[2];

	v.cx=cx;
	v.cy=cy;

	v.z=z;
	v.AuxData=0;

	return(v);
}

/*
-------------------------------------------------------------------------
Enlarge2DVertex:
  Scala una struttura dati Vertex
Parametri di ingresso:
  v       :  struttura dati da modificare
  val     :  aumento/diminuizione delle diagonali
Valori ritornati
  ritorna in v la struttura modificata
-------------------------------------------------------------------------
*/
void PrgOptimize2::Enlarge2DVertex(struct VertexType &v,float val)
{
	double a0[4];
	float  d[4];
	float newd=val*sqrt(2);

	for(int i=0;i<4;i++)
	{
		d[i]=sqrt((v.x[i]-v.cx)*(v.x[i]-v.cx)+(v.y[i]-v.cy)*(v.x[i]-v.cy))+newd;

		if((v.x[i]-v.cx)==0)
		{
			if((v.y[i]-v.cy)<0)
			{
				a0[i]=-PI/2;
			}
			else
			{
				a0[i]=PI/2;
			}
		}
		else
		{
			a0[i]=atan((v.y[i]-v.cy)/(v.x[i]-v.cx));
			if((v.x[i]-v.cx)<0)
			{
				a0[i]+=PI/2;
			}
		}
	}

	for(int i=0;i<4;i++)
	{
		v.x[i]=v.cx+d[i]*cos(a0[i]);
		v.y[i]=v.cy+d[i]*sin(a0[i]);
	}
}

/*
-------------------------------------------------------------------------
Rotate2DVertex:
  Ruota una struttura dati Vertex
Parametri di ingresso:
  v       :  struttura dati da modificare
  rot     :  rotazione in gradi richiesta
Valori ritornati
  ritorna in v la struttura modificata
-------------------------------------------------------------------------
*/
void PrgOptimize2::Rotate2DVertex(struct VertexType &v,float rot)
{
	if(rot!=0)
	{
		struct VertexType tmpv=v;

		float cosrot=cos(rot*2*PI/360.0);
		float sinrot=sin(rot*2*PI/360.0);

		for(int i=0;i<4;i++)
		{
			tmpv.x[i]=(v.x[i]-v.cx)*cosrot-(v.y[i]-v.cy)*sinrot+v.cx;
			tmpv.y[i]=(v.x[i]-v.cx)*sinrot+(v.y[i]-v.cy)*cosrot+v.cy;
		}

		v=tmpv;
	}
}

/*
-------------------------------------------------------------------------
CheckVertexIntersect
Controlla se due strutture dati Vertex si intersecano tra loro
Parametri di ingresso:
  v1      :  struttura dati 1
  v2      :  struttura dati 2
Valori ritornati
  ritorna 1 se i vertici si intersecano, 0 altrimenti
-------------------------------------------------------------------------
*/
int PrgOptimize2::CheckVertexIntersect(struct VertexType v1,struct VertexType v2)
{
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			if(SegmentIntersect(v1.x[i],v1.y[i],v1.x[(i+1)%4],v1.y[(i+1)%4],v2.x[j],v2.y[j],v2.x[(j+1)%4],v2.y[(j+1)%4]))
				return(1);
		}
	}

	return(0);
}

/*
-------------------------------------------------------------------------
CalcComp2DVertex
Calcola la struttura dati Vertex rappresentante un componente del programma
di assemblaggio
Parametri di ingresso:
  v       : variabile di ritorna della struttura calcolata
  n       : numero del record nel programma di assemblaggio
Valori ritornati
  ritorna in v la struttura dati calcolata
-------------------------------------------------------------------------
*/
void PrgOptimize2::CalcComp2DVertex(struct VertexType &v,int n)
{
	struct TabPrg tdat=AllPrg[n];
	struct Zeri zdat=AllZer[tdat.scheda];
	struct CarDat cdat=AllCar[GetCarRec(tdat.Caric)];
	SPackageData* pdat = &currentLibPackages[cdat.C_PackIndex-1];

	float cx=zdat.Z_xzero+tdat.XMon;
	float cy=zdat.Z_yzero+tdat.YMon;

	float xdim,ydim;

	if( pdat->centeringMode == CenteringMode::NONE || pdat->centeringMode == CenteringMode::EXTCAM )
	{
		xdim=pdat->x;
		ydim=pdat->y;
	}
	else
	{
		if(pdat->orientation < 0)
		{
			pdat->orientation += 360.0;
		}

		if((pdat->orientation >= 0 && pdat->orientation < 90) || (pdat->orientation >= 180 && pdat->orientation < 270))
		{
			xdim=pdat->snpX;
			ydim=pdat->snpY;
		}
		else
		{
			xdim=pdat->snpY;
			ydim=pdat->snpX;
		}
	}

	v=Create2DVertex(cx,cy,xdim,ydim,pdat->z);

	v.AuxData=(void *)(AuxVertexList+n);

}

/*
-------------------------------------------------------------------------
CalcCompUge2DVertex
Data una struttura dati Vertex relativa ad un componente calcola quella
relativa all'ugello per prelevare quel componente
Parametri di ingresso:
  ugev    : variabile di ritorno della struttura calcolata
  compv   : struttura dati vertex del componente
  deltauge:
Valori ritornati
  ritorna in ugev la struttura dati calcolata
-------------------------------------------------------------------------
*/
void PrgOptimize2::CalcCompUge2DVertex(struct VertexType &ugev,struct VertexType compv,float deltauge)
{
	float dx=2*ERRPREL_DELTA+deltauge;
	float dy=2*ERRPREL_DELTA+deltauge;

	float cdx=fabs(compv.x[1]-compv.x[0])+deltauge;
	float cdy=fabs(compv.y[0]-compv.y[3])+deltauge;

	if(dx>cdx)
	{
		dx=cdx;
	}

	if(dy>cdy)
	{
		dy=cdy;
	}

	struct VertexType tmp=Create2DVertex(compv.cx,compv.cy,dx,dy,compv.z);

	tmp.enable=compv.enable;
	tmp.AuxData=compv.AuxData;

	if(CheckVertexIntersect(compv,tmp))
	{
		if(tmp.x[0]<compv.x[0])
		{
			tmp.x[0]=compv.x[0]-deltauge;
			tmp.x[3]=compv.x[3]-deltauge;
		}
		if(tmp.x[1]>compv.x[1])
		{
			tmp.x[1]=compv.x[1]+deltauge;
			tmp.x[2]=compv.x[2]+deltauge;
		}
		if(tmp.y[0]>compv.y[0])
		{
			tmp.y[0]=compv.y[0]+deltauge;
			tmp.y[1]=compv.y[1]+deltauge;
		}
		if(tmp.y[2]<compv.y[2])
		{
			tmp.y[2]=compv.y[2]-deltauge;
			tmp.y[3]=compv.y[3]-deltauge;
		}
		ugev=tmp;
	}
	else
	{
		if(((compv.x[0]<tmp.x[0]) && (compv.x[1]>tmp.x[1])) && ((compv.y[0]>tmp.y[0]) && (compv.y[3]<tmp.y[3])))
		{
			ugev=compv;
		}
		else
		{
			ugev=tmp;
		}
	}
}
