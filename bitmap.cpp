//----------------------------------------------
//	BITMAP LIBRARY
//
//	Algoritmi di filtraggio e di matching
//----------------------------------------------
#include "bitmap.h"

#include <opencv/highgui.h>
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <assert.h>

#include "mathlib.h"
#include "tv.h"
#include "q_graph.h"
#include "hough.h"
#include "q_debug.h"
#include "keyutils.h"

#include <mss.h>


//da in uscita STDERR i messaggi di errore
#define BMP_ERRORS


#define COLOR_RGB( r, g, b )  cvScalar( (r), (g), (b), 0 )

//ERROR MESSAGES LIST
#define ERRORMSG_1      "\nFilename specificato non valido!\nErrore 1."
#define ERRORMSG_2      "\nFile "<<nomefile<<" non trovato!\nErrore 2."
#define ERRORMSG_3      "\nErrore nel grabbing. Ingressi non validi!\nErrore 3."

using namespace std;


bitmap::bitmap()
{
	pImage = 0;
	pIDebug = 0;
	pFrame = 0;
}

bitmap::bitmap( unsigned int width, unsigned int height, unsigned char color )
{
	if( width == 0 )
		width = 4;

	if( height == 0 )
		height = 4;

	pImage = cvCreateImage( cvSize( width, height ), 8, 1 );
	cvSet( pImage, cvScalar( color ) );

	xhspot = 0;
	yhspot = 0;

	pIDebug = 0;
	pFrame = 0;
}

//COSTRUTTORE DA FILE
bitmap::bitmap( char* nomefile )
{
	load(nomefile);

	pIDebug = 0;
	pFrame = 0;
}

//COSTRUTTORE DA VIDEO DEVICE
bitmap::bitmap( unsigned int width, unsigned int height, unsigned int centerx, unsigned int centery )
{
	//COTROLLO VALIDITA' INGRESSI
	//MANCA LA CONDIZIONE SUPERIORE CHE DIPENDE DALLA RISOLUZIONE DEL VIDEO
	if((width<=0) || (height<=0) || (centerx<=0) || (centery<=0) || (centerx<width/2) || (centery<height/2) )
	{
		#ifdef BMP_ERRORS
		cerr << ERRORMSG_3;
		#endif
		exit(1);
	}

	// SI RIPORTANO LE COORDINATE A VALORE PARI
	width = width / 2 * 2;
	height = height / 2 * 2;

	pImage = cvCreateImage( cvSize( width, height ), 8, 1 );

	//INIZIALIZZAZIONE AL CENTRO DELLA BITMAP
	xhspot = centerx;
	yhspot = centery;

	pIDebug = 0;
	pFrame = 0;

	grabFrame( centerx, centery );
}


//DSTRUTTORE
bitmap::~bitmap()
{
	if( pImage )
		cvReleaseImage( &pImage );
	if( pIDebug )
		cvReleaseImage( &pIDebug );
	if( pFrame )
		cvReleaseImage( &pFrame );
}

bool bitmap::load(char* nomefile)
{
	if( !nomefile )
	{
		#ifdef BMP_ERRORS
		cerr << ERRORMSG_1;
		#endif
		return false;
	}

	if( ( pImage = cvLoadImage( nomefile, CV_LOAD_IMAGE_GRAYSCALE ) ) == 0 )
	{
		#ifdef BMP_ERRORS
		cerr << ERRORMSG_2;
		#endif
		return false;
	}

	xhspot = pImage->width / 2;
	yhspot = pImage->height / 2;
	return true;
}

void bitmap::grabFrame( int xc, int yc )
{
	if( pImage == NULL )
		return;

	if( pFrame )
	{
		cvReleaseImage( &pFrame );
	}
	pFrame = cvCreateImage( cvSize( getFrameWidth(), getFrameHeight() ), 8, getFrameBpp() );

	captureFrame( pFrame->imageData );

	int src_startx = xc - ( pImage->width / 2 );
	int src_starty = yc - ( pImage->height / 2 );

	cvSetImageROI( pFrame, cvRect( src_startx, src_starty, pImage->width, pImage->height ) );

	if( getFrameBpp() == 1 )
	{
		// image BPP = 1
		cvCopy( pFrame, pImage );
	}
	else
	{
		// image BPP = 3
		cvCvtColor( pFrame, pImage, CV_BGR2GRAY );
	}

	cvResetImageROI( pFrame );
}

void bitmap::createFromFrame(void* img,int xc,int yc)
{
	if( pImage == NULL )
		return;

	if( pFrame )
	{
		cvReleaseImage( &pFrame );
	}
	pFrame = cvCreateImage( cvSize( getFrameWidth(), getFrameHeight() ), 8, getFrameBpp() );

	memcpy( pFrame->imageData, img, getFrameWidth() * getFrameHeight() * getFrameBpp() );

	int src_startx = xc - ( pImage->width / 2 );
	int src_starty = yc - ( pImage->height / 2 );

	cvSetImageROI( pFrame, cvRect( src_startx, src_starty, pImage->width, pImage->height ) );

	if( getFrameBpp() == 1 )
	{
		// image BPP = 1
		cvCopy( pFrame, pImage );
	}
	else
	{
		// image BPP = 3
		cvCvtColor( pFrame, pImage, CV_BGR2GRAY );
	}

	cvResetImageROI( pFrame );
}

