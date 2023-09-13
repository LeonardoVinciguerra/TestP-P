//---------------------------------------------------------------------------
//
// Name:        tv.cpp
// Author:      Gabriel Ferri
// Created:     22/10/2008 14.22
// Description: camera functions implementation
//
//---------------------------------------------------------------------------
#include "tv.h"

#include <string>
#include <SDL/SDL.h>

#include "video_dev.h"
#include "q_inifile.h"
#include "msglist.h"
#include "q_grcol.h"
#include "q_debug.h"
#include "q_oper.h"
#include "q_fox.h"
#include "q_help.h"
#include "bitmap.h"
#include "lnxdefs.h"
#include "comaddr.h"
#include "q_camera.h"

#include "gui_defs.h"
#include "gui_functions.h"
#include "gui_widgets.h"

#include <mss.h>


extern struct cur_data  CurDat;
extern struct vis_data Vision;

//----------------------//
//  Dispositivo video   //
//----------------------//

#define VIDEO_W             	800
#define VIDEO_H             	600
#define VIDEO_FPS           	25
#define VIDEO_ZOOM				2.0f

// live video window
#define LVW_WIDTH_CHARS    		(VIDEO_W/GUI_CharW()+1)
#define LVW_WIDTH_PIXELS		1350
#define LVW_WIDTH_CHARS_EXP		(LVW_WIDTH_PIXELS/GUI_CharW()+1)
#define LVW_HEIGHT_CHARS   		36
#define LVW_VIDEO_BORDER   		10
#define LVW_VIDEO_WIDTH    		(VIDEO_W-2*LVW_VIDEO_BORDER)
#define LVW_VIDEO_HEIGHT   		(VIDEO_H-2*LVW_VIDEO_BORDER)
#define LVW_VIDEO_Y        		13

CWindow* _wLiveVideo = 0;
boost::mutex live_video_mutex;

#ifndef __DOME_FEEDER
#define CROSS_MAX_COL   6
#define CROSS_MAX_TYPE  CROSS_MAX_COL*2
#else
#define CROSS_MAX_COL   12
#define CROSS_MAX_TYPE  18
#endif


//-----------------------------------//
//  Definizione forme overlay video  //
//-----------------------------------//

// center
//#define CROSS_CX        (_wLiveVideo->GetX()+(_wLiveVideo->GetW()/2))
#define CROSS_CX        (_wLiveVideo->GetX()+(LVW_VIDEO_WIDTH/2)+LVW_VIDEO_BORDER+LVW_VIDEO_BORDER/2)
#define CROSS_CY        (_wLiveVideo->GetY() + LVW_VIDEO_Y + LVW_VIDEO_HEIGHT / 2)

// main cross
#define CROSSLENX       150
#define CROSSLENY       150
// line LEFT
#define CROSSPOS1X      CROSS_CX-CROSSLENX-1
#define CROSSPOS1XE     CROSS_CX-2
#define CROSSPOS1Y      CROSS_CY
// line UP
#define CROSSPOS2X      CROSS_CX
#define CROSSPOS2Y      CROSS_CY-CROSSLENY-1
#define CROSSPOS2YE     CROSS_CY-2
// line RIGHT
#define CROSSPOS3X      CROSS_CX+2
#define CROSSPOS3XE     CROSS_CX+CROSSLENX+1
#define CROSSPOS3Y      CROSS_CY
// line DOWN
#define CROSSPOS4X      CROSS_CX
#define CROSSPOS4Y      CROSS_CY+2
#define CROSSPOS4YE     CROSS_CY+CROSSLENY+1


// box 1 (piccolo)
#define BOX1SIZE        25
#define BOX1POS1X       CROSS_CX-BOX1SIZE/2
#define BOX1POS1Y       CROSS_CY-BOX1SIZE/2
#define BOX1POS2X       CROSS_CX-BOX1SIZE/2
#define BOX1POS2Y       CROSS_CY+BOX1SIZE/2
#define BOX1POS3X       CROSS_CX+BOX1SIZE/2
#define BOX1POS3Y       CROSS_CY-BOX1SIZE/2
// box 2
#define BOX2SIZE        51
#define BOX2POS1X       CROSS_CX-BOX2SIZE/2
#define BOX2POS1Y       CROSS_CY-BOX2SIZE/2
#define BOX2POS2X       CROSS_CX-BOX2SIZE/2
#define BOX2POS2Y       CROSS_CY+BOX2SIZE/2
#define BOX2POS3X       CROSS_CX+BOX2SIZE/2
#define BOX2POS3Y       CROSS_CY-BOX2SIZE/2
// box 3 (grande)
#define BOX3SIZE        75
#define BOX3POS1X       CROSS_CX-BOX3SIZE/2
#define BOX3POS1Y       CROSS_CY-BOX3SIZE/2
#define BOX3POS2X       CROSS_CX-BOX3SIZE/2
#define BOX3POS2Y       CROSS_CY+BOX3SIZE/2
#define BOX3POS3X       CROSS_CX+BOX3SIZE/2
#define BOX3POS3Y       CROSS_CY-BOX3SIZE/2
// box 1 ext (piccolo)
#define BOX1EXT_SIZE  194
#define BOX1EXT_POS1X CROSS_CX-BOX1EXT_SIZE/2
#define BOX1EXT_POS1Y CROSS_CY-BOX1EXT_SIZE/2
#define BOX1EXT_POS2X CROSS_CX-BOX1EXT_SIZE/2
#define BOX1EXT_POS2Y CROSS_CY+BOX1EXT_SIZE/2
#define BOX1EXT_POS3X CROSS_CX+BOX1EXT_SIZE/2
#define BOX1EXT_POS3Y CROSS_CY-BOX1EXT_SIZE/2
// box 2 ext (grande)
#define BOX2EXT_SIZE  277
#define BOX2EXT_POS1X CROSS_CX-BOX2EXT_SIZE/2
#define BOX2EXT_POS1Y CROSS_CY-BOX2EXT_SIZE/2
#define BOX2EXT_POS2X CROSS_CX-BOX2EXT_SIZE/2
#define BOX2EXT_POS2Y CROSS_CY+BOX2EXT_SIZE/2
#define BOX2EXT_POS3X CROSS_CX+BOX2EXT_SIZE/2
#define BOX2EXT_POS3Y CROSS_CY-BOX2EXT_SIZE/2


#ifdef __DOME_FEEDER
// box 4 (piccolo)
#define BOX4SIZE        78
#define BOX4POS1X       CROSS_CX-BOX4SIZE/2
#define BOX4POS1Y       CROSS_CY-BOX4SIZE/2
#define BOX4POS2X       CROSS_CX-BOX4SIZE/2
#define BOX4POS2Y       CROSS_CY+BOX4SIZE/2
#define BOX4POS3X       CROSS_CX+BOX4SIZE/2
#define BOX4POS3Y       CROSS_CY-BOX4SIZE/2
// box 5
#define BOX5SIZE        130
#define BOX5POS1X       CROSS_CX-BOX5SIZE/2
#define BOX5POS1Y       CROSS_CY-BOX5SIZE/2
#define BOX5POS2X       CROSS_CX-BOX5SIZE/2
#define BOX5POS2Y       CROSS_CY+BOX5SIZE/2
#define BOX5POS3X       CROSS_CX+BOX5SIZE/2
#define BOX5POS3Y       CROSS_CY-BOX5SIZE/2
// box 6
#define BOX6SIZE        182
#define BOX6POS1X       CROSS_CX-BOX6SIZE/2
#define BOX6POS1Y       CROSS_CY-BOX6SIZE/2
#define BOX6POS2X       CROSS_CX-BOX6SIZE/2
#define BOX6POS2Y       CROSS_CY+BOX6SIZE/2
#define BOX6POS3X       CROSS_CX+BOX6SIZE/2
#define BOX6POS3Y       CROSS_CY-BOX6SIZE/2
// box 7 (grande)
#define BOX7SIZE        234
#define BOX7POS1X       CROSS_CX-BOX7SIZE/2
#define BOX7POS1Y       CROSS_CY-BOX7SIZE/2
#define BOX7POS2X       CROSS_CX-BOX7SIZE/2
#define BOX7POS2Y       CROSS_CY+BOX7SIZE/2
#define BOX7POS3X       CROSS_CX+BOX7SIZE/2
#define BOX7POS3Y       CROSS_CY-BOX7SIZE/2
#endif //__DOME_FEEDER

#define MAX_CONTROL_VALUE		60000
#define MIN_CONTROL_VALUE		1000
#define DEF_CONTROL_VALUE		32768

