//---------------------------------------------------------------------------
//
// Name:        q_mapping.cpp
// Author:      Gabriel Ferri
// Created:     14/11/2011
// Description:
//
//---------------------------------------------------------------------------
#include "q_mapping.h"

#include "c_win_par.h"
#include "gui_defs.h"
#include "msglist.h"
#include "q_oper.h"
#include "q_vision.h"
#include "q_help.h"
#include "keyutils.h"
#include "q_prog.h"
#include "q_tabe.h"
#include "bitmap.h"
#include "bitmap.h"
#include "lnxdefs.h"
#include "q_packages.h"
#include "q_grcol.h"
#include "q_debug.h"

#include <mss.h>


#define DEF_MAP_X              25
#define DEF_MAP_Y              32
#define DEF_MAP_GAP_Y          5
#define DEF_MAP_GAP_X1         5
#define DEF_MAP_GAP_X2         4
#define DEF_MAP_OFFSET_MM      20.f

#define CHECK_ERROR_MAX        0.03f

#define CHECK_ERROR_L1         0.02f // livello 1: zona errore limite
#define CHECK_ERROR_L2         0.04f // livello 2: errore non accettabile



extern CfgParam QParam;
extern CfgHeader QHeader;
extern struct vis_data Vision;
extern SPackageData currentLibPackages[MAXPACK];

extern void Image_showDebug( bitmap* img, bitmap* pattern, float corr, float threshold, int errX, int errY );



	//-----------------//
	//   Global vars   //
	//-----------------//

extern bool use_mapping_correction;

int mapTestDebug = 0;
int mapTestLight = 0;
float mapTestX = 0.f;
float mapTestY = 0.f;


struct eMap_point
{
	eMap_point()
	{
		dx = dy = 0.f;
		px = py = 0;
		s = 0;
	}

	float dx;
	float dy;
	int px;
	int py;
	char s; // stato: 0 = errore, 1 = ok
};

struct eMapStruct
{
	vector<eMap_point> data;
	float x, y;
	int nx, ny;
	float sx, sy;
};

eMapStruct eMap;


// forward declaration
bool saveMapData( const char* filename, vector<eMap_point>& data, float posX, float posY, int numX, int numY, float dX, float dY );
bool loadMapData( const char* filename, eMapStruct& mapdata );



//---------------------------------------------------------------------------------
// Mostra informazioni debug mappatura
//---------------------------------------------------------------------------------
#define VIDEO_CALLBACK_Y      29

int mdErrorX = 0;
int mdErrorY = 0;

void displayInfoMappingDebug( void* parentWin )
{
	GUI_Freeze_Locker lock;

	CWindow* parent = (CWindow*)parentWin;

	C_Combo c3( 10, VIDEO_CALLBACK_Y + 2, "Error X pix :", 8, CELL_TYPE_SINT );
	c3.SetTxt( mdErrorX );
	c3.Show( parent->GetX(), parent->GetY() );

	C_Combo c4( 45, VIDEO_CALLBACK_Y + 2, "Error Y pix :", 8, CELL_TYPE_SINT );
	c4.SetTxt( mdErrorY );
	c4.Show( parent->GetX(), parent->GetY() );
}

//-----------------------------------------------------------------------------
// Visualizza il risultato del riconoscimento
//-----------------------------------------------------------------------------
void Mapping_showDebug( bitmap* img, int errX, int errY )
{
	mdErrorX = errX;
	mdErrorY = errY;

	Set_Tv_Title( "Vision debug" );
	Set_Tv_FromImage( 1, true, displayInfoMappingDebug );

	if( img )
	{
		img->showFrame( GetVideoCenterX(), GetVideoCenterY() );
		img->showDebug( GetVideoCenterX(), GetVideoCenterY() );
	}

	//svuota buffer
	flushKeyboardBuffer();
	// attende pressione ESC
	while( !Esc_Press() );

	Set_Tv_FromImage( 0 );
}



//---------------------------------------------------------------------------
// finestra: Show mapping data
//---------------------------------------------------------------------------
#include <vector>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
using namespace boost::accumulators;

#define SMD_VERIFY               0
#define SMD_CORRECTION           1
#define SMD_VERIFY_CORRECTION    2

class ShowMapDataUI : public CWindowParams
{
public:
	ShowMapDataUI( CWindow* parent, std::vector<eMap_point>& verifyMap, std::vector<eMap_point>& correctionMap, int X, int Y, bool defaultMap, bool disableCorrection = false ) : CWindowParams( parent )
	{
		SetStyle( WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
		SetClientAreaSize( 110, 40 );

		verifyData = verifyMap;
		correctionData = correctionMap;
		numX = X;
		numY = Y;
		def_map = defaultMap;

		if( verifyData.size() == correctionData.size() )
		{
			mode = SMD_VERIFY_CORRECTION;
		}
		else
		{
			mode = SMD_VERIFY;
		}

		if( disableCorrection )
		{
			allowCorrect = false;
		}
		else
		{
			if( correctionData.size() == eMap.data.size() && X == eMap.nx && Y == eMap.ny )
				allowCorrect = true;
			else
				allowCorrect = false;
		}
	}

protected:
	void onInit()
	{
	}

	void onShow()
	{
		// draw panel
		RectI _panel( 1, 1, 78, GetH()/GUI_CharH() - 2 );
		DrawPanel( _panel );

		int size_x = _panel.W*GUI_CharW() - 1;
		int size_y = _panel.H*GUI_CharH() - 1;

		// first point
		dPix.X = float(size_x)/numX;
		dPix.Y = float(size_y)/numY;
		fp.X = GetX() + _panel.X*GUI_CharW() + dPix.X/2;
		fp.Y = GetY() + (_panel.Y+_panel.H)*GUI_CharH() - dPix.Y/2;

		refresh();
		makeReport();
		showLegend();
	}

	void onRefresh()
	{
	}

	void onEdit()
	{
	}

	void onShowMenu()
	{
	}

	bool onKeyPress( int key )
	{
		switch( key )
		{
			case K_SHIFT_F1:
				onShowVerify();
				return true;

			case K_SHIFT_F2:
				if( correctionData.size() )
				{
					onShowCorrection();
				}
				return true;

			case K_SHIFT_F3:
				if( verifyData.size() == correctionData.size() )
				{
					onShowVerifyCorrection();
				}
				return true;

			default:
				break;
		}

		return false;
	}

	void onClose()
	{
		if( allowCorrect )
		{
			onCorrect();
		}
	}

private:
	int onShowVerify()
	{
		mode = SMD_VERIFY;
		refresh();
		return 1;
	}

	int onShowCorrection()
	{
		mode = SMD_CORRECTION;
		refresh();
		return 1;
	}

	int onShowVerifyCorrection()
	{
		mode = SMD_VERIFY_CORRECTION;
		refresh();
		return 1;
	}

	int onCorrect()
	{
		if( !W_Deci( 1, MsgGetString(Msg_00119) ) )
		{
			return 0;
		}

		allowCorrect = false;

		// correzione mappatura
		for( unsigned int i = 0; i < correctionData.size(); i++ )
		{
			int pos = correctionData[i].px + correctionData[i].py * eMap.nx;
			if( pos < 0 || pos >= eMap.nx*eMap.ny )
			{
				W_Mess( MsgGetString(Msg_00120) );
				// reload old data
				LoadMapData( MAPDATAFILEXY );
				return 0;
			}

			eMap.data[pos].dx += correctionData[i].dx;
			eMap.data[pos].dy += correctionData[i].dy;
		}

		// save data
		saveMapData( MAPDATAFILEXY, eMap.data, eMap.x, eMap.y, eMap.nx, eMap.ny, eMap.sx, eMap.sy );
		// load new data
		LoadMapData( MAPDATAFILEXY );

		W_Mess( MsgGetString(Msg_00145) );
		return 1;
	}

	void refresh()
	{
		GUI_Freeze_Locker lock;
		unsigned int size;

		//TODO messaggi
		if( mode == SMD_VERIFY )
		{
			SetTitle( "Results: Verify" );
			size = verifyData.size();
		}
		else if( mode == SMD_CORRECTION )
		{
			SetTitle( "Results: Correction" );
			size = correctionData.size();
		}
		else
		{
			SetTitle( "Results: Verify + Correction" );
			//NOTE: in questo caso i due vettori devono avere la stessa lunghezza
			size = verifyData.size();
		}

		for( unsigned int i = 0; i < size; i++ )
		{
			eMap_point err;

			if( mode == SMD_VERIFY )
			{
				err = verifyData[i];
			}
			else if( mode == SMD_CORRECTION )
			{
				err = correctionData[i];
			}
			else
			{
				err = verifyData[i];
				err.dx -= correctionData[i].dx;
				err.dy -= correctionData[i].dy;
			}

			if( def_map && ((err.px < DEF_MAP_GAP_X1 && err.py < DEF_MAP_GAP_Y) || (err.px >= (DEF_MAP_X - DEF_MAP_GAP_X2) && err.py < DEF_MAP_GAP_Y)) )
			{
				// salta punto
				continue;
			}

			PointI p = fp;
			p.X += err.px * dPix.X;
			p.Y -= err.py * dPix.Y;

			GUI_color color;
			float vErr = sqrt((err.dx*err.dx) + (err.dy*err.dy));
			if( vErr < CHECK_ERROR_L1 )
			{
				color = GUI_color(GR_LIGHTGREEN);
			}
			else if( vErr < CHECK_ERROR_L2 )
			{
				color = GUI_color(GR_YELLOW);
			}
			else
			{
				color = GUI_color(GR_RED);
			}

			GUI_FillCircle( p, 3, color );
		}
	}

	void makeReport()
	{
		//accumulator_set< double, stats<tag::min, tag::max, tag::mean, tag::variance, tag::density> > accX( tag::density::num_bins = 10, tag::density::cache_size = 10 );
		accumulator_set< double, stats<tag::min, tag::max, tag::mean, tag::variance> > accX;
		accumulator_set< double, stats<tag::min, tag::max, tag::mean, tag::variance> > accY;

		for( unsigned int i = 0; i < verifyData.size(); i++ )
		{
			//TODO: i calcoli farli usando verifica+correzione o solo verifica ???
			eMap_point err = verifyData[i];

			if( def_map && ((err.px < DEF_MAP_GAP_X1 && err.py < DEF_MAP_GAP_Y) || (err.px >= (DEF_MAP_X - DEF_MAP_GAP_X2) && err.py < DEF_MAP_GAP_Y)) )
			{
				// salta punto
				continue;
			}

			accX( err.dx );
			accY( err.dy );
		}

		double maxX = ( extract::max( accX ) > fabs(extract::min( accX )) ) ? extract::max( accX ) : extract::min( accX );
		double maxY = ( extract::max( accY ) > fabs(extract::min( accY )) ) ? extract::max( accY ) : extract::min( accY );

		GUI_Freeze_Locker lock;

		char buf[120];
		snprintf( buf, sizeof(buf), "%s (mm)", MsgGetString(Msg_01804) );
		DrawTextCentered( 80, 110, 1, buf );
		snprintf( buf, sizeof(buf), "  Max  : %.3f, %.3f   ", maxX, maxY );
		DrawText( 82, 3, buf );
		snprintf( buf, sizeof(buf), "  Mean : %.3f, %.3f   ", extract::mean( accX ), extract::mean( accY ) );
		DrawText( 82, 4, buf );
		snprintf( buf, sizeof(buf), "  Std  : %.3f, %.3f   ", sqrt(extract::variance( accX )), sqrt(extract::variance( accY )) );
		DrawText( 82, 5, buf );
		snprintf( buf, sizeof(buf), "  Std3 : %.3f, %.3f   ", 3*sqrt(extract::variance( accX )), 3*sqrt(extract::variance( accY )) );
		DrawText( 82, 6, buf );


		/*
		// density histogram
		typedef boost::iterator_range<std::vector<std::pair<double, double> >::iterator > histogram_type;

		histogram_type histogram = density( accX );

		for( std::size_t i = 0; i < histogram.size(); ++i )
		{
			cout << "[" << i << "] Bin lower bound: " << histogram[i].first << "\t Value: " << histogram[i].second << endl;
		}
		*/
	}

	void showLegend()
	{
		DrawPanel( RectI( 81, GetH()/GUI_CharH() - 6, GetW()/GUI_CharW() - 83, 5 ) );

		PointI p;
		p.X = GetX() + TextToGraphX( 82 );
		p.Y = GetY() + GetH() - TextToGraphY( 5 ) + 10;
		int txtY = GetH()/GUI_CharH() - 5;
		char buf[100];

		GUI_FillCircle( p, 3, GUI_color(GR_LIGHTGREEN) );
		snprintf(buf, sizeof(buf), "%s <= %.2f", MsgGetString(Msg_01804), CHECK_ERROR_L1 );
		DrawText( 83, txtY, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );

		p.Y += TextToGraphY( 1 );
		txtY++;
		GUI_FillCircle( p, 3, GUI_color(GR_YELLOW) );
		snprintf(buf, sizeof(buf), "%.2f < %s <= %.2f", CHECK_ERROR_L1, MsgGetString(Msg_01804), CHECK_ERROR_L2 );
		DrawText( 83, txtY, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );

		p.Y += TextToGraphY( 1 );
		txtY++;
		GUI_FillCircle( p, 3, GUI_color(GR_RED) );
		snprintf(buf, sizeof(buf), "%s > %.2f", MsgGetString(Msg_01804), CHECK_ERROR_L2 );
		DrawText( 83, txtY, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color( 0, 0, 0 ) );
	}

	PointF dPix;
	PointI fp;
	int numX, numY;
	bool def_map;
	int mode;
	bool allowCorrect;

	std::vector<eMap_point> verifyData;
	std::vector<eMap_point> correctionData;
};



//---------------------------------------------------------------------------
// Ricerca pattern mappatura
// Ritorna: true se trovato, false altrimenti
//---------------------------------------------------------------------------
#define MAP_RETRY					3
#define PATTERN_ACCEPTABLE_AREA		2.0

bool Find_MapPattern( float* x_pos, float* y_pos )
{
	bool miss = true;
	float results[2];

	// la velocita' del primo punto viene decisa dal chiamante
	if( !NozzleXYMove( *x_pos, *y_pos ) )
	{
		return false;
	}

	char NameFile[MAXNPATH];

	// CARICAMENTO IMAGE DATA DA FILE
	SetImageName( NameFile, MAPPING_IMG, DATA );
	if( access(NameFile,0) != 0 )
	{
		char buf[80];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_07029), NameFile );
		W_Mess( buf );