bool bitmap::crop(unsigned int x1,unsigned int y1, unsigned int x2,unsigned  int y2)
{
	//CONTROLLO SU VALIDITA' INGRESSI
	if( x1>x2 || y1>y2 || x2>pImage->width || y2>pImage->height || ( (x1-x2==0) && (y2-y1==0) ) )
		return 1;

	unsigned int width = x2 - x1;
	unsigned int height = y2 - y1;

	IplImage* pTemp = cvCreateImage( cvSize( width, height ), 8, 1 );

	cvSetImageROI( pImage, cvRect( x1, y1, pTemp->width, pTemp->height ) );
	cvCopy( pImage, pTemp );

	// Swap
	IplImage *pSwap = pImage;
	pImage = pTemp;
	pTemp = pSwap;

	cvReleaseImage( &pTemp );
	return 0;
}

//----------------------------------------------------------------------------------
// ATTENZIONE !!!
// funziona solo con IplImage del tipo IPL_DEPTH_32F a singolo canale
//----------------------------------------------------------------------------------
void cvMaxLocLim( const IplImage* pVotes, double* max_val, CvPoint* max_loc = 0, double max_lim = 1.0 )
{
	*max_val = 0.0;
	int row_max = 0;
	int col_max = 0;

	for( int row = 0; row < pVotes->height; row++ )
	{
		float *val_ptr = (float*)((unsigned char*)pVotes->imageDataOrigin + row * pVotes->widthStep);

		for( int col = 0; col < pVotes->width; col++ )
		{
			if( *val_ptr > *max_val && *val_ptr <= max_lim )
			{
				*max_val = *val_ptr;
				col_max = col;
				row_max = row;
			}
			val_ptr++;
		}
	}

	if( max_loc )
	{
		max_loc->x = col_max;
		max_loc->y = row_max;
	}
}

double bitmap::matching( bitmap* pattern )
{
	if( !pattern )
	{
		return 0.0;
	}

	IplImage* pTemplateImage = pattern->pImage;

	// create votes matrix
	int vote_w = pImage->width - pTemplateImage->width + 1;
	int vote_h = pImage->height - pTemplateImage->height + 1;
	IplImage* votes = cvCreateImage( cvSize( vote_w, vote_h ), IPL_DEPTH_32F, 1 );

	cvMatchTemplate( pImage, pTemplateImage, votes, CV_TM_CCOEFF_NORMED );

	// find max value in the "votes"-image and its positions
	CvPoint pp;
	double maxVal;
	cvMaxLocLim( votes, &maxVal, &pp, 0.99 );

	xhspot = pp.x + pTemplateImage->width/2;
	yhspot = pImage->height - ( pp.y + pTemplateImage->height/2 );

	cvReleaseImage( &votes );

	return maxVal;
}


