/*
> Q_VISION.H                   <
> Dichiarazioni delle funzioni <
> esterne per gestione visione <

++++            Modulo di automazione QUADRA.               ++++
++++           Tutti i diritti sono riservati.              ++++
++++		!Massimone Corporation!			    ++++
++++			99.10.21			    ++++
*/

#if !defined(__Q_VISION_)
#define __Q_VISION_

#include "q_cost.h"
#include "tv.h"


#define BRD_IMG_MAXX            240
#define BRD_IMG_MAXY            240

#define PKG_BRD_IMG_MAXX        400
#define PKG_BRD_IMG_MAXY        400

#define PACK_PATTERN_MAXX       250
#define PACK_PATTERN_MAXY       250


#define INTERRUPTMSG    MsgGetString(Msg_00763) //Integr. Loris
#define MAPERROR        MsgGetString(Msg_01497)


#define DEFAULT_EXT_CAM_SHUTTER   3
#define DEFAULT_EXT_CAM_GAIN      6

//*************************** FUNZIONI ******************************

//MATCHING GENERICO
bool Image_match( float* x_pos, float* y_pos, int imageType, int matchType = MATCH_CORRELATION, int imageNum = 0 );

//matching per package
bool Image_match_package( int punta,float *x_pos, float *y_pos, int packCode,struct PackVisData pvdat, char* libname , int ImageType, int checkMaxError );

bool ImageMatch_ExtCam( float* x_pos, float* y_pos, int imageType, int nozzle );


// Carica i parametri per la visione
bool load_visionpar();

// Integr. Loris
bool Image_Elabora( int imageType, int imageNum = 0 );
bool ImagePack_Elabora( char* imageName, int imageType );


#define NOCHOOSE     0
#define YESCHOOSE    1
bool ImageCaptureSave( int imageType, int mode = YESCHOOSE, int camera = CAMERA_HEAD, int imageNum = 0 );


// Integr. Loris
int ChoosePattern(int x0, int y0, int &sizex, int &sizey, int step=2,int minx=-1,int miny=-1,int maxx=-1,int maxy=-1);

#endif

