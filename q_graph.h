/*
>>>> Q_GRAPH.H


INCLUDE per funzioni grafiche di supporto (DJGPP)


++++            Modulo di automazione QUADRA.                  ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1995/99  ++++
++++           Tutti i diritti sono riservati.                 ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1999       ++++

*/


#ifndef __q_graph
#define __q_graph

#include "c_window.h"


/***************************************************************************

Funzioni di supporto nuova interfaccia grafica.

****************************************************************************/


#define GRAPH_NUMTYPEX       0x03
#define GRAPH_NUMTYPEY       0x0C

#define GRAPH_NUMTYPEX_FLOAT     0x00
#define GRAPH_NUMTYPEX_INT       0x01
#define GRAPH_NUMTYPEX_BYTE      0x02

#define GRAPH_NUMTYPEY_FLOAT     0x04
#define GRAPH_NUMTYPEY_INT       0x08
#define GRAPH_NUMTYPEY_BYTE      0x0C

#define GRAPH_AXISTYPE_XY        0x10
#define GRAPH_AXISTYPE_NORMAL    0x00

#define GRAPH_SHOWPOSTXT         0x40

#define GRAPH_DRAWTYPE_MASK      0x180
#define GRAPH_DRAWTYPE_HISTOGRAM 0x100
#define GRAPH_DRAWTYPE_LINE      0x80
#define GRAPH_DRAWTYPE_DOTS      0x00

#define GRAPH_NOSTART_SHOWCURSOR 0x200

#define GRAPH_POS 5,3,76,22

class C_Graph
{
  protected:
    int x1,x2,y1,y2;
    int gx1,gx2,gy1,gy2;
    int px,py,w,h;
    int type;
    int col,row;
    
    std::string Title;
    float **FbufX,**FbufY;
    int   **IbufX,**IbufY;
    char  **BbufX,**BbufY;

    void *vidbufX,*vidbufY;
    int externalWind;

    CWindow* Q_graph;

	float fMinX,fMinY,fMaxX,fMaxY;
	int iMinX,iMinY,iMaxX,iMaxY;
	char cMinX,cMinY,cMaxX,cMaxY;

	float yt;

	int* ndata;
	int totdata;
	int ngraph;

	float scalex,scaley;
	//GF: aggiunti per display valori grafici secondari
	float _scalex[10], _scaley[10];
	float _minX[10], _minY[10], _maxY[10];


	GUI_color* graphColor;
	
	bool shown;

  public:
    C_Graph(int _x1,int _y1,int _x2,int _y2,const char *_title,int _type,int ngr);
    ~C_Graph(void);

    void ResetGraphData(int=0);

    void SetTitle( const char* _title );
    void SetType(int _type);

    void SetVMinY(float _min);
    void SetVMinY(int _min);
    void SetVMinY(char _min);

    void SetVMinX(float _min);
    void SetVMinX(int _min);
    void SetVMinX(char _min);

    void SetVMaxY(float _max);
    void SetVMaxY(int _max);
    void SetVMaxY(char _max);

    void SetVMaxX(float _max);
    void SetVMaxX(int _max);
    void SetVMaxX(char _max);

    void SetNData(int n,int ngr);
    void SetGraphColor( const GUI_color& color, int ngr );
    
    void SetDataX(char *buf,int ngr);
    void SetDataY(char *buf,int ngr);
    void SetDataX(int *buf,int ngr);
    void SetDataY(int *buf,int ngr);
    void SetDataX(float *buf,int ngr);
    void SetDataY(float *buf,int ngr);

    void SetTick(float _yt);
    void SetTick(int _yt);
    void SetTick(char _yt);

    void GetScale(float &xs,float &ys);
	
	void DrawVerticalLine( float pos, const GUI_color& colour );
	void DrawHorizontalLine( float pos, const GUI_color& colour );

    void SetPos(int px,int py,int w,int h);
    void Show(void);
    void MoveCursorX(int inc);
    void MoveCursorY(int inc);
    void SetCursorX(int pos);
    void SetCursorY(int pos);    
    void DrawCursors(void);
    void PrintMsgLow( int y, char* msg );
    void GestKey(int c);

    void SetGraphScale( float minX, float maxX, float minY, float maxY, int ngr );
};

#endif
