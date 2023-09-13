//---------------------------------------------------------------------------
//
// Name:        q_decode.cpp
// Author:      Gabriel Ferri
// Created:     11/04/2012
// Description: Quadra decode file
//
//---------------------------------------------------------------------------
#include "q_decode.h"

#include <vector>
#include <string>
#include <stdio.h>
#include "q_cost.h"
#include "q_tabe.h"
#include "q_feederfile.h"
#include "strutils.h"
#include "msglist.h"
#include "q_carobj.h"

#include <mss.h>

int _programImportAsciiQ( const char* filename, std::vector<TabPrg>& prgout );
int _programExportAsciiQ( FILE* ASQFile, TabPrg qrec );
int _feederConfigExportCSV( FILE* CSVFile, const CarDat& feeder );


//---------------------------------------------------------------------------
// Importazione ASCII-Q
// Ritorna 1 se import ok, 0 altrimenti
//---------------------------------------------------------------------------
int Program_ImportAsciiQ( const char* prg_filename, const char* asq_filename )
{
	std::vector<TabPrg> newprg;
	if( !_programImportAsciiQ( asq_filename, newprg ) )
	{
		return 0;
	}

	// save new program
	TPrgFile* TPrgConv = new TPrgFile( prg_filename, PRG_NORMAL );
	if( !TPrgConv->Create() )
	{
		delete TPrgConv;
		return 0;
	}
	TPrgConv->Open( SKIPHEADER );

	for( unsigned int i = 0; i < newprg.size(); i++ )
	{
		TPrgConv->Write( newprg[i], i );
	}

	delete TPrgConv;
	return 1;
}

//---------------------------------------------------------------------------
// Esportazione ASCII-Q
// Ritorna 1 se export ok, 0 altrimenti
//---------------------------------------------------------------------------
int Program_ExportAsciiQ( const char* prg_filename, const char* asq_filename )
{
	// load program
	TPrgFile* TPrgConv = new TPrgFile( prg_filename, PRG_NORMAL );
	if( !TPrgConv->Open( SKIPHEADER ) )
	{
		delete TPrgConv;
		return 0;
	}

	// create asq file
	FILE* ASQFile = fopen( asq_filename, "w" );
	if( !ASQFile )
	{
		delete TPrgConv;
		return 0;
	}

	TabPrg qrec;

	int nrecs = TPrgConv->Count();
	for( int i = 0; i < nrecs; i++ )
	{
		TPrgConv->Read( qrec, i );
		_programExportAsciiQ( ASQFile, qrec );
	}

	fclose( ASQFile );
	delete TPrgConv;
	return 1;
}


//---------------------------------------------------------------------------
// Import programma ASCII-Q - Funzioni di trascodifica
//---------------------------------------------------------------------------
void _programTranslateField( char* field, int num, TabPrg& qrec )
{
	switch( num )
	{
		case 0: // codice componente
			qrec.status = MOUNT_MASK;
			snprintf( qrec.CodCom, 17, "%s", field );
			DelSpcR( qrec.CodCom );
			DelSpcL( qrec.CodCom );
			break;

		case 1: // X deposito
			qrec.XMon = atof( field );
			break;

		case 2: // Y deposito
			qrec.YMon = atof( field );
			break;

		case 3: // Rot deposito
			qrec.Rotaz = atof( field );

			while( qrec.Rotaz >= 360.f )
			{
				qrec.Rotaz -= 360.f;
			}
			while( qrec.Rotaz <= -360.f )
			{
				qrec.Rotaz += 360.f;
			}
			break;

		case 5: // nome package
			snprintf( qrec.pack_txt, 21, "%s", field );
			break;

		case 6: // tipo componente
			snprintf( qrec.TipCom, 26, "%s", field );
			break;

		case 9: // punta
			qrec.Punta = field[0];
			if( qrec.Punta != '1' || qrec.Punta != '2' )
			{
				qrec.Punta = '1';
			}
			break;

		case 10: // caricatore
			qrec.Caric = atoi(field);
			break;

		case 15: // stato
			switch( field[0] )
			{
				case 'N':
					qrec.status = 0;
					break;
				case 'V':
					qrec.status = MOUNT_MASK | VERSION_MASK;
					break;
				case 'G':
					qrec.status = DODOSA_MASK;
					break;
				default:
					qrec.status = MOUNT_MASK;
					break;
			}
			break;

		case 16: // note
			snprintf( qrec.NoteC, 41, "%s", field );
			break;
	}
}

