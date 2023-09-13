//---------------------------------------------------------------------------
//
// Name:        q_param_new.h
// Author:      Gabriel Ferri
// Created:     15/09/2011
// Description: Quadra configuration parameters
//
//---------------------------------------------------------------------------

#ifndef __Q_PARAM_H
#define __Q_PARAM_H

#include "c_window.h"


int fn_PIDBrushlessParams( CWindow* parent );
int fn_SpeedLimits( CWindow* parent );
int fn_FeederTimings( CWindow* parent );
int fn_ExtCameraAdvancedParams( CWindow* parent );
int fn_OtherAdvancedParams( CWindow* parent );

#endif
