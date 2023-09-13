#ifndef __Q_NET_
#define __Q_NET_

#include "q_cost.h"
#include "datetime.h"

#define ISSERVEROK_NRETRY_MAX     3
#define ISSERVEROK_NRETRY_DELAY   150

// Rete non trovata
#define NONETMSG  MsgGetString(Msg_01307)

#define ENV_SERVERNAME "SERVERNAME"

#ifndef __NET_TEST
	#define REMOTEDISK         "/mnt/qserver"
#else
	#define REMOTEDISK         "backuptmp"
#endif

#define SHAREDIR           REMOTEDISK "/SHARED"

#define MACHINES_LIST_FILE       REMOTEDISK "/" STDFILENAME ".mls"

#define MACHINES_LIST_FILE_QMODE REMOTEDISK "/QLASER.MLS"

#define NET_BKPACCESS_ERR  MsgGetString(Msg_01641)

#define NETID_EXIST    MsgGetString(Msg_01654)

#define NETID_EXIST_WARN    MsgGetString(Msg_02006)

#define ASKDISABLENET      MsgGetString(Msg_01666)
#define NETDISABLED        MsgGetString(Msg_01667)

#define NETACCESS_ERR      MsgGetString(Msg_01985)

#define NETID_TXT_LEN		51

struct NetPar
{
	char NetID[NETID_TXT_LEN];
	int enabled;
	int executed;
	struct date last;
	unsigned char NetID_Idx;
};

int CheckNetID(void);

int NewNetMachineName(char *name);


int GetNetID_idx(void);

int CheckDB();

void DisableNet(void);
void EnableNet(void);
bool IsNetEnabled(void);
bool IsNetDisabledOnError(void);

void ForceNetEnabled(void);
void RemoveForceNetEnabled(void);

void ErrNetMsg(void);
int CheckMachineID();



extern struct NetPar nwpar;

#endif
