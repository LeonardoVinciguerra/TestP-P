#include <list>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "q_cost.h"
#include "msglist.h"
#include "strutils.h"
#include "q_help.h"
#include "q_tabe.h"
#include "q_wind.h"
#include "q_net.h"
#include "q_files.h"
#include "q_init.h"
#include "q_graph.h"
#include "filefn.h"
#include "fileutils.h"
#include "q_filest.h"
#include "q_conf_new.h"

#include "lnxdefs.h"

#include <mss.h>


using namespace std;

struct NetPar nwpar;

bool netDisabledOnErr=false;

void ErrNetMsg(void)
{
	bipbip();
	W_Mess(NETACCESS_ERR);

	bipbip();
	W_Mess(NETDISABLED);
}


int NewNetMachineName(char *name)
{
  if(!IsNetEnabled())
  {
    return(1);
  }

  char buf[MAXNPATH];
  strcpy(buf,REMOTEDISK "/" REMOTEBKP_DIR "/");
  strcat(buf,nwpar.NetID);

    int s=SearchMachineName(name);

    if(s==-1)
    {
      return(-1);
    }

	if(CheckDirectory(buf) || (s!=0))
    {
      bipbip();
      if(!W_Deci(0,NETID_EXIST))
      {
        return(0);
      }
      else
      {
        char tbuf[250];
        DelSpcR(name);
        sprintf(tbuf,NETID_EXIST_WARN,name);
        if(!W_Deci(0,tbuf))
        {
          return(0);
        }
      }
    }

	if(!CheckDirectory(buf))
    {
      if(mkdir(buf,DIR_CREATION_FLAG))
      {
        return(-1);
      }
    }

    if(s==0)
    {
      if(!AddMachineName(name))
      {
        return(-1);
      }

      return((unsigned char)GetLastMachineID());
    }
    else
    {
      return(s);
    }
}

int CheckNetID(void)
{
  if(IsNetEnabled())
  {
    if(!GetNetID_idx())
    {
	  ErrNetMsg();
	  DisableNet();
	  return(0);
    }
  }

  return(1);
}


int GetNetID_idx(void)
{
  int s=SearchMachineName(nwpar.NetID);

  //errore
  if(s==-1)
  {
    return(0);
  }

  //non esiste
  if(s==0)
  {
    if(!AddMachineName(nwpar.NetID))
    {
      return(0);
    }
    else
    {
      return((unsigned char)GetLastMachineID());
    }
  }

  //ok: trovato
  nwpar.NetID_Idx=s;
  WriteNetPar(nwpar);
  
  return(s);
}

void DisableNet(void)
{
	netDisabledOnErr=true;
}

void EnableNet(void)
{
	netDisabledOnErr=false;
}

bool forcedNetEnable=false;

//da usare solo per riattivare temporaneamente la rete prima di
//disattivarla definitivamente
void ForceNetEnabled(void)
{
	forcedNetEnable=true;
}

void RemoveForceNetEnabled(void)
{
	forcedNetEnable=false;
}

bool IsNetDisabledOnError(void)
{
	return(netDisabledOnErr);
}

//NOTA: se la rete viene disabilitata la modifica avviene subito
//se era disattivata un'ulteriore attivazione non verra' considerata fino
//al riavvio del software

bool IsNetEnabled(void)
{
	static bool first_check = true;
	static bool enabled = false;

	if(forcedNetEnable)
	{
		return(true);
	}

	if(first_check)
	{
		if(nwpar.enabled && !netDisabledOnErr)
		{
			enabled = true;
		}
		first_check = false;
	}
	else
	{
		if(netDisabledOnErr)
		{
			enabled = false;
		}
	}

	return(enabled);
}


int CheckMachineID()
{
	while(strlen(nwpar.NetID)==0)
	{
		bipbip();
		W_Mess(NO_MACHINE_ID);

		fn_MachineID();
	}

	return !CheckNetID() ? 0 : 1;
}
