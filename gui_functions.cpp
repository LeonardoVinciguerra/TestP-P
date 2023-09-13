//---------------------------------------------------------------------------
//
// Name:        gui_functions.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI usefull functions implementation
//
//---------------------------------------------------------------------------
#include "gui_functions.h"

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/sge.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "strutils.h"
#include "q_inifile.h"

#include <mss.h>


//Screen attributes
#define SCREEN_WIDTH_1440   1440
#define SCREEN_WIDTH_1600   1600
#define SCREEN_WIDTH_1920   1920
#define SCREEN_HEIGHT       900
#define SCREEN_HEIGHT_1080  1080
#define SCREEN_BPP          24


// "font/DejaVuSansMono-Bold.ttf"
// "font/Anonymous.ttf"
// "font/Inconsolata.ttf"

#define FONT_DEFAULT        "font/DejaVuSansMono.ttf"
#define FONT_DEFAULT_SIZE   17

#define FONT_SMALL          "font/DejaVuSansMono-Bold.ttf"
#define FONT_SMALL_SIZE     16

#define FONT_XSMALL         "font/DejaVuSansMono.ttf"
#define FONT_XSMALL_SIZE    12


//Private functions
void _UpdateScreen( int x, int y, int w, int h );
void _GUI_SetCharAndScreenInfo();



	//---------------//
	//  Global vars  //
	//---------------//

SDL_Surface* _screen = 0;
TTF_Font* _font_def = 0;
TTF_Font* _font_small = 0;
TTF_Font* _font_xsmall = 0;

int _freeze_screen = 0;

int _char_w[GUI_Font_Num];
int _char_h[GUI_Font_Num];
int _screen_cols = 0;
int _screen_rows = 0;
int _text_offset_x = 0;
int _text_offset_y = 0;



	//------------------//
	//  Init functions  //
	//------------------//

//--------------------------------------------------------------------------
// Inizializza modo grafico
//--------------------------------------------------------------------------
bool GUI_Init( bool windowed )
{
	//Start SDL
	if( SDL_Init( SDL_INIT_VIDEO ) == -1 )
	{
		printf( "GUI_Init: unable to SDL_Init !\n" );
		return false;
	}

	//Set up screen
	int flag = SDL_SWSURFACE;
	if( !windowed )
	{
		flag |= SDL_FULLSCREEN;
	}

	if( Get_Res1920() )
		_screen = SDL_SetVideoMode( SCREEN_WIDTH_1920, SCREEN_HEIGHT_1080, SCREEN_BPP, flag );
	else if( Get_Res1600() )
		_screen = SDL_SetVideoMode( SCREEN_WIDTH_1600, SCREEN_HEIGHT, SCREEN_BPP, flag );
	else
		_screen = SDL_SetVideoMode( SCREEN_WIDTH_1440, SCREEN_HEIGHT, SCREEN_BPP, flag );
	if( _screen == NULL )
	{
		printf( "GUI_Init: unable to SDL_SetVideoMode !\n" );
		return false;
	}

	//Set the window caption
	SDL_WM_SetCaption( "QDVC evo", NULL );

	// initialize SDL ttf module
	if( TTF_Init() == -1 )
	{
		printf( "GUI_Init: unable to TTF_Init !\n" );
		return false;
	}

	// initialize SDL image module
	IMG_Init( IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_TIF );

	//Open the fonts
	_font_def = TTF_OpenFont( FONT_DEFAULT, FONT_DEFAULT_SIZE );
	_font_small = TTF_OpenFont( FONT_SMALL, FONT_SMALL_SIZE );
	_font_xsmall = TTF_OpenFont( FONT_XSMALL, FONT_XSMALL_SIZE );

	//Get video surface
	_screen = SDL_GetVideoSurface();

	_GUI_SetCharAndScreenInfo();

	// disable update screen SDL_sge
	sge_Update_OFF();
	// enable key repeat event
	SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
	// hide cursor
	SDL_ShowCursor( SDL_DISABLE );
	// enable unicode support
	SDL_EnableUNICODE( SDL_ENABLE );

	return true;
}

