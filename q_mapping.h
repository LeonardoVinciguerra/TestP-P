//---------------------------------------------------------------------------
//
// Name:        q_mapping.h
// Author:      Gabriel Ferri
// Created:     14/11/2011
// Description:
//
//---------------------------------------------------------------------------

#ifndef __Q_MAPPING_H
#define __Q_MAPPING_H

#include "c_window.h"


// Genera mappatura piano
int Errormap_Create( int dir );

// Verifica dati mappatura
int Errormap_Check_Correction( int dir );
int Errormap_Check_XY( int dir );
int Errormap_Check_Random();

int Show_Errormap_Check( CWindow* parent );

// Cancella dati mappatura
int Errormap_Delete();

// Applica la correzione da mappatua errore
void Errormap_Correct(float &x_pos, float &y_pos);

// Calibra ortogonalita' assi
int OrthogonalityXY_Calibrate( bool save_result );

// Calibra scala encoder
int EncoderScale_Calibrate( bool save_result );

// Carica/salva i dati della mappatura
bool LoadMapData( const char* filename );

#endif
