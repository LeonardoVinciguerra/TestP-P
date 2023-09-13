/*
 * q_conveyor.h
 *
 *  Created on: 15/ott/2014
 *      Author: dbelloni
 */

#ifndef Q_CONVEYOR_H_
#define Q_CONVEYOR_H_

#include "q_oper.h"

#define CONV_MICROSTEP		16

#define CONV_HOME			0
#define CONV_ZERO			1
#define CONV_REF			2
#define CONV_ACT			3
#define CONV_STEP1			4
#define CONV_STEP2			5
#define CONV_STEP3			6

bool Get_ConveyorEnabled();
bool InitConveyor();
bool Get_ConveyorInit();
bool Get_UseConveyor();
bool SearchConveyorZero();
bool WaitConveyorOrigin();
bool MoveConveyor( float mm, int mode = ABS_MOVE );
bool WaitConveyor();
bool MoveConveyorAndWait( float mm, int mode = ABS_MOVE );
float GetConveyorPosition( int pos = CONV_ACT );
bool SetConveyorPosition( int pos );
bool MoveConveyorPosition( int pos );
bool UpdateConveyorSpeedAcc( int speed, int accDec );
int GetConveyorActualSteps();
bool ConveyorLimitsEnable( bool state );
int UpdateConveyor();

#endif /* Q_CONVEYOR_H_ */
