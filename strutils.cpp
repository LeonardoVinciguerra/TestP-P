//---------------------------------------------------------------------------
//
// Name:        strutils.cpp
// Author:      Gabriel Ferri
// Created:     15/10/2008 9.43.20
// Description: strutils functions implementation
//
//---------------------------------------------------------------------------
#include "strutils.h"

#include <ctype.h>
#include <stdlib.h>

#include <mss.h>


char* strupr( char *string )
{
	char* s;

	if( string )
	{
		for( s = string; *s; ++s )
			*s = toupper(*s);
	}
	return string;
}

char* strlwr( char *string )
{
	char* s;

	if( string )
	{
		for( s = string; *s; ++s )
			*s = tolower(*s);
	}
	return string;
}


//---------------------------------------------------------------------------
// Elimina gli spazi in fondo alla stringa.
//---------------------------------------------------------------------------
void DelSpcR( char* txt )
{
	int len = strlen(txt);
	if( len == 0 )
		return;

	txt = txt + len - 1;

	while( (*txt == ' ') || (*txt == 32) )
	{
		txt--;
	}

	*(txt+1) = '\0';
}

void DelSpcR( std::string& s )
{
	size_t last_not_space = s.find_last_not_of(' ');

	if(last_not_space != std::string::npos)
	{
		s.erase(last_not_space + 1);
	}
}

//---------------------------------------------------------------------------
// Elimina gli spazi all'inizio della stringa.
//---------------------------------------------------------------------------
void DelSpcL( char* txt )
{
	char *startTxt=txt;

	if(strlen(txt)!=0)
	{
		while ((*txt==' ') && (*txt!='\0'))
			txt++;

		if(*txt!='\0')
		{
			memmove(startTxt,txt,strlen(txt)+1);
		}
		else
			*startTxt='\0';
	}
}

void DelSpcL( std::string& s )
{
	size_t first_not_space = s.find_last_not_of(' ');

	if(first_not_space != std::string::npos)
	{
		s.erase(0,first_not_space);
	}
}


/*--------------------------------------------------------------------------
Elimina tutti gli spazi all'interno di una stringa
INPUT:	txt: Stringa da cui eliminare gli spazi
GLOBAL:	-
RETURN:	Ritorna la stringa modificata
NOTE:	Integr. Simone >>S260701
--------------------------------------------------------------------------*/
void DelAllSpc( char* txt )
{
	DelSpcR( txt );
	DelSpcL( txt );
}

//---------------------------------------------------------------------------
// Overload della funzione strncpy per gestire l'inserimento del carattere
// di fine stringa nella stringa s1, quando la stringa s2 e' piu' lunga di
// maxcar.
//---------------------------------------------------------------------------
char *strncpyQ(char *s1,const char *s2,unsigned int maxcar)
{
	if( s1 == NULL ||  s2 == NULL )
		return NULL;

	if( *s2 == '\0' )
	{
		*s1 = '\0';
	}
	else
	{
		if( strlen(s2) <= maxcar )
		{
			memmove((char *)s1,(char *)s2,strlen(s2)+1);
		}
		else
		{
			memcpy((char *)s1,(char *)s2,maxcar);
			s1[maxcar]='\0';
		}
	}

	return s1;
}

//---------------------------------------------------------------------------
// Overload della funzione strcasecmp per eliminare gli spazi in fondo
// alle stringhe prima di esegiure il confronto
//---------------------------------------------------------------------------
int strcasecmpQ(const char *s1, const char *s2)
{
	char tmp1[1024];
	char tmp2[1024];
	
	strncpy(tmp1,s1,1024);
	strncpy(tmp2,s2,1024);
		
	DelSpcR((char*)tmp1);
	DelSpcR((char*)tmp2);

	char* ptr1=tmp1;
	char* ptr2=tmp2;
	
	while(tolower((unsigned char)*ptr1) == tolower((unsigned char)*ptr2))
	{
		if(*ptr1 == 0)
			break;
		ptr1++;
		ptr2++;
	}

	return (int)(*ptr1) - (int)(*ptr2);
}

//---------------------------------------------------------------------------
// Overload della funzione strncasecmp per eliminare gli spazi in fondo
// alle stringhe prima di esegiure il confronto
//---------------------------------------------------------------------------
int strncasecmpQ( const char *s1, const char *s2,unsigned int n)
{
	if(n == 0)
		return(0);

	char tmp1[1024];
	char tmp2[1024];
	
	strncpy(tmp1,s1,1024);
	strncpy(tmp2,s2,1024);

	DelSpcR((char*)tmp1);
	DelSpcR((char*)tmp2);

	char* ptr1=tmp1;
	char* ptr2=tmp2;	

	do
	{
		if(tolower((unsigned char)*ptr1) != tolower((unsigned char)*ptr2++))
			return (int)tolower((unsigned char)*ptr1) - (int)tolower((unsigned char)*--ptr2);

		if(*ptr1++ == 0)
			break;
	} while(--n !=0);

	return(0);
}

//--------------------------------------------------------------------------
// strNoTilde
// Elimina dalla stringa il segno dell'hotkey (~) e ritorna in c il
// carattere successivo
//--------------------------------------------------------------------------
int strNoTilde( char* msg, char* c )
{
	char* ptr = strchr( msg, '~' );
	int l = strlen( msg );

	if( !ptr || !l )
	{
		*c = 0;
		return -1;
	}

	memmove( ptr, ptr+1, l-(ptr-msg) );
	msg[l-1] = 0;
	*c = *ptr;
	
	return ptr-msg;
}
