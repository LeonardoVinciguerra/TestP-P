//---------------------------------------------------------------------------
//
// Name:        hough.h
// Author:      Gabriel Ferri
// Created:     20/11/2008
// Description: Hough Transform circle detection functions declaration
//
//---------------------------------------------------------------------------
#ifndef __HOUGH_H
#define __HOUGH_H

#include "opencv2/imgproc/imgproc_c.h"

void houghCircle( CvMat* edges, CvMat* radii, float dp, int& X, int& Y, int& index, float acc_thr_rel );
void houghCircles( CvMat* edges, CvMat* radii, float dp, CvMat* acc, CvMat* indices, float acc_thr_rel, unsigned short filter_dim = 3 );

void houghRectangle( CvMat* edges, CvMat* side_x, CvMat* side_y, float dp, int& X, int& Y, int& index_x, int& index_y, float acc_thr_rel );

#endif