		Wait_PuntaXY();
		return false;
	}

	img_data imgData;
	ImgDataLoad( NameFile, &imgData );

	SetImgBrightCont( imgData.bright, imgData.contrast );

	int atl_dim = 320;

	Wait_PuntaXY();
	delay( Vision.wait_time );


	bitmap* captured = 0;
	int retry = 0;

	float mmpix = (Vision.mmpix_x + Vision.mmpix_y) / 2.f;
	int cDiameter = (Vision.circleDiameter / mmpix) / 100.f;
	int cTolerance = (Vision.circleTolerance / mmpix) / 100.f;

	for( int itera = 0; itera < 3; itera++ )
	{
		if( itera > 0 )
		{
			SetNozzleXYSpeed_Index( Vision.matchSpeedIndex );
			if( !NozzleXYMove( *x_pos, *y_pos ) )
			{
				return false;
			}
			Wait_PuntaXY();
			delay( Vision.wait_time );
		}

		if( captured )
			delete captured;

		//COSTRUTTORE CON CAPTURING DA VGA (ATLANTE)
		captured = new bitmap( atl_dim, atl_dim, getFrameWidth()/2 ,getFrameHeight()/2 );

		if( captured->findMapPattern( results[0], results[1], cDiameter, cTolerance,
				Vision.circleFSmoothDim, Vision.circleFEdgeThr,
				Vision.circleFAccum, mapTestDebug ) == false )
		{
			retry++;
			if( retry >= MAP_RETRY )
			{
				print_debug( "ERROR: findMapPattern\n" );
				break;
			}
			else
			{
				print_debug( "RETRY: findMapPattern\n" );
			}
			continue;
		}

		retry = 0;

		float deltaXpix = results[0]-(atl_dim/2);
		float deltaYpix = results[1]-(atl_dim/2);

		print_debug( "[%d] Position   X = %.2f   Y = %.2f  (%.2f,%.2f)\n", itera+1, deltaXpix, deltaYpix, results[0], results[1] );

		float deltaXmm = deltaXpix * Vision.mmpix_x;
		float deltaYmm = deltaYpix * Vision.mmpix_y;

		if( fabs(deltaXmm) >= Vision.match_err || fabs(deltaYmm) >= Vision.match_err )
		{
			print_debug( "ERROR: Delta mm   X = %.2f   Y = %.2f\n", deltaXmm, deltaYmm );
			break;
		}

		*x_pos += deltaXmm;
		*y_pos += deltaYmm;

		if( fabs(deltaXpix) < PATTERN_ACCEPTABLE_AREA && fabs(deltaYpix) < PATTERN_ACCEPTABLE_AREA )
		{
			print_debug( "OK FIND\n" );
			miss = false;
			break;
		}
	}

	#ifdef BMP_DEBUG
	if( 1 )
	#else
	if( mapTestDebug > 0 )
	#endif
	{
		int errX = results[0]-(atl_dim/2);
		int errY = results[1]-(atl_dim/2);

		Mapping_showDebug( captured, errX, errY );
	}

	delete captured;

	return !miss;
}


//---------------------------------------------------------------------------
// Ricerca componente rettangolare
// Ritorna: true se trovato, false altrimenti
//---------------------------------------------------------------------------
//TODO //TEMP - poi ripristinare il vecchio
bool Find_RectangularComponent( float& x_pos, float& y_pos, float& angle, int line )
//bool Find_RectangularComponent( float& x_pos, float& y_pos, float& angle )
{
	bool miss = true;
	PointF result;

	// la velocita' del primo punto viene decisa dal chiamante
	if( !NozzleXYMove( x_pos, y_pos ) )
	{
		return false;
	}

	char NameFile[MAXNPATH];

	// CARICAMENTO IMAGE DATA DA FILE
	SetImageName( NameFile, RECTCOMP_IMG, DATA );
	if( access(NameFile,0) != 0 )
	{
		char buf[80];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_07029), NameFile );
		W_Mess( buf );

		Wait_PuntaXY();
		return false;
	}

	img_data imgData;
	ImgDataLoad( NameFile, &imgData );

	SetImgBrightCont( imgData.bright, imgData.contrast );

	bitmap* captured = 0;
	int retry = 0;

	float mmpix = (Vision.mmpix_x + Vision.mmpix_y) / 2.f;
	int rectX = (Vision.rectX / mmpix) / 100.f;
	int rectY = (Vision.rectX / mmpix) / 100.f;
	int rectTolerance = (Vision.rectTolerance / mmpix) / 100.f;

	int maxSide = MAX( rectX, rectY );
	int atl_dim = MID( 240, maxSide*2, 500 );


	Wait_PuntaXY();
	delay( Vision.wait_time );


	for( int itera = 0; itera < 3; itera++ )
	{
		if( itera > 0 )
		{
			SetNozzleXYSpeed_Index( Vision.matchSpeedIndex );
			if( !NozzleXYMove( x_pos, y_pos ) )
			{
				return false;
			}
			Wait_PuntaXY();
			delay( Vision.wait_time );
		}

		if( captured )
			delete captured;

		//COSTRUTTORE CON CAPTURING DA VGA (ATLANTE)
		captured = new bitmap( atl_dim, atl_dim, getFrameWidth()/2 ,getFrameHeight()/2 );

		if( captured->findRotatedRectangle( result.X, result.Y, angle, rectX, rectY, rectTolerance, Vision.rectFSmoothDim, Vision.rectFBinThrMin, Vision.rectFBinThrMax, Vision.rectFApprox/100.f, mapTestDebug ) == false )
		{
			retry++;
			if( retry >= MAP_RETRY )
			{
				print_debug( "ERROR: findRotatedRectangle\n" );
				break;
			}
			else
			{
				print_debug( "RETRY: findRotatedRectangle\n" );
			}
			continue;
		}

		retry = 0;

		float deltaXpix = result.X-(atl_dim/2);
		float deltaYpix = result.Y-(atl_dim/2);

		//print_debug( "[%d] Position   X = %.2f   Y = %.2f  (%.2f,%.2f)\n", itera+1, deltaXpix, deltaYpix, results[0], results[1] );

		float deltaXmm = deltaXpix * Vision.mmpix_x;
		float deltaYmm = deltaYpix * Vision.mmpix_y;


		//TODO //TEMP - solo per test, poi togliere
		//Salva immagine
		if( (fabs(deltaXmm) > 0.04 || fabs(deltaYmm) > 0.04) && itera == 0 )
		{
			char nf[256];
			snprintf( nf, sizeof(nf), "rc_%03d.png", line );
			captured->save( nf );
		}


		if( fabs(deltaXmm) >= Vision.match_err || fabs(deltaYmm) >= Vision.match_err )
		{
			print_debug( "ERROR: Delta mm   X = %.2f   Y = %.2f\n", deltaXmm, deltaYmm );
			break;
		}

		x_pos += deltaXmm;
		y_pos += deltaYmm;

		if( fabs(deltaXpix) < PATTERN_ACCEPTABLE_AREA && fabs(deltaYpix) < PATTERN_ACCEPTABLE_AREA )
		{
			//print_debug( "OK FIND\n" );
			miss = false;
			break;
		}
	}

	#ifdef BMP_DEBUG
	if( 1 )
	#else
	if( mapTestDebug > 0 )
	#endif
	{
		int errX = result.X-(atl_dim/2);
		int errY = result.Y-(atl_dim/2);

		Mapping_showDebug( captured, errX, errY );
	}

	delete captured;

	return !miss;
}


//---------------------------------------------------------------------------
// Ricerca cerchio (inchiostro)
// Ritorna: true se trovato, false altrimenti
//---------------------------------------------------------------------------
bool Find_InkMark( float& x_pos, float& y_pos )
{
	bool miss = true;
	PointF result;

	// la velocita' del primo punto viene decisa dal chiamante
	if( !NozzleXYMove( x_pos, y_pos ) )
	{
		return false;
	}

	char NameFile[MAXNPATH];

	// CARICAMENTO IMAGE DATA DA FILE
	SetImageName( NameFile, INKMARK_IMG, DATA );
	if( access(NameFile,0) != 0 )
	{
		char buf[80];
		snprintf( buf, sizeof(buf), MsgGetString(Msg_07029), NameFile );
		W_Mess( buf );

		Wait_PuntaXY();
		return false;
	}

	img_data imgData;
	ImgDataLoad( NameFile, &imgData );

	SetImgBrightCont( imgData.bright, imgData.contrast );

	bitmap* captured = 0;
	int retry = 0;

	float mmpix = (Vision.mmpix_x + Vision.mmpix_y) / 2.f;
	unsigned short circleDiameter = (Vision.circleDiameter / mmpix) / 100.f;
	unsigned short circleTolerance = (Vision.circleTolerance / mmpix) / 100.f;

	int atl_dim = MID( 240, circleDiameter*2, 500 );


	Wait_PuntaXY();
	delay( Vision.wait_time );


	for( int itera = 0; itera < 3; itera++ )
	{
		if( itera > 0 )
		{
			SetNozzleXYSpeed_Index( Vision.matchSpeedIndex );
			if( !NozzleXYMove( x_pos, y_pos ) )
			{
				return false;
			}
			Wait_PuntaXY();
			delay( Vision.wait_time );
		}

		if( captured )
			delete captured;

		//COSTRUTTORE CON CAPTURING DA VGA (ATLANTE)
		captured = new bitmap( atl_dim, atl_dim, getFrameWidth()/2 ,getFrameHeight()/2 );

		if( captured->findCircle( result.X, result.Y, circleDiameter, circleTolerance, Vision.circleFSmoothDim, Vision.circleFEdgeThr, Vision.circleFAccum, mapTestDebug ) == false )
		{
			retry++;
			if( retry >= MAP_RETRY )
			{
				print_debug( "ERROR: findCircle\n" );
				break;
			}
			else
			{
				print_debug( "RETRY: findCircle\n" );
			}
			continue;
		}

		retry = 0;

		float deltaXpix = result.X-(atl_dim/2);
		float deltaYpix = result.Y-(atl_dim/2);

		float deltaXmm = deltaXpix * Vision.mmpix_x;
		float deltaYmm = deltaYpix * Vision.mmpix_y;

		if( fabs(deltaXmm) >= Vision.match_err || fabs(deltaYmm) >= Vision.match_err )
		{
			print_debug( "ERROR: Delta mm   X = %.2f   Y = %.2f\n", deltaXmm, deltaYmm );
			break;
		}

		x_pos += deltaXmm;
		y_pos += deltaYmm;

		if( fabs(deltaXpix) < PATTERN_ACCEPTABLE_AREA && fabs(deltaYpix) < PATTERN_ACCEPTABLE_AREA )
		{
			//print_debug( "OK FIND\n" );
			miss = false;
			break;
		}
	}

	#ifdef BMP_DEBUG
	if( 1 )
	#else
	if( mapTestDebug > 0 )
	#endif
	{
		int errX = result.X-(atl_dim/2);
		int errY = result.Y-(atl_dim/2);

		Mapping_showDebug( captured, errX, errY );
	}

	delete captured;

	return !miss;
}


//---------------------------------------------------------------------------
// TeachTestPosition: apprendimento posizione di test
//---------------------------------------------------------------------------
int Test_TeachPosition( int image_type )
{
	// load image data
	img_data imgData;
	char filename[MAXNPATH];

	SetImageName( filename, image_type, DATA );
	if( access(filename,0) == 0 )
	{
		ImgDataLoad( filename, &imgData );

		// setta parametri immagine
		SetImgBrightCont( imgData.bright, imgData.contrast);
	}

	PointF pos( mapTestX, mapTestY );

	if( ManualTeaching( &pos.X, &pos.Y, MsgGetString(Msg_00823) ) )
	{
		mapTestX = pos.X;
		mapTestY = pos.Y;

		// salva parametri immagine
		imgData.bright = GetImageBright();
		imgData.contrast = GetImageContrast();

		ImgDataSave( filename, &imgData );
		return 1;
	}

	return 0;
}

