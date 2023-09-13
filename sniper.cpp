//-----------------------------------------------------------------------------
// File: Sniper.cpp
//
// Desc: Implementation of the Sniper class
//-----------------------------------------------------------------------------

#ifdef __SNIPER

#include "sniper.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include "q_cost.h"
#include "tws_sniper.h"
#include "q_snprt.h"

#include "filefn.h"
#include "msglist.h"
#include "q_gener.h"
#include "q_help.h"
#include "q_grcol.h"
#include "q_oper.h"
#include "q_graph.h"
#include "q_wind.h"
#include "q_tabe.h"
#include "q_fox.h"
#include "comaddr.h"

#include "keyutils.h"
#include "lnxdefs.h"
#include "getmac.h"

#include "gui_submenu.h"
#include "gui_functions.h"
#include "gui_defs.h"

#include "c_pan.h"

#include "centering_thread.h"

#include <mss.h>


#define SNIPER_IMG_QUALITY_MIN		220
#define SNIPER_IMG_QUALITY_MAX		240

#define SNIPER_IMAGETEST_MUL		4


// Struttura contenente i dati di configurazione della macchina
extern CfgHeader QHeader;
extern CfgParam  QParam;


// Oggetto sniper globale
SniperModule* Sniper1 = NULL;
SniperModule* Sniper2 = NULL;


//---------------------------------------------------------------------------------
// Abilita i centratori sniper
//---------------------------------------------------------------------------------
void Snipers_Enable()
{
	if( Get_OnFile() )
		return;

	if( Sniper1 )
	{
		Sniper1->Enable();
	}
	if( Sniper2 )
	{
		Sniper2->Enable();
	}
}


//---------------------------------------------------------------------------------
// Calibra costante pixel/mm sensore sniper
// Ritorna 0 se errore, 1 altrimenti
//---------------------------------------------------------------------------------
int Sniper_Calibrate( int nozzle )
{
	if( Get_OnFile() )
		return 1;

	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	if( sniper->IsOnError() )
		return 0;

	PuntaZPosMm( nozzle, SNIPER_CAL_PIX_UM_DEFZPOS );
	PuntaZPosWait( nozzle );

	int measure_status;
	int measure_angle;
	int measure_position;
	int measure_shadow;

	sniper->MeasureOnce_Pixels( measure_status, measure_angle, measure_position, measure_shadow );

	if( measure_status != STATUS_OK )
	{
		char buf[160];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_05013), nozzle );
		W_Mess( buf );
		PuntaZSecurityPos( nozzle );
		PuntaZPosWait( nozzle );
		return 0;
	}

	float newK = ( SNIPER_CAL_PIX_UM_DEFSIZE * 1000.0 ) / measure_shadow;

	if( fabs( newK - QHeader.sniper_kpix_um[nozzle-1] ) > MAX_VAR_SNIPER_CAL_PIX_UM )
	{
		char buf[160];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_05014), nozzle, newK, QHeader.sniper_kpix_um[nozzle-1] );
		if( !W_Deci( 0,buf ) )
		{
			PuntaZSecurityPos( nozzle );
			PuntaZPosWait( nozzle );
			return 0;
		}
	}

	QHeader.sniper_kpix_um[nozzle-1] = newK;
	sniper->SetKPixelUm( newK );

	Mod_Cfg( QHeader );

	float ccal = GetCCal(nozzle) * 1000.f;

	PuntaZSecurityPos( nozzle );
	PuntaZPosWait( nozzle );

	//CCal espresso in um e rispetto al sistema di riferimento sniper e non in quello "laser compatibile"
	float CCal_um = (( SNIPER_RIGHT_USABLE - SNIPER_LEFT_USABLE + 1 ) * newK) - ccal;
	float measure_position_um = measure_position * newK;

	if( fabs( measure_position_um - CCal_um ) > MAX_ERROR_SNIPER_CCAL )
	{
		char buf[240];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_05156), nozzle, measure_position_um/1000.f, CCal_um/1000.f );
		W_Mess( buf );
	}

	return 1;
}

//---------------------------------------------------------------------------------
// Check presenza componente su punta specificata
// Ritorna 0 se componente non presente, 1 altrimenti
//---------------------------------------------------------------------------------
int Sniper_CheckComp( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	if( Get_OnFile() || sniper->IsOnError() || QParam.DemoMode )
		return 1;

	int measure_status;
	int measure_angle;
	int measure_position;
	int measure_shadow;

	sniper->MeasureOnce_Pixels( measure_status, measure_angle, measure_position, measure_shadow );

	if( measure_status == STATUS_EMPTY )
		return 0;

	return 1;
}

//---------------------------------------------------------------------------------
// Check presenza punta su zero
// Ritorna 0 se errore, 1 altrimenti
//---------------------------------------------------------------------------------
int Sniper_ZeroCheck( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	if( Get_OnFile() || sniper->IsOnError() )
		return 1;

	int ret = 1;

	// Scende di 6 passi
	PuntaZPosStep( nozzle, 6, ABS_MOVE );
	PuntaZPosWait( nozzle );

	int measure_status;
	int measure_angle;
	int measure_position;
	int measure_shadow;

	sniper->MeasureOnce_Pixels( measure_status, measure_angle, measure_position, measure_shadow );

	if( measure_status != STATUS_OK )
	{
		ret = 0;
	}
	else
	{
		// Sale di -6 passi
		PuntaZPosStep ( nozzle,-6,ABS_MOVE );
		PuntaZPosWait ( nozzle );

		sniper->MeasureOnce_Pixels( measure_status, measure_angle, measure_position, measure_shadow );

		if( measure_status == STATUS_OK )
		{
			ret = 0;
		}
	}

	// Porta punta a zero
	PuntaZPosStep( nozzle, 0, ABS_MOVE );
	PuntaZPosWait( nozzle );

	return ret;
}


