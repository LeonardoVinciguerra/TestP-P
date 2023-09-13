//---------------------------------------------------------------------------
//
// Name:        q_camera.cpp
// Author:      Gabriel Ferri
// Created:     23/02/2012
// Description: Camera functions implementation
//
//---------------------------------------------------------------------------
#include "q_camera.h"

#include "video_dev.h"

#include "gui_functions.h"
#include "q_graph.h"
#include "tv.h"
#include "Timer.h"
#include "c_combo.h"
#include "q_files_new.h"
#include "q_oper.h"
#include "q_tabe.h"
#include "q_help.h"
#include "msglist.h"
#include "keyutils.h"
#include "q_debug.h"
#include "q_inifile.h"
#include "q_opt2.h"

#include "lnxdefs.h"

#include <mss.h>


extern struct CfgHeader QHeader;


int CameraTeaching_Keys( float& x_coord, float& y_coord, int crossBox = 0, CameraTeachingKeysCallback keyCallback = 0 );

//---------------------------------------------------------------------------------
// Apprendimento con telecamera
//---------------------------------------------------------------------------------
int CameraTeaching( float& x, float& y, char* title, int camera, int crossBox, CameraTeachingKeysCallback keyCallback )
{
	// setta titolo finestra video
	Set_Tv_Title( title );
	// setta tipo croce
	setCross( CROSS_CURR_MODE, crossBox );

	// apre finestra video
	Set_Tv( 1, camera );

	MoveComponentUpToSafePosition(1);
	MoveComponentUpToSafePosition(2);
	PuntaZPosWait(1);
	PuntaZPosWait(2);

	// posiziona testa
	NozzleXYMove( x, y );
	Wait_PuntaXY();

	int ret;
	float x_coord = x;
	float y_coord = y;

	while( 1 )
	{
		// gestione tasti apprendimento
		ret = CameraTeaching_Keys( x_coord, y_coord, crossBox, keyCallback );

		if( ret == K_ESC )
		{
			break;
		}

		if( ret == K_ENTER )
		{
			break;
		}

		NozzleXYMove( x_coord, y_coord );
		x_coord = GetLastXPos();
		y_coord = GetLastYPos();

		//TODO: aggiorna posizione a video

		Wait_PuntaXY();
	}

	Set_Tv( 0 );

	return ret;
}


//---------------------------------------------------------------------------------
// Incrementa le quantita` di spostamento a seconda del tempo del tasto premuto
//---------------------------------------------------------------------------------
#define STEP_INC_PERCENTAGE     1.05f
#define STEP_START_AT           10
#define STEP_STOP_AT            100
#define STEP_TIMEOUT_MS         1000

float AdjustMovement( int key )
{
	static float currentStep = 0.f;
	static int pressCount = 0;
	static int prevKey = -1;
	static Timer timeOut;

	if( key != prevKey )
	{
		prevKey = key;
		pressCount = 0;
		timeOut.start();

		switch( key )
		{
		case K_ALT_RIGHT:
		case K_ALT_UP:
			currentStep = 10.f;
			break;

		case K_ALT_LEFT:
		case K_ALT_DOWN:
			currentStep = -10.f;
			break;

		case K_CTRL_RIGHT:
		case K_CTRL_UP:
			currentStep = 1.f;
			break;

		case K_CTRL_LEFT:
		case K_CTRL_DOWN:
			currentStep = -1.f;
			break;

		case K_SHIFT_RIGHT:
		case K_SHIFT_UP:
			currentStep = 0.1f;
			break;

		case K_SHIFT_LEFT:
		case K_SHIFT_DOWN:
			currentStep = -0.1f;
			break;

		case K_RIGHT:
			currentStep = QHeader.PassoX;
			break;

		case K_LEFT:
			currentStep = -QHeader.PassoX;
			break;

		case K_UP:
			currentStep = QHeader.PassoY;
			break;

		case K_DOWN:
			currentStep = -QHeader.PassoY;
			break;

		default:
			prevKey = -1;
			return 0.f; // errore: tasto non gestito
		}
	}

	// resetta il contatore dopo un certo tempo
	if( timeOut.getElapsedTimeInMilliSec() > STEP_TIMEOUT_MS )
	{
		pressCount = 0;
	}
	timeOut.start();

	pressCount++;

	if( pressCount < STEP_STOP_AT )
	{
		if( pressCount > STEP_START_AT )
		{
			currentStep *= STEP_INC_PERCENTAGE;
		}
	}

	return currentStep;
}


