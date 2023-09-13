#ifndef _TV_H_
#define _TV_H_

#include "q_cost.h"

// immagine elaborata
#define ELAB_VIDEO_W           460
#define ELAB_VIDEO_H           380
#define ELAB_VIDEO_STARTX      850

// Parametri fiduciali
#define DEF_PATTERN_VALUE		80
#define MIN_ATLANTE_VALUE		50
#define DEF_ATLANTE_VALUE		200
#define MIN_ITER_VALUE			1
#define MAX_ITER_VALUE			10
#define DEF_ITER_VALUE			3
#define MIN_THR_VALUE			0.0
#define MAX_THR_VALUE			1.0
#define DEF_THR_VALUE			0.65
#define MIN_FTYPE_VALUE			1
#define MAX_FTYPE_VALUE			9
#define DEF_FTYPE_VALUE			0
#define MIN_FP1_VALUE			1
#define MAX_FP1_VALUE			50
#define DEF_FP1_VALUE			4
#define MIN_FP2_VALUE			1
#define MAX_FP2_VALUE			50
#define DEF_FP2_VALUE			15
#define MIN_FP3_VALUE			1
#define MAX_FP3_VALUE			50
#define DEF_FP3_VALUE			1
#define MIN_BRIGHT_VALUE		10
#define MAX_BRIGHT_VALUE		60000
#define DEF_BRIGHT_VALUE		32768
#define MIN_CONTRAST_VALUE		10
#define MAX_CONTRAST_VALUE		60000
#define DEF_CONTRAST_VALUE		32768

#define MIN_DIAMETER_VALUE		0.2
#define MAX_DIAMETER_VALUE		6.0
#define DEF_DIAMETER_VALUE		1.5
#define MIN_TOLERANCE_VALUE		0.05
#define MAX_TOLERANCE_VALUE		0.3
#define DEF_TOLERANCE_VALUE		0.1
#define MIN_ACCUMULATOR_VALUE	0.1
#define MAX_ACCUMULATOR_VALUE	1.0
#define DEF_ACCUMULATOR_VALUE	0.10
#define MIN_SMOOTH_VALUE		3
#define MAX_SMOOTH_VALUE		15
#define DEF_SMOOTH_VALUE		11
#define MIN_EDGE_VALUE			10
#define MAX_EDGE_VALUE			60
#define DEF_EDGE_VALUE			40
#define MIN_SEARCHX_VALUE		100
#define MAX_SEARCHX_VALUE		ELAB_VIDEO_W
#define MIN_SEARCHY_VALUE		100
#define MAX_SEARCHY_VALUE		ELAB_VIDEO_H
#define DEF_SEARCHX_VALUE		ELAB_VIDEO_W
#define DEF_SEARCHY_VALUE		ELAB_VIDEO_H


	//---------------//
	//  Funzioni TV  //
	//------------- -//

int open_tv( void );
int close_tv( void );

#define NO_CAMERA     -1
#define CAMERA_HEAD    1
#define CAMERA_EXT     3

#define LIGHT_OFF      0
#define LIGHT_ON       1

#define CAMERA_OFF     0
#define CAMERA_ON      1

// Modalita' finestre telecamera (normale o con anche i dati elaborati)
#define VIDEO_NORMAL	0
#define VIDEO_EXPANDED	1

// Modalita' visualizzazione puntatore
#define	 POINTER_NONE	0
#define	 POINTER_CROSS	1
#define	 POINTER_CIRCLE	2


void setLiveVideoCallback( void (*f)( void* parentWin ) );
void showVideoControls( bool show );
void playLiveVideo( int camera, bool loadControl = true );
int pauseLiveVideo( bool turOffLight = false );

void set_style( int style );
void set_pointer( int pointer );
void circle_pointer_movexy( int stepx, int stepy );

void set_currentcam( int camera );
int get_currentcam();

void set_bright_abs( int value, bool save );
void set_bright( int step, bool save = true );
void set_contrast_abs( int value, bool save );
void set_contrast( int step, bool save = true );

void set_zoom( bool zoom );
bool get_zoom();

int GetVideoCenterX();
int GetVideoCenterY();

int getFrameWidth();
int getFrameHeight();
int getFrameBpp();
void captureFrame( void* frame );

void captureFramedImageAndDisplay( bool showCross );

void set_diameter( double step, bool save = true );
void set_tolerance( double step, bool save = true );
void set_accumulator( double step, bool save = true );
void set_smooth( int step, bool save = true );
void set_edge( int step, bool save = true );
void set_area_x( int step, bool save = true );
void set_area_y( int step, bool save = true );
double get_diameter();
double get_tolerance();
double get_accumulator();
int get_smooth();
int get_edge();
unsigned int get_area_x();
unsigned int get_area_y();
void set_vectorial_parameters( double diameter, double tolerance, int smooth, int edge, double accumulator, unsigned int areax, unsigned int areay );

bool findCircleFiducial( float& posX, float& posY, unsigned short& circleDiameter );

#define CROSS_CURR_MODE  0
#define CROSS_NEXT_MODE  1

#define CROSSBOX_PACK    0x01
#define CROSSBOX_EXTCAM  0x02

void setCross( int mode, int boxType = 0 );
void displayCross();
void displayCircle();


// Setta la luminosita' e il contrasto senza salvarlo
void SetImgBrightCont( int bright, int contrast );
int GetImageBright();
int GetImageContrast();


void OpenImgBox(int x0, int y0, int sizex, int sizey, int maxx, int maxy);
void CloseImgBox();
void MoveImgBox(int x0, int y0, int sizex, int sizey);
void ShowImgBox(int x0, int y0, int sizex, int sizey);


// abilita-disabilita telecamera sul video PC
int Set_Tv( int mode, int camera = CAMERA_HEAD, void(*fInfo)( void* parentWin ) = 0 );
// Imposta il titolo usato in Set_Tv e Set_Tv_FromImage
void Set_Tv_Title( const char* title = 0 );
// abilita-disabilita immagine sul video PC
int Set_Tv_FromImage( int mode, bool showCross = false, void(*fInfo)( void* parentWin ) = 0 );



	//--------------------------//
	//  Definizioni ext camera  //
	//--------------------------//

#define EXTCAM_LIGHT_DEFAULT     128
#define EXTCAM_GAIN_DEFAULT      8
#define EXTCAM_SHUTTER_DEFAULT   3

#define EXTCAM_MIN_SHUTTER      2
#define EXTCAM_MAX_SHUTTER      6

#define EXTCAM_NOLIGHT       0
#define EXTCAM_CENTRAL_LIGHT 1
#define EXTCAM_SIDE_LIGHT1   2
#define EXTCAM_SIDE_LIGHT2   4
#define EXTCAM_SIDE_LIGHT    (EXTCAM_SIDE_LIGHT1 | EXTCAM_SIDE_LIGHT2)
#define EXTCAM_FULL_LIGHT    (EXTCAM_SIDE_LIGHT | EXTCAM_CENTRAL_LIGHT)


const char* GetExtCamPorName(void);
unsigned int InitExtCamPort(void);
void CloseExtCamPort(void);

unsigned char SetExtCam_Light(unsigned char light,int inc=0);
unsigned char SetExtCam_Gain(unsigned char gain,int inc=0);
unsigned char SetExtCam_Shutter(unsigned char shutter,int inc=0);
unsigned char GetExtCam_Light(void);
unsigned char GetExtCam_Gain(void);
unsigned char GetExtCam_Shutter(void);

#endif