//---------------------------------------------------------------------------------
// Azzera la punta sullo zero sniper
// Ritorna 0 se errore, 1 altrimenti
//---------------------------------------------------------------------------------
int Sniper_FindNozzleHeight( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	if( Get_OnFile() || sniper->IsOnError() )
		return 1;

	int pos = 0;
	int step;
	float stepTrasl = ( nozzle == 1 ) ? QHeader.Step_Trasl1 : QHeader.Step_Trasl2;
	int STEP_1MM = int ( 0.5*stepTrasl );


	int measure_status;
	int measure_angle;
	int measure_position;
	int measure_shadow;

	// Misurazione
	sniper->MeasureOnce_Pixels( measure_status, measure_angle, measure_position, measure_shadow );

	if( measure_status != STATUS_EMPTY )
	{
		// Cursore sullo sniper occorre salire
		step = -DELTASTEP1;
		while( fabs(float(pos)/stepTrasl) < ZPOSMAX )
		{
			// Muove punta di step passi
			PuntaZPosStep( nozzle, step, REL_MOVE, ZLIMITOFF );
			PuntaZPosWait( nozzle );

			// Incrementa contatore posizione attuale
			pos += step;

			// Misurazione
			sniper->MeasureOnce_Pixels( measure_status, measure_angle, measure_position, measure_shadow );

			// Se risultato aspettato, esci da ciclo...
			if( measure_status == STATUS_EMPTY )
				break;
			// ...altrimenti continua fino a superamento limite ZMAX
		}
	}
	else
	{
		// Cursore fuori dallo sniper (scendere)
		step = DELTASTEP1;
		while( fabs(float(pos)/stepTrasl) < ZPOSMAX )
		{
			// Muove punta di step passi
			PuntaZPosStep( nozzle, step, REL_MOVE, ZLIMITOFF );
			PuntaZPosWait( nozzle );

			// Incrementa contatore posizione attuale
			pos += step;

			// Misurazione
			sniper->MeasureOnce_Pixels( measure_status, measure_angle, measure_position, measure_shadow );

			// Se risultato aspettato, esci da ciclo...
			if( measure_status != STATUS_EMPTY )
				break;
			// ...altrimenti continua fino a superamento limite ZMAX
		}
	}


	if( fabs(float(pos)/stepTrasl)  >= ZPOSMAX )
		return 0; // Se exit per ZMAX -> return error

	// Si sale a -1mm
	PuntaZPosStep( nozzle, -STEP_1MM, REL_MOVE, ZLIMITOFF );
	PuntaZPosWait( nozzle );

	step = DELTASTEP;
	pos = 0;

	// Scende fino a che cursore su sniper
	while( float(pos)/stepTrasl < ZPOSMAX )
	{
		PuntaZPosStep( nozzle, step, REL_MOVE, ZLIMITOFF );
		PuntaZPosWait( nozzle );

		pos += step;

		// Misurazione
		sniper->MeasureOnce_Pixels( measure_status, measure_angle, measure_position, measure_shadow );

		if( measure_status != STATUS_EMPTY )
			break;
	}

	if( fabs( float(pos)/stepTrasl ) >= ZPOSMAX )
		return 0;	// Se exit per superamento lim. ZMAX ->return error

	// Memorizza posizione raggiunta
	int pos1 = pos;

	// Si scende a 1mm
	PuntaZPosStep( nozzle, STEP_1MM, REL_MOVE, ZLIMITOFF );
	PuntaZPosWait( nozzle );

	pos += STEP_1MM;
	step = -DELTASTEP;

	// Sale fino a che cursore no su snper
	while( fabs( float(pos)/stepTrasl ) < ZPOSMAX )
	{
		PuntaZPosStep( nozzle, step, REL_MOVE, ZLIMITOFF );
		PuntaZPosWait( nozzle );

		pos += step;

		// Misurazione
		sniper->MeasureOnce_Pixels( measure_status, measure_angle, measure_position, measure_shadow );

		if( measure_status == STATUS_EMPTY )
			break;
	}

	// Zero = (pos. raggiunta - pos. ris. precedente)/2
	PuntaZPosStep( nozzle, abs( (pos-pos1)/2 ), REL_MOVE, ZLIMITOFF ); // Vai a livello di zero
	PuntaZPosWait( nozzle );

	// setta pos. attuale come livello di zero
	PuntaZPosStep( nozzle, 0, SET_POS );
	FoxHead->SetZero( ( nozzle-1 )*2+1 );
	//FoxHead->MoveRel((nozzle-1)*2+1,0);
	//PuntaZPosStep(nozzle,1,REL_MOVE,ZLIMITOFF);

	return 1;
}


//---------------------------------------------------------------------------------
// Ricerca zero theta
// Parametri di ingresso:
//   check: se 1 controlla solamente la posizione di zero
// Ritorna 0 se errore, 1 altrimenti
//---------------------------------------------------------------------------------
int Sniper_FindZeroTheta( int nozzle, int check )
{
	if( Get_OnFile() )
	{
		return 1;
	}

	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	if( sniper->IsOnError() )
	{
		return 0;
	}

	CPan pan( -1, 1, MsgGetString(Msg_00291) ); // Please wait ...

	// Porta punta a quota ricerca zero theta
	int stepToZeroTheta = ( nozzle == 1 ) ? QHeader.Step_Trasl1*QHeader.zth_level1 : QHeader.Step_Trasl2*QHeader.zth_level2;
	PuntaZPosStep( nozzle, stepToZeroTheta );
	PuntaZPosWait( nozzle );

	if( !check )
	{
		// Setta pos theta attuale come zero
		PuntaRotDeg( 0, nozzle, BRUSH_RESET );
	}

	// Definisci costante passi encoder relativa alla punta
	short int k_encoder = ( nozzle == 1 ) ? QHeader.Enc_step1 : QHeader.Enc_step2;


	SniperModes::inst().useRecord( 0 );

	//==========================================================================
	//                   *** CARICAMENTO PARAMETRI SNIPER **

	int numSteps = int ( SniperModes::inst().getRecord().search_angle * k_encoder / 360.0 );

	// use all encoder steps and PULSES for FPGA
	if( !sniper->SetUseEveryFrames() || !sniper->SetPulsesForFPGA( numSteps ) )
	{
		return 0;
	}

	if( !sniper->SetUseModifiedAlgorithm() )
	{
		return 0;
	}

	//==========================================================================
	//                            *** CICLO SNIPER **

	// Salvo velocita' e accelerazione attuale
	SaveNozzleRotSpeed( nozzle );

	int ret = 0;

	while( 1 )
	{
		//prerotazione per azzeramento theta
		if( !check )
		{
			PuntaRotDeg( QHeader.zertheta_Prerot[nozzle-1], nozzle );
		}

		// Attende fine rotazione
		Wait_EncStop( nozzle );

		delay ( 40 );

		// Azzera conteggio encoder sniper
		sniper->Zero_Cmd();

		// Setto velocita'/accelerazione di ricerca
		PuntaRotSpeed( nozzle, SniperModes::inst().getRecord().speed );
		PuntaRotAcc( nozzle, SniperModes::inst().getRecord().acceleration );

		// Inizio ciclo di ricerca ombra minima
		if( !sniper->StartFirst() )
		{
			break;
		}

		// Avvio movimento motore per effettuare la scansione del componente
		PuntaRotStep( numSteps, nozzle, BRUSH_REL );
		// Attendo la fine del movimento
		Wait_EncStop( nozzle );

		// Leggo i risultati del ciclo di ricerca
		int measure_status;
		int measure_angle;
		float measure_position;
		float measure_shadow;

		if( !sniper->GetFirst( measure_status, measure_angle, measure_position, measure_shadow ) )
		{
			break;
		}

		// Errore di lettura del minimo
		if( measure_status != STATUS_OK && measure_status != STATUS_L_BLK && measure_status != STATUS_R_BLK )
		{
			char buf[80];
			snprintf( buf, sizeof(buf), MsgGetString(Msg_00720), nozzle );

			if( !W_Deci( 1, buf ) )
			{
				break;
			}
		}
		else
		{
			// Calcolo passi per punta in posizione di zero
			int stepsDiff = sniper->GetEncoder() - measure_angle;

			// Ruoto la punta in posizione di ombra minima
			PuntaRotStep ( -stepsDiff, nozzle, BRUSH_REL );


			int ok = 0;
			if( QHeader.brushlessNPoles[nozzle-1] == 4 )
			{
				bool left_valid[2];
				float left_value[2];

				ok = 1;

				for( int i = 0 ; i < 2; i++ )
				{
					if( i == 0 )
					{
						PuntaRotDeg( +90, nozzle, BRUSH_REL );
					}
					else
					{
						PuntaRotDeg( -180, nozzle, BRUSH_REL );
					}
					Wait_EncStop( nozzle );


					if( !sniper->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow ) )
					{
						ok = 0;
						break;
					}

					if( measure_status == STATUS_OK || measure_status == STATUS_R_BLK )
					{
						left_valid[i] = true;
						left_value[i] = measure_position - measure_shadow/2.0f;
					}
					else if( measure_status == STATUS_L_BLK || measure_status == STATUS_B_BLK )
					{
						left_valid[i] = false;
					}
					else
					{
						ok = 0;
						break;
					}
				}

				if( ok )
				{
					//angolo tra la posizione di ombra minima trovata e lo zero theta della macchina (per ora
					//sono define ma potranno diventare valori configurabili)
					float zero_rotation_respect_minimum = (nozzle == 1) ? SNIPER1_ZEROTHETA_ROT : SNIPER2_ZEROTHETA_ROT;

					//angolo tra la posizione di ombra minima + 90 gradi e lo zero theta dela macchina (per ora vale
					//zero perche' le define sono poste pari a +90, quando e se saranno sostituite da parametri configurabili
					//questo valore potra' assumere valori anche diversi da zero).
					float zero_rotation_90 = zero_rotation_respect_minimum - 90;

					//NOTA la punta adesso si trova all'angolo corrispondente al secondo risultato (-90 rispetto
					//all'angolo dove era stato trovato il minimo)

					if( left_valid[0] && left_valid[1] )
					{
						if( left_value[0] > left_value[1] )
						{
							//il primo risultato corrisponde allo zero theta: bisogna ruotare di +180
							PuntaRotDeg(+180 + zero_rotation_90, nozzle, BRUSH_REL);
						}
						else
						{
							//il secondo risultato corrisponde allo zero theta: basta ruotare dell'angolo residuo zero_rotation_90
							PuntaRotDeg(zero_rotation_90, nozzle, BRUSH_REL);
						}
					}
					else
					{
						if(!left_valid[0] & !left_valid[1])
						{
							ok = 0;
						}
						else
						{
							if(left_valid[0])
							{
								//il primo risultato corrisponde allo zero theta: bisogna ruotare di +180
								PuntaRotDeg(+180 + zero_rotation_90, nozzle, BRUSH_REL);
							}
							else
							{
								//il secondo risultato corrisponde allo zero theta: basta ruotare dell'angolo residuo zero_rotation_90
								PuntaRotDeg(zero_rotation_90, nozzle, BRUSH_REL);
							}
						}
					}
				}

				if( !ok )
				{
					char buf[80];
					snprintf( buf, sizeof(buf), MsgGetString(Msg_00720), nozzle );

					if( !W_Deci( 1, buf ) )
					{
						break;
					}
				}
			}
			else
			{
				// Ruoto la punta per andare in posizione di zero
				// (per ora sono define, andranno messe come par in config...)
				PuntaRotDeg( nozzle == 1 ? SNIPER1_ZEROTHETA_ROT : SNIPER2_ZEROTHETA_ROT, nozzle, BRUSH_REL );

				ok = 1;
			}

			if( ok )
			{
				// Attendo la fine del movimento
				Wait_EncStop( nozzle );

				if( !check )
				{
					// Set theta attuale come zero
					PuntaRotDeg ( 0, nozzle, BRUSH_RESET );
				}

				ret = 1;
				break;
			}
		}

		// Riporto punta in posizione iniziale
		PuntaRotStep ( -numSteps, nozzle, BRUSH_REL );
		// Attendo la fine del movimento
		Wait_EncStop ( nozzle );
	}

	// Porta punta a z-zero
	PuntaZPosStep( nozzle, 0 );
	PuntaZPosWait( nozzle );

	RestoreNozzleRotSpeed( nozzle );

	if( sniper->IsOnError() )
	{
		return 0;
	}

	return ret;
}



