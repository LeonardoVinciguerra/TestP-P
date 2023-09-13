//-----------------------------------------------------------------------------
// File: Sniper.cpp
//
// Desc: Implementation of the las_scan class
//-----------------------------------------------------------------------------

#ifdef __SNIPER

#include "las_scan.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include "q_cost.h"
#include "sniper.h"
#include "q_snprt.h"

#include "filefn.h"
#include "msglist.h"
#include "q_gener.h"
#include "q_help.h"
#include "q_graph.h"
#include "q_grcol.h"
#include "q_oper.h"
#include "q_wind.h"
#include "q_tabe.h"
#include "q_fox.h"
#include "q_packages.h"

#include "q_init.h"

#include "keyutils.h"
#include "lnxdefs.h"
#include "getmac.h"

#include "gui_defs.h"
#include "gui_functions.h"
#include "gui_submenu.h"
#include "c_waitbox.h"

#include <mss.h>


// Struttura contenente i dati di configurazione della macchina
extern CfgHeader QHeader;
extern CfgParam  QParam;

extern SniperModule* Sniper1;
extern SniperModule* Sniper2;

GUI_SubMenu* Q_ShortCutsMenu;


#define WIN_SCAN_W      86
#define WIN_SCAN_H      26


/******** FUNZIONI MEMBRO CLASSE LasScan **************************************/

/*---------------------------------------------------------------------------------
Costruttore della classe di gestione del laser scan test LasScan
Parametri di ingresso:
   lasClas  : puntatore alla classe di gestione Laser da utilizzare.
   _punta   : numero della punta da utilizzare.
   _mm_max  : range in mm, su cui effettuare lo scantest,
              LASSCAN_DEFZRANGE=usa valore di default
   _mm_delta: delta in mm tra i due cursori
   _mm_cur  : offset (in mm) tra lo zero laser ed i cursori
   _mode    : Bitfield
              LASSCAN_NORMAL = modo normale
              LASSCAN_VACUO  = attiva il vuoto prima dello scantest e disattivalo dopo
              LASSCAN_BIGWIN = NON UTILIZZATO
              LASSCAN_ZEROCUR= poszione cursore=zero laser+curpos+posizione corrente della punta
Valori di ritorno:
   nessuno.
----------------------------------------------------------------------------------*/
LasScan::LasScan( SniperModule *lasCla,int _punta,float _mm_max,float _mm_delta,float _mm_cur,int _mode )
{
	punta=_punta;
	mode=_mode;
	snp=lasCla;

	// Definisce costante di conversione passo/mm per la punta specificata
	if ( punta==1 )
	{
		step_trasl=QHeader.Step_Trasl1;
	}
	else
	{
		step_trasl=QHeader.Step_Trasl2;
	}

	zero = 0;
	xmin = 0;
	xmax = 0;
	ymin = 0;
	ymax = 0;
	yzlas = 0;

	samples.clear();

	int zstart_steps = PuntaZPosStep(punta,0,RET_POS);	
	
	// Determina scanrange in passi
	if ( _mm_max==LASSCAN_DEFZRANGE )
	{
		#ifdef __SCANTEST_UPDIR
		endpos_steps = zstart_steps - int( SCANDEF_MM*step_trasl )/ 2;
		#else
		endpos_steps = zstart_steps + int( SCANDEF_MM*step_trasl )/ 2;
		#endif
		
		stepmax = int( SCANDEF_MM*step_trasl );
	}
	else
	{
		#ifdef __SCANTEST_UPDIR
		endpos_steps = zstart_steps - int ( _mm_max*step_trasl ) / 2 ;
		#else
		endpos_steps = zstart_steps + int ( _mm_max*step_trasl ) / 2 ;
		
		stepmax = int( _mm_max*step_trasl );
		
		#endif				
	}

	_panel = RectI( 8, 1, WIN_SCAN_W - 10, WIN_SCAN_H - 3 );

	// Numero di passi motore per pixel schermo su asse y
	k_step = ( ( float ) stepmax ) / (_panel.H*GUI_CharH()-2);

	// Offset della posizione cursore rispetto allo zero laser, espressa in pixel video
	int y_cur=int ( ( _mm_cur*step_trasl ) /k_step );

	// Numero di pixel corrispondenti a mm_delta
	int deltapix=int ( ( _mm_delta*step_trasl ) /k_step );

	curs1 = y_cur;
	curs2 = y_cur + deltapix;
	scur = y_cur; // Delta in pixel video tra zero laser e posizione cursore

	vidbuf = 0;
	vidbuf2 = 0;

	Q_SnpScan=NULL;
	scanbuf = NULL;
}


