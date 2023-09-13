/*
>>>> Q_ZERI.CPP

Gestione della tabella degli zeri scheda.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1995    ++++

++++  	Modificato da TWS Simone 06.08.96
++++     Mod. W042000
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#include "msglist.h"
#include "q_cost.h"
#include "q_gener.h"
#include "q_wind.h"
#include "q_tabe.h"
#include "q_help.h"
#include "q_files.h"
#include "q_oper.h"
#include "q_prog.h"
#include "q_progt.h"
#include "q_zerit.h"
#include "q_zeri.h"
#include "q_assem.h"
#include "q_vision.h"
#include "tv.h"
#include <unistd.h>

#include "q_conf.h"
#include "q_init.h"
#include "keyutils.h"
#include "lnxdefs.h"
#include "strutils.h"

#include "gui_desktop.h"
#include "gui_defs.h"
#include "c_win_par.h"
#include "c_pan.h"

#include <mss.h>


extern GUI_DeskTop* guiDeskTop;

bool panelCompositionRefresh = false;

// Le strutture di dati per I/O su file sono definite in q_tabe.h.

struct Zeri KK_Zeri[MAXRZER+1];  // struct dati rifer. scheda
extern struct CfgHeader QHeader;      // struct memo parametri vari.
extern struct CfgParam  QParam;
//Integr. Loris
extern struct CfgHeader QHeader;
//Integr. Loris
extern struct vis_data Vision;

extern TPrgFile *TPrg;
extern TPrgFile *TPrgNormal;
extern TPrgFile *TPrgExp;

ZerFile *FZeri=NULL;

int zerCurRow;
int zerCurRecord;
int zMod;

struct dta_data* Dta_Val;

// Legge i records dal file zeri e carica l'array di structs.
void Z_read_recs(int roffset)
{
	register int floop = 0;   // counter - record on file
	while( floop <= MAXRZER-1 )
	{
		if(!FZeri->Read(KK_Zeri[floop],roffset+floop)) break; // if eof()
		floop++;
	}
}


// Scrolla i valori dell'array di structs di una riga in Direction.
// 0 up / 1 down
void Z_dat_scroll(int Direction)
{
    int Indent = 0;
    if(Direction) Indent = 1;
#ifdef __GNUC__
    memmove(&KK_Zeri[Indent],&KK_Zeri[!Indent],sizeof(KK_Zeri)-sizeof(KK_Zeri[0]));
#else
    movmem(&KK_Zeri[!Indent],&KK_Zeri[Indent],sizeof(KK_Zeri)-sizeof(KK_Zeri[0]));
#endif
} // Z_dat_scroll

// Inserimento di una nuova scheda in tabella
int New_scheda(void)
{
	// n. recs in tab. zeri
	int ult_scheda = FZeri->GetNRecs();

	char s_scheda[25];
	snprintf( s_scheda, 25, "%d", ult_scheda );

	int r = Z_scheda( ult_scheda, ZER_APP_ZERO );

	Zeri tmp;
	FZeri->Read(tmp,ult_scheda);
	tmp.Z_note[0]=0;
	tmp.Z_ass=1;
	FZeri->Write( tmp, ult_scheda );

	return r;
}

// Ridefinisce la posizione degli zeri scheda
int Z_redef(float &deltax,float &deltay)
{
	int numerorecs;
	int is_open=0,retval=1,i;
	struct Zeri zerdat;
	float oldx,oldy;

	if(FZeri==NULL)
	{
		FZeri=new ZerFile(QHeader.Prg_Default);
		if(!FZeri->Open())
		{
			bipbip();
			W_Mess(NOZSCHFILE);
			delete FZeri;
			FZeri=NULL;
			return 0;
		}
		is_open=1;
	}

	numerorecs=FZeri->GetNRecs();                   // n. recs in tab. zeri

	if(numerorecs>1)                          // recs presenti tab. zeri
	{
		FZeri->Read(zerdat,1);
		oldx=zerdat.Z_xzero;
		oldy=zerdat.Z_yzero;
		if( ManualTeaching(&(zerdat.Z_xzero),&(zerdat.Z_yzero), MsgGetString(Msg_00745)) )
		{
			deltax=zerdat.Z_xzero-oldx;
			deltay=zerdat.Z_yzero-oldy;
	
			for(i=1;i<numerorecs;i++)
			{
				FZeri->Read(zerdat,i);
				zerdat.Z_xzero+=deltax;
				zerdat.Z_yzero+=deltay;
				zerdat.Z_xrif+=deltax;
				zerdat.Z_yrif+=deltay;
				FZeri->Write( zerdat, i );
			}
		}
		else
		{
			retval=0;
		}
	}

	if(is_open)
	{
		delete FZeri;
		FZeri=NULL;
	}

	return retval;
}

// Autoapprend. zero scheda/ punto di riferim.
// n_scheda: num record della scheda
// mode=0 manuale
// mode=1 automatico
int Z_scheda(int n_scheda,int z_type,int mode,int tv_on)
{
	float zx_oo, zy_oo;
	float rx_oo, ry_oo;
	float zxm_oo, zym_oo;
	float rxm_oo, rym_oo;
	float deltax, deltay;
	float arctan;
	int numerorecs;
	int nz_scheda=n_scheda;
	int yes_zero=1;
	int yes_rife=1;
	int is_open=0;
	int zero_near=0;
	int matchMode=0;
	char filename[MAXNPATH+1];
	img_data imgData;
	int prevBright = 0;
	int prevContrast = 0;

	dta_data dtaval;
	Read_Dta( &dtaval );

	if(FZeri==NULL)
	{
		FZeri=new ZerFile(QHeader.Prg_Default,ZER_ADDPATH,true);
		if(!FZeri->Open())
		{
			bipbip();
			W_Mess(NOZSCHFILE);
			delete FZeri;
			FZeri=NULL;
			return 0;
		}
		is_open=1;
	}

	numerorecs=FZeri->GetNRecs();					// n. recs in tab. zeri

	if(numerorecs>=1)								// se almeno MASTER presente
	{
		if(n_scheda < numerorecs)					// se scheda selez. presente
		{
			FZeri->Read(KK_Zeri[0],nz_scheda);		// read record
		}
		else										// altrimenti
		{
			FZeri->Read(KK_Zeri[0],nz_scheda-1);	// read record precedente
		}
		
		FZeri->Read(KK_Zeri[1],0);					// read master

		zxm_oo=KK_Zeri[1].Z_xzero;
		zym_oo=KK_Zeri[1].Z_yzero;
		rxm_oo=KK_Zeri[1].Z_xrif;
		rym_oo=KK_Zeri[1].Z_yrif;
		
		zx_oo=KK_Zeri[0].Z_xzero;
		zy_oo=KK_Zeri[0].Z_yzero;
		rx_oo=KK_Zeri[0].Z_xrif;
		ry_oo=KK_Zeri[0].Z_yrif;
	}
	else
	{
		//MASTER non definito nessuna scheda definita
		zxm_oo=0;
		zym_oo=0;
		rxm_oo=0;
		rym_oo=0;
		zx_oo=0;
		zy_oo=0;
		rx_oo=0;
		ry_oo=0;

		//reset di master e scheda
		KK_Zeri[0].Z_xzero=0;
		KK_Zeri[0].Z_yzero=0;
		KK_Zeri[0].Z_xrif=0;
		KK_Zeri[0].Z_yrif=0;

		KK_Zeri[1].Z_xzero=0;
		KK_Zeri[1].Z_yzero=0;
		KK_Zeri[1].Z_xrif=0;
		KK_Zeri[1].Z_yrif=0;
	}

	//se modo automatico e scheda selezionata non e' da assemblare
	if(mode && !KK_Zeri[0].Z_ass)
	{
		//se il file era chiuso prima di entrare nella routine
		if(is_open)
		{
			//chiude il file
			delete FZeri;
			FZeri=NULL;
		}

		if(!tv_on && !mode)
		{
			Set_Tv(3); // chiude immagine su video VISIONE MASSIMONE 991015
		}

		return 1;
	}

	//se apprendimento del solo punto di riferimento e scheda corrente no MASTER
	if((z_type==ZER_APP_REF) && (n_scheda!=0))
	{
		//se in MASTER punto di riferimento coincidente con zero esci
		if (fabs(zxm_oo-rxm_oo)<0.02 && fabs(zym_oo-rym_oo)<0.02)
		{
			//se modo manuale notifica impossibilita a procedere
			if(mode==ZER_APP_MANUAL)
			{
				char buf[180];
				snprintf( buf, sizeof(buf),NOREFAPP_MSG,n_scheda);
				W_Mess(buf);
			}
		
			if(is_open)
			{
				delete FZeri;
				FZeri=NULL;
			}

			return 1; // L704
		}
	}

	if(!tv_on && !mode)
	{
		//TODO_TV
		if( dtaval.matching_mode == MATCH_VECTORIAL )
		{
			set_style( VIDEO_EXPANDED );
			matchMode = AUTOAPP_VECTORIAL;
		}
		else
			set_style( VIDEO_NORMAL );

		Set_Tv(2);      // predispone per non richiudere immagine su video
	}

	//se apprendimento solo zero o zero+riferimento
	if(z_type==ZER_APP_ZEROREF || z_type==ZER_APP_ZERO)
	{
		//se modo automatico
		if(mode)
		{
			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			Set_Tv_Title( MsgGetString(Msg_00015) ); // zero scheda

			//ricerca zero
			yes_zero = Image_match(&zx_oo,&zy_oo,ZEROIMG,dtaval.matching_mode);

			//ricerca zero fallita
			if(!yes_zero)
			{
				//svuotamento buffer tastiera
				flushKeyboardBuffer();

				//notifica errore
				W_Mess(MsgGetString(Msg_00737),MSGBOX_YCENT,0,GENERIC_ALARM);

				if( dtaval.matching_mode == MATCH_VECTORIAL )
				{
					// seleziona il nome del file dati immagine in base al tipo immagine
					SetImageName( filename, ZEROIMG, DATA );

					// carica dati immagine
					ImgDataLoad( filename, &imgData );

					set_vectorial_parameters( imgData.vect_diameter, imgData.vect_tolerance, imgData.vect_smooth, imgData.vect_edge, imgData.vect_accumulator, imgData.vect_atlante_x, imgData.vect_atlante_y );
				}

				//apprendimento manuale della posizione dello zero
				yes_zero = ManualTeaching(&zx_oo,&zy_oo, MsgGetString(Msg_00015), matchMode);
			}
	
			bipbip();
	
			//procedura di ricerca automatica abbandonata
			if( Esc_Press() )
			{
				if(W_Deci(0,INTERRUPTMSG))
				{
					yes_zero=0;
				}
			}
		}
		else
		{
			// seleziona il nome del file dati immagine in base al tipo immagine
			SetImageName( filename, ZEROIMG, DATA );

			// carica dati immagine
			ImgDataLoad( filename, &imgData );

			// setta i parametri della telecamera
			prevBright = GetImageBright();
			prevContrast = GetImageContrast();
			SetImgBrightCont( imgData.bright*655, imgData.contrast*655 );

			if( dtaval.matching_mode == MATCH_VECTORIAL )
			{
				set_vectorial_parameters( imgData.vect_diameter, imgData.vect_tolerance, imgData.vect_smooth, imgData.vect_edge, imgData.vect_accumulator, imgData.vect_atlante_x, imgData.vect_atlante_y );
			}

			//altrimenti: ricerca zero manuale
			yes_zero=ManualTeaching(&zx_oo,&zy_oo, MsgGetString(Msg_00015), matchMode);
		}
	}

	//se apprendimento eseguito e apprendimento per zero+rif. o solo zero
	if(yes_zero && (z_type==ZER_APP_ZEROREF || z_type==ZER_APP_ZERO))
	{
		//se in MASTER zero e riferimento non coincidenti
		if(fabs(rxm_oo-zxm_oo)>0.01 || fabs(rym_oo-zym_oo)>0.01) //L704b
		{
			//sposta anche rifer. della stessa quantita' dello zero
			rx_oo+=zx_oo-(KK_Zeri[0].Z_xzero); //L704b
			ry_oo+=zy_oo-(KK_Zeri[0].Z_yzero);
		}
		else
		{
			//altrimenti setta flag zero e riferimento coincidenti
			zero_near=1; //L704b
		}
	}

	if(yes_zero) //se apprendimento eseguito
	{
		//se modo manuale, scheda selezionata non e' MASTER e apprendimento zero+rifer.
		if(!mode && (n_scheda!=0) && (z_type==ZER_APP_ZEROREF) )
		{
			if( dtaval.matching_mode == MATCH_CORRELATION )
			{
				int cam = pauseLiveVideo();

				if(W_Deci(1,MEMIMAGEQ)) //chiede conferma apprend. immagine zero scheda
				{
					ImageCaptureSave( ZEROIMG, YESCHOOSE, cam ); //esegue apprend.
				}

				playLiveVideo( cam );
			}
			else
			{
				imgData.vect_diameter = get_diameter();
				imgData.vect_tolerance = get_tolerance();
				imgData.vect_edge = (unsigned char)get_edge();
				imgData.vect_smooth = (unsigned char)get_smooth();
				imgData.vect_accumulator = get_accumulator();
				imgData.vect_atlante_x = (short int)get_area_x();
				imgData.vect_atlante_y = (short int)get_area_y();
				// salva luminosita' e contrasto correnti
				imgData.bright = GetImageBright() / 655;
				imgData.contrast = GetImageContrast() / 655;

				// salva dati immagine
				ImgDataSave( filename, &imgData );
			}
		}

		//se zero troppo vicino a riferimento
		if(zero_near)
		{
			//pone riferimento uguale a zero
			rx_oo=zx_oo;
			ry_oo=zy_oo;
			//azzera il flag per evitare che venga considerato come un errore
			zero_near=0;
		}

		//in MASTER punto di riferimento coincidente con zero e se scheda corrente
		//non e' MASTER: non eseguire apprendimento o ricerca riferimento
		if(!((fabs(zxm_oo-rxm_oo)<0.02 && fabs(zym_oo-rym_oo)<0.02) && (n_scheda!=0)))
		{
			if(z_type==ZER_APP_ZEROREF || z_type==ZER_APP_REF)
			{
				//apprendimento riferimento
				//se modo automatico
				if(mode)
				{
					SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );
			
					//ricerca riferimento
					Set_Tv_Title( MsgGetString(Msg_00016) );    // rif. scheda

					yes_rife = Image_match(&rx_oo,&ry_oo,RIFEIMG,dtaval.matching_mode);
					
					//se riferimento non trovato
					if(!yes_rife)
					{
						flushKeyboardBuffer();

						//notifica errore
						W_Mess(MsgGetString(Msg_00737),MSGBOX_YCENT,0,GENERIC_ALARM);
						
						if( dtaval.matching_mode == MATCH_VECTORIAL )
						{
							// seleziona il nome del file dati immagine in base al tipo immagine
							SetImageName( filename, RIFEIMG, DATA );

							// carica dati immagine
							ImgDataLoad( filename, &imgData );

							set_vectorial_parameters( imgData.vect_diameter, imgData.vect_tolerance, imgData.vect_smooth, imgData.vect_edge, imgData.vect_accumulator, imgData.vect_atlante_x, imgData.vect_atlante_y );
						}

						//apprendimento manuale
						yes_rife=ManualTeaching(&rx_oo,&ry_oo, MsgGetString(Msg_00016), matchMode);
					}

					bipbip();
	
					if(Esc_Press())
					{
						if (W_Deci(0,INTERRUPTMSG))
						{
							yes_rife=0;                //procedura abbandonata
						}
					}
				}
				else
				{
					// seleziona il nome del file dati immagine in base al tipo immagine
					SetImageName( filename, RIFEIMG, DATA );

					// carica dati immagine
					ImgDataLoad( filename, &imgData );

					// setta i parametri della telecamera
					SetImgBrightCont( imgData.bright*655, imgData.contrast*655 );

					if( dtaval.matching_mode == MATCH_VECTORIAL )
					{
						set_vectorial_parameters( imgData.vect_diameter, imgData.vect_tolerance, imgData.vect_smooth, imgData.vect_edge, imgData.vect_accumulator, imgData.vect_atlante_x, imgData.vect_atlante_y );
					}

					//apprendimento manuale
					yes_rife=ManualTeaching(&rx_oo,&ry_oo, MsgGetString(Msg_00016), matchMode);
				}

				//se modo manuale, apprendimento zero+riferimento e non MASTER e modalita' correlazione
				if(yes_rife && !mode && (n_scheda!=0) && (z_type==ZER_APP_ZEROREF))
				{
					if( dtaval.matching_mode == MATCH_CORRELATION )
					{
						int cam = pauseLiveVideo();

						if(W_Deci(1,MEMIMAGEQ)) //chiede memorizzazione immagine
						{
							ImageCaptureSave( RIFEIMG, YESCHOOSE, cam ); //memorizza immagine punto di riferimento
						}

						playLiveVideo( cam );
					}
					else
					{
						imgData.vect_diameter = get_diameter();
						imgData.vect_tolerance = get_tolerance();
						imgData.vect_edge = (unsigned char)get_edge();
						imgData.vect_smooth = (unsigned char)get_smooth();
						imgData.vect_accumulator = get_accumulator();
						imgData.vect_atlante_x = (short int)get_area_x();
						imgData.vect_atlante_y = (short int)get_area_y();
						// salva luminosita' e contrasto correnti
						imgData.bright = GetImageBright() / 655;
						imgData.contrast = GetImageContrast() / 655;

						// salva dati immagine
						ImgDataSave( filename, &imgData );
					}
				}

				//se apprendimento riferimento eseguito e modo=zero+rifer. o solo rifer.
				if((yes_rife && (z_type==ZER_APP_ZEROREF || z_type==ZER_APP_REF)))
				{
					//se zero e riferimento appresi o riconosciuti sono troppo vicini
					//notifica errore
					if( (fabs(rx_oo-zx_oo)<(float)MINXAPP) && (fabs(ry_oo-zy_oo)<(float)MINYAPP))
					{
						if(fabs(rx_oo-zx_oo)>0.01 || fabs(ry_oo-zy_oo)>0.01)
						{
							W_Mess(TOONEAR);
						}

						//pone riferimento uguale a zero
						rx_oo=zx_oo;
						ry_oo=zy_oo;
						zero_near=1; //L704b
					}
				}
			}
		}
		else
		{
			rx_oo=zx_oo;
			ry_oo=zy_oo;
			zero_near=1;
		}
	}

	// Ritocco riferim. sotto dist. minima in app. master
	// il riferimento master/master+circ.1  diventa = allo zero scheda */
