/*
>>>> Q_GRAPH.CPP


Funzioni grafiche di supporto (DJGPP)


++++            Modulo di automazione QUADRA.                  ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995/99  ++++
++++           Tutti i diritti sono riservati.                 ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1999/2000  ++++

++++ Integrazioni LCM Simone >> S250701
*/

#include "q_graph.h"

#include <math.h>
#include <stdarg.h>

#include "q_msg.h"
#include "q_grcol.h"
#include "q_oper.h"
#include "q_init.h"
#include "strutils.h"

#include "gui_defs.h"
#include "gui_functions.h"
#include "keyutils.h"

#include <mss.h>


/***************************************************************************

               Funzioni di supporto nuova interfaccia grafica.

****************************************************************************/


/**********************  Gestione dei grafici *******************************/

C_Graph::C_Graph(int _x1,int _y1,int _x2,int _y2,const char *_title,int _type,int ngr)
{
	col=0;
	row=0;
	
	shown = false;
	
	x1=_x1;
	x2=_x2;
	
	y1=_y1;
	y2=_y2;
	
	SetPos( 0, 0, x2-2-x1-2, y2-2-y1-2 );
	
	Title = _title;
	type=_type;
	
	totdata=0;
	
	switch(type & GRAPH_NUMTYPEY)
	{
		case GRAPH_NUMTYPEY_FLOAT:
			FbufY=new float* [ngr];
			break;
		case GRAPH_NUMTYPEY_INT:
			IbufY=new int* [ngr];
			break;
		case GRAPH_NUMTYPEY_BYTE:
			BbufY=new char* [ngr];
			break;            
	}
	
	switch(type & GRAPH_NUMTYPEX)
	{
		case GRAPH_NUMTYPEX_FLOAT:
			FbufX=new float* [ngr];
			break;
		case GRAPH_NUMTYPEX_INT:
			IbufX=new  int* [ngr];
			break;
		case GRAPH_NUMTYPEX_BYTE:
			BbufX=new  char*[ngr];
			break;            
	}
	
	ngraph=ngr;
	
	ndata=new int[ngr];
	graphColor = new GUI_color[ngr];
	
	for(int i=0;i<ngr;i++)
	{
		ndata[i] = 0;
		graphColor[i] = GUI_color(GR_YELLOW);
	
		switch(type & GRAPH_NUMTYPEX)
		{
			case GRAPH_NUMTYPEX_FLOAT:
				FbufX[i]=NULL;
				break;
			case GRAPH_NUMTYPEX_INT:
				IbufX[i]=NULL;
				break;
			case GRAPH_NUMTYPEX_BYTE:
				BbufX[i]=NULL;
				break;
		}
	
		switch(type & GRAPH_NUMTYPEY)
		{
			case GRAPH_NUMTYPEY_FLOAT:
				FbufY[i]=NULL;
				break;
			case GRAPH_NUMTYPEY_INT:
				IbufY[i]=NULL;
				break;
			case GRAPH_NUMTYPEY_BYTE:
				BbufY[i]=NULL;
				break;
		}
	}
	
	vidbufX=NULL;
	vidbufY=NULL;  
	
	Q_graph=NULL;
	
	externalWind=0;
}


C_Graph::~C_Graph(void)
{
	delete [] ndata;
	delete [] graphColor;

	ResetGraphData();

	if(!externalWind && Q_graph )
	{
		delete Q_graph;
	}
}