#define CONTROL_VALUE_RANGE		65536

//SL 22-01-19 #define CAM_HEAD_KEY		"UVC"//"USB"//"PC"//"ITech"//"UVC"//"SMI"
#define CAM_HEAD_KEY1		"UVC"//"USB"//"PC"//"ITech"//"UVC"//"SMI"
#define CAM_HEAD_KEY2		"HD WEBCAM"//"USB"//"PC"//"ITech"//"UVC"//"SMI"
#define CAM_EXT_KEY			"BT878"


unsigned int _current_bright = DEF_CONTROL_VALUE;
unsigned int _current_contrast = DEF_CONTROL_VALUE;

int _currentcam = NO_CAMERA;

double _current_diameter = DEF_DIAMETER_VALUE;
double _current_tolerance = DEF_TOLERANCE_VALUE;
double _current_accumulator = DEF_ACCUMULATOR_VALUE;
unsigned int _current_smooth = DEF_SMOOTH_VALUE;
unsigned int _current_edge = DEF_EDGE_VALUE;
unsigned int _current_area_x = DEF_SEARCHX_VALUE;
unsigned int _current_area_y = DEF_SEARCHY_VALUE;

int pointerOnFrame = POINTER_CROSS;

int circlePosX = 0;
int circlePosY = 0;

	//--------------------//
	//  Funzioni display  //
	//--------------------//

GUI_color _cross_color;
bool _cross_box = 0;
bool _cross_packbox = 0;
bool _cross_auxbox = 0;
#ifdef __DOME_FEEDER
int _cross_domebox = 0;
#endif

#define TV_TITLE_LEN    80
char _tvTitle[TV_TITLE_LEN+1] = "";

int videoStyle = VIDEO_NORMAL;

void displayFramedImage_Camera( unsigned char* img, int img_w, int img_h, int bpp );
void (*fnDisplayFrameCallback)( void* parentWin ); // puntatore a funzione di callback
bool _showVideoControls = true;

bitmap* captured = 0;

	//----------------------------//
	//  FPS - Frames Per Seconds  //
	//----------------------------//

#define __CHECK_FPS
#ifdef __CHECK_FPS
#include <sys/time.h>

#define MAXSAMPLES		10
int tickindex = 0;
unsigned int ticklist[MAXSAMPLES];
unsigned int ticksum = 0;
timeval t1, t2;
double elapsedTime;

void FPS_Init()
{
	for( int i = 0; i < MAXSAMPLES; i++ )
		ticklist[i] = 0;

	gettimeofday( &t1, NULL );
}

void FPS_Update()
{
	gettimeofday( &t2, NULL );

	// compute the elapsed time in millisec
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

	t1 = t2;

	// subtract value falling off
	ticksum -= ticklist[tickindex];
	// save new value so it can be subtracted later
	ticklist[tickindex] = elapsedTime;
	// add new value
	ticksum += ticklist[tickindex];

	if( ++tickindex == MAXSAMPLES )
		tickindex = 0;
}

float FPS_CalcAverage()
{
	return( MAXSAMPLES / ((float)ticksum / 1000.f) );
}

void FPS_Display( int x, int y )
{
	char buf[16];
	snprintf( buf, sizeof(buf), "FPS: %.1f", FPS_CalcAverage() );

	GUI_DrawText( x, y, buf, GUI_DefaultFont, GUI_color( 0,0,0 ), GUI_color( 255,255,255 ) );
}
#endif



	//---------------//
	//  VideoDevice  //
	//---------------//

VideoDevice videoDev_Head;
VideoDevice videoDev_Ext;

bool zoom_head = false;
bool zoom_ext = false;



//---------------------------------------------------------------------------------
// Setta la funzione di callback chiamata nella visualizzazione di ciascun frame
//---------------------------------------------------------------------------------
void setLiveVideoCallback( void (*f)( void* parentWin ) )
{
	fnDisplayFrameCallback = f;
}

//---------------------------------------------------------------------------------
// Visualizza/Nasconde i controlli dell'immagine (lum. e contr.)
//---------------------------------------------------------------------------------
void showVideoControls( bool show )
{
	_showVideoControls = show;
}


//---------------------------------------------------------------------------------
// set_style
// setta lo stile (centrato o sinistro) di visualizzazione
//---------------------------------------------------------------------------------
void set_style( int style )
{
	if( !Get_UseCam() )
	{
		return;
	}

	videoStyle = style;
}

//---------------------------------------------------------------------------------
// playLiveVideo
// attiva la visualizzazione della telecamera sullo schermo
//---------------------------------------------------------------------------------
void playLiveVideo( int camera, bool loadControl  )
{
	if( !Get_UseCam() )
	{
		return;
	}

	if( camera == CAMERA_HEAD )
	{
		// led camera on
		Set_HeadCameraLight( 1 );
		set_currentcam( CAMERA_HEAD );

		if( loadControl )
		{
			CameraControls_Load(); //DB270418
			CameraControls_Load(); //DB270418
		}

		videoDev_Head.PlayVideo();
	}
	else if( camera == CAMERA_EXT )
	{
		set_currentcam( CAMERA_EXT );

		videoDev_Ext.PlayVideo();
	}
}

//---------------------------------------------------------------------------------
// pauseLiveVideo
// ferma la visualizzazione della telecamera sullo schermo
// ritorna: la telecamera in uso
//---------------------------------------------------------------------------------
int pauseLiveVideo( bool turOffLight )
{
	if( !Get_UseCam() )
	{
		return NO_CAMERA;
	}

	if( get_currentcam() == CAMERA_HEAD )
	{
		if( videoDev_Head.IsVideoLive() )
		{
			videoDev_Head.StopVideo();

			if( turOffLight )
			{
				// led camera off
				Set_HeadCameraLight( -1 );
			}

			return CAMERA_HEAD;
		}
	}
	else if( get_currentcam() == CAMERA_EXT )
	{
		if( videoDev_Ext.IsVideoLive() )
		{
			videoDev_Ext.StopVideo();

			return CAMERA_EXT;
		}
	}

	return NO_CAMERA;
}

//---------------------------------------------------------------------------------
// captureFrame
//---------------------------------------------------------------------------------
void captureFrame( void* frame )
{
	if( get_currentcam() == CAMERA_HEAD )
	{
		videoDev_Head.GetFrame( frame );
		return;
	}
	else if( get_currentcam() == CAMERA_EXT )
	{
		videoDev_Ext.GetFrame( frame );
		return;
	}

	print_debug( "captureFrame: CAMERA not selected!\n" );
}

//---------------------------------------------------------------------------------
// Ritorna la posizione del centro del video
//---------------------------------------------------------------------------------
int GetVideoCenterX()
{
	if( _wLiveVideo )
	{
		return _wLiveVideo->GetX() + _wLiveVideo->GetW() / 2;
	}

	return GUI_ScreenW()/2;
}

int GetVideoCenterY()
{
	if( _wLiveVideo )
	{
		return _wLiveVideo->GetY() + LVW_VIDEO_Y + LVW_VIDEO_HEIGHT / 2;
	}

	return GUI_ScreenH()/2;
}


//---------------------------------------------------------------------------------
// getFrameWidth
//---------------------------------------------------------------------------------
int getFrameWidth()
{
	if( get_currentcam() == CAMERA_HEAD )
	{
		return videoDev_Head.GetFrame_Width();
	}
	else if( get_currentcam() == CAMERA_EXT )
	{
		return videoDev_Ext.GetFrame_Width();
	}

	print_debug( "getFrameWidth: CAMERA not selected!\n" );
	return -1;
}

//---------------------------------------------------------------------------------
// getFrameHeight
//---------------------------------------------------------------------------------
int getFrameHeight()
{
	if( get_currentcam() == CAMERA_HEAD )
	{
		return videoDev_Head.GetFrame_Height();
	}
	else if( get_currentcam() == CAMERA_EXT )
	{
		return videoDev_Ext.GetFrame_Height();
	}

	print_debug( "CAMERA not selected!\n" );
	return -1;
}

//---------------------------------------------------------------------------------
// getFrameBpp
//---------------------------------------------------------------------------------
int getFrameBpp()
{
	if( get_currentcam() == CAMERA_HEAD )
	{
		return videoDev_Head.GetFrame_Bpp();
	}
	else if( get_currentcam() == CAMERA_EXT )
	{
		return videoDev_Ext.GetFrame_Bpp();
	}

	print_debug( "CAMERA not selected!\n" );
	return -1;
}

