//______________________________________________
//
//      Q_VISION.CPP
//
// Gestione della visione per la macchina QUADRA.
//	Massimone 99.10.21
//______________________________________________

#include <time.h>

#include <unistd.h>

#include "filefn.h"
#include "q_cost.h"
#include "q_opert.h"
#include "q_msg.h"
#include "msglist.h"
#include "q_help.h"
#include "q_oper.h"
#include "tv.h"
#include "q_vision.h"
#include "bitmap.h"
#include "q_prog.h"
#include "q_gener.h"
#include "q_init.h"
#include "q_mapping.h"

#include "keyutils.h"
#include "lnxdefs.h"

#include "c_win_imgpar.h"

//TEST TIMINGS
#include "q_assem.h"

#include <mss.h>


extern struct vis_data Vision;
extern struct CfgHeader QHeader;
extern struct CfgParam  QParam;
extern struct cur_data CurDat;

extern bool use_orthoXY_correction;
extern bool use_encScale_correction;
extern bool use_mapping_correction;

#define ATLANTE_MAX_X        340
#define ATLANTE_MAX_Y        340

#define VIS_DBG_ONLY_ERROR   1 // mostra solo gli errori
#define VIS_DBG_RESULT       2 // mostra sempre il risultato finale
#define VIS_DBG_ALL_ITER     4 // mostra tutte le iterazioni




//---------------------------------------------------------------------------------
// Mostra informazioni debug visione
//---------------------------------------------------------------------------------
#define VIDEO_CALLBACK_Y      29

float vdCorrelation = 0.f;
float vdThreshold = 0.f;
int vdErrorX = 0;
int vdErrorY = 0;
float vdDiameter = 0.f;

