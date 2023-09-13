//---------------------------------------------------------------------------
//
// Name:        tws_sc.h
// Author:      Gabriel Ferri
// Created:     07/05/2008 (mod: 20/06/2011)
// Description: Serial communication functions declaration
//
//---------------------------------------------------------------------------
#ifndef __TWS_SC_H
#define __TWS_SC_H


int TWSBus1_Send_FD( void* comInt, unsigned char NSlave, unsigned char Cmd, unsigned char* TxBuf, unsigned short TxLen, unsigned char* RxBuf, unsigned short* RxLen );
int TWSBus1_Send_FD( void* comInt, unsigned char NSlave, unsigned char Cmd, unsigned char* TxBuf, unsigned short TxLen );
int TWSBus1_Send_Command_FD( void* comInt, unsigned char NSlave, unsigned char Cmd, unsigned char* RxData );
int TWSBus1_Send_Command_FD( void* comInt, unsigned char NSlave, unsigned char Cmd, unsigned short TxData, unsigned char* RxData );

int TWSBus1_Send_HD( void* comInt, unsigned char NSlave, unsigned char Cmd, unsigned char* TxBuf, unsigned short TxLen, unsigned char* RxBuf, unsigned short* RxLen );
int TWSBus1_Send_HD( void* comInt, unsigned char NSlave, unsigned char Cmd, unsigned char* TxBuf, unsigned short TxLen );
int TWSBus1_Send_Command_HD( void* comInt, unsigned char NSlave, unsigned char Cmd, unsigned char* RxData );
int TWSBus1_Send_Command_HD( void* comInt, unsigned char NSlave, unsigned char Cmd, unsigned short TxData, unsigned char* RxData );

#endif
