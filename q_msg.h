/*----------------------------------------------------------------------------
File        : q_msg.h
Info        : header file for per the msg.h module
Created by  : SN [3May94]
Modified by : SN MG[07Jul94] Simone[06.08.96]
-----------------------------------------------------------------------------*/
#ifndef __MSG_H__
#define __MSG_H__


enum LANGUAGE
 {
	LANG_ENGLISH = 0,
	LANG_ITALIAN,
	LANG_FRENCH,
	LANG_PORTUGUESE,
	LANG_SPANISH,
	LANG_TURKISH,
	LANG_NUM
 };

typedef const char* MSG[LANG_NUM];

bool ReadLanguageFile();

void MsgSetLanguage( LANGUAGE Lang );
LANGUAGE MsgGetLanguage();

const char* MsgGetString( const MSG Msg );

#endif