//---------------------------------------------------------------------------------
// getFrameSizeInByte
//---------------------------------------------------------------------------------
int getFrameSizeInByte()
{
	if( get_currentcam() == CAMERA_HEAD )
	{
		return videoDev_Head.GetFrame_SizeInByte();
	}
	else if( get_currentcam() == CAMERA_EXT )
	{
		return videoDev_Ext.GetFrame_SizeInByte();
	}

	print_debug( "CAMERA not selected!\n" );
	return -1;
}

//---------------------------------------------------------------------------------
// captureFramedImageAndDisplay
//---------------------------------------------------------------------------------
void captureFramedImageAndDisplay( bool showCross )
{
	int frame_size = getFrameSizeInByte();
	if( frame_size == -1 )
	{
		return;
	}

	unsigned char* img = new unsigned char[ frame_size ];
	captureFrame( img );

	GUI_Freeze_Locker lock;

	displayFramedImage_Camera( img, getFrameWidth(), getFrameHeight(), getFrameBpp() );

	if( showCross )
		displayCross();

	delete [] img;
}

//---------------------------------------------------------------------------------
// Disegna il frame della vista telecamera
//---------------------------------------------------------------------------------
void displayLiveVideoFrame( int camera, const char* title, void(*fInfo)( void* parentWin ) = 0 )
{
	GUI_Freeze_Locker lock;

	if( camera == CAMERA_HEAD )
	{
		RectI r;
		if( videoStyle == VIDEO_NORMAL )
			r.X = _wLiveVideo->GetX() + (_wLiveVideo->GetW()-LVW_VIDEO_WIDTH)/2 - 1;
		else
			r.X = _wLiveVideo->GetX() + LVW_VIDEO_BORDER + 4;
		r.Y = _wLiveVideo->GetY() + LVW_VIDEO_Y - 1;
		r.W = LVW_VIDEO_WIDTH+2;
		r.H = LVW_VIDEO_HEIGHT+2;

		GUI_Rect( r, GUI_color( 0,0,0 ) );

		if( videoStyle == VIDEO_EXPANDED )
		{
			RectI r1;
			r1.X = _wLiveVideo->GetX()+ELAB_VIDEO_STARTX-1;
			r1.Y = _wLiveVideo->GetY() + LVW_VIDEO_Y-1;
			r1.W = ELAB_VIDEO_W+2;
			r1.H = ELAB_VIDEO_H+2;
			GUI_FillRect( r1, GUI_color( WIN_COL_SUBTITLE ) );
			GUI_Rect( r1, GUI_color( 0,0,0 ) );
		}
	}

	_wLiveVideo->SetTitle( title );

	// display info
	if( fInfo )
	{
		fInfo( _wLiveVideo );
	}
}

//---------------------------------------------------------------------------------
// setCross
// Imposta la croce su schermo
// Mode = 0  usa colore (forma) corrente
// Mode = 1  incrementa colore (forma) aggiornando CurDat
// Per i colori da 0 a 5 solo croce, atrimenti (6..11) croce con quadrati
//---------------------------------------------------------------------------------
extern struct CfgHeader QHeader;

void setCross( int mode, int boxType )
{
	int type;  // temp colore croce (0..11) con codifica quadrati
	int color; // colore croce      (0..5)

	_cross_box = 0;
	#ifdef __DOME_FEEDER
	_cross_domebox = 0;
	#endif

	type = MAX( 0, CurDat.Cross );

	if( mode ) // modo incrementa colore
	{
		if( type < CROSS_MAX_TYPE-1 )
		{
			type++;
			if(!(QHeader.debugMode2 & DEBUG2_BIGCROSS) && (type==CROSS_MAX_COL))
			{
				type = 0; // inizia dal primo
			}
		}
		else
		{
			type = 0; // inizia dal primo
		}

		CurDat.Cross = type;  // salva in CurDat.Cross (va fatto prima di set_clip)
		CurData_Write( CurDat ); // salva CurDat su disco
	}

	if( type >= CROSS_MAX_COL ) // se quadrati presenti
	{
		color = type - CROSS_MAX_COL; // setta colore croce
		_cross_box = 1;                       // setta quadrati presenti
	}
	else
	{
		#ifdef __DOME_FEEDER
		if((type >= 6) && (type < 12))
		{
			color = type - 6;
			_cross_domebox = 1;
		}
		else
		#endif
		{
			color = type; // setta colore croce
		}
	}

	_cross_packbox = ( boxType & CROSSBOX_PACK ) ? 1 : 0;
	_cross_auxbox = ( boxType & CROSSBOX_EXTCAM ) ? 1 : 0;

	switch( color ) // seleziona colore per croce
	{
		case 0:
			_cross_color = GUI_color(GR_WHITE);
			break;
		case 1:
			_cross_color = GUI_color(GR_LIGHTYELLOW);
			break;
		case 2:
			_cross_color = GUI_color(GR_LIGHTGREEN);
			break;
		case 3:
			_cross_color = GUI_color(GR_LIGHTRED);
			break;
		case 4:
			_cross_color = GUI_color(GR_LIGHTBLUE);
			break;
		case 5:
			_cross_color = GUI_color(GR_BLACK);
			break;
		default:
			_cross_color = GUI_color(GR_WHITE);
			break;
	}
}


	//----------------//
	//  Frame camera  //
	//--- ------------//

void displayFrame( unsigned char* img, int img_w, int img_h, int bpp )
{
	boost::mutex::scoped_lock lock_video( live_video_mutex );

	if( !_wLiveVideo )
		return;

	GUI_Freeze_Locker lock;

	displayFramedImage_Camera( img, img_w, img_h, bpp );

	if( _showVideoControls )
	{
		int x1 = _wLiveVideo->GetX() + 30;
		int x2 = _wLiveVideo->GetX() + 670;
		int y = _wLiveVideo->GetY() + LVW_VIDEO_Y + LVW_VIDEO_HEIGHT + 30;
		// update bright and contrast controls
		GUI_Showbar( x1, y, _current_bright, MIN_CONTROL_VALUE, MAX_CONTROL_VALUE, MsgGetString(Msg_00716) ); // brightness control
		GUI_Showbar( x2, y, _current_contrast, MIN_CONTROL_VALUE, MAX_CONTROL_VALUE, MsgGetString(Msg_00717) ); // contrast control
	}

	// display cross
	if( pointerOnFrame == POINTER_CROSS )
		displayCross();
	else if( pointerOnFrame == POINTER_CIRCLE )
		displayCircle();

	if( fnDisplayFrameCallback )
	{
		// execute callback function
		fnDisplayFrameCallback( _wLiveVideo );
	}

	#ifdef __CHECK_FPS
	if( videoStyle == VIDEO_NORMAL )
		FPS_Display( _wLiveVideo->GetX()+(_wLiveVideo->GetW()-LVW_VIDEO_WIDTH)/2, _wLiveVideo->GetY() + LVW_VIDEO_Y );
	else
		FPS_Display( _wLiveVideo->GetX()+LVW_VIDEO_BORDER+4, _wLiveVideo->GetY() + LVW_VIDEO_Y );
	FPS_Update();
	#endif
}

