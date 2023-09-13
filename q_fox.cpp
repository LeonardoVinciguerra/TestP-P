#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "gui_functions.h"
#include "q_fox.h"
#include "q_gener.h"
#include "msglist.h"
#include "q_help.h"
#include "q_wind.h"
#include "q_graph.h"
#include "q_oper.h"
#include "filefn.h"
#include "q_cost.h"

#include "keyutils.h"
#include "fileutils.h"
#include "lnxdefs.h"

#include <mss.h>

//#define __FOX_ENABLE_LOG

#define START   ' '
#define END     0x0D
#define NAKCAR  'N'

#define NOERR           0
#define NAK             0x100
#define ACK             0x300
#define CHKSUMERR       0x400
#define ENDERR          0x500
#define BADADDR         0x600
#define BADNBYTE        0x700

#define FOXERR_TIMEOUT  MsgGetString(Msg_01018) //timeout in ricezione
#define FOXERR_NAK      MsgGetString(Msg_01047) //ritornato nak
#define FOXERR_DATA     MsgGetString(Msg_01048) //dati ricevuti non congruenti
#define FOXERR_ADDR     MsgGetString(Msg_01210) //errore indirizzo mittente

// Dim. e posizione finestra comunicazione seriale
#define   FOXPOS	4,6,76,22

//DANY291102
#define	FOXPOSERR	   MsgGetString(Msg_01153)
#define	FOXVELERR	   MsgGetString(Msg_01154)
#define	FOXACCERR	   MsgGetString(Msg_01155)
#define	FOXTNOISEERR	MsgGetString(Msg_01156)
#define	FOXPNOISEERR	MsgGetString(Msg_01157)

//numero massimo di errori consecutivi prima di disattivare i movimenti
#define  FOXMAXERRORS   3
#define  FOXMAXERR_MSG  MsgGetString(Msg_01244)

#define  FOXENCREP_POS  5,3,76,22



//costruttore per disabilitazione
FoxCommClass::FoxCommClass(void)
{
	serial=NULL;
	enabled=0;
	enableLog = 0;
	foxlog = NULL;
	last_rx=new char[MAXFOXBUF];
}

FoxCommClass::FoxCommClass( CommClass* com_port )
{
	serial = com_port;
	serial->flush();
	errcode=0;
	flag=0;
	enabled=1;
	enableLog = 0;
	foxlog = NULL;
	last_rx=new char[MAXFOXBUF];
}

FoxCommClass::~FoxCommClass(void)
{
	if(serial!=NULL)
	{
		delete serial;
	}

	if(last_rx!=NULL)
	{
		delete[] last_rx;
	}

	if(foxlog!=NULL)
	{
		FilesFunc_fclose(foxlog);
	}
}

void FoxCommClass::StartLog(int append)
{
#ifdef __FOX_ENABLE_LOG
	if( foxlog == NULL )
	{
		if(!append)
		{
			foxlog=FilesFunc_fopen("fox.log","wr");
		}
		else
		{
			foxlog=FilesFunc_fopen("fox.log","ar+");
		}

		if(foxlog==NULL)
		{
			enableLog = 0;
			return;
		}
	}

	enableLog = 1
#else
	enableLog = 0;
#endif
}

void FoxCommClass::EndLog(void)
{
	if(foxlog!=NULL)
	{
		FilesFunc_fclose(foxlog);
		foxlog=NULL;
		enableLog = 0;
	}
}

void FoxCommClass::RestartLog(void)
{
#ifdef __FOX_ENABLE_LOG
	if(foxlog!=NULL)
	{
		enableLog = 1;
	}
#endif
}

void FoxCommClass::PauseLog(void)
{
	enableLog = 0;
}

void FoxCommClass::PrintLog(const char *buf)
{
	if( enableLog == 1 )
	{
		fprintf(foxlog,"%s\n",buf);
	}
}

//calcola checksum a partire da una stringa
unsigned char FoxCommClass::CalcChecksum(char *buf)
{
  char *ptr=buf+1;
  unsigned char checksum=0;

  while(*ptr!=0)
  { checksum+=*ptr;
    ptr++;
  }
  return (~checksum);
}

// visualizza stringhe comunicazione.
void FoxCommClass::Show_CommData(void)
{
	int cam = pauseLiveVideo();

	CWindow* Q_Err = new CWindow( 4,6,76,22, MsgGetString(Msg_00635) );

	GUI_Freeze();

	Q_Err->Show();

	Q_Err->DrawPanel( RectI( 2, 5, Q_Err->GetW()/GUI_CharW() - 4, 8 ) );

	int i = 1;
	while (_txbuf[i]!='\0' && _txbuf[i]!=0x0d && i<22)
	{
		Q_Err->DrawTextF( 5+3*(i-1), 6, 0, "%c", _txbuf[i] );
		Q_Err->DrawTextF( 4+3*(i-1), 7, 0, "%2X", (unsigned char)_txbuf[i] );
		i++;
	}

	i=0;
	while (last_rx[i]!='\0' && last_rx[i]!=0x0d && i<22)
	{
		Q_Err->DrawTextF( 5+3*i, 12, 0, "%c", last_rx[i] );
		Q_Err->DrawTextF( 4+3*i, 13, 0, "%2X", (unsigned char)last_rx[i] );
		i++;
	}

	if(i==22)
	{
		while (last_rx[i]!='\0' && last_rx[i]!=0x0d && i<44)
		{
			Q_Err->DrawTextF( 5+3*(i-22), 15, 0, "%c", last_rx[i] );
			Q_Err->DrawTextF( 4+3*(i-22), 16, 0, "%2X", (unsigned char)last_rx[i] );
			i++;
		}
	}

	GUI_Thaw();

	flushKeyboardBuffer();
	waitKeyPress();

	delete Q_Err;

	playLiveVideo( cam );
}