//---------------------------------------------------------------------------
// Esegue test riconoscimento pattern mappatura
//---------------------------------------------------------------------------
int Test_MapPattern()
{
	if( Get_OnFile() )
		return 1;

	PointF pos( mapTestX, mapTestY );

	if( !Check_XYMove( pos.X, pos.Y ) )
	{
		W_Mess( MsgGetString(Msg_00172) );
		return 0;
	}

	//svuota buffer
	flushKeyboardBuffer();

	EnableForceFinec(ON); // forza check finecorsa ad on impedendo cambiamenti

	// led camera on
	Set_HeadCameraLight( 1 );
	set_currentcam( CAMERA_HEAD );

	SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

	// enable debug 1 (test results)
	int oldValue = mapTestDebug;
	mapTestDebug |= 1;

	Find_MapPattern( &pos.X, &pos.Y );

	mapTestDebug = oldValue;

	// led camera off
	Set_HeadCameraLight( 0 );

	DisableForceFinec();
	Set_Finec(OFF);

	return 1;
}

//---------------------------------------------------------------------------
// Esegue test riconoscimento componente
//---------------------------------------------------------------------------
int Test_RectangularComponent()
{
	if( Get_OnFile() )
		return 1;

	PointF pos( mapTestX, mapTestY );
	float angle = 0.f;

	if( !Check_XYMove( pos.X, pos.Y ) )
	{
		W_Mess( MsgGetString(Msg_00172) );
		return 0;
	}

	//svuota buffer
	flushKeyboardBuffer();

	EnableForceFinec(ON); // forza check finecorsa ad on impedendo cambiamenti

	// led camera on
	Set_HeadCameraLight( mapTestLight );
	set_currentcam( CAMERA_HEAD );

	SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

	// enable debug 1 (test results)
	int oldValue = mapTestDebug;
	mapTestDebug |= 1;

	Find_RectangularComponent( pos.X, pos.Y, angle, 0 );

	mapTestDebug = oldValue;

	// led camera off
	Set_HeadCameraLight( 0 );

	DisableForceFinec();
	Set_Finec(OFF);

	return 1;
}

//---------------------------------------------------------------------------
// Esegue test riconoscimento inchiostro
//---------------------------------------------------------------------------
int Test_InkMark()
{
	if( Get_OnFile() )
		return 1;

	PointF pos( mapTestX, mapTestY );

	if( !Check_XYMove( pos.X, pos.Y ) )
	{
		W_Mess( MsgGetString(Msg_00172) );
		return 0;
	}

	//svuota buffer
	flushKeyboardBuffer();

	EnableForceFinec(ON); // forza check finecorsa ad on impedendo cambiamenti

	// led camera on
	Set_HeadCameraLight( 1 );
	set_currentcam( CAMERA_HEAD );

	SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

	// enable debug 1 (test results)
	int oldValue = mapTestDebug;
	mapTestDebug |= 1;

	Find_InkMark( pos.X, pos.Y );

	mapTestDebug = oldValue;

	// led camera off
	Set_HeadCameraLight( 0 );

	DisableForceFinec();
	Set_Finec(OFF);

	return 1;
}



//------------------------------------------------------------------------------------------
// Resetta dati mappatura
//------------------------------------------------------------------------------------------
void ClearMapData()
{
	eMap.data.clear();
	eMap.x = eMap.y = 0.f;
	eMap.nx = eMap.ny = 0;
	eMap.sx = eMap.sy = 0.f;
}

//------------------------------------------------------------------------------------------
// Salva dati mappatura
//------------------------------------------------------------------------------------------
bool saveMapData( const char* filename, vector<eMap_point>& data, float posX, float posY, int numX, int numY, float sX, float sY )
{
	FILE* pFile = fopen( filename, "wt" );
	if( pFile == NULL )
	{
		char buf[256];
		sprintf( buf, MsgGetString(Msg_05056), filename );
		W_Mess( buf );
		return false;
	}

	// map position start
	fprintf( pFile, "%.3f;%.3f;", posX, posY );

	// map grid dimemsion
	fprintf( pFile, "%d;%d;", numX, numY );

	// map grid delta
	fprintf( pFile, "%.3f;%.3f\n", sX, sY );

	// map errors
	for( unsigned int i = 0; i < data.size(); i++ )
	{
		fprintf( pFile, "%.3f;%.3f;%d;%d\n", data[i].dx, data[i].dy, data[i].px, data[i].py );
	}

	fclose( pFile );
	return true;
}

//------------------------------------------------------------------------------------------
// Carica dati mappatura
//------------------------------------------------------------------------------------------
#define SEARCH_NEXT_VAL(p)		p = strchr( p, ';' ); if( p ) { p++; } else { fclose( pFile );	return false; }

bool loadMapData( const char* filename, eMapStruct& mapdata )
{
	FILE* pFile = fopen( filename, "rt" );
	if( pFile == NULL )
	{
		char buf[256];
		sprintf( buf, MsgGetString(Msg_05056), filename );
		W_Mess( buf );
		return false;
	}

	char mystring[100];

	// read first line
	if( fgets( mystring, sizeof(mystring), pFile ) == 0 )
	{
		fclose( pFile );
		return false;
	}
	char* p = mystring;

	// position X
	mapdata.x = atof( p );
	// position Y
	SEARCH_NEXT_VAL( p );
	mapdata.y = atof( p );

	// dimension X
	SEARCH_NEXT_VAL( p );
	mapdata.nx = atoi( p );
	// dimension Y
	SEARCH_NEXT_VAL( p );
	mapdata.ny = atoi( p );

	// step X
	SEARCH_NEXT_VAL( p );
	mapdata.sx = atof( p );
	// step Y
	SEARCH_NEXT_VAL( p );
	mapdata.sy = atof( p );


	// check data
	if( mapdata.nx <= 0 || mapdata.nx > 10000 || mapdata.ny <= 0 || mapdata.ny > 10000 )
	{
		fclose( pFile );
		return false;
	}

	// data
	mapdata.data.resize( mapdata.nx*mapdata.ny );

	while( !feof( pFile ) )
	{
		if( fgets( mystring, 100, pFile ) == 0 )
		{
			break;
		}
		p = mystring;

		eMap_point ePoint;

		// point error X
		ePoint.dx = atof( p );
		// point error Y
		SEARCH_NEXT_VAL( p );
		ePoint.dy = atof( p );

		// point position X
		SEARCH_NEXT_VAL( p );
		ePoint.px = atoi( p );
		// point position Y
		SEARCH_NEXT_VAL( p );
		ePoint.py = atoi( p );

		int pos = ePoint.px + ePoint.py * mapdata.nx;
		if( pos < 0 || pos >= mapdata.nx*mapdata.ny )
		{
			fclose( pFile );
			return false;
		}

		mapdata.data[pos] = ePoint;
	}

	fclose( pFile );
	return true;
}


bool LoadMapData( const char* filename )
{
	// erase previous data
	ClearMapData();

	if( !loadMapData( filename, eMap ) )
	{
		ClearMapData();
		W_Mess( MsgGetString(Msg_00765) ); // map params corrupted or not exist
		return false;
	}

	return true;
}


//------------------------------------------------------------------------------------------
// Apprende punto origine calibrazioni
//------------------------------------------------------------------------------------------
int Errormap_OriginTeach()
{
	if( Get_OnFile() )
		return 1;

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	return ManualTeaching( &Vision.pos_x, &Vision.pos_y, MsgGetString(Msg_00099), AUTOAPP_MAPOFF );
	{
		return 0;
	}

	VisDataSave( Vision );
	return 1;
}