//----------------------------------------------------------------------------------
// Find the most like circle shape
// showDebug: 0x0001 mostra elaborazione finale
//            0x0002 mostra immagine filtro+edge
//----------------------------------------------------------------------------------
bool bitmap::findCircle( float& posX, float& posY, unsigned short& circleDiameter, int circleTolerance, int filterSmoothDim, int filterEdgeThr, float circleAccum, int showDebug )
{
	// cropping
	//-----------------------------------------------------
	int roi_x = 0;
	int roi_y = 0;
	int roi_w = pImage->width - 2 * roi_x;
	int roi_h = pImage->height - 2 * roi_y;

	IplImage* crop = cvCreateImage( cvSize(roi_w, roi_h), IPL_DEPTH_8U, 1 );

	cvSetImageROI( pImage, cvRect( roi_x, roi_y, roi_w, roi_h ) );
	cvCopy( pImage, crop );
	cvResetImageROI( pImage );


	// filtering
	//-----------------------------------------------------

	// improve edge detection reducing image background noise
	int fsd = filterSmoothDim | 0x00000001;
	cvSmooth( crop, crop, CV_GAUSSIAN, fsd, fsd );

	// edge detection
	IplImage* edges = cvCreateImage( cvSize(crop->width, crop->height), IPL_DEPTH_8U, 1 );
	cvCanny( crop, edges, filterEdgeThr/2, filterEdgeThr, 3 );


	// debug
	//-----------------------------------------------------
	if( showDebug & 0x0002 )
	{
		if( pIDebug )
			cvReleaseImage( &pIDebug );

		IplImage* pItemp = cvCloneImage( pImage );

		cvSetImageROI( pItemp, cvRect( roi_x, roi_y, roi_w, roi_h ) );
		cvCopy( edges, pItemp );
		cvResetImageROI( pItemp );

		pIDebug = cvCreateImage( cvSize( pImage->width, pImage->height ), 8, 3 );
		cvCvtColor( pItemp, pIDebug, CV_GRAY2RGB );

		cvReleaseImage( &pItemp );
		cvReleaseImage( &crop );
		cvReleaseImage( &edges );

		posX = pImage->width / 2.0f;
		posY = pImage->height / 2.0f;
		return true;
	}


	// setup data
	//-----------------------------------------------------

	// radii array
	int minRadius = ( circleDiameter - circleTolerance ) / 2.0;
	CvMat* radii = cvCreateMat( 1, 2*circleTolerance+1, CV_32F );
	cvRange( radii, minRadius, minRadius + radii->cols );

	// edges matrix
	CvMat stub, *edges_mat = (CvMat*)edges;
	edges_mat = cvGetMat( edges_mat, &stub );

	// resolution
	float dp = 0.5;


	// search circle
	//-----------------------------------------------------
	int x, y, index;
	houghCircle( edges_mat, radii, dp, x, y, index, circleAccum );

	if( index != -1 )
	{
		// returned values are in ROI coordinates
		posX = x*dp + roi_x;
		posY = y*dp + roi_y;
		circleDiameter = cvGetReal1D( radii, index ) * 2;

		// change coordinates
		posY = pImage->height - posY;
	}

	// Clean
	cvReleaseImage( &crop );
	cvReleaseImage( &edges );
	cvReleaseMat( &radii );


	#ifdef BMP_DEBUG
	if( 1 )
	#else
	if( showDebug & 0x0001 )
	#endif
	{
		// prepare image to display
		//-----------------------------------------------------
		if( pIDebug )
			cvReleaseImage( &pIDebug );

		pIDebug = cvCreateImage( cvSize( pImage->width, pImage->height ), 8, 3 );
		cvCvtColor( pImage, pIDebug, CV_GRAY2RGB );

		if( index != -1 )
		{
			cvCircle( pIDebug, cvPoint( cvRound(posX), cvRound(pImage->height - posY) ), ftoi(circleDiameter/2.f), COLOR_RGB(0,255,0) );
		}
	}

	return index == -1 ? false : true;
}


//----------------------------------------------------------------------------------
// Find the map pattern used to calibrate the camera offset
//----------------------------------------------------------------------------------

#define PATTERN_CIRCLES				7 // numero massimo di punti riconosciuti nell'immagine
#define PATTERN_PPL					3 // numero massimo di punti utilizzati per il riconoscimento della linea
#define PATTERN_FILTER_DIM			2 // dimensione filtro funzione houghCircles
#define PATTERN_MELT_DISTANCE		(PATTERN_FILTER_DIM + 5) // distanza entro la quale si fa la media dei cerchi
#define PATTERN_ALIGN_TOLERANCE		10 // distanza massima (pixel) per considerare i punti allineati
#define PATTERN_ALIGN_MIN			2 // numero minimo di punti allineati
#define PATTERN_NORMAL_ERROR		2.f // errore (gradi) di perpendicolarita' massimo

struct CirclePattern
{
	CirclePattern()
	{
		value = 0;
		ok = false;
	};

	float X;
	float Y;
	float R;

	int value;
	bool ok;
};


