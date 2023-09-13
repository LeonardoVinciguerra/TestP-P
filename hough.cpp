//---------------------------------------------------------------------------
//
// Name:        hough.cpp
// Author:      Gabriel Ferri
// Created:     20/11/2008
// Description: Hough Transform circle detection functions implementation
//
//---------------------------------------------------------------------------
#include "hough.h"

#include <math.h>

#include <mss.h>


//----------------------------------------------------------------------------------
// vote
// vote a single cell with size check
//----------------------------------------------------------------------------------
inline void vote( unsigned short* buf, int bwidth, int bheight, int x, int y )
{
	if( ( x >= 0 ) && ( x < bwidth ) && ( y >= 0 ) && ( y < bheight ) )
		buf[ x + y * bwidth ]++;
}

//----------------------------------------------------------------------------------
// _vote
// vote a single cell without size check
//----------------------------------------------------------------------------------
inline void _vote( unsigned short* buf, int bwidth, int x, int y )
{
	buf[ x + y * bwidth ]++;
}

	//------------//
	//   CIRCLE   //
	//------------//

//----------------------------------------------------------------------------------
// votesNumberCircle
// returns the number of expected votes for the specified radius
//----------------------------------------------------------------------------------
#define LINE_M	5.81
#define LINE_Q	4.80
inline int votesNumberCircle( int radius )
{
	return cvRound( LINE_M * radius + LINE_Q );
}

//----------------------------------------------------------------------------------
// voteCircle
// vote a circular shape on the accumulator 
//----------------------------------------------------------------------------------
void voteCircle( unsigned short* buf, int bwidth, int bheight, int xc, int yc, int radius )
{
	int x = 0;
	int y = radius;
	int p = 1 - radius;

	if( ( xc >= radius ) && ( xc < bwidth - radius ) && ( yc >= radius ) && ( yc < bheight - radius ) )
	{
		// circle inside buf, no needs to check
		_vote(buf,bwidth,xc+x,yc+y);
		_vote(buf,bwidth,xc-x,yc+y);
		_vote(buf,bwidth,xc+x,yc-y);
		_vote(buf,bwidth,xc-x,yc-y);
		_vote(buf,bwidth,xc+y,yc+x);
		_vote(buf,bwidth,xc-y,yc+x);
		_vote(buf,bwidth,xc+y,yc-x);
		_vote(buf,bwidth,xc-y,yc-x);

		while( x < y )
		{
			x++;
			if( p < 0 )
				p += 2 * x + 1;
			else
			{
				y--;
				p += 2 * (x - y) + 1;
			}

			_vote(buf,bwidth,xc+x,yc+y);
			_vote(buf,bwidth,xc-x,yc+y);
			_vote(buf,bwidth,xc+x,yc-y);
			_vote(buf,bwidth,xc-x,yc-y);
			_vote(buf,bwidth,xc+y,yc+x);
			_vote(buf,bwidth,xc-y,yc+x);
			_vote(buf,bwidth,xc+y,yc-x);
			_vote(buf,bwidth,xc-y,yc-x);
		}
	}
	else
	{
		vote(buf,bwidth,bheight,xc+x,yc+y);
		vote(buf,bwidth,bheight,xc-x,yc+y);
		vote(buf,bwidth,bheight,xc+x,yc-y);
		vote(buf,bwidth,bheight,xc-x,yc-y);
		vote(buf,bwidth,bheight,xc+y,yc+x);
		vote(buf,bwidth,bheight,xc-y,yc+x);
		vote(buf,bwidth,bheight,xc+y,yc-x);
		vote(buf,bwidth,bheight,xc-y,yc-x);

		while( x < y )
		{
			x++;
			if( p < 0 )
				p += 2 * x + 1;
			else
			{
				y--;
				p += 2 * (x - y) + 1;
			}

			vote(buf,bwidth,bheight,xc+x,yc+y);
			vote(buf,bwidth,bheight,xc-x,yc+y);
			vote(buf,bwidth,bheight,xc+x,yc-y);
			vote(buf,bwidth,bheight,xc-x,yc-y);
			vote(buf,bwidth,bheight,xc+y,yc+x);
			vote(buf,bwidth,bheight,xc-y,yc+x);
			vote(buf,bwidth,bheight,xc+y,yc-x);
			vote(buf,bwidth,bheight,xc-y,yc-x);
		}
	}
}