//	if(yes_rife && !z_type) {

	// verifica sempre se riferimento troppo vicino a zero scheda L704b

	if(yes_rife)
	{
		// Integr. Loris
		if( (fabs(rx_oo-zx_oo)<(float)MINXAPP) && (fabs(ry_oo-zy_oo)<(float)MINYAPP))
		{
			rx_oo=zx_oo;
			ry_oo=zy_oo;
			zero_near=1; //L704b
		}
	}

	if(yes_rife || zero_near)        // appr. riferimento
	{
		if(zero_near)  //L704b
		{
			arctan = 0;
		}
		else
		{
			deltax=(rx_oo-zx_oo);     // dimensioni dei segmenti app.
			deltay=(ry_oo-zy_oo);     // L704A
			arctan = (float) (atan2((double)deltay,(double)deltax));  // rad.
		}

		//memorizza struttura dati scheda appresa
		KK_Zeri[0].Z_xzero=zx_oo;
		KK_Zeri[0].Z_yzero=zy_oo;
		KK_Zeri[0].Z_xrif=rx_oo;
		KK_Zeri[0].Z_yrif=ry_oo;
		KK_Zeri[0].Z_rifangle=arctan;

		//se tabella zeri vuota o se apprendimento MASTER
		if(numerorecs<1 || nz_scheda==0)
		{
			KK_Zeri[0].Z_scheda=0;
			KK_Zeri[0].Z_ass=0;               // forza NO per master
			FZeri->Write(KK_Zeri[0],0);     // scrive su disco nuovo master
			FZeri->Reduce(sizeof(KK_Zeri[0])*(int)(2));
			nz_scheda=1;
			KK_Zeri[0].Z_ass=1;
		}

		//salva su disco la scheda appresa (in caso di MASTER copia la scheda MASTER su scheda 1)
		KK_Zeri[0].Z_scheda=nz_scheda;
		FZeri->Write(KK_Zeri[0],nz_scheda);
	}

	//se prima della procedura il file di zeri era chiuso, richiudere prima di uscire
	if(is_open)
	{
		delete FZeri;
		FZeri=NULL;
	}

	if(!tv_on && !mode)
	{
		Set_Tv(3); // chiude immagine su video
	}
	
	set_style( VIDEO_NORMAL );

	// Resetta i parametri della telecamera
	SetImgBrightCont( prevBright, prevContrast );

	if(z_type==ZER_APP_ZERO && yes_zero) return 1;
	if(z_type==ZER_APP_REF && yes_rife) return 1;
	if(z_type==ZER_APP_ZEROREF && (yes_rife && yes_zero)) return 1;

	return 0;
}// Z_scheda