//---------------------------------------------------------------------------------
// Visualizza a video un'immagine "ritagliata"
//   img_w: image width
//   bpp: byte per pixel -> 1 (GREY), 3 (RGB)
//---------------------------------------------------------------------------------
void displayFramedImage_Camera( unsigned char* img, int img_w, int img_h, int bpp )
{
	GUI_Freeze_Locker lock;

	int _img_delta_x = (img_w - LVW_VIDEO_WIDTH) / 2;
	int _img_delta_y = (img_h - LVW_VIDEO_HEIGHT) / 2;
	_img_delta_x = MAX( _img_delta_x, 0 );
	_img_delta_y = MAX( _img_delta_y, 0 );

	unsigned char* pix_ptr = img + (( _img_delta_y * img_w) + _img_delta_x) * bpp;

	int sW = MIN( LVW_VIDEO_WIDTH, img_w );
	int sH = MIN( LVW_VIDEO_HEIGHT, img_h );
	void* fromCamera;

	// create surface
	if( bpp == 1 )
	{
		fromCamera = SDL_CreateRGBSurfaceFrom( pix_ptr, sW, sH, 8, img_w, 0x000000FF, 0x000000FF, 0x000000FF, 0 );
	}
	else
	{
		fromCamera = SDL_CreateRGBSurfaceFrom( pix_ptr, sW, sH, 24, img_w * bpp, 0x000000FF, 0x0000FF00, 0x00FF0000, 0 );
	}

	// draw surface
	if( videoStyle == VIDEO_NORMAL )
		GUI_DrawSurface( PointI( _wLiveVideo->GetX() + (_wLiveVideo->GetW()-sW)/2, _wLiveVideo->GetY() + LVW_VIDEO_Y + (LVW_VIDEO_HEIGHT-sH)/2 ), fromCamera );
	else
		GUI_DrawSurface( PointI( _wLiveVideo->GetX() + LVW_VIDEO_BORDER+1+4, _wLiveVideo->GetY() + LVW_VIDEO_Y + (LVW_VIDEO_HEIGHT-sH)/2 ), fromCamera );
	GUI_FreeSurface( &fromCamera );


	//TEMP: soluzione temporanea
	if( img_w == 744 )
	{
		// telecamera esterna -> rifilare immagine
		RectI r;
		r.X = _wLiveVideo->GetX() + (_wLiveVideo->GetW()-img_w)/2 - 1;
		r.Y = _wLiveVideo->GetY() + LVW_VIDEO_Y + (LVW_VIDEO_HEIGHT-img_h)/2 - 1;
		r.W = img_w+2;
		r.H = img_h+2;

		GUI_FillRect( RectI( r.X, r.Y, 28, r.H ), GUI_color( WIN_COL_CLIENTAREA ) );
		GUI_FillRect( RectI( r.X+r.W-28, r.Y, 28, r.H ), GUI_color( WIN_COL_CLIENTAREA ) );
		GUI_FillRect( RectI( r.X, r.Y, r.W, 15 ), GUI_color( WIN_COL_CLIENTAREA ) );
		GUI_FillRect( RectI( r.X, r.Y+r.H-15, r.W, 15 ), GUI_color( WIN_COL_CLIENTAREA ) );

		GUI_Rect( RectI( r.X+28, r.Y+15, r.W-28*2, r.H-15*2 ), GUI_color( 0,0,0 ) );
	}

	// Gestione immagine elaborata
	if( videoStyle == VIDEO_EXPANDED )
	{
		if( captured != 0 )
		{
			delete captured;
			captured = 0;
		}

		RectI r1;
		r1.X = _wLiveVideo->GetX()+ELAB_VIDEO_STARTX-1;
		r1.Y = _wLiveVideo->GetY() + LVW_VIDEO_Y-1;
		r1.W = ELAB_VIDEO_W+2;
		r1.H = ELAB_VIDEO_H+2;
		GUI_FillRect( r1, GUI_color( WIN_COL_SUBTITLE ) );
		GUI_Rect( r1, GUI_color( 0,0,0 ) );

		captured = new bitmap( MID( MIN_SEARCHX_VALUE, _current_area_x, MAX_SEARCHX_VALUE), MID( MIN_SEARCHY_VALUE, _current_area_y, MAX_SEARCHY_VALUE), 0 );
		captured->createFromFrame((void*)img, getFrameWidth()/2 ,getFrameHeight()/2 );
		//captured->show(ELAB_VIDEO_STARTX+ELAB_VIDEO_W/2,_wLiveVideo->GetY() + LVW_VIDEO_Y + ELAB_VIDEO_H/2 );

		float mmpix = (Vision.mmpix_x + Vision.mmpix_y) / 2.f;
		float posX, posY;
		unsigned short diameter = ( (_current_diameter-_current_tolerance/2.0) / mmpix );
		//Mostra solo i risultati dell'edge detection
		captured->findCircle( posX, posY, diameter, (_current_tolerance / mmpix), _current_smooth, _current_edge, _current_accumulator, 0x0002 );
		captured->showDebug(_wLiveVideo->GetX()+ELAB_VIDEO_STARTX+ELAB_VIDEO_W/2,_wLiveVideo->GetY() + LVW_VIDEO_Y + ELAB_VIDEO_H/2 );
	}

}

//----------------------------------------------------------------------------------
// Find the most like circle shape fiducial
//----------------------------------------------------------------------------------
bool findCircleFiducial( float& posX, float& posY, unsigned short& circleDiameter )
{
	float mmpix = (Vision.mmpix_x + Vision.mmpix_y) / 2.f;

	bool res = captured->findCircle( posX, posY, circleDiameter, (_current_tolerance / mmpix), _current_smooth, _current_edge, _current_accumulator, 0x0001 );
	captured->showDebug(_wLiveVideo->GetX()+ELAB_VIDEO_STARTX+ELAB_VIDEO_W/2,_wLiveVideo->GetY() + LVW_VIDEO_Y + ELAB_VIDEO_H/2 );

	return res;
}

// pack box
#define PACKBOX_DIM   250