//--------------------------------------------------------------------------
// Inizializza modo grafico
//--------------------------------------------------------------------------
void GUI_Quit()
{
	//Close the fonts
	if( _font_def )
	{
		TTF_CloseFont( _font_def );
		_font_def = 0;
	}
	if( _font_small )
	{
		TTF_CloseFont( _font_small );
		_font_small = 0;
	}
	if( _font_xsmall )
	{
		TTF_CloseFont( _font_xsmall );
		_font_xsmall = 0;
	}

	SDL_EnableUNICODE( SDL_DISABLE );

	//Quit SDL modules
	TTF_Quit();
	IMG_Quit();

	//Quit SDL
	SDL_Quit();
}



	//---------------------//
	//  Metrics functions  //
	//---------------------//

int GUI_ScreenW()
{
	return _screen->w;
}

int GUI_ScreenH()
{
	return _screen->h;
}

int GUI_ScreenBPP()
{
	return SCREEN_BPP;
}

int GUI_CharW( GUI_font font )
{
	return _char_w[font];
}

int GUI_CharH( GUI_font font )
{
	return _char_h[font];
}

int GUI_ScreenCols()
{
	return _screen_cols;
}

int GUI_ScreenRows()
{
	return _screen_rows;
}

int GUI_TextOffsetX()
{
	return _text_offset_x;
}

int GUI_TextOffsetY()
{
	return _text_offset_y;
}

int TextToGraphX( int textPos )
{
	return textPos * _char_w[GUI_DefaultFont];
}

int TextToGraphY( int textPos )
{
	return textPos * _char_h[GUI_DefaultFont];
}

void GUI_TextToGraph( const RectI& textRect, RectI& graphRect )
{
	graphRect.X = textRect.X * _char_w[GUI_DefaultFont] + _text_offset_x;
	graphRect.Y = textRect.Y * _char_h[GUI_DefaultFont] + _text_offset_y;
	graphRect.W = textRect.W * _char_w[GUI_DefaultFont];
	graphRect.H = textRect.H * _char_h[GUI_DefaultFont];
}


#if 0
void GUI_TextInfo()
{
	SDL_Rect rect;
	rect.x = text_offset_x;
	rect.y = text_offset_y;
	rect.w = char_w * screen_cols;
	rect.h = char_h * screen_rows;
	SDL_FillRect( screen, &rect, SDL_MapRGB( screen->format, 80, 80, 100 ) );

	char* buf = new char[screen_cols+1];
	SDL_Color textColor = { 255, 255, 255 };

	sprintf( buf, "Font: %s   (size: %d)", FONT_DEFAULT, FONT_DEFAULT_SIZE );
	rect.y += char_h;
	GUI_DrawString( rect.x + char_w, rect.y, buf, font_def, textColor );

	sprintf( buf, "Char size: %d x %d   Screen text size: %d x %d", GUI_CharW(), GUI_CharH(), GUI_ScreenCols(), GUI_ScreenRows() );
	rect.y += 2*char_h;
	GUI_DrawString( rect.x + char_w, rect.y, buf, font_def, textColor );

	rect.y += 2*char_h;
	int i = 0;
	int start = '!';
	int end = '~';
	for( int c = start; c <= end; c++ )
	{
		buf[i] = c;
		i++;
		if( i % screen_cols == 0 || c == end )
		{
			buf[i] = '\0';

			GUI_DrawString( rect.x, rect.y, buf, font_def, textColor );

			i = 0;
			rect.y += char_h;
		}
	}

	delete [] buf;
}
#endif



	//-----------------//
	//  GUI functions  //
	//-----------------//

void GUI_Freeze()
{
	_freeze_screen++;
}

void GUI_Thaw()
{
	_freeze_screen = MAX( _freeze_screen - 1, 0 );

	if( !_freeze_screen )
	{
		SDL_Flip( _screen );
	}
}

GUI_Freeze_Locker::GUI_Freeze_Locker()
{
	GUI_Freeze();
}

GUI_Freeze_Locker::~GUI_Freeze_Locker()
{
	GUI_Thaw();
}

GUI_Update_Force::GUI_Update_Force()
{
	freeze = _freeze_screen;
	_freeze_screen = 0;

	SDL_Flip( _screen );
}

GUI_Update_Force::~GUI_Update_Force()
{
	_freeze_screen += freeze;
}



	//-------------------//
	//  Draw primitives  //
	//-------------------//

void GUI_PutPixel( int x, int y, const GUI_color& color )
{
	sge_PutPixel( _screen, x, y, SDL_MapRGB( _screen->format, color.r, color.g, color.b ) );

	_UpdateScreen( x, y, 1, 1 );
}

