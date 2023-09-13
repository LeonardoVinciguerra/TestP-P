//---------------------------------------------------------------------------
//
// Name:        mathlib.h
// Author:      Gabriel Ferri
// Created:     03/12/2010
// Description:
//
//---------------------------------------------------------------------------
#ifndef __MATHLIB_H
#define __MATHLIB_H


#ifndef MAX
#define MAX(a, b)			(((a)>(b))?(a):(b))
#endif

#ifndef MIN
#define MIN(a, b)			(((a)>(b))?(b):(a))
#endif

#ifndef MID
#define MID(m,x,M)			(MIN(MAX((m),(x)),(M)))
#endif

#ifndef PI
#define PI					3.14159265358979323846
#endif

#define RTOD(r)				((r) * 180 / PI)
#define DTOR(d)				((d) * PI / 180)

#define isNan(x)			((x) != (x))

#ifndef ABS
#define ABS(a)              ((a)<0 ? -(a) : (a))
#endif



	//-----------//
	// Functions //
	//-----------//

int ftoi( double x );
unsigned char Hex2Dec( unsigned char hex );


	//--------------//
	// Structs Defs //
	//--------------//

struct PointD
{
	PointD() { X = 0.0; Y = 0.0; };
	PointD( double x, double y ) { X = x; Y = y; };
	PointD( const PointD& p ) { X = p.X; Y = p.Y; };

	double X, Y;
};

struct PointF
{
	PointF() { X = 0.f; Y = 0.f; };
	PointF( float x, float y ) { X = x; Y = y; };
	PointF( const PointF& p ) { X = p.X; Y = p.Y; };

	float X, Y;
};

struct PointI
{
	PointI() { X = 0; Y = 0; };
	PointI( int x, int y ) { X = x; Y = y; };
	PointI( const PointI& p ) { X = p.X; Y = p.Y; };

	int X, Y;
};

struct RectI
{
	RectI() { X = 0; Y = 0; W = 0; H = 0; };
	RectI( int x, int y, int width, int height ) { X = x; Y = y; W = width; H = height; };
	RectI( const RectI& r ) { X = r.X; Y = r.Y; W = r.W; H = r.H; };

	union { int X; int X1; };
	union { int Y; int Y1; };
	union { int W; int X2; };
	union { int H; int Y2; };
};

struct CircleF
{
	CircleF() { X = Y = R = 0.f; };
	CircleF( float x, float y, float r ) { X = x; Y = y; R = r; };
	CircleF( const CircleF& c ) { X = c.X; Y = c.Y; R = c.R; };

	float X, Y, R;
};

#endif
