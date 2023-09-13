//---------------------------------------------------------------------------
//
// Name:        q_conf.h
// Author:      Gabriel Ferri
// Created:     15/09/2011
// Description: Quadra configuration parameters
//
//---------------------------------------------------------------------------

#ifndef __Q_CONF_H
#define __Q_CONF_H

#include "c_window.h"


int fn_FeedersDefPosition();
int fn_SpeedAcceleration();
int fn_SpeedAccelerationTable();
int fn_ToolsParams();
int fn_NozzleOffsetCalibration();
int fn_PlacementMapping();
int fn_WorkingModes();
int fn_ControlParams();
int fn_AxesCalibration();
int fn_RotationCenterCalibration();
int fn_NozzleRotationAdjustment();
int fn_UnitOriginPosition();
int fn_PlacementAreaCalib();
int fn_LimitsPositions();
int fn_DispenserPointsPosition( CWindow* parent, int ndisp, int test_point );
int fn_DispenserParams( CWindow* parent, int ndisp );
int fn_DispenserOffset( CWindow* parent, int ndisp );

#ifdef __SNIPER
int fn_DeltaZCalibration();
#endif

int fn_CenteringCameraParams();
int fn_VisionParams();
int fn_WarmUpParams();
int fn_MachineInfo();
int fn_MachineID();

#endif