//---------------------------------------------------------------------------------
// Funzione di ricerca angolo iniziale zero theta
//   Ritorna 0 se e' avvenuto un errore, 1 altrimenti
//---------------------------------------------------------------------------------
int Sniper_FindPrerotZeroTheta( int nozzle )
{
	if( Get_OnFile() )
	{
		return 1;
	}

	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	if( sniper->IsOnError() )
	{
		return 0;
	}

	CPan pan( -1, 1, MsgGetString(Msg_00291) ); // Please wait ...

	// Porta punta a quota ricerca zero theta
	int stepToZeroTheta = ( nozzle == 1 ) ? QHeader.Step_Trasl1*QHeader.zth_level1 : QHeader.Step_Trasl2*QHeader.zth_level2;
	PuntaZPosStep( nozzle, stepToZeroTheta );
	PuntaZPosWait( nozzle );



	FoxHead->MotorDisable( (nozzle == 1) ? BRUSH1 : BRUSH2 ); //disabilita brushless
	if( !FoxPort->enabled )
	{
		return 0;
	}
	delay(100);
	FoxHead->MotorEnable( (nozzle == 1) ? BRUSH1 : BRUSH2 );
	if( !FoxPort->enabled )
	{
		return 0;
	}
	delay(1500);



	// Setta pos theta attuale = 0
	PuntaRotDeg( 0, nozzle, BRUSH_RESET );

	// Definisci costante passi encoder relativa alla punta
	short int k_encoder = ( nozzle == 1 ) ? QHeader.Enc_step1 : QHeader.Enc_step2;


	SniperModes::inst().useRecord( 0 );

	//==========================================================================
	//                   *** CARICAMENTO PARAMETRI SNIPER **

	//int numSteps = int ( SniperModes::inst().getRecord().search_angle * k_encoder / 360.0 );
	int numSteps = int ( 90 * k_encoder / 360.0 );

	// use all encoder steps and PULSES for FPGA
	if( !sniper->SetUseEveryFrames() || !sniper->SetPulsesForFPGA( numSteps ) )
	{
		return 0;
	}

	if( !sniper->SetUseModifiedAlgorithm() )
	{
		return 0;
	}

	//==========================================================================
	//                            *** CICLO SNIPER **

	// Salvo velocita' e accelerazione attuale
	SaveNozzleRotSpeed( nozzle );

	while( 1 )
	{
		//prerotazione per ricerca minimo theta
		//PuntaRotDeg( QHeader.zertheta_Prerot[nozzle-1], nozzle );
		PuntaRotDeg( -10, nozzle, BRUSH_REL );
		// Attende fine rotazione
		Wait_EncStop( nozzle );

		delay( 40 );

		// Azzera conteggio encoder sniper
		sniper->Zero_Cmd();

		// Setto velocita'/accelerazione di ricerca
		PuntaRotSpeed( nozzle, SniperModes::inst().getRecord().speed );
		PuntaRotAcc( nozzle, SniperModes::inst().getRecord().acceleration );

		// Inizio ciclo di ricerca ombra minima
		if( !sniper->StartFirst() )
		{
			break;
		}

		// Avvio movimento motore per effettuare la scansione del componente
		PuntaRotStep( numSteps, nozzle, BRUSH_REL );
		// Attendo la fine del movimento
		Wait_EncStop( nozzle );

		// Leggo i risultati del ciclo di ricerca
		int measure_status;
		int measure_angle;
		float measure_position;
		float measure_shadow;

		if( !sniper->GetFirst( measure_status, measure_angle, measure_position, measure_shadow ) )
		{
			break;
		}

		// Errore di lettura del minimo
		if( measure_status != STATUS_OK && measure_status != STATUS_L_BLK && measure_status != STATUS_R_BLK )
		{
			if( !W_Deci( 1, MsgGetString(Msg_00159) ) )
			{
				break;
			}
			else
			{
				continue;
			}
		}

		// Ruoto la punta in posizione di minimo
		int stepsDiff = sniper->GetEncoder() - measure_angle;
		PuntaRotStep( -stepsDiff, nozzle, BRUSH_REL );
		// Ruoto la punta per andare in posizione di zero
		PuntaRotDeg( nozzle == 1 ? SNIPER1_ZEROTHETA_ROT : SNIPER2_ZEROTHETA_ROT, nozzle, BRUSH_REL );
		Wait_EncStop( nozzle );


		// minimo trovato
		//--------------------------------------------------------------------------
		if( !W_Deci( 1, MsgGetString(Msg_00161) ) )
		{
			if( !W_Deci( 1, MsgGetString(Msg_00160) ) )
			{
				break;
			}
			else
			{
				// Riporto la punta in posizione di minimo
				PuntaRotDeg( nozzle == 1 ? -SNIPER1_ZEROTHETA_ROT : -SNIPER2_ZEROTHETA_ROT, nozzle, BRUSH_REL );
				// Riporto la punta in posizione di scansione
				PuntaRotStep( stepsDiff, nozzle, BRUSH_REL );
				// Attendo la fine del movimento
				Wait_EncStop( nozzle );

				continue;
			}
		}

		// Riporto la punta in posizione di minimo
		PuntaRotDeg( nozzle == 1 ? -SNIPER1_ZEROTHETA_ROT : -SNIPER2_ZEROTHETA_ROT, nozzle, BRUSH_REL );
		// Attendo la fine del movimento
		Wait_EncStop( nozzle );

		float min_pos = GetPuntaRotDeg( nozzle );

		// Ruoto la punta per andare in posizione di zero
		PuntaRotDeg( nozzle == 1 ? SNIPER1_ZEROTHETA_ROT : SNIPER2_ZEROTHETA_ROT, nozzle, BRUSH_REL );
		// Attendo la fine del movimento
		Wait_EncStop( nozzle );

		// Set theta attuale come zero
		PuntaRotDeg( 0, nozzle, BRUSH_RESET );

		// Save zero theta prerot angle
		min_pos = (min_pos-180) - 10;

		while( min_pos >= 180 )
			min_pos -= 360;
		while( min_pos < -180 )
			min_pos += 360;

		QHeader.zertheta_Prerot[nozzle-1] = min_pos;
		Mod_Cfg( QHeader );
		break;
	}

	// Porta punta a z-zero
	PuntaZPosStep( nozzle, 0 );
	PuntaZPosWait( nozzle );

	RestoreNozzleRotSpeed( nozzle );

	return sniper->IsOnError() ? 0 : 1;
}



