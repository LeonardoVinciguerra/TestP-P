//---------------------------------------------------------------------------
//
// Name:        gui_progressbar.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_ProgressBar class implementation
//
//---------------------------------------------------------------------------
#include "gui_progressbar.h"

#include "q_graph.h"

#include "gui_functions.h"
#include "gui_defs.h"

#include <mss.h>


//--------------------------------------------------------------------------
// Costruttore della classe
//--------------------------------------------------------------------------
GUI_ProgressBar_OLD::GUI_ProgressBar_OLD( GUIWindow* parent, int x, int y, int w, int h, float max, float start )
{
	maxValue = max;
	progress = MID( 0.f, start, maxValue );

	_rect = RectI( x * GUI_CharW() + parent->GetX(), y * GUI_CharH() + parent->GetY(), w * GUI_CharW(), h * GUI_CharH() );
	_kx = ( w * GUI_CharW() - 2 ) / maxValue;
	_kx = MAX( 0, _kx );

	GUI_Freeze_Locker lock;

	GUI_Rect( _rect, GUI_color( PB_COL_BORDER ) );

	_rect.X += 1;
	_rect.Y += 1;
	_rect.W -= 2;
	_rect.H -= 2;

	Show();
}

//--------------------------------------------------------------------------
// Distruttore della classe
//--------------------------------------------------------------------------
GUI_ProgressBar_OLD::~GUI_ProgressBar_OLD()
{
}

//--------------------------------------------------------------------------
// Visualizza la barra
//--------------------------------------------------------------------------
void GUI_ProgressBar_OLD::Show()
{
	// Tolto perche' rallenta di 10 volte la funzione
	//GUI_Freeze_Locker lock;

	int pix = _kx * progress;

	RectI r = _rect;
	r.X += pix + 1;
	r.W -= pix + 1;
	if( r.W > 0 )
	{
		GUI_FillRect( r, GUI_color( PB_COL_BG ) );
	}

	r = _rect;
	r.W = pix;
	if( r.W > 0 )
	{
		GUI_FillRect( r, GUI_color( PB_COL_PROGRESS ) );
	}
}

//--------------------------------------------------------------------------
// Setta valore avanzamento
//--------------------------------------------------------------------------
void GUI_ProgressBar_OLD::SetValue( float value )
{
	progress = MID( 0.f, value, maxValue );
	Show();
}

//--------------------------------------------------------------------------
// Incrementa valore avanzamento
//--------------------------------------------------------------------------
void GUI_ProgressBar_OLD::Increment( float inc )
{
	progress = MID( 0.f, progress + inc, maxValue );
	Show();
}




//--------------------------------------------------------------------------
// Costruttore/Distruttore
//--------------------------------------------------------------------------
GUI_ProgressBar::GUI_ProgressBar( GUIWindow* parent, const RectI& area, int start )
{
	_parent = parent;
	_min = 0;
	_max = 100;
	_value = MID( _min, start, _max );

	_rect = area;
	_kx = ( _rect.W - 2 ) / (_max - _min);
	_kx = MAX( 0, _kx );

	_is_shown = false;
}

GUI_ProgressBar::~GUI_ProgressBar()
{
}

//--------------------------------------------------------------------------
// Visualizza la barra
//--------------------------------------------------------------------------
void GUI_ProgressBar::Show()
{
	if( _parent )
	{
		_rect.X += _parent->GetX();
		_rect.Y += _parent->GetY();
	}

	GUI_Freeze_Locker lock;

	GUI_Rect( _rect, GUI_color( PB_COL_BORDER ) );

	_rect.X += 1;
	_rect.Y += 1;
	_rect.W -= 2;
	_rect.H -= 2;

	refresh();

	_is_shown = true;
}

void GUI_ProgressBar::refresh()
{
	// Tolto perche' rallenta di 10 volte la funzione
	//GUI_Freeze_Locker lock;

	int pix = _kx * _value;

	RectI r = _rect;
	r.X += pix + 1;
	r.W -= pix + 1;
	if( r.W > 0 )
	{
		GUI_FillRect( r, GUI_color( PB_COL_BG ) );
	}

	r = _rect;
	r.W = pix;
	if( r.W > 0 )
	{
		GUI_FillRect( r, GUI_color( PB_COL_PROGRESS ) );
	}
}

//--------------------------------------------------------------------------
// Setta minimo e massino
//--------------------------------------------------------------------------
void GUI_ProgressBar::SetVMinMax( int min, int max )
{
	if( min >= max )
		return;

	_min = min;
	_max = max;

	_kx = float( _rect.W - 2 ) / (_max - _min);
	_kx = MAX( 0, _kx );
	_value = MID( _min, _value, _max );

	if( _is_shown )
	{
		refresh();
	}
}

//--------------------------------------------------------------------------
// Setta avanzamento
//--------------------------------------------------------------------------
void GUI_ProgressBar::SetValue( int value )
{
	_value = MID( _min, value, _max );

	if( _is_shown )
	{
		refresh();
	}
}

//--------------------------------------------------------------------------
// Incrementa avanzamento
//--------------------------------------------------------------------------
void GUI_ProgressBar::Increment()
{
	_value = MID( _min, _value + 1, _max );

	if( _is_shown )
	{
		refresh();
	}
}
