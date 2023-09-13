#ifndef __Q_FOXH_
#define __Q_FOXH_

#define __FOX11

#define __FOX_DUMP      //attiva funzione di dump scheda fox

#define MAXSTEPVAL      32767
//#define MAXVELVAL     4000000
#define MAXVELVAL       35000
#define MAXACCVAL       4000000
#define MAXTNOISEVAL    64000
#define MAXPNOISEVAL    64000

#define FOXV0 4882.8125

#define WATCHDOG_DELAY  10 //ritardo dopo l'invio del set watchdog

//SMOD240103
#define FOX_NRETRYMAX      5 //numero di tentativi invio di un singolo comando
#define FOX_COPYRXBUF 1 //in rx_buf copia i dati come sono stati ricevuti
#define FOX_CONVHEX   0 //converte in rx_buf i dati ricevuti da hex a binario
#define FOX_ERROROFF  2 //disattiva notifica errori
#define FOX_ERRORON   0 //attiva notifica errori

#define START   ' '
#define END     0x0D
#define NAKCAR  'N'

#define FOXDEBUG        0

#define FOX             0
#define BRUSH1          0
#define BRUSH2          2
#define STEP1           1
#define STEP2           3

//flag di rotazione continua x motori brushless
#define POS_DIR         +1
#define NEG_DIR         -1

//maschere per estrazione stato
#define M1M2_MASK        0x0C    //stato risoluzione stepper
#define CODABUSY_MASK    0x01    //stato coda libera(0)/piena(1)
#define DRIVER_MASK      0x02    //stato driver attiv./disatt.

//valori stato risoluzione stepper
#define STEP_RES8  0x0C // 1/8
#define STEP_RES4  0x08 // 1/4
#define STEP_RES2  0x04 // 1/2

//maschere per estrazione ingressi
#define HALL_BRUSH1  0x01
#define HALL_BRUSH2  0x02
#define REED_STAT    0x04
#define TRASL1_STAT  0x08
#define TRASL2_STAT  0x10

//bitfield uscite
#define DISPMOTOR  0x0001
#define DISPPRESS1 0x0002  //FP1



#ifndef __DISP2

#define DISPPRESS  0x0004

#define DISPMOVE   0x0008

#else

#define DISPPRESS2 0x0004

#define DISPMOVE2  0x0008

#endif

#define CAM_SELECT 0x0010
#define LIGHT2     0x0020
#define LIGHT1     0x0040
#define CPRS1      0x0080
#define CPRS2      0x0100
#define VACUO1     0x0200
#define VACUO2     0x0400
#define DISPMOVE1  0x0800   //RV1
#define DISPINV    0x1000
#define LED2       0x2000
#define ENC_SWITCH 0x8000

#define ENC2048_2P    0
#define ENC4096_2P    1
#define ENC4096_4P    3

//costanti riduzione della corrente
//corrente
#define I100m  0 //100mA
#define I200m  1 //200mA
#define I400m  2 //400mA
#define I600m  3 //600mA
#define I800m  4 //800mA
#define I1000m 5 //1A
#define I1200m 6 //1.2A
#define I1400m 7 //1.4A
//tempo
#define T150ms  0 //150ms
#define T300ms  1 //300ms
#define T600ms  2 //600ms
#define T1250ms 3 //1.25s
#define T2500ms 4 //2.5s
#define T5s     5 //5s
#define T10s    6 //10s
#define T20s    7 //20s

#define TEXTMODE 0
#define GRMODE   1

#include <stdio.h>
#include "commclass.h"
#include "timeprof.h"

#define MAXFOXBUF 256

class FoxCommClass
{ protected:
    unsigned int GetByte(int checksum_mode=0);
    int lastErr;
    int sender;
    unsigned char checksum;
    int errcode;
    int enableLog;
    CommClass *serial;
    unsigned char CalcChecksum(char *buf);
    FILE *foxlog;
    char _txbuf[MAXFOXBUF];
    char *last_rx;