//---------------------------------------------------------------------------------
// sottofunzione: Disegna intensita' linea sniper
//---------------------------------------------------------------------------------
void Sniper_ImageTestDraw( CWindow* window, SniperModule* sniper, const RectI& _panel )
{
	window->DrawPanel( _panel, GUI_color( GR_CYAN ), GUI_color( 0,0,0 ) );

	int plotX = window->GetX() + _panel.X * GUI_CharW() + 1;
	//int plotY = window->GetY() + (_panel.Y+_panel.H) * GUI_CharH() - 2;
	int plotY = window->GetY() + _panel.Y * GUI_CharH() + 300;

	// Legge immagine da Sniper (prima meta'...)
	int win_half = SNIPER_LEFT_USABLE + SNIPER_RIGHT_USABLE / 2;
	sniper->SetWindow( SNIPER_LEFT_USABLE, win_half + 1 );

	unsigned char scan_data[512];
	if( !sniper->ScanLine( scan_data ) )
	{
		sniper->SetWindow( SNIPER_LEFT_USABLE, SNIPER_RIGHT_USABLE );
		return;
	}

	float kx = float((_panel.W * GUI_CharW()) - 2) / ( SNIPER_RIGHT_USABLE - SNIPER_LEFT_USABLE + 1 );
	int min_val = 10000000;
	int max_val = -10000000;
	float average = 0.f;

	// Disegna i risultati
	for( int i = 0; i < win_half; i++ )
	{
		GUI_PutPixel( plotX+int( i*kx ), plotY-scan_data[i], GUI_color( GR_WHITE ) );

		if( scan_data[i] < min_val )
		{
			min_val = scan_data[i];
		}
		if( scan_data[i] > max_val )
		{
			max_val = scan_data[i];
		}

		average += scan_data[i];
	}

	// Legge immagine da Sniper (seconda meta'...)
	sniper->SetWindow( win_half + 1, SNIPER_RIGHT_USABLE );

	if( !sniper->ScanLine( scan_data ) )
	{
		sniper->SetWindow( SNIPER_LEFT_USABLE, SNIPER_RIGHT_USABLE );
		return;
	}

	// Disegna i risultati
	for( int i = 0; i < win_half; i++ )
	{
		GUI_PutPixel( plotX+int( (i+win_half)*kx ), plotY-scan_data[i], GUI_color( GR_WHITE ) );

		if( scan_data[i] < min_val )
		{
			min_val = scan_data[i];
		}
		if( scan_data[i] > max_val )
		{
			max_val = scan_data[i];
		}

		average += scan_data[i];
	}

	average = average / (SNIPER_RIGHT_USABLE-SNIPER_LEFT_USABLE+1);

	// Si esegue una misurazione che non fa mai male...
	int measure_status;
	int measure_angle;
	float measure_position;
	float measure_shadow;

	if( !sniper->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow ) )
	{
		return;
	}


	GUI_color color;
	bool failed = false;
	char buf[40];

	// min
	window->DrawText( 3, 23, MsgGetString(Msg_05067) );
	if( min_val >= SNIPER_IMG_QUALITY_MIN && max_val >= min_val )
	{
		color = GUI_color(GR_LIGHTGREEN);
	}
	else
	{
		color = GUI_color(GR_LIGHTRED);
		failed = true;
	}
	snprintf( buf, sizeof(buf), "%4d", min_val );
	window->DrawText( 19, 23, buf, GUI_SmallFont, color, GUI_color( 0,0,0 ) );

	// max
	window->DrawText( 3, 24, MsgGetString(Msg_05068) );
	if( max_val <= SNIPER_IMG_QUALITY_MAX && max_val >= min_val )
	{
		color = GUI_color(GR_LIGHTGREEN);
	}
	else
	{
		color = GUI_color(GR_LIGHTRED);
		failed = true;
	}
	snprintf( buf, sizeof(buf), "%4d", max_val );
	window->DrawText( 19, 24, buf, GUI_SmallFont, color, GUI_color( 0,0,0 ) );

	// average
	window->DrawText( 30, 24, MsgGetString(Msg_05069) );
	if( average >= SNIPER_IMG_QUALITY_MIN && average < SNIPER_IMG_QUALITY_MAX && max_val >= min_val )
	{
		color = GUI_color(GR_LIGHTGREEN);
	}
	else
	{
		color = GUI_color(GR_LIGHTRED);
		failed = true;
	}
	snprintf( buf, sizeof(buf), "%4d", int(average) );
	window->DrawText( 49, 24, buf, GUI_SmallFont, color, GUI_color( 0,0,0 ) );

	// result
	window->DrawText( 3, 1, MsgGetString(Msg_01333) );
	if( !failed )
	{
		window->DrawText( 26, 1, MsgGetString(Msg_01335), GUI_SmallFont, GUI_color(GR_LIGHTGREEN), GUI_color( 0,0,0 ) );
	}
	else
	{
		window->DrawText( 26, 1, MsgGetString(Msg_01334), GUI_SmallFont, GUI_color(GR_LIGHTRED), GUI_color( 0,0,0 ) );
	}

	// measures
	if( measure_status != STATUS_EMPTY )
	{
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01454), measure_position / 1000.f );
		window->DrawText( 3, 21, buf );

		snprintf( buf, sizeof(buf), MsgGetString(Msg_01455), measure_shadow / 1000.f );
		window->DrawText( 50, 21, buf );
	}
}

