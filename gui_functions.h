//---------------------------------------------------------------------------
//
// Name:        gui_functions.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GGUI usefull functions definition
//
//---------------------------------------------------------------------------

#ifndef __GUI_FUNCTIONS_H
#define __GUI_FUNCTIONS_H

#include "mathlib.h"

#include "stdint.h"
typedef uint8_t		Uint8;
typedef uint16_t	Uint16;


	//--------------------//
	//  Data definitions  //
	//--------------------//

struct GUI_color
{
	GUI_color()
	{
		r = 0;
		g = 0;
		b = 0;
		a = 0;
	}

	GUI_color( Uint8 R, Uint8 G, Uint8 B )
	{
		r = R;
		g = G;
		b = B;
		a = 0;
	}

	GUI_color( unsigned int color )
	{
		r = color;
		g = color >> 8;
		b = color >> 16;
		a = 0;
	}

	Uint8 r;
	Uint8 g;
	Uint8 b;
	Uint8 a;
};

enum GUI_font
{
	GUI_DefaultFont = 0,
	GUI_SmallFont,
	GUI_XSmallFont,
	GUI_Font_Num
};



	//------------------//
	//  Init functions  //
	//------------------//

bool GUI_Init( bool windowed );
void GUI_Quit();



	//---------------------//
	//  Metrics functions  //
	//---------------------//

// screen dimension and color depth
int GUI_ScreenW();
int GUI_ScreenH();
int GUI_ScreenBPP();

// char dimension
int GUI_CharW( GUI_font font = GUI_DefaultFont );
int GUI_CharH( GUI_font font = GUI_DefaultFont );

// screen dimension in chars (default font)
int GUI_ScreenCols();
int GUI_ScreenRows();

// text centering offset (default font)
int GUI_TextOffsetX();
int GUI_TextOffsetY();

int TextToGraphX( int textPos );
int TextToGraphY( int textPos );
void GUI_TextToGraph( const RectI& textRect, RectI& graphRect );



	//-----------------//
	//  GUI functions  //
	//-----------------//

void GUI_Freeze();
void GUI_Thaw();

class GUI_Freeze_Locker
{
public:
	GUI_Freeze_Locker();
	~GUI_Freeze_Locker();
};

class GUI_Update_Force
{
public:
	GUI_Update_Force();
	~GUI_Update_Force();
private:
	int freeze;
};



	//-------------------//
	//  Draw primitives  //
	//-------------------//

void GUI_PutPixel( int x, int y, const GUI_color& color );
void GUI_Line( int x1, int y1, int x2, int y2, const GUI_color& color );
void GUI_HLine( int x1, int x2, int y, const GUI_color& color );
void GUI_VLine( int x, int y1, int y2, const GUI_color& color );
void GUI_Rect( const RectI& rect, const GUI_color& color );
void GUI_FillRect( const RectI& rect, const GUI_color& color );
void GUI_FillRectA( const RectI& rect, const GUI_color& color );
void GUI_Trigon( int x1, int y1, int x2, int y2, int x3, int y3, const GUI_color& color );
void GUI_FillTrigon( int x1, int y1, int x2, int y2, int x3, int y3, const GUI_color& color );
void GUI_Circle( const PointI& center, int radius, const GUI_color& color );
void GUI_FillCircle( const PointI& center, int radius, const GUI_color& color );
void GUI_FillPie( const PointI& center, int radius, int start, int end, const GUI_color& color );



	//-----------------------//
	//  Draw text functions  //
	//-----------------------//

void GUI_DrawText( int x, int y, const char* text, GUI_font font, const GUI_color& fgColor );
void GUI_DrawText( int x, int y, const char* text, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor );
void GUI_DrawTextCentered( int x1, int x2, int y, const char* text, GUI_font font, const GUI_color& fgColor );
void GUI_DrawTextCentered( int x1, int x2, int y, const char* text, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor );
void GUI_DrawText_HotKey( int x, int y, const char* text, GUI_font font, const GUI_color& fgColor, const GUI_color& hkColor );
void GUI_DrawText_HotKey( int x, int y, const char* text, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor, const GUI_color& hkColor );
void GUI_DrawGlyph( int x, int y, Uint16 ch, GUI_font font, const GUI_color& fgColor );
void GUI_DrawGlyph( int x, int y, Uint16 ch, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor );



	//-------------------//
	//  Image functions  //
	//-------------------//

void* GUI_SaveScreen( const RectI& rect );
void GUI_DrawSurface( const PointI& pos, void* surface );
void GUI_FreeSurface( void** surface );
void* GUI_LoadImage( const char* filename );

#endif
