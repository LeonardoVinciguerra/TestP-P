//---------------------------------------------------------------------------
// Name:        q_feederfile.cpp
// Author:      
// Created:     
// Description: Gestione file configurazione caricatori.
//---------------------------------------------------------------------------
#include "q_feederfile.h"

#include "q_packages.h"
#include "q_help.h"
#include "msglist.h"
#include "strutils.h"
#include "q_feeders.h"
#include "q_debug.h"
#include "q_gener.h"
#include "c_waitbox.h"

#include <mss.h>


extern struct CfgHeader QHeader;
extern SPackageData currentLibPackages[MAXPACK];
extern CarDat currentFeedersConfig[MAXCAR];


//--------------------------------------------------------------------------
// Feeder File
// Classe di gestione file caricatori
//--------------------------------------------------------------------------


//---------------------------------------------------------------------------------
// Costruttore classe di gestione file caricatori
// Parametri di ingresso:
//    name: nome file configurazione caricatori
//    flag: 0 aggiunge automaticamente il path al nome file specificato (default)
//          1 name e' gia' completo di pathname
//----------------------------------------------------------------------------------
FeederFile::FeederFile( char* name, bool check )
{
	lastfind = -1;
	lastfind_pkg = -1;
	lastfind_okpkg = -1;

	filename = name;

	opened = 0;

	//apre file
	if( !FeedersConfig_Load( name ) )
	{
		return;
	}
	if( check )
	{
		if( !CheckFeederFile() )
		{
			return;
		}
	}

	opened = 1;
}

/*---------------------------------------------------------------------------------
Distruttore classe di gestione file caricatori.
Parametri di ingresso:
   nessuno.
----------------------------------------------------------------------------------*/
FeederFile::~FeederFile(void)
{

}

//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
bool FeederFile::SaveFile()
{
	return FeedersConfig_Save( filename );
}


//---------------------------------------------------------------------------------
// Controlla correttezza del file configurazione caricatori.
// Ritorna   0 se il controllo fallisce
//           1 altrimenti
//---------------------------------------------------------------------------------
int FeederFile::CheckFeederFile()
{
	CWaitBox waitBox( 0, 15, MsgGetString(Msg_01895), MAXCAR );
	waitBox.Show();

	int ask = 1;
	for( int i = 0; i < MAXCAR; i++ )
	{
		struct CarDat rec = currentFeedersConfig[i];

		if( rec.C_codice != GetCarCode(i) )
		{
			if( ask )
			{
				ask = 0;

				char buf_err[160];
				snprintf( buf_err, sizeof(buf_err), MsgGetString(Msg_01894), filename.c_str() );

				if(!W_Deci(1,buf_err))
				{
					waitBox.Hide();
					return 0;
				}
			}

			rec.C_codice = GetCarCode(i);

			currentFeedersConfig[i] = rec;
		}

		waitBox.Increment();
	}

	if( ask == 0 )
	{
		SaveFile();
	}

	waitBox.Hide();

	return 1;
}

/*---------------------------------------------------------------------------------
Dati codice e posizione del Caricatore, setta i valori di default e salva
Parametri di ingresso:
   code: codice del caricatore
   posx, posy: posizione del caricatore
----------------------------------------------------------------------------------*/
void FeederFile::SetDefValues( int code, float posx, float posy )
{
	CarDat car;

	car.C_codice      = code;
	car.C_comp[0]     = 0;
	car.C_tipo        = 0;
	car.C_thFeederAdd = 0;
	car.C_att         = 0;
	car.C_xcar        = posx;
	car.C_ycar        = posy;
	car.C_nx          = 1;
	car.C_ny          = 1;
	car.C_incx        = 0;
	car.C_incy        = 0;
	car.C_offprel     = code < FIRSTTRAY ? 0 : 10; //mm
	car.C_quant       = 0;
	car.C_avan        = 0;
	car.C_Ncomp       = 1;
	car.C_sermag      = 0;
	car.C_PackIndex   = 0;
	car.C_Package[0]  = 0;
	car.C_note[0]     = 0;
	car.C_checkPos    = 0;
	car.C_checkNum    = 0;

	SaveX( code , car );
}