//---------------------------------------------------------------------------------
// displayCross
// disegna la croce su schermo
//---------------------------------------------------------------------------------
void displayCross()
{
	GUI_Freeze_Locker lock;

	if( _cross_packbox )
	{
		GUI_VLine( CROSS_CX-PACKBOX_DIM/2, CROSS_CY-PACKBOX_DIM/2, CROSS_CY+PACKBOX_DIM/2, _cross_color ); //LEFT
		GUI_HLine( CROSS_CX-PACKBOX_DIM/2, CROSS_CX+PACKBOX_DIM/2, CROSS_CY-PACKBOX_DIM/2, _cross_color ); //UP
		GUI_VLine( CROSS_CX+PACKBOX_DIM/2, CROSS_CY-PACKBOX_DIM/2, CROSS_CY+PACKBOX_DIM/2, _cross_color ); //RIGHT
		GUI_HLine( CROSS_CX-PACKBOX_DIM/2, CROSS_CX+PACKBOX_DIM/2, CROSS_CY+PACKBOX_DIM/2, _cross_color ); //DOWN

		GUI_Thaw();
		return;
	}

    // disegna croce principale
	GUI_HLine( CROSSPOS1X, CROSSPOS1XE, CROSSPOS1Y, _cross_color ); //LEFT
	GUI_VLine( CROSSPOS2X, CROSSPOS2Y, CROSSPOS2YE, _cross_color ); //UP
	GUI_HLine( CROSSPOS3X, CROSSPOS3XE, CROSSPOS3Y, _cross_color ); //RIGHT
	GUI_VLine( CROSSPOS4X, CROSSPOS4Y, CROSSPOS4YE, _cross_color ); //DOWN

	// disegna quadrati
	if( _cross_box || _cross_auxbox )
	{
		// box 1 (piccolo)
		GUI_VLine( BOX1POS1X, BOX1POS1Y, BOX1POS1Y+BOX1SIZE-1, _cross_color ); //LEFT
		GUI_HLine( BOX1POS1X, BOX1POS1X+BOX1SIZE-1, BOX1POS1Y, _cross_color ); //UP
		GUI_VLine( BOX1POS3X, BOX1POS3Y, BOX1POS3Y+BOX1SIZE-1, _cross_color ); //RIGHT
		GUI_HLine( BOX1POS2X, BOX1POS2X+BOX1SIZE-1, BOX1POS2Y, _cross_color ); //DOWN
		// box 2
		GUI_VLine( BOX2POS1X, BOX2POS1Y, BOX2POS1Y+BOX2SIZE-1, _cross_color ); //LEFT
		GUI_HLine( BOX2POS1X, BOX2POS1X+BOX2SIZE-1, BOX2POS1Y, _cross_color ); //UP
		GUI_VLine( BOX2POS3X, BOX2POS3Y, BOX2POS3Y+BOX2SIZE-1, _cross_color ); //RIGHT
		GUI_HLine( BOX2POS2X, BOX2POS2X+BOX2SIZE-1, BOX2POS2Y, _cross_color ); //DOWN
		// box 3 (grande)
		GUI_VLine( BOX3POS1X, BOX3POS1Y, BOX3POS1Y+BOX3SIZE-1, _cross_color ); //LEFT
		GUI_HLine( BOX3POS1X, BOX3POS1X+BOX3SIZE-1, BOX3POS1Y, _cross_color ); //UP
		GUI_VLine( BOX3POS3X, BOX3POS3Y, BOX3POS3Y+BOX3SIZE-1, _cross_color ); //RIGHT
		GUI_HLine( BOX3POS2X, BOX3POS2X+BOX3SIZE-1, BOX3POS2Y, _cross_color ); //DOWN
	}

	if( _cross_auxbox )
	{
		// box 1 aux (piccolo)
		GUI_VLine( BOX1EXT_POS1X, BOX1EXT_POS1Y, BOX1EXT_POS1Y+BOX1EXT_SIZE-1, _cross_color ); //LEFT
		GUI_HLine( BOX1EXT_POS1X, BOX1EXT_POS1X+BOX1EXT_SIZE-1, BOX1EXT_POS1Y, _cross_color ); //UP
		GUI_VLine( BOX1EXT_POS3X, BOX1EXT_POS3Y, BOX1EXT_POS3Y+BOX1EXT_SIZE-1, _cross_color ); //RIGHT
		GUI_HLine( BOX1EXT_POS2X, BOX1EXT_POS2X+BOX1EXT_SIZE-1, BOX1EXT_POS2Y, _cross_color ); //DOWN
		// box 2 aux (grande)
		GUI_VLine( BOX2EXT_POS1X, BOX2EXT_POS1Y, BOX2EXT_POS1Y+BOX2EXT_SIZE-1, _cross_color ); //LEFT
		GUI_HLine( BOX2EXT_POS1X, BOX2EXT_POS1X+BOX2EXT_SIZE-1, BOX2EXT_POS1Y, _cross_color ); //UP
		GUI_VLine( BOX2EXT_POS3X, BOX2EXT_POS3Y, BOX2EXT_POS3Y+BOX2EXT_SIZE-1, _cross_color ); //RIGHT
		GUI_HLine( BOX2EXT_POS2X, BOX2EXT_POS2X+BOX2EXT_SIZE-1, BOX2EXT_POS2Y, _cross_color ); //DOWN
	}

	#ifdef __DOME_FEEDER
	if( _cross_domebox )
	{
		// box 4 (piccolo)
		GUI_VLine( BOX4POS1X, BOX4POS1Y, BOX4POS1Y+BOX4SIZE-1, _cross_color ); //LEFT
		GUI_HLine( BOX4POS1X, BOX4POS1X+BOX4SIZE-1, BOX4POS1Y, _cross_color ); //UP
		GUI_VLine( BOX4POS3X, BOX4POS3Y, BOX4POS3Y+BOX4SIZE-1, _cross_color ); //RIGHT
		GUI_HLine( BOX4POS2X, BOX4POS2X+BOX4SIZE-1, BOX4POS2Y, _cross_color ); //DOWN
		// box 5
		GUI_VLine( BOX5POS1X, BOX5POS1Y, BOX5POS1Y+BOX5SIZE-1, _cross_color ); //LEFT
		GUI_HLine( BOX5POS1X, BOX5POS1X+BOX5SIZE-1, BOX5POS1Y, _cross_color ); //UP
		GUI_VLine( BOX5POS3X, BOX5POS3Y, BOX5POS3Y+BOX5SIZE-1, _cross_color ); //RIGHT
		GUI_HLine( BOX5POS2X, BOX5POS2X+BOX5SIZE-1, BOX5POS2Y, _cross_color ); //DOWN
		// box 6
		GUI_VLine( BOX6POS1X, BOX6POS1Y, BOX6POS1Y+BOX6SIZE-1, _cross_color ); //LEFT
		GUI_HLine( BOX6POS1X, BOX6POS1X+BOX6SIZE-1, BOX6POS1Y, _cross_color ); //UP
		GUI_VLine( BOX6POS3X, BOX6POS3Y, BOX6POS3Y+BOX6SIZE-1, _cross_color ); //RIGHT
		GUI_HLine( BOX6POS2X, BOX6POS2X+BOX6SIZE-1, BOX6POS2Y, _cross_color ); //DOWN
		// box 7 (grande)
		GUI_VLine( BOX7POS1X, BOX7POS1Y, BOX7POS1Y+BOX7SIZE-1, _cross_color ); //LEFT
		GUI_HLine( BOX7POS1X, BOX7POS1X+BOX7SIZE-1, BOX7POS1Y, _cross_color ); //UP
		GUI_VLine( BOX7POS3X, BOX7POS3Y, BOX7POS3Y+BOX7SIZE-1, _cross_color ); //RIGHT
		GUI_HLine( BOX7POS2X, BOX7POS2X+BOX7SIZE-1, BOX7POS2Y, _cross_color ); //DOWN
	}
	#endif
}

//---------------------------------------------------------------------------------
// displayCircle
// disegna il cerchio su schermo
//---------------------------------------------------------------------------------
void displayCircle()
{
	GUI_Freeze_Locker lock;

	float mmpix = (Vision.mmpix_x + Vision.mmpix_y) / 2.f;
	int cRadius = ((_current_diameter/2.0) / mmpix);

	GUI_Circle( PointI( circlePosX, circlePosY ), cRadius, _cross_color );
}

//---------------------------------------------------------------------------------
// circle_pointer_movexy
// sposta il cerchio su schermo
//---------------------------------------------------------------------------------
void circle_pointer_movexy( int stepx, int stepy )
{
	float mmpix = (Vision.mmpix_x + Vision.mmpix_y) / 2.f;
	int cRadius = ((_current_diameter/2.0) / mmpix);

	int tmpX = circlePosX + stepx;
	int tmpY = circlePosY + stepy;

	if( ((tmpX - cRadius) > 0) && ((tmpX + cRadius) < VIDEO_W) )
		circlePosX += stepx;
	if( ((tmpY - cRadius) > 0) && ((tmpY + cRadius) < VIDEO_H) )
		circlePosY += stepy;
}

void set_pointer( int pointer )
{
	pointerOnFrame = pointer;

	circlePosX = CROSS_CX;
	circlePosY = CROSS_CY;
}

//---------------------------------------------------------------------------------
// Inizializzazione dispositivo video
//---------------------------------------------------------------------------------
int open_tv()
{
	if( !Get_UseCam() )
	{
		return 1;
	}

	int head_index = -1;
	int ext_index = -1;

	// Enumerate available devices
	std::vector<V4L2Device> devices;
	EnumerateV4L2Device( devices );
	if( devices.size() == 0 )
	{
		W_Mess( MsgGetString(Msg_05049) );
		return 0;
	}

	for( int i = 0; i < devices.size(); i++ )
	{
		V4L2Device* device = &devices[i];
		printf( "Device name: %s\n", device->name.c_str() );

		// SL
		if(( strcasestr( device->name.c_str(), CAM_HEAD_KEY1 ) != NULL ) || ( strcasestr( device->name.c_str(), CAM_HEAD_KEY2 ) != NULL ))
		// SL 22-01-19 if( strstr( device->name.c_str(), CAM_HEAD_KEY ) != NULL )
		{
			head_index = i;
		}
		if( strstr( device->name.c_str(), CAM_EXT_KEY ) != NULL )
		{
			ext_index = i;
		}
	}


	// open video device heads
	if( head_index == -1 )
	{
		W_Mess( MsgGetString(Msg_05050) );
	}
	else
	{
		V4L2Device* device = &devices[head_index];

		printf( "Open camera HEAD: %s\n", device->longName.c_str() );

		// open video device head
		//if( !videoDev_Head.OpenDevice( device->longName.c_str(), 1600, 1200, V4L2_PIX_FMT_YUYV, VIDEO_FPS ) )
		if( !videoDev_Head.OpenDevice( device->longName.c_str(), VIDEO_W, VIDEO_H, V4L2_PIX_FMT_YUYV, VIDEO_FPS ) )
		//if( !videoDev_Head.OpenDevice( device->longName.c_str(), VIDEO_W, VIDEO_H, V4L2_PIX_FMT_MJPEG, VIDEO_FPS ) )
		//if( !videoDev_Head.OpenDevice( device->longName.c_str(), 2592, 1944, V4L2_PIX_FMT_MJPEG, VIDEO_FPS ) )
		{
			W_Mess( MsgGetString(Msg_05048) );
			head_index = -1;
		}
		videoDev_Head.SetDeviceKey( device->name.c_str() );
//SL 22-01-19		videoDev_Head.SetDeviceKey( CAM_HEAD_KEY );
	}


	// open video device ext
	if( ext_index == -1 )
	{
		W_Mess( MsgGetString(Msg_05051) );
	}
	else
	{
		V4L2Device* device = &devices[ext_index];

		char buf[200];
		snprintf( buf, sizeof(buf),"v4lctl -c %s setnorm PAL-N", device->longName.c_str() );
		system( buf );
		snprintf( buf, sizeof(buf),"v4lctl -c %s setnorm PAL-Nc", device->longName.c_str() );
		system( buf );

		printf( "Open camera EXT: %s\n", device->longName.c_str() );

		// open video device head
		if( !videoDev_Ext.OpenDevice( device->longName.c_str(), VIDEO_W, VIDEO_H, V4L2_PIX_FMT_GREY, VIDEO_FPS ) )
		{
			printf( "Error opening: %s\n", device->longName.c_str() );
			W_Mess( MsgGetString(Msg_05047) );
			ext_index = -1;
		}

		videoDev_Ext.SetDeviceKey( CAM_EXT_KEY );

		videoDev_Ext.GetV4LDevice()->selectInput( "S-Video" );
		//videoDev_Ext.GetV4LDevice()->selectInput( "Composite1" ); //TEMP: per vedere cam in gf pc
	}


	// carica parametri telecamere
	CameraControls_Load( false );

	// setta funzioni di callback e avvia thread per acquisizione video
	fnDisplayFrameCallback = 0;
	if( head_index != -1 )
	{
		videoDev_Head.SetCrop_Centered( VIDEO_W, VIDEO_H );

		videoDev_Head.SetVideoCallback( displayFrame );
		videoDev_Head.StartThread();
	}
	if( ext_index != -1 )
	{
		videoDev_Ext.SetVideoCallback( displayFrame );
		videoDev_Ext.StartThread();
	}

	// set last cross type
	setCross( CROSS_CURR_MODE );

	#ifdef __CHECK_FPS
	FPS_Init();
	#endif

	set_currentcam( NO_CAMERA );
	return 1;
}

