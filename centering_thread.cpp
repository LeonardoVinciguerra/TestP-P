//---------------------------------------------------------------------------
//
// Name:        centering_thread.cpp
// Author:      Gabriel Ferri
// Created:     23/11/2011
// Description:
//
//---------------------------------------------------------------------------
#include "centering_thread.h"

#include <boost/thread.hpp>
#include "lnxdefs.h"
#include "q_oper.h"
#include "q_packages.h"
#include "tws_sniper.h"

//serve per profiler tempi
#include "q_assem.h"

#ifdef __SNIPER
#include "sniper.h"
#endif


extern CfgHeader QHeader;
extern SniperModule* Sniper1;
extern SniperModule* Sniper2;
extern SPackageOffsetData currentLibOffsetPackages[MAXPACK];



	//-----------------//
	//   Global Vars   //
	//-----------------//

boost::thread centeringThread;
boost::mutex centeringMutex;

struct CenteringData
{
	CenteringData()
	{
		StartCycleReq = false;
		Running = false;
	}

	// centering thread flags
	bool StartCycleReq;
	bool Running;

	// centering result
	int Result;
	float Position1;
	float Position2;
	float Shadow1;
	float Shadow2;

	// raw centering results
	int Status[4];
	int Angle[4];
	float Position[4];
	float Shadow[4];

	// operative data

	// pre centering angle
	int PreAngle;
	// place angle
	int PlaceAngle;
	// package data
	const SPackageData* Package;

	// phase 1 steps
	int Steps;

	bool FourMeasuresMode;
};

CenteringData centeringData[2];


enum eHeadCenteringPhase
{
	CC_NOT_STARTED = 0,
	CC_SEARCH_READY,
	CC_SEARCH_1_STARTED,
	CC_SEARCH_2_STARTED,
	CC_SEARCH_3_STARTED,
	CC_SEARCH_4_STARTED,
	CC_PLACE_ROTATION_STARTED,
	CC_COMPLETED
};

eHeadCenteringPhase head_centering_phase[2] = { CC_NOT_STARTED, CC_NOT_STARTED };



#ifdef __SNIPER

//---------------------------------------------------------------------------
// Inizia ciclo sniper se possibile (nessun movimento in theta in corso)
// Ritorna:  -1 errore
//            1 pronto per il centraggio
//            2 centraggio non necessario
//---------------------------------------------------------------------------
int Centering_Setup( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	centeringData[nozzle-1].Result = STATUS_OK;

	// setta velocita' di rotazione
	SetNozzleRotSpeed_Index( nozzle, centeringData[nozzle-1].Package->speedRot );

	// se centraggio non necessario esci
	if( centeringData[nozzle-1].Package->centeringMode == CenteringMode::NONE )
	{
		return 2;
	}

	//TODO: aggiornare finestra
	SniperModes::inst().useRecord( centeringData[nozzle-1].Package->snpMode );


	// pre-centering rotation
	//------------------------------------------------------

	float pre_angle = -SniperModes::inst().getRecord().prerotation - 90;

	while( pre_angle >= 360 )
		pre_angle -= 360;
	while( pre_angle <= -360 )
		pre_angle += 360;

	PuntaRotDeg( pre_angle, nozzle, BRUSH_ABS );


	// setup sniper parameters
	//------------------------------------------------------

	short int k_encoder = ( nozzle == 1 ) ? QHeader.Enc_step1 : QHeader.Enc_step2;
	centeringData[nozzle-1].Steps = int( SniperModes::inst().getRecord().search_angle * k_encoder / 360.0 ) - SniperModes::inst().getRecord().discard_nframes;

	if( SniperModes::inst().getRecord().algorithm == 1 )
	{
		// use all encoder steps and PULSES for FPGA, find first minimum
		if( !sniper->SetUseEveryFrames() || !sniper->SetPulsesForFPGA( centeringData[nozzle-1].Steps ) )
		{
			return -1;
		}
	}
	else if( SniperModes::inst().getRecord().algorithm == 2 )
	{
		// skip one encoder step and set PULSES for FPGA, find first minimum
		if( !sniper->SetUseHalfFrames() || !sniper->SetPulsesForFPGA( centeringData[nozzle-1].Steps/2 ) )
		{
			return -1;
		}
	}

	if( !sniper->SetUseStandardAlgorithm() )
	{
		return -1;
	}

	return 1;
}