//mode=0 dealloca tutto (default) 
//    =1 dealloca ma lascia inalterato il numero dei grafici
//       (per mantenere il numero dei grafici stabilito ma risettare i dati)
void C_Graph::ResetGraphData(int mode)
{
  if(vidbufX!=NULL)
  {
    free(vidbufX);
    vidbufX=NULL;
  }
  if(vidbufY!=NULL)
  {
    free(vidbufY);
    vidbufY=NULL;
  }

  for(int i=0;i<ngraph;i++)
  {
    ndata[i]=0;
    totdata=0;

    switch(type & GRAPH_NUMTYPEX)
    {
      case GRAPH_NUMTYPEX_FLOAT:
        if(FbufX[i]==NULL)
          break;
        delete[] FbufX[i];
        FbufX[i]=NULL;
        break;
      case GRAPH_NUMTYPEX_INT:
        if(IbufX[i]==NULL)
          break;
        delete[] IbufX[i];
        IbufX[i]=NULL;
        break;
      case GRAPH_NUMTYPEX_BYTE:
        if(BbufX[i]==NULL)
          break;
        delete[] BbufX[i];
        BbufX[i]=NULL;
        break;
    }

    switch(type & GRAPH_NUMTYPEY)
    {
      case GRAPH_NUMTYPEY_FLOAT:
        if(FbufY[i]==NULL)
          break;
        delete[] FbufY[i];
          FbufY[i]=NULL;
        break;
      case GRAPH_NUMTYPEY_INT:
        if(IbufY[i]==NULL)
          break;
        delete[] IbufY[i];
        IbufY[i]=NULL;
        break;
      case GRAPH_NUMTYPEY_BYTE:
        if(BbufY[i]==NULL)
          break;
        delete[] BbufY[i];
        BbufY[i]=NULL;
        break;
    }
  }

  if(!mode)
  {
    switch(type & GRAPH_NUMTYPEX)
    {
      case GRAPH_NUMTYPEX_FLOAT:
        delete[] FbufX;
        FbufX=NULL;
        break;
      case GRAPH_NUMTYPEX_INT:
        delete[] IbufX;
        IbufX=NULL;
        break;
      case GRAPH_NUMTYPEX_BYTE:
        delete[] BbufX;
        BbufX=NULL;
        break;
    }

    switch(type & GRAPH_NUMTYPEY)
    {
      case GRAPH_NUMTYPEY_FLOAT:
        delete[] FbufY;
        FbufY=NULL;
        break;
      case GRAPH_NUMTYPEY_INT:
        delete[] IbufY;
        IbufY=NULL;
        break;
      case GRAPH_NUMTYPEY_BYTE:
        delete[] BbufY;
        BbufY=NULL;
        break;
    }
  }
}

void C_Graph::SetTitle( const char* _title )
{
	Title = _title;
}

void C_Graph::SetType(int _type)
{
  type=_type;
}

void C_Graph::SetVMinY(float _min)
{
  fMinY=_min;
}

void C_Graph::SetVMinY(int _min)
{
  iMinY=_min;
}

void C_Graph::SetVMinY(char _min)
{
  cMinY=_min;
}

void C_Graph::SetVMaxY(float _max)
{
  fMaxY=_max;
}

void C_Graph::SetVMaxY(int _max)
{
  iMaxY=_max;
}

void C_Graph::SetVMaxY(char _max)
{
  cMaxY=_max;
}

void C_Graph::SetVMinX(float _min)
{
  fMinX=_min;
}

void C_Graph::SetVMinX(int _min)
{
  iMinX=_min;
}

void C_Graph::SetVMinX(char _min)
{
  cMinX=_min;
}

void C_Graph::SetVMaxX(float _max)
{
  fMaxX=_max;
}

void C_Graph::SetVMaxX(int _max)
{
  iMaxX=_max;
}

void C_Graph::SetVMaxX(char _max)
{
  cMaxX=_max;
}

void C_Graph::SetNData(int n,int ngr)
{
	ndata[ngr]=n;

	if(totdata<n)
	{
		totdata=n;
	}
}

void C_Graph::SetGraphColor( const GUI_color& color, int ngr )
{
	graphColor[ngr] = color;
}

void C_Graph::SetDataX(float *buf,int ngr)
{
  if(FbufX[ngr]!=NULL)
    delete[] FbufX[ngr];

  FbufX[ngr]=new float[ndata[ngr]];
  memcpy(FbufX[ngr],buf,sizeof(float)*ndata[ngr]);
}