//----------------------------------------------------------------------------------
// houghCircle
// detect circle using Hough Transform
//----------------------------------------------------------------------------------
void houghCircle( CvMat* edges, CvMat* radii, float dp, int& X, int& Y, int& index, float acc_thr_rel )
{
	float idp = 1.f / dp;
	int edges_rows = edges->rows;
	int edges_cols = edges->cols;
	int nums = radii->cols;
	int accum_rows = int(edges_rows * idp);
	int accum_cols = int(edges_cols * idp);
	int accum_dim = accum_rows * accum_cols;
	int acc_thr;
	index = -1;

	/////////////////////////////////////////////////////
	// vote accumulator 3D matrix
	unsigned short *accums = new unsigned short[ accum_dim * nums ];
	memset( accums, 0, accum_dim * nums * 2 );

	for( int y = 0; y < edges_rows; y++ )
	{
		const uchar* edges_row = edges->data.ptr + y * edges->step;

		for( int x = 0; x < edges_cols; x++ )
		{
			if( !edges_row[x] )
				continue;

			// vote
			int x_idp = int(x*idp);
			int y_idp = int(y*idp);
			for( int i = 0; i < nums; i++ )
				voteCircle( (accums + accum_dim * i ), accum_cols, accum_rows, x_idp, y_idp, int(cvGetReal1D( radii, i )*idp) );
		} // for( x )
	} // for( y )

	/////////////////////////////////////////////////////
	// find most voted circle
	unsigned short max = 0;
	for( int y = 0; y < accum_rows; y++ )
	{
		for( int x = 0; x < accum_cols; x++ )
		{
			int displacement = y * accum_cols + x;
			for( int i = 0; i < nums; i++ )
			{
				unsigned short value = accums[ displacement + accum_dim * i ];
				if( value > max )
				{
					// BISOGNA FILTRARE IL MASSIMO PERCENTUALE ESPRESSO IN RELAZIONE AL RAGGIO
					max = value;
					X = x;
					Y = y;
					index = i;
				}
			} // for( i )
		} // for( x )
	} // for( y )

	// evaluate threshold
	if( index != -1 )
	{
		acc_thr = int( votesNumberCircle( int( cvGetReal1D( radii, index )*idp ) ) * acc_thr_rel );
		if( max < acc_thr )
			index = -1;
	}

	// clear
	delete [] accums;
}

