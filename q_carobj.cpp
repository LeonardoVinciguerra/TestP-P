//---------------------------------------------------------------------------
// Name:        q_carobj.cpp
// Author:      
// Created:     
// Description: Gestione caricatori.
//---------------------------------------------------------------------------
#include "q_carobj.h"

#include <math.h>

#include "q_tabe.h"
#include "msglist.h"
#include "q_help.h"
#include "q_vision.h"
#include "q_oper.h"
#include "q_assem.h"
#include "q_prog.h"
#include "q_packages.h"

#include <mss.h>


extern char auto_text1[50];
extern char auto_text2[50];
extern char auto_text3[50];

extern struct CfgHeader QHeader;
extern struct CfgParam  QParam;


//--------------------------------------------------------------------------
// Feeder Class
// Classe di gestione caricatore

/*---------------------------------------------------------------------------------
Costruttore classe caricatore.
Parametri di ingresso:
   _file   : puntatore alla classe di gestione file caricatore da utilizzare
   _code   : codice del caricatore
----------------------------------------------------------------------------------*/
FeederClass::FeederClass(FeederFile *_file,int _code)
{
	file=_file;
	code=_code;
	file->Read(code,car); //legge struttura dati caricatore da file
	Set_StartNComp();     //setta il numero di componenti disponibili dopo un avanzamento
}

/*---------------------------------------------------------------------------------
Costruttore classe caricatore.
Non inizializza il codice caricatore: non e' possibile eseguire operazioni sul
caricatore, prima di aver settato il codice.
Parametri di ingresso:
   _file   : puntatore alla classe di gestione file caricatore da utilizzare
----------------------------------------------------------------------------------*/
FeederClass::FeederClass(FeederFile *_file)
{
	file=_file;
	code=-1;
}

FeederClass::FeederClass(const CarDat& rec_data)
{
	car = rec_data;
	file = NULL;
	code = car.C_codice;
}

/*---------------------------------------------------------------------------------
Ritorna il numero di componenti per singolo avanzamento
a valore di default.
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   numero di componenti per singolo avanzamento
----------------------------------------------------------------------------------*/
int FeederClass::Get_StartNComp(void)
{
	return StartNComp;
}


/*---------------------------------------------------------------------------------
Resetta il numero di componenti per singolo avanzamento
a valore di default.
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   nessuno.
----------------------------------------------------------------------------------*/
void FeederClass::Set_StartNComp(void)
{
	switch(car.C_att)
	{
		case CARAVA_NORMAL:
		case CARAVA_MEDIUM:
		case CARAVA_LONG:
		case CARAVA_NORMAL1:
		case CARAVA_MEDIUM1:
		case CARAVA_LONG1:
			StartNComp=1;
			break;
		case CARAVA_DOUBLE:
		case CARAVA_DOUBLE1:
			StartNComp=2;
			break;
		default:
			StartNComp=1;
			break;
	}
}

//---------------------------------------------------------------------------------
// Resetta il codice caricatore
//---------------------------------------------------------------------------------
int FeederClass::GetCode(void)
{
	return code;
}

/*---------------------------------------------------------------------------------
Legge da file i dati del caricatore con codice specificato.
Parametri di ingresso:
   _code: codice del caricatore.
Valori di ritorno:
   nessuno.
----------------------------------------------------------------------------------*/
void FeederClass::SetCode(int _code)
{
	code = _code;
	file->Read( code, car );
	Set_StartNComp();  //setta il numero di componenti disponibili dopo un avanzamento
	
	#ifdef __DOME_FEEDER
	if(car.C_tipo == CARTYPE_DOME)
	{
		car.C_att = CARAVA_DOMES;
	}
	#endif
}

/*---------------------------------------------------------------------------------
Legge da file i dati del caricatore con record specificato.
Parametri di ingresso:
   _rec: record file caricatori da cui leggere
Valori di ritorno:
   nessuno.
----------------------------------------------------------------------------------*/
void FeederClass::SetRec(int _rec)
{
	file->ReadRec(_rec,car);
	code=car.C_codice;
	Set_StartNComp();  //setta il numero di componenti disponibili dopo un avanzamento
	
	#ifdef __DOME_FEEDER
	if(car.C_tipo == CARTYPE_DOME)
	{
		car.C_att = CARAVA_DOMES;
	}
	#endif
}