void C_Graph::SetDataY(float *buf,int ngr)
{
  if(FbufY[ngr]!=NULL)
    delete[] FbufY[ngr];
    
  FbufY[ngr]=new float[ndata[ngr]];
  memcpy(FbufY[ngr],buf,sizeof(float)*ndata[ngr]);
}

void C_Graph::SetDataX(int *buf,int ngr)
{
  if(IbufX[ngr]!=NULL)
    delete[] IbufX[ngr];

  IbufX[ngr]=new int[ndata[ngr]];
  memcpy(IbufX[ngr],buf,sizeof(int)*ndata[ngr]);
}

void C_Graph::SetDataY(int *buf,int ngr)
{
  if(IbufY[ngr]!=NULL)
    delete[] IbufY[ngr];
    
  IbufY[ngr]=new int[ndata[ngr]];
  memcpy(IbufY[ngr],buf,sizeof(int)*ndata[ngr]);
}

void C_Graph::SetDataX(char *buf,int ngr)
{
  if(BbufX[ngr]!=NULL)
    delete[] BbufX[ngr];

  BbufX[ngr]=new char[ndata[ngr]];
  memcpy(BbufX[ngr],buf,sizeof(char)*ndata[ngr]);
}

void C_Graph::SetDataY(char *buf,int ngr)
{
  if(BbufY[ngr]!=NULL)
    delete[] BbufY[ngr];
    
  BbufY[ngr]=new char[ndata[ngr]];
  memcpy(BbufY[ngr],buf,sizeof(char)*ndata[ngr]);
}


void C_Graph::SetTick(float _yt)
{
  yt=_yt;
}

void C_Graph::SetTick(int _yt)
{
  yt=_yt;
}

void C_Graph::SetTick(char _yt)
{
  yt=_yt;
}

void C_Graph::GetScale(float &xs,float &ys)
{
  xs=scalex;
  ys=scaley;
}

void C_Graph::SetPos(int _px,int _py,int _w,int _h)
{
	if(_h>(y2-2-y1-_py-2))
	{
		_h=y2-2-y1-2-_py;
	}

	if(_w>(x2-2-x1-_px-2))
	{
		_w=x2-2-x1-2-_px;
	}

	px = _px;
	py = _py;
	w = _w;
	h = _h;

	gx1=((x1+2+px)*GUI_CharW())+4;
	gy1=((y1+2+py)*GUI_CharH());
	gx2=gx1+(w-1)*GUI_CharW()-8;
	gy2=gy1+(h-1)*GUI_CharH();
}