/*---------------------------------------------------------------------------------
Dato un codice caricatore, legge i dati di quel caricatore nella struttura dati
specificata.
Parametri di ingresso:
   code: codice del caricatore da leggere
   car : struttuta dati in cui inserire i dati letti
Valori di ritorno:
   Ritorna 1 operazione di lettura terminata correttamente, 0 se fallita.
----------------------------------------------------------------------------------*/
int FeederFile::Read( int code, CarDat& car )
{
	int nrec = GetCarRec( code );
	if( nrec == -1 )
		return 0;

	car = currentFeedersConfig[nrec];

	if( car.C_Ncomp > 2 || car.C_Ncomp < 0 )
	{
		SetFeederNCompDef( car );
	}
	return 1;
}

/*---------------------------------------------------------------------------------
Data una struttura dati caricatore la salva su file nella posizione
specificata da code
Parametri di ingresso:
   code: codice del caricatore su cui scrivere
   car : struttuta dati da scrivere
Valori di ritorno:
   Ritorna 0 se operazione fallita, 1 altrimenti.
----------------------------------------------------------------------------------*/
int FeederFile::SaveX( int code, CarDat car )
{
	int nrec = GetCarRec( code );
	if( nrec == -1 )
		return 0;

	return SaveRecX( nrec, car );
}

/*---------------------------------------------------------------------------------
Legge un record del file caricarore nella struttura dati indicata.
Parametri di ingresso:
   nrec: record da leggere
   car : struttuta dati in cui inserire i dati letti
Valori di ritorno:
   Ritorna 1 operazione di lettura terminata correttamente, 0 se fallita.
----------------------------------------------------------------------------------*/
int FeederFile::ReadRec( int nrec, CarDat& car )
{
	if( !opened || nrec < 0 || nrec >= MAXCAR )
		return 0;

	car = currentFeedersConfig[nrec];
	return 1;
}

/*---------------------------------------------------------------------------------
Data una struttura dati caricatore la salva su file nella posizione
specificata da numero record
Parametri di ingresso:
   nrec: record del file su cui scrivere
   car : struttuta dati da scrivere
Valori di ritorno:
   Ritorna 0 se operazione fallita, 1 altrimenti.
----------------------------------------------------------------------------------*/
int FeederFile::SaveRecX( int nrec, CarDat car )
{
	if( !opened || nrec < 0 || nrec >= MAXCAR )
		return 0;

	currentFeedersConfig[nrec] = car;
	return 1;
}


/*---------------------------------------------------------------------------------
Ricerca un tipo componente.
Parametri di ingresso:
   txt    : tipo componente da cercare
   car_ret: struttura dati in cui inserire l'eventuale caricatore trovato
   start  : record da cui partire nella ricerca.
   dir    : CAR_SEARCHDIR_UP ricerca verso l'inizio del file
            CAR_SEARCHDIR_DW ricerca verso la fine del file

Valori di ritorno:
   Ritorna numero record trovato. CAR_SEARCHFAIL se ricerca fallita.
----------------------------------------------------------------------------------*/
int FeederFile::Search(char *txt,struct CarDat &car_ret,int start,int dir)
{
	lastfind = CAR_SEARCHFAIL;

	if( !opened || start < 0 || start >= MAXCAR )
	{
		return lastfind;
	}

	unsigned int i;
	struct CarDat tmpCaric;

	char *tmp=new char[strlen(txt)+1];
	strcpy(tmp,txt);
	DelSpcR(tmp);

	strupr(tmp); //SMOD201103

	strcpy(lastsearch,tmp);

	if(dir==CAR_SEARCHDIR_DW)
	{
		for(i=start;i<MAXCAR;i++)
		{
			tmpCaric = currentFeedersConfig[i];

			DelSpcR( tmpCaric.C_comp );
			strupr( tmpCaric.C_comp );

			if(!(strcmp(tmp,tmpCaric.C_comp))) //se tipo comp. trovato
			{
				lastfind=i; //setta lasfind=record letto
				break;
			}
		}
	}
	else
	{
		for(i=start;i>=0;i--)
		{
			tmpCaric = currentFeedersConfig[i];

			DelSpcR(tmpCaric.C_comp);
			strupr(tmpCaric.C_comp);

			if(!(strcmp(tmp,tmpCaric.C_comp)))
			{
				lastfind=i;
				break;
			}
		}
	}

	if(lastfind_pkg!=-1)
	memcpy(&car_ret,&tmpCaric,sizeof(struct CarDat));

	delete[] tmp;

	return(lastfind);
}

