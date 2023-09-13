/*
>>>> Q_SER.CPP     >>> VERSIONE CON INTERFACCIA SERIALE LORIS <<<


>>> Gestione della comunicazione seriale


++++            Modulo di automazione QUADRA.               ++++
++++  (C) L.C.M. di Claudio Arrighi - Carrara               ++++
++++                 Italy 1995/96/97                       ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1997    ++++
++++    Mod.: W042000
*/

#include "q_cost.h"

#ifdef __SERIAL

#include <stdio.h>
#include <string.h>

#include "gui_functions.h"

#include "q_cost.h"
#include "q_graph.h"
#include "q_gener.h"
#include "q_help.h"
#include "q_tabe.h"
#include "q_oper.h"
#include "q_ser.h"
#include "commclass.h"
#include "msglist.h"
#include "q_opert.h"
#include "q_wind.h"

#include "lnxdefs.h"
#include "keyutils.h"

#include <mss.h>


extern struct CfgHeader QHeader;

int com_err;     // public nel modulo, errore in comunicazione CPU


void cnvex(unsigned int d_num, int n_bytes, char *ex_num)
{
	switch(n_bytes)
	{
	case 1:
		sprintf(ex_num,"%1X",d_num);
		break;
	case 2:
		sprintf(ex_num,"%02X",d_num);
		break;
	case 3:
		sprintf(ex_num,"%03X",d_num);
		break;
	default:
		sprintf(ex_num,"%04X",d_num);
		break;
	}
} // cnvex


// visualizza stringhe comunicazione.
void Visu_Comm_Data( char* c_buffer, char* c_answer )
{
	int cam = pauseLiveVideo();

	CWindow win( 0 );
	win.SetStyle( win.GetStyle() | WIN_STYLE_NO_MENU | WIN_STYLE_CENTERED );
	win.SetClientAreaSize( 72, 18 );
	win.SetTitle( MsgGetString(Msg_00635) );

	GUI_Freeze();

	win.Show();

	win.DrawText( 3, 2, MsgGetString(Msg_00636) );
	win.DrawPanel( RectI( 2, 4, win.GetW()/GUI_CharW() - 4, 4 ) );

	win.DrawText( 3, 7, MsgGetString(Msg_00637) );
	win.DrawPanel( RectI( 2, 11, win.GetW()/GUI_CharW() - 4, 6 ) );

	int i=1;
	while (c_buffer[i]!='\0' && i<22)
	{
		win.DrawTextF( 5+3*(i-1), 6, 0, "%c", c_buffer[i] );
		win.DrawTextF( 4+3*(i-1), 7, 0, "%2X", (unsigned char)c_buffer[i] );
		i++;
	}

	i=0;
	while (c_answer[i]!='\0' && c_answer[i]!=0x0d && i<22)
	{
		win.DrawTextF( 5+3*i, 12, 0, "%c", c_answer[i] );
		win.DrawTextF( 4+3*i, 13, 0, "%2X", (unsigned char)c_answer[i] );
		i++;
	}

	if(i==22)
	{
		while (c_answer[i]!='\0' && c_answer[i]!=0x0d && i<44)
		{
			win.DrawTextF( 5+3*(i-22), 15, 0, "%c", c_answer[i] );
			win.DrawTextF( 4+3*(i-22), 16, 0, "%2X", (unsigned char)c_answer[i] );
			i++;
		}
	}

	GUI_Thaw();

	flushKeyboardBuffer();
	waitKeyPress();

	win.Hide();

	playLiveVideo( cam );

	SetConfirmRequiredBeforeNextXYMovement(true);
}