//---------------------------------------------------------------------------
// Inizia ciclo sniper se possibile (nessun movimento in theta in corso)
// Ritorna:  -1 errore
//            0 attendi
//            1 centraggio iniziato
//---------------------------------------------------------------------------
int Centering_Start_1( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	// attende punta ferma (theta)
	if( !Check_PuntaRot( nozzle ) )
	{
		return 0;
	}
	Wait_EncStop( nozzle );

	// attende punta ferma (z)
	PuntaZPosWait( nozzle );


	// start cycle
	//------------------------------------------------------

	centeringData[nozzle-1].PreAngle = GetPuntaRotStep( nozzle );

	// salvo velocita'/accelerazione corrente
	SaveNozzleRotSpeed( nozzle );

	// setto velocita'/accelerazione di ricerca
	PuntaRotSpeed( nozzle, SniperModes::inst().getRecord().speed );
	PuntaRotAcc( nozzle, SniperModes::inst().getRecord().acceleration );

	// invio comando inizio ciclo
	if( !sniper->StartFirst() )
	{
		// setto velocita'/accelerazione precedentemente impostata
		RestoreNozzleRotSpeed( nozzle );
		return -1;
	}

	// avvio movimento motore per effettuare la scansione del componente
	PuntaRotStep( centeringData[nozzle-1].Steps, nozzle, BRUSH_REL );
	return 1;
}

//---------------------------------------------------------------------------
// Prende i dati della ricerca del primo minimo
// Ritorna:  -1 errore
//            0 attendi
//            1 continua ciclo
//---------------------------------------------------------------------------
int Centering_Get_1( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	// check fine rotazione punta
	//NOTA: non e' necessario assicurarsi che il motore sia perfettamente a regime
	if( !Check_PuntaRot( nozzle ) )
	{
		return 0;
	}

	// setto velocita'/accelerazione precedentemente impostata
	RestoreNozzleRotSpeed( nozzle );

	// leggo risultati fase 1
	if( !sniper->GetFirst( centeringData[nozzle-1].Status[0], centeringData[nozzle-1].Angle[0], centeringData[nozzle-1].Position[0], centeringData[nozzle-1].Shadow[0] ) )
	{
		centeringData[nozzle-1].Result = STATUS_COM_ERROR;
		return -1;
	}

	// controllo stato
	if( centeringData[nozzle-1].Status[0] != STATUS_OK && centeringData[nozzle-1].Status[0] != STATUS_R_BLK )
	{
		centeringData[nozzle-1].Result = centeringData[nozzle-1].Status[0];
		return -1;
	}

	// controllo valore angolo "ombra minima"
	if( centeringData[nozzle-1].Angle[0] <= centeringData[nozzle-1].PreAngle || centeringData[nozzle-1].Angle[0] >= centeringData[nozzle-1].PreAngle + centeringData[nozzle-1].Steps )
	{
		centeringData[nozzle-1].Result = STATUS_ENCODER;
		return -1;
	}

	centeringData[nozzle-1].Position1 = centeringData[nozzle-1].Position[0];
	centeringData[nozzle-1].Shadow1 = centeringData[nozzle-1].Shadow[0];

	return 1;
}

//---------------------------------------------------------------------------
// Ruota punta per misura secondo minimo
//---------------------------------------------------------------------------
int Centering_Start_2( int nozzle )
{
	// Ruoto il componente di 90deg rispetto alla posizione di ombra minima
	int pos = Deg2Step( 90, nozzle ) + centeringData[nozzle-1].Angle[0];
	PuntaRotStep( pos, nozzle, BRUSH_ABS );

	return 1;
}