void FeederClass::ReloadData(void)
{
	file->Read(car.C_codice,car);
	Set_StartNComp();
	
	#ifdef __DOME_FEEDER
	if(car.C_tipo == CARTYPE_DOME)
	{
		car.C_att = CARAVA_DOMES;
	}
	#endif
}

/*---------------------------------------------------------------------------------
Struttura dati associata alla classe
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   Ritorna la struttura dati associata al caricatore.
----------------------------------------------------------------------------------*/
CarDat FeederClass::GetData(void)
{
	return car;
}

CarDat& FeederClass::GetDataRef(void)
{
	return car;
}

const CarDat& FeederClass::GetDataConstRef(void)
{
	return car;
}

/*---------------------------------------------------------------------------------
Se la classe e' stata associata ad un caricatore, ritorna il package ad
esso associato (se presente)
----------------------------------------------------------------------------------*/
int FeederClass::Get_AssociatedPack( SPackageData& pack )
{
	if( code == -1 )
		return 0;

	return file->Get_AssociatedPack( code, pack );
}


/*---------------------------------------------------------------------------------
Setta dall'esterno la struttura dati del caricatore
Parametri di ingresso:
   data: struttura dati caricatore da settare.
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void FeederClass::SetData(struct CarDat data)
{
	memcpy(&car,&data,sizeof(struct CarDat));
	code=car.C_codice;
	Set_StartNComp();  //setta il numero di componenti disponibili dopo un avanzamento

	#ifdef __DOME_FEEDER
	if(car.C_tipo == CARTYPE_DOME)
	{
		car.C_att = CARAVA_DOMES;
	}
	#endif
}

/*---------------------------------------------------------------------------------
Ritorna quantita' di componenti
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   quantita' componenti
----------------------------------------------------------------------------------*/
int FeederClass::GetQuantity(void)
{
	return(car.C_quant);
}

/*---------------------------------------------------------------------------------
Decrementa numero di componenti che possono essere prelevati senza avanzare
caricatore.
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void FeederClass::DecNComp(void)
{
	if(car.C_quant!=0)     //decrementa contatore comp. totali su caricatore
	{
		car.C_quant--;
	}
	
	if(car.C_Ncomp!=0)     //se ci sono ancora componenti da prlevare
	{
		car.C_Ncomp--;     //decrementa contatore componenti
	}

	UpdateStatus();        //aggiorna struttura dati su file
}

/*---------------------------------------------------------------------------------
Valido solo per i vassoi: incrementa quanita' componenti
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   nessuno
Nota: Usato solo per far scartare un componente sulla posizione vassoio
da cui e' stato prelevato. A questa funzione deve corrispondere una chiamata
a DecNComp dopo l'esecuzione dello scarto.
----------------------------------------------------------------------------------*/
void FeederClass::IncNComp(void)
{
	if(code>=FIRSTTRAY)
	{
		//incrementa contatore comp. totali su caricatore
		car.C_quant++;
	}
	
	UpdateStatus();          //aggiorna struttura dati su file
}

/*---------------------------------------------------------------------------------
Setta numero di componenti che possono essere prelevati senza avanzare
caricatore.
Parametri di ingresso:
   val: valore da settare
Valori di ritorno:
   nessuno
----------------------------------------------------------------------------------*/
void FeederClass::SetNComp(int val)
{
	car.C_Ncomp=val;
	
	UpdateStatus();          //aggiorna struttura dati su file
}

/*---------------------------------------------------------------------------------
Imposta la quantita di componenti presenti sul caricatore
Parametri di ingresso:
  quant: nuova quantita componenti
----------------------------------------------------------------------------------*/
void FeederClass::SetQuantity(unsigned int quant)
{
	car.C_quant = quant;
	UpdateStatus();
}



