//---------------------------------------------------------------------------
//
// Name:        c_win_imgpar_pack.cpp
// Author:      Gabriel Ferri
// Created:     15/09/2011
// Description: Quadra package image configuration parameters
//
//---------------------------------------------------------------------------
#include "c_win_imgpar_pack.h"

#include "c_win_par.h"
#include "msglist.h"
#include "q_grcol.h"
#include "q_prog.h"
#include "q_vision.h"
#include "q_help.h"
#include "q_oper.h"
#include "bitmap.h"
#include "gui_defs.h"

#include <mss.h>


bool PackImageShow( char* filename, int ximg, int yimg, int xmax, int ymax, float& xscale, float& yscale );
bool PackTemplateBoxShow( char* filename, int ximg, int yimg, float xscale, float yscale );


//---------------------------------------------------------------------------
// Package images parameters UI
//---------------------------------------------------------------------------

class PackImagesParametersUI : public CWindowParams
{
public:
	PackImagesParametersUI( CWindow* parent, int pack_code, char* pack_lib ) : CWindowParams( parent )
	{
		packCode = pack_code;
		strcpy( packLib, pack_lib );
		
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 69, 29 );
		
		// set title
		SetTitle( MsgGetString(Msg_00756) );
	}

	typedef enum
	{
		MATCH_THR,
		MATCH_ITER
	} combo_labels;

protected:
	void onInit()
	{
		LoadData();
		
		// create combos
		m_combos[MATCH_THR]  = new C_Combo( 4, 27, MsgGetString(Msg_00776), 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[MATCH_ITER] = new C_Combo( 36, 27, MsgGetString(Msg_01615), 5, CELL_TYPE_UINT );
		
		// set params
		m_combos[MATCH_THR]->SetVMinMax( 0.f, 1.f );
		m_combos[MATCH_ITER]->SetVMinMax( 1, 10 );
		
		// add to combo list
		m_comboList->Add( m_combos[MATCH_THR],  0, 0 );
		m_comboList->Add( m_combos[MATCH_ITER], 0, 1 );
	}

	void onShow()
	{
		DrawText( 2,  6, MsgGetString(Msg_00042) );
		DrawText( 2, 19, MsgGetString(Msg_00043) );

		ShowImage();
	}

	void onRefresh()
	{
		m_combos[MATCH_THR]->SetTxt( packVisData.match_thr );
		m_combos[MATCH_ITER]->SetTxt( packVisData.niteraz );
	}

	void onEdit()
	{
		packVisData.match_thr = m_combos[MATCH_THR]->GetFloat();
		packVisData.niteraz = m_combos[MATCH_ITER]->GetInt();
	}

	void onClose()
	{
		SaveData();
	}

	void LoadData()
	{
		// carica dati package
		char filename[MAXNPATH+1];
		PackVisData_GetFilename( filename, packCode, packLib );

		PackVisData_Open( filename );
		PackVisData_Read( packVisData );
		PackVisData_Close();
	}

	void SaveData()
	{
		// salva dati package
		char filename[MAXNPATH+1];
		PackVisData_GetFilename( filename, packCode, packLib );

		PackVisData_Open( filename );
		PackVisData_Write( packVisData );
		PackVisData_Close();
	}

	void ShowImage()
	{
		char filename[MAXNPATH+1];

		int ximgL = GetX() + 254;
		int ximgR = ximgL + 266;
		int yimgH = GetY() + 135;
		int yimgL = yimgH + 266;
		int xmax = SHOW_IMG_MAXX;
		int ymax = SHOW_IMG_MAXY;
		float scalex, scaley;

		SetImageName( filename, PACKAGEVISION_LEFT_1, IMAGE, packCode, packLib );
		if( PackImageShow( filename, ximgL, yimgH, xmax, ymax, scalex, scaley ) )
		{
			SetImageName( filename, PACKAGEVISION_LEFT_1, ELAB, packCode, packLib );
			PackTemplateBoxShow( filename, ximgL, yimgH, scalex, scaley );
		}

		SetImageName( filename, PACKAGEVISION_RIGHT_1, IMAGE, packCode, packLib );
		if( PackImageShow( filename, ximgR, yimgH, xmax, ymax, scalex, scaley ) )
		{
			SetImageName( filename, PACKAGEVISION_RIGHT_1, ELAB, packCode, packLib );
			PackTemplateBoxShow( filename, ximgR, yimgH, scalex, scaley );
		}

		SetImageName( filename, PACKAGEVISION_LEFT_2, IMAGE, packCode, packLib );
		if( PackImageShow( filename, ximgL, yimgL, xmax, ymax, scalex, scaley ) )
		{
			SetImageName( filename, PACKAGEVISION_LEFT_2, ELAB, packCode, packLib );
			PackTemplateBoxShow( filename, ximgL, yimgL, scalex, scaley );
		}

		SetImageName( filename, PACKAGEVISION_RIGHT_2, IMAGE, packCode, packLib );
		if( PackImageShow( filename, ximgR, yimgL, xmax, ymax, scalex, scaley ) )
		{
			SetImageName( filename, PACKAGEVISION_RIGHT_2, ELAB, packCode, packLib );
			PackTemplateBoxShow( filename, ximgR, yimgL, scalex, scaley );
		}
	}

	int packCode;
	char packLib[MAXNPATH+1];
	struct PackVisData packVisData;
};

//---------------------------------------------------------------------------
// Mostra finestra parametri immagine
//---------------------------------------------------------------------------
int ShowPackImgParams( CWindow* parent, int pack_code, char* pack_lib )
{
	PackImagesParametersUI win( parent, pack_code, pack_lib );
	win.Show();
	win.Hide();

	return 1;
}

//---------------------------------------------------------------------------
// Visualizza immagine
// filename      : immagine da mostrare
// ximg,yimg     : posizione sullo schermo dove posizionare l'immagine
// xmax,ymax     : dimensioni massime
// xscale,yscale : ritorna la scala utilizzata
//---------------------------------------------------------------------------
bool PackImageShow( char* filename, int ximg, int yimg, int xmax, int ymax, float& xscale, float& yscale )
{
	if( access( filename, 0 ) != 0 )
		return false;

	bitmap image( filename );

	int xdim = image.get_width();
	int ydim = image.get_height();

	if( xdim > xmax )
	{
		xdim = xmax;
		xscale = ((float)image.get_width())/((float)xdim);
	}
	else
	{
		xscale = 1.f;
	}

	if( ydim > ymax )
	{
		ydim = ymax;
		yscale = ((float)image.get_height())/((float)ydim);
	}
	else
	{
		yscale = 1.f;
	}

	if( xdim != image.get_width() || ydim != image.get_height() )
	{
		image.redim(xdim,ydim);
	}

	GUI_Rect( RectI(ximg-xmax/2-1, yimg-ymax/2-1, xmax+2, ymax+2), GUI_color( GR_BLACK ) );
	image.show( ximg, yimg );
	return true;
}

//---------------------------------------------------------------------------
// Visualizza box immagine template (forzando la scala)
//---------------------------------------------------------------------------
bool PackTemplateBoxShow( char* filename, int ximg, int yimg, float xscale, float yscale )
{
	if( access(filename,0) != 0 )
		return false;

	bitmap image( filename );

	int xmax = int(image.get_width()/xscale);
	int ymax = int(image.get_height()/yscale);

	GUI_Rect( RectI(ximg-xmax/2-1, yimg-ymax/2-1, xmax+2, ymax+2), GUI_color( GR_LIGHTGREEN ) );
	return true;
}
