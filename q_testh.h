/*	  
>>>> Q_TESTH.h

  Header Procedure test hardware.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1997  ++++
++++           Tutti i diritti sono riservati.              ++++
++++   Sviluppo : Loris Gassani     - Massa   - Italy 1997  ++++

*/

#if !defined(__Q_TESTH_)
#define __Q_TESTH_

#include "c_window.h"


#define TESTHW_INPUT  0
#define TESTHW_OUTPUT 1
#define TESTHW_TOGGLE 2
#define TESTHW_FLOAT  4

struct TestHwRows
{
	char **items;
	int nitems;
	float *values;
	int *types;
	char *hotkeys;
	int len;
	float *update_delay;
	unsigned int *timer;  
};

struct TestHwPanels
{
	struct TestHwRows *rows;
	RectI area;
	int x0,x1,y0,y1;
	int drow;
	int width,height;
	int nrows;
	int (*f_Output)(int n,float val);
	int (*f_Input)(int n,float &val);
	int type;
	char *title;
};


class WindTestHw: public CWindow
{
protected:

	TestHwPanels *Panels;

	int hx,hy,lx,ly,oy;
	int borderx,bordery;
	int rowdelta;
	int npanels;
	int nhotk;

	int  SearchHotKey(int c,int np,int &ni,int &nr,int &idx);
	void Gest_OutputPanel(int c);
	void RefreshInput(int np);
	void ShowPanel(int pan,int mode);
	void ShowRow(int np,int nr);

public:
	WindTestHw(int _Hx,int _Hy,int _Lx,int _Ly,int _np,int _bx,int _by,const char *_Title);
	~WindTestHw(void);

	//SMOD120504
	void Set_UpdateDelayItem(int np,int nr,int nitem,int delay);

	void SetRow(int np,int nr,int nitems);
	void SetRowItem(int np,int nr,int nitem,const char *text,int type=0);
	void SetPanel(int _np,int _nrows,int offy,int drow,int type=TESTHW_INPUT,const char* _title=NULL);
	void Set_OutputF(int np,int(*f)(int n,float val));
	void Set_InputF(int np,int(*f)(int n,float &val));

	int Activate(void);
};


// prototipi delle funzioni definite in Q_TESTH.CPP

// Main test hardware schede
void Test_hardw(void);

// Test CPU
int Test_Cpu(int ncicli);

void TestExtCam(void);

#endif



