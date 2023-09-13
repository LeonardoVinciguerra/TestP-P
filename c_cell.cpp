//---------------------------------------------------------------------------
// Name:        c_cell.cpp
// Author:      Gabriel Ferri
// Created:     24/01/2012
// Description: CCell class implementation
//---------------------------------------------------------------------------
#include "c_cell.h"

#include <stdio.h>
#include <limits.h>
#include "q_cost.h"
#include "q_gener.h"
#include "msglist.h"
#include "keyutils.h"
#include "gui_defs.h"
#include "gui_functions.h"

#include <mss.h>


const bool typeMatrix[CELL_TYPE_LAST+1][CELL_TYPE_LAST+1] =
{
//from...
	// TEXT
	//to    TEXT	UINT	SINT	UDEC	SDEC	YN
	{		true,	false,	false,	false,	false,	false	},
	// UINT
	{		true,	true,	true,	true,	true,	false	},
	// SINT
	{		true,	false,	true,	false,	true,	false	},
	// UDEC
	{		true,	false,	false,	true,	true,	false	},
	// SDEC
	{		true,	false,	false,	false,	true,	false	},
	//YN
	{		true,	false,	false,	false,	false,	true	},
};



//---------------------------------------------------------------------------
// Costruttore
//---------------------------------------------------------------------------
CCell::CCell( const char* text, int width, int type, int style )
{
	X = 0;
	Y = 0;
	cell_style = style;
	cell_type = type;

	visible = false;
	selected = false;
	editable = false;

	W = width;
	txt = new char[W+1];
	snprintf( txt, W+1, "%s", text );

	decdgt = 2;

	// colors
	bgColor = GUI_color( CB_COL_EDIT_BG );
	fgColor = GUI_color( WIN_COL_TXT );
	bgSelColor = GUI_color( GEN_COL_BG_SELECTED );
	fgSelColor = GUI_color( GEN_COL_FG_SELECTED );
	bgEditColor = GUI_color( GEN_COL_BG_EDITABLE );
	fgEditColor = GUI_color( GEN_COL_FG_EDITABLE );
	bgReadOnlyColor = GUI_color( CB_COL_EDIT_BG );

	txtSet = 0;
	txtSet_count = 0;
	txtSet_pos = 0;

	// set legal set for specified type
	switch( cell_type )
	{
	case CELL_TYPE_UINT:
		minval = 0;
		maxval = UINT_MAX;
		SetLegalChars( CHARSET_NUMUINT );
		break;
	case CELL_TYPE_SINT:
		minval = INT_MIN;
		maxval = INT_MAX;
		SetLegalChars( CHARSET_NUMSINT );
		break;
	case CELL_TYPE_SDEC:
		minval = INT_MIN;
		maxval = INT_MAX;
		SetLegalChars( CHARSET_NUMSDEC );
		break;
	case CELL_TYPE_UDEC:
		minval = 0;
		maxval = UINT_MAX;
		SetLegalChars( CHARSET_NUMUDEC );
		break;
	case CELL_TYPE_YN:
		SetLegalChars( CHARSET_TEXT );
		break;
	default:
		cell_type = CELL_TYPE_TEXT;
		SetLegalChars( CHARSET_TEXT );
		cell_style |= CELL_STYLE_OVERWRITE;
	}
}

//---------------------------------------------------------------------------
// Distruttore
//---------------------------------------------------------------------------
CCell::~CCell()
{
	if( txt )
	{
		delete [] txt;
	}

	clearTxtSet();
}

//--------------------------------------------------------------------------
// Set flag edit on/off
//--------------------------------------------------------------------------
void CCell::SetEdit( bool state )
{
	editable = state;
}

//--------------------------------------------------------------------------
// Seleziona la cella evidenziandone il testo
//--------------------------------------------------------------------------
void CCell::Select()
{
	selected = 1;
	Refresh();
}

//--------------------------------------------------------------------------
// Deseleziona la cella togliendole l'evidenziazione
//--------------------------------------------------------------------------
void CCell::Deselect()
{
	selected = 0;
	Refresh();
}

//--------------------------------------------------------------------------
// Setta testo
//--------------------------------------------------------------------------
void CCell::SetTxt( const char* text )
{
	snprintf( txt, W+1, "%s", text );
	Refresh();
}

//--------------------------------------------------------------------------
// Setta/Restituisce testo INT
//--------------------------------------------------------------------------
void CCell::SetTxt( int val )
{
	snprintf( txt, W+1, "%d", val );
	Refresh();
}

