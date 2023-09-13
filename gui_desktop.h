//---------------------------------------------------------------------------
//
// Name:        gui_desktop.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_DeskTop class definition
//              Gestione del desktop del programma
//
//---------------------------------------------------------------------------

#ifndef __GUI_DESKTOP_H
#define __GUI_DESKTOP_H

#include "gui_statusbar.h"


class GUI_DeskTop
{
	public:
		GUI_DeskTop();
		~GUI_DeskTop();

		void Show();
		void Quit();

		void ShowBanner();

		void ShowStatusBars( bool showMenuItem );
		void HideStatusBars();
		void SetSB_Top( const char* cust, const char* prog, const char* conf, const char* pack );
		void SetSB_Bottom( int ndisp, char* dosaconf );

		void SetEditMode( bool mode );
		bool GetEditMode() { return editMode; }

		void ShowMenuItem( bool show );
		void MenuItemActivated( bool activated );

	private:
		void createStatusBars();
		bool displayImage( const char* filename, int x, int y );

		void* background;
		void* foreground;

		int sbCounter;

		GUI_StatusBar* sb_top;
		GUI_StatusBar* sb_bottom;

		bool editMode;
};

#endif