//------------------------------------------------------------------------------------------
// Procedura mappatura piano: avanzamento Y (dir = 0) o X (dir = 1)
//------------------------------------------------------------------------------------------
int Errormap_Create( int dir )
{
	if( Get_OnFile() )
		return 1;

	if( !W_Deci( 0, MsgGetString(Msg_01256) ) )
		return 1;

	int npointx = Vision.mapnum_x;
	int npointy = Vision.mapnum_y;

	float offsetx = Vision.mapoff_x;
	float offsety = Vision.mapoff_y;

	PointF orig( Vision.pos_x, Vision.pos_y );
	PointF ref;

	bool def_map = false;

	if( npointx == 0 || npointy == 0 )
	{
		// abilita mappatura default
		npointx = DEF_MAP_X;
		npointy = DEF_MAP_Y;

		offsetx = DEF_MAP_OFFSET_MM;
		offsety = DEF_MAP_OFFSET_MM;

		def_map = true;
	}

	// forza check finecorsa ad ON impedendo cambiamenti
	EnableForceFinec(ON);

	// disattiva mappatura
	use_mapping_correction = false;

	//svuota buffer
	flushKeyboardBuffer();


	// load image data
	char filename[MAXNPATH];
	SetImageName( filename, MAPPING_IMG, DATA );
	if( access(filename,0) == 0 )
	{
		img_data imgData;
		ImgDataLoad( filename, &imgData );

		// setta parametri immagine
		SetImgBrightCont( imgData.bright, imgData.contrast );
	}


	int abort = 0;
	while( !abort )
	{
		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

		// apprende punto di riferimento 1
		char titolo[80];
		snprintf( titolo, 80, MsgGetString(Msg_01971), 1 );
		if( !ManualTeaching( &orig.X, &orig.Y, titolo, 59, AUTOAPP_MAPOFF ) )
		{
			abort = 1;
		}
		else
		{
			// controlla raggiungibilita' area mappatura
			bool pos_ok = false;

			if( def_map )
			{
				// punto iniziale basso sx
				ref.X = orig.X;
				ref.Y = orig.Y - offsety * DEF_MAP_GAP_Y;

				pos_ok = Check_XYMove( ref.X, ref.Y );

				if( pos_ok )
				{
					// punto finale alto dx
					ref.X = orig.X + offsetx * ( npointx - 1 );
					ref.Y = orig.Y + offsety * ( npointy - DEF_MAP_GAP_Y - 1 );

					pos_ok = Check_XYMove( ref.X, ref.Y );
				}
			}
			else
			{
				// punto finale alto dx
				ref.X = orig.X + offsetx * ( npointx - 1 );
				ref.Y = orig.Y + offsety * ( npointy - 1 );

				pos_ok = Check_XYMove( ref.X, ref.Y );
			}

			if( pos_ok )
			{
				break;
			}

			W_Mess( MsgGetString(Msg_00172) );
		}
	}

	if( abort )
	{
		// restore def acc/speed
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

		// attiva mappatura
		use_mapping_correction = true;

		DisableForceFinec();
		Set_Finec(OFF);
		return 0;
	}

	vector<eMap_point> errorMap;
	errorMap.resize( npointx*npointy );

	// save starting point
	Vision.pos_x = orig.X;
	Vision.pos_y = orig.Y;
	VisDataSave( Vision );

	// wait window
	CWindow* wait_window = new CWindow( 0 );
	wait_window->SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
	wait_window->SetClientAreaSize( 50, 7 );
	//
	wait_window->Show();
	wait_window->DrawTextCentered( 1, MsgGetString(Msg_05043) );
	wait_window->DrawTextCentered( 2, MsgGetString(Msg_05044) );
	GUI_ProgressBar_OLD* progress_bar = new GUI_ProgressBar_OLD( wait_window, 2, 4, 46, 1, (npointx * npointy) );

	// led camera on
	Set_HeadCameraLight( 1 );
	set_currentcam( CAMERA_HEAD );
	Set_Tv_Title( "" );

	// ciclo mappatura
	//--------------------------------------------------------------------------
	PointF pos, found;

	int x = 0;
	int y = 0;
	int teached_points = 0;
	int failed_points = 0;
	PointF _found;

	for( int np = 0; np < errorMap.size(); np++ )
	{
		if( Esc_Press() )
		{
			if( W_Deci(0,INTERRUPTMSG) )
			{
				abort = 1;
				break;
			}
		}


		// controlla punto
		//--------------------------------------------------------------------------

		bool skip = false;
		bool ask_user = false;

		if( def_map && ((x < DEF_MAP_GAP_X1 && y < DEF_MAP_GAP_Y) || (x >= (DEF_MAP_X - DEF_MAP_GAP_X2) && y < DEF_MAP_GAP_Y)) )
		{
			// salta punto
			skip = true;
		}
		else
		{
			// posizione punto
			//--------------------------------------------------------------------------

			if( def_map )
			{
				// posizione teorica
				pos.X = orig.X + x*offsetx;
				pos.Y = orig.Y + (y-DEF_MAP_GAP_Y)*offsety;

				found = pos;

				// aggiunge correzione punto precedente
				if( x < DEF_MAP_GAP_X1 || x >= (DEF_MAP_X - DEF_MAP_GAP_X2) )
				{
					if( y != DEF_MAP_GAP_Y ) // e  x qualsiasi
					{
						// punto precedente
						found.X += errorMap[x*npointy+(y-1)].dx;
						found.Y += errorMap[x*npointy+(y-1)].dy;
					}
					else if( x != 0 ) // e y = DEF_MAP_GAP_Y
					{
						// punto adiacente nella linea precedente
						found.X += errorMap[(x-1)*npointy+y].dx;
						found.Y += errorMap[(x-1)*npointy+y].dy;
					}
				}
				else
				{
					if( y != 0 ) // e  x qualsiasi
					{
						found.X += errorMap[x*npointy+(y-1)].dx;
						found.Y += errorMap[x*npointy+(y-1)].dy;
					}
					else if( x != 0 ) // e y = 0
					{
						found.X += errorMap[(x-1)*npointy].dx;
						found.Y += errorMap[(x-1)*npointy].dy;
					}
				}
			}
			else
			{
				// posizione teorica
				pos.X = orig.X + x*offsetx;
				pos.Y = orig.Y + y*offsety;

				found = pos;

				// aggiunge correzione punto precedente
				if( y != 0 ) // e  x qualsiasi
				{
					// punto precedente
					found.X += errorMap[x*npointy+(y-1)].dx;
					found.Y += errorMap[x*npointy+(y-1)].dy;
				}
				else if( x != 0 ) // e y = 0
				{
					// punto adiacente nella linea precedente
					found.X += errorMap[(x-1)*npointy].dx;
					found.Y += errorMap[(x-1)*npointy].dy;
				}
			}


			// ricerca pattern
			//--------------------------------------------------------------------------

			// salva poszione teorica con correzione del punto precedente
			_found = found;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( Find_MapPattern( &found.X, &found.Y ) == false )
			{
				// solo per il primo punto chiede apprendimento manuale se necessario
				if( teached_points == 0 )
				{
					if(W_Deci(1,MAPERROR))
					{
						// ripristina posizione
						found = _found;

						if(!ManualTeaching( &found.X, &found.Y, MsgGetString(Msg_00811) ))
						{
							abort=1;
							break;
						}

						Set_HeadCameraLight( 1 );
						set_currentcam( CAMERA_HEAD );
						Set_Tv_Title( "" );
					}
					else
					{
						abort = 1;
						break;
					}
				}
				else
				{
					ask_user = true;
					failed_points++;

					char buf[80];
					snprintf( buf, sizeof(buf), "Failed: %d", failed_points );
					wait_window->DrawText( 4, 5, buf );
				}
			}

			teached_points++;
		}

		progress_bar->Increment(1);


		// salvataggio dati errore punto
		//--------------------------------------------------------------------------

		eMap_point _err;
		if( skip  )
		{
			_err.dx = 0;
			_err.dy = 0;
			_err.s = 1;
		}
		else if( teached_points == 1 )
		{
			// definisce il punto di riferimento
			_err.dx = 0;
			_err.dy = 0;
			_err.s = 1;

			orig = found;
		}
		else if( ask_user )
		{
			_err.dx = _found.X - pos.X;
			_err.dy = _found.Y - pos.Y;
			_err.s = 0;
		}
		else
		{
			_err.dx = found.X - pos.X;
			_err.dy = found.Y - pos.Y;
			_err.s = 1;
		}

		_err.px = x;
		_err.py = y;

		errorMap[x*npointy+y] = _err;


		// avanza movimento
		//--------------------------------------------------------------------------

		if( dir == 0 )
		{
			// avanza lungo Y
			if( y < npointy - 1 )
				y++;
			else
			{
				y = 0;
				x++;
			}
		}
		else
		{
			// avanza lungo X
			if( x < npointx - 1 )
				x++;
			else
			{
				x = 0;
				y++;
			}
		}
	}


	// correzione manuale punti falliti
	//--------------------------------------------------------------------------
	if( !abort )
	{
		for( int np = 0; np < errorMap.size(); np++ )
		{
			// controlla punto
			//--------------------------------------------------------------------------
			if( errorMap[np].s == 1 )
			{
				continue;
			}

			// posizione punto
			//--------------------------------------------------------------------------
			if( def_map )
			{
				// posizione teorica
				pos.X = orig.X + errorMap[np].px*offsetx;
				pos.Y = orig.Y + (errorMap[np].py-DEF_MAP_GAP_Y)*offsety;
			}
			else
			{
				// posizione teorica
				pos.X = orig.X + errorMap[np].px*offsetx;
				pos.Y = orig.Y + errorMap[np].py*offsety;
			}

			found = pos;

			// aggiunge correzione punto precedente
			found.X += errorMap[np].dx;
			found.Y += errorMap[np].dy;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( !ManualTeaching( &found.X, &found.Y, MsgGetString(Msg_00811) ) )
			{
				abort = 1;
				break;
			}

			Set_HeadCameraLight( 1 );
			set_currentcam( CAMERA_HEAD );
			Set_Tv_Title( "" );

			// salvataggio dati errore punto
			//--------------------------------------------------------------------------
			errorMap[np].dx = found.X - pos.X;
			errorMap[np].dy = found.Y - pos.Y;
			errorMap[np].s = 1;
		}
	}


	delete progress_bar;
	delete wait_window;

	// led camera off
	Set_HeadCameraLight( 0 );

	// attiva mappatura
	use_mapping_correction = true;

	DisableForceFinec();
	Set_Finec(OFF);

	if( abort )
	{
		W_Mess( MsgGetString(Msg_00767) );
		return 0;
	}

	if( def_map )
	{
		// si lasciano le zone non mappate (blocco porta ugelli, telecamera esterna) con valori nulli

		// si trasla l'origine in basso
		orig.Y -= offsety * DEF_MAP_GAP_Y;
	}

	// save data
	saveMapData( MAPDATAFILEXY, errorMap, orig.X, orig.Y, npointx, npointy, offsetx, offsety );
	// load new data
	LoadMapData( MAPDATAFILEXY );
	// restore def acc/speed
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	W_Mess( MsgGetString(Msg_00178) );
	return 1;
}


//----------------------------------------------------------------------------------
// Calcola rotazione piano di mappatura
//----------------------------------------------------------------------------------
int Errormap_GetRotation( PointF& p1, PointF& p2, float& rot )
{
	bool abort = false;
	PointF pF[2];

	// led camera on
	Set_HeadCameraLight( 1 );
	set_currentcam( CAMERA_HEAD );

	for( int i = 0; i < 2; i++ )
	{
		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

		pF[i] = (i == 0) ? p1 : p2;
		if( Find_MapPattern( &pF[i].X, &pF[i].Y ) == false )
		{
			if( W_Deci(1,MAPERROR) )
			{
				pF[i] = (i == 0) ? p1 : p2;
				if( !ManualTeaching( &pF[i].X, &pF[i].Y, MsgGetString(Msg_00153) ) )
				{
					abort = true;
					break;
				}
			}
			else
			{
				abort = true;
				break;
			}
		}
	}

	// led camera off
	Set_HeadCameraLight( 0 );

	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	if( abort )
	{
		return 0;
	}

	// angolo teorico
	float a_theorical = atan2( p2.Y - p1.Y, p2.X - p1.X );
	// angolo reale
	float a_real = atan2( pF[1].Y - pF[0].Y, pF[1].X - pF[0].X );
	// calcolo rotazione
	rot = a_real - a_theorical;

	p1 = pF[0];
	p2 = pF[1];

	return 1;
}


//----------------------------------------------------------------------------------
// Procedura di correzione errore punto
//   Esegue 4 misurazioni (destra, alto, sinistra, basso) e calcola la media
// Ritorna: 0 errore
//          1 procedura effettuata
//          2 procedura non effettuata
//----------------------------------------------------------------------------------
#define CORRECTION_DELTA_MOVEMENT_MM       20.f

int Errormap_PointCorrection( const PointF& pos, PointF& correction )
{
	int teached_points = 0;
	PointF found[4];
	PointF meanValue;

	for( int np = 0; np < 4; np++ )
	{
		if( Esc_Press() )
		{
			if( W_Deci(0,INTERRUPTMSG) )
			{
				return 0;
			}
		}

		found[np] = pos;
		PointF posFrom = pos;

		if( np == 0 )
		{
			// movimento X positivo
			posFrom.X -= CORRECTION_DELTA_MOVEMENT_MM;
		}
		else if( np == 1 )
		{
			// movimento Y negativo
			posFrom.Y += CORRECTION_DELTA_MOVEMENT_MM;
		}
		else if( np == 2 )
		{
			// movimento X negativo
			posFrom.X += CORRECTION_DELTA_MOVEMENT_MM;
		}
		else
		{
			// movimento Y positivo
			posFrom.Y -= CORRECTION_DELTA_MOVEMENT_MM;
		}


		// controlla raggiungibilita' punto
		//--------------------------------------------------------------------------

		if( Check_XYMove( posFrom.X, posFrom.Y ) )
		{
			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( !NozzleXYMove( posFrom.X, posFrom.Y ) )
			{
				return 0;
			}
			if( !Wait_PuntaXY() )
			{
				return 0;
			}


			if( Find_MapPattern( &found[np].X, &found[np].Y ) == false )
			{
				return 2;
			}

			teached_points++;
			meanValue.X += found[np].X;
			meanValue.Y += found[np].Y;
		}
	}

	if( teached_points )
	{
		meanValue.X /= teached_points;
		meanValue.Y /= teached_points;

		correction.X = meanValue.X - pos.X;
		correction.Y = meanValue.Y - pos.Y;

		return 1;
	}

	return 0;
}