int CCell::GetInt()
{
	int i = atoi( txt );
	return i;
}

//--------------------------------------------------------------------------
// Setta/Restituisce testo LONG
//--------------------------------------------------------------------------
/*
void CCell::SetTxt( long val )
{
	snprintf( txt, W+1, "%ld", val );
	Refresh();
}

int CCell::GetLong()
{
	int i = atol( txt );
	return i;
}
*/

//--------------------------------------------------------------------------
// Setta/Restituisce testo FLOAT
//--------------------------------------------------------------------------
void CCell::SetTxt( float val )
{
	char format[20];
	snprintf( format, 20, "%%%d.%df", W, decdgt );
	snprintf( txt, W+1, format, val );
	Refresh();
}

float CCell::GetFloat()
{
	float f = atof( txt );
	return f;
}

//--------------------------------------------------------------------------
// Setta/Restituisce testo CHAR
//--------------------------------------------------------------------------
void CCell::SetTxt( char val )
{
	snprintf( txt, W+1, "%c", val );
	Refresh();
}

char CCell::GetChar()
{
	return txt[0];
}

//--------------------------------------------------------------------------
// Setta/Restituisce testo YES/NO
//--------------------------------------------------------------------------
void CCell::SetTxtYN( int val )
{
	SetTxt( val ? MsgGetString(Msg_00180) : MsgGetString(Msg_00181) );
}

int CCell::GetYN()
{
	char buf_no[8];

	if( W == 1 )
	{
		// short Yes/No
		strncpy( buf_no, MsgGetString(Msg_00013), 8 );
		return ( buf_no[0] == txt[0] ) ? 0 : 1;
	}

	// int Yes/No
	strncpy( buf_no, MsgGetString(Msg_00181), 8 );
	return ( strcmp( txt, buf_no ) == 0 ) ? 0 : 1;
}

//--------------------------------------------------------------------------
// Setta insieme di caratteri legali
//    NOTE: Esegue un overide dei caratteri validi per il modo di edit
//          corrente
//--------------------------------------------------------------------------
void CCell::SetLegalChars( const char* set )
{
	legalset = set;

	/*
	int len = strlen(set);

	if( len )
	{
		bool upper_only = true;
		for( int i = 0; i < len; i++ )
		{
			if( islower(legalset[i]) && isalpha(legalset[i]) )
			{
				upper_only = false;
				break;
			}
		}
		if( upper_only )
		{
			cell_style |= CELL_STYLE_UPPERCASE;
		}

		bool lower_only = true;
		for( int i = 0; i < len; i++ )
		{
			if( isupper(legalset[i]) && isalpha(legalset[i]) )
			{
				lower_only = false;
				break;
			}
		}
		if( lower_only )
		{
			cell_style |= CELL_STYLE_LOWERCASE;
		}
	}
	else
	{
		cell_style = cell_style & ~CELL_STYLE_UPPERCASE;
		cell_style = cell_style & ~CELL_STYLE_LOWERCASE;
	}
	*/
}

//--------------------------------------------------------------------------
// Setta valori minimo e massimo interi
//--------------------------------------------------------------------------
void CCell::SetVMinMax( int min, int max )
{
	minval = min;
	maxval = max;
}

//--------------------------------------------------------------------------
// Setta valori minimo e massimo float
//--------------------------------------------------------------------------
void CCell::SetVMinMax( float min, float max )
{
	minval = min;
	maxval = max;
}

//--------------------------------------------------------------------------
// Setta insieme di stringhe ammesse nella combo
//--------------------------------------------------------------------------
void CCell::SetLegalStrings( int nSet, char** set )
{
	clearTxtSet();

	txtSet_count = nSet;

	if( txtSet_count <= 0 )
	{
		return;
	}

	txtSet = new char*[txtSet_count];
	for( int i = 0; i < txtSet_count; i++ )
	{
		txtSet[i] = new char[strlen(set[i])+1];
		strcpy( txtSet[i], set[i] );
	}
}

//--------------------------------------------------------------------------
// Setta posizione (nel set di stringhe)
//--------------------------------------------------------------------------
void CCell::SetStrings_Pos( int pos )
{
	txtSet_pos = MID( 0, pos, txtSet_count-1 );
	SetTxt( txtSet[txtSet_pos] );
}