//---------------------------------------------------------------------------------
// Chiusura dispositivo video
//---------------------------------------------------------------------------------
int close_tv()
{
	if( !Get_UseCam() )
	{
		return 0;
	}

	// chiude thread per acquisizione video
	videoDev_Head.StopThread();
	videoDev_Ext.StopThread();

	return 1;
}


//---------------------------------------------------------------------------------
// seleziona l'ingresso video
//---------------------------------------------------------------------------------
void set_currentcam( int camera )
{
	if( camera != CAMERA_HEAD && camera != CAMERA_EXT && camera != NO_CAMERA )
		return;

	_currentcam = camera;
}

int get_currentcam()
{
	return _currentcam;
}


//---------------------------------------------------------------------------------
// setta la luminosita' dell'immagine telecamera
//---------------------------------------------------------------------------------
void set_bright_abs( int value, bool save )
{
	if( !Get_UseCam() )
		return;

	VideoDevice* vd;
	if( get_currentcam() == CAMERA_HEAD )
		vd = &videoDev_Head;
	else if( get_currentcam() == CAMERA_EXT )
		vd = &videoDev_Ext;
	else
	{
		print_debug( "CAMERA not selected!\n" );
		return;
	}

	_current_bright = MID( MIN_CONTROL_VALUE, value, MAX_CONTROL_VALUE );
	vd->SetBrightness( _current_bright );

	// salva su disco
	if( save )
	{
		CurDat.HeadBright = _current_bright;
		CurData_Write(CurDat);
	}
}


//---------------------------------------------------------------------------------
// step: 0 setta luminosita' di default
//       <>0: passo incremento luminosita' (range luminosita': 0..65535)
//---------------------------------------------------------------------------------
void set_bright( int step, bool save )
{
	set_bright_abs( ((step != 0) ? _current_bright + step : DEF_CONTROL_VALUE), save );
}

//---------------------------------------------------------------------------------
// setta il contrasto dell'immagine telecamera
//---------------------------------------------------------------------------------
void set_contrast_abs( int value, bool save )
{
	if( !Get_UseCam() )
		return;

	VideoDevice* vd;
	if( get_currentcam() == CAMERA_HEAD )
		vd = &videoDev_Head;
	else if( get_currentcam() == CAMERA_EXT )
		vd = &videoDev_Ext;
	else
	{
		print_debug( "CAMERA not selected!\n" );
		return;
	}

	_current_contrast = MID( MIN_CONTROL_VALUE, value, MAX_CONTROL_VALUE );
	vd->SetContrast( _current_contrast );

	// salva su disco
	if( save )
	{
		CurDat.HeadContrast = _current_contrast;
		CurData_Write(CurDat);
	}
}

//---------------------------------------------------------------------------------
// step: 0 setta contrasto di default
//       <>0: passo incremento contrasto (range contrasto: 0..65535)
//---------------------------------------------------------------------------------
void set_contrast( int step, bool save )
{
	set_contrast_abs( ((step != 0) ? _current_contrast + step : DEF_CONTROL_VALUE), save );
}

//---------------------------------------------------------------------------------
// Setta luminosita' e contrasto senza salvare (settaggio temporaneo visione)
//---------------------------------------------------------------------------------
void SetImgBrightCont( int bright, int contrast )
{
	set_bright_abs( bright, false );
	set_contrast_abs( contrast, false );
}

//---------------------------------------------------------------------------------
// Ritorna luminosita' corrente
//---------------------------------------------------------------------------------
int GetImageBright()
{
	return _current_bright;
}

//---------------------------------------------------------------------------------
// Ritorna contrasto corrente
//---------------------------------------------------------------------------------
int GetImageContrast()
{
	return _current_contrast;
}

//---------------------------------------------------------------------------------
// Set/Get current camera zoom
//---------------------------------------------------------------------------------
void set_zoom( bool zoom )
{
	VideoDevice* vd;
	if( get_currentcam() == CAMERA_HEAD )
	{
		vd = &videoDev_Head;
		zoom_head = zoom;
	}
	else if( get_currentcam() == CAMERA_EXT )
	{
		vd = &videoDev_Ext;
		zoom_ext = zoom;
	}
	else
	{
		print_debug( "set_zoom: CAMERA not selected!\n" );
		return;
	}

	int cam = pauseLiveVideo();
	vd->StopThread();

	if( zoom )
	{
		vd->SetResolution( VIDEO_W * VIDEO_ZOOM, VIDEO_H * VIDEO_ZOOM );
	}
	else
	{
		vd->SetResolution( VIDEO_W, VIDEO_H );
	}

	vd->StartThread();
	playLiveVideo( cam );
}

bool get_zoom()
{
	if( get_currentcam() == CAMERA_HEAD )
	{
		return zoom_head;
	}
	else if( get_currentcam() == CAMERA_EXT )
	{
		return zoom_ext;
	}

	print_debug( "CAMERA not selected!\n" );
	return false;
}

//---------------------------------------------------------------------------------
// step: passo incremento in mm
//---------------------------------------------------------------------------------
void set_diameter( double step, bool save )
{
	_current_diameter = MID( MIN_DIAMETER_VALUE, _current_diameter+step, MAX_DIAMETER_VALUE );

	// salva su disco
	if( save )
	{
		//CurDat.HeadContrast = _current_contrast;
		//CurData_Write(CurDat);
	}
}

//---------------------------------------------------------------------------------
// step: passo incremento in mm
//---------------------------------------------------------------------------------
void set_tolerance( double step, bool save )
{
	_current_tolerance = MID( MIN_TOLERANCE_VALUE, _current_tolerance+step, MAX_TOLERANCE_VALUE );

	// salva su disco
	if( save )
	{
		//CurDat.HeadContrast = _current_contrast;
		//CurData_Write(CurDat);
	}
}

//---------------------------------------------------------------------------------
// step: passo incremento
//---------------------------------------------------------------------------------
void set_accumulator( double step, bool save )
{
	_current_accumulator = MID( MIN_ACCUMULATOR_VALUE, _current_accumulator+step, MAX_ACCUMULATOR_VALUE );

	// salva su disco
	if( save )
	{
		//CurDat.HeadContrast = _current_contrast;
		//CurData_Write(CurDat);
	}
}

//---------------------------------------------------------------------------------
// step: passo incremento
//---------------------------------------------------------------------------------
void set_smooth( int step, bool save )
{
	_current_smooth = MID( MIN_SMOOTH_VALUE, _current_smooth+step, MAX_SMOOTH_VALUE );

	// salva su disco
	if( save )
	{
		//CurDat.HeadContrast = _current_contrast;
		//CurData_Write(CurDat);
	}
}

//---------------------------------------------------------------------------------
// step: passo incremento
//---------------------------------------------------------------------------------
void set_edge( int step, bool save )
{
	_current_edge = MID( MIN_EDGE_VALUE, _current_edge+step, MAX_EDGE_VALUE );

	// salva su disco
	if( save )
	{
		//CurDat.HeadContrast = _current_contrast;
		//CurData_Write(CurDat);
	}
}

//---------------------------------------------------------------------------------
// step: passo incremento
//---------------------------------------------------------------------------------
void set_area_x( int step, bool save )
{
	_current_area_x = MID( MIN_SEARCHX_VALUE, _current_area_x+step, MAX_SEARCHX_VALUE );

	// salva su disco
	if( save )
	{
		//CurDat.HeadContrast = _current_contrast;
		//CurData_Write(CurDat);
	}
}