int _programImportAsciiQ( const char* filename, std::vector<TabPrg>& prgout )
{
	FILE* ASQFile = fopen( filename, "rt" );
	if( !ASQFile )
	{
		return 0;
	}

	int field_counter = 0;
	char buf[128];
	int buf_index = 0;
	TabPrg qrec;

	while( !feof(ASQFile) )
	{
		char c = fgetc( ASQFile );

		if( c == '#' )
		{
			continue;
		}

		if( c == ',' || c == '\n' )
		{
			if( buf_index > 127 )
			{
				buf_index = 127;
			}
			buf[buf_index] = 0;

			_programTranslateField( buf, field_counter, qrec );

			field_counter++;
			buf_index = 0;
			buf[buf_index] = 0;

			if( c == '\n' )
			{
				// ci assicuriamo che non esista un campo con lo stesso codice componente
				int found=0;
				for( unsigned int i = 0; i < prgout.size(); i++ )
				{
					if( !strcmp(prgout[i].CodCom, qrec.CodCom) )
					{
						found = 1;
						break;
					}
				}

				if( !found )
				{
					qrec.Riga = prgout.size()+1;
					qrec.Uge = -1;
					qrec.scheda = 1;
					qrec.Changed = 0;

					prgout.push_back( qrec );
				}

				field_counter = 0;
			}
			continue;
     	}

		if( buf_index < 127 )
		{
			buf[buf_index] = c;
			buf_index++;
		}
	}

	fclose( ASQFile );
	return 1;
}


//---------------------------------------------------------------------------
// Export programma ASCII-Q - Funzioni di trascodifica
//---------------------------------------------------------------------------
int _programExportAsciiQ( FILE* ASQFile, TabPrg qrec )
{
	DelSpcR( qrec.CodCom );
	DelSpcR( qrec.pack_txt );
	DelSpcR( qrec.TipCom );
	DelSpcR( qrec.NoteC );

	qrec.CodCom[16] = '\0';
	qrec.pack_txt[20] = '\0';
	qrec.TipCom[25] = '\0';
	qrec.NoteC[40] = '\0';

	// codice componente
	fprintf( ASQFile, "#%s#", qrec.CodCom );
	// X, Y, Rot
	fprintf( ASQFile, ",%7.2f,%7.2f,%7.2f", qrec.XMon, qrec.YMon, qrec.Rotaz );
	// PXY, package, tipo componente
	fprintf( ASQFile, ",#PXY#,#%s#,#%s#", qrec.pack_txt, qrec.TipCom );
	// 1, T, punta, caricatore, F, TAPE, X, A
	fprintf( ASQFile, ",1,T,#%c#,%d,F,#TAPE#,#X#,#A#", qrec.Punta, qrec.Caric );
	// status
	if( qrec.status & VERSION_MASK )
	{
		fputs( ",#V#", ASQFile );
	}
	else if( qrec.status & MOUNT_MASK )
	{
		fputs( ",##", ASQFile );
	}
	else if( qrec.status & DODOSA_MASK )
	{
		fputs( ",#G#", ASQFile );
	}
	else
	{
		fputs( ",#N#", ASQFile );
	}
	// note, F
	fprintf( ASQFile, ",#%s#,F\n", qrec.NoteC );
	return 1;
}



//---------------------------------------------------------------------------
// Esportazione configurazione caricatori in formato CSV
// Ritorna 1 se ok, 0 altrimenti
//---------------------------------------------------------------------------
int FeederConfig_ExportCSV( const char* fed_filename, const char* csv_filename )
{
	// load feeder config
	FeederFile* feederConf = new FeederFile( (char*)fed_filename );
	if( !feederConf->opened )
	{
		delete feederConf;
		return 0;
	}

	// create csv file
	FILE* CSVFile = fopen( csv_filename, "w" );
	if( !CSVFile )
	{
		delete feederConf;
		return 0;
	}

	// intestazione colonne
	fprintf( CSVFile, "%s,%s,%s,%s,%s,%s,Quant.\n", MsgGetString(Msg_00490), MsgGetString(Msg_00510), MsgGetString(Msg_00678), MsgGetString(Msg_01034), MsgGetString(Msg_01622), MsgGetString(Msg_01623) );

	CarDat feeder;
	for( int i = 0; i < MAXCAR; i++ )
	{
		feederConf->ReadRec( i, feeder );

		_feederConfigExportCSV( CSVFile, feeder );
	}

	fclose( CSVFile );
	delete feederConf;
	return 1;
}

//---------------------------------------------------------------------------
// Export configurazione CSV - Funzioni di trascodifica
//---------------------------------------------------------------------------
int _feederConfigExportCSV( FILE* CSVFile, const CarDat& feeder )
{
	fprintf( CSVFile, "%d,%s,%s,%s", feeder.C_codice, feeder.C_comp, feeder.C_Package, feeder.C_note );

	#ifdef __DOME_FEEDER
	if( feeder.C_tipo == CARTYPE_DOME )
	{
		fprintf( CSVFile, "," );
	}
	else
	#endif
	{
		if( feeder.C_att < 8 )
		{
			fprintf( CSVFile, ",%s", CarAvType_StrVect[feeder.C_att] );
		}
		else
		{
			fprintf( CSVFile, ",0" );
		}
	}

	if( feeder.C_tipo >= 0 && feeder.C_tipo <= 2 )
	{
		fprintf( CSVFile, ",%s", CarType_StrVect[feeder.C_tipo] );
	}
	else
	{
		fprintf( CSVFile, "," );
	}

	fprintf( CSVFile, ",%d\n", feeder.C_quant );
	return 1;
}