//--------------------------------------------------------------------------
// Setta testo (nel set di stringhe)
//--------------------------------------------------------------------------
void CCell::SetStrings_Text( const char* text )
{
	for( int i = 0; i < txtSet_count; i++ )
	{
		if( strcmp( txtSet[i], text ) == 0 )
		{
			txtSet_pos = i;
			snprintf( txt, W+1, "%s", text );
			Refresh();
			break;
		}
	}
}

//--------------------------------------------------------------------------
// Ritorna la posizione corrente (nel set di stringhe)
//--------------------------------------------------------------------------
int CCell::GetStrings_Pos()
{
	return txtSet_pos;
}

//--------------------------------------------------------------------------
// Disegna il testo della casella
//--------------------------------------------------------------------------
void CCell::Refresh()
{
	if( !visible )
		return;

	GUI_Freeze();

	GUI_font font = ( cell_style & CELL_STYLE_FONT_DEFAULT ) ? GUI_DefaultFont : GUI_SmallFont;
	GUI_color _bgCol = bgColor;
	GUI_color _fgCol = fgColor;

	if( selected )
	{
		if( editable && !(cell_style & CELL_STYLE_READONLY) )
		{
			_bgCol = bgEditColor;
			_fgCol = fgEditColor;
		}
		else
		{
			_bgCol = bgSelColor;
			_fgCol = fgSelColor;
		}
	}
	else if( cell_style & CELL_STYLE_READONLY )
	{
		_bgCol = bgReadOnlyColor;
	}

	int _y = ( cell_style & CELL_STYLE_FONT_DEFAULT ) ? Y : Y+1;

	if( cell_style & CELL_STYLE_FONT_DEFAULT )
	{
		// pulisce background / seleziona cella
		GUI_HLine( X, X+W*GUI_CharW(), _y+GUI_CharH()-3, GUI_color( CB_COL_EDIT_BG ) );
		GUI_FillRect( RectI( X, _y, W*GUI_CharW(), GUI_CharH()-3 ), _bgCol );
	}
	else
	{
		// pulisce background / seleziona cella
		//GUI_VLine( X+W*GUI_CharW(), Y, Y+GUI_CharH()-5, GUI_color( CB_COL_EDIT_BG ) );
		GUI_HLine( X, X+W*GUI_CharW(), _y+GUI_CharH()-5, GUI_color( CB_COL_EDIT_BG ) );
		GUI_FillRect( RectI( X, _y, W*GUI_CharW(), GUI_CharH()-5 ), _bgCol );
	}


	if( cell_style & CELL_STYLE_CENTERED || cell_type == CELL_TYPE_YN )
	{
		// disegna testo
		GUI_DrawTextCentered( X, X+W*GUI_CharW(), _y-2, txt, font, _fgCol );
	}
	else
	{
		char* buf = new char[W+1];
		strcpy( buf, txt );

		if( cell_type == CELL_TYPE_UINT || cell_type == CELL_TYPE_SINT || cell_type == CELL_TYPE_UDEC || cell_type == CELL_TYPE_SDEC )
		{
			AllignR( W, buf );
		}
		else
		{
			//Pad( buf, W, ' ' );
		}

		// disegna testo
		GUI_DrawText( X, _y-2, buf, font, _fgCol );

		delete [] buf;
	}

	GUI_Thaw();
}