//----------------------------------------------------------------------------------
// Correzione mappatura piano: avanzamento Y (dir = 0) o X (dir = 1)
//----------------------------------------------------------------------------------
int Errormap_Check_Correction( int dir )
{
	if( Get_OnFile() )
		return 1;

	int npointx = Vision.mapnum_x;
	int npointy = Vision.mapnum_y;

	float offsetx = Vision.mapoff_x;
	float offsety = Vision.mapoff_y;

	PointF orig( Vision.pos_x, Vision.pos_y );
	PointF ref;

	bool def_map = false;

	if( npointx == 0 || npointy == 0 )
	{
		// abilita mappatura default
		npointx = DEF_MAP_X;
		npointy = DEF_MAP_Y;

		offsetx = DEF_MAP_OFFSET_MM;
		offsety = DEF_MAP_OFFSET_MM;

		def_map = true;
	}

	EnableForceFinec(ON); // forza check finecorsa ad on impedendo cambiamenti

	//svuota buffer
	flushKeyboardBuffer();


	// load image data
	char filename[MAXNPATH];
	SetImageName( filename, MAPPING_IMG, DATA );
	if( access(filename,0) == 0 )
	{
		img_data imgData;
		ImgDataLoad( filename, &imgData );

		// setta parametri immagine
		SetImgBrightCont( imgData.bright, imgData.contrast );
	}


	int abort = 0;
	while( !abort )
	{
		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

		// apprende punto di riferimento 1
		char titolo[80];
		snprintf( titolo, 80, MsgGetString(Msg_01971), 1 );
		if( !ManualTeaching( &orig.X, &orig.Y, titolo, 59, AUTOAPP_MAPOFF ) )
		{
			abort = 1;
		}
		else
		{
			// controlla raggiungibilita' area mappatura
			bool pos_ok = false;

			if( def_map )
			{
				// punto iniziale basso sx
				ref.X = orig.X;
				ref.Y = orig.Y - offsety * DEF_MAP_GAP_Y;

				pos_ok = Check_XYMove( ref.X, ref.Y );

				if( pos_ok )
				{
					// punto finale alto dx
					ref.X = orig.X + offsetx * ( npointx - 1 );
					ref.Y = orig.Y + offsety * ( npointy - DEF_MAP_GAP_Y - 1 );

					pos_ok = Check_XYMove( ref.X, ref.Y );
				}
			}
			else
			{
				// punto finale alto dx
				ref.X = orig.X + offsetx * ( npointx - 1 );
				ref.Y = orig.Y + offsety * ( npointy - 1 );

				pos_ok = Check_XYMove( ref.X, ref.Y );
			}

			if( pos_ok )
			{
				break;
			}

			W_Mess( MsgGetString(Msg_00172) );
		}
	}

	if( abort )
	{
		// restore def acc/speed
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

		DisableForceFinec();
		Set_Finec(OFF);
		return 0;
	}


	// calcolo rotazione tra origine (basso sx) e punto finale (alto dx)
	float da = 0.f;
	if( !Errormap_GetRotation( orig, ref, da ) )
	{
		// restore def acc/speed
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

		DisableForceFinec();
		Set_Finec(OFF);
		return 0;
	}


	// conferma utilizzo rotazione scheda
	char buf[120];
	snprintf( buf, sizeof(buf), MsgGetString(Msg_00225), da*180.f/PI );
	if( !W_Deci( 1, buf ) )
	{
		da = 0.f;
	}

	vector<eMap_point> verifyMap;
	vector<eMap_point> correctionMap;

	CWindow* wait_window = new CWindow( 0 );
	wait_window->SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
	wait_window->SetClientAreaSize( 50, 7 );
	//
	wait_window->Show();
	wait_window->DrawTextCentered( 1, MsgGetString(Msg_05045) );
	wait_window->DrawTextCentered( 2, MsgGetString(Msg_05044) );
	GUI_ProgressBar_OLD* progress_bar = new GUI_ProgressBar_OLD( wait_window, 2, 4, 46, 1, (npointx * npointy) );

	// led camera on
	Set_HeadCameraLight( 1 );
	set_currentcam( CAMERA_HEAD );
	Set_Tv_Title( MsgGetString(Msg_01196) );

	// ciclo mappatura
	//--------------------------------------------------------------------------
	PointF pos, found;

	int x = 0;
	int y = 0;
	int teached_points = 0;
	int failed_points = 0;
	for( int np = 0; np < npointx*npointy; np++ )
	{
		if( Esc_Press() )
		{
			if( W_Deci(0,INTERRUPTMSG) )
			{
				abort = 1;
				break;
			}
		}


		// controlla punto
		//--------------------------------------------------------------------------

		bool skip = false;
		bool ask_user = false;

		if( def_map && ((x < DEF_MAP_GAP_X1 && y < DEF_MAP_GAP_Y) || (x >= (DEF_MAP_X - DEF_MAP_GAP_X2) && y < DEF_MAP_GAP_Y)) )
		{
			// salta punto
			skip = true;
		}
		else
		{
			// posizione punto
			//--------------------------------------------------------------------------

			if( def_map )
			{
				pos.X = x*offsetx;
				pos.Y = (y-DEF_MAP_GAP_Y)*offsety;
			}
			else
			{
				pos.X = x*offsetx;
				pos.Y = y*offsety;
			}

			// correzione rotazione
			PointF segment( pos );
			pos.X = orig.X + (segment.X * cos(da) - segment.Y * sin(da));
			pos.Y = orig.Y + (segment.X * sin(da) + segment.Y * cos(da));

			found = pos;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( Find_MapPattern( &found.X, &found.Y ) == false )
			{
				// solo per il primo punto chiede apprendimento manuale se necessario
				if( teached_points == 0 )
				{
					if( W_Deci(1,MAPERROR) )
					{
						found = pos;

						if( !ManualTeaching( &found.X, &found.Y, MsgGetString(Msg_00811) ) )
						{
							abort = 1;
							break;
						}

						Set_HeadCameraLight( 1 );
						set_currentcam( CAMERA_HEAD );
						Set_Tv_Title( "" );
					}
					else
					{
						abort = 1;
						break;
					}
				}
				else
				{
					ask_user = true;
					failed_points++;

					char buf[80];
					snprintf( buf, sizeof(buf), "Failed: %d", failed_points );
					wait_window->DrawText( 4, 5, buf );
				}
			}

			teached_points++;
		}

		progress_bar->Increment(1);


		// salvataggio dati errore punto
		//--------------------------------------------------------------------------

		eMap_point _err;
		if( skip || ask_user )
		{
			_err.dx = 0;
			_err.dy = 0;
			_err.s = ask_user ? 0 : 1;
		}
		else if( teached_points == 1 )
		{
			// definisce il punto di riferimento
			_err.dx = 0;
			_err.dy = 0;
			_err.s = 1;

			orig = found;
		}
		else
		{
			_err.dx = found.X - pos.X;
			_err.dy = found.Y - pos.Y;
			_err.s = 1;
		}

		_err.px = x;
		_err.py = y;

		verifyMap.push_back( _err );


		// correzione errore
		//--------------------------------------------------------------------------
		float vErr = sqrt((_err.dx*_err.dx) + (_err.dy*_err.dy));
		if( vErr > CHECK_ERROR_MAX )
		{
			PointF correction;
			int ret = Errormap_PointCorrection( pos, correction );
			if( ret == 0 )
			{
				abort = 1;
				break;
			}
			else if( ret == 2 )
			{
				// nessuna correzione
				_err.dx = 0;
				_err.dy = 0;

				// segnala punto fallito
				verifyMap[verifyMap.size()-1].s = 0;
				failed_points++;

				char buf[80];
				snprintf( buf, sizeof(buf), "Failed: %d", failed_points );
				wait_window->DrawText( 4, 5, buf );
			}
			else
			{
				_err.dx = correction.X;
				_err.dy = correction.Y;
			}
		}
		else
		{
			// nessuna correzione
			_err.dx = 0;
			_err.dy = 0;
		}

		correctionMap.push_back( _err );


		if( dir == 0 )
		{
			// avanza lungo Y
			if( y < npointy - 1 )
				y++;
			else
			{
				y = 0;
				x++;
			}
		}
		else
		{
			// avanza lungo X
			if( x < npointx - 1 )
				x++;
			else
			{
				x = 0;
				y++;
			}
		}
	}


	// correzione manuale punti falliti
	//--------------------------------------------------------------------------
	if( !abort )
	{
		for( int np = 0; np < verifyMap.size(); np++ )
		{
			// controlla punto
			//--------------------------------------------------------------------------
			if( verifyMap[np].s == 1 )
			{
				continue;
			}

			// posizione punto
			//--------------------------------------------------------------------------
			if( def_map )
			{
				pos.X = verifyMap[np].px*offsetx;
				pos.Y = (verifyMap[np].py-DEF_MAP_GAP_Y)*offsety;
			}
			else
			{
				pos.X = verifyMap[np].px*offsetx;
				pos.Y = verifyMap[np].py*offsety;
			}

			// correzione rotazione
			PointF segment( pos );
			pos.X = orig.X + (segment.X * cos(da) - segment.Y * sin(da));
			pos.Y = orig.Y + (segment.X * sin(da) + segment.Y * cos(da));

			found = pos;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( !ManualTeaching( &found.X, &found.Y, MsgGetString(Msg_00811) ) )
			{
				abort = 1;
				break;
			}

			Set_HeadCameraLight( 1 );
			set_currentcam( CAMERA_HEAD );
			Set_Tv_Title( "" );

			//progress_bar->Increment(1);


			// salvataggio dati errore punto
			//--------------------------------------------------------------------------
			verifyMap[np].dx = found.X - pos.X;
			verifyMap[np].dy = found.Y - pos.Y;
			verifyMap[np].s = 1;

			correctionMap[np].dx = found.X - pos.X;
			correctionMap[np].dy = found.Y - pos.Y;
		}
	}


	delete progress_bar;
	delete wait_window;

	// led camera off
	Set_HeadCameraLight( 0 );

	if( !abort )
	{
		if( def_map )
		{
			// si trasla l'origine in basso
			orig.Y -= offsety * DEF_MAP_GAP_Y;
		}

		// save data
		saveMapData( "map_check_correct.txt", verifyMap, orig.X, orig.Y, npointx, npointy, offsetx, offsety );
		saveMapData( "map_correction.txt", correctionMap, orig.X, orig.Y, npointx, npointy, offsetx, offsety );

		ShowMapDataUI showWin( 0, verifyMap, correctionMap, npointx, npointy, def_map );
		showWin.Show();
		showWin.Hide();
	}

	// restore def acc/speed
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	DisableForceFinec();
	Set_Finec(OFF);

	return abort ? 0 : 1;
}


//----------------------------------------------------------------------------------
// Verifica mappatura piano: avanzamento Y pos (dir = 0) o Y neg (dir = 1)
//----------------------------------------------------------------------------------
int Errormap_Check_XY( int dir )
{
	if( Get_OnFile() )
		return 1;

	int npointx = Vision.mapnum_x;
	int npointy = Vision.mapnum_y;

	float offsetx = Vision.mapoff_x;
	float offsety = Vision.mapoff_y;

	PointF orig( Vision.pos_x, Vision.pos_y );
	PointF ref;

	bool def_map = false;

	if( npointx == 0 || npointy == 0 )
	{
		// abilita mappatura default
		npointx = DEF_MAP_X;
		npointy = DEF_MAP_Y;

		offsetx = DEF_MAP_OFFSET_MM;
		offsety = DEF_MAP_OFFSET_MM;

		def_map = true;
	}

	EnableForceFinec(ON); // forza check finecorsa ad on impedendo cambiamenti

	//svuota buffer
	flushKeyboardBuffer();


	// load image data
	char filename[MAXNPATH];
	SetImageName( filename, MAPPING_IMG, DATA );
	if( access(filename,0) == 0 )
	{
		img_data imgData;
		ImgDataLoad( filename, &imgData );

		// setta parametri immagine
		SetImgBrightCont( imgData.bright, imgData.contrast );
	}


	int abort = 0;
	while( !abort )
	{
		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

		// apprende punto di riferimento 1
		char titolo[80];
		snprintf( titolo, 80, MsgGetString(Msg_01971), 1 );
		if( !ManualTeaching( &orig.X, &orig.Y, titolo, 59, AUTOAPP_MAPOFF ) )
		{
			abort = 1;
		}
		else
		{
			// controlla raggiungibilita' area mappatura
			bool pos_ok = false;

			if( def_map )
			{
				// punto iniziale basso sx
				ref.X = orig.X;
				ref.Y = orig.Y - offsety * DEF_MAP_GAP_Y;

				pos_ok = Check_XYMove( ref.X, ref.Y );

				if( pos_ok )
				{
					// punto finale alto dx
					ref.X = orig.X + offsetx * ( npointx - 1 );
					ref.Y = orig.Y + offsety * ( npointy - DEF_MAP_GAP_Y - 1 );

					pos_ok = Check_XYMove( ref.X, ref.Y );
				}
			}
			else
			{
				// punto finale alto dx
				ref.X = orig.X + offsetx * ( npointx - 1 );
				ref.Y = orig.Y + offsety * ( npointy - 1 );

				pos_ok = Check_XYMove( ref.X, ref.Y );
			}

			if( pos_ok )
			{
				break;
			}

			W_Mess( MsgGetString(Msg_00172) );
		}
	}

	if( abort )
	{
		// restore def acc/speed
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

		DisableForceFinec();
		Set_Finec(OFF);
		return 0;
	}


	// calcolo rotazione tra origine (basso sx) e punto finale (alto dx)
	float da = 0.f;
	if( !Errormap_GetRotation( orig, ref, da ) )
	{
		// restore def acc/speed
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

		DisableForceFinec();
		Set_Finec(OFF);
		return 0;
	}


	// conferma utilizzo rotazione scheda
	char buf[120];
	snprintf( buf, sizeof(buf), MsgGetString(Msg_00225), da*180.f/PI );
	if( !W_Deci( 1, buf ) )
	{
		da = 0.f;
	}

	vector<eMap_point> verifyMap;

	CWindow* wait_window = new CWindow( 0 );
	wait_window->SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
	wait_window->SetClientAreaSize( 50, 7 );
	//
	wait_window->Show();
	wait_window->DrawTextCentered( 1, MsgGetString(Msg_05045) );
	wait_window->DrawTextCentered( 2, MsgGetString(Msg_05044) );
	GUI_ProgressBar_OLD* progress_bar = new GUI_ProgressBar_OLD( wait_window, 2, 4, 46, 1, (npointx * npointy) );

	// led camera on
	Set_HeadCameraLight( 1 );
	set_currentcam( CAMERA_HEAD );
	Set_Tv_Title( MsgGetString(Msg_01196) );

	// ciclo mappatura
	//--------------------------------------------------------------------------
	PointF pos, found;

	int x = 0;
	int y = ( dir == 0 ) ? 0 : npointy - 1;
	int teached_points = 0;
	int failed_points = 0;
	for( int np = 0; np < npointx*npointy; np++ )
	{
		if( Esc_Press() )
		{
			if( W_Deci(0,INTERRUPTMSG) )
			{
				abort = 1;
				break;
			}
		}


		// controlla punto
		//--------------------------------------------------------------------------

		bool skip = false;
		bool ask_user = false;

		if( def_map && ((x < DEF_MAP_GAP_X1 && y < DEF_MAP_GAP_Y) || (x >= (DEF_MAP_X - DEF_MAP_GAP_X2) && y < DEF_MAP_GAP_Y)) )
		{
			// salta punto
			skip = true;
		}
		else
		{
			// posizione punto
			//--------------------------------------------------------------------------

			if( def_map )
			{
				pos.X = x*offsetx;
				pos.Y = (y-DEF_MAP_GAP_Y)*offsety;
			}
			else
			{
				pos.X = x*offsetx;
				pos.Y = y*offsety;
			}

			// correzione rotazione
			PointF segment( pos );
			pos.X = orig.X + (segment.X * cos(da) - segment.Y * sin(da));
			pos.Y = orig.Y + (segment.X * sin(da) + segment.Y * cos(da));

			found = pos;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( Find_MapPattern( &found.X, &found.Y ) == false )
			{
				// solo per il primo punto chiede apprendimento manuale se necessario
				if( teached_points == 0 )
				{
					if( W_Deci(1,MAPERROR) )
					{
						found = pos;

						if( !ManualTeaching( &found.X, &found.Y, MsgGetString(Msg_00811) ) )
						{
							abort = 1;
							break;
						}

						Set_HeadCameraLight( 1 );
						set_currentcam( CAMERA_HEAD );
						Set_Tv_Title( "" );
					}
					else
					{
						abort = 1;
						break;
					}
				}
				else
				{
					ask_user = true;
					failed_points++;

					char buf[80];
					snprintf( buf, sizeof(buf), "Failed: %d", failed_points );
					wait_window->DrawText( 4, 5, buf );
				}
			}

			teached_points++;
		}

		progress_bar->Increment(1);


		// salvataggio dati errore punto
		//--------------------------------------------------------------------------

		eMap_point _err;
		if( skip || ask_user )
		{
			_err.dx = 0;
			_err.dy = 0;
			_err.s = ask_user ? 0 : 1;
		}
		else if( teached_points == 1 )
		{
			// definisce il punto di riferimento
			_err.dx = 0;
			_err.dy = 0;
			_err.s = 1;

			if( dir == 0 )
			{
				orig = found;
			}
		}
		else
		{
			_err.dx = found.X - pos.X;
			_err.dy = found.Y - pos.Y;
			_err.s = 1;
		}

		_err.px = x;
		_err.py = y;

		verifyMap.push_back( _err );


		if( dir == 0 )
		{
			// avanza Y POS
			if( y < npointy - 1 )
				y++;
			else
			{
				y = 0;
				x++;
			}
		}
		else
		{
			// avanza Y NEG
			if( y > 0 )
				y--;
			else
			{
				y = npointy - 1;
				x++;
			}
		}
	}


	// correzione manuale punti falliti
	//--------------------------------------------------------------------------
	if( !abort )
	{
		for( int np = 0; np < verifyMap.size(); np++ )
		{
			// controlla punto
			//--------------------------------------------------------------------------
			if( verifyMap[np].s == 1 )
			{
				continue;
			}

			// posizione punto
			//--------------------------------------------------------------------------
			if( def_map )
			{
				pos.X = verifyMap[np].px*offsetx;
				pos.Y = (verifyMap[np].py-DEF_MAP_GAP_Y)*offsety;
			}
			else
			{
				pos.X = verifyMap[np].px*offsetx;
				pos.Y = verifyMap[np].py*offsety;
			}

			// correzione rotazione
			PointF segment( pos );
			pos.X = orig.X + (segment.X * cos(da) - segment.Y * sin(da));
			pos.Y = orig.Y + (segment.X * sin(da) + segment.Y * cos(da));

			found = pos;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( !ManualTeaching( &found.X, &found.Y, MsgGetString(Msg_00811) ) )
			{
				abort = 1;
				break;
			}

			Set_HeadCameraLight( 1 );
			set_currentcam( CAMERA_HEAD );
			Set_Tv_Title( "" );

			//progress_bar->Increment(1);


			// salvataggio dati errore punto
			//--------------------------------------------------------------------------
			verifyMap[np].dx = found.X - pos.X;
			verifyMap[np].dy = found.Y - pos.Y;
			verifyMap[np].s = 1;
		}
	}


	delete progress_bar;
	delete wait_window;

	// led camera off
	Set_HeadCameraLight( 0 );

	if( !abort )
	{
		if( def_map )
		{
			// si trasla l'origine in basso
			orig.Y -= offsety * DEF_MAP_GAP_Y;
		}

		// save data
		if( dir == 0 )
			saveMapData( "map_check_Y_pos.txt", verifyMap, orig.X, orig.Y, npointx, npointy, offsetx, offsety );
		else
			saveMapData( "map_check_Y_neg.txt", verifyMap, orig.X, orig.Y, npointx, npointy, offsetx, offsety );

		vector<eMap_point> emptyMap;
		ShowMapDataUI showWin( 0, verifyMap, emptyMap, npointx, npointy, def_map );
		showWin.Show();
		showWin.Hide();
	}

	// restore def acc/speed
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	DisableForceFinec();
	Set_Finec(OFF);

	return abort ? 0 : 1;
}

