//---------------------------------------------------------------------------
//
// Name:        comaddr.h
// Author:      Gabriel Ferri
// Created:     28/10/2008 17.10
// Description: com port addresses declaration
//
//---------------------------------------------------------------------------

#ifndef __COMADDR_H
#define __COMADDR_H

#define PC_COM			"/dev/ttyS0"
#define UARTPCI_COM1	"/dev/ttyS4"
#define UARTPCI_COM2	"/dev/ttyS5"
#define UARTPCI_COM3	"/dev/ttyS6"
#define UARTPCI_COM4	"/dev/ttyS7"
#define USB_COM0		"/dev/ttyUSB0"

#define EXTCAM_COM_PORT		PC_COM
#define CPU_COM_PORT		UARTPCI_COM1
#define FOX_COM_PORT		UARTPCI_COM2
#define SNIPER_COM_PORT		UARTPCI_COM3

#define EXTCAM_BAUD			B9600
#define CPU_310_BAUD		PCIbaud_8333
#define CPU_320_BAUD		B115200
#define FOX_BAUD			B115200
#define SNIPER_BAUD			B230400
#define STEPPERAUX_BAUD		B115200
#define MOTORHEAD_BAUD		B460800

#define SNIPER1_ADDR		1
#define SNIPER2_ADDR		2

#endif