//---------------------------------------------------------------------------
// Prende i dati della ricerca del secondo minimo
// Ritorna:  -1 errore
//            0 attendi
//            1 centraggio concluso
//            3 passare alla misura della faccia 3
//            4 passare alla misura della faccia 4
//---------------------------------------------------------------------------
int Centering_Get_2( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	//check fine rotazione punta
	if( !Check_PuntaRot( nozzle ) )
	{
		return 0;
	}
	Wait_EncStop( nozzle );

	// leggo risultati fase 2
	if( !sniper->MeasureOnce( centeringData[nozzle-1].Status[1], centeringData[nozzle-1].Angle[1], centeringData[nozzle-1].Position[1], centeringData[nozzle-1].Shadow[1] ) )
	{
		centeringData[nozzle-1].Result = STATUS_COM_ERROR;
		return -1;
	}

	// controllo stato
	if( centeringData[nozzle-1].Status[1] != STATUS_OK && centeringData[nozzle-1].Status[1] != STATUS_R_BLK )
	{
		centeringData[nozzle-1].Result = centeringData[nozzle-1].Status[1];
		return -1;
	}

	centeringData[nozzle-1].Position2 = centeringData[nozzle-1].Position[1];
	centeringData[nozzle-1].Shadow2 = centeringData[nozzle-1].Shadow[1];

	// controllo prime 2 fasi centraggio
	centeringData[nozzle-1].FourMeasuresMode = false;

	float four_measures_limit = getSniperType1ComponentLimit( nozzle, CCenteringReservedParameters::inst().getData() );

	if( centeringData[nozzle-1].Package->snpX >= four_measures_limit || centeringData[nozzle-1].Package->snpY >= four_measures_limit )
	{
		centeringData[nozzle-1].FourMeasuresMode = true;

		// Elaboro la faccia opposta del lato minimo assoluto (terza faccia)
		return 3;
	}

	// Se necessario elaboro la faccia opposta del lato minimo assoluto (terza faccia)
	if( centeringData[nozzle-1].Status[0] == STATUS_R_BLK )
	{
		return 3;
	}

	// Se necessario elaboro la faccia opposta del lato minimo relativo (quarta faccia)
	if( centeringData[nozzle-1].Status[1] == STATUS_R_BLK )
	{
		return 4;
	}

	return 1;
}

//---------------------------------------------------------------------------
// Ruota punta per misura terzo minimo
//---------------------------------------------------------------------------
int Centering_Start_3( int nozzle, int angleStep )
{
	PuntaRotStep( Deg2Step( angleStep, nozzle ), nozzle, BRUSH_REL );
	return 1;
}

//---------------------------------------------------------------------------
// Prende i dati della ricerca del terzo minimo
// Ritorna:  -1 errore
//            0 attendi
//            1 centraggio concluso
//            4 passare alla misura della faccia 4
//---------------------------------------------------------------------------
int Centering_Get_3( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	//check fine rotazione punta
	if( !Check_PuntaRot( nozzle ) )
	{
		return 0;
	}
	Wait_EncStop( nozzle );

	// leggo risultati fase 3
	if( !sniper->MeasureOnce( centeringData[nozzle-1].Status[2], centeringData[nozzle-1].Angle[2], centeringData[nozzle-1].Position[2], centeringData[nozzle-1].Shadow[2] ) )
	{
		centeringData[nozzle-1].Result = STATUS_COM_ERROR;
		return -1;
	}

	// controllo stato
	if( centeringData[nozzle-1].Status[2] != STATUS_OK && centeringData[nozzle-1].Status[2] != STATUS_R_BLK )
	{
		centeringData[nozzle-1].Result = centeringData[nozzle-1].Status[2];
		return -1;
	}

	//CCal espresso in um e rispetto al sistema di riferimento sniper e non in quello "laser compatibile"
	float CCal_um = ((SNIPER_RIGHT_USABLE - SNIPER_LEFT_USABLE + 1) * sniper->GetKPixelUm()) - (GetCCal(nozzle) * 1000.0);


	if( centeringData[nozzle-1].Status[2] == STATUS_R_BLK || centeringData[nozzle-1].FourMeasuresMode )
	{
		float shadow0 = CCal_um - (centeringData[nozzle-1].Position[0] - centeringData[nozzle-1].Shadow[0]/2.0 );
		float shadow180 = CCal_um - ( centeringData[nozzle-1].Position[2] - centeringData[nozzle-1].Shadow[2]/2.0 );

		centeringData[nozzle-1].Position1 = CCal_um - ( shadow0 - shadow180 ) / 2;
		centeringData[nozzle-1].Shadow1 = shadow0 + shadow180;
	}
	else if ( centeringData[nozzle-1].Status[2] == STATUS_OK )
	{
		// questa faccia e' not-blocked, prendo questa come minimo
		centeringData[nozzle-1].Position1 = CCal_um + ( CCal_um - centeringData[nozzle-1].Position[2] );
		centeringData[nozzle-1].Shadow1 = centeringData[nozzle-1].Shadow[2];
	}
	else
	{
		// error
		centeringData[nozzle-1].Result = STATUS_NO_MIN;
		return -1;
	}

	// Se necessario elaboro la faccia opposta del lato minimo relativo (quarta faccia)
	if( centeringData[nozzle-1].FourMeasuresMode || centeringData[nozzle-1].Status[1] == STATUS_R_BLK )
	{
		return 4;
	}

	return 1;
}