//Invia comando (cmd) ad address, composto da ndata bytes. Ritorna in buffer data.
void FoxCommClass::Send(unsigned char address,unsigned char ndata,char cmd,unsigned char *data)
{
  char buf[3];

  unsigned char tx_checksum;
  int i;

  memset(_txbuf,0,9+2*ndata+1);

  if(!enabled) return;

  _txbuf[0]=START;

  Val2Hex(address,1,buf);
  strcpy(_txbuf+1,buf);

  Val2Hex(ndata,1,buf);
  strcpy(_txbuf+3,buf);

  _txbuf[5]=cmd;

  for(i=0;i<ndata;i++)
  { Val2Hex(data[i],1,buf);
    strcpy(_txbuf+6+i*2,buf);
  }

  *(_txbuf+6+2*ndata)=0;
  tx_checksum=CalcChecksum(_txbuf);

  Val2Hex(tx_checksum,1,buf);
  strcpy(_txbuf+6+2*ndata,buf);

  _txbuf[8+2*ndata]=END;

  _txbuf[9+2*ndata]=0;

	if( enableLog == 1 )
	{
		fprintf(foxlog,"\nSent data:\n   %s",_txbuf);
		fflush(foxlog);
	}

	serial->flush();
	int _txlen = strlen( _txbuf );
	serial->putstring( _txbuf, _txlen );

	_txbuf[8+2*ndata]=0;
	
	// txbuf rimane allocato fino al prossimo send !!!
}

// Lettura di un byte con conversione da hex a num.
// ritorna codice errore o byte letto
// se checksum_mode=1 usa dati ricevuti per il calcolo del checksum di ricezione
unsigned int FoxCommClass::GetByte(int checksum_mode)
{ unsigned int d_byte;

  int ch;

  ch=serial->getbyte();

	if( enableLog == 1 )
	{
		if(ch!=SERIAL_ERROR_TIMEOUT)
			fprintf(foxlog,"%c",ch);
		else
			fprintf(foxlog,"(TIMEOUT)");
		fflush(foxlog);
	}

  if(checksum_mode)
    checksum+=ch;

  if(ch==SERIAL_ERROR_TIMEOUT)
	  return(SERIAL_ERROR_TIMEOUT);
  d_byte=Hex2Dec(ch)*16;

  ch=serial->getbyte();

	if( enableLog == 1 )
	{
		if(ch!=SERIAL_ERROR_TIMEOUT)
			fprintf(foxlog,"%c",ch);
		else
			fprintf(foxlog,"(TIMEOUT)");
		fflush(foxlog);
	}

  if(checksum_mode)
    checksum+=ch;

  if(ch==SERIAL_ERROR_TIMEOUT)
	  return(SERIAL_ERROR_TIMEOUT);
  d_byte+=Hex2Dec(ch);

  return(d_byte);
}

// Riceve dati.
// Riorna:
//   Codice status in upper byte, byte ritornati (se ok) in lower byte.
//   Dati ricevuti in rxdata.
//   Indirizzo mittente in sender.