//---------------------------------------------------------------------------------
// step: passo incremento
//---------------------------------------------------------------------------------
void set_area_y( int step, bool save )
{
	_current_area_y = MID( MIN_SEARCHY_VALUE, _current_area_y+step, MAX_SEARCHY_VALUE );

	// salva su disco
	if( save )
	{
		//CurDat.HeadContrast = _current_contrast;
		//CurData_Write(CurDat);
	}
}

void set_vectorial_parameters( double diameter, double tolerance, int smooth, int edge, double accumulator, unsigned int areax, unsigned int areay )
{
	_current_diameter = diameter;
	_current_tolerance = tolerance;
	_current_smooth = smooth;
	_current_edge = edge;
	_current_accumulator = accumulator;
	_current_area_x = areax;
	_current_area_y = areay;
}

double get_diameter()
{
	return _current_diameter;
}

double get_tolerance()
{
	return _current_tolerance;
}

double get_accumulator()
{
	return _current_accumulator;
}

int get_smooth()
{
	return _current_smooth;
}

int get_edge()
{
	return _current_edge;
}

unsigned int get_area_x()
{
	return _current_area_x;
}

unsigned int get_area_y()
{
	return _current_area_y;
}


	//------------------------//
	//  Controlli ext camera  //
	//------------------------//

#define EXTCAM_STARTCHAR    ' '
#define EXTCAM_ENDCHAR      0x0D

#define EXTCAM_SETLIGHT     'L'
#define EXTCAM_SETGAIN      'A'
#define EXTCAM_SETSHUTTER   'S'
#define EXTCAM_RESETCMD     'R'

#define MIN_EXTCAM_LIGHT     0
#define MAX_EXTCAM_LIGHT     7

#define MIN_EXTCAM_SHUTTER   0
#define MAX_EXTCAM_SHUTTER   7

#define MIN_EXTCAM_GAIN      0
#define MAX_EXTCAM_GAIN      7


unsigned char _extcam_light = 0;
unsigned char _extcam_gain = 0;
unsigned char _extcam_shutter = 0;

int ExtCam_SendCommand(unsigned char cmd,unsigned char tx_data,unsigned char &rx_data)
{
	if( !(Get_UseCam() && Get_UseExtCam()) )
		return 1;

	if( ComPortExt == NULL )
		return 0;

	char buf[8];
	unsigned char chksum=(tx_data+cmd) | 0x80;
	snprintf( buf, sizeof(buf),"%c%c%d%c%c",EXTCAM_STARTCHAR,cmd,tx_data,chksum,EXTCAM_ENDCHAR);

	ComPortExt->flush();
	int len = strlen( buf );
	ComPortExt->putstring( buf, len );

	unsigned int c;
	for( int i = 0; i < 5; i++ )
	{
		c = ComPortExt->getbyte();

		if( c == SERIAL_ERROR_TIMEOUT )
		{
			//print_debug("Timeout\n");
			return(0);
		}
	
		switch( i )
		{
		case 0:
			if(c!=EXTCAM_STARTCHAR)
			{
				//print_debug("No start %c\n", c );
				return(0);
			}
			break;
		case 1:
			if(c!=cmd)
			{
				//print_debug("No echo %c\n", c );
				return(0);
			}
			break;
		case 2:
			rx_data=c;
			break;
		case 3:
			if(c!=((EXTCAM_STARTCHAR+cmd+rx_data) | 0x80))
			{
				//print_debug("Chksum error Rx=%x(%x)\n",c,(cmd+rx_data) | 0x80);
				return(0);
			}
			break;
		case 4:
			if(c!=EXTCAM_ENDCHAR)
			{
				//print_debug("No End %d\n", c );
				return(0);
			}
			break;
		}
	}

	return 1;
}

const char* GetExtCamPorName(void)
{
	if(ComPortExt != NULL)
	{
		return ComPortExt->GetPort();
	}
	else
	{
		return("NO-NAME [ERROR!]");
	}
}

unsigned int InitExtCamPort(void)
{
	if( !Get_UseCam() )
		return 1;

	if( ComPortExt != NULL )
		return 0;

	ComPortExt = new CommClass( EXTCAM_COM_PORT, EXTCAM_BAUD);
	if( ComPortExt == NULL )
		return 0;

	ComPortExt->open();

	unsigned char dummy;
//  if( !ExtCam_SendCommand(EXTCAM_RESETCMD,0,dummy) )
	if( !ExtCam_SendCommand(EXTCAM_SETLIGHT,0,dummy) )
	{
		ComPortExt->close();
		delete ComPortExt;
		ComPortExt = NULL;
		return 0;
	}

	SetExtCam_Light(EXTCAM_CENTRAL_LIGHT);
	delay(250);
	SetExtCam_Light(EXTCAM_SIDE_LIGHT);
	delay(250);
	SetExtCam_Light(EXTCAM_FULL_LIGHT);
	delay(250);
	SetExtCam_Light(EXTCAM_NOLIGHT);

	return 1;
}

void CloseExtCamPort(void)
{
	if( ComPortExt != NULL )
	{
		ComPortExt->close();
		delete ComPortExt;
		ComPortExt = NULL;
	}
}

unsigned char SetExtCam_Light(unsigned char light,int inc)
{
	unsigned char dummy;

	if(inc)
	{
		if((CurDat.extcam_light==MAX_EXTCAM_LIGHT) && (inc>0))
		{
			light=0;
		}
		else
		{
			if((CurDat.extcam_light==0) && (inc<0))
			{
				light=MAX_EXTCAM_LIGHT;
			}
			else
			{
				light=CurDat.extcam_light+inc;
			}
		}
	}
	else
	{
		if(light>MAX_EXTCAM_LIGHT)
		{
			light=MAX_EXTCAM_LIGHT;
		}
	}

	if(ExtCam_SendCommand(EXTCAM_SETLIGHT,light,dummy))
	{
		CurDat.extcam_light = light;
		_extcam_light = light;
		CurData_Write( CurDat );
	}

	return CurDat.extcam_light;
}

unsigned char SetExtCam_Gain(unsigned char gain,int inc)
{
	unsigned char dummy;

	if(inc)
	{
		if((CurDat.extcam_gain==MAX_EXTCAM_GAIN) && (inc>0))
		{
			gain=0;
		}
		else
		{
			if((CurDat.extcam_gain==0) && (inc<0))
			{
				gain=MAX_EXTCAM_GAIN;
			}
			else
			{
				gain=CurDat.extcam_gain+inc;
			}
		}
	}
	else
	{
		if(gain>MAX_EXTCAM_GAIN)
		{
			gain=MAX_EXTCAM_GAIN;
		}
	}

	if(ExtCam_SendCommand(EXTCAM_SETGAIN,gain,dummy))
	{
		CurDat.extcam_gain = gain;
		_extcam_gain = gain;
		CurData_Write( CurDat );
	}

	return CurDat.extcam_gain;
}

unsigned char SetExtCam_Shutter(unsigned char shutter,int inc)
{
	unsigned char dummy;

	if(inc)
	{
		if((CurDat.extcam_shutter==MAX_EXTCAM_SHUTTER) && (inc>0))
		{
			shutter=0;
		}
		else
		{
			if((CurDat.extcam_shutter==0) && (inc<0))
			{
				shutter=MAX_EXTCAM_SHUTTER;
			}
			else
			{
				shutter=CurDat.extcam_shutter+inc;
			}
		}
	}
	else
	{
		if(shutter>MAX_EXTCAM_SHUTTER)
		{
			shutter=MAX_EXTCAM_SHUTTER;
		}
	}

	if(ExtCam_SendCommand(EXTCAM_SETSHUTTER,shutter,dummy))
	{
		CurDat.extcam_shutter = shutter;
		_extcam_shutter = shutter;
		CurData_Write( CurDat );
	}

	return CurDat.extcam_shutter;
}

unsigned char GetExtCam_Light(void)
{
	return _extcam_light;
}

unsigned char GetExtCam_Gain(void)
{
	return _extcam_gain;
}

unsigned char GetExtCam_Shutter(void)
{
	return _extcam_shutter;
}


//--------------------------------------------------------------------------
// Gestione rettangoli per selezione pattern.
//--------------------------------------------------------------------------
void* selectionBoxBackBuffer = 0;
int selectionBox_X;
int selectionBox_Y;