//----------------------------------------------------------------------------------
// Verifica mappatura piano: avanzamento casuale
//----------------------------------------------------------------------------------
int Errormap_Check_Random()
{
	if( Get_OnFile() )
		return 1;

	int npointx = Vision.mapnum_x;
	int npointy = Vision.mapnum_y;

	float offsetx = Vision.mapoff_x;
	float offsety = Vision.mapoff_y;

	PointF orig( Vision.pos_x, Vision.pos_y );
	PointF ref;

	bool def_map = false;

	if( npointx == 0 || npointy == 0 )
	{
		// abilita mappatura default
		npointx = DEF_MAP_X;
		npointy = DEF_MAP_Y;

		offsetx = DEF_MAP_OFFSET_MM;
		offsety = DEF_MAP_OFFSET_MM;

		def_map = true;
	}

	EnableForceFinec(ON); // forza check finecorsa ad on impedendo cambiamenti

	//svuota buffer
	flushKeyboardBuffer();


	// load image data
	char filename[MAXNPATH];
	SetImageName( filename, MAPPING_IMG, DATA );
	if( access(filename,0) == 0 )
	{
		img_data imgData;
		ImgDataLoad( filename, &imgData );

		// setta parametri immagine
		SetImgBrightCont( imgData.bright, imgData.contrast );
	}


	int abort = 0;
	while( !abort )
	{
		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

		// apprende punto di riferimento 1
		char titolo[80];
		snprintf( titolo, 80, MsgGetString(Msg_01971), 1 );
		if( !ManualTeaching( &orig.X, &orig.Y, titolo, 59, AUTOAPP_MAPOFF ) )
		{
			abort = 1;
		}
		else
		{
			// controlla raggiungibilita' area mappatura
			bool pos_ok = false;

			if( def_map )
			{
				// punto iniziale basso sx
				ref.X = orig.X;
				ref.Y = orig.Y - offsety * DEF_MAP_GAP_Y;

				pos_ok = Check_XYMove( ref.X, ref.Y );

				if( pos_ok )
				{
					// punto finale alto dx
					ref.X = orig.X + offsetx * ( npointx - 1 );
					ref.Y = orig.Y + offsety * ( npointy - DEF_MAP_GAP_Y - 1 );

					pos_ok = Check_XYMove( ref.X, ref.Y );
				}
			}
			else
			{
				// punto finale alto dx
				ref.X = orig.X + offsetx * ( npointx - 1 );
				ref.Y = orig.Y + offsety * ( npointy - 1 );

				pos_ok = Check_XYMove( ref.X, ref.Y );
			}

			if( pos_ok )
			{
				break;
			}

			W_Mess( MsgGetString(Msg_00172) );
		}
	}

	if( abort )
	{
		// restore def acc/speed
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

		DisableForceFinec();
		Set_Finec(OFF);
		return 0;
	}


	// calcolo rotazione tra origine (basso sx) e punto finale (alto dx)
	float da = 0.f;
	if( !Errormap_GetRotation( orig, ref, da ) )
	{
		// restore def acc/speed
		SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

		DisableForceFinec();
		Set_Finec(OFF);
		return 0;
	}


	vector<eMap_point> errorMap;

	CWindow* wait_window = new CWindow( 0 );
	wait_window->SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
	wait_window->SetClientAreaSize( 50, 6 );
	//
	wait_window->Show();
	wait_window->DrawTextCentered( 1, MsgGetString(Msg_05045) );
	wait_window->DrawTextCentered( 2, MsgGetString(Msg_05044) );
	GUI_ProgressBar_OLD* progress_bar = new GUI_ProgressBar_OLD( wait_window, 2, 4, 46, 1, (npointx*npointy) );

	// led camera on
	Set_HeadCameraLight( 1 );
	set_currentcam( CAMERA_HEAD );
	Set_Tv_Title( MsgGetString(Msg_01196) );


	// ciclo mappatura
	//--------------------------------------------------------------------------
	PointF pos, found;
	int x = 0;
	int y = 0;
	int teached_points = 0;

	// initialize random seed
	srand( time(NULL) );

	while( 1 )
	{
		if( Esc_Press() )
		{
			if( W_Deci(0,INTERRUPTMSG) )
			{
				abort = 1;
				break;
			}
		}


		// controlla punto
		//--------------------------------------------------------------------------

		bool skip = false;

		if( def_map && ((x < DEF_MAP_GAP_X1 && y < DEF_MAP_GAP_Y) || (x >= (DEF_MAP_X - DEF_MAP_GAP_X2) && y < DEF_MAP_GAP_Y)) )
		{
			// salta punto
			skip = true;
		}
		else
		{
			// posizione punto
			//--------------------------------------------------------------------------

			if( def_map )
			{
				pos.X = x*offsetx;
				pos.Y = (y-DEF_MAP_GAP_Y)*offsety;
			}
			else
			{
				pos.X = x*offsetx;
				pos.Y = y*offsety;
			}

			// correzione rotazione
			PointF segment( pos );
			pos.X = orig.X + (segment.X * cos(da) - segment.Y * sin(da));
			pos.Y = orig.Y + (segment.X * sin(da) + segment.Y * cos(da));

			found = pos;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( Find_MapPattern( &found.X, &found.Y ) == false )
			{
				skip = true;
			}

			teached_points++;
		}

		progress_bar->Increment(1);


		// salvataggio dati errore punto
		//--------------------------------------------------------------------------

		if( !skip )
		{
			eMap_point _err;
			if( teached_points == 1 )
			{
				// definisce il punto di riferimento
				_err.dx = 0;
				_err.dy = 0;

				orig = found;
			}
			else
			{
				_err.dx = found.X - pos.X;
				_err.dy = found.Y - pos.Y;
			}

			_err.px = x;
			_err.py = y;

			errorMap.push_back( _err );
		}


		x = rand() % npointx;
		y = rand() % npointy;
	}

	delete progress_bar;
	delete wait_window;

	// led camera off
	Set_HeadCameraLight( 0 );

	if( def_map )
	{
		// si trasla l'origine in basso
		orig.Y -= offsety * DEF_MAP_GAP_Y;
	}

	// save data
	saveMapData( "map_check_rand.txt", errorMap, orig.X, orig.Y, npointx, npointy, offsetx, offsety );

	// restore def acc/speed
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	DisableForceFinec();
	Set_Finec(OFF);

	return abort ? 0 : 1;
}

//----------------------------------------------------------------------------------
// Visualizza dati ultima verifica
//----------------------------------------------------------------------------------
int Show_Errormap_Check( CWindow* parent )
{
	eMapStruct checkMap;
	eMapStruct correctionMap;

	loadMapData( "map_check_correct.txt", checkMap );
	loadMapData( "map_correction.txt", correctionMap );

	bool def_map = false;
	if( checkMap.nx == DEF_MAP_X || checkMap.ny == DEF_MAP_Y )
	{
		def_map = true;
	}

	ShowMapDataUI showWin( parent, checkMap.data, correctionMap.data, checkMap.nx, checkMap.ny, def_map, true );
	showWin.Show();
	showWin.Hide();

	return 1;
}


//----------------------------------------------------------------------------------
// Elimina dati mappatura
//----------------------------------------------------------------------------------
int Errormap_Delete()
{
	char buf[180];
	snprintf( buf, 180, "%s %s", MsgGetString(Msg_00766), AREYOUSURETXT );
	if( !W_Deci( 0, buf ) )
	{
		return 0;
	}

	// resetta mappatura
	ClearMapData();

	return 1;
}


//----------------------------------------------------------------------------------
// Esegue calibrazione ortogonalita' assi XY
//----------------------------------------------------------------------------------
int OrthogonalityXY_Calibrate( bool save_result )
{
	if( Get_OnFile() )
	{
		return 1;
	}

	//svuota buffer
	flushKeyboardBuffer();

	EnableForceFinec(ON); // forza check finecorsa ad on impedendo cambiamenti

	float x1 = Vision.pos_x;
	float y1 = Vision.pos_y;
	float x2, y2, x3, y3;

	int abort = 0;

	// disattiva mappatura
	use_mapping_correction = false;

	do
	{
		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

		//autoapprende punto di riferimento 1
		char titolo[80];
		snprintf( titolo, 80, MsgGetString(Msg_01971), 1 );
		if( !ManualTeaching( &x1, &y1, titolo ) )
		{
			abort = 1;
		}
		else
		{
			// controlla raggiungibilita' punti
			x2 = x1;
			y2 = y1 + QParam.OrthoXY_Movement_y;

			x3 = x1 + QParam.OrthoXY_Movement_x;
			y3 = y1;

			if( !Check_XYMove( x2, y2 ) || !Check_XYMove( x3, y3 ) )
			{
				W_Mess( MsgGetString(Msg_00172) );
			}
			else
			{
				//ok
				break;
			}
		}
	} while( !abort );

	if( !abort )
	{
		// wait window
		CWindow* wait_window = new CWindow( 0 );
		wait_window->SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
		wait_window->SetClientAreaSize( 50, 4 );
		//
		wait_window->Show();
		wait_window->DrawTextCentered( 1, MsgGetString(Msg_05045) );
		wait_window->DrawTextCentered( 2, MsgGetString(Msg_05044) );

		// led camera on
		Set_HeadCameraLight( 1 );
		set_currentcam( CAMERA_HEAD );
		Set_Tv_Title( "" );

		PointF pos;

		for( int i = 0; i < 3; i++ )
		{
			if( Esc_Press() )
			{
				if( W_Deci(0,INTERRUPTMSG) )
				{
					abort = 1;
					break;
				}
			}

			if( i == 0 )
			{
				pos.X = x1;
				pos.Y = y1;
			}
			else if( i == 1 )
			{
				pos.X = x1;
				pos.Y = y1 + QParam.OrthoXY_Movement_y;
			}
			else
			{
				pos.X = x1 + QParam.OrthoXY_Movement_x;
				pos.Y = y1;
			}

			float tmpX = pos.X;
			float tmpY = pos.Y;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			//GF_TEMP_ENC_TEST
			if( Find_MapPattern( &pos.X, &pos.Y ) == false )
			{
				if( W_Deci(1,MAPERROR) )
				{
					pos.X = tmpX;
					pos.Y = tmpY;

					if( !ManualTeaching( &pos.X, &pos.Y, MsgGetString(Msg_00811) ) )
					{
						abort = 1;
						break;
					}

					// led camera on
					Set_HeadCameraLight( 1 );
					set_currentcam( CAMERA_HEAD );
					Set_Tv_Title( "" );
				}
				else
				{
					abort = 1;
					break;
				}
			}

			if( i == 0 )
			{
				x1 = pos.X;
				y1 = pos.Y;
			}
			else if( i == 1 )
			{
				x2 = pos.X;
				y2 = pos.Y;
			}
			else
			{
				x3 = pos.X;
				y3 = pos.Y;
			}
		}

		delete wait_window;

		// led camera off
		Set_HeadCameraLight( 0 );
	}

	// riattiva mappatura
	use_mapping_correction = true;

	// restore def acc/speed
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	DisableForceFinec();
	Set_Finec(OFF);

	//svuota buffer
	flushKeyboardBuffer();

	if(abort)
	{
		return 0;
	}

	float deltaX, deltaY;

	// Y axis
	deltaX = x2 - x1;
	deltaY = y2 - y1;
	// calcolo come se fosse ruotato di 90 deg in senso orario
	float yAxisRot = atan2( -deltaX, deltaY );

	// X axis
	deltaX = x3 - x1;
	deltaY = y3 - y1;
	float xAxisRot = atan2( deltaY, deltaX );

	// update
	float new_value = QParam.OrthoXY_Error + (yAxisRot - xAxisRot);

	char buf[120];
	snprintf( buf, sizeof(buf), "%.6f  ->  %.6f", QParam.OrthoXY_Error, new_value );
	W_Mess( buf, MSGBOX_YCENT, MsgGetString(Msg_00215) );

	if( save_result )
	{
		// save
		QParam.OrthoXY_Error = new_value;
		Mod_Par( QParam );
	}

	return 1;
}

