//---------------------------------------------------------------------------
//
// Name:        gui_statusbar.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: GUI_StatusBar class definition
//
//---------------------------------------------------------------------------

#ifndef __GUI_STATUSBAR_H
#define __GUI_STATUSBAR_H

#include <vector>

using namespace std;


struct GUI_StatusData
{
	int width;
	char* label;
	char type;
	char show;
};


#define SB_EMPTY    -1
#define SB_TEXT     0
#define SB_GRAPH_1  1 // square
#define SB_GRAPH_2  2 // square used in menu


class GUI_StatusBar
{
	public:
		GUI_StatusBar( int y, int h );
		~GUI_StatusBar();
		
		void Add( int width, int type, const char* txt, int show = 1 );
		
		void SetLabel( int n, const char* label, bool show = true );
		void SetStatus( int n, int show );
		void SetType( int n, int type );
		
		void Show();
		void Hide() { isShown = false; }

	protected:
		void showItems();
		
		int Y, H;
		bool isShown;
		
		vector<GUI_StatusData*> items;
};

#endif