/*---------------------------------------------------------------------------------
Continua ricerca per tipo componente proseguendo verso la fine del file.
Parametri di ingresso:
   car_ret: struttura dati in cui inserire l'eventuale caricatore trovato
Valori di ritorno:
   Ritorna numero record trovato. CAR_SEARCHFAIL se ricerca fallita.
----------------------------------------------------------------------------------*/
int FeederFile::FindNext(struct CarDat &car_ret)
{
	if(lastfind!=CAR_SEARCHFAIL)
		lastfind=Search(lastsearch,car_ret,lastfind+1,CAR_SEARCHDIR_DW);
	return lastfind;
}

/*---------------------------------------------------------------------------------
Continua ricerca per tipo componente proseguendo verso l'inizio del file
Parametri di ingresso:
   car_ret: struttura dati in cui inserire l'eventuale caricatore trovato
Valori di ritorno:
   Ritorna numero record trovato. CAR_SEARCHFAIL se ricerca fallita.
----------------------------------------------------------------------------------*/
int FeederFile::FindPrev(struct CarDat &car_ret)
{ if(lastfind!=CAR_SEARCHFAIL)
    lastfind=Search(lastsearch,car_ret,lastfind-1,CAR_SEARCHDIR_UP);
  return lastfind;
}

/*---------------------------------------------------------------------------------
Ritorna se presente il package associato al caricatore specificato.
----------------------------------------------------------------------------------*/
int FeederFile::Get_AssociatedPack( int car, SPackageData& pack )
{
	struct CarDat tmpCar;
	Read( car, tmpCar );

	if( (tmpCar.C_PackIndex > 0) && (tmpCar.C_PackIndex <= MAXPACK) )
	{
		SPackageData tmpPack = currentLibPackages[tmpCar.C_PackIndex-1];

		DelSpcR( tmpCar.C_Package );
		DelSpcR( tmpPack.name );

		strupr( tmpCar.C_Package );
		strupr( tmpPack.name );

		if( !strcmp( tmpCar.C_Package, tmpPack.name ) )
		{
			//associazione package: ok
			pack = currentLibPackages[tmpCar.C_PackIndex-1];
			return 1;
		}

		for( int i = 0; i < MAXPACK; i++ )
		{
			if( !strcmp( tmpCar.C_Package, currentLibPackages[i].name ) )
			{
				tmpCar.C_PackIndex = i+1;
				SaveX( car, tmpCar );
				SaveFile();

				pack = currentLibPackages[i];
				return 1;
			}
		}

		//il package in tabella caricatori non esiste nella libreria package
		char buf[256];
		sprintf( buf, PACKNOTFOUND1, tmpCar.C_Package, tmpCar.C_codice );
		W_Mess( buf );
	}

	return 0;
}

/*---------------------------------------------------------------------------------
Ritorna se presente il package associato al caricatore record nrec
Se l'associazione e' non valida, aggiorna tutta la configurazione.
----------------------------------------------------------------------------------*/
int FeederFile::GetRec_AssociatedPack( int nrec, SPackageData& pack )
{
	return Get_AssociatedPack( GetCarCode(nrec), pack );
}