//--------------------------------------------------------------------------
// Entra in modalita' edit
//--------------------------------------------------------------------------
int CCell::Edit( int key )
{
	// Se la casella non e' modificabile esce
	if( cell_style & CELL_STYLE_READONLY )
	{
		return 0;
	}

	key = manageChar( key );

	// Se il carattere non e' ammesso esce
	if( !checkLegalChars( key ) )
	{
		bipbip();
		return 0;
	}

	// Copia del vecchio contenuto della casella per un eventuale rispristino
	std::string old_txt = txt;
	int result = 0;


	// Se e' presente un set di stringhe
	if( txtSet )
	{
		while( key != K_ENTER && key != K_ESC )
		{
			txtSet_pos++;
			if( txtSet_pos == txtSet_count )
			{
				txtSet_pos = 0;
			}

			SetTxt( txtSet[txtSet_pos] );

			key = Handle();
		}

		result = (key == K_ESC) ? 0 : 1;
	}
	else if( cell_type == CELL_TYPE_YN )
	{
		key = toupper( key );
		const char* Y = SHORT_YES;
		const char* N = SHORT_NO;

		if( key == toupper( *Y ) )
		{
			snprintf( txt, W+1, "%s", LONG_YES );
			result = 1;
		}
		else if( key == toupper( *N ) )
		{
			snprintf( txt, W+1, "%s", LONG_NO );
			result = 1;
		}
	}
	else
	{
		result = editString( key, txt );

		if( result != 0 )
		{
			char* pTxt;

			// Formattazione del testo a seconda del tipo di casella
			switch( cell_type )
			{
				case CELL_TYPE_SINT:
					// controlla che non siano presenti i simboli '+' e '-' dopo il primo carattere
					if( strchr((txt+1),'+') != NULL || strchr((txt+1),'-') != NULL )
					{
						result = 0;
					}
					break;

				case CELL_TYPE_UDEC:
					// controlla che non sia rupetuto il simbolo '.'
					pTxt = strchr( txt, '.' );
					if( pTxt != NULL )
					{
						if( strchr( pTxt+1, '.' ) )
						{
							result = 0;
						}
					}
					break;

				case CELL_TYPE_SDEC:
					// controlla che non siano presenti i simboli '+' e '-' dopo il primo carattere
					if( strchr((txt+1),'+') != NULL || strchr((txt+1),'-') != NULL )
					{
						result = 0;
					}
					else
					{
						// controlla che non sia rupetuto il simbolo '.'
						pTxt = strchr( txt, '.' );
						if( pTxt != NULL )
						{
							if( strchr( pTxt+1, '.' ) )
							{
								result = 0;
							}
						}
					}
					break;

				default:
					break;
			}
		}
	}

	// Se ci sono stati problemi nell'inserimento, il testo viene risettato come
	// prima dell'editing
	if( result == 0 )
	{
		strcpy( txt, old_txt.c_str() );
	}
	else
	{
		checkMinMax();
   	}

	Refresh();

	return result;
}

//--------------------------------------------------------------------------
// Copia il testo in memoria
//--------------------------------------------------------------------------
void CCell::CopyToClipboard()
{
	copyTextToClipboard( txt, cell_type );
}

//--------------------------------------------------------------------------
// Incolla il testo dalla memoria
//--------------------------------------------------------------------------
void CCell::PasteFromClipboard()
{
	if( !editable )
	{
		return;
	}

	if( checkClipboardText() )
	{
		//TODO - manca controllo:
		//  - sui caratteri ammessi
		//  - sulle stringhe preimpostate
		SetTxt( pasteTextFromClipboard() );
		checkMinMax();
	}
}

//--------------------------------------------------------------------------
// Modifica carattere passato in base a legalset
//--------------------------------------------------------------------------
char CCell::manageChar( char key )
{
	// rimuove modificatori aggiunti in keyRead()
	key &= ~(K_LSHIFT | K_RSHIFT | K_LCTRL | K_RCTRL | K_LALT | K_RALT );

	if( key < 256 )
	{
		if( cell_style & CELL_STYLE_UPPERCASE )
		{
			key = toupper( key );
		}
		else if( cell_style & CELL_STYLE_LOWERCASE )
		{
			key = tolower( key );
		}
	}

	return key;
}

//--------------------------------------------------------------------------
// Verifica se il carattere passato e' valido
//--------------------------------------------------------------------------
int CCell::checkLegalChars( int key )
{
	if( cell_type == CELL_TYPE_YN )
	{
		key = toupper( key );
		const char* Y = SHORT_YES;
		const char* N = SHORT_NO;
		return ( key == toupper(*Y) || key == toupper(*N) ) ? 1 : 0;
	}

	return ischar_legal( legalset.c_str(), key );
}