/*---------------------------------------------------------------------------------
Distruttore della classe LasScan
-------------------------------------------------------------------------------*/
LasScan::~LasScan()
{
	if( vidbuf )
	{
		GUI_FreeSurface( &vidbuf );
	}

	if( vidbuf2 )
	{
		GUI_FreeSurface( &vidbuf2 );
	}

	if( scanbuf )
	{
		delete [] scanbuf;
	}

	if( Q_SnpScan )
	{
		delete Q_SnpScan;
	}
}

//---------------------------------------------------------------------------------
// Disegna su schermo i risultati ottenuti dallo scantest
//---------------------------------------------------------------------------------
void LasScan::Draw()
{
	GUI_Freeze_Locker lock;

	bool first = true;

	// Crea finestra
	Q_SnpScan = new CWindow ( 0 );
	Q_SnpScan->SetStyle( WIN_STYLE_CENTERED );
	Q_SnpScan->SetClientAreaSize( WIN_SCAN_W, WIN_SCAN_H );
	Q_SnpScan->SetTitle( "Sniper Image Scan" );

	Q_SnpScan->Show();

	Q_SnpScan->DrawPanel( _panel );

	Q_SnpScan->DrawText( 1, WIN_SCAN_H-1, MsgGetString(Msg_00265) );

	int drawW = _panel.W * GUI_CharW() - 3;

	xmin = Q_SnpScan->GetX() + _panel.X * GUI_CharW() + 1;
	xmax = xmin + drawW;
	ymin = Q_SnpScan->GetY() + (_panel.Y + _panel.H) * GUI_CharH() - 2;
	ymax = Q_SnpScan->GetY() + _panel.Y * GUI_CharH() + 1;
	yzlas = (ymin+ymax)/2;

	curs1 += yzlas;
	curs2 += yzlas;

	// Disegna linea di zero laser
	GUI_HLine( xmin, xmax, yzlas, GUI_color( GR_WHITE ) );

	int prev_x1;
	int prev_x2;
	int prev_y;

	for ( std::vector<sample_t>::const_iterator i = samples.begin(); i != samples.end(); i++)
	{
		if(i->valid)
		{
			int x1 = xmin + ftoi ( ( ( (*i).center - (*i).width / 2 ) * drawW ) / ( ( SNIPER_RIGHT_USABLE - SNIPER_LEFT_USABLE + 1 ) * QHeader.sniper_kpix_um[punta-1] / 1000 ) );
			int x2 = xmin + ftoi ( ( ( (*i).center + (*i).width / 2 ) * drawW ) / ( ( SNIPER_RIGHT_USABLE - SNIPER_LEFT_USABLE + 1 ) * QHeader.sniper_kpix_um[punta-1] / 1000 ) );
			int y = yzlas + ftoi((*i).z / k_step);

			if(first)
			{
				GUI_PutPixel( x1, y, GUI_color( GR_WHITE ) );
				GUI_PutPixel( x2, y, GUI_color( GR_WHITE ) );
				first = 0;
			}
			else
			{
				GUI_Line( prev_x1, prev_y, x1, y, GUI_color( GR_WHITE ) );
				GUI_Line( prev_x2, prev_y, x2, y, GUI_color( GR_WHITE ) );
			}
			
			prev_x1 = x1;
			prev_x2 = x2;
			prev_y = y;
		}
	}
}