/*---------------------------------------------------------------------------------
Avanza caricatore, resettando a default il numero di componenti.
L'avanzamento non e' possibile se il numero di componenti e' divero da 0
(ci sono ancora componenti prelevabili sul caricatore)
Parametri di ingresso:
   C_att: se passato, forza il tipo di avanzamento
          se non indicato, utilizza il tipo di avanzamento del caricatore.
   test : 1 se modalita test (trascura assenza componenti)
          0 altrimenti
Valori di ritorno:
   1 se avanzamento eseguito
   0 avanzamento non necessario
----------------------------------------------------------------------------------*/
int FeederClass::Avanza(int C_att,int test)
{
	if(!test)
	{
		if(car.C_Ncomp!=0) //avanza solo se non ci sono componenti sul caricatore
		{
			return(0);
		}
	}
	
	if(!QParam.DemoMode)
	{
		PressStatus(1); //check pressione aria
		
		int tipoav;
		
		#ifdef __DOME_FEEDER
		if(car.C_tipo == CARTYPE_DOME)
		{
			if(C_att == -99)
			{
				car.C_att = CARAVA_DOMES;
				tipoav = car.C_att;
			}
			else
			{
				tipoav = C_att;
			}
		}
		else
		#endif
		{
			if(C_att == -99)
			{
				tipoav = car.C_att;
			}
			else
			{
				tipoav = C_att;
			}
		}
		
		CaricMov( code, tipoav, car.C_tipo, car.C_thFeederAdd, car.C_att ); //THFEEDER
	}
	
	car.C_Ncomp = StartNComp; //reset numero componenti a numero start componenti dopo avanzamento
	UpdateStatus();         //aggiorna struttura dati su file
}


/*---------------------------------------------------------------------------------
Attende che il caricatore sia pronto per un prossimo avanzamento
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   0 se controllo fallito, 1 altrimenti
----------------------------------------------------------------------------------*/
//THFEEDER
int FeederClass::WaitReady(void)
{
	return CaricWait( car.C_tipo, car.C_thFeederAdd );
}

/*---------------------------------------------------------------------------------
Controlla se il caricatore e' pronto per un prossimo avanzamento
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   1 se pronto, 0 altrimenti
----------------------------------------------------------------------------------*/
//THFEEDER
int FeederClass::CheckReady(void)
{
	return CaricCheck( car.C_tipo, car.C_thFeederAdd );
}

/*---------------------------------------------------------------------------------
Aggiorna su file la struttura dati del caricatore
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   0 se controllo fallito, 1 altrimenti
---------------------------------------------------------------------------------*/
void FeederClass::UpdateStatus(void)
{
	file->SaveX(code,car);
	file->SaveFile();
}

#ifdef __DOME_FEEDER
int FeederClass::DomesForcedUp(void)
{
	if(car.C_tipo != CARTYPE_DOME)
	{
		bipbip();
		return(0);
	}
	
	if(!QParam.DemoMode)
	{
		//gli ultimi due parametri non sono usati (servono solo per i caricatori led th)
		CaricMov( code, CARAVA_DOMES_FORCED_UP, CARTYPE_DOME, 0,0);
	}
	
	return(1);
}

int FeederClass::DomesForcedDown(void)
{
	if(car.C_tipo != CARTYPE_DOME)
	{
		bipbip();
		return(0);
	}
	
	if(!QParam.DemoMode)
	{
		//gli ultimi due parametri non sono usati (servono solo per i caricatori led th)
		CaricMov( code, CARAVA_DOMES_FORCED_DOWN, CARTYPE_DOME, 0,0);
	}
	
	return(1);
}
#endif

/*---------------------------------------------------------------------------------
Porta punta su posizione caricatore
Parametri di ingresso:
   punta: numero della punta da portare sul caricatore
          1:     punta 1
          2:     punta 2
          0:     porta la telecamera sul caricatore
---------------------------------------------------------------------------------*/
#define CCCP_MAX_POSITION_ERROR_MM         1.f