//--------------------------------------------------------------------------
// Gestisce edit della stringa con controllo dei caratteri validi e la
// stampa a video alle coordinate passate
// L'inserimento di un nuovo carattere implica l'overwriting della stringa
// RETURN: 1 se il carattere e' diverso da ESC, 0 altrimenti
//   NOTE: Modifica la stringa
//--------------------------------------------------------------------------
int CCell::editString( char key, char* s )
{
	//se modo no-overwrite -> cancella testo presente
	if( cell_style & CELL_STYLE_OVERWRITE )
	{
		// Inserisce primo carattere lasciando gli altri
		if( W > 0 )
		{
			if( s[0] == '\0' )
				s[1] = '\0';
			s[0] = key;
		}
	}
	else
	{
		// Inserisce primo carattere cancellando il resto
		snprintf( s, W+1, "%c", key );
	}

	int c, pos = 1;
	int insert = 0;

	GUI_font font = ( cell_style & CELL_STYLE_FONT_DEFAULT ) ? GUI_DefaultFont : GUI_SmallFont;
	GUI_color _fgCol( 0, 0, 0 );
	int len = strlen( s );

	// Ciclo di gestione dei tasti premuti
	do
	{
		if( pos >= W )
		{
			pos = W-1;
		}
		// Posiziona il cursore
		int curX = X + pos*GUI_CharW(font);

		GUI_Freeze();

		// pulisce background e stampa la stringa e il cirsore
		if( cell_style & CELL_STYLE_FONT_DEFAULT )
		{
			GUI_FillRect( RectI( X, Y, W*GUI_CharW(), GUI_CharH()-3 ), GUI_color( CB_COL_EDIT_BG ) );
			GUI_DrawText( X, Y-2, s, font, _fgCol );
			GUI_Rect( RectI( curX, Y, GUI_CharW(font)+1, GUI_CharH(font)-3 ), GUI_color( 0,0,0 ) );
		}
		else
		{
			GUI_FillRect( RectI( X, Y, W*GUI_CharW()+1, GUI_CharH()-3 ), GUI_color( CB_COL_EDIT_BG ) );
			GUI_DrawText( X, Y-1, s, font, _fgCol );
			GUI_Rect( RectI( curX, Y+1, GUI_CharW(font)+1, GUI_CharH(font)-3 ), GUI_color( 0,0,0 ) );
		}

		GUI_Thaw();

		switch( c = Handle() )
		{
			case K_HOME:
				pos = 0;
				break;

			case K_END:
				pos = len;
				break;

			case K_INSERT:
				insert = !insert;
				break;

			case K_LEFT:
				if( pos > 0 )
				{
					pos--;
				}
				break;

			case K_RIGHT:
				if( pos < len )
				{
					pos++;
				}
				break;

			case K_BACKSPACE:
				if( pos > 0 )
				{
					memmove(&s[pos-1], &s[pos], len - pos + 1);
					pos--;
					len--;
				}
				break;

			case K_DEL:
				if( pos < len )
				{
					memmove(&s[pos], &s[pos+1], len - pos);
					len--;
				}
				break;

			case K_ENTER:
				break;

			case K_UP:
			case K_DOWN:
				c = K_ENTER;
				break;

			case K_ESC:
				len = 0;
				break;

			default:
				// Modifica caratteri della stringa
				c = manageChar( c );

				if(((legalset[0] == 0) || (strchr(legalset.c_str(), c) != NULL)) && ((c >= ' ') && (c <= '~')))
				{
					if( insert && len < W )
					{
						memmove(&s[pos + 1], &s[pos], len - pos + 1);
						len++;
						s[pos++] = c;
					}
					else // non insert
					{
						if( pos >= len && len < W && !insert )
						{
							len++;
							s[pos++] = c;
						}
						else
						{
							if( pos < len && !insert )
							{
								s[pos++] = c;
							}
						}
					}
				}
				break;
		}
		s[len] = 0;
	}
	while( (c != K_ENTER) && (c != K_ESC) );

	return (c != K_ESC);
}

//--------------------------------------------------------------------------
// Elimina il set di stringhe
//--------------------------------------------------------------------------
void CCell::clearTxtSet()
{
	if( txtSet )
	{
		for( int i = 0; i < txtSet_count; i++ )
		{
			delete [] txtSet[i];
		}
		delete [] txtSet;
		txtSet = 0;
	}

	txtSet_count = 0;
	txtSet_pos = 0;
}

//--------------------------------------------------------------------------
// Controlla testo nella clipboard
//--------------------------------------------------------------------------
bool CCell::checkClipboardText()
{
	int type = getClipboardTextType();

	if( type < CELL_TYPE_FIRST || type > CELL_TYPE_LAST )
	{
		return false;
	}

	return typeMatrix[type][cell_type];
}

//--------------------------------------------------------------------------
// Controlla valore minimo/massimo ammissibile
//--------------------------------------------------------------------------
void CCell::checkMinMax()
{
	if( cell_type == CELL_TYPE_UINT || cell_type == CELL_TYPE_SINT )
	{
		int val = atoi(txt);
		val = MID( minval, val, maxval );

		snprintf( txt, W+1, "%d", val );
	}
	else if( cell_type == CELL_TYPE_UDEC || cell_type == CELL_TYPE_SDEC )
	{
		float val = atof( txt );
		val = MID( minval, val, maxval );

		char format[20];
		snprintf( format, 20, "%%%d.%df", W, decdgt );
		snprintf( txt, W+1, format, val );
	}
}