// Servizio: lancio apprend. zero/riferimento con refresh della tabella
int Z_rscheda( int g_scheda, int g_type )
{
	int is_open=0;

	//SMOD151206 per sicurezza inserito comunque questo controllo
	if(g_scheda==0)
	{
		return 0;
	}
	
	if(FZeri==NULL)
	{
		FZeri=new ZerFile(QHeader.Prg_Default,ZER_ADDPATH,true);
		if(!FZeri->Open())                   
		{
			bipbip();
			W_Mess(NOZSCHFILE);
			delete FZeri;
			FZeri=NULL;
			return 0;
		}
		is_open=1;
	}

	int nr=FZeri->GetNRecs();               // n. recs in tab. zeri

	if(g_scheda>nr)
	{
		return 0;
	}

	if(!AskConfirm_ZRefBoardMode())
	{
		return 0;
	}

	int ret = Z_scheda(g_scheda,g_type);

	zerCurRow=zerCurRecord=1;
	Z_read_recs(0);
	panelCompositionRefresh = true;

	if(is_open)
	{
		delete FZeri;
		FZeri=NULL;
	}

	return ret;
}

// Servizio: apprend. sequenziali
int Z_sequenza( int a_type, int mode, std::string text )
{
	int auto_man=0;
	int n_schede;
	int loop_sch=1;
	int is_open=0;
	int interrupted=0; //L704
	char *Z_NomeFile;
	
	Z_NomeFile = (char *)malloc(MAXNPATH);
	
	if(!AskConfirm_ZRefBoardMode())
	{
		return 0;
	}
	
	if(FZeri==NULL)
	{
		FZeri=new ZerFile(QHeader.Prg_Default,ZER_ADDPATH,true);
		if(!FZeri->Open())
		{
			bipbip();
			W_Mess(NOZSCHFILE);
			delete FZeri;
			FZeri=NULL;
			return 0;
		}
		is_open=1;
	}
	
	n_schede=FZeri->GetNRecs();                // n. recs in tab. zeri
	
	if(n_schede==0)
	{
		bipbip();
	
		if(is_open)
		{
			delete FZeri;
			FZeri=NULL;
		}
	
		return 0;
	}
	
	struct Zeri *zerDat=new struct Zeri [n_schede];
	FZeri->ReadAll(zerDat,n_schede);
	
	if(is_open)
	{
		delete FZeri;
		FZeri=NULL;
	}
	
	//FINESTRA AUTO-MAN
	switch(mode)
	{
		case 1:
			auto_man=1;
			break;
		case 2:
			auto_man=0;
			break;
		default:
			auto_man = W_DeciManAuto( text.c_str(), 10 );
			break;
	}
	
	CPan pan= CPan( -1, 1, MsgGetString(Msg_05040) );
	
	while(loop_sch<n_schede)
	{
		if(!zerDat[loop_sch].Z_ass)
		{
			loop_sch++;
			continue;
		}
	
		//se apprendimento automatico del riferimento e se riferimento e zero
		//sono coincidenti salta apprendimento del riferimento
		if( auto_man && a_type &&
			((fabs(zerDat[loop_sch].Z_xzero-zerDat[loop_sch].Z_xrif)<0.02) && (fabs(zerDat[loop_sch].Z_yzero-zerDat[loop_sch].Z_yrif)<0.02)))
		{
			loop_sch++;
			continue;
		}

		if(!Z_scheda(loop_sch,a_type+1,auto_man,1))
		{
			interrupted=1; //L704
			break; // ESC in appr.
		}
		
		loop_sch++;
	}
	
	delete[] zerDat;
	
	if (!mode)
	{
		zerCurRow=zerCurRecord=1;
		Z_read_recs(0);
		panelCompositionRefresh = true;
	}
	
	free(Z_NomeFile);
	return interrupted; //L704
} // Z_sequenza



//---------------------------------------------------------------------------
// finestra: Boards number along axes
//---------------------------------------------------------------------------
class BoardsNumberUI : public CWindowParams
{
public:
	BoardsNumberUI( CWindow* parent, int x, int y ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X | WIN_STYLE_NO_MENU | WIN_STYLE_EDITMODE_ON );
		SetClientAreaPos( 0, 7 );
		SetClientAreaSize( 40, 5 );
		SetTitle( MsgGetString(Msg_00051) );