int FoxCommClass::Receive(char *rxdata=NULL)
{
	unsigned char rxbuf[MAXFOXBUF];

	unsigned int ch=0;
	unsigned int nbyte;
	unsigned int tmp;

	unsigned int rx_checksum;

	char *tmp_lastrx;

	int i;

	checksum=0;

	if(!enabled)
		return(ACK);

	if( enableLog == 1 )
		fprintf(foxlog,"\nReceived:\n   ");

	do
	{
		ch=serial->getbyte();

		if( enableLog == 1 )
		{
			if(ch!=SERIAL_ERROR_TIMEOUT)
				fprintf(foxlog,"%c",ch);
			else
				fprintf(foxlog,"(TIMEOUT)");
			fflush(foxlog);
		}
	} while((ch!=START) && (ch!=SERIAL_ERROR_TIMEOUT));

  if(ch==SERIAL_ERROR_TIMEOUT)
  {
		lastErr=SERIAL_ERROR_TIMEOUT;
		//W_Mess("START TIMEOUT");
		return(SERIAL_ERROR_TIMEOUT);
  }

  sender=GetByte(1);

  if(sender==SERIAL_ERROR_TIMEOUT)
  { lastErr=SERIAL_ERROR_TIMEOUT;
    //W_Mess("SENDER TIMEOUT");
  return(SERIAL_ERROR_TIMEOUT);
  }

  nbyte=GetByte(1);
  if(nbyte==SERIAL_ERROR_TIMEOUT)
  { lastErr=SERIAL_ERROR_TIMEOUT;
    //W_Mess("NBYTE TIMEOUT");
  return(SERIAL_ERROR_TIMEOUT);
  }

  ch=serial->getbyte();

  if( enableLog == 1 )
	{
		if(ch!=SERIAL_ERROR_TIMEOUT)
			fprintf(foxlog,"%c",ch);
		else
			fprintf(foxlog,"(TIMEOUT)");
		fflush(foxlog);
	}

  checksum+=ch;

//  rxbuf=new unsigned char[nbyte+1];
  *rxbuf=ch;

/*
  if(last_rx!=NULL)
  { //MSS_CHECK_POINTER_VALIDITY(last_rx);
    delete[] last_rx;
  }
*/

//  last_rx=new char[nbyte*2+10];     //init buffer ultimi dati ricevuti
  memset(last_rx,0,nbyte*2+10);

  *last_rx=START;
  Val2Hex(sender,1,last_rx+1,0);
  Val2Hex(nbyte,1,last_rx+3,0);
  last_rx[5]=ch;

  tmp_lastrx=last_rx;

  last_rx+=6;

  switch(ch)
  { case NAKCAR:
      lastErr=NAK;
      last_rx=tmp_lastrx;
//      delete[] rxbuf;
      return(NAK);
      break;
	  case SERIAL_ERROR_TIMEOUT:
		  lastErr=SERIAL_ERROR_TIMEOUT;
      last_rx=tmp_lastrx;
//      delete[] rxbuf;
      //W_Mess("ECHO TIMEOUT");
	  return(SERIAL_ERROR_TIMEOUT);
      break;
  }

  for(i=1;i<=nbyte;i++)
  { tmp=GetByte(1);
  if(tmp==SERIAL_ERROR_TIMEOUT)
	{ lastErr=SERIAL_ERROR_TIMEOUT;
      //W_Mess("DATA TIMEOUT");
      last_rx=tmp_lastrx;
//      delete[] rxbuf;
	  return(SERIAL_ERROR_TIMEOUT);
    }

    rxbuf[i]=(unsigned char)tmp;
    Val2Hex((unsigned char)tmp,1,last_rx,0);
    last_rx+=2;

  }

  checksum=~checksum;

  rx_checksum=GetByte();

  Val2Hex(rx_checksum,1,last_rx);

  if(rx_checksum==SERIAL_ERROR_TIMEOUT)
  { lastErr=SERIAL_ERROR_TIMEOUT;
//    delete[] rxbuf;
    last_rx=tmp_lastrx;
    //W_Mess("CHECKSUM TIMEOUT");
	return(SERIAL_ERROR_TIMEOUT);
  }
  if(rx_checksum!=checksum)
  { lastErr=CHKSUMERR;
//    delete[] rxbuf;
    last_rx=tmp_lastrx;
    return(CHKSUMERR);
  }

  ch=serial->getbyte();

	if( enableLog == 1 )
	{
		if(ch!=SERIAL_ERROR_TIMEOUT)
			fprintf(foxlog,"%c",ch);
		else
			fprintf(foxlog,"(TIMEOUT)");
		fflush(foxlog);
	}

  last_rx[2]=ch;
  last_rx[3]='\0';
  
  switch(ch)
  { case SERIAL_ERROR_TIMEOUT:
      lastErr=CHKSUMERR;
//      delete[] rxbuf;
      last_rx=tmp_lastrx;
      //W_Mess("END TIMEOUT");
	  return(SERIAL_ERROR_TIMEOUT);
      break;
    case END:
      lastErr=ACK;
      break;
    default:
      lastErr=ENDERR;
//      delete[] rxbuf;
      last_rx=tmp_lastrx;
      return(ENDERR);
      break;
  }

  memcpy(rxdata,rxbuf,nbyte+1);

//  delete[] rxbuf;

  last_rx=tmp_lastrx;

  return(lastErr | (nbyte & 0xFF));
}

// ritorna ultimo codice di errore
int FoxCommClass::GetErr(void)
{ return lastErr;
}