void C_Graph::Show(void)
{
	int nt=0;
	
	float MinY;
	float MinX;
	
	if(!(type & GRAPH_AXISTYPE_XY))
	{
		if(type & GRAPH_DRAWTYPE_HISTOGRAM)
		{
			int tmp=(gx2-gx1)/totdata;
			scalex=tmp;
		}
		else
		{
			scalex=((float)gx2-gx1)/((float)totdata-1);
		}
   	}
	else
	{
		switch(type & GRAPH_NUMTYPEX)
		{
			case GRAPH_NUMTYPEX_FLOAT:
				scalex=(gx2-gx1)/(fMaxX-fMinX);
				MinX=fMinX;
				break;
			case GRAPH_NUMTYPEX_INT:
				scalex=((float)(gx2-gx1))/((float)(iMaxX-iMinX));
				MinX=iMinX;
				break;
			case GRAPH_NUMTYPEX_BYTE:
				scalex=((float)(gx2-gx1))/(float)((cMaxX-cMinX));
				MinX=cMinX;
				break;
		}
   	}

	switch(type & GRAPH_NUMTYPEY)
	{
		case GRAPH_NUMTYPEY_FLOAT:
			scaley=(gy2-gy1)/(fMaxY-fMinY);
			nt=int((fMaxY-fMinY)/yt);
			MinY=fMinY;
		break;
		case GRAPH_NUMTYPEY_INT:
			scaley=((float)(gy2-gy1))/((float)(iMaxY-iMinY));
			nt=int((iMaxY-iMinY)/yt);
			MinY=iMinY;
		break;
		case GRAPH_NUMTYPEY_BYTE:
			scaley=((float)(gy2-gy1))/((float)(cMaxY-cMinY));
			nt=int((cMaxY-cMinY)/yt);
			MinY=cMinY;
		break;
	}

	GUI_Freeze_Locker lock;

	if(Q_graph==NULL)
	{
		Q_graph = new CWindow(x1,y1,x2,y2,Title);
		Q_graph->Show();
	}
	
	//GF_TEMP
	int X1 = Q_graph->GetX()/GUI_CharW();
	int Y1 = Q_graph->GetY()/GUI_CharH();

	x1 = X1;
	y1 = Y1;

	gx1 = ((X1+2+px)*GUI_CharW())+4;
	gy1 = ((Y1+2+py)*GUI_CharH());
	gx2 = gx1+(w-1)*GUI_CharW()-8;
	gy2 = gy1+(h-1)*GUI_CharH();


	//G_bluepanel(px+x1+2,py+y1+2,px+w+x1+2,py+h+y1+2);


	if( yt != 0 )
	{
		for(int i=1;i<=nt;i++)
			GUI_Line( gx1,gy2-int(scaley*i*yt),gx2,gy2-int(scaley*i*yt), GUI_color(GR_CYAN) );
	}
		
	//disegna assi
	GUI_VLine( gx1, gy1, gy2, GUI_color(GR_BLACK) );
	GUI_HLine( gx1, gx2, gy2, GUI_color(GR_BLACK) );

	int xstart,xend,ystart,yend;   

	float datax[2],datay[2];

	for(int j=0;j<ngraph;j++)
	{
		int nend = 0;
	
		switch(type & GRAPH_DRAWTYPE_MASK)
		{
			case GRAPH_DRAWTYPE_LINE:
				nend=ndata[j]-1;
				break;
			case GRAPH_DRAWTYPE_DOTS:
			case GRAPH_DRAWTYPE_HISTOGRAM:
				nend=ndata[j];
				break;
		}

		for( int i = 0; i < nend; i++ )
		{
			if(type & GRAPH_AXISTYPE_XY)
			{
				switch(type & GRAPH_NUMTYPEX)
				{
					case GRAPH_NUMTYPEX_FLOAT:
						datax[0]=FbufX[j][i];
						datax[1]=FbufX[j][i+1];
					break;
					case GRAPH_NUMTYPEX_INT:
						datax[0]=IbufX[j][i];
						datax[1]=IbufX[j][i+1];
					break;
					case GRAPH_NUMTYPEX_BYTE:
						datax[0]=BbufX[j][i];
						datax[1]=BbufX[j][i+1];
					break;
				}
			}
			else
			{
				datax[0]=i;
				datax[1]=i+1;
			}
       

			switch(type & GRAPH_NUMTYPEY)
			{
				case GRAPH_NUMTYPEY_FLOAT:
					datay[0]=FbufY[j][i];
					datay[1]=FbufY[j][i+1];
					break;
				case GRAPH_NUMTYPEY_INT:
					datay[0]=IbufY[j][i];
					datay[1]=IbufY[j][i+1];
					break;
				case GRAPH_NUMTYPEY_BYTE:
					datay[0]=BbufY[j][i];
					datay[1]=BbufY[j][i+1];
					break;
			}

			xstart=gx1+int((datax[0]-MinX)*scalex);
			xend=gx1+int((datax[1]-MinX)*scalex);
		
			ystart=gy2-int(scaley*(datay[0]-MinY));
			yend=gy2-int(scaley*(datay[1]-MinY));
		
			switch(type & GRAPH_DRAWTYPE_MASK)
			{
				case GRAPH_DRAWTYPE_LINE:
					GUI_Line( xstart, ystart, xend, yend, graphColor[j] );
					break;
				case GRAPH_DRAWTYPE_DOTS:
					GUI_PutPixel( xstart, ystart, graphColor[j] );
					break;
				case GRAPH_DRAWTYPE_HISTOGRAM:
					GUI_FillRect( RectI( xstart+1, ystart, xend-xstart, gy2-ystart ), graphColor[j] );
					break;
			}
     	}
   	}

	if(!(type & GRAPH_NOSTART_SHOWCURSOR))
	{
		col=0;
		row=0;
		DrawCursors();
	}

	shown = true;
}