/*---------------------------------------------------------------------------------
Ricerca un tipo componente e package
Parametri di ingresso:
   comp   : tipo componente da cercare
   pack   : package da cercare
   car_ret: struttura dati in cui inserire l'eventuale caricatore trovato
   start  : record da cui partire nella ricerca.
   dir    : CAR_SEARCHDIR_UP ricerca verso l'inizio del file
            CAR_SEARCHDIR_DW ricerca verso la fine del file
            altro:come CAR_SEARCHDIR_DW

Valori di ritorno:
   Ritorna numero record trovato. CAR_SEARCHFAIL se ricerca fallita.
----------------------------------------------------------------------------------*/
int FeederFile::SearchPack(char *comp,char *pack,struct CarDat &car_ret,int start,int dir)
{
	lastfind_pkg = CAR_SEARCHFAIL;

	if( !opened || start < 0 || start >= MAXCAR )
	{
		return lastfind_pkg;
	}

	int i;

	struct CarDat tmpCaric;
	
	char *tmp_comp=new char[strlen(comp)+1];
	char *tmp_pack=new char[strlen(pack)+1];
	
	strcpy(tmp_comp,comp);
	strcpy(tmp_pack,pack);
	
	DelSpcR(tmp_comp);
	DelSpcR(tmp_pack);
	
	strupr(tmp_comp); //SMOD040903
	strupr(tmp_pack); //SMOD040903
	
	strcpy(lastsearch_comp,tmp_comp);
	strcpy(lastsearch_pack,tmp_pack);
	
	if(dir==CAR_SEARCHDIR_DW)
	{
		for(i=start;i<MAXCAR;i++)
		{
			tmpCaric = currentFeedersConfig[i];

			DelSpcR(tmpCaric.C_comp);
			DelSpcR(tmpCaric.C_Package);
		
			strupr(tmpCaric.C_comp);
			strupr(tmpCaric.C_Package);
		
			if(!(strcmp(tmp_comp,tmpCaric.C_comp)))
			{
				if(!strcmp(tmp_pack,tmpCaric.C_Package))
				{
					lastfind_pkg=i;      //componente e package trovati
					lastfind_okpkg=i;
					break;
				}
			}
		}
	}
	else
	{
		for(i=start;i>=0;i--)
		{
			tmpCaric = currentFeedersConfig[i];

			DelSpcR(car_ret.C_comp);
			DelSpcR(car_ret.C_Package);
		
			strupr(tmpCaric.C_comp);      //SMOD040903
			strupr(tmpCaric.C_Package);   //SMOD040903
		
			if(!(strcmp(tmp_comp,tmpCaric.C_comp)))
			{
				if(!strcmp(tmp_pack,tmpCaric.C_Package))
				{
					lastfind_pkg=i;
					lastfind_okpkg=i;        //SMOD201103
					break;
				}
			}
		}
	}
	
	delete[] tmp_comp;
	delete[] tmp_pack;

	if(lastfind_pkg!=-1)
	{
		memcpy(&car_ret,&tmpCaric,sizeof(struct CarDat));
	}

  	return(lastfind_pkg);
}

/*---------------------------------------------------------------------------------
Continua ricerca per tipo componente e/o package proseguendo verso la fine del
file.
Parametri di ingresso:
   car_ret: struttura dati in cui inserire l'eventuale caricatore trovato
Valori di ritorno:
   Ritorna numero record trovato. CAR_SEARCHFAIL se ricerca fallita.
----------------------------------------------------------------------------------*/
int FeederFile::FindNextPack(struct CarDat &car_ret)
{
	int ret=CAR_SEARCHFAIL;
	
	if(lastfind_okpkg!=CAR_SEARCHFAIL)
	{
		//se l'ultima ricerca per package, non ricercava anche il tipo componente
		if( *lastsearch_comp == -1 )
		{
			//continua ricerca per solo package
			ret=SearchPack(lastsearch_pack,car_ret,lastfind_okpkg+1,CAR_SEARCHDIR_DW);
		}
		else
		{
			//continua ricerca per package e tipo comp.
			ret=SearchPack(lastsearch_comp,lastsearch_pack,car_ret,lastfind_okpkg+1,CAR_SEARCHDIR_DW);
		}
	}
	
	return ret;
}