//----------------------------------------------------------------------------------
// Esegue calibrazione scala encoder assi XY
//----------------------------------------------------------------------------------
int EncoderScale_Calibrate( bool save_result )
{
	if( Get_OnFile() )
	{
		return 1;
	}

	EnableForceFinec(ON); // forza check finecorsa ad on impedendo cambiamenti

	PointF p1( Vision.pos_x, Vision.pos_y );
	PointF p2, p3;

	int abort = 0;

	// disattiva mappatura
	use_mapping_correction = false;

	do
	{
		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

		//autoapprende punto di riferimento 1
		char titolo[80];
		snprintf( titolo, 80, MsgGetString(Msg_01971), 1 );
		if( !ManualTeaching( &p1.X, &p1.Y, titolo ) )
		{
			abort = 1;
		}
		else
		{
			// controlla raggiungibilita' punti
			p2.X = p1.X;
			p2.Y = p1.Y + QParam.EncScale_Movement_y;

			p3.X = p1.X + QParam.EncScale_Movement_x;
			p3.Y = p1.Y;

			if( !Check_XYMove( p2.X, p2.Y ) || !Check_XYMove( p3.X, p3.Y ) )
			{
				W_Mess( MsgGetString(Msg_00172) );
			}
			else
			{
				//ok
				break;
			}
		}
	} while( !abort );

	if( !abort )
	{
		// wait window
		CWindow* wait_window = new CWindow( 0 );
		wait_window->SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
		wait_window->SetClientAreaSize( 50, 4 );
		//
		wait_window->Show();
		wait_window->DrawTextCentered( 1, MsgGetString(Msg_05045) );
		wait_window->DrawTextCentered( 2, MsgGetString(Msg_05044) );


		// led camera on
		Set_HeadCameraLight( 1 );
		set_currentcam( CAMERA_HEAD );
		Set_Tv_Title( "" );

		PointF pos;

		for( int i = 0; i < 3; i++ )
		{
			if( Esc_Press() )
			{
				if( W_Deci(0,INTERRUPTMSG) )
				{
					abort = 1;
					break;
				}
			}

			if( i == 0 )
			{
				pos = p1;
			}
			else if( i == 1 )
			{
				pos.X = p1.X;
				pos.Y = p1.Y + QParam.EncScale_Movement_y;
			}
			else
			{
				pos.X = p1.X + QParam.EncScale_Movement_x;
				pos.Y = p1.Y;
			}

			PointF pTmp = pos;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			//GF_TEMP_ENC_TEST
			if( Find_MapPattern( &pos.X, &pos.Y ) == false )
			{
				if( W_Deci(1,MAPERROR) )
				{
					pos = pTmp;

					if( !ManualTeaching( &pos.X, &pos.Y, MsgGetString(Msg_00811) ) )
					{
						abort = 1;
						break;
					}

					// led camera on
					Set_HeadCameraLight( 1 );
					set_currentcam( CAMERA_HEAD );
					Set_Tv_Title( "" );
				}
				else
				{
					abort = 1;
					break;
				}
			}

			if( i == 0 )
			{
				p1 = pos;
			}
			else if( i == 1 )
			{
				p2 = pos;
			}
			else
			{
				p3 = pos;
			}
		}

		delete wait_window;

		// led camera off
		Set_HeadCameraLight( 0 );
	}

	// riattiva mappatura
	use_mapping_correction = true;

	// restore def acc/speed
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	DisableForceFinec();
	Set_Finec(OFF);

	//svuota buffer
	flushKeyboardBuffer();

	if(abort)
	{
		return 0;
	}

	float deltaX, deltaY, encoderError;

	// Y axis
	deltaY = p2.Y - p1.Y;
	if( deltaY == 0.f || QParam.EncScale_Movement_y == 0.f )
		encoderError = 1.0;
	else
		encoderError = deltaY / QParam.EncScale_Movement_y;
	// update
	float new_valueY = QParam.EncScale_y * encoderError;

	// X axis
	deltaX = p3.X - p1.X;
	if( deltaX == 0.f || QParam.EncScale_Movement_x == 0.f )
		encoderError = 1.0;
	else
		encoderError = deltaX / QParam.EncScale_Movement_x;
	// update
	float new_valueX = QParam.EncScale_x * encoderError;

	char buf[120];
	snprintf( buf, sizeof(buf), "%.6f  ->  %.6f\n%.6f  ->  %.6f", QParam.EncScale_x, new_valueX, QParam.EncScale_y, new_valueY );
	W_Mess( buf, MSGBOX_YCENT, MsgGetString(Msg_00216) );

	if( save_result )
	{
		// save
		QParam.EncScale_x = new_valueX;
		QParam.EncScale_y = new_valueY;
		Mod_Par( QParam );
	}

	return 1;
}


//----------------------------------------------------------------------------------
// GetXYMapError: Get error map
//----------------------------------------------------------------------------------
float _PiramidalInterpolation(float v1,float v2,float v3,float v4,float dx,float dy,float x,float y)
{
	float v =	(v1/dx*(dx-x)*(dy-y)+
				v2/dx*x*(dy-y)+
				v3/dx*x*y+
				v4/dx*(dx-x)*y)/dy;

	return v;
}

float _XMapError( int x, int y )
{
	return eMap.data[x + y * eMap.nx].dx;
}
float _YMapError( int x, int y )
{
	return eMap.data[x + y * eMap.nx].dy;
}

void GetXYMapError( float xcoord, float ycoord, float &xoff, float &yoff )
{
	// esci se dati non presenti
	if( eMap.data.size() == 0 )
	{
		return;
	}

	// Recupero la cella di appartenenza della coordinata x
	float pos_X_tab = xcoord - eMap.x;
	float cellx = (int)floor(pos_X_tab / eMap.sx);
	float scartox = pos_X_tab - cellx * eMap.sx;

	// Recupero la cella di appartenenza della coordinata y
	float pos_Y_tab = ycoord - eMap.y;
	float celly = (int)floor(pos_Y_tab / eMap.sy);
	float scartoy = pos_Y_tab - celly * eMap.sy;

	// Casi in cui la coordinata X e' < della X di start
	if( xcoord <= eMap.x )
	{
		// Y < Y di start
		if( ycoord <= eMap.y )
		{
			xoff = _XMapError(0, 0);
			yoff = _YMapError(0, 0);
		}
		// Y > Y di end
		else if( ycoord >= eMap.y + (eMap.ny - 1) * eMap.sy )
		{
			xoff = _XMapError(0, eMap.ny-1);
			yoff = _YMapError(0, eMap.ny-1);
		}
		// Y in mezzo alla zona mappata (interpolazione lineare)
		else
		{
			xoff = _XMapError(0, celly) + scartoy * (_XMapError(0, celly+1) - _XMapError(0, celly)) / eMap.sy;
			yoff = _YMapError(0, celly) + scartoy * (_YMapError(0, celly+1) - _YMapError(0, celly)) / eMap.sy;
		}
	}
	// Casi in cui la coordinata X e' > della X di end
	else if( xcoord >= eMap.x + (eMap.nx - 1) * eMap.sx )
	{
		// Y < Y di start
		if( ycoord <= eMap.y )
		{
			xoff = _XMapError(eMap.nx-1, 0);
			yoff = _YMapError(eMap.nx-1, 0);
		}
		// Y > Y di end
		else if( ycoord >= eMap.y + (eMap.ny - 1) * eMap.sy )
		{
			xoff = _XMapError(eMap.nx-1, eMap.ny-1);
			yoff = _YMapError(eMap.nx-1, eMap.ny-1);
		}
		// Y in mezzo alla zona mappata (interpolazione lineare)
		else
		{
			xoff = _XMapError(eMap.nx-1, celly) + scartoy * (_XMapError(eMap.nx-1, celly+1) - _XMapError(eMap.nx-1, celly)) / eMap.sy;
			yoff = _YMapError(eMap.nx-1, celly) + scartoy * (_YMapError(eMap.nx-1, celly+1) - _YMapError(eMap.nx-1, celly)) / eMap.sy;
		}
	}
	// X in mezzo alla zona mappata (interpolazione lineare)
	else
	{
		// Y < Y di start
		if( ycoord <= eMap.y )
		{
			xoff = _XMapError(cellx, 0) + scartox * (_XMapError(cellx+1, 0) - _XMapError(cellx, 0)) / eMap.sx;
			yoff = _YMapError(cellx, 0) + scartox * (_YMapError(cellx+1, 0) - _YMapError(cellx, 0)) / eMap.sx;
		}
		// Y > Y di end
		else if( ycoord >= eMap.y + (eMap.ny - 1) * eMap.sy )
		{
			xoff = _XMapError(cellx, eMap.ny-1) + scartox * (_XMapError(cellx+1, eMap.ny-1) - _XMapError(cellx, eMap.ny-1)) / eMap.sx;
			yoff = _YMapError(cellx, eMap.ny-1) + scartox * (_YMapError(cellx+1, eMap.ny-1) - _YMapError(cellx, eMap.ny-1)) / eMap.sx;
		}
		// X e Y in mezzo alla zona mappata (Metodo delle piramidi)
		else
		{
			xoff = _PiramidalInterpolation(
					_XMapError(cellx, celly),
					_XMapError(cellx+1, celly),
					_XMapError(cellx+1, celly+1),
					_XMapError(cellx, celly+1),
					eMap.sx, eMap.sx,
					scartox, scartoy );

			yoff = _PiramidalInterpolation(
					_YMapError(cellx, celly),
					_YMapError(cellx+1, celly),
					_YMapError(cellx+1, celly+1),
					_YMapError(cellx, celly+1),
					eMap.sx, eMap.sx,
					scartox, scartoy );
		}
	}
}

//----------------------------------------------------------------------------------
// Corregge la posizione passata con la mappatura
//----------------------------------------------------------------------------------
void Errormap_Correct( float& x, float& y )
{
	if( !(QHeader.modal & ENABLE_XYMAP) )
	{
		return;
	}

	// esci se dati non presenti
	if( eMap.data.size() == 0 )
	{
		return;
	}

	//TODO: da sistemare. cosi' disattiva la mappatura nell'origine
	if( fabs( x ) < 10.f && fabs( y ) < 10.f )
		return;

	float errx, erry;
	GetXYMapError( x, y, errx, erry );

	//print_debug( "map [ %.2f, %.2f] -> CORREZIONI:  %.3f,   %.3f\n", x, y, errx, erry );

	x += errx;
	y += erry;
}





//---------------------------------------------------------------------------------
// Controlla posizionamento dei componenti
//---------------------------------------------------------------------------------
extern void GetSchedaPos(struct Zeri master,struct Zeri zero,float x_posiz,float y_posiz,float &cx_oo,float &cy_oo);

struct eMap_component
{
	eMap_component()
	{
		dx = dy = 0.f;
		angle = 0.f;
		px = py = 0;
		line = 0;
		s = 0;
	}

	float dx;
	float dy;
	float angle;
	float px;
	float py;
	int line;
	char s; // stato: 0 = errore, 1 = ok
};