//---------------------------------------------------------------------------------
// Inizia lo scantest
//---------------------------------------------------------------------------------
void LasScan::Start()
{
	// Ritorna se disabilitato o se errore
	if ( Get_OnFile() || !snp->IsEnabled() )		
	{
		return;
	}

	// Cancella buffer keyboard (Loris)
	flushKeyboardBuffer();

	// Posizione attuale in z del terminale o dell'ugello se presente, espressa in passi.
	zero = int( PuntaZPosMm ( punta,0,RET_POS ) *step_trasl );

	// Se modo=vuoto on, attiva vuoto
	if ( mode & LASSCAN_VACUO )
	{
		Set_Vacuo ( punta,ON );
	}

	// Ritorna posizione corrente
	float oldpos = PuntaZPosMm ( punta,0,RET_POS );
	int oldpos_steps = PuntaZPosStep(punta,0,RET_POS);

	// Porta la punta a quota di misura iniziale
	#ifdef __SCANTEST_UPDIR
	PuntaZPosStep ( punta,stepmax/2,REL_MOVE );
	#else
	PuntaZPosStep ( punta,-stepmax/2,REL_MOVE );
	#endif	
	PuntaZPosWait ( punta );

	int deltaz = abs(endpos_steps - PuntaZPosStep(punta,0,RET_POS) + 1);

	CWaitBox waitBox( 0, 15, MsgGetString(Msg_00201), deltaz );
	waitBox.Show();

	while( 1 )
	{
		int z_current = PuntaZPosStep(punta,0,RET_POS);
		#ifdef __SCANTEST_UPDIR
		if(z_current < endpos_steps)
		#else
		if(z_current > endpos_steps)
		#endif
		{
			break;
		}
		
		if(Get_OnFile())
		{
			break;
		}

		int measure_status;
		int measure_angle;
		float measure_position;
		float measure_shadow;
		snp->MeasureOnce( measure_status, measure_angle, measure_position, measure_shadow );


		sample_t sample;
		
		sample.z = oldpos_steps - z_current;
		
		if ( measure_status == STATUS_EMPTY )
		{
			sample.valid = false;
		}
		else
		{
			// Larghezza in mm
			sample.width = measure_shadow / 1000.f;
			// Posizione in mm
			sample.center = measure_position / 1000.f;
			sample.valid = true;
		}

		samples.push_back(sample);
		
		waitBox.Increment();
		
		// Movimento relativo della punta di un passo
		#ifdef __SCANTEST_UPDIR
		PuntaZPosStep ( punta,-1,REL_MOVE );
		#else
		PuntaZPosStep ( punta,+1,REL_MOVE );
		#endif
		PuntaZPosWait ( punta );		
	}
	
	// Riporta punta alla posizione z che aveva prima del test
	PuntaZPosMm ( punta,oldpos );
	PuntaZPosWait ( punta );

	// Se mode scantest con vuoto, disattiva vuoto
	if ( mode & LASSCAN_VACUO )
	{
		Set_Vacuo ( punta,OFF );
	}

	waitBox.Hide();
}

//---------------------------------------------------------------------------------
// Elimina cursore da posizione corrente, ripristinando contenuto al di sotto
//---------------------------------------------------------------------------------
void LasScan::Restore_CursorLine()
{
	GUI_Freeze_Locker lock;

	GUI_DrawSurface( PointI(xmin, curs1 ), vidbuf );
	GUI_FreeSurface( &vidbuf );

	if( vidbuf2 )
	{
		GUI_DrawSurface( PointI( xmin, curs2 ), vidbuf2 );
		GUI_FreeSurface( &vidbuf2 );
	}

	GUI_FillRect( RectI( Q_SnpScan->GetX() + 4, curs1-10, 6*GUI_CharW(GUI_XSmallFont), 2*GUI_CharH(GUI_XSmallFont)), GUI_color( WIN_COL_CLIENTAREA ) );
}

//---------------------------------------------------------------------------------
// Mostra cursore
//---------------------------------------------------------------------------------
void LasScan::Show_Cursor()
{
	GUI_Freeze_Locker lock;

	char buf[15];

	vidbuf = GUI_SaveScreen( RectI( xmin, curs1, xmax-xmin+1, 1 ) );
	GUI_HLine( xmin, xmax, curs1, GUI_color( GR_LIGHTRED ) );

	if( use_buf2 && curs2 < ymin )
	{
		vidbuf2 = GUI_SaveScreen( RectI( xmin, curs2, xmax-xmin+1, 1 ) );
		GUI_HLine( xmin, xmax, curs2, GUI_color( GR_LIGHTRED ) );
	}

	float cy = ( scur*k_step ) /step_trasl;
	if ( cy<0 )
	{
		snprintf( buf, sizeof(buf),"@ = %6.3f",cy );
	}
	else
	{
		snprintf( buf, sizeof(buf),"@ = +%5.3f",cy );
	}

	GUI_DrawText( Q_SnpScan->GetX() + 4, curs1-10, buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( 0, 0, 0 ) );

	float d = ( ( scur-startcur ) *k_step ) /step_trasl;

	if ( d<0 )
	{
		snprintf( buf, sizeof(buf),"D = %6.3f",d );
	}
	else
	{
		snprintf( buf, sizeof(buf),"D = +%5.3f",d );
	}

	GUI_DrawText( Q_SnpScan->GetX() + 4, curs1-10+GUI_CharH(GUI_XSmallFont), buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( 0, 0, 0 ) );
}


