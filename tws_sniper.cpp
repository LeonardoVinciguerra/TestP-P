//---------------------------------------------------------------------------
//
// Name:        tws_sniper.cpp
// Author:      Gabriel Ferri
// Created:     01/12/2011
// Description: Sniper class implementation
//
//---------------------------------------------------------------------------
#ifdef __SNIPER
#include "tws_sniper.h"

#include <boost/thread.hpp>
#include <stdio.h>
#include <time.h>
#include "tws_sc.h"
#include "q_snprt.h"
#include "getmac.h"
#include "lnxdefs.h"

#include <mss.h>



	//-----------------//
	//   Global Vars   //
	//-----------------//

boost::mutex sniperSerialMutex;



//---------------------------------------------------------------------------------
// Costruttore della classe SniperModule.
// Parametri di ingresso:
//    m_comIntRef: porta seriale
//    address: indirizzo del modulo Sniper
//---------------------------------------------------------------------------------
SniperModule::SniperModule(  void* comInt, unsigned char address  )
{
	m_comInt = comInt;
	m_address = address;

	Error = 0;
	is_expired = 0;
	disabled = 0;

	use_full_frames = -1;
	pulses_for_fpga = -1;
	use_std_algorithm = -1;
	use_first_min = -1;

	k_pixel_um = 1.f;


	for( int phase = 0; phase <= 21 ; phase++ )
	{
		switch( phase )
		{
			case 0:
				SoftReset();
				break;
			case 1:
				// Legge firmware modulo SNIPER
				GetVersion();
				break;
			case 2:
				// Legge firmware sensore VGA
				Read_Reg_VGA( VGA_CHIP_VERSION, &vga_version );
				break;
			case 3:
				// FPGA SETTINGS
				Write_Reg_FPGA( FPGA_REG_THRESHOLD, 110 );
				break;
			case 4:
				// VGA SETTINGS
				Write_Reg_VGA( VGA_TOTAL_SHUTTER_WIDTH, 3 );
				break;
			case 5:
				Write_Reg_VGA( VGA_ANALOG_GAIN, 16 );
				break;
			case 6:
				// per i seguenti settaggi vedi TN-09-60.pdf
				Write_Reg_VGA( VGA_CHIP_CONTROL, 0x0398 );
				break;
			case 7:
				Write_Reg_VGA( VGA_AGC_AEC_ENABLE, 0x0000 );
				break;
			case 8:
				Write_Reg_VGA( VGA_BL_CALIB_CONTROL, 0x8081 );
				break;
			case 9:
				if( vga_version >= VGA_VERSION_1324 )
					Write_Reg_VGA( VGA_HORIZONTAL_BLANKING, 0x003D );
				else
					Write_Reg_VGA( VGA_HORIZONTAL_BLANKING, 0x002B );
				break;
			case 10:
				if( vga_version >= VGA_VERSION_1324 )
					Write_Reg_VGA( VGA_VERTICAL_BLANKING, 0x0002 );
				else
					Write_Reg_VGA( VGA_VERTICAL_BLANKING, 0x0004 );
				break;
			case 11:
				// dalla "Technical Note MT9V022"
				Write_Reg_VGA( 0x20, 0x03D5 );
				break;
			case 12:
				// setta le dimensioni della finestra
				SetWindow( SNIPER_LEFT_USABLE, SNIPER_RIGHT_USABLE );
				break;
			case 13:
				// setta la linea di scansione
				Write_Reg_VGA( VGA_ROW_START, 240 );
				break;
			case 14:
				Write_Reg_VGA( VGA_WINDOW_HEIGHT, 1 );
				break;
			case 15:
				Send_Command( USE_INC_PULSES, rx_data );
				break;
			case 16:
				// Setta l'algoritmo di ricerca normale per sfruttare la parte dx della finestra
				Send_Command( USE_RIGHT_WINDOW, rx_data );
				break;
			case 17:
				// Legge  dummy line 1 ...
				Send_Command( MEASURE_ONCE, rx_data );
				break;
			case 18:
				// Legge  dummy line 2 ...
				Send_Command( MEASURE_ONCE, rx_data );
				break;
			case 19:
				Write_Reg_Micro ( MICRO_REG_MIN_SHADOW_DELTA, 3 );
				break;
			case 20:
				if( vga_version >= VGA_VERSION_1324 )
					Write_Reg_VGA( VGA_BL_CALIBRATION, 127 );
				break;
			case 21:
				if( sniper_version > 1 || ( sniper_version == 1 && sniper_revision > 4 ) )
					CheckActivation();
				break;
		}

		if( Error )
		{
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Costruttore della classe SniperModule.
// Non esegue nessuna operazione e disabilita tutte le operazioni sulla classe.
//---------------------------------------------------------------------------------
SniperModule::SniperModule()
{
	Error = 0;
	disabled = 1;

	m_comInt = NULL;

	k_pixel_um = 1.f;
}


//---------------------------------------------------------------------------------
// Distruttore della classe Sniper
//---------------------------------------------------------------------------------
SniperModule::~SniperModule()
{
}

//---------------------------------------------------------------------------------
// Setta stato errore macchina
//---------------------------------------------------------------------------------
void SniperModule::setError()
{
	Error = 1;
}

//-----------------------------------------------------------------------------
// Name: Send_Command()
// Desc: return 1 on error, 0 otherwise
//-----------------------------------------------------------------------------
int SniperModule::Send_Command( unsigned char Cmd, unsigned char *RxData )
{
	if( !IsEnabled() )
		return 1;

	if( Error )
		return 0;

	while( !sniperSerialMutex.try_lock() )
	{
		delay( 1 );
	}

	int r = TWSBus1_Send_Command_FD( m_comInt, m_address, Cmd, RxData );
	if( r )
		setError();

	sniperSerialMutex.unlock();
	return r;
}

//-----------------------------------------------------------------------------
// Name: Send_Command_xBytes()
// Desc:
//-----------------------------------------------------------------------------
int SniperModule::Send_Command_xBytes( unsigned char Cmd, unsigned char *TxData, unsigned short TxLen, unsigned char *RxData)
{
	if( !IsEnabled() )
		return 1;

	if( Error )
		return 0;

	while( !sniperSerialMutex.try_lock() )
	{
		delay( 1 );
	}

	unsigned short rx_len = 0;

	int r = TWSBus1_Send_FD( m_comInt, m_address, Cmd, TxData, TxLen, RxData, &rx_len );
	if( r )
		setError();

	sniperSerialMutex.unlock();
	return r;
}

//-----------------------------------------------------------------------------
// Name: Read_Reg()
// Desc:
//-----------------------------------------------------------------------------
int SniperModule::Read_Reg( unsigned char Cmd, unsigned char RegAdd, unsigned short *Value )
{
	if( !IsEnabled() )
		return 1;

	if( Error )
		return 0;

	while( !sniperSerialMutex.try_lock() )
	{
		delay( 1 );
	}

	unsigned char rx_buffer[2];
	unsigned short rx_len = 0;

	int r = TWSBus1_Send_FD( m_comInt, m_address, Cmd, &RegAdd, 1, rx_buffer, &rx_len );
	if( r )
		setError();
	else
		*Value = 256 * rx_buffer[0] + rx_buffer[1];

	sniperSerialMutex.unlock();
	return r;
}

//-----------------------------------------------------------------------------
// Name: Write_Reg()
// Desc:
//-----------------------------------------------------------------------------
int SniperModule::Write_Reg( unsigned char Cmd, unsigned char RegAdd, unsigned short Value )
{
	if( !IsEnabled() )
		return 1;

	if( Error )
		return 0;

	while( !sniperSerialMutex.try_lock() )
	{
		delay( 1 );
	}

	unsigned char tx_buffer[3];
	tx_buffer[0] = RegAdd;			// slave address
	tx_buffer[1] = Value >> 8;		// Value (H)
	tx_buffer[2] = Value;			// Value (L)

	int r = TWSBus1_Send_FD( m_comInt, m_address, Cmd, tx_buffer, 3 );
	if( r )
		setError();

	sniperSerialMutex.unlock();
	return r;
}

//-----------------------------------------------------------------------------
// Name: Read_Reg_Micro()
// Desc:
//-----------------------------------------------------------------------------
int SniperModule::Read_Reg_Micro( unsigned char RegAdd, unsigned short *Value )
{
	return Read_Reg( READ_MICRO_REG, RegAdd, Value );
}

//-----------------------------------------------------------------------------
// Name: Write_Reg_Micro()
// Desc:
//-----------------------------------------------------------------------------
int SniperModule::Write_Reg_Micro( unsigned char RegAdd, unsigned short Value )
{
	return Write_Reg( WRITE_MICRO_REG, RegAdd, Value );
}

//-----------------------------------------------------------------------------
// Name: Read_Reg_FPGA()
// Desc:
//-----------------------------------------------------------------------------
int SniperModule::Read_Reg_FPGA( unsigned char RegAdd, unsigned short *Value )
{
	return Read_Reg( READ_FPGA_REG, RegAdd, Value );
}

//-----------------------------------------------------------------------------
// Name: Write_Reg_FPGA()
// Desc:
//-----------------------------------------------------------------------------
int SniperModule::Write_Reg_FPGA( unsigned char RegAdd, unsigned short Value )
{
	return Write_Reg( WRITE_FPGA_REG, RegAdd, Value );
}

//-----------------------------------------------------------------------------
// Name: Read_Reg_VGA()
// Desc:
//-----------------------------------------------------------------------------
int SniperModule::Read_Reg_VGA( unsigned char RegAdd, unsigned short *Value )
{
	return Read_Reg( READ_VGA_REG, RegAdd, Value );
}

//-----------------------------------------------------------------------------
// Name: Write_Reg_VGA()
// Desc:
//-----------------------------------------------------------------------------
int SniperModule::Write_Reg_VGA( unsigned char RegAdd, unsigned short Value )
{
	return Write_Reg( WRITE_VGA_REG, RegAdd, Value );
}


//---------------------------------------------------------------------------------
// Ritorna la versione del firmware (sniper e fpga)
//---------------------------------------------------------------------------------
char* SniperModule::GetVersion()
{
	if( Send_Command( GET_VERSION, rx_data ) )
	{
		setError();
		strcpy( version_str, "Error!!!" );
	}
	else
	{
		snprintf( version_str, sizeof(version_str), "%d.%d.%d", rx_data[0], rx_data[1], rx_data[2] );
		sniper_version = rx_data[0];
		sniper_revision = rx_data[1];
		fpga_version = rx_data[2];
	}

	return version_str;
}

//-----------------------------------------------------------------------------
// Attiva la sospensione seriale dello Sniper
//-----------------------------------------------------------------------------
int SniperModule::Suspend()
{
	// Attiva la sospensione seriale
	if( Send_Command( SUSPENDSERIAL_CMD, rx_data ) )
	{
		setError();
		return 0;
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Modifica indirizzo seriale dello Sniper
// ATTENZIONE !!! - manda un comando con indirizzo 0
//     Ritorna 0 se e' avvenuto un errore, 1 altrimenti
//-----------------------------------------------------------------------------
//TODO: rivedere valori di ritorno di tutte le funzioni
int SniperModule::ChangeAddress( int address )
{
	while( !sniperSerialMutex.try_lock() )
	{
		delay( 1 );
	}

	m_address = address;

	int r = TWSBus1_Send_Command_FD( m_comInt, 0, CHANGEADDRESS_CMD, m_address, rx_data );
	if( r )
		setError();

	sniperSerialMutex.unlock();
	return r;
}


/*---------------------------------------------------------------------------------
Controlla lo stato dello SNIPER e se tutto ok lo attiva
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   nessuno.
----------------------------------------------------------------------------------*/
void SniperModule::CheckActivation( void )
{
	unsigned char tx_buffer[4];

	// get system date
	time_t now = time( NULL ) / 60; // minutes since 01/01/1970
	tx_buffer[0] = now >> 24;
	tx_buffer[1] = now >> 16;
	tx_buffer[2] = now >> 8;
	tx_buffer[3] = now;

	if( Send_Command_xBytes( ACTIVATION_CHECK, tx_buffer, 4, rx_data ) )
	{
		setError();
	}
	else
	{
		is_expired = rx_data[0];
		days_left = 256 * rx_data[1] + rx_data[2];
	}
}

/*---------------------------------------------------------------------------------
Ritorna lo stato di attivazione
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   1 macchina "scaduta", 0 macchina "attiva".
----------------------------------------------------------------------------------*/
int SniperModule::IsExpired( void )
{
	if( sniper_version > 1 || ( sniper_version == 1 && sniper_revision > 4 ) )
		return is_expired;

	return 0;
}

/*---------------------------------------------------------------------------------
Ritorna il numero di giorni mancanti alla scandeza
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   giorni mancanti, -1 attivazione "illimitata".
----------------------------------------------------------------------------------*/
int SniperModule::GetDaysLeft( void )
{
	if( sniper_version > 1 || ( sniper_version == 1 && sniper_revision > 4 ) )
	{
		if( days_left == 0xFFFF )
			return -1;

		return days_left;
	}

	return -1;
}

/*---------------------------------------------------------------------------------
Ritorna il codice di richiesta attivazione
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   nessuno.
----------------------------------------------------------------------------------*/
void SniperModule::GetActivationCode( unsigned char *code )
{
	unsigned char tx_buffer[10];

	// get system date
	time_t now = time( NULL ) / 60; // minutes since 01/01/1970
	tx_buffer[0] = now >> 24;
	tx_buffer[1] = now >> 16;
	tx_buffer[2] = now >> 8;
	tx_buffer[3] = now;

	// get MAC address (6 bytes)
	GetMACAddress( &tx_buffer[4] );

	if( Send_Command_xBytes( ACTIVATION_CODE, tx_buffer, 10, rx_data ) )
	{
		setError();
	}
	else
	{
		memcpy( code, rx_data, 26 );
	}
}

/*---------------------------------------------------------------------------------
Ritorna il codice di richiesta attivazione
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   -1 se errore comunicazione, 0 attivazione OK, > 0 errore di attivazione.
----------------------------------------------------------------------------------*/
int SniperModule::Activate( unsigned char *code )
{
	if( Send_Command_xBytes( ACTIVATION_ACTIVATE, code, 26, rx_data ) )
	{
		setError();
		return -1;
	}

	return rx_data[0];
}

/*---------------------------------------------------------------------------------
Ritorna il codice di richiesta attivazione
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   -1 se errore comunicazione, 0 attivazione OK, > 0 errore di attivazione.
----------------------------------------------------------------------------------*/
int SniperModule::ActivateAlign( unsigned char *code )
{
	if( Send_Command_xBytes( ACTIVATION_ALIGN, code, 26, rx_data ) )
	{
		setError();
		return -1;
	}

	return rx_data[0];
}

/*---------------------------------------------------------------------------------
Imposta modo USE_EVERY_FRAMES
Parametri di ingresso:
Valori di ritorno:
   1 se ok, 0 se errore
----------------------------------------------------------------------------------*/
//BOOST
int SniperModule::SetUseEveryFrames()
{
	if( use_full_frames == 1 )
	{
		return 1;
	}

	if( Send_Command( USE_EVERY_FRAMES, rx_data ) )
	{
		setError();
		return 0;
	}
	else
	{
		use_full_frames = 1;
		return 1;
	}
}

/*---------------------------------------------------------------------------------
Imposta modo USE_HALF_FRAMES
Parametri di ingresso:
Valori di ritorno:
   1 se ok, 0 se errore
----------------------------------------------------------------------------------*/
//BOOST
int SniperModule::SetUseHalfFrames()
{
	if( use_full_frames == 0 )
	{
		return 1;
	}

	if( Send_Command( USE_HALF_FRAMES, rx_data ) )
	{
		setError();
		return 0;
	}
	else
	{
		use_full_frames = 0;
		return 1;
	}
}

/*---------------------------------------------------------------------------------
Imposta registro FPGA_REG_PULSES
Parametri di ingresso:
  numSteps : valore da caricare
Valori di ritorno:
1 se ok, 0 se errore
----------------------------------------------------------------------------------*/
//BOOST
int SniperModule::SetPulsesForFPGA ( int numSteps )
{
	if( pulses_for_fpga == numSteps )
	{
		return 1;
	}

	if( Write_Reg_FPGA( FPGA_REG_PULSES, numSteps ) )
	{
		setError();
		return 0;
	}
	else
	{
		pulses_for_fpga = numSteps;
		return 1;
	}
}

/*---------------------------------------------------------------------------------
Imposta algoritmo di ricerca standard (la finestra non deve essere bloccata)
Valori di ritorno:
1 se ok, 0 se errore
----------------------------------------------------------------------------------*/
int SniperModule::SetUseStandardAlgorithm()
{
	if( use_std_algorithm == 1 )
	{
		return 1;
	}

	if( Send_Command( USE_ALGO_STD, rx_data ) )
	{
		setError();
		return 0;
	}
	else
	{
		use_std_algorithm = 1;
		return 1;
	}
}

/*---------------------------------------------------------------------------------
Imposta algoritmo di ricerca modificato (permette che la finestra sia bloccata)
Valori di ritorno:
1 se ok, 0 se errore
----------------------------------------------------------------------------------*/
int SniperModule::SetUseModifiedAlgorithm()
{
	if( use_std_algorithm == 0 )
	{
		return 1;
	}

	if( Send_Command( USE_ALGO_MOD, rx_data ) )
	{
		setError();
		return 0;
	}
	else
	{
		use_std_algorithm = 0;
		return 1;
	}
}

/*---------------------------------------------------------------------------------
Setta la ricerca del primo minimo (l'algoritmo si arresta dopo il primo minimo)
Valori di ritorno:
1 se ok, 0 se errore
----------------------------------------------------------------------------------*/
int SniperModule::SetUseFirstMinimum()
{
	if( use_first_min == 1 )
		return 1;

	if( Send_Command( FIND_FIRST_MIN, rx_data ) )
	{
		setError();
		return 0;
	}

	use_first_min = 1;
	return 1;
}

/*---------------------------------------------------------------------------------
Setta la ricerca del minimo assoluto (l'algoritmo non si ferma dopo il primo minimo)
Valori di ritorno:1 se ok, 0 se errore
----------------------------------------------------------------------------------*/
int SniperModule::SetUseAbsoluteMinimum()
{
	if( use_first_min == 0 )
		return 1;

	if( Send_Command( FIND_ABS_MIN, rx_data ) )
	{
		setError();
		return 0;
	}

	use_first_min = 0;
	return 1;
}

/*---------------------------------------------------------------------------------
Setta la finestra di scansione
Parametri di ingresso:
   left : estremo sinistro della finestra espresso in pixel del sensore
   right: estremo destro   della finestra espresso in pixel del sensore
Valori di ritorno:1 se ok, 0 se errore
----------------------------------------------------------------------------------*/
int SniperModule::SetWindow( int left, int right )
{
	if( ( left == leftw ) && ( right == rightw ) )
	{
		return 1;
	}

	// scrive start col = left
	if( Write_Reg_VGA( VGA_COLUMN_START, left ) )
	{
		setError();
		return 0;
	}

	// scrive num col = right - left + 1
	if( Write_Reg_VGA( VGA_WINDOW_WIDTH, right - left + 1 ) )
	{
		setError();
		return 0;
	}

	leftw = left;
	rightw = right;

	return 1;
}

/*---------------------------------------------------------------------------------
Azzera contatore encoder dello SNIPER
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   1 se il comando e' andato a buon fine, 0 altrimenti
----------------------------------------------------------------------------------*/
int SniperModule::Zero_Cmd()
{
	if( Send_Command( RESET_ENCODER, rx_data ) )
	{
		setError();
		return 0;
	}

	return 1;
}

/*---------------------------------------------------------------------------------
Resetta FPGA e VGA all'interno dello Sniper
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   1 se il comando e' andato a buon fine, 0 altrimenti
----------------------------------------------------------------------------------*/
int SniperModule::SoftReset()
{
	if( Send_Command( RESET_FPGA_VGA, rx_data ) )
	{
		setError();
		return 0;
	}

	return 1;
}

/*---------------------------------------------------------------------------------
Resetta MICRO all'interno dello Sniper (riparte il bootloader dello Sniper)
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   nessuno.
----------------------------------------------------------------------------------*/
int SniperModule::HardReset()
{
	if( Send_Command( RESET_MICRO, rx_data ) )
	{
		setError();
		return 0;
	}

	return 1;
}

//---------------------------------------------------------------------------
// Inizia ciclo di ricerca del primo minimo
// Ritorna: 0 errore, 1 altrimenti
//---------------------------------------------------------------------------
int SniperModule::StartFirst()
{
	if( Error )
		return 0;

	// inizio ciclo di ricerca ombra minima
	if( Send_Command( START_CENTERING, rx_data ) )
	{
		setError();
		return 0;
	}

	return 1;
}

//---------------------------------------------------------------------------
// Prende i dati alla fine della ricerca del primo minimo
// Ritorna:  0 errore
//           1 altrimenti
//---------------------------------------------------------------------------
int SniperModule::GetFirst( int& status, int& angle, float& position, float& shadow )
{
	// Leggo i risultati del ciclo di ricerca
	if( Send_Command( GET_RESULT, rx_data ) )
	{
		setError();
		return 0;
	}

	status   = rx_data[0];
	angle    = ( short )((rx_data[1]<<8) + rx_data[2] );
	shadow   = ((rx_data[3]<<8) + rx_data[4]) * k_pixel_um;
	position = ((rx_data[5]<<8) + rx_data[6]) * k_pixel_um;
	return 1;
}

//---------------------------------------------------------------------------
// Effettua una singola misurazione
// Ritorna:  0 errore
//           1 altrimenti
//---------------------------------------------------------------------------
int SniperModule::MeasureOnce( int& status, int& angle, float& position, float& shadow )
{
	// Effettuo una misurazione e prendo i risultati
	if( Send_Command( MEASURE_ONCE, rx_data ) )
	{
		setError();
		return 0;
	}

	status   = rx_data[0];
	angle    = ( short )((rx_data[1]<<8) + rx_data[2] );
	shadow   = ((rx_data[3]<<8) + rx_data[4]) * k_pixel_um;
	position = ((rx_data[5]<<8) + rx_data[6]) * k_pixel_um;

	return 1;
}


//---------------------------------------------------------------------------
// Effettua una singola misurazione
// Ritorna:  0 errore
//           1 altrimenti
//---------------------------------------------------------------------------
int SniperModule::MeasureOnce_Pixels( int& status, int& angle, int& position, int& shadow )
{
	// Effettuo una misurazione e prendo i risultati
	if( Send_Command( MEASURE_ONCE, rx_data ) )
	{
		setError();
		return 0;
	}

	status   = rx_data[0];
	angle    = ( short )((rx_data[1]<<8) + rx_data[2] );
	shadow   = (rx_data[3]<<8) + rx_data[4];
	position = (rx_data[5]<<8) + rx_data[6];

	return 1;
}


//---------------------------------------------------------------------------
// Imposta linea di scansione
// Ritorna:  0 errore
//           1 altrimenti
//---------------------------------------------------------------------------
int SniperModule::SetScalLine( int line )
{
	return !Write_Reg_VGA( VGA_ROW_START, line );
}

//---------------------------------------------------------------------------
// Effettua scansione della linea selezionata
// Ritorna:  0 errore
//           1 altrimenti
//---------------------------------------------------------------------------
int SniperModule::ScanLine( unsigned char* buf )
{
	return !Send_Command( SCAN_LINE, buf );
}

/*---------------------------------------------------------------------------------
Ritorna contatore encoder corrente per SNIPER
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   Contatore encoder.
----------------------------------------------------------------------------------*/
int SniperModule::GetEncoder()
{
	unsigned short value;

	if( Read_Reg_FPGA ( FPGA_REG_ENCODER, &value ) )
	{
		setError();
		return 0;
	}

	return ( ( short ) value );
}

/*---------------------------------------------------------------------------------
Memorizza angolo e ampiezza dell'ombra per i frames catturati durante l'ultimo
centraggio.
Parametri di ingresso:
   nessuno.
Valori di ritorno:
   nessuno.
----------------------------------------------------------------------------------*/
int SniperModule::GetFrames( int &nFrame, int* frames )
{
	nFrame = 0;

	if( Error )
		return 0;

	// legge il buffer dello sniper
	if( Send_Command( READ_DATA_BUF, rx_data ) )
	{
		setError();
		return 0;
	}

	nFrame = 256 * rx_data[10] + rx_data[11];
	if( nFrame <= 0 )
	{
		nFrame = 0;
		return 0;
	}
	if( nFrame > 500 )
		nFrame = 500;

	for( int i = 0; i < nFrame; i++ )
	{
		frames[i] = rx_data[i+1+12];
	}

	return 1;
}

#endif //__SNIPER