//----------------------------------------------------------------------------------
// showDebug: 0x0001 mostra elaborazione finale
//            0x0002 mostra immagine filtro+edge
//            0x0004 mostra riconoscimento cerchi
//            0x0008 mostra riconoscimento cerchi dopo filtraggio
//----------------------------------------------------------------------------------
bool bitmap::findMapPattern( float& posX, float& posY, int circleDiameter, int circleTolerance, int filterSmoothDim, int filterEdgeThr, float circleAccum, int showDebug )
{
	bool isError = false;

	// cropping
	//-----------------------------------------------------
	int roi_x = 0;
	int roi_y = 0;
	int roi_w = pImage->width - 2 * roi_x;
	int roi_h = pImage->height - 2 * roi_y;

	IplImage* crop = cvCreateImage( cvSize(roi_w, roi_h), IPL_DEPTH_8U, 1 );

	cvSetImageROI( pImage, cvRect( roi_x, roi_y, roi_w, roi_h ) );
	cvCopy( pImage, crop );
	cvResetImageROI( pImage );


	// filtering
	//-----------------------------------------------------

	// improve edge detection reducing image background noise
	int fsd = filterSmoothDim | 0x00000001;
	cvSmooth( crop, crop, CV_GAUSSIAN, fsd, fsd );

	// edge detection
	IplImage* edges = cvCreateImage( cvSize(crop->width, crop->height), IPL_DEPTH_8U, 1 );
	cvCanny( crop, edges, filterEdgeThr/2, filterEdgeThr, 3 );


	// debug
	//-----------------------------------------------------
	if( showDebug & 0x0002 )
	{
		if( pIDebug )
			cvReleaseImage( &pIDebug );

		IplImage* pItemp = cvCloneImage( pImage );

		cvSetImageROI( pItemp, cvRect( roi_x, roi_y, roi_w, roi_h ) );
		cvCopy( edges, pItemp );
		cvResetImageROI( pItemp );

		pIDebug = cvCreateImage( cvSize( pImage->width, pImage->height ), 8, 3 );
		cvCvtColor( pItemp, pIDebug, CV_GRAY2RGB );

		cvReleaseImage( &pItemp );
		cvReleaseImage( &crop );
		cvReleaseImage( &edges );

		posX = pImage->width / 2.0f;
		posY = pImage->height / 2.0f;
		return true;
	}


	// setup data
	//-----------------------------------------------------

	// radii array
	int min_radius = ( circleDiameter - circleTolerance ) / 2.0;
	CvMat* radii = cvCreateMat( 1, 2*circleTolerance+1, CV_32F );
	cvRange( radii, min_radius, min_radius + radii->cols );

	// edges matrix
	CvMat stub, *edges_mat = (CvMat*)edges;
	edges_mat = cvGetMat( edges_mat, &stub );

	// resolution
	float dp = 1.f;
	float idp = 1.f / dp;

	int acc_rows = crop->height * idp;
	int acc_cols = crop->width * idp;

	// accumulator matrix
	CvMat* acc = cvCreateMat( acc_rows, acc_cols, CV_16UC1 );
	// indices matrix
	CvMat* indices = cvCreateMat( acc_rows, acc_cols, CV_8UC1 );

	// search circles
	//-----------------------------------------------------
	houghCircles( edges_mat, radii, dp, acc, indices, circleAccum, PATTERN_FILTER_DIM );

	CirclePattern circlesArray[PATTERN_CIRCLES];
	for( int y = 0; y < acc_rows; y++ )
	{
		for( int x = 0; x < acc_cols; x++ )
		{
			int value = cvGet2D( acc, y, x ).val[0];

			for( int i = 0; i < PATTERN_CIRCLES ; i++ )
			{
				if( value > circlesArray[i].value )
				{
					if( i == 0 )
					{
						circlesArray[i].value = value;
						circlesArray[i].ok = true;
						// returned values are in ROI coordinates
						circlesArray[i].X = x*dp + roi_x;
						circlesArray[i].Y = y*dp + roi_y;
						int index = cvGet2D( indices, y, x ).val[0];
						circlesArray[i].R = cvGetReal1D( radii, index );
					}
					else
					{
						CirclePattern tc = circlesArray[i];
						circlesArray[i] = circlesArray[i-1];
						circlesArray[i-1] = tc;
					}
				}
				else
				{
					break;
				}
			}
		}
	}

	// clean
	cvReleaseImage( &crop );
	cvReleaseImage( &edges );
	cvReleaseMat( &acc );
	cvReleaseMat( &indices );
	cvReleaseMat( &radii );


	// debug
	//-----------------------------------------------------
	if( showDebug & 0x0004 )
	{
		if( pIDebug )
			cvReleaseImage( &pIDebug );

		pIDebug = cvCreateImage( cvSize( pImage->width, pImage->height ), 8, 3 );
		cvCvtColor( pImage, pIDebug, CV_GRAY2RGB );

		// circles (GREEN)
		for( int i = 0; i < PATTERN_CIRCLES; i++ )
		{
			if( circlesArray[i].ok )
			{
				cvCircle( pIDebug, cvPoint( cvRound(circlesArray[i].X), cvRound(circlesArray[i].Y) ), circlesArray[i].R, COLOR_RGB(0,255,0) );

				print_debug( "[%d] FindCircle   X = %.2f   Y = %.2f   R = %.2f\n", i, circlesArray[i].X, circlesArray[i].Y, circlesArray[i].R );
			}
		}

		posX = pImage->width / 2.0f;
		posY = pImage->height / 2.0f;
		return true;
	}


	//  points check : relative distances
	//-----------------------------------------------------

	if( 1 )
	{
		int melt_distance = PATTERN_MELT_DISTANCE;
		
		for( int i1 = 0; i1 < PATTERN_CIRCLES; i1++ )
		{
			if( !circlesArray[i1].ok )
				continue;
			
			CirclePattern circlesAvg;
			memset( &circlesAvg, 0, sizeof(CirclePattern) );
			int numCircles = 1;
			circlesAvg.X += circlesArray[i1].X;
			circlesAvg.Y += circlesArray[i1].Y;
			circlesAvg.R += circlesArray[i1].R;
			
			for( int i2 = 0; i2 < PATTERN_CIRCLES; i2++ )
			{
				if( !circlesArray[i2].ok || i1 == i2 )
					continue;
				
				if( fabs( circlesArray[i1].X - circlesArray[i2].X ) < melt_distance &&
					fabs( circlesArray[i1].Y - circlesArray[i2].Y ) < melt_distance )
				{
					circlesArray[i1].ok = false;
					circlesArray[i2].ok = false;
					
					circlesAvg.X += circlesArray[i2].X;
					circlesAvg.Y += circlesArray[i2].Y;
					circlesAvg.R += circlesArray[i2].R;
					numCircles++;
				}
			}
			
			if( !circlesArray[i1].ok )
			{
				circlesArray[i1].ok = true;
				
				circlesArray[i1].X = circlesAvg.X / numCircles;
				circlesArray[i1].Y = circlesAvg.Y / numCircles;
				circlesArray[i1].R = circlesAvg.R / numCircles;
			}
		}
	}

	if( 1 )
	{
		int min_distance = circleDiameter;
		bool circlesArrayRemove[PATTERN_CIRCLES];
		memset( circlesArrayRemove, 0, sizeof(bool) * PATTERN_CIRCLES );
		
		for( int i1 = 0; i1 < PATTERN_CIRCLES; i1++ )
		{
			if( !circlesArray[i1].ok )
				continue;
			
			for( int i2 = 0; i2 < PATTERN_CIRCLES; i2++ )
			{
				if( !circlesArray[i2].ok || i1 == i2 )
					continue;
				
				if( fabs( circlesArray[i1].X - circlesArray[i2].X ) < min_distance &&
					fabs( circlesArray[i1].Y - circlesArray[i2].Y ) < min_distance )
				{
					circlesArrayRemove[i1] = true;
					break;
				}
			}
		}
		
		for( int i1 = 0; i1 < PATTERN_CIRCLES; i1++ )
		{
			if( circlesArrayRemove[i1] )
				circlesArray[i1].ok = false;
		}
	}



	// debug
	//-----------------------------------------------------
	if( showDebug & 0x0008 )
	{
		if( pIDebug )
			cvReleaseImage( &pIDebug );

		// circles (GREEN)
		for( int i = 0; i < PATTERN_CIRCLES; i++ )
		{
			if( circlesArray[i].ok )
			{
				cvCircle( pIDebug, cvPoint( cvRound(circlesArray[i].X), cvRound(circlesArray[i].Y) ), circlesArray[i].R, COLOR_RGB(255,0,0) );

				print_debug( "[%d] FindCircle   X = %.2f   Y = %.2f   R = %.2f\n", i, circlesArray[i].X, circlesArray[i].Y, circlesArray[i].R );
			}
		}

		posX = pImage->width / 2.0f;
		posY = pImage->height / 2.0f;
		return true;
	}


	//  points alignment
	//-----------------------------------------------------
	// Un punto e' preso in considerazione se e' in linea con almeno altri (PATTERN_ALIGN_MIN-1) punti

	int lineX_i = 0, lineY_i = 0;
	int lineX[PATTERN_PPL];
	int lineY[PATTERN_PPL];

	// lineX - orizzontale
	for( int i1 = 0; i1 < PATTERN_CIRCLES; i1++ )
	{
		if( !circlesArray[i1].ok )
			continue;

		int matching = 0;
		for( int i2 = 0; i2 < PATTERN_CIRCLES; i2++ )
		{
			if( !circlesArray[i2].ok || i1 == i2 )
				continue;

			if( fabs( circlesArray[i1].Y - circlesArray[i2].Y ) < PATTERN_ALIGN_TOLERANCE )
			{
				matching++;
				if( matching == PATTERN_ALIGN_MIN - 1 )
				{
					lineX[lineX_i] = i1;
					lineX_i++;

					if( lineX_i == PATTERN_PPL )
					{
						// exit condition
						i1 = i2 = PATTERN_CIRCLES;
					}
					break;
				}
			}
		}
	}

	// lineY - vertical
	for( int i1 = 0; i1 < PATTERN_CIRCLES; i1++ )
	{
		if( !circlesArray[i1].ok )
			continue;

		int matching = 0;
		for( int i2 = 0; i2 < PATTERN_CIRCLES; i2++ )
		{
			if( !circlesArray[i2].ok || i1 == i2 )
				continue;

			if( fabs( circlesArray[i1].X - circlesArray[i2].X ) < PATTERN_ALIGN_TOLERANCE )
			{
				matching++;
				if( matching == PATTERN_ALIGN_MIN - 1 )
				{
					lineY[lineY_i] = i1;
					lineY_i++;

					if( lineY_i == PATTERN_PPL )
					{
						// exit condition
						i1 = i2 = PATTERN_CIRCLES;
					}
					break;
				}
			}
		}
	}

	if( lineX_i < PATTERN_ALIGN_MIN || lineY_i < PATTERN_ALIGN_MIN )
		isError = true;

	if( isError )
		return false;


	//  fit lines
	//-----------------------------------------------------
	float lineX_fit[4], lineY_fit[4]; // [ Vx, Vy, x, y ]
	CvMat* points;

	// lineX - orizzontale
	points = cvCreateMat( 1, lineX_i, CV_32FC2 );
	for( int counter = 0; counter < lineX_i; counter++ )
		cvSet2D( points, 0, counter, cvScalar( circlesArray[lineX[counter]].X, circlesArray[lineX[counter]].Y ) );
	cvFitLine( points, CV_DIST_L2, 0, 0.01, 0.01, lineX_fit );
	cvReleaseMat( &points );

	// lineY - vertical
	points = cvCreateMat( 1, lineY_i, CV_32FC2 );
	for( int counter = 0; counter < lineY_i; counter++ )
		cvSet2D( points, 0, counter, cvScalar( circlesArray[lineY[counter]].X, circlesArray[lineY[counter]].Y ) );
	cvFitLine( points, CV_DIST_L2, 0, 0.01, 0.01, lineY_fit );
	cvReleaseMat( &points );


	//  Normal check
	//-----------------------------------------------------
	float a1, a2;

	if( lineX_fit[0] != 0.f ) //TEMP - da rivedere controllo
		a1 = atan( lineX_fit[1] / lineX_fit[0] );
	else
		isError = true;

	if( lineY_fit[1] != 0.f )
		a2 = -atan( lineY_fit[0] / lineY_fit[1] );
	else
		isError = true;

	if( RTOD(fabs(a1-a2)) > PATTERN_NORMAL_ERROR )
		isError = true;

	if( isError )
		return false;


	//  intersection
	//-----------------------------------------------------

	// num2 = Vx1*(y2-y1) - Vy1*(x2-x1)
	float num2 = lineX_fit[0]*(lineY_fit[3]-lineX_fit[3]) - lineX_fit[1]*(lineY_fit[2]-lineX_fit[2]);
	// den2 = Vx2*Vy1 - Vx1*Vy2
	float den2 = lineY_fit[0]*lineX_fit[1] - lineX_fit[0]*lineY_fit[1];
	if( den2 == 0 )
		isError = true;
	else
	{
		float K2 = num2 / den2;
		posX = lineY_fit[2] + K2 * lineY_fit[0];
		posY = lineY_fit[3] + K2 * lineY_fit[1];

		// change coordinates
		posY = pImage->height - posY;
	}

	#ifdef BMP_DEBUG
	if( 1 )
	#else
	if( showDebug & 0x0001 )
	#endif
	{
		// prepare image to display
		//-----------------------------------------------------
		if( pIDebug )
			cvReleaseImage( &pIDebug );

		pIDebug = cvCreateImage( cvSize( pImage->width, pImage->height ), 8, 3 );
		cvCvtColor( pImage, pIDebug, CV_GRAY2RGB );

		// circles (GREEN)
		for( int i = 0; i < PATTERN_CIRCLES; i++ )
		{
			if( circlesArray[i].ok )
			{
				for( int counter = 0; counter < lineX_i; counter++ )
				{
					if( lineX[counter] == i )
					{
						cvCircle( pIDebug, cvPoint( cvRound(circlesArray[i].X), cvRound(circlesArray[i].Y) ), circlesArray[i].R, COLOR_RGB(0,255,0) );
						break;
					}
				}

				for( int counter = 0; counter < lineY_i; counter++ )
				{
					if( lineY[counter] == i )
					{
						cvCircle( pIDebug, cvPoint( cvRound(circlesArray[i].X), cvRound(circlesArray[i].Y) ), circlesArray[i].R, COLOR_RGB(0,255,0) );
						break;
					}
				}
			}
		}

		// lines (ORANGE)
		cvLine( pIDebug,
				cvPoint( cvRound(lineX_fit[2]+lineX_fit[0]*500),cvRound(lineX_fit[3]+lineX_fit[1]*500) ),
				cvPoint( cvRound(lineX_fit[2]-lineX_fit[0]*500),cvRound(lineX_fit[3]-lineX_fit[1]*500) ),
				COLOR_RGB(255,128,0) );
		cvLine( pIDebug,
				cvPoint( cvRound(lineY_fit[2]+lineY_fit[0]*500),cvRound(lineY_fit[3]+lineY_fit[1]*500) ),
				cvPoint( cvRound(lineY_fit[2]-lineY_fit[0]*500),cvRound(lineY_fit[3]-lineY_fit[1]*500) ),
				COLOR_RGB(255,128,0) );

		// intersection (CYAN)
		cvCircle( pIDebug, cvPoint( cvRound(posX), cvRound(pImage->height - posY) ), 2, COLOR_RGB(0,128,128), CV_FILLED );
	}

	return !isError;
}

