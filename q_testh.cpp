/*
>>>> Q_TESTH.CPP

Funzioni test hardware elettronica seriale.

++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara - Italy 1997  ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1997    ++++
++++    Sviluppo : Loris Gassani  - Massa   - Italy 1997    ++++
++++    Mod.: W042000 x nuova interf. grafica - 2000
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#include "gui_functions.h"
#include "gui_defs.h"
#include "filefn.h"
#include "msglist.h"
#include "q_cost.h"
#include "q_graph.h"
#include "q_gener.h"
#include "q_wind.h"
#include "q_help.h"
#include "q_tabe.h"
#include "q_testh.h"
#include "q_cost.h"
#include "q_grcol.h"

#include "q_ser.h"
#include "q_oper.h"
#include "commclass.h"
#include "q_fox.h"
#include "q_dosat.h"
#include "q_carint.h"
#include <unistd.h>  //GNU

#include "keyutils.h"
#include "datetime.h"
#include "lnxdefs.h"
#include "q_vision.h"

#include <mss.h>


#define	 CARBRD		MsgGetString(Msg_00607)
#define	 HEADBRD	MsgGetString(Msg_00608)
#define	 CPUBRD		MsgGetString(Msg_00609)
#define	 PACK 		MsgGetString(Msg_00631)
#define	 FOXBRD		MsgGetString(Msg_01321)
#define	 OUTBRD		MsgGetString(Msg_00613)
#define	 INBRD		MsgGetString(Msg_00614)

#define	 NOERR		0
#define	 MINERR		1
#define	 MAXERR		2


extern struct CfgHeader QHeader;
extern struct cur_data CurDat;

char AllHotkList[37]="1234567890QWERTYUIOPASDFGHJKLZXCVBNM";


#define TESTHW_DELTATEXT 3
#define TESTHW_SPACEX    3

#define TESTHW_REFRESHPAN 1
#define TESTHW_SHOWPAN    0


WindTestHw::WindTestHw(int _Hx,int _Hy,int _Lx,int _Ly,int _np,int _bx,int _by,const char *_Title):CWindow(_Hx,_Hy,_Lx,_Ly,_Title)
{
	hx=_Hx;
	hy=_Hy;
	lx=_Lx;
	ly=_Ly;
	borderx=_bx;
	bordery=_by;
	
	nhotk=0;
	
	npanels=_np;
	Panels=new struct TestHwPanels[npanels];
	memset((char *)Panels,0,npanels*sizeof(struct TestHwPanels));

}

WindTestHw::~WindTestHw()
{
	for(int k=0;k<npanels;k++)
	{
		int idx=0;
	
		for(int i=0;i<Panels[k].nrows;i++)
		{
			for(int j=0;j<Panels[k].rows[i].nitems;j++)
			{
				if((Panels[k].type & TESTHW_OUTPUT) && (Panels[k].f_Output!=NULL))
					Panels[k].f_Output(idx,0);
				idx++;
		
				delete[] Panels[k].rows[i].items[j];
			}
	
			delete[] Panels[k].rows[i].update_delay;
			delete[] Panels[k].rows[i].timer;
			delete[] Panels[k].rows[i].items;
			delete[] Panels[k].rows[i].values;
			delete[] Panels[k].rows[i].hotkeys;
			delete[] Panels[k].rows[i].types;
		
		}
		delete[] Panels[k].rows;
	
		if(Panels[k].title!=NULL)
		{
			delete[] Panels[k].title;
		}
	}

	delete[] Panels;
}

void WindTestHw::SetPanel(int pan,int _nrows,int offy,int drow,int _type,const char* _title)
{
	Panels[pan].rows=new struct TestHwRows[_nrows];
	
	Panels[pan].nrows=_nrows;

	Panels[pan].area.Y = offy;
	Panels[pan].area.H = _nrows*drow+2;

	Panels[pan].x0 = hx + borderx;
	Panels[pan].x1 = lx - borderx;
	
	Panels[pan].y0 =hy+bordery+offy;
	Panels[pan].y1 =hy+bordery+offy+Panels[pan].nrows*drow;
	
	Panels[pan].drow=drow;
	
	Panels[pan].width=Panels[pan].x1-Panels[pan].x0;
	Panels[pan].height=Panels[pan].y1-Panels[pan].y0;
	
	Panels[pan].f_Input=NULL;
	Panels[pan].f_Output=NULL;
	
	Panels[pan].type=_type;
	
	if(_title!=NULL)
	{
		Panels[pan].title=new char[strlen(_title)+1];
		strcpy(Panels[pan].title,_title);
	}
	else
	{
		Panels[pan].title=NULL;
	}
}

void WindTestHw::SetRow(int np,int nr,int nitems)
{
	if(nr<Panels[np].nrows)
	{
		Panels[np].rows[nr].items=new char* [nitems];
		Panels[np].rows[nr].values=new float[nitems];
		Panels[np].rows[nr].hotkeys=new char[nitems];
		Panels[np].rows[nr].nitems=nitems;
		Panels[np].rows[nr].types=new int[nitems];
		Panels[np].rows[nr].update_delay=new float[nitems];
		Panels[np].rows[nr].timer=new unsigned int[nitems];
		
		for(int i=0;i<nitems;i++)
		{
			Panels[np].rows[nr].len=0;
			Panels[np].rows[nr].hotkeys[i]=0;
			Panels[np].rows[nr].items[i]=NULL;
			Panels[np].rows[nr].values[i]=0;
			Panels[np].rows[nr].types[i]=0;
			Panels[np].rows[nr].update_delay[i]=-1;
			Panels[np].rows[nr].timer[i]=0;		
		}
	}
}

void WindTestHw::SetRowItem(int np,int nr,int nitem,const char *text,int type)
{
	if((nr<Panels[np].nrows) && (Panels[np].rows[nr].items[nitem]==NULL))
	{
		Panels[np].rows[nr].len+=strlen(text)+TESTHW_SPACEX;
	
		if(nitem<Panels[np].rows[nr].nitems)
		{
			Panels[np].rows[nr].items[nitem]=new char[strlen(text)+1];
		
			strcpy(Panels[np].rows[nr].items[nitem],text);
		}
	
		Panels[np].rows[nr].types[nitem]=type;
	
		if(Panels[np].type & TESTHW_OUTPUT)
		{
			Panels[np].rows[nr].hotkeys[nitem]=AllHotkList[nhotk++];
		}
	}
}

//SMOD120504
void WindTestHw::Set_UpdateDelayItem(int np,int nr,int nitem,int delay)
{
	if(np>npanels)
	{
		return;
	}
	
	if(nr>Panels[np].nrows)
	{
		return;
	}
	
	if(nitem>Panels[np].rows[nr].nitems)
	{
		return;
	}
	
	Panels[np].rows[nr].update_delay[nitem]=delay;
	Panels[np].rows[nr].timer[nitem]=clock();

}

void WindTestHw::Set_OutputF(int np,int(*f)(int n,float val))
{
	if(Panels[np].type & TESTHW_OUTPUT)
	{
		Panels[np].f_Output=f;
	}
}

void WindTestHw::Set_InputF(int np,int(*f)(int n,float &val))
{
	if(!(Panels[np].type & TESTHW_OUTPUT))
	{
		Panels[np].f_Input=f;
	}
}


int WindTestHw::Activate(void)
{
	CWindow::Show();
	
	int c,ret=1;
	
	for(int i=0;i<npanels;i++)
	{
		ShowPanel(i,TESTHW_SHOWPAN);
	}

	do
	{
		for(int np=0;np<npanels;np++)
		{
			if(!(Panels[np].type & TESTHW_OUTPUT))
			{
				RefreshInput(np);
			}
		}

		SetConfirmRequiredBeforeNextXYMovement(true);

		c = Handle( false );
		if( c != 0 )
		{
			if(c==K_ESC)
			{
				ret=0;
				break;
			}

			if((c>=K_F1) && (c<=K_F12))
			{
				ret=c;
				break;
			}
    
			Gest_OutputPanel(c);
		}
	} while(1);

	return(ret);

}

void WindTestHw::Gest_OutputPanel(int c)
{
	c=toupper(c);
	int ni,nr,idx;
	
	for(int np=0;np<npanels;np++)
	{
		if((Panels[np].f_Output!=NULL) && (SearchHotKey(c,np,ni,nr,idx)))
		{
			if(Panels[np].rows[nr].types[ni] & TESTHW_TOGGLE)
			{
				if(Panels[np].rows[nr].values[ni]==1)
				{
					Panels[np].rows[nr].values[ni]=0;
				}
				else
				{
					Panels[np].rows[nr].values[ni]=1;
				}				
				ShowPanel(np,TESTHW_REFRESHPAN);
				Panels[np].f_Output(idx,Panels[np].rows[nr].values[ni]);
			}
			else
			{
				Panels[np].rows[nr].values[ni]=1;
				ShowPanel(np,TESTHW_REFRESHPAN);
				Panels[np].f_Output(idx,1);
				Panels[np].rows[nr].values[ni]=0;
				Panels[np].f_Output(idx,0);
				ShowPanel(np,TESTHW_REFRESHPAN);
			}
			return;      
		}
	}
	bipbip();
}



int WindTestHw::SearchHotKey(int c,int np,int &ni,int &nr,int &idx)
{
	if(Panels[np].type & TESTHW_OUTPUT)
	{
		int k=0;
		for(int i=0;i<Panels[np].nrows;i++)
		{
			for(int j=0;j<Panels[np].rows[i].nitems;j++)
			{
				if(c==Panels[np].rows[i].hotkeys[j])
				{
					idx=k;
					nr=i;
					ni=j;
					return(1);          
				}
				k++;
			}
		}
	}
	return(0);
}

void WindTestHw::RefreshInput(int np)
{
	if((Panels[np].type & TESTHW_OUTPUT) || (Panels[np].f_Input==NULL))
	{
		return;
	}
	else
	{
		int idx=0;
	
		for(int i=0;i<Panels[np].nrows;i++)
		{
			for(int j=0;j<Panels[np].rows[i].nitems;j++)
			{
				if(Panels[np].rows[i].update_delay[j]!=-1)
				{
					float dt=clock()-Panels[np].rows[i].timer[j];
			
					if((dt*1000/CLOCKS_PER_SEC)>Panels[np].rows[i].update_delay[j])
					{
						Panels[np].f_Input(idx,Panels[np].rows[i].values[j]);
						Panels[np].rows[i].timer[j]=clock();
					}
				}
				else
				{
					Panels[np].f_Input(idx,Panels[np].rows[i].values[j]);
				}
		
				idx++;
			}
		}
		ShowPanel(np,TESTHW_REFRESHPAN);
	}
}

void WindTestHw::ShowPanel(int pan,int mode)
{
	GUI_Freeze_Locker lock;

	if(mode==TESTHW_SHOWPAN)
	{
		Panels[pan].area.X = 2;
		Panels[pan].area.W = GetW()/GUI_CharW() - 2*Panels[pan].area.X;
		DrawPanel( Panels[pan].area );

		if(Panels[pan].title!=NULL)
		{
			DrawPanel( RectI( Panels[pan].area.X, Panels[pan].area.Y-1, Panels[pan].area.W, 1 ), GUI_color(0,0,0), GUI_color(0,0,0) );
			DrawTextCentered( Panels[pan].area.X, Panels[pan].area.X+Panels[pan].area.W, Panels[pan].area.Y-1, Panels[pan].title, GUI_DefaultFont, GUI_color(0,0,0), GUI_color(WIN_COL_SELECTED) );
		}
	}
		
	for(int i=0;i<Panels[pan].nrows;i++)
	{
		ShowRow(pan,i);
	}
}

void WindTestHw::ShowRow(int np,int nr)
{
	char buf[80];

	if(nr<Panels[np].nrows)
	{
		int textX1=Panels[np].x0+(Panels[np].x1-Panels[np].x0-(Panels[np].rows[nr].len+2))/2;
		int textX2=Panels[np].x1-(Panels[np].x1-Panels[np].x0-(Panels[np].rows[nr].len+2))/2;
	
		int delta=(textX2-textX1)/Panels[np].rows[nr].nitems;
	
		for(int i=0;i<Panels[np].rows[nr].nitems;i++)
		{
			if(!(Panels[np].type & TESTHW_OUTPUT))
			{
				if(Panels[np].rows[nr].types[i]==TESTHW_FLOAT)
					snprintf( buf, sizeof(buf),"%5.2f",Panels[np].rows[nr].values[i]);
				else
					snprintf( buf, sizeof(buf),"%5d",(int)Panels[np].rows[nr].values[i]);
				
				int _y = Panels[np].area.Y + nr*Panels[np].drow + 1;

				DrawText( (textX1+delta*i)+(delta-strlen(buf))/2, _y, buf , GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
				DrawText( (textX1+delta*i)+(delta-strlen(Panels[np].rows[nr].items[i]))/2, _y+1,Panels[np].rows[nr].items[i], GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color(GR_YELLOW) );
			}
			else
			{

				buf[0]=Panels[np].rows[nr].hotkeys[i];
				buf[1]=0;

				GUI_color fgColor;
				if(!Panels[np].rows[nr].values[i])
				{
					fgColor = GUI_color(GR_LIGHTGREEN);
				}
				else
				{
					fgColor = GUI_color(GR_LIGHTRED);
				}

				int _y = Panels[np].area.Y + nr*Panels[np].drow + 1;

				DrawText( (textX1+delta*i)+(delta-1)/2, _y, buf, GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), fgColor );
				DrawText( (textX1+delta*i)+(delta-strlen(Panels[np].rows[nr].items[i]))/2, _y+1, Panels[np].rows[nr].items[i], GUI_DefaultFont, GUI_color( WIN_COL_SUBTITLE ), GUI_color( 0,0,0 ) );
			}
		}
	}
}





// variabili globali al modulo

float v_Car[16], v_C24[16], i_Czero[16], i_mot[16], i_inv[16];
float i_rot1[16], i_rot2[16], i_rot3[16], i_rot4[16];
float i_rot5[16], i_rot6[16], i_rot7[16], i_rot8[16];
float v_fox30V, v_fox24V, v_las24V, v_foxVST, v_fox12V, v_fox24S, v_las24S;
float v_CpuDD, v_Cpu12, t_Cpu;
float Cpu_Ver, Cpu_Rev;

int v_car_err, v_c24_err, i_c0_err, i_mot_err[16], i_inv_err[16];
int i_rot1_err[16], i_rot2_err[16], i_rot3_err[16], i_rot4_err[16];
int i_rot5_err[16], i_rot6_err[16], i_rot7_err[16], i_rot8_err[16];
int v_fox30V_err, v_fox24V_err, v_las24V_err, v_foxVST_err;
int v_fox12V_err, v_fox24S_err, v_las24S_err;
int v_cpudd_err, v_cpu12_err, t_cpu_err;

int Test_Caric(int ncicli, int car)
{
	char stato, buf[40];
	int ciclo;

	v_Car[car] = 0;
	v_C24[car] = 0;
	i_Czero[car] = 0;
	i_mot[car] = 0;
	i_inv[car] = 0;
	i_rot1[car] = 0;
	i_rot2[car] = 0;
	i_rot3[car] = 0;
	i_rot4[car] = 0;
	i_rot5[car] = 0;
	i_rot6[car] = 0;
	i_rot7[car] = 0;
	i_rot8[car] = 0;

	for (ciclo=0; ciclo<ncicli; ciclo++)
	{
		stato='W';
		while (stato=='W') {

			flushKeyboardBuffer();

			strcpy(buf,"HC");           // comando
			if (car<10)
				buf[2]=car+'0';        // caricatore (0..9)
			else
				buf[2]=car-10+'A';     // caricatore (10..14)
			buf[3]=0;                   // terminatore

			stato=cpu_cmd(2,buf);
		}

		v_Car[car] += (Hex2Dec(buf[1])+16*Hex2Dec(buf[0]))*COSTV1;
		v_C24[car]  += (Hex2Dec(buf[3])+16*Hex2Dec(buf[2]))*COSTV2;
		i_Czero[car] += (Hex2Dec(buf[5])+16*Hex2Dec(buf[4]))*COSTI1;
		i_mot[car] += (Hex2Dec(buf[7])+16*Hex2Dec(buf[6]))*COSTI1;
		i_inv[car] += (Hex2Dec(buf[9])+16*Hex2Dec(buf[8]))*COSTI1;
		i_rot8[car] += (Hex2Dec(buf[11])+16*Hex2Dec(buf[10]))*COSTI1;
		i_rot7[car] += (Hex2Dec(buf[13])+16*Hex2Dec(buf[12]))*COSTI1;
		i_rot6[car] += (Hex2Dec(buf[15])+16*Hex2Dec(buf[14]))*COSTI1;
		i_rot5[car] += (Hex2Dec(buf[17])+16*Hex2Dec(buf[16]))*COSTI1;
		i_rot4[car] += (Hex2Dec(buf[19])+16*Hex2Dec(buf[18]))*COSTI1;
		i_rot3[car] += (Hex2Dec(buf[21])+16*Hex2Dec(buf[20]))*COSTI1;
		i_rot2[car] += (Hex2Dec(buf[23])+16*Hex2Dec(buf[22]))*COSTI1;
		i_rot1[car] += (Hex2Dec(buf[25])+16*Hex2Dec(buf[24]))*COSTI1;
		delay(10);
	}

	v_Car[car] = v_Car[car]/ncicli;
	v_C24[car] = v_C24[car]/ncicli;
	i_Czero[car] = i_Czero[car]/ncicli;
	i_mot[car] = i_mot[car]/ncicli;
	i_inv[car] = i_inv[car]/ncicli;
	i_rot1[car] = i_rot1[car]/ncicli;
	i_rot2[car] = i_rot2[car]/ncicli;
	i_rot3[car] = i_rot3[car]/ncicli;
	i_rot4[car] = i_rot4[car]/ncicli;
	i_rot5[car] = i_rot5[car]/ncicli;
	i_rot6[car] = i_rot6[car]/ncicli;
	i_rot7[car] = i_rot7[car]/ncicli;
	i_rot8[car] = i_rot8[car]/ncicli;

	return 1;
} // Test_Caric


int Test_Head(int ncicli) {

	char stato, buf[40];
	int ciclo;

	v_fox30V = 0;
	v_foxVST = 0;
	v_fox24V = 0;
	v_fox12V = 0;
	v_las24V = 0;
	v_fox24S = 0;
	v_las24S = 0;

	for (ciclo=0; ciclo<ncicli; ciclo++) {

	  stato='W';
	  while (stato=='W') {
		  flushKeyboardBuffer();

		strcpy(buf,"HH");           // comando
		stato=cpu_cmd(2,buf);

	  }

	  v_fox30V += (Hex2Dec(buf[1])+16*Hex2Dec(buf[0]))*COSTV4;
	  v_foxVST += (Hex2Dec(buf[3])+16*Hex2Dec(buf[2]))*COSTV4;
	  v_fox12V += (Hex2Dec(buf[5])+16*Hex2Dec(buf[4]))*COSTV4;
	  v_fox24V += (Hex2Dec(buf[7])+16*Hex2Dec(buf[6]))*COSTV4;
	  v_las24V += (Hex2Dec(buf[9])+16*Hex2Dec(buf[8]))*COSTV4;
	  v_fox24S += (Hex2Dec(buf[11])+16*Hex2Dec(buf[10]))*COSTV4;
	  v_las24S += (Hex2Dec(buf[13])+16*Hex2Dec(buf[12]))*COSTV4;
	  delay(10);
	}

	v_fox30V = v_fox30V/ncicli;
	v_foxVST = v_foxVST/ncicli;
	v_fox12V = v_fox12V/ncicli;
	v_fox24V = v_fox24V/ncicli;
	v_las24V = v_las24V/ncicli;
	v_fox24S = v_fox24S/ncicli;
	v_las24S = v_las24S/ncicli;

} // Test_Head

int Test_Cpu(int ncicli) {

	char buf[40];
	int ciclo, status;

	v_CpuDD = 0;
	v_Cpu12  = 0;
	t_Cpu = 0;

	for (ciclo=0; ciclo<ncicli; ciclo++) {

	  strcpy(buf,"HM");           // comando

	  status=cpu_cmd(2,buf);             // go

	  Cpu_Ver = Hex2Dec(buf[1])+16*Hex2Dec(buf[0]);
	  Cpu_Rev = Hex2Dec(buf[3])+16*Hex2Dec(buf[2]);
	  v_CpuDD += (Hex2Dec(buf[5])+16*Hex2Dec(buf[4]))*COSTV1;
	  v_Cpu12 += (Hex2Dec(buf[7])+16*Hex2Dec(buf[6]))*COSTV3;
     t_Cpu += ((Hex2Dec(buf[9])+16*Hex2Dec(buf[8]))*COSTT1)+COSTT2;
	 delay(10);
	}

	v_CpuDD = v_CpuDD/ncicli;
	v_Cpu12 = v_Cpu12/ncicli;
	t_Cpu = t_Cpu/ncicli;
   return status;
} // Test_Cpu

void Range_hardw(void)
{
	int car;

	v_car_err=NOERR;
	if (v_Car[0]<VINMIN) v_car_err=MINERR;
	if (v_Car[0]>VINMAX) v_car_err=MAXERR;

	v_c24_err=NOERR;
	if (v_C24[0]<VC24MIN) v_c24_err=MINERR;
	if (v_C24[0]>VC24MAX) v_c24_err=MAXERR;

	i_c0_err=NOERR;
	if (i_Czero[0]<IC0MIN) i_c0_err=MINERR;
	if (i_Czero[0]>IC0MAX) i_c0_err=MAXERR;

	v_fox30V_err=NOERR;
	if (v_fox30V<V30MIN) v_fox30V_err=MINERR;
	if (v_fox30V>V30MAX) v_fox30V_err=MAXERR;

	v_foxVST_err=NOERR;
	if (v_foxVST<V30MIN) v_foxVST_err=MINERR;
	if (v_foxVST>V30MAX) v_foxVST_err=MAXERR;

	v_fox24V_err=NOERR;
	if (v_fox24V<V24MIN) v_fox24V_err=MINERR;
	if (v_fox24V>V24MAX) v_fox24V_err=MAXERR;

	v_fox12V_err=NOERR;
	if (v_fox12V<V12MIN) v_fox12V_err=MINERR;
	if (v_fox12V>V12MAX) v_fox12V_err=MAXERR;

	v_las24V_err=NOERR;
	if (v_las24V<V24MIN) v_las24V_err=MINERR;
	if (v_las24V>V24MAX) v_las24V_err=MAXERR;

	v_fox24S_err=NOERR;
	if (v_fox24S<V24MIN) v_fox24S_err=MINERR;
	if (v_fox24S>V24MAX) v_fox24S_err=MAXERR;

	v_las24S_err=NOERR;
	if (v_las24S<V24MIN) v_las24S_err=MINERR;
	if (v_las24S>V24MAX) v_las24S_err=MAXERR;

	v_cpudd_err=NOERR;
	if (v_CpuDD<VINMIN) v_cpudd_err=MINERR;
	if (v_CpuDD>VINMAX) v_cpudd_err=MAXERR;

	v_cpu12_err=NOERR;
	if (v_Cpu12<VP12MIN) v_cpu12_err=MINERR;
	if (v_Cpu12>VP12MAX) v_cpu12_err=MAXERR;

	t_cpu_err=NOERR;
	if (t_Cpu<TPMIN) t_cpu_err=MINERR;
	if (t_Cpu>TPMAX) t_cpu_err=MAXERR;

	for (car=0;car<15;car++) {

		i_rot1_err[car]=NOERR;
		if (i_rot1[car]<IROTMIN) i_rot1_err[car]=MINERR;
		if (i_rot1[car]>IROTMAX) i_rot1_err[car]=MAXERR;

		i_rot2_err[car]=NOERR;
		if (i_rot2[car]<IROTMIN) i_rot2_err[car]=MINERR;
		if (i_rot2[car]>IROTMAX) i_rot2_err[car]=MAXERR;

		i_rot3_err[car]=NOERR;
		if (i_rot3[car]<IROTMIN) i_rot3_err[car]=MINERR;
		if (i_rot3[car]>IROTMAX) i_rot3_err[car]=MAXERR;

		i_rot4_err[car]=NOERR;
		if (i_rot4[car]<IROTMIN) i_rot4_err[car]=MINERR;
		if (i_rot4[car]>IROTMAX) i_rot4_err[car]=MAXERR;

		i_rot5_err[car]=NOERR;
		if (i_rot5[car]<IROTMIN) i_rot5_err[car]=MINERR;
		if (i_rot5[car]>IROTMAX) i_rot5_err[car]=MAXERR;

		i_rot6_err[car]=NOERR;
		if (i_rot6[car]<IROTMIN) i_rot6_err[car]=MINERR;
		if (i_rot6[car]>IROTMAX) i_rot6_err[car]=MAXERR;

		i_rot7_err[car]=NOERR;
		if (i_rot7[car]<IROTMIN) i_rot7_err[car]=MINERR;
		if (i_rot7[car]>IROTMAX) i_rot7_err[car]=MAXERR;

		i_rot8_err[car]=NOERR;
		if (i_rot8[car]<IROTMIN) i_rot8_err[car]=MINERR;
		if (i_rot8[car]>IROTMAX) i_rot8_err[car]=MAXERR;

		i_mot_err[car]=NOERR;
		if (i_mot[car]<IMOTMIN) i_mot_err[car]=MINERR;
		if (i_mot[car]>IMOTMAX) i_mot_err[car]=MAXERR;

		i_inv_err[car]=NOERR;
		if (i_inv[car]<IINVMIN) i_inv_err[car]=MINERR;
		if (i_inv[car]>IINVMAX) i_inv_err[car]=MAXERR;

	}
} // Range_hardw

void stampa( CWindow* wind, int errore, int x, int y )
{
	if( errore )
	{
		if ((errore&0x1ff)==0x1ff)
		{
			wind->DrawText( x, y, MsgGetString(Msg_00632), GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_LIGHTMAGENTA) );
		}
		else
		{
			char buf[81];
			snprintf( buf, sizeof(buf), MsgGetString(Msg_00634), errore );
			wind->DrawText( x, y, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_LIGHTRED) );
		}            
	}
	else
	{
		wind->DrawText( x, y, "OK", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_LIGHTGREEN) );
	}
}

// Interpreta i risultati del test hardware
int Test_parse()
{
	int errore;

	char buf[80];

	CWindow* wind = new CWindow( 0 );
	wind->SetStyle( WIN_STYLE_CENTERED_X );
	wind->SetClientAreaSize( 59, 24 );
	wind->SetClientAreaPos( 0, 5 );
	wind->SetTitle( MsgGetString(Msg_00606) );

	GUI_Freeze();

	wind->Show();

	RectI panel1( 2, 1, 55, 22 );
	wind->DrawPanel( panel1 );
	wind->DrawPanel( RectI( panel1.X, panel1.Y, panel1.W, 1 ), GUI_color(0,0,0), GUI_color(0,0,0) );
	wind->DrawTextCentered( panel1.X, panel1.X+panel1.W, panel1.Y, MsgGetString(Msg_00607), GUI_DefaultFont, GUI_color(0,0,0), GUI_color(WIN_COL_SELECTED) );

	// stampa stato scheda CPU
	snprintf( buf, sizeof(buf), "%19s = ", MsgGetString(Msg_00609) );
	wind->DrawText( panel1.X+3, panel1.Y+2, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	errore = (v_cpudd_err != 0) | ((v_cpu12_err!=0)<<1) | ((t_cpu_err!=0)<<2);
	stampa( wind, errore, panel1.X+28, panel1.Y+2 );

	// stampa stato scheda CARICATORI
	snprintf( buf, sizeof(buf),"%19s = ", MsgGetString(Msg_00607) );
	wind->DrawText( panel1.X+3, panel1.Y+3, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	errore = (v_car_err != 0) | ((v_c24_err!=0)<<1) | ((i_c0_err!=0)<<1);
	stampa( wind, errore, panel1.X+28, panel1.Y+3 );

	// stampa stato scheda TESTA
	snprintf( buf, sizeof(buf),"%19s = ", MsgGetString(Msg_00608) );
	wind->DrawText( panel1.X+3, panel1.Y+4, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	errore=v_fox30V_err!=0 | (v_foxVST_err!=0)<<1 | (v_fox24V_err!=0)<<2;
	errore|=(v_fox12V_err!=0)<<3 | (v_las24V_err!=0)<<4 | (v_fox24S_err!=0)<<5 | (v_las24S_err!=0)<<6;
	stampa( wind, errore, panel1.X+28, panel1.Y+4 );

	// stampa stato pacchi CARICATORI
	for( int car = 0; car < 15; car++ )
	{
		snprintf( buf, sizeof(buf),"%12s %2d = ", MsgGetString(Msg_00631), car+1 );
		wind->DrawText( panel1.X+5, panel1.Y+6+car, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
		
		errore=i_rot1_err[car]!=0  | (i_rot2_err[car]!=0)<<1 | (i_rot3_err[car]!=0)<<2 |
			(i_rot4_err[car]!=0)<<3 | (i_rot5_err[car]!=0)<<4 | (i_rot6_err[car]!=0)<<5 |
			(i_rot7_err[car]!=0)<<6 | (i_rot8_err[car]!=0)<<7 |
			(i_mot_err[car]!=0)<<8  | (i_inv_err[car]!=0)<<9;
		stampa( wind, errore, panel1.X+28, panel1.Y+6+car );
	}

	GUI_Thaw();

	int c;
	do
	{
		c=Handle();
	} while((c!=K_ESC) && ((c<K_F1) && (c>K_F12)));

	delete wind;

	if( c != K_ESC )
	{
		return c;
	}
	return 0;
}

// legge i dati relativi allo stato delle schede
void Read_hardw(void)
{
	Test_Head(1);
	Test_Cpu(1);

	for( int i = 0; i < 15; i++ )
	{
		Test_Caric(1,i);
	}
} // Read_hardw

// Crea il file testo con i dati del test hardware.
int Hardw_File(void)
{
	char eof = 26;
	int i, HandleHrd;
	char buff1[80];
	char buff2[80];
	char acapo [3];
	struct date data;

	acapo[0]=13;
	acapo[1]=10;
	acapo[2]=0;

	if(!F_File_Create(HandleHrd, HARDWNAME))
	{
		FilesFunc_close(HandleHrd);
		return 0;
	}

	strcpy(buff1,"                   ");
	strcat(buff1, MsgGetString(Msg_00606));
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	write(HandleHrd, acapo, 2);

	getdate(&data);  // L2905

	snprintf( buff1, sizeof(buff1),"  %02d/%02d/%04d",data.da_day, data.da_mon, data.da_year);
	write(HandleHrd, buff1, strlen(buff1));

	write(HandleHrd, acapo, 2);
	write(HandleHrd, acapo, 2);

	strcpy(buff1,"                   ");
	strcat(buff1, CARBRD);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	write(HandleHrd, acapo, 2);

	strcpy(buff1, "  Vi= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",v_Car[0]);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	strcpy(buff1, "  Vo= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",v_C24[0]);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	strcpy(buff1, "  Io= ");
	snprintf( buff2, sizeof(buff2),"%1.2f",i_Czero[0]);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	write(HandleHrd, acapo, 2);

	strcpy(buff1,"        I1   I2   I3   I4   I5   I6   I7   I8   Im   Ii");
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);

	strcpy(buff1,"       -------------------------------------------------");
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);

	for (i=0; i<15; i++) {

		snprintf( buff1, sizeof(buff1),"  %2d -",i+1);

		snprintf( buff2, sizeof(buff2)," %1.2f",i_rot1[i]);
		strcat(buff1, buff2);
		snprintf( buff2, sizeof(buff2)," %1.2f",i_rot2[i]);
		strcat(buff1, buff2);
		snprintf( buff2, sizeof(buff2)," %1.2f",i_rot3[i]);
		strcat(buff1, buff2);
		snprintf( buff2, sizeof(buff2)," %1.2f",i_rot4[i]);
		strcat(buff1, buff2);
		snprintf( buff2, sizeof(buff2)," %1.2f",i_rot5[i]);
		strcat(buff1, buff2);
		snprintf( buff2, sizeof(buff2)," %1.2f",i_rot6[i]);
		strcat(buff1, buff2);
		snprintf( buff2, sizeof(buff2)," %1.2f",i_rot7[i]);
		strcat(buff1, buff2);
		snprintf( buff2, sizeof(buff2)," %1.2f",i_rot8[i]);
		strcat(buff1, buff2);
		snprintf( buff2, sizeof(buff2)," %1.2f",i_mot[i]);
		strcat(buff1, buff2);
		snprintf( buff2, sizeof(buff2)," %1.2f",i_inv[i]);
		strcat(buff1, buff2);

		write(HandleHrd, buff1, strlen(buff1));
		write(HandleHrd, acapo, 2);
	}
	write(HandleHrd, acapo, 2);

	strcpy(buff1,"                   ");
	strcat(buff1, HEADBRD);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	write(HandleHrd, acapo, 2);

	strcpy(buff1, "Fox30V= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",v_fox30V);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	strcpy(buff1, "FoxVST= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",v_foxVST);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	strcpy(buff1, "Fox24V= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",v_fox24V);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	strcpy(buff1, "Fox12V= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",v_fox12V);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	
	#ifdef __SNIPER
	strcpy(buff1, "Aux24V= ");
	#endif
	
	snprintf( buff2, sizeof(buff2),"%2.1f",v_las24V);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	strcpy(buff1, "Fox24S= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",v_fox24S);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	
	#ifdef __SNIPER
	strcpy(buff1, "Aux24S= ");
	#endif
	
	snprintf( buff2, sizeof(buff2),"%2.1f",v_las24S);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	write(HandleHrd, acapo, 2);

	strcpy(buff1,"                   ");
	strcat(buff1, CPUBRD);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	write(HandleHrd, acapo, 2);

	strcpy(buff1, "  Ver= ");
	snprintf( buff2, sizeof(buff2),"%2.2f",Cpu_Ver+Cpu_Rev/100);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	strcpy(buff1, "   Vi= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",v_CpuDD);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	strcpy(buff1, "  V12= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",v_Cpu12);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);
	strcpy(buff1, "    T= ");
	snprintf( buff2, sizeof(buff2),"%2.1f",t_Cpu);
	strcat(buff1, buff2);
	write(HandleHrd, buff1, strlen(buff1));
	write(HandleHrd, acapo, 2);

	write(HandleHrd,&eof,1);
	FilesFunc_close(HandleHrd);
	return 0;
}// Hardw_File


int Test_page()
{
	GUI_color colore[3];
	colore[0] = GUI_color(GR_LIGHTGREEN);
	colore[1] = GUI_color(GR_LIGHTMAGENTA);
	colore[2] = GUI_color(GR_LIGHTRED);

	char buf[80];

	Read_hardw();
	Range_hardw();
	Hardw_File();

	CWindow* wind = new CWindow( 0 );
	wind->SetStyle( WIN_STYLE_CENTERED_X );
	wind->SetClientAreaSize( 78, 21 );
	wind->SetClientAreaPos( 0, 5 );
	wind->SetTitle( MsgGetString(Msg_00606) );

	GUI_Freeze();

	wind->Show();

	RectI panel1( 2, 1, 55, 19 );
	RectI panel2( 59, 1, 17, 10 );
	RectI panel3( 59, 12, 17, 7 );

	wind->DrawPanel( panel1 );
	wind->DrawPanel( panel2 );
	wind->DrawPanel( panel3 );

	wind->DrawPanel( RectI( panel1.X, panel1.Y, panel1.W, 1 ), GUI_color(0,0,0), GUI_color(0,0,0) );
	wind->DrawTextCentered( panel1.X, panel1.X+panel1.W, panel1.Y, MsgGetString(Msg_00607), GUI_DefaultFont, GUI_color(0,0,0), GUI_color(WIN_COL_SELECTED) );
	wind->DrawPanel( RectI( panel2.X, panel2.Y, panel2.W, 1 ), GUI_color(0,0,0), GUI_color(0,0,0) );
	wind->DrawTextCentered( panel2.X, panel2.X+panel2.W, panel2.Y, MsgGetString(Msg_00608), GUI_DefaultFont, GUI_color(0,0,0), GUI_color(WIN_COL_SELECTED) );
	wind->DrawPanel( RectI( panel3.X, panel3.Y, panel3.W, 1 ), GUI_color(0,0,0), GUI_color(0,0,0) );
	wind->DrawTextCentered( panel3.X, panel3.X+panel3.W, panel3.Y, MsgGetString(Msg_00609), GUI_DefaultFont, GUI_color(0,0,0), GUI_color(WIN_COL_SELECTED) );

	wind->DrawText( panel1.X+5, panel1.Y+1, "       Vi =       Vo =       Io =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	wind->DrawText( panel1.X+4, panel1.Y+2, "  I1   I2   I3   I4   I5   I6   I7   I8   Im   Ii", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	for( int i = 1; i < 16; i++ )
	{
		if( i < 10 )
		{
			snprintf( buf, sizeof(buf), " %d.", i );
		}
		else
		{
			snprintf( buf, sizeof(buf), "%d.", i );
		}

		wind->DrawText( panel1.X+1, panel1.Y+2+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	}

	wind->DrawText( panel2.X+2, panel2.Y+2, "Fox30V =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	wind->DrawText( panel2.X+2, panel2.Y+3, "FoxVST =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	wind->DrawText( panel2.X+2, panel2.Y+4, "Fox24V =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	wind->DrawText( panel2.X+2, panel2.Y+5, "Fox12V =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	#ifdef __SNIPER
	wind->DrawText( panel2.X+2, panel2.Y+6, "Aux24V =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	#endif
	wind->DrawText( panel2.X+2, panel2.Y+7, "Fox24S =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	#ifdef __SNIPER
	wind->DrawText( panel2.X+2, panel2.Y+8, "Aux24S =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	#endif

	wind->DrawText( panel3.X+1, panel3.Y+2, " Ver =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	wind->DrawText( panel3.X+1, panel3.Y+3, "  Vi =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	wind->DrawText( panel3.X+1, panel3.Y+4, " V12 =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );
	wind->DrawText( panel3.X+1, panel3.Y+5, "   T =", GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_YELLOW) );


	snprintf( buf, sizeof(buf), "%2.1f", v_Car[0] );
	wind->DrawText( panel1.X+17, panel1.Y+1, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_car_err] );
	snprintf( buf, sizeof(buf), "%2.1f", v_C24[0] );
	wind->DrawText( panel1.X+28, panel1.Y+1, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_c24_err] );
	snprintf( buf, sizeof(buf), "%2.1f", i_Czero[0] );
	wind->DrawText( panel1.X+39, panel1.Y+1, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_c0_err] );

	for( int i = 0; i < 15; i++ )
	{
		snprintf( buf, sizeof(buf),"%1.2f",i_rot1[i]);
		wind->DrawText( panel1.X+5, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_rot1_err[i]] );
		snprintf( buf, sizeof(buf),"%1.2f",i_rot2[i]);
		wind->DrawText( panel1.X+10, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_rot2_err[i]] );
		snprintf( buf, sizeof(buf),"%1.2f",i_rot3[i]);
		wind->DrawText( panel1.X+15, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_rot3_err[i]] );
		snprintf( buf, sizeof(buf),"%1.2f",i_rot4[i]);
		wind->DrawText( panel1.X+20, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_rot4_err[i]] );
		snprintf( buf, sizeof(buf),"%1.2f",i_rot5[i]);
		wind->DrawText( panel1.X+25, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_rot5_err[i]] );
		snprintf( buf, sizeof(buf),"%1.2f",i_rot6[i]);
		wind->DrawText( panel1.X+30, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_rot6_err[i]] );
		snprintf( buf, sizeof(buf),"%1.2f",i_rot7[i]);
		wind->DrawText( panel1.X+35, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_rot7_err[i]] );
		snprintf( buf, sizeof(buf),"%1.2f",i_rot8[i]);
		wind->DrawText( panel1.X+40, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_rot8_err[i]] );
		snprintf( buf, sizeof(buf),"%1.2f",i_mot[i]);
		wind->DrawText( panel1.X+45, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_mot_err[i]] );
		snprintf( buf, sizeof(buf),"%1.2f",i_inv[i]);
		wind->DrawText( panel1.X+50, panel1.Y+3+i, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[i_inv_err[i]] );
	}

	snprintf( buf, sizeof(buf),"%2.1f",v_fox30V);
	wind->DrawText( panel2.X+11, panel2.Y+2, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_fox30V_err] );
	snprintf( buf, sizeof(buf),"%2.1f",v_foxVST);
	wind->DrawText( panel2.X+11, panel2.Y+3, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_foxVST_err] );
	snprintf( buf, sizeof(buf),"%2.1f",v_fox24V);
	wind->DrawText( panel2.X+11, panel2.Y+4, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_fox24V_err] );
	snprintf( buf, sizeof(buf),"%2.1f",v_fox12V);
	wind->DrawText( panel2.X+11, panel2.Y+5, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_fox12V_err] );
	snprintf( buf, sizeof(buf),"%2.1f",v_las24V);
	wind->DrawText( panel2.X+11, panel2.Y+6, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_las24V_err] );
	snprintf( buf, sizeof(buf),"%2.1f",v_fox24S);
	wind->DrawText( panel2.X+11, panel2.Y+7, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_fox24S_err] );
	snprintf( buf, sizeof(buf),"%2.1f",v_las24S);
	wind->DrawText( panel2.X+11, panel2.Y+8, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_las24S_err] );

	snprintf( buf, sizeof(buf),"%2.2f",Cpu_Ver+Cpu_Rev/100);
	wind->DrawText( panel3.X+9, panel3.Y+2, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), GUI_color(GR_WHITE) );
	snprintf( buf, sizeof(buf),"%2.1f",v_CpuDD);
	wind->DrawText( panel3.X+9, panel3.Y+3, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_cpudd_err] );
	snprintf( buf, sizeof(buf),"%2.1f",v_Cpu12);
	wind->DrawText( panel3.X+9, panel3.Y+4, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[v_cpu12_err] );
	snprintf( buf, sizeof(buf),"%2.1f",t_Cpu);
	wind->DrawText( panel3.X+9, panel3.Y+5, buf, GUI_DefaultFont, GUI_color(WIN_COL_SUBTITLE), colore[t_cpu_err] );

	GUI_Thaw();

	int c;
	do
	{
		c = Handle();
	} while((c!=K_ESC) && ((c<K_F1) && (c>K_F12)));

	delete wind;

	if( c != K_ESC )
	{
		return c;
	}

	return 0;
}



//--------------------------------------------------------------------------

#ifdef __DISP2
int test_dosapulse_done;
#endif

int THw_FoxInput(int n,float &val)
{
	switch(n)
	{
		case 0:
			val=(float)FoxHead->ReadEncoder(BRUSH1);
			break;
		case 1:
			val=FoxHead->GetCurrent(BRUSH1);
			break;
		case 2:
			val=FoxHead->GetCurrent(BRUSH2);
			break;
		case 3:
			val=FoxHead->ReadEncoder(BRUSH2);
			break;
		case 4:
			val=Check_PuntaTraslStatus(1);
			break;
		case 5:
			val=Check_PuntaTraslStatus(2);
			break;
		case 6:
			val=Get_Vacuo(1);
			break;      
		case 7:
			val=Get_Vacuo(2);
			break;
	}

	return 1;
}

int THw_FoxOutput(int n,float val)
{
	switch(n)
	{
		case 0:
			Set_Vacuo(1,(int)val);
			break;
		case 1:
			Set_Vacuo(2,(int)val);
			break;    
		case 2:
			Set_Contro(1,(int)val);
			break;
		case 3:
			Set_Contro(2,(int)val);
			break;    
		case 4:
			if(!val)
			{
				FoxHead->ClearOutput(FOX,CAM_SELECT); //set cam1
			}
			else
			{
				FoxHead->SetOutput(FOX,CAM_SELECT); //set cam2
			}
			break;
		case 5:
			Set_HeadCameraLight( (int)val );
			break;
		case 6:
			break;

		#ifndef __DISP2
		case 7:
			if(val)
			{
				FoxHead->SetOutput(FOX,DISPMOVE);
			}
			else
			{
				FoxHead->ClearOutput(FOX,DISPMOVE);
			}
			break;
		case 8:
			if(val)
			{
				Dosatore->SetPressImpulse(DOSA_DEFAULTTIME,DOSA_DEFAULTVISC,DOSAPRESS_TESTMODE1);
			}
			break;
		case 9:
			if(val)
			{
				Dosatore->SetMotorOn();
			}
			else
			{
				Dosatore->SetMotorOff();
			}
			break;
		case 10:
			if(val)
			{
				Dosatore->SetInversion(DOSADIR_INVERTED);
			}
			else
			{
				Dosatore->SetInversion(DOSADIR_NORMAL);
			}
			break;
		#else
		case 7:
			if(val)
			{
				FoxHead->SetOutput(FOX,DISPMOVE1);
			}
			else
			{
				FoxHead->ClearOutput(FOX,DISPMOVE1);

			}
			break;

		case 8:
			if(val)
			{
				FoxHead->SetOutput(FOX,DISPMOVE2);
			}
			else
			{
				FoxHead->ClearOutput(FOX,DISPMOVE2);
			}
			break;
   
		case 9:
			if(val)
			{
				Dosatore->SelectCurrentDisp(1);
				Dosatore->SetPressImpulse(DOSA_DEFAULTTIME,DOSA_DEFAULTVISC,DOSAPRESS_TESTMODE1);
				test_dosapulse_done|=1;      
			}
			break;

		case 10:
			if(val)
			{
				Dosatore->SelectCurrentDisp(2);
				Dosatore->SetPressImpulse(DOSA_DEFAULTTIME,DOSA_DEFAULTVISC,DOSAPRESS_TESTMODE1);
				test_dosapulse_done|=2;   
			}
			break;

		case 11:
			Dosatore->SelectCurrentDisp(2);
			if(val)
			{				
				Dosatore->SetMotorOn();
			}
			else
			{
				Dosatore->SetMotorOff();
			}
			break;

		case 12:
			Dosatore->SelectCurrentDisp(2);
			if(val)
			{
				Dosatore->SetInversion(DOSADIR_INVERTED);
			}
			else
			{
				Dosatore->SetInversion(DOSADIR_NORMAL);
			}    
			break;			
		#endif
	}

	return 1;
}


int TestFox(void)
{
	#ifdef __DISP2
	test_dosapulse_done=0;
	#endif	
	
	WindTestHw* w = new WindTestHw( 0, 5, 78, 26, 2,1,1,MsgGetString(Msg_00606));
	
	#ifndef __DISP2
	w->SetPanel(0,3,2,2,TESTHW_OUTPUT,FOXBRD);
	#else
	w->SetPanel(0,4,2,2,TESTHW_OUTPUT,FOXBRD);
	#endif

	w->SetRow(0,0,4);
	w->SetRow(0,1,3);  
	w->SetRow(0,2,4);
	
	#ifdef __DISP2
	w->SetRow(0,3,2);
	#endif	
	
	//panel0 - row 0
	w->SetRowItem(0,0,0,"Vacuo1",TESTHW_TOGGLE);
	w->SetRowItem(0,0,1,"Vacuo2",TESTHW_TOGGLE);
	w->SetRowItem(0,0,2,"Contro1",TESTHW_TOGGLE);
	w->SetRowItem(0,0,3,"Contro2",TESTHW_TOGGLE);

	//panel0 - row 1
	w->SetRowItem(0,1,0,"CamSel",TESTHW_TOGGLE);
	w->SetRowItem(0,1,1,"Light1",TESTHW_TOGGLE);
	w->SetRowItem(0,1,2,"Light2",TESTHW_TOGGLE);
	
	//panel0 - row 2
	#ifndef __DISP2
	w->SetRowItem(0,2,0,"DispMove",TESTHW_TOGGLE);
	w->SetRowItem(0,2,1,"DispPress");
	w->SetRowItem(0,2,2,"Motor",TESTHW_TOGGLE);
	w->SetRowItem(0,2,3,"Inv",TESTHW_TOGGLE);
	#else
	w->SetRowItem(0,2,0,"DispMove1",TESTHW_TOGGLE);
	w->SetRowItem(0,2,1,"DispMove2",TESTHW_TOGGLE);
	w->SetRowItem(0,2,2,"DispPress1");
	w->SetRowItem(0,2,3,"DispPress2");
	w->SetRowItem(0,3,0,"Motor",TESTHW_TOGGLE);
	w->SetRowItem(0,3,1,"Inv",TESTHW_TOGGLE);	
	#endif
	
	w->Set_OutputF(0,THw_FoxOutput);
	
	w->SetPanel(1,3,12,2,TESTHW_INPUT);

	w->SetRow(1,0,4);
	w->SetRow(1,1,2);
	w->SetRow(1,2,2);  

	
	//panel 1 -row 0
	w->SetRowItem(1,0,0,"Encoder1");
	w->SetRowItem(1,0,1,"Current1",TESTHW_FLOAT);
	w->SetRowItem(1,0,2,"Current2",TESTHW_FLOAT);
	w->SetRowItem(1,0,3,"Encoder2");
	
	for(int i=0;i<4;i++)
	{
		w->Set_UpdateDelayItem(1,0,i,150);
	}

	w->SetRowItem(1,1,0,"Trasl1");
	w->SetRowItem(1,1,1,"Trasl2");

	//panel 1 -row 1
	w->SetRowItem(1,2,0,"Vacuo1");
	w->SetRowItem(1,2,1,"Vacuo2");
	w->Set_UpdateDelayItem(1,2,1,150);  

	w->Set_UpdateDelayItem(1,2,0,150);
	w->Set_InputF(1,THw_FoxInput);
	
	int ret=w->Activate();
	
	#ifdef __DISP2
	if(test_dosapulse_done & 1)
	{
		Dosatore->DoVacuoFinalPulse(1);
	}

	if(test_dosapulse_done & 2)
	{
		Dosatore->DoVacuoFinalPulse(2);
	}
	#endif	
	
	delete w;
	
	return ret;
}

//--------------------------------------------------------------------------

int THw_HeadOutput(int n,float val)
{
  switch(n)
  {
    case 0:
      Set_UgeBlock((int)val);
      break;
    case 1:
      if(val)
		  cpu_cmd(1,(char*)"U81");
      else
		  cpu_cmd(1,(char*)"U80");
      break;
  }

}

int THw_HeadInput(int n,float &val)
{
  switch(n)
  {
    case 0:
      val = CheckLimitSwitchY();
      break;
    case 1:
      val=Read_Input(REED_UGE);
      break;
    case 2:
		val=(cpu_cmd(2,(char*)"ES0")=='C');
      break;
    case 3:
		val=(cpu_cmd(2,(char*)"ER0")=='C');
      break;    
  }
}


int TestHead(void)
{
  WindTestHw *w=new WindTestHw( 0, 5, 78, 18, 2,1,1,MsgGetString(Msg_00606));

  w->SetPanel(0,1,2,2,TESTHW_OUTPUT,HEADBRD);
  w->SetRow(0,0,2);
  w->SetRowItem(0,0,0,"OutTools",TESTHW_TOGGLE);
  w->SetRowItem(0,0,1,"OutSpare1",TESTHW_TOGGLE);
  w->Set_OutputF(0,THw_HeadOutput);

  w->SetPanel(1,1,8,2,TESTHW_INPUT);
  w->SetRow(1,0,4);
  w->SetRowItem(1,0,0,"Switch-Y");
  w->SetRowItem(1,0,1,"ToolsReed");
  w->SetRowItem(1,0,2,"InSpareP");
  w->SetRowItem(1,0,3,"InSpare1");
  w->Set_InputF(1,THw_HeadInput);

  int ret=w->Activate();

  delete w;

  return(ret);
}

//--------------------------------------------------------------------------

int THw_FeedOutput(int n,float val)
{
	switch(n)
	{
		default:
			if(val)
			{
				for(int i=0;i<8;i++)
				{
					Set_CaricMov(n,i,0);
					//THFEEDER - da rivedere per ora passo tipo 0 = tape/air
					CaricWait(0,0);
				}
			}
			break;
	}
}

int TestFeeder(void)
{
	WindTestHw *w=new WindTestHw( 2, 5, 78, 18, 1,1,1,MsgGetString(Msg_00606));

	w->SetPanel(0,3,2,2,TESTHW_OUTPUT,CARBRD);

	w->SetRow(0,0,6);

	w->SetRow(0,1,6);

	w->SetRow(0,2,4);

	w->SetRowItem(0,0,0,"Feed-01");
	w->SetRowItem(0,0,1,"Feed-02");
	w->SetRowItem(0,0,2,"Feed-03");
	w->SetRowItem(0,0,3,"Feed-04");
	w->SetRowItem(0,0,4,"Feed-05");
	w->SetRowItem(0,0,5,"Feed-06");
	
	w->SetRowItem(0,1,0,"Feed-07");
	w->SetRowItem(0,1,1,"Feed-08");
	w->SetRowItem(0,1,2,"Feed-09");
	w->SetRowItem(0,1,3,"Feed-10");
	w->SetRowItem(0,1,4,"Feed-11");
	w->SetRowItem(0,1,5,"Feed-12");
	
	w->SetRowItem(0,2,0,"Feed-13");
	w->SetRowItem(0,2,1,"Feed-14");
	w->SetRowItem(0,2,2,"Feed-15");
	w->SetRowItem(0,2,3,"Feed-16",TESTHW_OUTPUT);
	
	w->Set_OutputF(0,THw_FeedOutput);
	
	int ret=w->Activate();
	
	delete w;
	
	return(ret);
}


//--------------------------------------------------------------------------

extern int Check_ProtSwitch(int Tipo);


int THw_InputF(int n,float &val)
{
  switch(n)
  {
    case 0:
      #ifdef HWTEST_RELEASE
      val=CheckSecurityInput_HT();
      #else
      val=CheckSecurityInput();
      #endif
      break;
    case 1:
      val=(cpu_cmd(2,(char*)"EA")=='C');
      break;
    case 2:
      val = CheckLimitSwitchX();
      break;
    case 3:
      val = CheckLimitSwitchY();
      break;
    case 4:
      val=Check_PuntaTraslStatus(1);
      break;
    case 5:
      val=Check_PuntaTraslStatus(2);
      break;
    case 6:
      val=Read_Input(REED_UGE);
      break;
    case 7:
      val=Check_ProtSwitch(0);
      break;
    case 8:
      val=Check_ProtSwitch(1);
      break;
    case 9:
		val=(cpu_cmd(2,(char*)"E0")!='C');
      break;
    case 10:
		val=(cpu_cmd(2,(char*)"ER0")=='C');
      break;
  }
}

int TestInput(void)
{
  WindTestHw *w=new WindTestHw( 0, 5, 78, 15, 1,1,1,MsgGetString(Msg_00606));

  w->SetPanel(0,2,2,2,TESTHW_INPUT);
  w->SetRow(0,0,7);
  w->SetRow(0,1,4);
  
  w->SetRowItem(0,0,0,"Prot");
  w->SetRowItem(0,0,1,"Pres");
  w->SetRowItem(0,0,2,"Switch-X");
  w->SetRowItem(0,0,3,"Switch-Y");
  w->SetRowItem(0,0,4,"Trasl1");
  w->SetRowItem(0,0,5,"Trasl2");
  w->SetRowItem(0,0,6,"ToolsReed");

  w->SetRowItem(0,1,0,"Zer-X");
  w->SetRowItem(0,1,1,"Zer-Y");
  w->SetRowItem(0,1,2,"Zer-E");
  w->SetRowItem(0,1,3,"InSpare1");

  w->Set_InputF(0,THw_InputF);

  int ret=w->Activate();

  delete w;

  return(ret);
}

//--------------------------------------------------------------------------

int THw_OtherOutput(int n,float val)
{
	int v = int(val);

	switch(n)
	{
		case 0:
			Set_UgeBlock((int)val);
		  break;
		case 1:
			Set_Cmd(3,v);
		  break;
	}
}

int THw_OtherInput(int n,float &val)
{
  switch(n)
  {
    case 0:
    	val=Read_Input(REED_UGE);
      break;
    case 1:
      #ifdef HWTEST_RELEASE
    	val=CheckSecurityInput_HT();
      #else
    	val=CheckSecurityInput();
      #endif
      break;
    case 2:
    	val=(cpu_cmd(2,(char*)"EA")=='C');
      break;
    case 3:
    	val = CheckLimitSwitchX();
      break;
    case 4:
    	val = CheckLimitSwitchY();
      break;
  }
}


int TestOtherIO(void)
{
  WindTestHw *w=new WindTestHw( 0, 5, 78, 20, 2,1,1,MsgGetString(Msg_00606));

  w->SetPanel(0,1,2,2,TESTHW_OUTPUT,OUTBRD);
  w->SetRow(0,0,2);
  w->SetRowItem(0,0,0,"OutTools",TESTHW_TOGGLE);
  w->SetRowItem(0,0,1,"Alarm",TESTHW_TOGGLE);
  w->Set_OutputF(0,THw_OtherOutput);

  w->SetPanel(1,2,8,2,TESTHW_INPUT,INBRD);
  w->SetRow(1,0,3);
  w->SetRow(1,1,2);
  w->SetRowItem(1,0,0,"ToolsReed");
  w->SetRowItem(1,0,1,"Prot");
  w->SetRowItem(1,0,2,"Press");
  w->SetRowItem(1,1,0,"Switch-X");
  w->SetRowItem(1,1,1,"Switch-Y");
  w->Set_InputF(1,THw_OtherInput);

  int ret=w->Activate();

  delete w;

  return(ret);
}

void Test_hardw(void)
{
	int mode=1,prevmode;
	
	do
	{
		switch(mode)
		{
		case 1:
			prevmode=mode;
			mode=Test_page();
			break;
		case 2:
			prevmode=mode;
			mode=Test_parse();
			break;
		case 3:
			prevmode=mode;
			mode=TestFox();
			break;
		case 4:
			prevmode=mode;
			mode=TestOtherIO();
			break;
		case 5:
			prevmode=mode;
			mode=TestFeeder();
			break;
		case 6:
			FeedersTest();
			break;
		default:
			bipbip();
			mode=prevmode+K_F1-1;
			break;
		}
		
		if(mode==0)
		{
			break;
		}
		else
		{
			mode=mode-K_F1+1;
		}
	
	} while(1);
} // Test_hardw



extern void videoCallback_LSG( void* parentWin );

void TestExtCam(void)
{
	NozzleXYMove(0,0);

	// setta parametri telecamera
	SetImgBrightCont( CurDat.HeadBright, CurDat.HeadContrast );
	// salva parametri
	int prevBright = GetImageBright();
	int prevContrast = GetImageContrast();

	SetExtCam_Light(0);
	SetExtCam_Shutter(DEFAULT_EXT_CAM_SHUTTER);
	SetExtCam_Gain(DEFAULT_EXT_CAM_GAIN);

	Wait_PuntaXY();

	setLiveVideoCallback( videoCallback_LSG );
	Set_Tv_Title( MsgGetString(Msg_01863) );
	Set_Tv( 1, CAMERA_EXT );

	int c;
	do
	{
		c = Handle();
		
		switch( c )
		{
		case K_SHIFT_F5:
			SetExtCam_Light(0,-1);
			break;
			
		case K_SHIFT_F6:
			SetExtCam_Light(0,1);
			break;
			
		case K_SHIFT_F7:
			SetExtCam_Shutter(0,-1);
			break;
			
		case K_SHIFT_F8:
			SetExtCam_Shutter(0,1);
			break;
			
		case K_SHIFT_F9:
			SetExtCam_Gain(0,-1);
			break;
			
		case K_SHIFT_F10:
			SetExtCam_Gain(0,+1);
			break;
			
		case K_F4:
			set_bright( 0, false );
			set_contrast( 0, false );
			break;
			
		case K_F5:
			set_bright(-512, false );
			break;
			
		case K_F6:
			set_bright(512, false );
			break;
			
		case K_F7:
			set_contrast(-512, false );
			break;
			
		case K_F8:
			set_contrast(512, false );
			break;
		}
	} while( c != K_ESC );

	Set_Tv( 0, CAMERA_EXT );
	setLiveVideoCallback( 0 );

	// ripristina vecchi valori
	SetImgBrightCont( prevBright, prevContrast );

	SetExtCam_Light( 0 );
	return;
}