//---------------------------------------------------------------------------
// Ruota punta per misura quarto minimo
//---------------------------------------------------------------------------
int Centering_Start_4( int nozzle, int angleStep )
{
	PuntaRotStep( Deg2Step( angleStep, nozzle ), nozzle, BRUSH_REL );
	return 1;
}

//---------------------------------------------------------------------------
// Prende i dati della ricerca del quarto minimo
// Ritorna:  -1 errore
//            0 attendi
//            1 centraggio concluso
//---------------------------------------------------------------------------
int Centering_Get_4( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	//check fine rotazione punta
	if( !Check_PuntaRot( nozzle ) )
	{
		return 0;
	}
	Wait_EncStop( nozzle );

	// leggo risultati fase 4
	if( !sniper->MeasureOnce( centeringData[nozzle-1].Status[3], centeringData[nozzle-1].Angle[3], centeringData[nozzle-1].Position[3], centeringData[nozzle-1].Shadow[3] ) )
	{
		centeringData[nozzle-1].Result = STATUS_COM_ERROR;
		return -1;
	}

	// controllo stato
	if( centeringData[nozzle-1].Status[3] != STATUS_OK && centeringData[nozzle-1].Status[3] != STATUS_R_BLK )
	{
		centeringData[nozzle-1].Result = centeringData[nozzle-1].Status[3];
		return -1;
	}

	//CCal espresso in um e rispetto al sistema di riferimento sniper e non in quello "laser compatibile"
	float CCal_um = (( SNIPER_RIGHT_USABLE - SNIPER_LEFT_USABLE + 1 ) * sniper->GetKPixelUm()) - (GetCCal(nozzle) * 1000.0);

	if( centeringData[nozzle-1].Status[3] == STATUS_R_BLK || centeringData[nozzle-1].FourMeasuresMode )
	{
		float shadow90 = CCal_um - ( centeringData[nozzle-1].Position[1] - centeringData[nozzle-1].Shadow[1]/2.0 );
		float shadow270 = CCal_um - ( centeringData[nozzle-1].Position[3] - centeringData[nozzle-1].Shadow[3]/2.0 );

		centeringData[nozzle-1].Position2 = CCal_um - ( shadow90 - shadow270 ) / 2;
		centeringData[nozzle-1].Shadow2 = shadow90 + shadow270;
	}
	else if( centeringData[nozzle-1].Status[3] == STATUS_OK )
	{
		// questa faccia e' not-blocked, prendo questa come minimo
		centeringData[nozzle-1].Position2 = CCal_um + ( CCal_um - centeringData[nozzle-1].Position[3] );
		centeringData[nozzle-1].Shadow2 = centeringData[nozzle-1].Shadow[3];
	}
	else
	{
		// error
		centeringData[nozzle-1].Result = STATUS_NO_MIN;
		return -1;
	}

	return 1;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
//TODO: inserire ri-tentativo di centraggio se fallisce il primo come parametro generale o per package ???
bool Centering_GetResults( int nozzle, int errmode )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	if( sniper->IsOnError() )
	{
		// errore irreversibile nel centratore sniper
		centeringData[nozzle-1].Result = STATUS_COM_ERROR;
		centeringData[nozzle-1].Position1 = 0.f;
		centeringData[nozzle-1].Position2 = 0.f;
		centeringData[nozzle-1].Shadow1 = 0.f;
		centeringData[nozzle-1].Shadow2 = 0.f;
		return false;
	}

	const SPackageData* package = centeringData[nozzle-1].Package;

	// Se non e' richiesto il centraggio...
	if( package->centeringMode == CenteringMode::NONE || QParam.DemoMode || (QHeader.modal & NOLASCENT_MASK) || Get_OnFile() )
	{
		centeringData[nozzle-1].Result = STATUS_OK;
		centeringData[nozzle-1].Position1 = 0.f;
		centeringData[nozzle-1].Position2 = 0.f;
		centeringData[nozzle-1].Shadow1 = 0.f;
		centeringData[nozzle-1].Shadow2 = 0.f;
		return true;
	}

	float dim1 = MIN( package->snpX, package->snpY );
	float dim2 = MAX( package->snpX, package->snpY );

	// Larghezza misurata
	float w1 = centeringData[nozzle-1].Shadow1 / 1000.0;
	float w2 = centeringData[nozzle-1].Shadow2 / 1000.0;

	// Se grandezza non compatibile con le dimensioni memorizzate nel package...
	if( fabs(dim1-w1) > package->snpTolerance && centeringData[nozzle-1].Result == STATUS_OK )
	{
		centeringData[nozzle-1].Result = CEN_ERR_DIMX;
	}
	else if( fabs(dim2-w2) > package->snpTolerance && centeringData[nozzle-1].Result == STATUS_OK )
	{
		centeringData[nozzle-1].Result = CEN_ERR_DIMX;
	}

	if( centeringData[nozzle-1].Result != STATUS_OK )
	{
		centeringData[nozzle-1].Position1 = 0;
		centeringData[nozzle-1].Position2 = 0;
		return false;
	}

	// Delta dal centro del componente
	float CCal = GetCCal( nozzle );
	float delta1 = ( ((SNIPER_RIGHT_USABLE-SNIPER_LEFT_USABLE+1) * sniper->GetKPixelUm() - centeringData[nozzle-1].Position1 ) / 1000.0 ) - CCal;
	float delta2 = ( ((SNIPER_RIGHT_USABLE-SNIPER_LEFT_USABLE+1) * sniper->GetKPixelUm() - centeringData[nozzle-1].Position2 ) / 1000.0 ) - CCal;

	// Calcolo degli spostamenti da effettuare per compensare il fatto che il componente non viene prelevato e ruotato attorno al suo baricentro
	if( nozzle == 1 )
	{
		centeringData[nozzle-1].Position1 = -delta1;
		centeringData[nozzle-1].Position2 = -delta2;
	}
	else
	{
		centeringData[nozzle-1].Position1 = delta1;
		centeringData[nozzle-1].Position2 = delta2;
	}

	// Angolo totale da assegnare alla rotazione del componente per posizionarlo
	float rot_degree = Step2Deg( centeringData[nozzle-1].PlaceAngle, nozzle );

	double segment = sqrt( pow( centeringData[nozzle-1].Position1, 2 ) + pow( centeringData[nozzle-1].Position2, 2 ) );
	double beta = atan2( centeringData[nozzle-1].Position1, centeringData[nozzle-1].Position2 ) + DTOR(rot_degree);

	centeringData[nozzle-1].Position1 = segment * cos( beta );
	centeringData[nozzle-1].Position2 = segment * sin( beta );

	// Se le correzione sono molto piccole (e quindi non effettuabili dalle teste), si mettono a 0
	if( fabs( centeringData[nozzle-1].Position1 ) < 0.0001 )
	{
		centeringData[nozzle-1].Position1 = 0;
	}
	if( fabs( centeringData[nozzle-1].Position2 ) < 0.0001 )
	{
		centeringData[nozzle-1].Position2 = 0;
	}

	return true;
}