		m_x = x;
		m_y = y;
	}

	int GetExitCode() { return m_exitCode; }
	int GetNumberOnX() { return m_x; }
	int GetNumberOnY() { return m_y; }

	typedef enum
	{
		NUM_X,
		NUM_Y
	} combo_labels;

protected:
	void onInit()
	{
		// create combos
		m_combos[NUM_X] = new C_Combo( 8, 1, MsgGetString(Msg_00052), 3, CELL_TYPE_UINT );
		m_combos[NUM_Y] = new C_Combo( 8, 3, MsgGetString(Msg_00053), 3, CELL_TYPE_UINT );

		// set params
		m_combos[NUM_X]->SetVMinMax( 1, 99 );
		m_combos[NUM_Y]->SetVMinMax( 1, 99 );

		// add to combo list
		m_comboList->Add( m_combos[NUM_X], 0, 0 );
		m_comboList->Add( m_combos[NUM_Y], 1, 0 );
	}

	void onShow()
	{
		tips = new CPan( 20, 2, MsgGetString(Msg_00296), MsgGetString(Msg_00297) );
	}

	void onRefresh()
	{
		m_combos[NUM_X]->SetTxt( m_x );
		m_combos[NUM_Y]->SetTxt( m_y );
	}

	void onEdit()
	{
		m_x = m_combos[NUM_X]->GetInt();
		m_y = m_combos[NUM_Y]->GetInt();
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_ESC:
				m_exitCode = WIN_EXITCODE_ESC;
				break;

			case K_ENTER:
				forceExit();
				m_exitCode = WIN_EXITCODE_ENTER;
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		delete tips;
	}

	CPan* tips;
	int m_exitCode;
	int m_x, m_y;
};


// Autoapprend. riferimenti quadrotto e composiz. circuito multiplo.
int M_scheda(void)
{
	float zx_oo, zy_oo;
	float rx_oo, ry_oo;
	float deltax, deltay;
	float x_step, y_step;
	int numerorecs;
	int C_assex=1, C_assey=1;
	int xloop, yloop, nscheda=1;
	int is_open=0;

	struct Zeri board;
	struct Zeri master;

	if(FZeri==NULL)
	{
		FZeri=new ZerFile(QHeader.Prg_Default,ZER_ADDPATH,true);
		if(!FZeri->Open())
		{
			bipbip();
			W_Mess(NOZSCHFILE);
			delete FZeri;
			FZeri=NULL;
			return 0;
		}
		is_open=1;
	}

	numerorecs=FZeri->GetNRecs();              // n. recs in tab. zeri
	
	if(numerorecs<2)                           // no recs zeri presenti
	{
		if(is_open)
		{
			delete FZeri;
			FZeri=NULL;
		}
		W_Mess(NOZSCHEDA);
		return 0;
	}

	FZeri->Read(master,0);                   // read board 1...
	FZeri->Read(board,1);                   // ...read again

	zx_oo = board.Z_xzero;
	zy_oo = board.Z_yzero;
	rx_oo = board.Z_xrif;
	ry_oo = board.Z_yrif;

	// appr. zero scheda basso dx
	if(!ManualTeaching(&zx_oo,&zy_oo, MsgGetString(Msg_00017)))
	{
		if(is_open)
		{
			delete FZeri;
			FZeri=NULL;
		}
		
		return 0;
	}

	rx_oo=zx_oo;
	ry_oo=zy_oo;

	if(!ManualTeaching(&rx_oo,&ry_oo, MsgGetString(Msg_00018)))
	{
		// appr. zero scheda alto  sx
		if(is_open)
		{
			delete FZeri;
			FZeri=NULL;
		}

	  return 0;
	}

	if((rx_oo==zx_oo) && (ry_oo==zy_oo))
	{
		W_Mess(COINCID);
	
		if(is_open)
		{
			delete FZeri;
			FZeri=NULL;
		}

		return 0;
	}

	double dtheta;
	int ok_dtheta=0;
	
	/*
	if(W_Deci(1,ZMULTI_APP_DELTA))
	{
		ok_dtheta=AppZMasterDTheta(zx_oo,zy_oo,dtheta);
	}
	*/


	deltax=rx_oo-zx_oo;         // dimensioni dei segmenti app.
	deltay=ry_oo-zy_oo;


	BoardsNumberUI inputBox( 0, 1, 1 );
	inputBox.Show();
	inputBox.Hide();
	if( inputBox.GetExitCode() == WIN_EXITCODE_ESC )
	{
		if(is_open)
		{
			delete FZeri;
			FZeri=NULL;
		}
		return 0;
	}

	if( inputBox.GetExitCode() == WIN_EXITCODE_ENTER )
	{
		C_assex = inputBox.GetNumberOnX();
		C_assey = inputBox.GetNumberOnY();

		if((C_assex*C_assey+1)<numerorecs)    // records diminuiti
		{
			FZeri->Reduce(sizeof(KK_Zeri[0])*(int)(C_assex*C_assey+1));
		}

		if(C_assex==1 && C_assey==1)
		{
			board.Z_xzero=zx_oo;
			board.Z_yzero=zy_oo;
			board.Z_xrif=board.Z_xzero+(master.Z_xrif-master.Z_xzero);
			board.Z_yrif=board.Z_yzero+(master.Z_yrif-master.Z_yzero);

			board.Z_rifangle = atan2((board.Z_yrif-board.Z_yzero),(board.Z_xrif-board.Z_xzero));
			board.Z_scheda= 1;
			board.Z_ass=1;
			board.Z_note[0]='\0';

			 FZeri->Write( board, 1 ); // write record

			//Switch_ZRefSearch_Board(1);
		}
		else                                         // composizione circuito
		{
			nscheda=1;

			if(C_assex==1)
			{
				x_step=deltax;
			}
			else
			{
				x_step=deltax/(C_assex-1);
			}

			if(C_assey==1)
			{
				y_step=deltay;
			}
			else
			{
				y_step=deltay/(C_assey-1);
		}

		if(ok_dtheta)
		{
			float xRot = (cos(-dtheta)*x_step-sin(-dtheta)*y_step);
			float yRot = (sin(-dtheta)*x_step+cos(-dtheta)*y_step);
			x_step = xRot;
			y_step = yRot;

			float dx = (master.Z_xrif-master.Z_xzero);
			float dy = (master.Z_yrif-master.Z_yzero);

			master.Z_xrif=(cos(-dtheta)*dx-sin(-dtheta)*dy)+master.Z_xzero;
			master.Z_yrif=(sin(-dtheta)*dx+cos(-dtheta)*dy)+master.Z_yzero;
		}

		for(xloop=0;xloop<C_assex;xloop++)
		{
			for(yloop=0;yloop<C_assey;yloop++)
			{
				board.Z_xzero    = zx_oo+(x_step*xloop);
				board.Z_yzero    = zy_oo+(y_step*yloop);

				board.Z_xrif     = board.Z_xzero+(master.Z_xrif-master.Z_xzero);
				board.Z_yrif     = board.Z_yzero+(master.Z_yrif-master.Z_yzero);

				board.Z_rifangle = atan2((board.Z_yrif-board.Z_yzero),(board.Z_xrif-board.Z_xzero));
				board.Z_scheda   = nscheda;

				board.Z_ass=1;

				board.Z_note[0]='\0';

				FZeri->Write( board, nscheda );

				nscheda++;
				}
			}
	
			/*
			if(W_Deci(0,ZREF_SEARCHMODE_ASKPANEL))
			{
				Create_ZRefPanelFile();
			}
			*/
		}
	}

	if(is_open)
	{
		delete FZeri;
		FZeri=NULL;
	}

	zerCurRow=zerCurRecord=1;

	Z_read_recs(0);
	panelCompositionRefresh = true;

	zMod|=ZER_UPDATED;
	
	return 1;
} // M_scheda