/* Invia un comando alla scheda CPU, ritorna la risposta */
/* l_comando -> lunghezza del comando (dati esclusi) in bytes */
int _cpu_cmd(int l_comando, char *comando)
{
	int i, count, loop_chr, cicli=0;
	char c_buffer[256], c_answer[40], c_status;
	unsigned char checksum=0;
	unsigned int chr;

	if(com_err) 
	{
		return(0);        // se si e' verificato un com error
	}

	if(l_comando>2)
	{
		l_comando=2;  // comando min 1, max 2 bytes
	}

	if(l_comando<1)
	{
		l_comando=1;
	}

	c_buffer[0]=0x20;  // testa messaggio
	c_buffer[1]=0;

	strcat(c_buffer,comando);

	while(cicli<5)     // loop per errore nella risposta
	{
		//GF_21_11_2012 - prima si trovava al di fuori del ciclo while
		ComPortCPU->flush();

		checksum=0;
		loop_chr=0;
		
		do
		{
			ComPortCPU->putbyte(c_buffer[loop_chr]);    // tx messaggio
			checksum=checksum+c_buffer[loop_chr];
			loop_chr++;
		} while(c_buffer[loop_chr]!=0);
	
		ComPortCPU->putbyte(checksum|128);             // tx checksum
		ComPortCPU->putbyte(0xD);                      // tx cr

		count=0;
		chr=0;
		while ((chr != SERIAL_ERROR_TIMEOUT) && (chr != 0xD))
		{
			chr = ComPortCPU->getbyte();
			if (chr == ' ')
			{
				count = 0;
			}
			else
			{
				c_answer[count++] = chr;
			}
	
			if(count>=40) 
			{
				chr=SERIAL_ERROR_TIMEOUT; //L251099
			}
		}

		c_answer[count]='\0';

		// debug comucazione seriale se bit 2 Mode 4 settato
		// visualizza dati ogni ciclo risposta.
		if(((comando[0]!='E') || (comando[1]!='F')) && (QHeader.debugMode1 & DEBUG1_CPUSHOWALL))
		{
			Visu_Comm_Data(c_buffer, c_answer);
		}
	
		if (chr == 0xD)
		{
			count=l_comando;
	
			if((c_buffer[1]=='S') && (c_buffer[2]=='K'))
			{
				comando[0]=c_answer[count];
				comando[1]=0;
			}

			//SMOD211204
			if((c_buffer[1]=='E')&&(c_buffer[2]=='K'))
			{
				for(i=0;i<9;i++)
				{
					comando[i]=c_answer[count++];
				}
				comando[i]=0;
			}

			//SMOD211204
			if(c_buffer[1]=='K')
			{
				for(i=0;i<17;i++)
				{
					comando[i]=c_answer[count++];
				}
				comando[i]=0;
			}

			if((c_buffer[1]=='E')&&(c_buffer[2]=='V')) // lettura vuoto...
			{
				for (i=0; i<2; i++)
				{
					comando[i]=c_answer[count++];       // risposta
				}
				comando[i]=0;		       		      // string terminator
			}

			if((c_buffer[1]=='H')&&(c_buffer[2]=='C')) // test caricatori...
			{
				for (i=0; i<26; i++)
				{
					comando[i]=c_answer[count++];       // risposta
				}
	
				comando[i]=0;		       		      // string terminator
			}

			if((c_buffer[1]=='H')&&(c_buffer[2]=='H')) // test testa...
			{
				for (i=0; i<14; i++)
				{
					comando[i]=c_answer[count++];       // risposta
				}
	
				comando[i]=0;		       		      // string terminator
			}

			if((c_buffer[1]=='H')&&(c_buffer[2]=='M')) // test testa...
			{
				for (i=0; i<10; i++)
				{
					comando[i]=c_answer[count++];       // risposta
				}
			
				comando[i]=0;		       		      // string terminator
	
				c_status=c_answer[count];       // legge lo stato della risposta
	
				if(c_status=='I')
				{
					// risposta OK
					return c_status;
				}
			}

			c_status=c_answer[count];       // legge lo stato della risposta
	
			if(strchr("NKOCWTR",c_status))
			{
				// risposta OK
				return c_status;
			}

			if(c_status=='H')
			{
				W_Mess( MsgGetString(Msg_00610) );
				return c_status;
			}

			if(c_status=='X' || c_status=='Y')
			{
				return c_status;
			}
		}
	
		// debug comucazione seriale se bit 0 Mode 4 settato
		if(QHeader.debugMode1 & DEBUG1_CPUBEEPERR)
		{
			bipbip();
		}
	
		// debug comucazione seriale se bit 1 Mode 4 settato
		// visualizza dati se ciclo risposta errato
		if(QHeader.debugMode1 & DEBUG1_CPUSHOWERR)
		{
			Visu_Comm_Data(c_buffer, c_answer);
		}
	
		comm_end();
	
		comm_init();
	
		cicli++;
		delay(5);
	}

	// Se dopo tre tenatativi la risposta e' un errore
	sprintf(c_buffer, "%s - %c  (%X)", MsgGetString(Msg_00585), c_status, (unsigned char)c_status);
	W_Mess(c_buffer,MSGBOX_YLOW,0,GENERIC_ALARM); //Errore comunicazione CPU: risposta
	com_err=1;
	Set_OnFile(1); // movimenti off

	return c_status;
}

int cpu_cmd(int l_comando, char *comando)
{
	int status;
	char buffer[80];
	
	if(!ComPortCPU->enabled)
	{
		return(1);
	}
	
	status=_cpu_cmd(l_comando, comando);
	
	if(status=='X' || status=='Y')
	{
		if(Get_OnFile())
		{
			return(status);
		}

		_cpu_cmd(2,(char*)"SR1");            // reset elettronica esterna
		_cpu_cmd(2,(char*)"SR0");            // azzera reset flag

		Set_OnFile(1);
		sprintf(buffer, "%s - %c", MsgGetString(Msg_00005), status);
		W_Mess(buffer,MSGBOX_YLOW,0,GENERIC_ALARM);
	}

	return status;
}


/* Ritorna lo stato della comunicazione con la CPU */

int com_chk(void) { return(com_err); }

/* Com port init */
void comm_init(void)
{
	com_err=0;           // flag errore off
	ComPortCPU->open();
}

/* Com port close */
void comm_end(void)
{
	ComPortCPU->close();
}

int GetCPUVersion( int& ver, int& rev )
{
	char buf[40];

	strcpy( buf, "HM" ); // comando
	int status = cpu_cmd( 2, buf );

	ver = Hex2Dec(buf[1])+16*Hex2Dec(buf[0]);
	rev = Hex2Dec(buf[3])+16*Hex2Dec(buf[2]);

	return status;
}

int GetMachineTemperature( float& temp )
{
	char buf[40];

	strcpy( buf, "HM" ); // comando
	int status = cpu_cmd( 2, buf );

	temp = ((Hex2Dec(buf[9])+16*Hex2Dec(buf[8]))*COSTT1)+COSTT2;

	return status;
}

#endif