//---------------------------------------------------------------------------
// Esegue rotazione di deposito
// Ritorna: 1 continua ciclo
//---------------------------------------------------------------------------
int Centering_Start_PlaceRotation( int nozzle, int phaseAngle )
{
	int a = centeringData[nozzle-1].PlaceAngle - Deg2Step( phaseAngle, nozzle );
	PuntaRotStep( a, nozzle, BRUSH_REL );

	return 1;
}

//---------------------------------------------------------------------------
// Esegue rotazione di deposito
// Ritorna: 0 attendi
//          1 centraggio concluso
//---------------------------------------------------------------------------
int Centering_Wait_PlaceRotation( int nozzle )
{
	// attende punta ferma (theta)
	if( !Check_PuntaRot( nozzle ) )
	{
		return 0;
	}
	Wait_EncStop( nozzle );

	return 1;
}

//---------------------------------------------------------------------------
// Avanza lo stato del centraggio
//---------------------------------------------------------------------------
void NextCenteringPhase( int nozzle, int errmode )
{
	int ret_val;
	switch( head_centering_phase[nozzle-1] )
	{
		case CC_NOT_STARTED:
			ret_val = Centering_Setup( nozzle );
			if( ret_val == 1 )
			{
				head_centering_phase[nozzle-1] = CC_SEARCH_READY;
				ASSEMBLY_PROFILER_MEASURE("sniper %d setup parameters",nozzle);
			}
			else if( ret_val == 2 )
			{
				Centering_GetResults( nozzle, errmode );
				Centering_Start_PlaceRotation( nozzle, 0 );
				head_centering_phase[nozzle-1] = CC_PLACE_ROTATION_STARTED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d place rotation started",nozzle);
			}
			else if( ret_val == -1 )
			{
				Centering_GetResults( nozzle, errmode );
				head_centering_phase[nozzle-1] = CC_COMPLETED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d setup error",nozzle);
			}
			break;

		case CC_SEARCH_READY:
			ret_val = Centering_Start_1( nozzle );
			if( ret_val == 1 )
			{
				head_centering_phase[nozzle-1] = CC_SEARCH_1_STARTED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 1 started",nozzle);
			}
			else if( ret_val == -1 )
			{
				Centering_GetResults( nozzle, errmode );
				head_centering_phase[nozzle-1] = CC_COMPLETED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 0 error",nozzle);
			}
			break;

		case CC_SEARCH_1_STARTED:
			ret_val = Centering_Get_1(nozzle);
			if( ret_val == 1 )
			{
				Centering_Start_2(nozzle); // ad oggi ritorna sempre 1
				head_centering_phase[nozzle-1] = CC_SEARCH_2_STARTED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 2 started",nozzle);
			}
			else if( ret_val == -1 )
			{
				Centering_GetResults( nozzle, errmode );
				head_centering_phase[nozzle-1] = CC_COMPLETED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 1 error",nozzle);
			}
			break;

		case CC_SEARCH_2_STARTED:
			ret_val = Centering_Get_2(nozzle);
			if( ret_val == 1 )
			{
				Centering_GetResults( nozzle, errmode );
				Centering_Start_PlaceRotation( nozzle, 90 );
				head_centering_phase[nozzle-1] = CC_PLACE_ROTATION_STARTED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d place rotation started",nozzle);
			}
			else if( ret_val == 3 )
			{
				Centering_Start_3( nozzle, 90 ); // ad oggi ritorna sempre 1
				head_centering_phase[nozzle-1] = CC_SEARCH_3_STARTED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 3 started",nozzle);
			}
			else if( ret_val == 4 )
			{
				Centering_Start_4( nozzle, 180 ); // ad oggi ritorna sempre 1
				head_centering_phase[nozzle-1] = CC_SEARCH_4_STARTED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 4 started",nozzle);
			}
			else if( ret_val == -1 )
			{
				Centering_GetResults( nozzle, errmode );
				head_centering_phase[nozzle-1] = CC_COMPLETED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 2 error",nozzle);
			}
			break;

		case CC_SEARCH_3_STARTED:
			ret_val = Centering_Get_3(nozzle);
			if( ret_val == 1 )
			{
				Centering_GetResults( nozzle, errmode );
				Centering_Start_PlaceRotation( nozzle, 180 );
				head_centering_phase[nozzle-1] = CC_PLACE_ROTATION_STARTED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d place rotation started",nozzle);
			}
			else if( ret_val == 4 )
			{
				Centering_Start_4( nozzle, 90 ); // ad oggi ritorna sempre 1
				head_centering_phase[nozzle-1] = CC_SEARCH_4_STARTED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 4 started",nozzle);
			}
			else if( ret_val == -1 )
			{
				Centering_GetResults( nozzle, errmode );
				head_centering_phase[nozzle-1] = CC_COMPLETED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 3 error",nozzle);
			}
			break;

		case CC_SEARCH_4_STARTED:
			ret_val = Centering_Get_4( nozzle );
			if( ret_val == 1 )
			{
				Centering_GetResults( nozzle, errmode );
				Centering_Start_PlaceRotation( nozzle, 270 );
				head_centering_phase[nozzle-1] = CC_PLACE_ROTATION_STARTED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d place rotation started",nozzle);
			}
			else if( ret_val == -1 )
			{
				Centering_GetResults( nozzle, errmode );
				head_centering_phase[nozzle-1] = CC_COMPLETED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d phase 3 error",nozzle);
			}
			break;

		case CC_PLACE_ROTATION_STARTED:
			ret_val = Centering_Wait_PlaceRotation( nozzle );
			if( ret_val == 1 )
			{
				head_centering_phase[nozzle-1] = CC_COMPLETED;
				ASSEMBLY_PROFILER_MEASURE("sniper %d centering completed",nozzle);
			}
			break;

		case CC_COMPLETED:
			break;
	}
}