void C_Graph::SetGraphScale( float minX, float maxX, float minY, float maxY, int ngr )
{
	if( ngr < 0 || ngr >= 10 )
		return;

	if( !(type & GRAPH_AXISTYPE_XY) )
	{
		return;
	}

	_scalex[ngr] = (gx2-gx1)/(maxX-minX);
	_minX[ngr] = minX;

	_scaley[ngr] = (gy2-gy1)/(maxY-minY);
	_minY[ngr] = minY;
	_maxY[ngr] = maxY;
}

void C_Graph::PrintMsgLow( int y, char* msg )
{
	int txtX = 3;
	int txtY = py + h + 1 + y;

	Q_graph->DrawText( txtX, txtY, msg, GUI_SmallFont, GUI_color( WIN_COL_CLIENTAREA ), GUI_color( WIN_COL_TXT ) );
}

void C_Graph::DrawVerticalLine(float pos,const GUI_color& colour)
{
	GUI_Freeze_Locker lock;

	float mx = 0.f;

	switch(type & GRAPH_NUMTYPEX)
	{
		case GRAPH_NUMTYPEX_FLOAT:
			mx=fMinX;
			break;
		case GRAPH_NUMTYPEX_INT:
			mx=iMinX;
			break;
		case GRAPH_NUMTYPEX_BYTE:
			mx=cMinX;
			break;
	}

	int pixel_pos = (pos - mx) * scalex;
	GUI_VLine( gx1 + pixel_pos, gy1, gy2, colour );

	DrawCursors();
}

void C_Graph::DrawHorizontalLine(float pos,const GUI_color& colour)
{
	GUI_Freeze_Locker lock;

	float my = 0.f;
	
	switch(type & GRAPH_NUMTYPEY)
	{
		case GRAPH_NUMTYPEY_FLOAT:
			my=fMinY;
			break;
		case GRAPH_NUMTYPEY_INT:
			my=iMinY;
			break;
		case GRAPH_NUMTYPEY_BYTE:
			my=cMinY;
			break;
	}

	int pixel_pos = (pos - my) * scaley;
	GUI_HLine( gx1, gx2, gy2 - pixel_pos, colour );

	DrawCursors();
}

void C_Graph::DrawCursors()
{
	GUI_Freeze_Locker lock;

	vidbufX = GUI_SaveScreen( RectI( gx1+col, gy1, 1, gy2-gy1+1 ) );
	vidbufY = GUI_SaveScreen( RectI( gx1, gy2-row, gx2-gx1+1, 1 ) );

	GUI_VLine( gx1+col, gy1, gy2, GUI_color(80,80,80) );
	GUI_HLine( gx1, gx2, gy2-row, GUI_color(80,80,80) );

	float mx,my,My;
	switch(type & GRAPH_NUMTYPEX)
	{
		case GRAPH_NUMTYPEX_FLOAT:
			mx=fMinX;
			break;
		case GRAPH_NUMTYPEX_INT:
			mx=iMinX;
			break;
		case GRAPH_NUMTYPEX_BYTE:
			mx=cMinX;
			break;
	}
	
	switch(type & GRAPH_NUMTYPEY)
	{
		case GRAPH_NUMTYPEY_FLOAT:
			my = fMinY;
			My = fMaxY;
			break;
		case GRAPH_NUMTYPEY_INT:
			my = iMinY;
			My = iMaxY;
			break;
		case GRAPH_NUMTYPEY_BYTE:
			my = cMinY;
			My = cMaxY;
			break;
	}

	if(type & GRAPH_SHOWPOSTXT)
	{
		int txtX = 3;
		int txtY = py + h + 1;

		char buf[80];
		snprintf( buf, sizeof(buf), "%7.3f, %7.3f     [ %6.3f, %6.3f ]    ", mx+col/scalex, my+row/scaley, my, My );
		Q_graph->DrawText( txtX, txtY, buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), graphColor[0] );

		//TODO: da sistemare tutta la classe per avere grafici decenti !
		for( int j = 2; j < ngraph; j++ )
		{
			if( ndata[j] )
			{
				snprintf( buf, sizeof(buf), "%7.3f, %7.3f     [ %6.3f, %6.3f ]    ", _minX[j]+col/_scalex[j], _minY[j]+row/_scaley[j], _minY[j], _maxY[j] );
				Q_graph->DrawText( txtX, txtY+j-1, buf, GUI_XSmallFont, GUI_color( WIN_COL_CLIENTAREA ), graphColor[j] );
			}
		}
	}
}