/*---------------------------------------------------------------------------------
Gestione tastiera e cursore
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   0 se premuto ESC, 1 altrimenti.
------------------------------------------------------------------------------*/
int LasScan::Cursor( void )
{
	if( !Q_SnpScan )
	{
		return 0;
	}

	int c,i;

	// Se modo=cursore a posizione z di zero
	if ( mode & LASSCAN_ZEROCUR )
	{
		// Somma a cursori la posizione z di zero
		curs1+=zero;
		curs2+=zero;
		scur+=zero;
	}

	startcur = scur;

	// Mostra cursori
	GUI_HLine( xmin, xmax, curs1, GUI_color( GR_LIGHTGREEN ) );
	if( curs2 < ymin )
	{
		GUI_HLine( xmin, xmax, curs2, GUI_color( GR_LIGHTGREEN ) );
	}

	do
	{
		c = Handle();
	}
	while( ( c != K_ESC ) && ( c != K_F2 ) );

	// Se ESC abbandono
	if ( c == K_ESC )
	{
		return ( 0 );
	}

	// premuto F2

	vidbuf = 0;
	vidbuf2 = 0;

	use_buf2 = ( curs1 != curs2 ) ? true : false;

	Show_Cursor();

	i=0; // Spostamento in passi rispetto alla posizione iniziale del cursore

	int deltaY = abs((ymax-ymin)/2);

	do
	{
		c = Handle();
		switch ( c )
		{
			case K_UP:
				// Se indice posizione maggiore di massima
				if( i <= -deltaY )
				{
					break;
				}
				GUI_Freeze();
				Restore_CursorLine();

				// Decrementa posizione cursore
				curs1--;
				curs2--;
				scur--;
				i--;
				Show_Cursor();
				GUI_Thaw();
				break;

			case K_DOWN:
				if( i >= deltaY )
				{
					break;
				}
				GUI_Freeze();
				Restore_CursorLine();

				// Incrementa posizione cursore
				i++;
				curs1++;
				curs2++;
				scur++;
				Show_Cursor();
				GUI_Thaw();
				break;
		}
	}
	while( ( c!=K_ESC ) && ( c!=K_ENTER ) );

	return ( c == K_ESC ) ? 0 : 1;
}


/*---------------------------------------------------------------------------------
Ritorna posizione del cursore, rispetto allo zero laser.
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   Posizione del cursore, rispetto allo zero laser.
------------------------------------------------------------------------------*/
float LasScan::Get_CurPos ( void )
{
	return ( ( scur*k_step ) /step_trasl );
}


/*---------------------------------------------------------------------------------
Ritorna spostamento in mm effettuato dal cursore
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   Spostamento in mm effettuato dal cursore
------------------------------------------------------------------------------*/
float LasScan::Get_DeltaCur()
{
	return ( ( ( scur-startcur ) *k_step ) /step_trasl );
}

/*********************************************************************************/

/*---------------------------------------------------------------------------------
Esegue scantest della punta
Parametri di ingresso:
   punta : numero della punta su cui fare il test
   mode,range,curdelta,curpos : vedi costruttore classe LasScan
   delta : se indicato, puntatore alla variabile in cui ritornare
           i mm di cui il cursore si e' spostato.
Valori di ritorno:
   se delta!=NULL, vi ritorna i mm di cui il cursore si e' spostato
   Ritorna: 1 se si e' confermato con ENTER, 0 altrimenti.
------------------------------------------------------------------------------*/
int ScanTest( int punta,int mode,float range,float curdelta,float curpos,float *delta )
{
	int retval=0;

	LasScan* scan = new LasScan( (punta == 1) ? Sniper1 : Sniper2, punta, range, curdelta, curpos, mode );

	scan->Start();
	scan->Draw();

	if ( scan->Cursor() )
		retval=1;

	if ( delta!=NULL )
		*delta=scan->Get_DeltaCur();

	delete scan;

	return ( retval );
}