// sequenza completa di invio generico comando
// ritorna 1 se ok
//         0 se fail
//SMOD240103
int FoxCommClass::SendCmd(int addr,char cmd,int *data_val,int ntx,int *rx_data,int nrx,int mode)
{
  unsigned char tx_data[MAXFOXBUF];
  char rx_buf[MAXFOXBUF];
  
  char errbuff[80];

  if(!enabled)
  {
    return(1);
  }

  int i,result,retval=1;
  int nretry=0;

  *tx_data=0;
  *rx_data=0;

  for(i=0;i<ntx;i++)
    tx_data[i]=data_val[i/4]>>(i*8) & 0xFF;


  int fine=0;
  int send_flag=1;
  int ok=0;

  do
  {
    //print_debug("nr=%d\n",nretry);

    if(send_flag)
    { 
	  	Send(addr,ntx,cmd,tx_data);
      send_flag=0;
    }

    result=Receive(rx_buf);
    result=(result & 0xF00);

    //print_debug("nr0=%d\n",nretry);

    if(((*rx_buf==cmd) && (sender==addr)) && (result==ACK))
    { fine=1;
      ok=1;
    }
    else
    {
      send_flag=1;
      nretry++;
	  
	  time_t raw_time;
	  time(&raw_time);

      //print_debug("nr1=%d\n",nretry);
      
      if(nretry>FOX_NRETRYMAX)
        fine=1;

      //print_debug("fine=%d\n",fine);

      if(fine && !ok)
	  {
		if(!(mode & FOX_ERROROFF))
		{
			switch(result)
			{
				case NAK:
					snprintf( errbuff, sizeof(errbuff),FOXERR_NAK,cmd,addr);
					break;
				case SERIAL_ERROR_TIMEOUT:
					snprintf( errbuff, sizeof(errbuff),FOXERR_TIMEOUT,cmd,addr);
					break;
				default:
					if(*rx_buf!=cmd)
					{
						snprintf( errbuff, sizeof(errbuff),FOXERR_DATA,cmd,addr);
					}
					else
					{
						if(sender!=addr)
						{
							snprintf( errbuff, sizeof(errbuff),FOXERR_DATA,cmd,addr);
						}
					}
					break;
			}

        	//print_debug("nr2=%d\n",nretry);

			//print_debug("bip\n");
			W_Mess(errbuff,MSGBOX_YLOW,0,GENERIC_ALARM);
			//print_debug("mess\n");
			Show_CommData();
			//print_debug("err\n");
		}

      }
      //print_debug("nr3=%d\n",nretry);
    }
  } while(!fine);

  //print_debug("end loop\n");

  if(ok)
  { if(mode & FOX_COPYRXBUF)
      memcpy((char *)rx_data,((char *)rx_buf)+1,nrx);
    else
    { *rx_data=0;
      retval=1;
      for(i=1;i<=nrx;i++)
        *rx_data+=(unsigned char)rx_buf[i]*Potenza(16,(i-1)*2);
    }
  }
  else
    retval=0;

  if(!ok)
  {

     W_Mess(FOXMAXERR_MSG,MSGBOX_YLOW,0,GENERIC_ALARM);

    Set_OnFile(1);
    enabled=0;
  }

  return(retval);
}


//--------------------------------------------------------------------------

#define FOX_MOVABS      'A'
#define FOX_MOVREL      'R'
#define FOX_SETACC      'J'
#define FOX_SETVMIN     'H'
#define FOX_SETVMAX     'V'
#define FOX_SETZERO     'Z'
#define FOX_READ_ADC    'U'
#define FOX_STOPSTEP    'M'
#define FOX_ROTATE      'Q'
#define FOX_SETOUT      'S'
#define FOX_GETPOS      'P'
#define FOX_READIN      'I'
#define FOX_RESET       'F'
#define FOX_LOWCUR      'L'
#define FOX_GETSTATUS   'T'
#define FOX_READENC     'W'
#define FOX_SETPID      'D'
#define FOX_GETVER      'v'
#define FOX_SETENCTYPE  'e'
#define FOX_DISABLEALL  'd'
#define FOX_MOTORENABLE 'm'
#define FOX_CURPROT     'p'
#define FOX_RDCURRENT   'c'
#define FOX_CRASH       'h'
#define FOX_ARMWATCHDOG 'w'
#define FOX_TNOISE      'n'
#define FOX_PNOISE      'o'
#define FOX_LOGON       'q'
#define FOX_LOGOFF      'r'
#define FOX_LOGREPORT   's'
#define FOX_GETBRUSHPOS 'b'
#define FOX_DEBUG       'g'
#define CURRENT_TIMER_MS 3.2768

//movimento assoluto
//ritorna ok/fail
int FoxClass::MoveAbs(int motor,int pos)
{ if(disabled[motor] || !Fox->enabled)
    return(1);
  else
  {
    if(motor==BRUSH1)
    {
      lastRotPos[0]=pos;
    }

    if(motor==BRUSH2)
    {
      lastRotPos[1]=pos;
    }    
  
    if((pos>-MAXSTEPVAL) && (pos<MAXSTEPVAL))
      return(Fox->SendCmd(motor,FOX_MOVABS,(int *)&pos,2,&rxval,0));
    else
    {
      char mess[150];      
      snprintf( mess, sizeof(mess),FOXPOSERR,motor);
      W_Mess(mess,MSGBOX_YLOW,0,GENERIC_ALARM);
      return 1;
    }
  }
}

//movimento relativo
//ritorna ok/fail
int FoxClass::MoveRel(int motor,int step)
{
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  {
    if(motor==BRUSH1)
    {
      lastRotPos[0]+=step;
    }

    if(motor==BRUSH2)
    {
      lastRotPos[1]+=step;
    }    

    if( (step>-MAXSTEPVAL) && (step<MAXSTEPVAL) )
      return(Fox->SendCmd(motor,FOX_MOVREL,(int *)&step,2,&rxval,0));
    else
    {
      char mess[150];
      snprintf( mess, sizeof(mess),FOXPOSERR,motor);
      W_Mess(mess,MSGBOX_YLOW,0,GENERIC_ALARM);
      return(1);
    }
  }
}

//ritorna posizione attuale
short int FoxClass::ReadPos(int motor) //SMOD060503
{ int *spare = 0;
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  { Fox->SendCmd(motor,FOX_GETPOS,spare,0,&rxval,2);
    return(rxval);
  }
}

//inizia rotazione continua (SOLO PER BRUSHLESS)
//ritorna ok/fail
int FoxClass::Rotate(int motor,int dir)
{
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
    return(Fox->SendCmd(motor,FOX_ROTATE,(int *)&dir,2,&rxval,0));
}