//----------------------------------------------------------------------------------
// houghCircles
// detect circles using Hough Transform
//----------------------------------------------------------------------------------
void houghCircles( CvMat* edges, CvMat* radii, float dp, CvMat* acc, CvMat* indices, float acc_thr_rel, unsigned short filter_dim )
{
	float idp = 1.f / dp;
	int edges_rows = edges->rows;
	int edges_cols = edges->cols;
	int nums = radii->cols;
	int accum_rows = int(edges_rows * idp);
	int accum_cols = int(edges_cols * idp);
	int accum_dim = accum_rows * accum_cols;

	/////////////////////////////////////////////////////
	// vote accumulator 3D matrix
	unsigned short* accums = new unsigned short[ accum_dim * nums ];
	memset( accums, 0, accum_dim * nums * sizeof(unsigned short) );

	for( int y = 0; y < edges_rows; y++ )
	{
		const uchar* edges_row = edges->data.ptr + y * edges->step;

		for( int x = 0; x < edges_cols; x++ )
		{
			if( !edges_row[x] )
				continue;

			// vote
			int x_idp = int(x*idp);
			int y_idp = int(y*idp);
			for( int i = 0; i < nums; i++ )
				voteCircle( (accums + accum_dim * i ), accum_cols, accum_rows, x_idp, y_idp, int(cvGetReal1D( radii, i )*idp) );
		} // for( x )
	} // for( y )


	/////////////////////////////////////////////////////
	// create resultant accumulator

	// evaluate max radius
	unsigned short* max_accums = new unsigned short[ 2 * accum_dim ];
	for( int y = 0; y < accum_rows; y++ )
	{
		for( int x = 0; x < accum_cols; x++ )
		{
			unsigned short max = 0;
			unsigned short max_i = 0;
			int displacement = y * accum_cols + x;
			
			for( int i = 0; i < nums; i++ )
			{
				unsigned short value = accums[ displacement + accum_dim * i ];
				
				if( value > max )
				{
					max = value;
					max_i = i;
				}
			}
			
			max_accums[ displacement ] = max;
			max_accums[ displacement + accum_dim ] = max_i;
		}
	}


	// evaluate threshold
	int* acc_thr_radii = new int[nums];
	for( int i = 0; i < nums; i++ )
	{
		acc_thr_radii[i] = int(votesNumberCircle( int(cvGetReal1D( radii, i )*idp) ) * acc_thr_rel);
	}


	for( int y = 0; y < accum_rows; y++ )
	{
		for( int x = 0; x < accum_cols; x++ )
		{
			if( y < filter_dim || y >= accum_rows - filter_dim )
			{
				cvSet2D( acc, y, x, cvScalar( 0 ) );
				continue;
			}

			if( x < filter_dim || x >= accum_cols - filter_dim )
			{
				cvSet2D( acc, y, x, cvScalar( 0 ) );
				continue;
			}


			unsigned short max = 0;
			unsigned short max_x, max_y, max_index = 0;
			
			for( int my = MAX(0,y-filter_dim); my <= MIN(y+filter_dim,accum_rows-1); my++ )
			{
				for( int mx = MAX(0,x-filter_dim); mx <= MIN(x+filter_dim,accum_cols-1); mx++ )
				{
					// get maximum from different radii
					int displacement = my * accum_cols + mx;
					
					unsigned short value = max_accums[ displacement ];
					
					if( value > max )
					{
						// BISOGNA FILTRARE IL MASSIMO PERCENTUALE ESPRESSO IN RELAZIONE AL RAGGIO
						max = value;
						max_x = mx;
						max_y = my;
						max_index = max_accums[ displacement + accum_dim ];
					}
				}
			}


			if( max > 0 && max_x == x && max_y == y && max >= acc_thr_radii[max_index] )
			{
				cvSet2D( acc, y, x, cvScalar( max ) );
				cvSet2D( indices, y, x, cvScalar( max_index ) );
			}
			else
			{
				cvSet2D( acc, y, x, cvScalar( 0 ) );
			}
		} // for( x )
	} // for( y )

	// clear
	delete [] acc_thr_radii;
	delete [] max_accums;
	delete [] accums;
}


	//---------------//
	//   RECTANGLE   //
	//---------------//

//----------------------------------------------------------------------------------
// votesNumberRectangle
// returns the number of maximum votes for the specified side
//----------------------------------------------------------------------------------
inline int votesNumberRectangle( int side_x, int side_y )
{
	return (2*side_x + 2*side_y - 4);
}

//----------------------------------------------------------------------------------
// voteLineH
// vote an horizontal line into the accumulator
//----------------------------------------------------------------------------------
void voteLineH( unsigned short* buf, int bwidth, int bheight, int x1, int x2, int y )
{
	if( y < 0 || y >= bheight )
		return;

	int xs = MAX( 0, MIN( x1, x2 ) );
	int xe = MIN( bwidth-1, MAX( x1, x2 ) );

	while( xs <= xe )
		_vote( buf, bwidth, xs++, y );
}