  public:
    int enabled;
    int flag;
    CommClass* GetSerial() { return serial; }
    void Show_CommData(void);
    void Send(unsigned char address,unsigned char ndata,char cmd,unsigned char *data);
    int Receive(char *rxdata);
    int GetErr(void);
    int SendCmd(int addr,char cmd,int *data_val,int ntx,int *rx_data,int nrx,int mode=FOX_CONVHEX | FOX_ERRORON);
    void StartLog(int append=0);
    void EndLog(void);
    void PauseLog(void);
    void RestartLog(void);
    void PrintLog(const char *buf);

    FoxCommClass(void);
	FoxCommClass( CommClass* com_port );
    ~FoxCommClass(void);
};

class FoxClass
{ protected:
    int rxval;
    FoxCommClass *Fox;
    int disabled[4];
    int outval;
    struct _brushless
    { unsigned int prop;
      unsigned int integral;
      unsigned int deriv;
      unsigned int clipint;
    } brushless;

    int lastRotPos[2];
    
    int SetMotorOn_Off(int motor,int status);

  public:

    int SetZero(int motor);
    int MoveAbs(int motor,int pos);
    int MoveRel(int motor,int step);
    short int ReadPos(int motor); //SMOD060503
    int SpaceStop(int motor,int step);
    int Rotate(int motor,int dir);
    short int ReadEncoder(int motor); //SMOD060503
    short int ReadBrushPosition(int motor); //SMOD060503
    int GetLastRotPos(int motor);
    unsigned char ReadStatus(int motor); //SMOD060503

    int SetIntegral(int motor,unsigned int integral);
    int SetDeriv(int motor,unsigned int integral);
    int SetProp(int motor,unsigned int integral);
    int SetClipInt(int motor,unsigned int integral);
    int SetPID(int motor,unsigned int prop,unsigned int integral,unsigned int deriv,unsigned int clipint);
    int SwitchEncoder(int motor);
    int SetEncoderType(int motor,int mode);
    int MotorEnable(int motor);
    int MotorDisable(int motor);
    int MotorDisAll(void);
    int SetMaxCurrent(int motor,int current,int timer);
    float GetCurrent(int motor);

    int SetAcc(int motor,int acc);
    int SetVMin(int motor,int vmin);
    int SetVMax(int motor,int vmax);

    int SetOutput(int address,int mask);
    int ClearOutput(int address,int mask);
    unsigned char ReadInput(int address); //SMOD060503
    int ReadADC(short int &chA,short int &chB);

    int ForceLowCurrent(int motor,char state);
    int SetLowCurrent_Timer(int motor,float time);
    int Reset(void);

    //SMOD240103-SMOD060503
    short int GetVersion(int errmode=FOX_ERRORON);

    int Crash(void);
    int ArmWatchdog(void);

    void Disable(void);
    void Disable(int addr);
    void Enable(void);
    void Enable(int addr);

    int  IsEnabled(int addr);
    bool IsEnabled() { return Fox->enabled; }

    int SetTorqueNoise(int motor,int val);
    int SetPositionNoise(int motor,int val);

    int GetErr(void);
    int RxData(void);

    int  LogOn(int motor);
    short int  LogOff(int motor); //SMOD060503
    int  LogReport(int motor,int index,int *buffer);
    void LogGraph(int motor,int idxLimit,int bufLimit,int ytick);

#ifdef __FOX_DUMP
    int GetDebug(int motor);
    int Debug(int motor);
#endif
	
	const char* GetPort(void);

    FoxClass(FoxCommClass *comm);

    //void LogGetBuf(int motor,int idxLimit,int bufLimit,int *encBuf,float *tickBuf,int &encmin,int &encmax,float &tickmin,float &tickmax,int &ndata);
    //void LogShow(int encBuf[][265],float tickBuf[][265],int nlogs,int *ndata,int logEncmin,int logEncmax,float logTickmin,float logTickmax);
};

extern FoxClass       *FoxHead;  //classe di gestione scheda Fox-testa
extern FoxCommClass   *FoxPort;  //classe di comunicazione protocollo Fox


#endif