//ritorna valore del registro degli ingressi
unsigned char FoxClass::ReadInput(int address) //SMOD060503
{ int *spare = 0;

  if(!Fox->enabled)
    return(0);
    
  int ret=Fox->SendCmd(address,FOX_READIN,spare,0,&rxval,1);
  ret=rxval;
  return(ret);

}

//imposta numero di passi che verranno effettuati a vel minima prima del
//termine del movimento. (SOLO PER STEPPER)
//ritorna ok/fail
int FoxClass::SpaceStop(int motor,int step)
{
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
    return(Fox->SendCmd(motor,FOX_STOPSTEP,(int *)&step,2,&rxval,0));
}

//setta registro delle uscite
//ritorna ok/fail
int FoxClass::SetOutput(int address,int mask)
{
  if (!Fox->enabled)
    return(0);
  outval=outval | mask;
  return(Fox->SendCmd(address,FOX_SETOUT,(int *)&outval,2,&rxval,0));

}

int FoxClass::ClearOutput(int address,int mask)
{
  if (!Fox->enabled)
    return(0);
  outval=outval & (~mask);
  return(Fox->SendCmd(address,FOX_SETOUT,(int *)&outval,2,&rxval,0));

}

//setta posizione attuale come posizione di zero
int FoxClass::SetZero(int motor)
{ int *spare = 0;
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  {
    if(motor==BRUSH1)
      lastRotPos[0]=0;
    if(motor==BRUSH2)
      lastRotPos[1]=0;

    return(Fox->SendCmd(motor,FOX_SETZERO,spare,0,&rxval,0));
  }
}

//setta accelerazione espressa in step/s^2
int FoxClass::SetAcc(int motor,int acc)
{
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  { //if(motor==STEP1 || motor==STEP2)
//      print_debug("Motor %d. Acc    =%d\n",motor,acc);
    if( (acc<MAXACCVAL) && (acc>0) )
      return(Fox->SendCmd(motor,FOX_SETACC,(int *)&acc,4,&rxval,0));
    else
    {
      char mess[150];
      snprintf( mess, sizeof(mess),FOXACCERR,motor);
      W_Mess(mess,MSGBOX_YLOW,0,GENERIC_ALARM);
      return(1);
    }
  }
}

//setta vel di start
int FoxClass::SetVMin(int motor,int vmin)
{
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  {
    if( (vmin<MAXVELVAL) && (vmin>0) )
      return(Fox->SendCmd(motor,FOX_SETVMIN,&vmin,4,&rxval,0));
    else
    {
      if(vmin>0)
      { vmin=MAXVELVAL;
        return(Fox->SendCmd(motor,FOX_SETVMIN,&vmin,4,&rxval,0));
      }
      else
      {
        char mess[150];
        snprintf( mess, sizeof(mess),FOXVELERR,motor);
        W_Mess(mess,MSGBOX_YLOW,0,GENERIC_ALARM);
        return(1);
      }
    }
  }

/*
    if( (vmin<MAXVELVAL) && (vmin>0) )
      return(Fox->SendCmd(motor,FOX_SETVMIN,&vmin,4,&rxval,0));
    else
    {
      //DANY100103
      LOGKEY(LOGERR_LIMIT,"CMD");

      char mess[150];
      snprintf( mess, sizeof(mess),FOXVELERR,motor);
      W_Mess(mess);
      return(1);
    }
*/
}

//setta vel massima
int FoxClass::SetVMax(int motor,int vmax)
{
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  {
    if((vmax<MAXVELVAL) && (vmax>0))
      return(Fox->SendCmd(motor,FOX_SETVMAX,&vmax,4,&rxval,0));
    else
    {
      if(vmax>0)
      { vmax=MAXVELVAL;
        return(Fox->SendCmd(motor,FOX_SETVMAX,&vmax,4,&rxval,0));
      }
      else
      {
        char mess[150];
        snprintf( mess, sizeof(mess),FOXVELERR,motor);
        W_Mess(mess,MSGBOX_YLOW,0,GENERIC_ALARM);
        return(1);
      }
    }
  }
/*
    else
    {
      char mess[150];
      snprintf( mess, sizeof(mess),FOXVELERR,motor);
      W_Mess(mess);
      return(1);
    }
*/
}

//Reset software della scheda
int FoxClass::Reset(void)
{
  int *spare = 0;
  if (!Fox->enabled)
    return(0);
  return(Fox->SendCmd(0,FOX_RESET,spare,0,&rxval,0));
}


//Lettura dei 2 canali analogici
int FoxClass::ReadADC(short int &chA,short int &chB)
{ int *spare = 0;

  if (!Fox->enabled)
  {
	  chA = chB = 0;
	  return(0);
  }

  int ret=Fox->SendCmd(0,FOX_READ_ADC,spare,0,&rxval,4);
  if(ret)
  { //fprintf(stdout,"%x\n",rxval);
    chA=rxval & 0xFFFF;
    chB=(rxval >> 16) & 0xFFFF;
  }
  return ret;
}

//forza a state il flag di riduzione della corrente (SOLO PER STEPPER)
int FoxClass::ForceLowCurrent(int motor,char state)
{ int dbyte=0x80 | (state << 6);

  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
    return(Fox->SendCmd(motor,FOX_LOWCUR,(int *)&dbyte,1,&rxval,0));
}