//---------------------------------------------------------------------------------
// Disegna intensita' linea sniper
//---------------------------------------------------------------------------------
void Sniper_ImageTest( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	if( sniper->IsOnError() )
		return;

	int cam = pauseLiveVideo();

	int line = 240;
	sniper->SetScalLine( line );

	float prev_pos = PuntaZPosMm( nozzle, 0, RET_POS );
	PuntaZPosMm( nozzle, QHeader.LaserOut );
	PuntaZPosWait( nozzle );

	GUI_Freeze();

	// crea finestra su cui mostra i risultati
	CWindow* window = new CWindow( 0 );
	window->SetStyle( WIN_STYLE_CENTERED );
	window->SetClientAreaSize( 80, 30 );

	char _title[80];
	snprintf( _title, sizeof(_title), "Sniper %d - Line Test", nozzle );
	window->SetTitle( _title );

	window->Show();


	RectI _panel( 2, 2, window->GetW()/GUI_CharW() - 4, window->GetH()/GUI_CharH() - 11 );

	// line
	char _line_buf[20];
	snprintf( _line_buf, sizeof(_line_buf), MsgGetString(Msg_00081), line );
	window->DrawText( _panel.X+_panel.W-strlen(_line_buf)-1, 1, _line_buf  );

	// info
	window->DrawPanel( RectI( 2, window->GetH()/GUI_CharH() - 4, window->GetW()/GUI_CharW() - 4, 3 ) );
	window->DrawTextCentered( window->GetH()/GUI_CharH() - 3, MsgGetString(Msg_05052), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );

	Sniper_ImageTestDraw( window, sniper, _panel );

	GUI_Thaw();

	// Attende la pressione del tasto ESC
	int c;
	do
	{
		int scan = 0;

		c = Handle();
		switch ( c )
		{
			case K_PAGEUP:
				if( line > 0 )
				{
					if( !sniper->SetScalLine( line-1 ) )
					{
						line--;
						scan = 1;
					}
					else
					{
						bipbip();
					}
				}
				else
				{
					bipbip();
				}
				break;

			case K_PAGEDOWN:
				if( line < 480-1 )
				{
					if( !sniper->SetScalLine( line+1 ) )
					{
						line++;
						scan = 1;
					}
					else
					{
						bipbip();
					}
				}
				else
				{
					bipbip();
				}
				break;

			case K_CTRL_PAGEUP:
				if( !sniper->SetScalLine( MAX( line-25, 0 ) ) )
				{
					line = MAX( line-25, 0 );
					scan = 1;
				}
				else
				{
					bipbip();
				}
				break;

			case K_CTRL_PAGEDOWN:
				if( !sniper->SetScalLine( MIN( line+25, 480-1 ) ) )
				{
					line = MIN( line+25, 480-1 );
					scan = 1;
				}
				else
				{
					bipbip();
				}
				break;

			case K_ESC:
				break;

			default:
				bipbip();
		}

		if( scan )
		{
			GUI_Freeze();

			// line
			char _line_buf[20];
			snprintf( _line_buf, sizeof(_line_buf), MsgGetString(Msg_00081), line );
			window->DrawText( _panel.X+_panel.W-strlen(_line_buf)-1, 1, _line_buf  );

			Sniper_ImageTestDraw( window, sniper, _panel );
			GUI_Thaw();
		}
	}
	while( c != K_ESC );

	delete window;

	sniper->SetScalLine( 240 );
	sniper->SetWindow( SNIPER_LEFT_USABLE, SNIPER_RIGHT_USABLE );

	PuntaZPosMm( nozzle, prev_pos );
	PuntaZPosWait( nozzle );

	playLiveVideo( cam );
}