#endif


//---------------------------------------------------------------------------
// Funzione principale del ciclo di centraggio
//---------------------------------------------------------------------------
void CenteringCycle()
{
	bool running;

	// questo ritardo serve per permettere l'esecuzione del thread principale
	boost::posix_time::milliseconds sleepTime( 10 );

	for(;;)
	{
		// punto di interruzione: serve per poter terminare il thread
		boost::this_thread::interruption_point();

		// for each nozzle...
		for( int i = 0; i < 2; i++ )
		{
			// start centering cycle condition
			//------------------------------------------------------
			centeringMutex.lock();
			if( centeringData[i].StartCycleReq  && !centeringData[i].Running )
			{
				centeringData[i].StartCycleReq = false;
				centeringData[i].Running = true;
				head_centering_phase[i] = CC_NOT_STARTED;
			}
			running = centeringData[i].Running;
			centeringMutex.unlock();

			// pump centering cycle
			//------------------------------------------------------
			if( running )
			{
				NextCenteringPhase( i+1, 0 );

				if( head_centering_phase[i] == CC_COMPLETED )
				{
					centeringMutex.lock();
					centeringData[i].Running = false;
					centeringMutex.unlock();
				}
			}

			boost::this_thread::sleep( sleepTime );
		}
	}
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void StartCenteringThread()
{
	centeringThread = boost::thread( CenteringCycle );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void StopCenteringThread()
{
	centeringThread.interrupt();
	centeringThread.join();
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bool StartCentering( int nozzle, int placeAngle, const SPackageData* package )
{
	if( Get_OnFile() )
		return true;

	bool ret = false;

	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	if( !centeringData[nozzle-1].Running )
	{
		centeringData[nozzle-1].PlaceAngle = placeAngle;
		centeringData[nozzle-1].Package = package;
		centeringData[nozzle-1].StartCycleReq = true;
		ret = true;
	}

	centeringMutex.unlock();

	if( !ret )
		ASSEMBLY_PROFILER_MEASURE( "centering cycle nozzle %d already running", nozzle );
	else
		ASSEMBLY_PROFILER_MEASURE( "centering cycle nozzle %d start", nozzle );

	return ret;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bool IsCenteringCompleted( int nozzle )
{
	bool ret;

	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	if( !centeringData[nozzle-1].StartCycleReq && !centeringData[nozzle-1].Running )
		ret = true;
	else
		ret = false;

	centeringMutex.unlock();

	return ret;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int GetCenteringResult( int nozzle, CenteringResultData& data )
{
	while( !centeringMutex.try_lock() )
	{
		delay( 1 );
	}

	data.Result = centeringData[nozzle-1].Result;
	data.Position1 = centeringData[nozzle-1].Position1;
	data.Position2 = centeringData[nozzle-1].Position2;
	data.Shadow1 = centeringData[nozzle-1].Shadow1;
	data.Shadow2 = centeringData[nozzle-1].Shadow2;

	centeringMutex.unlock();

	return data.Result;
}