//----------------------------------------------------------------------------------
// voteLineV
// vote a vertical line into the accumulator 
//----------------------------------------------------------------------------------
void voteLineV( unsigned short* buf, int bwidth, int bheight, int x, int y1, int y2 )
{
	if( x < 0 || x >= bwidth )
		return;

	int ys = MAX( 0, MIN( y1, y2 ) );
	int ye = MIN( bheight-1, MAX( y1, y2 ) );

	while( ys <= ye )
		_vote( buf, bwidth, x, ys++ );
}

//----------------------------------------------------------------------------------
// voteRectangle
// vote a rectangular shape into the accumulator 
//----------------------------------------------------------------------------------
void voteRectangle( unsigned short* buf, int bwidth, int bheight, int xc, int yc, int w, int h )
{
	int x1 = xc - w / 2;
	int y1 = yc - h / 2;
	int x2 = xc + w / 2;
	int y2 = yc + h / 2;

	voteLineH( buf, bwidth, bheight, x1, x2, y1 );
	voteLineH( buf, bwidth, bheight, x1, x2, y2 );
	voteLineV( buf, bwidth, bheight, x1, y1+1, y2-1 );
	voteLineV( buf, bwidth, bheight, x2, y1+1, y2-1 );
}

//----------------------------------------------------------------------------------
// houghRectangle
// detect rectangle using Hough Transform
//----------------------------------------------------------------------------------
void houghRectangle( CvMat* edges, CvMat* side_x, CvMat* side_y, float dp, int& X, int& Y, int& index_x, int& index_y, float acc_thr_rel )
{
	float idp = 1.f / dp;
	int edges_rows = edges->rows;
	int edges_cols = edges->cols;
	int nums = side_x->cols * side_y->cols;
	int accum_rows = int(edges_rows * idp);
	int accum_cols = int(edges_cols * idp);
	int accum_dim = accum_rows * accum_cols;
	index_x = index_y = -1;

	/////////////////////////////////////////////////////
	// vote accumulator 3D matrix
	unsigned short *accums = new unsigned short[ accum_dim * nums ];
	memset( accums, 0, accum_dim * nums * 2 );

	for( int y = 0; y < edges_rows; y++ )
	{
		const uchar* edges_row = edges->data.ptr + y * edges->step;

		for( int x = 0; x < edges_cols; x++ )
		{
			if( !edges_row[x] )
				continue;

			// vote
			int x_idp = int(x*idp);
			int y_idp = int(y*idp);
			int i = 0;
			for( int i_x = 0; i_x < side_x->cols; i_x++ )
				for( int i_y = 0; i_y < side_y->cols; i_y++, i++ )
					voteRectangle( (accums + accum_dim * i), accum_cols, accum_rows, x_idp, y_idp, int( cvGetReal1D( side_x, i_x )*idp ), int( cvGetReal1D( side_y, i_y )*idp ) );
		} // for( x )
	} // for( y )

	/////////////////////////////////////////////////////
	// find most voted circle
	unsigned short max = 0;
	for( int y = 0; y < accum_rows; y++ )
	{
		for( int x = 0; x < accum_cols; x++ )
		{
			int displacement = y * accum_cols + x;
			for( int i = 0; i < nums; i++ )
			{
				unsigned short value = accums[ displacement + accum_dim * i ];
				if( value > max )
				{
					// BISOGNA FILTRARE IL MASSIMO PERCENTUALE ESPRESSO IN RELAZIONE AL RAGGIO
					max = value;
					X = x;
					Y = y;
					index_x = i / side_y->cols;
					index_y = i % side_y->cols;
				}
			} // for( i )
		} // for( x )
	} // for( y )

	// evaluate threshold
	if( index_x != -1 )
	{
		int acc_thr = int( votesNumberRectangle( int( cvGetReal1D( side_x, index_x )*idp ), int( cvGetReal1D( side_y, index_y )*idp ) ) * acc_thr_rel );
		if( max < acc_thr )
		{
			index_x = -1;
			index_y = -1;
		}
	}

	// clear
	delete [] accums;
}
