/*----------------------------------------------------------------------------
File        : q_msg.cpp
Info        : funzioni per la gestione dei messaggi
Created by  : SN [3May94]
Modified by : MG[07Jul94] AF[13Dec95] Simone[06Ago96]
-----------------------------------------------------------------------------*/
#include "q_msg.h"

#include <stdio.h>
#include "q_cost.h"
#include "strutils.h"

#include <mss.h>


// Linguaggio attivo
static LANGUAGE _curlang = LANG_ENGLISH;

// Permette di settare il linguaggio attivo
void MsgSetLanguage( LANGUAGE Lang )
{
	if( Lang < 0 || Lang >= LANG_NUM )
	{
		_curlang = LANG_ENGLISH;
	}
	else
	{
		_curlang = Lang;
	}
}

// Restituisce il linguaggio attivo
LANGUAGE MsgGetLanguage()
{
	return _curlang;
}


// Restituisce il puntatore al messaggio nel linguaggio utilizzato
const char* MsgGetString( const MSG Msg )
{
	const char* pStr = Msg[_curlang];
	if( pStr )
	{
		return pStr;
	}

	return Msg[0]; // return the english message
}


// Setta il linguaggio memorizzato in configurazione
bool ReadLanguageFile()
{
	FILE* flang = fopen( DEFFILE_LANGUAGE, "r" );
	if( flang == NULL )
	{
		MsgSetLanguage( LANG_ENGLISH ); // default language
		return false;
	}

	char buf[80];
	bool setLang = false;

	while( !feof(flang) )
	{
		fgets( buf, sizeof(buf), flang );
		strupr( buf );

		char* p = strchr(buf,'\n');

		if(p!=NULL)
		{
			*p='\0';
		}

		p = strchr(buf,'\r');

		if(p!=NULL)
		{
			*p='\0';
		}

		DelSpcR(buf);

		if( !strcmp(buf,"ENGLISH") )
		{
			setLang = true;
			MsgSetLanguage( LANG_ENGLISH );
			break;
		}

		if( !strcmp(buf,"ITALIAN") )
		{
			setLang = true;
			MsgSetLanguage( LANG_ITALIAN );
			break;
		}

		if( !strcmp(buf,"FRENCH") )
		{
			setLang = true;
			MsgSetLanguage( LANG_FRENCH );
			break;
		}

		if( !strcmp(buf,"PORTUGUESE") )
		{
			setLang = true;
			MsgSetLanguage( LANG_PORTUGUESE );
			break;
		}

		if( !strcmp(buf,"SPANISH") )
		{
			setLang = true;
			MsgSetLanguage( LANG_SPANISH );
			break;
		}

		if( !strcmp(buf,"TURKISH") )
		{
			setLang = true;
			MsgSetLanguage( LANG_TURKISH );
			break;
		}
	}

	fclose( flang );

	if( !setLang )
	{
		MsgSetLanguage( LANG_ENGLISH ); // default language
		return false;
	}

	return true;
}