//----------------------------------------------------------------------------------
// Find rotated rectangle
// showDebug: 0x0001 mostra elaborazione finale
//            0x0002 mostra immagine filtro+binarization
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Finds a cosine of angle between vectors
//----------------------------------------------------------------------------------
double _angle( cv::Point2f pt0, cv::Point2f pt1, cv::Point2f pt2 )
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

bool bitmap::findRotatedRectangle( float& posX, float& posY, float& angle, int rectX, int rectY, int rectTolerance, int filterSmoothDim, int filterBinThrMin, int filterBinThrMax, float filterApprox, int showDebug )
{
#ifdef __UBUNTU18
	cv::Mat imgM = cv::cvarrToMat( pImage );
#else
	cv::Mat imgM( pImage );
#endif

	// cropping
	//-----------------------------------------------------
	int roi_x = 0;
	int roi_y = 0;
	int roi_w = imgM.size().width - 2 * roi_x;
	int roi_h = imgM.size().height - 2 * roi_y;

	cv::Mat imgROI( imgM, cv::Rect( roi_x, roi_y, roi_w, roi_h ) );
	cv::Mat crop = imgROI.clone();


	// filtering
	//-----------------------------------------------------

	// improve edge detection reducing image background noise
	int fsd = filterSmoothDim | 0x00000001;
	cv::GaussianBlur( crop, crop, cv::Size( fsd, fsd ), 0, 0 );

	// binarization
	cv::Mat bin;

	cv::Mat dst1;
	cv::threshold( crop, dst1, filterBinThrMin, 255, CV_THRESH_BINARY_INV );
	cv::Mat dst2;
	cv::threshold( crop, dst2, filterBinThrMax, 255, CV_THRESH_BINARY );

	bin = dst1 + dst2;

	// erode and dilate
	int erode_type = cv::MORPH_RECT;
	int erode_size = 5;
	cv::Mat element = cv::getStructuringElement( erode_type, cv::Size( 2*erode_size + 1, 2*erode_size+1 ), cv::Point( erode_size, erode_size ) );

	cv::dilate( bin, bin, element );
	cv::erode( bin, bin, element );


	// debug
	//-----------------------------------------------------
	if( showDebug & 0x0002 )
	{
		if( pIDebug )
		{
			cvReleaseImage( &pIDebug );
		}
		pIDebug = cvCreateImage( cvSize( imgM.size().width, imgM.size().height ), 8, 3 );
#ifdef __UBUNTU18
		cv::Mat debugM = cv::cvarrToMat( pIDebug );
#else
		cv::Mat debugM( pIDebug );
#endif

		// copy image to temp
		cv::Mat imgTemp = imgM.clone();

		// copy bin image to temp
		cv::Mat imgTempROI( imgTemp, cv::Rect( roi_x, roi_y, roi_w, roi_h ) );
		bin.copyTo( imgTempROI );

		// convert temp to debug
		cvtColor( imgTemp, debugM, CV_GRAY2RGB );

		posX = imgM.size().width / 2.0f;
		posY = imgM.size().height / 2.0f;
		return true;
	}


	// setup data
	//-----------------------------------------------------
	std::vector<std::vector<cv::Point> > contours;
	std::vector<std::vector<cv::Point> > approxContours;
	std::vector<cv::Vec4i> hierarchy;


	// find contours
	//-----------------------------------------------------
	cv::findContours( bin, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

	// Approximate contours to polygons
	//-----------------------------------------------------
	if( filterApprox > 0 )
	{
		approxContours.resize( contours.size() );

		for( unsigned int i = 0; i < contours.size(); i++ )
		{
			cv::approxPolyDP( cv::Mat(contours[i]), approxContours[i], filterApprox*10.f, true );
		}
	}
	else
	{
		approxContours = contours;
	}

	// Find the rotated rectangles for each contour
	//-----------------------------------------------------
	int minArea = (rectX-rectTolerance)*(rectY-rectTolerance);
	int maxArea = (rectX+rectTolerance)*(rectY+rectTolerance);

	std::vector<cv::RotatedRect> rotRects;

	for( unsigned int i = 0; i < approxContours.size(); i++ )
	{
		// square contours should have:
		// a) 4 vertices after approximation
		if( approxContours[i].size() != 4 )
		{
			printf( " NOT (vertices): %d\n", approxContours[i].size() );
			continue;
		}

		// b) be convex
		if( !cv::isContourConvex(approxContours[i]) )
		{
			printf( " NOT (convexity)\n" );
			continue;
		}

		// c) cosines of all angles be small
		double s = 0.0;
		for( int j = 0; j < 4; j++ )
		{
			double t = fabs( _angle( approxContours[i][j], approxContours[i][(j+1)%4], approxContours[i][(j+3)%4] ) );
			s = s > t ? s : t;
		}
		if( s >= 0.2 )
		{
			printf( " NOT (cosine angles): %.2f\n", s );
			continue;
		}

		// d) area inside the limits
		double area = cv::contourArea( approxContours[i], false );
		if( area < minArea || area > maxArea )
		{
			printf( " NOT (area): %d < %d < %d\n", minArea, int(area), maxArea );
			continue;
		}

		//TODO check su dimensione lati

		// ----  OK  ----

		// rotated rectangle
		cv::RotatedRect minRect = cv::minAreaRect( cv::Mat(approxContours[i]) );

		printf( "Rect: %.1f, %.1f, %.1f (%.1f, %.1f)\n", minRect.center.x, minRect.center.y, minRect.angle, minRect.size.width, minRect.size.height );

		rotRects.push_back( minRect );
	}
	printf( "--------------------\n" );


	// find most suitable rects
	//-----------------------------------------------------
	PointI center( imgM.size().width / 2.0f, imgM.size().height / 2.0f );
	float minDist = 999999;
	int found = -1;

	for( unsigned int i = 0; i < rotRects.size(); i++ )
	{
		float dist = sqrt( (rotRects[i].center.x-center.X)*(rotRects[i].center.x-center.X) + (rotRects[i].center.y-center.Y)*(rotRects[i].center.y-center.Y) );
		if( dist < minDist )
		{
			minDist = dist;
			found = i;
		}
	}

	if( found != -1 )
	{
		posX = rotRects[found].center.x;
		posY = rotRects[found].center.y;
		angle = rotRects[found].angle;

		// change coordinates
		posY = imgM.size().height - posY;
	}


	#ifdef BMP_DEBUG
	if( 1 )
	#else
	if( showDebug & 0x0001 )
	#endif
	{
		if( pIDebug )
		{
			cvReleaseImage( &pIDebug );
		}
		pIDebug = cvCreateImage( cvSize( pImage->width, pImage->height ), 8, 3 );
#ifdef __UBUNTU18
		cv::Mat debugM = cv::cvarrToMat( pIDebug );
#else
		cv::Mat debugM( pIDebug );
#endif

		cvCvtColor( pImage, pIDebug, CV_GRAY2RGB );

		cv::drawContours( debugM, approxContours, -1, COLOR_RGB(0,160,255), 1, 8, hierarchy );

		for( unsigned int i = 0; i < rotRects.size(); i++ )
		{
			// get vertices
			cv::Point2f rect_points[4];
			rotRects[i].points( rect_points );

			// draw results
			for( int j = 0; j < 4; j++ )
			{
				cv::line( debugM, rect_points[j], rect_points[(j+1)%4], (i == found) ? COLOR_RGB(0,255,0) : COLOR_RGB(255,154,0) );
			}
			cv::circle( debugM, rotRects[i].center, 3, (i == found) ? COLOR_RGB(0,255,0) : COLOR_RGB(255,154,0), CV_FILLED );
		}
	}

	return (found != -1 ) ? true : false;
}


bool bitmap::save(char* nomefile)
{
	if( !nomefile )
	{
		return false;
	}

	// elimina nomefile se esiste
	char buff[MAXNPATH];
	strcpy( buff, nomefile );
	strcat( buff, ".bmp" );

	cvSaveImage( buff, pImage );

	// remove nomefile
	remove( nomefile );

	// rename nomefile.bmp in nomefile
	rename( buff, nomefile );

	return true;
}

void bitmap::setscale(float scalex,float scaley)
{
	int new_width = ftoi( pImage->width / scalex );
	int new_height = ftoi( pImage->height / scaley );
	
	redim( new_width, new_height );
}

void bitmap::redim(int new_width,int new_height)
{
	IplImage* pNewImage;
	pNewImage = cvCreateImage( cvSize( new_width, new_height ), 8, 1 );
	
	cvResize( pImage, pNewImage );
	
	cvReleaseImage( &pImage );
	pImage = pNewImage;
}

// Visualizza la bitmap
void bitmap::show( int xc, int yc )
{
	void* _img;
	if( pImage->nChannels == 1 )
	{
		_img = SDL_CreateRGBSurfaceFrom( pImage->imageDataOrigin, pImage->width, pImage->height, 8, pImage->widthStep, 0x000000FF, 0x000000FF, 0x000000FF, 0 );
	}
	else
	{
		_img = SDL_CreateRGBSurfaceFrom( pImage->imageDataOrigin, pImage->width, pImage->height, 24, pImage->widthStep, 0x000000FF, 0x0000FF00, 0x00FF0000, 0 );
	}

	GUI_DrawSurface( PointI( xc-pImage->width/2, yc-pImage->height/2 ), _img );
	GUI_FreeSurface( &_img );
}

// Visualizza la bitmap di DEBUG
void bitmap::showDebug( int xc, int yc )
{
	if( !pIDebug )
		return;

	void* _img;
	if( pIDebug->nChannels == 1 )
	{
		_img = SDL_CreateRGBSurfaceFrom( pIDebug->imageDataOrigin, pIDebug->width, pIDebug->height, 8, pIDebug->widthStep, 0x000000FF, 0x000000FF, 0x000000FF, 0 );
	}
	else
	{
		_img = SDL_CreateRGBSurfaceFrom( pIDebug->imageDataOrigin, pIDebug->width, pIDebug->height, 24, pIDebug->widthStep, 0x000000FF, 0x0000FF00, 0x00FF0000, 0 );
	}

	GUI_DrawSurface( PointI( xc-pIDebug->width/2, yc-pIDebug->height/2 ), _img );
	GUI_FreeSurface( &_img );
}

// Visualizza la bitmap di DEBUG
void bitmap::showFrame( int xc, int yc )
{
	if( !pFrame )
		return;

	void* _img;
	if( pFrame->nChannels == 1 )
	{
		_img = SDL_CreateRGBSurfaceFrom( pFrame->imageDataOrigin, pFrame->width, pFrame->height, 8, pFrame->widthStep, 0x000000FF, 0x000000FF, 0x000000FF, 0 );
	}
	else
	{
		_img = SDL_CreateRGBSurfaceFrom( pFrame->imageDataOrigin, pFrame->width, pFrame->height, 24, pFrame->widthStep, 0x000000FF, 0x0000FF00, 0x00FF0000, 0 );
	}

	GUI_DrawSurface( PointI( xc-pFrame->width/2, yc-pFrame->height/2 ), _img );
	GUI_FreeSurface( &_img );
}