/*---------------------------------------------------------------------------------
Esegue una scansione standard.
Parametri di ingresso:
   punta : numero della punta su cui fare il test
Valori di ritorno:
   nessuno.
------------------------------------------------------------------------------*/
void StdScan( int nozzle )
{
	ScanTest( nozzle, LASSCAN_NORMAL );
}

int GetSniper1Frames()
{
	Sniper_PlotFrames( 1 );
	return 1;
}

int GetSniper2Frames()
{
	Sniper_PlotFrames( 2 );
	return 1;
}

void GetSniperFramesShortCut()
{
	if ( Q_ShortCutsMenu!=NULL )
	{
		bipbip();
		return;
	}

	Q_ShortCutsMenu = new GUI_SubMenu ( 10, 15 );
	Q_ShortCutsMenu->Add ( MsgGetString(Msg_00042), 0, 0, NULL, GetSniper1Frames );
	Q_ShortCutsMenu->Add ( MsgGetString(Msg_00043), 0, 0, NULL, GetSniper2Frames );

	Q_ShortCutsMenu->Show();

	delete Q_ShortCutsMenu;
	Q_ShortCutsMenu = NULL;
}

int GetSniper1ImageDetailed()
{
	Sniper_ImageTestDetailed( 1 );
	return 1;
}

int GetSniper2ImageDetailed()
{
	Sniper_ImageTestDetailed( 2 );
	return 1;
}

void GetSniperImageDetailedShortCut()
{
	if ( Q_ShortCutsMenu!=NULL )
	{
		bipbip();
		return;
	}

	int cam = pauseLiveVideo();

	Q_ShortCutsMenu = new GUI_SubMenu ( 10, 15 );
	Q_ShortCutsMenu->Add( MsgGetString(Msg_05015), 0, 0, NULL, GetSniper1ImageDetailed );
	Q_ShortCutsMenu->Add( MsgGetString(Msg_05016), 0, 0, NULL, GetSniper2ImageDetailed );

	Q_ShortCutsMenu->Show();

	delete Q_ShortCutsMenu;
	Q_ShortCutsMenu = NULL;

	playLiveVideo( cam );
}

int GetSniper1Image()
{
	Sniper_ImageTest( 1 );
	return 1;
}

int GetSniper2Image()
{
	Sniper_ImageTest( 2 );
	return 1;
}

void GetSniperImageShortCut()
{
	if( Q_ShortCutsMenu )
	{
		bipbip();
		return;
	}

	int cam = pauseLiveVideo();

	Q_ShortCutsMenu = new GUI_SubMenu( 10, 15 );
	Q_ShortCutsMenu->Add( MsgGetString(Msg_05015), 0, 0, NULL, GetSniper1Image );
	Q_ShortCutsMenu->Add( MsgGetString(Msg_05016), 0, 0, NULL, GetSniper2Image );

	Q_ShortCutsMenu->Show();

	delete Q_ShortCutsMenu;
	Q_ShortCutsMenu = NULL;

	playLiveVideo( cam );
}

int DoSniper1ScanTest()
{
	if(!isComponentOnNozzleTooBig(1))
	{
		StdScan( 1 );
	}
	else
	{
		W_Mess( MsgGetString(Msg_05074) );
	}
	return 1;
}

int DoSniper2ScanTest()
{
	if(!isComponentOnNozzleTooBig(2))
	{
		StdScan( 2 );
	}
	else
	{
		W_Mess( MsgGetString(Msg_05074) );
	}
	return 1;
}

void DoSniperScanTestShortCut ( void )
{
	if ( Q_ShortCutsMenu!=NULL )
	{
		bipbip();
		return;
	}

	int cam = pauseLiveVideo();

	Q_ShortCutsMenu = new GUI_SubMenu ( 10, 15 );
	Q_ShortCutsMenu->Add( MsgGetString(Msg_00042), 0, 0, NULL,DoSniper1ScanTest );
	Q_ShortCutsMenu->Add( MsgGetString(Msg_00043), 0, 0, NULL,DoSniper2ScanTest );

	Q_ShortCutsMenu->Show();

	delete Q_ShortCutsMenu;
	Q_ShortCutsMenu = NULL;

	playLiveVideo( cam );
}

#endif //__SNIPER