//---------------------------------------------------------------------------------
// Mostra livelli luminosita rilevati dal sensore sniper.
//---------------------------------------------------------------------------------
void Sniper_ImageTestDetailed( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	if( sniper->IsOnError() )
		return;

	int cam = pauseLiveVideo();

	// crea finestra su cui mostra i risultati
	CWindow* window = new CWindow( 0 );

	window->SetStyle( WIN_STYLE_CENTERED );
	window->SetClientAreaSize( 80, 25 );
	window->SetTitle( "Sniper Image Test" );

	window->Show();

	RectI _panel( 2, 1, window->GetW()/GUI_CharW() - 4, window->GetH()/GUI_CharH() - 3 );

	window->DrawPanel( _panel );


	// Si esegue una misurazione
	int measure_status = STATUS_EMPTY;
	int measure_angle;
	float measure_position;
	float measure_shadow;

	sniper->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow );

	int text_y = window->GetH()/GUI_CharH() - 1;
	if( measure_status != STATUS_EMPTY )
	{
		char buf[80];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_01454), measure_position/1000.f );
		window->DrawText( 9, text_y, buf );

		//posizione rispetto allo zero "laser compatibile"
		float tmp = (( SNIPER_RIGHT_USABLE - SNIPER_LEFT_USABLE + 1 ) * QHeader.sniper_kpix_um[nozzle-1] / 1000.0) - measure_position/1000.f;
		int _len = strlen( buf );
		snprintf( buf, sizeof(buf), " (%5.3f) mm", tmp );
		window->DrawText( 9+_len, text_y, buf );

		snprintf( buf, sizeof(buf), MsgGetString(Msg_01455), measure_shadow/1000.f );
		window->DrawText( 54, text_y, buf );
	}
	else
	{
		window->DrawText( 9, text_y, "Empty !" );
	}


	int xmin = _panel.X * GUI_CharW() + 1;
	int xmax = (_panel.X + _panel.W) * GUI_CharW() - 2;
	int ymax = 445;

	int dx = (( xmax - xmin ) - ( SNIPER_RIGHT_USABLE - SNIPER_LEFT_USABLE + 1 )) / 2;

	int win_half = SNIPER_LEFT_USABLE + SNIPER_RIGHT_USABLE / 2;

	int nrows = 0;
	unsigned char scan_data[1024];

	flushKeyboardBuffer();

	for( int row = 0 ; row < 480 ; row += SNIPER_IMAGETEST_MUL )
	{
		if( keyRead() )
		{
			break;
		}

		if( !sniper->SetScalLine( row ) )
		{
			break;
		}

		// Legge immagine da Sniper (prima meta'...)
		if( !sniper->SetWindow( SNIPER_LEFT_USABLE, win_half + 1 ) )
		{
			break;
		}

		if( !sniper->ScanLine( scan_data ) )
		{
			break;
		}

		// Legge immagine da Sniper (seconda meta'...)
		if( !sniper->SetWindow( win_half + 1, SNIPER_RIGHT_USABLE ) )
		{
			break;
		}

		if( !sniper->ScanLine( &scan_data[win_half] ) )
		{
			break;
		}

		// Disegna i risultati
		GUI_Freeze();
		int _x = xmin + window->GetX() + 1 + dx;
		for( int i = SNIPER_LEFT_USABLE-1; i < SNIPER_RIGHT_USABLE; i++ )
		{
			GUI_PutPixel( _x + i, ymax - row/SNIPER_IMAGETEST_MUL, GUI_color( scan_data[i],scan_data[i],scan_data[i] ) );
		}
		GUI_Thaw();

		nrows++;
	}

	int cursor_row = nrows / 2;
	int cursorX = window->GetX();

	void* vidbuf = GUI_SaveScreen( RectI( xmin+cursorX, ymax-cursor_row, xmax-xmin+1, 1 ) );
	GUI_HLine( xmin+cursorX, xmax+cursorX, ymax-cursor_row, GUI_color( GR_LIGHTRED ) );


	int c = 0;
	while( c != K_ESC )
	{
		c = Handle();
		switch(c)
		{
			case K_DOWN:
				if( cursor_row != 0 )
				{
					GUI_DrawSurface( PointI( xmin+cursorX, ymax-cursor_row ), vidbuf );
					GUI_FreeSurface( &vidbuf );
					cursor_row--;
					vidbuf = GUI_SaveScreen( RectI( xmin+cursorX, ymax-cursor_row, xmax-xmin+1, 1 ) );
					GUI_HLine( xmin+cursorX, xmax+cursorX, ymax-cursor_row, GUI_color( GR_LIGHTRED ) );
				}
				break;
			case K_UP:
				if( cursor_row != (nrows-1) )
				{
					GUI_DrawSurface( PointI( xmin+cursorX, ymax-cursor_row ), vidbuf );
					GUI_FreeSurface( &vidbuf );
					cursor_row++;
					vidbuf = GUI_SaveScreen( RectI( xmin+cursorX, ymax-cursor_row, xmax-xmin+1, 1 ) );
					GUI_HLine( xmin+cursorX, xmax+cursorX, ymax-cursor_row, GUI_color( GR_LIGHTRED ) );
				}
				break;
		}
	}

	free( vidbuf );
	delete window;

	sniper->SetScalLine( 240 );
	sniper->SetWindow( SNIPER_LEFT_USABLE, SNIPER_RIGHT_USABLE );

	playLiveVideo( cam );
}


//---------------------------------------------------------------------------------
// Mostra i dati dell'ultimo centraggio
//---------------------------------------------------------------------------------
void Sniper_PlotFrames( int nozzle )
{
	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;

	int min_w, max_w;
	int tot_frame;
	int buf_w[512];
	int buf_a[512];

	if( !sniper->GetFrames( tot_frame, buf_w ) )
	{
		W_Mess( "ERROR: Sniper GetFrames !" ); //TODO: messaggio
		return;
	}

	int cam = pauseLiveVideo();

	C_Graph* graph = new C_Graph( 5,3,76,22, "", GRAPH_NUMTYPEY_INT | GRAPH_NUMTYPEX_INT | GRAPH_AXISTYPE_XY | GRAPH_DRAWTYPE_LINE | GRAPH_SHOWPOSTXT, 1 );

	min_w = 2510;
	max_w = 0;
	for( int i = 0; i < tot_frame-1; i++ )
	{
		buf_a[i] = i+1;

		if( buf_w[i] > max_w )
			max_w = buf_w[i];
		if( buf_w[i] < min_w )
			min_w = buf_w[i];
	}

	graph->SetVMinY( min_w );
	graph->SetVMaxY( max_w );
	graph->SetVMinX( buf_a[0] );
	graph->SetVMaxX( buf_a[tot_frame-2] );
	graph->SetNData( tot_frame-1, 0 );
	graph->SetDataY( buf_w, 0 );
	graph->SetDataX( buf_a, 0 );

	graph->Show();

	CenteringResultData centeringResult;
	GetCenteringResult( nozzle, centeringResult );
	char buf[80];
	float w1 = centeringResult.Shadow1 / 1000.f;
	float w2 = centeringResult.Shadow2 / 1000.f;
	snprintf( buf, sizeof(buf),"w1 = %.2f   w2 = %.2f   (Result = %d)", w1, w2, centeringResult.Result );
	graph->PrintMsgLow( 2, buf );

	while( 1 )
	{
		int c = Handle();
		if( c == K_ESC )
		{
			break;
		}
		graph->GestKey( c );
	}

	delete graph;

	playLiveVideo( cam );
}


//---------------------------------------------------------------------------------
// Aggiorna firmware modulo Sniper
// Parametri di ingresso:
//    filename: nome del file contenente il firmware
// Ritorna 0 se errore, 1 altrimenti
//---------------------------------------------------------------------------------
#define RETRYTIMES              3
#define MAX_FAKE_CHARS          5
#define TX_OK                   0x11
#define ERROR_Crc               0x22
#define ERROR_Line              0x33
#define ERROR_Signature         0x44
#define ERROR_OverFlow          0x55
#define ERROR_TopModule         0x66