int Z_AppMC(void)
{
	int matchMode = 0;
	char filename[MAXNPATH+1];
	img_data imgData;

	if(!Exist_ZRefPanelFile())
	{
		return 1;
	}
	
	char path[MAXNPATH];
	
	dta_data dtaval;
	Read_Dta( &dtaval );

	PrgPath(path,QHeader.Prg_Default,PRG_ZER_MC);
	
	ZerFile *FZeriMC=new ZerFile(path,ZER_NOADDPATH,true);
	
	if(!FZeriMC->Open())
	{
		bipbip();
		W_Mess(NOZSCHFILE);
		delete FZeriMC;
		return 0;
	}
	
	int nz_mc=FZeriMC->GetNRecs();
	
	if(nz_mc==0)
	{
		delete FZeriMC;
		return 0;
	}
	
	int is_open=0;
	
	if(FZeri==NULL)
	{
		FZeri=new ZerFile(QHeader.Prg_Default);
		if(!FZeri->Open())
		{
			bipbip();
			W_Mess(NOZSCHFILE);
			delete FZeri;
			delete FZeriMC;
			FZeri=NULL;
			return 0;
		}
		
		is_open=1;
	}
	
	int nz=FZeri->GetNRecs();
	
	if(nz!=nz_mc)
	{
		if(is_open)
		{
			delete FZeri;
			FZeri=NULL;
		}
		delete FZeriMC;
		return 0;
	}
	
	struct Zeri firstZ,lastZ;
	
	FZeri->Read(firstZ,1);
	FZeri->Read(lastZ,nz-1);
	
	float zxF=firstZ.Z_xzero;
	float zyF=firstZ.Z_yzero;
	
	float rxF=lastZ.Z_xrif;
	float ryF=lastZ.Z_yrif;
	
	SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );
	
	Set_Tv_Title( MsgGetString(Msg_00015) ); // zero scheda
	
	int yes_zero=0;
	
	for(int i=0;i<2;i++)
	{
		if(i==0)
		{
			yes_zero=Image_match(&zxF,&zyF,ZEROIMG,dtaval.matching_mode);
		}
		else
		{
			yes_zero=Image_match(&rxF,&ryF,RIFEIMG,dtaval.matching_mode);
		}
	
		//ricerca zero fallita
		if(!yes_zero)
		{
			//svuotamento buffer tastiera
			flushKeyboardBuffer();
			
			//notifica errore
			W_Mess(MsgGetString(Msg_00737),MSGBOX_YCENT,0,GENERIC_ALARM);

			//apprendimento manuale della posizione dello zero
			if(i==0)
			{
				if( dtaval.matching_mode == MATCH_VECTORIAL )
				{
					// seleziona il nome del file dati immagine in base al tipo immagine
					SetImageName( filename, ZEROIMG, DATA );

					// carica dati immagine
					ImgDataLoad( filename, &imgData );

					set_vectorial_parameters( imgData.vect_diameter, imgData.vect_tolerance, imgData.vect_smooth, imgData.vect_edge, imgData.vect_accumulator, imgData.vect_atlante_x, imgData.vect_atlante_y );

					matchMode = AUTOAPP_VECTORIAL;
				}

				yes_zero=ManualTeaching(&zxF,&zyF, MsgGetString(Msg_00015), matchMode);
			}
			else
			{
				if( dtaval.matching_mode == MATCH_VECTORIAL )
				{
					// seleziona il nome del file dati immagine in base al tipo immagine
					SetImageName( filename, RIFEIMG, DATA );

					// carica dati immagine
					ImgDataLoad( filename, &imgData );

					set_vectorial_parameters( imgData.vect_diameter, imgData.vect_tolerance, imgData.vect_smooth, imgData.vect_edge, imgData.vect_accumulator, imgData.vect_atlante_x, imgData.vect_atlante_y );
				}

				yes_zero=ManualTeaching(&rxF,&ryF, MsgGetString(Msg_00015), matchMode);
			}
		
			if(!yes_zero)
			{
				break;
			}
		}
	}
	
	if(!yes_zero)
	{
		if(is_open)
		{
			delete FZeri;
			FZeri=NULL;
		}
		delete FZeriMC;
		return 0;
	}
	
	struct Zeri masterMC;
	struct Zeri masterBrd;
	
	FZeri->Read(masterBrd,0);
	FZeriMC->Read(masterMC,0);
	
	float zxM=masterMC.Z_xzero;
	float zyM=masterMC.Z_yzero;
	
	//float rxM=masterMC.Z_xrif;
	//float ryM=masterMC.Z_yrif;
	
	//NOTA: dtheta rappresenta quanto la scheda attuale e' ruotata rispetto
	//al primo posizionamento del master e NON rispetto ad una messa a squadra
	//perfetta virtuale.
	float dtheta=atan2((ryF-zyF),(rxF-zxF))-masterMC.Z_rifangle;
	
	for(int i=1;i<nz_mc;i++)
	{
		struct Zeri board;
		
		FZeri->Read(board,i);
	
		int mount_flag=board.Z_ass;
		
		FZeriMC->Read(board,i);
	
		double dx,dy;
		
		if(i!=0)
		{
			dx=board.Z_xzero-zxM;
			dy=board.Z_yzero-zyM;
		
			board.Z_xzero=(cos(dtheta)*dx-sin(dtheta)*dy)+zxF;
			board.Z_yzero=(sin(dtheta)*dx+cos(dtheta)*dy)+zyF;	
		}
	
		dx=board.Z_xrif-zxM;
		dy=board.Z_yrif-zyM;
	
		board.Z_xrif=(cos(dtheta)*dx-sin(dtheta)*dy)+zxF;
		board.Z_yrif=(sin(dtheta)*dx+cos(dtheta)*dy)+zyF;
	
		board.Z_rifangle+=dtheta;
	
		board.Z_ass=mount_flag;
	
		FZeri->Write( board, i );
	}
	
	if(is_open)
	{
		delete FZeri;
		FZeri=NULL;
	}
	
	delete FZeriMC;
	return 1;
}

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// Controlla se ZRefPanelFile esiste
// ritorna: 1 se esiste, 0 altrimenti
//-------------------------------------------------------------------------
int Exist_ZRefPanelFile(void)
{
	char path[MAXNPATH];
	PrgPath( path, QHeader.Prg_Default, PRG_ZER_MC );

	return (access( path, F_OK ) == 0 ) ? 1 : 0;
}

int Create_ZRefPanelFile(void)
{
	int nzer = FZeri->GetNRecs();
	if(nzer<3)
	{
		return 0;
	}
	
	char path[MAXNPATH];
	
	PrgPath(path,QHeader.Prg_Default,PRG_ZER_MC);
	
	ZerFile *FZeriMC=new ZerFile(path,ZER_NOADDPATH);
	
	FZeriMC->Create();
	
	FZeriMC->Open();
	
	struct Zeri ZMaster;
	
	FZeri->Read(ZMaster,0);
	
	struct Zeri first,last,board;
	
	FZeri->Read(first,1);
	FZeri->Read(last,nzer-1);
	
	memcpy(&board,&ZMaster,sizeof(board));
	board.Z_xzero=first.Z_xzero;
	board.Z_yzero=first.Z_yzero;
	board.Z_xrif=last.Z_xrif;
	board.Z_yrif=last.Z_yrif;
	
	//NOTA:Z_rifangle e' l'angolo di riferimento del circuitone master ma
	//non considerato come se fosse a squadra pefetta: angolo fisico
	board.Z_rifangle=atan2((board.Z_yrif-board.Z_yzero),(board.Z_xrif-board.Z_xzero));
	
	/*
	double angle;
	
	if(AppZMasterDTheta(first.Z_xzero,first.Z_yzero,angle))
	{
	board.Z_rifangle-=angle;
	}
	*/
	
	FZeriMC->Write(board,0);
	
	for(int i=1;i<nzer;i++)
	{
		struct Zeri board;
		
		FZeri->Read(board,i);
		FZeriMC->Write(board,i);
	}
	
	FZeriMC->Close();
	delete FZeriMC;
	
	return 1;
}

void Remove_ZRefPanelFile(void)
{
	char path[MAXNPATH];
	PrgPath( path, QHeader.Prg_Default, PRG_ZER_MC );
	
	remove( path );
}


int Set_ZRefSearch_Panel()
{
	if( FZeri->GetNRecs() < 3 )
	{
		W_Mess( MsgGetString(Msg_00848) );
		return 0;
	}

	if( !Exist_ZRefPanelFile() )
	{
		Create_ZRefPanelFile();
	}

	panelCompositionRefresh = true;
	return 1;
}

int Set_ZRefSearch_Board()
{
	if( Exist_ZRefPanelFile() )
	{
		Remove_ZRefPanelFile();
	}

	panelCompositionRefresh = true;
	return 1;
}

void Switch_ZRefSearch_Board(int show_msg/*=0*/)
{
	if(!Exist_ZRefPanelFile())
	{
		return;
	}

	Remove_ZRefPanelFile();

	if(show_msg)
	{
		W_Mess(ZREF_SEARCHMODE_SWITCHBRD1);
	}
}

