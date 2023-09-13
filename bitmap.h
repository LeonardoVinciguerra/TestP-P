//----------------------------------------------------------------------
//  BITMAP LIBRARY
//
//  Algoritmi di filtraggio e matching
//----------------------------------------------------------------------

#ifndef __BITMAPLIB_H
#define __BITMAPLIB_H

#include "opencv2/imgproc/imgproc_c.h"
#ifdef __UBUNTU18
#include "opencv2/imgproc/types_c.h"
#endif


//#define BMP_DEBUG


class bitmap
{
	//HOT SPOT
	unsigned int xhspot;
	unsigned int yhspot;

	IplImage* pImage;
	IplImage* pIDebug;
	IplImage* pFrame;

public:
	bitmap(void);
	bitmap(char*);
	bitmap(unsigned int,unsigned int,unsigned char);
	//GRABBER DA VIDEO DEVICE
	bitmap(unsigned int,unsigned int, unsigned int, unsigned int);

	~bitmap();

	//FUNZIONI BASE
	void grabFrame(int xc,int yc);
	void createFromFrame(void* img,int xc,int yc);

	//////////////////////////
	//SALVATAGGIO IN FILE
	bool save(char*);

	bool load(char*);


//FUNZIONI DI MODIFICA

	//////////////////////////
	//TAGLIA LA BITMAP
	bool crop(unsigned int, unsigned int, unsigned int, unsigned int);


//FUNZIONI DI ACCESSO

	//////////////////////////
	unsigned int get_width() { return pImage->width; };
	
	//////////////////////////
	unsigned int get_height() { return pImage->height; };

	//////////////////////////
	unsigned int hsx() { return xhspot; };

	//////////////////////////
	unsigned int hsy() { return yhspot; };


//FUNZIONI DI CONFRONTO

	//////////////////////////
	double matching( bitmap* );


//FUNZIONI DI RICERCA

	//////////////////////////
	bool findCircle( float& posX, float& posY, unsigned short& circleDiameter, int circleTolerance, int filterSmoothDim, int filterEdgeThr, float circleAccum, int showDebug );
	bool findMapPattern( float& posX, float& posY, int circleDiameter, int circleTolerance, int filterSmoothDim, int filterEdgeThr, float circleAccum, int showDebug );
	bool findRotatedRectangle( float& posX, float& posY, float& angle, int rectX, int rectY, int rectTolerance, int filterSmoothDim, int filterBinThrMin, int filterBinThrMax, float filterApprox, int showDebug );


	// ridimensiona bitmap
	void redim(int new_width,int new_height);
	void setscale(float scalex,float scaley);

	// Visualizza la bitmap a video
	void show( int xc, int yc );

	// Visualizza la bitmap di DEBUG a video
	void showDebug( int xc, int yc );

	// Visualizza il frame da cui e' stata estratta l'immagine
	void showFrame( int xc, int yc );
};

#endif //__BITMAPLIB_H