void displayInfoVisionDebug( void* parentWin )
{
	GUI_Freeze_Locker lock;

	CWindow* parent = (CWindow*)parentWin;

	C_Combo c1( 10, VIDEO_CALLBACK_Y + 1, "Correlation :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 3 );
	c1.SetTxt( vdCorrelation );
	c1.Show( parent->GetX(), parent->GetY() );

	C_Combo c2( 45, VIDEO_CALLBACK_Y + 1, "Threshold   :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 3 );
	c2.SetTxt( vdThreshold );
	c2.Show( parent->GetX(), parent->GetY() );

	C_Combo c3( 10, VIDEO_CALLBACK_Y + 2, "Error X pix :", 8, CELL_TYPE_SINT );
	c3.SetTxt( vdErrorX );
	c3.Show( parent->GetX(), parent->GetY() );

	C_Combo c4( 45, VIDEO_CALLBACK_Y + 2, "Error Y pix :", 8, CELL_TYPE_SINT );
	c4.SetTxt( vdErrorY );
	c4.Show( parent->GetX(), parent->GetY() );
}


//-----------------------------------------------------------------------------
// Visualizza il risultato del matching con correlazione
//-----------------------------------------------------------------------------
void Image_showDebug( bitmap* img, bitmap* pattern, float corr, float threshold, int errX, int errY )
{
	vdCorrelation = corr;
	vdThreshold = threshold;
	vdErrorX = errX;
	vdErrorY = errY;

	Set_Tv_Title( "Vision debug" );
	Set_Tv_FromImage( 1, true, displayInfoVisionDebug );

	if( img )
	{
		img->showFrame( GetVideoCenterX(), GetVideoCenterY() );
	}

	if( corr >= threshold && pattern )
	{
		pattern->show( GetVideoCenterX()+vdErrorX, GetVideoCenterY()-vdErrorY );
	}

	//svuota buffer
	flushKeyboardBuffer();
	// attende pressione ESC
	while( !Esc_Press() );

	Set_Tv_FromImage( 0 );
}

void displayInfoVisionDebugVectorial( void* parentWin )
{
	GUI_Freeze_Locker lock;

	CWindow* parent = (CWindow*)parentWin;

	C_Combo c1( 10, VIDEO_CALLBACK_Y + 1, "Diameter    :", 8, CELL_TYPE_SDEC, CELL_STYLE_DEFAULT, 2 );
	c1.SetTxt( vdDiameter );
	c1.Show( parent->GetX(), parent->GetY() );

	C_Combo c2( 10, VIDEO_CALLBACK_Y + 2, "Error X pix :", 8, CELL_TYPE_SINT );
	c2.SetTxt( vdErrorX );
	c2.Show( parent->GetX(), parent->GetY() );

	C_Combo c3( 45, VIDEO_CALLBACK_Y + 2, "Error Y pix :", 8, CELL_TYPE_SINT );
	c3.SetTxt( vdErrorY );
	c3.Show( parent->GetX(), parent->GetY() );
}


//-----------------------------------------------------------------------------
// Visualizza il risultato del matching vettoriale
//-----------------------------------------------------------------------------
void Image_showDebugVectorial( bitmap* img, float diameter, int errX, int errY )
{
	vdDiameter = diameter;
	vdErrorX = errX;
	vdErrorY = errY;

	Set_Tv_Title( "Vision debug" );
	Set_Tv_FromImage( 1, true, displayInfoVisionDebugVectorial );

	img->showFrame( GetVideoCenterX(), GetVideoCenterY() );
	img->showDebug( GetVideoCenterX(), GetVideoCenterY() );

	//svuota buffer
	flushKeyboardBuffer();
	// attende pressione ESC
	while( !Esc_Press() );

	Set_Tv_FromImage( 0 );
}

//--------------------------------------------------------------------------
// Cattura e salva su file un'immagine in base al tipo passato.
// mode=0 salva senza selezionare il pattern (rimane quello precedente)
// mode=1 salva selezionando il pattern, settando dimensione atlante e
// salvando il valore corrente della luminosita' e del contrasto.
//--------------------------------------------------------------------------
bool ImageCaptureSave( int imageType, int mode, int camera, int imageNum )
{
	char filename[MAXNPATH+1];
	int error=1;
	int sizex, sizey;

	int cam = pauseLiveVideo();

	// led camera on
	if( camera == CAMERA_HEAD )
	{
		Set_HeadCameraLight( 1 );
	}

	set_currentcam( camera );

	// cattura l'immagine dalla VGA
	bitmap captured( BRD_IMG_MAXX, BRD_IMG_MAXY, getFrameWidth()/2 ,getFrameHeight()/2 );

	// seleziona il nome del file immagine in base al tipo immagine
	SetImageName( filename, imageType, IMAGE, imageNum );
	
	error = !captured.save(filename); // salva immagine catturata

	// seleziona il nome del file dati immagine in base al tipo immagine
	SetImageName( filename, imageType, DATA, imageNum );

	img_data imgData;
	ImgDataLoad( filename, &imgData );

	if( mode == YESCHOOSE )
	{
		// immagine da selezionare
		
		// elabora immagine creando il pattern
		// carica dati immagine o crea default.
		Image_Elabora( imageType, imageNum );

		sizex=imgData.pattern_x;
		sizey=imgData.pattern_y;
	
		ChoosePattern( GetVideoCenterX(), GetVideoCenterY(), sizex, sizey );
	
		// salva la nuova diemensione del pattern
		imgData.pattern_x=sizex;
		imgData.pattern_y=sizey;

		// salva luminosita' e contrasto correnti
		imgData.bright = GetImageBright() / 655;
		imgData.contrast = GetImageContrast() / 655;

		if( imageType == ZEROMACHIMG || imageType == EXTCAM_NOZ_IMG || camera == CAMERA_EXT )
		{
			// si usa la massima dimensione dell'atlante
			imgData.atlante_x = ATLANTE_MAX_X;
			imgData.atlante_y = ATLANTE_MAX_Y;
		}
		else
		{
			// altrimenti si calcola atlante in base a errore max parametri visione
			imgData.atlante_x=sizex+2*(int)(Vision.match_err/Vision.mmpix_x);
			imgData.atlante_y=sizey+2*(int)(Vision.match_err/Vision.mmpix_y);
		}
	}
	else
	{
		// immagine senza selezione
		if( imageType == HEADCAM_SCALE_IMG || imageType == EXTCAM_SCALE_IMG )
		{
			// salva la nuova diemensione del pattern
			imgData.pattern_x = PATTERN_MAXX;
			imgData.pattern_y = PATTERN_MAXY;

			// salva luminosita' e contrasto correnti
			imgData.bright = GetImageBright() / 655;
			imgData.contrast = GetImageContrast() / 655;

			// usa la massima dimensione dell'atlante
			imgData.atlante_x = ATLANTE_MAX_X;
			imgData.atlante_y = ATLANTE_MAX_Y;
		}
	}

	// se immagine su camera esterna
	if( imageType == EXTCAM_NOZ_IMG || imageType == EXTCAM_SCALE_IMG )
	{
		// salva dati camera e illuminazione
		imgData.filter_p1 = GetExtCam_Light();
		imgData.filter_p2 = GetExtCam_Gain();
		imgData.filter_p3 = GetExtCam_Shutter();
	}

	// salva dati immagine
	ImgDataSave( filename, &imgData );

	Image_Elabora( imageType, imageNum );

	// led camera off
	if( camera == CAMERA_HEAD )
	{
		Set_HeadCameraLight( 0 );
	}

	playLiveVideo( cam );
	
	return error;
}


//-----------------------------------------------------------------------------
// Esegue image matching
//   input:
//      x_pos, y_pos: coordinate del punto nell'intorno del quale fare la ricerca
//      imageType   : tTipo di immagine
//      imageNum    : numero immagine
//-----------------------------------------------------------------------------
#define MATCH_ACCEPTABLE_AREA_MM		3

bool Image_match( float* x_pos, float* y_pos, int imageType, int matchType, int imageNum )
{
	double maxcorr;
	int results[2];
	float prev_xpos=*x_pos;
	float prev_ypos=*y_pos;
	char NameFile[MAXNPATH];
	bool imageFound = true;
	bitmap* captured;
	bitmap* pattern;

	if( matchType == MATCH_CORRELATION )
	{
		// LOAD PATTERN DA FILE
		SetImageName( NameFile, imageType, ELAB, imageNum );
		if( access(NameFile,0) != 0 )
		{
			Wait_PuntaXY();
			return false;
		}
		pattern = new bitmap(NameFile);
	}

	// LOAD IMAGE DATA DA FILE
	SetImageName( NameFile, imageType, DATA, imageNum );
	if( access(NameFile,0) != 0 )
	{
		Wait_PuntaXY();
		return false;
	}
	img_data imgData;
	ImgDataLoad( NameFile, &imgData );

	// La velocita' del primo punto viene decisa dal chiamante
	if( !NozzleXYMove( *x_pos, *y_pos ) )
	{
		return false;
	}

	Set_HeadCameraLight( 1 );
	set_currentcam( CAMERA_HEAD );

	SetImgBrightCont(imgData.bright*655,imgData.contrast*655);

	// parametri locali
	int atl_xdim;
	int atl_ydim;
	int raggio = (imgData.filter_type == 0) ? 0 : imgData.filter_p1;
	int numiteraz = imgData.match_iter;
	double threshold = imgData.match_thr;
	float mmpix = (Vision.mmpix_x + Vision.mmpix_y) / 2.f;
	float posX, posY;
	unsigned short diameter = ( (imgData.vect_diameter-imgData.vect_tolerance/2.0) / mmpix );

	if( matchType == MATCH_CORRELATION )
	{
		atl_xdim = imgData.atlante_x;
		atl_ydim = imgData.atlante_y;
	}
	else
	{
		atl_xdim = imgData.vect_atlante_x;
		atl_ydim = imgData.vect_atlante_y;
	}

	//ATTENDE TESTA IN POSIZIONE
	Wait_PuntaXY();
	delay(Vision.wait_time);

	int itera=1;
	while( itera && (itera<=numiteraz) )
	{
		if( matchType == MATCH_CORRELATION )
		{
			//COSTRUTTORE CON CAPTURING DA VGA (ATLANTE)
			captured = new bitmap( atl_xdim+4*raggio, atl_ydim+4*raggio, getFrameWidth()/2 ,getFrameHeight()/2 );

			//MATCHING
			maxcorr = captured->matching( pattern );

			results[0] = captured->hsx();
			results[1] = captured->hsy();
			// SOGLIA DI DECISIONE
			imageFound = (maxcorr < threshold) ? false : true;
		}
		else
		{
			captured = new bitmap( atl_xdim, atl_ydim, getFrameWidth()/2 ,getFrameHeight()/2 );
			imageFound = captured->findCircle( posX, posY, diameter, (imgData.vect_tolerance / mmpix), imgData.vect_smooth, imgData.vect_edge, imgData.vect_accumulator, 0x0001 );
			results[0] = cvRound(posX);
			results[1] = cvRound(posY);
		}

		int DeltaXpix = 0;
		int DeltaYpix = 0;
		if( imageFound )
		{
			DeltaXpix = results[0]-(atl_xdim/2);
			DeltaYpix = results[1]-(atl_ydim/2);
		}

		// show debug
		if( ((Vision.debug & VIS_DBG_ONLY_ERROR) && (!imageFound)) || (Vision.debug & VIS_DBG_ALL_ITER) )
		{
			if( matchType == MATCH_CORRELATION )
				Image_showDebug( captured, pattern, maxcorr, threshold, DeltaXpix, DeltaYpix );
			else
				Image_showDebugVectorial( captured, diameter*mmpix, DeltaXpix, DeltaYpix );
		}

		// se immagine non trovata esci
		if( !imageFound )
		{
			break;
		}


		if( imageType != HEADCAM_SCALE_IMG )
		{
			float deltaXmm = DeltaXpix * Vision.mmpix_x;
			float deltaYmm = DeltaYpix * Vision.mmpix_y;

			if( fabs(deltaXmm) <= MATCH_ACCEPTABLE_AREA_MM && fabs(deltaYmm) <= MATCH_ACCEPTABLE_AREA_MM )
			{
				// condizione di fine ciclo
				itera = 0;
			}
			else
			{
				itera++;
			}

			if( fabs(deltaXmm) >= Vision.match_err || fabs(deltaYmm) >= Vision.match_err )
			{
				maxcorr = 0;
				imageFound = false;
				break;
			}

			*x_pos += deltaXmm;
			*y_pos += deltaYmm;

			if( numiteraz > 1 )
			{
				SetNozzleXYSpeed_Index( Vision.matchSpeedIndex );
				if( !NozzleXYMove(*x_pos,*y_pos,AUTOAPP_NOZSECURITY) )
				{
					imageFound = false;
					break;
				}
				Wait_PuntaXY();
				delay(Vision.image_time);
			}
		}
		else
		{
			// in caso di calibrazione scala non occorre ripetere il ciclo
			// si ritorna il delta in pixels
			*x_pos = DeltaXpix;
			*y_pos = DeltaYpix;

			itera = 0;
		}
	}

	// show debug
	if( imageFound && (Vision.debug & VIS_DBG_RESULT) )
	{
		int errX = results[0]-(atl_xdim/2);
		int errY = results[1]-(atl_ydim/2);

		if( matchType == MATCH_CORRELATION )
			Image_showDebug( 0, pattern, maxcorr, threshold, errX, errY );
		else
			Image_showDebugVectorial( captured, diameter*mmpix, errX, errY );
	}

	if( !imageFound )
	{
		*x_pos = prev_xpos;
		*y_pos = prev_ypos;
	}

	Wait_PuntaXY();
	
	// led camera off
	Set_HeadCameraLight( 0 );

	set_currentcam( NO_CAMERA );

	delete captured;
	if( matchType == MATCH_CORRELATION )
		delete pattern;

	return imageFound;
}


bool Image_match_package( int punta, float* x_pos, float* y_pos, int packCode, struct PackVisData pvdat, char* libname, int ImageType, int checkMaxError )
{
	ASSEMBLY_PROFILER_MEASURE("package matching starts");

	char NameFile[MAXNPATH];
	int results[2];

	// LOAD PATTERN DA FILE
	SetImageName( NameFile, ImageType, ELAB, packCode, libname );
	if( access(NameFile,0) != 0 )
	{
		Wait_PuntaXY();
		return false;
	}

	bitmap pattern( NameFile );

	SetImageName( NameFile, ImageType, DATA, packCode, libname );
	if( access(NameFile,0) != 0 )
	{
		Wait_PuntaXY();
		return false;
	}

	img_data imgData;
	ImgDataLoad( NameFile, &imgData );
	ASSEMBLY_PROFILER_MEASURE("load pattern image");

	//TODO: rivedere perche' moltiplico per 655
	SetImgBrightCont( imgData.bright*655, imgData.contrast*655 );
	ASSEMBLY_PROFILER_MEASURE("set brightess and contrast");

	int atl_xdim = imgData.atlante_x;
	int atl_ydim = imgData.atlante_y;
	double threshold = pvdat.match_thr;

	//TODO: vedere differenza tra Vision.image_time e Vision.wait_time
	delay( Vision.image_time );

	//COSTRUTTORE CON CAPTURING (ATLANTE)
	bitmap captured( atl_xdim, atl_ydim, getFrameWidth()/2 ,getFrameHeight()/2 );
	ASSEMBLY_PROFILER_MEASURE("grab");

	//MATCHING
	double maxcorr = captured.matching( &pattern );
	ASSEMBLY_PROFILER_MEASURE("matching");

	int DeltaXpix = 0;
	int DeltaYpix = 0;
	bool imageFound = true;
	if( !QParam.DemoMode )
	{
		// SOGLIA DECISIONE
		if( maxcorr < threshold )
		{
			imageFound = false;
		}
		else
		{
			results[0] = captured.hsx();
			results[1] = captured.hsy();
			DeltaXpix = results[0]-(atl_xdim/2);
			DeltaYpix = results[1]-(atl_ydim/2);

			if( checkMaxError )
			{
				if( fabs(DeltaXpix * QParam.AuxCam_Scale_x[punta-1]) >= Vision.match_err ||
					fabs(DeltaYpix * QParam.AuxCam_Scale_y[punta-1]) >= Vision.match_err )
				{
					imageFound = false;
				}
			}
		}

		if( imageFound )
		{
			*x_pos -= DeltaXpix * QParam.AuxCam_Scale_x[punta-1];
			*y_pos -= DeltaYpix * QParam.AuxCam_Scale_y[punta-1];
		}
	}

	// debug
	if( ((Vision.debug & VIS_DBG_ONLY_ERROR) && (!imageFound)) || (Vision.debug & VIS_DBG_ALL_ITER) || (Vision.debug & VIS_DBG_RESULT) )
	{
		Image_showDebug( &captured, &pattern, maxcorr, threshold, DeltaXpix, DeltaYpix );
	}

	ASSEMBLY_PROFILER_MEASURE("package matching ends");
	
	return imageFound;
}


//---------------------------------------------------------------------------------
// Esegue image match sulla telecamera esterna
//---------------------------------------------------------------------------------
bool ImageMatch_ExtCam( float* x_pos, float* y_pos, int imageType, int nozzle )
{
	bool imageFound = true;

	double maxcorr;
	int results[2];

	float prev_xpos = *x_pos;
	float prev_ypos = *y_pos;

	char NameFile[MAXNPATH];

	// LOAD PATTERN DA FILE
	SetImageName( NameFile, imageType, ELAB );
	if( access(NameFile,0) != 0 )
	{
		Wait_PuntaXY();
		return false;
	}
	bitmap pattern(NameFile);

	// LOAD IMAGE DATA DA FILE
	SetImageName( NameFile, imageType, DATA );
	if( access(NameFile,0) != 0 )
	{
		Wait_PuntaXY();
		return false;
	}
	img_data imgData;
	ImgDataLoad( NameFile, &imgData );

	// set brightess and contrast
	SetImgBrightCont(imgData.bright*655,imgData.contrast*655);

	// setup ext cam
	SetExtCam_Light( imgData.filter_p1 );
	SetExtCam_Gain( imgData.filter_p2 );
	SetExtCam_Shutter( imgData.filter_p3 );

	set_currentcam( CAMERA_EXT );

	Wait_PuntaXY();
	//TODO: controllare se va bene
	delay( Vision.wait_time );


	int atl_xdim = imgData.atlante_x;
	int atl_ydim = imgData.atlante_y;
	int numiteraz = imgData.match_iter;
	double threshold = imgData.match_thr;

	int itera=1;
	while( itera && (itera<=numiteraz) )
	{
		//COSTRUTTORE CON CAPTURING DA VGA (ATLANTE)
		bitmap captured( atl_xdim, atl_ydim, getFrameWidth()/2 ,getFrameHeight()/2 );

		//MATCHING
		maxcorr = captured.matching( &pattern );

		results[0] = captured.hsx();
		results[1] = captured.hsy();
		//SOGLIA DI DECISIONE
		imageFound = (maxcorr < threshold) ? false : true;

		int deltaXpix = 0;
		int deltaYpix = 0;
		if( imageFound )
		{
			deltaXpix = results[0]-(atl_xdim/2);
			deltaYpix = results[1]-(atl_ydim/2);
		}

		// show debug
		if( ((Vision.debug & VIS_DBG_ONLY_ERROR) && (!imageFound)) || (Vision.debug & VIS_DBG_ALL_ITER) )
		{
			Image_showDebug( &captured, &pattern, maxcorr, threshold, deltaXpix, deltaYpix );
		}

		// se soglia bassa esci
		if( !imageFound )
		{
			break;
		}


		if( imageType != EXTCAM_SCALE_IMG )
		{
			float deltaXmm = deltaXpix * QParam.AuxCam_Scale_x[nozzle-1];
			float deltaYmm = deltaYpix * QParam.AuxCam_Scale_y[nozzle-1];

			if( fabs(deltaXmm) <= MATCH_ACCEPTABLE_AREA_MM && fabs(deltaYmm) <= MATCH_ACCEPTABLE_AREA_MM )
			{
				// condizione di fine ciclo
				itera = 0;
			}
			else
			{
				itera++;
			}

			if( fabs(deltaXmm) >= Vision.match_err || fabs(deltaYmm) >= Vision.match_err )
			{
				maxcorr = 0;
				imageFound = false;
				break;
			}

			*x_pos -= deltaXmm;
			*y_pos -= deltaYmm;

			if( numiteraz > 1 )
			{
				SetNozzleXYSpeed_Index( Vision.matchSpeedIndex );
				if( !NozzleXYMove(*x_pos,*y_pos,AUTOAPP_NOZSECURITY) )
				{
					imageFound = false;
					break;
				}
				Wait_PuntaXY();
				delay(Vision.image_time);
			}
		}
		else
		{
			// in caso di calibrazione scala non occorre ripetere il ciclo
			// si ritorna il delta in pixels
			*x_pos = deltaXpix;
			*y_pos = deltaYpix;

			itera = 0;
		}
	}

	// show debug
	if( imageFound && (Vision.debug & VIS_DBG_RESULT) )
	{
		int errX = results[0]-(atl_xdim/2);
		int errY = results[1]-(atl_ydim/2);

		Image_showDebug( 0, &pattern, maxcorr, threshold, errX, errY );
	}

	if( !imageFound )
	{
		*x_pos = prev_xpos;
		*y_pos = prev_ypos;
	}

	Wait_PuntaXY();

	SetExtCam_Light( 0 );
	set_currentcam( NO_CAMERA );

	return imageFound;
}



//----------------------------------------------------------------------------------
// Carica i parametri necessari alla visione
//----------------------------------------------------------------------------------
bool load_visionpar()
{
	if( !VisDataLoad( Vision ) )
	{
		//TODO: messaggio errore
	}

	return LoadMapData( MAPDATAFILEXY );
}



// elabora l'immagine salvata su disco creando il pattern di confronto
// se non esistono dati immagine viene creato un default
bool ImagePack_Elabora( char* imageName, int imageType )
{
	char filename[MAXNPATH+1];

	strcpy( filename, imageName );
	AppendImageMode( filename, imageType, DATA );

	//carica dati immagine se non esiste crea default
	img_data imgData;
	ImgDataLoad( filename, &imgData );

	strcpy( filename, imageName );
	AppendImageMode( filename, imageType, IMAGE );

	if(access(filename,0)!=0)
	{
		return false;
	}

	bitmap* image = new bitmap(filename);     // carica bitmap da file

	strcpy( filename, imageName );
	AppendImageMode( filename, imageType, ELAB );

	// prende solo la parte selezionata come pattern di confronto
	image->crop(image->get_width()/2-imgData.pattern_x/2,
					image->get_height()/2-imgData.pattern_y/2,
					image->get_width()/2+imgData.pattern_x/2,
					image->get_height()/2+imgData.pattern_y/2);

	bool error = !image->save(filename); // salva il pattern

	delete image;

	return error;
}


// elabora l'immagine salvata su disco creando il pattern di confronto
// se non esistono dati immagine viene creato un default
bool Image_Elabora( int imageType, int imageNum )
{
	char filename[MAXNPATH];

	// carica dati immagine se non esiste crea default
	SetImageName( filename, imageType, DATA, imageNum );  // trova nome file dati immagine

	struct img_data imgData;
	ImgDataLoad( filename, &imgData );

	SetImageName( filename, imageType, IMAGE, imageNum ); // trova nome file immagine
	if(access(filename,0)!=0)
	{
		return false; // esce se file immagine non presente
	}

	bitmap* image = new bitmap(filename);     // carica bitmap da file

	SetImageName( filename, imageType, ELAB, imageNum );  // trova nome per immagine elaborata

	// prende solo la parte selezionata come pattern di confronto
	image->crop(image->get_width()/2-imgData.pattern_x/2,
			image->get_height()/2-imgData.pattern_y/2,
			image->get_width()/2+imgData.pattern_x/2,
			image->get_height()/2+imgData.pattern_y/2);

	bool ret = image->save( filename ); // salva il pattern

	delete image;

	return ret;
}


// Integr. Loris
int ChoosePattern(int x0, int y0, int &sizex, int &sizey, int step,int minx,int miny,int maxx,int maxy)
{
	int tasto=0;

	captureFramedImageAndDisplay( false );

	if(minx==-1)
	{
		minx=PATTERN_MINX;
	}
	
	if(miny==-1)
	{
		miny=PATTERN_MINY;
	}

	if(maxx==-1)
	{
		maxx=PATTERN_MAXX;
	}

	if(maxy==-1)
	{
		maxy=PATTERN_MAXY;
	}

	//svuota buffer
	flushKeyboardBuffer();

	if(sizex>maxx) sizex=maxx;
	if(sizex<minx) sizex=minx;

	if(sizey>maxy) sizey=maxy;
	if(sizey<miny) sizey=miny;

	int savex=sizex, savey=sizey;
	
	OpenImgBox( x0, y0, sizex, sizey, maxx, maxy );

	while( tasto != K_ESC && tasto != K_ENTER )
	{
		tasto = Handle();
	
		switch( tasto )
		{
			case K_HOME:
				sizex=savex;
				sizey=savey;
				break;
	
			case K_UP:
				if((sizey+step)<=maxy) sizey+=step;
				break;
	
			case K_DOWN:
				if((sizey-step)>=miny) sizey-=step;
				break;
	
			case K_LEFT:
				if((sizex+step)<=maxx) sizex+=step;
				break;
	
			case K_RIGHT:
				if((sizex-step)>=minx) sizex-=step;
				break;
	
			case K_PAGEUP:
				if((sizex+step)<=maxx) sizex+=step;
				if((sizey+step)<=maxy) sizey+=step;
				break;
	
			case K_PAGEDOWN:
				if((sizex-step)>=minx) sizex-=step;
				if((sizey-step)>=miny) sizey-=step;
				break;
		}
		
		MoveImgBox( x0, y0, sizex, sizey );
	}

	CloseImgBox();

	return tasto != K_ESC ? 1 : 0;
}