//------------------------------------------------------------------------------------------
// Salva dati mappatura componenti
//------------------------------------------------------------------------------------------
bool SaveMapCompData( const char* filename, vector<eMap_component>& data, float posX, float posY )
{
	FILE* pFile = fopen( filename, "wt" );
	if( pFile == NULL )
	{
		char buf[256];
		sprintf( buf, MsgGetString(Msg_05056), filename );
		W_Mess( buf );
		return false;
	}

	// map position start
	fprintf( pFile, "%.3f;%.3f;", posX, posY );

	// map errors
	for( unsigned int i = 0; i < data.size(); i++ )
	{
		fprintf( pFile, "%.3f;%.3f;%.2f;%.2f;%.3f;%d\n", data[i].dx, data[i].dy, data[i].px, data[i].py, data[i].angle, data[i].line );
	}

	fclose( pFile );
	return true;
}

//------------------------------------------------------------------------------------------
// Controlla deposito componenti
//------------------------------------------------------------------------------------------
int ComponentsPlacing_Check()
{
	/*
	if( Get_LastRecMount() == -1 )
	{
		W_Mess( "Nessun programma eseguito !" ); //TODO messaggio
		return false;
	}

	// chiede se eseguire la ricerca sul programma attualmente selezionato
	char buf[80];
	snprintf( buf, sizeof(buf), MsgGetString(Msg_00561), QHeader.Prg_Default );
	if( !W_Deci(0,buf) )
	{
		return false;
	}
	*/

	struct Zeri AA_Zeri[2];
	struct TabPrg AA_Tab;
	struct CarDat AA_Caric;

	FeederFile* CarFile=new FeederFile( QHeader.Conf_Default );
	if( !CarFile->opened )
	{
		delete CarFile;
		return 0;
	}

	TPrgFile* TPrg = new TPrgFile(QHeader.Prg_Default,PRG_ASSEMBLY);
	if( !TPrg->Open(SKIPHEADER) )
	{
		delete TPrg;
		delete CarFile;
		return 0;
	}

	ZerFile* zer = new ZerFile(QHeader.Prg_Default);
	if( !zer->Open() )
	{
		W_Mess( NOZSCHFILE );
		delete zer;
		delete TPrg;
		delete CarFile;
		return 0;
	}

	if( !PackagesLib_Load( QHeader.Lib_Default ) )
	{
		delete zer;
		delete TPrg;
		delete CarFile;
		return 0;
	}

	if( zer->GetNRecs() < 2 ) // no recs zeri presenti
	{
		W_Mess( NOZSCHEDA );
		delete zer;
		delete TPrg;
		delete CarFile;
		return 0;
	}

	zer->Read(AA_Zeri[0],0); // read board 0

	if( TPrg->Count() < 1 ) // no recs in tabella progr...
	{
		W_Mess( NORPROG );
		delete zer;
		delete TPrg;
		delete CarFile;
		return 0;
	}

	Set_Finec(ON);


	int abort = 0;
	vector<eMap_component> errorMapComp;


	CWindow* wait_window = new CWindow( 0 );
	wait_window->SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
	wait_window->SetClientAreaSize( 50, 7 );
	//
	wait_window->Show();
	wait_window->DrawTextCentered( 1, MsgGetString(Msg_00146) );
	wait_window->DrawTextCentered( 2, MsgGetString(Msg_05044) );
	GUI_ProgressBar_OLD* progress_bar = new GUI_ProgressBar_OLD( wait_window, 2, 4, 46, 1, TPrg->Count() );

	Set_HeadCameraLight( mapTestLight );
	set_currentcam( CAMERA_HEAD );
	Set_Tv_Title( MsgGetString(Msg_00147) );

	int failed_points = 0;

	for( int i = 0; i < TPrg->Count(); i++ )
	{
		if( Esc_Press() )
		{
			if( W_Deci(0,INTERRUPTMSG) )
			{
				abort = 1;
				break;
			}
		}

		// legge il componente
		TPrg->Read( AA_Tab, i );
		// skip se flag non montare
		if( !(AA_Tab.status & MOUNT_MASK) )
		{
			continue;
		}

		// legge la scheda
		zer->Read( AA_Zeri[1], AA_Tab.scheda );
		// skip se board non da montare
		if( !AA_Zeri[1].Z_ass )
		{
			continue;
		}

		// legge dati caricatore
		CarFile->Read( AA_Tab.Caric, AA_Caric );

		PointF pos;
		GetSchedaPos( AA_Zeri[0], AA_Zeri[1], AA_Tab.XMon, AA_Tab.YMon, pos.X, pos.Y );

		PointF found = pos;
		float angle = 0.f;


		// controlla componente
		//--------------------------------------------------------------------------

		bool ask_user = false;

		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );
		if( Find_RectangularComponent( found.X, found.Y, angle, i ) == false )
		{
			ask_user = true;
			failed_points++;

			char buf[80];
			snprintf( buf, sizeof(buf), "Failed: %d", failed_points );
			wait_window->DrawText( 4, 5, buf );
		}

		progress_bar->Increment(1);


		// salvataggio dati errore punto
		//--------------------------------------------------------------------------

		eMap_component _err;
		if( ask_user )
		{
			_err.dx = 0;
			_err.dy = 0;
			_err.angle = 0;
			_err.s = 0;
		}
		else
		{
			_err.dx = found.X - pos.X;
			_err.dy = found.Y - pos.Y;
			_err.angle = angle;
			_err.s = 1;
		}

		_err.px = pos.X;
		_err.py = pos.Y;
		_err.line = i;

		errorMapComp.push_back( _err );
	}


	// correzione manuale punti falliti
	//--------------------------------------------------------------------------
	if( !abort )
	{
		for( int np = 0; np < errorMapComp.size(); np++ )
		{
			// controlla punto
			//--------------------------------------------------------------------------
			if( errorMapComp[np].s == 1 )
			{
				continue;
			}

			// posizione punto
			//--------------------------------------------------------------------------
			PointF pos;
			pos.X = errorMapComp[np].px;
			pos.Y = errorMapComp[np].py;

			PointF found = pos;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( !ManualTeaching( &found.X, &found.Y, MsgGetString(Msg_00811) ) )
			{
				abort = 1;
				break;
			}

			Set_HeadCameraLight( mapTestLight );
			set_currentcam( CAMERA_HEAD );
			Set_Tv_Title( "" );

			//progress_bar->Increment(1);


			// salvataggio dati errore punto
			//--------------------------------------------------------------------------
			errorMapComp[np].dx = found.X - pos.X;
			errorMapComp[np].dy = found.Y - pos.Y;
			errorMapComp[np].s = 1;
		}
	}


	delete progress_bar;
	delete wait_window;

	// led camera off
	Set_HeadCameraLight( 0 );

	if( !abort )
	{
		// save data
		SaveMapCompData( "map_rectcomp.txt", errorMapComp, 0, 0 );
	}

	// restore def acc/speed
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	DisableForceFinec();
	Set_Finec(OFF);

	// clean
	delete CarFile;
	delete TPrg;
	delete zer;

	return 1;
}

//------------------------------------------------------------------------------------------
// Controlla marcatura con inchiostro
//------------------------------------------------------------------------------------------
int InkMarks_Check()
{
	/*
	if( Get_LastRecMount() == -1 )
	{
		W_Mess( "Nessun programma eseguito !" ); //TODO messaggio
		return false;
	}

	// chiede se eseguire la ricerca sul programma attualmente selezionato
	char buf[80];
	snprintf( buf, sizeof(buf), MsgGetString(Msg_00561), QHeader.Prg_Default );
	if( !W_Deci(0,buf) )
	{
		return false;
	}
	*/

	struct Zeri AA_Zeri[2];
	struct TabPrg AA_Tab;
	struct CarDat AA_Caric;

	FeederFile* CarFile=new FeederFile( QHeader.Conf_Default );
	if( !CarFile->opened )
	{
		delete CarFile;
		return 0;
	}

	TPrgFile* TPrg = new TPrgFile(QHeader.Prg_Default,PRG_ASSEMBLY);
	if( !TPrg->Open(SKIPHEADER) )
	{
		delete TPrg;
		delete CarFile;
		return 0;
	}

	ZerFile* zer = new ZerFile(QHeader.Prg_Default);
	if( !zer->Open() )
	{
		W_Mess( NOZSCHFILE );
		delete zer;
		delete TPrg;
		delete CarFile;
		return 0;
	}

	if( !PackagesLib_Load( QHeader.Lib_Default ) )
	{
		delete zer;
		delete TPrg;
		delete CarFile;
		return 0;
	}

	if( zer->GetNRecs() < 2 ) // no recs zeri presenti
	{
		W_Mess( NOZSCHEDA );
		delete zer;
		delete TPrg;
		delete CarFile;
		return 0;
	}

	zer->Read(AA_Zeri[0],0); // read board 0

	if( TPrg->Count() < 1 ) // no recs in tabella progr...
	{
		W_Mess( NORPROG );
		delete zer;
		delete TPrg;
		delete CarFile;
		return 0;
	}

	Set_Finec(ON);


	int abort = 0;
	vector<eMap_component> errorMapComp;


	CWindow* wait_window = new CWindow( 0 );
	wait_window->SetStyle( WIN_STYLE_NO_CAPTION | WIN_STYLE_CENTERED | WIN_STYLE_NO_MENU );
	wait_window->SetClientAreaSize( 50, 7 );
	//
	wait_window->Show();
	wait_window->DrawTextCentered( 1, MsgGetString(Msg_00146) );
	wait_window->DrawTextCentered( 2, MsgGetString(Msg_05044) );
	GUI_ProgressBar_OLD* progress_bar = new GUI_ProgressBar_OLD( wait_window, 2, 4, 46, 1, TPrg->Count() );

	Set_HeadCameraLight( 1 );
	set_currentcam( CAMERA_HEAD );
	Set_Tv_Title( MsgGetString(Msg_00147) );

	int failed_points = 0;

	for( int i = 0; i < TPrg->Count(); i++ )
	{
		if( Esc_Press() )
		{
			if( W_Deci(0,INTERRUPTMSG) )
			{
				abort = 1;
				break;
			}
		}

		// legge il componente
		TPrg->Read( AA_Tab, i );
		// skip se flag non montare
		if( !(AA_Tab.status & MOUNT_MASK) )
		{
			continue;
		}

		// legge la scheda
		zer->Read( AA_Zeri[1], AA_Tab.scheda );
		// skip se board non da montare
		if( !AA_Zeri[1].Z_ass )
		{
			continue;
		}

		// legge dati caricatore
		CarFile->Read( AA_Tab.Caric, AA_Caric );

		PointF pos;
		GetSchedaPos( AA_Zeri[0], AA_Zeri[1], AA_Tab.XMon, AA_Tab.YMon, pos.X, pos.Y );

		PointF found = pos;
		float angle = 0.f;


		// controlla componente
		//--------------------------------------------------------------------------

		bool ask_user = false;

		SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );
		if( Find_InkMark( found.X, found.Y ) == false )
		{
			ask_user = true;
			failed_points++;

			char buf[80];
			snprintf( buf, sizeof(buf), "Failed: %d", failed_points );
			wait_window->DrawText( 4, 5, buf );
		}

		progress_bar->Increment(1);


		// salvataggio dati errore punto
		//--------------------------------------------------------------------------

		eMap_component _err;
		if( ask_user )
		{
			_err.dx = 0;
			_err.dy = 0;
			_err.angle = 0;
			_err.s = 0;
		}
		else
		{
			_err.dx = found.X - pos.X;
			_err.dy = found.Y - pos.Y;
			_err.angle = angle;
			_err.s = 1;
		}

		_err.px = pos.X;
		_err.py = pos.Y;
		_err.line = i;

		errorMapComp.push_back( _err );
	}


	// correzione manuale punti falliti
	//--------------------------------------------------------------------------
	if( !abort )
	{
		for( int np = 0; np < errorMapComp.size(); np++ )
		{
			// controlla punto
			//--------------------------------------------------------------------------
			if( errorMapComp[np].s == 1 )
			{
				continue;
			}

			// posizione punto
			//--------------------------------------------------------------------------
			PointF pos;
			pos.X = errorMapComp[np].px;
			pos.Y = errorMapComp[np].py;

			PointF found = pos;

			SetNozzleXYSpeed_Index( Vision.visionSpeedIndex );

			if( !ManualTeaching( &found.X, &found.Y, MsgGetString(Msg_00811) ) )
			{
				abort = 1;
				break;
			}

			Set_HeadCameraLight( 1 );
			set_currentcam( CAMERA_HEAD );
			Set_Tv_Title( "" );

			//progress_bar->Increment(1);


			// salvataggio dati errore punto
			//--------------------------------------------------------------------------
			errorMapComp[np].dx = found.X - pos.X;
			errorMapComp[np].dy = found.Y - pos.Y;
			errorMapComp[np].s = 1;
		}
	}


	delete progress_bar;
	delete wait_window;

	// led camera off
	Set_HeadCameraLight( 0 );

	if( !abort )
	{
		// save data
		SaveMapCompData( "map_inkmark.txt", errorMapComp, 0, 0 );
	}

	// restore def acc/speed
	SetNozzleXYSpeed_Index( ACC_SPEED_DEFAULT );

	DisableForceFinec();
	Set_Finec(OFF);

	// clean
	delete CarFile;
	delete TPrg;
	delete zer;

	return 1;
}