int AskConfirm_ZRefBoardMode()
{
	if( Exist_ZRefPanelFile() )
	{
		if(!W_Deci(1,ZREF_SEARCHMODE_SWITCHBRD2))
		{
			return 0;
		}

		Switch_ZRefSearch_Board();
	}

	return 1;
}

//-------------------------------------------------------------------------


int Set_Matching_Correlation()
{
	Dta_Val->matching_mode = MATCH_CORRELATION;

	Save_matchingMode( MATCH_CORRELATION );

	panelCompositionRefresh = true;
	return 1;
}

int Set_Matching_Vectorial()
{
	Dta_Val->matching_mode = MATCH_VECTORIAL;

	Save_matchingMode( MATCH_VECTORIAL );

	panelCompositionRefresh = true;
	return 1;
}

//-------------------------------------------------------------------------
// * Gestione Zeri Scheda >>S030901

void zerIncRow(void)
{
	if(FZeri->GetNRecs()==0)
	{
		return;
	}

	zerCurRecord++;
	zerCurRow++;

	if(zerCurRecord>=FZeri->GetNRecs())
	{
		zerCurRecord=FZeri->GetNRecs()-1;
		zerCurRow--;
		return;
	}

	if(zerCurRow==MAXRZER)
	{
		zerCurRow=MAXRZER-1;
		Z_dat_scroll(0);
		FZeri->Read(KK_Zeri[MAXRZER-1],zerCurRecord);
	}

	panelCompositionRefresh = true;
}

//DANY270103
void zerDecRow(void)
{
	if(FZeri->GetNRecs()==0)
	{
		return;
	}

	zerCurRecord--;
	zerCurRow--;

	if(zerCurRecord<1)
	{
		zerCurRecord=zerCurRow=1;
		Z_read_recs(0);
		panelCompositionRefresh = true;
		return;
	}

	if(zerCurRow<0)
	{
		zerCurRow=0;
		Z_dat_scroll(1);
		FZeri->Read(KK_Zeri[0],zerCurRecord);
	}

	panelCompositionRefresh = true;
}

void Zeri_CtrlPagDN()
{
	if(FZeri->GetNRecs()==0)
	{
		return;
	}

	if(FZeri->GetNRecs()<(MAXRZER-1))
		zerCurRecord=zerCurRow=FZeri->GetNRecs()-1;
	else
	{
		zerCurRecord=FZeri->GetNRecs()-1;
		Z_read_recs(zerCurRecord-(MAXRZER-1));
		panelCompositionRefresh = true;
		zerCurRow=MAXRZER-1;
	}
}

void Zeri_CtrlPagUP()
{
	if(FZeri->GetNRecs()==0)
	{
		return;
	}

	zerCurRow=1;
	zerCurRecord=1;
	Z_read_recs(0);
	panelCompositionRefresh = true;
}

void Zeri_DelScheda(void)
{
	if(!AskConfirm_ZRefBoardMode())
	{
		return;
	}

	int nrec=FZeri->GetNRecs();

	if(nrec<=2)
	{
		return;
	}

	char buf[80];
	snprintf( buf, 80, MsgGetString(Msg_01437), zerCurRecord );
	if(!W_Deci(0,buf))
	{
		return;
	}

	if(Get_AssemblingFlag() || Get_DosaFlag())
	{
		if(!W_Deci(0,WARN_ASSEMBLING1))
		{
			return;
		}
	}

	struct Zeri *bufZeri=new Zeri[nrec];

	for(int i=0;i<nrec;i++)
	{
		FZeri->Read(bufZeri[i],i);
	}

	if(zerCurRecord!=nrec-1)
	{
		memcpy(bufZeri+zerCurRecord,bufZeri+zerCurRecord+1,sizeof(struct Zeri)*(nrec-zerCurRecord-1));
		for(int i=zerCurRecord;i<nrec-1;i++)
		{
			bufZeri[i].Z_scheda=i;
			FZeri->Write(bufZeri[i],i);
		}
	}

	FZeri->Reduce(sizeof(struct Zeri)*(nrec-1));

	zerCurRow=zerCurRecord=1;
	Z_read_recs(0);
	panelCompositionRefresh = true;
}

void Zeri_NewScheda()
{
	if(!W_Deci(1,MsgGetString(Msg_01435)))
	{
		return;
	}

	if(!AskConfirm_ZRefBoardMode())
	{
		return;
	}

	int tmp=0;

	if(FZeri->GetNRecs()==0)
	{
		tmp|=ZER_NEWMASTER;
	}

	if(New_scheda())
	{
		if ((FZeri->GetNRecs()-1)<(MAXRZER-1))
			zerCurRow=FZeri->GetNRecs()-1;
		else
			zerCurRow=MAXRZER-1;

		zerCurRecord=FZeri->GetNRecs()-1;
		Z_read_recs(zerCurRecord-zerCurRow);
		panelCompositionRefresh = true;
		zMod|=ZER_UPDATED | tmp;
	}
}


int Zeri_AutoMaster()
{
	if(!AskConfirm_ZRefBoardMode())
	{
		return 0;
	}

	ZSchMaster_App();
	zerCurRow=1;
	zerCurRecord=1;
	panelCompositionRefresh = true;
	return 1;
}

int Zeri_Zerosingle()
{
	Z_rscheda(zerCurRecord,1);        // appr. singolo zero scheda
	return 1;
}

int Zeri_Rifsingle()
{
	Z_rscheda(zerCurRecord,2);  // appr. singolo riferim. scheda
	return 1;
}


//DANY161202
void ZerPAGEUP(void)
{
  if(FZeri->GetNRecs()==0) //SMOD260603 +++
  {
    bipbip();
    return;
  }

  zerCurRecord--;
  zerCurRow--;

  if(zerCurRecord-2*(MAXRZER-1)<1)
  {
	  zerCurRecord=1;
	  zerCurRow=1;
  }
  else
    zerCurRecord-=MAXRZER-1;

  Z_read_recs(zerCurRecord-zerCurRow);
  panelCompositionRefresh = true;
}

//DANY161202
void ZerPAGEDOWN(void)
{
  if(FZeri->GetNRecs()==0) //SMOD260603 +++
  {
    bipbip();
    return;
  }

  if(((zerCurRecord+MAXRZER-1)>=FZeri->GetNRecs())||((zerCurRecord+2*(MAXRZER-1))>=FZeri->GetNRecs()))
  { zerCurRecord=FZeri->GetNRecs()-1;
    if(zerCurRecord+1<MAXRZER)
      zerCurRow=zerCurRecord;
    else
      zerCurRow=MAXRZER-1;
  }
  else
    zerCurRecord+=MAXRZER-1;

  Z_read_recs(zerCurRecord-zerCurRow);
  panelCompositionRefresh = true;
}



//---------------------------------------------------------------------------
// finestra: Panel composition
//---------------------------------------------------------------------------
class PanelCompositionUI : public CWindowTable
{
public:
	PanelCompositionUI( CWindow* parent ) : CWindowTable( parent )
	{
		SetStyle( WIN_STYLE_CENTERED_X );
		SetClientAreaPos( 0, 6 );
		SetClientAreaSize( 79, 15 );
		SetTitle( MsgGetString(Msg_00351) );


		SM_SearchMode = new GUI_SubMenu();
		SM_SearchMode->Add( MsgGetString(Msg_00713), K_F1, 0, NULL, Set_ZRefSearch_Board );
		SM_SearchMode->Add( MsgGetString(Msg_00724), K_F2, 0, NULL, Set_ZRefSearch_Panel );

		SM_MatchMode = new GUI_SubMenu();
		SM_MatchMode->Add( MsgGetString(Msg_07013), K_F1, 0, NULL, Set_Matching_Correlation );
		SM_MatchMode->Add( MsgGetString(Msg_07014), K_F2, 0, NULL, Set_Matching_Vectorial );

		Dta_Val = new dta_data;
		Read_Dta( Dta_Val );
	}