void GUI_Line( int x1, int y1, int x2, int y2, const GUI_color& color )
{
	sge_Line( _screen, x1, y1, x2, y2, SDL_MapRGB( _screen->format, color.r, color.g, color.b ) );

	_UpdateScreen( MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2) );
}

void GUI_HLine( int x1, int x2, int y, const GUI_color& color )
{
	sge_HLine( _screen, x1, x2, y, SDL_MapRGB( _screen->format, color.r, color.g, color.b ) );

	_UpdateScreen( x1, y, x2-x1+1, 1 );
}

void GUI_VLine( int x, int y1, int y2, const GUI_color& color )
{
	sge_VLine( _screen, x, y1, y2, SDL_MapRGB( _screen->format, color.r, color.g, color.b ) );

	_UpdateScreen( x, y1, 1, y2-y1+1 );
}

void GUI_Rect( const RectI& rect, const GUI_color& color )
{
	sge_Rect( _screen, rect.X, rect.Y, rect.X+rect.W-1, rect.Y+rect.H-1, SDL_MapRGB( _screen->format, color.r, color.g, color.b ) );

	_UpdateScreen( rect.X, rect.Y, rect.W, rect.H );
}

void GUI_FillRect( const RectI& rect, const GUI_color& color )
{
	SDL_Rect r = { rect.X, rect.Y, rect.W, rect.H };
	SDL_FillRect( _screen, &r, SDL_MapRGB( _screen->format, color.r, color.g, color.b ) );

	_UpdateScreen( rect.X, rect.Y, rect.W, rect.H );
}

void GUI_FillRectA( const RectI& rect, const GUI_color& color )
{
	boxRGBA( _screen, rect.X, rect.Y, rect.X+rect.W-1, rect.Y+rect.H-1, color.r, color.g, color.b, color.a );

	_UpdateScreen( rect.X, rect.Y, rect.W, rect.H );
}

void GUI_Trigon( int x1, int y1, int x2, int y2, int x3, int y3, const GUI_color& color )
{
	aatrigonRGBA( _screen, x1, y1, x2, y2, x3, y3, color.r, color.g, color.b, 255 );

	int x = MIN(x1,MIN(x2,x3));
	int y = MIN(y1,MIN(y2,y3));
	int w = MAX(x1,MAX(x2,x3)) - x;
	int h = MAX(y1,MAX(y2,y3)) - y;

	_UpdateScreen( x, y, w, h );
}

void GUI_FillTrigon( int x1, int y1, int x2, int y2, int x3, int y3, const GUI_color& color )
{
	filledTrigonRGBA( _screen, x1, y1, x2, y2, x3, y3, color.r, color.g, color.b, 255 );

	int x = MIN(x1,MIN(x2,x3));
	int y = MIN(y1,MIN(y2,y3));
	int w = MAX(x1,MAX(x2,x3)) - x;
	int h = MAX(y1,MAX(y2,y3)) - y;

	_UpdateScreen( x, y, w, h );
}

void GUI_Circle( const PointI& center, int radius, const GUI_color& color )
{
	sge_AACircle( _screen, center.X, center.Y, radius, SDL_MapRGB( _screen->format, color.r, color.g, color.b ) );

	_UpdateScreen( center.X-radius, center.Y-radius, radius, radius );
}

void GUI_FillCircle( const PointI& center, int radius, const GUI_color& color )
{
	sge_AAFilledCircle( _screen, center.X, center.Y, radius, SDL_MapRGB( _screen->format, color.r, color.g, color.b ) );

	_UpdateScreen( center.X-radius, center.Y-radius, radius, radius );
}

void GUI_FillPie( const PointI& center, int radius, int start, int end, const GUI_color& color )
{
	filledPieRGBA( _screen, center.X, center.Y, radius, start, end, color.r, color.g, color.b, 255 );

	_UpdateScreen( center.X-radius, center.Y-radius, radius, radius );
}



	//-----------------------//
	//  Draw text functions  //
	//-----------------------//

//--------------------------------------------------------------------------
// Ritorna puntatore a font specificato
//--------------------------------------------------------------------------
TTF_Font* _GetTTFFont( GUI_font font )
{
	if( font == GUI_SmallFont )
		return _font_small;
	else if( font == GUI_XSmallFont )
		return _font_xsmall;

	return _font_def;
}