bool FeederClass::GoPos( int nozzle )
{
	float xpos, ypos;

	if( car.C_codice >= 160 )
	{
		// posiziona la punta sul vassoio
		if( !GetTrayPosition( xpos, ypos ) )
		{
			return false;
		}

		if( nozzle != 0 )
		{
			return NozzleXYMove_N( xpos, ypos, nozzle ) ? true : false;
		}

		return NozzleXYMove( xpos, ypos ) ? true : false;
	}

	// serve per i caricatori in cui si presentano piu' componenti per volta
	int n_inc = StartNComp - car.C_Ncomp;

	if( n_inc < 0 )
	{
		n_inc = 0;
	}
	xpos = car.C_xcar+(car.C_incx*n_inc);
	ypos = car.C_ycar+(car.C_incy*n_inc);

	//CCCP
	if( n_inc == 0 && car.C_checkCount > 0 )
	{
		car.C_checkCount--;
	}

	if( nozzle != 0 )
	{
		if( car.C_checkPos && car.C_checkCount == 0 )
		{
			car.C_checkCount = car.C_checkNum;

			// save current speed
			SaveNozzleXYSpeed();

			// ricerca componente
			if( !Image_match( &xpos, &ypos, CARCOMP_IMG, MATCH_CORRELATION, car.C_codice ) )
			{
				//notifica errore
				W_Mess( MsgGetString(Msg_06016) );

				if( !ManualTeaching( &xpos, &ypos, MsgGetString(Msg_06017) ) )
				{
					return false;
				}
			}

			// update car pos
			float newPosX = xpos - (car.C_incx*n_inc);
			float newPosY = ypos - (car.C_incy*n_inc);

			if( fabs(newPosX-car.C_xcar) >= CCCP_MAX_POSITION_ERROR_MM || fabs(newPosY-car.C_ycar) >= CCCP_MAX_POSITION_ERROR_MM )
			{
				//notifica errore
				W_Mess( MsgGetString(Msg_06016) );

				if( !ManualTeaching( &xpos, &ypos, MsgGetString(Msg_06017) ) )
				{
					return false;
				}
			}
			car.C_xcar = newPosX;
			car.C_ycar = newPosY;

			// restore saved speed
			RestoreNozzleXYSpeed();
		}

		//posiziona punta sul primo componente disponibile su caricatore
		if( !NozzleXYMove_N( xpos, ypos, nozzle ) )
		{
			return false;
		}
	}
	else
	{
		//porta la telecamera sul primo componente disponibile sul caricatore
		if( !NozzleXYMove( xpos, ypos  ) )
		{
			return false;
		}
	}

	return true;
}

int FeederClass::GetTrayPosition(float& x,float &y)
{
	float n_xpos, n_ypos;
	float p_xpos, p_ypos;             // **W0204
	unsigned int n_comp;
	unsigned int nc_y;

	if(car.C_nx == 0) return(0);       // 0 comp. su asse x

	n_comp=car.C_quant;

	if(n_comp>0)                               // vassoio non vuoto
	{
		n_comp-=1;
		nc_y=int(ceil(n_comp/car.C_nx));  //L2403   // valore assoluto y

		// coords del centroid ultimo componente autoappreso *W0204
		if(car.C_ny>1)
		{
			p_ypos=car.C_ycar+(car.C_incy*(car.C_ny-1));
		}
		else
		{
			p_ypos=car.C_ycar;        // solo un compon. in Y
		}

		if(car.C_nx>1)
		{
			p_xpos=car.C_xcar+(car.C_incx*(car.C_nx-1));
		}
		else
		{
			p_xpos=car.C_xcar;         // solo un compon. in X
		}

		n_ypos=p_ypos-(nc_y*car.C_incy);  // y posiz. componente
		n_xpos=p_xpos-(n_comp-(car.C_nx*(nc_y)))*car.C_incx;
	}
	else
	{
		// vassoio vuoto
		n_ypos=car.C_ycar;         // riprova su pos. autoapp.
		n_xpos=car.C_xcar;         // genera errore mancata presa
	}

	x = n_xpos;
	y = n_ypos;

	return 1;
}

