//---------------------------------------------------------------------------
//
// Name:        tws_sniper.h
// Author:      Gabriel Ferri
// Created:     01/12/2011
// Description: Sniper class declaration
//
//---------------------------------------------------------------------------
#ifndef __TWS_SNIPER_H
#define __TWS_SNIPER_H

#include "sniperad.h"

#define SNIPER_LEFT_USABLE			1
#define SNIPER_RIGHT_USABLE			752


//codici di errore centraggio
#define CEN_ERR_NONE		STATUS_OK
#define CEN_ERR_EMPTY		STATUS_EMPTY
#define CEN_ERR_L_BLOCKED	STATUS_L_BLK
#define CEN_ERR_R_BLOCKED	STATUS_R_BLK
#define CEN_ERR_B_BLOCKED	STATUS_B_BLK
#define CEN_ERR_NOMIN		STATUS_NO_MIN
#define CEN_ERR_BUF_FULL	STATUS_BUF_FULL
#define CEN_ERR_ENCODER		STATUS_ENCODER
#define CEN_ERR_TOO_BIG		STATUS_TOO_BIG
#define CEN_ERR_DIMX		256
#define CEN_ERR_DIMY		257


#define LASMSGON        1
#define LASMSGOFF       0

#define SNIPER1_ZEROTHETA_ROT		90
#define SNIPER2_ZEROTHETA_ROT		90



// Classe SniperModule per la gestione del modulo di scansione Sniper
class SniperModule
{
public:
	// Costruttori/distruttori
	SniperModule( void* comInt, unsigned char address );
	SniperModule();
	~SniperModule();

	char* GetVersion();

	int Suspend();
	int ChangeAddress( int address );

	void CheckActivation(void);
	int IsExpired();
	int GetDaysLeft( void );
	void GetActivationCode( unsigned char* code );
	int Activate( unsigned char* code );
	int ActivateAlign( unsigned char* code );

	int SoftReset(void);
	int HardReset(void);

	int Zero_Cmd(void);

	int GetFrames( int &nFrame, int* frames );

	int SetWindow( int left, int right );
	void GetWindow( int& left, int& right )  { left = leftw; right = rightw; }

	//GF_THREAD - START
	int StartFirst();
	int GetFirst( int& status, int& angle, float& position, float& shadow );

	int MeasureOnce( int& status, int& angle, float& position, float& shadow );
	int MeasureOnce_Pixels( int& status, int& angle, int& position, int& shadow );

	void SetKPixelUm( float k ) { k_pixel_um = k; }
	float GetKPixelUm() { return k_pixel_um; }
	//GF_THREAD - END

	void* GetComPort() { return m_comInt; }


	int GetEncoder(void); // Ritorna contatore encoder

	int SetScalLine( int line );
	int ScanLine( unsigned char* buf );

	void Enable() { Error = 0; disabled = 0; } // Disabilita funzionalita classe
	void Disable()  { Error = 1; disabled = 1; } // Abilita funzionalita classe
	int IsEnabled() { return !( Error || disabled ); }
	int IsOnError() { return Error; }
	void SetError() { Error = 1; }

	int SetUseEveryFrames(void);
	int SetUseHalfFrames(void);
	int SetPulsesForFPGA(int numSteps);
	int SetUseStandardAlgorithm(void);
	int SetUseModifiedAlgorithm(void);
	int SetUseFirstMinimum(void);
	int SetUseAbsoluteMinimum(void);

private:
	void* m_comInt;			// interfaccia comunicazione seriale
	char m_address;			// indirizzo associato allo Sniper

	int leftw, rightw;					// Estremi sinistro/destro della finestra

	int Error;						// Flag di segnalazione di errori avvenuti sul modulo sniper
	unsigned char is_expired;		// Flag di segnalazione stato attivazione
	unsigned short days_left;		// giorni mancanti alla scadenza, se 0xFFFF "illimitata"

	char version_str[20];			// Versione del modulo sniper
	unsigned char sniper_version;
	unsigned char sniper_revision;
	unsigned char fpga_version;
	unsigned short vga_version;

	int disabled;				// Disabilita le funzionalita' della classe sniper
								// (utilizzata per le applicazioni stand-alone senza hw dedicato)

	unsigned char rx_data[512];

	int use_std_algorithm;
	int pulses_for_fpga;
	int use_full_frames;
	int use_first_min;

	float k_pixel_um;

	int Send_Command( unsigned char Cmd, unsigned char *RxData);
	int Send_Command_xBytes( unsigned char Cmd, unsigned char *TxData, unsigned short TxDataLen, unsigned char *RxData);

	int Read_Reg(unsigned char Cmd, unsigned char RegAdd, unsigned short *Value );
	int Write_Reg( unsigned char Cmd, unsigned char RegAdd, unsigned short Value );
	int Read_Reg_Micro( unsigned char RegAdd, unsigned short *Value);
	int Write_Reg_Micro( unsigned char RegAdd, unsigned short Value);
	int Read_Reg_FPGA( unsigned char RegAdd, unsigned short *Value);
	int Write_Reg_FPGA( unsigned char RegAdd, unsigned short Value);
	int Read_Reg_VGA( unsigned char RegAdd, unsigned short *Value);
	int Write_Reg_VGA( unsigned char RegAdd, unsigned short Value);

	void setError();
};


#endif
