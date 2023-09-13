//---------------------------------------------------------------------------
//
// Name:        c_waitbox.h
// Author:      Gabriel Ferri
// Created:     06/06/2012
// Description: CWaitBox class definition
//
//---------------------------------------------------------------------------

#ifndef __CWaitBox_H
#define __CWaitBox_H

#include "c_window.h"
#include "gui_progressbar.h"


class CWaitBox : public CWindow
{
public:
	CWaitBox( CWindow* parent, int y, const char* label, int maxValue = 100 );
	~CWaitBox();

	void Show();

	void SetVMax( int max ) { m_progBar->SetVMinMax( 0, max ); }

	void SetValue( int value ) { m_progBar->SetValue( value ); }
	void Increment() { m_progBar->Increment(); }

protected:
	std::string m_label;
	GUI_ProgressBar* m_progBar;

private:
	void WorkingCycle();
};

#endif