//---------------------------------------------------------------------------------
// Apprendimento posizione caricatore. Se confermato salva su file le posizioni.
// Ritorna: 0 se autoapprendimento annullato, 1 altrimenti.
//---------------------------------------------------------------------------------
int FeederClass::TeachPosition()
{
	float p_x1, p_y1, p_x2, p_y2;
	int ritorno;

	snprintf( auto_text1, sizeof(auto_text1), "   %s %d", MsgGetString(Msg_00311), car.C_codice );
	*auto_text2 = 0;
	snprintf( auto_text3, sizeof(auto_text3), "   %s %s", MsgGetString(Msg_00310), car.C_comp );


	float c_ax = car.C_xcar;
	float c_ay = car.C_ycar;

	if( car.C_codice < 160 )
	{
		// setta i parametri della telecamera
		if( car.C_checkPos )
		{
			char NameFile[MAXNPATH];
			SetImageName( NameFile, CARCOMP_IMG, DATA, car.C_codice );
			if( access(NameFile,0) == 0 )
			{
				struct img_data imgData;
				ImgDataLoad( NameFile, &imgData );

				// set brightess and contrast
				SetImgBrightCont( imgData.bright*655, imgData.contrast*655 );
			}
		}

		Set_Tv(2); // predispone per non richiudere immagine su video

		if( ManualTeaching( &c_ax, &c_ay, MsgGetString(Msg_00026), AUTOAPP_COMP ) )    // autoappr. posizione caricatore
		{
			ritorno=1;
			car.C_xcar=c_ax;
			car.C_ycar=c_ay;

			//CCCP
			if( car.C_checkPos )
			{
				int cam = pauseLiveVideo();
				ImageCaptureSave( CARCOMP_IMG, YESCHOOSE, cam, car.C_codice );
				playLiveVideo( cam );
				
				//CCCP mettere da qualche parte
				car.C_checkCount = car.C_checkNum;
			}

			if((car.C_att==CARAVA_DOUBLE) || (car.C_att==CARAVA_DOUBLE1))   // autoappr. posiz. 2' comp 0402
			{
				c_ax=car.C_xcar+car.C_incx;
				c_ay=car.C_ycar+car.C_incy;
				
				if( ManualTeaching(&c_ax,&c_ay,MsgGetString(Msg_00664), AUTOAPP_COMP) )
				{
					car.C_incx=c_ax-car.C_xcar;
					car.C_incy=c_ay-car.C_ycar;
				}
				else
				{
					ritorno=0;
				}
			}

			file->SaveX(code,car);
			file->SaveFile();
		}
		else
			ritorno=0;
		
		Set_Tv(3); // chiude immagine su video
	}
	else
	{
		// autoapp. punto basso sx, integr. basso sx
		if( ManualTeaching(&c_ax,&c_ay,MsgGetString(Msg_00032), AUTOAPP_COMP) )
		{
			ritorno=1;
			p_x1=c_ax;
			p_y1=c_ay;
		}
		else
			ritorno=0;

		// autoapp. punto alto dx, integr. basso sx
		if(ritorno)
		{
			if( ManualTeaching(&c_ax,&c_ay,MsgGetString(Msg_00033), AUTOAPP_COMP) )
			{
				ritorno=1;
				p_x2=c_ax;
				p_y2=c_ay;
			}
			else
				ritorno=0;
		}

		// calcolo centro integr. basso sx, posizion. su integr. alto dx
		if(ritorno)
		{
			c_ax=(p_x2-p_x1)/2+p_x1;
			c_ay=(p_y2-p_y1)/2+p_y1;
		}

		car.C_xcar=c_ax;
		car.C_ycar=c_ay;

		file->SaveX(code,car);
		file->SaveFile();
	}

	return ritorno;
}


//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
// Legge i parametri dei caricatori da disco e inizializza la CPU
// ( Lanciata solo all'avviamento del programma. )
//---------------------------------------------------------------------------------
#include "q_ser.h"

#ifndef __DOME_FEEDER
#define  MAX_NCAR_TYPE  8
#else
#define  MAX_NCAR_TYPE  11
#endif

struct CarTimeData CarTiming[MAX_NCAR_TYPE];

void make_car(void)
{
	char buf[45];
	char n_buf[6];
	
	CarTime_Open();
	
	for( int i = 0; i < CarTime_NRec(); i++ )
	{
		if(i>=MAX_NCAR_TYPE)
		{
			break;
		}
	
		CarTime_Read(CarTiming[i],i);
	
		buf[0]='S';               // comando
		buf[1]='C';
		buf[2]=0;
	
		cnvex(i,1,n_buf);    // dati
		strcat(buf,n_buf);
	
		cnvex(CarTiming[i].start_ele,4,n_buf);
		strcat(buf,n_buf);
	
		cnvex(CarTiming[i].start_motor,4,n_buf);
		strcat(buf,n_buf);
	
		cnvex(CarTiming[i].start_inv,4,n_buf);
		strcat(buf,n_buf);
	
		cnvex(CarTiming[i].end_ele,4,n_buf);
		strcat(buf,n_buf);
	
		cnvex(CarTiming[i].end_motor,4,n_buf);
		strcat(buf,n_buf);
	
		cnvex(CarTiming[i].end_inv,4,n_buf);
		strcat(buf,n_buf);
	
		cnvex(CarTiming[i].wait,4,n_buf);
		strcat(buf,n_buf);
	
		cpu_cmd(2,buf);     // go
	}

	CarTime_Close();
}

FeederClass *Caricatore;