/*---------------------------------------------------------------------------------
Continua ricerca per tipo componente e/o package proseguendo verso l'inizio del
file
Parametri di ingresso:
   car_ret: struttura dati in cui inserire l'eventuale caricatore trovato
Valori di ritorno:
   Ritorna numero record trovato. CAR_SEARCHFAIL se ricerca fallita.
----------------------------------------------------------------------------------*/
int FeederFile::FindPrevPack(struct CarDat &car_ret)
{
	int ret=CAR_SEARCHFAIL;
	
	if(lastfind_okpkg!=CAR_SEARCHFAIL)
	{
		//se l'ultima ricerca per package, non ricercava anche il tipo componente
		if( *lastsearch_comp == -1 )
		{
			//continua ricerca per solo package
			ret=SearchPack(lastsearch_pack,car_ret,lastfind_okpkg-1,CAR_SEARCHDIR_UP);
		}
		else
		{
			//continua ricerca per package e tipo comp.
			ret=SearchPack(lastsearch_comp,lastsearch_pack,car_ret,lastfind_okpkg-1,CAR_SEARCHDIR_UP);
		}
	}
	
	return ret;
}

/*---------------------------------------------------------------------------------
Ricerca per solo package.
Parametri di ingresso:
   pack   : package da cercare
   car_ret: struttura dati in cui inserire l'eventuale caricatore trovato
   start  : record da cui partire nella ricerca.
   dir    : CAR_SEARCHDIR_UP ricerca verso l'inizio del file
            CAR_SEARCHDIR_DW ricerca verso la fine del file
            altro:come CAR_SEARCHDIR_DW

Valori di ritorno:
   Ritorna numero record trovato. CAR_SEARCHFAIL se ricerca fallita.
----------------------------------------------------------------------------------*/
int FeederFile::SearchPack(char *pack,struct CarDat &car_ret,int start,int dir)
{
	lastfind_pkg = CAR_SEARCHFAIL;

	if( !opened || start < 0 || start >= MAXCAR )
	{
		return lastfind_pkg;
	}

	int i;
	struct CarDat tmpCaric;
	
	*lastsearch_comp=-1;         //disattiva ricerca per tipo componente
	
	char *tmp_pack=new char[strlen(pack)+1];
	
	strcpy(tmp_pack,pack);
	
	DelSpcR(tmp_pack);
	
	strupr(tmp_pack); //SMOD040903
	
	strcpy(lastsearch_pack,tmp_pack);

	if(dir==CAR_SEARCHDIR_DW)
	{
		for(i=start;i<MAXCAR;i++)
		{
			tmpCaric = currentFeedersConfig[i];

			DelSpcR(tmpCaric.C_Package);
			strupr(tmpCaric.C_Package);
		
			if(!strcmp(tmp_pack,tmpCaric.C_Package))
			{
				lastfind_pkg=i; //package trovato: ritorna record
				lastfind_okpkg=i;
				break;
			}
		}
	}
	else
	{
		for(i=start;i>=0;i--)
		{
			tmpCaric = currentFeedersConfig[i];

			DelSpcR(tmpCaric.C_Package);
			strupr(tmpCaric.C_Package);
		
			if(!strcmp(tmp_pack,tmpCaric.C_Package))
			{
				lastfind_pkg=i;
				lastfind_okpkg=i;
				break;
			}
		}
	}

	delete[] tmp_pack;
	
	if(lastfind_pkg!=-1)
	{
		memcpy(&car_ret,&tmpCaric,sizeof(struct CarDat));
	}
	
	return(lastfind_pkg);
}

/*---------------------------------------------------------------------------------
Data una struttura dati Caricatore, resetta il numero di componenti per singolo avanzamento
a valore di default.
Parametri di ingresso:
   car : struttura dati caricatore
Valori di ritorno:
   ritorna in car la struttura dati modificata.
----------------------------------------------------------------------------------*/
void SetFeederNCompDef( CarDat& car )
{
	switch( car.C_att )
	{
		case CARAVA_NORMAL:
		case CARAVA_MEDIUM:
		case CARAVA_LONG:
		case CARAVA_NORMAL1:
		case CARAVA_MEDIUM1:
		case CARAVA_LONG1:
			car.C_Ncomp = 1;
			break;
		case CARAVA_DOUBLE:
		case CARAVA_DOUBLE1:
			car.C_Ncomp = 2;
			break;
		default:
			car.C_Ncomp = 1;
			break;
	}
}


FeederFile* CarFile;