void C_Graph::SetCursorX(int pos)
{
	if( vidbufX )
	{
		GUI_DrawSurface( PointI( gx1+col, gy1 ), vidbufX );
		GUI_FreeSurface( &vidbufX );
	}
	if( vidbufY )
	{
		GUI_DrawSurface( PointI( gx1, gy2-row ), vidbufY );
		GUI_FreeSurface( &vidbufY );
	}

	float px = pos*scalex;
	if((px+gx1)>gx2)
	{
		col=gx2-gx1;
	}
	else
	{
		col=int(px);
	}

	DrawCursors();
}

void C_Graph::SetCursorY(int pos)
{
	if( vidbufX )
	{
		GUI_DrawSurface( PointI( gx1+col, gy1 ), vidbufX );
		GUI_FreeSurface( &vidbufX );
	}
	if( vidbufY )
	{
		GUI_DrawSurface( PointI( gx1, gy2-row ), vidbufY );
		GUI_FreeSurface( &vidbufY );
	}

	float py=pos*scaley;
	if((gy2-py)<gy1)
	{
		row=gy2-gy1;
	}
	else
	{
		row=int(py);
	}

	DrawCursors();
}

void C_Graph::MoveCursorX(int inc)
{
	if(((col+inc)>=0) && ((col+inc+gx1)<gx2))
	{
		if( vidbufX )
		{
			GUI_DrawSurface( PointI( gx1+col, gy1 ), vidbufX );
			GUI_FreeSurface( &vidbufX );
		}
		if( vidbufY )
		{
			GUI_DrawSurface( PointI( gx1, gy2-row ), vidbufY );
			GUI_FreeSurface( &vidbufY );
		}

		col += inc;

		DrawCursors();
	}
}

void C_Graph::MoveCursorY(int inc)
{
	if(((row+inc)>=0) && ((gy2-row-inc)>=gy1))
	{
		if( vidbufX )
		{
			GUI_DrawSurface( PointI( gx1+col, gy1 ), vidbufX );
			GUI_FreeSurface( &vidbufX );
		}
		if( vidbufY )
		{
			GUI_DrawSurface( PointI( gx1, gy2-row ), vidbufY );
			GUI_FreeSurface( &vidbufY );
		}

		row += inc;

		DrawCursors();
	}
}

void C_Graph::GestKey(int c)
{
	SetConfirmRequiredBeforeNextXYMovement(true);
	switch(c)
	{
		case K_LEFT:
			MoveCursorX(-1);
			break;
		case K_RIGHT:
			MoveCursorX(1);
			break;
		case K_CTRL_LEFT:
			MoveCursorX(-10);
			break;
		case K_CTRL_RIGHT:
			MoveCursorX(10);
			break;      
		case K_UP:
			MoveCursorY(1);
			break;
		case K_DOWN:
			MoveCursorY(-1);
			break;
		case K_CTRL_UP:
			MoveCursorY(10);
			break;
		case K_CTRL_DOWN:
			MoveCursorY(-10);
			break;
		default:
			bipbip();
			break;
	}
}