int Sniper_BootLoader( int nozzle, const char* filename )
{
	unsigned short retries = 0;
	unsigned int response;
	unsigned char inByte;

	int i, txSize, bufferSize = 0;
	unsigned char buffer[0x20000];

	SniperModule* sniper = (nozzle == 1) ? Sniper1 : Sniper2;
	CommClass* com = (CommClass*)sniper->GetComPort();

	// ignore any received bytes (flush input buffer)
	com->flush();

	// open selected file
	FILE *firmware = fopen ( filename , "rb" );

	if( firmware == NULL )
		return -1;

	// parse .enc file
	while( !feof ( firmware ) )
	{
		fread( &inByte, sizeof ( inByte ), 1, firmware );
		buffer[bufferSize] = Hex2Dec( inByte ) * 16;

		fread( &inByte, sizeof ( inByte ), 1, firmware );
		buffer[bufferSize] += Hex2Dec( inByte );

		bufferSize++;
	}
	fclose( firmware );
	bufferSize--;

	// reset Sniper
	sniper->HardReset();

	// set 10secs timeout
	com->settimeout ( 10000000 );

	// looking for 'a' character
	for( i = 0; i < MAX_FAKE_CHARS; i++ )
	{
		response = com->getbyte();

		// check for timeout
		if( response == SERIAL_ERROR_TIMEOUT )
			return -2;

		inByte = response;

		if( inByte == 'a' )
		{
			inByte = 'b';
			com->putbyte ( inByte );
			break;
		}
	}

	if( i == MAX_FAKE_CHARS )
		return -3;

	// set 4secs timeout
	com->settimeout( 4000 );

	// looking for 'b' character
	response = com->getbyte();

	// check for timeout
	if( response == SERIAL_ERROR_TIMEOUT )
		return -4;

	inByte = response;

	if( inByte != 'b' )
		return -5;

	// data transfer
	for( i = 0; i < bufferSize; i += txSize + 2 )
	{
		txSize = buffer[i] << 8 | buffer[i + 1];

		// ignore any received bytes (flush input buffer)
		com->flush();

		delay ( 10 );

		// send data
		for ( int j = 0; j < txSize + 2; j++ )
		{
			com->putbyte ( buffer[i + j] );
		}
		delay ( 90 );

		// looking for target response
		response = com->getbyte();

		// check for timeout
		if( response == SERIAL_ERROR_TIMEOUT )
			return -4;

		inByte = response;

		switch ( inByte )
		{
			case TX_OK:
				retries = 0;
				break;

			case ERROR_Crc:
				retries++;
				if( retries >= RETRYTIMES )
					return -6;

				i -= ( txSize + 2 );
				break;

			case ERROR_Line:
				retries++;
				if( retries >= RETRYTIMES )
					return -7;

				i -= ( txSize + 2 );
				break;

			case ERROR_Signature:
				return -8;

			case ERROR_TopModule:
				return -9;
		}

		// update progress
		printf( "." );
		fflush( stdout );
	}

	return 1;
}


//---------------------------------------------------------------------------------
// Interfaccia testuale aggiornamento firmware Sniper
//---------------------------------------------------------------------------------
int UpdateSniper()
{
	printf("\n\tVersion %s\n",SOFT_VER);
	printf("\tBuild date %s\n\n",__DATE__);

	// Apertura porta seriale per comunicazione con moduli Sniper
	ComPortSniper = new CommClass( SNIPER_COM_PORT, SNIPER_BAUD );

	Sniper1 = new SniperModule( ComPortSniper, SNIPER1_ADDR );
	if(!Sniper1->IsOnError())
	{
		if(Sniper1->IsEnabled())
		{
			printf( MsgGetString(Msg_00122), Sniper1->GetVersion() );
			printf("\n");
		}
	}

	Sniper2 = new SniperModule( ComPortSniper, SNIPER2_ADDR );
	if(!Sniper2->IsOnError())
	{
		if(Sniper2->IsEnabled())
		{
			printf( MsgGetString(Msg_00123), Sniper2->GetVersion() );
			printf("\n");
		}
	}

	int exit_flag = 0;
	while( !exit_flag )
	{
		delay(1000);
		int ret = 0;

		printf("\n");
		printf(" 1 - Set address - Sniper nozzle 1\n");
		printf(" 2 - Set address - Sniper nozzle 2\n");
		printf(" 3 - Update firmware - Sniper nozzle 1\n");
		printf(" 4 - Update firmware - Sniper nozzle 2\n");
		printf(" q (Q) - Quit\n");
		printf("-> ");

		int c = getchar();

		// flush
		int c_dummy = c;
		while( c_dummy != 10 )
			c_dummy = getchar();

		if( c == 'q' || c == 'Q' )
			exit_flag = 1;
		else if( c >= '1' && c <= '4' )
		{
			// confirm
			printf("\n");
			printf("\n You choose to ");
			if( c == '1' )
				printf("set address - Sniper nozzle 1\n");
			else if( c == '2' )
				printf("set address - Sniper nozzle 2\n");
			else if( c == '3' )
				printf("update firmware - Sniper nozzle 1\n");
			else if( c == '4' )
				printf("update firmware - Sniper nozzle 2\n");
			printf(" c (C) - Continue\n");
			printf(" other key - Abort\n");
			printf("-> ");

			int cc = getchar();

			// flush
			int c_dummy = cc;
			while( c_dummy != 10 )
				c_dummy = getchar();

			if( cc == 'c' || cc == 'C' )
			{
				if( c == '1' )
				{
					printf("\n\t ATTENTION! Be sure to unplug the Sniper nozzle 2 device.\n");
					printf("\n\t Press ENTER when ready.\n");

					// flush
					int c_dummy = getchar();
					while( c_dummy != 10 )
						c_dummy = getchar();

					if( !Sniper1->ChangeAddress( 1 ) )
						printf("Address correctly set.\n");
					else
						printf("ERROR! Unable to set address.\n");
				}
				else if( c == '2' )
				{
					printf("\n\t ATTENTION! Be sure to unplug the Sniper nozzle 1 device.");
					printf("\n\t Press ENTER when ready.\n");

					// flush
					int c_dummy = getchar();
					while( c_dummy != 10 )
						c_dummy = getchar();

					if( !Sniper2->ChangeAddress( 2 ) )
						printf("Address correctly set.\n");
					else
						printf("ERROR! Unable to set address.\n");
				}
				else if( c == '3' )
				{
					printf("\n\n\t SNIPER - BOOTLOADER\n\n");
					printf("Wait...\n" );

					Sniper2->Suspend();
					ret = Sniper_BootLoader( 1, "sniper.enc" );
				}
				else if( c == '4' )
				{
					printf("\n\n\t SNIPER - BOOTLOADER\n\n");
					printf("Wait...\n" );

					Sniper1->Suspend();
					ret = Sniper_BootLoader( 2, "sniper.enc" );
				}

				if( c == '3' || c == '4' )
				{
					switch( ret )
					{
					case -1:
						printf("\nERROR - Firmware file (sniper.enc) doesn't exist !!!\n" );
						break;
					case -2:
						printf("\nERROR - Target device not found !!!\n");
						break;
					case -3:
						printf("\nERROR - MAX_FAKE_CHARS reached !!!\n");
						break;
					case -4:
						printf("\nERROR - Timeout occurred !!!\n");
						break;
					case -5:
						printf("\nERROR - Unexpected character !!!\n");
						break;
					case -6:
						printf("\nERROR - File damaged (Bad CRC) !!!\n");
						break;
					case -7:
						printf("\nERROR - Check connection !!!\n");
						break;
					case -8:
						printf("\nERROR - Invalid file !!!\n");
						break;
					case -9:
						printf("\nERROR - Communication out of sync !!!\n");
						break;
					case 1:
						printf("\nTransfering successfully completed !\n");
						break;
					default:
						break;
					}
				}
			}
		}
	}

	delete ComPortSniper;
	delete Sniper1;
	delete Sniper2;

	return 1;
}