//Setta timer x riduzione della corrente. (solo x stepper)
//time espresso in ms.
//valore max=416.1536ms
//risoluzione=3.2768ms
int FoxClass::SetLowCurrent_Timer(int motor,float time)
{
  int clicks=0;

  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  { clicks=int(time/CURRENT_TIMER_MS);
    return(Fox->SendCmd(motor,FOX_LOWCUR,(int *)&clicks,1,&rxval,0));
  }
}

unsigned char FoxClass::ReadStatus(int motor) //SMOD060503
{ int *spare = 0;

  if (disabled[motor] || !Fox->enabled)
    return(0xFF);
  else
  { Fox->SendCmd(motor,FOX_GETSTATUS,spare,0,&rxval,1);
    return(rxval);
  }
}

int FoxClass::GetLastRotPos(int motor)
{
  if(motor==BRUSH1)
    return(lastRotPos[0]);
  else
    return(lastRotPos[1]);
}

short int FoxClass::ReadEncoder(int motor) //SMOD060503
{ int *spare = 0;

  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  { Fox->SendCmd(motor,FOX_READENC,spare,0,&rxval,2);
    return(rxval);
  }
}

short int FoxClass::ReadBrushPosition(int motor) //SMOD060503
{
  int *spare = 0;

  if(disabled[motor] || !Fox->enabled)
    return(1);
  else
  {
    Fox->SendCmd(motor,FOX_GETBRUSHPOS,spare,0,&rxval,2);
    return(rxval);
  }
}

int FoxClass::SwitchEncoder(int motor)
{
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  { switch(motor)
    { case BRUSH1:
        ClearOutput(motor,ENC_SWITCH);
        break;
      case BRUSH2:
        SetOutput(motor,ENC_SWITCH);
        break;
    }
  }
  return(1);
}

int FoxClass::SetPID(int motor,unsigned int prop,unsigned int integral,unsigned int deriv,unsigned int clipint)
{ int ret;
  char tmp[11];

  unsigned int _prop,_integral,_deriv;
  unsigned int _clipint=clipint;

  if (disabled[motor] || !Fox->enabled)
    return(0);

  _prop=prop;
  _deriv=deriv;
  _integral=integral;

  brushless.integral=integral;
  brushless.deriv=deriv;
  brushless.prop=prop;
  brushless.clipint=clipint;

  memcpy(tmp,&_prop,2);
  memcpy(tmp+2,&_deriv,2);
  memcpy(tmp+4,&_integral,2);
  memcpy(tmp+6,&_clipint,4);

  ret=Fox->SendCmd(motor,FOX_SETPID,(int *)tmp,10,&rxval,0);

  return(ret);
}

int FoxClass::SetIntegral(int motor,unsigned int integral)
{ brushless.integral=integral;
  return(SetPID(motor,brushless.prop,brushless.integral,brushless.deriv,brushless.clipint));
}

int FoxClass::SetDeriv(int motor,unsigned int deriv)
{ brushless.deriv=deriv;
  return(SetPID(motor,brushless.prop,brushless.integral,brushless.deriv,brushless.clipint));
}

int FoxClass::SetProp(int motor,unsigned int prop)
{ brushless.prop=prop;
  return(SetPID(motor,brushless.prop,brushless.integral,brushless.deriv,brushless.clipint));
}

int FoxClass::SetClipInt(int motor,unsigned int clipint)
{ brushless.clipint=clipint;
  return(SetPID(motor,brushless.prop,brushless.integral,brushless.deriv,brushless.clipint));
}

//SMOD240103 - SMOD060503
short int FoxClass::GetVersion(int errmode)
{ int *spare = 0;
  rxval=0;
  if (!Fox->enabled)
    return(0);
  Fox->SendCmd(0,FOX_GETVER,spare,0,&rxval,2,errmode);
  return(rxval);
}

int FoxClass::SetEncoderType(int motor,int mode)
{
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
    return(Fox->SendCmd(motor,FOX_SETENCTYPE,(int *)&mode,1,&rxval,0));
}

int FoxClass::SetMotorOn_Off(int motor,int status)
{
  if(!Fox->enabled)
    return(1);
    
  disabled[motor]=!status;
  return(Fox->SendCmd(motor,FOX_MOTORENABLE,&status,1,&rxval,0));
}

int FoxClass::MotorDisable(int motor)
{ return(SetMotorOn_Off(motor,0));
}

int FoxClass::MotorEnable(int motor)
{ return(SetMotorOn_Off(motor,1));
}

int FoxClass::MotorDisAll(void)
{ int spare;
  if(!Fox->enabled)
    return(1);
  return(Fox->SendCmd(0,FOX_DISABLEALL,&spare,1,&rxval,0));
}

int FoxClass::SetMaxCurrent(int motor,int current,int timer)
{ int spare=((current & 7) | ((timer & 7) << 3));
  if(!Fox->enabled)
    return(1);
  return(Fox->SendCmd(motor,FOX_CURPROT,&spare,1,&rxval,2));
}


float FoxClass::GetCurrent(int motor)
{ int spare;

  if(disabled[motor] || !Fox->enabled)
    return(0);

  Fox->SendCmd(motor,FOX_RDCURRENT,&spare,0,&rxval,4);
  if(motor==BRUSH1)
    rxval=rxval & 0xFFFF;
  else
    rxval=(rxval>>16) & 0xFFFF;

  return((float)rxval*15/1024);
}


