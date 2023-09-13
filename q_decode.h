//---------------------------------------------------------------------------
//
// Name:        q_decode.h
// Author:      Gabriel Ferri
// Created:     11/04/2012
// Description: Quadra decode file
//
//---------------------------------------------------------------------------

#ifndef __Q_DECODE_H
#define __Q_DECODE_H

int Program_ImportAsciiQ( const char* prg_filename, const char* asq_filename );
int Program_ExportAsciiQ( const char* prg_filename, const char* asq_filename );

int FeederConfig_ExportCSV( const char* fed_filename, const char* csv_filename );

#endif
