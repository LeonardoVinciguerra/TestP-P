//---------------------------------------------------------------------------
//
// Name:        strutils.h
// Author:      Gabriel Ferri
// Created:     15/10/2008 9.43.20
// Description: strutils functions declaration
//
//---------------------------------------------------------------------------

#ifndef __STRUTILS_H
#define __STRUTILS_H

#include <string>

char* strupr( char* string );
char* strlwr( char* string );

void DelAllSpc( char* txt );
void DelSpcR( char* txt );
void DelSpcL( char* txt );
void DelSpcR( std::string& txt );
void DelSpcL( std::string& txt );

char *strncpyQ(char *s1,const char *s2,unsigned int maxcar);
int strcasecmpQ( const char* s1, const char* s2 );
int strncasecmpQ( const char* s1, const char* s2, unsigned int n );

int strNoTilde( char* msg, char* c );

#endif