int FoxClass::ArmWatchdog(void)
{ int spare;

  if(!Fox->enabled)
    return(0);
    
  int ret=(Fox->SendCmd(0,FOX_ARMWATCHDOG,&spare,0,&rxval,0));

  delay(WATCHDOG_DELAY);

  return(ret);
}

int FoxClass::Crash(void)
{ int spare;

  if(!Fox->enabled)
    return(0);

  return(Fox->SendCmd(0,FOX_CRASH,&spare,0,&rxval,0));
}

int  FoxClass::IsEnabled(int addr)
{
  return(!disabled[addr]);
}


void FoxClass::Disable(void)
{ disabled[0]=1;
  disabled[1]=1;
  disabled[2]=1;
  disabled[3]=1;
}

void FoxClass::Enable(void)
{ disabled[0]=0;
  disabled[1]=0;
  disabled[2]=0;
  disabled[3]=0;
}

void FoxClass::Disable(int addr)
{ disabled[addr]=1;
}

void FoxClass::Enable(int addr)
{ disabled[addr]=0;
}

//ritorna ultimo codice di errore
int FoxClass::GetErr(void)
{ return Fox->GetErr();
}

//ritorna contenuto buffer di ricezione
int FoxClass::RxData(void)
{ return(rxval);
}

FoxClass::FoxClass(FoxCommClass *comm)
{ Fox=comm;
  rxval=0;
  disabled[0]=0;
  disabled[1]=0;
  disabled[2]=0;
  disabled[3]=0;
  outval=0;
  lastRotPos[0]=0;
  lastRotPos[1]=0;
}

const char* FoxClass::GetPort(void)
{
	return(Fox->GetSerial()->GetPort());
}

int FoxClass::SetTorqueNoise(int motor,int val)
{ if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  {
    if( (val>=0) && (val<MAXTNOISEVAL) )
      return(Fox->SendCmd(motor,FOX_TNOISE,(int *)&val,2,&rxval,0));
    else
    {
      char mess[150];
      snprintf( mess, sizeof(mess),FOXTNOISEERR,motor);
      W_Mess(mess,MSGBOX_YLOW,0,GENERIC_ALARM);
      return(1);
    }
  }
}


int FoxClass::SetPositionNoise(int motor,int val)
{
  if (disabled[motor] || !Fox->enabled)
    return(1);
  else
  {
    if( (val>=0) && (val<MAXPNOISEVAL) )
      return(Fox->SendCmd(motor,FOX_PNOISE,(int *)&val,2,&rxval,0));
    else
    {
      char mess[150];
      snprintf( mess, sizeof(mess),FOXPNOISEERR,motor);
      W_Mess(mess,MSGBOX_YLOW,0,GENERIC_ALARM);
      return(1);
    }
  }
}

int FoxClass::LogOn(int motor)
{ int spare;
  if (!Fox->enabled)
    return(0);
  if((motor==BRUSH1 || motor==BRUSH2) && !disabled[motor])
    return(Fox->SendCmd(motor,FOX_LOGON,&spare,0,&rxval,0));
  return(0);
}

short int FoxClass::LogOff(int motor) //SMOD060503
{ int spare;

  if (!Fox->enabled)
    return(0);
    
  if((motor==BRUSH1 || motor==BRUSH2) && !disabled[motor])
  { Fox->SendCmd(motor,FOX_LOGOFF,&spare,0,&rxval,2);
    return((int)rxval);
  }
  return(0);
}

int FoxClass::LogReport(int motor,int index,int *buffer)
{
  if (!Fox->enabled)
    return(0);
  if((motor==BRUSH1 || motor==BRUSH2) && !disabled[motor])
    return(Fox->SendCmd(motor,FOX_LOGREPORT,(int *)&index,2,(int *)buffer,40,1));
  return(0);
}

/*
void FoxClass::LogGetBuf(int motor,int idxLimit,int bufLimit,int *encBuf,float *tickBuf,int &encmin,int &encmax,float &tickmin,float &tickmax,int &encLogIdx)
{
  int delta=0;

  encLogIdx=0;

  memset(encBuf,0,bufLimit*sizeof(int));
  memset(tickBuf,0,bufLimit*sizeof(float));

  encmin=5000;
  encmax=0;

  tickmin=999999;
  tickmax=-99999;

  unsigned short int logBuf[20];

  for(int i=0;i<idxLimit && encLogIdx<bufLimit;i+=10)
  {
    memset(logBuf,0,20);
    FoxHead->LogReport(motor,i,(int *)logBuf);

    for(int j=0;(j<20 && encLogIdx<bufLimit);j+=2)
    {
      if((i+j/2)==idxLimit)
        break;

      encBuf[encLogIdx]=logBuf[j+1];

      if((encLogIdx>0) && delta==0)
      {
        if(fabs(encBuf[encLogIdx-1]-encBuf[encLogIdx])>2048)
        {
          if((encBuf[encLogIdx-1]-encBuf[encLogIdx])>0)
          {
            delta=4096;
          }
          else
          {
            delta=-4096;
          }
        }
      }

      if(encmin>encBuf[encLogIdx])
        encmin=encBuf[encLogIdx];
      if(encmax<encBuf[encLogIdx])
        encmax=encBuf[encLogIdx];

      tickBuf[encLogIdx]=(logBuf[j]*409.6)/1000;
        
      if(tickmin>tickBuf[encLogIdx])
        tickmin=tickBuf[encLogIdx];

      if(tickmax<tickBuf[encLogIdx])
        tickmax=tickBuf[encLogIdx];

      encLogIdx++;
    }
  }
}
*/

