//---------------------------------------------------------------------------
//
// Name:        c_win_imgpar.cpp
// Author:      Gabriel Ferri
// Created:     15/09/2011
// Description: Quadra image configuration parameters
//
//---------------------------------------------------------------------------
#include "c_win_imgpar.h"

#include "c_win_par.h"
#include "gui_defs.h"

#include "msglist.h"
#include "q_prog.h"
#include "q_vision.h"
#include "q_help.h"
#include "q_oper.h"
#include "bitmap.h"
#include "keyutils.h"

#include <mss.h>


extern struct img_data imgData;
extern CfgHeader QHeader;

int ip_imgType;

bool ImageShow( char* filename, int ximg, int yimg, int xmax, int ymax, float& xscale, float& yscale );
bool ImageShow( char* filename, int ximg, int yimg, float xscale, float yscale );


//---------------------------------------------------------------------------
// Image parameters UI
//---------------------------------------------------------------------------

class ImageParametersUI : public CWindowParams
{
public:
	ImageParametersUI( int type, int num, char* libname ) : CWindowParams( 0 )
	{
		imgType = type;
		imgNum = num;
		isPack = libname ? true : false;
		
		SetStyle( WIN_STYLE_CENTERED );
		SetClientAreaSize( 71, 35 );
		
		// set title
		switch( imgType )
		{
			case ZEROIMG:
				SetTitle( MsgGetString(Msg_00750) );
				break;
			case RIFEIMG:
				SetTitle( MsgGetString(Msg_00751) );
				break;
			default:
				SetTitle( MsgGetString(Msg_00744) );
				break;
		}
		
		//
		GetImageName( imgName, imgType, imgNum, libname );
		
		// Eliminazione del path dal nome del file
		strcpy( imgNameNoPath, imgName );
		for( char* tok = strtok( imgNameNoPath, "/"); tok; tok = strtok( 0, "/" ) )
		{
			if( tok != NULL )
				strcpy( imgNameNoPath, tok );
		}
	}