//---------------------------------------------------------------------------------
// Gestione tasti durante apprendimento con telecamera
//---------------------------------------------------------------------------------
int CameraTeaching_Keys( float& x_coord, float& y_coord, int crossBox, CameraTeachingKeysCallback keyCallback )
{
	int key = Handle();

	ResetTimerConfirmRequiredBeforeNextXYMovement();

	float moveX = 0.f;
	float moveY = 0.f;

	if( keyCallback )
	{
		if( keyCallback( key ) == true )
		{
			// tasto gestito, non propagare l'evento
			return key;
		}
	}

	float step = AdjustMovement( key );

	switch( key )
	{
		case K_SHIFT_NUMPAD_1:
			x_coord = QParam.LX_mincl+((QParam.LX_maxcl-QParam.LX_mincl)/6);
			y_coord = QParam.LY_mincl+((QParam.LY_maxcl-QParam.LY_mincl)/6);
			break;
		case K_SHIFT_NUMPAD_2:
			x_coord = QParam.LX_mincl+3*((QParam.LX_maxcl-QParam.LX_mincl)/6);
			y_coord = QParam.LY_mincl+((QParam.LY_maxcl-QParam.LY_mincl)/6);
			break;
		case K_SHIFT_NUMPAD_3:
			x_coord = QParam.LX_mincl+5*((QParam.LX_maxcl-QParam.LX_mincl)/6);
			y_coord = QParam.LY_mincl+((QParam.LY_maxcl-QParam.LY_mincl)/6);
			break;
		case K_SHIFT_NUMPAD_4:
			x_coord = QParam.LX_mincl+((QParam.LX_maxcl-QParam.LX_mincl)/6);
			y_coord = QParam.LY_mincl+3*((QParam.LY_maxcl-QParam.LY_mincl)/6);
			break;
		case K_SHIFT_NUMPAD_5:
			x_coord = QParam.LX_mincl+3*((QParam.LX_maxcl-QParam.LX_mincl)/6);
			y_coord = QParam.LY_mincl+3*((QParam.LY_maxcl-QParam.LY_mincl)/6);
			break;
		case K_SHIFT_NUMPAD_6:
			x_coord = QParam.LX_mincl+5*((QParam.LX_maxcl-QParam.LX_mincl)/6);
			y_coord = QParam.LY_mincl+3*((QParam.LY_maxcl-QParam.LY_mincl)/6);
			break;
		case K_SHIFT_NUMPAD_7:
			x_coord = QParam.LX_mincl+((QParam.LX_maxcl-QParam.LX_mincl)/6);
			y_coord = QParam.LY_mincl+5*((QParam.LY_maxcl-QParam.LY_mincl)/6);
			break;
		case K_SHIFT_NUMPAD_8:
			x_coord = QParam.LX_mincl+3*((QParam.LX_maxcl-QParam.LX_mincl)/6);
			y_coord = QParam.LY_mincl+5*((QParam.LY_maxcl-QParam.LY_mincl)/6);
			break;
		case K_SHIFT_NUMPAD_9:
			x_coord = QParam.LX_mincl+5*((QParam.LX_maxcl-QParam.LX_mincl)/6);
			y_coord = QParam.LY_mincl+5*((QParam.LY_maxcl-QParam.LY_mincl)/6);
			break;

		case K_RIGHT:
		case K_ALT_RIGHT:
		case K_CTRL_RIGHT:
		case K_SHIFT_RIGHT:
		case K_LEFT:
		case K_ALT_LEFT:
		case K_CTRL_LEFT:
		case K_SHIFT_LEFT:
			moveX = step;
			moveY = 0.f;
			break;

		case K_UP:
		case K_ALT_UP:
		case K_CTRL_UP:
		case K_SHIFT_UP:
		case K_DOWN:
		case K_ALT_DOWN:
		case K_CTRL_DOWN:
		case K_SHIFT_DOWN:
			moveX = 0.f;
			moveY = step;
			break;

		case K_F3:
			setCross( CROSS_NEXT_MODE, crossBox );
			break;

		case K_F12:
			set_zoom( !get_zoom() );
			break;

		default:
			break;
	}

	if( moveX || moveY )
	{
		if( Check_XYMove( x_coord + moveX, y_coord + moveY ) )
		{
			x_coord += moveX;
			y_coord += moveY;
		}
	}

	return key;
}




	//----------------------------//
	//  Camera controls functions //
	//----------------------------//

extern VideoDevice videoDev_Head;
extern VideoDevice videoDev_Ext;


#define CC_CONTROL_W           19

boost::mutex cc_mutex;

int cc_index = 0;
V4L2Control* cc_ctrl = 0;
int cc_value;
char cc_value_str[CC_CONTROL_W+1];
VideoDevice* cc_vd = 0;