void FoxClass::LogGraph(int motor,int idxLimit,int bufLimit,int ytick)
{
  int encLogIdx=0;
  unsigned short int logBuf[20];
  int *encBuf,*deltaEnc,delta=0;
  float *tickBuf;

  int logEncmin=5000;
  int logEncmax=0;

  float logTickmin=999999;
  float logTickmax=-99999;
  
  char message[80];
  int i,j;

  if (!Fox->enabled)
    return;

//  print_debug("limit=%d %d\n",idxLimit,bufLimit);

  if((motor==BRUSH1 || motor==BRUSH2) && !disabled[motor])
  {
    encBuf=new int[bufLimit];
    memset(encBuf,0,bufLimit*sizeof(int));

    tickBuf=new float[bufLimit];
    memset(tickBuf,0,bufLimit*sizeof(float));

    deltaEnc=new int[bufLimit];
    memset(tickBuf,0,bufLimit*sizeof(int));
    

    for(i=0;i<idxLimit && encLogIdx<bufLimit;i+=10)
    {
      memset(logBuf,0,20);
      FoxHead->LogReport(motor,i,(int *)logBuf);

      for(j=0;(j<20 && encLogIdx<bufLimit);j+=2)
      {
        if((i+j/2)==idxLimit)
          break;

        encBuf[encLogIdx]=logBuf[j+1];

        if((encLogIdx>0) && delta==0)
        {
          if(fabs(encBuf[encLogIdx-1]-encBuf[encLogIdx])>2048)
          {
            if((encBuf[encLogIdx-1]-encBuf[encLogIdx])>0)
            {
              delta=4096;
            }
            else
            {
              delta=-4096;
            }
          }
        }

        if(logEncmin>encBuf[encLogIdx])
          logEncmin=encBuf[encLogIdx];
        if(logEncmax<encBuf[encLogIdx])
          logEncmax=encBuf[encLogIdx];

        tickBuf[encLogIdx]=(logBuf[j]*409.6)/1000;

        if(logTickmin>tickBuf[encLogIdx])
          logTickmin=tickBuf[encLogIdx];
        if(logTickmax<tickBuf[encLogIdx])
          logTickmax=tickBuf[encLogIdx];

        if(i==0 && j==0)
          deltaEnc[encLogIdx]=0;
        else
          deltaEnc[encLogIdx]=encBuf[encLogIdx]-encBuf[0];

        encLogIdx++;
      }
    }

    delete[] deltaEnc;


    snprintf( message, sizeof(message),"Min=%d Max=%d",logEncmin,logEncmax);
    W_Mess(message);
    
    C_Graph *graph=new C_Graph(FOXENCREP_POS,"",GRAPH_NUMTYPEY_INT | GRAPH_NUMTYPEX_FLOAT | GRAPH_AXISTYPE_XY | GRAPH_DRAWTYPE_LINE | GRAPH_SHOWPOSTXT,1);

    graph->SetVMinY(logEncmin);
    graph->SetVMaxY(logEncmax);
    graph->SetVMinX(logTickmin);
    graph->SetVMaxX(logTickmax);
    graph->SetNData(encLogIdx,0);
    graph->SetTick(ytick);

    graph->SetDataY(encBuf,0);
    graph->SetDataX(tickBuf,0);

    graph->Show();

    int c;

    do
    {
      c=Handle();
      if(c!=K_ESC)
        graph->GestKey(c);
    } while(c!=K_ESC);

    delete graph;

    delete[] encBuf;
    delete[] tickBuf;
  }
}

#ifdef __FOX_DUMP
int FoxClass::Debug(int motor)
{ int spare;
  if (!Fox->enabled)
    return(0);

  return(Fox->SendCmd(motor,FOX_DEBUG,&spare,0,&rxval,0));
}

int FoxClass::GetDebug(int motor)
{
  if (!Fox->enabled)
    return(0);

  unsigned char buffer[40];
    
  int idx=0;
  Fox->SendCmd(motor,FOX_LOGREPORT,(int *)&idx,2,(int *)buffer,40,1);

  char _buff[256];
  struct ffblk ffblk;
  int nfile=0;

  strcpy(_buff,"FOXDUMP?");

  int done = findfirst(_buff,&ffblk,0);

  while (!done)
  {
    nfile++;
    done = findnext(&ffblk);
  }

  if(nfile>9)
    return(0);

  _buff[7]='0'+nfile;

  FILE *foxdump=FilesFunc_fopen(_buff,"wrt");


  for(int j=0;j<6;j++)
  {
    idx=j*10;
    Fox->SendCmd(motor,FOX_LOGREPORT,(int *)&idx,2,(int *)buffer,40,1);
    for(int i=0;i<40;i++)
    {
      fprintf(foxdump,"i=%3d %X\n",i+j*40,buffer[i]);
    }
  }

  FilesFunc_fclose(foxdump);

  bipbip();

  return(1);
}
#endif


FoxClass       *FoxHead;
FoxCommClass   *FoxPort;