	typedef enum
	{
		FILENAME,
		BRIGHTNESS,
		CONTRAST,
		SEARCH_W,
		SEARCH_H,
		MATCH_ITER,
		MATCH_THR,
		PATTERN_W,
		PATTERN_H,
		FILTER_TYPE,
		FILTER_P1,
		FILTER_P2,
		FILTER_P3,
		DIAMETER,
		TOLERANCE,
		SMOOTH,
		EDGE,
		ACCUMULATOR,
		VECT_SEARCH_W,
		VECT_SEARCH_H
	} combo_labels;

protected:
	void onInit()
	{
		LoadData();
		
		// create combos
		m_combos[FILENAME]    	= new C_Combo(  4, 14, MsgGetString(Msg_00768), 22, CELL_TYPE_TEXT, CELL_STYLE_READONLY | CELL_STYLE_NOSEL );
		
		m_combos[BRIGHTNESS]  	= new C_Combo(  4, 15, MsgGetString(Msg_00769), 5, CELL_TYPE_UINT );
		m_combos[CONTRAST]    	= new C_Combo( 38, 15, MsgGetString(Msg_00770), 5, CELL_TYPE_UINT );
		
		m_combos[SEARCH_W]    	= new C_Combo(  4, 19, MsgGetString(Msg_00771), 5, CELL_TYPE_UINT );
		m_combos[SEARCH_H]    	= new C_Combo(  4, 20, MsgGetString(Msg_00772), 5, CELL_TYPE_UINT );
		m_combos[MATCH_ITER]  	= new C_Combo(  4, 23, MsgGetString(Msg_00774), 5, CELL_TYPE_UINT );
		m_combos[MATCH_THR]   	= new C_Combo(  4, 24, MsgGetString(Msg_00777), 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );

		m_combos[VECT_SEARCH_W] = new C_Combo(  4, 29, MsgGetString(Msg_00771), 5, CELL_TYPE_UINT );
		m_combos[VECT_SEARCH_H] = new C_Combo(  4, 30, MsgGetString(Msg_00772), 5, CELL_TYPE_UINT );
		m_combos[DIAMETER]    	= new C_Combo(  4, 31, MsgGetString(Msg_07015), 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		
		m_combos[PATTERN_W]   	= new C_Combo( 38, 19, MsgGetString(Msg_00778), 5, CELL_TYPE_UINT );
		m_combos[PATTERN_H]   	= new C_Combo( 38, 20, MsgGetString(Msg_00779), 5, CELL_TYPE_UINT );
		m_combos[FILTER_TYPE] 	= new C_Combo( 38, 21, MsgGetString(Msg_00780), 5, CELL_TYPE_UINT );
		m_combos[FILTER_P1]   	= new C_Combo( 38, 22, MsgGetString(Msg_00781), 5, CELL_TYPE_UINT );
		m_combos[FILTER_P2]   	= new C_Combo( 38, 23, MsgGetString(Msg_00782), 5, CELL_TYPE_UINT );
		m_combos[FILTER_P3]   	= new C_Combo( 38, 24, MsgGetString(Msg_00783), 5, CELL_TYPE_UINT );

		m_combos[SMOOTH]      	= new C_Combo( 38, 29, MsgGetString(Msg_07017), 5, CELL_TYPE_UINT );
		m_combos[ACCUMULATOR] 	= new C_Combo( 38, 30, MsgGetString(Msg_07019), 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[TOLERANCE]   	= new C_Combo( 38, 31, MsgGetString(Msg_07016), 5, CELL_TYPE_UDEC, CELL_STYLE_DEFAULT, 2 );
		m_combos[EDGE]   	  	= new C_Combo( 38, 32, MsgGetString(Msg_07018), 5, CELL_TYPE_UINT );
		
		// set text bkg color
		for( int i=3; i<20; i++ )
			m_combos[i]->SetTxtBkgColor( GUI_color( WIN_COL_SUBTITLE ) );

		// set params
		m_combos[FILENAME]->SetLegalChars( CHARSET_FILENAME );
		m_combos[BRIGHTNESS]->SetVMinMax( MIN_BRIGHT_VALUE, MAX_BRIGHT_VALUE );
		m_combos[CONTRAST]->SetVMinMax( MIN_CONTRAST_VALUE, MAX_CONTRAST_VALUE );
		m_combos[SEARCH_W]->SetVMinMax( MIN_ATLANTE_VALUE, isPack ? PKG_BRD_IMG_MAXX : BRD_IMG_MAXX );
		m_combos[SEARCH_H]->SetVMinMax( MIN_ATLANTE_VALUE, isPack ? PKG_BRD_IMG_MAXY : BRD_IMG_MAXY );
		m_combos[MATCH_ITER]->SetVMinMax( MIN_ITER_VALUE, MAX_ITER_VALUE );
		m_combos[MATCH_THR]->SetVMinMax( (float)MIN_THR_VALUE, (float)MAX_THR_VALUE );
		m_combos[PATTERN_W]->SetVMinMax( PATTERN_MINX, isPack ? PACK_PATTERN_MAXX : PATTERN_MAXX );
		m_combos[PATTERN_H]->SetVMinMax( PATTERN_MINY, isPack ? PACK_PATTERN_MAXY : PATTERN_MAXY );
		m_combos[FILTER_TYPE]->SetVMinMax( MIN_FTYPE_VALUE, MAX_FTYPE_VALUE );
		m_combos[FILTER_P1]->SetVMinMax( MIN_FP1_VALUE, MAX_FP1_VALUE );
		m_combos[FILTER_P2]->SetVMinMax( MIN_FP2_VALUE, MAX_FP2_VALUE );
		m_combos[FILTER_P3]->SetVMinMax( MIN_FP3_VALUE, MAX_FP3_VALUE );
		m_combos[DIAMETER]->SetVMinMax( (float)MIN_DIAMETER_VALUE, (float)MAX_DIAMETER_VALUE );
		m_combos[TOLERANCE]->SetVMinMax( (float)MIN_TOLERANCE_VALUE, (float)MAX_TOLERANCE_VALUE );
		m_combos[SMOOTH]->SetVMinMax( MIN_SMOOTH_VALUE, MAX_SMOOTH_VALUE );
		m_combos[EDGE]->SetVMinMax( MIN_EDGE_VALUE, MAX_EDGE_VALUE );
		m_combos[ACCUMULATOR]->SetVMinMax( (float)MIN_ACCUMULATOR_VALUE, (float)MAX_ACCUMULATOR_VALUE );
		m_combos[VECT_SEARCH_W]->SetVMinMax( MIN_SEARCHX_VALUE, MAX_SEARCHX_VALUE );
		m_combos[VECT_SEARCH_H]->SetVMinMax( MIN_SEARCHY_VALUE, MAX_SEARCHY_VALUE );
		
		// add to combo list
		m_comboList->Add( m_combos[FILENAME],      0, 0 );
		m_comboList->Add( m_combos[BRIGHTNESS],    1, 0 );
		m_comboList->Add( m_combos[CONTRAST],      1, 1 );
		m_comboList->Add( m_combos[SEARCH_W],      2, 0 );
		m_comboList->Add( m_combos[SEARCH_H],      3, 0 );
		m_comboList->Add( m_combos[MATCH_ITER],    6, 0 );
		m_comboList->Add( m_combos[MATCH_THR],     7, 0 );
		m_comboList->Add( m_combos[VECT_SEARCH_W], 8, 0 );
		m_comboList->Add( m_combos[VECT_SEARCH_H], 9, 0 );
		m_comboList->Add( m_combos[DIAMETER],      10, 0 );

		m_comboList->Add( m_combos[PATTERN_W],     2, 1 );
		m_comboList->Add( m_combos[PATTERN_H],     3, 1 );
		m_comboList->Add( m_combos[FILTER_TYPE],   4, 1 );
		m_comboList->Add( m_combos[FILTER_P1],     5, 1 );
		m_comboList->Add( m_combos[FILTER_P2],     6, 1 );
		m_comboList->Add( m_combos[FILTER_P3],     7, 1 );
		m_comboList->Add( m_combos[SMOOTH],        8, 1 );
		m_comboList->Add( m_combos[ACCUMULATOR],   9, 1 );
		m_comboList->Add( m_combos[TOLERANCE],     10, 1 );
		m_comboList->Add( m_combos[EDGE],          11, 1 );
	}

	void onShow()
	{
		DrawPanel( RectI( 2, 17, GetW()/GUI_CharW() - 4, 9 ) );
		DrawTextCentered( 17, MsgGetString(Msg_07020), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );

		DrawText( 29, 15, "%" );
		DrawText( 63, 15, "%" );
		DrawText( 29, 19, "pix", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 29, 20, "pix", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 63, 19, "pix", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 63, 20, "pix", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		
		DrawPanel( RectI( 2, 27, GetW()/GUI_CharW() - 4, 7 ) );
		DrawTextCentered( 27, MsgGetString(Msg_07021), GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 29, 29, "pix", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );
		DrawText( 29, 30, "pix", GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( WIN_COL_TXT ) );

		ShowImage();
	}

	void onRefresh()
	{
		m_combos[FILENAME]->SetTxt( imgNameNoPath );
		m_combos[BRIGHTNESS]->SetTxt( imgData.bright );
		m_combos[CONTRAST]->SetTxt( imgData.contrast );
		m_combos[SEARCH_W]->SetTxt( imgData.atlante_x );
		m_combos[SEARCH_H]->SetTxt( imgData.atlante_y );
		m_combos[MATCH_ITER]->SetTxt( imgData.match_iter );
		m_combos[MATCH_THR]->SetTxt( imgData.match_thr );
		m_combos[PATTERN_W]->SetTxt( imgData.pattern_x );
		m_combos[PATTERN_H]->SetTxt( imgData.pattern_y );
		m_combos[FILTER_TYPE]->SetTxt( imgData.filter_type );
		m_combos[FILTER_P1]->SetTxt( imgData.filter_p1 );
		m_combos[FILTER_P2]->SetTxt( imgData.filter_p2 );
		m_combos[FILTER_P3]->SetTxt( imgData.filter_p3 );
		m_combos[DIAMETER]->SetTxt( imgData.vect_diameter );
		m_combos[TOLERANCE]->SetTxt( imgData.vect_tolerance );
		m_combos[SMOOTH]->SetTxt( imgData.vect_smooth );
		m_combos[EDGE]->SetTxt( imgData.vect_edge );
		m_combos[ACCUMULATOR]->SetTxt( imgData.vect_accumulator );
		m_combos[VECT_SEARCH_W]->SetTxt( imgData.vect_atlante_x );
		m_combos[VECT_SEARCH_H]->SetTxt( imgData.vect_atlante_y );
	}

	void onEdit()
	{
		imgData.bright = m_combos[BRIGHTNESS]->GetInt();
		imgData.contrast = m_combos[CONTRAST]->GetInt();
		
		imgData.atlante_x = m_combos[SEARCH_W]->GetInt();
		imgData.atlante_y = m_combos[SEARCH_H]->GetInt();
		imgData.match_iter = m_combos[MATCH_ITER]->GetInt();
		imgData.match_thr = m_combos[MATCH_THR]->GetFloat();
		imgData.pattern_x = m_combos[PATTERN_W]->GetInt();
		imgData.pattern_y = m_combos[PATTERN_H]->GetInt();
		imgData.filter_type = m_combos[FILTER_TYPE]->GetInt();
		imgData.filter_p1 = m_combos[FILTER_P1]->GetInt();
		imgData.filter_p2 = m_combos[FILTER_P2]->GetInt();
		imgData.filter_p3 = m_combos[FILTER_P3]->GetInt();
		imgData.vect_diameter = m_combos[DIAMETER]->GetFloat();
		imgData.vect_tolerance = m_combos[TOLERANCE]->GetFloat();
		imgData.vect_smooth = (unsigned char)m_combos[SMOOTH]->GetInt();
		imgData.vect_edge = (unsigned char)m_combos[EDGE]->GetInt();
		imgData.vect_accumulator = m_combos[ACCUMULATOR]->GetFloat();
		imgData.vect_atlante_x = m_combos[VECT_SEARCH_W]->GetInt();
		imgData.vect_atlante_y = m_combos[VECT_SEARCH_H]->GetInt();
	}

	void onShowMenu()
	{
		if( imgType <= MAPPING_IMG )
		{
			m_menu->Add( MsgGetString(Msg_00748), K_F3, 0, NULL, boost::bind( &ImageParametersUI::onImageTeach, this ) ); // Apprendimento immagine
			m_menu->Add( MsgGetString(Msg_07022), K_F4, 0, NULL, boost::bind( &ImageParametersUI::Default_VectorialParams, this ) ); // Reset parametri vettoriali al default
		}
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_F3:
				onImageTeach();
				return true;
			
			case K_F4:
				Default_VectorialParams();
				return true;

			default:
				break;
		}
	
		return false;
	}

	void onClose()
	{
		SaveData();
	}

	void LoadData()
	{
		char filename[MAXNPATH+1];
		strcpy( filename, imgName );
		
		AppendImageMode( filename, imgType, DATA );
		
		ImgDataLoad( filename, &imgData );
	}

	void SaveData()
	{
		char filename[MAXNPATH+1];
		strncpy( filename, imgName, MAXNPATH );
		AppendImageMode( filename, imgType, DATA );

		ImgDataSave( filename, &imgData );
	}

	void ShowImage()
	{
		int ximg = GetX() + 170;
		int yimg = GetY() + 135;
		int xmax = SHOW_IMG_MAXX;
		int ymax = SHOW_IMG_MAXY;
		float scalex,scaley;
		
		char filename[MAXNPATH+1];
		strncpy( filename, imgName, MAXNPATH );
		AppendImageMode( filename, imgType, IMAGE );
		
		bool image_present = ImageShow( filename, ximg, yimg, xmax, ymax, scalex, scaley );
	
		strcpy( filename, imgName );
		AppendImageMode( filename, imgType, ELAB );
	
		if( image_present )
			ImageShow( filename, ximg+250, yimg, scalex, scaley );
		
		if( scalex < 1 )
		{
			scalex = 1;
		}
		if(scaley < 1 )
		{
			scaley = 1;
		}
	
		ShowImgBox( ximg, yimg, MIN(xmax,ftoi(imgData.atlante_x/scalex)), MIN(ymax,ftoi(imgData.atlante_y/scaley)) );
	}

protected:

	//---------------------------------------------------------------------------
	// Apprendimento immagine
	//---------------------------------------------------------------------------
	int onImageTeach()
	{
		float c_ax, c_ay;

		if( ip_imgType == ZEROIMG || ip_imgType == RIFEIMG )
		{
			ZerFile* zer = new ZerFile( QHeader.Prg_Default );
			if( !zer->Open() )
			{
				W_Mess( NOZSCHFILE );
				delete zer;
				return 0;
			}
			else
			{
				struct Zeri KK_Zeri;
				zer->Read( KK_Zeri, 1 );

				if( ip_imgType == ZEROIMG )
				{
					c_ax = KK_Zeri.Z_xzero;
					c_ay = KK_Zeri.Z_yzero;
				}
				else
				{
					c_ax = KK_Zeri.Z_xrif;
					c_ay = KK_Zeri.Z_yrif;
				}
			}

			delete zer;
		}
		else
		{
			c_ax = 0;
			c_ay = 0;
		}

		// setta i parametri della telecamera
		SetImgBrightCont( imgData.bright*655, imgData.contrast*655 );
		int prevBright = GetImageBright();
		int prevContrast = GetImageContrast();

		Set_Tv(2); // predispone per non richiudere immagine su video

		if( ManualTeaching( &c_ax, &c_ay, MsgGetString(Msg_00748), 0, CAMERA_HEAD ) )
		{
			// memorizzare immagine ?
			if( W_Deci( 1, MsgGetString(Msg_00729) ) )
			{
				ImageCaptureSave( ip_imgType, YESCHOOSE, CAMERA_HEAD );
			}

			imgData.bright = GetImageBright() / 655;
			imgData.contrast = GetImageContrast() / 655;

			ShowImage();
		}

		Set_Tv(3); // chiude immagine su video

		// ripristina vecchi valori
		SetImgBrightCont( prevBright, prevContrast );
		return 1;
	}

	//---------------------------------------------------------------------------
	// Resetta i parametri vettoriali al default
	//---------------------------------------------------------------------------
	int Default_VectorialParams()
	{
		imgData.vect_diameter = DEF_DIAMETER_VALUE;
		imgData.vect_tolerance = DEF_TOLERANCE_VALUE;
		imgData.vect_smooth = DEF_SMOOTH_VALUE;
		imgData.vect_edge = DEF_EDGE_VALUE;
		imgData.vect_accumulator = DEF_ACCUMULATOR_VALUE;
		imgData.vect_atlante_x = DEF_SEARCHX_VALUE;
		imgData.vect_atlante_y = DEF_SEARCHY_VALUE;

		return 1;
	}

	img_data imgData;
	int imgType;
	int imgNum;
	char imgName[MAXNPATH+1];
	char imgNameNoPath[MAXNPATH+1];
	bool isPack;
};

//---------------------------------------------------------------------------
// Mostra finestra parametri immagine
//---------------------------------------------------------------------------
int ShowImgParams( int type, int num, char* libname )
{
	ip_imgType = type;

	ImageParametersUI win( type, num, libname );
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
bool ImageShow( char* filename, int ximg, int yimg, int xmax, int ymax, float& xscale, float& yscale )
{
	if( access(filename,0) != 0 )
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

	GUI_Rect( RectI( ximg-xmax/2-1, yimg-ymax/2-1, xmax+2, ymax+2), GUI_color( 0,0,0) );
	image.show( ximg, yimg );
	return true;
}

//---------------------------------------------------------------------------
// Visualizza immagine (forzando la scala)
//---------------------------------------------------------------------------
bool ImageShow( char* filename, int ximg, int yimg, float xscale, float yscale )
{
	if( access(filename,0) != 0 )
		return false;

	bitmap image( filename );

	int xmax = int(image.get_width()/xscale);
	int ymax = int(image.get_height()/yscale);

	image.setscale( xscale, yscale );

	GUI_Rect( RectI( ximg-xmax/2-1, yimg-ymax/2-1, xmax+2, ymax+2), GUI_color( 0,0,0) );
	image.show( ximg, yimg );
	return true;
}
