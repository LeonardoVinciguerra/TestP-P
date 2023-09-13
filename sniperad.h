//-----------------------------------------------------------------------------
// File: SniperAd.h
//
// Desc: Definitions of used registers/commands addresses
//
//-----------------------------------------------------------------------------
#ifndef __SNIPERAD_H
#define __SNIPERAD_H

// versioni del chip VGA
#define VGA_VERSION_1324			0x1324

// indirizzo registri del MICRO
#define MICRO_REG_MIN_SHADOW_DELTA	0
#define MICRO_REG_TEMP_REGISTER		1
#define MICRO_REG_STATUS_REGISTER	2

// indirizzo registri della FPGA
#define FPGA_REG_THRESHOLD			0
#define FPGA_REG_COMMAND			1
#define FPGA_REG_MODE				2
#define FPGA_REG_LEFT_EDGE			3
#define FPGA_REG_RIGHT_EDGE			4
#define FPGA_REG_ENCODER			5
#define FPGA_REG_STATUS				6
#define FPGA_REG_RAM				7
#define FPGA_REG_PULSES				8
#define FPGA_REG_VERSION			9

// indirizzo registri della VGA
#define VGA_CHIP_VERSION			0x00
#define VGA_COLUMN_START			0x01
#define VGA_ROW_START				0x02
#define VGA_WINDOW_HEIGHT			0x03
#define VGA_WINDOW_WIDTH			0x04
#define VGA_HORIZONTAL_BLANKING		0x05
#define VGA_VERTICAL_BLANKING		0x06
#define VGA_CHIP_CONTROL			0x07
#define VGA_TOTAL_SHUTTER_WIDTH		0x0B
#define VGA_RESET					0x0C
#define VGA_ANALOG_GAIN				0x35
#define VGA_BL_CALIB_CONTROL		0x47
#define VGA_BL_CALIBRATION			0x48
#define VGA_AGC_AEC_ENABLE			0xAF

// Comandi implemetati
#define GET_VERSION				1
#define READ_MICRO_REG			2
#define WRITE_MICRO_REG			3
#define READ_FPGA_REG			4
#define WRITE_FPGA_REG			5
#define READ_VGA_REG			6
#define WRITE_VGA_REG			7
#define READ_DATA_BUF			8

#define ACTIVATION_READ			10

#define READ_RAM_BYTE			100
#define READ_RAM_BLOCK			101
#define MEASURE_ONCE			102
#define RESET_ENCODER			103
#define SCAN_LINE				104
#define START_CENTERING			105
#define USE_EVERY_FRAMES		106
#define USE_HALF_FRAMES			107
#define USE_INC_PULSES			108
#define USE_DEC_PULSES			109
#define GET_RESULT				110
#define USE_ALGO_STD			111
#define USE_ALGO_MOD			112
#define RESET_FPGA_VGA			113
#define RESET_MICRO				114
#define SUSPENDSERIAL_CMD		115
#define CHANGEADDRESS_CMD		116
#define EEPROMRESET_CMD			117
#define START_CENTERING_TIMER	118
#define READ_SHADOW_BUF			119
#define READ_LEFT_BUF			120
#define READ_ENCODER_BUF		121
#define LIGHTER_ON				122
#define LIGHTER_OFF				123
#define USE_LEFT_WINDOW			124
#define USE_RIGHT_WINDOW		125
#define FIND_FIRST_MIN			126
#define FIND_ABS_MIN			127
#define ACTIVATION_CHECK		128
#define ACTIVATION_CODE			129
#define ACTIVATION_ACTIVATE		130
#define ACTIVATION_ALIGN		131


// Possibili risultati dell'elaborazione
#define STATUS_OK				1
#define STATUS_EMPTY			2
#define STATUS_L_BLK			3
#define STATUS_R_BLK			4
#define STATUS_B_BLK			5
#define STATUS_BUF_FULL			6
#define STATUS_NO_MIN			7
#define STATUS_ENCODER			8
#define STATUS_TOO_BIG			9
#define STATUS_COM_ERROR		10

#endif