//---------------------------------------------------------------------------------
// Salva dati: camera controls
//---------------------------------------------------------------------------------
bool CameraControls_Save()
{
	while( !cc_mutex.try_lock() )
	{
		delay( 1 );
	}

	V4L2Control* ctrl;
	SCameraControls controls;

	int i = 0;
	int index = 0;

	while( videoDev_Head.GetV4LDevice()->getControlAt( index, &ctrl ) )
	{
		controls.cam[i] = CAMERACONTROLS_HEAD;
		controls.id[i] = ctrl->ctrl.id;

		int value;
		videoDev_Head.GetV4LDevice()->getControlValue( ctrl->ctrl.id, value );
		controls.value[i] = value;

		i++;
		index++;
	}

	index = 0;

	while( videoDev_Ext.GetV4LDevice()->getControlAt( index, &ctrl ) )
	{
		controls.cam[i] = CAMERACONTROLS_EXT;
		controls.id[i] = ctrl->ctrl.id;

		int value;
		videoDev_Ext.GetV4LDevice()->getControlValue( ctrl->ctrl.id, value );
		controls.value[i] = value;

		i++;
		index++;
	}

	while( i < CAMERACONTROLS_NUM )
	{
		controls.cam[i] = CAMERACONTROLS_EMPTY;
		i++;
	}

	cc_mutex.unlock();
	return CameraControls_Write( controls );
}

//---------------------------------------------------------------------------------
// Carica dati: camera controls
//---------------------------------------------------------------------------------
bool CameraControls_Load( bool check )
{
	if( !Get_UseCam() )
	{
		return true;
	}

	while( !cc_mutex.try_lock() )
	{
		delay( 1 );
	}

	SCameraControls controls;

	if( !CameraControls_Read( controls, false ) )
	{
		cc_mutex.unlock();
		return false;
	}


	for( int i = 0; i < CAMERACONTROLS_NUM; i++ )
	{
		V4L2Control* ctrl;
		if( controls.cam[i] == CAMERACONTROLS_HEAD )
		{
			if( videoDev_Head.GetV4LDevice()->getControl( controls.id[i], &ctrl ) )
			{
				//GF_19_06_12: workaround to fix previous implementation bug
				if( controls.id[i] == V4L2_CID_EXPOSURE_AUTO )
				{
					for( int c = 0; c < ctrl->choices_list.size(); c++ )
					{
						if( ctrl->choices_list[c].index == 1 )
						{
							videoDev_Head.GetV4LDevice()->setControlValue( V4L2_CID_EXPOSURE_AUTO, c );
							break;
						}
					}
				}
				else
				{
					if( check == true )
					{
						if( (controls.id[i] != V4L2_CID_BRIGHTNESS) && (controls.id[i] != V4L2_CID_CONTRAST) )
						{
							videoDev_Head.GetV4LDevice()->setControlValue( controls.id[i], controls.value[i] );
						}
					}
					else
						videoDev_Head.GetV4LDevice()->setControlValue( controls.id[i], controls.value[i] );
				}
			}
		}
		else if( controls.cam[i] == CAMERACONTROLS_EXT )
		{
			if( videoDev_Ext.GetV4LDevice()->getControl( controls.id[i], &ctrl ) )
			{
				videoDev_Ext.GetV4LDevice()->setControlValue( controls.id[i], controls.value[i] );
			}
		}
	}

	cc_mutex.unlock();
	return true;
}


//---------------------------------------------------------------------------------
// Gestione info-video: camera controls
//---------------------------------------------------------------------------------
#define VIDEO_CALLBACK_Y      29

void CameraControls_VideoCallback( void* parentWin )
{
	while( !cc_mutex.try_lock() )
	{
		delay( 1 );
	}

	if( cc_ctrl == 0 )
	{
		// notifica assenza controlli
		cc_mutex.unlock();
		return;
	}

	C_Combo c_name( 3, VIDEO_CALLBACK_Y, "Control :", 42, CELL_TYPE_TEXT );
	c_name.SetTxt( (char *)cc_ctrl->ctrl.name );

	C_Combo c_value( 56, VIDEO_CALLBACK_Y, "", CC_CONTROL_W, CELL_TYPE_TEXT );
	c_value.SetTxt( cc_value_str );

	GUI_Freeze();
	CWindow* parent = (CWindow*)parentWin;
	c_name.Show( parent->GetX(), parent->GetY() );
	c_value.Show( parent->GetX(), parent->GetY() );

	parent->DrawText( 20, VIDEO_CALLBACK_Y+2, MsgGetString(Msg_00306) );
	parent->DrawText( 20, VIDEO_CALLBACK_Y+3, MsgGetString(Msg_00307) );
	GUI_Thaw();

	cc_mutex.unlock();
}