void ShowSelectionBox( int x0, int y0, int sizex, int sizey )
{
	int x1 = x0-sizex/2;
	int y1 = y0-sizey/2;

	GUI_Freeze_Locker lock;

	GUI_Rect( RectI( x1+1, y1+1, sizex-1, sizey-1 ), GUI_color( 0,0,0 ) );
	GUI_Rect( RectI( x1, y1, sizex+1, sizey+1 ), GUI_color( 255,255,255 ) );
	GUI_Rect( RectI( x1-1, y1-1, sizex+3, sizey+3 ), GUI_color( 0,0,0 ) );
}

void SaveSelectionBoxArea( int x0, int y0, int sizex, int sizey )
{
	if( selectionBoxBackBuffer )
	{
		GUI_FreeSurface( &selectionBoxBackBuffer );
	}

	sizex += 5;
	sizey += 5;

	selectionBox_X = x0-sizex/2;
	selectionBox_Y = y0-sizey/2;

	selectionBoxBackBuffer = GUI_SaveScreen( RectI(selectionBox_X, selectionBox_Y, sizex, sizey) );
}

void RestoreSelectionBoxArea( bool deleteArea )
{
	if( selectionBoxBackBuffer )
	{
		GUI_Freeze_Locker lock;

		GUI_DrawSurface( PointI( selectionBox_X, selectionBox_Y ), selectionBoxBackBuffer );

		if( deleteArea )
		{
			GUI_FreeSurface( &selectionBoxBackBuffer );
		}
	}
}

//--------------------------------------------------------------------------
// Salva immagine sotto rettangolo e visualizza rettangolo sullo schermo
// x0, y0       : posizione centro rettangolo
// sizex, sizey : dimensioni rettangolo
// maxx, maxy   : dimensioni massime rettangolo
//--------------------------------------------------------------------------
void OpenImgBox( int x0, int y0, int sizex, int sizey, int maxx, int maxy )
{
	SaveSelectionBoxArea( x0, y0, maxx, maxy );
	ShowSelectionBox( x0, y0, sizex, sizey );
}

//--------------------------------------------------------------------------
// Ripristina immagine salvata eliminando e dealloca memoria
// x0, y0       : posizione centro rettangolo
// sizex, sizey : dimensioni rettangolo
//--------------------------------------------------------------------------
void CloseImgBox()
{
	RestoreSelectionBoxArea( true );
}

//--------------------------------------------------------------------------
// Visualizza rettangolo sullo schermo eliminando rettangolo precedente
// x0, y0       : posizione centro rettangolo
// sizex, sizey : dimensioni rettangolo
//--------------------------------------------------------------------------
void MoveImgBox( int x0, int y0, int sizex, int sizey )
{
	GUI_Freeze_Locker lock;

	RestoreSelectionBoxArea( false );
	ShowSelectionBox( x0, y0, sizex, sizey );
}

//--------------------------------------------------------------------------
// Visualizza rettangolo sullo schermo
// x0, y0       : posizione centro rettangolo
// sizex, sizey : dimensioni rettangolo
//--------------------------------------------------------------------------
void ShowImgBox( int x0, int y0, int sizex, int sizey )
{
	ShowSelectionBox( x0, y0, sizex, sizey );
}


//---------------------------------------------------------------------------------
// Abilita o disabilita l'immagine della telecamera passata come parametro sul
// video del PC.
// Parametri di ingresso:
//   camera: identificativo della telecamera (valore di default=CAMERA_HEAD)
//   mode: modalita' di attivazione della telecamera
//         0: chiude finestra
//         1: apre finestra e avvia live video con telecamera selezionata
//         2: come modo1 ma impedisce di chiudere la finestra con modo0
//         3: come modo0 ma chiude la finestra anche se aperta con modo2
//         4: aggiorna titolo se finestra aperta
// Valori di ritorno:
//         0: errore
//         1: altrimenti
//---------------------------------------------------------------------------------
int Set_Tv( int mode, int camera, void(*fInfo)( void* parentWin ) )
{
	static int openMode = 0;

	if( camera != CAMERA_HEAD && camera != CAMERA_EXT )
	{
		camera = CAMERA_HEAD;
	}

	switch( mode )
	{
		case 0: // spegne live video e chiude finestra
		case 3:
			if( mode == 0 && openMode == 2 )
				break;

			pauseLiveVideo( true );

			while( !live_video_mutex.try_lock() )
			{
				delay( 1 );
			}

			if( _wLiveVideo )
			{
				delete _wLiveVideo;
				_wLiveVideo = 0;
			}
			openMode = 0;

			live_video_mutex.unlock();
			break;

		case 1: // apre finestra e avvia live video con telecamera selezionata
		case 2:
			// ferma il video per non creare conflitti con l'altro thread
			pauseLiveVideo();

			while( !live_video_mutex.try_lock() )
			{
				delay( 1 );
			}

			if( !_wLiveVideo )
			{
				_wLiveVideo = new CWindow( 0 );
				_wLiveVideo->SetStyle( WIN_STYLE_CENTERED );
				if( videoStyle == VIDEO_NORMAL )
					_wLiveVideo->SetClientAreaSize( LVW_WIDTH_CHARS, LVW_HEIGHT_CHARS );
				else
					_wLiveVideo->SetClientAreaSize( LVW_WIDTH_CHARS_EXP, LVW_HEIGHT_CHARS );
				_wLiveVideo->Show();
			}

			displayLiveVideoFrame( camera, _tvTitle, fInfo );

			// registra il modo di abilitazione
			if( openMode != 2 )
				openMode = mode;

			live_video_mutex.unlock();

			playLiveVideo( camera );
			break;

		case 4:
			if( _wLiveVideo )
			{
				int cam = pauseLiveVideo();

				while( !live_video_mutex.try_lock() )
				{
					delay( 1 );
				}

				_wLiveVideo->SetTitle( _tvTitle );

				live_video_mutex.unlock();

				playLiveVideo( cam, false );
			}
			break;
	}

	return 1;
}

//---------------------------------------------------------------------------------
// Imposta il titolo della finestra visione
// Parametri di ingresso:
//   title: stringa da scrivere a video come titolo
// Valori di ritorno:
//   nessuno
//----------------------------------------------------------------------------------
void Set_Tv_Title( const char* title )
{
	if( title != NULL )
	{
		if( strcmp( title, "" ) )
		{
			snprintf( _tvTitle, TV_TITLE_LEN, "%s", title );
		}
		else
		{
			strcpy( _tvTitle, " " );
		}
		Set_Tv( 4 ); // aggiorna titolo se finestra aperta
	}
}

//---------------------------------------------------------------------------------
// Cattura una immagine e la mostra a video.
// Parametri di ingresso:
//   mode: modalita' di attivazione della telecamera
//         0: chiude finestra
//         1: apre finestra e cattura immagine dalla telecamera in uso
// Valori di ritorno:
//         0: errore
//         1: altrimenti
//---------------------------------------------------------------------------------
int Set_Tv_FromImage( int mode, bool showCross, void(*fInfo)( void* parentWin ) )
{
	int cam;

	switch( mode )
	{
		case 0: // chiude finestra
			while( !live_video_mutex.try_lock() )
			{
				delay( 1 );
			}

			if( _wLiveVideo )
			{
				delete _wLiveVideo;
				_wLiveVideo = 0;
			}

			live_video_mutex.unlock();
			break;

		case 1: // apre finestra e cattura immagine dalla telecamera in uso
			cam = get_currentcam();
			if( cam == NO_CAMERA )
				return 0;

			// led camera on
			if( cam == CAMERA_HEAD )
			{
				Set_HeadCameraLight( 1 );
			}

			while( !live_video_mutex.try_lock() )
			{
				delay( 1 );
			}

			if( !_wLiveVideo )
			{
				_wLiveVideo = new CWindow( 0 );
				_wLiveVideo->SetStyle( WIN_STYLE_CENTERED );
				if( videoStyle == VIDEO_NORMAL )
					_wLiveVideo->SetClientAreaSize( LVW_WIDTH_CHARS, LVW_HEIGHT_CHARS );
				else
					_wLiveVideo->SetClientAreaSize( LVW_WIDTH_CHARS_EXP, LVW_HEIGHT_CHARS );
				_wLiveVideo->Show();

				displayLiveVideoFrame( cam, _tvTitle, fInfo );
			}

			live_video_mutex.unlock();

			captureFramedImageAndDisplay( true );

			// led camera off
			if( cam == CAMERA_HEAD )
			{
				Set_HeadCameraLight( 0 );
			}
			break;
	}

	return 1;
}
