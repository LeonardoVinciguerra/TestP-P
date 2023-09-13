//---------------------------------------------------------------------------
// Name:        q_inifile.h
// Author:      Gabriel Ferri
// Created:     11/07/2012
// Description:
//---------------------------------------------------------------------------

#ifndef __Q_INIFILE_H
#define __Q_INIFILE_H


int ReadIniFile();


bool Get_UseExtCam();
bool Get_ShowAssemblyTime();
bool Get_WindowedMode();
bool Get_UseNetwork();
bool Get_UseReed();
bool Get_CheckUart();
#ifdef __SNIPER
bool Get_UseSniper( int num );
#endif
bool Get_UseBrush( int num );
bool Get_UseStep( int num );
bool Get_Tools20();
bool Get_WriteErrorLog();
bool Get_WriteDiscardLog();
bool Get_UseCommonFeederDir();
bool Get_UseQMode();
bool Get_UseCam();
bool Get_UseCpu();
bool Get_UseFinec();
bool Get_UseMotorhead();
bool Get_UseFox();
bool Get_FoxLog();
bool Get_DemoMode();
bool Get_AsciiqIdMode();
bool Get_MotorheadOnUart();
bool Get_UseSteppers();
bool Get_Res1600();
bool Get_Res1920();
bool Get_ShutterEnable();

void Reset_UseExtCam();
bool Get_WorkingLog();
bool Reset_WorkingLog();

int Get_FoxCurrent();
int Get_FoxTime();

#endif
