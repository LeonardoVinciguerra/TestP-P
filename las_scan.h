//-----------------------------------------------------------------------------
// File: Sniper.h
//
// Desc: Header file for the Sniper class
//
//-----------------------------------------------------------------------------

#ifndef __LAS_SCAN_H
#define __LAS_SCAN_H

#include <vector>
#include "q_wind.h"
#include "c_window.h"
#include "tws_sniper.h"


//#define __SCANTEST_UPDIR

//------------------------SCAN TEST CONSTANTS-----------------------------
//coordinate di disegno
#define LASSCAN_NORMAL  0       //modalita normale
#define LASSCAN_VACUO   1       //con attivazione vuoto
#define LASSCAN_BIGWIN  2       //con finestra forzata a max
#define LASSCAN_ZEROCUR 4       //cursore a posizione z zero+delta mm

//messaggio errore laser timeout
#define LASERR_TIMEOUT      MsgGetString(Msg_01180)

//messaggio errore componente piu' grande della finestra laser
#define LASERR_WINDOW       MsgGetString(Msg_01181)

//messaggio errore in centraggio laser (con codice errore)
#define LASERR_CENTERING    MsgGetString(Msg_01182)


#define LASSCAN_DEFZRANGE 0     //range scantest=default
//------------------------------------------------------------------------


// Class LasScan per la gestione a video di una scansione laser
class LasScan
{
	protected:
		
		typedef struct
		{
			int z;
			float center;
			float width;			
			bool valid;
		} sample_t;
		
		float step_trasl;			// Costante passo/mm per la punta selezionata
		int zero;					// Delta in passi, tra la posizione attuale e lo zero sniper
		int scur;					// Delta in pixel video tra zero laser e cursore
		int startcur;				// Valore iniziale di scur
		int endpos_steps;		    // Passi motore da eseguire
		int stepmax;
		int xmin,xmax,ymin,ymax;	// Coordinate area grafica per disegno
		int yzlas;					// Coordinata video y, rappresentante lo zero sniper
		int punta;					// Indice della punta
		int mode;					// Modalita' di scansione (vedi costruttore)
		std::vector<sample_t> samples; //Buffer dei campioni acquisiti
		float k_step;				// Costante passi-motore/pixel video
		int curs1,curs2;			// Coordinate video y dei cursori.
		void *vidbuf,*vidbuf2;		// Buffer per il salvataggio della finestra grafica
		bool use_buf2;
		unsigned char* scanbuf;
		RectI _panel;
		
		CWindow* Q_SnpScan;			// Istanza finestra
		
		SniperModule *snp;			// Istanza classe di controllo modulo sniper
	
		void Restore_CursorLine(void);
		void Show_Cursor(void);
	public:
		LasScan(SniperModule *lasCla,int _punta,float _mm_max,float _mm_delta,float _mm_cur=0,int _mode=0);
		void Start(void);
		void Draw(void);
		int  Cursor(void);
		float Get_CurPos(void);
		float Get_DeltaCur(void);
		~LasScan(void);
};


// Esegue lo scantest
int ScanTest(int punta,int mode,float range=LASSCAN_DEFZRANGE,float curdelta=0,float curpos=0,float *delta=NULL);

// Esegue una scansione standard
void StdScan( int nozzle );

void GetSniperFramesShortCut(void);
void GetSniperImageDetailedShortCut(void);
void GetSniperImageShortCut(void);
void DoSniperScanTestShortCut(void);

#endif
