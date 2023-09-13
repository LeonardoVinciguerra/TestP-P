//---------------------------------------------------------------------------
//
// Name:        gui_progressbar.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_ProgressBar class definition
//
//---------------------------------------------------------------------------

#ifndef __GUI_PROGRESSBAR_H
#define __GUI_PROGRESSBAR_H

#include "gui_window.h"


class GUI_ProgressBar_OLD
{
public:
	GUI_ProgressBar_OLD( GUIWindow* parent, int x, int y, int w, int h, float max, float start = 0 );
	~GUI_ProgressBar_OLD();

	void Show();

	void SetValue( float value );
	void Increment( float inc );

protected:
	float progress;
	float maxValue;

	RectI _rect;
	float _kx;
};


class GUI_ProgressBar
{
public:
	GUI_ProgressBar( GUIWindow* parent, const RectI& area, int start = 0 );
	~GUI_ProgressBar();

	void Show();

	void SetVMinMax( int min, int max );

	void SetValue( int value );
	int GetValue() { return _value; }

	void Increment();

protected:
	void refresh();

	GUIWindow* _parent;

	bool _is_shown;
	int _value;
	int _min, _max;

	RectI _rect;
	float _kx;
};

#endif
