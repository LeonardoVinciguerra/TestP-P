//---------------------------------------------------------------------------
//
// Name:        c_inputbox.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: CInputBox class definition
//
//---------------------------------------------------------------------------

#ifndef __CINPUTBOX_H
#define __CINPUTBOX_H

#include <string.h>
#include "c_win_par.h"


class CInputBox : public CWindowParams
{
public:
	CInputBox( CWindow* parent, int y, const char* title, const char* label, int editLen, int type = CELL_TYPE_TEXT, int dec_digit = 2 );
	~CInputBox() {}

	void SetText( const char* text ) { m_combos[0]->SetTxt( text ); }
	void SetText( int val ) { m_combos[0]->SetTxt( val ); }
	void SetText( float val ) { m_combos[0]->SetTxt( val ); }

	void SetLegalChars( const char* set ) { m_combos[0]->SetLegalChars( set ); }

	void SetVMinMax( float min, float max ) { m_combos[0]->SetVMinMax( min, max ); }
	void SetVMinMax( int min, int max ) { m_combos[0]->SetVMinMax( min, max ); }

	char* GetText() { return m_combos[0]->GetTxt(); }
	int GetInt() { return m_combos[0]->GetInt(); }
	float GetFloat() { return m_combos[0]->GetFloat(); }

	int GetExitCode() { return m_exitCode; }

	void Show( bool select = true, bool focused = true );

protected:
	void onInit();
	void onShow();
	bool onKeyPress( int key );
	void onClose();

	int m_exitCode;
};

#endif