int ActivateSniper()
{
	printf("\n\tVersion %s\n",SOFT_VER);
	printf("\tBuild date %s\n\n",__DATE__);

	// Apertura porta seriale per comunicazione con moduli Sniper
	ComPortSniper = new CommClass( SNIPER_COM_PORT, SNIPER_BAUD );

	Sniper1 = new SniperModule( ComPortSniper, SNIPER1_ADDR );
	Sniper2 = new SniperModule( ComPortSniper, SNIPER2_ADDR );

	int exit_flag = 0;
	while( !exit_flag )
	{
		delay(1000);

		printf("\n");
		printf(" 1 - Check activation status\n");
		printf(" 2 - Get activation request code\n");
		printf(" 3 - Insert activation code\n");
		printf(" q (Q) - Quit\n");
		printf("-> ");

		int c = getchar();

		// flush
		int c_dummy = c;
		while( c_dummy != 10 )
			c_dummy = getchar();

		if( c == 'q' || c == 'Q' )
			exit_flag = 1;
		if( c == '1' )
		{
			int sniper1_expired = -1;
			int sniper1_daysleft = 0;
			int sniper2_expired = -1;
			int sniper2_daysleft = 0;

			if( Sniper1->IsEnabled() )
			{
				Sniper1->CheckActivation();
				if( !Sniper1->IsOnError() )
				{
					sniper1_expired = Sniper1->IsExpired();
					sniper1_daysleft = Sniper1->GetDaysLeft();
				}
				else
				{
					printf("\n\nError !\n\n");
					exit_flag = 1;
				}
			}

			if( Sniper2->IsEnabled() )
			{
				Sniper2->CheckActivation();
				if( !Sniper2->IsOnError() )
				{
					sniper2_expired = Sniper2->IsExpired();
					sniper2_daysleft = Sniper2->GetDaysLeft();
				}

				else
				{
					printf("\n\nError !\n\n");
					exit_flag = 1;
				}
			}

			if( sniper1_expired == -1 && sniper2_expired == -1 )
				continue;
			else if( sniper1_expired > 0 || sniper2_expired > 0 )
				printf("\n\n\t Activation expired !\n\n  Please call your customer to proceed to a new activation.\n\n");
			else
			{
				printf("\n\n\t Activation OK !!!\n\n  Days left: ");

				if( sniper1_expired == -1 )
				{
					if( sniper2_daysleft == -1 )
						printf("unlimited");
					else
						printf("%d", sniper2_daysleft );
				}
				else if( sniper2_expired == -1 )
				{
					if( sniper1_daysleft == -1 )
						printf("unlimited");
					else
						printf("%d", sniper1_daysleft );
				}
				else
				{
					if( sniper1_daysleft == -1 && sniper2_daysleft == -1 )
						printf("unlimited");
					else if( sniper1_daysleft == -1 )
						printf("%d", sniper2_daysleft );
					else if( sniper2_daysleft == -1 )
						printf("%d", sniper1_daysleft );
					else
						printf("%d", MIN( sniper1_daysleft, sniper2_daysleft ) );
				}

				printf("\n\n");
			}
		}
		else if( c == '2' )
		{
			unsigned char code[26];

			if( Sniper1->IsEnabled() )
			{
				Sniper1->GetActivationCode( code );
				if( !Sniper1->IsOnError() )
				{
					printf("\n\n\t Activation request code: ");

					for( int i = 0; i < 26; i++ )
					{
						if( i > 0 && i % 5 == 0 )
							printf("-");

						printf("%c", code[i]);
					}

					printf("\n\n");
					continue;
				}
				else
				{
					printf("\n\nError !\n\n");
					exit_flag = 1;
				}
			}

			if( Sniper2->IsEnabled() )
			{
				Sniper2->GetActivationCode( code );
				if( !Sniper2->IsOnError() )
				{
					printf("\n\n\t Activation request code: ");

					for( int i = 0; i < 26; i++ )
					{
						if( i > 0 && i % 5 == 0 )
							printf("-");

						printf("%c", code[i]);
					}

					printf("\n\n");
					continue;
				}
				else
				{
					printf("\n\nError !\n\n");
					exit_flag = 1;
				}
			}
		}
		else if( c == '3' )
		{
			unsigned char BASE32_TABLE_ENC[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz234567=";
			unsigned char code[128];
			unsigned char code1_fake[32];
			unsigned char code2_fake[32];
			unsigned char code_char[] = "a";
			unsigned char code_valid = 1;
			int counter_chars;
			int counter_code;
			int sniper1_activation_err = 0;
			int sniper2_activation_err = 0;

			printf("\n\n\t Insert activation code (press ENTER when done):\n\n-> ");

			counter_chars = 0;
			code[counter_chars] = getchar();
			while( code[counter_chars] != 10 )
			{
				counter_chars++;
				if( counter_chars < 128 )
				{
					code[counter_chars] = getchar();
				}
				else
				{
					// flush
					int c_dummy = 0;
					while( c_dummy != 10 )
						c_dummy = getchar();

					break;
				}
			}

			// check
			counter_code = 0;
			for( int i = 0; i < counter_chars; i++ )
			{
				if( code[i] != '-' )
				{
					// is a valid char
					code_char[0] = code[i];
					if( strstr( (const char *)BASE32_TABLE_ENC, (const char *)code_char ) == NULL )
						code_valid = 0;

					code[counter_code] = code[i];
					counter_code++;
				}
				else
				{
					// is '-' in right position
					if( i != 5 && i != 11 && i != 17 && i != 23 && i != 29 )
						code_valid = 0;
				}
			}
			// chars number
			if( counter_code != 26 )
				code_valid = 0;

			if( code_valid )
			{
				if( Sniper1->IsEnabled() && Sniper2->IsEnabled() )
				{
					Sniper1->GetActivationCode( code1_fake ); // serve per il comando successivo
					if( Sniper1->IsOnError() )
					{
						sniper1_activation_err = 1;
					}

					if( !sniper1_activation_err )
					{
						sniper1_activation_err = Sniper1->Activate( code );
						if( sniper1_activation_err )
						{
							sniper1_activation_err = 1;
						}
					}

					if( !sniper1_activation_err )
					{
						Sniper2->GetActivationCode( code2_fake ); // serve per il comando successivo
						if( Sniper2->IsOnError() )
						{
							sniper2_activation_err = 1;
						}

						if( !sniper2_activation_err )
						{
							sniper2_activation_err = Sniper2->ActivateAlign( code1_fake ); // allinea il numero di attivazioni
							if( Sniper2->IsOnError() )
							{
								sniper2_activation_err = 1;
							}
						}

						if( !sniper2_activation_err )
						{
							sniper2_activation_err = Sniper2->Activate( code );
							if( Sniper2->IsOnError() )
							{
								sniper2_activation_err = 1;
							}
						}
					}

					if( sniper1_activation_err || sniper2_activation_err )
					{
						printf("\n\n\t ERROR ! Unable to activate the machine.\n\n  Please call your customer to solve the problem.\n\n");

						if( sniper1_activation_err == -1 || sniper2_activation_err == -1 )
							exit_flag = 1;
					}
					else
						printf("\n\n\t CONGRATULATION ! Machine successfully activated.\n\n");
				}
				else
				{
					printf("\n\n\t ERROR ! Unable to activate the machine.\n\n  Please call your customer to solve the problem.\n\n");
				}
			}
			else
				printf("\n\n\t ERROR ! Code not valid.\n\n  code must be: XXXXX-XXXXX-XXXXX-XXXXX-XXXXX-X\n\n");
		}
	}
	printf("\n\n");

	delete ComPortSniper;
	delete Sniper1;
	delete Sniper2;

	return 1;
}

#endif //__SNIPER