//--------------------------------------------------------------------------
// Stampa una stringa di testo alle coordinate passate
//--------------------------------------------------------------------------
void GUI_DrawText( int x, int y, const char* text, GUI_font font, const GUI_color& fgColor )
{
	if( !strlen(text) )
		return;

	// elimina tilde se presente
	char str[512];
	snprintf( str, 512, "%s", text );
	char c;
	strNoTilde( str, &c );

	int len = strlen(str);
	if( !len )
		return;

	// set font
	TTF_Font* pfont = _GetTTFFont( font );

	// set colors and draw text
	SDL_Color fg = { fgColor.r, fgColor.g, fgColor.b };
	//SDL_Surface* textSurface = TTF_RenderUTF8_Blended( pfont, str, fg );
	SDL_Surface* textSurface = TTF_RenderText_Blended( pfont, str, fg );

	SDL_Rect pos = { x, y };
	SDL_BlitSurface( textSurface, NULL, _screen, &pos );
	SDL_FreeSurface( textSurface );

	_UpdateScreen( x, y, textSurface->w, textSurface->h );
}

void GUI_DrawText( int x, int y, const char* text, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor )
{
	if( !strlen(text) )
		return;

	// elimina tilde se presente
	char str[512];
	snprintf( str, 512, "%s", text );
	char c;
	strNoTilde( str, &c );

	int len = strlen(str);
	if( !len )
		return;

	// set font
	TTF_Font* pfont = _GetTTFFont( font );

	// set colors and draw text
	SDL_Color fg = { fgColor.r, fgColor.g, fgColor.b };
	SDL_Color bg = { bgColor.r, bgColor.g, bgColor.b };
	//SDL_Surface* textSurface = TTF_RenderUTF8_Shaded( pfont, str, fg, bg );
	SDL_Surface* textSurface = TTF_RenderText_Shaded( pfont, str, fg, bg );

	SDL_Rect pos = { x, y };
	SDL_BlitSurface( textSurface, NULL, _screen, &pos );
	SDL_FreeSurface( textSurface );

	_UpdateScreen( x, y, textSurface->w, textSurface->h );
}

//--------------------------------------------------------------------------
// Stampa una stringa centrata tra le coordinate x1 e x2
//--------------------------------------------------------------------------
void GUI_DrawTextCentered( int x1, int x2, int y, const char* text, GUI_font font, const GUI_color& fgColor )
{
	int len = strlen( text );
	if( !len )
		return;

	if( strchr( text, '~' ) )
		len--;

	if( len )
	{
		int x = (x1 + x2 - len*_char_w[font]) / 2;
		GUI_DrawText( x, y, text, font, fgColor );
	}
}

void GUI_DrawTextCentered( int x1, int x2, int y, const char* text, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor )
{
	int len = strlen( text );
	if( !len )
		return;

	if( strchr( text, '~' ) )
		len--;

	if( len )
	{
		int x = (x1 + x2 - len*_char_w[GUI_DefaultFont]) / 2;
		GUI_DrawText( x, y, text, font, bgColor, fgColor );
	}
}

//--------------------------------------------------------------------------
// Stampa una stringa alla posizione specificata con controllo dell'hotkey ~
//--------------------------------------------------------------------------
void GUI_DrawText_HotKey( int x, int y, const char* text, GUI_font font, const GUI_color& fgColor, const GUI_color& hkColor )
{
	if( !strlen(text) )
		return;

	char str[512];
	snprintf( str, 512, "%s", text );
	char c[2];
	int rev_c = strNoTilde( str, &c[0] );

	GUI_DrawText( x, y, str, font, fgColor );

	if( rev_c != -1 )
	{
		c[1] = '\0';
		GUI_DrawText( x+rev_c*_char_w[font], y, c, font, hkColor );
	}
}

void GUI_DrawText_HotKey( int x, int y, const char* text, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor, const GUI_color& hkColor )
{
	if( !strlen(text) )
		return;

	char str[512];
	snprintf( str, 512, "%s", text );
	char c[2];
	int rev_c = strNoTilde( str, &c[0] );

	GUI_DrawText( x, y, str, font, bgColor, fgColor );

	if( rev_c != -1 )
	{
		c[1] = '\0';
		GUI_DrawText( x+rev_c*_char_w[font], y, c, font, bgColor, hkColor );
	}
}