	~PanelCompositionUI()
	{
		delete SM_SearchMode;
		delete SM_MatchMode;
		delete Dta_Val;
	}

protected:
	void onInit()
	{
		// create table
		m_table = new CTable( 3, 2, 8, TABLE_STYLE_DEFAULT, this );

		// add columns
		m_table->AddCol( MsgGetString(Msg_00489), 4, CELL_TYPE_UINT, CELL_STYLE_READONLY );
		m_table->AddCol( MsgGetString(Msg_00534), 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY );
		m_table->AddCol( MsgGetString(Msg_00535), 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY );
		m_table->AddCol( MsgGetString(Msg_00536), 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY );
		m_table->AddCol( MsgGetString(Msg_00537), 7, CELL_TYPE_SDEC, CELL_STYLE_READONLY );
		m_table->AddCol( MsgGetString(Msg_00538), 8, CELL_TYPE_SDEC, CELL_STYLE_READONLY );
		m_table->AddCol( MsgGetString(Msg_00539), 1, CELL_TYPE_YN );
		m_table->AddCol( MsgGetString(Msg_00060), 25, CELL_TYPE_TEXT );
	}

	void onShow()
	{
		panelCompositionRefresh = true;
	}

	void onRefresh()
	{
		if( panelCompositionRefresh == false )
		{
			return;
		}

		panelCompositionRefresh = false;

		GUI_Freeze_Locker lock;

		int num = FZeri->GetNRecs();
		for( unsigned int i = 0; i < m_table->GetRows(); i++ )
		{
			if( i < num )
			{
				if( KK_Zeri[i].Z_scheda == 0 )
					m_table->SetText( i, 0, "MAST" );
				else
					m_table->SetText( i, 0, KK_Zeri[i].Z_scheda );

				m_table->SetText( i, 1, KK_Zeri[i].Z_xzero );
				m_table->SetText( i, 2, KK_Zeri[i].Z_yzero );
				m_table->SetText( i, 3, KK_Zeri[i].Z_xrif );
				m_table->SetText( i, 4, KK_Zeri[i].Z_yrif );
				m_table->SetText( i, 5, (float)(180/PI)*KK_Zeri[i].Z_rifangle );
				m_table->SetTextYN( i, 6, KK_Zeri[i].Z_ass );
				m_table->SetText( i, 7, KK_Zeri[i].Z_note );
			}
			else
			{
				m_table->SetText( i, 0, "" );
				m_table->SetText( i, 1, "" );
				m_table->SetText( i, 2, "" );
				m_table->SetText( i, 3, "" );
				m_table->SetText( i, 4, "" );
				m_table->SetText( i, 5, "" );
				m_table->SetText( i, 6, "" );
				m_table->SetText( i, 7, "" );
			}
		}

		m_table->Select( zerCurRow, m_table->GetCurCol() );

		char buf[80];
		// scale factor
		snprintf( buf, 80, "%s : %1.5f%      ", MsgGetString(Msg_05182),  KK_Zeri[0].Z_scalefactor );
		DrawText( 3, 11, buf );

		// search type
		if( Exist_ZRefPanelFile() )
		{
			snprintf( buf, 80, "%s : %s      ", MsgGetString(Msg_00509), MsgGetString(Msg_00724) );
		}
		else
		{
			snprintf( buf, 80, "%s : %s      ", MsgGetString(Msg_00509), MsgGetString(Msg_00713) );
		}
		DrawText( 3, 12, buf );

		//matching type
		if( Dta_Val->matching_mode == MATCH_VECTORIAL )
		{
			snprintf( buf, 80, "%s : %s      ", MsgGetString(Msg_07012), MsgGetString(Msg_07014) );
		}
		else
		{
			snprintf( buf, 80, "%s : %s      ", MsgGetString(Msg_07012), MsgGetString(Msg_07013) );
		}
		DrawText( 3, 13, buf );
	}

	void onEdit()
	{
		int ass = KK_Zeri[zerCurRow].Z_ass;

		KK_Zeri[zerCurRow].Z_ass = m_table->GetYN( m_table->GetCurRow(), 6 );
		snprintf( KK_Zeri[zerCurRow].Z_note, 26, "%s", m_table->GetText( m_table->GetCurRow(), 7 ) );

		if( ass != KK_Zeri[zerCurRow].Z_ass )
			zMod |= ZER_UPDATED;

		FZeri->Write(KK_Zeri[zerCurRow],zerCurRecord);
	}

	void onShowMenu()
	{
		m_menu->Add( MsgGetString(Msg_00374), K_F3, 0, NULL, Zeri_AutoMaster ); // esegui circuito master
		m_menu->Add( MsgGetString(Msg_00051), K_F4, 0, NULL, M_scheda ); // composizione circuito multiplo
		m_menu->Add( MsgGetString(Msg_00376), K_F5, 0, NULL, boost::bind( &PanelCompositionUI::onSeqZeros, this ) ); // apprendimento zeri seq.
		m_menu->Add( MsgGetString(Msg_00377), K_F6, 0, NULL, Zeri_Zerosingle ); // apprendimento singolo zero
		m_menu->Add( MsgGetString(Msg_00378), K_F7, 0, NULL, boost::bind( &PanelCompositionUI::onSeqReferences, this ) ); // apprendimento rif. seq.
		m_menu->Add( MsgGetString(Msg_00379), K_F8, 0, NULL, Zeri_Rifsingle ); // apprendimento rif. sinfolo
		m_menu->Add( MsgGetString(Msg_00509), K_F9, 0, SM_SearchMode, NULL ); // selezione modo ricerca riferimenti
		m_menu->Add( MsgGetString(Msg_07012), K_F10, 0, SM_MatchMode, NULL ); // selezione modo riconoscimento riferimenti
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				Zeri_AutoMaster();
				return true;

			case K_F4:
				M_scheda();
				return true;

			case K_F5:
				onSeqZeros();
				return true;

			case K_F6:
				Zeri_Zerosingle();
				return true;

			case K_F7:
				onSeqReferences();
				return true;

			case K_F8:
				Zeri_Rifsingle();
				return true;

			case K_F9:
				SM_SearchMode->Show();
				return true;

			case K_F10:
				SM_MatchMode->Show();
				return true;

			case K_DOWN:
			case K_UP:
			case K_PAGEDOWN:
			case K_PAGEUP:
			case K_CTRL_PAGEUP:
			case K_CTRL_PAGEDOWN:
				return vSelect( key );

			default:
				break;
		}

		return false;
	}

private:
	//--------------------------------------------------------------------------
	// Sposta la selezione verticalmente
	//--------------------------------------------------------------------------
	bool vSelect( int key )
	{
		if( m_table->GetCurRow() < 0 )
			return false;

		if( key == K_DOWN )
		{
			zerIncRow();
			// aggiorno la tabella
			return true;
		}
		else if( key == K_UP )
		{
			zerDecRow();
			// aggiorno la tabella
			return true;
		}
		else if( key == K_PAGEDOWN )
		{
			ZerPAGEDOWN();
			// aggiorno la tabella
			return true;
		}
		else if( key == K_PAGEUP )
		{
			ZerPAGEUP();
			// aggiorno la tabella
			return true;
		}
		else if( key == K_CTRL_PAGEDOWN )
		{
			Zeri_CtrlPagDN();
			// aggiorno la tabella
			return true;
		}
		else if( key == K_CTRL_PAGEUP )
		{
			Zeri_CtrlPagUP();
			// aggiorno la tabella
			return true;
		}

		return false;
	}

	//--------------------------------------------------------------------------
	// Ricerca sequenziale zeri
	//--------------------------------------------------------------------------
	int onSeqZeros()
	{
		Deselect();

		// appr. sequenz. zero scheda
		Z_sequenza( 0, 0, MsgGetString(Msg_00376) );

		Select();
		return 1;
	}

	//--------------------------------------------------------------------------
	// Ricerca sequenziale riferimenti
	//--------------------------------------------------------------------------
	int onSeqReferences()
	{
		if( FZeri->GetNRecs() < 2 )
		{
			bipbip();
			return 0;
		}

		Deselect();

		Zeri master;
		FZeri->Read(master,0);

		if(fabs(master.Z_xzero-master.Z_xrif)<0.02 && fabs(master.Z_yzero-master.Z_yrif)<0.02)
		{
			W_Mess( MsgGetString(Msg_01476) );
		}
		else
		{
			Z_sequenza( 1, 0, MsgGetString(Msg_00378) );
		}

		Select();
		return 1;
	}


	GUI_SubMenu* SM_SearchMode;
	GUI_SubMenu* SM_MatchMode;
};


int G_Zeri(void)
{
	zMod = 0;

	FZeri=new ZerFile(QHeader.Prg_Default,ZER_ADDPATH,true);
	if(!FZeri->Open())                   
	{
		bipbip();
		W_Mess(NOZSCHFILE);
		delete FZeri;
		FZeri=NULL;
		return 0;
	}	

	Z_read_recs(0);
   	zerCurRow = 1;
   	zerCurRecord = 1;

   	PanelCompositionUI win( 0 );
   	win.Show();
   	win.Hide();

	delete FZeri;
	FZeri=NULL;

	return zMod;
}