//---------------------------------------------------------------------------------
// Gestione tasti: camera controls
//    F5/F6: scorre elenco controlli disponibili
//    F7/F8: modifica valore controllo corrente
//---------------------------------------------------------------------------------
bool CameraControls_KeyCallback( int& key )
{
	if( key == K_F5 || key == K_F6 )
	{
		while( !cc_mutex.try_lock() )
		{
			delay( 1 );
		}

		if( key == K_F6 )
		{
			cc_index++;
			if( cc_index >= cc_vd->GetV4LDevice()->getControlsCount() )
				cc_index = 0;
		}
		else
		{
			cc_index--;
			if( cc_index < 0 )
				cc_index = cc_vd->GetV4LDevice()->getControlsCount() - 1;
		}

		cc_vd->GetV4LDevice()->getControlAt( cc_index, &cc_ctrl );

		if( cc_ctrl )
		{
			cc_vd->GetV4LDevice()->getControlValue( cc_ctrl->ctrl.id, cc_value );

			if( cc_ctrl->ctrl.type == V4L2_CTRL_TYPE_MENU )
			{
				snprintf( cc_value_str, CC_CONTROL_W+1, "%s", cc_ctrl->choices_list[cc_value].name );
			}
			else
			{
				snprintf( cc_value_str, CC_CONTROL_W+1, "%d", cc_value );
			}
		}

		cc_mutex.unlock();
	}
	else if( key == K_F7 || key == K_F8 )
	{
		while( !cc_mutex.try_lock() )
		{
			delay( 1 );
		}

		if( cc_ctrl )
		{
			if( cc_ctrl->ctrl.type == V4L2_CTRL_TYPE_MENU )
			{
				if( cc_value > 0 && key == K_F7 )
					cc_value--;
				else if( cc_value < cc_ctrl->choices_list.size()-1 && key == K_F8 )
					cc_value++;

				cc_vd->GetV4LDevice()->setControlValue( cc_ctrl->ctrl.id, cc_value );
				snprintf( cc_value_str, CC_CONTROL_W+1, "%s", cc_ctrl->choices_list[cc_value].name );
			}
			else
			{
				if( cc_value > cc_ctrl->ctrl.minimum && key == K_F7 )
					cc_value -= cc_ctrl->ctrl.step;
				else if( cc_value < cc_ctrl->ctrl.maximum && key == K_F8 )
					cc_value += cc_ctrl->ctrl.step;

				cc_value = MID( cc_ctrl->ctrl.minimum, cc_value, cc_ctrl->ctrl.maximum );
				print_debug( "%d, %d, %d\n ", cc_ctrl->ctrl.minimum, cc_value, cc_ctrl->ctrl.maximum );

				cc_vd->GetV4LDevice()->setControlValue( cc_ctrl->ctrl.id, cc_value );
				snprintf( cc_value_str, CC_CONTROL_W+1, "%d", cc_value );
			}
		}

		cc_mutex.unlock();
	}

	return false;
}


int fn_CameraTeaching_Head()
{
	cc_vd = &videoDev_Head;

	float x = 0.f;
	float y = 0.f;

	showVideoControls( false );
	setLiveVideoCallback( CameraControls_VideoCallback );

	// imposta il primo controllo
	int key = K_F6;
	cc_index = -1;
	CameraControls_KeyCallback( key );

	CameraTeaching( x, y, "Camera Head - Advanced controls setting", CAMERA_HEAD, 0, CameraControls_KeyCallback );
	setLiveVideoCallback( 0 );
	showVideoControls( true );

	// salvare dati ?
	if( W_Deci( 1, MsgGetString(Msg_00790) ) )
	{
		CameraControls_Save();
	}
	else
	{
		CameraControls_Load();
	}

	return 1;
}

int fn_CameraTeaching_Ext()
{
	cc_vd = &videoDev_Ext;

	float x = QParam.AuxCam_X[0];
	float y = QParam.AuxCam_Y[0];

	showVideoControls( false );
	setLiveVideoCallback( CameraControls_VideoCallback );

	// imposta il primo controllo
	int key = K_F6;
	cc_index = -1;
	CameraControls_KeyCallback( key );

	CameraTeaching( x, y, "Camera Ext - Advanced controls setting", CAMERA_EXT, 0, CameraControls_KeyCallback );
	setLiveVideoCallback( 0 );
	showVideoControls( true );

	// salvare dati ?
	if( W_Deci( 1, MsgGetString(Msg_00790) ) )
	{
		CameraControls_Save();
	}
	else
	{
		CameraControls_Load();
	}

	return 1;
}