//--------------------------------------------------------------------------
// Stampa un carattere grafico alle coordinate passate
//--------------------------------------------------------------------------
void GUI_DrawGlyph( int x, int y, Uint16 ch, GUI_font font, const GUI_color& fgColor )
{
	// set font
	TTF_Font* pfont = _GetTTFFont( font );

	// set colors and draw text
	SDL_Color fg = { fgColor.r, fgColor.g, fgColor.b };
	SDL_Surface* textSurface = TTF_RenderGlyph_Blended( pfont, ch, fg );

	SDL_Rect pos = { x, y };
	SDL_BlitSurface( textSurface, NULL, _screen, &pos );
	SDL_FreeSurface( textSurface );

	_UpdateScreen( x, y, textSurface->w, textSurface->h );
}

void GUI_DrawGlyph( int x, int y, Uint16 ch, GUI_font font, const GUI_color& bgColor, const GUI_color& fgColor )
{
	// set font
	TTF_Font* pfont = _GetTTFFont( font );

	// set colors and draw text
	SDL_Color fg = { fgColor.r, fgColor.g, fgColor.b };
	SDL_Color bg = { bgColor.r, bgColor.g, bgColor.b };
	SDL_Surface* textSurface = TTF_RenderGlyph_Shaded( pfont, ch, fg, bg );

	SDL_Rect pos = { x, y };
	SDL_BlitSurface( textSurface, NULL, _screen, &pos );
	SDL_FreeSurface( textSurface );

	_UpdateScreen( x, y, textSurface->w, textSurface->h );
}



	//-------------------//
	//  Image functions  //
	//-------------------//


//--------------------------------------------------------------------------
// Salva porzione di schermo
//--------------------------------------------------------------------------
void* GUI_SaveScreen( const RectI& rect )
{
	SDL_Surface* buffer = SDL_CreateRGBSurface( SDL_SWSURFACE, rect.W, rect.H,
			_screen->format->BitsPerPixel, _screen->format->Rmask, _screen->format->Gmask, _screen->format->Bmask, _screen->format->Amask );
	// save screen
	SDL_Rect r;
	r.x = rect.X;
	r.y = rect.Y;
	r.w = rect.W;
	r.h = rect.H;
	SDL_BlitSurface( _screen, &r, buffer, NULL );

	return buffer;
}

void GUI_DrawSurface( const PointI& pos, void* surface )
{
	SDL_Surface* buffer = (SDL_Surface*)surface;
	SDL_Rect r;
	r.x = pos.X;
	r.y = pos.Y;
	SDL_BlitSurface( buffer, NULL, _screen, &r );

	_UpdateScreen( pos.X, pos.Y, buffer->w, buffer->h );
}

void GUI_FreeSurface( void** surface )
{
	SDL_FreeSurface( (SDL_Surface*)*surface );
	*surface = 0;
}

void* GUI_LoadImage( const char* filename )
{
	SDL_Surface* loadedImage = NULL;
	SDL_Surface* optimizedImage = NULL;

	loadedImage = IMG_Load( filename );

	if( loadedImage != NULL )
	{
		optimizedImage = SDL_DisplayFormat( loadedImage );
		SDL_FreeSurface( loadedImage );
	}

	return optimizedImage;
}



	//---------------------//
	//  Private functions  //
	//---------------------//

void _UpdateScreen( int x, int y, int w, int h )
{
	if( !_freeze_screen )
	{
		SDL_UpdateRect( _screen, x, y, w, h );
		//SDL_Flip( _screen );
	}
}

void _GUI_SetCharAndScreenInfo()
{
	TTF_SizeText( _font_def, "A", &_char_w[GUI_DefaultFont], &_char_h[GUI_DefaultFont] );
	TTF_SizeText( _font_small, "A", &_char_w[GUI_SmallFont], &_char_h[GUI_SmallFont] );
	TTF_SizeText( _font_xsmall, "A", &_char_w[GUI_XSmallFont], &_char_h[GUI_XSmallFont] );

	_screen_cols = GUI_ScreenW() / _char_w[GUI_DefaultFont];
	_screen_rows = GUI_ScreenH() / _char_h[GUI_DefaultFont];

	_text_offset_x = (GUI_ScreenW() - _char_w[GUI_DefaultFont] * _screen_cols) / 2;
	_text_offset_y = (GUI_ScreenH() - _char_h[GUI_DefaultFont] * _screen_rows) / 2;
}