int AppZMasterDTheta(float zxpos,float zypos,double &angle)
{
  float px1,px2,py1,py2;
  int ok_dtheta;
  char titolo[80];

  do
  {
    ok_dtheta=0;

    px1=zxpos;
    py1=zypos;

    snprintf( titolo, 80, MsgGetString(Msg_01608), 1 );
    if(!ManualTeaching(&px1,&py1, titolo))
    {
      break;
    }

    px2=px1;
    py2=py1;

    snprintf( titolo, 80, MsgGetString(Msg_01608), 2 );
    if(!ManualTeaching(&px2,&py2, titolo))
    {
      break;
    }

    float dx=px2-px1;
    float dy=py2-py1;

    if((fabs(dx)<0.02) && (fabs(dy)<=0.02))
    {
      bipbip();
      if(W_Deci(1,ZSCHMASTER_APP_POINTNEAR))
      {
        continue;
      }
      else
      {
        break;
      }
    }
    else
    {
      angle=atan(dy/dx);
    }

    ok_dtheta=0;

    //Se punti allineati orizzontalmente
	if((fabs(angle)>=0) && (fabs(angle)<=PI/16))
    {
		if(((angle>=0) && (angle<=PI/16)) || ((angle<0) && (angle>=(-PI/16))))
      {
        ok_dtheta=1;
      }
    }

	if((fabs(angle)>=(PI/2-PI/16)) && (fabs(angle)<=(PI/2+PI/16)))
    {
		if((angle>=(PI/2-PI/16)) && (angle<=(PI/2+PI/16)))
      {
		  angle=angle-PI/2;
        ok_dtheta=1;
      }
      else
      {
		  if((angle<=(-PI/2+PI/16)) && (angle>=-(PI/2+PI/16)))
        {
			angle=angle+PI/2;
          ok_dtheta=1;
        }
      }
    }

    if(!ok_dtheta)
    {
      bipbip();
      if(W_Deci(1,ZSCHMASTER_APP_NOALLIGNED))
      {
        continue;
      }
      else
      {
        break;
      }
    }
          
  } while(!ok_dtheta);

  return(ok_dtheta);
}

// Check se nel programma di montaggio ci sono 2 componenti coi flag FID1_MASK e FID2_MASK
// Se ci sono, ritorna le loro coordinate
bool FiducialCheck( float* X_fid1, float* Y_fid1, float* X_fid2, float* Y_fid2 )
{
	int nrec=TPrgNormal->Count();
	TabPrg tabrec;

	bool fid1_found = false, fid2_found = false;

	for(int i=0;i<nrec;i++)
	{
		TPrgNormal->Read(tabrec,i);

		if( (tabrec.status&FID1_MASK) == FID1_MASK )
		{
			fid1_found = true;
			*X_fid1 = tabrec.XMon;
			*Y_fid1 = tabrec.YMon;
		}

		if( (tabrec.status&FID2_MASK) == FID2_MASK )
		{
			fid2_found = true;
			*X_fid2 = tabrec.XMon;
			*Y_fid2 = tabrec.YMon;
		}
	}

	return fid1_found&fid2_found;
}

void ZSchMaster_App(void) //SMOD100504
{
	int is_open=0;
	
	if(!W_Deci(0,RIMASTER))
	{
		return;
	}
	
	if(FZeri==NULL)
	{
		FZeri=new ZerFile(QHeader.Prg_Default,ZER_ADDPATH,true);
		if(!FZeri->Open())
		{
			bipbip();
			W_Mess(NOZSCHFILE);
			delete FZeri;
			FZeri=NULL;
			return;
		}	
		is_open=1;
	}
	
	float xfid1, yfid1, xfid2, yfid2;
	bool fiducial_mode = FiducialCheck( &xfid1, &yfid1, &xfid2, &yfid2 );

	float zxpos,zypos;
	float zref_xpos,zref_ypos;
	
	struct Zeri zer;
	
	if(FZeri->GetNRecs()>1)
	{
		FZeri->Read(zer,0);
		zxpos=zer.Z_xzero;
		zypos=zer.Z_yzero;
	}
	else
	{
		zxpos=0;
		zypos=0;
	}

	Set_Tv(2);      // predispone per non richiudere immagine su video

	if(!ManualTeaching(&zxpos, &zypos, MsgGetString(Msg_00015)))
	{
		if(is_open)
		{
			delete FZeri;
			FZeri=NULL;
		}
		
		Set_Tv(3);
		return;
	}
	else if( fiducial_mode )
		W_Mess( FIDUCIAL1_SET );

	if( fiducial_mode )
	{
		zref_xpos=xfid2+zxpos;
		zref_ypos=yfid2+zypos;
	}
	else if(FZeri->GetNRecs()>1)
	{
		zref_xpos=zer.Z_xrif;
		zref_ypos=zer.Z_yrif;
	}
	else
	{
		zref_xpos=zxpos;
		zref_ypos=zypos;
	}

	int refok=1;

	if(!ManualTeaching(&zref_xpos, &zref_ypos, MsgGetString(Msg_00016)))
	{
		refok=0;
		zref_xpos=zxpos;
		zref_ypos=zypos;
	}
	else if( fiducial_mode )
		W_Mess( FIDUCIAL2_SET );
	
	if(refok)
	{
		//se zero e riferimento troppo vicini pone riferimento uguale allo zero e notifica errore
		if( (fabs(zref_xpos-zxpos)<(float)MINXAPP) && (fabs(zref_ypos-zypos)<(float)MINYAPP))
		{
			if( fabs(zref_xpos-zxpos) != 0.0 || fabs(zref_ypos-zypos) != 0.0 )
			{
				W_Mess( TOONEAR );
			}

			zref_xpos = zxpos;
			zref_ypos = zypos;
			refok = 0;
		}
	}

	int ok_dtheta=0;
	double angle;
	float scale = 1.0;
	
	if((zref_xpos!=zxpos) || (zref_ypos!=zypos))
	{
		// Se ci sono i fiduciali, si calcola la scala e l'angolo di rotazione del master...
		if( fiducial_mode )
		{
			float segment_theorical = sqrt( pow(xfid2-xfid1,2) + pow(yfid2-yfid1,2) );
			float segment_real = sqrt( pow(zref_xpos-zxpos,2) + pow(zref_ypos-zypos,2) );

			scale = segment_real / segment_theorical;

			angle = atan2(zref_ypos-zypos, zref_xpos-zxpos) - atan2(yfid2-yfid1, xfid2-xfid1);

			ok_dtheta = 1;
		}
		else
		{
			if(W_Deci(1,ZSCHMASTER_APP_DELTA))
			{
				ok_dtheta=AppZMasterDTheta(zxpos,zypos,angle);
			}
		}
	}

	Set_Tv(3);
	
	FZeri->Close();
	FZeri->Create();
	FZeri->Open();
	
	struct Zeri zdta;
	
	zdta.Z_scalefactor = scale;
	zdta.Z_scheda=0;
	zdta.Z_xzero=zxpos;
	zdta.Z_yzero=zypos;

	if(refok)
	{
		zdta.Z_rifangle=atan2(zref_ypos-zypos,zref_xpos-zxpos);
	
		if(ok_dtheta)
		{
			zdta.Z_rifangle-=angle;
		}    
	}
	else
	{
		zdta.Z_rifangle=0;
	}
  

	zdta.Z_note[0]='\0';
	zdta.Z_ass=0;
	zdta.Z_xrif=zref_xpos;
	zdta.Z_yrif=zref_ypos;
	if(!ok_dtheta)
	{
		zdta.Z_boardrot=0;
	}
	else
	{
		//rappresenta quanto il master e' ruotato rispetto alla posizione di
		//messa a squadra perfetta del circuito.
		
		zdta.Z_boardrot=angle;
	}

	FZeri->Write(zdta,0);
	
	zdta.Z_scheda=1;
	zdta.Z_ass=1;
	if(refok)
	{
		zdta.Z_rifangle=atan2(zref_ypos-zypos,zref_xpos-zxpos);
	}
	else
	{
		zdta.Z_rifangle=0;
	}
  

	FZeri->Write(zdta,1);
	
	if(is_open)
	{
		delete FZeri;
		FZeri=NULL;
	}

	zMod|=ZER_NEWMASTER;
	
	Z_read_recs(0);
	panelCompositionRefresh = true;
}
