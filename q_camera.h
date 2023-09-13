//---------------------------------------------------------------------------
//
// Name:        q_camera.h
// Author:      Gabriel Ferri
// Created:     23/02/2012
// Description: Camera functions definition
//
//---------------------------------------------------------------------------

#ifndef __Q_CAMERA_H
#define __Q_CAMERA_H

#include <boost/function.hpp>
#include <boost/bind.hpp>


typedef boost::function<bool(int&)> CameraTeachingKeysCallback;

int CameraTeaching( float& x, float& y, char* title, int camera, int crossBox = 0, CameraTeachingKeysCallback keyCallback = 0 );

bool CameraControls_Save();
bool CameraControls_Load( bool check = true );

int fn_CameraTeaching_Head();
int fn_CameraTeaching_Ext();


#endif
