//---------------------------------------------------------------------------
// Name:        c_pan.h
// Author:      Gabriel Ferri
// Created:     16/11/2011
// Description: CPan class definition
//---------------------------------------------------------------------------

#ifndef __C_PAN_H
#define __C_PAN_H

#include "c_window.h"


class CPan : public CWindow
{
public:
	CPan( int y, int n, ... );
	~CPan() { Hide(); }
};

#endif
